# HKT BLETools iOS v1 — 软件架构重构设计大纲

| 项 | 内容 |
| --- | --- |
| 文档版本 | v0.1 Review Draft |
| 日期 | 2026-09-03 |
| 输入基线 | [iOS v1 研发需求说明](../2026-09-03-ios-v1-rd-requirements.md) §3.2/§3.4；[firmware-traceability.md](../../../shared/devices/firmware-traceability.md)；[shared/protocol/](../../../shared/protocol/README.md) |
| 仓库基线 | main@54e199f（分支 nix/ios-v1-design） |
| 性质 | 《iOS 软件设计说明》的设计大纲：确定模块边界、并发模型、状态机与测试边界，供架构评审；评审通过后扩写为完整设计说明（M2 关闭条件）。本阶段不实现业务代码 |
| 关联文档 | [UI 设计计划](../design/2026-09-03-ios-v1-ui-design-plan.md)、[待确认项裁决建议](../design/2026-09-03-ios-v1-open-items-recommendations.md) |

## 1. 设计目标与 Android 反模式清单

Android 实现只作为协议、能力与业务规则参考。以下 Android 结构性问题明确禁止带入 iOS：

| Android 现状（参考） | iOS 禁止事项（需求 §3.4 硬性要求） |
| --- | --- |
| `MainActivity` companion object 持有全局 `BluetoothGatt`；`Communicate.kt` 全局 `mDeviceData/mDeviceEvent/mDeviceDataString` | 不使用全局 `var` 保存设备状态、GATT 会话、OTA 进度（硬性要求 1） |
| `StreamThread` 后台轮询线程与 BLE 回调无锁并发读写全局模型 | 每份可变模型有唯一 owner；跨线程只传值快照（硬性要求 2/10） |
| BLE 回调直接写 `mDeviceDataString`，UI 定时器读全局刷新 | BLE 回调不直接修改 SwiftUI 状态，只经 Session → ViewModel 快照链路（硬性要求 3） |
| `DeviceActivity.kt` 1800 行巨型页面，表单/协议分发/状态机混在 Activity | View 只做展示与路由；业务在 Use Case/Session（硬性要求 9） |
| OTA 进度 `otaLevel` 全局变量，UI 直接读 | OTA 用显式状态机，转移可测试（硬性要求 4） |
| 协议编解码散落在 `streamDevice()/streamRev()` 巨型函数、以 hex 字符串为中间格式 | 编解码为纯函数，以 `Data`/`UInt8` 为格式，不依赖 UI 类型（硬性要求 5） |

### 1.1 优于 Android 的判据

产品要求 iOS v1 整体优于 Android 版。优势判据锁定四个维度，不通过扩大 v1 功能面实现（功能广度仍按 §9.2 FeatureSurface 裁剪）：

1. **现场更可靠**：actor 隔离消除 Android 已记载的无锁并发隐患（CLAUDE.md「Concurrency hazard」）；连接状态机 + 会话守护 + 断开分类提示。
2. **失败更可解释**：错误十分类各自绑定人话文案与"建议动作"，Android 现状接近统一"失败"；"错误 + 建议动作"列入设计验收清单。
3. **OTA 更可信**：重启→重新广播→重连→版本比对的成功闭环（Android 以进度条+人工观察为主）；每次 OTA 生成可导出的升级报告存档（Android 无此能力）。
4. **迭代更快**：协议/OTA 纯函数 + 双端共享向量 + OTA 状态机 Mock 回放测试，后续加设备/加配置不回归。

体验侧对应的优势项清单见 [UI 设计计划 §10](../design/2026-09-03-ios-v1-ui-design-plan.md)。

## 2. 分层与依赖方向

```text
┌─────────────────────────────────────────────────────────┐
│ App/                        组合根、路由、App 生命周期      │
├─────────────────────────────────────────────────────────┤
│ Features/                   SwiftUI View + ViewModel      │
│   Scan/ Device/ OTA/ Debug/ Settings/                     │
├─────────────────────────────────────────────────────────┤
│ Core/Session (Use Case)     DeviceSession：业务流程编排     │
├─────────────────────────────────────────────────────────┤
│ Core/Protocol + Core/OTA    帧编解码、CRC、TLV、OTA 状态机  │
├─────────────────────────────────────────────────────────┤
│ Core/Device                 DeviceRegistry、能力集、状态模型│
├─────────────────────────────────────────────────────────┤
│ Core/BLE (Abstraction)      BleTransport 协议（扫描/连接/写）│
├─────────────────────────────────────────────────────────┤
│ Core/BLE (Adapter)          CoreBluetooth 唯一接触点        │
└─────────────────────────────────────────────────────────┘
依赖方向：只允许上层依赖下层；下层不得反向 import 上层模块。
```

目录落位（需求 §3.2 既定）：

