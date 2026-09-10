import XCTest
@testable import CoreProtocol

/// 夹具驱动：三家族查询响应 → 快照字段（对应 shared/fixtures/response-parse.json）。
final class DeviceSnapshotTests: XCTestCase {
    private func fixtureData(_ hex: String) -> Data {
        var data = Data()
        var index = hex.startIndex
        while index < hex.endIndex {
            let next = hex.index(index, offsetBy: 2)
            data.append(UInt8(hex[index..<next], radix: 16)!)
            index = next
        }
        return data
    }

    private func decode(_ hex: String, family: DeviceFamily) throws -> DeviceSnapshot {
        let (entries, unknownTail) = try HKTResponseParser.parse(fixtureData(hex), family: family)
        XCTAssertFalse(unknownTail)
        return DeviceSnapshotDecoder.snapshot(from: entries, family: family)
    }

    func testDC200FixtureDecodes() throws {
        let snapshot = try decode(
            "686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064",
            family: .dc200Family)
        XCTAssertEqual(snapshot.hardwareVersion, 0x0B)
        XCTAssertEqual(snapshot.softwareVersion, 0x1C)
        XCTAssertEqual(snapshot.power, 1)
        XCTAssertEqual(snapshot.batteryPercent, 87)
        XCTAssertEqual(snapshot.parkState, 1)
        XCTAssertEqual(snapshot.parkMode, 0)
        XCTAssertEqual(snapshot.tamper, 0)
        XCTAssertEqual(snapshot.reportPeriodMin, 20)
        XCTAssertEqual(snapshot.magX, 120)
        XCTAssertEqual(snapshot.magY, -120)
        XCTAssertEqual(snapshot.magZ, 400)
        XCTAssertEqual(snapshot.radarSpectrum, [10, 20, 30, 40, 50, 60, 70, 80, 90, 100])
    }

    func testUDSFixtureDecodes() throws {
        let snapshot = try decode(
            "686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708",
            family: .uds100)
        XCTAssertEqual(snapshot.hardwareVersion, 0x02)
        XCTAssertEqual(snapshot.softwareVersion, 0x0A)
        XCTAssertEqual(snapshot.batteryVoltageMV, 4000)
        XCTAssertEqual(snapshot.temperatureMilli, 25000)   // 0x0061A8 = 25000 → 25.000 ℃
        XCTAssertEqual(snapshot.humidityMilli, 12000)
        XCTAssertEqual(snapshot.angleCenti, 20)             // 固件 angle×100 → 显示 /100
        XCTAssertEqual(snapshot.slant, 0)
        XCTAssertEqual(snapshot.htAlarm, 0)
        XCTAssertEqual(snapshot.gpsPeriodMin, 60)
        XCTAssertEqual(snapshot.distanceMM, 1080)
        XCTAssertEqual(snapshot.overflowState, 0)
        XCTAssertEqual(snapshot.lowThresholdMM, 400)
        XCTAssertEqual(snapshot.highThresholdMM, 3000)
        XCTAssertEqual(snapshot.reportPeriodMin, 30)
        XCTAssertEqual(snapshot.latitude ?? 0, 16.909060, accuracy: 0.000001)   // 0x01020304 / 1e6
        XCTAssertEqual(snapshot.longitude ?? 0, 84.281096, accuracy: 0.000001)  // 0x05060708 / 1e6
    }

    func testSVCFixtureDecodes() throws {
        let snapshot = try decode(
            "686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E",
            family: .svc100)
        XCTAssertEqual(snapshot.hardwareVersion, 0x0D)
        XCTAssertEqual(snapshot.softwareVersion, 0x0D)
        XCTAssertEqual(snapshot.power, 1)
        XCTAssertEqual(snapshot.batteryPercent, 99)
        XCTAssertEqual(snapshot.valve1State, 1)
        XCTAssertEqual(snapshot.valve1Inserted, 0)
        XCTAssertEqual(snapshot.valve1Pulse, 100)
        XCTAssertEqual(snapshot.valve2State, 1)
        XCTAssertEqual(snapshot.valve2Inserted, 1)
        XCTAssertEqual(snapshot.valve2Pulse, 500)
        XCTAssertEqual(snapshot.voltageLevel, 2)
        XCTAssertEqual(snapshot.portFunction, 1)
        XCTAssertEqual(snapshot.stableTimeS, 5)
        XCTAssertEqual(snapshot.smartPower, 1)
        XCTAssertEqual(snapshot.timezone, 25)
        XCTAssertEqual(snapshot.reportPeriodMin, 30)
    }

    func testUnknownTailKeepsParsedFields() throws {
        // 尾部追加未知类型 0x77 → 已解析前缀保留 + unknownTail 标记（S-6/R-8）
        let (entries, unknownTail) = try HKTResponseParser.parse(
            fixtureData("686B740001010B1C77AA"), family: .dc200Family)
        XCTAssertTrue(unknownTail)
        var snapshot = DeviceSnapshot(family: .dc200Family)
        DeviceSnapshotDecoder.decode(entries, family: .dc200Family, into: &snapshot)
        XCTAssertEqual(snapshot.hardwareVersion, 0x0B)
        XCTAssertEqual(snapshot.softwareVersion, 0x1C)
    }

    func testSignedMagneticDecoding() {
        XCTAssertEqual(BEValue.i16(Data([0xFF, 0x88])), -120)
        XCTAssertEqual(BEValue.i24(Data([0x00, 0x61, 0xA8])), 25000)
        XCTAssertEqual(BEValue.i32(Data([0x00, 0x0F, 0x42, 0x40])), 1_000_000)
    }
}
