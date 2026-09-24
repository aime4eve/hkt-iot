package com.hkt.ble.bletools.core.device

import com.hkt.ble.bletools.core.protocol.DeviceFamily

/**
 * Device name → profile registry (requirement §4; Android parseDeviceType compatibility).
 * EPS100/MPS100/DC200 all normalize to the DC200Family profile; unknown names never connect.
 */
data class DeviceProfile(
    val family: DeviceFamily,
    val advertisedName: String,
    val capabilities: Set<DeviceCapability>,
)

enum class DeviceCapability {
    OTA,
    STATUS_QUERY,
    POWER_CONTROL,      // 0xFE
    CALIBRATION,        // 0xFD — long-running, async completion
    BASIC_CONFIG,       // 0x02
    SVC_TASKS,          // 0x03/0x04/0x05
    TIME_SYNC,          // 0x06
    TARGETED_SCAN,      // 定位/扫码
    VALVE_CONTROL,      // firmware-only 0xF9, never exposed (Q1)
}

object DeviceRegistry {
    /** Advertised names → family (Ultrasonic `config.h` PROCT_NAME; ParkingSensor `#if/#else`
     *  branches EPS100/MPS100; Solenoid SVC100). */
    private val familiesByAdvertisedName = mapOf(
        "UDS100" to DeviceFamily.UDS100,
        "DC200" to DeviceFamily.DC200_FAMILY,
        "EPS100" to DeviceFamily.DC200_FAMILY,
        "MPS100" to DeviceFamily.DC200_FAMILY,
        "SVC100" to DeviceFamily.SVC100,
    )

    fun match(advertisedName: String): DeviceProfile? {
        val family = familiesByAdvertisedName[advertisedName] ?: return null
        val capabilities = mutableSetOf(
            DeviceCapability.OTA, DeviceCapability.STATUS_QUERY, DeviceCapability.POWER_CONTROL,
            DeviceCapability.BASIC_CONFIG, DeviceCapability.TIME_SYNC, DeviceCapability.TARGETED_SCAN,
        )
        if (family != DeviceFamily.SVC100) {
            capabilities.add(DeviceCapability.CALIBRATION)   // UDS100 accelerometer/tilt; DC200Family magnetometer (async)
        }
        if (family == DeviceFamily.SVC100) {
            capabilities.add(DeviceCapability.SVC_TASKS)
            capabilities.add(DeviceCapability.VALVE_CONTROL) // registered, never surfaced (Q1)
        }
        return DeviceProfile(family, advertisedName, capabilities)
    }

    /** 广播名匹配（Android parseDeviceType 同语义：包含型号关键字即可，如 "UDS100 3F2A"）。 */
    fun matchBroadcast(broadcastName: String?): DeviceProfile? {
        broadcastName ?: return null
        for ((key, _) in familiesByAdvertisedName) {
            if (broadcastName.contains(key)) return match(key)
        }
        return null
    }

    /**
     * 广播名中的设备 EUI（用户裁决 2026-09-24：详情页头部显示 EUI 而非广播名）。
     * 真机广播名 = 型号关键字 + 分隔符 + EUI 段（如 "SVC100_0D26CF"→"0D26CF"，与 R-2 定位
     * 后缀匹配同一 EUI 段）；去掉型号关键字与分隔符后须全为 hex 才认，否则返回 null（交调用方回退原值）。
     */
    fun euiOf(broadcastName: String?): String? {
        broadcastName ?: return null
        var rest = broadcastName.uppercase()
        for (key in familiesByAdvertisedName.keys) rest = rest.replace(key, "")
        val eui = rest.trim(' ', '_', '-')
        return eui.takeIf { it.isNotEmpty() && it.all { ch -> ch in "0123456789ABCDEF" } }
    }
}
