package com.hkt.ble.bletools.core.protocol

/**
 * Device→App response stream parser.
 *
 * Firmware authority (all three projects, callback_BLEQuery/callback_BLESearch tails):
 * the device answers `hkt(3) 0x00 seq(1) TLV-stream` where every record is
 * `type(1) + fixed-size value` — there is **no length byte and no frame CRC**.
 * Consequences (design spec §2.2, S-6 revision): an unknown type cannot be
 * skipped safely, so parsing stops with `unknownTail` instead of skipping.
 *
 * Vectors: shared/fixtures/response-parse.json (RX-*).
 */

data class TLVEntry(val type: Int, val value: ByteArray) {
    override fun equals(other: Any?): Boolean =
        other is TLVEntry && other.type == type && other.value.contentEquals(value)
    override fun hashCode(): Int = type * 31 + value.contentHashCode()
}

enum class DeviceFamily { UDS100, DC200_FAMILY, SVC100 }

sealed class ResponseParseError(message: String) : Exception(message) {
    data object BadPrefix : ResponseParseError("bad prefix")
    data object Truncated : ResponseParseError("truncated")
}

/** 解析结果：已解出的记录 + 未知尾部标记（前向兼容语义见 [HKTResponseParser.parse]）。 */
data class ParsedResponse(val entries: List<TLVEntry>, val unknownTail: Boolean)

internal object TLVSizeTable {
    /** Fixed value size per type; `null` = unknown for this family. */
    fun sizeOf(type: Int, family: DeviceFamily): Int? = when (family) {
        DeviceFamily.DC200_FAMILY -> when (type) {
            0x01 -> 2  // hw/sw version
            0x03 -> 1  // battery
            0x3A -> 1  // park state
            0x3B -> 1  // park mode
            0x5D, 0x5E, 0x5F -> 2  // mag X/Y/Z int16 BE
            0x60 -> 20 // radar spectrum 10 × uint16 BE
            0x80 -> 4  // sync time
            0x84 -> 1  // tamper
            0x86 -> 2  // report period
            0x8D -> 1  // power
            0xFF -> 1  // ACK
            else -> null
        }
        DeviceFamily.UDS100 -> when (type) {
            0x01 -> 2
            0x09, 0x0A -> 3  // temp/humidity, value ×1000, s24 BE
            0x0E -> 2  // angle
            0x10, 0x11 -> 4  // latitude / longitude
            0x28 -> 1  // HT alarm
            0x44 -> 1  // slant
            0x45 -> 2  // GPS period
            0x46 -> 2  // distance
            0x47 -> 1  // overflow state
            0x48 -> 4  // overflow config: low(2)+high(2) BE
            0x80 -> 4
            0x86 -> 2
            0x8B -> 2  // battery voltage mV
            0x8D -> 1
            0xFF -> 1
            else -> null
        }
        DeviceFamily.SVC100 -> when (type) {
            0x01 -> 2
            0x03 -> 1
            0x3C -> 8  // dual valve: v1s, v1i, pulse1(2), v2s, v2i, pulse2(2)
            0x40, 0x41, 0x42, 0x43 -> 1  // voltage level / port function / stable time / smart power
            0x80 -> 4
            0x86 -> 2
            0x8A -> 1  // timezone (0-26, 25=+03:30, 26=+05:30)
            0x8D -> 1
            0xFF -> 1
            else -> null
        }
    }
}

object HKTResponseParser {
    val prefix = byteArrayOf(0x68, 0x6B, 0x74)

    /**
     * Parses a device response stream. On an unknown type the records parsed so far are
     * returned with `unknownTail == true` (forward compatibility: fields appended at the
     * tail by newer firmware never hide earlier fields, and the UI shows the data-abnormal
     * state instead of crashing or silently skipping — S-6 M3 revision).
     */
    fun parse(data: ByteArray, family: DeviceFamily): ParsedResponse {
        if (data.size < 5) throw ResponseParseError.Truncated
        if (data[0] != prefix[0] || data[1] != prefix[1] || data[2] != prefix[2]) throw ResponseParseError.BadPrefix
        // data[3] == 0x00, data[4] == rolling sequence number (packSyncNumber++).
        val entries = ArrayList<TLVEntry>()
        var unknownTail = false
        var offset = 5
        walk@ while (offset < data.size) {
            val type = data[offset].toInt() and 0xFF
            val size = TLVSizeTable.sizeOf(type, family)
            if (size == null) {
                unknownTail = true
                break@walk
            }
            val end = offset + 1 + size
            if (end > data.size) throw ResponseParseError.Truncated
            entries.add(TLVEntry(type, data.copyOfRange(offset + 1, end)))
            offset = end
        }
        return ParsedResponse(entries, unknownTail)
    }
}

/** Multi-byte protocol values are big-endian (all firmware setDataPackage cases shift >>8 first). */
object BEValue {
    fun u16(data: ByteArray, offset: Int = 0): Int =
        ((data[offset].toInt() and 0xFF) shl 8) or (data[offset + 1].toInt() and 0xFF)
    fun u24(data: ByteArray, offset: Int = 0): Int =
        ((data[offset].toInt() and 0xFF) shl 16) or ((data[offset + 1].toInt() and 0xFF) shl 8) or
            (data[offset + 2].toInt() and 0xFF)
    fun u32(data: ByteArray, offset: Int = 0): Int =
        (u24(data, offset) shl 8) or (data[offset + 3].toInt() and 0xFF)

    /** 二进制补码有符号 16 位（倾角/地磁等） */
    fun i16(data: ByteArray, offset: Int = 0): Int {
        val v = u16(data, offset)
        return if (v >= 0x8000) v - 0x10000 else v
    }

    /** 二进制补码有符号 24 位（温度/湿度毫度） */
    fun i24(data: ByteArray, offset: Int = 0): Int {
        val v = u24(data, offset)
        return if (v >= 0x800000) v - 0x1000000 else v
    }

    /** 32 位符号位幅值（GPS 经纬度固件约定：|值|×1e6 存储后负数置最高位，**非补码**——
     *  gps.c 定位更新 `value |= 0x80000000`）。补码解码会把南纬/西经读成天文数字。 */
    fun mag31(data: ByteArray, offset: Int = 0): Int {
        val v = u32(data, offset)
        return (v and 0x7FFF_FFFF) * (if (v and 0x8000_0000.toInt() != 0) -1 else 1)
    }

    /** 二进制补码有符号 32 位（经纬度）。Kotlin Int=32 位，符号换算走 Long。 */
    fun i32(data: ByteArray, offset: Int = 0): Long {
        val v = u32(data, offset).toLong() and 0xFFFFFFFFL
        return if (v >= 0x80000000L) v - 0x100000000L else v
    }
}
