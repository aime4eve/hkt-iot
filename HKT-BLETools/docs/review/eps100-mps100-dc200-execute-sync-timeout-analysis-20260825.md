# EPS100/MPS100/DC200 Execute 与 SYNC timeout 原因核查

> 核查日期：2026-08-25
> 蓝牙工具源码：`HKT-BLETools @ 85d073c`（协议文件无本地未提交改动）
> 地磁固件源码：`HKT-Firmwares/LoRaWAN_ParkingSensor @ 4c6c432`
> 设备范围：EPS100、MPS100、DC200。App 侧均解析为 `NAME_DC200`，固件侧共用 ParkingSensor 协议实现。
> 结论状态：静态代码核查结果，供研发确认；Execute 的完成阶段仍需要真机收包或固件日志最终确认。

---

## 1. 问题概述

现场现象：

- 在 EPS100/MPS100/DC200 面板点击 `Execute` 后，App 最终提示 `timeout`。
- 点击 `SYNC` 后，App 最终提示 `timeout`。
- 界面状态同步本身正常，说明 BLE 连接和周期状态查询基本可用。

初步结论：

| 功能 | 结论 | 置信度 |
|------|------|--------|
| SYNC timeout | 固件接收分支顺序吞掉 `0x06` 时间同步命令，真正同步分支不可达；App 侧 `0x06` 帧长度字段也写错 | 高，源码可直接证明 |
| Execute timeout | 初始 `0xFD` 命令和 `0xFF ACK` 路径源码上是对的；timeout 更可能发生在校准完成通知 `"Calibration Done"` 未发送、未收到，或校准流程未结束 | 中，需要真机确认 |

---

## 2. 协议帧格式与双方实现

### 2.1 App 下行帧格式

App 在 `Communicate.kt::streamDevice()` 中构造下行帧：

```text
68 6B 74 | packNum(1) | len(2) | cmd(1) | data(n) | crc(2)
```

因此字节偏移为：

| 偏移 | 含义 |
|------|------|
| `0..2` | 同步头 `68 6B 74` |
| `3` | 包序号 |
| `4..5` | `cmd + data` 长度，大端 |
| `6` | 命令字节 |
| `7...` | 数据 |

固件当前用 `data[5]` 判断长度低字节，用 `data[6]` 判断命令字节。对这些短帧可以工作，但没有完整解析 16 位长度。

### 2.2 固件上行状态/ACK 帧格式

固件在 `callback_BLEAck()`、`callback_BLEQuery()` 中构造上行帧：

```text
68 6B 74 | 00 | packNum(1) | TLV data...
```

其中 ACK TLV 为：

```text
FF 00
```

App 解析 `0xFF` 后推进状态机。该部分与固件当前输出一致。

---

## 3. Execute（校准）链路核查

### 3.1 App 侧期望流程

源码位置：

- `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`
- `app/src/main/java/com/hkt/ble/bletools/Communicate.kt`

状态链路：

```text
用户点击 Execute
  -> CALIBRATION_EVENT
  -> CALIBRATION_START_EVENT
  -> App 发送 0xFD
  -> 固件返回 0xFF ACK
  -> App 进入 CALIBRATION_EXECUTE_EVENT
  -> 固件校准完成
  -> 固件发送 ASCII "Calibration Done"
  -> App 进入 CALIBRATION_FINISH_EVENT
  -> 界面提示 Executed successfully
```

关键代码：

```kotlin
// Communicate.kt: StreamThread
if (mDeviceEvent.event == DeviceEventEnum.CALIBRATION_START_EVENT.ordinal) {
    BleHelper.sendCommand(device, streamDevice(0xFD, 0), false)
}
```

```kotlin
// Communicate.kt: streamRev，收到 0xFF ACK
if (mDeviceEvent.event == DeviceEventEnum.CALIBRATION_START_EVENT.ordinal) {
    mDeviceEvent.event = DeviceEventEnum.CALIBRATION_EXECUTE_EVENT.ordinal
}
```

```kotlin
// Communicate.kt: streamRev，收到校准完成字符串
if (content.contains("43616C6962726174696F6E20446F6E65")) {
    if (mDeviceEvent.event == DeviceEventEnum.CALIBRATION_EXECUTE_EVENT.ordinal) {
        mDeviceEvent.event = DeviceEventEnum.CALIBRATION_FINISH_EVENT.ordinal
    }
}
```

