package com.hkt.ble.bletools.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.content.Context
import android.os.Build
import com.hkt.ble.bletools.core.ble.BLEAvailability
import com.hkt.ble.bletools.core.ble.ConnectEvent
import com.hkt.ble.bletools.core.ble.ConnectFailure
import com.hkt.ble.bletools.core.ble.DiscoveredDevice
import com.hkt.ble.bletools.core.ble.HKTProfile
import com.hkt.ble.bletools.core.ble.PeripheralLink
import com.hkt.ble.bletools.core.ble.ScanOptions
import com.hkt.ble.bletools.model.LogStore
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanResult
import no.nordicsemi.android.support.v18.scanner.ScanSettings

/**
 * 真蓝牙实现（SystemCentral 的 Kotlin 移植）：BluetoothPort + PeripheralLink 双角色。
 *
 * 连接序（b21 定稿，V3.21 现网语义 + 本机队列防御）：链路一通 → 立即发现服务
 * （Google BLE 指南/iOS 同序）→ MTU 512 落定 → 订阅 Indicate（就绪）；
 * 写特征无响应优先；回调统一投递主线程（BluetoothPort 契约）。
 *
 * CoreBluetooth「同会话对同设备只报一次 didDiscover」的坑在安卓 Nordic scanner 不存在
 * （SCAN_CALLBACK_TYPE_UPDATED 会重复上报），但 startLocate 仍按先 stop 再 start 重开会话，
 * 与 iOS 语义保持一致（M7 真机核对项）。
 */
