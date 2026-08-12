package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
import android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
import android.os.Build
import android.os.Environment
import android.util.Log
import java.io.File
import java.util.*
import kotlin.experimental.xor

//蓝牙连接状态
public var connectState: Boolean = false

/** 简易文件日志（诊断用）：追加写入私有目录与 Download，并维护内存缓冲；日志文件上限 256K；可通过 About 菜单查看。 */
object FileLogger {
    private const val MAX_LOG_BYTES = 256 * 1024
    private val buffer = StringBuffer()
    private fun dirs(): List<File> {
        val result = mutableListOf<File>()
        try { BaseApp.context.getExternalFilesDir(null)?.let { result.add(File(it, "log")) } } catch (e: Exception) {}
        try { result.add(File(Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS), "BLETools_log")) } catch (e: Exception) {}
        return result
    }
    @Synchronized
    fun log(tag: String, msg: String) {
        val line = "${System.currentTimeMillis()} [$tag] $msg\n"
        buffer.append(line)
        if (buffer.length > 50000) buffer.delete(0, buffer.length - 50000)
        for (dir in dirs()) {
            try {
                if (!dir.exists()) dir.mkdirs()
                val f = File(dir, "run.log")
                f.appendText(line)
                // 日志文件上限 256K，超过则保留最新的一半
                if (f.length() > MAX_LOG_BYTES) {
                    val full = f.readText()
                    f.writeText(full.substring(full.length / 2))
                }
            } catch (e: Exception) {}
        }
    }
    @Synchronized
    fun snapshot(): String {
        val s = buffer.toString()
        return if (s.length > 8000) "(仅显示最后8000字符)\n" + s.substring(s.length - 8000) else s
    }
    /** 读取私有目录 run.log（上限 256K）。 */
    @Synchronized
    fun lastFileLog(): String {
        return try {
            val f = File(BaseApp.context.getExternalFilesDir(null), "log/run.log")
            if (f.exists()) f.readText() else "（run.log 不存在：FileLogger 可能未被调用，或私有目录写入失败）"
        } catch (e: Exception) {
            "读取文件失败: ${e.message}"
        }
    }
}

class BleUuid {

    companion object BleUuid {
        /**
         * 服务 UUID
         */
        const val SERVICE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CB7"

        /**
         * 描述 UUID
         */
        const val DESCRIPTOR_UUID = "00002902-0000-1000-8000-00805F9B34FB"
        /**
         * 特征（特性）写入 UUID
         */
        const val CHARACTERISTIC_WRITE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CBA"
        /**
         * 特征（特性）表示 UUID
         */
        const val CHARACTERISTIC_INDICATE_UUID = "0783B03E-8535-B5A0-7140-A304D2495CB8"
        /**
         * 是否过滤设备名称为Null的设备
         */
        const val NULL_NAME = "N/A"

        /**
         * 过滤信号强度值
         */
        const val RSSI = "rssi"
    }

}


data class BleDevice(var device:BluetoothDevice, var rssi:Int, var name:String?)


class BleCallback : BluetoothGattCallback() {
    private val TAG = BleCallback::class.java.simpleName
    private var uiCallback: MainActivity? = null
    private var uiDebugCallback: DebugActivity? = null

    fun setUiCallback(uiCallback: MainActivity) {
        this.uiCallback = uiCallback
    }

    fun setUiDebugCallback(uiDebugCallback: DebugActivity) {
        this.uiDebugCallback = uiDebugCallback
    }

    /**
     * 连接状态回调
     */
    @SuppressLint("MissingPermission")
    override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
        Log.d(TAG, "onConnectionStateChange: status=$status, newState=$newState")
        
        if (status != BluetoothGatt.GATT_SUCCESS) {
            Log.e(TAG, "Bluetooth GATT operation failed with status: $status")
            uiCallback?.stateEvent("Bluetooth connection failed: $status")
            // 当连接失败时，确保正确关闭GATT资源
            safeCloseGatt(gatt)
            connectState = false
            return
        }

