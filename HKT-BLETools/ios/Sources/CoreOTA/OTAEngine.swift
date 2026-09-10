import CoreProtocol
import Foundation

/// OTA 传输引擎（引导层回环，全部由设备 ACK 驱动）。
/// 真机实测修正（2026-09-10，SVC100）后的三阶段契约：
/// 1. **进入升级模式**：app 固件仅在收到 `len=0001` 纯命令帧（communicate.c data[5]==1）
///    或 `len=0005 且 payload[0]==1`（data[7]==1 兜底）时写更新标志并复位——OTA-NOTIFY-001
///    旧夹具（0xFF 填充）两条分支都不命中，真机超时实锤。两帧先后下发，双入口兜底。
/// 2. **启动帧节拍**：设备复位进 bootloader 后**被动等待** cmd=1 启动帧（带固件字节数，
///    uart.c case 1：读 size → ACK(2,0) → start_flag=1）。App 侧每 500 ms 重发启动帧
///    （Android ENTER_OTA 同款节奏；app 固件阶段该帧被忽略，复位后由 bootloader 处理）。
/// 3. **逐包传输**：ACK(2,n) 请求第 n 包（128 B，末包 8 字节边界补 FF 后仍 cmd=2）；
///    写满（firmware_write_size≥size）或末包后回 ACK(3)=完成；1 s 无数据 bootloader 催包
///    ACK(2,n)，10 s 无数据复位传输回 ACK(1,0)=从 0 号包重来（上限 2 次，重置后重发启动帧）。
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
        case enteringBootloader   // 已发进入命令，节拍重发启动帧，等 bootloader ACK(2,0)
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
    private var startBeat: Task<Void, Never>?
    /// 无 ACK 判超时的窗口（bootloader 自身 10 s 静默复位，窗口须大于它）。
    private let ackTimeout: TimeInterval
    private let maxRestarts: Int
    /// 时序参数（默认与 Android 节奏对齐；测试注入短时序）。
    private let enterAAfter: TimeInterval
    private let startBeatAfter: TimeInterval
    private let beatInterval: TimeInterval

    public init(image: Data, ackTimeout: TimeInterval = 12, maxRestarts: Int = 2,
                enterAAfter: TimeInterval = 0.4, startBeatAfter: TimeInterval = 0.9,
                beatInterval: TimeInterval = 0.5) {
        packets = OTATransferPlanner.packets(for: image)
        packetCount = packets.count
        self.ackTimeout = ackTimeout
        self.maxRestarts = maxRestarts
        self.enterAAfter = enterAAfter
        self.startBeatAfter = startBeatAfter
        self.beatInterval = beatInterval
    }

    /// 接线发送通道（App 层注入 DeviceSession.sendRaw）。
    public func bind(sender: @escaping (Data) -> Void) {
        self.send = sender
    }

    /// 发起升级：进入命令（双入口帧）→ 启动帧节拍，之后一切由 handle(_:) 驱动。
    public func begin() {
        guard state == .idle else { return }
        state = .enteringBootloader
        // 入口 B：len=0005 且 payload[0]==1（app 固件 data[7]==1 兜底分支）
        send?(HKTFrameEncoder.appFrame(packNum: 0, cmd: CommandCode.otaNotify,
                                       data: Data([0x01, 0x00, 0x00, 0x00])))
        armWatchdog()
        // 入口 A：len=0001 纯命令帧（app 固件 data[5]==1 分支），错开写 flash + 复位时间；
        // 随后进入启动帧节拍——bootloader 就绪前每 beatInterval 重发（Android ENTER_OTA 同款）
        startBeat = Task { [weak self] in
            try? await Task.sleep(for: .seconds(self?.enterAAfter ?? 0.4))
            guard let self, self.state == .enteringBootloader else { return }
            self.send?(HKTFrameEncoder.appFrame(packNum: 0, cmd: CommandCode.otaNotify, data: Data()))
            try? await Task.sleep(for: .seconds(self.startBeatAfter - self.enterAAfter))
            while self.state == .enteringBootloader, !Task.isCancelled {
                self.send?(OTATransferPlanner.startFrame(sizeBytes: self.totalBytes))
                try? await Task.sleep(for: .seconds(self.beatInterval))
            }
        }
    }

    /// 喂给引擎一条原始接收帧；是引导层 ACK 则消费并返回 true。
    public func handle(_ data: Data) -> Bool {
        guard (state == .enteringBootloader || state == .transferring),
              let ack = OTAACK.parse(data) else { return false }
        armWatchdog()
        switch ack.kind {
        case .requestPacket:
            if state == .enteringBootloader { state = .transferring; stopStartBeat() }
            guard Int(ack.requestedPacket) < packetCount else { return true }   // 防御：越界请求忽略
            packetsDone = Int(ack.requestedPacket)
            let packet = packets[Int(ack.requestedPacket)]
            send?(OTATransferPlanner.dataFrame(packetIndex: packet.index, chunk: packet.chunk))
        case .transferComplete:
            finish(.done)
        case .restartTransfer:
            // bootloader 静默 10 s 复位传输：计数清零、重发启动帧从头起步
            restarts += 1
            packetsDone = 0
            guard restarts <= maxRestarts else {
                finish(.failed(.tooManyRestarts))
                return true
            }
            state = .enteringBootloader
            stopStartBeat()
            startBeat = Task { [weak self] in
                try? await Task.sleep(for: .seconds(self?.beatInterval ?? 0.5))
                while let self, self.state == .enteringBootloader {
                    self.send?(OTATransferPlanner.startFrame(sizeBytes: self.totalBytes))
                    try? await Task.sleep(for: .seconds(self.beatInterval))
                }
            }
        }
        return true
    }

    public func cancel() {
        guard state == .enteringBootloader || state == .transferring else { return }
        finish(.failed(.cancelled))
    }

    private var totalBytes: Int {
        packets.reduce(0) { $0 + $1.chunk.count }
    }

    private func finish(_ outcome: State) {
        watchdog?.cancel()
        watchdog = nil
        stopStartBeat()
        state = outcome
    }

    private func stopStartBeat() {
        startBeat?.cancel()
        startBeat = nil
    }

    private func armWatchdog() {
        watchdog?.cancel()
        watchdog = Task { [weak self] in
            try? await Task.sleep(for: .seconds(self?.ackTimeout ?? 12))
            // sleep 被 cancel（新 watchdog 接管）时直接退出，不得沿用旧超时判定
            guard let self, !Task.isCancelled else { return }
            if self.state == .enteringBootloader || self.state == .transferring {
                self.finish(.failed(.timeout))
            }
        }
    }
}
