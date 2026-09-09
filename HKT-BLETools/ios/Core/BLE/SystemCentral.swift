import CoreBLE
import CoreBluetooth

/// 真蓝牙实现（SD 架构的 CoreBluetoothAdapter 位，实现 CoreBLE 的 BluetoothPort）。
/// 过滤规则以 ScanOptions 为唯一出处，与 MockCentral（假蓝牙源）行为一致。
final class SystemCentral: NSObject, CBCentralManagerDelegate {
    private var central: CBCentralManager?
    private var options = ScanOptions()
    private var seen: [UUID: DiscoveredDevice] = [:]

    /// 主线程回调：能力状态变化 / 当前去重发现列表。
    var onUpdate: (@Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)?

    func start(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void) {
        self.options = options
        self.onUpdate = onUpdate
        guard central == nil else { return }
        central = CBCentralManager(delegate: self, queue: .main)
    }

    func stopScan() {
        central?.stopScan()
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        emit(availability: BLEAvailability(central.state))
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        let name = peripheral.name ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
        guard options.isListable(name: name, rssi: RSSI.intValue), let name else { return }
        let device = DiscoveredDevice(name: name, identifier: peripheral.identifier, rssi: RSSI.intValue)
        seen[device.identifier] = device
        emit(availability: BLEAvailability(central.state))
    }

    private func emit(availability: BLEAvailability) {
        let list = seen.values.sorted { $0.rssi > $1.rssi }
        onUpdate?(availability, list)
    }
}

extension SystemCentral: BluetoothPort {}
