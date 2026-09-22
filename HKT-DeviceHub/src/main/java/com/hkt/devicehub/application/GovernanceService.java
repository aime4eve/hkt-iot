package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.AuditLog;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.AuditLogRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.application.UnprocessableEntityException;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Governance center backend (R-04, console §3.4): duplicate-name conflict
 * listing with copy-by-copy evidence, and the strictly guarded empty-copy
 * delete. Deletion refuses any TB device that has telemetry (TB null-value
 * artifacts filtered, L8) or is locally bound, and always writes audit_logs.
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class GovernanceService {

    private static final int TELEMETRY_PROBE_LIMIT = 100;

    private final TbClient tbClient;
    private final RegisteredDeviceRepository deviceRepository;
    private final AuditLogRepository auditLogRepository;
    private final ObjectMapper objectMapper;

    public record ConflictCopy(String tbDeviceId, String tbName, long createdTime,
                               Instant latestTelemetryAt, int approxTelemetryCount,
                               boolean boundLocally, boolean suggestedKeep) {}

    public record ConflictRow(String devEui, Long localDeviceId, String boundTbDeviceId,
                              List<ConflictCopy> candidates) {}

    /** TB devices sharing one (case-insensitive) EUI name, per project. */
    @Transactional(readOnly = true)
    public List<ConflictRow> conflicts(DeviceProject project) {
        List<RegisteredDevice> locals = deviceRepository.findByProject(project);
        Map<String, RegisteredDevice> localByEui = new LinkedHashMap<>();
        locals.forEach(d -> localByEui.put(d.getDevEui(), d));

        Map<String, List<TbClient.TbDeviceView>> byLowerName = new LinkedHashMap<>();
        for (TbClient.TbDeviceView view : tbClient.listTenantDevices()) {
            String name = view.name() == null ? "" : view.name().toLowerCase(Locale.ROOT);
            if (name.matches("^[0-9a-f]{16}$")) {
                byLowerName.computeIfAbsent(name, k -> new ArrayList<>()).add(view);
            }
        }

        List<ConflictRow> rows = new ArrayList<>();
        byLowerName.forEach((eui, views) -> {
            if (views.size() < 2) return;
            RegisteredDevice local = localByEui.get(eui);
            rows.add(new ConflictRow(eui,
                    local == null ? null : local.getId(),
                    local == null || local.getTbDeviceId() == null
                            ? null : local.getTbDeviceId().toString(),
                    views.stream().map(view -> toCopy(view, local)).toList()));
        });
        return rows;
    }

    private ConflictCopy toCopy(TbClient.TbDeviceView view, RegisteredDevice local) {
        Instant latest = null;
        int count = 0;
        try {
            latest = tbClient.fetchLatestTelemetryTs(view.id());
            count = tbClient.approximateTelemetryCount(view.id(), TELEMETRY_PROBE_LIMIT);
        } catch (Exception e) {
            log.warn("[Governance] telemetry probe for {} failed: {}", view.id(), e.getMessage());
        }
        boolean bound = local != null && local.getTbDeviceId() != null
                && local.getTbDeviceId().toString().equals(view.id());
        return new ConflictCopy(view.id(), view.name(), view.createdTime(), latest, count,
                bound, count > 0 || latest != null);
    }

    /**
     * Delete an empty TB duplicate. Hard rules: the device must carry zero
     * telemetry and must not be locally bound — violations are refused with
     * 422. Every attempt (success or refusal) lands in audit_logs.
     */
    @Transactional
    public void deleteEmptyCopy(String tbDeviceId, String operator) {
        List<RegisteredDevice> bound = deviceRepository.findAll().stream()
                .filter(d -> d.getTbDeviceId() != null
                        && d.getTbDeviceId().toString().equals(tbDeviceId))
                .toList();
        if (!bound.isEmpty()) {
            audit("DELETE_TB_DEVICE_REJECTED", tbDeviceId, operator,
                    Map.of("reason", "LOCALLY_BOUND",
                            "localDeviceIds", bound.stream().map(RegisteredDevice::getId).toList()));
            throw new UnprocessableEntityException(
                    "TB device " + tbDeviceId + " is bound to local device(s); refusing delete");
        }
        if (tbClient.hasAnyTelemetry(tbDeviceId)) {
            Instant latest = tbClient.fetchLatestTelemetryTs(tbDeviceId);
            audit("DELETE_TB_DEVICE_REJECTED", tbDeviceId, operator,
                    Map.of("reason", "HAS_TELEMETRY", "latestTelemetryAt", String.valueOf(latest)));
            throw new UnprocessableEntityException(
                    "TB device " + tbDeviceId + " has telemetry; refusing delete (zero-copy rule)");
        }
        tbClient.deleteDevice(tbDeviceId);
        audit("DELETE_TB_DEVICE", tbDeviceId, operator,
                Map.of("telemetryCheck", "result,data,dataHex empty (null artifacts filtered)"));
    }

    private void audit(String action, String target, String operator, Map<String, Object> detail) {
        AuditLog entry = new AuditLog();
        entry.setAction(action);
        entry.setTarget(target);
        entry.setOperator(operator == null || operator.isBlank() ? "console" : operator);
        try {
            entry.setDetail(objectMapper.writeValueAsString(detail));
        } catch (Exception e) {
            entry.setDetail("{}");
        }
        auditLogRepository.save(entry);
    }
}