        uiCallback?.stateEvent(
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    //获取MtuSize
                    gatt.requestMtu(512)
                    "connect success, wait enable notification..."
                }
                BluetoothProfile.STATE_DISCONNECTED -> {
                    connectState = false
                    // 确保在断开连接时正确关闭GATT资源
                    safeCloseGatt(gatt)
                    "Bluetooth Disconnect"
                }
                else -> "onConnectionStateChange: $status"
            }
        )
    }

    /**
     * 安全关闭GATT连接并释放资源
     */
    @SuppressLint("MissingPermission")
    private fun safeCloseGatt(gatt: BluetoothGatt?) {
        gatt?.let { gattInstance ->
            try {
                gattInstance.disconnect()
                gattInstance.close()
                Log.d(TAG, "GATT connection closed successfully")
            } catch (e: Exception) {
                Log.e(TAG, "Error closing GATT connection: ${e.message}")
            }
        }
    }

    /**
     * 获取MtuSize回调
     */
    @SuppressLint("MissingPermission")
    override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
        Log.d(TAG, "onMtuChanged: mtu=$mtu, status=$status")
        if (status == BluetoothGatt.GATT_SUCCESS) {
            //发现服务
            gatt.discoverServices()
        } else {
            Log.e(TAG, "Failed to set MTU size: $status")
            uiCallback?.stateEvent("Failed to set MTU size: $status")
            safeCloseGatt(gatt)
            connectState = false
        }
    }

    /**
     * 发现服务回调
     */
    @SuppressLint("MissingPermission")
    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
        Log.d(TAG, "onServicesDiscovered: status=$status")
        uiCallback?.stateEvent(if (!BleHelper.enableIndicateNotification(gatt)) {
            safeCloseGatt(gatt)
            "open notification error"
        } else "find server code: $status")
    }

    /**
     * 特性改变回调
     */
    override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
        // 该回调运行在 BLE Binder 线程，未捕获异常会触发默认 UncaughtExceptionHandler 杀掉整个进程
        // （表现为无 Toast 闪退）。整体用 try-catch 兜底，并对 characteristic.value 判空：
        // Android 13+ / 部分固件下 indication 的 value 可能为 null，直接 .map{} 会抛 NPE。
        // DC200 数据量最大（含 20 字节雷达频谱），最易第一个踩到该边界，故历史上"只有地磁闪退"。
        try {
            val value = characteristic.value
            if (value == null) {
                Log.e(TAG, "onCharacteristicChanged: characteristic.value is null, skip frame")
                return
            }
            val content = value.map { it.toChar() }.joinToString(separator = "")
            Log.d("ble-rev", " = $characteristic")
            Log.d("ble-rev", "value = 0x" + ByteUtils.bytesToHexString(value).toString().uppercase(Locale.getDefault()))

            if (debugActivityPageRun == 1) {
                uiDebugCallback?.let { callback ->
                    try {
                        callback.stateEvent("$content")
                    } catch (e: Exception) {
                        Log.e(TAG, "Error updating debug UI: ${e.message}")
                    }
                } ?: run {
                    Log.e(TAG, "uiDebugCallback is not initialized")
                }
            } else {
                try {
                    streamRev(
                        ByteUtils.bytesToHexString(value).toString()
                            .uppercase(Locale.getDefault())
                    )
                } catch (e: Exception) {
                    Log.e(TAG, "Error processing received data: ${e.message}")
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "onCharacteristicChanged unexpected error: ${e.message}", e)
        }
    }

    /**
     * 特性写入回调
     */
    override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        val command = ByteUtils.bytesToHexString(characteristic.value)
        Log.d(TAG, "onCharacteristicWrite: status=$status, command=$command")
        if (status != BluetoothGatt.GATT_SUCCESS) {
            Log.e(TAG, "Write failed (status=$status), allowing immediate retry for command: $command")
            mDeviceEvent.commandRetryWait = 0
        }
    }

    /**
     * 描述符写入回调
     */
    @SuppressLint("MissingPermission")
    override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
        if (BleUuid.DESCRIPTOR_UUID == descriptor.toString().lowercase(Locale.getDefault()) ||
            BleUuid.DESCRIPTOR_UUID == descriptor.uuid.toString().uppercase(Locale.getDefault())) {
            uiCallback?.stateEvent(if (status == BluetoothGatt.GATT_SUCCESS) {
                gatt.apply {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) readPhy()
                    readDescriptor(descriptor)
                    readRemoteRssi()
                    connectState = true
                }
                "open notification successful"
            } else {
                safeCloseGatt(gatt)
                "open notification fail"
            })
        }
    }

    /**
     * 读取远程设备的信号强度回调
     */
    override fun onReadRemoteRssi(gatt: BluetoothGatt?, rssi: Int, status: Int) {
        uiCallback?.stateEvent("onReadRemoteRssi: rssi: $rssi")
    }

    /**
     * UI回调
     */
    interface UiCallback {
        /**
         * 当前Ble状态信息
         */
        fun stateEvent(state: String)
    }
}


