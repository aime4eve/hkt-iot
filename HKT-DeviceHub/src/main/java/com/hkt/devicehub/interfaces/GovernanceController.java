package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.GovernanceService;
import com.hkt.devicehub.application.GovernanceService.ConflictRow;
import com.hkt.devicehub.domain.model.DeviceProject;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.DeleteMapping;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

/**
 * Governance center (R-04): duplicate-name conflicts and the guarded
 * empty-copy delete (server-side zero-telemetry check + audit log).
 */
@RestController
@RequestMapping("/api/v1/devices")
@Tag(name = "Governance", description = "Duplicate governance (R-04)")
public class GovernanceController {

    private final GovernanceService governanceService;

    public GovernanceController(GovernanceService governanceService) {
        this.governanceService = governanceService;
    }

    @GetMapping("/conflicts")
    @Operation(summary = "TB duplicate-name conflicts with copy-by-copy evidence")
    public List<ConflictRow> conflicts(@RequestParam DeviceProject project) {
        return governanceService.conflicts(project);
    }

    @DeleteMapping("/tb/{tbDeviceId}")
    @Operation(summary = "Delete an empty TB copy — refuses 422 when the device has telemetry or is bound")
    public ResponseEntity<Map<String, Object>> deleteEmptyCopy(
            @PathVariable String tbDeviceId,
            @RequestHeader(name = "X-Operator", required = false) String operator) {
        governanceService.deleteEmptyCopy(tbDeviceId, operator);
        return ResponseEntity.ok(Map.of("deleted", tbDeviceId));
    }
}
