package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import com.hkt.devicehub.infrastructure.thingsboard.TbWebSocketChannel;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.ObjectProvider;
import org.springframework.stereotype.Service;

import java.net.InetSocketAddress;
import java.net.Socket;
import java.time.Instant;
import java.util.EnumMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Dashboard topology aggregation (R-10, console §3.1): one endpoint that
 * probes every link segment — NS TCP reachability, TB auth, gateway OC
 * mapping coverage, WS subscription consistency, MQ volume, per-project
 * catch-up lag. Probe results are cached (default 30s) so page refreshes
 * never hammer TB/NS.
 */
@Service
@Slf4j
public class TopologyService {

    private final TbClient tbClient;
    private final TbProperties tbProperties;
    private final DeviceHubProperties properties;
    private final GatewayMappingService gatewayMapping;
    private final RegisteredDeviceRepository deviceRepository;
    private final TelemetryChannelMetrics metrics;
    private final ObjectProvider<TbWebSocketChannel> wsChannel;

    private volatile TopologyResponse cached;
    private volatile long cachedAtMs;

    public TopologyService(TbClient tbClient, TbProperties tbProperties,
                           DeviceHubProperties properties, GatewayMappingService gatewayMapping,
                           RegisteredDeviceRepository deviceRepository,
                           TelemetryChannelMetrics metrics,
                           ObjectProvider<TbWebSocketChannel> wsChannel) {
        this.tbClient = tbClient;
        this.tbProperties = tbProperties;
        this.properties = properties;
        this.gatewayMapping = gatewayMapping;
        this.deviceRepository = deviceRepository;
        this.metrics = metrics;
        this.wsChannel = wsChannel;
    }

    public record NsProbe(boolean reachable, String host, int port, Long latencyMs, String error) {}

    public record TbAuthProbe(boolean authenticated, String error) {}

    public record GatewayMappingProbe(boolean readable, int mappedProjectCount,
                                      List<Integer> mappedProjectIds, String error) {}

    public record WsConsistency(int subscriptions, long activeDevices, boolean consistent) {}

    public record MqStats(Long messages24h, String note) {}

    public record ProjectTopology(long activeDevices, Long maxCatchupLagMs, int noDataDevices) {}

    public record TopologyResponse(
            NsProbe ns, TbAuthProbe tbAuth, GatewayMappingProbe gatewayMapping,
            WsConsistency wsConsistency, MqStats mq,
            Map<String, ProjectTopology> projects,
            Map<String, Boolean> channels, Instant checkedAt, long cacheTtlMs) {}

    public synchronized TopologyResponse topology() {
        long now = System.currentTimeMillis();
        long ttl = properties.getTopology().getProbeCacheMs();
        if (cached != null && now - cachedAtMs < ttl) {
            return cached;
        }
        TopologyResponse fresh = new TopologyResponse(
                probeNs(), probeTbAuth(), probeGatewayMapping(), wsConsistency(), mqStats(),
                projectTopology(), metrics.channelStateSnapshot(), Instant.now(), ttl);
        cached = fresh;
        cachedAtMs = now;
        return fresh;
    }

    private NsProbe probeNs() {
        String host = properties.getTopology().getNsHost();
        int port = properties.getTopology().getNsPort();
        long started = System.currentTimeMillis();
        try (Socket socket = new Socket()) {
            socket.connect(new InetSocketAddress(host, port), 3_000);
            return new NsProbe(true, host, port, System.currentTimeMillis() - started, null);
        } catch (Exception e) {
            return new NsProbe(false, host, port, null, e.getMessage());
        }
    }

    private TbAuthProbe probeTbAuth() {
        if (!tbProperties.isEnabled()) {
            return new TbAuthProbe(false, "TB integration disabled (devicehub.tb.enabled=false)");
        }
        try {
            tbClient.getAccessToken();
            return new TbAuthProbe(true, null);
        } catch (Exception e) {
            return new TbAuthProbe(false, e.getMessage());
        }
    }

    private GatewayMappingProbe probeGatewayMapping() {
        try {
            var ids = gatewayMapping.mappedProjectIds().stream().sorted().toList();
            return new GatewayMappingProbe(true, ids.size(), ids, null);
        } catch (Exception e) {
            return new GatewayMappingProbe(false, 0, List.of(), e.getMessage());
        }
    }

    private WsConsistency wsConsistency() {
        long active = deviceRepository.countByStatus(RegistrationStatus.ACTIVE);
        TbWebSocketChannel ws = wsChannel.getIfAvailable();
        int subscriptions = ws == null ? 0 : ws.subscriptionCount();
        return new WsConsistency(subscriptions, active, subscriptions == active);
    }

    private MqStats mqStats() {
        // RocketMQ exposes no per-topic 24h message count via the client API;
        // wiring mqadmin/topic stats is a TODO.
        return new MqStats(null, "24h message count unavailable: RocketMQ client has no "
                + "time-window topic stats API; mqadmin integration TODO");
    }

    private Map<String, ProjectTopology> projectTopology() {
        List<RegisteredDevice> active = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        Map<DeviceProject, List<RegisteredDevice>> byProject = new EnumMap<>(DeviceProject.class);
        active.forEach(d -> byProject.computeIfAbsent(d.getProject(), k -> new java.util.ArrayList<>()).add(d));
        Map<String, ProjectTopology> out = new LinkedHashMap<>();
        for (DeviceProject project : DeviceProject.values()) {
            List<RegisteredDevice> devices = byProject.getOrDefault(project, List.of());
            out.put(project.name(), new ProjectTopology(
                    devices.size(),
                    CatchupLagCalculator.maxLagMs(devices),
                    CatchupLagCalculator.noDataCount(devices)));
        }
        return out;
    }
}
