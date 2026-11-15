package com.hkt.ble.bletools.core.protocol

/**
 * CRC32（IEEE 802.3 反射查表法）。新升级协议 0xF0 通知携带的"更新文件 CRC32 校验码"
 * （整包文件字节）由本实现计算；与 app OtaScreen adoptFile / iOS adoptFile 的私有实现同源。
 */
object CRC32 {
    private val table = IntArray(256) { index ->
        var value = index
        repeat(8) {
            value = if (value and 1 != 0) (0xEDB88320.toInt() xor (value ushr 1)) else (value ushr 1)
        }
        value
    }

    fun ieee(data: ByteArray): Long {
        var crc = 0xFFFF_FFFFL
        for (byte in data) {
            crc = (table[((crc xor byte.toLong()) and 0xFF).toInt()].toLong() and 0xFFFF_FFFFL) xor (crc ushr 8)
        }
        return crc xor 0xFFFF_FFFFL
    }
}
