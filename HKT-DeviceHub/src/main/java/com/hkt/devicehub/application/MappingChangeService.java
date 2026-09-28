package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.JsonNodeFactory;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.node.TextNode;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.Instant;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Automated half of the gateway mapping change runbook (R-07 assist): for one
 * NS project, back up the OC connector shared attribute, append the missing
 * topic filter (copying an existing mapping entry so the field shape always
 * matches), write it back, and verify by re-reading. What remains manual is
 * only the connector reload when a gateway does not hot-apply attribute
 * updates, plus end-to-end confirmation that a real uplink lands in TB
 * (docs/runbook-gateway-mapping.md).
 * <p>
 * Fail-closed: any structural surprise (attr missing, no mapping array, no
 * sibling entry to copy from, unparsable JSON) aborts before any write — a
 * corrupted OC config would silence every project at once.
 */
@Service
@Slf4j
public class MappingChangeService {

    static final Pattern TOPIC_PATTERN = Pattern.compile("org/(\\d+)/project/(\\d+)/");

    private final TbClient tbClient;
    private final TbProperties tbProperties;
    private final GatewayMappingService gatewayMapping;
    private final ObjectMapper objectMapper;

    public MappingChangeService(TbClient tbClient, TbProperties tbProperties,
                                GatewayMappingService gatewayMapping, ObjectMapper objectMapper) {
        this.tbClient = tbClient;
        this.tbProperties = tbProperties;
        this.gatewayMapping = gatewayMapping;
        this.objectMapper = objectMapper;
    }

    /**
     * @param dryRun true → plan only: report the new value without backup/write
     * @param reloadGateway after a real write, restart the TB gateway via RPC so
     *                      the connector re-reads its config — this gateway does
     *                      NOT hot-reload shared-attribute changes (verified
     *                      2026-09-28), so a mapping without reload stays dead
     */
    public record MappingChangeReport(
            boolean dryRun, boolean changed, int nsProjectId,
            List<String> addedTopicFilters, String orgId, String backupPath,
            String previousValue, String newValue, Boolean verifiedAfterWrite,
            Boolean reloaded, String reloadNote,
            String note, String error) {}