### 3.2 App 下行 `0xFD` 帧

`streamDevice(0xFD, 0)` 当前输出结构：

```text
68 6B 74 00 00 05 FD 00 00 00 00 <CRC16>
```

固件宽泛入口条件为：

```c
strstr((const char *)data, "hkt") && (data[5] == 5 || data[5] == 4)
```

`0xFD` 帧 `data[5] == 5`、`data[6] == 0xFD`，可以进入命令分支。

固件处理：

```c
else if (data[6] == 0xFD) {
    magnetometer_enter_cal();
    PWR_CTRL_H;
    DEBUG_TRACE(LOG_TAG, "Recv BLE MAG Reference CMD");
    callback_BLEAck();
}
```

因此，从静态代码看：

1. App 会发出 `0xFD`。
2. 固件能识别该命令。
3. 固件会立即返回 `0xFF ACK`。
4. App 收到 ACK 后能进入 `CALIBRATION_EXECUTE_EVENT`。

如果 Debug 页能看到 `0xFD` 下行和 `FF 00` 上行，但最后仍 timeout，问题应集中在后续校准完成通知，而不是初始命令或 ACK。

### 3.3 固件校准完成通知

固件校准完成时执行：

```c
if (device_t.ble_connected) {
    BLE_SendCMD("Calibration Done");
}
```

App 期望收到该 ASCII 字符串的 hex 形式：

```text
43616C6962726174696F6E20446F6E65
```

潜在问题：

1. `device_t.ble_connected` 状态未正确置位时，校准可能已经完成，但不会发送完成通知。
2. `"Calibration Done"` 是裸 ASCII 通知，不是带命令号和状态的二进制协议帧，健壮性不足。
3. 校准流程没有显式的“开始 ACK / 进行中 / 完成 / 失败”状态模型。
4. App 只认完整字符串，若 BLE 模块分包、拼接或转义方式不同，也可能匹配失败；当前代码未做跨分包缓冲。

### 3.4 固件校准算法风险

`qmc5883l.c::magnetometer_calibration()` 中存在未初始化变量问题：

```c
axis_info_t raw;

if (!started) {
    started = 1;
    over_time = get_syspant_ms();

    max.x = raw.x;
    max.y = raw.y;
    max.z = raw.z;
    min.x = raw.x;
    min.y = raw.y;
    min.z = raw.z;
}
```

`raw` 在第一次读取磁力计数据前就被用来初始化 `max/min`，这是未定义行为。

完成条件：

```c
if (!check_delay_expired(over_time, MAX_MAGNETOMETER_CALIBRATION_TIME))
    return;
```

当前：

```c
#define MAX_MAGNETOMETER_CALIBRATION_TIME 5000 // 5000ms
```

同时，每次出现新的 XYZ 极值都会刷新 `over_time`。这意味着完成时间取决于磁场数据变化，不是固定 90 秒。

另有一个前置条件：

```c
void magnetometer_work_process(void) {
    if (!device_t.power_on)
        return;

    switch (qmc5883l_t.state) {
        case MAGNETOMETER_IN_CALIBRATION:
            magnetometer_calibration();
            break;
    }
}
```

如果固件内部 `power_on` 与 App 显示状态不同步，即使收到了 `0xFD` 并返回 ACK，校准状态机也不会执行。

### 3.5 Execute 结论

初始命令与 ACK 路径源码上成立，不能仅凭 timeout 判断固件完全没有收到 `0xFD`。

Execute timeout 需要重点确认三件事：

1. App 是否收到 `FF 00` ACK。
2. 固件是否打印并执行 `Start Magnetometer Calibration` 到 `End Magnetometer Calibration`。
3. App 是否收到 `43616C6962726174696F6E20446F6E65`。

若第 1 步缺失，查 `0xFD` 下行和固件入口；若第 1 步存在但第 3 步缺失，查校准状态机、`power_on`、`ble_connected` 和完成通知；若第 3 步存在但 App timeout，查 App 字符串匹配和分包处理。

---

## 4. SYNC（时间同步）链路核查

### 4.1 App 下行 `0x06` 帧

App 构造 `0x06` 时间同步帧：

```kotlin
dataLenString = String.format("%0${4}X", 4)
cmdString = String.format("%0${2}X", cmd)
dataString = String.format("%0${8}X", mDeviceEvent.Timestamp)
```

实际帧结构：

