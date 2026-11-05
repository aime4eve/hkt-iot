package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import static org.assertj.core.api.Assertions.assertThat;

class GatewayMappingServiceTest {

    private static final ObjectMapper MAPPER = new ObjectMapper();

    @Test
    void minesProjectIdsFromStringEncodedOcConfig() throws Exception {
        // Real OC shared attribute: a JSON string whose configurationJson.mapping
        // entries carry topic filters like org/1/project/148/device/+/dat/up.
        String oc = "{\"configurationJson\":{\"mapping\":["
                + "{\"topicFilter\":\"org/1/project/89/device/+/dat/up\"},"
                + "{\"topicFilter\":\"org/1/project/148/device/+/dat/up\"},"
                + "{\"topicFilter\":\"org/1/project/217/device/+/dat/up\"}"
                + "]}}";
        JsonNode value = MAPPER.readTree(MAPPER.writeValueAsString(oc));
        Set<Integer> ids = new HashSet<>();
        GatewayMappingService.extractProjectIds("OC", value, ids);
        assertThat(ids).contains(89, 148, 217);
    }

    @Test
    void minesProjectIdsFromObjectMappings() throws Exception {
        JsonNode value = MAPPER.readTree(
                "{\"mapping\":[{\"topicFilter\":\"org/1/project/42/device/+/dat/up\",\"project\":7}]}");
        Set<Integer> ids = new HashSet<>();
        GatewayMappingService.extractProjectIds("OC", value, ids);
        assertThat(ids).contains(42, 7);
    }

    @Test
    void ignoresNonMappingText() throws Exception {
        Set<Integer> ids = new HashSet<>();
        GatewayMappingService.extractProjectIds("note",
                MAPPER.readTree("\"just a note, no topics\""), ids);
        assertThat(ids).isEmpty();
    }

    // ---- NIX-382 duplicate-mapping detection ----

    @Test
    void analyzeFlagsDuplicatesAcrossObjectAndStringEncodedConnectors() throws Exception {
        // OC: object-shaped configurationJson with a triple entry (the 2026-11-06
        // incident shape); oc-test1: string-encoded, clean.
        String oc = "{\"configurationJson\":{\"mapping\":["
                + "{\"topicFilter\":\"org/1/project/175/device/+/dat/up\"},"
                + "{\"topicFilter\":\"org/1/project/175/device/+/dat/up\"},"
                + "{\"topicFilter\":\"org/1/project/175/device/+/dat/up\"},"
                + "{\"topicFilter\":\"org/1/project/148/device/+/dat/up\"}"
                + "]}}";
        String test = "{\"configurationJson\":{\"mapping\":["
                + "{\"topicFilter\":\"org/1/project/175/device/+/dat/up\"}"
                + "]}}";
        var info = GatewayMappingService.analyzeDuplicates(Map.of(
                "active_connectors", MAPPER.readTree("[\"OC\",\"oc-test1\"]"),
                "OC", MAPPER.readTree(oc),
                "oc-test1", MAPPER.readTree(MAPPER.writeValueAsString(test))));

        assertThat(info.error()).isNull();
        assertThat(info.connectors()).containsExactly("OC", "oc-test1");
        assertThat(info.totalMappings()).isEqualTo(5);
        assertThat(info.duplicates()).hasSize(1);
        var dup = info.duplicates().get(0);
        assertThat(dup.topicFilter()).isEqualTo("org/1/project/175/device/+/dat/up");
        assertThat(dup.count()).isEqualTo(4);
        assertThat(dup.projects()).containsExactly("175");
    }

    @Test
    void analyzeCleanConfigYieldsNoDuplicatesAndSkipsMissingTopicFilters() throws Exception {
        String oc = "{\"configurationJson\":{\"mapping\":["
                + "{\"topicFilter\":\"org/1/project/148/device/+/dat/up\"},"
                + "{\"converter\":{\"type\":\"json\"}},"
                + "{}"
                + "]}}";
        var info = GatewayMappingService.analyzeDuplicates(Map.of(
                "active_connectors", MAPPER.readTree("[\"OC\"]"),
                "OC", MAPPER.readTree(oc)));

        assertThat(info.duplicates()).isEmpty();
        assertThat(info.totalMappings()).isEqualTo(3);
        assertThat(info.connectors()).containsExactly("OC");
    }

    @Test
    void analyzeScansUnlistedConnectorsToo() throws Exception {
        // A connector left out of active_connectors cannot double-deliver, but
        // its stale duplicates resurface on re-enable — scan it anyway.
        var info = GatewayMappingService.analyzeDuplicates(Map.of(
                "active_connectors", MAPPER.readTree("[\"OC\"]"),
                "OC", MAPPER.readTree(
                        "{\"configurationJson\":{\"mapping\":[{\"topicFilter\":\"org/1/project/82/device/+/dat/up\"}]}}"),
                "oc-test1", MAPPER.readTree(
                        "{\"configurationJson\":{\"mapping\":[{\"topicFilter\":\"org/1/project/82/device/+/dat/up\"}]}}")));

        assertThat(info.duplicates()).hasSize(1);
        assertThat(info.duplicates().get(0).count()).isEqualTo(2);
        assertThat(info.duplicates().get(0).projects()).containsExactly("82");
    }
}
