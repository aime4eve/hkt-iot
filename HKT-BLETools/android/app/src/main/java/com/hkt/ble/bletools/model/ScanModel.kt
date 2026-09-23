package com.hkt.ble.bletools.model

import com.hkt.ble.bletools.core.ble.BLEAvailability
import com.hkt.ble.bletools.core.ble.BluetoothPort
import com.hkt.ble.bletools.ui.HktLang
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.device.DeviceRegistry
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/** 会话驻留期可见的设备卡片模型（R-32）：已连接设备置顶带徽章，点击直接回详情。 */
data class ResidentDevice(val name: String, val identifier: String)

/**
 * P-01 扫描页模型：能力状态 + 去重发现列表 + 3 轮扫描生命周期（R-1/SP-1）。
 * iOS ScanModel 的 Kotlin 移植。契约：主线程使用（scope=Main 注入）。
 * 连接过程模型（ConnectModel，P-02）在 M6 接入——[CardTapOutcome.Connect] 暂携带目标设备。
 */
class ScanModel(
    private val port: BluetoothPort,
    private val scope: CoroutineScope,
    private val nowMs: () -> Long = System::currentTimeMillis,
    /** 演示模式（-mockble 对应物）：能力就绪后自动开始扫描。 */
    private val autoStartOnReady: Boolean = false,
    /** 演示脚本：扫描命中该前缀设备即停扫直连（-demo-flow 同构）。⚠️ 须构造传入——
     *  MockCentral 激活即同步发出首次发现，构造后赋值会错过首帧（iOS 为构造参数）。 */
    demoAutoConnectPrefix: String? = null,
    /** 演示脚本：扫描总时长上限（毫秒），到点自动停（原型 7 秒限时同构；null=不限）。 */
    demoAutoStopAfterMs: Long? = null,
    /** 连接成功回调——⚠️ 必须构造期注入：demoConnect 直连流在构造内同步触发
     *  （lifecycleScope=Main.immediate），构造后赋值会错过（同 demoAutoConnectPrefix 教训，M6.1 评审）。 */
    onSessionStarted: ((DeviceSession) -> Unit)? = null,
) {
    private val _availability = MutableStateFlow(BLEAvailability.INITIALIZING)
    val availability: StateFlow<BLEAvailability> = _availability.asStateFlow()
    private val _devices = MutableStateFlow<List<DiscoveredDevice>>(emptyList())
    val devices: StateFlow<List<DiscoveredDevice>> = _devices.asStateFlow()
    private val _isScanning = MutableStateFlow(false)
    val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()
    private val _scanRound = MutableStateFlow(1)
    val scanRound: StateFlow<Int> = _scanRound.asStateFlow()
    private val _scanElapsed = MutableStateFlow(0)
    val scanElapsed: StateFlow<Int> = _scanElapsed.asStateFlow()

    private val _allowedPrefixes = MutableStateFlow(DiscoveredDevice.supportedPrefixes)

    /** SP-1：名称前缀过滤（MPS/SVC/UDS/EPS，设置页可调并持久化）。 */
    var allowedPrefixes: Set<String>
        get() = _allowedPrefixes.value
        set(value) { _allowedPrefixes.value = value }
    val allowedPrefixesFlow: StateFlow<Set<String>> = _allowedPrefixes.asStateFlow()

    private val _rssiThreshold = MutableStateFlow(-80)

    /** SP-1：信号强度阈值（默认 -80 dBm）。 */
    var rssiThreshold: Int
        get() = _rssiThreshold.value
        set(value) { _rssiThreshold.value = value }
    val rssiThresholdFlow: StateFlow<Int> = _rssiThreshold.asStateFlow()

    /** P-07 扫描过滤：勾选/取消某前缀；已在列的设备按新规则复筛（iOS togglePrefix 同构）。 */
    fun togglePrefix(prefix: String) {
        allowedPrefixes = if (prefix in allowedPrefixes) allowedPrefixes - prefix else allowedPrefixes + prefix
        if (_isScanning.value) {
            _devices.value = _devices.value.filter { d ->
                DiscoveredDevice.prefixOf(d.name)?.let(allowedPrefixes::contains) ?: false
            }
        }
    }

    /** R-32：驻留会话（null=无会话）。 */
    private val _residentDevice = MutableStateFlow<ResidentDevice?>(null)
    val residentDevice: StateFlow<ResidentDevice?> = _residentDevice.asStateFlow()

    /** R-31：最近一次释放的会话（断开后设备进"最近设备"，空列表时显示）。 */
    private val _lastSession = MutableStateFlow<ResidentDevice?>(null)
    val lastSession: StateFlow<ResidentDevice?> = _lastSession.asStateFlow()

    /** R-32 切换确认框待连接目标（null = 无待确认）。 */
    private val _pendingSwitch = MutableStateFlow<DiscoveredDevice?>(null)
    val pendingSwitch: StateFlow<DiscoveredDevice?> = _pendingSwitch.asStateFlow()

    /** 活动会话（连接成功即建，断开即清）。 */
    private val _activeSession = MutableStateFlow<DeviceSession?>(null)
    val activeSession: StateFlow<DeviceSession?> = _activeSession.asStateFlow()

    /** 连接成功后请求打开详情页（覆盖层内触发）；扫描页消费后调 [consumeRequestShowDetail] 复位（评审 P1-2：不挂空线）。 */
    private val _requestShowDetail = MutableStateFlow(false)
    val requestShowDetail: StateFlow<Boolean> = _requestShowDetail.asStateFlow()

    fun consumeRequestShowDetail() {
        _requestShowDetail.value = false
    }

    /** 定位流连接成功后请求打开详情（iOS 覆盖层置位 requestShowDetail 同构；LocateFlowScreen 调用）。 */
    fun requestDetailOpen() {
        _requestShowDetail.value = true
    }

    val isReady: Boolean get() = _availability.value.isUsable

    private var scanTicker: Job? = null
    private val loggedDiscoveries = mutableSetOf<String>()
    private var scanTotalMs = 0L
    private var autoStarted = false
    private var stopped = false

    /** R-2 目标设备定位（P-01b 定位流 UI 观察：phase/suffix/hit 全 StateFlow）。 */
    enum class LocatePhase { FINDING, NOT_FOUND }

    private val _locatePhase = MutableStateFlow<LocatePhase?>(null)
    val locatePhase: StateFlow<LocatePhase?> = _locatePhase.asStateFlow()
    private val _locateSuffix = MutableStateFlow<String?>(null)
    val locateSuffix: StateFlow<String?> = _locateSuffix.asStateFlow()
    private var locateTimeoutJob: Job? = null
    private var savedPrefixes: Set<String>? = null

    /** 定位命中（扫码 DevEUI 后缀匹配）：定位层消费并自渲染连接覆盖层——
     *  勿在此直连会话：iOS 2026-09-15 真机缺陷同源（两个全屏弹层竞争宿主/旧会话协议串扰）。 */
    private val _locateHitDevice = MutableStateFlow<DiscoveredDevice?>(null)
    val locateHitDevice: StateFlow<DiscoveredDevice?> = _locateHitDevice.asStateFlow()

    fun consumeLocateHit() {
        _locateHitDevice.value = null
    }

    /** 演示脚本：扫描命中该前缀设备即停扫直连（-demo-flow 同构）。命中后置 null。 */
    var demoAutoConnectPrefix: String? = demoAutoConnectPrefix
        private set

    /** 演示脚本：扫描总时长上限（毫秒），到点自动停（原型 7 秒限时同构；null=不限）。 */
    var demoAutoStopAfterMs: Long? = demoAutoStopAfterMs

    /** 详情页/覆盖层需要展示新会话时由页面注册消费。 */
    /** 连接成功回调（构造期注入，见构造参数说明）。 */
    var onSessionStarted: ((DeviceSession) -> Unit)? = onSessionStarted
        private set

    init {
        // 启动即激活（真机：触发系统蓝牙权限弹窗，R-24）；就绪后按需自动扫描（演示模式）
        port.activate { availability ->
            scope.launch {
                _availability.value = availability
                if (availability.isUsable && autoStartOnReady && !autoStarted && !_isScanning.value) {
                    autoStarted = true
                    startScan()
                }
            }
        }
    }

    /** 供演示层在停止扫描后关闭模型（Activity 销毁）。 */
    fun shutdown() {
        stopped = true
        scanTicker?.cancel()
        locateTimeoutJob?.cancel()
        _activeSession.value?.stop()
    }

    fun startScan() {
        // R-32 重扫健康检测（原型 toggleScan）：活会话最后成功轮询距今 >5s = 无蓝牙信号
        //（连接态设备已停止广播，不能以扫描可见性判活，只看轮询心跳）→ 释放并原身份重连
        val session = _activeSession.value
        if (session != null && !session.linkLost.value) {
            val stale = session.secondsSinceLastResponse.value
            if (stale != null && stale > 5) {
                session.stop()
                _activeSession.value = null
                port.disconnectDevice()
                reconnectResident()
                return
            }
        }
        // 已在扫描中再点「附近设备」=重开新会话（清空旧发现）而非静默忽略：
        // 换环境后旧设备必须从列表消失（2026-09-20 真机缺陷修复同源语义）
        if (_isScanning.value) stopScan()
        if (!isReady) return
        _isScanning.value = true
        loggedDiscoveries.clear()
        LogStore.info(if (HktLang.isZh) "扫描启动" else "Scan started")
        _scanRound.value = 1
        _scanElapsed.value = 0
        scanTotalMs = 0
        scanTicker?.cancel()
        scanTicker = scope.launch {
            while (isActive && _isScanning.value) {
                delay(1_000)
                if (!_isScanning.value) return@launch
                _scanElapsed.value += 1
                scanTotalMs += 1_000
                val cap = demoAutoStopAfterMs
                if (cap != null && scanTotalMs >= cap) {
                    stopScan()   // 演示脚本节奏（原型 7 秒限时同构）
                    return@launch
                }
                if (_scanElapsed.value >= SCAN_ROUND_SECONDS) {
                    if (_scanRound.value < 3) {
                        _scanRound.value += 1
                        _scanElapsed.value = 0   // 轮次推进：列表继续累积
                    } else {
                        stopScan()   // 第 3 轮完成：自动停止，保留结果等待手动再扫
                        return@launch
                    }
                }
            }
        }
        port.startScan(
            com.hkt.ble.bletools.core.ble.ScanOptions(
                allowedPrefixes = allowedPrefixes,
                rssiThreshold = rssiThreshold,
            ),
        ) { availability, devices ->
            scope.launch {
                _availability.value = availability
                // R-2 定位模式：后缀命中即停扫、置 hit 交定位层接管（优先于常规列表）
                if (locatePhase.value == LocatePhase.FINDING) {
                    val suffix = _locateSuffix.value
                    val hit = devices.firstOrNull { suffix != null && it.name.uppercase().contains(suffix) }
                    if (hit != null) {
                        _locatePhase.value = null
                        _locateSuffix.value = null
                        locateTimeoutJob?.cancel()
                        stopScan()
                        savedPrefixes?.let { allowedPrefixes = it }
                        savedPrefixes = null
                        _locateHitDevice.value = hit
                        return@launch
                    }
                }
                // 驻留/列表冻结规则（R-32）：列表仅在扫描进行中刷新
                if (_isScanning.value) {
                    devices.forEach { device ->
                        if (loggedDiscoveries.add(device.identifier)) {
                            LogStore.info(if (HktLang.isZh) "发现设备 ${device.name} ${device.rssi}dBm" else "Discovered ${device.name} ${device.rssi}dBm")
                        }
                    }
                    _devices.value = devices
                    val prefix = demoAutoConnectPrefix
                    if (prefix != null) {
                        val auto = devices.firstOrNull { it.name.startsWith(prefix) }
                        if (auto != null) {
                            demoAutoConnectPrefix = null
                            stopScan()
                            makeAndStartSession(auto)
                            _requestShowDetail.value = true
                            onSessionStarted?.invoke(_activeSession.value ?: return@launch)
                        }
                    }
                }
            }
        }
    }

    fun stopScan() {
        if (!_isScanning.value) return
        _isScanning.value = false
        scanTicker?.cancel()
        scanTicker = null
        port.stopScan()
        LogStore.info(if (HktLang.isZh) "扫描停止" else "Scan stopped")
    }

    // MARK: - R-2 目标设备定位（扫码 16 位 DevEUI → 后 6 位匹配）

    fun startLocate(devEUI: String) {
        val cleaned = devEUI.trim().uppercase()
        if (cleaned.length != 16 || cleaned.any { !it.isHexish() }) return
        _locateSuffix.value = cleaned.takeLast(6)
        _locatePhase.value = LocatePhase.FINDING
        savedPrefixes = allowedPrefixes
        allowedPrefixes = DiscoveredDevice.supportedPrefixes   // 目标模式忽略前缀过滤
        if (_isScanning.value) stopScan()
        startScan()
        armLocateTimeout()
    }

    fun retryLocate() {
        if (_locateSuffix.value == null) return
        _locatePhase.value = LocatePhase.FINDING
        startScan()
        armLocateTimeout()
    }

    fun cancelLocate() {
        locateTimeoutJob?.cancel()
        if (_locatePhase.value == LocatePhase.FINDING) stopScan()
        _locateSuffix.value = null
        _locatePhase.value = null
        savedPrefixes?.let { allowedPrefixes = it }
        savedPrefixes = null
    }

    private fun armLocateTimeout() {
        locateTimeoutJob?.cancel()
        locateTimeoutJob = scope.launch {
            delay(30_000)   // 3 轮 × 10s（与扫描节奏一致）
            if (_locatePhase.value == LocatePhase.FINDING) {
                _locatePhase.value = LocatePhase.NOT_FOUND
                stopScan()
            }
        }
    }

    // MARK: - 会话

    /** 连接成功后的设备会话（家族判定 + 已连接链路）。 */
    fun makeSession(forDevice: DiscoveredDevice): DeviceSession? {
        val family: DeviceFamily = DeviceRegistry.matchBroadcast(forDevice.name)?.family ?: return null
        val link = port.makeLink(forDevice) ?: return null
        return DeviceSession(
            family = family,
            deviceName = forDevice.name,
            link = link,
            scope = scope,
            nowMs = nowMs,
            epochSeconds = { nowMs() / 1000 },
        )
    }

    /** 连接成功后建会话并启动 1s 轮询。 */
    fun makeAndStartSession(forDevice: DiscoveredDevice): DeviceSession? {
        if (_activeSession.value != null) return _activeSession.value
        val session = makeSession(forDevice) ?: return null
        _activeSession.value = session
        // 先通知（演示应答器需要 family 才回夹具帧）再启动轮询——首帧 0xFF 就要有应答。
        // ⚠️ 触发时 _residentDevice 尚未置位（其后一行才置）——回调只应取 family，不得读驻留（M6.1 评审 P3）
        onSessionStarted?.invoke(session)
        session.start()
        _residentDevice.value = ResidentDevice(forDevice.name, forDevice.identifier)
        return session
    }

    /** R-31 断开连接：停轮询 + GATT 断开 + 清驻留（设备进最近设备）。 */
    fun disconnectActive() {
        // 双调用路径（对话框确认/连接前清理）会让本函数走到两次——仅首次落日志（真机日志双行实证）
        if (_activeSession.value != null) {
            LogStore.info(if (HktLang.isZh) "手动断开（预期断开，不自动重连）" else "Manual disconnect (expected, no auto-reconnect)")
        }
        _residentDevice.value?.let { _lastSession.value = it }
        _activeSession.value?.stop()
        _activeSession.value = null
        port.disconnectDevice()
        _residentDevice.value = null
    }

    /** P-01 断线态「重新连接」：驻留设备原身份重建会话。
     *  ⚠️ linkLost 时死会话仍占位——须先原子释放（预期断开）再重连，否则本函数是空操作
     *  （M6.2 评审 P1-3；iOS ScanModel.swift 同病，另行提请两端同步）。 */
    fun reconnectResident(): DeviceSession? {
        val resident = _residentDevice.value ?: return null
        if (_activeSession.value != null) disconnectActive()
        val device = _devices.value.firstOrNull { it.identifier == resident.identifier }
            ?: DiscoveredDevice(resident.name, resident.identifier, Int.MIN_VALUE)
        return makeAndStartSession(device)
    }

    // MARK: - P-01 首页卡片点击语义（R-31/R-32，原型 deviceCardClick）

    enum class CardTapOutcome {
        /** 验活通过：同一设备直接回详情 */
        SHOW_DETAIL,
        /** 其他设备 → 切换确认框 */
        CONFIRM_SWITCH,
        /** 标准连接（连接覆盖层 P-02 于 M6 接入） */
        CONNECT,
    }

    /** 点击目标解析（UI 据此分流；[CardTapOutcome.CONNECT] 携带目标设备）。 */
    fun cardTapOutcome(forDevice: DiscoveredDevice): Pair<CardTapOutcome, DiscoveredDevice> {
        val session = _activeSession.value
        val resident = _residentDevice.value
        if (session != null && !session.linkLost.value && resident != null) {
            if (forDevice.identifier == resident.identifier) {
                return CardTapOutcome.SHOW_DETAIL to forDevice
            }
            return CardTapOutcome.CONFIRM_SWITCH to forDevice
        }
        if (_activeSession.value != null) disconnectActive()   // 失联兜底：原子释放旧会话
        stopScan()                                      // 安卓同款：连接前先停扫
        return CardTapOutcome.CONNECT to forDevice
    }

    /** R-32 确认切换：原子释放当前会话（预期断开）→ 标准连接新设备。 */
    fun confirmSwitch(): DiscoveredDevice? {
        val target = _pendingSwitch.value ?: return null
        _pendingSwitch.value = null
        disconnectActive()
        stopScan()
        return target
    }

    fun cancelSwitch() {
        _pendingSwitch.value = null
    }

    /** 为指定设备创建连接过程模型（P-02；端口与扫描同源）。 */
    fun connectModel(forDevice: DiscoveredDevice): ConnectModel =
        ConnectModel(target = forDevice, port = port, scope = scope)

    fun requestSwitch(toDevice: DiscoveredDevice) {
        _pendingSwitch.value = toDevice
    }

    /** 最近设备一键连接（空列表时的 recent 卡）。 */
    fun targetForLastSession(): DiscoveredDevice? {
        val last = _lastSession.value ?: return null
        val device = _devices.value.firstOrNull { it.identifier == last.identifier }
            ?: DiscoveredDevice(last.name, last.identifier, Int.MIN_VALUE)
        stopScan()
        return device
    }

    companion object {
        const val SCAN_ROUND_SECONDS = 10

    }
}

private fun Char.isHexish(): Boolean =
    this in '0'..'9' || this in 'A'..'F' || this in 'a'..'f'
