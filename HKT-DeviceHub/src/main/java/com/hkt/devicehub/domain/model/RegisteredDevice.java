package com.hkt.devicehub.domain.model;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.EnumType;
import jakarta.persistence.Enumerated;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.PrePersist;
import jakarta.persistence.PreUpdate;
import jakarta.persistence.Table;
import jakarta.persistence.UniqueConstraint;
import lombok.Getter;
import lombok.Setter;
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;
import java.util.UUID;

/**
 * A device registered in DeviceHub: the single local fact that ties a LoRaWAN
 * DevEUI to a ThingsBoard device id and a business project. Owns the REST
 * backfill cursor (telemetryCursorMs) and channel health counters.
 */
@Entity
@Table(name = "registered_devices",
        uniqueConstraints = @UniqueConstraint(name = "uk_registered_devices_project_eui",
                columnNames = {"project", "dev_eui"}))
@Getter
@Setter
public class RegisteredDevice {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "dev_eui", nullable = false)
    private String devEui;

    @Column(name = "tb_device_id")
    private UUID tbDeviceId;

    @Enumerated(EnumType.STRING)
    @Column(name = "project", nullable = false)
    private DeviceProject project;

    @Column(name = "external_ref")
    private String externalRef;

    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "capabilities", columnDefinition = "jsonb")
    private String capabilities = "{}";

    @Column(name = "telemetry_cursor_ms")
    private Long telemetryCursorMs;

    @Column(name = "last_event_at")
    private Instant lastEventAt;

    @Column(name = "consecutive_failures", nullable = false)
    private int consecutiveFailures = 0;

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false)
    private RegistrationStatus status = RegistrationStatus.PENDING;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt;

    @Column(name = "updated_at", nullable = false)
    private Instant updatedAt;

    @PrePersist
    void onCreate() {
        Instant now = Instant.now();
        this.createdAt = now;
        this.updatedAt = now;
    }

    @PreUpdate
    void onUpdate() {
        this.updatedAt = Instant.now();
    }
}
