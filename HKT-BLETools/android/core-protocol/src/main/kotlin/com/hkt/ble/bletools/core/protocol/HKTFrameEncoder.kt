package com.hkt.ble.bletools.core.protocol

/**
 * App-frame TX encoding: hkt(3) packNum(1) len(2 BE) cmd(1) data(n) crc(2 over cmd+data).
 * Firmware authority: 各工程 USER/Drive/communicate.c — hkt dispatch reads data[5]=lenLow, data[6]=cmd,
 * payload from data[7]; status query / power / cal / OTA-notify all carry a 4-byte filler payload
 * (power byte at payload[3], frame offset 10).
 * Vectors: shared/fixtures/app-frame-tx.json (TX-*).
 */
object HKTFrameEncoder {
    val prefix = byteArrayOf(0x68, 0x6B, 0x74)

    fun appFrame(packNum: Int, cmd: Int, data: ByteArray): ByteArray {
        val bodyLength = 1 + data.size
        val frame = ByteArray(3 + 1 + 2 + bodyLength + 2)
        var o = 0
        prefix.copyInto(frame, o); o += prefix.size
        frame[o++] = packNum.toByte()
        frame[o++] = ((bodyLength shr 8) and 0xFF).toByte()
        frame[o++] = (bodyLength and 0xFF).toByte()
        frame[o++] = cmd.toByte()
        data.copyInto(frame, o); o += data.size
        val crc = CRC16.ccitt(frame.copyOfRange(6, o))   // CRC over cmd+data
        frame[o++] = ((crc shr 8) and 0xFF).toByte()
        frame[o] = (crc and 0xFF).toByte()
        return frame
    }

    /** 4-byte filler payload used by 0xFF/0xFE/0xFD/0x01 (Android compatibility: 0xFF fill,
     *  power-on = 0x00000001). firmwareReference: communicate.c data[10] reads. */
    fun fillerPayload(fill: Int = 0xFF): ByteArray = ByteArray(4) { fill.toByte() }

    fun powerPayload(on: Boolean): ByteArray = byteArrayOf(0x00, 0x00, 0x00, (if (on) 0x01 else 0x00).toByte())

    // MARK: 0x02 写配置载荷（三家族）
    // 契约出处：Android Communicate.kt streamDevice(0x02)；固件核对 UDS data[5]==9 / DC data[5]==4 /
    // SVC data[5]==8 分支（载荷顺序与宽度逐字节一致）。任一参数非法固件整包拒绝（UDS 低/高阈值非法不回 ACK），
    // App 侧已按同规则前置校验。

    /** UDS100：上报周期 + GPS 周期 + 满溢低阈值 + 高阈值，各 u16 BE（8B，len=9）。 */
    fun udsConfigPayload(reportMin: Int, gpsMin: Int, lowMM: Int, highMM: Int): ByteArray =
        u16be(reportMin) + u16be(gpsMin) + u16be(lowMM) + u16be(highMM)

    /** DC200 家族：上报周期 u16 BE + 工作模式 u8（3B，len=4）。 */
    fun dcConfigPayload(reportMin: Int, mode: Int): ByteArray = u16be(reportMin) + byteArrayOf(mode.toByte())

    /** SVC100：电压档 / 端口功能 / 稳定时长 / 自动开关机 / 时区（各 u8）+ 上报周期 u16 BE（7B，len=8）。 */
    fun svcConfigPayload(volLevel: Int, port: Int, stableS: Int, autoPower: Int, timezone: Int, reportMin: Int): ByteArray =
        byteArrayOf(volLevel.toByte(), port.toByte(), stableS.toByte(), autoPower.toByte(), timezone.toByte()) + u16be(reportMin)

    // MARK: SVC 阀门任务（0x03/0x04/0x05）
    // 契约出处：Android Communicate.kt streamDevice + 固件 communicate.c data[5]==7/11/2 分支。
    // 设备忙（本地任务执行中）时 0x03 被静默忽略（无 ACK）；0x04/0x05 参数非法同样静默拒绝。

    /** 实时任务（0x03）：阀门 / 动作 / 持续秒数 u16 BE / 脉冲数 u16 BE（6B，len=7）。 */
    fun svcRealtimeTaskPayload(valve: Int, state: Int, durationS: Int, pulse: Int): ByteArray =
        byteArrayOf(valve.toByte(), state.toByte()) + u16be(durationS) + u16be(pulse)

    /** 定时任务（0x04）：槽位 / 阀门 / 动作 / 脉冲 u16 BE / 起止（午夜起分钟数 u16 BE）/
     *  重复位（bit0=周一 … bit6=周日）。共 10B，len=11。 */
    fun svcTimedTaskPayload(id: Int, valve: Int, state: Int, pulse: Int, startMinute: Int, endMinute: Int, repeatMask: Int): ByteArray =
        byteArrayOf(id.toByte(), valve.toByte(), state.toByte()) + u16be(pulse) + u16be(startMinute) + u16be(endMinute) +
            byteArrayOf(repeatMask.toByte())

    /** 删除定时任务（0x05）：槽位 id，0xFF = 全部删除（并强制停止执行中任务）。1B，len=2。 */
    fun svcDeleteTaskPayload(id: Int): ByteArray = byteArrayOf(id.toByte())

    /**
     * Time-sync (0x06) frame — **SVC100 only**: the hkt len=4/cmd=6 branch is reachable in
     * SVC (communicate.c:1585, answers ACK) but is dead code in UDS/DC (outer len gates make
     * it unreachable — audit ❌-2). UDS uses ASCII `syncDeviceTimestamp:<unix>` (no ACK,
     * firmware adds a fixed +8h); DC's ASCII path has a tm_mon+1 defect, so time sync is
     * disabled for DC. Wire quirk: declared len is 4 (data only, NOT cmd+data like every
     * other command); the firmware guard is `data[5]==4` while the stamp is read from
     * data[7..10]. stamp == 0 is silently ignored by the firmware.
     */
    fun timeSyncFrame(packNum: Int, stampBE: Long): ByteArray {
        val body = byteArrayOf(CommandCode.TIME_SYNC.toByte()) + u32be(stampBE)
        val frame = ByteArray(3 + 1 + 2 + body.size + 2)
        var o = 0
        prefix.copyInto(frame, o); o += prefix.size
        frame[o++] = packNum.toByte()
        frame[o++] = 0x00            // len=4：仅 data 长度，不含 cmd（固件卫语句 data[5]==4 依赖此怪癖）
        frame[o++] = 0x04
        body.copyInto(frame, o); o += body.size
        val crc = CRC16.ccitt(body)
        frame[o++] = ((crc shr 8) and 0xFF).toByte()
        frame[o] = (crc and 0xFF).toByte()
        return frame
    }
}

/** Command codes shared by all three firmware families (communicate.h dispatch). */
object CommandCode {
    const val OTA_NOTIFY = 0x01
    const val CONFIG = 0x02
    const val SVC_REALTIME_TASK = 0x03
    const val SVC_TIMED_TASK = 0x04
    const val SVC_DELETE_TASK = 0x05
    const val TIME_SYNC = 0x06
    const val CALIBRATE = 0xFD
    const val POWER = 0xFE
    const val QUERY = 0xFF
}