object BleHelper {

    /**
     * 启用指令通知
     */
    fun enableIndicateNotification(gatt: BluetoothGatt?): Boolean {
        gatt ?: run {
            Log.e("BleHelper", "BluetoothGatt is null, cannot enable notifications")
            return false
        }
        
        return try {
            val service = gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            service ?: run {
                Log.e("BleHelper", "Service not found: ${BleUuid.SERVICE_UUID}")
                return false
            }
            
            val characteristic = service.getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_INDICATE_UUID))
            characteristic ?: run {
                Log.e("BleHelper", "Characteristic not found: ${BleUuid.CHARACTERISTIC_INDICATE_UUID}")
                return false
            }
            
            setCharacteristicNotification(gatt, characteristic)
        } catch (e: Exception) {
            Log.e("BleHelper", "Error enabling notifications: ${e.message}")
            false
        }
    }

    /**
     * 设置特征通知
     * return true, if the write operation was initiated successfully
     */
    @SuppressLint("MissingPermission")
    private fun setCharacteristicNotification(gatt: BluetoothGatt, gattCharacteristic: BluetoothGattCharacteristic): Boolean {
        return try {
            if (gatt.setCharacteristicNotification(gattCharacteristic, true)) {
                val descriptor = gattCharacteristic.getDescriptor(UUID.fromString(BleUuid.DESCRIPTOR_UUID))
                descriptor ?: run {
                    Log.e("BleHelper", "Descriptor not found: ${BleUuid.DESCRIPTOR_UUID}")
                    return false
                }
                
                descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                gatt.writeDescriptor(descriptor)
            } else {
                Log.e("BleHelper", "Failed to set characteristic notification")
                false
            }
        } catch (e: Exception) {
            Log.e("BleHelper", "Error setting characteristic notification: ${e.message}")
            false
        }
    }
    
    /**
     * 发送指令
     * @param gatt gatt
     * @param command 指令
     * @param isResponse 是否响应
     */
    @SuppressLint("MissingPermission")
    fun sendCommand(gatt: BluetoothGatt?, command: String, isResponse: Boolean = true): Boolean {
        gatt ?: run {
            Log.e("BleHelper", "BluetoothGatt is null, cannot send command")
            return false
        }
        
        if (!connectState) {
            Log.e("BleHelper", "Bluetooth not connected, cannot send command")
            return false
        }
        
        return try {
            val service = gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            service ?: run {
                Log.e("BleHelper", "Service not found: ${BleUuid.SERVICE_UUID}")
                return false
            }
            
            val characteristic = service.getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_WRITE_UUID))
            characteristic ?: run {
                Log.e("BleHelper", "Characteristic not found: ${BleUuid.CHARACTERISTIC_WRITE_UUID}")
                return false
            }
            
            characteristic.writeType = if (isResponse) WRITE_TYPE_DEFAULT else WRITE_TYPE_NO_RESPONSE
            characteristic.value = ByteUtils.hexStringToBytes(command)
            gatt.writeCharacteristic(characteristic)
        } catch (e: Exception) {
            Log.e("BleHelper", "Error sending command: ${e.message}")
            false
        }
    }

    @SuppressLint("MissingPermission")
    fun sendCommandString(gatt: BluetoothGatt?, command: String, isResponse: Boolean = false): Boolean {
        gatt ?: run {
            Log.e("BleHelper", "BluetoothGatt is null, cannot send command string")
            return false
        }
        
        if (!connectState) {
            Log.e("BleHelper", "Bluetooth not connected, cannot send command string")
            return false
        }
        
        return try {
            val service = gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            service ?: run {
                Log.e("BleHelper", "Service not found: ${BleUuid.SERVICE_UUID}")
                return false
            }
            
            val characteristic = service.getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_WRITE_UUID))
            characteristic ?: run {
                Log.e("BleHelper", "Characteristic not found: ${BleUuid.CHARACTERISTIC_WRITE_UUID}")
                return false
            }
            
            characteristic.writeType = if (isResponse) WRITE_TYPE_DEFAULT else WRITE_TYPE_NO_RESPONSE
            characteristic.value = command.encodeToByteArray()
            gatt.writeCharacteristic(characteristic)
        } catch (e: Exception) {
            Log.e("BleHelper", "Error sending command string: ${e.message}")
            false
        }
    }
}

