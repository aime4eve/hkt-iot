package com.hkt.ble.bletools.core.ota

import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * OTA 传输引擎（引导层回环，全部由设备 ACK 驱动）。iOS OTAEngine 的 Kotlin 移植。
 * 真机实测修正（2026-09-10，SVC100）后的三阶段契约：
 * 1. **进入升级模式**：app 固件仅在收到 `len=0001` 纯命令帧（communicate.c data[5]==1）
 *    或 `len=0005 且 payload[0]==1`（data[7]==1 兜底）时写更新标志并复位——OTA-NOTIFY-001
 *    旧夹具（0xFF 填充）两条分支都不命中，真机超时实锤。两帧先后下发，双入口兜底。
 * 2. **启动帧节拍**：设备复位进 bootloader 后**被动等待** cmd=1 启动帧（带固件字节数，
 *    uart.c case 1：读 size → ACK(2,0) → start_flag=1）。App 侧每 500 ms 重发启动帧
 *    （Android ENTER_OTA 同款节奏；app 固件阶段该帧被忽略，复位后由 bootloader 处理）。
 * 3. **逐包传输**：ACK(2,n) 请求第 n 包（128 B，末包 8 字节边界补 FF 后仍 cmd=2）；
 *    写满（firmware_write_size≥size）或末包后回 ACK(3)=完成；1 s 无数据 bootloader 催包
 *    ACK(2,n)，10 s 无数据复位传输回 ACK(1,0)=从 0 号包重来（上限 2 次，重置后重发启动帧）。
 *
 * 契约：全部公开方法仅在主线程调用（对应 iOS @MainActor）；[scope] 由构造注入
 * （App=Main dispatcher，测试=TestScope 虚拟时钟）。
 */
