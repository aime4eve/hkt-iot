# HKT BLETools iOS v1 —《iOS 软件设计说明》

| 项 | 内容 |
| --- | --- |
| 文档版本 | v1.0 Draft（M2 评审对象） |
| 日期 | 2026-09-05 |
| 上游 | [架构设计大纲](2026-09-03-ios-v1-architecture-outline.md)（已评审，P1 修正完成）扩写而成；事实断言以大纲 §7.1/§8.1 固件证据为准 |
| 下游 | Features 实现蓝图；M3 起按本文档 + [测试用例基线](2026-09-04-ios-v1-stability-continuity-test-cases.md) 开发；实现规格条目与 [设计需求与实现规格统一清单](../design/2026-09-06-ios-v1-requirements-spec-checklist.md)（SP-*）同源 |
| 基线 | nix/ios-v1-design；iOS 17+（Q5）；仅 iPhone；Bundle ID `com.hkt.ble.bletools.ios`（Q10） |
| 状态 | 待评审。本文档评审通过 = M2 关闭；在此之前不创建 Xcode 工程、不实现业务代码 |

## 1. 模块边界与依赖方向（需求 §3.4-1）

```text
App(组合根/路由) → Features(View+VM) → Core/Session → Core/Protocol · Core/OTA · Core/Device → Core/BLE(Abstraction) → Core/BLE(Adapter)
```

编译期硬约束（评审不通过项）：`import CoreBluetooth` 仅允许 `Core/BLE/CoreBluetoothAdapter.swift`、`Core/BLE/CBPeripheralID.swift`；`import SwiftUI` 仅允许 `App/`、`Features/`、`DesignSystem/`；`Core/` 各子目录互不横向 import（仅经 public 接口）；Core 全域禁 import SwiftUI/UIKit。

模块清单（目录 = 需求 §3.2 既定）：

| 模块 | 目录 | 对外暴露 | 依赖 |
| --- | --- | --- | --- |
| CompositionRoot | `App/` | `HKTBLEToolsApp`、`AppEnvironment` | 全部（只读构造） |
| UI | `Features/` + `DesignSystem/` | 页面、`@Observable` ViewModel、Token 组件 | Session 接口、Device 模型 |
| Session | `Core/Session/` | `DeviceSession`（actor，经 `SessionServicing` 协议暴露） | Protocol、OTA、Device、BLE 抽象 |
| Protocol | `Core/Protocol/` | 纯函数编解码 + 值类型 | 无（仅 Foundation） |
| OTA | `Core/OTA/` | `OTAEngine`（actor）、`OTATransferPlanner`（纯函数） | Protocol |
| Device | `Core/Device/` | `DeviceRegistry`、Profile/Capability/字段描述符 | 无 |
| BLE 抽象 | `Core/BLE/BleTransport.swift` 等 | `BleTransport` 协议、`BleEvent`/`BleError` 值类型 | 无 |
| BLE 适配 | `Core/BLE/Adapter/` | `CoreBluetoothAdapter: BleTransport` | CoreBluetooth |
| 支撑 | `Support/` | `LogService`、`Persistence`、`AppError` | Foundation |

## 2. 各层职责与关键类型（需求 §3.4-2；Swift 签名为设计草图）

### 2.1 BLE 抽象层

```swift
protocol BleTransport: AnyObject, Sendable {
    var events: AsyncStream<BleEvent> { get }          // 多订阅（内部广播）
    var state: AsyncStream<BleSystemState> { get }     // poweredOn/off/unauthorized/unsupported/resetting
    func startScan(_ filter: ScanFilter) async throws  // filter: 名称白名单+RSSI 阈值+空名过滤+去重窗口
    func stopScan() async
    func connect(to id: PersistentPeripheralID) async throws -> ConnectionInfo
    // ConnectionInfo = WriteMax(Int), IndicateReady; connect 内部完成发现服务→订阅→MTU 读取，任一步超时抛 BleError
    func write(_ data: Data) async throws              // withoutResponse + peripheralIsReady 背压，见 §5.3
    func disconnect(from id: PersistentPeripheralID) async
}

enum BleEvent: Sendable {
    case discovered(DiscoveredDevice)        // name(原始广告名), id, rssi, timestamp
    case connectionChanged(id: PersistentPeripheralID, phase: ConnectionPhase)
    case indication(id: PersistentPeripheralID, data: Data)
    case writeReady(id: PersistentPeripheralID)
}
enum ConnectionPhase: Sendable { case connecting, discovering, subscribing, ready, lost(expected: Bool) }
```

