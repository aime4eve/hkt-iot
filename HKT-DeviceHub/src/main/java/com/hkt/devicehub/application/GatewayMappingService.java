package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Reads the IoT gateway (OC connector) shared attributes and extracts the set
 * of NS project ids that have a topic mapping (L4: missing mapping is the
 * first suspect when uplinks never reach TB). Read-only; mapping changes stay
 * a manual backup→change→reload→verify runbook.
 * <p>
 * Extraction heuristic: numeric values of fields named project/projectId and
 * pure-digit object keys count as project ids. Cached 30s.
 */
@Service
@Slf4j
public class GatewayMappingService {

    private final TbClient tbClient;
    private final TbProperties tbProperties;
    private final DeviceHubProperties properties;

    private volatile Set<Integer> cached = Set.of();
    private volatile long cachedAtMs;

    public GatewayMappingService(TbClient tbClient, TbProperties tbProperties,
                                 DeviceHubProperties properties) {
        this.tbClient = tbClient;
        this.tbProperties = tbProperties;
        this.properties = properties;
    }

    public synchronized Set<Integer> mappedProjectIds() {
        long now = System.currentTimeMillis();
        if (now - cachedAtMs < properties.getTopology().getProbeCacheMs()) {
            return cached;
        }
        try {
            return refreshMappedProjectIds();
        } catch (Exception e) {
            log.warn("[GatewayMapping] shared attributes read failed: {}", e.getMessage());
            return Set.of();
        }
    }

    /**
     * Like {@link #mappedProjectIds()} but propagates the TB read failure, so
     * callers can tell "no mappings" apart from "cannot read mappings".
     */
    public synchronized Set<Integer> refreshMappedProjectIds() {
        Map<String, JsonNode> attrs =
                tbClient.fetchSharedAttributes(tbProperties.getGatewayDeviceId());
        refreshCacheFrom(attrs);
        return cached;
    }

    private void refreshCacheFrom(Map<String, JsonNode> attrs) {
        Set<Integer> ids = new LinkedHashSet<>();
        attrs.forEach((key, value) -> extractProjectIds(key, value, ids));
        cached = Set.copyOf(ids);
        cachedAtMs = System.currentTimeMillis();
    }

    /** One duplicated topic filter: count includes every copy across all
     * active connectors (the 2026-11-06 incident was a single connector
     * holding the same entry twice, doubling every uplink). */
    public record DuplicateGroup(String topicFilter, int count, List<String> projects) {}

    /**
     * Duplicate-mapping scan result (NIX-382). {@code error} non-null means
     * the TB read failed and the lists are not authoritative.
     */
    public record MappingDuplicateInfo(List<DuplicateGroup> duplicates, int totalMappings,
                                       List<String> connectors, String error) {}

    /**
     * Read the gateway shared attributes and report duplicated topic filters
     * across all active connectors. Also refreshes the project-id cache —
     * callers get both from one TB round-trip.
     */
    public synchronized MappingDuplicateInfo detectDuplicateMappings() {
        Map<String, JsonNode> attrs;
        try {
            attrs = tbClient.fetchSharedAttributes(tbProperties.getGatewayDeviceId());
        } catch (Exception e) {
            log.warn("[GatewayMapping] duplicate scan read failed: {}", e.getMessage());
            return new MappingDuplicateInfo(List.of(), 0, List.of(), e.getMessage());
        }
        refreshCacheFrom(attrs);
        return analyzeDuplicates(attrs);
    }

