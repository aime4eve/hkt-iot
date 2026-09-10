import Foundation

/// P-02 连接三阶段：连接链路 → 发现服务 → 订阅通知。
public enum ConnectPhase: Int, Sendable {
    case link = 0
    case services = 1
    case subscribing = 2

    var next: ConnectPhase? { ConnectPhase(rawValue: rawValue + 1) }
}

/// 连接失败原因（R-G4：带阶段定位，禁止统一"失败"）。
public enum ConnectFailure: Error, Equatable, Sendable {
    /// SP-4 阶段预算超时（连接 10s / 发现 5s / 订阅 5s）
    case timeout(ConnectPhase)
    /// HKT 服务缺失（发现服务阶段）
    case serviceMissing
    /// 链路中断
    case connectionLost
    /// 用户取消
    case cancelled
}

/// SP-4 阶段预算（默认连接 10s / 发现 5s / 订阅 5s；测试可注入小预算）。
public struct ConnectBudget: Sendable {
    public var link: TimeInterval
    public var services: TimeInterval
    public var subscribing: TimeInterval

    public init(link: TimeInterval = 10, services: TimeInterval = 5, subscribing: TimeInterval = 5) {
        self.link = link
        self.services = services
        self.subscribing = subscribing
    }

    public func budget(for phase: ConnectPhase) -> TimeInterval {
        switch phase {
        case .link: return link
        case .services: return services
        case .subscribing: return subscribing
        }
    }
}

/// P-02/SP-4 连接三阶段编排：阶段推进 + 阶段预算超时 + 用户取消。
/// 传输无关：SystemCentral（真蓝牙）在系统回调里调用 advance()/abort()，
/// MockCentral（假蓝牙源）按脚本调用——两条轨道共用同一套状态机。
@MainActor
@Observable
public final class ConnectOrchestrator {
    public private(set) var phase: ConnectPhase?
    public private(set) var failure: ConnectFailure?
    public private(set) var isConnected = false
    public private(set) var isCancelled = false

    private var budget = ConnectBudget()
    private var timeoutTask: Task<Void, Never>?
    private var finished = false

    /// 终态回调（failure == nil 表示已连接）；App 层据此取消底层连接尝试并落驻留会话。
    public var onFinish: (@MainActor (ConnectFailure?) -> Void)?

    public init() {}

    /// 会话仍在（未到终态）。
    public var isActive: Bool { phase != nil && !finished }

    /// 开始三阶段（可重复调用，内部先复位）。
    public func begin(budget: ConnectBudget = ConnectBudget()) {
        timeoutTask?.cancel()
        self.budget = budget
        failure = nil
        isConnected = false
        isCancelled = false
        finished = false
        phase = .link
        armTimeout(.link)
    }

    /// 传输层回调：当前阶段完成（链路建立/服务发现完成/订阅完成）。
    public func advance() {
        guard isActive, let phase else { return }
        timeoutTask?.cancel()
        guard let next = phase.next else {
            succeed()
            return
        }
        self.phase = next
        armTimeout(next)
    }

    /// 传输层回调：链路中断 / HKT 服务缺失等不可继续错误。
    public func abort(_ failure: ConnectFailure) {
        guard isActive else { return }
        fail(failure)
    }

    /// 用户取消（P-02 取消按钮）。
    public func cancel() {
        guard isActive else { return }
        fail(.cancelled)
    }

    private func succeed() {
        finished = true
        phase = nil
        isConnected = true
        onFinish?(nil)
    }

    private func fail(_ failure: ConnectFailure) {
        finished = true
        phase = nil
        self.failure = failure
        if failure == .cancelled { isCancelled = true }
        onFinish?(failure)
    }

    private func armTimeout(_ phase: ConnectPhase) {
        let seconds = budget.budget(for: phase)
        timeoutTask = Task { [weak self] in
            try? await Task.sleep(for: .seconds(seconds))
            guard let self, !Task.isCancelled, !self.finished, self.phase == phase else { return }
            self.fail(.timeout(phase))
        }
    }
}
