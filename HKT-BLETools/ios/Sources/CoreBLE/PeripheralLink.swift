import Foundation

/// 已连接会话的收发链路（DeviceSession 只面向此协议；真实现 SystemLink，测试/演示 MockLink）。
public protocol PeripheralLink: AnyObject {
    /// 发送一条协议帧（写 Write 特征）。
    func send(_ frame: Data)
    /// 收到一条协议帧（Indicate 通知，主线程投递）。
    var onReceive: (@Sendable (Data) -> Void)? { get set }
    /// 对端断开（R-6 自动重连 / R-31 语义由上层处理）。
    var onDisconnected: (@Sendable () -> Void)? { get set }
}

/// 测试/预览用假链路：responder 按请求帧返回响应帧；记录已发帧供断言。
public final class MockLink: PeripheralLink, @unchecked Sendable {
    private let lock = NSLock()
    private var sent: [Data] = []

    public var responder: (@Sendable (Data) -> Data?)?
    public var onReceive: (@Sendable (Data) -> Void)?
    public var onDisconnected: (@Sendable () -> Void)?

    public init() {}

    public func send(_ frame: Data) {
        lock.lock()
        sent.append(frame)
        lock.unlock()
        if let response = responder?(frame) {
            onReceive?(response)
        }
    }

    /// 场景注入：模拟对端断开。
    public func simulateDisconnect() {
        onDisconnected?()
    }

    public var framesSent: Int {
        lock.lock()
        defer { lock.unlock() }
        return sent.count
    }

    public func sentFrame(at index: Int) -> Data {
        lock.lock()
        defer { lock.unlock() }
        return sent[index]
    }
}
