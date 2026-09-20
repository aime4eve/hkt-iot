package com.hkt.devicehub.interfaces;

import com.hkt.devicehub.application.CatchupLagCalculator;
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
 * Channel health: WS connection state, per-project catch-up lag and failure
 * counters, sourced from TelemetryChannelMetrics and registered_devices.
 * <p>
 * maxCursorLagMs semantics (changed 2026-09): relative catch-up lag
 * max(last_frame_at - telemetry_cursor_ms, 0) — "new frames on TB not yet
 * pulled". It is NOT now - cursor anymore: silent low-frequency devices whose
 * cursor sits at their last frame report 0, not days of bogus lag.
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

    public record ProjectHealth(int activeDevices, Long maxCursorLagMs, int noDataDevices,
                                int devicesWithFailures, int totalConsecutiveFailures) {}

    public record ChannelHealthResponse(Map<String, Boolean> channels,
                                        Map<String, ProjectHealth> projects) {}

    @GetMapping("/health")
    @Operation(summary = "WS connection state, per-project catch-up lag, failure counts")
    public ChannelHealthResponse health() {
        Map<String, ProjectHealth> projects = new LinkedHashMap<>();
        List<RegisteredDevice> active = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        Map<DeviceProject, List<RegisteredDevice>> byProject = new EnumMap<>(DeviceProject.class);
        active.forEach(d -> byProject.computeIfAbsent(d.getProject(), k -> new java.util.ArrayList<>()).add(d));
        for (DeviceProject project : DeviceProject.values()) {
            List<RegisteredDevice> devices = byProject.getOrDefault(project, List.of());
            projects.put(project.name(), new ProjectHealth(
                    devices.size(),
                    CatchupLagCalculator.maxLagMs(devices),
                    CatchupLagCalculator.noDataCount(devices),
                    (int) devices.stream().filter(d -> d.getConsecutiveFailures() > 0).count(),
                    devices.stream().mapToInt(RegisteredDevice::getConsecutiveFailures).sum()));
        }
        return new ChannelHealthResponse(metrics.channelStateSnapshot(), projects);
    }
}