```text
ios/
├── App/                      HKTBLEToolsApp.swift、AppEnvironment（组合根）、Router
├── Core/
│   ├── BLE/    BleTransport.swift、CoreBluetoothAdapter.swift、ScanFilter.swift、BleEvent.swift
│   ├── Protocol/ CRC16.swift、HKTFrameEncoder.swift、HKTFrameParser.swift、TLV.swift、SignedValueDecoder.swift、CommandCode.swift
│   ├── OTA/    OTAEngine.swift、OTAState.swift、OTATransferPlanner.swift
│   └── Device/ DeviceRegistry.swift、DeviceProfile.swift、DeviceCapability.swift、StatusView 系列模型
├── Features/                 各页面 View + ViewModel + 子 Use Case
├── DesignSystem/             Token、可复用组件（对齐 UI 设计计划 §6）
├── Support/                  日志、错误类型、扩展
├── Resources/                String Catalog、Assets
└── Tests/                    HKTBLEToolsTests / HKTBLEToolsUITests
```

依赖硬规则：

1. `import CoreBluetooth` 只允许出现在 `Core/BLE/CoreBluetoothAdapter*.swift`；协议层、ViewModel、View 中出现即评审不通过（硬性要求 6）。
2. `import SwiftUI` 只允许出现在 `Features/`、`App/`、`DesignSystem/`；`Core/` 内禁止。
3. `Core/Protocol` 与 `Core/OTA` 不 import 任何 BLE 类型，只消费/产出值类型（`Data`、结构体）——保证无设备可单测（需求 §3.2 分层要求 7）。

## 3. 各层职责

| 层 | 职责 | 输入/输出 | 禁止 |
| --- | --- | --- | --- |
| View（SwiftUI） | 展示状态快照、收集用户意图、路由 | 读 ViewModel `@Observable` 状态；发意图方法调用 | 持有 BLE/协议对象；包含业务判断 |
| ViewModel | 页面状态、意图转译、快照整形；`@MainActor` | 订阅 Session 事件流；调用 Use Case | 直接触碰 CBCentralManager/CBPeripheral；写共享可变模型 |
| Use Case / DeviceSession（`DeviceSessionActor`） | 连接会话生命周期、命令编排（查询/配置/校准/时间同步）、请求-响应关联、看门狗超时 | 命令入队 → 协议编码 → transport 写入；Indicate 字节流 → 协议解码 → 事件分发 | UI 类型；直接管理 GATT |
| Protocol Engine | 帧构造（App 帧）、CRC16、多 TLV 连续解析、带符号值解码、命令码表 | 纯函数：`(cmd, data, packNum) -> Data`；`Data -> [TLVRecord]` | 任何 IO、任何状态存储 |
| OTA State Machine（`OTAEngine`，actor） | 固件文件读取校验、分包规划（128 B/包、末包 FF 补 8 字节边界）、阶段流转、每阶段超时、重试 | 事件驱动：bootloader 包请求、写入结果、断开事件 | 直接 UI 更新；并行多会话 |
| Device 模型层 | 设备名识别（广告名 → Profile）、协议家族、能力集、状态/配置字段描述符（含固件边界） | 静态注册表 + 值类型模型 | UI 分支硬编码设备名 |
| BLE Abstraction（`BleTransport`） | 定义扫描、连接、服务发现、订阅 Indicate、写入、MTU、断开事件接口 | 协议类型：`AsyncStream<BleEvent>` + async 方法 | 实现；泄漏 CoreBluetooth 类型到签名之外 |
| CoreBluetooth Adapter | CBCentralManager/CBPeripheral 生命周期、delegate 转译、写入队列（`canSendWriteWithoutResponse` 节奏控制）、MTU 读取 | 实现上述协议；回调收敛到内部串行上下文后转为值事件 | 业务逻辑、协议解析 |

## 4. 并发模型与 actor 边界（需求 §10.1）

```text
CoreBluetooth delegate 回调（系统队列）
        │ 仅在 Adapter 内部串行上下文消费，转成值类型 BleEvent
        ▼
BleEvent AsyncStream ──► DeviceSessionActor（唯一可变会话状态 owner）
        │                    ├─ 连接状态机、请求队列、pending 命令关联、看门狗
        │                    └─ 调用 Protocol 纯函数编解码
        ▼
SessionEvent AsyncStream ──► ViewModel（@MainActor，@Observable 快照）
        ▼
SwiftUI View
OTAEngine（actor）：OTA 期间由 DeviceSession 独占授权（见 §8）
```

规则：

1. 每份可变状态唯一 owner：会话状态在 `DeviceSessionActor`，OTA 状态在 `OTAEngine`，页面状态在各自 ViewModel；禁止跨 actor 直接引用可变对象。
2. BLE 回调 → UI 的唯一通路是 `事件流 → actor 处理 → MainActor 快照`，与硬性要求 3 对应。
3. 全部跨层传值为结构体/枚举快照；Swift 结构体值语义天然避免 Android 的共享可变模型问题。
4. OTA 写入与状态轮询互斥：OTA 激活期间 Session 暂停 `0xFF` 轮询（对应 Android StreamThread 轮询语义的受控版本）。

