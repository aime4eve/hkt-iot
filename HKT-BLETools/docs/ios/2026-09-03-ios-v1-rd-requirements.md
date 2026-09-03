# HKT BLETools iOS v1 研发需求说明

| 项 | 内容 |
| --- | --- |
| 文档版本 | v0.5 Draft |
| 日期 | 2026-09-03 |
| 产品 | HKT BLETools iOS |
| 技术路线 | 原生 SwiftUI + CoreBluetooth |
| 参考基线 | 固件源码、Android/Kotlin 现有工程与协议实现 |
| 状态 | 已确认总体路线、UI 先行、软件架构重构和固件代码追溯原则；部分 v1 UI 边界仍在评审 |

## 1. 背景与目标

HKT BLETools 当前已有 Android/Kotlin 实现，用于扫描、连接、查询、配置和升级 HKT 系列 BLE IoT 设备。设备通信使用自定义 `hkt` 二进制协议，OTA 使用独立 Bootloader 帧。

由于现有 Android 工程无法直接生成 iOS 安装包，且 BLE、MTU、Indicate、分包写入和 Bootloader OTA 都需要稳定的平台级控制能力，iOS 版不采用 Flutter、React Native、Capacitor 或 KMP 作为首版方案。

iOS v1 的总体目标是：

1. 建立可长期演进的原生 SwiftUI 工程。
2. 使用 CoreBluetooth 实现 BLE 扫描、连接和状态会话。
3. 按固件源码和现有 Android 能力域建模协议层、设备模型和状态机。
4. 首版必须交付 UDS100、SVC100、EPS100、MPS100 的 OTA 能力。
5. 后续参数配置、任务配置、Debug、二维码入口等功能可以在同一架构上逐步开放。

## 2. 范围原则

本需求区分两个维度：

- **架构覆盖范围**：必须覆盖 Android 当前协议能力域，避免后续补功能造成结构性重构。
- **UI 交付范围**：可以按里程碑分批开放，但必须通过显式 feature surface 控制，不能让底层协议被窄化为“只支持 OTA”。
- **固件权威范围**：每款设备的功能、协议字段、取值范围、响应行为和 OTA 流程必须追溯到对应固件源码；Android 实现只是兼容性参考。

因此，本需求中的“延后”仅表示该能力不在首版主 UI 中开放或优先级较低，不代表协议层可以不建模。

## 3. 技术路线

### 3.1 已确认路线

| 项 | 决策 |
| --- | --- |
| UI 框架 | SwiftUI |
| BLE 框架 | CoreBluetooth |
| 跨平台方案 | 不采用 Flutter / React Native / Capacitor / KMP / Compose Multiplatform |
| 共享方式 | 只共享协议文档和测试向量，不共享跨平台运行时代码 |
| 仓库结构 | `android/`、`ios/`、`shared/` monorepo |
| 测试 | XCTest 覆盖固件追溯协议、设备识别、OTA 状态机和 BLE 调度抽象 |

### 3.2 iOS 目录约定

`ios/` 直接作为 iOS 源码根，不再建立 `ios/HKTBLETools/` 这层额外同名目录。

```text
ios/
├── HKTBLETools.xcodeproj
├── App/
├── Core/
│   ├── BLE/
│   ├── Protocol/
│   ├── OTA/
│   └── Device/
├── Features/
│   ├── Scan/
│   ├── Device/
│   ├── OTA/
│   ├── Debug/
│   └── Settings/
├── DesignSystem/
├── Support/
├── Resources/
└── Tests/
    ├── HKTBLEToolsTests/
    └── HKTBLEToolsUITests/
```

分层要求：

1. SwiftUI 只处理展示、用户输入和路由。
2. SwiftUI 不得直接调用 `CoreBluetooth`。
3. `Core/BLE` 负责 CBCentralManager、CBPeripheral、服务发现、MTU、写入、Indicate、断开守护。
4. `Core/Protocol` 负责帧构造、CRC、命令路由、TLV 解析。
5. `Core/OTA` 负责固件读取、分包、状态流转、进度和完成处理。
6. `Core/Device` 负责设备名识别、协议家族、能力集和状态模型。
7. 协议层和 OTA 状态机必须可以在不连接真实设备的情况下被单元测试。
8. 每个协议能力和测试向量必须登记固件源码引用；不能只引用 Android 实现。

### 3.3 UI 设计先行

UI 设计是 iOS v1 功能开发的强制前置阶段。未经 UI 设计基线确认，不得开始 SwiftUI 功能页面实现。

