package com.hkt.devicehub.application;

import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * R-07 gateway mapping gap inspection (read-only): lists every NS project
 * that already has devices but no OC connector topic mapping — the L4 class
 * of failure behind silent uplinks (projects 148/217/219). Surfaces gaps
 * proactively at dashboard time instead of when someone happens to preflight
 * a device of the project. Mapping changes remain a manual runbook
 * (docs/runbook-gateway-mapping.md).
 */
@Service
@Slf4j
public class MappingGapService {

    static final int SAMPLE_EUI_LIMIT = 5;

    private final NsClient nsClient;
    private final GatewayMappingService gatewayMapping;
    private final DeviceHubProperties properties;

    private volatile MappingGapReport cached;
    private volatile long cachedAtMs;

    public MappingGapService(NsClient nsClient, GatewayMappingService gatewayMapping,
                             DeviceHubProperties properties) {
        this.nsClient = nsClient;
        this.gatewayMapping = gatewayMapping;
        this.properties = properties;
    }

    /**
     * @param nsProjectId  NS project holding devices but absent from the OC mapping
     * @param deviceCount  devices seen in NS for this project
     * @param sampleDevEuis first (sorted) EUIs, capped — evidence for the ticket
     */
    public record MappingGap(int nsProjectId, long deviceCount, List<String> sampleDevEuis) {}

    /**
     * @param nsAvailable  false when NS is disabled or unreadable — gaps are
     *                     then absent because they are unknown, not because
     *                     the link is healthy
     * @param mappingReadable false when the TB gateway shared attributes could
     *                      not be read or were not consulted (NS unavailable) —
     *                      same "unknown ≠ healthy" semantics
     */
    public record MappingGapReport(boolean nsAvailable, String nsError,
                                   boolean mappingReadable, String mappingError,
                                   List<Integer> mappedProjectIds,
                                   List<MappingGap> gaps, Instant checkedAt) {}

    public MappingGapReport report() {
        long now = System.currentTimeMillis();
        if (cached != null && now - cachedAtMs < properties.getTopology().getProbeCacheMs()) {
            return cached;
        }
        MappingGapReport fresh = inspect();
        cached = fresh;
        cachedAtMs = now;
        return fresh;
    }

    /**
     * Drop the cached report so the next {@link #report()} recomputes. Called
     * right after a mapping auto-fix, otherwise the console keeps showing the
     * just-fixed project as a pending gap for up to 30s — which reads as "the
     * fix did nothing" and invites repeated clicks.
     */
    public synchronized void invalidate() {
        cached = null;
        cachedAtMs = 0;
    }

    MappingGapReport inspect() {
        if (!nsClient.isEnabled()) {
            return unavailable("NS 检查未启用（devicehub.ns.enabled=false）");
        }
        Set<Integer> mapped;
        try {
            mapped = gatewayMapping.refreshMappedProjectIds();
        } catch (Exception e) {
            log.warn("[MappingGap] gateway mapping read failed: {}", e.getMessage());
            return new MappingGapReport(true, null, false, e.getMessage(),
                    List.of(), List.of(), Instant.now());
        }
        try {
            return derive(nsClient.listDevices(null), mapped);
        } catch (Exception e) {
            log.warn("[MappingGap] NS device list failed: {}", e.getMessage());
            return new MappingGapReport(false, e.getMessage(), true, null,
                    List.copyOf(mapped), List.of(), Instant.now());
        }
    }

    private static MappingGapReport unavailable(String reason) {
        // mappingReadable=false too: it was never consulted, and unknown must
        // not masquerade as "no gaps = healthy".
        return new MappingGapReport(false, reason, false, null, List.of(), List.of(), Instant.now());
    }

    MappingGapReport derive(List<NsClient.NsDevice> devices, Set<Integer> mapped) {
        Map<Integer, List<String>> byProject = new TreeMap<>();
        for (NsClient.NsDevice device : devices) {
            // projectId 0 means the NS list API omitted the field — unknown, not a project
            if (device.projectId() <= 0) continue;
            byProject.computeIfAbsent(device.projectId(), k -> new ArrayList<>())
                    .add(device.eui());
        }
        List<MappingGap> gaps = new ArrayList<>();
        for (Map.Entry<Integer, List<String>> entry : byProject.entrySet()) {
            if (mapped.contains(entry.getKey())) continue;
            List<String> euis = entry.getValue().stream().sorted().toList();
            gaps.add(new MappingGap(entry.getKey(), euis.size(),
                    euis.subList(0, Math.min(SAMPLE_EUI_LIMIT, euis.size()))));
        }
        return new MappingGapReport(true, null, true, null,
                mapped.stream().sorted().toList(), gaps, Instant.now());
    }
}
