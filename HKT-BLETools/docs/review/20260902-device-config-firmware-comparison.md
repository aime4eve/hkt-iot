# 设备配置界面与固件协议对比确认报告

日期：2026-09-02  
App 分支：`nix/device-config-ux`  
审计方式：静态代码逐项对照  
结论：核对中发现 4 处偏差，已修正；修正后，配置协议、字段顺序、取值范围和 UI 映射与固件代码一致。BLE 真机回归仍需执行。

## 1. 审计范围

本报告覆盖以下配置与状态功能：

| 设备 | App 配置入口 | 对应固件 |
| --- | --- | --- |
| UDS100 垃圾满溢传感器 | UDS 参数页 | `HKT-Firmwares/LoRaWAN_Ultrasonic_Distance_Sensor/USER/Drive/communicate.c` |
| MPS100 / DC200 车位传感器 | DC200 参数页 | `HKT-Firmwares/LoRaWAN_ParkingSensor/USER/Drive/communicate.c`、`control_center.c` |
| SVC100 电磁阀控制器 | SVC 参数页、实时任务页、定时任务页 | `HKT-Firmwares/LoRaWAN_Solenoid_Valve_Controller/USER/Drive/communicate.c`、`control_center.h` |

App 侧核心代码：

- `app/src/main/java/com/hkt/ble/bletools/Communicate.kt`
- `app/src/main/java/com/hkt/ble/bletools/DeviceActivity.kt`
- `app/src/main/res/layout/dialog_config_uds100.xml`
- `app/src/main/res/layout/dialog_config_dc200.xml`
- `app/src/main/res/layout/dialog_config_svc100.xml`
- `app/src/main/res/layout/dialog_config_realtime_task.xml`
- `app/src/main/res/layout/dialog_config_timed_task.xml`

## 2. 本次核对发现并已修正的偏差

| 编号 | 设备 | 问题 | 固件定义 | 修正结果 |
| --- | --- | --- | --- | --- |
| F-01 | UDS100 | 上报周期校验误允许 `0` | `0x02` 配置中上报周期只接受 `1-1440` | 校验恢复为 `1-1440` |
| F-02 | UDS100 | 满溢状态只区分 `0` 和 `1`，且把 `1` 显示为 `Invalid` | `0=正常`，`1=低于低阈值`，`2=高于高阈值`，`0xFF=测量无效` | 四种状态分别显示为 `Normal / Low threshold triggered / High threshold triggered / Invalid` |
| F-03 | SVC100 | `0x41` 中 `0x82` 的端口文案互换 | `bit0=0` 表示端口 1 脉冲，`bit1=1` 表示端口 2 开关状态；`0x80` 表示启用稳定时间 | `0x82` 显示为 `Port 1 pulse, Port 2 switch (stable time)` |
| F-04 | SVC100 | `0x8A` 时区状态显示会生成 `UTC+-8` 这类错误文本 | 固件编码为 `0-12` 东区、`13-24` 西区、`25=UTC+03:30`、`26=UTC+05:30` | 状态显示直接复用与配置页相同的时区数组 |

## 3. UDS100 对比确认

### 3.1 配置命令 `0x02`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `9`，含义为 `cmd(1) + payload(8)` | `data[6] == 2 && data[5] == 9` | 一致 |
| 字段顺序 | `report(2) + gps(2) + low(2) + high(2)` | `data[7..8]`、`data[9..10]`、`data[11..12]`、`data[13..14]` | 一致 |
| 上报周期 | `1-1440` 分钟 | `1-1440` 分钟 | 一致 |
| GPS 周期 | `0` 或 `10-1440` 分钟 | `0` 或 `10-1440` 分钟；`0` 不定位 | 一致 |
| 低阈值 | `30-4500 mm` | `30-4500 mm` | 一致 |
| 高阈值 | `0` 或 `30-4500 mm` | `0` 或 `30-4500 mm`；`0` 表示不判断高阈值 | 一致 |
| 提交控制 | 任一字段非法时不发送 `0x02`，并在对应输入框标错 | 固件逐字段校验；低阈值非法会提前返回且不 ACK | 一致，且 App 已阻止非法帧 |

### 3.2 状态与显示

| 协议类型 | 固件状态值 | App 显示 | 结论 |
| --- | --- | --- | --- |
| `0x45` GPS 周期 | `u16`，单位分钟 | 数值 + `min` | 一致 |
| `0x46` 超声波距离 | `u16`，单位 mm | 数值 + `mm` | 一致 |
| `0x47` 满溢状态 | `0/1/2/0xFF` | `Normal / Low threshold triggered / High threshold triggered / Invalid` | 已修正，一致 |
| `0x48` 阈值回读 | `low(2) + high(2)` | `low(2) + high(2)` | 一致 |

