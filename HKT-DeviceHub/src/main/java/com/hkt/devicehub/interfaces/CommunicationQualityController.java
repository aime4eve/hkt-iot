package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.CommunicationQualityService;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

/**
 * Communication quality observability (MVP "solve existence first", research
 * note 2026-11-03). Read-only summary for the console card: per-device loss
 * rate (NS f_cnt_up differential vs retained frames), signal distribution,
 * silence gaps and online state. Cached server-side (devicehub.quality.
 * summary-cache-ms) — the dashboard polls every 30s against a 60s cache.
 */
@RestController
@RequestMapping("/api/v1/comms-quality")
@Tag(name = "CommunicationQuality", description = "Device↔gateway communication quality summary")
public class CommunicationQualityController {

    private final CommunicationQualityService service;

    public CommunicationQualityController(CommunicationQualityService service) {
        this.service = service;
    }

    @GetMapping("/summary")
    @Operation(summary = "Per-device loss rate, signal distribution, gaps and online state")
    public CommunicationQualityService.Summary summary() {
        return service.summary();
    }
}
