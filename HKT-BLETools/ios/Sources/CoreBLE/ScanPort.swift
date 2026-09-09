import Foundation

/// 扫描发现的设备（去重后）。identifier 为系统级稳定标识（S-7 冷启动恢复依赖）。
public struct DiscoveredDevice: Identifiable, Equatable, Sendable {
    public let name: String
    public let identifier: UUID
    public let rssi: Int

    public init(name: String, identifier: UUID, rssi: Int) {
        self.name = name
        self.identifier = identifier
        self.rssi = rssi
    }

    public var id: UUID { identifier }

    /// SP-1 名称前缀：广播名前 3 字符；无名称/不足 3 字符 → nil（未命名设备天然排除）。
    public var prefix: String? { Self.prefix(of: name) }

    public static func prefix(of name: String?) -> String? {
        guard let name else { return nil }
        let p = name.trimmingCharacters(in: .whitespaces).prefix(3).uppercased()
        return p.count == 3 ? String(p) : nil
    }

    /// v1 支持的设备名称前缀（R-1，用户 2026-09-08）。
    public static let supportedPrefixes: Set<String> = ["MPS", "SVC", "UDS", "EPS"]
}

/// 扫描入列规则（R-1/SP-1）：RSSI ≥ 阈值 && 广播名前缀命中所选集合；无名称天然排除。
/// 过滤规则的唯一出处：SystemCentral（真蓝牙）与 MockCentral（假蓝牙源）共用，保证两端行为一致。
public struct ScanOptions: Equatable, Sendable {
    public var allowedPrefixes: Set<String>
    public var rssiThreshold: Int

    public init(allowedPrefixes: Set<String> = DiscoveredDevice.supportedPrefixes, rssiThreshold: Int = -80) {
        self.allowedPrefixes = allowedPrefixes
        self.rssiThreshold = rssiThreshold
    }

    public func isListable(name: String?, rssi: Int) -> Bool {
        guard rssi >= rssiThreshold else { return false }
        guard let prefix = DiscoveredDevice.prefix(of: name) else { return false }
        return allowedPrefixes.contains(prefix)
    }
}

/// 蓝牙传输端口（SD 架构测试缝）：真蓝牙（SystemCentral）与可编程假蓝牙源（MockCentral）共用同一接口。
public protocol BluetoothPort: AnyObject {
    /// 激活：创建系统管理器并持续回报能力状态（不扫描）。App 启动即调用——
    /// 真机上这一步触发系统蓝牙权限弹窗（R-24）。
    func activate(onUpdate: @escaping @Sendable (BLEAvailability) -> Void)

    /// 开始扫描：就绪后自动发现；入列规则 = ScanOptions（阈值+前缀+无名称排除）。
    func startScan(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)

    /// 停止扫描（能力状态回报继续，R-32 驻留健康检测依赖）。
    func stopScan()

    /// 连接三阶段（P-02/SP-4）：事件为"完成语义"，驱动 ConnectOrchestrator 推进；
    /// 超时由编排器按 ConnectBudget 判定，与传输实现无关。
    func connect(to device: DiscoveredDevice, events: @escaping @Sendable (ConnectEvent) -> Void)

    /// 取消底层连接尝试（编排器取消后由 App 层调用；实现须停止推进事件）。
    func cancelConnect()
}
