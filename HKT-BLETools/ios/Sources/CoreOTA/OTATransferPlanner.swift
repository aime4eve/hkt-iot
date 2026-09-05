import Foundation
import CoreProtocol

/// Bootloader OTA transfer framing.
/// Firmware authority: */Compents/BootLoader/uart.c
/// - frame: hkt(3) len(2 BE) cmd(1) packNum(2 BE) data(n) crc(2) "bootload"(8); `len - 15 == dataLen`
/// - data packets: 128 bytes fixed, packet numbers start at **0** (first device ACK is (0x02, 0));
///   0x02 in the ACK is the command code, the following 2 bytes are the requested packet number.
/// - final chunk padded with FF to the 8-byte boundary; a final packet sent with cmd 0xFF flushes
///   flash and the device replies ACK(cmd=0x03) on completion.
/// - after ~10 s of silence the device resets the transfer and replies ACK(cmd=0x01, 0) = restart.
/// Vectors: shared/fixtures/ota-transfer.json (OTA-*).
public enum OTATransferPlanner {
    public static let chunkSize = 128
    static let bootloadSuffix: [UInt8] = [0x62, 0x6F, 0x6F, 0x74, 0x6C, 0x6F, 0x61, 0x64]

    /// Pads a final chunk with 0xFF up to the 8-byte boundary.
    public static func padded(_ chunk: Data) -> Data {
        var padded = chunk
        while padded.count % 8 != 0 {
            padded.append(0xFF)
        }
        return padded
    }

    /// Builds one data packet frame. Normal packets (and the final one) use cmd 0x02 — the
    /// device completes on its own once `firmware_write_size >= firmware_size` and replies
    /// ACK(cmd=0x03). `forceFlush: true` sends the packet as cmd 0xFF, forcing a flash flush
    /// (firmware case "0xFF || 2"); only needed as a recovery path.
    public static func dataFrame(packetIndex: UInt16, chunk: Data, forceFlush: Bool = false) -> Data {
        let payload = padded(chunk)
        var body = Data([forceFlush ? 0xFF : 0x02])
        body.append(UInt8(packetIndex >> 8))
        body.append(UInt8(packetIndex & 0xFF))
        body.append(payload)
        let length = UInt16(body.count)

        var frame = Data([0x68, 0x6B, 0x74])
        frame.append(UInt8(length >> 8))
        frame.append(UInt8(length & 0xFF))
        frame.append(body)
        let crc = CRC16.ccitt(body)
        frame.append(UInt8(crc >> 8))
        frame.append(UInt8(crc & 0xFF))
        frame.append(contentsOf: bootloadSuffix)
        return frame
    }

    /// Splits a validated firmware image into (packetIndex, chunk) pairs.
    public static func packets(for image: Data) -> [(index: UInt16, chunk: Data, isFinal: Bool)] {
        var result: [(UInt16, Data, Bool)] = []
        var offset = 0
        while offset < image.count {
            let end = min(offset + chunkSize, image.count)
            let chunk = image.subdata(in: offset..<end)
            let isFinal = end == image.count
            result.append((UInt16(result.count), chunk, isFinal))
            offset = end
        }
        return result
    }
}

/// Parsed device bootloader ACK: hkt(3) cmd(1) count(2 BE) bootload(8) — 14 bytes.
public struct OTAACK: Equatable, Sendable {
    public enum Kind: UInt8, Sendable {
        case restartTransfer = 0x01  // device reset the transfer: start from packet 0
        case requestPacket = 0x02    // device requests `requestedPacket`
        case transferComplete = 0x03
    }

    public let kind: Kind
    public let requestedPacket: UInt16

    public static func parse(_ data: Data) -> OTAACK? {
        let bytes = [UInt8](data)
        guard bytes.count == 14,
              bytes[0] == 0x68, bytes[1] == 0x6B, bytes[2] == 0x74,
              Array(bytes[6...13]) == OTATransferPlanner.bootloadSuffix,
              let kind = Kind(rawValue: bytes[3]) else { return nil }
        let packet = UInt16(bytes[4]) << 8 | UInt16(bytes[5])
        return OTAACK(kind: kind, requestedPacket: packet)
    }
}
