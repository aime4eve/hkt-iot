import XCTest
@testable import CoreBLE

/// R-2 目标设备定位（ScanModel 侧逻辑由 App 层持有；此处验证 MockCentral 侧的命中链路）。
/// ScanModel 是 @MainActor App 类型，CoreBLE 测试只验证传输层行为；
/// 定位状态机（后缀匹配/超时/忽略前缀过滤）随 App 目标迁移（如需 App 层测试再建 HKTBLEToolsTests）。
final class LocatePipelineTests: XCTestCase {
    @MainActor
    func testLocateScanFindsSuffixHitIgnoringPrefixFilter() async {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.setAvailability(.ready)
        mock.activate { collector.record($0, []) }
        // 目标模式忽略前缀过滤：允许集只剩支持前缀，但脚本设备是 "TESTER A3F2AB"（非支持前缀）也应命中
        mock.startScan(options: ScanOptions(allowedPrefixes: DiscoveredDevice.supportedPrefixes,
                                            rssiThreshold: -80)) { _, devices in
            collector.record(.ready, devices)
        }
        // 广播名带后缀（小写混合，验证忽略大小写 contains）
        mock.discover(name: "tester a3f2ab", identifier: UUID(), rssi: -55, targetMode: true)
        let hit = collector.devices.first { $0.name.uppercased().contains("A3F2AB") }
        XCTAssertNotNil(hit, "目标模式应命中带后缀广播名")
        XCTAssertEqual(hit?.rssi, -55)
        XCTAssertEqual(collector.availability, .ready)
    }

    @MainActor
    func testLocateIgnoresNonMatchingAndWeakDevices() async {
        let mock = MockCentral()
        let collector = UpdateCollector()
        mock.activate { _ in }
        mock.startScan(options: ScanOptions()) { _, devices in
            collector.record(.ready, devices)
        }
        mock.discover(name: "UDS100 998877", identifier: UUID(), rssi: -50)   // 后缀不符
        mock.discover(name: "MPS100 A3F2AB", identifier: UUID(), rssi: -90)   // 低于阈值仍被扫规则排除
        XCTAssertFalse(collector.devices.contains { $0.name.contains("A3F2AB") })
        XCTAssertTrue(collector.devices.allSatisfy { $0.name.contains("998877") })
    }
}
