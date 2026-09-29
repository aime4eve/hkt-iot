package com.hkt.devicehub.infrastructure.mqtt;

import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * NS→TB backup telemetry channel configuration (devicehub.backup-channel.*).
 * Docs: ns-direct-tb-backup-channel.md. Broker credentials are NOT stored
 * here — they are read at connect time from the TB gateway device's shared
 * attribute (OC.configurationJson.broker.security), same source the bridge
 * uses; config host/port are only the bootstrap fallback.
 */
@Component
@ConfigurationProperties(prefix = "devicehub.backup-channel")
@Getter
@Setter
public class BackupChannelProperties {

    /** Off by default — the channel is opt-in until the Phase 1 pilot verdict. */
    private boolean enabled = false;
    /**
     * Must be unique on the broker and MUST NOT collide with the live bridge
     * clientId (gw_ prefix): same-id reconnects kick the other session.
     */
    private String clientId = "dhbk-backup-01";
    /** Bootstrap broker coordinates; OC attribute wins when present. */
    private String brokerHost = "172.17.201.15";
    private int brokerPort = 1883;
    private String topicFilter = "org/1/project/#";
    private int qos = 1;
    /** Pilot NS projects; empty = COUNT_ONLY (frames are counted, never written). */
    private List<Long> projectWhitelist = List.of();
    /** Write queue capacity; overflow is counted (queueOverflow), never blocking. */
    private int maxQueue = 2000;
    /** Registry match cache TTL in ms (negative results cached too). */
    private long matchCacheTtlMs = 300_000;

    public void setProjectWhitelist(List<Long> whitelist) {
        this.projectWhitelist = whitelist == null ? List.of()
                : whitelist.stream().filter(java.util.Objects::nonNull).collect(Collectors.toList());
    }

    public java.util.Set<Long> whitelistSet() {
        return projectWhitelist.stream().collect(Collectors.toUnmodifiableSet());
    }

    public static BackupChannelProperties fromRaw(String clientId, String host, int port,
                                                  String topicFilter, int qos,
                                                  String whitelistCsv, int maxQueue) {
        BackupChannelProperties p = new BackupChannelProperties();
        p.setClientId(clientId);
        p.setBrokerHost(host);
        p.setBrokerPort(port);
        p.setTopicFilter(topicFilter);
        p.setQos(qos);
        p.setProjectWhitelist(whitelistCsv == null || whitelistCsv.isBlank() ? List.of()
                : Stream.of(whitelistCsv.split(",")).map(String::trim)
                        .filter(s -> !s.isEmpty()).map(Long::valueOf).toList());
        p.setMaxQueue(maxQueue);
        return p;
    }
}
