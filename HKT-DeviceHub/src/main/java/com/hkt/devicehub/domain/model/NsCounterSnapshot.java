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

/**
 * NS device/detail counter snapshot taken once per poll cycle (communication
 * quality MVP). f_cnt_up differential between two snapshots gives the exact
 * uplink send count; paired with retained-frame counts it yields the loss
 * rate. ABP devices reset f_cnt to zero on reboot, so a non-positive delta is
 * recorded as-is and treated as a reset event (never as negative loss).
 * ns_ok=false rows document NS unavailability — unknown must not read as
 * healthy.
 */
@Entity
@Table(name = "ns_counter_snapshots")
@Getter
@Setter
public class NsCounterSnapshot {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "dev_eui", nullable = false)
    private String devEui;

    @Column(name = "f_cnt_up")
    private Long fCntUp;

    @Column(name = "n_f_cnt_down")
    private Long nFCntDown;

    @Column(name = "a_f_cnt_down")
    private Long aFCntDown;

    @Column(name = "latest_gateway")
    private String latestGateway;

    @Column(name = "last_seen")
    private Instant lastSeen;

    @Column(name = "ns_ok", nullable = false)
    private boolean nsOk;

    @Column(name = "error")
    private String error;

    @Column(name = "polled_at", nullable = false, updatable = false)
    private Instant polledAt;

    @PrePersist
    void onCreate() {
        if (polledAt == null) {
            polledAt = Instant.now();
        }
    }

    public static NsCounterSnapshot ok(String devEui, Long fCntUp, Long nFCntDown, Long aFCntDown,
                                       String latestGateway, Instant lastSeen) {
        NsCounterSnapshot snapshot = new NsCounterSnapshot();
        snapshot.setDevEui(devEui);
        snapshot.setFCntUp(fCntUp);
        snapshot.setNFCntDown(nFCntDown);
        snapshot.setAFCntDown(aFCntDown);
        snapshot.setLatestGateway(latestGateway);
        snapshot.setLastSeen(lastSeen);
        snapshot.setNsOk(true);
        return snapshot;
    }

    public static NsCounterSnapshot failed(String devEui, String error) {
        NsCounterSnapshot snapshot = new NsCounterSnapshot();
        snapshot.setDevEui(devEui);
        snapshot.setNsOk(false);
        snapshot.setError(error);
        return snapshot;
    }
}
