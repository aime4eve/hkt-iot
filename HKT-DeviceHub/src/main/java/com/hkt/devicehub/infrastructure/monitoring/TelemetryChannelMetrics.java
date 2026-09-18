package com.hkt.devicehub.infrastructure.monitoring;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.Timer;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Component;

import java.time.Clock;
import java.time.Duration;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;

/**
 * Telemetry channel metrics (Micrometer dotted names; Prometheus renders them
 * snake_case with _total / _seconds suffixes). Also backs the
 * /api/v1/channels/health endpoint via the in-memory state snapshot.
 */
@Component
public class TelemetryChannelMetrics {

    public static final String PROVIDER = "THINGSBOARD";
    public static final String CHANNEL_WS = "tb-ws";
    public static final String CHANNEL_REST = "tb-rest";

    private final MeterRegistry registry;
    private final Clock clock;
    private final Map<String, AtomicLong> channelStates = new ConcurrentHashMap<>();
    private final Map<String, AtomicLong> cursorLags = new ConcurrentHashMap<>();

    @Autowired
    public TelemetryChannelMetrics(MeterRegistry registry) {
        this(registry, Clock.systemUTC());
    }

    TelemetryChannelMetrics(MeterRegistry registry, Clock clock) {
        this.registry = registry;
        this.clock = clock;
        setChannelState(CHANNEL_WS, false);
        setChannelState(CHANNEL_REST, false);
    }

    public void recordFrame(String channel, String project, String result) {
        if (registry == null) return;
        Counter.builder("devicehub.iot.telemetry.frames")
                .description("Telemetry frames normalized and published")
                .tag("provider", PROVIDER)
                .tag("channel", safe(channel))
                .tag("project", safe(project))
                .tag("result", safe(result))
                .register(registry)
                .increment();
    }

    public void recordParseFailure(String channel) {
        recordFrame(channel, "unknown", "parse_failed");
    }

    public void setChannelState(String channel, boolean connected) {
        AtomicLong value = channelStates.computeIfAbsent(channel, ignored -> {
            AtomicLong state = new AtomicLong();
            if (registry != null) {
                Gauge.builder("devicehub.iot.channel.state", state, AtomicLong::doubleValue)
                        .description("Telemetry channel state: 1 connected, 0 disconnected/backoff")
                        .tag("provider", PROVIDER)
                        .tag("channel", safe(channel))
                        .strongReference(true)
                        .register(registry);
            }
            return state;
        });
        value.set(connected ? 1 : 0);
    }

    public void updateCursorLag(String devEui, long lagMillis) {
        AtomicLong value = cursorLags.computeIfAbsent(safe(devEui), ignored -> {
            AtomicLong lag = new AtomicLong();
            if (registry != null) {
                Gauge.builder("devicehub.iot.cursor.lag.seconds", lag,
                                stored -> stored.doubleValue() / 1000.0)
                        .description("Seconds a device cursor is behind now")
                        .tag("provider", PROVIDER)
                        .tag("dev_eui", safe(devEui))
                        .strongReference(true)
                        .register(registry);
            }
            return lag;
        });
        value.set(Math.max(0, lagMillis));
    }

    public void recordPushLatency(String channel, long providerEventTs) {
        if (registry == null || providerEventTs <= 0) return;
        Timer.builder("devicehub.iot.push.latency")
                .description("TB frame time to DeviceHub receive time")
                .tag("provider", PROVIDER)
                .tag("channel", safe(channel))
                .publishPercentileHistogram()
                .register(registry)
                .record(Duration.ofMillis(Math.max(0, clock.millis() - providerEventTs)));
    }

    public void recordReconnect(String channel) {
        if (registry == null) return;
        Counter.builder("devicehub.iot.reconnects")
                .description("Telemetry channel reconnections")
                .tag("provider", PROVIDER)
                .tag("channel", safe(channel))
                .register(registry)
                .increment();
    }

    /** Snapshot for ChannelHealthController. */
    public Map<String, Boolean> channelStateSnapshot() {
        Map<String, Boolean> snapshot = new LinkedHashMap<>();
        channelStates.forEach((channel, state) -> snapshot.put(channel, state.get() == 1));
        return snapshot;
    }

    private static String safe(String value) {
        return value == null || value.isBlank() ? "-" : value;
    }
}