## 5. 协议引擎（需求 §6）

### 5.1 帧与 CRC

- App 帧：`hkt(3)+packnum(1)+len(2)+cmd(1)+data(n)+crc(2)`；`len` = `cmd+data` 字节数；CRC16-CCITT（poly `0x1021`/reflected `0x8408`，init `0x0000`，xor-out `0x0000`）计算范围为 `cmd+data`。
- CRC 实现为查表纯函数 `CRC16.ccitt(_ data: Data) -> UInt16`，golden vector 直接使用 `shared/fixtures/crc16.json`（`empty→0000`、`abc→29B1`、`hkt→77D1`），双端共享（FW-REF：三固件 `Compents/BootLoader/crclib.c`；Bootloader 校验 CRC，App 层 iOS 一律发送合法 CRC，不声称固件 App 层拒收坏 CRC——见追溯文档 §7）。

### 5.2 帧构造与解析

- `HKTFrameEncoder.encode(cmd:packNum:data:) -> Data`：纯函数。
- `HKTFrameParser.parse(_ frame: Data) -> ParsedResponse`：必须支持一条 Indicate 内多条 TLV 连续解析（App-frame 契约明确要求，不得假设单值包）；产出 `[TLVRecord]` + 顶层命令 + ACK 判定。
- `TLV` 类型枚举按设备家族建视图（TLV 类型来自各固件 `USER/Drive/include/communicate.h`）：
  - 通用：`0x01` 版本、`0x03` 电池、`0x80` 同步时间、`0x86` 上报周期、`0x8D` 电源、`0xFF` ACK；
  - UDS100：`0x09` 温度、`0x0A` 湿度、`0x0E` 倾角、`0x28` HT 告警、`0x44` slant、`0x45` GPS 周期、`0x46` 距离、`0x47` 满溢状态、`0x48` 满溢配置、`0x8B` 电池电压；
  - DC200Family：`0x3A` 车位状态、`0x3B` 工作模式、`0x84` 防拆、`0x5D/0x5E/0x5F` 地磁三轴、`0x60` 雷达频谱（10 值）；
  - SVC100：`0x3C` 设备/双阀状态、`0x40` 电压档、`0x41` 端口功能、`0x42` 稳定时间、`0x43` 智能电源、`0x8A` 时区（含半时区编码 25=+03:30、26=+05:30）。
- `SignedValueDecoder`：保留 Android 的符号位判定规则（24 位 `>0x800000`、32 位 `>0x80000000`、16 位 `>0x8000`），以向量测试锁定。
- 命令码表 `CommandCode`：`0xFF` 查询/ACK、`0xFE` 电源、`0xFD` 校准、`0x02` 配置、`0x03/0x04/0x05` SVC 任务、`0x06` 时间同步、`0x01` OTA 通知；固件专用 `0xF9` 阀控登记但默认不暴露（Q1/裁决）。

### 5.3 命令路由

`DeviceProfile` 提供每设备的命令描述符：cmd → 载荷布局（字段偏移/长度/取值范围/编码函数）。载荷布局来自固件 `fromBleDataHandle()`：

| 命令 | UDS100 | DC200Family | SVC100 |
| --- | --- | --- | --- |
| `0x02` 配置 | 9 B：上报周期+GPS 周期+低阈值+高阈值（周期 `1-1440`、GPS `0/10-1440`、阈值 `30-4500`/`0 或 30-4500`） | 4 B：上报周期(2B)+工作模式（周期 `0 或 1-1440`） | 8 B：电压档+端口功能+稳定时间+智能电源+时区+上报周期（周期 `0 或 1-1440`） |
| `0x03` 实时任务 | — | — | 7 B：阀+状态+时间+脉冲数；本地定时任务执行中固件拒绝 |
| `0x04` 定时任务 | — | — | 11 B：任务 ID(1-16)+阀+状态+脉冲数+起止分钟+重复掩码 |
| `0x05` 删除任务 | — | — | 2 B：任务 ID；`0xFF` 删全部 |
| `0xFD` 校准 | ACK 后进入加速度计校准 | ACK 后进入磁力计校准，**异步完成**，按长时运行状态建模 | — |
| `0x06` 时间同步 | 4 B 时间戳 + 固定 UTC+8 | 同左 | 4 B 时间戳 + 按配置时区换算 |

超时与错误分类（需求 §10.2 十类 → `AppError` 树）：`input / permission / bluetoothUnavailable / connection / serviceMissing / writeFailure / protocolCrc / responseFormat / otaFile / otaTimeout`；每类绑定本地化文案、**建议动作**与 UI 呈现方式（横幅/页面态/弹窗），禁止统一"失败"。请求超时由 Session 看门狗统一管理（默认值：单命令 2 s，校准例外走长时运行状态，OTA 阶段超时见 §8），超时可配置。

