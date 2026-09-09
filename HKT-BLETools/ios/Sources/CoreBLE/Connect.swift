import Foundation

/// HKT 透明桥服务与特征 UUID（Android BLEUtils.kt 核实，固件模组侧配置）。
public enum HKTProfile {
    public static let serviceUUID = UUID(uuidString: "0783B03E-8535-B5A0-7140-A304D2495CB7")!
    /// Indicate 特征：设备 → App（状态/响应）
    public static let indicateUUID = UUID(uuidString: "0783B03E-8535-B5A0-7140-A304D2495CB8")!
    /// Write 特征：App → 设备（命令帧）
    public static let writeUUID = UUID(uuidString: "0783B03E-8535-B5A0-7140-A304D2495CBA")!
}

/// 连接进展事件（完成语义：每个事件 = 对应 ConnectPhase 完成，编排器据此推进）。
public enum ConnectEvent: Equatable, Sendable {
    /// 链路建立（didConnect）
    case linkEstablished
    /// HKT 服务与特征发现完成
    case servicesDiscovered
    /// Indicate 订阅完成（链路就绪，可收发协议帧）
    case notificationsEnabled
    /// 连接过程失败（原因见 ConnectFailure）
    case failed(ConnectFailure)
}
