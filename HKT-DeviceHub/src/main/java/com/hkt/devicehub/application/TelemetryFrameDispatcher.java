package com.hkt.devicehub.application;

import com.hkt.devicehub.application.disposal.DisposalService;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import com.hkt.devicehub.infrastructure.mq.TelemetryEventPublisher;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.time.Instant;

/**
 * Single exit point for normalized frames from both channels (WS push and
 * REST backfill): publish to RocketMQ and record metrics. Fail-open per
 * frame — one bad frame never stalls the channel.
 * <p>
 * Deliberately persists nothing: the WS caller passes its long-lived
 * subscription-cached entity and this used to save it per frame, writing the
 * stale telemetry_cursor_ms snapshot back over the REST backfill's fresh
 * cursor (2026-11-02 stuck-cursor incident). Field ownership:
 * last_frame_at/last_event_at — LastFrameTracker (WS) and
 * TbRestBackfillChannel (REST, together with the cursor); the cursor is only
 * ever advanced by TbRestBackfillChannel.
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class TelemetryFrameDispatcher {

    private final TelemetryEventPublisher publisher;
    private final TelemetryChannelMetrics metrics;
    private final DisposalService disposalService;

    /**
     * @return true when the frame was handed to the broker; false means the
     *         caller (backfill) must not advance the cursor past this frame.
     */
    public boolean dispatch(RegisteredDevice device, TelemetryFrame frame, String channel) {
        boolean sent = publisher.publish(frame, device.getProject().name());
        metrics.recordFrame(channel, device.getProject().name(), sent ? "published" : "mq_failed");
        metrics.recordPushLatency(channel, frame.ts());
        if (sent) {
            // Observation auto-close hook (console §3.4 ③) — fail-open.
            try {
                disposalService.onFrameArrived(device.getDevEui(), Instant.ofEpochMilli(frame.ts()));
            } catch (Exception e) {
                log.warn("[Disposal] resolve hook for {} failed: {}", device.getDevEui(), e.getMessage());
            }
        }
        return sent;
    }
}
