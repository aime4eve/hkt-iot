package com.hkt.ble.bletools.harness

import com.hkt.ble.bletools.core.ble.BLEAvailability
import com.hkt.ble.bletools.core.ble.BluetoothPort
import com.hkt.ble.bletools.core.ble.ConnectEvent
import com.hkt.ble.bletools.core.ble.DeviceSession
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.core.ble.PeripheralLink
import com.hkt.ble.bletools.core.ble.ScanOptions
import com.hkt.ble.bletools.core.device.DeviceRegistry
import com.hkt.ble.bletools.core.ota.OtaEngine
import com.hkt.ble.bletools.core.protocol.CommandCode
import com.hkt.ble.bletools.core.protocol.DeviceFamily
import com.hkt.ble.bletools.core.protocol.DeviceSnapshot
import com.hkt.ble.bletools.core.protocol.HKTFrameEncoder
import kotlinx.coroutines.CoroutineScope
import kotlin.coroutines.CoroutineContext
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import kotlinx.serialization.json.Json
import kotlinx.serialization.json.JsonObject
import kotlinx.serialization.json.int
import kotlinx.serialization.json.jsonPrimitive
import kotlinx.serialization.json.long
import java.io.BufferedReader
import java.io.BufferedWriter
import java.net.InetAddress
import java.net.Socket
import java.util.concurrent.Executors
import kotlin.concurrent.thread

/**
 * M7 真机验证 harness（Mac 蓝牙桥路线）：经 tools/macble-bridge（CoreBluetooth ↔ TCP 行 JSON）
 * 驱动安卓 v2 的**真协议栈**（core-protocol/core-ble/core-ota，零 Android 依赖）直连真机。
 * 模拟器无蓝牙硬件，UI 已在 M6.3 模拟器验证；本 harness 验证 M7 的独有风险——真实固件字节级兼容。
 *
 * 命令：
 *   scan                     扫描 8s，打印入列设备（名称/标识/RSSI）
 *   verify <名称前缀>        连接→6 轮询真值打印→配置同值回写（ACK）→UDS 对时→断开
 *   ota <名称前缀> <bin路径>  三重防线校验→真包 OTA 传输→重连读版本
 */
fun main(args: Array<String>) {
    run { runBlocking { main2(args) } }
    kotlin.system.exitProcess(0)   // 协程执行器非等待：显式退出（守护线程兜底）
}

private fun main2(args: Array<String>) = runBlocking {
    val cmd = args.getOrNull(0) ?: error("用法: scan | verify <前缀> | ota <前缀> <bin>")
    val bridge = BridgeClient()
    bridge.connect()
    val port = MacBridgePort(bridge)
    val mainCtx = Executors.newSingleThreadExecutor { r ->
        Thread(r, "harness-main").apply { isDaemon = true }
    }.asCoroutineDispatcher()
    val scope = CoroutineScope(mainCtx)
    when (cmd) {
        "scan" -> {
            val devices = port.scanOnce(8_000)
            println("发现 ${devices.size} 台入列设备：")
            devices.forEach { println("  ${it.name}  id=${it.identifier}  ${it.rssi} dBm") }
        }
        "verify" -> {
            val prefix = args.getOrNull(1) ?: error("缺名称前缀")
            verify(port, scope, mainCtx, prefix)
        }
        "ota" -> {
            val prefix = args.getOrNull(1) ?: error("缺名称前缀")
            val bin = args.getOrNull(2) ?: error("缺固件路径")
            ota(port, scope, mainCtx, prefix, java.io.File(bin).readBytes(), java.io.File(bin).name)
        }
        else -> error("未知命令 $cmd")
    }
    bridge.close()
}

// MARK: - 逐家族验证

/** 写后读回闭环：UDS 比对上报周期（越界现值合法化后应生效）。 */
private fun verifyRoundTrip(session: DeviceSession) {
    val s = session.snapshot.value
    if (session.family == DeviceFamily.UDS100) {
        val expect = legalUdsReport(s.reportPeriodMin)
        println(if (s.reportPeriodMin == expect) "✓ 读回闭环：上报周期=$expect min 与写入一致" else "✕ 读回不一致：上报周期=${s.reportPeriodMin}（期望 $expect）")
    }
}