object ByteUtils {

    /**
     * Convert hex string to byte[]
     *
     * @param hexString the hex string
     * @return byte[]
     */
    fun hexStringToBytes(hexString: String): ByteArray {
        val hexString = hexString.uppercase(Locale.getDefault())
        val length = hexString.length / 2
        val hexChars = hexString.toCharArray()
        val byteArrayResult = ByteArray(length)
        for (i in 0 until length) {
            val pos = i * 2
            byteArrayResult[i] = (charToByte(hexChars[pos]).toInt().shl(4) or charToByte(hexChars[pos + 1]).toInt()).toByte()
        }
        Log.d("TAG", "hexStringToBytes: "+ byteArrayResult.contentToString())
        return byteArrayResult
    }

    /**
     * Convert byte[] to string
     */
    fun bytesToHexString(src: ByteArray?): String? {
        val stringBuilder = StringBuilder("")
        if (src == null || src.isEmpty()) {
            return null
        }
        for (element in src) {
            val v = element.toInt() and 0xFF
            val hv = Integer.toHexString(v)
            if (hv.length < 2) {
                stringBuilder.append(0)
            }
            stringBuilder.append(hv)
        }
        return stringBuilder.toString()
    }


    private fun byteToHexString(src: Byte): String = Integer.toHexString((src.toInt() and 0xFF))

    /**
     * 亦或校验(BCC校验)
     *
     * @param datas
     * @return
     */
    private fun getBCCResult(datas: ByteArray): Byte {
        var temp = datas[0]
        for (i in 1 until datas.size) {
            temp = temp xor datas[i]
        }
        return temp
    }

    fun getBCCResult(hexString: String): String {
        val byteToHexString = byteToHexString(getBCCResult(hexStringToBytes(hexString)))
        return if (byteToHexString.length < 2)  "0$byteToHexString" else byteToHexString
    }

    fun byteArrayToHexString(byteArray: ByteArray): String {
        val sb = StringBuilder()
        for (i in byteArray.indices) {
            var hex = Integer.toHexString((byteArray[i]).toInt() and 0xFF)
            if (hex.length == 1) {
                hex = "0$hex"
            }
            sb.append(hex.uppercase(Locale.getDefault()))
        }
        return sb.toString()
    }

    /**
     * Convert char to byte
     *
     * @param c char
     * @return byte
     */
    private fun charToByte(c: Char): Byte = "0123456789ABCDEF".indexOf(c).toByte()

}