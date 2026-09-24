package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.ota.Fixtures.bytesToHex
import com.hkt.ble.bletools.core.ota.Fixtures.hexToBytes
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.test.advanceTimeBy
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.runTest
import kotlinx.serialization.json.jsonObject
import kotlinx.serialization.json.jsonPrimitive

/// OTAEngine 行为测试：三阶段契约（进入双帧→启动节拍→逐包停等→finish）。
/// 虚拟时钟驱动 delay；时序默认值与引擎一致（400/900/500ms，超时 12s）。
/// iOS 对照：CoreOTATests/OTAEngineTests（7 用例）。
@OptIn(ExperimentalCoroutinesApi::class)
class OtaEngineTests {
    private class Harness(val scope: TestScope, image: ByteArray, ackTimeoutMs: Long = 12_000) {
        val sent = mutableListOf<ByteArray>()
        val engine = OtaEngine(
            image = image,
            ackTimeoutMs = ackTimeoutMs,
            scope = scope,
        )

        init {
            engine.bind { sent.add(it) }
        }

        fun hexSent() = sent.map { bytesToHex(it) }
    }

    // 300B 镜像 = DATA-001 的 128B 包 ×2 + FINAL-001 反提的 44B 末包（与夹具逐字节对齐）
    private fun syntheticImage(): ByteArray {
        val vectors = Fixtures.jsonArray("ota-transfer.json")
        val byId = vectors.associate { it.jsonObject["id"]!!.jsonPrimitive.content to it.jsonObject }
        val dataReq = hexToBytes(byId["OTA-DATA-001"]!!["request"]!!.jsonPrimitive.content)
        val chunk128 = dataReq.copyOfRange(8, 8 + 128)
        val finalReq = hexToBytes(byId["OTA-FINAL-001"]!!["request"]!!.jsonPrimitive.content)
        val chunk44 = finalReq.copyOfRange(8, 56).copyOfRange(0, 44)
        return chunk128 + chunk128 + chunk44
    }

    private fun ackRequest(cmd: Int, packet: Int) =
        hexToBytes("686B74%02X%04X626F6F746C6F6164".format(cmd, packet))

    @Test
    fun testEnterFramesThenStartBeat() = runTest {
        val h = Harness(this, syntheticImage())
        h.engine.begin()
        runCurrent()
        // 入口 B 立即发（len=0005 payload[0]=1）
        assertEquals(1, h.sent.size)
        assertEquals(
            Fixtures.jsonArray("ota-transfer.json").map { it.jsonObject }
                .first { it["id"]!!.jsonPrimitive.content == "OTA-ENTER-B" }["request"]!!.jsonPrimitive.content.uppercase(),
            bytesToHex(h.sent[0]),
        )
        // 入口 A（400ms 后，len=0001 纯命令）错开写 flash + 复位时间
        advanceTimeBy(400)
        runCurrent()
        assertEquals(2, h.sent.size)
        assertEquals(
            Fixtures.jsonArray("ota-transfer.json").map { it.jsonObject }
                .first { it["id"]!!.jsonPrimitive.content == "OTA-ENTER-A" }["request"]!!.jsonPrimitive.content.uppercase(),
            bytesToHex(h.sent[1]),
        )
        // 900ms 起每 500ms 重发启动帧（bootloader 就绪前的节拍）
        advanceTimeBy(500)
        runCurrent()
        assertTrue(bytesToHex(h.sent[2]).startsWith("686B74000501"), "3rd frame must be start frame, got ${bytesToHex(h.sent[2])}")
        val startCount = h.sent.size
        advanceTimeBy(500)
        runCurrent()
        assertEquals(startCount + 1, h.sent.size, "start frame re-sent every 500ms until ACK(2,0)")
        assertEquals(OtaEngine.State.EnteringBootloader, h.engine.state.value)
    }

