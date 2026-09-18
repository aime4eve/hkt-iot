package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Channel health: WS connection state, per-project cursor lag and failure
 * counters, sourced from TelemetryChannelMetrics and registered_devices.
 */
@RestController
@RequestMapping("/api/v1/channels")
@Tag(name = "Channels", description = "Telemetry channel health")
public class ChannelHealthController {

    private final TelemetryChannelMetrics metrics;
    private final RegisteredDeviceRepository deviceRepository;

    public ChannelHealthController(TelemetryChannelMetrics metrics,
                                   RegisteredDeviceRepository deviceRepository) {
        this.metrics = metrics;
        this.deviceRepository = deviceRepository;
    }

    public record ProjectHealth(int activeDevices, Long maxCursorLagMs,
                                int devicesWithFailures, int totalConsecutiveFailures) {}

    public record ChannelHealthResponse(Map<String, Boolean> channels,
                                        Map<String, ProjectHealth> projects) {}

    @GetMapping("/health")
    @Operation(summary = "WS connection state, per-project cursor lag, failure counts")
    public ChannelHealthResponse health() {
        long now = System.currentTimeMillis();
        Map<String, ProjectHealth> projects = new LinkedHashMap<>();
        List<RegisteredDevice> active = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        Map<DeviceProject, List<RegisteredDevice>> byProject = new EnumMap<>(DeviceProject.class);
        active.forEach(d -> byProject.computeIfAbsent(d.getProject(), k -> new java.util.ArrayList<>()).add(d));
        for (DeviceProject project : DeviceProject.values()) {
            List<RegisteredDevice> devices = byProject.getOrDefault(project, List.of());
            Long maxLag = devices.stream()
                    .map(RegisteredDevice::getTelemetryCursorMs)
                    .filter(cursor -> cursor != null && cursor > 0)
                    .map(cursor -> Math.max(0, now - cursor))
                    .max(Long::compareTo)
                    .orElse(null);
            int withFailures = (int) devices.stream()
                    .filter(d -> d.getConsecutiveFailures() > 0).count();
            int totalFailures = devices.stream()
                    .mapToInt(RegisteredDevice::getConsecutiveFailures).sum();
            projects.put(project.name(),
                    new ProjectHealth(devices.size(), maxLag, withFailures, totalFailures));
        }
        return new ChannelHealthResponse(metrics.channelStateSnapshot(), projects);
    }
}
