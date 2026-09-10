import CoreProtocol
import Foundation

/// OTA 传输引擎（引导层回环，全部由设备 ACK 驱动）。
/// 契约出处：shared/fixtures/ota-transfer.json（OTA-*）+ BootLoader/uart.c：
/// - begin() 发 OTA 通知（应用帧 0x01，0xFF 填充，OTA-NOTIFY-001）→ 设备写更新标志并复位进引导；
/// - 设备 ACK(0x02, n) 请求第 n 包（首请求即 0 号，OTA-FIRST-REQUEST-001）→ 发 128B 数据包
///   （末包按 8 字节边界最小补 FF，仍用 cmd 0x02，OTA-FINAL-001）；
/// - 收满全部字节后设备自行完成并回 ACK(0x03)（无需完成帧）；
/// - 静默 ~10s 设备复位传输并回 ACK(0x01, 0) = 从 0 号包重来（OTA-RESET-001，上限 2 次）。
@MainActor
@Observable
public final class OTAEngine {
    public enum EngineError: Equatable, Sendable {
        case timeout
        case tooManyRestarts
        case cancelled
    }

    public enum State: Equatable, Sendable {
        case idle
        case transferring
        case done
        case failed(EngineError)
    }

    public private(set) var state: State = .idle
    /// 设备已确认写入的包数（最近一次 ACK 请求的包号）。
    public private(set) var packetsDone = 0
    /// 设备复位重传次数。
    public private(set) var restarts = 0

    public let packetCount: Int
    public var progress: Double {
        packetCount == 0 ? 0 : Double(min(packetsDone, packetCount)) / Double(packetCount)
    }

    private let packets: [(index: UInt16, chunk: Data, isFinal: Bool)]
    private var send: ((Data) -> Void)?
    private var watchdog: Task<Void, Never>?
    /// 无 ACK 判超时的窗口（设备自身 10s 静默即复位重传，窗口须大于它）。
    private let ackTimeout: TimeInterval
    private let maxRestarts: Int

    public init(image: Data, ackTimeout: TimeInterval = 12, maxRestarts: Int = 2) {
        packets = OTATransferPlanner.packets(for: image)
        packetCount = packets.count
        self.ackTimeout = ackTimeout
        self.maxRestarts = maxRestarts
    }

    /// 接线发送通道（App 层注入 DeviceSession.sendRaw）。
    public func bind(sender: @escaping (Data) -> Void) {
        self.send = sender
    }

    /// 发起升级：OTA 通知帧 → 设备复位进引导，之后一切由 handle(_:) 驱动。
    public func begin() {
        guard state == .idle else { return }
        state = .transferring
        // OTA-NOTIFY-001：应用帧 0x01 + 0xFFFFFFFF 填充
        send?(HKTFrameEncoder.appFrame(packNum: 0, cmd: CommandCode.otaNotify,
                                       data: HKTFrameEncoder.fillerPayload()))
        armWatchdog()
    }

    /// 喂给引擎一条原始接收帧；是引导层 ACK 则消费并返回 true。
    public func handle(_ data: Data) -> Bool {
        guard state == .transferring, let ack = OTAACK.parse(data) else { return false }
        armWatchdog()
        switch ack.kind {
        case .requestPacket:
            guard Int(ack.requestedPacket) < packetCount else { return true }   // 防御：越界请求忽略
            packetsDone = Int(ack.requestedPacket)
            let packet = packets[Int(ack.requestedPacket)]
            send?(OTATransferPlanner.dataFrame(packetIndex: packet.index, chunk: packet.chunk))
        case .transferComplete:
            finish(.done)
        case .restartTransfer:
            restarts += 1
            packetsDone = 0
            if restarts > maxRestarts {
                finish(.failed(.tooManyRestarts))
            }
            // 设备复位计数后随即重发 (0x02, 0) 请求，等待即可
        }
        return true
    }

    public func cancel() {
        guard state == .transferring else { return }
        finish(.failed(.cancelled))
    }

    private func finish(_ outcome: State) {
        watchdog?.cancel()
        watchdog = nil
        state = outcome
    }

    private func armWatchdog() {
        watchdog?.cancel()
        watchdog = Task { [weak self] in
            try? await Task.sleep(for: .seconds(self?.ackTimeout ?? 12))
            guard let self, self.state == .transferring else { return }
            self.finish(.failed(.timeout))
        }
    }
}
