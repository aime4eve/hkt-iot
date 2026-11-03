package com.hkt.devicehub.infrastructure.config;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

/**
 * Communication quality MVP settings (devicehub.quality.*). Defaults live in
 * application.yml; nothing here requires Nacos — env overrides follow the
 * compose whitelist like every other group.
 */
@Component
@ConfigurationProperties(prefix = "devicehub.quality")
@Getter
@Setter
public class QualityProperties {

    /** Master switch for frame retention; off = offer() drops and flush no-ops. */
    private boolean frameRetentionEnabled = true;
    /** Frame flush cadence; batches are small (periodic-reporting fleet). */
    private long flushIntervalMs = 5_000;
    /** In-memory pending cap before oldest-frame dropping starts (counted). */
    private int maxPendingFrames = 20_000;
    /** NS device/detail poll cadence: 15 min default (loss differential granularity). */
    private long nsPollIntervalMs = 900_000;
    private long nsPollInitialDelayMs = 30_000;
    /** Console summary cache TTL (query-time metrics over the whole fleet). */
    private long summaryCacheMs = 60_000;
    /** Metric computation window for the summary. */
    private int summaryWindowHours = 48;
    /** Consecutive-frame silence counted as an interruption (default 20 min). */
    private long gapThresholdMs = 1_200_000;
}
