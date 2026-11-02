package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.time.Instant;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

/**
 * Flush must persist only the two timestamp fields (last_frame_at,
 * last_event_at) of a freshly-read entity — never the REST backfill cursor —
 * and must re-queue devices whose row failed, so one bad row never drops the
 * observation.
 */
class LastFrameTrackerTest {

    private RegisteredDeviceRepository devices;
    private LastFrameTracker tracker;

    @BeforeEach
    void setUp() {
        devices = mock(RegisteredDeviceRepository.class);
        tracker = new LastFrameTracker(devices);
    }

    @Test
    void emptyFlushTouchesNothing() {
        tracker.flush();
        verifyNoInteractions(devices);
    }

    @Test
    void newerFrameUpdatesBothTimestampsAndKeepsCursor() {
        RegisteredDevice device = new RegisteredDevice();
        device.setId(7L);
        device.setTelemetryCursorMs(1_000L);
        device.setLastFrameAt(Instant.ofEpochMilli(2_000L));
        when(devices.findById(7L)).thenReturn(Optional.of(device));

        tracker.record(7L, 5_000L);
        tracker.flush();

        verify(devices).save(device);
        assertEquals(Instant.ofEpochMilli(5_000L), device.getLastFrameAt());
        assertEquals(Instant.ofEpochMilli(5_000L), device.getLastEventAt());
        assertEquals(1_000L, device.getTelemetryCursorMs());
    }

    @Test
    void staleFrameNeverRegressesTimestamps() {
        RegisteredDevice device = new RegisteredDevice();
        device.setId(8L);
        device.setLastFrameAt(Instant.ofEpochMilli(9_000L));
        when(devices.findById(8L)).thenReturn(Optional.of(device));

        tracker.record(8L, 5_000L);
        tracker.flush();

        verify(devices, never()).save(any(RegisteredDevice.class));
        assertEquals(Instant.ofEpochMilli(9_000L), device.getLastFrameAt());
    }

    @Test
    void failedRowIsRequeuedAndRetriedOnNextFlush() {
        RegisteredDevice device = new RegisteredDevice();
        device.setId(9L);
        when(devices.findById(9L)).thenReturn(Optional.of(device));

        tracker.record(9L, 4_000L);
        // doThrow/doReturn syntax: when(devices.save(...)) would really invoke
        // the mock and trip over the previous thenThrow stub.
        doThrow(new RuntimeException("db down")).when(devices).save(device);
        tracker.flush();
        verify(devices, times(1)).save(device);

        doReturn(device).when(devices).save(device);
        tracker.flush();
        verify(devices, times(2)).save(device);
        assertEquals(Instant.ofEpochMilli(4_000L), device.getLastFrameAt());
    }

    @Test
    void unknownDeviceIsDroppedWithoutError() {
        tracker.record(404L, 4_000L);
        when(devices.findById(404L)).thenReturn(Optional.empty());
        tracker.flush();
        verify(devices, never()).save(any(RegisteredDevice.class));
    }
}
