import CoreBluetooth
import Foundation

/// 扫描发现的设备（去重后）。identifier 为系统级稳定标识（S-7 冷启动恢复依赖）。
struct DiscoveredDevice: Identifiable, Equatable, Sendable {
    let name: String
    let identifier: UUID
    let rssi: Int

    var id: UUID { identifier }

    /// SP-1 名称前缀：广播名前 3 字符；无名称/不足 3 字符 → nil（未命名设备天然排除）。
    var prefix: String? { Self.prefix(of: name) }

    static func prefix(of name: String?) -> String? {
        guard let name else { return nil }
        let p = name.trimmingCharacters(in: .whitespaces).prefix(3).uppercased()
        return p.count == 3 ? String(p) : nil
    }

    /// v1 支持的设备名称前缀（R-1，用户 2026-09-08）。
    static let supportedPrefixes: Set<String> = ["MPS", "SVC", "UDS", "EPS"]
}

/// 系统中心管理器封装：扫描入列规则 = RSSI ≥ 阈值 && 前缀命中所选集合（R-1/SP-1），
/// 按 identifier 去重并更新 RSSI；回调统一切回主线程（queue = .main）。
final class SystemCentral: NSObject, CBCentralManagerDelegate {
    struct Options: Sendable {
        var allowedPrefixes: Set<String> = DiscoveredDevice.supportedPrefixes
        var rssiThreshold: Int = -80
    }

    private var central: CBCentralManager?
    private var options = Options()
    private var seen: [UUID: DiscoveredDevice] = [:]

    /// 主线程回调：能力状态变化 / 当前去重发现列表。
    var onUpdate: (@Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)?

    func start(options: Options) {
        self.options = options
        guard central == nil else { return }
        central = CBCentralManager(delegate: self, queue: .main)
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        emit(availability: BLEAvailability(central.state))
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let name = peripheral.name ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
        guard RSSI.intValue >= options.rssiThreshold,
              let name,
              let prefix = DiscoveredDevice.prefix(of: name),
              options.allowedPrefixes.contains(prefix) else { return }
        let device = DiscoveredDevice(name: name, identifier: peripheral.identifier, rssi: RSSI.intValue)
        seen[device.identifier] = device
        emit(availability: BLEAvailability(central.state))
    }

    private func emit(availability: BLEAvailability) {
        let list = seen.values.sorted { $0.rssi > $1.rssi }
        onUpdate?(availability, list)
    }
}