private suspend fun verify(port: MacBridgePort, scope: CoroutineScope, mainCtx: CoroutineContext, prefix: String) {
    val device = port.scanAndPick(prefix) ?: return
    println("== 连接 ${device.name} ==")
    val link = port.connectAndWait(device) ?: run { println("✕ 连接失败"); return }
    val family = DeviceRegistry.matchBroadcast(device.name)?.family ?: run { println("✕ 家族未识别"); return }
    val session = DeviceSession(
        family = family, deviceName = device.name, link = link,
        scope = scope, nowMs = System::currentTimeMillis, epochSeconds = { System.currentTimeMillis() / 1000 },
    )
    withContext(mainCtx) { session.start() }
    println("等 6 轮询积累…")
    delay(6_500)
    withContext(mainCtx) {
        printSnapshot(session)
        // 配置同值回写（0x02）：从当前快照取值，非破坏性往返
        // FD-004：写窗口暂停 1s 轮询（设备单 RX 缓冲，与 App 端 ConfigScreen 同款防护）。
        // 9-15 排查结论：固件 50ms 空闲判帧 + 轮询阻塞处理数百 ms——suspend 后须等最后一条
        // 在途轮询处理完（≥1s）再写，否则写帧拼进处理中的缓冲被整段丢弃。
        session.setPollingSuspended(true)
        delay(1_200)
        val acked = roundTripConfig(session)
        session.setPollingSuspended(false)
        println(if (acked) "✓ 0x02 配置写入 ACK" else "✕ 0x02 无 ACK")
        if (acked) {
            delay(2_500)   // 等 2 轮询让设备回读值上屏
            withContext(mainCtx) { verifyRoundTrip(session) }
        }
        if (family == DeviceFamily.UDS100) {
            println("UDS 对时: " + session.sendTimeSync())
        }
    }
    withContext(mainCtx) { session.stop() }
    port.disconnectDevice()
    println("== ${device.name} 验证完成 ==\n")
}

/** 固件规则：UDS 周期 0 或 10–1440。设备现值若越界（如 3min）取最近合法值（写回原值会被静默拒绝）。 */
private fun legalUdsReport(v: Int?) = if (v == null || (v != 0 && v < 10)) 10 else v

private suspend fun roundTripConfig(session: DeviceSession): Boolean {
    val s = session.snapshot.value
    return when (session.family) {
        DeviceFamily.UDS100 -> session.sendWrite(
            CommandCode.CONFIG,
            HKTFrameEncoder.udsConfigPayload(legalUdsReport(s.reportPeriodMin), s.gpsPeriodMin ?: 0, s.lowThresholdMM ?: 0, s.highThresholdMM ?: 0),
        )
        DeviceFamily.DC200_FAMILY -> session.sendWrite(
            CommandCode.CONFIG,
            HKTFrameEncoder.dcConfigPayload(s.reportPeriodMin ?: 0, s.parkMode ?: 0),
        )
        DeviceFamily.SVC100 -> session.sendWrite(
            CommandCode.CONFIG,
            HKTFrameEncoder.svcConfigPayload(s.voltageLevel ?: 0, s.portFunction ?: 0, s.stableTimeS ?: 0, s.smartPower ?: 0, s.timezone ?: 0, s.reportPeriodMin ?: 0),
        )
    }
}

