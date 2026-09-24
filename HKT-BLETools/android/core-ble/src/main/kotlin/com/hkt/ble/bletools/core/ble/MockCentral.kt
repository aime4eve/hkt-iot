package com.hkt.ble.bletools.core.ble

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

/**
 * 可编程假蓝牙源（SD 架构测试缝的演示/测试侧）：与 SystemCentral 共用 [BluetoothPort]，
 * 自身兼任 [PeripheralLink]（MockCentral.connect 扩展模式，iOS MockConnect 同构）。
 * DemoResponder（App 层）挂进 [responder] 即得 -mockble 演示模式。
 */
class MockCentral(private val scope: CoroutineScope) : BluetoothPort, PeripheralLink {

    /** 连接脚本：Success=按延迟推进三阶段事件；Never=永不推进（弱信号失败分支）。 */
    sealed class ConnectScript {
        data class Success(val delayMs: Long = 0) : ConnectScript()
        data object Never : ConnectScript()
    }

    private var availability: BLEAvailability = BLEAvailability.READY
    private var availabilityCallback: ((BLEAvailability) -> Unit)? = null
    private var scanCallback: ((BLEAvailability, List<DiscoveredDevice>) -> Unit)? = null
    private var scanOptions: ScanOptions = ScanOptions()
    private val scripted = LinkedHashMap<String, DiscoveredDevice>()
    private var connectedDevice: DiscoveredDevice? = null
    private var connectJob: Job? = null

    /** 演示应答器（DemoResponder）：按请求帧返回响应帧。 */
    var responder: ((ByteArray) -> ByteArray?)? = null

    /** 连接脚本（默认成功）。 */
    var connectScript: ConnectScript = ConnectScript.Success(0)

    // ---- PeripheralLink（连接成功后由 makeLink 交出自身） ----
    override var onReceive: ((ByteArray) -> Unit)? = null
    override var onDisconnected: (() -> Unit)? = null

    private val sent = ArrayList<ByteArray>()

    // ---- BluetoothPort ----

    override fun activate(onUpdate: (BLEAvailability) -> Unit) {
        availabilityCallback = onUpdate
        onUpdate(availability)
    }

    override fun startScan(options: ScanOptions, onUpdate: (BLEAvailability, List<DiscoveredDevice>) -> Unit) {
        scanOptions = options
        scanCallback = onUpdate
        onUpdate(availability, listableDevices())
    }

    override fun stopScan() {
        scanCallback = null
    }

    override fun connect(device: DiscoveredDevice, events: (ConnectEvent) -> Unit) {
        cancelConnect()
        val script = connectScript
        if (script is ConnectScript.Never) return   // 永不推进（测试弱信号失败）
        connectedDevice = device
        connectJob = scope.launch {
            if (script is ConnectScript.Success && script.delayMs > 0) delay(script.delayMs)
            events(ConnectEvent.LinkEstablished)
            events(ConnectEvent.ServicesDiscovered)
            events(ConnectEvent.NotificationsEnabled)
        }
    }

    override fun cancelConnect() {
        connectJob?.cancel()
        connectJob = null
        connectedDevice = null
    }

    override fun disconnectDevice() {
        connectedDevice = null
    }

    override fun makeLink(forDevice: DiscoveredDevice): PeripheralLink? =
        when {
            // 已连接同一设备 → 链路
            connectedDevice == forDevice -> this
            // 未连接（demo 直连流 demoAutoConnect 不经 connect()）→ 隐式连接取链路
            connectedDevice == null -> {
                connectedDevice = forDevice
                this
            }
            else -> null
        }

    // ---- 场景注入 API ----

    /** 注入/更新能力状态（MOCK_AVAILABILITY 演示四态门）。 */
    fun setAvailability(availability: BLEAvailability) {
        this.availability = availability
        availabilityCallback?.invoke(availability)
        scanCallback?.let { it(availability, listableDevices()) }
    }

    /** 投放/更新脚本设备并重新按入列规则广播（渐次入列 = 分次调用）。 */
    fun discover(device: DiscoveredDevice) {
        scripted[device.identifier] = device
        scanCallback?.let { it(availability, listableDevices()) }
    }

    fun clearDevices() {
        scripted.clear()
        scanCallback?.let { it(availability, emptyList()) }
    }

    private fun listableDevices(): List<DiscoveredDevice> =
        scripted.values
            .filter { scanOptions.isListable(it.name, it.rssi) }
            .sortedByDescending { it.rssi }

    // ---- PeripheralLink（会话收发） ----

    override fun send(frame: ByteArray) {
        synchronized(sent) { sent.add(frame) }
        responder?.invoke(frame)?.let { response -> onReceive?.invoke(response) }
    }

    /** 场景注入：模拟对端断开。 */
    fun simulateDisconnect() {
        onDisconnected?.invoke()
    }

    /** 场景注入：直接投递一条设备帧（不经 responder）。 */
    fun inject(data: ByteArray) {
        onReceive?.invoke(data)
    }

    /** 已发帧（演示/测试断言用）。 */
    fun framesSentSnapshot(): List<ByteArray> = synchronized(sent) { sent.toList() }
}
