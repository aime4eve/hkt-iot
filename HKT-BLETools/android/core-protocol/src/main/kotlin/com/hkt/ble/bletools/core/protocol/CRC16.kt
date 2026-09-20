package com.hkt.ble.bletools.core.protocol

/**
 * CRC16-CCITT reflected implementation (KERMIT), per shared/protocol/crc16.md.
 * Firmware authority: HKT-Firmwares 各工程 Compents/BootLoader/crclib.c (crc16_ccitt).
 * Golden vectors: shared/fixtures/crc16.json (abc→58E9, 123456789→2189).
 */
object CRC16 {
    fun ccitt(data: ByteArray): Int {
        var crc = 0x0000
        for (byte in data) {
            crc = crc xor (byte.toInt() and 0xFF)
            repeat(8) {
                crc = if ((crc and 1) == 1) (crc ushr 1) xor 0x8408 else crc ushr 1
            }
        }
        return crc and 0xFFFF
    }
}
