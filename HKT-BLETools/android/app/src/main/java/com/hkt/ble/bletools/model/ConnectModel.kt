package com.hkt.ble.bletools.model

import com.hkt.ble.bletools.core.ble.BluetoothPort
import com.hkt.ble.bletools.core.ble.ConnectEvent
import com.hkt.ble.bletools.core.ble.ConnectFailure
import com.hkt.ble.bletools.core.ble.ConnectOrchestrator
import com.hkt.ble.bletools.core.ble.ConnectPhase
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch

/**
 * P-02 连接过程模型：ConnectOrchestrator（阶段/预算/取消）+ 端口事件映射。
 * iOS ConnectModel 的 Kotlin 移植。契约：主线程使用。
 */
class ConnectModel(
    val target: DiscoveredDevice,
    private val port: BluetoothPort,
    private val scope: CoroutineScope,
) {
    enum class Outcome { CONNECTED, CANCELLED, FAILED }

    private val orchestrator = ConnectOrchestrator(scope)

    private val _outcome = MutableStateFlow<Outcome?>(null)
    val outcome: StateFlow<Outcome?> = _outcome.asStateFlow()
    private val _failure = MutableStateFlow<ConnectFailure?>(null)
    val failure: StateFlow<ConnectFailure?> = _failure.asStateFlow()

    /**
     * P-02 三步进度流（0 连接中 / 1 发现服务 / 2 订阅通知；3 = 完成）。
     * 派生 StateFlow——UI 直接 collect，不依赖「碰巧连带刷新」的读法（M6.1 评审 P2-8）。
     * Eagerly：测试无收集器也推进（后台作用域运行）。
     */
    val phaseIndex: StateFlow<Int> = combine(
        orchestrator.isConnected,
        orchestrator.phase,
    ) { connected, phase -> if (connected) 3 else (phase?.ordinal ?: 0) }
        .stateIn(scope, SharingStarted.Eagerly, 0)

    /** 阶段流（失败文案/调试用）。 */
    val phaseFlow: StateFlow<ConnectPhase?> get() = orchestrator.phase

    /** 终态回调（RootView 据此置驻留会话/关闭覆盖层）；与 [outcome] 同步触发。 */
    var onFinished: ((Outcome) -> Unit)? = null

    fun start() {
        _outcome.value = null
        _failure.value = null
        orchestrator.onFinish = { failure ->
            if (failure != null) {
                port.cancelConnect()
                val result = if (failure == ConnectFailure.CANCELLED) Outcome.CANCELLED else Outcome.FAILED
                _failure.value = failure
                _outcome.value = result
                onFinished?.invoke(result)
            } else {
                _outcome.value = Outcome.CONNECTED
                onFinished?.invoke(Outcome.CONNECTED)
            }
        }
        // 顺序关键：先启动状态机再发起连接——同步事件（如假链路）不得在 begin 前丢失
        orchestrator.begin()
        port.connect(target) { event ->
            scope.launch {
                when (event) {
                    is ConnectEvent.LinkEstablished,
                    is ConnectEvent.ServicesDiscovered,
                    is ConnectEvent.NotificationsEnabled -> orchestrator.advance()
                    is ConnectEvent.Failed -> orchestrator.abort(event.failure)
                }
            }
        }
    }

    fun cancel() {
        orchestrator.cancel()
        port.cancelConnect()
    }

    fun retry() = start()
}
