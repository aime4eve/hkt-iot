package com.hkt.ble.bletools.demo

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.DeviceSnapshotDecoder
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import com.hkt.ble.bletools.core.protocol.HKTResponseParser
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/**
 * 演示应答器单测：0xFF 夹具回帧 / 0x02 配置回写（偏移逐位验证）/ 0xFE 电源位 / 0x03 阀状态 / ACK 契约。
 * iOS 对照：App/DemoResponder.swift（device 侧无专测，此处为 Kotlin 移植补齐的覆盖）。
 */
class DemoResponderTest {
    private val responder = DemoResponder()

    private fun queryFrame(packNum: Int) = HKTFrameEncoder.appFrame(packNum, CommandCode.QUERY, HKTFrameEncoder.fillerPayload())

    @Test
    fun testQueryReturnsFamilyFixtureWithPowerBit() {
        val resp = responder.respond(queryFrame(1), DeviceFamily.SVC100)
        val parsed = HKTResponseParser.parse(resp!!, DeviceFamily.SVC100)
        val snapshot = DeviceSnapshotDecoder.snapshotFrom(parsed.entries, DeviceFamily.SVC100)
        assertEquals(1, snapshot.power)
        assertEquals(99, snapshot.batteryPercent)
        assertEquals(25, snapshot.timezone)
    }

    @Test
    fun testQueryWithoutFamilySilent() {
        assertNull(responder.respond(queryFrame(1), null))
    }

    @Test
    fun testConfigWritePatchesFixtureAndAcks() {
        val config = HKTFrameEncoder.appFrame(
            1, CommandCode.CONFIG,
            HKTFrameEncoder.udsConfigPayload(reportMin = 20, gpsMin = 60, lowMM = 400, highMM = 3000),
        )
        assertEquals(DemoResponder.ACK_FRAME.toList(), responder.respond(config, DeviceFamily.UDS100)!!.toList())

        // 下轮轮询回新值（配置保存→详情页联动演示闭环）
        val resp = responder.respond(queryFrame(2), DeviceFamily.UDS100)
        val snapshot = DeviceSnapshotDecoder.snapshotFrom(
            HKTResponseParser.parse(resp!!, DeviceFamily.UDS100).entries,
            DeviceFamily.UDS100,
        )
        assertEquals(20, snapshot.reportPeriodMin)
        assertEquals(60, snapshot.gpsPeriodMin)
        assertEquals(400, snapshot.lowThresholdMM)
        assertEquals(3000, snapshot.highThresholdMM)
    }

    @Test
    fun testConfigSilentRejectKnob() {
        responder.configAcks = false
        val config = HKTFrameEncoder.appFrame(
            1, CommandCode.CONFIG,
            HKTFrameEncoder.dcConfigPayload(reportMin = 30, mode = 1),
        )
        assertNull(responder.respond(config, DeviceFamily.DC200_FAMILY))
    }

    @Test
    fun testPowerOffFlipsFixtureByteAndAcks() {
        // F-1 真载荷：powerPayload 电源字节在帧偏移 10；有效载荷必回 ACK（真机 2026-09-20 实证）
        val off = HKTFrameEncoder.appFrame(1, CommandCode.POWER, HKTFrameEncoder.powerPayload(false))
        assertEquals(DemoResponder.ACK_FRAME.toList(), responder.respond(off, DeviceFamily.UDS100)!!.toList())
        val resp = responder.respond(queryFrame(2), DeviceFamily.UDS100)
        val snapshot = DeviceSnapshotDecoder.snapshotFrom(
            HKTResponseParser.parse(resp!!, DeviceFamily.UDS100).entries,
            DeviceFamily.UDS100,
        )
        assertEquals(0, snapshot.power)
    }

    @Test
    fun testRealtimeTaskWritesValveStateAndBusyIgnores() {
        // 阀 2 关（夹具原值 v2s=1 → 0）
        val close = HKTFrameEncoder.appFrame(
            1, CommandCode.SVC_REALTIME_TASK,
            HKTFrameEncoder.svcRealtimeTaskPayload(valve = 2, state = 0, durationS = 60, pulse = 200),
        )
        assertEquals(DemoResponder.ACK_FRAME.toList(), responder.respond(close, DeviceFamily.SVC100)!!.toList())
        var resp = responder.respond(queryFrame(2), DeviceFamily.SVC100)
        var snapshot = DeviceSnapshotDecoder.snapshotFrom(
            HKTResponseParser.parse(resp!!, DeviceFamily.SVC100).entries,
            DeviceFamily.SVC100,
        )
        assertEquals(0, snapshot.valve2State)
        assertEquals(1, snapshot.valve1State)   // 阀 1 不受影响

        // busy 旋钮：0x03 静默忽略（无 ACK）且不写夹具
        responder.realtimeBusy = true
        val open = HKTFrameEncoder.appFrame(
            2, CommandCode.SVC_REALTIME_TASK,
            HKTFrameEncoder.svcRealtimeTaskPayload(valve = 2, state = 1, durationS = 60, pulse = 200),
        )
        assertNull(responder.respond(open, DeviceFamily.SVC100))
        resp = responder.respond(queryFrame(3), DeviceFamily.SVC100)
        snapshot = DeviceSnapshotDecoder.snapshotFrom(
            HKTResponseParser.parse(resp!!, DeviceFamily.SVC100).entries,
            DeviceFamily.SVC100,
        )
        assertEquals(0, snapshot.valve2State)
    }

    @Test
    fun testCalibrateAcks() {
        val cal = HKTFrameEncoder.appFrame(1, CommandCode.CALIBRATE, HKTFrameEncoder.fillerPayload())
        assertEquals(DemoResponder.ACK_FRAME.toList(), responder.respond(cal, DeviceFamily.UDS100)!!.toList())
    }

    @Test
    fun testUnknownAndShortFramesSilent() {
        assertNull(responder.respond(byteArrayOf(0x01, 0x02), DeviceFamily.UDS100))
        assertNull(responder.respond(HKTFrameEncoder.appFrame(1, 0x77, HKTFrameEncoder.fillerPayload()), DeviceFamily.UDS100))
    }
}
