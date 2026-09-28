package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.TextNode;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;
import org.mockito.ArgumentCaptor;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Mapping auto-fix semantics: copy a sibling entry, fail closed on any
 * structural surprise, always back up before writing, never write on dryRun.
 */
class MappingChangeServiceTest {

    private static final String GW = "0f7da2b0-e91d-11ef-a8ee-99a8c68f9649";
    private static final ObjectMapper MAPPER = new ObjectMapper();

    @TempDir
    Path tempDir;

    private TbClient tbClient;
    private GatewayMappingService gatewayMapping;
    private TbProperties properties;
    private MappingChangeService service;

    @BeforeEach
    void setUp() {
        tbClient = mock(TbClient.class);
        gatewayMapping = mock(GatewayMappingService.class);
        properties = new TbProperties();
        properties.setEnabled(true);
        properties.setGatewayDeviceId(GW);
        properties.setMappingBackupDir(tempDir.resolve("backups").toString());
        service = new MappingChangeService(tbClient, properties, gatewayMapping, MAPPER);
    }

    /** Mirrors the real OC shared attribute: a JSON-encoded string. */
    private static String ocWithProjects(int... projectIds) {
        StringBuilder sb = new StringBuilder("{\"configurationJson\":{\"mapping\":[");
        for (int i = 0; i < projectIds.length; i++) {
            if (i > 0) sb.append(',');
            sb.append("{\"topicFilter\":\"org/1/project/").append(projectIds[i])
                    .append("/device/+/dat/up\"}");
        }
        return sb.append("]}}").toString();
    }

    private Map<String, JsonNode> attrs(String oc) {
        return Map.of("OC", MAPPER.getNodeFactory().textNode(oc));
    }

