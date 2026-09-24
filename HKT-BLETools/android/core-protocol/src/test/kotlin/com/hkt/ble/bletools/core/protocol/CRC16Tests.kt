package com.hkt.ble.bletools.core.protocol

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// CRC16 golden vectors: shared/fixtures/crc16.json (abc→58E9, 123456789→2189).
/// iOS 对照：CoreProtocolTests/CRC16Tests。
class CRC16Tests {
    @Test
    fun testGoldenVectors() {
        val vectors = Fixtures.jsonArray("crc16.json")
        assert(vectors.isNotEmpty())
        for (v in vectors) {
            val obj = v.jsonObject
            val input = Fixtures.hexToBytes(obj["hex"]!!.jsonPrimitive.content)
            val expected = obj["expected"]!!.jsonPrimitive.content
            assertEquals(expected, "%04X".format(CRC16.ccitt(input)), "vector ${obj["name"]}")
        }
    }

    @Test
    fun testKnownValues() {
        assertEquals(0x58E9, CRC16.ccitt("abc".toByteArray()))
        assertEquals(0x2189, CRC16.ccitt("123456789".toByteArray()))
    }
}