UI 设计必须覆盖以下范围：

1. 四类支持设备的能力差异和入口状态。
2. 扫描、过滤、设备列表、连接、断开和重连流程。
3. 设备状态展示。
4. 固件选择和 OTA 全生命周期。
5. 权限、蓝牙关闭、扫描失败、连接失败、升级失败等异常状态。
6. 中英文文案、深色模式、Dynamic Type、iPhone 不同尺寸适配。
7. Debug、参数配置、二维码/DevEUI 等待定入口的占位与开放规则。

UI 设计交付物至少包括：

| 交付物 | 要求 |
| --- | --- |
| 信息架构 | 定义主要页面、导航关系、入口层级和 feature surface |
| 关键用户流程 | 覆盖扫描、连接、状态查看、OTA 成功/失败/恢复流程 |
| 低保真线框图 | 先确认页面结构、信息层级和操作路径 |
| 高保真视觉稿 | 覆盖浅色、深色、中英文和关键空态/加载/错误状态 |
| 交互说明 | 定义按钮状态、禁用条件、确认弹窗、返回保护、重试策略 |
| 组件与设计 Token | 色彩、字体、间距、圆角、图标、状态样式可复用 |
| OTA 过程原型 | 明确每个阶段的界面反馈和用户可执行动作 |
| 设计验收清单 | UI 与需求、设备能力、异常状态、本地化逐项对应 |

设计文件建议保存到 `docs/ios/design/` 或通过仓库文档登记外部 Figma 链接。外部链接必须可被团队访问，且仓库内必须保留导出的关键页面截图或 PDF 版本。

UI 设计验收标准：

1. 每个功能入口都有明确状态，不允许“加载中”“失败”等空白描述。
2. 每个面向用户的文案都有中英文来源。
3. 四类设备的差异能力在 UI 上可识别、可解释。
4. OTA 各阶段都有界面反馈、禁止操作和错误恢复路径。
5. 设计稿中不出现协议内部术语，用户可理解当前状态和下一步动作。
6. SwiftUI 实现可通过设计 Token 和组件复用，而不是逐页硬编码样式。

### 3.4 软件架构重构要求

Android 工程仅作为协议、设备能力和业务规则参考，不作为 iOS 软件架构模板。iOS 必须重新设计模块边界，禁止照搬 Android 当前的全局可变状态、Activity 内巨型页面逻辑和 BLE 回调直接驱动 UI 的结构。

开始功能实现前，必须先输出《iOS 软件设计说明》，并完成架构评审。设计说明至少覆盖：

1. 模块边界和依赖方向。
2. SwiftUI View、ViewModel、Use Case、Protocol、BLE、OTA 的职责。
3. 四类设备的公共模型、协议家族和差异能力。
4. 命令路由、响应解析、错误分类和超时处理。
5. OTA 状态机和并发模型。
6. BLE 会话生命周期与 UI 状态同步方式。
7. feature surface 的配置方式和 UI 显隐规则。
8. 日志、错误上报、本地化和测试边界。
9. 依赖注入方式。
10. 关键架构决策记录。

iOS 软件架构采用以下分层：

```text
SwiftUI View
    只负责展示、输入、路由
        ↓
ViewModel / Observable State
    只负责页面状态、用户意图和状态快照
        ↓
Use Case / Device Session
    负责业务流程、能力判断、状态编排
        ↓
Protocol Engine + OTA State Machine
    负责帧构造、CRC、TLV 解析、分包和阶段流转
        ↓
BLE Abstraction
    定义扫描、连接、写入、Indicate、MTU、断开接口
        ↓
CoreBluetooth Adapter
    唯一接触 CBCentralManager / CBPeripheral 的实现层
```

架构重构硬性要求：

1. 不使用全局 `var` 保存设备状态、GATT 会话或 OTA 进度。
2. 不允许多个线程直接读写同一份可变模型。
3. BLE 回调不得直接修改 SwiftUI 页面状态。
4. OTA 必须使用显式状态机，状态转移必须可测试。
5. 协议编解码必须是纯函数或纯服务，不依赖 UI 类型。
6. CoreBluetooth 类型不得泄漏到 SwiftUI 和协议层。
7. 设备能力由 `DeviceRegistry` 统一描述，UI 和业务层不硬编码设备名分支。
8. UI 显隐由能力集和 feature surface 控制。
9. ViewModel 通过依赖注入获取 Use Case 和 BLE 会话。
10. 所有跨层状态同步必须明确线程或 actor 边界。

