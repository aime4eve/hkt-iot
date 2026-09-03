package com.hkt.ble.bletools

import android.bluetooth.BluetoothGatt
import android.util.Log
import java.util.Locale


//hkt 686B74  bootloader 626F6F746C6F6164
//hkt(3) len(2)(cmd+data) cmd(1) data(n) crc(2)(cmd+data) bootloader(8)
const val HKT_STR = "686B74"
const val BOOTLOAD_STR = "626F6F746C6F6164"

var levelString = ""
var otaLevel = 0
var groupViewUpdate = 0
var fileBin = StringBuffer()


data class DeviceTypeData(
    var name:Int = 0,
    var hardVersion:Int = 0,
    var softVersion:Int = 0,
    var battery:Int = 0,
    var power:Int = 0,
    var powerMode:Int = 0,
    var temperature:Float = 0.0F,
    var humidity:Float = 0.0F,
    var htAlarm:Int = 0,
    var angle:Float = 0.0F,
    var slant:Int = 0,
    var distance:Int = 0,
    var overflowState:Int = 0,
    var overflowLowThreshold:Int = 0,
    var overflowHighThreshold:Int = 0,
    var latitude:Double = 0.0,
    var longitude:Double = 0.0,
    var reportPeriod:Int = 0,
    var gpsPeriod:Int = 0,
    var tamperAlarm:Int = 0,
    var parkMode:Int = 0,
    var parkStatus:Int = 0,
    var valveState:Int = 0,

    // svc100
    var value1State:Int = 0,
    var insert1Connected:Int = 0,
    var pulse1Count:Int = 0,
    var value2State:Int = 0,
    var insert2Connected:Int = 0,
    var pulse2Count:Int = 0,
    var volOut: Int = 0,
    var autoPower:Int = 0,
    var valveMode:Int = 0,
    var buffetingDuration:Int = 0,
    var timeZone :Int = 0,

    // geomagnetic sensor
    var magX:Int = 0,
    var magY:Int = 0,
    var magZ:Int = 0,
    var radarSpectrum:List<Int> = List(10) { 0 },
)

data class DeviceTypeDataString(
    var name: String = "",
    var version: String = "",
    var battery: String = "",
    var power: String = "",
    var powerMode: String = "",
    var temperature: String = "",
    var humidity: String = "",
    var htAlarm: String = "",
    var angle: String = "",
    var slant: String = "",
    var distance: String = "",
    var overflowState: String = "",
    var overflowLowThreshold: String = "",
    var overflowHighThreshold: String = "",
    var latitude: String = "",
    var longitude: String = "",
    var reportPeriod: String = "",
    var gpsPeriod: String = "",
    var tamperAlarm: String = "",
    var parkMode: String = "",
    var parkStatus: String = "",

    var value1State: String = "",
    var insert1Connected: String = "",
    var pulse1Count: String = "",
    var value2State: String = "",
    var insert2Connected: String = "",
    var pulse2Count: String = "",
    var volOut: String = "",
    var valveMode: String = "",
    var buffetingDuration: String = "",
    var timeZone: String = "",

    var magX: String = "",
    var magY: String = "",
    var magZ: String = "",
    var radarSpectrum: String = "",
)

enum class DeviceEventEnum {
    // 枚举常量
    VALUE_NULL,

    POWER_ON_EVENT,
    POWER_ON_START_EVENT,
    POWER_ON_FINISH_EVENT,

    POWER_OFF_EVENT,
    POWER_OFF_START_EVENT,
    POWER_OFF_FINISH_EVENT,

    CALIBRATION_EVENT,
    CALIBRATION_START_EVENT,
    CALIBRATION_EXECUTE_EVENT,
    CALIBRATION_FINISH_EVENT,

    CONFIG_PARAMETER_EVENT,
    CONFIG_PARAMETER_START_EVENT,
    CONFIG_PARAMETER_FINISH_EVENT,

    SYNC_TIMESTAMP_EVENT,
    SYNC_TIMESTAMP_START_EVENT,
    SYNC_TIMESTAMP_FINISH_EVENT,

