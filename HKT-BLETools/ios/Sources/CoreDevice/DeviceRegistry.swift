import Foundation
import CoreProtocol

/// Device name → profile registry (requirement §4; Android parseDeviceType compatibility).
/// EPS100/MPS100/DC200 all normalize to the DC200Family profile; unknown names never connect.
public struct DeviceProfile: Equatable, Sendable {
    public let family: DeviceFamily
    public let advertisedName: String
    public let capabilities: Set<DeviceCapability>

    public init(family: DeviceFamily, advertisedName: String, capabilities: Set<DeviceCapability>) {
        self.family = family
        self.advertisedName = advertisedName
        self.capabilities = capabilities
    }
}

public enum DeviceCapability: String, Sendable {
    case ota
    case statusQuery
    case powerControl       // 0xFE — protocol modeled, UI deferred (Q1)
    case calibration        // 0xFD — long-running, async completion
    case basicConfig        // 0x02
    case svcTasks           // 0x03/0x04/0x05
    case timeSync           // 0x06
    case targetedScan       // reserved, v1 closed (Q4)
    case valveControl       // firmware-only 0xF9, never exposed (Q1)
}

public enum DeviceRegistry {
    /// Advertised names → family (Ultrasonic `config.h` PROCT_NAME; ParkingSensor `#if/#else`
    /// branches EPS100/MPS100; Solenoid SVC100).
    static let familiesByAdvertisedName: [String: DeviceFamily] = [
        "UDS100": .uds100,
        "DC200": .dc200Family,
        "EPS100": .dc200Family,
        "MPS100": .dc200Family,
        "SVC100": .svc100,
    ]

    public static func match(advertisedName: String) -> DeviceProfile? {
        guard let family = familiesByAdvertisedName[advertisedName] else { return nil }
        var capabilities: Set<DeviceCapability> = [.ota, .statusQuery, .powerControl, .basicConfig, .timeSync, .targetedScan]
        if family != .svc100 {
            capabilities.insert(.calibration)   // UDS100 accelerometer/tilt; DC200Family magnetometer (async)
        }
        if family == .svc100 {
            capabilities.insert(.svcTasks)
            capabilities.insert(.valveControl)  // registered, never surfaced (Q1)
        }
        return DeviceProfile(family: family, advertisedName: advertisedName, capabilities: capabilities)
    }

    /// 广播名匹配（Android parseDeviceType 同语义：包含型号关键字即可，如 "UDS100 3F2A"）。
    public static func matchBroadcast(_ broadcastName: String?) -> DeviceProfile? {
        guard let broadcastName else { return nil }
        for (key, family) in familiesByAdvertisedName where broadcastName.contains(key) {
            return match(advertisedName: key)
        }
        return nil
    }

    /// 广播名中的设备 EUI（用户裁决 2026-09-24：详情页头部显示 EUI 而非广播名）。
    /// 真机广播名 = 型号关键字 + 分隔符 + EUI 段（如 "SVC100_0D26CF"→"0D26CF"，与 R-2 定位
    /// 后缀匹配同一 EUI 段）；去掉型号关键字与分隔符后须全为 hex 才认，否则返回 nil（交调用方回退原值）。
    public static func euiOf(_ broadcastName: String?) -> String? {
        guard var rest = broadcastName?.uppercased() else { return nil }
        for key in familiesByAdvertisedName.keys {
            rest = rest.replacingOccurrences(of: key, with: "")
        }
        let eui = rest.trimmingCharacters(in: CharacterSet(charactersIn: " _-"))
        guard !eui.isEmpty, eui.allSatisfy({ $0.isHexDigit }) else { return nil }
        return eui
    }
}
