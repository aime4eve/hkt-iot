import XCTest
import CoreProtocol
@testable import CoreDevice

/// Device recognition vectors — requirement §4; firmware identity per
/// shared/devices/firmware-traceability.md §3 (PROCT_NAME in USER/config.h).
final class DeviceRegistryTests: XCTestCase {
    func testSupportedNamesMatchFamilies() {
        XCTAssertEqual(DeviceRegistry.match(advertisedName: "UDS100")?.family, .uds100)
        XCTAssertEqual(DeviceRegistry.match(advertisedName: "SVC100")?.family, .svc100)
        for name in ["DC200", "EPS100", "MPS100"] {
            XCTAssertEqual(DeviceRegistry.match(advertisedName: name)?.family, .dc200Family, name)
        }
    }

    /// EPS100/MPS100 are build variants of the same ParkingSensor firmware project
    /// (config.h `#if 0` selection) — same family, advertised name preserved.
    func testEPS100AndMPS100ShareFamilyButKeepName() {
        let eps = DeviceRegistry.match(advertisedName: "EPS100")!
        let mps = DeviceRegistry.match(advertisedName: "MPS100")!
        XCTAssertEqual(eps.family, mps.family)
        XCTAssertEqual(eps.advertisedName, "EPS100")
        XCTAssertEqual(mps.advertisedName, "MPS100")
    }

    func testUnknownNameNeverMatches() {
        XCTAssertNil(DeviceRegistry.match(advertisedName: "RBC100"))
        XCTAssertNil(DeviceRegistry.match(advertisedName: ""))
        XCTAssertNil(DeviceRegistry.match(advertisedName: "uds100"))  // case-sensitive
    }

    func testCapabilitySets() {
        let uds = DeviceRegistry.match(advertisedName: "UDS100")!
        XCTAssertTrue(uds.capabilities.contains(.ota))
        XCTAssertTrue(uds.capabilities.contains(.statusQuery))
        XCTAssertFalse(uds.capabilities.contains(.svcTasks))

        let svc = DeviceRegistry.match(advertisedName: "SVC100")!
        XCTAssertTrue(svc.capabilities.contains(.svcTasks))
        XCTAssertTrue(svc.capabilities.contains(.valveControl))  // registered, never exposed (Q1)

        let mps = DeviceRegistry.match(advertisedName: "MPS100")!
        XCTAssertTrue(mps.capabilities.contains(.calibration))   // async magnetometer calibration
    }
}
