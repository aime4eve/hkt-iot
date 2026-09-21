package com.hkt.ble.bletools.core.ble

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

/** P-02 连接三阶段：连接链路 → 发现服务 → 订阅通知。 */
enum class ConnectPhase {
    LINK, SERVICES, SUBSCRIBING;

    val nextPhase: ConnectPhase?
        get() = when (this) {
            LINK -> SERVICES
            SERVICES -> SUBSCRIBING
            SUBSCRIBING -> null
        }
}

/**
 * P-02/SP-4 连接三阶段编排：阶段推进 + 阶段预算超时 + 用户取消。
 * 传输无关：SystemCentral（真蓝牙）在系统回调里调用 advance()/abort()，
 * MockCentral（假蓝牙源）按脚本调用——两条轨道共用同一套状态机。
 * 契约：主线程使用（对应 iOS @MainActor）；[scope] 注入（App=Main，测试=TestScope）。
 */
class ConnectOrchestrator(private val scope: CoroutineScope) {
    /** SP-4 阶段预算（默认连接 10s / 发现 5s / 订阅 5s；测试可注入小预算）。 */
    data class Budget(val linkMs: Long = 10_000, val servicesMs: Long = 5_000, val subscribingMs: Long = 5_000)

    private val _phase = MutableStateFlow<ConnectPhase?>(null)
    val phase: StateFlow<ConnectPhase?> = _phase.asStateFlow()
    private val _failure = MutableStateFlow<ConnectFailure?>(null)
    val failure: StateFlow<ConnectFailure?> = _failure.asStateFlow()
    private val _isConnected = MutableStateFlow(false)
    val isConnected: StateFlow<Boolean> = _isConnected.asStateFlow()
    private val _isCancelled = MutableStateFlow(false)
    val isCancelled: StateFlow<Boolean> = _isCancelled.asStateFlow()

    private var budget = Budget()
    private var timeoutTask: Job? = null
    private var finished = false

    /** 终态回调（failure == null 表示已连接）；App 层据此取消底层连接尝试并落驻留会话。 */
    var onFinish: ((ConnectFailure?) -> Unit)? = null

    /** 会话仍在（未到终态）。 */
    val isActive: Boolean get() = _phase.value != null && !finished

    /** 开始三阶段（可重复调用，内部先复位）。 */
    fun begin(budget: Budget = Budget()) {
        timeoutTask?.cancel()
        this.budget = budget
        _failure.value = null
        _isConnected.value = false
        _isCancelled.value = false
        finished = false
        _phase.value = ConnectPhase.LINK
        armTimeout(ConnectPhase.LINK)
    }

    /** 传输层回调：当前阶段完成（链路建立/服务发现完成/订阅完成）。 */
    fun advance() {
        val p = _phase.value
        if (!isActive || p == null) return
        timeoutTask?.cancel()
        val next = p.nextPhase
        if (next == null) {
            succeed()
            return
        }
        _phase.value = next
        armTimeout(next)
    }

    /** 传输层回调：链路中断 / HKT 服务缺失等不可继续错误。 */
    fun abort(failure: ConnectFailure) {
        if (!isActive) return
        fail(failure)
    }

    /** 用户取消（P-02 取消按钮）。 */
    fun cancel() {
        if (!isActive) return
        fail(ConnectFailure.CANCELLED)
    }

    private fun succeed() {
        finished = true
        _phase.value = null
        _isConnected.value = true
        onFinish?.invoke(null)
    }

    private fun fail(failure: ConnectFailure) {
        finished = true
        _phase.value = null
        _failure.value = failure
        if (failure == ConnectFailure.CANCELLED) _isCancelled.value = true
        onFinish?.invoke(failure)
    }

    private fun armTimeout(phase: ConnectPhase) {
        val ms = when (phase) {
            ConnectPhase.LINK -> budget.linkMs
            ConnectPhase.SERVICES -> budget.servicesMs
            ConnectPhase.SUBSCRIBING -> budget.subscribingMs
        }
        timeoutTask?.cancel()
        timeoutTask = scope.launch {
            delay(ms)
            // delay 被 cancel（新预算接管）时直接退出，不得误杀新阶段（iOS 取消竞态教训）
            if (!finished && _phase.value == phase) {
                fail(
                    when (phase) {
                        ConnectPhase.LINK -> ConnectFailure.TIMEOUT_LINK
                        ConnectPhase.SERVICES -> ConnectFailure.TIMEOUT_SERVICES
                        ConnectPhase.SUBSCRIBING -> ConnectFailure.TIMEOUT_SUBSCRIBING
                    }
                )
            }
        }
    }
}
