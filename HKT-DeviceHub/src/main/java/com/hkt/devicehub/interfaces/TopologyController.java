package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.MappingGapService;
import com.hkt.devicehub.application.TopologyService;
import com.hkt.devicehub.application.TopologyService.TopologyResponse;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

/**
 * Dashboard topology aggregation (R-10): NS reachability, TB auth, gateway
 * OC mapping, WS subscription consistency, MQ stats, per-project lag.
 */
@RestController
@RequestMapping("/api/v1/channels")
@Tag(name = "Channels", description = "Telemetry channel health & topology")
public class TopologyController {

    private final TopologyService topologyService;
    private final MappingGapService mappingGapService;

    public TopologyController(TopologyService topologyService, MappingGapService mappingGapService) {
        this.topologyService = topologyService;
        this.mappingGapService = mappingGapService;
    }

    @GetMapping("/topology")
    @Operation(summary = "End-to-end link topology probes (cached 30s)")
    public TopologyResponse topology() {
        return topologyService.topology();
    }

    @GetMapping("/mapping-gaps")
    @Operation(summary = "R-07 gateway mapping gap inspection: NS projects with "
            + "devices but no OC topic mapping (read-only, cached 30s)")
    public MappingGapService.MappingGapReport mappingGaps() {
        return mappingGapService.report();
    }
}
