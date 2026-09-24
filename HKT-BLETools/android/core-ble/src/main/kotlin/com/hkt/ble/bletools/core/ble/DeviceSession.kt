package com.hkt.ble.bletools.core.ble

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.DeviceSnapshot
import com.hkt.ble.bletools.core.protocol.DeviceSnapshotDecoder
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import com.hkt.ble.bletools.core.protocol.HKTResponseParser
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * 设备会话（R-5/R-7/R-8）：1s 轮询 0xFF 查询 → TLV 解码 → 快照发布。iOS DeviceSession 的 Kotlin 移植。
 * 传输无关：面向 [PeripheralLink]（真 = SystemCentral 链路，假 = MockLink 夹具应答）。
 * - R-7：最后成功响应距今超过 staleAfter 秒 → isStale（界面显示"最后更新 x 秒前"）；
 * - R-8/S-6：响应尾部未知类型 → unknownTail 标记（已解析前缀保留，不崩溃不跳过）；
 * - 字段保留上次有效值（轮询整体刷新语义，SP-5）。
 *
 * 契约：主线程使用（对应 iOS @MainActor）；回调在主线程投递；[scope]/[nowMs]/[epochSeconds]
 * 由构造注入（App=Main+系统时钟，测试=TestScope+虚拟时钟）。
 */
