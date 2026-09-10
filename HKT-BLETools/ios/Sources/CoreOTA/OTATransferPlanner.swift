import Foundation
import CoreProtocol

/// Bootloader OTA transfer framing.
/// Firmware authority: */Compents/BootLoader/uart.c（页大小 FLASH_ONE_PAGE_SIZE=2048B）
/// - frame: hkt(3) len(2 BE) cmd(1) packNum(2 BE) data(n) crc(2) "bootload"(8); `len - 15 == dataLen`
/// - data packets: 128 bytes fixed, packet numbers start at **0** (first device ACK is (0x02, 0));
///   0x02 in the ACK is the command code, the following 2 bytes are the requested packet number.
/// - final chunk padded with FF to the 8-byte boundary (copy granularity only) and **sent with
///   cmd 0xFF to force the flash write**: page size is 2048 B, so a non-2048-aligned image never
///   fills the page buffer and without the flush the transfer never completes.
/// - completion ACK(cmd=0x03) does NOT jump — the host must follow with the cmd=3 finish frame
///   (`finishFrame`) for `AppProgramRun()`; otherwise the device stays in the bootloader.
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

    /// Bootloader start frame (cmd 1, carries the firmware size in bytes):
    /// `hkt len(2)=0005 cmd(1) size(4 BE) crc(2) bootload(8)` — no pack number.
    /// Bootloader uart.c `fromBleDataHandle` case 1 reads size at data[6..9], answers
    /// ACK(cmd+1=2, 0) and starts requesting packets. App firmware ignores this frame
    /// (bootload suffix), so the sender must re-send it after the device reboots into
    /// the bootloader (Android re-sends every 500 ms while ENTER_OTA is pending).
    public static func startFrame(sizeBytes: Int) -> Data {
        let size = UInt32(truncatingIfNeeded: sizeBytes)
        var body = Data([0x01,
                         UInt8((size >> 24) & 0xFF),
                         UInt8((size >> 16) & 0xFF),
                         UInt8((size >> 8) & 0xFF),
                         UInt8(size & 0xFF)])
        var frame = Data([0x68, 0x6B, 0x74])
        frame.append(UInt8(body.count >> 8))
        frame.append(UInt8(body.count & 0xFF))
        frame.append(body)
        let crc = CRC16.ccitt(body)
        frame.append(UInt8(crc >> 8))
        frame.append(UInt8(crc & 0xFF))
        frame.append(contentsOf: bootloadSuffix)
        return frame
    }

    /// Bootloader finish frame (cmd 3, no data/packNum): `hkt 0001 03 crc bootload` (16 B).
    /// The ONLY jump-to-app trigger (uart.c case 3: InfoUartAck(3,0) → AppProgramRun);
    /// the size-threshold completion path merely ACKs and never jumps — omitting this
    /// frame leaves the device stuck in the bootloader. CRC covers the single cmd byte.
    public static func finishFrame() -> Data {
        let body = Data([0x03])
        var frame = Data([0x68, 0x6B, 0x74])
        frame.append(UInt8(body.count >> 8))
        frame.append(UInt8(body.count & 0xFF))
        frame.append(body)
        let crc = CRC16.ccitt(body)
        frame.append(UInt8(crc >> 8))
        frame.append(UInt8(crc & 0xFF))
        frame.append(contentsOf: bootloadSuffix)
        return frame
    }

    /// Bootloader ACK frame shape (uart.c InfoUartAck): hkt cmd count(2) bootload — 14 B.
    /// Documented here because OTAACK.parse mirrors it.

    /// Builds one data packet frame. Regular packets use cmd 0x02; **the final packet must use
    /// cmd 0xFF (`forceFlush: true`)**: the page buffer is 2048 B and only `cmd == 0xFF` (or a
    /// full 2048 B page) triggers the flash write followed by the completion ACK(3,0) — a
    /// partially-filled last packet sent as cmd 0x02 would stall the transfer forever.
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
