package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.ota.Fixtures.bytesToHex
import com.hkt.ble.bletools.core.ota.Fixtures.hexToBytes
import com.hkt.ble.bletools.core.protocol.CRC16
import com.hkt.ble.bletools.core.protocol.CRC32
import com.hkt.ble.bletools.core.protocol.UpgradeFrame
import com.hkt.ble.bletools.core.protocol.UpgradeInbound
import com.hkt.ble.bletools.core.protocol.UpgradeMsgType
import com.hkt.ble.bletools.core.protocol.UpgradePkgType
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.advanceTimeBy
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.runTest
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// DiffUpgradeEngine 行为测试：设备拉包停等契约（0xF0 通知→0xFE 应答→0xF2 拉包→0xF1 回包→0xF3 结果）。
/// 虚拟时钟驱动看门狗；时序默认值与引擎一致（通知 6s/拉包 15s/重发 2 次）。
class DiffUpgradeEngineTests {

    private class Harness(
        scope: TestScope,
        file: ByteArray = hexToBytes("0102030405060708"),
        pkgType: Int = UpgradePkgType.FULL,
        packetSize: Int = 4,
    ) {
        val sent = mutableListOf<ByteArray>()
        val engine = DiffUpgradeEngine(
            file = file,
            pkgType = pkgType,
            hwVersion = 0x11,
            swVersion = 0x28,
            fileCrc32 = 0x12345678L,
            packetSize = packetSize,
            scope = scope,
        )

        init {
            engine.bind { sent.add(it) }
        }

        fun hexSent() = sent.map { bytesToHex(it) }
    }

    private fun vector(id: String) =
        Fixtures.jsonArray("upgrade-protocol.json").map { it.jsonObject }
            .first { it["id"]!!.jsonPrimitive.content == id }

    private fun respond(id: String) = hexToBytes(vector(id)["response"]!!.jsonPrimitive.content)

    @Test
    fun testFullFlowDevicePullLoop() = runTest {
        val h = Harness(this)
        h.engine.begin()
        runCurrent()
        // 0xF0 通知与文档示例①逐字节一致（同入参：hw 0x11/sw 0x28/2 包×4B/全量/CRC32 0x12345678）
        assertEquals(vector("UPG-NOTIFY-001")["request"]!!.jsonPrimitive.content.uppercase(), h.hexSent()[0])
        assertEquals(DiffUpgradeEngine.State.Notifying, h.engine.state.value)

        // 0xFE(F0) 应答 → Transferring
        assertTrue(h.engine.handle(respond("UPG-ACK-NOTIFY-001")))
        assertEquals(DiffUpgradeEngine.State.Transferring, h.engine.state.value)

        // 设备拉第 0 包 → APP 回 0xF1（包序号/CRC16/长度/数据逐字段核对）
        assertTrue(h.engine.handle(respond("UPG-REQUEST-001")))
        assertEquals(2, h.sent.size)   // sent[0]=0xF0 通知，sent[1]=0xF1 数据帧
        val f1 = h.sent[1]
        assertEquals(UpgradeMsgType.DATA, f1[5].toInt() and 0xFF)
        assertEquals(0, ((f1[6].toInt() and 0xFF) shl 8) or (f1[7].toInt() and 0xFF))
        assertEquals(CRC16.ccitt(hexToBytes("01020304")), ((f1[8].toInt() and 0xFF) shl 8) or (f1[9].toInt() and 0xFF))
        assertEquals(4, ((f1[10].toInt() and 0xFF) shl 8) or (f1[11].toInt() and 0xFF))
        assertEquals(0, h.engine.packetsDone.value)   // 设备刚要第 0 包，尚无已落设备计数

        // 设备拉第 1 包 ⇒ 第 0 包已落设备（进度 = 已确认落设备的包数）
        assertTrue(h.engine.handle(respond("UPG-REQUEST-002")))
        assertEquals(1, h.engine.packetsDone.value)

        // 0xF3 成功 → Done（进度补满；设备随后自行合成/复位安装）
        assertTrue(h.engine.handle(respond("UPG-RESULT-OK")))
        assertEquals(DiffUpgradeEngine.State.Done(0), h.engine.state.value)
        assertEquals(2, h.engine.packetsDone.value)
        assertNull(h.engine.reportedCode)
        // Done 态不再消费迟到帧（由 DeviceSession 分流层静默丢弃）
        assertFalse(h.engine.handle(respond("UPG-RESULT-OK")))
    }