class DeviceSession(
    val family: DeviceFamily,
    val deviceName: String,
    private val link: PeripheralLink,
    private val pollIntervalMs: Long = 1_000,
    private val scope: CoroutineScope,
    private val nowMs: () -> Long = { 0L },
    private val epochSeconds: () -> Long = { 0L },
) {
    enum class CalibrationOutcome { DONE, TIMEOUT }

    private val _snapshot = MutableStateFlow(DeviceSnapshot(family))
    val snapshot: StateFlow<DeviceSnapshot> = _snapshot.asStateFlow()
    private val _unknownTail = MutableStateFlow(false)
    val unknownTail: StateFlow<Boolean> = _unknownTail.asStateFlow()
    private val _pollsSent = MutableStateFlow(0)
    val pollsSent: StateFlow<Int> = _pollsSent.asStateFlow()
    private val _isPolling = MutableStateFlow(false)
    val isPolling: StateFlow<Boolean> = _isPolling.asStateFlow()
    private val _linkLost = MutableStateFlow(false)
    val linkLost: StateFlow<Boolean> = _linkLost.asStateFlow()
    private val _isTimeSyncing = MutableStateFlow(false)
    val isTimeSyncing: StateFlow<Boolean> = _isTimeSyncing.asStateFlow()
    private val _timeSyncOutcome = MutableStateFlow<TimeSyncOutcome?>(null)
    val timeSyncOutcome: StateFlow<TimeSyncOutcome?> = _timeSyncOutcome.asStateFlow()
    private val _lastResponseAtMs = MutableStateFlow<Long?>(null)
    val lastResponseAtMs: StateFlow<Long?> = _lastResponseAtMs.asStateFlow()
    private val _secondsSinceLastResponse = MutableStateFlow<Int?>(null)
    val secondsSinceLastResponse: StateFlow<Int?> = _secondsSinceLastResponse.asStateFlow()

    /** R-7 停摆判定窗口（秒）。 */
    var staleAfter: Int = 4

    /** 引导层 ACK 帧旁路：返回 true 表示该帧已被 OTA 消费，不再走 TLV 解析。 */
    var rawFrameHandler: ((ByteArray) -> Boolean)? = null

    private var packNum = 0
    private var stopped = false
    private var pollingSuspended = false
    private var ackDeferred: CompletableDeferred<Boolean>? = null
    private var ackTimeoutJob: Job? = null
    private var calibrationDeferred: CompletableDeferred<CalibrationOutcome>? = null
    private var calibrationDeadlineJob: Job? = null
    private var pollTask: Job? = null

    /** R-7：链路丢失 / 响应过旧 / 首响应迟迟不来。 */
    val isStale: Boolean
        get() {
            if (_linkLost.value) return true
            val seconds = _secondsSinceLastResponse.value
            if (seconds != null) return seconds > staleAfter
            return _pollsSent.value >= 3
        }

    fun start() {
        if (_isPolling.value) return
        _isPolling.value = true
        stopped = false
        // 契约：onReceive/onDisconnected 在主线程投递——此处同步处理（iOS MainActor.assumeIsolated 同款）
        link.onReceive = { data -> handleReceive(data) }
        link.onDisconnected = { _linkLost.value = true }
        pollTask = scope.launch {
            while (isActive && !stopped) {
                pollOnce()
                delay(pollIntervalMs)
            }
        }
    }

    /** R-31 手动断开 / 页面退出：停止轮询（调用方负责关停传输层连接）。 */
    fun stop() {
        stopped = true
        _isPolling.value = false
        pollTask?.cancel()
        resumeAck(false)
    }

    /** 发送一条协议帧（界面动作调用）。 */
    fun send(cmd: Int, data: ByteArray) {
        packNum = (packNum + 1) and 0xFF
        link.send(HKTFrameEncoder.appFrame(packNum, cmd, data))
    }

    /** OTA 传输期间直发引导层原始帧（自带头/长度/CRC/bootload 后缀，不再包应用帧）。 */
    fun sendRaw(frame: ByteArray) {
        link.send(frame)
    }

    /** OTA/校准/写窗口期间暂停 0xFF 轮询（引导层不应答询帧；FD-004 RX 撞车修法）。 */
    fun setPollingSuspended(suspended: Boolean) {
        pollingSuspended = suspended
    }

    /**
     * 写入命令 + 确认等待（0x02/0x03/0x04/0x05 通用）：固件仅在 callback_BLEAck 回含 0xFF
     * 应答段的专用帧（轮询/状态回应不含该段），收到即视为设备确认；超时未回 = 未确认
     * （可能被固件静默拒绝）。Android 同语义：streamRev 收到 0xFF 段才把 START 事件转 FINISH。
     * 先注册等待再发送（原子），ACK 不会先于注册到达——MockLink 同步应答尤其依赖此序。
     */
    suspend fun sendWrite(cmd: Int, data: ByteArray, timeoutMs: Long = 3_000): Boolean {
        resumeAck(false)   // 模态保证串行，此处仅防御并发重入
        val deferred = CompletableDeferred<Boolean>()
        ackDeferred = deferred
        armAckTimeout(deferred, timeoutMs)
        packNum = (packNum + 1) and 0xFF
        link.send(HKTFrameEncoder.appFrame(packNum, cmd, data))
        return deferred.await()
    }

    /** 对已自行成帧的原始帧等待 0xFF 段确认（对时 SVC 路径等帧内自带 len/packNum 的帧用）。 */
    suspend fun sendRawAwaitingAck(frame: ByteArray, timeoutMs: Long = 3_000): Boolean {
        resumeAck(false)
        val deferred = CompletableDeferred<Boolean>()
        ackDeferred = deferred
        armAckTimeout(deferred, timeoutMs)
        link.send(frame)
        return deferred.await()
    }

    private fun armAckTimeout(deferred: CompletableDeferred<Boolean>, timeoutMs: Long) {
        ackTimeoutJob?.cancel()
        ackTimeoutJob = scope.launch {
            delay(timeoutMs)
            // delay 被取消（新等待接管）时直接退出，不得误杀新注册的等待（iOS 取消竞态教训）
            if (ackDeferred === deferred) resumeAck(false)
        }
    }

    private fun resumeAck(value: Boolean) {
        ackTimeoutJob?.cancel()
        ackTimeoutJob = null
        val deferred = ackDeferred ?: return
        ackDeferred = null
        deferred.complete(value)
    }

    /**
     * 校准（0xFD，仅 UDS/DC 家族）：设备立即 ACK，随后设备端执行，完成时以纯 ASCII 文本
     * "Calibration Done" 上报（非协议帧）。上报超时 = 失败（Android 同源：DC 180s / UDS 120s）。
     */
    suspend fun startCalibration(reportTimeoutMs: Long? = null): CalibrationOutcome {
        finishCalibration(CalibrationOutcome.TIMEOUT)   // 页面中断后的遗留等待先释放，避免续体泄漏
        // 0xFD 与 1Hz 轮询在设备单 RX 缓冲（uart.c 50ms 空闲判帧）上互相挤撞，即发 ACK 常被整帧丢弃；
        // 且校准期间设备主循环被阻塞、本就不应答轮询——全程停轮询，结束（完成/超时）统一恢复
        setPollingSuspended(true)
        try {
            val acked = sendWrite(CommandCode.CALIBRATE, HKTFrameEncoder.fillerPayload(), timeoutMs = 5_000)
            if (!acked) return CalibrationOutcome.TIMEOUT
            val deferred = CompletableDeferred<CalibrationOutcome>()
            calibrationDeferred = deferred
            val budget = reportTimeoutMs ?: (if (family == DeviceFamily.DC200_FAMILY) 180_000 else 120_000)
            calibrationDeadlineJob?.cancel()
            calibrationDeadlineJob = scope.launch {
                delay(budget)
                // delay 被取消（完成上报已接管/新一轮校准）时直接退出，不得误杀新等待
                if (calibrationDeferred === deferred) finishCalibration(CalibrationOutcome.TIMEOUT)
            }
            return deferred.await()
        } finally {
            setPollingSuspended(false)
        }
    }

    /** 页面中途退出：放弃等待本次完成上报（设备端校准继续，无需取消命令）。 */
    fun cancelCalibrationWait() {
        finishCalibration(CalibrationOutcome.TIMEOUT)
    }

    private fun finishCalibration(outcome: CalibrationOutcome) {
        calibrationDeadlineJob?.cancel()
        calibrationDeadlineJob = null
        val deferred = calibrationDeferred ?: return
        calibrationDeferred = null
        deferred.complete(outcome)
    }

    /**
     * SP-25 对时（按家族分路径，审计 ❌-1/❌-2）：
     * - SVC：hkt 对时帧（len=4 特例）直发，等 0xFF 段 ACK（SVC-comm:1585-1601 唯一可达路径）；
     * - UDS：hkt 对时分支不可达（外层 len 门互斥），改发 ASCII `syncDeviceTimestamp:<unix秒>`
     *   （无 ACK；固件固定 +8h 解析——缺陷已记台账）；
     * - DC：ASCII 路径存在 tm_mon+1 月份偏差（DC-comm:1014），同步会写错月份 → 暂不支持。
     */
    enum class TimeSyncOutcome { ACKNOWLEDGED, SENT, UNSUPPORTED, FAILED }

    suspend fun sendTimeSync(): TimeSyncOutcome {
        if (_isTimeSyncing.value) return TimeSyncOutcome.FAILED
        _isTimeSyncing.value = true
        try {
            val outcome: TimeSyncOutcome = when (family) {
                DeviceFamily.SVC100 -> {
                    packNum = (packNum + 1) and 0xFF
                    val frame = HKTFrameEncoder.timeSyncFrame(packNum, epochSeconds().coerceIn(0, 0xFFFFFFFFL))
                    if (sendRawAwaitingAck(frame)) TimeSyncOutcome.ACKNOWLEDGED else TimeSyncOutcome.FAILED
                }
                DeviceFamily.UDS100 -> {
                    link.send("syncDeviceTimestamp:${epochSeconds()}".toByteArray(Charsets.US_ASCII))
                    TimeSyncOutcome.SENT
                }
                DeviceFamily.DC200_FAMILY -> TimeSyncOutcome.UNSUPPORTED
            }
            _timeSyncOutcome.value = outcome
            val holdMs = if (outcome == TimeSyncOutcome.ACKNOWLEDGED) 2_600L else 4_000L
            scope.launch {
                delay(holdMs)
                _timeSyncOutcome.value = null
            }
            return outcome
        } finally {
            _isTimeSyncing.value = false
        }
    }

    private fun pollOnce() {
        if (stopped || pollingSuspended) return
        _pollsSent.value += 1
        packNum = (packNum + 1) and 0xFF
        link.send(HKTFrameEncoder.appFrame(packNum, CommandCode.QUERY, HKTFrameEncoder.fillerPayload()))
        if (_lastResponseAtMs.value == null) {
            // 无首响应：按发送次数近似停摆进度（1 次 ≈ 1s）
            _secondsSinceLastResponse.value = _pollsSent.value
        }
    }

    private fun handleReceive(data: ByteArray) {
        // 引导层应答帧（hkt cmd count bootload，14 B）与 TLV 数据流永久互斥：
        // 识别即分流，无 handler 时丢弃——OTA 完成后迟到的重复 ACK(3,0) 不得污染
        // TLV 解析（否则详情页误报「响应数据异常」，2026-09-11 真机 OTA 实测）
        if (data.size == 14 && data.startsWith(HKTResponseParser.prefix) &&
            data.endsWith(OTA_BOOTLOAD_SUFFIX)
        ) {
            if (rawFrameHandler?.invoke(data) == true) return
            return   // 无 handler（OTA 已收尾）：静默丢弃
        }
        // 校准完成上报是纯 ASCII 文本（非 hkt 帧），parse 会当坏前缀丢弃，须先查
        if (calibrationDeferred != null) {
            val text = data.decodeToString(0, minOf(data.size, 64))
            if (text.contains("Calibration Done")) {
                finishCalibration(CalibrationOutcome.DONE)
                return
            }
        }
        val parsed = try {
            HKTResponseParser.parse(data, family)
        } catch (_: Exception) {
            return
        }
        // ⚠️ 必须解码进【副本】再发射：StateFlow 以 equals 判重，原地改同一实例 = UI 永不刷新
        //（@Observable→StateFlow 移植头号陷阱；iOS 逐属性通知无此问题）
        val updated = _snapshot.value.copy()
        DeviceSnapshotDecoder.decode(parsed.entries, family, updated)
        _snapshot.value = updated
        _unknownTail.value = parsed.unknownTail
        _lastResponseAtMs.value = nowMs()
        _secondsSinceLastResponse.value = 0
        _linkLost.value = false
        // 写入确认：专用 ACK 帧仅含 0xFF 应答段（三家族轮询回应均不带该段，不会误判）
        if (ackDeferred != null && parsed.entries.any { it.type == 0xFF }) {
            resumeAck(true)
        }
    }

    companion object {
        val OTA_BOOTLOAD_SUFFIX = byteArrayOf(
            0x62.toByte(), 0x6F.toByte(), 0x6F.toByte(), 0x74.toByte(),
            0x6C.toByte(), 0x6F.toByte(), 0x61.toByte(), 0x64.toByte(),
        )
    }
}

private fun ByteArray.startsWith(prefix: ByteArray): Boolean {
    if (size < prefix.size) return false
    for (i in prefix.indices) if (this[i] != prefix[i]) return false
    return true
}

private fun ByteArray.endsWith(suffix: ByteArray): Boolean {
    if (size < suffix.size) return false
    val offset = size - suffix.size
    for (i in suffix.indices) if (this[offset + i] != suffix[i]) return false
    return true
}
