import CoreProtocol
import XCTest
@testable import CoreOTA

/// OTA 传输引擎——夹具 shared/fixtures/ota-transfer.json（OTA-*）同源向量。
@MainActor
final class OTAEngineTests: XCTestCase {
    /// 可编程发送通道：记录全部已发帧，测试按需喂 ACK。
    private final class FakeChannel: @unchecked Sendable {
        private let lock = NSLock()
        private var sent: [Data] = []
        var onSend: ((Data) -> Void)?

        func record(_ frame: Data) {
            lock.lock()
            sent.append(frame)
            lock.unlock()
            onSend?(frame)
        }

        var frames: [Data] {
            lock.lock()
            defer { lock.unlock() }
            return sent
        }
    }

    private func hex(_ hex: String) -> Data {
        var data = Data()
        var index = hex.startIndex
        while index < hex.endIndex {
            let next = hex.index(index, offsetBy: 2)
            data.append(UInt8(hex[index..<next], radix: 16)!)
            index = next
        }
        return data
    }

    private let firstRequest = "686B74020000626F6F746C6F6164"   // OTA-FIRST-REQUEST-001: ACK(2,0)
    private let resetAck = "686B74010000626F6F746C6F6164"       // OTA-RESET-001: ACK(1,0)
    private func request(_ packet: UInt16) -> Data {
        hex("686B7402" + String(format: "%04X", packet) + "626F6F746C6F6164")
    }
    private let completeAck = "686B74030000626F6F746C6F6164"    // ACK(3,0)

    func testNotifyFrameMatchesFixture() {
        // OTA-NOTIFY-001：begin() 首帧 = 应用帧 0x01 + 0xFFFFFFFF 填充
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0, count: 300))
        engine.bind { channel.record($0) }
        engine.begin()
        XCTAssertEqual(channel.frames.first,
                       hex("686B7400000501FFFFFFFFF8DD"))
        XCTAssertEqual(engine.state, .transferring)
    }

    func testFirstRequestIsPacketZero() {
        // OTA-FIRST-REQUEST-001：设备首个 ACK(2,0) 请求 0 号包
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0xAB, count: 300))
        engine.bind { channel.record($0) }
        engine.begin()
        XCTAssertTrue(engine.handle(hex(firstRequest)))
        XCTAssertEqual(channel.frames.count, 2)
        let packet = channel.frames[1]
        XCTAssertEqual(packet.prefix(8), hex("686B740083020000"))       // len 0x83 cmd 0x02 pack 0x0000
        XCTAssertEqual(packet.count, 146)                            // 128B 数据 + 帧头尾 18B
        XCTAssertEqual(packet.dropFirst(8).dropLast(10).last, 0xAB)
        XCTAssertEqual(engine.packetsDone, 0)
    }

    func testFullTransferLoopCompletes() {
        // 300B → 3 包；ACK 驱动逐包；末包后 ACK(3) 完成
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0x11, count: 300))
        engine.bind { channel.record($0) }
        engine.begin()
        XCTAssertTrue(engine.handle(request(0)))
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(request(2)))
        XCTAssertEqual(engine.packetsDone, 2)
        XCTAssertEqual(channel.frames.count, 4)                      // 通知 + 3 数据包
        // 末包 44B → 补 FF 至 48B（8 字节边界）
        let finalPacket = channel.frames[3]
        XCTAssertEqual(finalPacket.count, 48 + 18)
        // 末包 44B → 补 FF 4B；帧尾 = crc(2)+bootload(8)，补位区在 payload 末端
        for offset in 4...7 {
            XCTAssertEqual(finalPacket[finalPacket.count - 10 - 8 + offset], 0xFF, "padding byte \(offset)")
        }
        XCTAssertTrue(engine.handle(hex(completeAck)))
        XCTAssertEqual(engine.state, .done)
    }

    func testDeviceResetRestart() {
        // OTA-RESET-001：ACK(1,0) = 设备复位传输，计数清零后按请求从 0 号包重来
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0x22, count: 300))
        engine.bind { channel.record($0) }
        engine.begin()
        XCTAssertTrue(engine.handle(request(0)))
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(hex(resetAck)))                  // 设备复位
        XCTAssertEqual(engine.restarts, 1)
        XCTAssertEqual(engine.packetsDone, 0)
        XCTAssertEqual(engine.state, .transferring)
        XCTAssertTrue(engine.handle(request(0)))                // 设备从头请求
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(request(2)))
        XCTAssertTrue(engine.handle(hex(completeAck)))
        XCTAssertEqual(engine.state, .done)
    }

    func testTooManyRestartsFails() {
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0x33, count: 128))
        engine.bind { channel.record($0) }
        engine.begin()
        XCTAssertTrue(engine.handle(hex(resetAck)))
        XCTAssertTrue(engine.handle(hex(resetAck)))
        XCTAssertTrue(engine.handle(hex(resetAck)))                  // 第 3 次 > 上限 2
        XCTAssertEqual(engine.state, .failed(.tooManyRestarts))
    }

    func testCancelStopsTransfer() {
        let channel = FakeChannel()
        let engine = OTAEngine(image: Data(repeating: 0x44, count: 128))
        engine.bind { channel.record($0) }
        engine.begin()
        engine.cancel()
        XCTAssertEqual(engine.state, .failed(.cancelled))
        XCTAssertFalse(engine.handle(request(0)))               // 取消后不再消费
    }

    func testNonBootloaderFrameRejected() {
        let engine = OTAEngine(image: Data(repeating: 0x55, count: 128))
        XCTAssertFalse(engine.handle(Data("Calibration Done".utf8)))
        XCTAssertFalse(engine.handle(Data([0x68, 0x6B, 0x74, 0x02])))   // 截断帧
    }
}
