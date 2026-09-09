import CoreBLE
import SwiftUI

/// P-02 连接过程模型：ConnectOrchestrator（阶段/预算/取消）+ 端口事件映射。
@MainActor
@Observable
final class ConnectModel: Identifiable {
    enum Outcome: Equatable {
        case connected
        case cancelled
        case failed(ConnectFailure)
    }

    let target: DiscoveredDevice
    let id: UUID

    private(set) var orchestrator = ConnectOrchestrator()
    /// 终态（连接成功 / 取消 / 失败）；nil = 仍在连接过程。
    private(set) var outcome: Outcome?

    private let port: any BluetoothPort
    /// 连接终态回调（RootView 据此置驻留会话/关闭覆盖层）。
    var onFinished: (@MainActor (Outcome) -> Void)?

    init(target: DiscoveredDevice, port: any BluetoothPort) {
        self.target = target
        self.id = target.identifier
        self.port = port
    }

    /// P-02 三步进度序号（0 连接中 / 1 发现服务 / 2 订阅通知；3 = 完成）。
    var phaseIndex: Int {
        if orchestrator.isConnected { return 3 }
        return orchestrator.phase?.rawValue ?? 0
    }

    var failureText: String? {
        guard let failure = orchestrator.failure else { return nil }
        switch failure {
        case .timeout(let phase):
            switch phase {
            case .link: return "连接超时"
            case .services: return "发现服务超时"
            case .subscribing: return "订阅通知超时"
            }
        case .serviceMissing: return "设备缺少 HKT 服务"
        case .connectionLost: return "连接已中断"
        case .cancelled: return "已取消"
        }
    }

    func start() {
        outcome = nil
        orchestrator.onFinish = { [weak self] failure in
            guard let self else { return }
            if let failure {
                port.cancelConnect()
                let result: Outcome = failure == .cancelled ? .cancelled : .failed(failure)
                outcome = result
                onFinished?(result)
            } else {
                outcome = .connected
                onFinished?(.connected)
            }
        }
        port.connect(to: target) { [weak self] event in
            MainActor.assumeIsolated {
                guard let self else { return }
                switch event {
                case .linkEstablished, .servicesDiscovered, .notificationsEnabled:
                    self.orchestrator.advance()
                case .failed(let failure):
                    self.orchestrator.abort(failure)
                }
            }
        }
        orchestrator.begin()
    }

    func cancel() {
        orchestrator.cancel()
        port.cancelConnect()
    }

    func retry() {
        start()
    }
}