    SELECT_FILE,
    START_OTA,
    ENTER_OTA,
    UPDATING_OTA,
    SYNC_EVENT,

    // svc100
    CONFIG_TIMED_TASK,
    CONFIG_TIMED_TASK_START_EVENT,
    CONFIG_TIMED_TASK_FINISH_EVENT,
    DELETE_TIMED_TASK,
    DELETE_TIMED_TASK_START_EVENT,
    DELETE_TIMED_TASK_FINISH_EVENT,

    CONFIG_REALTIME_TASK,
    CONFIG_REALTIME_TASK_START_EVENT,
    CONFIG_REALTIME_TASK_FINISH_EVENT,

}

enum class DeviceNameEnum {
    // 枚举常量
    VALUE_NULL,
    NAME_UDS100,
    NAME_DC200,
    NAME_SVC100,
    NAME_MPS100,
}

data class DeviceEventData(
    var event: Int = 0,
    var commandRetryWait: Int = 0,
    var overflowLowThreshold: Int = 0,
    var overflowHighThreshold: Int = 0,
    var reportPeriod: Int = 0,
    var gpsPeriod: Int = 0,
    var power: Int = 0,
    var calibration: Int = 0,
    var otaState: Int = 0,
    var parkMode: Int = 0,
    var valveState: Int = 0,

    // SVC100 使用
    var idTimed: Int = 0,
    var valveTimed: Int = 0,
    var stateTimed: Int = 0,
    var pulseTimed: Int = 0,
    var startTimeTimed: Int = 0,
    var endTimeTimed: Int = 0,
    var repeatTimed: Int = 0,

    var valveRealtime: Int = 0,
    var stateRealtime: Int = 0,
    var pulseRealtime: Int = 0,
    var timeRealtime: Int = 0,
    var buffetingDuration: Int = 0,
    var autoPower: Int = 0,
    var valveMode: Int = 0,
    var volOut: Int = 0,
    var timeZone: Int = 0,

    var Timestamp: Int = 0,
)

var mDeviceData = DeviceTypeData()
var mDeviceEvent = DeviceEventData()
var mDeviceDataString = DeviceTypeDataString()


