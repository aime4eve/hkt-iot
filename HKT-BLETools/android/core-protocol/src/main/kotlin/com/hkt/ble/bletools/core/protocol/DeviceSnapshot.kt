package com.hkt.ble.bletools.core.protocol

/**
 * 详情页/界面共用的设备状态快照（R-5 字段全集）。
 * 由 TLV 记录解码（DeviceSnapshotDecoder）；轮询每秒整体刷新，字段保留上次有效值（R-7）。
 */
data class DeviceSnapshot(
    val family: DeviceFamily,
    var hardwareVersion: Int = 0,
    var softwareVersion: Int = 0,
    /** 0x8D：1 开 / 0 关 */
    var power: Int = 0,
    /** DC200Family/SVC100 估算电量（0x03，1B 百分比） */
    var batteryPercent: Int? = null,
    /** UDS100 电压（0x8B，2B mV） */
    var batteryVoltageMV: Int? = null,
    /** 上报周期（分钟，0x86） */
    var reportPeriodMin: Int? = null,
    // UDS100（0x09 温度 /0x0A 湿度 存毫度；0x46 距离 mm；0x47 满溢四态；0x0E 倾角=原始值/100）
    var temperatureMilli: Int? = null,
    var humidityMilli: Int? = null,
    var distanceMM: Int? = null,
    var overflowState: Int? = null,
    var lowThresholdMM: Int? = null,
    var highThresholdMM: Int? = null,
    var angleCenti: Int? = null,
    var slant: Int? = null,
    var htAlarm: Int? = null,
    var gpsPeriodMin: Int? = null,
    var latitude: Double? = null,
    var longitude: Double? = null,
    // DC200Family（0x3A 车位 0/1/0xFF 遮挡；0x3B 模式 0/1/2；0x84 防拆；0x5D/5E/5F 地磁；0x60 雷达 10 段）
    var parkState: Int? = null,
    var parkMode: Int? = null,
    var tamper: Int? = null,
    var magX: Int? = null,
    var magY: Int? = null,
    var magZ: Int? = null,
    var radarSpectrum: List<Int>? = null,
    // SVC100（0x3C 阀 8B；0x40 电压档 0/1/2=12/9/5V；0x41 端口功能 7 值；0x42 稳定 s；
    // 0x43 智能电源 1 自动；0x8A 时区 0-26）
    var valve1State: Int? = null,
    var valve1Inserted: Int? = null,
    var valve1Pulse: Int? = null,
    var valve2State: Int? = null,
    var valve2Inserted: Int? = null,
    var valve2Pulse: Int? = null,
    var voltageLevel: Int? = null,
    var portFunction: Int? = null,
    var stableTimeS: Int? = null,
    var smartPower: Int? = null,
    var timezone: Int? = null,
)

/**
 * TLV 记录 → 快照字段。字段-类型-尺寸表以固件 setDataPackage/callback_BLEQuery 为唯一出处
 * （firmware-traceability §2.1；Android streamRev 对照）。未知类型由 HKTResponseParser 报
 * unknownTail，这里不参与解码（R-8/S-6）。
 */
object DeviceSnapshotDecoder {
    /** 将记录解码进快照（保留未出现字段的上次有效值——轮询整体刷新语义）。 */
    fun decode(entries: List<TLVEntry>, family: DeviceFamily, snapshot: DeviceSnapshot) {
        for (entry in entries) {
            val v = entry.value
            when (entry.type) {
                0x01 -> if (v.size >= 2) { snapshot.hardwareVersion = v[0].toInt() and 0xFF; snapshot.softwareVersion = v[1].toInt() and 0xFF }
                0x03 -> if (v.size >= 1) snapshot.batteryPercent = v[0].toInt() and 0xFF
                0x09 -> if (v.size >= 3) snapshot.temperatureMilli = BEValue.i24(v)
                0x0A -> if (v.size >= 3) snapshot.humidityMilli = BEValue.i24(v)
                0x0E -> if (v.size >= 2) snapshot.angleCenti = BEValue.u16(v)
                0x10 -> if (v.size >= 4) snapshot.latitude = BEValue.mag31(v) / 1_000_000.0
                0x11 -> if (v.size >= 4) snapshot.longitude = BEValue.mag31(v) / 1_000_000.0
                0x28 -> if (v.size >= 1) snapshot.htAlarm = v[0].toInt() and 0xFF
                0x3A -> if (v.size >= 1) snapshot.parkState = v[0].toInt() and 0xFF
                0x3B -> if (v.size >= 1) snapshot.parkMode = v[0].toInt() and 0xFF
                0x3C -> if (v.size >= 8) {
                    snapshot.valve1State = v[0].toInt() and 0xFF
                    snapshot.valve1Inserted = v[1].toInt() and 0xFF
                    snapshot.valve1Pulse = BEValue.u16(v, 2)
                    snapshot.valve2State = v[4].toInt() and 0xFF
                    snapshot.valve2Inserted = v[5].toInt() and 0xFF
                    snapshot.valve2Pulse = BEValue.u16(v, 6)
                }
                0x40 -> if (v.size >= 1) snapshot.voltageLevel = v[0].toInt() and 0xFF
                0x41 -> if (v.size >= 1) snapshot.portFunction = v[0].toInt() and 0xFF
                0x42 -> if (v.size >= 1) snapshot.stableTimeS = v[0].toInt() and 0xFF
                0x43 -> if (v.size >= 1) snapshot.smartPower = v[0].toInt() and 0xFF
                0x44 -> if (v.size >= 1) snapshot.slant = v[0].toInt() and 0xFF
                0x45 -> if (v.size >= 2) snapshot.gpsPeriodMin = BEValue.u16(v)
                0x46 -> if (v.size >= 2) snapshot.distanceMM = BEValue.u16(v)
                0x47 -> if (v.size >= 1) snapshot.overflowState = v[0].toInt() and 0xFF
                0x48 -> if (v.size >= 4) {
                    snapshot.lowThresholdMM = BEValue.u16(v)
                    snapshot.highThresholdMM = BEValue.u16(v, 2)
                }
                0x84 -> if (v.size >= 1) snapshot.tamper = v[0].toInt() and 0xFF
                0x86 -> if (v.size >= 2) snapshot.reportPeriodMin = BEValue.u16(v)
                0x8A -> if (v.size >= 1) snapshot.timezone = v[0].toInt() and 0xFF
                0x8B -> if (v.size >= 2) snapshot.batteryVoltageMV = BEValue.u16(v)
                0x8D -> if (v.size >= 1) snapshot.power = v[0].toInt() and 0xFF
                0x5D -> if (v.size >= 2) snapshot.magX = BEValue.i16(v)
                0x5E -> if (v.size >= 2) snapshot.magY = BEValue.i16(v)
                0x5F -> if (v.size >= 2) snapshot.magZ = BEValue.i16(v)
                0x60 -> if (v.size >= 20) {
                    snapshot.radarSpectrum = (0..<10).map { BEValue.u16(v, it * 2) }
                }
            }
        }
    }

    /** 一次解码：空快照 + 记录 → 完整快照。 */
    fun snapshotFrom(entries: List<TLVEntry>, family: DeviceFamily): DeviceSnapshot {
        val snapshot = DeviceSnapshot(family)
        decode(entries, family, snapshot)
        return snapshot
    }
}
