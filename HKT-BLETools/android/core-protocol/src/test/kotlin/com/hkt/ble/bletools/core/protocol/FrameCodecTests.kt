package com.hkt.ble.bletools.core.protocol

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.fail
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// App-frame TX golden vectors: shared/fixtures/app-frame-tx.json (TX-*).
/// Every fixture frame is rebuilt with HKTFrameEncoder and compared byte-for-byte against
/// the recorded `request` hex — the fixture is the authority, encoder params live in this map.
/// iOS 对照：CoreProtocolTests/FrameCodecTests + ConfigBoundaryTests（编码部分）。
class FrameCodecTests {
    private fun buildFor(id: String): ByteArray = when (id) {
        "TX-QUERY-001" -> HKTFrameEncoder.appFrame(0, CommandCode.QUERY, HKTFrameEncoder.fillerPayload())
        "TX-DC200-CFG-001" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.dcConfigPayload(30, 1))
        // 夹具 payload 000001 = period 0 + mode 1（"mode must stay 0-2" 边界沿用合法值 1）
        "TX-DC200-CFG-002" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.dcConfigPayload(0, 1))
        "TX-DC200-CFG-003" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.dcConfigPayload(30, 3))
        "TX-UDS-CFG-001" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.udsConfigPayload(30, 60, 300, 3000))
        "TX-UDS-CFG-002" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.udsConfigPayload(30, 60, 29, 3000))
        "TX-SVC-CFG-001" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.svcConfigPayload(2, 1, 5, 1, 25, 30))
        "TX-SVC-CFG-002" -> HKTFrameEncoder.appFrame(0, CommandCode.CONFIG, HKTFrameEncoder.svcConfigPayload(2, 1, 5, 1, 27, 30))
        // 历史裁决：pulse 为 2B（len=7 门槛 = cmd+6B 载荷），夹具 request 为准
        "TX-SVC-TASK-001" -> HKTFrameEncoder.appFrame(0, CommandCode.SVC_REALTIME_TASK, HKTFrameEncoder.svcRealtimeTaskPayload(1, 1, 5, 100))
        "TX-SVC-TASK-002" -> HKTFrameEncoder.appFrame(0, CommandCode.SVC_TIMED_TASK, HKTFrameEncoder.svcTimedTaskPayload(1, 1, 1, 100, 480, 1110, 0x7F))
        "TX-SVC-TASK-003" -> HKTFrameEncoder.appFrame(0, CommandCode.SVC_DELETE_TASK, HKTFrameEncoder.svcDeleteTaskPayload(0xFF))
        "TX-POWER-001" -> HKTFrameEncoder.appFrame(0, CommandCode.POWER, HKTFrameEncoder.powerPayload(true))
        "TX-CAL-001" -> HKTFrameEncoder.appFrame(0, CommandCode.CALIBRATE, HKTFrameEncoder.fillerPayload())
        "TX-SYNC-001" -> HKTFrameEncoder.timeSyncFrame(0, 0x69546780L)
        else -> fail("fixture id $id has no encoder mapping — add it here when extending app-frame-tx.json")
    }

    @Test
    fun testAllGoldenFramesByteIdentical() {
        val vectors = Fixtures.jsonArray("app-frame-tx.json")
        assert(vectors.isNotEmpty())
        for (v in vectors) {
            val obj = v.jsonObject
            val id = obj["id"]!!.jsonPrimitive.content
            val expected = obj["request"]!!.jsonPrimitive.content.uppercase()
            val built = Fixtures.bytesToHex(buildFor(id))
            assertEquals(expected, built, "frame mismatch for $id (${obj["purpose"]})")
        }
    }

    @Test
    fun testAckRecordShape() {
        // expectedAck "686B740000FFFF" = hkt + 0x0000 + ACK 记录 0xFFFF（无帧 CRC）
        val ack = Fixtures.hexToBytes("686B740000FFFF")
        val parsed = HKTResponseParser.parse(ack, DeviceFamily.DC200_FAMILY)
        assertEquals(1, parsed.entries.size)
        assertEquals(0xFF, parsed.entries[0].type)
        assertEquals(false, parsed.unknownTail)
    }

    @Test
    fun testFillerPayloadDefaults() {
        assertEquals("FFFFFFFF", Fixtures.bytesToHex(HKTFrameEncoder.fillerPayload()))
        assertEquals("00000001", Fixtures.bytesToHex(HKTFrameEncoder.powerPayload(true)))
        assertEquals("00000000", Fixtures.bytesToHex(HKTFrameEncoder.powerPayload(false)))
    }
}
