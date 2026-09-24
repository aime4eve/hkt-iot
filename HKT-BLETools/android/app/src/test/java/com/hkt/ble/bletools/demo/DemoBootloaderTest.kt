package com.hkt.ble.bletools.demo

import com.hkt.ble.bletools.core.ota.OtaAck
import com.hkt.ble.bletools.core.ota.OTATransferPlanner
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * DemoBootloader（演示模式 bootloader 仿真）ACK 契约测试：
 * 启动帧→ACK(2,0)、数据帧→ACK(2,n+1)、0xFF 强制落盘→ACK(3,0)、finish→静默；
 * 应用层帧（无 bootload 尾）不受影响。
 */
class DemoBootloaderTest {

    private val bootloader = DemoBootloader()

    @Test
    fun `start frame requests packet 0`() {
        val start = OTATransferPlanner.startFrame(16384)
        val ack = bootloader.ackFor(start)
        assertEquals(OtaAck.Kind.REQUEST_PACKET, OtaAck.parse(ack!!)?.kind)
        assertEquals(0, OtaAck.parse(ack)?.requestedPacket)
    }

    @Test
    fun `data frame requests next packet`() {
        val frame = OTATransferPlanner.dataFrame(41, ByteArray(128))
        val ack = bootloader.ackFor(frame)
        assertEquals(42, OtaAck.parse(ack!!)?.requestedPacket)
    }

    @Test
    fun `force flush final frame completes transfer`() {
        val final = OTATransferPlanner.dataFrame(127, ByteArray(64), forceFlush = true)
        val ack = bootloader.ackFor(final)
        assertEquals(OtaAck.Kind.TRANSFER_COMPLETE, OtaAck.parse(ack!!)?.kind)
    }

    @Test
    fun `finish frame gets no reply`() {
        assertNull(bootloader.ackFor(OTATransferPlanner.finishFrame()))
    }

    @Test
    fun `app-layer frames are not intercepted`() {
        val poll = com.hkt.ble.bletools.core.protocol.HKTFrameEncoder.appFrame(
            0, com.hkt.ble.bletools.core.protocol.CommandCode.QUERY, byteArrayOf(0x01),
        )
        assertNull(bootloader.ackFor(poll))
        // OTA 入口帧（cmd=1 应用层帧，无 bootload 尾）同样穿透
        val entry = com.hkt.ble.bletools.core.protocol.HKTFrameEncoder.appFrame(
            0, com.hkt.ble.bletools.core.protocol.CommandCode.OTA_NOTIFY, ByteArray(0),
        )
        assertNull(bootloader.ackFor(entry))
    }

    @Test
    fun `ack frame is parseable by OtaAck`() {
        val ack = bootloader.ackFor(OTATransferPlanner.dataFrame(0, ByteArray(128)))!!
        assertEquals(14, ack.size)
        assertTrue(OtaAck.parse(ack) != null)
    }
}
