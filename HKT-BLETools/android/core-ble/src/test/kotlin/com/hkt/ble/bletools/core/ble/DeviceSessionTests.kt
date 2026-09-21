package com.hkt.ble.bletools.core.ble

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.advanceTimeBy
import kotlinx.coroutines.test.runCurrent
import kotlinx.coroutines.test.runTest

/// 设备会话轮询（R-5/R-7/R-8）——MockLink 夹具应答，无真机全流程测试。
/// 虚拟时钟驱动：nowMs/epochSeconds 接 testScheduler.currentTime。
/// iOS 对照：CoreBLETests/DeviceSessionTests（15 用例，本文件为等价移植）。
@OptIn(ExperimentalCoroutinesApi::class)
class DeviceSessionTests {
    private val udsResponse =
        "686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708"
    private val dc200Response =
        "686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064"

    private fun fixture(hex: String) = Fixtures.hexToBytes(hex)

    private fun TestScope.session(
        link: PeripheralLink,
        family: DeviceFamily = DeviceFamily.UDS100,
        pollIntervalMs: Long = 50,
        staleAfter: Int = 4,
    ): DeviceSession = DeviceSession(
        family = family,
        deviceName = "TEST",
        link = link,
        pollIntervalMs = pollIntervalMs,
        scope = this,
        nowMs = { testScheduler.currentTime },
        epochSeconds = { 1_700_000_000 },
    ).apply { this.staleAfter = staleAfter }

    @Test
    fun testPollReceivesAndDecodesSnapshot() = runTest {
        val link = MockLink()
        link.responder = { _ -> fixture(udsResponse) }
        val session = session(link)
        session.start()
        advanceTimeBy(400)
        runCurrent()
        assertTrue(session.pollsSent.value >= 2)
        assertEquals(25000, session.snapshot.value.temperatureMilli)
        assertEquals(12000, session.snapshot.value.humidityMilli)
        assertEquals(4000, session.snapshot.value.batteryVoltageMV)
        assertEquals(1080, session.snapshot.value.distanceMM)
        assertFalse(session.isStale)
        assertFalse(session.unknownTail.value)
        session.stop()
    }

    @Test
    fun testQueryFrameShape() = runTest {
        val link = MockLink()
        link.responder = { _ -> null }
        val session = session(link)
        session.start()
        advanceTimeBy(250)
        runCurrent()
        assertTrue(link.framesSent >= 2)
        // TX-QUERY-001 同构：hkt + packNum + len(0005) + FF + FFFFFFFF + CRC = 13B
        val frame = link.sentFrameAt(0)
        assertEquals(0x68.toByte(), frame[0])
        assertEquals(0x6B.toByte(), frame[1])
        assertEquals(0x74.toByte(), frame[2])
        assertEquals(13, frame.size)
        session.stop()
    }

    @Test
    fun testUnknownTailMarksAbnormal() = runTest {
        val link = MockLink()
        val abnormal = udsResponse + "77AA"   // 尾部追加未知类型 0x77
        link.responder = { _ -> fixture(abnormal) }
        val session = session(link)
        session.start()
        advanceTimeBy(300)
        runCurrent()
        assertTrue(session.unknownTail.value)                          // R-8：标记而非崩溃/跳过
        assertEquals(25000, session.snapshot.value.temperatureMilli)   // 已解析前缀保留
        session.stop()
    }

    @Test
    fun testStalenessWithoutResponse() = runTest {
        val link = MockLink()
        link.responder = { _ -> null }   // 停摆：不响应
        val session = session(link, staleAfter = 1)
        session.start()
        advanceTimeBy(500)
        runCurrent()
        assertTrue(session.isStale)              // R-7：超窗判定
        assertTrue(session.pollsSent.value >= 3)
        assertNull(session.snapshot.value.temperatureMilli)
        session.stop()
    }

    @Test
    fun testDisconnectMarksLinkLost() = runTest {
        val link = MockLink()
        link.responder = { _ -> fixture(udsResponse) }
        val session = session(link)
        session.start()
        advanceTimeBy(200)
        runCurrent()
        link.responder = { _ -> null }   // 断开后设备不再响应（真实语义）
        link.simulateDisconnect()
        advanceTimeBy(300)
        runCurrent()
        assertTrue(session.linkLost.value)   // R-31 语义：链路丢失标记保持
        assertTrue(session.isStale)          // R-7：无响应超窗
        session.stop()
    }

