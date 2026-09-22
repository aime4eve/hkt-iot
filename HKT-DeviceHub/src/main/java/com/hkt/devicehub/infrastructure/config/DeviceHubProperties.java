package com.hkt.devicehub.infrastructure.config;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Console-facing DeviceHub settings (devicehub.*): deviceType → TB profile
 * mapping (R-02/R-11), deviceType → default report interval (R-06), and
 * topology probe targets.
 */
@Component
@ConfigurationProperties(prefix = "devicehub")
@Getter
@Setter
public class DeviceHubProperties {

    /** deviceType (CAPSULE/TRACKER/GEOMAGNETIC) → TB profile reference. */
    private Map<String, ProfileRef> profiles = new LinkedHashMap<>();

    /** deviceType → expected report interval seconds (initial defaults, R-06). */
    private Map<String, Integer> reportIntervals = new LinkedHashMap<>(Map.of(
            "CAPSULE", 14400,
            "TRACKER", 60,
            "GEOMAGNETIC", 3600));

    private Topology topology = new Topology();

    public int defaultIntervalSeconds(String deviceType) {
        if (deviceType != null) {
            Integer interval = reportIntervals.get(deviceType.toUpperCase(java.util.Locale.ROOT));
            if (interval != null) return interval;
        }
        return 3600;
    }

    @Getter
    @Setter
    public static class ProfileRef {
        private String name;
        private String id;
    }

    @Getter
    @Setter
    public static class Topology {
        /** NS TCP probe target (LoRaWAN MQTT port of the NS host). */
        private String nsHost = "172.17.201.15";
        private int nsPort = 1883;
        /** Probe result cache TTL to avoid hammering TB/NS on every refresh. */
        private long probeCacheMs = 30_000;
    }
}