    public synchronized MappingChangeReport apply(int nsProjectId, boolean dryRun, boolean reloadGateway) {
        if (!tbProperties.isEnabled()) {
            return fail(nsProjectId, dryRun, "TB 集成未启用（devicehub.tb.enabled=false）");
        }
        if (!tbProperties.isMappingAutoFixEnabled()) {
            return fail(nsProjectId, dryRun, "映射自动修复已关闭"
                    + "（devicehub.tb.mapping-auto-fix-enabled=false），请走人工工单");
        }
        String attrKey = tbProperties.getMappingAttrKey();
        String gwId = tbProperties.getGatewayDeviceId();
        Map<String, JsonNode> attrs;
        try {
            attrs = tbClient.fetchSharedAttributes(gwId);
        } catch (Exception e) {
            return fail(nsProjectId, dryRun, "网关共享属性读取失败：" + e.getMessage());
        }
        JsonNode raw = attrs.get(attrKey);
        if (raw == null) {
            return fail(nsProjectId, dryRun, "网关共享属性中不存在 key '" + attrKey
                    + "'（现有 keys: " + String.join(", ", attrs.keySet()) + "）");
        }
        String rawAsText = raw.isTextual() ? raw.asText() : raw.toString();

        JsonNode content;
        boolean stringEncoded;
        try {
            if (raw.isTextual()) {
                content = objectMapper.readTree(raw.asText());
                stringEncoded = true;
            } else {
                content = raw;
                stringEncoded = false;
            }
        } catch (Exception e) {
            return fail(nsProjectId, dryRun, attrKey + " 值不是合法 JSON，拒绝自动修改，请走人工工单");
        }

        if (containsProject(content.toString(), nsProjectId)) {
            return new MappingChangeReport(dryRun, false, nsProjectId, List.of(), null, null,
                    rawAsText, rawAsText, true, null, null,
                    "项目 " + nsProjectId + " 已有映射，无需变更", null);
        }

        ArrayNode mapping = findMappingArray(content);
        if (mapping == null || mapping.isEmpty()) {
            return fail(nsProjectId, dryRun,
                    "未找到已有 mapping 条目（configurationJson.mapping），无从复制字段结构，请走人工工单");
        }
        JsonNode template = null;
        for (JsonNode entry : mapping) {
            if (entry.isObject() && TOPIC_PATTERN.matcher(entry.toString()).find()) {
                template = entry;
                break;
            }
        }
        if (template == null) {
            return fail(nsProjectId, dryRun, "mapping 条目中不含 topic 过滤器，无法复制结构，请走人工工单");
        }

        JsonNode entry = template.deepCopy();
        List<String> added = new ArrayList<>();
        rewriteProjectIds(entry, nsProjectId, added);
        mapping.add(entry);

        String newSerialized = content.toString();
        JsonNode newValue = stringEncoded
                ? JsonNodeFactory.instance.textNode(newSerialized)
                : content;

        if (dryRun) {
            return new MappingChangeReport(true, true, nsProjectId, added, orgOf(entry),
                    null, rawAsText, newSerialized, null, null, null,
                    "试运行：仅生成计划，未写入", null);
        }

        String backupPath;
        try {
            backupPath = writeBackup(attrKey, gwId, nsProjectId, raw);
        } catch (Exception e) {
            return fail(nsProjectId, dryRun, "备份失败，未写入任何变更：" + e.getMessage());
        }

        try {
            tbClient.saveSharedAttribute(gwId, attrKey, newValue);
        } catch (Exception e) {
            return fail(nsProjectId, dryRun, "共享属性写入失败（备份在 " + backupPath + "）：" + e.getMessage());
        }

        Boolean verified;
        String note;
        try {
            Map<String, JsonNode> after = tbClient.fetchSharedAttributes(gwId);
            Set<Integer> ids = new HashSet<>();
            JsonNode fresh = after.get(attrKey);
            if (fresh != null) {
                GatewayMappingService.extractProjectIds(attrKey, fresh, ids);
            }
            verified = ids.contains(nsProjectId);
            gatewayMapping.refreshMappedProjectIds(); // preflight reads this 30s cache
        } catch (Exception e) {
            verified = null;
        }
        Boolean reloaded = null;
        String reloadNote = null;
        if (reloadGateway) {
            var reload = reloadGateway();
            reloaded = reload.success();
            reloadNote = reload.success()
                    ? "网关已重载（gateway_restart 成功），等设备下一真实上行到达 TB 作最终验证"
                    : "网关重载失败：" + (reload.error() == null ? "未知错误" : reload.error())
                            + "；映射已写入，需按 runbook 手动重载";
        }
        note = verified == null
                ? "写入已执行，回读校验失败"
                : Boolean.TRUE.equals(verified) ? "映射已写入并回读确认" : "写入已执行，但回读未见项目 " + nsProjectId;
        if (reloadNote != null) {
            note = note + "；" + reloadNote;
        }
        log.info("[MappingChange] project {} auto-fixed, added {}, backup {}, reloaded {}",
                nsProjectId, added, backupPath, reloaded);
        return new MappingChangeReport(false, true, nsProjectId, added, orgOf(entry), backupPath,
                rawAsText, newSerialized, verified, reloaded, reloadNote, note, null);
    }

    /** Gateway reload report for the standalone reload endpoint. */
    public record GatewayReloadReport(boolean success, String error, Instant at) {}

    /**
     * Restart the TB gateway so every connector re-reads its config from the
     * shared attributes. Verified 2026-09-28: this gateway does not hot-reload
     * attribute changes, and its per-connector connector_reboot RPC is broken
     * ("connector not found" even for names listed in active_connectors) —
     * gateway_restart is the only reliable reload path.
     */
    public GatewayReloadReport reloadGateway() {
        String gwId = tbProperties.getGatewayDeviceId();
        try {
            tbClient.sendRpcTwoWay(gwId,
                    Map.of("method", "gateway_restart", "params", Map.of()), 20_000L);
            log.info("[MappingChange] gateway restarted via RPC ({})", gwId);
            return new GatewayReloadReport(true, null, Instant.now());
        } catch (Exception e) {
            log.warn("[MappingChange] gateway restart RPC failed: {}", e.getMessage());
            return new GatewayReloadReport(false, e.getMessage(), Instant.now());
        }
    }