## 4. MPS100 / DC200 对比确认

### 4.1 配置命令 `0x02`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `4`，含义为 `cmd(1) + payload(3)` | `data[6] == 2 && data[5] == 4` | 一致 |
| 字段顺序 | `report(2) + parkMode(1)` | `data[7..8]` 为上报周期，`data[9]` 为模式 | 一致 |
| 上报周期 | App 限制 `1-1440` 分钟 | 固件接受 `0` 或 `1-1440` 分钟 | App 为安全子集；未暴露固件允许的 `0`，不产生协议错误 |
| 检测模式 | `0/1/2` | 固件只接受 `0/1/2` | 一致 |
| 保存动作 | 周期和模式一次提交 | 固件一次解析并分别写 Flash | 一致 |

### 4.2 模式与状态映射

| 固件值 | 固件含义 | App 显示 / 选项 | 结论 |
| --- | --- | --- | --- |
| `park_mode = 0` | 融合模式 | `Magnetic + Radar` | 一致 |
| `park_mode = 1` | 仅磁力计 | `Magnetic Only` | 一致 |
| `park_mode = 2` | 雷达优先 | `Radar Priority` | 一致 |
| `park_state = 0` | 无车 | `Vacant` | 一致 |
| `park_state = 1` | 有车 | `Occupied` | 一致 |
| `park_state = 0xFF` | 异常 / 被覆盖 | `Covered` | 一致 |

## 5. SVC100 对比确认

### 5.1 通用配置命令 `0x02`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `8`，含义为 `cmd(1) + payload(7)` | `data[5] == 8 && data[6] == 2` | 一致 |
| 字段顺序 | `volOut + portMode + stableTime + autoPower + timezone + report(2)` | `data[7]` 至 `data[13]` 按同一顺序解析 | 一致 |
| 输出电压 | Spinner 位置 `0/1/2` 映射 `12V/9V/5V` | `vol_control_sw = 0/1/2` 对应 `12V/9V/5V` | 一致 |
| 端口功能 | Spinner 位置映射 `0x00/0x01/0x02/0x03/0x81/0x82/0x83` | `port_mode` 低 2 位选择端口模式，`0x80` 启用稳定时间 | 一致 |
| 稳定时间 | `1-255` 秒；非 `0x8x` 模式下禁用输入但仍保留原值下发 | 字段为 `u8`，只在 `0x8x` 稳定时间模式中使用 | 一致 |
| 电源模式 | `0=Manual power on/off`，`1=Automatic power on/off` | `power_mode = 0/1` | 一致 |
| 时区编码 | 数组索引 `0-26` 直接作为协议值 | `0-12` 东区，`13-24` 西区，`25=UTC+03:30`，`26=UTC+05:30` | 一致 |
| 上报周期 | `0` 或 `1-1440` 分钟 | `(1-1440) || 0` | 一致 |

### 5.2 端口功能映射

| 协议值 | 固件行为 | App 选项 | 结论 |
| --- | --- | --- | --- |
| `0x00` | 端口 1 脉冲，端口 2 脉冲 | `Port 1 + Port 2 pulse` | 一致 |
| `0x01` | 端口 1 开关状态，端口 2 脉冲 | `Port 1 switch, Port 2 pulse` | 一致 |
| `0x02` | 端口 1 脉冲，端口 2 开关状态 | `Port 1 pulse, Port 2 switch` | 一致 |
| `0x03` | 端口 1 开关状态，端口 2 开关状态 | `Port 1 + Port 2 switch` | 一致 |
| `0x81` | 端口 1 开关状态，端口 2 脉冲；启用稳定时间 | `Port 1 switch, Port 2 pulse (stable time)` | 一致 |
| `0x82` | 端口 1 脉冲，端口 2 开关状态；启用稳定时间 | `Port 1 pulse, Port 2 switch (stable time)` | 已修正，一致 |
| `0x83` | 端口 1 开关状态，端口 2 开关状态；启用稳定时间 | `Port 1 + Port 2 switch (stable time)` | 一致 |

### 5.3 实时任务命令 `0x03`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `7` | `data[5] == 7 && data[6] == 3` | 一致 |
| 阀门选择 | Spinner `0/1/2` 显示 `1&2 / 1 / 2` | 固件 `0=双阀`，`1=阀1`，`2=阀2` | 一致 |
| 初始状态 | Spinner `0/1` 显示 `Close / Open` | 固件 `0` 执行回关，`1` 执行开启 | 一致 |
| 持续时间 | `u16`，秒 | 固件读取 `u16`，秒 | 一致 |
| 脉冲数 | App 限制 `0-65535`，发送 `u16` | 固件结构体为 `u16` | 功能一致 |