```text
68 6B 74 00 00 04 06 <timestamp 4 bytes> <CRC16>
```

这里存在 App 侧协议错误：`len` 表示 `cmd + data` 长度，应为 `1 + 4 = 5`，当前写成了 `4`。

### 4.2 固件分支顺序问题

固件先进入宽泛分支：

```c
else if (strstr((const char *)data, "hkt") && (data[5] == 5 || data[5] == 4)) {
    // 只处理 FF / 07 / 08 / 76 / FC / 09 / 0A / FE / FD / 02
}
```

当前 App 的 `0x06` 帧 `data[5] == 4`，所以会被这个宽泛分支命中。

但该宽泛分支内部没有 `data[6] == 0x06` 的处理逻辑。

真正的时间同步分支在后面：

```c
else if (strstr((const char *)data, "hkt") && data[5] == 4 && data[6] == 6) {
    // 设置时间并 callback_BLEAck()
}
```

由于前面的宽泛分支已经命中，这个真正的时间同步分支永远不可达。

因此 SYNC timeout 的直接原因是：

```text
App 发送 0x06
  -> 固件进入 data[5] == 4 的宽泛分支
  -> 宽泛分支没有 0x06 handler
  -> 不设置时间
  -> 不返回 ACK
  -> App 等待超时
```

### 4.3 可能的 OTA 误路由

宽泛分支末尾还有：

```c
else if (data[7] == 1) {
    FLASH_Write_Update_Flag();
    mDelay(100);
    NVIC_SystemReset();
}
```

对于 `0x06` 帧，`data[7]` 是时间戳最高字节。如果时间戳最高字节等于 `0x01`，SYNC 命令会被误判为 OTA 更新指令并触发复位。

当前 2026 年附近的 Unix timestamp 最高字节通常是 `0x0A`，日常测试大概率不会触发 OTA 误路由，只会因为无 handler 而 timeout。但这是一个必须修复的协议缺陷。

### 4.4 SYNC 结论

SYNC timeout 不是等待时间不足，而是协议解析不一致：

1. App 侧 `0x06` 帧 `len` 字段错误：应为 `0005`，当前为 `0004`。
2. 固件侧宽泛分支优先级错误，导致真正的 `0x06` 分支不可达。
3. 固件宽泛分支还存在把时间戳首字节 `0x01` 误判为 OTA 的风险。

只放大 App timeout 或只调整固件等待时间都无法解决该问题。

---

## 5. 建议的修复方案

### 5.1 固件协议解析重构

建议不要再依赖多层 `strstr + data[5] + data[6]` 的 else-if 顺序判断，改为显式解析帧头、长度和命令：

```c
if (len >= 7 && memcmp(data, SYNC_HEAD, 3) == 0) {
    uint16_t payload_len = ((uint16_t)data[4] << 8) | data[5];
    uint8_t cmd = data[6];

    switch (cmd) {
        case 0xFF: /* query */
            break;
        case 0xFE: /* power */
            break;
        case 0xFD: /* calibration */
            break;
        case 0x02: /* config */
            break;
        case 0x06: /* timestamp */
            break;
        default:
            break;
    }
}
```

同时应校验：

- 帧长度是否足够。
- `payload_len` 是否与实际接收长度一致。
- CRC 是否正确。
- 特定命令的数据长度是否符合协议。

### 5.2 SYNC 兼容策略

建议双方同时修正：

1. App 将 `0x06` 的 `len` 从 `4` 改为 `5`。
2. 固件按 `cmd == 0x06` 优先处理时间同步，不再让它落入宽泛命令分支。
3. 过渡期固件可兼容 `len == 4` 和 `len == 5` 两种旧 App 帧，但必须基于 `cmd == 0x06` 判断，而不是继续依赖 `data[5] == 4`。

最小验证：

```text
App 0x06 下行
  -> 固件日志 Recv BLE Sync Time
  -> 固件上行 FF 00 ACK
  -> App 提示 Executed successfully
```

### 5.3 Execute 完成通知方案

短期最小修复：

1. 修正 `raw` 未初始化问题。
2. 确认 `device_t.power_on` 为真时校准状态机可运行。
3. 确认 `device_t.ble_connected` 在 BLE 连接期间稳定为真。
4. 保留 `"Calibration Done"`，先让当前 App 可以完成状态闭环。

中长期协议化方案：