    private MappingChangeReport fail(int nsProjectId, boolean dryRun, String error) {
        log.warn("[MappingChange] apply project {} aborted: {}", nsProjectId, error);
        return new MappingChangeReport(dryRun, false, nsProjectId, List.of(), null, null,
                null, null, null, null, null, null, error);
    }

    private static boolean containsProject(String text, int nsProjectId) {
        Matcher m = TOPIC_PATTERN.matcher(text);
        while (m.find()) {
            if (Integer.parseInt(m.group(2)) == nsProjectId) return true;
        }
        return false;
    }

    /** First array named "mapping" anywhere in the tree, descending into
     * string-encoded JSON as needed. */
    private ArrayNode findMappingArray(JsonNode node) {
        if (node == null) return null;
        if (node.isObject()) {
            var fields = node.fields();
            while (fields.hasNext()) {
                var field = fields.next();
                if ("mapping".equals(field.getKey()) && field.getValue().isArray()) {
                    return (ArrayNode) field.getValue();
                }
            }
            fields = node.fields();
            while (fields.hasNext()) {
                ArrayNode found = findMappingArray(fields.next().getValue());
                if (found != null) return found;
            }
        } else if (node.isArray()) {
            for (JsonNode item : node) {
                ArrayNode found = findMappingArray(item);
                if (found != null) return found;
            }
        } else if (node.isTextual()) {
            String text = node.asText().trim();
            if (text.startsWith("{")) {
                try {
                    return findMappingArray(objectMapper.readTree(text));
                } catch (Exception ignored) {
                    // not JSON after all
                }
            }
        }
        return null;
    }

    /** Replace org/{org}/project/{old}/ with project/{new}/ in every textual
     * field of the copied entry, collecting the rewritten topic filters. */
    private void rewriteProjectIds(JsonNode node, int nsProjectId, List<String> added) {
        if (node.isObject()) {
            var fields = node.fields();
            while (fields.hasNext()) {
                var field = fields.next();
                String name = field.getKey();
                JsonNode value = field.getValue();
                if (value.isTextual() && value.asText().contains("/project/")) {
                    String rewritten = TOPIC_PATTERN.matcher(value.asText())
                            .replaceAll("org/$1/project/" + nsProjectId + "/");
                    ((ObjectNode) node).put(name, rewritten);
                    if (TOPIC_PATTERN.matcher(rewritten).find()) added.add(rewritten);
                } else {
                    rewriteProjectIds(value, nsProjectId, added);
                }
            }
        } else if (node.isArray()) {
            ArrayNode array = (ArrayNode) node;
            for (int i = 0; i < array.size(); i++) {
                JsonNode item = array.get(i);
                if (item.isTextual() && item.asText().contains("/project/")) {
                    String rewritten = TOPIC_PATTERN.matcher(item.asText())
                            .replaceAll("org/$1/project/" + nsProjectId + "/");
                    array.set(i, TextNode.valueOf(rewritten));
                    if (TOPIC_PATTERN.matcher(rewritten).find()) added.add(rewritten);
                } else {
                    rewriteProjectIds(item, nsProjectId, added);
                }
            }
        }
    }

    private static String orgOf(JsonNode entry) {
        Matcher m = TOPIC_PATTERN.matcher(entry.toString());
        return m.find() ? m.group(1) : null;
    }

    private String writeBackup(String attrKey, String gwId, int nsProjectId, JsonNode raw) throws IOException {
        Path dir = Path.of(tbProperties.getMappingBackupDir());
        Files.createDirectories(dir);
        String stamp = DateTimeFormatter.ofPattern("yyyyMMdd-HHmmss").format(ZonedDateTime.now());
        Path file = dir.resolve(attrKey + "-" + stamp + "-p" + nsProjectId + ".json");
        Map<String, Object> backup = new LinkedHashMap<>();
        backup.put("deviceId", gwId);
        backup.put("attrKey", attrKey);
        backup.put("savedAt", Instant.now().toString());
        backup.put("reason", "mapping auto-fix for NS project " + nsProjectId);
        backup.put("previousValue", raw.isTextual() ? raw.asText() : raw.toString());
        Files.writeString(file, objectMapper.writerWithDefaultPrettyPrinter()
                .writeValueAsString(backup), StandardCharsets.UTF_8);
        return file.toString();
    }
}
