import Foundation

/// 可编程假蓝牙源（轨道一：无真机/无签名下的全流程开发与测试载体，对应原型"场景演示"思路）。
/// 行为与真蓝牙实现共用同一套 ScanOptions 规则，保证模拟与真实过滤一致。
/// 端口契约：所有回调在主线程投递（主线程调用则同步执行，保持测试确定性）。
public final class MockCentral: BluetoothPort, @unchecked Sendable {
    public private(set) var availability: BLEAvailability = .initializing
    public private(set) var devices: [DiscoveredDevice] = []
    public private(set) var isScanning = false

    /// 场景注入：扫描启动后渐次投放的脚本设备（演示真实扫描节奏）。
    public var scriptedDevices: [DiscoveredDevice] = []
    /// 场景注入：连接脚本（成功 / 不推进判超时）。
    public var connectScript: MockConnectScript = .success(delay: 0.3)

    var options: ScanOptions?
    var onAvailability: (@Sendable (BLEAvailability) -> Void)?
    var onScanUpdate: (@Sendable (BLEAvailability, [DiscoveredDevice]) -> Void)?
    var connectEvents: (@Sendable (ConnectEvent) -> Void)?
    var connectTask: Task<Void, Never>?

    public init() {}

    public func activate(onUpdate: @escaping @Sendable (BLEAvailability) -> Void) {
        onAvailability = onUpdate
        deliver { [weak self] in
            guard let self else { return }
            onAvailability?(availability)
        }
    }

    public func startScan(options: ScanOptions, onUpdate: @escaping @Sendable (BLEAvailability, [DiscoveredDevice]) -> Void) {
        self.options = options
        onScanUpdate = onUpdate
        isScanning = true
        deliver { [weak self] in
            guard let self else { return }
            onScanUpdate?(availability, devices)
        }
        feedScriptedDevices()
    }

    public func stopScan() {
        isScanning = false
    }

    /// 场景注入：设置系统能力状态（如 .ready / .denied / .poweredOff）。
    public func setAvailability(_ availability: BLEAvailability) {
        self.availability = availability
        deliver { [weak self] in
            guard let self else { return }
            onAvailability?(availability)
            onScanUpdate?(availability, devices)
        }
    }

    /// 场景注入：模拟一次广播发现（仅扫描中且通过过滤规则时入列）。
    public func discover(name: String, identifier: UUID, rssi: Int) {
        guard isScanning, let options else { return }
        guard options.isListable(name: name, rssi: rssi) else { return }
        devices.removeAll { $0.identifier == identifier }
        devices.append(DiscoveredDevice(name: name, identifier: identifier, rssi: rssi))
        devices.sort { $0.rssi > $1.rssi }
        deliver { [weak self] in
            guard let self else { return }
            onScanUpdate?(availability, devices)
        }
    }

    /// 场景注入：清空发现列表（如设备消失/轮询停摆恢复失败）。
    public func clearDevices() {
        devices = []
        deliver { [weak self] in
            guard let self else { return }
            onScanUpdate?(availability, devices)
        }
    }

    /// 主线程投递：已在主线程则同步执行（测试确定性），否则切主队列（端口契约）。同模块扩展共用。
    func deliver(_ action: @escaping @Sendable () -> Void) {
        if Thread.isMainThread {
            action()
        } else {
            DispatchQueue.main.async(execute: action)
        }
    }

    private func feedScriptedDevices() {
        guard !scriptedDevices.isEmpty else { return }
        let feed = scriptedDevices
        Task { [weak self] in
            for (index, device) in feed.enumerated() {
                try? await Task.sleep(for: .seconds(0.5 * Double(index + 1)))
                guard let self, self.isScanning else { return }
                self.discover(name: device.name, identifier: device.identifier, rssi: device.rssi)
            }
        }
    }
}