    @Test
    void appendsEntryCopiedFromSiblingAndVerifiesByReadBack() throws Exception {
        String before = ocWithProjects(89, 148);
        String after = ocWithProjects(89, 148, 219);
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs(before), attrs(after));
        when(tbClient.sendRpcTwoWay(anyString(), any(), anyLong()))
                .thenReturn(MAPPER.readTree("{\"success\":true}"));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, true);

        assertFalse(report.dryRun());
        assertTrue(report.changed());
        assertEquals(List.of("org/1/project/219/device/+/dat/up"), report.addedTopicFilters());
        assertEquals("1", report.orgId());
        assertEquals(Boolean.TRUE, report.verifiedAfterWrite());
        assertEquals(Boolean.TRUE, report.reloaded());
        assertNotNull(report.backupPath());
        assertTrue(report.note().contains("网关已重载"));

        ArgumentCaptor<JsonNode> captor = ArgumentCaptor.forClass(JsonNode.class);
        verify(tbClient).saveSharedAttribute(eq(GW), eq("OC"), captor.capture());
        JsonNode written = captor.getValue();
        assertTrue(written.isTextual(), "string-encoded attr must be written back as a string");
        assertTrue(written.asText().contains("project/219"));
        JsonNode parsed = MAPPER.readTree(written.asText());
        assertEquals(3, parsed.at("/configurationJson/mapping").size());
        verify(tbClient).sendRpcTwoWay(eq(GW), eq(Map.of("method", "gateway_restart", "params", Map.of())), eq(20_000L));
    }

    @Test
    void reloadFailureKeepsWriteResultAndSaysSo() throws Exception {
        when(tbClient.fetchSharedAttributes(GW))
                .thenReturn(attrs(ocWithProjects(148)), attrs(ocWithProjects(148, 219)));
        when(tbClient.sendRpcTwoWay(anyString(), any(), anyLong()))
                .thenThrow(new IllegalStateException("RPC timeout"));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, true);

        assertTrue(report.changed());
        assertEquals(Boolean.FALSE, report.reloaded());
        assertTrue(report.note().contains("网关重载失败"));
        assertTrue(report.note().contains("RPC timeout"));
        verify(tbClient).saveSharedAttribute(eq(GW), eq("OC"), any());
    }

    @Test
    void applyWithoutReloadSkipsGatewayRpc() {
        when(tbClient.fetchSharedAttributes(GW))
                .thenReturn(attrs(ocWithProjects(148)), attrs(ocWithProjects(148, 219)));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertTrue(report.changed());
        assertNull(report.reloaded());
        verify(tbClient, never()).sendRpcTwoWay(anyString(), any(), anyLong());
    }

    @Test
    void backupFileHoldsPreviousValueBeforeWrite() throws Exception {
        String before = ocWithProjects(148);
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs(before), attrs(ocWithProjects(148, 219)));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        JsonNode backup = MAPPER.readTree(Files.readString(Path.of(report.backupPath())));
        assertEquals(before, backup.path("previousValue").asText());
        assertEquals(GW, backup.path("deviceId").asText());
        assertTrue(report.backupPath().contains("-p219"));
    }

    @Test
    void dryRunPlansChangeWithoutWriting() {
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs(ocWithProjects(148)));

        MappingChangeService.MappingChangeReport report = service.apply(219, true, true);

        assertTrue(report.dryRun());
        assertTrue(report.changed());
        assertEquals(List.of("org/1/project/219/device/+/dat/up"), report.addedTopicFilters());
        assertTrue(report.newValue().contains("project/219"));
        assertNull(report.backupPath());
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void alreadyMappedProjectIsNoOpWithoutWrite() {
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs(ocWithProjects(148, 219)));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertFalse(report.changed());
        assertEquals(Boolean.TRUE, report.verifiedAfterWrite());
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void missingAttrKeyFailsClosed() {
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(Map.of());

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertFalse(report.changed());
        assertTrue(report.error().contains("不存在 key"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void jsonWithoutMappingArrayFailsClosed() {
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs("{\"a\":1}"));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertFalse(report.changed());
        assertTrue(report.error().contains("mapping"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void emptyMappingArrayFailsClosed() {
        when(tbClient.fetchSharedAttributes(GW))
                .thenReturn(attrs("{\"configurationJson\":{\"mapping\":[]}}"));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertFalse(report.changed());
        assertTrue(report.error().contains("人工工单"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void nonJsonAttrValueFailsClosed() {
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs("not json at all"));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertFalse(report.changed());
        assertTrue(report.error().contains("人工工单"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void objectValuedAttrIsSupported() {
        String oc = ocWithProjects(148);
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(
                Map.of("OC", uncheckedParse(oc)),
                Map.of("OC", uncheckedParse(ocWithProjects(148, 219))));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertTrue(report.changed());
        assertEquals(Boolean.TRUE, report.verifiedAfterWrite());
        ArgumentCaptor<JsonNode> captor = ArgumentCaptor.forClass(JsonNode.class);
        verify(tbClient).saveSharedAttribute(eq(GW), eq("OC"), captor.capture());
        assertTrue(captor.getValue().isObject(), "object attr must be written back as an object");
    }

    @Test
    void downlinkVariantInTemplateIsRewrittenToo() throws Exception {
        String before = "{\"configurationJson\":{\"mapping\":["
                + "{\"topicFilter\":\"org/3/project/148/device/+/dat/up\","
                + "\"replyFilter\":\"org/3/project/148/device/+/dat/down\"}]}}";
        when(tbClient.fetchSharedAttributes(GW)).thenReturn(attrs(before), attrs(before));

        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);

        assertEquals(List.of(
                "org/3/project/219/device/+/dat/up",
                "org/3/project/219/device/+/dat/down"), report.addedTopicFilters());
        assertEquals("3", report.orgId());
        ArgumentCaptor<JsonNode> captor = ArgumentCaptor.forClass(JsonNode.class);
        verify(tbClient).saveSharedAttribute(eq(GW), eq("OC"), captor.capture());
        String written = captor.getValue().asText();
        assertTrue(written.contains("org/3/project/219/device/+/dat/down"));
        assertEquals(2, written.split("project/219", -1).length - 1,
                "the copy carries up+down filters for 219");
        assertEquals(2, written.split("project/148", -1).length - 1,
                "original entry keeps its two 148 filters; the copy must not keep any");
    }

    @Test
    void autoFixDisabledRefusesWithoutWrite() {
        properties.setMappingAutoFixEnabled(false);
        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);
        assertFalse(report.changed());
        assertTrue(report.error().contains("自动修复已关闭"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    @Test
    void tbDisabledRefusesWithoutWrite() {
        properties.setEnabled(false);
        MappingChangeService.MappingChangeReport report = service.apply(219, false, false);
        assertFalse(report.changed());
        assertTrue(report.error().contains("TB 集成未启用"));
        verify(tbClient, never()).saveSharedAttribute(anyString(), anyString(), any());
    }

    private JsonNode uncheckedParse(String json) {
        try {
            return MAPPER.readTree(json);
        } catch (Exception e) {
            throw new IllegalStateException(e);
        }
    }

    @Test
    void reloadGatewayReportsSuccessAndFailure() throws Exception {
        when(tbClient.sendRpcTwoWay(anyString(), any(), anyLong()))
                .thenReturn(MAPPER.readTree("{\"success\":true}"));
        assertTrue(service.reloadGateway().success());

        when(tbClient.sendRpcTwoWay(anyString(), any(), anyLong()))
                .thenThrow(new IllegalStateException("device offline"));
        MappingChangeService.GatewayReloadReport failed = service.reloadGateway();
        assertFalse(failed.success());
        assertTrue(failed.error().contains("device offline"));
    }
}
