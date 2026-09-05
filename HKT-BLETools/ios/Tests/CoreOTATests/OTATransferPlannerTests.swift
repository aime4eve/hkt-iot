import XCTest
import CoreProtocol
@testable import CoreOTA

private extension Data {
    var hex: String { map { String(format: "%02X", $0) }.joined() }
}

private extension String {
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

/// OTA golden vectors mirrored from shared/fixtures/ota-transfer.json (OTA-*).
/// Firmware authority: Compents/BootLoader/uart.c — packet numbers start at 0;
/// first device ACK is (0x02, 0); final chunk padded to 8-byte boundary with FF;
/// silence >10 s → device resets and ACKs (0x01, 0).
final class OTATransferPlannerTests: XCTestCase {
    func testFinalPadding() {
        XCTAssertEqual(OTATransferPlanner.padded("0102030405".data).hex, "0102030405FFFFFF")
        XCTAssertEqual(OTATransferPlanner.padded("0102030405060708".data).hex, "0102030405060708")
    }

    /// OTA-DATA-001: packet 0 with bytes 0x00..0x7F → 146-byte frame.
    func testDataFramePacketZero() {
        let chunk = Data((0..<128).map { UInt8($0) })
        let frame = OTATransferPlanner.dataFrame(packetIndex: 0, chunk: chunk)
        XCTAssertEqual(frame.hex.count / 2, 146)
        XCTAssertEqual(frame.hex,
            "686B740083020000000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7FA62C626F6F746C6F6164")
    }

    /// OTA-FINAL-001: 100 real bytes + minimal FF padding to the 8-byte boundary (104 sent).
    /// The final packet stays cmd 0x02 — the device completes on the size threshold and
    /// answers ACK(cmd=0x03); cmd 0xFF is only the forced-flush recovery path.
    func testFinalFlushFrame() {
        let chunk = Data((0..<100).map { UInt8($0) })
        let frame = OTATransferPlanner.dataFrame(packetIndex: 5, chunk: chunk)
        XCTAssertEqual(frame.hex.count / 2, 122)  // hkt3+len2+cmd1+pack2+104+crc2+suffix8
        XCTAssertEqual(frame[3...5].map { $0 }, [0x00, 0x6B, 0x02])  // len=107, cmd=0x02
        XCTAssertEqual(frame[107], 0x63)             // last real byte (99)
        XCTAssertEqual(frame[108...111].map { $0 }, [0xFF, 0xFF, 0xFF, 0xFF])  // minimal padding
        XCTAssertTrue(frame.hex.hasSuffix("626F6F746C6F6164"))
    }

    func testPacketsSplit() {
        let image = Data((0..<300).map { UInt8($0 % 251) })
        let packets = OTATransferPlanner.packets(for: image)
        XCTAssertEqual(packets.map { $0.index }, [0, 1, 2])
        XCTAssertEqual(packets[0].chunk.count, 128)
        XCTAssertEqual(packets[2].chunk.count, 44)
        XCTAssertTrue(packets[2].isFinal)
        XCTAssertFalse(packets[0].isFinal)
    }

    /// OTA-FIRST-REQUEST-001: first device ACK requests packet **0** — never 0x0002.
    func testFirstRequestIsPacketZero() {
        let ack = OTAACK.parse("686B74020000626F6F746C6F6164".data)
        XCTAssertEqual(ack, OTAACK(kind: .requestPacket, requestedPacket: 0))
    }

    /// OTA-RESET-001: device reset after ~10 s silence.
    func testDeviceResetRestart() {
        let ack = OTAACK.parse("686B74010000626F6F746C6F6164".data)
        XCTAssertEqual(ack, OTAACK(kind: .restartTransfer, requestedPacket: 0))
    }

    func testCompletionAck() {
        let ack = OTAACK.parse("686B74030000626F6F746C6F6164".data)
        XCTAssertEqual(ack, OTAACK(kind: .transferComplete, requestedPacket: 0))
    }

    func testAckRejectsBadFrame() {
        XCTAssertNil(OTAACK.parse("686B74020000626F6F746C6F6165".data))  // bad suffix
        XCTAssertNil(OTAACK.parse("686B7402".data))                      // too short
    }

    /// OTA-NOTIFY-001: app-frame notify that flips the device into the bootloader.
    func testNotifyFrame() {
        let frame = HKTFrameEncoder.appFrame(packNum: 0, cmd: CommandCode.otaNotify,
                                             data: HKTFrameEncoder.fillerPayload())
        XCTAssertEqual(frame.hex, "686B7400000501FFFFFFFFF8DD")
    }
}
