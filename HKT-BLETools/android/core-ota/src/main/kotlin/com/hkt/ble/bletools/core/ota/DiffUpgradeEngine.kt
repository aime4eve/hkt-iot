package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.protocol.UpgradeFrame
import com.hkt.ble.bletools.core.protocol.UpgradeInbound
import com.hkt.ble.bletools.core.protocol.UpgradeMsgType
import com.hkt.ble.bletools.core.protocol.UpgradeResultCode
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/**
 * 差分/全量升级引擎（《蓝牙升级协议.docx》2026-11，地磁 DC200 新固件）。
 * 与旧 [OtaEngine]（引导层回环、bootload 后缀帧）是两套互不相干的升级方式：
 *
 * 传输为**设备拉包停等**模式，全程由设备驱动：
 * 1. APP 发 0xF0 新版本通知（版本/总包数/每包长度/包类型/整包 CRC32），超时无应答则重发；
 * 2. 设备回 0xFE(F0) 应答后进入更新态，按 0xF2 逐包请求（包序号从 0 起）；
 * 3. APP 对每个请求回 0xF1（包序号 + 包数据 CRC16 + 包长 + 数据）——设备逐包校验；
 *    同包号连续请求视为设备侧校验失败重拉（设备端连续 5 包失败自行退出升级流程）；
 * 4. 全部接收且整包 CRC 通过后设备回 0xF3 结果（0=成功）；随后设备**自行**在 APP 区内
 *    合成新固件→写标志→复位进 BOOT 搬运安装（此阶段 BLE 可能断开，APP 侧不算失败）。
 *
 * 全量包与差分包走同一传输链，仅 0xF0 包类型字段不同（全量兜底救砖：底包损坏/版本不明）。
 *
 * 契约：全部公开方法仅在主线程调用（对应 iOS @MainActor）；[scope] 由构造注入
 * （App=Main dispatcher，测试=TestScope 虚拟时钟）。入站帧先经 DeviceSession 分流
 * （[UpgradeInbound.parse] 识别）再喂 [handle]。
 */
