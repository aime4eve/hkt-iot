package com.hkt.devicehub.domain.model;

/**
 * Registration lifecycle of a device in DeviceHub.
 * PENDING: created locally, TB binding not yet confirmed.
 * ACTIVE: TB device bound, subscribed by the WS channel and polled by backfill.
 * FAILED: provisioning failed (see consecutiveFailures / logs).
 */
public enum RegistrationStatus {
    PENDING,
    ACTIVE,
    FAILED
}