## 6. 设备模型与能力表（硬性要求 7/8）

```swift
struct DeviceProfile {
    let displayName: String            // 原始广告名保留（需求 §4 要求 1）
    let family: Family                 // .uds100 / .dc200Family / .svc100
    let advertisedNames: Set<String>   // DC200Family: {"DC200","EPS100","MPS100"}
    let capabilities: OptionSet<DeviceCapability>
    let statusFields: [FieldDescriptor]   // 驱动 P-03 状态模板
    let configFields: [FieldDescriptor]   // 含固件边界，驱动 P-05 校验
}
enum DeviceCapability { ota, statusQuery, powerControl, calibration,
                        basicConfig, svcTasks, timeSync, targetedScan, valveControl }
```

- `DeviceRegistry` 静态注册 UDS100 / DC200Family / SVC100 三 Profile；广告名 `EPS100`、`MPS100`、`DC200` 归一 `DC200Family`（与 Android 解析一致，需求 §4 要求 2）；不识别名称不入列、不可连接（要求 3）。
- UI 显隐 = `profile.capabilities ∩ FeatureSurface`，任何页面不得出现 `if name == "UDS100"` 分支（硬性要求 7/8）。
- `valveControl`（`0xF9`）登记为固件能力但 v1 全链路不暴露；`powerControl/calibration/timeSync` 协议层实现、UI 随 M8 后裁决。
- FW-REF：`PROCT_NAME`/版本已核实——UDS100 hw `0x02`/sw `0x0A`；ParkingSensor 当前 `#else` 分支 MPS100 hw `0x0B`/sw `0x1C`、`#if 0` 分支 EPS100 hw `0x0A`（EPS100 固件基线待固件组提供，见待确认项 Q7）；SVC100 hw `0x0D`/sw `0x0D`。

## 7. BLE 抽象与 CoreBluetooth Adapter（需求 §5）

```swift
protocol BleTransport: AnyObject {
    var events: AsyncStream<BleEvent> { get }
    func startScan(_ filter: ScanFilter) async throws
    func stopScan() async
    func connect(to id: PeripheralID) async throws   // 成功即完成服务发现+订阅+MTU
    func write(_ data: Data) async throws            // Write Without Response 优先，节奏受 MTU 约束
    func disconnect() async
}
enum BleEvent { caseStateChanged(BTState), discovered(DiscoveredDevice),
                connectionChanged(ConnectionPhase), indication(Data), writeReady }
```

- 固定 UUID 常量（需求 §5.1，与 Android 一致）：Service `0783B03E-…-A304D2495CB7`、Write `…5CBA`、Indicate `…5CB8`、CCCD `00002902-…`。
- 连接状态机（Session 持有）：`idle → connecting → discovering → subscribing → ready → (connectionLost | disconnecting → idle)`；每个中间态有超时；`connectionLost` 区分预期断开/异常断开，驱动 P-03 断开提示与重连引导（A-06）。
- Adapter 职责边界：权限状态转译（`CBManagerAuthorization`）、蓝牙开关状态、扫描模式选择（`lowLatency`，扫描周期语义由 Session 层编排为"周期性扫描窗口"以对齐 Android 三轮节奏但允许平台差异）、写入队列背压（等 `peripheralIsReady` 再发下一包，OTA 分包依赖此机制）、MTU 协商结果上报（`maximumWriteValueLength`）。
- 同一协议提供 `MockBleTransport`（脚本化事件回放），供单元测试与 SwiftUI Preview 使用——协议层/OTA 无真机可测的落地手段。
- iOS 模拟器不能验证 BLE；真机验收按需求 §5.4/§12.2 执行。

### 7.1 硬件性能约束与写入适配（固件证据驱动）

目标设备为 STM32L4 级低功耗 MCU + 透明桥 BLE 模组（模组与 MCU 之间是 LPUART1 @115200）。已核实的硬件约束及 iOS 适配要求：