class DiffUpgradeEngine(
    private val file: ByteArray,
    val pkgType: Int,
    private val hwVersion: Int,
    private val swVersion: Int,
    private val fileCrc32: Long,
    private val packetSize: Int = DEFAULT_PACKET_SIZE,
    private val ackTimeoutMs: Long = 6_000,
    private val pullTimeoutMs: Long = 30_000,
    private val maxNotifyRetries: Int = 2,
    private val maxRepeatRequests: Int = 5,
    private val scope: CoroutineScope,
) {
    enum class EngineError { NOTIFY_TIMEOUT, PULL_TIMEOUT, DEVICE_REJECTED, REPEATED_PACKET, CANCELLED }

    sealed interface State {
        data object Idle : State
        data object Notifying : State       // 0xF0 已发（含重发），等 0xFE(F0)
        data object Transferring : State    // 设备 0xF2 拉包循环中
        data class Done(val extra: Int) : State    // 0xF3 成功；设备随后自行合成/复位安装
        data class Failed(val error: EngineError) : State
    }

    private val _state = MutableStateFlow<State>(State.Idle)
    val state: StateFlow<State> = _state.asStateFlow()
    private val _packetsDone = MutableStateFlow(0)
    val packetsDone: StateFlow<Int> = _packetsDone.asStateFlow()

    /** 0xF3 结果码（Failed(DEVICE_REJECTED) 时非空，UI 文案映射用）。 */
    var reportedCode: Int? = null
        private set

    val packetCount: Int = (file.size + packetSize - 1) / packetSize
    val progress: Double
        get() = if (packetCount == 0) 0.0
        else minOf(_packetsDone.value, packetCount).toDouble() / packetCount

    private var send: ((ByteArray) -> Unit)? = null
    private var watchdog: Job? = null
    private var txSeq = 0
    private var notifyRetries = 0
    private var lastRequested = -1
    private var repeatCount = 0

    /** 接线发送通道（App 层注入 DeviceSession.sendRaw）。 */
    fun bind(sender: (ByteArray) -> Unit) {
        this.send = sender
    }

    /** 发起升级：发 0xF0 通知并武装应答超时；此后一切由 [handle] 驱动。 */
    fun begin() {
        if (_state.value != State.Idle) return
        require(packetCount > 0) { "升级文件为空" }
        _state.value = State.Notifying
        sendNotify()
        armWatchdog(ackTimeoutMs)
    }

    /**
     * 喂给引擎一条原始接收帧；是新升级协议入站帧则消费并返回 true（Done/Failed 态返回 false，
     * 迟到帧由 DeviceSession 分流层静默丢弃，不污染 TLV 解析）。
     */
    fun handle(data: ByteArray): Boolean {
        val s = _state.value
        if (s != State.Notifying && s != State.Transferring) return false
        val inbound = UpgradeInbound.parse(data) ?: return false
        armWatchdog(pullTimeoutMs)
        when (inbound) {
            is UpgradeInbound.GenericAck ->
                // 0xFE(F0) = 设备已知悉新版本 → 等它拉包；其余 ACK 忽略（重置看门狗即可）
                if (s is State.Notifying && inbound.ackedType == UpgradeMsgType.NOTIFY) {
                    _state.value = State.Transferring
                }

            is UpgradeInbound.PacketRequest -> {
                // 容错：设备未回 0xFE 直接拉包，视同已应答
                if (s is State.Notifying) _state.value = State.Transferring
                val n = inbound.packetIndex
                if (n < 0 || n >= packetCount) return true   // 防御：越界请求忽略（旧引擎同款纪律）
                if (n == lastRequested) {
                    repeatCount += 1
                    if (repeatCount > maxRepeatRequests) {
                        finish(State.Failed(EngineError.REPEATED_PACKET))
                        return true
                    }
                } else {
                    lastRequested = n
                    repeatCount = 0
                }
                // 设备请求第 n 包 ⇒ 0..n-1 已落设备（回拉重试不回退进度）
                if (n > _packetsDone.value) _packetsDone.value = n
                send?.invoke(UpgradeFrame.dataFrame(nextSeq(), n, chunk(n)))
            }

            is UpgradeInbound.ResultReport ->
                if (inbound.code == UpgradeResultCode.SUCCESS) {
                    _packetsDone.value = packetCount   // 整包校验通过，进度补满
                    finish(State.Done(inbound.extra))
                } else {
                    reportedCode = inbound.code
                    finish(State.Failed(EngineError.DEVICE_REJECTED))
                }
        }
        return true
    }

    fun cancel() {
        val s = _state.value
        if (s != State.Notifying && s != State.Transferring) return
        finish(State.Failed(EngineError.CANCELLED))
    }

    private fun chunk(index: Int): ByteArray =
        file.copyOfRange(index * packetSize, minOf((index + 1) * packetSize, file.size))

    private fun nextSeq(): Int {
        txSeq = (txSeq + 1) and 0xFF
        return txSeq
    }

    private fun sendNotify() {
        send?.invoke(
            UpgradeFrame.notifyFrame(nextSeq(), hwVersion, swVersion, packetCount, packetSize, pkgType, fileCrc32),
        )
    }

    private fun finish(outcome: State) {
        watchdog?.cancel()
        watchdog = null
        _state.value = outcome
    }

    /**
     * 看门狗：通知阶段超时→重发 0xF0（[maxNotifyRetries] 次后判 NOTIFY_TIMEOUT）；
     * 传输阶段超时→PULL_TIMEOUT。每次合法入站帧重置。
     */
    private fun armWatchdog(timeoutMs: Long) {
        watchdog?.cancel()
        watchdog = scope.launch {
            delay(timeoutMs)
            // delay 被 cancel（新看门狗接管）时直接退出，不得沿用旧超时判定（iOS 取消竞态教训）
            when (_state.value) {
                State.Notifying ->
                    if (notifyRetries < maxNotifyRetries) {
                        notifyRetries += 1
                        sendNotify()
                        armWatchdog(ackTimeoutMs)
                    } else {
                        finish(State.Failed(EngineError.NOTIFY_TIMEOUT))
                    }
                State.Transferring -> finish(State.Failed(EngineError.PULL_TIMEOUT))
                else -> {}
            }
        }
    }

    companion object {
        /**
         * 每包长度 0x0080（帧长 6+128+8=141B）。协议文档默认 0x0100，但 269B 帧在 DA14531
         * 透传链路上被丢（真机联调 2026-11-03：设备 F2 正常到达、APP 的 269B F1 无响应写
         * 发不出去——常见 MTU 上限 247 时 ATT 载荷上限 244B）。141B 与旧 OTA 128B 包同尺寸，
         * 同链路验证多年；设备按 0xF0 声明的 pkt_len 拉包（ble_upgrade.c 只约束 0<plen≤512）。
         */
        const val DEFAULT_PACKET_SIZE = 128
    }
}