    /**
     * Pure analyzer over the shared-attribute map: aggregate every active
     * connector's mapping entries by topicFilter. Tolerates the two stored
     * shapes of configurationJson (object and JSON-encoded string) and
     * entries without a topicFilter (skipped, counted via totalMappings).
     */
    static MappingDuplicateInfo analyzeDuplicates(Map<String, JsonNode> attrs) {
        JsonNode active = attrs.get("active_connectors");
        List<String> connectors = new java.util.ArrayList<>();
        if (active != null && active.isArray()) {
            active.forEach(n -> connectors.add(n.asText()));
        }
        // A connector config may exist without being listed in
        // active_connectors (e.g. left disabled); scan those too — a disabled
        // connector cannot double-deliver, but its stale duplicates resurface
        // the moment it is re-enabled (observed: oc-test1).
        List<String> scanKeys = new java.util.ArrayList<>(connectors);
        attrs.keySet().stream()
                .filter(k -> !"active_connectors".equals(k))
                .filter(k -> !scanKeys.contains(k))
                .forEach(scanKeys::add);

        java.util.Map<String, Integer> counts = new java.util.LinkedHashMap<>();
        int total = 0;
        for (String key : scanKeys) {
            JsonNode value = attrs.get(key);
            if (value == null) continue;
            ArrayNode mapping = MappingChangeService.findMappingArray(value);
            if (mapping == null) continue;
            for (JsonNode entry : mapping) {
                total++;
                String topic = entry.path("topicFilter").asText(null);
                if (topic == null || topic.isBlank()) continue;
                counts.merge(topic, 1, Integer::sum);
            }
        }
        List<DuplicateGroup> duplicates = counts.entrySet().stream()
                .filter(e -> e.getValue() > 1)
                .map(e -> new DuplicateGroup(e.getKey(), e.getValue(), projectsOf(e.getKey())))
                .sorted(java.util.Comparator.comparing(DuplicateGroup::topicFilter))
                .toList();
        return new MappingDuplicateInfo(duplicates, total, connectors, null);
    }

    private static List<String> projectsOf(String topicFilter) {
        Set<String> projects = new LinkedHashSet<>();
        java.util.regex.Matcher m = java.util.regex.Pattern
                .compile("org/\\d+/project/(\\d+)/")
                .matcher(topicFilter);
        while (m.find()) projects.add(m.group(1));
        return List.copyOf(projects);
    }

    public boolean isMapped(Integer nsProjectId) {
        return nsProjectId != null && mappedProjectIds().contains(nsProjectId);
    }

    static void extractProjectIds(String key, JsonNode value, Set<Integer> out) {
        if (key != null && key.matches("\\d+")) {
            out.add(Integer.parseInt(key));
        }
        if (value == null) return;
        if (value.isTextual()) {
            String text = value.asText();
            String trimmed = text == null ? "" : text.trim();
            // OC connector config is stored as a JSON-encoded string: parse it,
            // and mine topic filters like org/1/project/148/device/+/dat/up.
            java.util.regex.Matcher topic = java.util.regex.Pattern
                    .compile("org/\\d+/project/(\\d+)/")
                    .matcher(trimmed);
            while (topic.find()) {
                out.add(Integer.parseInt(topic.group(1)));
            }
            if (trimmed.startsWith("{") || trimmed.startsWith("[")) {
                try {
                    extractProjectIds(key, new com.fasterxml.jackson.databind.ObjectMapper()
                            .readTree(trimmed), out);
                } catch (Exception ignored) {
                    // not JSON after all; topic regex above already mined it
                }
            }
            if (trimmed.matches("\\d+") && key != null && key.toLowerCase().contains("project")) {
                out.add(Integer.parseInt(trimmed));
            }
            return;
        }
        if (value.isObject()) {
            value.fields().forEachRemaining(entry -> {
                String field = entry.getKey();
                if ((field.equalsIgnoreCase("project") || field.equalsIgnoreCase("projectId")
                        || field.equalsIgnoreCase("project_id")) && entry.getValue().isNumber()) {
                    out.add(entry.getValue().asInt());
                } else {
                    extractProjectIds(field, entry.getValue(), out);
                }
            });
        } else if (value.isArray()) {
            value.forEach(item -> extractProjectIds(null, item, out));
        }
    }
}