| 硬件约束（固件证据） | 对 iOS 的含义 | 适配设计 |
| --- | --- | --- |
| MCU 按「空闲超时」判帧：BLE 口 50ms 无新字节才处理已收帧（`USER/Drive/uart.c` LPUART1_IRQHandler `recvTimeout=50`） | 一条协议帧的全部字节必须落在同一个 50ms 窗口内到达 MCU；单命令 RTT 天然含 ~50ms 判帧延迟 | 不做固定延时盲发；单命令等待响应后再发下一条（单飞），看门狗预算覆盖 50ms 判帧延迟 |
| 应用层 UART RX 缓冲 128 B、Bootloader 255 B（`USER/Drive/include/uart.h` 128 / `Compents/BootLoader/uart.h` 255，超限字节静默丢弃） | OTA 帧 146 B 只能被 Bootloader 承接；应用层响应必须 <128 B（固件查询响应已满足，解析器按此上限做防御） | OTA 只在 Bootloader 阶段发送大帧；协议解析器对超长/截断帧按 `responseFormat` 错误处理 |
| MTU：Android 连接后协商 512；小 MTU 会把 146 B OTA 帧拆成多次写，跨 50ms 窗口即被判帧逻辑切碎 | MTU 是 OTA 可行性前置条件 | 连接后读取 `maximumWriteValueLength`；**OTA 启动前置检查 ≥160 B**（146 B 帧 + 余量），不足则 OTA 入口禁用并提示，禁止盲试 |
| BLE↔UART 115200 ≈ 11.5 KB/s 理论上限，实际由停等流控决定吞吐 | 吞吐瓶颈在设备侧，不是 App 侧；追求写入并发无意义 | 单飞 + 设备驱动节奏（§8.1），不做流水线发送 |
| 设备带休眠逻辑（`ble_connected` 位、sleep 延迟），会话空闲可能断开 | 空闲断开是预期行为 | 保持 1 s 级轮询维持活跃（§9.1）；断开守护即时反映（§7） |
| **连接后 5 s 无交互即进睡眠、60 s 未连接强制睡眠**（`control_center.h` `WAIT_SLEEP_TIME_DELAY=5`/`MAX_WAIT_SLEEP_TIMEOUT=60`，`ble_sleep_mode` 可配置） | 1 s 轮询不是 UI 优化而是**保活硬约束**；连接序列（服务发现/订阅/首次查询）必须紧凑，任一环节超 5 s 设备入睡、链路可能中断 | 连接成功即自动发起首轮 `0xFF` 查询（不等用户进 P-03）；会话任何"暂停轮询"策略必须保证间隔 ≤4 s，否则改为显式断开 |
| LoRa 上行与 BLE 同主循环（上报时刻 `dataReportTimestamp` 触发 9600 UART 慢速收发），期间 BLE 响应延迟 | 会话中偶发秒级响应延迟，非设备故障 | 命令看门狗 2 s 起步并区分"响应慢"与"无响应"；延迟事件记入诊断日志辅助现场判断 |
| 连接参数由设备侧模组决定 | iOS 不请求连接间隔，也不应假设固定间隔 | 写入节奏只依赖 ACK/回调驱动，不依赖连接间隔假设 |

## 8. OTA 状态机与并发模型（需求 §8，v1 强制）

```text
idle → fileValidated → notifying(size) → transferring(packet n/N)
     → finalPadding(FF 补齐至 8 字节边界) → completionSent(cmd 0x03)
     → waitingReboot(超时 T1) → waitingAdvertisement(超时 T2)
     → reconnecting(超时 T3，复用 §7 连接状态机) → verifyingVersion
     → success | failed(OTAError: 阶段+原因)
```

要点：

1. **验收门**：`success` 唯一前置是 `verifyingVersion` 通过——重连后 `0xFF` 查询读取 TLV `0x01` 与期望版本一致（Q8 裁决）；`transferring` 到 100% 只是 `completionSent` 的输入，不是成功（需求 §8.4）。
2. 分包规划 `OTATransferPlanner` 为纯函数：128 B/包、包号自 `0x0002` 起（bootloader 请求驱动）、末包 `FF` 补齐 8 字节边界、完成命令 `0x03`；全部可向量测试（FW-REF：`Compents/BootLoader/uart.c`，三固件契约一致）。
3. 并发模型：OTA 期间 `DeviceSession` 将传输授权移交 `OTAEngine`（单飞），暂停状态轮询、拒绝其他命令入队；传输写入经 Adapter 写入队列按 `writeReady` 背压推进，不使用固定延时盲发。

### 8.1 设备停等流控与超时预算（固件语义驱动）

Bootloader 采用**停等式 ARQ**（`Compents/BootLoader/uart.c`）：每包数据写入后回 `InfoUartAck(cmd=2, 下一包号)`，App 只能发送设备正在请求的包号——**传输节奏由设备驱动，App 端永远至多一个未确认包**。此语义优先级高于任何通用背压策略。

超时与恢复预算（每项均有固件依据）：

| 场景 | 固件行为（证据） | iOS 预算/动作 |
| --- | --- | --- |
| 单包 ACK 等待 | 设备每 16 包执行一次 2 KB 页擦+写（`FLASH_ONE_PAGE_SIZE=0x800`），第 16n 包 ACK 延迟显著增大 | 单包超时 3 s（覆盖页写峰值）；同一包连续 2 次超时判失败并释放资源 |
| 传输静默 | 设备每秒发 nudge ACK 请求当前包；连续约 10 s 无数据则**自复位传输状态**（`flash_write_count=0`、`start_flag=0`）并回 `InfoUartAck(1, 0)` | 任何传输间隙必须 <8 s（留 2 s 余量）；App 端各传输阶段看门狗 ≤3 s，远小于 10 s 复位阈值 |
| 收到 `InfoUartAck(1, 0)`（设备已复位传输） | 不是错误，是设备发出的"从头再来"信号 | 传输中收到即从包 0 自动重传，限 2 次；升级报告记录"设备超时重置，已自动重启传输"；超限判失败 |
| CRC 校验 | Bootloader 逐帧校验 CRC16（`crc16_ccitt`），坏帧静默丢弃 → 表现为该包 ACK 缺失 | 复用单包超时路径；升级报告标记疑似 CRC 丢帧 |
| 等待重启/重广播/重连 | Bootloader 完成后交棒应用，不复位传输状态机 | T1/T2/T3 阶段超时不受 10 s 约束，维持 §8 主流程取值（真机标定） |

