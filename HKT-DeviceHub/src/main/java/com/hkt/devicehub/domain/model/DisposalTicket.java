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
import lombok.Getter;
import lombok.Setter;
import org.hibernate.annotations.JdbcTypeCode;
import org.hibernate.type.SqlTypes;

import java.time.Instant;

/**
 * No-data device disposal ticket (console v1.5 §3.4). State machine:
 * PENDING_ASSIGN → APPROVING → PROCESSING → OBSERVING → RESOLVED,
 * rejections fall back to PENDING_ASSIGN, DECOMMISSIONED is a terminal exit
 * from any non-terminal state. timeline is a JSON array of
 * {ts, action, source, detail} entries, appended on every transition.
 */
@Entity
@Table(name = "device_disposal_tickets")
@Getter
@Setter
public class DisposalTicket {

    public enum Severity { NOTICE, CRITICAL }

    public enum Category { FIELD, PLATFORM }

    public enum Status { PENDING_ASSIGN, APPROVING, PROCESSING, OBSERVING, RESOLVED, DECOMMISSIONED }

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "dev_eui", nullable = false)
    private String devEui;

    @Enumerated(EnumType.STRING)
    @Column(name = "project", nullable = false)
    private DeviceProject project;

    @Enumerated(EnumType.STRING)
    @Column(name = "severity", nullable = false)
    private Severity severity = Severity.NOTICE;

    @Enumerated(EnumType.STRING)
    @Column(name = "category")
    private Category category;

    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "triage_evidence", columnDefinition = "jsonb")
    private String triageEvidence = "{}";

    @Enumerated(EnumType.STRING)
    @Column(name = "status", nullable = false)
    private Status status = Status.PENDING_ASSIGN;

    @Column(name = "assignee")
    private String assignee;

    @Column(name = "dingtalk_instance_id")
    private String dingtalkInstanceId;

    @Column(name = "decommission_reason")
    private String decommissionReason;

    @Column(name = "first_frame_at")
    private Instant firstFrameAt;

    @JdbcTypeCode(SqlTypes.JSON)
    @Column(name = "timeline", columnDefinition = "jsonb")
    private String timeline = "[]";

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
