package com.hkt.ble.bletools.demo

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily

/**
 * 演示应答器（-mockble 模式）：按 HKT 协议对 MockCentral 的请求帧回响应。
 * 三家族查询夹具与 shared/fixtures/response-parse.json 同源（RX-*-QUERY-001）。
 * iOS 对照：App/DemoResponder.swift。
 */
object DemoResponder {
    /** 三家族 0xFF 查询响应夹具（与 response-parse.json RX-*-QUERY-001 逐字节同源）。 */
    private val familyQueryResponse = mapOf(
        DeviceFamily.UDS100 to
            "686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708",
        DeviceFamily.DC200_FAMILY to
            "686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064",
        DeviceFamily.SVC100 to
            "686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E",
    )

    /** 演示旋钮（对应 iOS MOCK_* 环境变量）。 */
    var calDoneDelayMs: Long = 0          // MOCK_CAL_DONE_DELAY：校准完成文本延迟
    var calFail: Boolean = false          // MOCK_CAL_FAIL：不回校准完成
    var cfgAck: Boolean = true            // MOCK_CFG_ACK=0 → 0x02 静默拒绝演示

    /** 每台 MockCentral 独立一份会话内存（夹具可写：0x02 配置改夹具、0xFE 翻电源位）。 */
    class Session(private val family: DeviceFamily) {
        private var queryResponse = hexToBytes(familyQueryResponse.getValue(family))
        private var calAnnouncedAt = 0L

        /** 返回对请求帧的响应帧；null = 静默（不回）。 */
        fun respond(frame: ByteArray, nowMs: Long): ByteArray? {
            if (frame.size < 7 || frame[0] != 0x68.toByte() || frame[1] != 0x6B.toByte() || frame[2] != 0x74.toByte()) {
                return null
            }
            val cmd = frame[6].toInt() and 0xFF
            return when (cmd) {
                CommandCode.QUERY -> queryResponse   // 0xFF 轮询 → 家族夹具
                CommandCode.CONFIG -> {              // 0x02 写配置 → ACK 并把载荷写进夹具对应 TLV 偏移
                    if (cfgAck) {
                        patchConfig(frame)
                        ack()
                    } else {
                        null
                    }
                }
                CommandCode.CALIBRATE -> {           // 0xFD 校准 → 立即 ACK，随后纯文本 "Calibration Done"
                    calAnnouncedAt = if (calFail) -1 else nowMs + calDoneDelayMs
                    ack()
                }
                CommandCode.SVC_REALTIME_TASK,
                CommandCode.SVC_TIMED_TASK,
                CommandCode.SVC_DELETE_TASK -> ack()
                CommandCode.POWER -> {               // 0xFE 开关机 → ACK + 翻夹具 0x8D 电源位
                    if (frame.size >= 11) {
                        val on = frame[10].toInt() and 0xFF == 0x01
                        setPowerByte(if (on) 1 else 0)
                    }
                    ack()
                }
                CommandCode.TIME_SYNC -> ack()       // 0x06 对时（SVC 演示路径）
                else -> null
            }
        }

        /** 需要异步上报的事件（校准完成纯文本）；App 演示层轮询消费。 */
        fun pendingText(nowMs: Long): ByteArray? {
            if (calAnnouncedAt in 1..nowMs) {
                calAnnouncedAt = 0
                return "Calibration Done".toByteArray(Charsets.US_ASCII)
            }
            return null
        }

        private fun ack() = hexToBytes("686B740000FFFF")