Adapter 实现要点：`CBCentralManagerDelegate`/`CBPeripheralDelegate` 事件先收敛到内部 `DispatchQueue`（串行），转换为值事件后投递 `AsyncStream`；扫描用 `[.serviceUUIDs]` 不可用（设备广播自定义 service 需验证，M4 确认；默认全量广播+名称过滤）；identifier 统一封装 `PersistentPeripheralID(UUID)`，禁止 CBPeripheral 类型外泄。

### 2.2 协议引擎（纯函数，无 IO 无状态）

```swift
enum CRC16 { static func ccitt(_ data: Data) -> UInt16 }             // poly 0x8408 反射, init 0, KERMIT；向量 shared/fixtures/crc16.json
enum HKTFrameEncoder {
    static func appFrame(cmd: UInt8, packNum: UInt8, data: Data) -> Data   // hkt+packNum+len(2 BE, cmd+data)+cmd+data+crc
}
enum HKTResponseParser {
    // 设备回包 = hkt(3)+0x00+seq(1)+记录流；记录 = type(1)+定长 value，无长度字节/命令字节/CRC。
    // 按家族类型表定长分发；未知类型无法跳过 → throw dataAbnormal（S-6 M3 修订，TC-BC-011 同步修订）。
    static func parse(_ data: Data, family: DeviceFamily) throws -> [TLVEntry]
}
public struct TLVEntry: Sendable { let type: UInt8; let value: Data }
enum SignedValueDecoder { static func s24/s32/s16(_ v: Data) -> Int }     // 符号位规则，向量锁定；多字节一律大端
enum CommandCode { static let query: UInt8 = 0xFF /* …0xFE/0xFD/0x02..0x06/0x01 */ }
// 0x06 时间同步帧怪癖：len 字段=4（仅 data，非 cmd+data），固件卫语句 data[5]==4（三固件一致；Android 同款）；时间偏移 UDS100/DC200=固定 UTC+8，SVC100=按配置时区；stamp==0 静默忽略；成功均 ACK（traceability §2.1-10）；iOS 口径=发送手机当前 Unix 秒（Date().timeIntervalSince1970 取整），App 不自行加减时区——『同步手机时间到设备』（用户 2026-09-06 澄清）
```

M3 已按固件核实并落地 `ios/Package.swift`（CoreProtocol/CoreOTA/CoreDevice + 29 个 XCTest 全绿）：三家族类型定长表、ACK 记录 `0xFF 0xFF`、0xFF/0xFE/0xFD/0x01 的 4 字节填充载荷（电源字节在 payload[3]）、DC200Family 配置=周期(2B BE)+模式(0-2)、UDS100 配置=周期+GPS+低/高阈值（4×2B BE，非法整包静默拒绝）、SVC100 配置=电压+端口+稳定+自动电源+时区+周期（时区非法无 ACK）、时区编码 25=+03:30/26=+05:30。

固件值证据（TLV/命令/边界）全部引自 firmware-traceability.md §4，本 spec 不重复罗列；实现时以 `Core/Protocol/TLV.swift` 常量表登记并逐项挂 FW-REF。

### 2.3 设备模型层

```swift
struct DeviceProfile: Sendable {
    let family: Family                        // .uds100 / .dc200Family / .svc100
    let advertisedNames: Set<String>          // dc200Family: ["DC200","EPS100","MPS100"]
    let capabilities: DeviceCapabilities      // OptionSet
    let statusFields: [FieldDescriptor]       // 驱动 P-03 网格（key→本地化、单位、渲染器）
    let configFields: [ConfigFieldDescriptor] // 驱动 P-05（含固件边界 min/max/0 值语义）
}
struct DeviceRegistry { static func match(advertisedName: String) -> DeviceProfile? }  // 不识别名 → nil（不可连接）
```

能力矩阵 v1：三 family 均含 `ota/statusQuery`；`basicConfig/powerControl/calibration/timeSync/svcTasks` 协议层建模、UI 随 FeatureSurface；`valveControl(0xF9)` 仅登记。UI 全域禁止 `if name == "…"` 分支（评审硬性要求 7）。

### 2.4 Session 层（Use Case / Device Session）

