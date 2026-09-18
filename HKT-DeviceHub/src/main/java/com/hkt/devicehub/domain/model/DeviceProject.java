package com.hkt.devicehub.domain.model;

/**
 * Business projects that own devices registered in DeviceHub.
 * Doubles as the RocketMQ tag for telemetry frames.
 */
public enum DeviceProject {
    LIVESTOCK,
    PARKING
}