        /** 0x02 载荷写进夹具对应 TLV 偏移（演示闭环：配置页保存后详情页字段联动）。 */
        private fun patchConfig(frame: ByteArray) {
            val len = ((frame[4].toInt() and 0xFF) shl 8) or (frame[5].toInt() and 0xFF)
            val payload = frame.copyOfRange(7, 7 + (len - 1).coerceAtLeast(0))
            fun patch(type: Int, value: Int) {
                val idx = tlvOffset(type) ?: return
                if (idx + 1 < queryResponse.size) queryResponse[idx + 1] = value.toByte()
            }
            when (family) {
                DeviceFamily.UDS100 -> if (payload.size >= 8) {
                    patchInto(0x86, (payload[0].toInt() and 0xFF) shl 8 or (payload[1].toInt() and 0xFF), 2)
                    patchInto(0x45, (payload[2].toInt() and 0xFF) shl 8 or (payload[3].toInt() and 0xFF), 2)
                    patchInto(0x48, (payload[4].toInt() and 0xFF) shl 8 or (payload[5].toInt() and 0xFF), 2, 0)
                    patchInto(0x48, (payload[6].toInt() and 0xFF) shl 8 or (payload[7].toInt() and 0xFF), 2, 2)
                }
                DeviceFamily.DC200_FAMILY -> if (payload.size >= 3) {
                    patchInto(0x86, (payload[0].toInt() and 0xFF) shl 8 or (payload[1].toInt() and 0xFF), 2)
                    patch(0x3B, payload[2].toInt() and 0xFF)
                }
                DeviceFamily.SVC100 -> if (payload.size >= 7) {
                    patch(0x40, payload[0].toInt() and 0xFF)
                    patch(0x41, payload[1].toInt() and 0xFF)
                    patch(0x42, payload[2].toInt() and 0xFF)
                    patch(0x43, payload[3].toInt() and 0xFF)
                    patch(0x8A, payload[4].toInt() and 0xFF)
                    patchInto(0x86, (payload[5].toInt() and 0xFF) shl 8 or (payload[6].toInt() and 0xFF), 2)
                }
            }
        }

        private fun patchInto(type: Int, value: Int, width: Int, offsetInValue: Int = 0) {
            val idx = tlvOffset(type) ?: return
            if (offsetInValue + width > width && idx + 1 + offsetInValue + width - 1 < queryResponse.size) {
                if (width == 2) {
                    queryResponse[idx + 1 + offsetInValue] = ((value shr 8) and 0xFF).toByte()
                    queryResponse[idx + 2 + offsetInValue] = (value and 0xFF).toByte()
                } else {
                    queryResponse[idx + 1 + offsetInValue] = value.toByte()
                }
            }
        }

        /** 夹具内 type 记录的 value 起始下标（type(1) 后一位）；未知类型返回 null。 */
        private fun tlvOffset(type: Int): Int? {
            var offset = 5
            while (offset < queryResponse.size) {
                val t = queryResponse[offset].toInt() and 0xFF
                if (t == type) return offset + 1
                val width = valueWidth(t) ?: return null
                offset += 1 + width
            }
            return null
        }

        private fun valueWidth(type: Int): Int? = when (family) {
            DeviceFamily.DC200_FAMILY -> when (type) {
                0x01 -> 2; 0x03 -> 1; 0x3A -> 1; 0x3B -> 1; 0x5D, 0x5E, 0x5F -> 2; 0x60 -> 20
                0x80 -> 4; 0x84 -> 1; 0x86 -> 2; 0x8D -> 1; 0xFF -> 1; else -> null
            }
            DeviceFamily.UDS100 -> when (type) {
                0x01 -> 2; 0x09, 0x0A -> 3; 0x0E -> 2; 0x10, 0x11 -> 4; 0x28 -> 1; 0x44 -> 1
                0x45 -> 2; 0x46 -> 2; 0x47 -> 1; 0x48 -> 4; 0x80 -> 4; 0x86 -> 2; 0x8B -> 2
                0x8D -> 1; 0xFF -> 1; else -> null
            }
            DeviceFamily.SVC100 -> when (type) {
                0x01 -> 2; 0x03 -> 1; 0x3C -> 8; 0x40, 0x41, 0x42, 0x43 -> 1
                0x80 -> 4; 0x86 -> 2; 0x8A -> 1; 0x8D -> 1; 0xFF -> 1; else -> null
            }
        }

        private fun setPowerByte(value: Int) {
            val idx = tlvOffset(0x8D) ?: return
            if (idx < queryResponse.size) queryResponse[idx] = value.toByte()
        }
    }

    private fun hexToBytes(hex: String): ByteArray {
        val bytes = ByteArray(hex.length / 2)
        for (i in bytes.indices) bytes[i] = hex.substring(i * 2, i * 2 + 2).toInt(16).toByte()
        return bytes
    }
}
