package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Batched last_frame_at/last_event_at maintenance for the WS channel: frames
 * arrive per-message, but the timestamps only feed health computation and
 * console display, so updates are accumulated in memory (max ts per device
 * wins) and flushed on a fixed delay instead of one DB write per frame. The
 * flush always reads a fresh entity (findById) and persists only these two
 * timestamp fields on it — the REST backfill cursor is never touched here.
 */
@Component
@RequiredArgsConstructor
@Slf4j
public class LastFrameTracker {

    private final RegisteredDeviceRepository deviceRepository;

    /** deviceId → newest frame ts seen since last flush. */
    private final ConcurrentHashMap<Long, Long> pending = new ConcurrentHashMap<>();

    public void record(Long deviceId, long frameTs) {
        if (deviceId == null || frameTs <= 0) return;
        pending.merge(deviceId, frameTs, Math::max);
    }

    @Scheduled(fixedDelayString = "${devicehub.tb.last-frame-flush-ms:10000}")
    @Transactional
    public void flush() {
        if (pending.isEmpty()) return;
        List<Map.Entry<Long, Long>> batch = new ArrayList<>(pending.entrySet());
        pending.clear();
        for (Map.Entry<Long, Long> entry : batch) {
            try {
                RegisteredDevice device = deviceRepository.findById(entry.getKey()).orElse(null);
                if (device == null) continue;
                Instant frameAt = Instant.ofEpochMilli(entry.getValue());
                if (device.getLastFrameAt() == null || device.getLastFrameAt().isBefore(frameAt)) {
                    Instant previousFrameAt = device.getLastFrameAt();
                    Instant previousEventAt = device.getLastEventAt();
                    device.setLastFrameAt(frameAt);
                    device.setLastEventAt(frameAt);
                    try {
                        deviceRepository.save(device);
                    } catch (Exception e) {
                        // Undo the in-memory mutation, or the re-queued entry
                        // would no-op on retry (timestamps already "newer").
                        device.setLastFrameAt(previousFrameAt);
                        device.setLastEventAt(previousEventAt);
                        throw e;
                    }
                }
            } catch (Exception e) {
                // Re-queue so one bad row never drops the observation.
                pending.merge(entry.getKey(), entry.getValue(), Math::max);
                log.warn("[LastFrame] flush device {} failed: {}", entry.getKey(), e.getMessage());
            }
        }
    }
}