软件架构评审通过标准：

1. 四类设备的新增和差异能力只影响设备模型和能力表，不需要重写 OTA 或 BLE 层。
2. 新增参数配置、SVC 任务、Debug 或二维码入口，不需要修改 CoreBluetooth Adapter。
3. OTA 状态机可以在无真实设备条件下完成单元测试。
4. BLE 会话断开、重连、超时、写入失败路径完整可追踪。
5. UI 设计稿中的每个状态都能映射到明确的 ViewModel 状态。
## 4. 支持设备

| 产品名称 | 广告名匹配 | iOS 内部协议模型 |
| --- | --- | --- |
| UDS100 | `UDS100` | `UDS100` |
| DC200 | `DC200` | `DC200Family` |
| EPS100 | `EPS100` | `DC200Family` |
| MPS100 | `MPS100` | `DC200Family` |
| SVC100 | `SVC100` | `SVC100` |

要求：

1. 设备识别必须保留原始广告名。
2. EPS100 和 MPS100 归入 `DC200Family`，与 Android 现有解析保持一致。
3. 不支持或不识别的设备不能误连为目标设备。
4. 设备能力必须由 `DeviceRegistry` 统一描述，UI 不允许散落硬编码设备名判断。

### 4.1 固件代码追溯基线

每款设备的功能必须与对应固件工程核对。固件源码是协议和设备能力的权威依据，Android App 是兼容性参考；两者不一致时，必须先做固件版本裁决，再决定 iOS 是否实现、提示不支持或作为受限能力开放。

固件追溯基线见 [`shared/devices/firmware-traceability.md`](../../shared/devices/firmware-traceability.md)。初始映射如下：

| iOS 模型 | 固件工程 | 初始核对结果 |
| --- | --- | --- |
| UDS100 | `LoRaWAN_Ultrasonic_Distance_Sensor` | `USER/config.h` 定义 `PROCT_NAME "UDS100"`；协议类型和 BLE 处理位于 `USER/Drive/include/communicate.h`、`USER/Drive/communicate.c`；Bootloader 位于 `Compents/BootLoader/uart.c`。 |
| DC200Family / EPS100 / MPS100 | `LoRaWAN_ParkingSensor` | 同一固件工程覆盖 EPS100/MPS100。当前检出源码选择 `MPS100`；EPS100 必须确认对应分支、tag 或构建配置后再认证。 |
| SVC100 | `LoRaWAN_Solenoid_Valve_Controller` | `USER/config.h` 定义 `PROCT_NAME "SVC100"`；基础配置、实时任务、定时任务、删除任务和时区处理位于 `USER/Drive/communicate.c`。 |

任何固件能力进入 iOS 前，必须至少提供：

1. 固件仓库、分支、tag 或 commit。
2. `USER/config.h` 中的产品名和软硬件版本。
3. `USER/Drive/include/communicate.h` 中的 TLV 类型。
4. `USER/Drive/communicate.c` 中 `callback_BLEQuery()`、`fromBleDataHandle()` 或等价函数的行为。
5. Bootloader OTA 处理函数。
6. Android 实现与固件行为的差异说明。
7. 由固件行为推导出的测试向量。

没有固件引用的能力只能标记为“待固件核对”，不得进入 v1 验收。

## 5. BLE 需求

### 5.1 Service 与 Characteristic

Android 端固定 UUID 如下，iOS 首版必须保持一致：

| 项 | UUID |
| --- | --- |
| Service | `0783B03E-8535-B5A0-7140-A304D2495CB7` |
| Write Characteristic | `0783B03E-8535-B5A0-7140-A304D2495CBA` |
| Indicate Characteristic | `0783B03E-8535-B5A0-7140-A304D2495CB8` |
| CCCD | `00002902-0000-1000-8000-00805F9B34FB` |

### 5.2 扫描

必须支持：

1. 手动启动和停止扫描。
2. 按支持设备名过滤。
3. 读取 RSSI。
4. 支持信号强度阈值过滤。
5. 支持过滤无名称设备。
6. 扫描结果去重。
7. 显示设备名、MAC/Identifier、RSSI。
8. 点击设备后发起连接。

Android 现有扫描节奏为三轮扫描，每轮 10 秒，轮间间隔 500 ms。iOS 可在保留“周期扫描结束”概念的基础上适配 CoreBluetooth 行为，但 UI 必须明确显示扫描进行中、扫描完成或扫描失败。

