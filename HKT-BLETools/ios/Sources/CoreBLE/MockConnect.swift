import Foundation

/// MockCentral 的连接脚本（演示/测试用）。
public enum MockConnectScript: Sendable {
    /// 依次上报 链路建立→服务发现→订阅完成，间隔 delay 秒
    case success(delay: Double)
    /// 不推进任何事件 → 由编排器按阶段预算判超时（演示超时分支）
    case neverAdvances
}

extension MockCentral: PeripheralLink {
    /// 场景注入：批量设置脚本设备（扫描启动后渐次投放）。
    public func setScriptedDevices(_ devices: [DiscoveredDevice]) {
        scriptedDevices = devices
    }

    /// 会话链路（演示模式：DeviceSession 面向 MockCentral 收发，responder 提供响应）。
    public func makeLink(for device: DiscoveredDevice) -> (any PeripheralLink)? { self }

    /// 连接三阶段（脚本驱动，事件语义与 SystemCentral 完全一致；回调经主线程投递）。
    public func connect(to device: DiscoveredDevice, events: @escaping @Sendable (ConnectEvent) -> Void) {
        connectEvents = events
        connectedName = device.name   // 演示应答按名判家族
        connectedName = device.name
        switch connectScript {
        case .success(let delay):
            deliver { events(.linkEstablished) }
            connectTask = Task { [weak self] in
                guard let self else { return }
                try? await Task.sleep(for: .seconds(delay))
                guard !Task.isCancelled else { return }
                deliver { events(.servicesDiscovered) }
                try? await Task.sleep(for: .seconds(delay))
                guard !Task.isCancelled else { return }
                deliver { events(.notificationsEnabled) }
            }
        case .neverAdvances:
            break
        }
    }

    public func cancelConnect() {
        connectTask?.cancel()
        connectEvents = nil
    }
}