class OtaEngine(
    image: ByteArray,
    private val ackTimeoutMs: Long = 12_000,
    private val maxRestarts: Int = 2,
    private val enterAAfterMs: Long = 400,
    private val startBeatAfterMs: Long = 900,
    private val beatIntervalMs: Long = 500,
    private val scope: CoroutineScope,
) {
    enum class EngineError { TIMEOUT, TOO_MANY_RESTARTS, CANCELLED }

    sealed interface State {
        data object Idle : State
        data object EnteringBootloader : State   // 已发进入命令，节拍重发启动帧，等 bootloader ACK(2,0)
        data object Transferring : State
        data object Done : State
        data class Failed(val error: EngineError) : State
    }

    private val _state = MutableStateFlow<State>(State.Idle)
    val state: StateFlow<State> = _state.asStateFlow()
    private val _packetsDone = MutableStateFlow(0)
    val packetsDone: StateFlow<Int> = _packetsDone.asStateFlow()
    private val _restarts = MutableStateFlow(0)
    val restarts: StateFlow<Int> = _restarts.asStateFlow()

    private val packets: List<OtaPacket> = OTATransferPlanner.packets(image)
    val packetCount: Int = packets.size
    val progress: Double
        get() = if (packetCount == 0) 0.0
        else minOf(_packetsDone.value, packetCount).toDouble() / packetCount

    private var send: ((ByteArray) -> Unit)? = null
    private var watchdog: Job? = null
    private var startBeat: Job? = null

    /** 接线发送通道（App 层注入 DeviceSession.sendRaw）。 */
    fun bind(sender: (ByteArray) -> Unit) {
        this.send = sender
    }

    /** 发起升级：进入命令（双入口帧）→ 启动帧节拍，之后一切由 [handle] 驱动。 */
    fun begin() {
        if (_state.value != State.Idle) return
        _state.value = State.EnteringBootloader
        // 入口 B：len=0005 且 payload[0]==1（app 固件 data[7]==1 兜底分支）
        send?.invoke(HKTFrameEncoder.appFrame(0, CommandCode.OTA_NOTIFY, byteArrayOf(0x01, 0x00, 0x00, 0x00)))
        armWatchdog()
        // 入口 A：len=0001 纯命令帧（app 固件 data[5]==1 分支），错开写 flash + 复位时间；
        // 随后进入启动帧节拍——bootloader 就绪前每 beatInterval 重发（Android ENTER_OTA 同款）
        startBeat = scope.launch {
            delay(enterAAfterMs)
            if (_state.value != State.EnteringBootloader) return@launch
            send?.invoke(HKTFrameEncoder.appFrame(0, CommandCode.OTA_NOTIFY, ByteArray(0)))
            delay(startBeatAfterMs - enterAAfterMs)
            while (_state.value == State.EnteringBootloader && isActive) {
                send?.invoke(OTATransferPlanner.startFrame(totalBytes))
                delay(beatIntervalMs)
            }
        }
    }

    /**
     * 喂给引擎一条原始接收帧；是引导层 ACK 则消费并返回 true。
     * done 态继续吞 ACK(3,0)：finish 帧的应答会再回一帧同样的 ACK(3,0)。
     */
    fun handle(data: ByteArray): Boolean {
        val s = _state.value
        if (s != State.EnteringBootloader && s != State.Transferring && s != State.Done) return false
        val ack = OtaAck.parse(data) ?: return false
        if (s == State.Done) return true
        armWatchdog()
        when (ack.kind) {
            OtaAck.Kind.REQUEST_PACKET -> {
                if (_state.value == State.EnteringBootloader) {
                    _state.value = State.Transferring
                    stopStartBeat()
                }
                if (ack.requestedPacket >= packetCount) return true   // 防御：越界请求忽略
                _packetsDone.value = ack.requestedPacket
                val packet = packets[ack.requestedPacket]
                // 末包用 cmd=0xFF 强制落盘：页大小 2048B（uart.c:383），非对齐镜像不满页时
                // 只有 0xFF 能触发落盘→ACK(3,0)（审计 ❌-3；Android 末包同款）
                send?.invoke(OTATransferPlanner.dataFrame(packet.index, packet.chunk, packet.isFinal))
            }
            OtaAck.Kind.TRANSFER_COMPLETE -> {
                // 固件收到 finish 帧（case 3）才 AppProgramRun 跳转新固件（uart.c:410-422）——
                // 阈值完成路径只 ACK 不跳转，漏发 = 设备永久驻留 bootloader（审计 ❌-4）
                send?.invoke(OTATransferPlanner.finishFrame())
                finish(State.Done)
            }
            OtaAck.Kind.RESTART_TRANSFER -> {
                // bootloader 静默 10 s 复位传输：计数清零、重发启动帧从头起步
                _restarts.value += 1
                _packetsDone.value = 0
                if (_restarts.value > maxRestarts) {
                    finish(State.Failed(EngineError.TOO_MANY_RESTARTS))
                    return true
                }
                _state.value = State.EnteringBootloader
                stopStartBeat()
                startBeat = scope.launch {
                    delay(beatIntervalMs)
                    while (_state.value == State.EnteringBootloader && isActive) {
                        send?.invoke(OTATransferPlanner.startFrame(totalBytes))
                        delay(beatIntervalMs)
                    }
                }
            }
        }
        return true
    }

    fun cancel() {
        val s = _state.value
        if (s != State.EnteringBootloader && s != State.Transferring) return
        finish(State.Failed(EngineError.CANCELLED))
    }

    private val totalBytes: Int get() = packets.sumOf { it.chunk.size }

    private fun finish(outcome: State) {
        watchdog?.cancel()
        watchdog = null
        stopStartBeat()
        _state.value = outcome
    }

    private fun stopStartBeat() {
        startBeat?.cancel()
        startBeat = null
    }

    private fun armWatchdog() {
        watchdog?.cancel()
        watchdog = scope.launch {
            delay(ackTimeoutMs)
            // delay 被 cancel（新 watchdog 接管）时直接退出，不得沿用旧超时判定（iOS 取消竞态教训）
            val s = _state.value
            if (s == State.EnteringBootloader || s == State.Transferring) {
                finish(State.Failed(EngineError.TIMEOUT))
            }
        }
    }
}