1. 固件发送带命令号和状态的二进制完成帧，例如 `FD 00` 表示成功、`FD 01` 表示失败。
2. App 的 `0xFD` 状态机同时兼容旧的 ASCII `"Calibration Done"` 和新的二进制完成帧。
3. 校准过程增加明确的开始、进行中、完成、失败状态，避免用 timeout 作为唯一失败出口。

---

## 6. 研发确认清单

### 6.1 基础信息确认

请研发先确认测试设备实际固件版本：

```text
App 状态页 Version = ?
固件构建目标 = EPS100 / MPS100 / DC200?
SOFTWARE_VER = ?
```

当前源码配置为：

```text
HARDWARE_VER = 0x0B
PROCT_NAME = "MPS100"
SOFTWARE_VER = 0x1C
```

即源码对应当前配置显示为 `V10.28`。如果设备不是该版本，需要按实际固件源码或固件包重新比对。

### 6.2 SYNC 抓包/日志确认

预期当前现象：

| 步骤 | 预期结果 |
|------|-----------|
| App 点击 SYNC | 下行 `68 6B 74 00 00 04 06 ...` |
| 固件宽泛分支 | 命中 `data[5] == 4` |
| 固件 `0x06` 专属分支 | 不执行 |
| 固件日志 | 无 `Recv BLE Sync Time` |
| 固件 ACK | 无 `FF 00` |
| App | timeout |

### 6.3 Execute 抓包/日志确认

请按下表分段记录：

| 阶段 | App/固件证据 | 判断 |
|------|--------------|------|
| 点击 Execute | App 下行 `68 6B 74 00 00 05 FD 00 00 00 00 ...` | 确认命令已发送 |
| 初始 ACK | 固件上行 `68 6B 74 00 <seq> FF 00` | 确认固件已收到并进入校准 |
| 固件日志 | `Recv BLE MAG Reference CMD` | 确认进入 `0xFD` handler |
| 校准开始 | `Start Magnetometer Calibration` | 确认状态机运行 |
| 校准结束 | `End Magnetometer Calibration` / `Calibration Done` 日志 | 确认算法完成 |
| 完成通知 | App 收到 `43616C6962726174696F6E20446F6E65` | 确认 BLE 通知可达 |
| App 状态 | `Executed successfully` | 闭环 |

优先补充缺失环节的日志或抓包，即可定位 Execute 的具体断点。

---

## 7. 最终判断

SYNC 的问题可以直接定性为固件解析分支与 App 协议实现不一致，且 App 侧长度字段也有错误，建议优先修复。

Execute 的初始命令和 ACK 在源码上匹配，timeout 更可能发生在校准完成阶段。研发需要用上述日志和收包证据确认是校准算法未结束、固件完成通知未发送、BLE 通知丢失，还是 App 未识别完成通知。

不建议继续通过单纯放大 timeout 掩盖该问题；应先修正协议解析和状态反馈，再验证真实耗时。

---

## 8. 源码索引

| 主题 | 文件 | 关键位置 |
|------|------|----------|
| App Execute 触发 | `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt` | 340, 1252-1255 |
| App SYNC 触发 | `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt` | 372, 1267-1270 |
| App 命令重发线程 | `app/src/main/java/com/hkt/ble/bletools/Communicate.kt` | 216-242 |
| App `0xFD` 组帧 | `app/src/main/java/com/hkt/ble/bletools/Communicate.kt` | 284-289 |
| App `0x06` 组帧 | `app/src/main/java/com/hkt/ble/bletools/Communicate.kt` | 364-372 |
| App `0xFF ACK` 状态推进 | `app/src/main/java/com/hkt/ble/bletools/Communicate.kt` | 697-723 |
| App 校准完成字符串匹配 | `app/src/main/java/com/hkt/ble/bletools/Communicate.kt` | 729-732 |
| 固件 ACK 组帧 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/communicate.c` | 537-558 |
| 固件宽泛命令入口 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/communicate.c` | 899 |
| 固件 `0xFD` 处理 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/communicate.c` | 952-956 |
| 固件不可达的 `0x06` 分支 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/communicate.c` | 987-1004 |
| 固件校准算法 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/qmc5883l.c` | 100-258 |
| 固件校准完成通知 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/qmc5883l.c` | 253-255 |
| 固件校准状态机入口 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/qmc5883l.c` | 648-670 |
| 校准 5 秒稳定期配置 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/include/qmc5883l.h` | 62 |
