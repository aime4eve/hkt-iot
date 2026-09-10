import XCTest
@testable import CoreBLE

/// 线程安全事件收集器。
final class ConnectEventBox: @unchecked Sendable {
    private let lock = NSLock()
    private(set) var events: [ConnectEvent] = []

    func append(_ event: ConnectEvent) {
        lock.lock()
        events.append(event)
        lock.unlock()
    }
}

final class MockConnectTests: XCTestCase {
    private let target = DiscoveredDevice(name: "MPS100 9C01", identifier: UUID(), rssi: -63)

    func testSuccessScriptEmitsFullSequence() async {
        let mock = MockCentral()
        mock.connectScript = .success(delay: 0.05)
        let box = ConnectEventBox()
        mock.connect(to: target, events: { box.append($0) })
        try? await Task.sleep(for: .seconds(0.4))
        XCTAssertEqual(box.events, [.linkEstablished, .servicesDiscovered, .notificationsEnabled])
    }

    func testNeverAdvancesEmitsNothing() async {
        let mock = MockCentral()
        mock.connectScript = .neverAdvances
        let box = ConnectEventBox()
        mock.connect(to: target, events: { box.append($0) })
        try? await Task.sleep(for: .seconds(0.2))
        XCTAssertTrue(box.events.isEmpty) // 由编排器按预算判超时（App 层）
    }

    func testCancelConnectStopsScriptedEvents() async {
        let mock = MockCentral()
        mock.connectScript = .success(delay: 0.1)
        let box = ConnectEventBox()
        mock.connect(to: target, events: { box.append($0) })
        try? await Task.sleep(for: .seconds(0.05))
        mock.cancelConnect()
        let countAtCancel = box.events.count
        try? await Task.sleep(for: .seconds(0.3))
        XCTAssertEqual(box.events.count, countAtCancel) // 取消后不再推进
        XCTAssertGreaterThanOrEqual(countAtCancel, 1)   // 至少已收到链路建立
    }

    func testScriptedDevicesFeedOnScanStart() async {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.setScriptedDevices([
            .init(name: "MPS100 9C01", identifier: UUID(), rssi: -63),
            .init(name: "UDS100 3F2A", identifier: UUID(), rssi: -58),
        ])
        mock.startScan(options: ScanOptions(), onUpdate: { availability, devices in
            collector.record(availability, devices)
        })
        mock.setAvailability(.ready)
        try? await Task.sleep(for: .seconds(2))
        XCTAssertEqual(collector.devices.count, 2) // 渐次投放后两台入列
    }
}
