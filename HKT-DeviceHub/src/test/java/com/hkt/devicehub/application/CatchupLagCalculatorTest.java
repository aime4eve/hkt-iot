package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import org.junit.jupiter.api.Test;

import java.time.Instant;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Catch-up lag semantics: lag is relative to the device's own newest TB-side
 * frame, never to wall-clock now — silent devices must report 0.
 */
class CatchupLagCalculatorTest {

    private static RegisteredDevice device(Long cursorMs, Instant lastFrameAt) {
        RegisteredDevice device = new RegisteredDevice();
        device.setProject(DeviceProject.LIVESTOCK);
        device.setDevEui("0018b20000001122");
        device.setTelemetryCursorMs(cursorMs);
        device.setLastFrameAt(lastFrameAt);
        return device;
    }

    @Test
    void silentDeviceReportsZeroLag() {
        // Capsule silent for 10 days: cursor sits at its last frame.
        long lastFrame = System.currentTimeMillis() - 10L * 86_400_000;
        RegisteredDevice silent = device(lastFrame, Instant.ofEpochMilli(lastFrame));
        assertEquals(0, CatchupLagCalculator.lagMs(silent));
    }

    @Test
    void uncaughtNewFramesReportPositiveLag() {
        long cursor = 1_000_000L;
        RegisteredDevice behind = device(cursor, Instant.ofEpochMilli(cursor + 5_000));
        assertEquals(5_000, CatchupLagCalculator.lagMs(behind));
    }

    @Test
    void cursorAheadOfLastFrameIsClampedToZero() {
        RegisteredDevice device = device(2_000L, Instant.ofEpochMilli(1_000L));
        assertEquals(0, CatchupLagCalculator.lagMs(device));
    }

    @Test
    void noDataDevicesExcludedFromLagAndCountedSeparately() {
        RegisteredDevice noData = device(null, null);
        RegisteredDevice silent = device(1_000L, Instant.ofEpochMilli(1_000L));
        RegisteredDevice behind = device(1_000L, Instant.ofEpochMilli(6_000L));
        List<RegisteredDevice> devices = List.of(noData, silent, behind);

        assertTrue(CatchupLagCalculator.isNoData(noData));
        assertEquals(1, CatchupLagCalculator.noDataCount(devices));
        assertEquals(5_000, CatchupLagCalculator.maxLagMs(devices));
    }

    @Test
    void nullCursorWithFramesCountsAsZeroLagNotAsHugeLag() {
        // First backfill cycle has not run yet; per contract this is 0, and
        // the device is not "no data" because last_frame_at exists.
        RegisteredDevice fresh = device(null, Instant.ofEpochMilli(1_000L));
        assertEquals(0, CatchupLagCalculator.lagMs(fresh));
        assertTrue(!CatchupLagCalculator.isNoData(fresh));
    }

    @Test
    void allNoDataProjectHasNullMaxLag() {
        assertNull(CatchupLagCalculator.maxLagMs(List.of(device(null, null))));
    }
}
