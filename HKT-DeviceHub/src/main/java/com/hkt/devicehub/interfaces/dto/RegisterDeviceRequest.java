package com.hkt.devicehub.interfaces.dto;

import jakarta.validation.constraints.NotBlank;
import jakarta.validation.constraints.Pattern;

import java.util.Map;

/**
 * Single-device registration request.
 */
public record RegisterDeviceRequest(
        @NotBlank
        @Pattern(regexp = "(?i)^[0-9a-f]{16}$", message = "devEui must be 16 hex chars")
        String devEui,
        @NotBlank
        @Pattern(regexp = "(?i)LIVESTOCK|PARKING", message = "project must be LIVESTOCK or PARKING")
        String project,
        String externalRef,
        @Pattern(regexp = "(?i)CAPSULE|TRACKER|GEOMAGNETIC",
                message = "deviceType must be CAPSULE, TRACKER or GEOMAGNETIC")
        String deviceType,
        Map<String, Object> capabilities) {
}
