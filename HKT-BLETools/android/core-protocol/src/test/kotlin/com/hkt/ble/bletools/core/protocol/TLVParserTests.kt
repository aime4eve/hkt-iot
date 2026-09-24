package com.hkt.ble.bletools.core.protocol

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlin.test.fail
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// Response-parse golden vectors: shared/fixtures/response-parse.json (RX-*).
/// Parse + decode each fixture stream, then assert every key of the fixture `expected`
/// object against the DeviceSnapshot. iOS 对照：CoreProtocolTests/TLVParserTests + DeviceSnapshotTests。
class TLVParserTests {
    private fun familyOf(device: String) = when (device) {
        "UDS100" -> DeviceFamily.UDS100
        "SVC100" -> DeviceFamily.SVC100
        else -> DeviceFamily.DC200_FAMILY
    }

    private fun intOf(raw: kotlinx.serialization.json.JsonElement): Int {
        val s = raw.jsonPrimitive.content
        return if (s.startsWith("0x")) s.substring(2).toInt(16) else s.toInt()
    }

    @Test
    fun testAllGoldenResponses() {
        val vectors = Fixtures.jsonArray("response-parse.json")
        assert(vectors.isNotEmpty())
        for (v in vectors) {
            val obj = v.jsonObject
            val id = obj["id"]!!.jsonPrimitive.content
            val device = obj["device"]?.jsonPrimitive?.content ?: "ALL"
            val bytes = Fixtures.hexToBytes(obj["response"]!!.jsonPrimitive.content)
            val expected = obj["expected"]!!.jsonObject

            if (id == "RX-UNKNOWN-TYPE-001") {
                // 未知类型无长度字节 → 无法安全跳过：停在未知处并标记 unknownTail（S-6 修订）
                val parsed = HKTResponseParser.parse(bytes, DeviceFamily.DC200_FAMILY)
                assertTrue(parsed.unknownTail, "$id: unknownTail expected")
                assertEquals(1, parsed.entries.size, "$id: records before unknown type must survive")
                assertEquals(0x01, parsed.entries[0].type)
                continue
            }

            val parsed = HKTResponseParser.parse(bytes, familyOf(device))
            assertEquals(false, parsed.unknownTail, "$id: unexpected unknownTail")
            val snapshot = DeviceSnapshotDecoder.snapshotFrom(parsed.entries, familyOf(device))

            for ((key, raw) in expected) {
                when (key) {
                    "ack" -> assertTrue(parsed.entries.any { it.type == 0xFF }, "$id: ACK record expected")
                    "hardwareVersion" -> assertEquals(intOf(raw), snapshot.hardwareVersion, "$id.$key")
                    "softwareVersion" -> assertEquals(intOf(raw), snapshot.softwareVersion, "$id.$key")
                    "power" -> assertEquals(intOf(raw), snapshot.power, "$id.$key")
                    "battery" -> assertEquals(intOf(raw), snapshot.batteryPercent, "$id.$key")
                    "batteryVoltagemV" -> assertEquals(intOf(raw), snapshot.batteryVoltageMV, "$id.$key")
                    "temperatureMilli" -> assertEquals(intOf(raw), snapshot.temperatureMilli, "$id.$key")
                    "humidityMilli" -> assertEquals(intOf(raw), snapshot.humidityMilli, "$id.$key")
                    // 夹具存原始 mag31 整数；快照已 /1e6
                    "latitude" -> assertEquals(intOf(raw) / 1_000_000.0, snapshot.latitude!!, 1e-9, "$id.$key")
                    "longitude" -> assertEquals(intOf(raw) / 1_000_000.0, snapshot.longitude!!, 1e-9, "$id.$key")
                    "angle" -> assertEquals(intOf(raw), snapshot.angleCenti, "$id.$key")
                    "slant" -> assertEquals(intOf(raw), snapshot.slant, "$id.$key")
                    "htAlarm" -> assertEquals(intOf(raw), snapshot.htAlarm, "$id.$key")
                    "gpsPeriod" -> assertEquals(intOf(raw), snapshot.gpsPeriodMin, "$id.$key")
                    "distance" -> assertEquals(intOf(raw), snapshot.distanceMM, "$id.$key")
                    "overflowState" -> assertEquals(intOf(raw), snapshot.overflowState, "$id.$key")
                    "lowThreshold" -> assertEquals(intOf(raw), snapshot.lowThresholdMM, "$id.$key")
                    "highThreshold" -> assertEquals(intOf(raw), snapshot.highThresholdMM, "$id.$key")
                    "reportPeriod" -> assertEquals(intOf(raw), snapshot.reportPeriodMin, "$id.$key")
                    "parkState" -> assertEquals(intOf(raw), snapshot.parkState, "$id.$key")
                    "parkMode" -> assertEquals(intOf(raw), snapshot.parkMode, "$id.$key")
                    "tamper" -> assertEquals(intOf(raw), snapshot.tamper, "$id.$key")
                    "magX" -> assertEquals(intOf(raw), snapshot.magX, "$id.$key")
                    "magY" -> assertEquals(intOf(raw), snapshot.magY, "$id.$key")
                    "magZ" -> assertEquals(intOf(raw), snapshot.magZ, "$id.$key")
                    "radarSpectrum" -> {
                        val list = raw.toString().filter { it.isDigit() || it == ',' || it == '-' }
                            .split(',').filter { it.isNotEmpty() }.map { it.trim().toInt() }
                        assertEquals(list, snapshot.radarSpectrum, "$id.$key")
                    }
                    "valve1State" -> assertEquals(intOf(raw), snapshot.valve1State, "$id.$key")
                    "valve1Inserted" -> assertEquals(intOf(raw), snapshot.valve1Inserted, "$id.$key")
                    "valve1Pulse" -> assertEquals(intOf(raw), snapshot.valve1Pulse, "$id.$key")
                    "valve2State" -> assertEquals(intOf(raw), snapshot.valve2State, "$id.$key")
                    "valve2Inserted" -> assertEquals(intOf(raw), snapshot.valve2Inserted, "$id.$key")
                    "valve2Pulse" -> assertEquals(intOf(raw), snapshot.valve2Pulse, "$id.$key")
                    "voltageLevel" -> assertEquals(intOf(raw), snapshot.voltageLevel, "$id.$key")
                    "portFunction" -> assertEquals(intOf(raw), snapshot.portFunction, "$id.$key")
                    "stableTime" -> assertEquals(intOf(raw), snapshot.stableTimeS, "$id.$key")
                    "smartPower" -> assertEquals(intOf(raw), snapshot.smartPower, "$id.$key")
                    // 时区编码 25=+03:30（0-26 表，含半小时码）
                    "timezone" -> {
                        val label = raw.jsonPrimitive.content
                        val enc = when (label) {
                            "+03:30" -> 25
                            "+05:30" -> 26
                            else -> fail("unmapped timezone label $label")
                        }
                        assertEquals(enc, snapshot.timezone, "$id.$key")
                    }
                    else -> fail("$id: unmapped expected key $key — extend TLVParserTests")
                }
            }
        }
    }

    @Test
    fun testBadPrefixThrows() {
        assertFailsWith<ResponseParseError.BadPrefix> {
            HKTResponseParser.parse(byteArrayOf(0x00, 0x01, 0x02, 0x00, 0x00), DeviceFamily.UDS100)
        }
    }

    @Test
    fun testTruncatedThrows() {
        // 0x0E 类型声明 2B 但只带 1B → Truncated（未知类型走 unknownTail，两者路径不同）
        val truncated = Fixtures.hexToBytes("686B740001010B1C0E00")
        assertFailsWith<ResponseParseError.Truncated> {
            HKTResponseParser.parse(truncated, DeviceFamily.UDS100)
        }
    }
}
