package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
import android.bluetooth.BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
import android.os.Build
import android.util.Log
import java.util.*
import kotlin.experimental.xor

//蓝牙连接状态
public var connectState: Boolean = false

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
    private lateinit var uiCallback: MainActivity
    private lateinit var uiDebugCallback: DebugActivity

    fun setUiCallback(uiCallback: MainActivity) {
        this.uiCallback = uiCallback
    }

    fun setUiDebugCallback(uiDebugCallback: DebugActivity) {
        this.uiDebugCallback = uiDebugCallback
    }

    /**
     * 连接状态回调serviceUuid.isNullOrEmpty()
     */
    @SuppressLint("MissingPermission")
    override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
        if (newState == BluetoothProfile.STATE_DISCONNECTED) {
            // 断开连接时无论 status 是什么都要更新状态，否则异常断开无法检测
            connectState = false
            uiCallback.stateEvent("Bluetooth Disconnect")
            return
        }

        if (status != BluetoothGatt.GATT_SUCCESS) {
            Log.e(TAG, "onConnectionStateChange: $status")
            return
        }

        uiCallback.stateEvent(
            when (newState) {
                BluetoothProfile.STATE_CONNECTED -> {
                    gatt.requestMtu(512)
                    "connect success, wait enable notification..."
                }
                else -> "onConnectionStateChange: $status"
            }
        )
    }

    /**
     * 获取MtuSize回调
     */
    @SuppressLint("MissingPermission")
    override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
//        uiCallback.stateEvent("get mtu size：$mtu")
        //发现服务
        gatt.discoverServices()
    }

    /**
     * 发现服务回调
     */
    @SuppressLint("MissingPermission")
    override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
        uiCallback.stateEvent(if (!BleHelper.enableIndicateNotification(gatt)) { gatt.disconnect()
            "open notification error "
        } else "find server code: $status")
    }

    /**
     * 特性改变回调
     */
    override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
        val content = characteristic.value.map { it.toChar() } // 将每个整数转换为对应的字符
            .joinToString(separator = "") // 将字符拼接成字符串
        Log.d("ble-rev", " = $characteristic");
        Log.d("ble-rev", "value = 0x" + ByteUtils.bytesToHexString(characteristic.value).toString().uppercase(Locale.getDefault()));

        if(debugActivityPageRun == 1){
            if (uiDebugCallback != null) {
                uiDebugCallback.stateEvent("$content")
            } else {
                Log.e("ble-error", "uiDebugCallback is not initialized")
            }
        }else {
            streamRev(
                ByteUtils.bytesToHexString(characteristic.value).toString()
                    .uppercase(Locale.getDefault())
            )
        }
    }

    /**
     * 特性写入回调
     */
    override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
        val command = ByteUtils.bytesToHexString(characteristic.value)
//        if(!OTAState) {
//            uiCallback.stateEvent("send: ${if (status == BluetoothGatt.GATT_SUCCESS) "success：" else "fail："}$command code: $status")
//        }
    }

    /**
     * 描述符写入回调
     */
    @SuppressLint("MissingPermission")
    override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
        if (BleUuid.DESCRIPTOR_UUID == descriptor.toString().lowercase(Locale.getDefault()) ||
            BleUuid.DESCRIPTOR_UUID == descriptor.uuid.toString().uppercase(Locale.getDefault())) {
            uiCallback.stateEvent(if (status == BluetoothGatt.GATT_SUCCESS) {
                gatt.apply {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) readPhy()
                    readDescriptor(descriptor)
                    readRemoteRssi()
                    connectState = true
                }
                "open notification successful"
            } else "open notification fail")
        }
    }

    /**
     * 读取远程设备的信号强度回调
     */
    override fun onReadRemoteRssi(gatt: BluetoothGatt?, rssi: Int, status: Int) = uiCallback.stateEvent("onReadRemoteRssi: rssi: $rssi")

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
    fun enableIndicateNotification(gatt: BluetoothGatt): Boolean =
        setCharacteristicNotification(gatt, gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            .getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_INDICATE_UUID)))

    /**
     * 设置特征通知
     * return true, if the write operation was initiated successfully
     */
    @SuppressLint("MissingPermission")
    private fun setCharacteristicNotification(gatt: BluetoothGatt, gattCharacteristic: BluetoothGattCharacteristic): Boolean =
        if (gatt.setCharacteristicNotification(gattCharacteristic, true))
            gatt.writeDescriptor(gattCharacteristic.getDescriptor(UUID.fromString(BleUuid.DESCRIPTOR_UUID))
                .apply {
                    value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                }) else false
    /**
     * 发送指令
     * @param gatt gatt
     * @param command 指令
     * @param isResponse 是否响应
     */
    @SuppressLint("MissingPermission")
    fun sendCommand(gatt: BluetoothGatt, command: String, isResponse: Boolean = true): Boolean =
        gatt.writeCharacteristic(gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            .getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_WRITE_UUID)).apply {
                writeType = if (isResponse) WRITE_TYPE_DEFAULT else WRITE_TYPE_NO_RESPONSE
                value = ByteUtils.hexStringToBytes(command) })

    @SuppressLint("MissingPermission")
    fun sendCommandString(gatt: BluetoothGatt, command: String, isResponse: Boolean = false): Boolean =
        gatt.writeCharacteristic(gatt.getService(UUID.fromString(BleUuid.SERVICE_UUID))
            .getCharacteristic(UUID.fromString(BleUuid.CHARACTERISTIC_WRITE_UUID)).apply {
                writeType = if (isResponse) WRITE_TYPE_DEFAULT else WRITE_TYPE_NO_RESPONSE
                value = command.encodeToByteArray() })
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
            if(hex.length == 1){
                hex = "0$hex"
                sb.append(hex.uppercase(Locale.getDefault()))
            }
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