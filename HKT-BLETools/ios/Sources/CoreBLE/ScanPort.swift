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

/// 蓝牙传输端口（SD 架构测试缝）：真蓝牙（SystemCentral）与可编程假蓝牙源（MockCentral）共用同一接口，
/// 连接/会话接口随会话里程碑在此扩展。
public protocol BluetoothPort: AnyObject {
    func start(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)
    func stopScan()
}
