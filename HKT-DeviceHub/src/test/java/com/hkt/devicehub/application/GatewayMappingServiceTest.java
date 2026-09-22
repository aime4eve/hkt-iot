package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import java.util.HashSet;
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
}
