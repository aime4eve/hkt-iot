import Foundation

/// 可编程假蓝牙源（轨道一：无真机/无签名下的全流程开发与测试载体，对应原型"场景演示"思路）。
/// 行为与真蓝牙实现共用同一套 ScanOptions 规则，保证模拟与真实过滤一致。
public final class MockCentral: BluetoothPort {
    public private(set) var availability: BLEAvailability = .initializing
    public private(set) var devices: [DiscoveredDevice] = []
    public private(set) var isScanning = false

    private var options: ScanOptions?
    private var onUpdate: (@Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)?

    public init() {}

    public func start(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void) {
        self.options = options
        self.onUpdate = onUpdate
        isScanning = true
        emit()
    }

    public func stopScan() {
        isScanning = false
    }

    /// 场景注入：设置系统能力状态（如 .ready / .denied / .poweredOff）。
    public func setAvailability(_ availability: BLEAvailability) {
        self.availability = availability
        emit()
    }

    /// 场景注入：模拟一次广播发现（仅扫描中且通过过滤规则时入列）。
    public func discover(name: String, identifier: UUID, rssi: Int) {
        guard isScanning, let options else { return }
        guard options.isListable(name: name, rssi: rssi) else { return }
        devices.removeAll { $0.identifier == identifier }
        devices.append(DiscoveredDevice(name: name, identifier: identifier, rssi: rssi))
        devices.sort { $0.rssi > $1.rssi }
        emit()
    }

    /// 场景注入：清空发现列表（如设备消失/轮询停摆恢复失败）。
    public func clearDevices() {
        devices = []
        emit()
    }

    private func emit() {
        onUpdate?(availability, devices)
    }
}
