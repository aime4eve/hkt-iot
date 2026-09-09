import CoreBLE
import CoreBluetooth

/// 真蓝牙实现（SD 架构的 CoreBluetoothAdapter 位，实现 CoreBLE 的 BluetoothPort）。
/// 过滤规则以 ScanOptions 为唯一出处，与 MockCentral（假蓝牙源）行为一致。
/// 连接三阶段：didConnect → 发现 HKT 服务 → 订阅 Indicate，逐段上报完成事件（SP-4 预算在编排器）。
final class SystemCentral: NSObject, CBCentralManagerDelegate, CBPeripheralDelegate {
    private var central: CBCentralManager?
    private var options = ScanOptions()
    private var seen: [UUID: DiscoveredDevice] = [:]
    private var peripherals: [UUID: CBPeripheral] = [:]
    private var isScanning = false

    private var onAvailability: (@Sendable (BLEAvailability) -> Void)?
    private var onScanUpdate: (@Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)?
    private var connectEvents: (@Sendable (ConnectEvent) -> Void)?
    private var connectingPeripheral: CBPeripheral?
    private var indicateCharacteristic: CBCharacteristic?
    private var writeCharacteristic: CBCharacteristic?

    /// 已连接会话的外设与写特征（1s 轮询/命令写入由会话层使用）。
    private(set) var connectedPeripheral: CBPeripheral?
    private(set) var connectedWriteCharacteristic: CBCharacteristic?

    /// 会话层收帧回调（Indicate 数据，主线程投递）。
    public var onReceiveFrame: (@Sendable (Data) -> Void)?
    /// 已连接会话断开回调（会话层据此置停摆/触发 R-6）。
    public var onLinkDisconnected: (@Sendable () -> Void)?

    // MARK: - BluetoothPort

    func activate(onUpdate: @escaping @Sendable (BLEAvailability) -> Void) {
        onAvailability = onUpdate
        if central == nil {
            central = CBCentralManager(delegate: self, queue: .main)   // 真机：此处触发系统蓝牙权限弹窗
        }
        guard let central else { return }
        emit(availability: BLEAvailability(central.state))
    }

    func startScan(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void) {
        self.options = options
        onScanUpdate = onUpdate
        isScanning = true
        scanIfReady()
    }

    func stopScan() {
        isScanning = false
        central?.stopScan()
    }

    func connect(to device: DiscoveredDevice, events: @escaping @Sendable (ConnectEvent) -> Void) {
        if let peripheral = peripherals[device.identifier] {
            startConnect(peripheral, events: events)
        } else if let central,
                  let peripheral = central.retrievePeripherals(withIdentifiers: [device.identifier]).first {
            // 冷启动直连兜底（S-7 最近设备）：系统 identifier 持久，无需重新扫描
            peripherals[device.identifier] = peripheral
            startConnect(peripheral, events: events)
        } else {
            events(.failed(.connectionLost))
        }
    }

    func cancelConnect() {
        if let peripheral = connectingPeripheral {
            central?.cancelPeripheralConnection(peripheral)
        }
        connectEvents = nil
        connectingPeripheral = nil
        indicateCharacteristic = nil
        writeCharacteristic = nil
    }

    // MARK: - 内部

    private func startConnect(_ peripheral: CBPeripheral, events: @escaping @Sendable (ConnectEvent) -> Void) {
        connectEvents = events
        connectingPeripheral = peripheral
        peripheral.delegate = self
        central?.connect(peripheral)
    }

    private func scanIfReady() {
        guard isScanning, let central, central.state == .poweredOn else { return }
        central.scanForPeripherals(withServices: nil)
    }

    private func emit(availability: BLEAvailability) {
        onAvailability?(availability)
        let list = seen.values.sorted { $0.rssi > $1.rssi }
        onScanUpdate?(availability, list)
    }