    @Test
    fun testFullTransferLoopCompletes() = runTest {
        val image = syntheticImage()
        val h = Harness(this, image)
        h.engine.begin()
        advanceTimeBy(900)
        h.engine.handle(ackRequest(0x02, 0))
        runCurrent()
        assertEquals(OtaEngine.State.Transferring, h.engine.state.value)
        assertEquals(0, h.engine.packetsDone.value)
        // ACK(2,n) 停等驱动：请求 n 发 n；末包 cmd=0xFF 强制落盘
        h.engine.handle(ackRequest(0x02, 1))
        h.engine.handle(ackRequest(0x02, 2))
        runCurrent()
        assertEquals(2, h.engine.packetsDone.value)
        val last = bytesToHex(h.sent.last())
        // 引导层帧布局 hkt(3B)+len(2B)+cmd(1B)：cmd=0xFF 在 hex 下标 10-11
        assertTrue(last.substring(10, 12) == "FF", "final packet must carry cmd=0xFF, got $last")
        // 完成 ACK(3,0) → 发 finish 帧（唯一跳转触发）→ done
        h.engine.handle(ackRequest(0x03, 0))
        runCurrent()
        assertEquals(OtaEngine.State.Done, h.engine.state.value)
        assertEquals("686B740001", bytesToHex(h.sent.last()).substring(0, 10))
        // progress 语义与 iOS 一致：跟踪最近被请求的包号（2/3），100% 由 state=Done 表达
        assertEquals(2, h.engine.packetsDone.value)
        assertEquals(2.0 / 3.0, h.engine.progress, 1e-9)
        // done 态继续吞 finish 引发的二次 ACK(3,0)
        assertTrue(h.engine.handle(ackRequest(0x03, 0)))
        assertEquals(OtaEngine.State.Done, h.engine.state.value)
    }

    @Test
    fun testDeviceResetRestartLimit() = runTest {
        val h = Harness(this, syntheticImage())
        h.engine.begin()
        advanceTimeBy(900)
        h.engine.handle(ackRequest(0x02, 0))
        // 静默 10s 复位：ACK(1,0) → 从包 0 重来，重发启动帧
        h.engine.handle(ackRequest(0x01, 0))
        runCurrent()
        assertEquals(1, h.engine.restarts.value)
        assertEquals(OtaEngine.State.EnteringBootloader, h.engine.state.value)
        assertEquals(0, h.engine.packetsDone.value)
        val before = h.sent.size
        advanceTimeBy(500)
        runCurrent()
        assertTrue(h.sent.size > before, "start frame re-sent after restart")
        // 超限（第 3 次复位）→ 失败
        h.engine.handle(ackRequest(0x02, 0))
        h.engine.handle(ackRequest(0x01, 0))
        h.engine.handle(ackRequest(0x02, 0))
        h.engine.handle(ackRequest(0x01, 0))
        assertEquals(OtaEngine.State.Failed(OtaEngine.EngineError.TOO_MANY_RESTARTS), h.engine.state.value)
    }

    @Test
    fun testAckTimeoutFails() = runTest {
        val h = Harness(this, syntheticImage(), ackTimeoutMs = 5_000)
        h.engine.begin()
        advanceTimeBy(5_000)
        runCurrent()
        assertEquals(OtaEngine.State.Failed(OtaEngine.EngineError.TIMEOUT), h.engine.state.value)
    }

    @Test
    fun testCancel() = runTest {
        val h = Harness(this, syntheticImage())
        h.engine.begin()
        h.engine.cancel()
        assertEquals(OtaEngine.State.Failed(OtaEngine.EngineError.CANCELLED), h.engine.state.value)
        // 终态后 handle 不再消费
        assertFalse(h.engine.handle(ackRequest(0x02, 0)))
    }

    @Test
    fun testNonBootloaderFrameRejected() = runTest {
        val h = Harness(this, syntheticImage())
        h.engine.begin()
        // app 层帧（如 0xFF 查询应答）不是引导层 ACK → 返回 false 不消费
        val appResponse = HKTFrameEncoder.appFrame(0, CommandCode.QUERY, HKTFrameEncoder.fillerPayload())
        assertFalse(h.engine.handle(appResponse))
    }

    @Test
    fun testIdleIgnoresFrames() = runTest {
        val h = Harness(this, syntheticImage())
        assertFalse(h.engine.handle(ackRequest(0x02, 0)))
        assertEquals(OtaEngine.State.Idle, h.engine.state.value)
    }
}
