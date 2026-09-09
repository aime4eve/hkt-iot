import Foundation

/// 已连接外设的收发链路（SD 架构的传输缝）：SystemCentral 提供 GATT 真实现，
/// MockLink 提供可编程假实现（0xFF 查询 → 夹具响应自动应答）。DeviceSession 只面向此协议。
public protocol PeripheralLink: AnyObject {
    /// 发送一条协议帧（写 Write 特征）。
    func send(_ frame: Data)
    /// 收到一条协议帧（Indicate 通知，主线程投递）。
    var onReceive: (@Sendable (Data) -> Void)? { get set }
    /// 对端断开（R-6 自动重连 / R-31 语义由上层处理）。
    var onDisconnected: (@Sendable () -> Void)? { get set }
}

/// 测试/演示用假链路：responder 按请求帧返回响应帧；记录全部已发帧供断言。
public final class MockLink: PeripheralLink, @unchecked Sendable {
    private let lock = NSLock()
    private var sentFrames: [Data] = []

    /// 请求帧 → 响应帧（nil = 不响应，用于停摆场景）。
    public var responder: (@Sendable (Data) -> Data?)?
    public var onReceive: (@Sendable (Data) -> Void)?
    public var onDisconnected: (@Sendable () -> Void)?

    public init() {}

    public func send(_ frame: Data) {
        lock.lock()
        sentFrames.append(frame)
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
        return sentFrames.count
    }

    public func sentFrame(at index: Int) -> Data {
        lock.lock()
        defer { lock.unlock() }
        return sentFrames[index]
    }
}