private fun printSnapshot(session: DeviceSession) {
    val s = session.snapshot.value
    println("-- 快照（轮询真值）--")
    println("   firmware v${s.hardwareVersion}.${s.softwareVersion}  power=${s.power}  battery=" +
        (if (session.family == DeviceFamily.UDS100) "${s.batteryVoltageMV}mV" else "${s.batteryPercent}%"))
    when (session.family) {
        DeviceFamily.UDS100 -> {
            println("   温度=${s.temperatureMilli}m°C 湿度=${s.humidityMilli}m% 距离=${s.distanceMM}mm 满溢=${s.overflowState}")
            println("   低阈值=${s.lowThresholdMM} 高阈值=${s.highThresholdMM} 倾角=${s.angleCenti}c 倾斜=${s.slant} HT告警=${s.htAlarm}")
            println("   GPS周期=${s.gpsPeriodMin}min 上报周期=${s.reportPeriodMin}min 纬度=${s.latitude} 经度=${s.longitude}")
        }
        DeviceFamily.DC200_FAMILY -> {
            println("   车位=${s.parkState} 工作模式=${s.parkMode} 防拆=${s.tamper} 上报周期=${s.reportPeriodMin}min")
            println("   地磁 X=${s.magX} Y=${s.magY} Z=${s.magZ} 雷达=${s.radarSpectrum}")
        }
        DeviceFamily.SVC100 -> {
            println("   阀1=${s.valve1State}(插入${s.valve1Inserted}/脉冲${s.valve1Pulse}) 阀2=${s.valve2State}(插入${s.valve2Inserted}/脉冲${s.valve2Pulse})")
            println("   电压档=${s.voltageLevel} 端口=${s.portFunction} 稳定=${s.stableTimeS}s 自动开关机=${s.smartPower} 时区=${s.timezone} 周期=${s.reportPeriodMin}min")
        }
    }
    println("   unknownTail=${session.unknownTail.value} linkLost=${session.linkLost.value} 距上次应答=${session.secondsSinceLastResponse.value}s")
}

// MARK: - OTA 真包传输

private suspend fun ota(port: MacBridgePort, scope: CoroutineScope, mainCtx: CoroutineContext, prefix: String, image: ByteArray, imageName: String) {
    // 三重防线（与 app adoptFirmware 同规）：零字节 / 大小 / 栈顶指针 / 复位向量
    check(image.isNotEmpty()) { "固件文件为空（0 字节），不允许升级" }
    check(image.size in 8192..245_760) { "固件大小 ${image.size} B 超出 8KB–240KB" }
    fun u32le(o: Int): Long = (image[o].toLong() and 0xFF) or ((image[o + 1].toLong() and 0xFF) shl 8) or
        ((image[o + 2].toLong() and 0xFF) shl 16) or ((image[o + 3].toLong() and 0xFF) shl 24)
    check(u32le(0) and 0xFFFE_0000L == 0x2000_0000L) { "栈顶指针不在 RAM（不是合法 .bin 镜像）" }
    val reset = u32le(4) and 0xFFFF_FFFEL
    check(reset in 0x0800_4000L until 0x0804_0000L) { "复位向量不在 App 区" }
    println("镜像校验通过：$imageName ${image.size} B")

    val device = port.scanAndPick(prefix) ?: return
    val link = port.connectAndWait(device) ?: return
    val family = DeviceRegistry.matchBroadcast(device.name)?.family ?: return
    val session = DeviceSession(
        family = family, deviceName = device.name, link = link,
        scope = scope, nowMs = System::currentTimeMillis, epochSeconds = { System.currentTimeMillis() / 1000 },
    )
    withContext(mainCtx) { session.start() }
    delay(3_000)
    val before = session.snapshot.value
    println("升级前版本 v${before.hardwareVersion}.${before.softwareVersion}")

    val engine = OtaEngine(image, scope = scope)
    withContext(mainCtx) {
        session.setPollingSuspended(true)
        session.rawFrameHandler = { frame -> engine.handle(frame) }
        engine.bind { session.sendRaw(it) }
    }
    var lastPkt = -1
    val terminal = kotlinx.coroutines.CompletableDeferred<String>()
    val collector = scope.launch {
        engine.state.collect { state ->
            val pkt = engine.packetsDone.value
            if (pkt != lastPkt && pkt % 64 == 0) {
                println("  包 $pkt / ${engine.packetCount}")
                lastPkt = pkt
            }
            when (state) {
                is OtaEngine.State.Done -> {
                    println("✓ 传输完成（${engine.packetCount} 包，复位重传 ${engine.restarts.value} 次），finish 帧已发")
                    withContext(mainCtx) {
                        session.rawFrameHandler = null
                        session.setPollingSuspended(false)
                        session.stop()
                    }
                    port.disconnectDevice()
                    terminal.complete("done")
                }
                is OtaEngine.State.Failed -> {
                    println("✕ OTA 失败: ${state.error}（复位重传 ${engine.restarts.value} 次）")
                    withContext(mainCtx) { session.setPollingSuspended(false) }
                    terminal.complete("failed:${state.error}")
                }
                else -> {}
            }
        }
    }
    withContext(mainCtx) { engine.begin() }
    val outcome = terminal.await()
    collector.cancel()
    if (outcome != "done") return

    println("等设备重启广播…")
    delay(12_000)
    // 重连读版本
    val link2 = port.connectAndWait(device) ?: run { println("✕ 重连失败（设备可能仍在重启）"); return }
    val session2 = DeviceSession(
        family = family, deviceName = device.name, link = link2,
        scope = scope, nowMs = System::currentTimeMillis, epochSeconds = { System.currentTimeMillis() / 1000 },
    )
    withContext(mainCtx) { session2.start() }
    delay(5_000)
    val after = session2.snapshot.value
    println("升级后版本 v${after.hardwareVersion}.${after.softwareVersion}（升级前 v${before.hardwareVersion}.${before.softwareVersion}）")
    withContext(mainCtx) { session2.stop() }
    port.disconnectDevice()
}

