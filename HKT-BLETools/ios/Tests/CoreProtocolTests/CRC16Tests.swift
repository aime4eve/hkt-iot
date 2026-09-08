import XCTest
@testable import CoreProtocol

/// Golden vectors mirrored from shared/fixtures/crc16.json
/// (source of truth; regenerate via scripts/fixtures/gen_fixtures.py).
final class CRC16Tests: XCTestCase {
    func testVectors() {
        XCTAssertEqual(CRC16.ccitt(Data()), 0x0000)
        XCTAssertEqual(CRC16.ccitt(Data("abc".utf8)), 0x58E9)
        XCTAssertEqual(CRC16.ccitt(Data("hkt".utf8)), 0x77D1)
        // KERMIT standard check value — proves the algorithm parameters.
        XCTAssertEqual(CRC16.ccitt(Data("123456789".utf8)), 0x2189)
    }
}
