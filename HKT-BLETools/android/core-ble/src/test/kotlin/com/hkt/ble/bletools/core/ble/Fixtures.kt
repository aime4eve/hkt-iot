package com.hkt.ble.bletools.core.ble

import kotlinx.serialization.json.Json
import kotlinx.serialization.json.jsonArray

/** 测试工具：hex ↔ bytes、classpath 夹具加载（真源 = 仓库级 shared/fixtures）。 */
object Fixtures {
    fun load(name: String) = requireNotNull(Fixtures::class.java.getResourceAsStream("/$name")) {
        "fixture $name not on test classpath (check srcDir ../../shared/fixtures)"
    }.readBytes().decodeToString()

    fun jsonArray(name: String) = Json.parseToJsonElement(load(name)).jsonArray

    fun hexToBytes(hex: String): ByteArray {
        val clean = hex.trim()
        val bytes = ByteArray(clean.length / 2)
        for (i in bytes.indices) {
            bytes[i] = clean.substring(i * 2, i * 2 + 2).toInt(16).toByte()
        }
        return bytes
    }

    fun bytesToHex(bytes: ByteArray): String = bytes.joinToString("") { "%02X".format(it) }
}
