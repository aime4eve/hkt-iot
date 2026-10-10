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
    private final Object refreshLock = new Object();
    private boolean refreshRunning;
    private final java.util.concurrent.ExecutorService refresher =
            java.util.concurrent.Executors.newSingleThreadExecutor(r -> {
                Thread t = new Thread(r, "mapping-gap-refresh");
                t.setDaemon(true);
                return t;
            });

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
     * @param lastSeenAt   most recent NS last_seen among the project's devices;
     *                     null when none was ever heard — such projects are
     *                     dormant inventory, not live frame loss
     */
    public record MappingGap(int nsProjectId, long deviceCount, List<String> sampleDevEuis,
                             Instant lastSeenAt) {}

    /**
     * @param nsAvailable  false when NS is disabled or unreadable — gaps are
     *                     then absent because they are unknown, not because
     *                     the link is healthy
     * @param mappingReadable false when the TB gateway shared attributes could
     *                      not be read or were not consulted (NS unavailable) —
     *                      same "unknown ≠ healthy" semantics
     * @param gaps         ACTIVE gaps: some device heard within activeWindowMs —
     *                     these are silently dropping live frames right now
     * @param dormantGaps  projects with no recent uplink — informational only;
     *                     this NS is shared company-wide and most of its
     *                     projects never needed a TB mapping (2026-10-08: 103
     *                     dormant vs a dozen active, the alert must not conflate)
     */
    public record MappingGapReport(boolean nsAvailable, String nsError,
                                   boolean mappingReadable, String mappingError,
                                   List<Integer> mappedProjectIds,
                                   List<MappingGap> gaps, List<MappingGap> dormantGaps,
                                   long activeWindowMs, Instant checkedAt) {}

    /**
     * Non-blocking, single-flight, stale-while-revalidate. A cold inspection
     * pages the whole NS inventory (~4k devices, minutes when NS is loaded —
     * observed 2026-10-09: 40 pages × 7.5s); answering HTTP requests with that
     * scan stacked overlapping inspections on top of each other and made NS
     * slower still. Now: fresh cache → return; stale/absent → kick one async
     * refresh and answer the last report (or a "computing" placeholder).
     */
    public MappingGapReport report() {
        MappingGapReport snap = cached;
        if (snap != null
                && System.currentTimeMillis() - cachedAtMs < properties.getTopology().getGapCacheMs()) {
            return snap;
        }
        triggerRefresh();
        return snap != null ? snap : computing();
    }

    /**
     * Drop the cached report and kick an async recompute. Called right after a
     * mapping auto-fix so the gap panel refreshes within a scan cycle instead of
     * showing the just-fixed project as pending until the TTL expires.
     */
    public void invalidate() {
        cached = null;
        cachedAtMs = 0;
        triggerRefresh();
    }

    private void triggerRefresh() {
        synchronized (refreshLock) {
            if (refreshRunning) return;
            refreshRunning = true;
        }
        refresher.execute(() -> {
            try {
                refreshNow();
            } finally {
                synchronized (refreshLock) {
                    refreshRunning = false;
                }
            }
        });
    }

    /** Synchronous single-flight recompute — the async trigger funnels here. */
    synchronized MappingGapReport refreshNow() {
        MappingGapReport snap = cached;
        if (snap != null
                && System.currentTimeMillis() - cachedAtMs < properties.getTopology().getGapCacheMs()) {
            return snap;
        }
        MappingGapReport fresh = inspect();
        cached = fresh;
        cachedAtMs = System.currentTimeMillis();
        return fresh;
    }

    private MappingGapReport computing() {
        return new MappingGapReport(false, "巡检计算中（NS 全量清单分页拉取，约 1 个扫描周期）",
                false, null, List.of(), List.of(), List.of(), activeWindowMs(), Instant.now());
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
                    List.of(), List.of(), List.of(), activeWindowMs(), Instant.now());
        }
        try {
            return derive(nsClient.listDevices(null), mapped);
        } catch (Exception e) {
            log.warn("[MappingGap] NS device list failed: {}", e.getMessage());
            return new MappingGapReport(false, e.getMessage(), true, null,
                    List.copyOf(mapped), List.of(), List.of(), activeWindowMs(), Instant.now());
        }
    }

    private long activeWindowMs() {
        return properties.getTopology().getGapActiveWindowMs();
    }

    private MappingGapReport unavailable(String reason) {
        // mappingReadable=false too: it was never consulted, and unknown must
        // not masquerade as "no gaps = healthy".
        return new MappingGapReport(false, reason, false, null,
                List.of(), List.of(), List.of(), activeWindowMs(), Instant.now());
    }

    MappingGapReport derive(List<NsClient.NsDevice> devices, Set<Integer> mapped) {
        return derive(devices, mapped, Instant.now());
    }

    MappingGapReport derive(List<NsClient.NsDevice> devices, Set<Integer> mapped, Instant now) {
        Map<Integer, List<NsClient.NsDevice>> byProject = new TreeMap<>();
        for (NsClient.NsDevice device : devices) {
            // projectId 0 means the NS list API omitted the field — unknown, not a project
            if (device.projectId() <= 0) continue;
            byProject.computeIfAbsent(device.projectId(), k -> new ArrayList<>())
                    .add(device);
        }
        Instant activeSince = now.minusMillis(activeWindowMs());
        List<MappingGap> gaps = new ArrayList<>();
        List<MappingGap> dormant = new ArrayList<>();
        for (Map.Entry<Integer, List<NsClient.NsDevice>> entry : byProject.entrySet()) {
            if (mapped.contains(entry.getKey())) continue;
            List<String> euis = entry.getValue().stream().map(NsClient.NsDevice::eui).sorted().toList();
            Instant lastSeen = entry.getValue().stream().map(NsClient.NsDevice::lastSeen)
                    .filter(java.util.Objects::nonNull).max(Instant::compareTo).orElse(null);
            MappingGap gap = new MappingGap(entry.getKey(), euis.size(),
                    euis.subList(0, Math.min(SAMPLE_EUI_LIMIT, euis.size())), lastSeen);
            (lastSeen != null && !lastSeen.isBefore(activeSince) ? gaps : dormant).add(gap);
        }
        return new MappingGapReport(true, null, true, null,
                mapped.stream().sorted().toList(), gaps, dormant, activeWindowMs(), now);
    }
}
