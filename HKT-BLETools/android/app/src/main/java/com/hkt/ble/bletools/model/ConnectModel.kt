package com.hkt.ble.bletools.model

import com.hkt.ble.bletools.core.ble.BluetoothPort
import com.hkt.ble.bletools.core.ble.ConnectEvent
import com.hkt.ble.bletools.core.ble.ConnectFailure
import com.hkt.ble.bletools.core.ble.ConnectOrchestrator
import com.hkt.ble.bletools.core.ble.ConnectPhase
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
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

    /** P-02 三步进度序号（0 连接中 / 1 发现服务 / 2 订阅通知；3 = 完成）。 */
    val phaseIndex: Int
        get() = if (orchestrator.isConnected.value) 3 else (orchestrator.phase.value?.ordinal ?: 0)

    val phase: ConnectPhase? get() = orchestrator.phase.value

    /** 阶段流（Compose 观察用）。 */
    val phaseFlow: StateFlow<ConnectPhase?> get() = orchestrator.phase

    /** 终态回调（RootView 据此置驻留会话/关闭覆盖层）；与 [outcome] 同步触发。 */
    var onFinished: ((Outcome) -> Unit)? = null

    /** 失败文案（R-G4：带阶段/原因定位；ZH——文案随语言切换在 P-07 语言 store 落地后统一）。 */
    val failureText: String?
        get() = when (orchestrator.failure.value) {
            ConnectFailure.TIMEOUT_LINK -> "连接超时"
            ConnectFailure.TIMEOUT_SERVICES -> "发现服务超时"
            ConnectFailure.TIMEOUT_SUBSCRIBING -> "订阅通知超时"
            ConnectFailure.SERVICE_MISSING -> "设备缺少 HKT 服务"
            ConnectFailure.CONNECTION_LOST -> "连接已中断"
            ConnectFailure.CANCELLED -> "已取消"
            null -> null
        }

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
