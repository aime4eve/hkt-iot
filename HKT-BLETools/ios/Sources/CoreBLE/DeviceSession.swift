import CoreProtocol
import Foundation

/// 设备会话（R-5/R-7/R-8）：1s 轮询 0xFF 查询 → TLV 解码 → 快照发布。
/// 传输无关：面向 PeripheralLink（真 = SystemCentral 链路，假 = MockLink 夹具应答）。
/// - R-7：最后成功响应距今超过 staleAfter 秒 → isStale（界面显示"最后更新 x 秒前"）；
/// - R-8/S-6：响应尾部未知类型 → unknownTail 标记（已解析前缀保留，不崩溃不跳过）；
/// - 字段保留上次有效值（轮询整体刷新语义，SP-5）。
@MainActor
@Observable
public final class DeviceSession {
    public private(set) var snapshot: DeviceSnapshot
    public private(set) var unknownTail = false
    public private(set) var lastResponseAt: Date?
    /// 距最后成功响应的秒数（nil = 尚无响应）。
    public private(set) var secondsSinceLastResponse: Int?
    public private(set) var pollsSent = 0
    public private(set) var isPolling = false
    public private(set) var linkLost = false
    public private(set) var isTimeSyncing = false
    /// 对时完成提示态（原型 tsState：同步中 → 完成 2.6s 后回落）。
    public private(set) var timeSyncDone = false

    /// R-7 停摆判定窗口（秒）。
    public var staleAfter: Int = 4

    public let family: DeviceFamily
    public let deviceName: String

    private let link: any PeripheralLink
    private let pollInterval: TimeInterval
    private var pollTask: Task<Void, Never>?
    private var packNum: UInt8 = 0
    private var stopped = false

    public init(family: DeviceFamily, deviceName: String, link: any PeripheralLink, pollInterval: TimeInterval = 1.0) {
        self.family = family
        self.deviceName = deviceName
        self.link = link
        self.pollInterval = pollInterval
        snapshot = DeviceSnapshot(family: family)
    }

    /// R-7：链路丢失 / 响应过旧 / 首响应迟迟不来。
    public var isStale: Bool {
        if linkLost { return true }
        if let seconds = secondsSinceLastResponse { return seconds > staleAfter }
        return pollsSent >= 3
    }

    public func start() {
        guard !isPolling else { return }
        isPolling = true
        stopped = false
        link.onReceive = { [weak self] data in
            MainActor.assumeIsolated {
                self?.handleReceive(data)
            }
        }
        link.onDisconnected = { [weak self] in
            MainActor.assumeIsolated {
                self?.linkLost = true
            }
        }
        pollTask = Task { [weak self] in
            while let self, !self.stopped, !Task.isCancelled {
                self.pollOnce()
                try? await Task.sleep(for: .seconds(self.pollInterval))
            }
        }
    }

    /// R-31 手动断开 / 页面退出：停止轮询并断开链路（调用方负责关停传输层连接）。
    public func stop() {
        stopped = true
        isPolling = false
        pollTask?.cancel()
    }

    /// 发送一条协议帧（界面动作调用）。
    public func send(cmd: UInt8, data: Data) {
        packNum &+= 1
        link.send(HKTFrameEncoder.appFrame(packNum: packNum, cmd: cmd, data: data))
    }

    /// SP-25 对时：发送手机当前 Unix 秒（App 不做时区换算）。
    public func sendTimeSync() {
        guard !isTimeSyncing else { return }
        isTimeSyncing = true
        send(cmd: CommandCode.timeSync,
             data: HKTFrameEncoder.timeSyncFrame(packNum: packNum &+ 1,
                                                  stampBE: UInt32(Date().timeIntervalSince1970)))
        Task { [weak self] in
            try? await Task.sleep(for: .seconds(1.2))
            guard let self else { return }
            await MainActor.run {
                self.isTimeSyncing = false
                self.timeSyncDone = true
            }
            try? await Task.sleep(for: .seconds(2.6))
            await MainActor.run { self.timeSyncDone = false }
        }
    }

    private func pollOnce() {
        guard !stopped else { return }
        pollsSent &+= 1
        packNum &+= 1
        link.send(HKTFrameEncoder.appFrame(packNum: packNum, cmd: CommandCode.query,
                                           data: HKTFrameEncoder.fillerPayload()))
        if lastResponseAt == nil {
            secondsSinceLastResponse = pollsSent   // 无首响应：按发送次数近似停摆进度（1 次 ≈ 1s）
        }
    }

    private func handleReceive(_ data: Data) {
        guard let (entries, unknownTail) = try? HKTResponseParser.parse(data, family: family) else { return }
        DeviceSnapshotDecoder.decode(entries, family: family, into: &snapshot)
        self.unknownTail = unknownTail
        lastResponseAt = Date()
        secondsSinceLastResponse = 0
        linkLost = false
    }
}