```swift
actor DeviceSession: SessionServicing {
    private let transport: BleTransport
    private var phase: ConnectionPhase            // 唯一可变会话状态 owner
    private var pending: PendingCommand?          // 单飞：cmd+超时任务+continuation
    func connect(to d: DiscoveredDevice) async throws
    func startPolling() / stopPolling(policy: PollPolicy)   // 1s 周期 0xFF；见 §5.4
    func send(cmd: UInt8, payload: Data, timeout: Duration = .seconds(2)) async throws -> [TLVEntry]
    func rawExchange(_ hex: String) async throws -> String   // P-06 Debug 旁路（仍经编码器，不发裸字节）
    func beginOTA(file: FirmwareFile, expectedVersion: FirmwareVersion) async throws  // 授权移交 OTAEngine
    func handle(_ event: BleEvent) async                     // transport 事件唯一入口
    func shutdown() async                                    // 全量清理（断开/停轮询/取消看门狗）
}
```

规则：一条命令"编码→写入→等响应→超时"全程单飞，响应按 cmd 关联 `pending`；用户命令优先，轮询让位（轮询帧被跳过即视为该周期无数据，UI 显示最后有效值+时间戳）。
- **同页刷新保持滚动位置与输入焦点**：轮询驱动的刷新只更新数据，不得重建用户正在查看/编辑区域的焦点与滚动位置（原型已实现并经浏览器验证；Android V3.17/V3.21 的 Spinner/校验回归同属此类教训，见 PR #4）。

### 2.5 OTA 层（状态机 §6；纯函数分包 §2.2 风格）

```swift
actor OTAEngine {
    enum State: Sendable { case idle, fileValidated, notifying, transferring(requested: UInt16, sent: UInt16),
        finalPadding, completionSent, waitingReboot, waitingAdvertisement, reconnecting(attempt: Int),
        verifyingVersion, success(OTASummary), failed(OTAError) }
    private(set) var state: State                  // 每次转移发布 OTAResult/OTAResport 快照
    func run(session: DeviceSession, file: FirmwareFile, expected: FirmwareVersion) async
}
struct OTATransferPlanner {                        // 纯函数
    static func dataFrame(packetIndex: UInt16, file: Data) -> Data  // 128B/包，包号自 0x0000，末包 FF 补齐 8 字节边界
    static func validate(_ file: FirmwareFile) throws -> ValidatedFirmware  // 空/超大/后缀/型号前缀弱校验(Q7)
}
```

### 2.6 ViewModel 层（`@MainActor` + `@Observable`）

`ScanViewModel / DeviceDetailViewModel / OTAViewModel / DebugViewModel / SettingsViewModel`：只消费 Session/OTA 快照流（`AsyncStream<Snapshot>`），对外暴露只读属性与意图方法；不持有 CoreBluetooth/Session 内部可变对象；页面状态与设计稿状态一一对应（§10）。

## 3. 关键时序

**连接建立**（P-02 驱动）：`connect(id)` → transport 连接（超时 10s）→ 发现服务（5s）→ 订阅 Indicate（5s）→ 读 `maximumWriteValueLength` → `ready` → Session 自动发首轮 `0xFF` → P-03。任一步超时/失败 → `connectionChanged(.lost)` → 错误映射。

**状态轮询**：`ready` 后每 1s 投递 `0xFF`（用户命令插入时顺延一拍）；响应 TLV → 快照 → ViewModel 节流（≥500ms 合并）→ UI。

**OTA 全流程**（P-04 六阶段）：

```text
beginOTA → planner.validate（含 MTU≥160 检查，不足→otaFile 错误，OTA 不发起）
→ 会话暂停轮询/拒绝新命令（单飞移交）
→ 发 0x01+固件大小 → 等 ACK(1,0)          // InfoUartAck: 68 6B 74 | cmd(1) | count(2) | bootload(8)
→ loop: 收 ACK(cmd=0x02, n) → 发 dataFrame(n)   // 包号自 0，永远只发被请求的包（停等）
        单包 3s 超时 → 等待设备秒级 nudge ACK 重发该包；连续 ≥15s 无任何 ACK → failed(.otaTimeout)
        收 ACK(cmd=0x01, 0) → 设备已复位：从包 0 重传（≤2 次，升级报告记录）
        (packCount % 16 == 0) 预期页写延迟（2KB 页擦写）→ 不作错误
→ 末包 FF 补齐 → 发 0x03 → 收 ACK(cmd=0x03, 0)   // 传输完成
→ waitingReboot(T1=30s) → waitingAdvertisement(T2=60s，按名扫描发现)
→ reconnecting(T3=15s×2 次；先 retrievePeripherals，失败回退同名短扫描)
→ verifyingVersion: 0xFF 读 TLV 0x01 == expected → success；不符/超时 → failed（结果页显示旧→实际版本）
→ 会话恢复轮询、释放单飞；生成 OTASummary（各阶段起止/耗时/包数/重置次数）
```

