package com.hkt.ble.bletools.demo

/**
 * 演示模式 bootloader 仿真（-mockble 全 UI 流程的最后一块）：对 OtaEngine 发出的引导层帧
 * 回 ACK，让模拟器也能走**真实引擎端到端**（iOS demo 用 tick 380ms 复刻，Android mock 直接
 * 扮演设备——同一套 OtaScreen/OtaEngine 代码路径，真机只换对端）。
 * ACK 契约（uart.c InfoUartAck）：hkt(3) cmd(1) count(2 BE) bootload(8) = 14 B、无 len 无 CRC。
 * - 启动帧（len=0005 cmd=1）→ ACK(2,0)：请求 0 号包（bootloader 就绪后的正常首应答）；
 * - 数据帧（cmd=2，packNum@6..7）→ ACK(2,n+1)：顺序请求下一包；
 * - 强制落盘（cmd=0xFF 末包）→ ACK(3,0)：传输完成（引擎随后发 finish 帧，仿真设备静默跳转）。
 * 无状态：ACK 的 count 直接取自入站帧的 packNum，引擎重启/重传语义无需仿真方记账。
 */
class DemoBootloader {
    fun ackFor(frame: ByteArray): ByteArray? {
        if (frame.size < 14) return null
        val tail = frame.copyOfRange(frame.size - 8, frame.size)
        if (!tail.contentEquals(BOOTLOAD_SUFFIX)) return null
        val len = ((frame[3].toInt() and 0xFF) shl 8) or (frame[4].toInt() and 0xFF)
        return when (frame[5].toInt() and 0xFF) {
            0x01 -> if (len == 5) ack(2, 0) else null                 // 启动帧
            0x02 -> if (len >= 3) {
                val n = ((frame[6].toInt() and 0xFF) shl 8) or (frame[7].toInt() and 0xFF)
                ack(2, n + 1)
            } else null
            0xFF -> ack(3, 0)                                          // 末包强制落盘 → 完成
            else -> null                                               // finish(3) 跳转无应答
        }
    }

    private fun ack(cmd: Int, count: Int): ByteArray {
        val out = ByteArray(14)
        out[0] = 0x68; out[1] = 0x6B; out[2] = 0x74
        out[3] = cmd.toByte()
        out[4] = ((count shr 8) and 0xFF).toByte()
        out[5] = (count and 0xFF).toByte()
        BOOTLOAD_SUFFIX.copyInto(out, 6)
        return out
    }

    companion object {
        private val BOOTLOAD_SUFFIX = "bootload".toByteArray(Charsets.US_ASCII)
    }
}
