package com.hkt.ble.bletools

import org.junit.Assert.assertEquals
import org.junit.Test

class CommunicateTest {

    @Test
    fun testStreamRev_whenHumidityIsPositive_shouldDivideByThousand() {
        streamRev("686B7400" + "000A0186A0")

        assertEquals(100.0F, mDeviceData.humidity)
        assertEquals("100.0%", mDeviceDataString.humidity)
    }

    @Test
    fun testStreamRev_whenHumidityHasSignBit_shouldDecodeTwosComplement() {
        streamRev("686B7400" + "000AFFE890")

        assertEquals(-6.0F, mDeviceData.humidity)
        assertEquals("-6.0%", mDeviceDataString.humidity)
    }
}
