package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.LinkedHashSet;
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
            Map<String, JsonNode> attrs =
                    tbClient.fetchSharedAttributes(tbProperties.getGatewayDeviceId());
            Set<Integer> ids = new LinkedHashSet<>();
            attrs.forEach((key, value) -> extractProjectIds(key, value, ids));
            cached = Set.copyOf(ids);
            cachedAtMs = now;
        } catch (Exception e) {
            log.warn("[GatewayMapping] shared attributes read failed: {}", e.getMessage());
        }
        return cached;
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
