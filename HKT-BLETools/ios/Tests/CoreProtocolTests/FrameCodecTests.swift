import XCTest
@testable import CoreProtocol

/// TX golden vectors mirrored from shared/fixtures/app-frame-tx.json (TX-*).
/// Every vector carries a firmwareReference in the JSON source of truth.
final class FrameCodecTests: XCTestCase {
    private func frame(packNum: UInt8, cmd: UInt8, data: Data) -> String {
        HKTFrameEncoder.appFrame(packNum: packNum, cmd: cmd, data: data).hex
    }

    func testQueryFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: CommandCode.query, data: HKTFrameEncoder.fillerPayload()),
                       "686B74000005FFFFFFFFFF06C3")
    }

    func testDC200ConfigFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x02, data: Data([0x00, 0x1E, 0x01])),
                       "686B7400000402001E01277E")
    }

    func testDC200ZeroPeriodBoundary() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x02, data: Data([0x00, 0x00, 0x01])),
                       "686B740000040200000128FF")
    }

    func testUDS100ConfigFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x02,
                             data: Data([0x00, 0x1E, 0x00, 0x3C, 0x01, 0x2C, 0x0B, 0xB8])),
                       "686B7400000902001E003C012C0BB86CBB")
    }

    func testSVC100ConfigFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x02,
                             data: Data([0x02, 0x01, 0x05, 0x01, 0x19, 0x00, 0x1E])),
                       "686B74000008020201050119001EE264")
    }

    func testSVC100RealtimeTask() {
        // valve 1 + open + duration 5 s (2B BE) + pulse 100 (3B BE); busy → silently ignored (no ACK)
        XCTAssertEqual(frame(packNum: 0, cmd: 0x03,
                             data: Data([0x01, 0x01, 0x00, 0x05, 0x00, 0x64])),
                       "686B7400000703010100050064BB9E")
    }

    /// TX-SVC-TASK-002: id 1 + valve 1 + open + pulse 100 + 08:00–18:30 + repeat 0x7F (bit0=Mon…bit6=Sun)
    func testSVC100TimedTask() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x04,
                             data: Data([0x01, 0x01, 0x01, 0x00, 0x64, 0x01, 0xE0, 0x04, 0x56, 0x7F])),
                       "686B7400000B04010101006401E004567FF9D5")
    }

    /// TX-SVC-TASK-003: delete-all is id 0xFF; deleting a running task force-stops it; ACK always.
    func testSVC100DeleteAllTasks() {
        XCTAssertEqual(frame(packNum: 0, cmd: 0x05, data: Data([0xFF])),
                       "686B7400000205FF71C0")
    }

    func testPowerFrame() {
        // Power byte sits at payload[3] (frame offset 10) — firmware reads data[10].
        XCTAssertEqual(frame(packNum: 0, cmd: CommandCode.power, data: HKTFrameEncoder.powerPayload(on: true)),
                       "686B74000005FE00000001EF97")
    }

    func testCalibrationFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: CommandCode.calibrate, data: HKTFrameEncoder.fillerPayload()),
                       "686B74000005FDFFFFFFFF104B")
    }

    func testTimeSyncFrame() {
        XCTAssertEqual(frame(packNum: 0, cmd: CommandCode.timeSync,
                             data: Data([0x69, 0x54, 0x67, 0x80])),
                       "686B74000005066954678017C8")
    }
}

extension Data {
    var hex: String { map { String(format: "%02X", $0) }.joined() }
}

extension String {
    var data: Data {
        var value = Data()
        var index = startIndex
        while index < endIndex {
            let next = self.index(index, offsetBy: 2)
            value.append(UInt8(self[index..<next], radix: 16)!)
            index = next
        }
        return value
    }
}