// MARK: - MacBridgePort（BluetoothPort over TCP 桥）

private class MacBridgePort(private val bridge: BridgeClient) : BluetoothPort {
    private val devices = LinkedHashMap<String, DiscoveredDevice>()
    private var scanUpdate: ((BLEAvailability, List<DiscoveredDevice>) -> Unit)? = null
    private var scanOptions = ScanOptions()
    private var connectEvents: ((ConnectEvent) -> Unit)? = null
    private var failure: ((String) -> Unit)? = null
    private var activeLink: MacLink? = null
    private var job: kotlinx.coroutines.Job? = null

    init {
        job = CoroutineScope(
            Executors.newSingleThreadExecutor { r ->
                Thread(r, "bridge-events").apply { isDaemon = true }
            }.asCoroutineDispatcher(),
        ).launch {
            bridge.events.collect { ev ->
                when (ev["ev"]?.jsonPrimitive?.content) {
                    "scan" -> {
                        val name = ev["name"]?.jsonPrimitive?.content ?: return@collect
                        val id = ev["id"]?.jsonPrimitive?.content ?: return@collect
                        val rssi = ev["rssi"]?.jsonPrimitive?.int
                        val old = devices[id]
                        if (old == null || (rssi != null && rssi > old.rssi)) {
                            devices[id] = DiscoveredDevice(name, id, rssi ?: Int.MIN_VALUE)
                        }
                        publish()
                    }
                    "ready" -> {
                        connectEvents?.invoke(ConnectEvent.LinkEstablished)
                        connectEvents?.invoke(ConnectEvent.ServicesDiscovered)
                        connectEvents?.invoke(ConnectEvent.NotificationsEnabled)
                    }
                    "connectFailed" -> {
                        failure?.invoke(ev["reason"]?.jsonPrimitive?.content ?: "unknown")
                    }
                    "rx" -> {
                        val hex = ev["hex"]?.jsonPrimitive?.content ?: return@collect
                        activeLink?.onReceive?.invoke(hexToBytes(hex))
                    }
                    "disc" -> {
                        activeLink?.onDisconnected?.invoke()
                    }
                }
            }
        }
    }

    private fun publish() {
        val list = devices.values
            .filter { scanOptions.isListable(it.name, it.rssi) }
            .sortedByDescending { it.rssi }
        scanUpdate?.invoke(BLEAvailability.READY, list)
    }

    /** 扫描 [timeoutMs] 后返回入列设备（供 harness 命令行直接取用）。 */
    suspend fun scanOnce(timeoutMs: Long): List<DiscoveredDevice> {
        var latest: List<DiscoveredDevice> = emptyList()
        startScan(ScanOptions()) { _, list -> latest = list }
        delay(timeoutMs)
        stopScan()
        return latest
    }

    /** 扫描直至命中名称前缀设备（复用入列规则）。 */
    suspend fun scanAndPick(prefix: String): DiscoveredDevice? {
        println("扫描寻找 $prefix…")
        var hit: DiscoveredDevice? = null
        startScan(ScanOptions()) { _, list -> hit = list.firstOrNull { it.name.startsWith(prefix) } ?: hit }
        val deadline = System.currentTimeMillis() + 15_000
        while (hit == null && System.currentTimeMillis() < deadline) delay(300)
        stopScan()
        return hit ?: run { println("✕ 15s 内未发现 $prefix 设备"); null }
    }

