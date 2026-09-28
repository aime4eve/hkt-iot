package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.MappingChangeService;
import com.hkt.devicehub.application.MappingChangeService.MappingChangeReport;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

/**
 * Gateway mapping auto-fix (R-07 assist): appends a missing OC topic mapping
 * for one NS project — backup → append → write → read-back verify. dryRun
 * plans the change without touching TB; every real run stores a backup file
 * first. Manual fallback: docs/runbook-gateway-mapping.md.
 */
@RestController
@RequestMapping("/api/v1/gateway-mapping")
@Tag(name = "GatewayMapping", description = "OC gateway project mapping auto-fix")
public class GatewayMappingController {

    private final MappingChangeService mappingChangeService;

    public GatewayMappingController(MappingChangeService mappingChangeService) {
        this.mappingChangeService = mappingChangeService;
    }

    @PostMapping("/{nsProjectId}/apply")
    @Operation(summary = "Append the missing OC topic mapping for one NS project "
            + "(backup → append → write → verify → gateway restart); dryRun=true plans only, "
            + "reload=false skips the gateway restart")
    public MappingChangeReport apply(@PathVariable int nsProjectId,
                                     @RequestParam(defaultValue = "false") boolean dryRun,
                                     @RequestParam(defaultValue = "true") boolean reload) {
        return mappingChangeService.apply(nsProjectId, dryRun, reload);
    }

    @PostMapping("/reload")
    @Operation(summary = "Restart the TB gateway via RPC so connectors re-read their configs "
            + "(the gateway does not hot-reload shared-attribute changes)")
    public MappingChangeService.GatewayReloadReport reloadGateway() {
        return mappingChangeService.reloadGateway();
    }
}
