import XCTest
@testable import CoreProtocol

/// Response golden vectors mirrored from shared/fixtures/response-parse.json (RX-*).
/// Firmware authority: callback_BLEQuery/callback_BLESearch + setDataPackage fixed sizes.
final class TLVParserTests: XCTestCase {
    func testDC200FamilyQueryResponse() throws {
        let raw = ("686B740001010B1C8D0103573A013B0084008600145D00785EFF885F0190"
            + "60000A0014001E00280032003C00460050005A0064").data
        let (entries, _) = try HKTResponseParser.parse(raw, family: .dc200Family)
        XCTAssertEqual(entries.count, 11)
        XCTAssertEqual(entries.first { $0.type == 0x01 }?.value, "0B1C".data)
        XCTAssertEqual(BEValue.u16(entries.first { $0.type == 0x86 }!.value), 20)
        // int16 big-endian: 0xFF88 = -120
        XCTAssertEqual(SignedValueDecoder.s16(entries.first { $0.type == 0x5E }!.value), -120)
        XCTAssertEqual(SignedValueDecoder.s16(entries.first { $0.type == 0x5D }!.value), 120)
        let radar = entries.first { $0.type == 0x60 }!.value
        XCTAssertEqual(radar.count, 20)
        XCTAssertEqual((0..<10).map { BEValue.u16(radar.subdata(in: (2 * $0)..<(2 * $0 + 2))) },
                       [10, 20, 30, 40, 50, 60, 70, 80, 90, 100])
    }

    func testUDS100QueryResponse() throws {
        let raw = ("686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C"
            + "46043847004801900BB886001E10010203041105060708").data
        let (entries, _) = try HKTResponseParser.parse(raw, family: .uds100)
        XCTAssertEqual(entries.count, 15)
        XCTAssertEqual(entries.first { $0.type == 0x01 }?.value, "020A".data)
        XCTAssertEqual(BEValue.u16(entries.first { $0.type == 0x8B }!.value), 4000)  // 4000 mV
        XCTAssertEqual(SignedValueDecoder.s24(entries.first { $0.type == 0x09 }!.value), 25000) // 25.000 °C
        XCTAssertEqual(SignedValueDecoder.s24(entries.first { $0.type == 0x0A }!.value), 12000) // 12.000 %
        let overflowConfig = entries.first { $0.type == 0x48 }!.value
        XCTAssertEqual(BEValue.u16(overflowConfig.subdata(in: 0..<2)), 400)   // low threshold
        XCTAssertEqual(BEValue.u16(overflowConfig.subdata(in: 2..<4)), 3000)  // high threshold
        XCTAssertEqual(BEValue.u32(entries.first { $0.type == 0x10 }!.value), 16909060)
    }

    func testSVC100QueryResponse() throws {
        let raw = "686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E".data
        let (entries, _) = try HKTResponseParser.parse(raw, family: .svc100)
        XCTAssertEqual(entries.count, 10)
        let valve = entries.first { $0.type == 0x3C }!.value
        XCTAssertEqual([UInt8](valve), [1, 0, 0, 100, 1, 1, 1, 244])  // v1 on/insert 0/pulse 100, v2 on/insert 1/pulse 500
        XCTAssertEqual(BEValue.u16(valve.subdata(in: 2..<4)), 100)
        XCTAssertEqual(BEValue.u16(valve.subdata(in: 6..<8)), 500)
        XCTAssertEqual(entries.first { $0.type == 0x8A }?.value.first, 25)  // firmware: 25 = +03:30
        XCTAssertEqual(entries.first { $0.type == 0x40 }?.value.first, 2)
    }

    func testAckRecord() throws {
        let (entries, _) = try HKTResponseParser.parse("686B740004FFFF".data, family: .dc200Family)
        XCTAssertEqual(entries, [TLVEntry(type: 0xFF, value: Data([0xFF]))])
    }

    /// S-6 revision: TLV records carry no length byte, so an unknown type breaks
    /// stream sync — keep the parsed prefix, flag the unknown tail, never skip silently.
    func testUnknownTypeDataAbnormal() throws {
        let (entries, unknownTail) = try HKTResponseParser.parse("686B740005010B1C77000363".data, family: .dc200Family)
        XCTAssertTrue(unknownTail)
        XCTAssertEqual(entries, [TLVEntry(type: 0x01, value: "0B1C".data)])
    }

    func testBadPrefixAndTruncated() {
        XCTAssertThrowsError(try HKTResponseParser.parse("0000000000".data, family: .svc100)) {
            XCTAssertEqual($0 as? ResponseParseError, .badPrefix)
        }
        XCTAssertThrowsError(try HKTResponseParser.parse("686B740000FF".data, family: .svc100)) {
            XCTAssertEqual($0 as? ResponseParseError, .truncated)
        }
    }
}