### 5.3 目标设备定位

Android 已有能力包括：

1. 手工输入 16 位十六进制 DevEUI。
2. 扫描二维码或条码后提取 DevEUI。
3. 根据 DevEUI 后缀匹配目标设备。
4. 目标扫描期间优先等待目标设备出现。

iOS 架构必须保留目标设备查找能力。该能力是否进入 v1 主 UI 见“待确认项”。

### 5.4 连接

连接流程必须覆盖：

1. 发起连接。
2. 连接超时和失败提示。
3. 服务发现。
4. 订阅 Indicate Characteristic。
5. 写入 CCCD。
6. MTU 协商或读取实际可用 MTU。
7. 连接成功后进入设备页。
8. 断开时通知用户并退出设备会话。
9. 连接状态守护，避免 UI 状态与 CoreBluetooth 状态不一致。

iOS 模拟器不能验证 BLE。所有 BLE 和 OTA 验收必须使用真实 iPhone 和真实目标设备。

## 6. 协议需求

### 6.0 固件契约优先

本节只定义 iOS 协议层必须覆盖的能力域。每个命令、字段偏移、长度、取值范围、状态类型和 ACK 行为，必须以 [`shared/devices/firmware-traceability.md`](../../shared/devices/firmware-traceability.md) 登记的固件源码为准。Android 的 `Communicate.kt` 只用于证明既有兼容行为，不得单独作为协议正确性依据。

协议文档、测试向量和 iOS 实现必须能回答：

1. 固件在哪个函数接收该命令？
2. 固件校验哪些长度和取值范围？
3. 固件是否持久化该配置？
4. 固件返回 ACK、查询数据、错误日志，还是静默忽略？
5. 同一命令在不同固件版本间是否存在差异？

### 6.1 App 帧

```text
hkt(3) + packnum(1) + len(2) + cmd(1) + data(n) + crc(2)
```

要求：

1. 前缀为 `686B74`，即 ASCII `hkt`。
2. `len` 表示 `cmd + data` 的字节长度。
3. CRC16-CCITT 计算范围为 `cmd + data`。
4. 响应中可能包含多条 TLV 记录，iOS 必须支持连续解析。
5. 不得假设一条 Indicate 响应只包含一个字段。

### 6.2 必须建模的命令

| 命令 | 能力 | v1 要求 |
| --- | --- | --- |
| `0xFF` | 状态查询 / ACK | 必须实现 |
| `0xFE` | 电源开关 | 协议层必须实现；UI 优先级待定 |
| `0xFD` | 校准 | 协议层必须实现；UI 优先级待定 |
| `0x02` | 参数配置 | 协议层必须实现；UI 范围待定 |
| `0x03` | SVC100 实时任务 | 协议层必须实现；UI 范围待定 |
| `0x04` | SVC100 定时任务 | 协议层必须实现；UI 范围待定 |
| `0x05` | SVC100 删除任务 | 协议层必须实现；UI 范围待定 |
| `0x06` | 时间同步 | 协议层必须实现；UI 范围待定 |
| `0x01` / Bootloader | OTA | v1 必须实现 |

### 6.3 CRC

协议使用 CRC16-CCITT：

- polynomial：`0x1021`
- reflected implementation polynomial：`0x8408`
- init：`0x0000`
- xor-out：`0x0000`

iOS 必须使用与 Android 一致的算法，并通过共享 golden vector 测试。

### 6.4 状态响应解析

协议层必须支持以下响应字段类型：

1. 版本。
2. 电量或电压。
3. 温度、湿度。
4. 倾角、倾斜状态。
5. 距离。
6. 满溢状态和阈值。
7. 经纬度。
8. 车位状态和工作模式。
9. 防拆状态。
10. 地磁三轴。
11. 雷达频谱。
12. 双阀状态、插入状态、脉冲数。
13. 输出电压、端口功能、防抖时长、电源模式、时区。
14. 上报周期、GPS 周期。

解析层需要区分不同设备模型的数据视图，但底层 TLV parser 应保持通用。

## 7. 设备状态与配置需求

### 7.1 UDS100

状态字段：

1. Name。
2. Version。
3. Power。
4. Battery。
5. Temperature。
6. Humidity。
7. Alarm Status。
8. Angle。
9. Slant。
10. Distance。
11. Overflow State。
12. Low Threshold。
13. High Threshold。
14. Latitude。
15. Longitude。
16. Report Period。
17. GPS Period。