@SuppressLint("MissingPermission")
class SystemCentral(private val context: Context) :
    com.hkt.ble.bletools.core.ble.BluetoothPort, PeripheralLink {

    private val mainHandler = android.os.Handler(android.os.Looper.getMainLooper())
    private fun onMain(block: () -> Unit) {
        if (android.os.Looper.myLooper() == android.os.Looper.getMainLooper()) block() else mainHandler.post(block)
    }

    private val adapter: BluetoothAdapter?
        get() = (context.getSystemService(Context.BLUETOOTH_SERVICE) as? BluetoothManager)?.adapter

    // ---- 扫描状态 ----
    private var availabilityCallback: ((BLEAvailability) -> Unit)? = null
    private var scanCallback: ((BLEAvailability, List<DiscoveredDevice>) -> Unit)? = null
    private var scanOptions = ScanOptions()
    private val discovered = LinkedHashMap<String, DiscoveredDevice>()

    // ---- 连接状态 ----
    private var gatt: BluetoothGatt? = null
    private var pendingConnectDevice: DiscoveredDevice? = null
    private var connectEvents: ((ConnectEvent) -> Unit)? = null
    private var connectedDevice: DiscoveredDevice? = null
    private var indicateCharacteristic: BluetoothGattCharacteristic? = null
    private var writeCharacteristic: BluetoothGattCharacteristic? = null
    /** 服务发现已受理（discoverServices 返回 true）；false=操作排队被拒，允许补发。 */
    private var discoveryIssued = false
    /** 订阅已受理（writeDescriptor 返回 true）；false=MTU 在途被拒，允许补发。 */
    private var subscribeIssued = false
    /** 收帧计数（遥测：首 5 帧逐帧记，之后每 50 帧记一次——区分「无上行」与「有上行」）。 */
    private var receivedFrames = 0

    // ---- PeripheralLink 对外回调 ----
    override var onReceive: ((ByteArray) -> Unit)? = null
    override var onDisconnected: (() -> Unit)? = null

    // ---- BluetoothPort: activate ----

    override fun activate(onUpdate: (BLEAvailability) -> Unit) {
        availabilityCallback = onUpdate
        val state = when {
            adapter == null -> BLEAvailability.UNSUPPORTED
            adapter?.isEnabled == true -> BLEAvailability.READY
            else -> BLEAvailability.POWERED_OFF
        }
        onUpdate(state)
    }

    // ---- BluetoothPort: scan ----

    private val scannerCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val name = result.scanRecord?.deviceName ?: result.device.name
            val rssi = result.rssi
            val id = result.device.address
            if (!scanOptions.isListable(name, rssi)) return
            discovered[id] = DiscoveredDevice(name ?: "", id, rssi)
            scanCallback?.invoke(BLEAvailability.READY, discovered.values.toList())
        }
    }

    override fun startScan(options: ScanOptions, onUpdate: (BLEAvailability, List<DiscoveredDevice>) -> Unit) {
        scanOptions = options
        scanCallback = onUpdate
        discovered.clear()   // 新会话=全新发现（换环境扫描修复同源语义，40eb235）
        val a = adapter
        if (a == null || !a.isEnabled) {
            onUpdate(BLEAvailability.POWERED_OFF, emptyList())
            return
        }
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .setLegacy(false)
            .build()
        BluetoothLeScannerCompat.getScanner().startScan(null, settings, scannerCallback)
    }

    override fun stopScan() {
        runCatching { BluetoothLeScannerCompat.getScanner().stopScan(scannerCallback) }
        // 能力状态回调保留（R-32 驻留健康检测依赖）
    }

    // ---- BluetoothPort: connect ----

    override fun connect(device: DiscoveredDevice, events: (ConnectEvent) -> Unit) {
        cancelConnect()
        connectEvents = events
        pendingConnectDevice = device
        val a = adapter ?: run { events(ConnectEvent.Failed(ConnectFailure.CONNECTION_LOST)); return }
        val remote: BluetoothDevice = a.getRemoteDevice(device.identifier)
        // 现网同款：连接前先停扫描（macOS/安卓同经验，边扫边连干扰 GATT 建立）
        stopScan()
        LogStore.info("GATT 连接发起 ${device.name} (${device.identifier})")
        discoveryIssued = false
        subscribeIssued = false
        receivedFrames = 0
        val g = try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                remote.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
            } else {
                remote.connectGatt(context, false, gattCallback)
            }
        } catch (e: SecurityException) {
            LogStore.error("GATT 连接权限异常: $e")
            events(ConnectEvent.Failed(ConnectFailure.CONNECTION_LOST))
            return
        }
        gatt = g
    }

    override fun cancelConnect() {
        connectEvents = null
        pendingConnectDevice = null
        runCatching { gatt?.disconnect(); gatt?.close() }
        gatt = null
    }

    override fun disconnectDevice() {
        runCatching { gatt?.disconnect(); gatt?.close() }
        gatt = null
        connectedDevice = null
        indicateCharacteristic = null
        writeCharacteristic = null
    }

    override fun makeLink(forDevice: DiscoveredDevice): PeripheralLink? =
        if (connectedDevice == forDevice) this else null

    // ---- PeripheralLink ----

    override fun send(frame: ByteArray) {
        val g = gatt ?: return
        val ch = writeCharacteristic ?: return
        // iOS 对位（canSendWriteWithoutResponse 优先）：特征支持则用无响应写——
        // 透明桥模组按 write-command 收帧最稳，也免占客户端操作队列等写回执
        val noResponse =
            ch.properties and BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE != 0
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val code = g.writeCharacteristic(
                ch, frame,
                if (noResponse) BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
                else BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT,
            )
            if (code != 0) {   // BluetoothStatusCodes.SUCCESS == 0（常量内联，免引 33+ 类）
                LogStore.warn("GATT 写被拒 err=$code len=${frame.size}")
            }
        } else {
            @Suppress("DEPRECATION")
            ch.writeType = if (noResponse) BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            else BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            @Suppress("DEPRECATION")
            ch.value = frame
            @Suppress("DEPRECATION")
            if (!g.writeCharacteristic(ch)) {
                LogStore.warn("GATT 写被拒（旧 API）len=${frame.size}")
            }
        }
    }

    /** 服务发现单飞：仅在 discoverServices 受理（返回 true）时置位；被拒（操作在途）留给下个回调补发。 */
    private fun ensureDiscovery(g: BluetoothGatt) {
        if (discoveryIssued) return
        val accepted = g.discoverServices()
        discoveryIssued = accepted
        // 受理=false＝操作队列忙（真机排障关键信号：出现即序位冲突，回调必不来）
        LogStore.info("GATT 服务发现请求 受理=$accepted")
    }

    /** 订阅单飞（本地通知开关 + CCCD 写）：仅在 writeDescriptor 受理时置位；被拒留给 MTU 回调/兜底时钟补发。 */
    private fun ensureSubscribe(g: BluetoothGatt) {
        if (subscribeIssued) return
        val indicate = indicateCharacteristic ?: return
        g.setCharacteristicNotification(indicate, true)
        val cccd = indicate.getDescriptor(UUID_CCCD) ?: return
        // 现网 V3.21 同款坑位修正：该特征属性为 NOTIFY，CCCD 必须写 0x0100——
        // 写 INDICATION 值(0x0200)设备拒收 → onDescriptorWrite status=129（真机实证）
        val cccdValue =
            if (indicate.properties and BluetoothGattCharacteristic.PROPERTY_NOTIFY != 0) {
                BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
            } else {
                BluetoothGattDescriptor.ENABLE_INDICATION_VALUE
            }
        @Suppress("DEPRECATION")
        cccd.value = cccdValue
        @Suppress("DEPRECATION")
        val accepted = g.writeDescriptor(cccd)
        subscribeIssued = accepted
        LogStore.info("GATT 订阅请求 受理=$accepted")
    }

    // ---- GATT 回调（系统线程 → 主线程投递） ----

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            onMain {
                LogStore.info("GATT 状态: newState=$newState status=$status")
                // 残留回调守卫：旧 gatt 的回调不得误伤新连接的回调槽（真机竞态）
                if (g !== gatt && newState == BluetoothProfile.STATE_DISCONNECTED) {
                    LogStore.info("GATT 忽略残留实例断开回调")
                    runCatching { g.close() }
                    return@onMain
                }
                when (newState) {
                    BluetoothProfile.STATE_CONNECTED -> {
                        // 三阶段上报①链路建立（iOS didConnect 对位）：编排器据此结束 LINK
                        // 预算、给服务发现独立计时——此前只发末端 NotificationsEnabled，
                        // 阶段恒停 LINK，真机 BLE 全链成功也 10s 报「连接超时」
                        connectEvents?.invoke(ConnectEvent.LinkEstablished)
                        // 连接序对调（b20）：链路一通立即发现服务（Google BLE 指南/iOS 同序），
                        // MTU 后置到订阅完成之后。真机实证（b18/b19 八连试）：requestMtu
                        // 先行时即使 onMtuChanged 已回 status=0，部分机型 GATT 操作队列
                        // 仍视 MTU 在途，discoverServices 恒被拒（受理=false）且无补发时机，
                        // 服务发现回调永不至——现网 V3.21 的 MTU 先行序在此机型不可复刻
                        ensureDiscovery(g)
                    }
                    BluetoothProfile.STATE_DISCONNECTED -> {
                        val wasConnected = connectedDevice != null
                        runCatching { g.close() }
                        if (g === gatt) gatt = null
                        connectedDevice = null
                        indicateCharacteristic = null
                        writeCharacteristic = null
                        if (wasConnected) {
                            onDisconnected?.invoke()
                        } else {
                            connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.CONNECTION_LOST))
                            connectEvents = null
                        }
                    }
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            // MTU 落定 → 订阅（正常路径：此刻队列空闲必受理；兜底已受理过则单飞跳过）
            onMain {
                LogStore.info("GATT MTU=$mtu status=$status")
                if (g === gatt) ensureSubscribe(g)
            }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            onMain {
                LogStore.info("GATT 服务发现 status=$status")
                val fail = {
                    connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.SERVICE_MISSING))
                    connectEvents = null
                }
                if (status != BluetoothGatt.GATT_SUCCESS) {
                    fail()
                    return@onMain
                }
                val service = g.getService(HKTProfile.serviceUUID)
                val indicate = service?.getCharacteristic(HKTProfile.indicateUUID)
                val write = service?.getCharacteristic(HKTProfile.writeUUID)
                // iOS didDiscoverCharacteristicsFor 对位：两特征缺一即服务不完整
                if (service == null || indicate == null || write == null) {
                    fail()
                    return@onMain
                }
                indicateCharacteristic = indicate
                writeCharacteristic = write
                // 三阶段上报②服务与特征就绪（Android 单回调＝iOS 服务发现+特征发现两步合并）
                connectEvents?.invoke(ConnectEvent.ServicesDiscovered)
                // 订阅序（b21 回归 V3.21 现网语义）：先谈 MTU、落定后再写 CCCD——
                // b20 曾把 MTU 后置到订阅之后，真机轮询/ACK 双无（设备开机却显示
                // 开机页＝上行帧零到达）：透明桥模组 notify 管线在订阅后遭遇 MTU
                // 交换会停摆。本机队列怪癖防御（b18/b19 教训）：MTU 请求在途时写
                // CCCD 会被拒（ensureSubscribe 受理=false 不置位），由 onMtuChanged
                // 或兜底时钟补发。
                if (g.requestMtu(512)) {
                    mainHandler.postDelayed({ if (g === gatt) ensureSubscribe(g) }, 1_500)
                    mainHandler.postDelayed({ if (g === gatt) ensureSubscribe(g) }, 3_000)
                } else {
                    ensureSubscribe(g)
                }
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            onMain {
                LogStore.info("GATT 订阅回执 status=$status")
                if (descriptor.uuid != UUID_CCCD) return@onMain
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    // 订阅成功 = 链路就绪（失败不得报连接成功——M5 自查修正）
                    connectedDevice = pendingConnectDevice
                    connectEvents?.invoke(ConnectEvent.NotificationsEnabled)
                    connectEvents = null
                } else {
                    connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.CONNECTION_LOST))
                    connectEvents = null
                }
            }
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            onMain {
                // 收帧遥测：区分「上行断流」与「上行正常」的关键信号（b20 后真机
                // 设备开机却零上行，日志无从分辨）
                receivedFrames += 1
                if (receivedFrames <= 5 || receivedFrames % 50 == 0) {
                    LogStore.info("GATT 收帧 #$receivedFrames len=${value.size}")
                }
                onReceive?.invoke(value)
            }
        }

        @Deprecated("pre-33 path")
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            val value = characteristic.value ?: return
            onCharacteristicChanged(g, characteristic, value)
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            // 仅带响应写会回调（无响应写靠 send() 的受理码把关）
            onMain { if (status != BluetoothGatt.GATT_SUCCESS) LogStore.warn("GATT 写回执 err=$status") }
        }
    }

    companion object {
        private val UUID_CCCD: java.util.UUID = java.util.UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    }
}
