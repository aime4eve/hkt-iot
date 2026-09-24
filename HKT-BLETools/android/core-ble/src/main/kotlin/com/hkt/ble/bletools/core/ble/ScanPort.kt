package com.hkt.ble.bletools.core.ble

/**
 * 扫描发现的设备（去重后）。identifier 为系统级稳定标识（S-7 冷启动恢复依赖）。
 */
data class DiscoveredDevice(
    val name: String,
    val identifier: String,
    val rssi: Int,
) {
    /** SP-1 名称前缀：广播名前 3 字符；无名称/不足 3 字符 → null（未命名设备天然排除）。 */
    val prefix: String? get() = prefixOf(name)

    companion object {
        fun prefixOf(name: String?): String? {
            name ?: return null
            val p = name.trim().take(3).uppercase()
            return if (p.length == 3) p else null
        }

        /** v1 支持的设备名称前缀（R-1，用户 2026-09-08）。 */
        val supportedPrefixes: Set<String> = setOf("MPS", "SVC", "UDS", "EPS")
    }
}

/**
 * 扫描入列规则（R-1/SP-1）：RSSI ≥ 阈值 && 广播名前缀命中所选集合；无名称天然排除。
 * 过滤规则的唯一出处：SystemCentral（真蓝牙）与 MockCentral（假蓝牙源）共用，保证两端行为一致。
 */
data class ScanOptions(
    val allowedPrefixes: Set<String> = DiscoveredDevice.supportedPrefixes,
    val rssiThreshold: Int = -80,
) {
    fun isListable(name: String?, rssi: Int): Boolean {
        if (rssi < rssiThreshold) return false
        val prefix = DiscoveredDevice.prefixOf(name) ?: return false
        return prefix in allowedPrefixes
    }
}