    private var serviceCBUUID: CBUUID { CBUUID(string: HKTProfile.serviceUUID.uuidString) }
    private var indicateCBUUID: CBUUID { CBUUID(string: HKTProfile.indicateUUID.uuidString) }
    private var writeCBUUID: CBUUID { CBUUID(string: HKTProfile.writeUUID.uuidString) }

    // MARK: - CBCentralManagerDelegate

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        emit(availability: BLEAvailability(central.state))
        scanIfReady()   // 蓝牙开启/恢复后自动继续当前扫描
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral,
                        advertisementData: [String: Any], rssi RSSI: NSNumber) {
        guard isScanning else { return }
        let name = peripheral.name ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
        guard options.isListable(name: name, rssi: RSSI.intValue), let name else { return }
        let device = DiscoveredDevice(name: name, identifier: peripheral.identifier, rssi: RSSI.intValue)
        seen[device.identifier] = device
        peripherals[device.identifier] = peripheral
        let list = seen.values.sorted { $0.rssi > $1.rssi }
        onScanUpdate?(BLEAvailability(central.state), list)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        connectEvents?(.linkEstablished)
        peripheral.delegate = self
        peripheral.discoverServices([serviceCBUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        connectEvents?(.failed(.connectionLost))
    }

    // MARK: - CBPeripheralDelegate

    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let service = peripheral.services?.first(where: { $0.uuid == serviceCBUUID }) else {
            connectEvents?(.failed(.serviceMissing))
            return
        }
        connectEvents?(.servicesDiscovered)
        peripheral.discoverCharacteristics([indicateCBUUID, writeCBUUID], for: service)
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let indicate = service.characteristics?.first(where: { $0.uuid == indicateCBUUID }),
              let write = service.characteristics?.first(where: { $0.uuid == writeCBUUID }) else {
            connectEvents?(.failed(.serviceMissing))
            return
        }
        indicateCharacteristic = indicate
        writeCharacteristic = write
        peripheral.setNotifyValue(true, for: indicate)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateNotificationStateFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil else {
            connectEvents?(.failed(.connectionLost))
            return
        }
        connectedPeripheral = peripheral
        connectedWriteCharacteristic = writeCharacteristic
        // 连接完成：后续断开由会话层（R-6 自动重连 / R-31 手动断开）接管
        let callback = connectEvents
        connectEvents = nil
        callback?(.notificationsEnabled)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard error == nil, characteristic == indicateCharacteristic, let value = characteristic.value else { return }
        onReceiveFrame?(value)
    }

    func peripheral(_ peripheral: CBPeripheral, didDisconnect device: CBPeripheral, error: Error?) {
        if peripheral == connectedPeripheral {
            connectedPeripheral = nil
            connectedWriteCharacteristic = nil
            onLinkDisconnected?()
        }
        connectEvents?(.failed(.connectionLost))
    }
}

/// HKT 透明桥服务与特征（需在类型外引用时经此转发）。
extension SystemCentral {
    func makeLink(for device: DiscoveredDevice) -> (any PeripheralLink)? {
        connectedPeripheral?.identifier == device.identifier ? self : nil
    }
}

/// 已连接会话的收发链路（DeviceSession 面向它轮询/发命令）。
extension SystemCentral: PeripheralLink {
    func send(_ frame: Data) {
        guard let peripheral = connectedPeripheral, let write = connectedWriteCharacteristic else { return }
        if peripheral.canSendWriteWithoutResponse {
            peripheral.writeValue(frame, for: write, type: .withoutResponse)
        } else {
            peripheral.writeValue(frame, for: write, type: .withResponse)
        }
    }

    var onReceive: (@Sendable (Data) -> Void)? {
        get { onReceiveFrame }
        set { onReceiveFrame = newValue }
    }

    var onDisconnected: (@Sendable () -> Void)? {
        get { onLinkDisconnected }
        set { onLinkDisconnected = newValue }
    }
}

extension SystemCentral: BluetoothPort {}
