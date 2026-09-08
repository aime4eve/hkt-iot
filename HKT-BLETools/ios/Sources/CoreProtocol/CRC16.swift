/// CRC16-CCITT reflected implementation (KERMIT), per shared/protocol/crc16.md.
/// Firmware authority: HKT-Firmwares/*/Compents/BootLoader/crclib.c (crc16_ccitt).
/// Golden vectors: shared/fixtures/crc16.json (abc→58E9, 123456789→2189).
import Foundation
public enum CRC16 {
    public static func ccitt(_ data: Data) -> UInt16 {
        var crc: UInt16 = 0x0000
        for byte in data {
            crc ^= UInt16(byte)
            for _ in 0..<8 {
                crc = (crc & 1) == 1 ? (crc >> 1) ^ 0x8408 : crc >> 1
            }
        }
        return crc
    }
}