配置字段：

| 字段 | 有效范围 |
| --- | --- |
| Low Threshold | `30-4500 mm` |
| High Threshold | `0` 或 `30-4500 mm` |
| Report Period | `1-1440 min` |
| GPS Period | `0` 或 `10-1440 min` |

### 7.2 DC200Family：DC200 / EPS100 / MPS100

状态字段：

1. Name。
2. Version。
3. Power。
4. Battery。
5. Parking Status。
6. Tamper Status。
7. Report Period。
8. Mag X。
9. Mag Y。
10. Mag Z。
11. Radar Spectrum。

配置字段：

| 字段 | 说明 |
| --- | --- |
| Report Period | `1-1440 min` |
| Work Mode | 融合模式、仅地磁、雷达优先 |

DC200Family 还存在校准流程，Android 端等待时间较长。iOS 必须将校准建模为长时间运行状态，不能按普通即时 ACK 处理。

### 7.3 SVC100

状态字段：

1. Name。
2. Version。
3. Power。
4. Battery。
5. Port 1 value state。
6. Port 1 insert state。
7. Port 1 pulse count。
8. Port 2 value state。
9. Port 2 insert state。
10. Port 2 pulse count。
11. Voltage out level。
12. Interface function。
13. Buffeting duration。
14. Timezone。
15. Report Period。

基础配置字段：

| 字段 | 说明 |
| --- | --- |
| Output Voltage | 与 Android 当前电压档位保持一致 |
| Port Function | 与 Android 当前端口功能值集保持一致 |
| Stable / Buffeting Duration | `1-255 s`，仅在端口功能对应模式下可编辑 |
| Power Mode | Auto / Manual |
| Timezone | 与 Android 当前时区数组保持一致 |
| Report Period | `1-1440 min` |

任务能力：

1. 实时任务。
2. 定时任务。
3. 删除任务。

协议层必须建模上述能力；v1 UI 是否开放见“待确认项”。

### 7.4 固件差异裁决

固件核对发现以下差异不能直接照抄 Android：

| 设备 | 固件行为 | iOS 处理要求 |
| --- | --- | --- |
| DC200Family | 上报周期接受 `0` 或 `1-1440`，当前 Android UI 只提供 `1-1440` | 产品确认是否开放 `0`；若隐藏，必须在需求中记录为 iOS 受限行为，并保留固件边界测试。 |
| SVC100 | 上报周期接受 `0` 或 `1-1440`，时区包含半时区编码 | 生成全边界测试向量；UI 是否开放 `0` 必须与 DC200Family 的策略一致。 |
| SVC100 | 实时任务在本地任务执行中会拒绝启动 | UI 和状态机必须表达 busy/rejected 状态。 |
| SVC100 | 固件存在 `0xF9` 直接阀门控制路径，当前 Android v1 UI 未暴露 | 登记为固件能力，产品确认是否进入 iOS。 |
| App 帧 CRC | Android 发送 CRC，Bootloader 校验 CRC；当前应用层处理函数未观察到 CRC 拒收逻辑 | iOS 继续发送合法 CRC，但测试文档不得声称应用层固件一定会校验 CRC，除非逐版本核实。 |

## 8. OTA 需求

### 8.1 基本要求

OTA 是 iOS v1 的强制能力，覆盖 UDS100、SVC100、EPS100、MPS100。

OTA 使用独立 Bootloader 帧：

```text
hkt(3) + len(2) + cmd(1) + packnum(2) + data(n) + crc(2) + bootloader(8)
```

后缀为：

```text
626F6F746C6F6164
```

即 ASCII `bootload`。

### 8.2 传输流程

1. 用户选择固件文件。
2. App 显示文件名和文件大小。
3. 用户触发升级。
4. App 发送命令 `0x01`，携带固件字节数。
5. Bootloader 请求数据包号。
6. App 每包发送 128 字节有效数据。
7. 最后一包不足 8 字节边界时，使用 `FF` 补齐。
8. 全部数据发送完成后，发送完成命令。
9. 设备执行升级并重启。
10. 设备重新广播。
11. App 重新连接设备。
12. App 读取新固件版本。
13. App 判定本次 OTA 成功。

### 8.3 UI 要求

OTA 页面必须显示：

1. 已选固件名。
2. 固件大小。
3. 当前阶段：待发送、传输中、完成包、等待重启、重新连接、版本确认。
4. 传输进度百分比。
5. 成功或失败结果。
6. 可重试入口。