    @Test
    fun testNotifyRetryThenTimeout() = runTest {
        val h = Harness(this)
        h.engine.begin()
        runCurrent()
        assertEquals(1, h.sent.size)
        // 6s 无 0xFE → 重发 0xF0（共 2 次重发），再超时 → Failed(NOTIFY_TIMEOUT)
        advanceTimeBy(6_000); runCurrent()
        assertEquals(2, h.sent.size)
        advanceTimeBy(6_000); runCurrent()
        assertEquals(3, h.sent.size)
        advanceTimeBy(6_000); runCurrent()
        assertEquals(DiffUpgradeEngine.State.Failed(DiffUpgradeEngine.EngineError.NOTIFY_TIMEOUT), h.engine.state.value)
    }

    @Test
    fun testRepeatedSamePacketFails() = runTest {
        val h = Harness(this)
        h.engine.begin(); runCurrent()
        h.engine.handle(respond("UPG-ACK-NOTIFY-001"))
        // 同包首请 + 5 次重拉容忍（镜像设备端"连续 5 包校验失败退出"），第 6 次重拉 → 中止
        repeat(6) { assertTrue(h.engine.handle(respond("UPG-REQUEST-001"))) }
        assertTrue(h.engine.handle(respond("UPG-REQUEST-001")))
        assertEquals(DiffUpgradeEngine.State.Failed(DiffUpgradeEngine.EngineError.REPEATED_PACKET), h.engine.state.value)
    }

    @Test
    fun testDeviceRejectedCarriesResultCode() = runTest {
        val h = Harness(this)
        h.engine.begin(); runCurrent()
        h.engine.handle(respond("UPG-ACK-NOTIFY-001"))
        h.engine.handle(respond("UPG-REQUEST-001"))
        assertTrue(h.engine.handle(respond("UPG-RESULT-CRC-FAIL")))
        assertEquals(DiffUpgradeEngine.State.Failed(DiffUpgradeEngine.EngineError.DEVICE_REJECTED), h.engine.state.value)
        assertEquals(1, h.engine.reportedCode)   // UI 文案映射 0x01 校验失败
    }

    @Test
    fun testPullTimeoutWhileTransferring() = runTest {
        val h = Harness(this)
        h.engine.begin(); runCurrent()
        h.engine.handle(respond("UPG-ACK-NOTIFY-001"))
        h.engine.handle(respond("UPG-REQUEST-001"))
        advanceTimeBy(30_000); runCurrent()   // 设备停止拉包（等待窗口 30s：覆盖设备合成耗时）
        assertEquals(DiffUpgradeEngine.State.Failed(DiffUpgradeEngine.EngineError.PULL_TIMEOUT), h.engine.state.value)
    }

    @Test
    fun testCancelFromNotifying() = runTest {
        val h = Harness(this)
        h.engine.begin(); runCurrent()
        h.engine.cancel()
        assertEquals(DiffUpgradeEngine.State.Failed(DiffUpgradeEngine.EngineError.CANCELLED), h.engine.state.value)
    }

    @Test
    fun testDiffPkgTypeFlaggedInNotify() = runTest {
        val h = Harness(this, pkgType = UpgradePkgType.DIFF)
        h.engine.begin(); runCurrent()
        // 0xF0 第 7 数据字节（帧内偏移 12）= 包类型 0x01 差分
        assertEquals(UpgradePkgType.DIFF, h.sent[0][12].toInt() and 0xFF)
        // 结构自证：整帧可再算校验
        val f = h.sent[0]
        assertEquals(UpgradeFrame.checksum(f, f.size - 1), f[f.size - 1].toInt() and 0xFF)
    }

    @Test
    fun testCrc32HelperMatchesKnownVector() {
        // CRC32/IEEE 已知向量（"123456789" → 0xCBF43926），锁定与 iOS/旧 OtaScreen 同源实现
        assertEquals(0xCBF43926L, CRC32.ieee("123456789".toByteArray()))
    }

    @Test
    fun testInboundParseRejectsCorruption() {
        val frame = respond("UPG-ACK-NOTIFY-001")
        assertNull(UpgradeInbound.parse(frame.copyOf(frame.size).also { it[it.size - 1] = 0x00.toByte() }))
    }
}
