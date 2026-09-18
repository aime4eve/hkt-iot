package com.hkt.devicehub.application.provisioning;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;
import java.util.regex.Pattern;

/**
 * Generic device provisioning orchestration (livestock
 * TbDeviceProvisioningService blueprint, stripped of livestock couplings:
 * no farm / audit_log / NS hard dependency).
 * <p>
 * Registration state machine: PENDING → ACTIVE (TB device resolved or
 * created and bound) / FAILED. Reconcile diffs TB inventory against
 * registered_devices per project: TB side has it but local does not
 * (TB_ONLY), local has it but TB does not (TB_MISSING), or CONSISTENT.
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class DeviceProvisioningService {

    private static final Pattern EUI_PATTERN = Pattern.compile("^[0-9a-f]{16}$");

    private final TbClient tbClient;
    private final RegisteredDeviceRepository deviceRepository;
    private final NsReconciliationPort nsReconciliation;
    private final ObjectMapper objectMapper;

    public record RegisterCommand(String devEui, String project, String externalRef,
                                  Map<String, Object> capabilities) {}

    public record RegisterResult(Long id, String devEui, String project, String tbDeviceId,
                                 String status, String result) {}

    public record ReconcileRow(String devEui, String tbDeviceId, String status,
                               List<String> differenceCodes) {}

    public record ReconcileReport(String project, List<ReconcileRow> rows, Map<String, Long> counts) {}

    @Transactional
    public RegisterResult register(RegisterCommand command) {
        String eui = requireEui(command.devEui());
        DeviceProject project = requireProject(command.project());

        RegisteredDevice device = deviceRepository.findByProjectAndDevEui(project, eui)
                .orElse(null);
        if (device != null && device.getStatus() == RegistrationStatus.ACTIVE) {
            return new RegisterResult(device.getId(), eui, project.name(),
                    String.valueOf(device.getTbDeviceId()), device.getStatus().name(),
                    "ALREADY_REGISTERED");
        }
        if (device == null) {
            device = new RegisteredDevice();
            device.setDevEui(eui);
            device.setProject(project);
            device.setStatus(RegistrationStatus.PENDING);
        }
        if (command.externalRef() != null) device.setExternalRef(command.externalRef());
        if (command.capabilities() != null) {
            device.setCapabilities(writeCapabilities(command.capabilities()));
        }

        try {
            String tbDeviceId = resolveOrCreateTbDevice(eui);
            device.setTbDeviceId(UUID.fromString(tbDeviceId));
            device.setStatus(RegistrationStatus.ACTIVE);
            device.setConsecutiveFailures(0);
            device = deviceRepository.save(device);
            // Optional NS isolation step (no-op by default).
            nsReconciliation.afterRegister(device);
            log.info("[Provisioning] registered {} ({}) -> TB {}", eui, project, tbDeviceId);
            return new RegisterResult(device.getId(), eui, project.name(), tbDeviceId,
                    device.getStatus().name(), "REGISTERED");
        } catch (Exception e) {
            device.setStatus(RegistrationStatus.FAILED);
            device.setConsecutiveFailures(device.getConsecutiveFailures() + 1);
            deviceRepository.save(device);
            log.warn("[Provisioning] register {} ({}) failed: {}", eui, project, e.getMessage());
            return new RegisterResult(device.getId(), eui, project.name(),
                    device.getTbDeviceId() == null ? null : device.getTbDeviceId().toString(),
                    device.getStatus().name(), "FAILED: " + e.getMessage());
        }
    }

    @Transactional
    public List<RegisterResult> importBatch(List<RegisterCommand> items) {
        List<RegisterResult> results = new ArrayList<>();
        for (RegisterCommand item : items) {
            try {
                results.add(register(item));
            } catch (Exception e) {
                results.add(new RegisterResult(null, item.devEui(), item.project(), null,
                        "FAILED", "FAILED: " + e.getMessage()));
            }
        }
        return results;
    }

    @Transactional(readOnly = true)
    public ReconcileReport reconcile(String project) {
        DeviceProject deviceProject = requireProject(project);
        List<RegisteredDevice> local = deviceRepository.findByProject(deviceProject);
        Map<String, RegisteredDevice> localByEui = new LinkedHashMap<>();
        local.forEach(d -> localByEui.put(d.getDevEui(), d));

        List<TbClient.TbDeviceView> tbDevices = tbClient.listTenantDevices();
        Map<String, TbClient.TbDeviceView> tbByEui = new LinkedHashMap<>();
        for (TbClient.TbDeviceView view : tbDevices) {
            String name = view.name() == null ? "" : view.name().toLowerCase(Locale.ROOT);
            if (EUI_PATTERN.matcher(name).matches()) {
                tbByEui.putIfAbsent(name, view);
            }
        }

        List<ReconcileRow> rows = new ArrayList<>();
        Map<String, Long> counts = new LinkedHashMap<>();
        for (RegisteredDevice device : local) {
            List<String> diffs = new ArrayList<>();
            TbClient.TbDeviceView tb = tbByEui.get(device.getDevEui());
            if (tb == null) {
                diffs.add("TB_MISSING");
            } else if (device.getTbDeviceId() == null
                    || !device.getTbDeviceId().toString().equals(tb.id())) {
                diffs.add("TB_IDENTITY_CONFLICT");
            }
            if (device.getStatus() != RegistrationStatus.ACTIVE) {
                diffs.add("LOCAL_NOT_ACTIVE");
            }
            diffs.addAll(nsReconciliation.reconcileDifferences(device));
            if (diffs.isEmpty()) diffs.add("CONSISTENT");
            rows.add(new ReconcileRow(device.getDevEui(),
                    device.getTbDeviceId() == null ? null : device.getTbDeviceId().toString(),
                    device.getStatus().name(), diffs));
        }
        for (Map.Entry<String, TbClient.TbDeviceView> entry : tbByEui.entrySet()) {
            if (!localByEui.containsKey(entry.getKey())) {
                rows.add(new ReconcileRow(entry.getKey(), entry.getValue().id(), null,
                        List.of("TB_ONLY")));
            }
        }
        rows.forEach(row -> row.differenceCodes()
                .forEach(code -> counts.merge(code, 1L, Long::sum)));
        counts.put("LOCAL_TOTAL", (long) local.size());
        return new ReconcileReport(deviceProject.name(), rows, counts);
    }

    private String resolveOrCreateTbDevice(String eui) {
        List<TbClient.TbDeviceView> matches = tbClient.findDevices(eui);
        if (matches.isEmpty()) {
            return tbClient.createDevice(eui);
        }
        if (matches.size() > 1) {
            throw new IllegalStateException("Ambiguous TB devices for EUI " + eui + ": "
                    + matches.stream().map(TbClient.TbDeviceView::id).toList());
        }
        return matches.get(0).id();
    }

    private String writeCapabilities(Map<String, Object> capabilities) {
        try {
            return objectMapper.writeValueAsString(capabilities);
        } catch (Exception e) {
            throw new IllegalArgumentException("capabilities not serializable", e);
        }
    }

    public static String normalizeEui(String eui) {
        return eui == null ? "" : eui.trim().toLowerCase(Locale.ROOT);
    }

    private static String requireEui(String eui) {
        String normalized = normalizeEui(eui);
        if (!EUI_PATTERN.matcher(normalized).matches()) {
            throw new IllegalArgumentException("invalid devEui format: " + eui);
        }
        return normalized;
    }

    private static DeviceProject requireProject(String project) {
        try {
            return DeviceProject.valueOf(project == null ? "" : project.trim().toUpperCase(Locale.ROOT));
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("invalid project: " + project
                    + " (expected LIVESTOCK or PARKING)");
        }
    }
}
