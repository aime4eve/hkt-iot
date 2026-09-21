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
import no.nordicsemi.android.support.v18.scanner.BluetoothLeScannerCompat
import no.nordicsemi.android.support.v18.scanner.ScanCallback
import no.nordicsemi.android.support.v18.scanner.ScanResult
import no.nordicsemi.android.support.v18.scanner.ScanSettings

/**
 * 真蓝牙实现（SystemCentral 的 Kotlin 移植）：BluetoothPort + PeripheralLink 双角色。
 *
 * 连接序列对齐现网 BLEUtils 语义（MTU 512 → 发现服务 → 订阅 Indicate → 就绪）；
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
        val g = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            remote.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
        } else {
            remote.connectGatt(context, false, gattCallback)
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
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            g.writeCharacteristic(ch, frame, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
        } else {
            @Suppress("DEPRECATION")
            ch.value = frame
            @Suppress("DEPRECATION")
            g.writeCharacteristic(ch)
        }
    }

    // ---- GATT 回调（系统线程 → 主线程投递） ----

    private val gattCallback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            onMain {
                when (newState) {
                    BluetoothProfile.STATE_CONNECTED -> g.requestMtu(512)
                    BluetoothProfile.STATE_DISCONNECTED -> {
                        val wasConnected = connectedDevice != null
                        runCatching { g.close() }
                        if (g === gatt) gatt = null
                        connectedDevice = null
                        indicateCharacteristic = null
                        writeCharacteristic = null
                        if (wasConnected) {
                            onMain { onDisconnected?.invoke() }
                        } else {
                            connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.CONNECTION_LOST))
                            connectEvents = null
                        }
                    }
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            onMain { g.discoverServices() }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            onMain {
                val service = g.getService(HKTProfile.serviceUUID)
                if (service == null) {
                    connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.SERVICE_MISSING))
                    connectEvents = null
                    return@onMain
                }
                indicateCharacteristic = service.getCharacteristic(HKTProfile.indicateUUID)
                writeCharacteristic = service.getCharacteristic(HKTProfile.writeUUID)
                val indicate = indicateCharacteristic
                if (indicate == null) {
                    connectEvents?.invoke(ConnectEvent.Failed(ConnectFailure.SERVICE_MISSING))
                    connectEvents = null
                    return@onMain
                }
                g.setCharacteristicNotification(indicate, true)
                val cccd = indicate.getDescriptor(UUID_CCCD)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    g.writeDescriptor(cccd, BluetoothGattDescriptor.ENABLE_INDICATION_VALUE)
                } else {
                    @Suppress("DEPRECATION")
                    cccd.value = BluetoothGattDescriptor.ENABLE_INDICATION_VALUE
                    @Suppress("DEPRECATION")
                    g.writeDescriptor(cccd)
                }
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            onMain {
                if (descriptor.uuid == UUID_CCCD) {
                    connectedDevice = pendingConnectDevice
                    connectEvents?.invoke(ConnectEvent.NotificationsEnabled)
                    connectEvents = null
                }
            }
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            onMain { onReceive?.invoke(value) }
        }

        @Deprecated("pre-33 path")
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            val value = characteristic.value ?: return
            onCharacteristicChanged(g, characteristic, value)
        }
    }

    companion object {
        private val UUID_CCCD: java.util.UUID = java.util.UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")
    }
}
