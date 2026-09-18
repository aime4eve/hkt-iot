package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import com.hkt.devicehub.infrastructure.mq.TelemetryEventPublisher;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Propagation;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;

/**
 * Single exit point for normalized frames from both channels (WS push and
 * REST backfill): publish to RocketMQ, bump lastEventAt, record metrics.
 * Fail-open per frame — one bad frame never stalls the channel.
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class TelemetryFrameDispatcher {

    private final TelemetryEventPublisher publisher;
    private final RegisteredDeviceRepository deviceRepository;
    private final TelemetryChannelMetrics metrics;

    /**
     * @return true when the frame was handed to the broker; false means the
     *         caller (backfill) must not advance the cursor past this frame.
     */
    @Transactional(propagation = Propagation.REQUIRES_NEW)
    public boolean dispatch(RegisteredDevice device, TelemetryFrame frame, String channel) {
        boolean sent = publisher.publish(frame, device.getProject().name());
        metrics.recordFrame(channel, device.getProject().name(), sent ? "published" : "mq_failed");
        metrics.recordPushLatency(channel, frame.ts());
        if (sent) {
            device.setLastEventAt(Instant.ofEpochMilli(frame.ts()));
            deviceRepository.save(device);
        }
        return sent;
    }
}