class StreamThread(gatt: BluetoothGatt?):Thread () {
    //run函数是线程执行start()后执行的函数
    //由于请求过程处于阻塞状态，所以整个请求过程得用线程
    private val device: BluetoothGatt? = gatt
    private var timeout: Int = 0
    override fun run() {
        while (!isInterrupted) {
            // 后台线程的未捕获异常会触发默认 UncaughtExceptionHandler 杀掉整个进程
            // （表现为无 Toast 闪退），因此整个循环体需要兜底。
            try {
                sleep(500)
            } catch (e: InterruptedException) {
                // 被 cleanUp() 的 interrupt() 唤醒，属于正常退出轮询
                interrupt()
                break
            }
            try {
                if(connectState) {
                    if (mDeviceEvent.event > 0 && mDeviceEvent.event != DeviceEventEnum.SYNC_EVENT.ordinal) {
                        if(mDeviceEvent.event == DeviceEventEnum.POWER_ON_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0xFE, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.POWER_OFF_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0xFE, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.CALIBRATION_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0xFD, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_PARAMETER_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0x02, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.ENTER_OTA.ordinal){
                            BleHelper.sendCommand(device, backBootLoaderBuf(fileBin.length/2,fileBin.toString(),1,0),false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_REALTIME_TASK_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0x03, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_TIMED_TASK_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0x04, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.DELETE_TIMED_TASK_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0x05, 0), false)
                        }else if(mDeviceEvent.event == DeviceEventEnum.SYNC_TIMESTAMP_START_EVENT.ordinal){
                            BleHelper.sendCommand(device, streamDevice(0x06, 0), false)
                        }
                    } else {
                        if(debugActivityPageRun == 0) {
                            timeout++
                            if (timeout >= 2) {
                                timeout = 0
                                BleHelper.sendCommand(device, streamDevice(0xFF, 0), false)
                            }
                        }
                    }
                }
            } catch (e: Exception) {
                // 兜底记录，不让单次发送/解析异常拖垮整个轮询线程和进程
                Log.e("StreamThread", "Error in stream loop: ${e.message}", e)
            }
        }
    }
    //获取待发送数据
    private fun streamDevice(cmd: Int, packNum: Int): String {

        var dataLen = 1
        var dataLenString = ""
        var dataString = ""
        var crcData = ""
        var packNumString = ""
        var cmdString = ""

        packNumString = String.format("%0${2}X", packNum)
        if (cmd == 0x01) {   //下行发起更新通知指令，并携带包大小 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 5)
            cmdString = String.format("%0${2}X", cmd)
            dataLen = 4
            dataString = String.format("%0${2 * dataLen}X", 0xFF)
            crcData = String.format("%0${2}X", cmd) + dataString
        } else if (cmd == 0xFF) {   //查询设备状态 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 5)
            cmdString = String.format("%0${2}X", cmd)
            dataLen = 4
            dataString = String.format("%0${2 * dataLen}X", 0xFF)
            crcData = String.format("%0${2}X", cmd) + dataString
        } else if (cmd == 0xFE) {   //开机关机配置 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 5)
            cmdString = String.format("%0${2}X", cmd)
            dataLen = 4
            dataString = if (mDeviceEvent.power == 1)
                String.format("%0${2 * dataLen}X", 0x01)
            else
                String.format("%0${2 * dataLen}X", 0x00)
            crcData = String.format("%0${2}X", cmd) + dataString
        } else if (cmd == 0xFD) {   //校准 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 5)
            cmdString = String.format("%0${2}X", cmd)
            dataLen = 4
            dataString = String.format("%0${2 * dataLen}X", 0x00)
            crcData = String.format("%0${2}X", cmd) + dataString
        } else if (cmd == 0x02) {
            if(mDeviceData.name == DeviceNameEnum.NAME_UDS100.ordinal) {
                //垃圾桶满溢监测传感器上报周期+高低阈值配置 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
                dataLenString = String.format("%0${4}X", 9)
                cmdString = String.format("%0${2}X", cmd)
                dataString = String.format(
                    "%0${4}X%0${4}X%0${4}X%0${4}X",
                    mDeviceEvent.reportPeriod,
                    mDeviceEvent.gpsPeriod,
                    mDeviceEvent.overflowLowThreshold,
                    mDeviceEvent.overflowHighThreshold
                )
            }else if(mDeviceData.name == DeviceNameEnum.NAME_DC200.ordinal) {
                //地磁传感器传感器上报周期+工作模式配置 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
                dataLenString = String.format("%0${4}X", 4)
                cmdString = String.format("%0${2}X", cmd)
                dataString = String.format(
                    "%0${4}X%0${2}X",
                    mDeviceEvent.reportPeriod,
                    mDeviceEvent.parkMode
                )
            }else if(mDeviceData.name == DeviceNameEnum.NAME_SVC100.ordinal) {
                //电磁阀控制器 电压输出值+接口功能+防抖时长+时区+上报周期 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
                dataLenString = String.format("%0${4}X", 8)
                cmdString = String.format("%0${2}X", cmd)
                dataString = String.format(
                    "%0${2}X%0${2}X%0${2}X%0${2}X%0${2}X%0${4}X",
                    mDeviceEvent.volOut,
                    mDeviceEvent.valveMode,
                    mDeviceEvent.buffetingDuration,
                    mDeviceEvent.autoPower,
                    mDeviceEvent.timeZone,
                    mDeviceEvent.reportPeriod,
                )
            }
            crcData = String.format("%0${2}X", cmd) + dataString
        }else if (cmd == 0x03) {
                //电磁阀配置 实时任务 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
                dataLenString = String.format("%0${4}X", 7)
                cmdString = String.format("%0${2}X", cmd)
                dataString = String.format(
                    "%0${2}X%0${2}X%0${4}X%0${4}X",
                    mDeviceEvent.valveRealtime,
                    mDeviceEvent.stateRealtime,
                    mDeviceEvent.timeRealtime,
                    mDeviceEvent.pulseRealtime
                )
            crcData = String.format("%0${2}X", cmd) + dataString
        }
        else if (cmd == 0x04) {
            //电磁阀配置 定时任务 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 11)
            cmdString = String.format("%0${2}X", cmd)
            dataString = String.format(
                "%0${2}X%0${2}X%0${2}X%0${4}X%0${4}X%0${4}X%0${2}X",
                mDeviceEvent.idTimed,
                mDeviceEvent.valveTimed,
                mDeviceEvent.stateTimed,
                mDeviceEvent.pulseTimed,
                mDeviceEvent.startTimeTimed,
                mDeviceEvent.endTimeTimed,
                mDeviceEvent.repeatTimed
            )
            crcData = String.format("%0${2}X", cmd) + dataString
        }
        else if (cmd == 0x05) {
            //电磁阀删除 定时任务 hkt(3) packnum(1) len(2)(cmd+data) cmd(1) data(变长) crc(2)
            dataLenString = String.format("%0${4}X", 2)
            cmdString = String.format("%0${2}X", cmd)
            dataString = String.format(
                "%0${2}X",
                mDeviceEvent.idTimed,
            )
            crcData = String.format("%0${2}X", cmd) + dataString
        }else if (cmd == 0x06) {
            // 同步时间戳
            dataLenString = String.format("%0${4}X", 4)
            cmdString = String.format("%0${2}X", cmd)
            dataString = String.format(
                "%0${8}X",
                mDeviceEvent.Timestamp,
            )
            crcData = String.format("%0${2}X", cmd) + dataString
        }
        //CRC计算
        val crcByteArray = ByteUtils.hexStringToBytes(crcData)
        val crcString = String.format("%0${4}X", CRC16.CRC16_CCITT(crcByteArray))
        return HKT_STR + packNumString + dataLenString + cmdString + dataString + crcString
    }
}