进度条不能作为唯一成功判定依据。

### 8.4 OTA 验收标准

单次 OTA 判定成功必须同时满足：

1. 固件文件成功读取。
2. Bootloader 传输完整完成。
3. 设备升级后自动重启。
4. 设备重新广播。
5. App 能重新连接设备。
6. App 能读取到新固件版本。
7. 升级过程中 App 不崩溃。
8. 连接资源正确释放或恢复。

仅进度条到达 100%，不能判定 OTA 成功。

### 8.5 OTA 异常场景

首版至少验证：

1. 选择非预期文件或空文件。
2. 文件读取失败。
3. BLE 断开。
4. OTA 过程中 App 进入后台后返回前台。
5. OTA 过程中用户尝试返回。
6. OTA 过程中蓝牙被关闭。
7. 设备在 OTA 中失去响应。
8. 升级完成后设备未立即重新广播。
9. 重新连接失败。
10. 版本读取失败。

对上述场景，App 必须给出明确状态，不允许出现不可理解的无响应界面。

## 9. 辅助能力需求

### 9.1 Debug 原始命令

Android 设备页支持原始命令发送和响应查看。该能力对现场协议排查和 OTA 联调非常重要。

iOS 架构必须支持向当前连接设备写入原始 hex 命令，并记录原始 Indicate 数据。该能力是否作为 v1 主 UI 入口见“待确认项”。

### 9.2 日志

iOS 必须提供本地诊断日志，至少覆盖：

1. 扫描启动、停止、失败。
2. 目标设备发现。
3. 连接状态变化。
4. 服务发现。
5. Indicate 订阅结果。
6. 写入命令。
7. 收到响应。
8. OTA 阶段变化。
9. 错误和恢复动作。

日志不得包含用户敏感数据和密钥。

### 9.3 国际化

所有面向用户的文案必须通过 iOS 本地化资源管理，禁止在 SwiftUI view 或错误提示中硬编码中英文。

首版至少提供：

1. 简体中文。
2. English。

字段名、状态、错误、按钮、弹窗、OTA 阶段都必须纳入本地化。

### 9.4 权限与系统能力

iOS 必须处理：

1. Bluetooth 权限。
2. Bluetooth 被关闭。
3. 设备不支持 BLE。
4. App 进入后台导致扫描受限。
5. 长时间 OTA 期间的屏幕锁定风险。

如果使用后台保持、Local Notification 或其他系统能力，必须在实现前补充 iOS 平台评审。

## 10. 非功能需求

### 10.1 并发安全

iOS 不得复制 Android 当前全局可变状态模式。

要求：

1. BLE 会话状态集中在明确 owner 中。
2. OTA 状态机使用显式状态类型。
3. UI 只读取稳定快照或响应式状态。
4. BLE 回调、OTA 状态和 UI 更新之间的线程边界清晰。
5. 避免后台任务和 UI 同时写同一份可变模型。

### 10.2 错误处理

错误必须区分：

1. 用户输入错误。
2. 权限错误。
3. 蓝牙不可用错误。
4. 连接错误。
5. 服务或特征缺失。
6. 写入失败。
7. 协议 CRC 错误。
8. 响应格式错误。
9. OTA 文件错误。
10. OTA 超时或设备无响应。

不允许统一展示为“失败”。

### 10.3 性能

1. 扫描列表更新不得因高频 RSSI 变化导致 UI 卡顿。
2. 状态轮询和 UI 刷新必须有节流。
3. OTA 期间不得因日志或界面刷新阻塞 BLE 写入。
4. 大固件读取不得一次性构造无意义的大字符串副本。

### 10.4 可测试性

iOS 项目必须提供 XCTest：

1. CRC16 测试。
2. App 帧构造测试。
3. Bootloader OTA 帧构造测试。
4. 分包和末包补齐测试。
5. TLV 响应解析测试。
6. 设备名识别测试。
7. OTA 状态机测试。
8. 配置字段边界值测试。
9. 每个四类设备的关键命令测试。
10. 固件接受边界、拒绝边界和静默行为测试。

测试向量必须以固件源码为依据，并登记 `FW-REF-*`。Android 与 iOS 应使用 `shared/fixtures` 中的相同测试向量；只有固件不支持的 Android 行为必须单独标记为兼容性遗留或已裁决不实现。

## 11. 与 Android 能力盘点对应的处置

