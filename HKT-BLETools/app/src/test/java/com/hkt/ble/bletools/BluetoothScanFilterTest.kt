package com.hkt.ble.bletools

import org.junit.Assert.assertFalse
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import com.hkt.ble.bletools.MainActivity.MainActivity.getDevEuiMatchSuffix
import com.hkt.ble.bletools.MainActivity.MainActivity.isTargetDevice
import com.hkt.ble.bletools.MainActivity.MainActivity.isValidDevEui
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

    @Test
    fun testIsValidDevEui_whenSixteenHexCharacters_shouldAccept() {
        assertTrue(isValidDevEui("0123456789ABCDEF"))
        assertTrue(isValidDevEui("0123456789abcdef"))
    }

    @Test
    fun testIsValidDevEui_whenInvalidValue_shouldReject() {
        assertFalse(isValidDevEui(""))
        assertFalse(isValidDevEui("0123456789ABC"))
        assertFalse(isValidDevEui("0123456789ABCDEG"))
    }

    @Test
    fun testGetDevEuiMatchSuffix_shouldReturnUppercaseLastSixCharacters() {
        assertEquals("ABCDEF", getDevEuiMatchSuffix("0123456789abcdef"))
    }

    @Test
    fun testGetDevEuiMatchSuffix_whenInvalidValue_shouldReturnNull() {
        assertNull(getDevEuiMatchSuffix("0123456789ABC"))
    }

    @Test
    fun testIsTargetDevice_shouldMatchSuffixIgnoringCase() {
        assertTrue(isTargetDevice("UDS100-ABCDEF", "abcdef"))
        assertFalse(isTargetDevice("UDS100-ABCDEF", "fff123"))
    }

    @Test
    fun testManualEuiInput_whenPrefixPlusLowercaseCharacters_shouldUseUppercaseSuffix() {
        val devEui = MainActivity.MainActivity.DEFAULT_DEV_EUI_PREFIX + "a1b2c3d45"

        assertEquals(7, MainActivity.MainActivity.DEFAULT_DEV_EUI_PREFIX.length)
        assertTrue(isValidDevEui(devEui))
        assertEquals("2C3D45", getDevEuiMatchSuffix(devEui))
    }
}
