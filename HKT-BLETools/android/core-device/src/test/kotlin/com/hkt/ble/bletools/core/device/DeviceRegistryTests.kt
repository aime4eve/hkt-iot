package com.hkt.ble.bletools.core.device

import com.hkt.ble.bletools.core.protocol.DeviceFamily
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue

/// iOS 对照：CoreDeviceTests/DeviceRegistryTests（4 用例）。
class DeviceRegistryTests {
    @Test
    fun testMatchByAdvertisedName() {
        assertEquals(DeviceFamily.UDS100, DeviceRegistry.match("UDS100")?.family)
        assertEquals(DeviceFamily.DC200_FAMILY, DeviceRegistry.match("DC200")?.family)
        assertEquals(DeviceFamily.SVC100, DeviceRegistry.match("SVC100")?.family)
    }

    @Test
    fun testUnknownNameNeverMatches() {
        assertNull(DeviceRegistry.match("UNKNOWN"))
        assertNull(DeviceRegistry.matchBroadcast("something-else"))
        assertNull(DeviceRegistry.matchBroadcast(null))
    }

    @Test
    fun testBroadcastContainsModel() {
        // 真实广播名带后缀（真机实证 "SVC100_0D137C" 带下划线）
        val profile = DeviceRegistry.matchBroadcast("SVC100_0D137C")
        assertEquals(DeviceFamily.SVC100, profile?.family)
        assertEquals(DeviceFamily.DC200_FAMILY, DeviceRegistry.matchBroadcast("EPS100 0288CF")?.family)
        assertEquals(DeviceFamily.DC200_FAMILY, DeviceRegistry.matchBroadcast("MPS100 12AB")?.family)
    }

    @Test
    fun testCapabilitySets() {
        val svc = DeviceRegistry.match("SVC100")!!
        assertTrue(DeviceCapability.SVC_TASKS in svc.capabilities)
        assertTrue(DeviceCapability.CALIBRATION !in svc.capabilities)   // SVC100 固件无 0xFD
        assertTrue(DeviceCapability.VALVE_CONTROL in svc.capabilities)  // 注册但永不露出（Q1）

        val uds = DeviceRegistry.match("UDS100")!!
        assertTrue(DeviceCapability.CALIBRATION in uds.capabilities)    // UDS=角度校准
        assertTrue(DeviceCapability.SVC_TASKS !in uds.capabilities)

        val dc = DeviceRegistry.matchBroadcast("MPS100")!!
        assertTrue(DeviceCapability.CALIBRATION in dc.capabilities)     // DC200Family=磁力计校准
    }
}
