import CoreProtocol
import XCTest
@testable import CoreBLE

/// 设备会话轮询（R-5/R-7/R-8）——MockLink 夹具应答，无真机全流程测试。
@MainActor
final class DeviceSessionTests: XCTestCase {
    /// 三家族查询响应夹具（shared/fixtures/response-parse.json 同源）。
    private static let udsResponse =
        "686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708"
    private static let dc200Response =
        "686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064"

    private func fixture(_ hex: String) -> Data {
        var data = Data()
        var index = hex.startIndex
        while index < hex.endIndex {
            let next = hex.index(index, offsetBy: 2)
            data.append(UInt8(hex[index..<next], radix: 16)!)
            index = next
        }
        return data
    }

    func testPollReceivesAndDecodesSnapshot() async {
        let link = MockLink()
        link.responder = { [response = fixture(Self.udsResponse)] _ in response }   // 任何查询 → UDS 响应夹具
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.staleAfter = 2
        session.start()
        try? await Task.sleep(for: .seconds(0.4))
        XCTAssertGreaterThanOrEqual(session.pollsSent, 2)
        XCTAssertEqual(session.snapshot.temperatureMilli, 25000)
        XCTAssertEqual(session.snapshot.humidityMilli, 12000)
        XCTAssertEqual(session.snapshot.batteryVoltageMV, 4000)
        XCTAssertEqual(session.snapshot.distanceMM, 1080)
        XCTAssertFalse(session.isStale)
        XCTAssertFalse(session.unknownTail)
        session.stop()
    }

    func testQueryFrameShape() async {
        let link = MockLink()
        link.responder = { _ in nil }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.start()
        try? await Task.sleep(for: .seconds(0.25))
        XCTAssertGreaterThanOrEqual(link.framesSent, 2)
        // TX-QUERY-001 同构：hkt + packNum + len(0005) + FF + FFFFFFFF + CRC
        XCTAssertEqual(link.sentFrame(at: 0).prefix(3), Data([0x68, 0x6B, 0x74]))
        XCTAssertEqual(link.sentFrame(at: 0).count, 13)
        session.stop()
    }

    func testUnknownTailMarksAbnormal() async {
        let link = MockLink()
        let abnormal = Self.udsResponse + "77AA"   // 尾部追加未知类型 0x77
        link.responder = { [response = fixture(abnormal)] _ in response }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.start()
        try? await Task.sleep(for: .seconds(0.3))
        XCTAssertTrue(session.unknownTail)                          // R-8：标记而非崩溃/跳过
        XCTAssertEqual(session.snapshot.temperatureMilli, 25000)    // 已解析前缀保留
        session.stop()
    }

    func testStalenessWithoutResponse() async {
        let link = MockLink()
        link.responder = { _ in nil }   // 停摆：不响应
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.staleAfter = 1
        session.start()
        try? await Task.sleep(for: .seconds(0.5))
        XCTAssertTrue(session.isStale)              // R-7：超窗判定
        XCTAssertGreaterThanOrEqual(session.pollsSent, 3)
        XCTAssertNil(session.snapshot.temperatureMilli)
        session.stop()
    }

    func testDisconnectMarksLinkLost() async {
        let link = MockLink()
        link.responder = { [response = fixture(Self.udsResponse)] _ in response }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.start()
        try? await Task.sleep(for: .seconds(0.2))
        link.responder = { _ in nil }   // 断开后设备不再响应（真实语义）
        link.simulateDisconnect()
        try? await Task.sleep(for: .seconds(0.3))
        XCTAssertTrue(session.linkLost)   // R-31 语义：链路丢失标记保持
        XCTAssertTrue(session.isStale)    // R-7：无响应超窗
        session.stop()
    }

    func testCommandSendUsesEncoder() {
        let link = MockLink()
        let session = DeviceSession(family: .svc100, deviceName: "SVC100 B4D2",
                                    link: link, pollInterval: 0.05)
        // 0x06 对时：packNum 1、stamp 由 App 层传入（SP-25：App 不做时区换算）
        session.send(cmd: CommandCode.timeSync, data: Data([0x00, 0x00, 0x00, 0x01]))
        let frame = link.sentFrame(at: 0)
        XCTAssertEqual(frame.prefix(3), Data([0x68, 0x6B, 0x74]))
        XCTAssertEqual(frame[4], 0)                    // len 高字节
        XCTAssertEqual(frame[5], 5)                    // len 低字节 = cmd(1) + data(4)
        XCTAssertEqual(frame[6], CommandCode.timeSync)
    }

    // MARK: 写入确认（sendWrite：专用 ACK 帧 = hkt 00 seq FF 值）

    func testSendWriteResolvesOnSyncAckFrame() async {
        // MockLink 同步应答：ACK 在 send 内部到达，先注册等待再发送的顺序不能丢 ACK
        let link = MockLink()
        let ack = fixture("686B740000FF00")
        link.responder = { [ack] frame in
            frame.count > 6 && frame[6] == CommandCode.config ? ack : nil   // 轮询帧不回
        }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 3600)
        session.start()
        let acked = await session.sendWrite(cmd: CommandCode.config,
                                            data: HKTFrameEncoder.udsConfigPayload(reportMin: 20, gpsMin: 60, lowMM: 400, highMM: 3000),
                                            timeout: 0.5)
        XCTAssertTrue(acked)
        session.stop()
    }

    func testSendWriteTimesOutWithoutAck() async {
        let link = MockLink()
        link.responder = { [response = fixture(Self.udsResponse)] frame in
            frame.count > 6 && frame[6] == CommandCode.query ? response : nil   // 只回轮询，写入被固件静默拒绝
        }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 3600)
        session.start()
        let acked = await session.sendWrite(cmd: CommandCode.config,
                                            data: HKTFrameEncoder.udsConfigPayload(reportMin: 20, gpsMin: 60, lowMM: 400, highMM: 3000),
                                            timeout: 0.3)
        XCTAssertFalse(acked)
        session.stop()
    }

    func testSendWriteIgnoresPollResponseOnly() async {
        // 轮询回应不含 0xFF 段（固件事实），不得误判为写入确认
        let link = MockLink()
        link.responder = { [response = fixture(Self.udsResponse)] frame in
            frame.count > 6 && frame[6] == CommandCode.query ? response : nil
        }
        let session = DeviceSession(family: .uds100, deviceName: "UDS100 3F2A",
                                    link: link, pollInterval: 0.05)
        session.start()
        let acked = await session.sendWrite(cmd: CommandCode.config,
                                            data: HKTFrameEncoder.udsConfigPayload(reportMin: 20, gpsMin: 60, lowMM: 400, highMM: 3000),
                                            timeout: 0.25)
        XCTAssertFalse(acked)   // 只有轮询回应流动，无专用 ACK → 超时
        session.stop()
    }
}
