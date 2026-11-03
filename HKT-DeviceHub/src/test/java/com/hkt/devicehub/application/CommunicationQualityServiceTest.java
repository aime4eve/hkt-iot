package com.hkt.devicehub.application;

import org.junit.jupiter.api.Test;

import java.time.Instant;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Pure metric derivation contract: loss windows from f_cnt_up differentials,
 * ABP counter resets are events (never loss), gaps from frame-ts silence,
 * offline from last_frame_at vs 3× expected interval.
 */
class CommunicationQualityServiceTest {

    private static final Instant NOW = Instant.parse("2026-11-03T06:00:00Z");
    private static final long GAP_20MIN_MS = 20 * 60 * 1000L;

    private CommunicationQualityService.SnapPoint ok(Instant polledAt, long fCntUp) {
        return new CommunicationQualityService.SnapPoint(polledAt, true, fCntUp, null, null, "gw-1", null);
    }

    private CommunicationQualityService.FramePoint frame(Instant ts, Integer rssi, Integer snr) {
        return new CommunicationQualityService.FramePoint(ts, rssi, snr);
    }

    @Test
    void lossWindowCountsMissingFrames() {
        Instant t0 = NOW.minusSeconds(3600);
        Instant t1 = NOW;
        // Expected 10 sends, only 9 frames retained → 10%.
        List<CommunicationQualityService.FramePoint> frames = new java.util.ArrayList<>();
        for (int i = 1; i <= 9; i++) {
            frames.add(frame(t0.plusSeconds(60L * i), -90, 5));
        }

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, NOW.minusSeconds(60),
                List.of(ok(t0, 100), ok(t1, 110)), frames, NOW, GAP_20MIN_MS);

        assertEquals(10, d.expectedFrames());
        assertEquals(9, d.receivedFrames());
        assertEquals(1, d.lostFrames());
        assertEquals(10.0, d.lossRatePercent());
        assertEquals(1, d.lossWindows());
        assertTrue(d.online());
        assertEquals(0, d.gapCount());
    }

    @Test
    void counterResetIsAnEventNotLoss() {
        Instant t0 = NOW.minusSeconds(7200);
        Instant t1 = NOW.minusSeconds(3600);
        Instant t2 = NOW;
        // Device rebooted between t1 and t2: f_cnt drops 103 → 13 (ABP).
        List<CommunicationQualityService.FramePoint> frames = List.of(
                frame(t0.plusSeconds(600), -95, 5));

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, NOW.minusSeconds(60),
                List.of(ok(t0, 100), ok(t1, 103), ok(t2, 13)), frames, NOW, GAP_20MIN_MS);

        // Window t0→t1: expected 3, received 1 → lost 2. Window t1→t2: reset.
        assertEquals(1, d.resetEvents());
        assertEquals(1, d.lossWindows());
        assertEquals(3, d.expectedFrames());
        assertEquals(1, d.receivedFrames());
        assertEquals(2, d.lostFrames());
        assertEquals(66.7, d.lossRatePercent());
    }

    @Test
    void failedSnapshotBreaksWindowWithoutHidingTheFact() {
        Instant t0 = NOW.minusSeconds(7200);
        Instant t1 = NOW.minusSeconds(3600);
        Instant t2 = NOW;
        CommunicationQualityService.SnapPoint failed =
                new CommunicationQualityService.SnapPoint(t1, false, null, null, null, null, null);

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, null,
                List.of(ok(t0, 100), failed, ok(t2, 110)), List.of(), NOW, GAP_20MIN_MS);

        // No complete ok→ok pair survives → loss unknown (never 0%).
        assertEquals(0, d.lossWindows());
        assertNull(d.lossRatePercent());
        assertEquals(0, d.resetEvents());
    }

    @Test
    void silenceGapBeyondThresholdIsCounted() {
        Instant base = NOW.minusSeconds(7200);
        List<CommunicationQualityService.FramePoint> frames = List.of(
                frame(base, -90, 5),
                frame(base.plusSeconds(600), -90, 5),
                frame(base.plusSeconds(600).plusSeconds(90 * 60), -90, 5)); // 90 min hole

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, null,
                List.of(), frames, NOW, GAP_20MIN_MS);

        assertEquals(1, d.gapCount());
        assertEquals(90, d.longestGapMinutes());
    }

    @Test
    void offlineWhenSilenceExceedsThreeExpectedIntervals() {
        Instant lastFrame = NOW.minusSeconds(5 * 3600); // 5h silent, threshold 3×1h
        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, lastFrame,
                List.of(), List.of(), NOW, GAP_20MIN_MS);
        assertFalse(d.online());

        CommunicationQualityService.DeviceQuality fresh = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, NOW.minusSeconds(600),
                List.of(), List.of(), NOW, GAP_20MIN_MS);
        assertTrue(fresh.online());
    }

    @Test
    void signalDistributionMedianAndNegativeSnrRatio() {
        Instant base = NOW.minusSeconds(7200);
        List<CommunicationQualityService.FramePoint> frames = List.of(
                frame(base, -101, 5),
                frame(base.plusSeconds(600), -90, -2),
                frame(base.plusSeconds(1200), -90, -10));

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, null,
                List.of(), frames, NOW, GAP_20MIN_MS);

        assertEquals(-90.0, d.rssiMedian());
        assertEquals(66.7, d.snrNegativePercent());
        assertEquals(3, d.signalFrames());
    }

    @Test
    void downlinkCounterGrowthIsFirstToLast() {
        Instant t0 = NOW.minusSeconds(7200);
        Instant t1 = NOW.minusSeconds(3600);
        Instant t2 = NOW;
        List<CommunicationQualityService.SnapPoint> snaps = List.of(
                new CommunicationQualityService.SnapPoint(t0, true, 100L, 60L, 50L, "gw-1", null),
                new CommunicationQualityService.SnapPoint(t1, true, 105L, 61L, 50L, "gw-1", null),
                new CommunicationQualityService.SnapPoint(t2, true, 110L, 65L, 52L, "gw-1", null));

        CommunicationQualityService.DeviceQuality d = CommunicationQualityService.derive(
                "001a0103ff000319", "LIVESTOCK", "CAPSULE", 3600, null,
                snaps, List.of(), NOW, GAP_20MIN_MS);

        assertEquals(5L, d.nFCntDownGrowth());
        assertEquals(2L, d.aFCntDownGrowth());
        assertEquals(110L, d.fCntUp());
    }
}
