import XCTest
@testable import CoreBLE

/// SP-4 连接三阶段编排（无设备测试：预算可注入小值，超时用真实短等待）。
@MainActor
final class ConnectOrchestratorTests: XCTestCase {
    func testThreePhasesReachConnected() {
        let o = ConnectOrchestrator()
        o.begin(budget: ConnectBudget(link: 10, services: 5, subscribing: 5))
        XCTAssertEqual(o.phase, .link)
        o.advance() // 链路建立 → 发现服务
        XCTAssertEqual(o.phase, .services)
        o.advance() // 服务发现完成 → 订阅
        XCTAssertEqual(o.phase, .subscribing)
        o.advance() // 订阅完成 → 已连接
        XCTAssertTrue(o.isConnected)
        XCTAssertNil(o.failure)
        XCTAssertNil(o.phase)
    }

    func testTimeoutFailsAtCurrentPhase() async {
        let o = ConnectOrchestrator()
        o.begin(budget: ConnectBudget(link: 0.05, services: 5, subscribing: 5))
        try? await Task.sleep(for: .seconds(0.2))
        XCTAssertEqual(o.failure, .timeout(.link))
        XCTAssertFalse(o.isConnected)
        // 结束后再推进不复活状态机
        o.advance()
        XCTAssertNil(o.phase)
        XCTAssertEqual(o.failure, .timeout(.link))
    }

    func testCancelMidPhase() {
        let o = ConnectOrchestrator()
        o.begin()
        o.advance()
        XCTAssertEqual(o.phase, .services)
        o.cancel()
        XCTAssertEqual(o.failure, .cancelled)
        XCTAssertTrue(o.isCancelled)
        XCTAssertFalse(o.isConnected)
    }

    func testAbortReportsCauseAndIgnoresLaterEvents() async {
        let o = ConnectOrchestrator()
        o.begin(budget: ConnectBudget(link: 5, services: 5, subscribing: 5))
        o.advance()
        o.abort(.serviceMissing)
        XCTAssertEqual(o.failure, .serviceMissing)
        o.advance()
        o.cancel()
        XCTAssertEqual(o.failure, .serviceMissing) // 终态后事件被忽略
        XCTAssertFalse(o.isConnected)
    }

    func testRestartAfterFailure() {
        let o = ConnectOrchestrator()
        o.begin(budget: ConnectBudget(link: 0.05, services: 5, subscribing: 5))
        o.cancel()
        o.begin(budget: ConnectBudget(link: 10, services: 5, subscribing: 5))
        XCTAssertNil(o.failure)
        XCTAssertFalse(o.isCancelled)
        XCTAssertEqual(o.phase, .link)
        o.advance()
        o.advance()
        o.advance()
        XCTAssertTrue(o.isConnected)
    }
}