说明：固件 `0x03` 解析表达式读取了 3 字节脉冲，但目标字段 `real_time_task.pulse_count` 是 `u16`，第 17 位以上会被截断。App 明确限制为 16 位，行为正确。

### 5.4 定时任务命令 `0x04`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `11` | `data[5] == 11 && data[6] == 4` | 一致 |
| 任务号 | Spinner 前 16 项发送 `1-16`；保存时禁止 `FF` | 固件保存任务号只接受 `1-16` | 一致 |
| 阀门选择 | 发送 `0/1/2` | 固件只接受 `<=2` | 一致 |
| 初始状态 | 发送 `0/1` | 固件只接受 `<=1` | 一致 |
| 脉冲数 | `0-65535`，`u16` | `u16` | 一致 |
| 开始/结束时间 | 时间选择器写入分钟数，`u16`；结束必须晚于开始 | 固件还原为小时和分钟，并检查 `0-23`、`0-59` | 一致 |
| 重复位 | 周一到周日映射 `bit0-bit6` | 固件按 `bit0-bit6` 判断周一到周日 | 一致 |

### 5.5 删除定时任务命令 `0x05`

| 项 | App 行为 | 固件行为 | 结论 |
| --- | --- | --- | --- |
| 长度字段 | `2` | `data[5] == 2 && data[6] == 5` | 一致 |
| 单任务 | Spinner 前 16 项映射 `1-16` | 固件删除 `1-16` | 一致 |
| 全部删除 | `FF` 项映射 `0xFF` | 固件支持 `0xFF` 全清 | 一致 |

### 5.6 SVC 状态显示

| 协议类型 | 固件字段 | App 显示 | 结论 |
| --- | --- | --- | --- |
| `0x3C` 设备状态 | `valve1 + insert1 + pulse1(2) + valve2 + insert2 + pulse2(2)` | 同顺序解析为 `Open/Close`、`Connect/Disconnect`、脉冲数 | 一致 |
| `0x40` 输出电压 | `0/1/2` | `12V/9V/5V` | 一致 |
| `0x41` 端口功能 | `port_mode` 原值 | 复用端口功能数组显示 | 已修正，一致 |
| `0x42` 稳定时间 | `stable_time`，秒 | 数值显示 | 一致 |
| `0x43` 电源模式 | `0/1` | `Manual/Auto` | 一致 |
| `0x8A` 时区 | `0-26` 编码 | 复用时区数组显示 | 已修正，一致 |

## 6. 交互与语言口径

| 项 | 当前结果 | 结论 |
| --- | --- | --- |
| 默认语言 | App 启动时通过 `AppCompatDelegate.setApplicationLocales()` 固定为 `en-US` | 符合当前“默认英文”要求 |
| 配置按钮 | UDS / DC200 / SVC 参数页和 SVC 任务页统一为 `Save Settings` | 已消除 `Config 1/2/3/4/5` 的不可读命名 |
| 输入错误 | 关键输入框标错，非法值不触发 BLE 配置命令 | 已修复 UDS 原有反向判断问题 |
| i18n | 默认英文已保证；中英文全量覆盖仍是遗留任务 | 记录在 `Readme.md` 当前限制中 |

## 7. 验证结果

| 验证项 | 结果 |
| --- | --- |
| `assembleDevDebug` | 通过 |
| `testDevDebugUnitTest` | 通过 |
| `git diff --check` | 通过 |
| 静态协议字段对照 | 通过 |
| BLE 真机写入 / ACK / 配置回读 | 待验证 |

## 8. 遗留风险与建议

1. **BLE 真机验证未完成**：需要分别连接 UDS100、MPS100/DC200、SVC100，执行保存、ACK、断连重连和配置回读。
2. **DC200 上报周期 `0` 未暴露**：固件 BLE 解析接受 `0`，但其周期调度逻辑会使 `0` 继续满足立即上报条件；App 当前限制为 `1-1440` 是安全子集。若确需支持 `0`，应先由固件确认语义。
3. **完整 i18n 未完成**：当前默认英文可保证客户界面一致，但状态、菜单和提示的中文资源仍需后续统一补齐。
4. **SVC 实时任务固件解析存在额外读取**：固件解析表达式临时使用 3 字节脉冲，但目标字段为 `u16`；App 已按 `u16` 限制，实际功能一致。该点建议后续在固件中清理。