| 能力 | Android 现状 | iOS 处置 |
| --- | --- | --- |
| BLE 扫描 | 已实现 | v1 必须 |
| RSSI / 空名称过滤 | 已实现 | v1 必须 |
| 手工 DevEUI | 已实现 | 架构保留；UI 优先级待定 |
| 二维码 / 条码 | 已实现 | 架构保留；UI 优先级待定 |
| 连接与会话守护 | 已实现 | v1 必须 |
| 状态查询 | 已实现 | v1 必须 |
| 电源控制 | 已实现 | 协议层必须；UI 待定 |
| 校准 | 已实现 | 协议层必须；UI 待定 |
| UDS100 参数配置 | 已实现 | 协议层必须；UI 待定 |
| DC200Family 参数配置 | 已实现 | 协议层必须；UI 待定 |
| SVC100 基础配置 | 已实现 | 协议层必须；UI 待定 |
| SVC100 任务配置 | 已实现 | 协议层必须；UI 待定 |
| 时间同步 | 已实现 | 协议层必须；UI 待定 |
| OTA | 已实现 | v1 必须 |
| Debug 原始命令 | 已实现 | 架构保留；UI 待定 |
| 诊断日志 | 已实现 | v1 建议 |
| About / 语言切换 | 已实现 | v1 建议 |
| Excel 导出 | 代码存在但未发现调用 | 不作为已交付能力；如需要另立需求 |
| `DeviceFeatureConfig` | 未发现调用，且 MPS100 判断与实际解析不一致 | 不照抄；重新定义设备能力集 |

上表中的 Android 现状仅是兼容性参考。每一行进入开发前都必须补充固件证据链接；无法提供固件证据的能力只能保留在架构预留层，不能进入 v1 验收。

## 12. 验收矩阵

### 12.1 功能验收

| 编号 | 验收项 | 通过条件 |
| --- | --- | --- |
| A-00 | UI 设计基线 | 信息架构、关键流程、高保真稿、组件状态、中英文文案和 OTA 原型通过评审 |
| A-00b | 软件架构基线 | 《iOS 软件设计说明》通过评审，模块边界、状态机、能力集和测试方案完整 |
| A-00c | 固件追溯基线 | 四类设备的固件仓库、版本基线、协议函数、TLV 类型、命令行为、差异裁决和测试向量全部登记 |
| A-01 | 权限流程 | Bluetooth 权限拒绝、允许、关闭蓝牙时均有清晰状态 |
| A-02 | 扫描 | 能发现四类支持设备，RSSI 与名称显示正确 |
| A-03 | 过滤 | RSSI 阈值和空名称过滤生效 |
| A-04 | 连接 | 四类设备均可连接、发现服务并订阅 Indicate |
| A-05 | 状态查询 | 可读取基础状态且字段格式正确 |
| A-06 | 断开处理 | 设备断开时 App 状态正确并提示用户 |
| A-07 | 固件选择 | 文件名、大小显示正确，异常文件有提示 |
| A-08 | OTA | 四类设备 OTA 后重新连接并读取新版本 |
| A-09 | 日志 | BLE 和 OTA 关键事件可追踪 |
| A-10 | 稳定性 | 扫描、连接、状态轮询、OTA 切换过程中不崩溃 |

### 12.2 OTA 真机顺序

建议顺序：

1. MPS100
2. EPS100
3. UDS100
4. SVC100

每个设备至少执行：

1. 正常升级。
2. 升级后重启。
3. 重新广播确认。
4. 重新连接。
5. 新版本读取。
6. 异常路径中的 App 状态检查。

### 12.3 回归测试

发布前必须执行：

1. iOS 单元测试全部通过。
2. 四类设备固件追溯矩阵无未裁决差异。
3. 四类真机 OTA 验收通过。
4. 扫描、连接、状态查询回归通过。
5. 中英文界面检查通过。
6. 常见异常场景检查通过。
7. 性能和内存无明显异常增长。

## 13. 发布与打包

iOS 打包使用 Xcode Archive / `xcodebuild`，并依赖 Apple Developer Program 账号完成签名。

本阶段已确认拥有 Apple Developer Program 账号，但以下事项暂缓：

1. App ID 注册。
2. 证书创建。
3. Provisioning Profile。
4. 测试设备 UDID 注册。
5. Ad Hoc / TestFlight / App Store 分发方式选择。
6. `.ipa` 导出流程。

这些工作应在功能真机验证通过后配置。

## 14. 待确认项

以下问题必须在进入开发前闭合：

