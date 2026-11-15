package com.hkt.ble.bletools.core.protocol

import com.hkt.ble.bletools.core.protocol.Fixtures.bytesToHex
import com.hkt.ble.bletools.core.protocol.Fixtures.hexToBytes
import com.hkt.ble.bletools.core.protocol.Fixtures.jsonArray
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// 新升级协议帧编解码金向量测试（真源 = 仓库级 shared/fixtures/upgrade-protocol.json，
/// 向量取自《蓝牙升级协议.docx》完整升级示例，校验和逐字节手工核对）。
class UpgradeFrameTests {

    private fun vector(id: String) =
        jsonArray("upgrade-protocol.json").map { it.jsonObject }
            .first { it["id"]!!.jsonPrimitive.content == id }

    private fun request(id: String) = hexToBytes(vector(id)["request"]!!.jsonPrimitive.content)
    private fun response(id: String) = hexToBytes(vector(id)["response"]!!.jsonPrimitive.content)

    @Test
    fun testNotifyFrameMatchesDocVector() {
        // 文档示例①：hw 0x11 sw 0x28 总包数 2 每包 4B 全量 CRC32 0x12345678 → seq=1 时逐字节一致
        val frame = UpgradeFrame.notifyFrame(
            seq = 1, hwVersion = 0x11, swVersion = 0x28,
            totalPackets = 2, packetLen = 4, pkgType = UpgradePkgType.FULL, fileCrc32 = 0x12345678L,
        )
        assertEquals(bytesToHex(request("UPG-NOTIFY-001")), bytesToHex(frame))
    }

    @Test
    fun testParseAckForNotify() {
        assertEquals(UpgradeInbound.GenericAck(0xF0), UpgradeInbound.parse(response("UPG-ACK-NOTIFY-001")))
    }

    @Test
    fun testParsePacketRequests() {
        assertEquals(UpgradeInbound.PacketRequest(0), UpgradeInbound.parse(response("UPG-REQUEST-001")))
        assertEquals(UpgradeInbound.PacketRequest(1), UpgradeInbound.parse(response("UPG-REQUEST-002")))
    }

    @Test
    fun testParseResultReports() {
        assertEquals(UpgradeInbound.ResultReport(0, 0), UpgradeInbound.parse(response("UPG-RESULT-OK")))
        assertEquals(UpgradeInbound.ResultReport(1, 0), UpgradeInbound.parse(response("UPG-RESULT-CRC-FAIL")))
    }

    @Test
    fun testDataFrameStructureAndChecksum() {
        // 文档示例④⑤的包 CRC 为占位值（0x1234/0x5678），不做逐字节比对；
        // 结构性断言：帧长 = 8+6+N、包序号/包长字段、CRC16=KERMIT(chunk)、尾校验有效
        val chunk = hexToBytes("01020304")
        val frame = UpgradeFrame.dataFrame(seq = 4, packetIndex = 0, chunk = chunk)
        assertEquals(8 + 6 + 4, frame.size)
        assertEquals("F1", "%02X".format(frame[5]))
        assertEquals(0, ((frame[6].toInt() and 0xFF) shl 8) or (frame[7].toInt() and 0xFF))          // 包序号 0
        assertEquals(CRC16.ccitt(chunk), ((frame[8].toInt() and 0xFF) shl 8) or (frame[9].toInt() and 0xFF)) // 包 CRC16
        assertEquals(4, ((frame[10].toInt() and 0xFF) shl 8) or (frame[11].toInt() and 0xFF))        // 包长
        assertTrue(chunk.contentEquals(frame.copyOfRange(12, 16)))
        assertEquals(
            UpgradeFrame.checksum(frame, frame.size - 1),
            frame[frame.size - 1].toInt() and 0xFF,
        )
    }

    @Test
    fun testRejectsCorruptedChecksum() {
        val frame = response("UPG-REQUEST-001").copyOf()
        frame[frame.size - 1] = (frame[frame.size - 1] + 1).toByte()
        assertNull(UpgradeInbound.parse(frame))
    }

    @Test
    fun testRejectsLegacyFrames() {
        // 旧 app/TLV 帧不得被识别为升级帧（前缀同为 hkt，但类型位/定长/尾校验三重判据互斥）
        assertNull(UpgradeInbound.parse(hexToBytes("686B740005010000012CF9F2626F6F746C6F6164"))) // 引导层启动帧（OTA-START-001）
        assertNull(UpgradeInbound.parse(hexToBytes("686B740005FF00000001")))                        // 旧 TLV 状态流
        assertNull(UpgradeInbound.parse("syncDeviceTimestamp:1770000000".toByteArray()))            // ASCII 对时
    }
}
