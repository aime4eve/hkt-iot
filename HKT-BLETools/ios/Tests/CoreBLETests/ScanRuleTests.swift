import XCTest
@testable import CoreBLE

/// 线程安全收集器：MockCentral 的回调为 @Sendable，测试用锁保护断言数据。
final class UpdateCollector: @unchecked Sendable {
    private let lock = NSLock()
    private(set) var availability: BLEAvailability?
    private(set) var devices: [DiscoveredDevice] = []

    func record(_ availability: BLEAvailability, _ devices: [DiscoveredDevice]) {
        lock.lock()
        self.availability = availability
        self.devices = devices
        lock.unlock()
    }
}

final class ScanRuleTests: XCTestCase {
    // MARK: - SP-1 名称前缀

    func testPrefixExtractsFirstThreeCharsUppercased() {
        XCTAssertEqual(DiscoveredDevice.prefix(of: "MPS100 9C01"), "MPS")
        XCTAssertEqual(DiscoveredDevice.prefix(of: "svc100"), "SVC")
        XCTAssertEqual(DiscoveredDevice.prefix(of: " UDS100 "), "UDS")
    }

    func testPrefixRejectsNilShortAndBlank() {
        XCTAssertNil(DiscoveredDevice.prefix(of: nil))
        XCTAssertNil(DiscoveredDevice.prefix(of: ""))
        XCTAssertNil(DiscoveredDevice.prefix(of: "MP"))
        XCTAssertNil(DiscoveredDevice.prefix(of: "   "))
    }

    // MARK: - SP-1 入列规则（阈值 + 前缀 + 无名称排除）

    let options = ScanOptions(allowedPrefixes: ["MPS", "SVC"], rssiThreshold: -80)

    func testListableAcceptsAllowedPrefixAboveThreshold() {
        XCTAssertTrue(options.isListable(name: "MPS100 9C01", rssi: -63))
        XCTAssertTrue(options.isListable(name: "svc100 b4d2", rssi: -80)) // 阈值取等
    }

    func testListableRejectsBelowThreshold() {
        XCTAssertFalse(options.isListable(name: "MPS100 9C01", rssi: -81))
    }

    func testListableRejectsUnsupportedPrefixAndUnnamed() {
        XCTAssertFalse(options.isListable(name: "AirPods Pro", rssi: -50))
        XCTAssertFalse(options.isListable(name: nil, rssi: -50))
        XCTAssertFalse(options.isListable(name: "MP", rssi: -50))
    }

    // MARK: - MockCentral 扫描管线（轨道一：无设备全流程载体）

    func testMockCentralFiltersDedupesAndSorts() {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.startScan(options: ScanOptions(), onUpdate: { availability, devices in
            collector.record(availability, devices)
        })

        mock.setAvailability(.ready)
        mock.discover(name: "UDS100 3F2A", identifier: UUID(), rssi: -58)
        mock.discover(name: "AirPods Pro", identifier: UUID(), rssi: -40)   // 前缀不符 → 排除
        mock.discover(name: "MPS100 9C01", identifier: UUID(), rssi: -90)   // 低于阈值 → 排除
        let udsID = collector.devices[0].identifier
        mock.discover(name: "UDS100 3F2A", identifier: udsID, rssi: -61)     // 去重更新 RSSI

        XCTAssertEqual(mock.availability, .ready)
        XCTAssertEqual(mock.devices.map(\.name), ["UDS100 3F2A"])
        XCTAssertEqual(mock.devices[0].rssi, -61)
        XCTAssertEqual(collector.availability, .ready)
        XCTAssertEqual(collector.devices.count, 1)
    }

    func testMockCentralIgnoresDiscoveryWhenNotScanning() {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.startScan(options: ScanOptions(), onUpdate: { availability, devices in
            collector.record(availability, devices)
        })
        mock.stopScan()
        mock.discover(name: "MPS100 9C01", identifier: UUID(), rssi: -40)
        XCTAssertTrue(collector.devices.isEmpty)
    }

    func testMockCentralAvailabilityGate() {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.startScan(options: ScanOptions(), onUpdate: { availability, devices in
            collector.record(availability, devices)
        })
        mock.setAvailability(.poweredOff)
        XCTAssertEqual(collector.availability, .poweredOff)
        XCTAssertFalse(mock.availability.isUsable)
    }
}
