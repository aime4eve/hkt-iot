package com.hkt.devicehub.application;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.util.List;
import java.util.Locale;
import java.util.Map;

/**
 * Ledger queries for the management console (console §3.2): paged list with
 * project/status/q filters, and the detail drawer with recent TB frames.
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class DeviceQueryService {

    private final RegisteredDeviceRepository deviceRepository;
    private final TbClient tbClient;
    private final ObjectMapper objectMapper;

    /**
     * Four-level health lights (R-05): nsActivated / hasUplink need per-device
     * NS lookups not yet exposed — returned as null (documented); tbDecoded
     * means TB holds at least one frame (includes undecoded dataHex fallback
     * frames); ingested means at least one frame was published to MQ.
     */
    public record HealthLights(Boolean nsActivated, Boolean hasUplink,
                               boolean tbDecoded, boolean ingested) {}

    public record DeviceListItem(
            Long id, String devEui, String project, String deviceType,
            String tbDeviceId, String status, String binding,
            Map<String, Object> capabilities, int expectedReportIntervalSeconds,
            java.time.Instant lastFrameAt, Long telemetryCursorMs,
            Long secondsSinceLastFrame, HealthLights health) {}

    public record DeviceDetail(
            Long id, String devEui, String project, String deviceType, String externalRef,
            String tbDeviceId, String status, int consecutiveFailures,
            Map<String, Object> capabilities, int expectedReportIntervalSeconds,
            java.time.Instant lastFrameAt, java.time.Instant lastEventAt,
            Long telemetryCursorMs, HealthLights health,
            List<TbClient.FrameSummary> recentFrames) {}

    @Transactional(readOnly = true)
    public Page<DeviceListItem> list(DeviceProject project, RegistrationStatus status,
                                     String q, Pageable pageable) {
        String normalizedQ = q == null || q.isBlank() ? null : q.toLowerCase(Locale.ROOT).trim();
        return deviceRepository.search(project, status, normalizedQ, pageable).map(this::toListItem);
    }

    @Transactional(readOnly = true)
    public DeviceDetail detail(long id) {
        RegisteredDevice device = deviceRepository.findById(id)
                .orElseThrow(() -> new IllegalArgumentException("device not found: " + id));
        List<TbClient.FrameSummary> recent = List.of();
        if (device.getTbDeviceId() != null) {
            try {
                recent = tbClient.fetchRecentFrames(device.getTbDeviceId().toString(), 10);
            } catch (Exception e) {
                log.warn("[Devices] recent frames for {} failed: {}", device.getDevEui(), e.getMessage());
            }
        }
        return new DeviceDetail(
                device.getId(), device.getDevEui(), device.getProject().name(),
                device.getDeviceType(), device.getExternalRef(),
                device.getTbDeviceId() == null ? null : device.getTbDeviceId().toString(),
                device.getStatus().name(), device.getConsecutiveFailures(),
                capabilitiesOf(device), device.getExpectedReportIntervalSeconds(),
                device.getLastFrameAt(), device.getLastEventAt(), device.getTelemetryCursorMs(),
                healthOf(device), recent);
    }

    private DeviceListItem toListItem(RegisteredDevice device) {
        return new DeviceListItem(
                device.getId(), device.getDevEui(), device.getProject().name(),
                device.getDeviceType(),
                device.getTbDeviceId() == null ? null : device.getTbDeviceId().toString(),
                device.getStatus().name(),
                device.getTbDeviceId() != null ? "BOUND" : "UNBOUND",
                capabilitiesOf(device), device.getExpectedReportIntervalSeconds(),
                device.getLastFrameAt(), device.getTelemetryCursorMs(),
                device.getLastFrameAt() == null ? null
                        : Math.max(0, (System.currentTimeMillis()
                                - device.getLastFrameAt().toEpochMilli()) / 1000),
                healthOf(device));
    }

    private HealthLights healthOf(RegisteredDevice device) {
        return new HealthLights(null, null,
                device.getLastFrameAt() != null, device.getLastEventAt() != null);
    }

    private Map<String, Object> capabilitiesOf(RegisteredDevice device) {
        try {
            String raw = device.getCapabilities();
            if (raw == null || raw.isBlank()) return Map.of();
            return objectMapper.readValue(raw, new TypeReference<>() {});
        } catch (Exception e) {
            return Map.of();
        }
    }
}
