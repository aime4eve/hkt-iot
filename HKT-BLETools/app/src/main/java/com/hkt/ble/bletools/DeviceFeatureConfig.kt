package com.hkt.ble.bletools

object DeviceFeatureConfig {

    enum class Feature {
        SYNC_TIMESTAMP,
    }

    private val disabledFeatures: Map<DeviceNameEnum, Set<Feature>> = mapOf(
        DeviceNameEnum.NAME_MPS100 to setOf(Feature.SYNC_TIMESTAMP),
        DeviceNameEnum.NAME_UDS100 to setOf(Feature.SYNC_TIMESTAMP),
    )

    fun isFeatureEnabled(deviceType: Int, feature: Feature): Boolean {
        val deviceEnum = DeviceNameEnum.values().getOrNull(deviceType) ?: return true
        return disabledFeatures[deviceEnum]?.contains(feature) != true
    }
}
