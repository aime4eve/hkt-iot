package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.JsonNode;
import com.hkt.devicehub.application.CatchupLagCalculator;
import com.hkt.devicehub.application.TelemetryFrame;
import com.hkt.devicehub.application.TelemetryFrameDispatcher;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.List;

/**
 * ThingsBoard REST backfill channel (fallback). On a fixed delay it pages each
 * ACTIVE device's timeseries with a persistent per-device cursor
 * (registered_devices.telemetry_cursor_ms) and publishes what the WS channel
 * missed.
 * <p>
 * Cursor discipline (livestock TbTelemetryChannel blueprint): successful
 * prefix semantics — a failed frame is never jumped over; undecodable frames
 * reported by the normalizer are skipped on purpose and the cursor passes
 * them; limit truncation continues from the boundary frame onward.
 */
@Component
@ConditionalOnProperty(name = "devicehub.tb.enabled", havingValue = "true")
@RequiredArgsConstructor
@Slf4j
public class TbRestBackfillChannel {

    private static final int MAX_PAGES_PER_CYCLE = 50;

    private final TbProperties properties;
    private final TbClient tbClient;
    private final TbFrameNormalizer normalizer;
    private final TelemetryFrameDispatcher dispatcher;
    private final RegisteredDeviceRepository deviceRepository;
    private final TelemetryChannelMetrics metrics;
    private final Clock clock = Clock.systemUTC();

    @Scheduled(fixedDelayString = "${devicehub.tb.poll-interval-ms:300000}", initialDelay = 10_000)
    public void poll() {
        List<RegisteredDevice> active = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        if (active.isEmpty()) return;
        log.info("[TbRest] backfilling {} ACTIVE device(s)", active.size());
        for (RegisteredDevice device : active) {
            try {
                pollDevice(device);
                metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_REST, true);
            } catch (Exception e) {
                metrics.setChannelState(TelemetryChannelMetrics.CHANNEL_REST, false);
                log.warn("[TbRest] device {} cycle failed: {}", device.getDevEui(), e.getMessage());
            }
        }
    }

    void pollDevice(RegisteredDevice device) {
        if (device.getTbDeviceId() == null) {
            log.debug("[TbRest] device {} has no TB id, skipping", device.getDevEui());
            return;
        }
        String tbDeviceId = device.getTbDeviceId().toString();
        TbFrameNormalizer.DeviceRef ref =
                new TbFrameNormalizer.DeviceRef(tbDeviceId, device.getDevEui(),
                        device.getProject().name());

        long now = clock.millis();
        long pageStart = device.getTelemetryCursorMs() != null
                ? device.getTelemetryCursorMs() + 1
                : now - Duration.ofDays(properties.getLookbackDays()).toMillis();
        long overallMaxTs = Long.MIN_VALUE;
        long skippedMaxTs = Long.MIN_VALUE;
        int pages = 0;
        boolean failed = false;

        while (pages < MAX_PAGES_PER_CYCLE) {
            TbFrameNormalizer.ParseResult parsed;
            try {
                JsonNode timeseries = tbClient.fetchTimeseries(
                        tbDeviceId, pageStart, now, properties.getBatchSize());
                parsed = normalizer.normalizePage(ref, timeseries);
                // Undecodable frames are dropped on purpose; the cursor must
                // pass them or the same page is re-fetched forever.
                if (!parsed.skippedTs().isEmpty()) {
                    skippedMaxTs = Math.max(skippedMaxTs,
                            parsed.skippedTs().get(parsed.skippedTs().size() - 1));
                    metrics.recordParseFailure(TelemetryChannelMetrics.CHANNEL_REST);
                    log.warn("[TbRest] device {} skipping {} undecodable frame(s), latest at {}",
                            device.getDevEui(), parsed.skippedTs().size(), skippedMaxTs);
                }
            } catch (Exception e) {
                log.warn("[TbRest] device {} page failed at {}: {}",
                        device.getDevEui(), pageStart, e.getMessage());
                failed = true;
                break;
            }
            if (parsed.frames().isEmpty()) break;

            long batchMaxTs = Long.MIN_VALUE;
            for (TelemetryFrame frame : parsed.frames()) {
                // Continuation pages re-fetch the boundary frame (startTs is
                // inclusive); skip already-published frames.
                if (pages > 0 && frame.ts() <= pageStart) continue;
                if (!dispatcher.dispatch(device, frame, TelemetryChannelMetrics.CHANNEL_REST)) {
                    failed = true;
                    break;
                }
                batchMaxTs = Math.max(batchMaxTs, frame.ts());
            }
            pages++;
            overallMaxTs = Math.max(overallMaxTs, batchMaxTs);
            if (failed) break;
            if (batchMaxTs == Long.MIN_VALUE) break;
            if (parsed.frames().size() < properties.getBatchSize()) break;
            // Limit truncation: continue from the boundary frame onward.
            pageStart = batchMaxTs;
        }

        // Cheap DESC probe for the newest TB-side frame: feeds the catch-up
        // lag metric (max(last_frame_at - cursor, 0)). Probe failure must not
        // fail the cycle — the cursor discipline above is untouched.
        try {
            Instant latestOnTb = tbClient.fetchLatestTelemetryTs(tbDeviceId);
            if (latestOnTb != null && (device.getLastFrameAt() == null
                    || device.getLastFrameAt().isBefore(latestOnTb))) {
                device.setLastFrameAt(latestOnTb);
            }
        } catch (Exception e) {
            log.debug("[TbRest] device {} latest-frame probe failed: {}",
                    device.getDevEui(), e.getMessage());
        }

        if (failed) {
            device.setConsecutiveFailures(device.getConsecutiveFailures() + 1);
            // Keep the successful prefix cursor; never jump past a failed frame.
            if (overallMaxTs != Long.MIN_VALUE) {
                device.setTelemetryCursorMs(overallMaxTs);
                device.setLastEventAt(Instant.ofEpochMilli(overallMaxTs));
            }
        } else {
            device.setConsecutiveFailures(0);
            long processedMaxTs = Math.max(overallMaxTs, skippedMaxTs);
            if (processedMaxTs != Long.MIN_VALUE) {
                device.setTelemetryCursorMs(processedMaxTs);
                device.setLastEventAt(Instant.ofEpochMilli(processedMaxTs));
                log.info("[TbRest] device {} cursor advanced to {}",
                        device.getDevEui(), processedMaxTs);
            }
        }
        metrics.updateCursorLag(device.getDevEui(), CatchupLagCalculator.lagMs(device));
        deviceRepository.save(device);
    }
}
