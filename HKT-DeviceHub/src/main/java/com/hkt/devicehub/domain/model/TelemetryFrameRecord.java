package com.hkt.devicehub.domain.model;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.PrePersist;
import jakarta.persistence.Table;
import lombok.Getter;
import lombok.Setter;

import java.time.Instant;
import java.util.UUID;

/**
 * Retained telemetry frame (communication quality MVP). One row per unique
 * frameId — the WS channel, REST backfill and the backup channel can deliver
 * the same frame more than once, and the unique index collapses those to a
 * single row. This is evidence storage for loss-rate / signal / gap metrics,
 * not a second event bus: the MQ contract is untouched.
 */
@Entity
@Table(name = "telemetry_frame_records")
@Getter
@Setter
public class TelemetryFrameRecord {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    /** Stable dedup key (same UUID as the MQ contract, tbDeviceId:ts based). */
    @Column(name = "frame_id", nullable = false, unique = true)
    private UUID frameId;

    @Column(name = "project", nullable = false)
    private String project;

    @Column(name = "dev_eui", nullable = false)
    private String devEui;

    @Column(name = "tb_device_id")
    private String tbDeviceId;

    @Column(name = "ts", nullable = false)
    private Instant ts;

    @Column(name = "data_hex")
    private String dataHex;

    @Column(name = "rssi")
    private Integer rssi;

    @Column(name = "snr")
    private Integer snr;

    @Column(name = "gateway_id")
    private String gatewayId;

    @Column(name = "received_at", nullable = false, updatable = false)
    private Instant receivedAt;

    @PrePersist
    void onCreate() {
        if (receivedAt == null) {
            receivedAt = Instant.now();
        }
    }
}
