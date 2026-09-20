package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.ota.Fixtures.bytesToHex
import com.hkt.ble.bletools.core.ota.Fixtures.hexToBytes
import com.hkt.ble.bletools.core.protocol.CRC16
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// OTA transfer golden vectors: shared/fixtures/ota-transfer.json (OTA-*).
/// Wire-format vectors are asserted byte-for-byte; engine behavior vectors are exercised
/// in OtaEngineTests. iOS 对照：CoreOTATests/OTATransferPlannerTests。
class OTATransferPlannerTests {
    @Test
    fun testEnterFramesMatchFixtures() {
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val byId = vectors.associate { it.jsonObject["id"]!!.jsonPrimitive.content to it.jsonObject }
        // 入口 B：len=0005 payload[0]=1；入口 A：len=0001 纯命令（双入口兜底，顺序 B→A）
        assertEquals(
            byId["OTA-ENTER-B"]!!["request"]!!.jsonPrimitive.content.uppercase(),
            bytesToHex(HKTFrameEncoder.appFrame(0, 0x01, byteArrayOf(0x01, 0x00, 0x00, 0x00))),
        )
        assertEquals(
            byId["OTA-ENTER-A"]!!["request"]!!.jsonPrimitive.content.uppercase(),
            bytesToHex(HKTFrameEncoder.appFrame(0, 0x01, ByteArray(0))),
        )
    }

    @Test
    fun testStartFrameMatchesFixture() {
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val start = vectors.map { it.jsonObject }.first { it["id"]!!.jsonPrimitive.content == "OTA-START-001" }
        assertEquals(
            start["request"]!!.jsonPrimitive.content.uppercase(),
            bytesToHex(OTATransferPlanner.startFrame(300)),
            "start frame: cmd=1 + size(4 BE) + bootload suffix",
        )
        val ack = OtaAck.parse(hexToBytes(start["expectedAck"]!!.jsonPrimitive.content))
        assertEquals(OtaAck.Kind.REQUEST_PACKET, ack?.kind)
        assertEquals(0, ack?.requestedPacket)
    }

    @Test
    fun testFirstRequestIsPacketZero() {
        // OTA-FIRST-REQUEST-001：首请求包号 0（0x02 是 ACK 的命令码，不是包号——照抄文档会死锁）
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val first = vectors.map { it.jsonObject }.first { it["id"]!!.jsonPrimitive.content == "OTA-FIRST-REQUEST-001" }
        val ack = OtaAck.parse(hexToBytes(first["response"]!!.jsonPrimitive.content))
        assertEquals(OtaAck.Kind.REQUEST_PACKET, ack?.kind)
        assertEquals(0, ack?.requestedPacket)
    }

    @Test
    fun testDataFrameMatchesFixture() {
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val byId = vectors.associate { it.jsonObject["id"]!!.jsonPrimitive.content to it.jsonObject }
        val dataReq = hexToBytes(byId["OTA-DATA-001"]!!["request"]!!.jsonPrimitive.content.uppercase())
        // 帧布局 hkt(3)+len(2)+cmd(1)+packNum(2)+payload → payload 从下标 8 起
        val chunk128 = dataReq.copyOfRange(8, 8 + 128)
        assertEquals(
            bytesToHex(dataReq),
            bytesToHex(OTATransferPlanner.dataFrame(0, chunk128)),
            "regular packet: cmd=0x02, 128B chunk, len=0x0083 (146B frame)",
        )
    }

    @Test
    fun testFinalFlushFrameMatchesFixture() {
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val byId = vectors.associate { it.jsonObject["id"]!!.jsonPrimitive.content to it.jsonObject }
        val finalReq = hexToBytes(byId["OTA-FINAL-001"]!!["request"]!!.jsonPrimitive.content)
        // 300B 镜像末包 44B → 补 FF 到 48B；从夹具 payload（下标 8 起）反提 44B 原文再编码
        val paddedPayload = finalReq.copyOfRange(8, 8 + 48)
        val chunk44 = paddedPayload.copyOfRange(0, 44)
        assertEquals(
            bytesToHex(finalReq),
            bytesToHex(OTATransferPlanner.dataFrame(2, chunk44, forceFlush = true)),
            "final packet MUST use cmd=0xFF to force flash flush (audit ❌-3)",
        )
        // planner 分包：300B → 128+128+44，末包 isFinal
        val image = ByteArray(128) { 0x11 } + ByteArray(128) { 0x22 } + chunk44
        val packets = OTATransferPlanner.packets(image)
        assertEquals(3, packets.size)
        assertEquals(44, packets[2].chunk.size)
        assertTrue(packets[2].isFinal)
    }

    @Test
    fun testFinishFrameShape() {
        // `hkt 0001 03 crc bootload`（16B）——唯一跳转触发（审计 ❌-4）；CRC 覆盖单命令字节
        val frame = OTATransferPlanner.finishFrame()
        assertEquals(16, frame.size)
        assertEquals("686B74", bytesToHex(frame.copyOfRange(0, 3)))
        assertEquals("0001", bytesToHex(frame.copyOfRange(3, 5)))
        assertEquals("03", bytesToHex(frame.copyOfRange(5, 6)))
        assertEquals("626F6F746C6F6164", bytesToHex(frame.copyOfRange(8, 16)))
        val expectedCrc = CRC16.ccitt(byteArrayOf(0x03))
        assertEquals("%02X%02X".format(expectedCrc shr 8, expectedCrc and 0xFF), bytesToHex(frame.copyOfRange(6, 8)))
    }

    @Test
    fun testAckParseRejectsMalformed() {
        assertNull(OtaAck.parse(ByteArray(13)))
        assertNull(OtaAck.parse(hexToBytes("686B74050000626F6F746C6F6164")))  // 未知命令码
        assertNull(OtaAck.parse(hexToBytes("686B74020000626F6F746C6F6165"))) // 后缀不匹配
    }

    @Test
    fun testPaddedBoundaries() {
        assertEquals(8, OTATransferPlanner.padded(ByteArray(8)).size)
        assertEquals(48, OTATransferPlanner.padded(ByteArray(44)).size)
        assertEquals(0xFF.toByte(), OTATransferPlanner.padded(ByteArray(44))[47])
        assertEquals(128, OTATransferPlanner.padded(ByteArray(128)).size)  // 对齐不补
    }
}
