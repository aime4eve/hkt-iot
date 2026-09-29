package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.infrastructure.mqtt.BackupChannelService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/**
 * Backup telemetry channel observability (docs/ns-direct-tb-backup-channel.md
 * §5 Phase 1). The status payload feeds the console "备份通道" card: run
 * state, subscription confirmation, today's frame count, per-path discard
 * counters, unknown-device counter. Every discard is counted here — the
 * channel must not become a new silent-dropping point.
 */
@RestController
@RequestMapping("/api/v1/backup-channel")
@Tag(name = "BackupChannel", description = "NS→TB backup telemetry channel status and runtime switch")
public class BackupChannelController {

    private final BackupChannelService service;

    public BackupChannelController(BackupChannelService service) {
        this.service = service;
    }

    @GetMapping("/status")
    @Operation(summary = "Run state, subscription confirmation, counters and last errors")
    public BackupChannelService.Status status() {
        return service.status();
    }

    @PostMapping("/switch")
    @Operation(summary = "Runtime enable/disable (config devicehub.backup-channel.enabled "
            + "still seeds the boot default); disabling keeps the broker session queued")
    public BackupChannelService.Status switchChannel(@RequestParam boolean enabled) {
        service.switchEnabled(enabled);
        return service.status();
    }
}