1. **参数配置 UI 是否进入 v1**：协议层必须实现，但主 UI 是否开放待定。
2. **状态展示字段范围**：是只展示版本、电量和 OTA 相关状态，还是按 Android 状态页完整展示。
3. **Debug 原始命令入口是否进入 v1**：建议至少保留内部调试入口。
4. **手工 DevEUI 与二维码入口是否进入 v1**：架构保留，UI 优先级待定。
5. **最低支持 iOS 版本**：需要确定 iOS 16、iOS 17 或其他基线。
6. **最低支持 iPhone 机型**：需要结合 CoreBluetooth 和产品使用场景确认。
7. **每类设备的标准测试固件**：需要提供旧版本、目标版本和必要时可重复验证的固件方案。
8. **OTA 后预期版本格式**：需要定义“新固件版本”的机器可读判定方式。
9. **分发方式**：TestFlight、Ad Hoc 或开发签名尚未选择。
10. **App 名称与 Bundle ID**：默认建议 `HKTBLETools` 和 `com.hkt.ble.bletools.ios`，需最终确认。

## 15. 里程碑建议

| 阶段 | 目标 | 退出条件 |
| --- | --- | --- |
| M0 | 需求闭合 | 本文件中的待确认项形成最终结论 |
| M1 | UI 设计基线 | 信息架构、关键流程、高保真稿、组件状态、中英文文案和 OTA 原型通过评审 |
| M2 | 软件架构重构设计 | 《iOS 软件设计说明》和 ADR 通过评审，明确分层、状态机、依赖注入和测试边界 |
| M3 | 固件协议与测试基线 | 四类设备固件能力核对完成；CRC、帧、TLV、OTA 分包和设备识别测试携带固件引用并通过 |
| M4 | BLE 骨架 | 真实 iPhone 可扫描、过滤、连接四类设备 |
| M5 | 状态会话 | 四类设备基础状态可读取并按设计稿展示 |
| M6 | OTA 优先交付 | MPS100 OTA 达到研发级验收标准 |
| M7 | 全设备 OTA | EPS100、UDS100、SVC100 OTA 验收通过 |
| M8 | UI 完整化 | 根据最终 v1 矩阵补齐配置、Debug、语言和诊断能力 |
| M9 | 发布准备 | 签名、分发、版本号、隐私权限说明和回归完成 |

## 16. 风险

| 风险 | 影响 | 缓解措施 |
| --- | --- | --- |
| UI 设计滞后或反复 | 功能开发被迫返工 | UI 设计基线未通过前，禁止实现 SwiftUI 功能页面 |
| 架构重构不足 | 后续补配置和任务能力时引发结构性改造 | 先完成《iOS 软件设计说明》评审，再开发业务功能 |
| iOS 与固件理解不一致 | 配置失败、状态解析错误，甚至 OTA 误判 | 每个功能和测试向量登记固件源码引用；固件差异未裁决前不进入验收 |
| CoreBluetooth 与 Android BLE 行为差异 | 写入节奏、MTU、Indicate 时序不同 | 先做 BLE 抽象层和真实设备联调 |
| OTA 是不可逆现场操作 | 固件升级失败可能造成设备不可用 | 先用 MPS100 打通，再扩展其他设备 |
| iOS 后台限制 | 长时间 OTA 可能被系统干扰 | 首版以前台 OTA 为主，测试锁屏、后台返回和蓝牙关闭 |
| 需求范围继续变化 | 架构反复调整 | 按能力域建模，UI 通过 feature surface 控制 |
| 设备固件行为不一致 | 同名设备或不同批次表现差异 | 建立真机矩阵并记录固件版本 |
| 签名和分发延后 | 内部测试受阻 | 功能开发不阻塞，但真机验证前需准备开发签名 |

## 17. 结论

iOS v1 应按“原生架构完整、UI 里程碑可控”的方式开发：

1. 不做跨平台捷径。
2. 不把协议层窄化为 OTA 工具。
3. UI 设计先于 SwiftUI 功能实现。
4. iOS 软件架构基于 Android 能力清单重新设计，不复制 Android 全局状态实现。
5. 每款设备的功能、协议、测试用例和软件设计必须追溯到对应固件代码。
6. 首版强制交付四类设备 OTA。
7. OTA 成功必须以设备重启、重连和新版本读取为准。
8. 参数配置、SVC 任务、Debug 等能力先进入协议和设备模型，再按最终 UI 矩阵开放。
