package com.hkt.devicehub.infrastructure.ns;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpMethod;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.client.RestOperations;

import java.net.URI;
import java.time.Instant;
import java.time.ZonedDateTime;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.startsWith;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * device/detail parsing contract: tolerant of object- or array-shaped data,
 * NS-local ("yyyy-MM-dd HH:mm:ss", UTC+8) last_seen, and org+dev_eui query
 * params. Field-name fallbacks cover vendor naming drift.
 */
class NsClientDeviceDetailTest {

    private RestOperations rest;
    private NsClient client;

    @BeforeEach
    void setUp() {
        rest = mock(RestOperations.class);
        NsProperties properties = new NsProperties();
        properties.setEnabled(true);
        properties.setBaseUrl("http://ns.test:8080");
        properties.setUsername("u");
        properties.setPassword("p");
        properties.setOrgId(1);
        client = new NsClient(properties, new ObjectMapper(), rest);

        when(rest.postForEntity(startsWith("http://ns.test:8080/backend/api/login/"), any(HttpEntity.class),
                eq(String.class))).thenReturn(
                ResponseEntity.ok("{\"code\":0,\"data\":{\"token\":\"tok-1\"}}"));
    }

    private void stubGet(String body) {
        when(rest.exchange(any(URI.class), eq(HttpMethod.GET), any(HttpEntity.class), eq(String.class)))
                .thenReturn(new ResponseEntity<>(body, HttpStatus.OK));
    }

    @Test
    void parsesObjectShapedDetail() {
        stubGet("{\"code\":0,\"data\":{\"f_cnt_up\":123,\"n_f_cnt_down\":60,\"a_f_cnt_down\":53,"
                + "\"latest_gateway\":\"c0ba1ffffe063105\",\"last_seen\":\"2026-11-03 10:00:00\","
                + "\"skip_f_cnt_check\":true}}");

        NsClient.NsDeviceDetail detail = client.fetchDeviceDetail("001a0103ff000319");

        assertEquals(123L, detail.fCntUp());
        assertEquals(60L, detail.nFCntDown());
        assertEquals(53L, detail.aFCntDown());
        assertEquals("c0ba1ffffe063105", detail.latestGateway());
        // detail last_seen measured UTC (2026-11-03 live probe: 02:33:28 ≈ 30 min
        // prior), unlike the list endpoint's NS-local UTC+8.
        assertEquals(Instant.from(ZonedDateTime.of(2026, 11, 3, 10, 0, 0, 0,
                java.time.ZoneId.of("UTC"))), detail.lastSeen());
        assertTrue(detail.skipFCntCheck());
    }

    @Test
    void parsesArrayShapedDetail() {
        stubGet("{\"code\":0,\"data\":[{\"f_cnt_up\":7,\"last_seen\":\"\"}]}");

        NsClient.NsDeviceDetail detail = client.fetchDeviceDetail("001a0103ff000319");

        assertEquals(7L, detail.fCntUp());
        assertNull(detail.lastSeen());
        assertNull(detail.skipFCntCheck());
    }

    @Test
    void detailRequestCarriesOrgAndEuiParams() {
        stubGet("{\"code\":0,\"data\":{}}");
        client.fetchDeviceDetail("001a0103ff000319");

        ArgumentCaptor<URI> uri = ArgumentCaptor.forClass(URI.class);
        org.mockito.Mockito.verify(rest).exchange(uri.capture(), eq(HttpMethod.GET),
                any(HttpEntity.class), eq(String.class));
        String url = uri.getValue().toString();
        assertTrue(url.startsWith("http://ns.test:8080/backend/org_api/lora_wan/device/detail/"),
                "unexpected url: " + url);
        assertTrue(url.contains("org=1"), "unexpected url: " + url);
        assertTrue(url.contains("dev_eui=001a0103ff000319"), "unexpected url: " + url);
    }

    @Test
    void nsErrorSurfacesAsIllegalState() {
        stubGet("{\"code\":1001,\"msg\":\"bad token\"}");

        IllegalStateException e = assertThrows(IllegalStateException.class,
                () -> client.fetchDeviceDetail("001a0103ff000319"));
        assertTrue(e.getMessage().contains("NS device detail"));
    }
}
