package com.hkt.ble.bletools.demo

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily

/**
 * 演示模式（-mockble 对应物）仿真应答器。iOS DemoResponder.swift 的 Kotlin 移植（单实例持有三家族夹具，
 * family 由调用方按当前连接设备传入）：
 * - 0xFF 轮询 → 回该家族的标准快照夹具帧（与 shared/fixtures/response-parse.json 同源字节），
 *   帧内 0x8D 电源位按当前电源态回写；
 * - 0x02 写配置 → 回专用 ACK 帧（hkt 00 seq FF 值，固件 callback_BLEAck 同构），
 *   并把载荷按**固定字节偏移**写进夹具（偏移对冻结夹具逐位核对过，同 iOS applyConfig）——
 *   保存后下轮轮询即回新值，与真机一致；
 * - 0x03 实时任务 → 阀状态写回 SVC 夹具 0x3C（v1s@13 / v2s@17）；busy 旋钮=静默忽略；
 * - 0xFD 校准 → 立即 ACK（完成文本 "Calibration Done" 由 App 层接线延迟注入，见 ComposeActivity）；
 * - 0xFE 开关机 → 按真载荷语义（F-1 修正：电源字节在帧偏移 10，payload[3]）改写夹具电源位。
 *   ⚠️ 有意偏离 iOS demo（iOS 读 frame[7] 是潜伏错位、从未被真实 0xFE 路径触发）；
 *   且真机固件对有效 0xFE 载荷必回 ACK（2026-09-20 实证），故此处回 ACK 供详情页 sendWrite 判定；
 * - 0x04/0x05 → ACK；其余命令 → 静默（与真机行为一致）。
 */
class DemoResponder {
    /** 演示旋钮：false 时 0x02 静默拒绝（验收配置失败横幅）。 */
    var configAcks = true

    /** 演示旋钮：true 时 0x03 静默忽略（验收任务页 busy 横幅）。 */
    var realtimeBusy = false

    private var powerOn = true

    private val frames = hashMapOf(
        DeviceFamily.DC200_FAMILY to hex(
            "686B740001010B1C8D0103573A013B0084008600145D00785EFF885F019060000A0014001E00280032003C00460050005A0064",
        ),
        DeviceFamily.UDS100 to hex(
            "686B74000201020A8D018B0FA0090061A80A002EE00E00144400280045003C46043847004801900BB886001E10010203041105060708",
        ),
        DeviceFamily.SVC100 to hex(
            "686B740003010D0D8D0103633C01000064010101F440024101420543018A1986001E",
        ),
    )

    fun respond(frame: ByteArray, family: DeviceFamily?): ByteArray? {
        if (frame.size <= 6) return null
        return when (frame[6].toInt() and 0xFF) {
            CommandCode.QUERY -> {
                family ?: return null
                val response = frames[family] ?: return null
                val out = response.copyOf()
                val idx = out.indexOfFirst { it == 0x8D.toByte() }
                if (idx in 0 until out.size - 1) out[idx + 1] = (if (powerOn) 0x01 else 0x00).toByte()
                out
            }
            CommandCode.CONFIG -> {
                family ?: return null
                applyConfig(frame, family)
                if (configAcks) ACK_FRAME else null
            }
            CommandCode.SVC_REALTIME_TASK -> {
                // 载荷：valve(1) state(1) 时长(2) 脉冲(2)——立即驱动阀状态写回夹具 0x3C
                if (!realtimeBusy && frame.size > 9) {
                    applyValveState(valve = frame[7].toInt() and 0xFF, state = frame[8].toInt() and 0xFF)
                }
                if (realtimeBusy) null else ACK_FRAME   // 设备忙=静默忽略，与固件一致
            }
            CommandCode.SVC_TIMED_TASK, CommandCode.SVC_DELETE_TASK -> ACK_FRAME
            CommandCode.POWER -> {
                if (frame.size >= 11) powerOn = (frame[10].toInt() and 0xFF) == 0x01
                ACK_FRAME
            }
            CommandCode.CALIBRATE -> ACK_FRAME
            else -> null
        }
    }

    /** 把 0x02 载荷按固定 TLV 偏移写进演示夹具（偏移对冻结夹具逐位核对过，同 iOS applyConfig）。 */
    private fun applyConfig(request: ByteArray, family: DeviceFamily) {
        val response = frames[family] ?: return
        if (request.size < 15) return
        fun put(offset: Int, bytes: ByteArray) {
            bytes.forEachIndexed { i, b -> if (offset + i < response.size) response[offset + i] = b }
        }
        when (family) {
            DeviceFamily.UDS100 -> {   // 载荷：report(2) gps(2) low(2) high(2)
                put(42, byteArrayOf(request[7], request[8]))                                // 0x86 上报周期
                put(29, byteArrayOf(request[9], request[10]))                               // 0x45 GPS 周期
                put(37, byteArrayOf(request[11], request[12], request[13], request[14]))    // 0x48 低/高阈值
            }
            DeviceFamily.DC200_FAMILY -> {   // 载荷：report(2) mode(1)
                put(19, byteArrayOf(request[7], request[8]))                                // 0x86 上报周期
                put(15, byteArrayOf(request[9]))                                            // 0x3B 工作模式
            }
            DeviceFamily.SVC100 -> {   // 载荷：vol port stable autoPower tz(各1) report(2)
                put(22, byteArrayOf(request[7]))                                            // 0x40 电压档
                put(24, byteArrayOf(request[8]))                                            // 0x41 端口功能
                put(26, byteArrayOf(request[9]))                                            // 0x42 稳定时长
                put(28, byteArrayOf(request[10]))                                           // 0x43 自动开关机
                put(30, byteArrayOf(request[11]))                                           // 0x8A 时区
                put(32, byteArrayOf(request[12], request[13]))                              // 0x86 上报周期
            }
        }
    }

    /** 实时任务：阀状态写进 SVC 夹具 0x3C（value@13=v1s, @17=v2s），详情页下轮轮询即见。 */
    private fun applyValveState(valve: Int, state: Int) {
        val response = frames[DeviceFamily.SVC100] ?: return
        fun setBit(offset: Int, on: Boolean) {
            if (offset < response.size) response[offset] = (if (on) 0x01 else 0x00).toByte()
        }
        when (valve) {
            1 -> setBit(13, state == 1)
            2 -> setBit(17, state == 1)
            0 -> { setBit(13, state == 1); setBit(17, state == 1) }
        }
    }

    companion object {
        /** 专用 ACK 帧（三家族同构）：hkt(3) + 0x00 + seq(1) + 0xFF 段（类型 + 1 字节值）。 */
        val ACK_FRAME = hex("686B740000FF00")

        private fun hex(hex: String): ByteArray {
            val bytes = ByteArray(hex.length / 2)
            for (i in bytes.indices) bytes[i] = hex.substring(i * 2, i * 2 + 2).toInt(16).toByte()
            return bytes
        }
    }
}