fun streamRev(content: String) {
    val gatt: BluetoothGatt? = MainActivity.getGatt()
    if(content.toString().startsWith("686B74")) {
        if(content.toString().endsWith("626F6F746C6F6164")) {
            val cmd = content.toString().substring(6,8).toInt()
            val packNum = Integer.parseInt(content.toString().substring(8,12), 16)
            if(packNum == 2){
                mDeviceEvent.event = DeviceEventEnum.UPDATING_OTA.ordinal
            }
            BleHelper.sendCommand(gatt, backBootLoaderBuf(fileBin.length/2,fileBin.toString(),cmd,packNum),false)
        }else {
            val packLen = content.length - 5 * 2
            val packNum = Integer.parseInt(content.toString().substring(8, 10), 16)
            val byteArray = ByteUtils.hexStringToBytes(content)
            var array = byteArray.copyOfRange(5, byteArray.size)
            var subArray = IntArray(array.size)
            for ((subArrayLen, i) in array.withIndex()) {
                subArray[subArrayLen] = i.toInt()
                if (subArray[subArrayLen] < 0)
                    subArray[subArrayLen] += 0x100
            }
            var dataLen = subArray.size
            var cmd = 0
            var indexLen = 0
            while (dataLen > 0) {
                if (dataLen <= 0) break
                cmd = subArray[0 + indexLen]
                if (cmd == 0x01) {
                    mDeviceData.hardVersion = subArray[1 + indexLen]
                    mDeviceData.softVersion = subArray[2 + indexLen]
                    mDeviceDataString.version =
                        "V" + mDeviceData.hardVersion.toString() + "." + mDeviceData.softVersion.toString()
                    dataLen -= 3
                    indexLen += 3
                }else if (cmd == 0x03) {
                    mDeviceData.battery = subArray[1 + indexLen]
                    mDeviceDataString.battery = mDeviceData.battery.toString() + "%"
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x09) {
                    var temperature =
                        subArray[1 + indexLen]shl 16 or subArray[2 + indexLen] shl 8 or subArray[3 + indexLen]
                    if (temperature > 0x800000) {
                        temperature = -temperature
                    }
                    mDeviceData.temperature = temperature.toFloat() / 1000
                    mDeviceDataString.temperature = mDeviceData.temperature.toString() + "℃"
                    dataLen -= 4
                    indexLen += 4
                } else if (cmd == 0x0A) {
                    var humidity =
                        (subArray[1 + indexLen] shl 16) or (subArray[2 + indexLen] shl 8) or subArray[3 + indexLen]
                    if (humidity >= 0x800000) {
                        humidity -= 0x1000000
                    }
                    mDeviceData.humidity = humidity.toFloat() / 1000
                    mDeviceDataString.humidity = mDeviceData.humidity.toString() + "%"
                    dataLen -= 4
                    indexLen += 4
                } else if (cmd == 0x0E) {
                    var angle = subArray[1 + indexLen]shl 8 or subArray[2 + indexLen]
                    if (angle > 0x800000) {
                        angle = -angle
                    }
                    mDeviceData.angle = angle.toFloat() / 100
                    mDeviceDataString.angle = mDeviceData.angle.toString() + "°"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x10) {
                    var latitude =
                        subArray[1 + indexLen]shl 24 or subArray[2 + indexLen]shl 16 or subArray[3 + indexLen]shl 8 or subArray[4 + indexLen]
                    if (latitude > 0x80000000) {
                        latitude = -latitude
                    }
                    mDeviceData.latitude = latitude.toDouble() / 1000000
                    mDeviceDataString.latitude = mDeviceData.latitude.toString()
                    dataLen -= 5
                    indexLen += 5
                } else if (cmd == 0x11) {
                    var longitude =
                        subArray[1 + indexLen]shl 24 or subArray[2 + indexLen]shl 16 or subArray[3 + indexLen]shl 8 or subArray[4 + indexLen]
                    if (longitude > 0x80000000) {
                        longitude = -longitude
                    }
                    mDeviceData.longitude = longitude.toDouble() / 1000000
                    mDeviceDataString.longitude = mDeviceData.longitude.toString()
                    dataLen -= 5
                    indexLen += 5
                } else if (cmd == 0x28) {
                    mDeviceData.htAlarm = subArray[1 + indexLen]
                    if(mDeviceData.htAlarm == 1)
                        mDeviceDataString.htAlarm = "High temperature alarm"
                    else
                        mDeviceDataString.htAlarm = "Normal"
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x3A) {
                    val localizedContext = BaseApp.localizedContext()
                    mDeviceData.parkStatus = subArray[1 + indexLen]
                    mDeviceDataString.parkStatus = when (mDeviceData.parkStatus) {
                        0 -> localizedContext.getString(R.string.parking_status_vacant)
                        1 -> localizedContext.getString(R.string.parking_status_occupied)
                        0xFF -> localizedContext.getString(R.string.parking_status_covered)
                        else -> localizedContext.getString(R.string.parking_status_unknown)
                    }
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x3B) {
                    val localizedContext = BaseApp.localizedContext()
                    mDeviceData.parkMode = subArray[1 + indexLen]
                    mDeviceDataString.parkMode = when (mDeviceData.parkMode) {
                        0 -> localizedContext.getString(R.string.parking_mode_fusion)
                        1 -> localizedContext.getString(R.string.parking_mode_magnetic_only)
                        2 -> localizedContext.getString(R.string.parking_mode_radar_priority)
                        else -> localizedContext.getString(R.string.parking_status_unknown)
                    }
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x3C) {
                    mDeviceData.value1State = subArray[1 + indexLen]
                    mDeviceData.insert1Connected = subArray[2 + indexLen]
                    mDeviceData.pulse1Count = subArray[3 + indexLen]shl 8 or subArray[4 + indexLen]
                    mDeviceData.value2State = subArray[5 + indexLen]
                    mDeviceData.insert2Connected = subArray[6 + indexLen]
                    mDeviceData.pulse2Count = subArray[7 + indexLen]shl 8 or subArray[8 + indexLen]

                    mDeviceDataString.value1State = if (mDeviceData.value1State == 1) {
                        "OPEN"
                    } else {
                        "OFF"
                    }
                    mDeviceDataString.insert1Connected = if (mDeviceData.insert1Connected == 1) {
                        "Connect"
                    } else {
                        "Disconnect"
                    }
                    mDeviceDataString.pulse1Count = mDeviceData.pulse1Count.toString()

                    mDeviceDataString.value2State = if (mDeviceData.value2State == 1) {
                        "OPEN"
                    } else {
                        "OFF"
                    }
                    mDeviceDataString.insert2Connected = if (mDeviceData.insert2Connected == 1) {
                        "Connect"
                    } else {
                        "Disconnect"
                    }
                    mDeviceDataString.pulse2Count = mDeviceData.pulse2Count.toString()

                    dataLen -= 9
                    indexLen += 9
                }else if (cmd == 0x40) {
                    mDeviceData.volOut = subArray[1 + indexLen]
                    if(mDeviceData.volOut == 0){
                        mDeviceDataString.volOut = "12V"
                    }else if(mDeviceData.volOut == 1){
                        mDeviceDataString.volOut = "9V"
                    }else if(mDeviceData.volOut == 2){
                        mDeviceDataString.volOut = "5V"
                    }
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x41) {
                    mDeviceData.valveMode = subArray[1 + indexLen]
                    val localizedContext = BaseApp.localizedContext()

                    val portModeValues = intArrayOf(0x00, 0x01, 0x02, 0x03, 0x81, 0x82, 0x83)
                    val portModeIndex = portModeValues.indexOf(mDeviceData.valveMode)
                    mDeviceDataString.valveMode = if (portModeIndex >= 0) {
                        localizedContext.resources.getStringArray(R.array.spinner_items_valve_mode)[portModeIndex]
                    } else {
                        localizedContext.getString(R.string.parking_status_unknown)
                    }
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x42) {
                    mDeviceData.buffetingDuration = subArray[1 + indexLen]
                    mDeviceDataString.buffetingDuration = mDeviceData.buffetingDuration.toString()
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x43) {
                    mDeviceData.powerMode = subArray[1 + indexLen]
                    mDeviceDataString.powerMode = if (mDeviceData.powerMode == 1) {
                        "Auto"
                    } else {
                        "Manual"
                    }
                    dataLen -= 2
                    indexLen += 2
                } else if (cmd == 0x44) {
                    mDeviceData.slant = subArray[1 + indexLen]
                    mDeviceDataString.slant = if (mDeviceData.slant == 1) {
                        "Slant"
                    } else {
                        "Normal"
                    }
                    dataLen -= 2
                    indexLen += 2
                } else if (cmd == 0x45) {
                    mDeviceData.gpsPeriod = subArray[1 + indexLen]shl 8 or subArray[2 + indexLen]
                    mDeviceDataString.gpsPeriod = mDeviceData.gpsPeriod.toString() + "min"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x46) { //超声波距离
                    mDeviceData.distance = subArray[1 + indexLen]shl 8 or subArray[2 + indexLen]
                    mDeviceDataString.distance = mDeviceData.distance.toString() + "mm"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x47) {  //垃圾桶满溢状态
                    val localizedContext = BaseApp.localizedContext()
                    mDeviceData.overflowState = subArray[1 + indexLen]
                    mDeviceDataString.overflowState = when (mDeviceData.overflowState) {
                        0 -> localizedContext.getString(R.string.overflow_state_normal)
                        1 -> localizedContext.getString(R.string.overflow_state_low_threshold)
                        2 -> localizedContext.getString(R.string.overflow_state_high_threshold)
                        0xFF -> localizedContext.getString(R.string.overflow_state_invalid)
                        else -> localizedContext.getString(R.string.parking_status_unknown)
                    }
                    dataLen -= 2
                    indexLen += 2
                } else if (cmd == 0x48) { //超声波测距告警阈值
                    mDeviceData.overflowLowThreshold =
                        subArray[1 + indexLen]shl 8 or subArray[2 + indexLen]
                    mDeviceData.overflowHighThreshold =
                        subArray[3 + indexLen]shl 8 or subArray[4 + indexLen]
                    mDeviceDataString.overflowLowThreshold =
                        mDeviceData.overflowLowThreshold.toString() + "mm"
                    mDeviceDataString.overflowHighThreshold =
                        mDeviceData.overflowHighThreshold.toString() + "mm"
                    dataLen -= 5
                    indexLen += 5
                }else if (cmd == 0x84) {
                    mDeviceData.tamperAlarm = subArray[1 + indexLen]
                    mDeviceDataString.tamperAlarm = if (mDeviceData.tamperAlarm == 1) {
                        "Alarm"
                    } else {
                        "Normal"
                    }
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x86) {
                    mDeviceData.reportPeriod =
                        subArray[1 + indexLen] shl 8 or subArray[2 + indexLen]
                    mDeviceDataString.reportPeriod = mDeviceData.reportPeriod.toString() + "min"
                    dataLen -= 3
                    indexLen += 3
                }else if (cmd == 0x8A) {
                    val localizedContext = BaseApp.localizedContext()
                    mDeviceData.timeZone = subArray[1 + indexLen]
                    mDeviceDataString.timeZone = localizedContext.resources
                        .getStringArray(R.array.spinner_items_timezone)
                        .getOrNull(mDeviceData.timeZone) ?: localizedContext.getString(R.string.parking_status_unknown)
                    dataLen -= 2
                    indexLen += 2
                }else if (cmd == 0x8B) {
                    mDeviceData.battery = subArray[1 + indexLen]shl 8 or subArray[2 + indexLen]
                    mDeviceDataString.battery = mDeviceData.battery.toString() + "mV"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x8D) {
                    if(mDeviceData.power != subArray[1 + indexLen]){
                        groupViewUpdate = 0
                        mDeviceData.power = subArray[1 + indexLen]
                    }
                    mDeviceDataString.power = if (mDeviceData.power == 1) {
                        "ON"
                    } else {
                        "OFF"
                    }
                    dataLen -= 2
                    indexLen += 2
                } else if (cmd == 0x5D) {
                    var magX = subArray[1 + indexLen] shl 8 or subArray[2 + indexLen]
                    if (magX > 0x8000) {
                        magX = magX - 0x10000
                    }
                    mDeviceData.magX = magX
                    mDeviceDataString.magX = mDeviceData.magX.toString() + " μT"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x5E) {
                    var magY = subArray[1 + indexLen] shl 8 or subArray[2 + indexLen]
                    if (magY > 0x8000) {
                        magY = magY - 0x10000
                    }
                    mDeviceData.magY = magY
                    mDeviceDataString.magY = mDeviceData.magY.toString() + " μT"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x5F) {
                    var magZ = subArray[1 + indexLen] shl 8 or subArray[2 + indexLen]
                    if (magZ > 0x8000) {
                        magZ = magZ - 0x10000
                    }
                    mDeviceData.magZ = magZ
                    mDeviceDataString.magZ = mDeviceData.magZ.toString() + " μT"
                    dataLen -= 3
                    indexLen += 3
                } else if (cmd == 0x60) {
                    val spectrumValues = IntArray(10)
                    for (i in 0 until 10) {
                        spectrumValues[i] = subArray[1 + indexLen + i * 2] shl 8 or subArray[1 + indexLen + i * 2 + 1]
                    }
                    mDeviceData.radarSpectrum = spectrumValues.toList()
                    mDeviceDataString.radarSpectrum = spectrumValues.joinToString(", ")
                    dataLen -= 21
                    indexLen += 21
                } else if (cmd == 0xFF) {
                    if(mDeviceEvent.event == DeviceEventEnum.POWER_ON_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.POWER_ON_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.POWER_OFF_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.POWER_OFF_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.CALIBRATION_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.CALIBRATION_EXECUTE_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_PARAMETER_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.CONFIG_PARAMETER_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_REALTIME_TASK_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.CONFIG_REALTIME_TASK_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.CONFIG_TIMED_TASK_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.CONFIG_TIMED_TASK_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.DELETE_TIMED_TASK_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.DELETE_TIMED_TASK_FINISH_EVENT.ordinal
                    }
                    else if(mDeviceEvent.event == DeviceEventEnum.SYNC_TIMESTAMP_START_EVENT.ordinal){
                        mDeviceEvent.event = DeviceEventEnum.SYNC_TIMESTAMP_FINISH_EVENT.ordinal
                    }
                    dataLen -= 2
                    indexLen += 2
                } else {
                    dataLen = 0
                }
            }
        }
    }else if (content.toString().contains("43616C6962726174696F6E20446F6E65")) {
        if(mDeviceEvent.event == DeviceEventEnum.CALIBRATION_EXECUTE_EVENT.ordinal){
            mDeviceEvent.event = DeviceEventEnum.CALIBRATION_FINISH_EVENT.ordinal
        }
    }
}