    @Test
    fun testCommandSendUsesEncoder() {
        val link = MockLink()
        val session = DeviceSession(
            DeviceFamily.SVC100, "SVC100 B4D2", link,
            pollIntervalMs = 50, scope = TestScope(),
        )
        // 0x06 对时：packNum 1、stamp 由 App 层传入（SP-25：App 不做时区换算）
        session.send(cmd = CommandCode.TIME_SYNC, data = byteArrayOf(0x00, 0x00, 0x00, 0x01))
        val frame = link.sentFrameAt(0)
        assertEquals(0x68.toByte(), frame[0])
        assertEquals(0x00, frame[4])                 // len 高字节
        assertEquals(0x05, frame[5])                 // len 低字节 = cmd(1) + data(4)
        assertEquals(CommandCode.TIME_SYNC.toByte(), frame[6])
    }

    // MARK: 写入确认（sendWrite：专用 ACK 帧 = hkt 00 seq FF 值）

    @Test
    fun testSendWriteResolvesOnSyncAckFrame() = runTest {
        // MockLink 同步应答：ACK 在 send 内部到达，先注册等待再发送的顺序不能丢 ACK
        val link = MockLink()
        val ack = fixture("686B740000FF00")
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.CONFIG.toByte()) ack else null   // 轮询帧不回
        }
        val session = session(link, pollIntervalMs = 3_600_000)
        session.start()
        val acked = session.sendWrite(
            cmd = CommandCode.CONFIG,
            data = HKTFrameEncoder.udsConfigPayload(20, 60, 400, 3000),
            timeoutMs = 500,
        )
        assertTrue(acked)
        session.stop()
    }

    @Test
    fun testSendWriteTimesOutWithoutAck() = runTest {
        val link = MockLink()
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.QUERY.toByte()) fixture(udsResponse) else null
        }
        val session = session(link, pollIntervalMs = 3_600_000)
        session.start()
        var acked: Boolean? = null
        val job = launch { acked = session.sendWrite(cmd = CommandCode.CONFIG, data = HKTFrameEncoder.udsConfigPayload(20, 60, 400, 3000), timeoutMs = 300) }
        runCurrent()
        advanceTimeBy(300)
        runCurrent()
        job.join()
        assertEquals(false, acked)
        session.stop()
    }

    @Test
    fun testSendWriteIgnoresPollResponseOnly() = runTest {
        // 轮询回应不含 0xFF 段（固件事实），不得误判为写入确认
        val link = MockLink()
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.QUERY.toByte()) fixture(udsResponse) else null
        }
        val session = session(link)
        session.start()
        var acked: Boolean? = null
        val job = launch { acked = session.sendWrite(cmd = CommandCode.CONFIG, data = HKTFrameEncoder.udsConfigPayload(20, 60, 400, 3000), timeoutMs = 250) }
        runCurrent()
        advanceTimeBy(250)
        runCurrent()
        job.join()
        assertEquals(false, acked)   // 只有轮询回应流动，无专用 ACK → 超时
        session.stop()
    }

    @Test
    fun testLateBootloaderAckAfterOTADoesNotMarkUnknownTail() = runTest {
        // 审计后续（2026-09-11 真机 OTA）：finish 帧引发的迟到 ACK(3,0) 在引擎收尾后到达，
        // 属引导层帧——静默丢弃，不得置 unknownTail 让详情页误报「响应数据异常」
        val link = MockLink()
        link.responder = { _ -> null }
        val session = session(link, family = DeviceFamily.SVC100, pollIntervalMs = 3_600_000)
        session.start()
        link.onReceive?.invoke(fixture("686B74030000626F6F746C6F6164"))
        runCurrent()
        assertFalse(session.unknownTail.value)
        var consumed = false
        session.rawFrameHandler = { consumed = true; true }   // 引擎在位时照常转发
        link.onReceive?.invoke(fixture("686B74020000626F6F746C6F6164"))
        runCurrent()
        assertTrue(consumed)
        session.stop()
    }

    // MARK: 对时分家族（审计 ❌-1/❌-2：SVC=hkt 帧有 ACK / UDS=ASCII 无回执 / DC=不支持）

    @Test
    fun testTimeSyncSVCUsesHktFrameAndWaitsAck() = runTest {
        val link = MockLink()
        val ack = fixture("686B740000FFFF")
        link.responder = { frame ->
            // 对时帧特征：len=0004、cmd=06（hkt 帧，非 bootloader 后缀）
            if (frame.size >= 7 && frame[4] == 0x00.toByte() && frame[5] == 0x04.toByte() && frame[6] == 0x06.toByte()) ack else null
        }
        val session = session(link, family = DeviceFamily.SVC100, pollIntervalMs = 3_600_000)
        session.start()
        val outcome = session.sendTimeSync()
        assertEquals(DeviceSession.TimeSyncOutcome.ACKNOWLEDGED, outcome)
        val frame = link.sentFrameAt(0)
        // 无双重封装：hkt + packNum(1) + len(0004) + cmd(06)
        val expectedPrefix = byteArrayOf(0x68, 0x6B, 0x74, 0x01, 0x00, 0x04, 0x06)
        for (i in expectedPrefix.indices) assertEquals(expectedPrefix[i], frame[i], "prefix[$i]")
        session.stop()
    }

    @Test
    fun testTimeSyncUDSSendsAsciiCommand() = runTest {
        // UDS：hkt 对时分支固件不可达（死分支），改发 ASCII syncDeviceTimestamp
        val link = MockLink()
        link.responder = { _ -> null }
        val session = session(link, pollIntervalMs = 3_600_000)
        session.start()
        val outcome = session.sendTimeSync()
        assertEquals(DeviceSession.TimeSyncOutcome.SENT, outcome)
        val sent = link.sentFrameAt(0)
        assertEquals("syncDeviceTimestamp:", String(sent, 0, 20, Charsets.US_ASCII))
        session.stop()
    }

    @Test
    fun testTimeSyncDCUnsupported() = runTest {
        // DC：ASCII 对时含 tm_mon+1 固件缺陷，暂不支持
        val link = MockLink()
        link.responder = { _ -> null }
        val session = session(link, family = DeviceFamily.DC200_FAMILY, pollIntervalMs = 3_600_000)
        session.start()
        val outcome = session.sendTimeSync()
        assertEquals(DeviceSession.TimeSyncOutcome.UNSUPPORTED, outcome)
        assertEquals(0, link.framesSent)   // 不发任何帧
        session.stop()
    }

    // MARK: 校准（0xFD：立即 ACK → 等纯 ASCII "Calibration Done" 上报）

    @Test
    fun testCalibrationCompletesOnTextReport() = runTest {
        val link = MockLink()
        val ack = fixture("686B740000FFFF")
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.CALIBRATE.toByte()) ack else null   // 即发 ACK
        }
        val session = session(link, pollIntervalMs = 3_600_000)
        session.start()
        var outcome: DeviceSession.CalibrationOutcome? = null
        val job = launch { outcome = session.startCalibration(reportTimeoutMs = 2_000) }
        runCurrent()   // 0xFD 已发、同步 ACK 已回、完成上报等待已注册
        link.onReceive?.invoke("Calibration Done".toByteArray(Charsets.US_ASCII))
        runCurrent()
        job.join()
        assertEquals(DeviceSession.CalibrationOutcome.DONE, outcome)
        session.stop()
    }

    @Test
    fun testCalibrationTimesOutWithoutReport() = runTest {
        val link = MockLink()
        val ack = fixture("686B740000FFFF")
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.CALIBRATE.toByte()) ack else null   // ACK 后不再上报
        }
        val session = session(link, family = DeviceFamily.DC200_FAMILY, pollIntervalMs = 3_600_000)
        session.start()
        var outcome: DeviceSession.CalibrationOutcome? = null
        val job = launch { outcome = session.startCalibration(reportTimeoutMs = 300) }
        runCurrent()
        advanceTimeBy(300)
        runCurrent()
        job.join()
        assertEquals(DeviceSession.CalibrationOutcome.TIMEOUT, outcome)
        session.stop()
    }

    @Test
    fun testCalibrationSuspendsPollingDuringWindow() = runTest {
        // FD-004 修法：校准全程停轮询（防 0xFD 与 1Hz 轮询在设备单 RX 缓冲撞车）
        val link = MockLink()
        val ack = fixture("686B740000FFFF")
        var sawCalibrate = false
        link.responder = { frame ->
            if (frame.size > 6 && frame[6] == CommandCode.CALIBRATE.toByte()) {
                sawCalibrate = true
                ack
            } else {
                null
            }
        }
        val session = session(link, pollIntervalMs = 50)
        session.start()
        advanceTimeBy(100)
        runCurrent()
        val pollsBefore = session.pollsSent.value
        assertTrue(pollsBefore >= 1)
        var outcome: DeviceSession.CalibrationOutcome? = null
        val job = launch { outcome = session.startCalibration(reportTimeoutMs = 300) }
        advanceTimeBy(300)
        runCurrent()
        job.join()
        assertEquals(DeviceSession.CalibrationOutcome.TIMEOUT, outcome)
        assertEquals(pollsBefore, session.pollsSent.value, "轮询必须在校准窗口内暂停")
        // 结束后恢复
        advanceTimeBy(100)
        runCurrent()
        assertTrue(session.pollsSent.value > pollsBefore)
        session.stop()
    }
}
