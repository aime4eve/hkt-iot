package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.repository.TelemetryFrameRecordRepository;
import com.hkt.devicehub.infrastructure.config.QualityProperties;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.time.Instant;
import java.util.UUID;
import java.util.concurrent.ConcurrentLinkedDeque;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Per-frame retention for communication quality metrics. The dispatcher hands
 * every normalized frame here; frames are buffered in memory and flushed in
 * batches with {@code ON CONFLICT (frame_id) DO NOTHING}, so WS/REST/backup
 * double delivery collapses to one row.
 * <p>
 * Mirrors {@link LastFrameTracker} discipline, plus the hot-path guarantees
 * required after the 2026-11-02 stuck-cursor incident: offer() is O(1),
 * never throws, and this class holds no entity references — the frame record
 * is built from the immutable event payload only. When the buffer cap is hit
 * the oldest frame is dropped and counted (never silently: see
 * {@link Status#droppedFrames}).
 */
@Component
@RequiredArgsConstructor
@Slf4j
public class FrameRetentionRecorder {

    private final TelemetryFrameRecordRepository repository;
    private final QualityProperties properties;

    private final ConcurrentLinkedDeque<PendingFrame> pending = new ConcurrentLinkedDeque<>();
    /** Approximate queue depth; races are acceptable, it only bounds memory. */
    private final AtomicInteger approxSize = new AtomicInteger();
    private final AtomicBoolean flushBusy = new AtomicBoolean();

    private final AtomicLong insertedFrames = new AtomicLong();
    private final AtomicLong duplicateFrames = new AtomicLong();
    private final AtomicLong droppedFrames = new AtomicLong();
    private volatile String lastError;

    public void offer(String project, TelemetryFrame frame) {
        if (!properties.isFrameRetentionEnabled() || frame == null) return;
        try {
            if (approxSize.get() >= properties.getMaxPendingFrames()) {
                PendingFrame oldest = pending.pollFirst();
                if (oldest != null) {
                    approxSize.decrementAndGet();
                    droppedFrames.incrementAndGet();
                }
            }
            pending.offerLast(PendingFrame.of(project, frame));
            approxSize.incrementAndGet();
        } catch (Exception e) {
            // Retention must never take the dispatch path down.
            log.warn("[FrameRetention] offer failed: {}", e.getMessage());
        }
    }

    @Scheduled(fixedDelayString = "${devicehub.quality.flush-interval-ms:5000}")
    public void flush() {
        if (!properties.isFrameRetentionEnabled() || approxSize.get() == 0) return;
        // Single-flush gate: overlapping runs would only reorder inserts.
        if (!flushBusy.compareAndSet(false, true)) return;
        try {
            int drained = 0;
            int budget = 500;
            while (budget-- > 0) {
                PendingFrame entry = pending.pollFirst();
                if (entry == null) break;
                approxSize.decrementAndGet();
                try {
                    int inserted = repository.insertIgnoreConflict(entry.frameId(), entry.project(),
                            entry.devEui(), entry.tbDeviceId(), entry.ts(), entry.dataHex(),
                            entry.rssi(), entry.snr(), entry.gatewayId());
                    if (inserted > 0) {
                        insertedFrames.incrementAndGet();
                    } else {
                        duplicateFrames.incrementAndGet();
                    }
                    drained++;
                } catch (Exception e) {
                    // Re-queue at the head and stop draining: DB down must not
                    // silently drop the batch.
                    pending.offerFirst(entry);
                    approxSize.incrementAndGet();
                    lastError = e.getMessage();
                    log.warn("[FrameRetention] flush failed, {} requeued, drained {} before error: {}",
                            approxSize.get(), drained, e.getMessage());
                    return;
                }
            }
        } finally {
            flushBusy.set(false);
        }
    }

    public Status status() {
        return new Status(properties.isFrameRetentionEnabled(), approxSize.get(),
                insertedFrames.get(), duplicateFrames.get(), droppedFrames.get(), lastError);
    }

    public record Status(boolean enabled, int pending, long insertedFrames, long duplicateFrames,
                         long droppedFrames, String lastError) {}

    record PendingFrame(UUID frameId, String project, String devEui, String tbDeviceId,
                        Instant ts, String dataHex, Integer rssi, Integer snr, String gatewayId) {

        static PendingFrame of(String project, TelemetryFrame frame) {
            return new PendingFrame(UUID.fromString(frame.frameId()), project,
                    frame.devEui(), frame.tbDeviceId(), Instant.ofEpochMilli(frame.ts()),
                    frame.dataHex(), frame.rssi(), frame.snr(), frame.gatewayId());
        }
    }
}
