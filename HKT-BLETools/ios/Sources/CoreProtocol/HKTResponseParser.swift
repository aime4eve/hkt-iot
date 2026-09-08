/// Device→App response stream parser.
///
/// Firmware authority (all three projects, callback_BLEQuery/callback_BLESearch tails):
/// the device answers `hkt(3) 0x00 seq(1) TLV-stream` where every record is
/// `type(1) + fixed-size value` — there is **no length byte and no frame CRC**.
/// Consequences (design spec §2.2, S-6 revision): an unknown type cannot be
/// skipped safely, so parsing fails with `dataAbnormal` instead of skipping.
///
/// Vectors: shared/fixtures/response-parse.json (RX-*).
import Foundation
public struct TLVEntry: Equatable, Sendable {
    public let type: UInt8
    public let value: Data

    public init(type: UInt8, value: Data) {
        self.type = type
        self.value = value
    }
}

public enum DeviceFamily: String, Sendable {
    case uds100
    case dc200Family
    case svc100
}

public enum ResponseParseError: Error, Equatable, Sendable {
    case badPrefix
    case truncated
}

enum TLVSizeTable {
    /// Fixed value size per type; `nil` = unknown for this family.
    static func size(of type: UInt8, family: DeviceFamily) -> Int? {
        switch family {
        case .dc200Family:
            switch type {
            case 0x01: return 2  // hw/sw version
            case 0x03: return 1  // battery
            case 0x3A: return 1  // park state
            case 0x3B: return 1  // park mode
            case 0x5D, 0x5E, 0x5F: return 2  // mag X/Y/Z int16 BE
            case 0x60: return 20 // radar spectrum 10 × uint16 BE
            case 0x80: return 4  // sync time
            case 0x84: return 1  // tamper
            case 0x86: return 2  // report period
            case 0x8D: return 1  // power
            case 0xFF: return 1  // ACK
            default: return nil
            }
        case .uds100:
            switch type {
            case 0x01: return 2
            case 0x09, 0x0A: return 3  // temp/humidity, value ×1000, s24 BE
            case 0x0E: return 2  // angle
            case 0x10, 0x11: return 4  // latitude / longitude
            case 0x28: return 1  // HT alarm
            case 0x44: return 1  // slant
            case 0x45: return 2  // GPS period
            case 0x46: return 2  // distance
            case 0x47: return 1  // overflow state
            case 0x48: return 4  // overflow config: low(2)+high(2) BE
            case 0x80: return 4
            case 0x86: return 2
            case 0x8B: return 2  // battery voltage mV
            case 0x8D: return 1
            case 0xFF: return 1
            default: return nil
            }
        case .svc100:
            switch type {
            case 0x01: return 2
            case 0x03: return 1
            case 0x3C: return 8  // dual valve: v1s, v1i, pulse1(2), v2s, v2i, pulse2(2)
            case 0x40, 0x41, 0x42, 0x43: return 1  // voltage level / port function / stable time / smart power
            case 0x80: return 4
            case 0x86: return 2
            case 0x8A: return 1  // timezone (0-26, 25=+03:30, 26=+05:30)
            case 0x8D: return 1
            case 0xFF: return 1
            default: return nil
            }
        }
    }
}

public enum HKTResponseParser {
    public static let prefix: [UInt8] = [0x68, 0x6B, 0x74]

    /// Parses a device response stream. On an unknown type the records parsed so far are
    /// returned with `unknownTail == true` (forward compatibility: fields appended at the
    /// tail by newer firmware never hide earlier fields, and the UI shows the data-abnormal
    /// state instead of crashing or silently skipping — S-6 M3 revision).
    public static func parse(_ data: Data, family: DeviceFamily) throws -> (entries: [TLVEntry], unknownTail: Bool) {
        let bytes = [UInt8](data)
        guard bytes.count >= 5 else { throw ResponseParseError.truncated }
        guard Array(bytes[0..<3]) == Self.prefix else { throw ResponseParseError.badPrefix }
        // bytes[3] == 0x00, bytes[4] == rolling sequence number (packSyncNumber++).
        var entries: [TLVEntry] = []
        var unknownTail = false
        var offset = 5
        walk: while offset < bytes.count {
            let type = bytes[offset]
            guard let size = TLVSizeTable.size(of: type, family: family) else {
                unknownTail = true
                break walk
            }
            let end = offset + 1 + size
            guard end <= bytes.count else { throw ResponseParseError.truncated }
            entries.append(TLVEntry(type: type, value: Data(bytes[(offset + 1)..<end])))
            offset = end
        }
        return (entries, unknownTail)
    }
}

/// Multi-byte protocol values are big-endian (all firmware setDataPackage cases shift >>8 first).
extension FixedWidthInteger {
    var dataBE: Data {
        var v = self.bigEndian
        return withUnsafeBytes(of: &v) { Data($0) }
    }
}

public enum BEValue {
    public static func u16(_ data: Data) -> Int { Int(data[data.startIndex]) << 8 | Int(data[data.startIndex + 1]) }
    public static func u24(_ data: Data) -> Int {
        Int(data[data.startIndex]) << 16 | Int(data[data.startIndex + 1]) << 8 | Int(data[data.startIndex + 2])
    }
    public static func u32(_ data: Data) -> Int {
        u24(data) << 8 | Int(data[data.startIndex + 3])
    }
}