    /** 连接并等 indicate 订阅完成（三阶段事件齐）。 */
    suspend fun connectAndWait(device: DiscoveredDevice): PeripheralLink? {
        var ready = false
        var reason: String? = null
        // 先建链再连接：ready 事件可能先于 makeLink 到达（桥时序不受控）
        val link = MacLink(bridge)
        activeLink = link
        connectEvents = { if (it == ConnectEvent.NotificationsEnabled) ready = true }
        failure = { reason = it }
        connect(device, connectEvents!!)
        val deadline = System.currentTimeMillis() + 15_000
        while (!ready && reason == null && System.currentTimeMillis() < deadline) delay(200)
        if (!ready) {
            println("✕ 连接失败: " + (reason ?: "超时"))
            return null
        }
        return link
    }

    override fun activate(onUpdate: (BLEAvailability) -> Unit) {
        onUpdate(BLEAvailability.READY)
    }

    override fun startScan(options: ScanOptions, onUpdate: (BLEAvailability, List<DiscoveredDevice>) -> Unit) {
        scanOptions = options
        scanUpdate = onUpdate
        devices.clear()
        bridge.send(JsonObject(mapOf("cmd" to kotlinx.serialization.json.JsonPrimitive("scan"))))
    }

    override fun stopScan() {
        bridge.send(JsonObject(mapOf("cmd" to kotlinx.serialization.json.JsonPrimitive("scanStop"))))
    }

    override fun connect(device: DiscoveredDevice, events: (ConnectEvent) -> Unit) {
        connectEvents = events
        bridge.send(JsonObject(mapOf("cmd" to kotlinx.serialization.json.JsonPrimitive("connect"), "id" to kotlinx.serialization.json.JsonPrimitive(device.identifier))))
    }

    override fun cancelConnect() {
        bridge.send(JsonObject(mapOf("cmd" to kotlinx.serialization.json.JsonPrimitive("disconnect"))))
    }

    override fun disconnectDevice() {
        bridge.send(JsonObject(mapOf("cmd" to kotlinx.serialization.json.JsonPrimitive("disconnect"))))
        activeLink = null
    }

    override fun makeLink(forDevice: DiscoveredDevice): PeripheralLink? {
        val link = MacLink(bridge)
        activeLink = link
        return link
    }
}

private class MacLink(private val bridge: BridgeClient) : PeripheralLink {
    override var onReceive: ((ByteArray) -> Unit)? = null
    override var onDisconnected: (() -> Unit)? = null

    override fun send(frame: ByteArray) {
        bridge.send(
            JsonObject(
                mapOf(
                    "cmd" to kotlinx.serialization.json.JsonPrimitive("write"),
                    "hex" to kotlinx.serialization.json.JsonPrimitive(frame.joinToString("") { "%02X".format(it) }),
                ),
            ),
        )
    }
}

// MARK: - 桥 TCP 客户端

private class BridgeClient(private val host: String = "127.0.0.1", private val port: Int = 9876) {
    private val json = Json
    private lateinit var socket: Socket
    private lateinit var writer: BufferedWriter
    val events = MutableSharedFlow<JsonObject>(extraBufferCapacity = 512)

    fun connect() {
        socket = Socket(InetAddress.getByName(host), port)
        writer = socket.getOutputStream().bufferedWriter()
        thread(isDaemon = true, name = "bridge-reader") {
            runCatching {
                val reader: BufferedReader = socket.getInputStream().bufferedReader()
                while (true) {
                    val line = reader.readLine() ?: break
                    runCatching {
                        val obj = json.parseToJsonElement(line)
                        if (obj is JsonObject) events.tryEmit(obj)
                    }
                }
            }   // close() 时 SocketException 正常退出
        }
    }

    fun send(obj: JsonObject) {
        synchronized(writer) {
            writer.write(obj.toString())
            writer.newLine()
            writer.flush()
        }
    }

    fun close() {
        runCatching { socket.close() }
    }
}

private fun hexToBytes(hex: String): ByteArray =
    ByteArray(hex.length / 2) { i -> hex.substring(i * 2, i * 2 + 2).toInt(16).toByte() }