T1/T2/T3 初值为设计值，M6 真机标定后定稿（登记变更）。

## 4. 并发模型与线程边界（需求 §3.4-10/§10.1）

| 可变状态 | 唯一 owner | 访问方式 |
| --- | --- | --- |
| 连接相位/pending 命令 | `DeviceSession` actor | await 方法/事件流 |
| OTA 状态 | `OTAEngine` actor | 状态快照流 |
| 扫描结果集 | `ScanSession`（Session 内部） | 快照流 |
| 页面状态 | 各 ViewModel（@MainActor） | SwiftUI 绑定 |
| 设置/最近设备 | `Persistence`（串行队列） | 协议注入 |

规则：跨层只传 Sendable 值快照；BLE 系统回调→Adapter 串行队列→值事件→actor mailbox→MainActor 快照，全链路无共享可变对象；禁用 `DispatchSemaphore` 跨 actor 等待；`AsyncStream` 的 `continuation` 仅在 owner actor 内 finish。

## 5. 传输细节（与固件约束对齐，证据见大纲 §7.1/§8.1）

1. **写入方式**：全部 `.withoutResponse` + `peripheralIsReady(for:)` 背压；协议帧一次性成帧发出，不跨写拆帧。
2. **MTU 前置**：OTA 发起前校验 `peripheral.maximumWriteValueLength(for: .withoutResponse) ≥ 160`（146B OTA 帧+余量），不足 → 拒绝发起（P-04 入口禁用 + `ota.mtu.blocked` 提示）。
3. **50ms 帧窗口**：成帧单写天然满足；`writeReady` 驱动连发时帧间不插入延时，但**不同协议帧**之间以"上一帧响应/ACK"为界（停等+单飞），不存在背靠背双帧。
4. **固件帧事实**：App 帧 `hkt+packNum+len+cmd+data+crc`；Boot 帧 `hkt+len+cmd+packNum(2)+data+crc+bootload(8)`；设备 ACK 14 字节 `68 6B 74 | cmd | count(2) | bootload`；首 ACK=(0x02, 0x0000)。

## 6. 错误体系（需求 §10.2 十类）

```swift
enum AppError: Error, Sendable {
    case input(field: String, reason: String)
    case permission(PermissionKind)                    // bluetooth
    case bluetoothUnavailable(BleSystemState)
    case connection(reason: ConnectionFailure)         // timeout/failed/lost
    case serviceMissing
    case writeFailure(underlying: String)
    case crcMismatch
    case responseFormat(detail: String)                // 含未知 TLV 不报此错（S-6 仅记日志）
    case otaFile(OTAFileIssue)                         // empty/tooLarge/badSuffix/modelMismatch
    case otaTimeout(stage: OTAStage)
}
```

每 case 绑定：本地化 key（文案表 §2.5）+ 建议动作 + UI 呈现形态（横幅/页面态/弹窗，映射 UI 计划 §5.1）；禁止 `NSError` 穿透到 ViewModel。

## 7. FeatureSurface 与 UI 显隐（需求 §3.4-7/8）

```swift
struct FeatureSurface: Sendable {
    var basicConfigUI = false      // M8 按 Q1 置 true
    var debugUI = false            // 关于页连点版本 7 次置 true；dev 构建初始 true
    var targetedScanUI = true      // 2026-09-07 用户裁决选 B：定位进 v1（翻案 Q4）；R-2/SP-2
    var barcodeUI = true           // 扫码定位依赖二维码/条码识别（需相机权限）
    var calibrationUI = false      // Q1/P-09 预案
    var languageSwitchUI = true
    var diagnosticLogUI = true
}
```

入口可见性 = `profile.capabilities ∩ surface`；组合根构造后只读传递；运行期唯一可变项 `debugUI`（用户触发），持久化到 UserDefaults。

## 8. 持久化与日志（S-7、需求 §9.2）

- `Persistence` 协议（组合根注入）：`recentDevices`（Application Support/`recent-devices.json`，identifier+名称+时间，上限 10）、`otaReports`（`ota-reports/<date>-<device>.json`，含中断记录）、设置经 `UserDefaults` 包装（RSSI 阈值/空名过滤/语言/debug 开关）。Core 层不触盘，全部经协议。
- `LogService`：OSLog（开发）+ 环形内存缓冲（512 条）落盘 `diagnostics/`；九类事件枚举打点；导出 = 环形缓冲+报告汇总为带环境头（App 版本/构建、机型、iOS、语言）的 `.txt` 走 ShareSheet；白名单脱敏（仅设备名/identifier 后缀）。