//获取待发送数据
fun backBootLoaderBuf(fileSize: Int, fileBytes: String, cmd: Int, packNum: Int): String {

    var dataLen = 0
    var dataLenString = ""
    var dataString = ""
    var crcData = ""
    var packNumString = ""
    var cmdString = ""
    var level = 0f

    if (cmd == 1) {   //下行发起更新通知指令，并携带包大小 hkt(3) len(2)(cmd+data) cmd(1) data(4) crc(2) bootloader(8)
        dataLenString = String.format("%0${4}X", 5)
        dataString = String.format("%0${8}X", fileSize)
        crcData = String.format("%0${2}X", cmd) + dataString
        cmdString = String.format("%0${2}X", cmd)

    } else if (cmd == 2) {  //取待更新数据  hkt(3) len(2)(cmd+packnum+data) cmd(1) packnum(2) data(n) crc(2) bootloader(8)
        val alreadySendStringLen = packNum * 256
        val fileStringSize = fileSize * 2
        level = (alreadySendStringLen.toFloat() * 100) / fileStringSize.toFloat()
        // 有效数据
        if (fileStringSize - alreadySendStringLen >= 256) {
            dataString = fileBytes.substring(alreadySendStringLen, alreadySendStringLen + 256).uppercase(
                Locale.getDefault())// 截取数据部分 128bytes
            dataLen = 128
            cmdString = String.format("%0${2}X", cmd)
        } else {
            dataString = fileBytes.substring(alreadySendStringLen, fileStringSize).uppercase(Locale.getDefault())// 截取数据部分
            dataLen = (fileStringSize - alreadySendStringLen)/2
            // 长度不为8的倍数时,在结尾补ff
            if(dataLen % 8 != 0){
                val replenishLen = 8 - (dataLen % 8)
                dataLen += replenishLen
                for (j in 1..replenishLen) {
                    dataString += "FF"
                }
            }
            cmdString = String.format("%0${2}X", 0xFF)
        }
        dataLen += 3
        dataLenString = String.format("%0${4}X", dataLen)
        packNumString = String.format("%0${4}X", packNum)
        crcData = cmdString + packNumString + dataString

    } else if (cmd == 3) {  //完成更新 hkt(3) len(2)(cmd) cmd(1) crc(2) bootloader(8)
        dataLenString = String.format("%0${4}X", 1)
        crcData = String.format("%0${2}X", cmd)
        cmdString = String.format("%0${2}X", cmd)
        level = 100f
    }

    levelString  = (((level * 10).toInt()).toFloat() / 10).toString()
    otaLevel = level.toInt()

    //CRC计算
    val crcByteArray = ByteUtils.hexStringToBytes(crcData)
    val crcString = String.format("%0${4}X",CRC16.CRC16_CCITT(crcByteArray))

    return HKT_STR + dataLenString + cmdString + packNumString + dataString + crcString + BOOTLOAD_STR
}
