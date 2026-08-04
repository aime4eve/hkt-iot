package com.hkt.ble.bletools

import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import com.hkt.ble.bletools.MainActivity.MainActivity.shouldFilterDevice

class BluetoothScanFilterTest {

    @Test
    fun testFilterNullNameDevice_whenEnabled_shouldFilterNullNames() {
        // When filtering is enabled (isScanNullNameDevice = true)
        
        // 1. Device name is null -> Should return true (filter it out)
        assertTrue(shouldFilterDevice(true, null))
        
        // 2. Device name is NOT null -> Should return false (keep it)
        assertFalse(shouldFilterDevice(true, "Device"))
    }

    @Test
    fun testFilterNullNameDevice_whenDisabled_shouldAllowNullNames() {
        // When filtering is disabled (isScanNullNameDevice = false)
        
        // 1. Device name is null -> Should return false (keep it)
        assertFalse(shouldFilterDevice(false, null))
        
        // 2. Device name is NOT null -> Should return false (keep it)
        assertFalse(shouldFilterDevice(false, "Device"))
    }
}
