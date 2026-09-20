package com.hkt.ble.bletools.core.protocol

/**
 * Signed-value decoding, preserving the Android sign-bit rules
 * (24-bit `> 0x800000`, 32-bit `> 0x80000000`, 16-bit `> 0x8000`).
 * Used for e.g. UDS100 temperature (×1000, s24 BE) and DC200Family mag axes (s16 BE).
 * 注意：与 [BEValue] 的 i16/i24 补码判定（`>=`）刻意不同——`>` 是安卓现网行为的历史怪癖，逐字保留。
 */
object SignedValueDecoder {
    fun s16(data: ByteArray, offset: Int = 0): Int {
        val v = BEValue.u16(data, offset)
        return if (v > 0x8000) v - 0x10000 else v
    }

    fun s24(data: ByteArray, offset: Int = 0): Int {
        val v = BEValue.u24(data, offset)
        return if (v > 0x800000) v - 0x1000000 else v
    }

    fun s32(data: ByteArray, offset: Int = 0): Long {
        val v = BEValue.u32(data, offset).toLong() and 0xFFFFFFFFL
        return if (v > 0x80000000L) v - 0x100000000L else v
    }
}
