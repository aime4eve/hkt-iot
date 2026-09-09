import CoreBluetooth

/// 蓝牙系统能力/权限映射（R-24 四态引导 + 就绪/初始化中）。
public enum BLEAvailability: Equatable, Sendable {
    /// CBCentralManager 尚未回报状态（含首次触发系统权限弹窗）
    case initializing
    /// 已授权且蓝牙开启
    case ready
    /// 权限被拒（引导去系统设置）
    case denied
    /// 蓝牙已关闭（引导开启）
    case poweredOff
    /// 设备不支持 BLE
    case unsupported

    public var isUsable: Bool { self == .ready }

    public init(_ state: CBManagerState) {
        switch state {
        case .unknown, .resetting: self = .initializing
        case .poweredOn: self = .ready
        case .poweredOff: self = .poweredOff
        case .unauthorized: self = .denied
        case .unsupported: self = .unsupported
        @unknown default: self = .unsupported
        }
    }
}
