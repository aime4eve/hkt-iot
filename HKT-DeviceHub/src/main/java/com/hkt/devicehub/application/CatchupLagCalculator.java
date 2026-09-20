package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegisteredDevice;

import java.util.List;

/**
 * Catch-up lag semantics for channel health: a device is "lagging" only when
 * TB holds newer frames than the local cursor — max(lastFrameAt - cursor, 0).
 * Silent devices (no new frames) report 0 instead of now - cursor, so
 * low-frequency devices no longer masquerade as a stuck backfill.
 */
public final class CatchupLagCalculator {

    private CatchupLagCalculator() {}

    /**
     * Catch-up lag of one device in ms. Devices without a cursor are treated
     * as 0 (they either have no data — see noDataDevices — or have not been
     * polled yet; the first backfill cycle establishes the cursor).
     */
    public static long lagMs(RegisteredDevice device) {
        Long cursor = device.getTelemetryCursorMs();
        if (cursor == null || device.getLastFrameAt() == null) return 0;
        return Math.max(0, device.getLastFrameAt().toEpochMilli() - cursor);
    }

    /** True when the device has never produced nor received any frame. */
    public static boolean isNoData(RegisteredDevice device) {
        return device.getTelemetryCursorMs() == null && device.getLastFrameAt() == null;
    }

    /** Project aggregation: max catch-up lag (null when no device has data). */
    public static Long maxLagMs(List<RegisteredDevice> devices) {
        return devices.stream()
                .filter(d -> !isNoData(d))
                .map(CatchupLagCalculator::lagMs)
                .max(Long::compareTo)
                .orElse(null);
    }

    public static int noDataCount(List<RegisteredDevice> devices) {
        return (int) devices.stream().filter(CatchupLagCalculator::isNoData).count();
    }
}