设计规则：`OTAEngine` 每包发送前断言 `pendingPacket == deviceRequestedPacket`；任何阶段超时取值必须显式登记固件依据，禁止拍脑袋常数。
4. 每阶段有独立超时与错误归属：`waitingReboot/waitingAdvertisement/reconnecting` 超时分别映射需求 §8.5 场景 7/8/9；断开事件在任何传输阶段触发 `failed(.connectionLost)` 并释放会话资源（A-08/A-10）。
5. 重连匹配策略 v1：按断开前设备 `PeripheralID` 直接 `retrievePeripherals(withIdentifiers:)` 重连；若系统检索失败再回退短时扫描匹配同广告名。真机阶段需验证升级重启后 identifier 稳定性（MPS100 先行验证，风险登记 §12）。
6. UI 契约：`OTAEngine` 每次转移发布 `OTAProgress` 快照（阶段+包计数+耗时），P-04 六阶段界面一一映射（UI 设计计划 §3 P-04）；返回保护由 ViewModel 依据状态 ∈ {transferring, finalPadding, completionSent, waitingReboot, waitingAdvertisement, reconnecting} 决定。

## 9. 会话生命周期、feature surface 与依赖注入

### 9.1 会话生命周期与 UI 同步

- `DeviceSessionActor` 由组合根按"单活跃会话"创建/销毁；进入 P-03 建会话，退出或 `connectionLost` 清理并回列表。
- 轮询策略：`ready` 后以约 1 s 间隔发 `0xFF`（对齐 Android 节奏），响应经节流（≥500 ms 合并）再推 UI，满足需求 §10.3 性能要求；轮询暂停条件：OTA 激活、Debug 独占、页面不可见（可选优化）。
- ViewModel 只缓存不可变快照；`@MainActor` 保证 UI 单线程消费。

### 9.2 FeatureSurface（硬性要求 8）

```swift
struct FeatureSurface {
    var basicConfigUI: Bool = false     // M8 随 Q1 裁决置 true
    var debugUI: Bool = false           // 隐藏入口开启；dev 构建默认 true
    var targetedScanUI: Bool = false    // Q4：v1 关闭
    var barcodeUI: Bool = false
    var languageSwitchUI: Bool = true
    var diagnosticLogUI: Bool = true
}
```

注入组合根构造；页面入口可见性 = 能力交集（§6）；v1 主线（扫描/连接/状态/OTA/日志/语言）恒开。

### 9.3 依赖注入（硬性要求 9）

- 纯构造注入，不引第三方 DI 框架：组合根 `AppEnvironment` 创建 `CoreBluetoothAdapter → DeviceSession → OTAEngine → ViewModel`。
- ViewModel 依赖协议类型（`BleTransport`、`SessionServicing`），Preview/测试注入 Mock；无服务定位器、无单例可变状态。

### 9.4 日志、本地化、错误上报（需求 §9.2/§9.3）

- 结构化日志：OSLog（开发）+ 进程内环形文件缓冲（诊断导出），覆盖扫描启停/目标发现/连接变化/服务发现/订阅结果/写入/响应/OTA 阶段/错误恢复九类事件；记录字段为值类型快照；敏感数据（无密钥类数据，仅设备标识）白名单制，导出前人工可见内容固定模板。
- **OTA 升级报告**：每次 OTA 结束自动生成结构化摘要（设备型号、升级前版本、期望/实际版本、各阶段起止与耗时、失败原因与阶段），支持随诊断日志一键导出存档，作为现场支持与验收留痕材料；数据全部来自 OTAEngine 已发布的快照事件，无额外采集成本。
- 本地化：String Catalog，禁止 view/错误提示硬编码（UI 设计计划 §7）。
- 无远端错误上报（v1 无后端），诊断靠日志导出。

### 9.5 业务连续性设计（会话恢复与设备状态收敛）

现场工具的业务连续性目标：**工程师的操作流程不因断线、后台、App 重启而丢状态；任何中断后，设备和 App 都收敛到明确、可解释、可恢复的状态**。

#### 9.5.1 设备侧连续性：OTA 中断后的状态收敛（固件证据）

