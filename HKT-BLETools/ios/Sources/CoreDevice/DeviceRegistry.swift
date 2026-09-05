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
        var capabilities: Set<DeviceCapability> = [.ota, .statusQuery, .powerControl, .calibration, .basicConfig, .timeSync, .targetedScan]
        if family == .svc100 {
            capabilities.insert(.svcTasks)
            capabilities.insert(.valveControl)  // registered, never surfaced (Q1)
        }
        return DeviceProfile(family: family, advertisedName: advertisedName, capabilities: capabilities)
    }
}
