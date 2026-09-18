package com.hkt.devicehub.infrastructure.mq;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.application.TelemetryFrame;
import lombok.extern.slf4j.Slf4j;
import org.apache.rocketmq.common.message.MessageConst;
import org.springframework.cloud.stream.function.StreamBridge;
import org.springframework.messaging.Message;
import org.springframework.messaging.support.MessageBuilder;
import org.springframework.stereotype.Component;

/**
 * Publishes normalized telemetry frames to RocketMQ via the Spring Cloud
 * Stream binder.
 * <p>
 * Contract: topic DEVICEHUB_TELEMETRY_FRAME (binding telemetryFrame-out-0),
 * tag = frame project (LIVESTOCK/PARKING), message key = frameId, payload =
 * TelemetryFrame JSON. Consumers must be idempotent on frameId — see
 * docs/telemetry-event-contract.md.
 */
@Component
@Slf4j
public class TelemetryEventPublisher {

    public static final String BINDING = "telemetryFrame-out-0";

    private final StreamBridge streamBridge;
    private final ObjectMapper objectMapper;

    public TelemetryEventPublisher(StreamBridge streamBridge, ObjectMapper objectMapper) {
        this.streamBridge = streamBridge;
        this.objectMapper = objectMapper;
    }

    public boolean publish(TelemetryFrame frame, String project) {
        try {
            Message<String> message = MessageBuilder
                    .withPayload(objectMapper.writeValueAsString(frame))
                    .setHeader(MessageConst.PROPERTY_KEYS, frame.frameId())
                    .setHeader(MessageConst.PROPERTY_TAGS, project)
                    .setHeader("rocketmq_keys", frame.frameId())
                    .setHeader("rocketmq_tags", project)
                    .build();
            boolean sent = streamBridge.send(BINDING, message);
            if (!sent) {
                log.warn("[MQ] publish returned false for frame {}", frame.frameId());
            }
            return sent;
        } catch (Exception e) {
            log.warn("[MQ] publish frame {} failed: {}", frame.frameId(), e.getMessage());
            return false;
        }
    }
}