- Bootloader 启动时读取更新标志（`Compents/BootLoader/main.c` `FLASH_Read_Update_Flag()`）：标志有效则驻留 Bootloader 升级模式等待重刷，否则跳转应用区。
- 因此 **OTA 传输中断 ≠ 变砖**：设备重启后留在升级模式、重新广播；App 重连后 `0xFF` 状态查询无版本响应，即为"设备处于升级模式"的判定信号。
- App 处理：识别该状态 → P-03/P-04 显示"设备处于升级模式"专用恢复态 → 引导用户重新选择固件完整重刷（复用同一 OTA 流程），**禁止**把该设备当正常设备展示空状态页。
- 风险登记：`AppProgramRun()` 跳转前仅校验栈顶地址合法性（`flash.c` `(AppSpInitVal & 0x2FFE0000) == 0x20000000`），**无应用区 CRC 校验**；完成 ACK 之后、清标志/跳转之间失败的窗口行为需 M3 逐设备核实（升级标志写入/清除时序），确认前 App 对"完成包已发但未读到新版本"一律按失败态引导重刷。

#### 9.5.2 会话连续性：断线、蓝牙复位与 App 生命周期

| 中断 | 行为设计 |
| --- | --- |
| 异常断开 | 自动重连（退避，上限 3 次），期间 P-03 显示"重连中"；失败转手动重试；预期断开（用户退出/OTA 重连流程）不触发自动重连 |
| 蓝牙关闭再开启 / 系统蓝牙复位 | Adapter 收到 `poweredOff/reset` 全量清理会话与 OTA 状态（CoreBluetooth 的 peripheral 引用随之失效）；恢复后自动回到扫描就绪，**不要求重启 App** |
| App 前后台切换 | 扫描自动暂停/恢复；OTA 前台策略按 §13；返回前台先校准状态（会话存活校验）再恢复 UI 刷新 |
| App 被杀/崩溃后冷启动 | BLE 会话不跨进程存续——不自动重连；但恢复用户上下文：最近设备（`CBPeripheral` identifier 系统级持久，可列表直达重连）、最近固件选择、未完成 OTA 记录标记为"中断-待验证"，引导对目标设备重新执行版本验证或重刷 |
| 长时操作跨 App 重启（如校准） | 设备侧自行继续；App 重启后通过 `0xFF` 查询反映当前状态，不做本地"进行中"假状态 |

#### 9.5.3 操作连续性：幂等、资源收敛与解析向前兼容

1. 配置/查询命令幂等可安全重试（固件为持久化写/纯读）；OTA 是唯一不可逆流程，其恢复语义由 §8.1 停等+设备复位机制承担。
2. 会话切换（设备 A→B 或退出设备页）必须先完全释放旧会话（停轮询、终止 OTA、断开 GATT、清超时器）再建新会话，防资源泄漏与串台（A-10）。
3. TLV 解析器对未知类型**跳过不报错**——固件升级新增字段后，旧 App 仍可正常读取其余状态（跨版本业务连续性）；未知类型记入诊断日志。
4. 本地持久化（v1 最小集）：最近设备列表、扫描过滤设置、语言选择、OTA 历史报告（含中断记录）；采用 App 沙盒 JSON 文件（按需迁移 SQLite），全部经组合根注入的持久化接口访问，Core 层不直接触盘。

## 10. 测试边界（需求 §10.4）

| 测试域 | 载体 | 向量来源 |
| --- | --- | --- |
| CRC16 | `CRC16Tests` | `shared/fixtures/crc16.json`（双端共享） |
| App 帧构造/解析（含多 TLV） | `FrameCodecTests` | FW-VEC：固件 `communicate.c` 查询响应构造 |
| Bootloader 帧/分包/末包补齐 | `OTATransferPlannerTests` | FW-VEC：`Compents/BootLoader/uart.c` |
| TLV 解析 + 符号值 | `TLVParserTests` / `SignedValueDecoderTests` | FW-VEC：各家族 `communicate.h` 类型表 |
| 设备名识别/归一 | `DeviceRegistryTests` | 需求 §4 表 |
| OTA 状态机（含超时/断开注入） | `OTAEngineTests`（MockBleTransport 回放） | 需求 §8.2/§8.5 |
| 配置边界（接受/拒绝/静默） | `ConfigBoundaryTests` | FW-VEC：`fromBleDataHandle()` 各范围 |
| 错误分类映射 | `ErrorMappingTests` | 需求 §10.2 |
| 稳定性与业务连续性（保活/睡眠/安装形态/中断恢复） | [稳定性与连续性测试用例基线](2026-09-04-ios-v1-stability-continuity-test-cases.md)（TC-ST/TC-BC，XCTest + 真机） | 需求 §18 S-1~S-8 |

每个测试登记 `FW-REF-*`/`VEC-*`/`TC-*`（追溯文档 §5 规则）；M3 固件协议与测试基线里程碑关闭时补全向量 JSON（`shared/fixtures/`）。

## 11. 评审通过标准映射（需求 §3.4）