## 9. 本地化（需求 §9.3）

String Catalog `Localizable.xcstrings`，默认 zh-Hans 全量 + en；key 规范与核心文案见 [Token 与文案表](../design/2026-09-05-ios-v1-design-tokens-copy.md)；应用内切换 = 自定义 `locale` 存 UserDefaults + 根视图重建；构建期以脚本检查 `.xcstrings` 覆盖率（zh 缺失即报错）。

## 10. UI 状态 ↔ ViewModel 状态映射（评审通过标准 5）

| 页面状态（UI 计划 §3/线框） | ViewModel 来源 |
| --- | --- |
| P-01 ⑤⑥⑦⑧ | `ScanViewModel.scanPhase` + `discovered: [DiscoveredDevice]`（transport 事件） |
| P-02 ①~⑤ | `DeviceDetailViewModel.connectPhase`（ConnectionPhase 映射） |
| P-03 ①②②b③④⑤ | `sessionPhase` + `lastSnapshotTime` + `lastError(AppError)` + `upgradeModeDetected`（0xFF 超时+无版本） |
| P-04 六阶段/成功/失败 | `OTAViewModel.state: OTAEngine.State` + `progress` 快照；返回保护 = `state ∈ {transferring…reconnecting}` |
| P-06 | `DebugViewModel.lines: [DebugLine]`（rawExchange 记录） |
| P-07/P-08 | `SettingsViewModel`（Persistence 只读投影）；P-08 由 `BleSystemState`+权限查询驱动 |

## 11. 依赖注入与组合根（硬性要求 9）

`AppEnvironment`（唯一组合点）按序构造：`CoreBluetoothAdapter → LogService/Persistence → DeviceSession → OTAEngine → FeatureSurface → ViewModels`；全部构造注入协议类型；SwiftUI 侧以 `.environment(...)` 下发；Preview/测试注入 `MockBleTransport`（脚本化事件回放）。无服务定位器/无全局单例。

## 12. 测试方案（需求 §10.4，落地 [用例基线](2026-09-04-ios-v1-stability-continuity-test-cases.md)）

XCTest 分层：`CRC16Tests`（fixtures 向量）/ `FrameCodecTests` / `TLVParserTests`（含未知 TLV 跳过）/ `SignedValueDecoderTests` / `OTATransferPlannerTests`（包 0 起、FF 补齐、146B 算术）/ `OTAEngineTests`（Mock 回放：正常流、nudge 重发、15s 判失败、ACK(1,0) 重传 ≤2、版本不符）/ `DeviceRegistryTests` / `ConfigBoundaryTests`（M3 向量）/ `ErrorMappingTests`。真机用例按用例基线 TC-ST/TC-BC 执行，M3 登记向量与 FW-REF 后进入追溯矩阵。

## 13. ADR 正式化

大纲 §12 的 ADR-001~010 在本文档即为定稿文本（含评审修正：包号 0 起、CRC 向量、超时次序、睡眠语义）。新增：**ADR-011** OTA 写入统一 `.withoutResponse`+背压、MTU≥160 前置；**ADR-012** v1 持久化用沙盒 JSON/UserDefaults，不引数据库。

## 14. 实施顺序（M3 起，均待评审放行）

| 里程碑 | 交付 | 依赖本文档章节 |
| --- | --- | --- |
| M3 | 协议层 + fixtures 扩充（含 `123456789→2189`）+ 向量登记 + `ble_sleep_mode`/模组参数/EPS100 基线核对 | §2.2/§12 |
| M4 | Adapter + Session 连接/扫描骨架（真机：扫描→连接→订阅四类设备） | §2.1/§2.4/§5 |
| M5 | 轮询 + P-03 状态页（依 UI 设计基线放行） | §2.6/§10 |
| M6 | OTA 全流程 + MPS100 真机验收（T1/T2/T3 标定） | §2.5/§3 |
| M7 | EPS100/UDS100/SVC100 OTA；M8 配置/Debug/语言补齐（Q1）；M9 签名分发 | §7 |

## 15. 评审核对表

需求 §3.4 十项覆盖：1→§1；2→§2；3→§2.3；4→§2.4/§2.2；5→§2.5/§3；6→§4；7→§7；8→§8/§9；9→§11；10→§13。§3.4 硬性要求 10 条逐条可追溯；评审通过标准 5 条：§1（新设备）、§7（新功能不动 Adapter）、§12（OTA 无真机可测）、§4/§8（断线路径可追踪）、§10（UI 映射完整）。
