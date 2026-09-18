package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.provisioning.DeviceProvisioningService;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.RegisterCommand;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.RegisterResult;
import com.hkt.devicehub.application.provisioning.DeviceProvisioningService.ReconcileReport;
import com.hkt.devicehub.interfaces.dto.RegisterDeviceRequest;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import jakarta.validation.Valid;
import jakarta.validation.constraints.NotEmpty;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
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

    public DeviceController(DeviceProvisioningService provisioningService) {
        this.provisioningService = provisioningService;
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
                request.externalRef(), request.capabilities());
    }
}