| 评审标准 | 设计落点 |
| --- | --- |
| 1. 新增/差异设备只影响设备模型与能力表 | §6 DeviceRegistry/Profile；OTA/BLE 层无设备分支 |
| 2. 新增配置/SVC 任务/Debug/二维码不改 Adapter | §5.2 命令码表 + §9.2 FeatureSurface + §7 Adapter 仅传输语义 |
| 3. OTA 状态机无真机可测 | §8 纯函数 Planner + Mock 回放（§7/§10） |
| 4. 断开/重连/超时/写失败路径完整可追踪 | §7 连接状态机 + §8 阶段超时 + §9.4 日志九类事件 |
| 5. UI 状态可映射 ViewModel 状态 | §3/§8/§9.1 与 UI 设计计划 §3 页面状态集逐项对照（评审时核对） |

## 12. 关键架构决策记录（ADR 摘要）

| ADR | 决策 | 状态 |
| --- | --- | --- |
| ADR-001 | 原生 SwiftUI + CoreBluetooth；不引入跨平台运行时；仅共享协议文档与 fixtures | 已定（需求 §3.1） |
| ADR-002 | 六层单向依赖；CoreBluetooth 隔离于 Adapter；Core 不 import SwiftUI | 本文 §2，待评审 |
| ADR-003 | iOS 17 基线，`@Observable` 做状态同步 | 随 Q5 评审 |
| ADR-004 | `DeviceSessionActor` 单 owner + 事件流快照，替代 Android 全局可变状态/轮询线程 | 本文 §4，待评审 |
| ADR-005 | DeviceRegistry 能力表驱动 UI，禁止设备名硬编码 | 本文 §6，待评审 |
| ADR-006 | OTA 显式状态机；成功判定 = 版本确认；进度 100% 非成功 | 本文 §8，随 Q8 评审 |
| ADR-007 | 协议编解码纯函数 + 双端共享 golden vectors；hex 字符串不作为运行时中间格式 | 本文 §5，待评审 |
| ADR-008 | FeatureSurface 控制功能显隐；UI 里程碑与架构能力解耦 | 本文 §9.2，待评审 |
| ADR-009 | OTA 单飞（会话独占）；写入背压由 Adapter `writeReady` 驱动，替代固定延时盲发 | 本文 §8，待评审 |
| ADR-010 | 命名与 Bundle ID（`HKT BLETools` / `com.hkt.ble.bletools.ios`） | 随 Q10 评审 |

## 13. 风险与开放点

| 风险/开放点 | 处理 |
| --- | --- |
| 升级重启后 `PeripheralID` 是否稳定（§8.5） | M6 MPS100 真机首验；回退策略已设计（同广告名短扫描） |
| MTU/写入节奏与 Android 差异 | Adapter 背压机制 + M4 真机联调专项 |
| EPS100 固件基线缺失 | Q7 待固件组提供；阻塞 M7 不阻塞 M4–M6 |
| `.bin` 文件版本解析可行性 | M3 核对 Bootloader flash 布局；未证实前用"用户确认期望版本" |
| 固件 App 层 CRC 是否拒收未证实 | 测试文档只声明"iOS 发送合法 CRC"（追溯文档 §7） |
| 后台/锁屏对长 OTA 干扰 | v1 前台 OTA 为准；锁屏/后台往返列入 §8 异常测试；系统级后台保活实现前另做平台评审（需求 §9.4） |
| 设备 UART 帧窗口/复位语义在不同固件版本的差异 | §7.1/§8.1 约束已按当前源码核实；M3 固件基线锁定时逐设备复核 `recvTimeout`/缓冲/复位阈值；真机矩阵增加"OTA 中注入 5/10/15 s 停顿"验证 nudge 与复位恢复 |
| 不同 iPhone 与设备模组的 MTU 协商差异 | M4 真机联调记录各机型协商结果；OTA 前置 MTU ≥160 B 检查兜底 |
| 弱信号下的连接与 OTA 稳定性 | 稳定性矩阵增加 RSSI -90 dBm 附近的扫描/连接/OTA 用例（A-10 扩展） |
| OTA 中断后设备状态与升级标志时序 | §9.5.1 已按源码核实"中断→驻留 Bootloader"主路径；完成 ACK→清标志→跳转窗口行为 M3 逐设备核实；真机矩阵增加"传输中杀 App/断电后重连"用例验证恢复态识别 |
| **BLE 模组配置未纳入固件追溯** | 广播间隔、连接间隔、监督超时由透明桥模组侧配置决定，MCU 源码不可见；M3 必须登记模组型号/固件/配置参数作为 FW-REF 的一部分，否则连接稳定性分析不完整 |
| 地磁设备安装环境极端（地埋/表贴 + 车体遮挡，2.4 GHz 衰减大） | 真机矩阵必须含"设备地埋 + 车辆压顶"的发现/连接/OTA 场景；DC200（地埋）与 MPS100（表贴）两种安装形态分别记录 RSSI 与成功率 |
