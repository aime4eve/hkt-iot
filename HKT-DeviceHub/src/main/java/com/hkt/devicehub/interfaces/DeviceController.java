package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.DeviceQueryService;
import com.hkt.devicehub.application.DeviceQueryService.DeviceDetail;
import com.hkt.devicehub.application.DeviceQueryService.DeviceListItem;
import com.hkt.devicehub.application.PreflightService;
import com.hkt.devicehub.application.PreflightService.PreflightReport;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.RegisterCommand;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.RegisterResult;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.ReconcileReport;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.interfaces.dto.RegisterDeviceRequest;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotEmpty;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.PageRequest;
import org.springframework.data.domain.Sort;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;

/**
 * Device registration / batch import / reconciliation API.
 */
@RestController
@RequestMapping("/api/v1/devices")
@Tag(name = "Devices", description = "Device provisioning against ThingsBoard")
public class DeviceController {

    private final DeviceProvisioningService provisioningService;
    private final DeviceQueryService queryService;
    private final PreflightService preflightService;

    public DeviceController(DeviceProvisioningService provisioningService,
                            DeviceQueryService queryService,
                            PreflightService preflightService) {
        this.provisioningService = provisioningService;
        this.queryService = queryService;
        this.preflightService = preflightService;
    }

    @GetMapping
    @Operation(summary = "Ledger list: project/status filters + q fuzzy (devEui/externalRef/deviceType)")
    public Page<DeviceListItem> list(
            @RequestParam(required = false) DeviceProject project,
            @RequestParam(required = false) RegistrationStatus status,
            @RequestParam(required = false) String q,
            @RequestParam(defaultValue = "0") int page,
            @RequestParam(defaultValue = "20") int size) {
        return queryService.list(project, status, q,
                PageRequest.of(page, Math.min(size, 200), Sort.by("id")));
    }

    @GetMapping("/{id}")
    @Operation(summary = "Device detail + last 10 TB frames (null artifacts filtered)")
    public DeviceDetail detail(@PathVariable long id) {
        return queryService.detail(id);
    }

    @GetMapping("/preflight")
    @Operation(summary = "Five-check onboarding preflight (R-01)")
    public PreflightReport preflight(@RequestParam String devEui,
                                     @RequestParam(required = false) String deviceType) {
        return preflightService.preflight(devEui, deviceType);
    }

    @PostMapping("/register")
    @Operation(summary = "Register one device: create/resolve the TB device, bind, activate")
    public ResponseEntity<RegisterResult> register(
            @Valid @RequestBody RegisterDeviceRequest request) {
        RegisterResult result = provisioningService.register(toCommand(request));
        return "REGISTERED".equals(result.result()) || "ALREADY_REGISTERED".equals(result.result())
                ? ResponseEntity.ok(result)
                : ResponseEntity.unprocessableEntity().body(result);
    }

    @PostMapping("/import")
    @Operation(summary = "Batch register devices")
    public List<RegisterResult> importBatch(
            @NotEmpty @RequestBody List<@Valid RegisterDeviceRequest> requests) {
        return provisioningService.importBatch(requests.stream().map(this::toCommand).toList());
    }

    @GetMapping("/reconcile")
    @Operation(summary = "Diff TB inventory against registered_devices for a project")
    public ReconcileReport reconcile(@RequestParam String project) {
        return provisioningService.reconcile(project);
    }

    private RegisterCommand toCommand(RegisterDeviceRequest request) {
        return new RegisterCommand(request.devEui(), request.project(),
                request.externalRef(), request.deviceType(), request.capabilities());
    }
}
