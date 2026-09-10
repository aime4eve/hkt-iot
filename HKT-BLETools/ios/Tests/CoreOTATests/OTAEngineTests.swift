import CoreProtocol
import XCTest
@testable import CoreOTA

/// OTA 传输引擎——夹具 shared/fixtures/ota-transfer.json（OTA-*）同源向量。
/// 时序（进入帧 A 40ms / 启动节拍 100ms 起、间隔 50ms）为测试注入的短时序。
@MainActor
final class OTAEngineTests: XCTestCase {
    /// 可编程发送通道：记录全部已发帧，测试按需喂 ACK。
    private final class FakeChannel: @unchecked Sendable {
        private let lock = NSLock()
        private var sent: [Data] = []

        func record(_ frame: Data) {
            lock.lock()
            sent.append(frame)
            lock.unlock()
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
    private let completeAck = "686B74030000626F6F746C6F6164"    // ACK(3,0)
    private let enterB = "686B74000005010100000017FF"             // 入口 B：len=0005 payload[0]=1
    private let enterA = "686B74000001011189"                     // 入口 A：len=0001 纯命令帧
    private let startFrame300 = "686B740005010000012CF9F2626F6F746C6F6164"   // cmd1 带 size=300

    private func request(_ packet: UInt16) -> Data {
        hex("686B7402" + String(format: "%04X", packet) + "626F6F746C6F6164")
    }

    private func makeEngine(_ bytes: Int, channel: FakeChannel) -> OTAEngine {
        let engine = OTAEngine(image: Data(repeating: 0xAB, count: bytes),
                               ackTimeout: 5, maxRestarts: 2,
                               enterAAfter: 0.04, startBeatAfter: 0.1, beatInterval: 0.05)
        engine.bind { channel.record($0) }
        return engine
    }

    func testEnterFramesThenStartBeat() async {
        // 双入口帧 + 启动帧节拍（cmd1 带 size，bootload 后缀）
        let channel = FakeChannel()
        let engine = makeEngine(300, channel: channel)
        engine.begin()
        XCTAssertEqual(channel.frames[0], hex(enterB))
        XCTAssertEqual(engine.state, .enteringBootloader)
        try? await Task.sleep(for: .seconds(0.3))
        let frames = channel.frames
        XCTAssertEqual(frames[1], hex(enterA))
        XCTAssertTrue(frames.count >= 4)
        XCTAssertTrue(frames[2...].contains(hex(startFrame300)))
    }

    func testFirstRequestIsPacketZero() async {
        // OTA-FIRST-REQUEST-001：bootloader 首个 ACK(2,0) 请求 0 号包，随后发数据包 0
        let channel = FakeChannel()
        let engine = makeEngine(300, channel: channel)
        engine.bind { channel.record($0) }
        engine.begin()
        try? await Task.sleep(for: .seconds(0.15))
        XCTAssertTrue(engine.handle(hex(firstRequest)))
        XCTAssertEqual(engine.state, .transferring)
        let packet = channel.frames.last!
        XCTAssertEqual(packet.prefix(8), hex("686B740083020000"))   // len 0x83 cmd 0x02 pack 0x0000
        XCTAssertEqual(packet.count, 146)                            // 128B 数据 + 帧头尾 18B
        XCTAssertEqual(engine.packetsDone, 0)
    }

    func testFullTransferLoopCompletes() async {
        // 300B → 3 包；ACK 驱动逐包；末包后 ACK(3) 完成
        let channel = FakeChannel()
        let engine = makeEngine(300, channel: channel)
        engine.bind { channel.record($0) }
        engine.begin()
        try? await Task.sleep(for: .seconds(0.15))
        XCTAssertTrue(engine.handle(request(0)))
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(request(2)))
        XCTAssertEqual(engine.packetsDone, 2)
        XCTAssertTrue(engine.handle(hex(completeAck)))
        XCTAssertEqual(engine.state, .done)
    }

    func testDeviceResetRestart() async {
        // OTA-RESET-001：ACK(1,0) = 设备复位传输 → 重发启动帧，重新从 0 号包起步
        let channel = FakeChannel()
        let engine = makeEngine(300, channel: channel)
        engine.bind { channel.record($0) }
        engine.begin()
        try? await Task.sleep(for: .seconds(0.15))
        XCTAssertTrue(engine.handle(request(0)))
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(hex(resetAck)))
        XCTAssertEqual(engine.restarts, 1)
        XCTAssertEqual(engine.packetsDone, 0)
        XCTAssertEqual(engine.state, .enteringBootloader)
        try? await Task.sleep(for: .seconds(0.25))                   // 等重启后的启动帧节拍
        let startFrames = channel.frames.filter { $0 == hex(startFrame300) }
        XCTAssertGreaterThanOrEqual(startFrames.count, 1)
        XCTAssertTrue(engine.handle(request(0)))
        XCTAssertTrue(engine.handle(request(1)))
        XCTAssertTrue(engine.handle(request(2)))
        XCTAssertTrue(engine.handle(hex(completeAck)))
        XCTAssertEqual(engine.state, .done)
    }

    func testTooManyRestartsFails() async {
        let channel = FakeChannel()
        let engine = makeEngine(128, channel: channel)
        engine.bind { channel.record($0) }
        engine.begin()
        try? await Task.sleep(for: .seconds(0.15))
        XCTAssertTrue(engine.handle(hex(resetAck)))
        XCTAssertTrue(engine.handle(hex(resetAck)))
        XCTAssertTrue(engine.handle(hex(resetAck)))                  // 第 3 次 > 上限 2
        XCTAssertEqual(engine.state, .failed(.tooManyRestarts))
    }

    func testCancelStopsTransfer() async {
        let channel = FakeChannel()
        let engine = makeEngine(128, channel: channel)
        engine.bind { channel.record($0) }
        engine.begin()
        engine.cancel()
        XCTAssertEqual(engine.state, .failed(.cancelled))
        XCTAssertFalse(engine.handle(request(0)))                    // 取消后不再消费
    }

    func testNonBootloaderFrameRejected() {
        let engine = makeEngine(128, channel: FakeChannel())
        XCTAssertFalse(engine.handle(Data("Calibration Done".utf8)))
        XCTAssertFalse(engine.handle(Data([0x68, 0x6B, 0x74, 0x02])))   // 截断帧
    }
}
