package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.RestOperations;

import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Locks the TB 3.8 telemetry-write route discovered in the Phase 0 spike:
 * POST /api/plugins/telemetry/DEVICE/{id}/timeseries/SERVER_SCOPE with an
 * explicit ts. The legacy uppercase /TIMESERIES path no longer exists there —
 * a POST to it lands on the attribute-scope route and 500s.
 */
class TbClientWriteTelemetryTest {

    private RestOperations rest;
    private TbClient client;

    @BeforeEach
    @SuppressWarnings("unchecked")
    void setUp() {
        rest = mock(RestOperations.class);
        when(rest.exchange(anyString(), any(HttpMethod.class), any(HttpEntity.class),
                eq(String.class))).thenReturn(ResponseEntity.ok("{}"));
        when(rest.postForEntity(anyString(), any(HttpEntity.class), eq(String.class)))
                .thenReturn(ResponseEntity.ok("{\"token\":\"t\",\"refreshToken\":\"r\"}"));
        client = new TbClient(new TbProperties(), new ObjectMapper(), rest);
    }

    @Test
    void writesExplicitTsPointOnTimeseriesScopeRoute() {
        client.writeDeviceTelemetry("0e7a76e0-bae5-11f1-8ac2-9b57e1be74c1", 1790646684763L, "686b74");

        ArgumentCaptor<String> url = ArgumentCaptor.forClass(String.class);
        @SuppressWarnings("unchecked")
        ArgumentCaptor<HttpEntity<Object>> entity = ArgumentCaptor.forClass(HttpEntity.class);
        verify(rest).exchange(url.capture(), eq(HttpMethod.POST), entity.capture(), eq(String.class));

        assertTrue(url.getValue()
                .endsWith("/api/plugins/telemetry/DEVICE/0e7a76e0-bae5-11f1-8ac2-9b57e1be74c1"
                        + "/timeseries/SERVER_SCOPE"));
        Object rawBody = entity.getValue().getBody();
        String body = rawBody == null ? "" : rawBody.toString();
        assertTrue(body.contains("\"ts\":1790646684763"));
        assertTrue(body.contains("\"dataHex\":\"686b74\""));
    }
}
