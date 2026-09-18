package com.hkt.devicehub.interfaces;

import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.Map;

/**
 * Phase-2 placeholder: downlink commands (TB server-side RPC) and device
 * scheduling are reserved but not implemented. Every command endpoint
 * answers 501 until the command module lands (see docs/roadmap-commands.md).
 */
@RestController
@RequestMapping("/api/v1/devices/{deviceId}/commands")
@Tag(name = "Commands (phase 2)", description = "Downlink command placeholder — 501 this phase")
public class CommandController {

    @RequestMapping("/**")
    @Operation(summary = "Reserved for phase 2 — always 501")
    public ResponseEntity<Map<String, Object>> notImplemented(@PathVariable Long deviceId) {
        return ResponseEntity.status(HttpStatus.NOT_IMPLEMENTED).body(Map.of(
                "status", 501,
                "error", "Not Implemented",
                "message", "Downlink commands and device scheduling are phase-2 scope",
                "deviceId", deviceId));
    }
}
