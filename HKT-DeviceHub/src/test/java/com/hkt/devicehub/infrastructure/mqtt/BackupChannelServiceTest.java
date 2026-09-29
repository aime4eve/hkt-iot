package com.hkt.devicehub.infrastructure.mqtt;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.UUID;
import java.util.function.BooleanSupplier;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Backup channel pipeline semantics: whitelist gate (empty whitelist =
 * COUNT_ONLY), registry match rules (exactly one ACTIVE row wins; ambiguity
 * and unknown devices are counted, never written), project-mismatch
 * accounting, and the every-discard-is-counted invariant.
 */
class BackupChannelServiceTest {

    private static final String EUI = "0095690e00003588";
    private static final String TB_ID = "0e7a76e0-bae5-11f1-8ac2-9b57e1be74c1";

    private TbClient tbClient;
    private RegisteredDeviceRepository repository;
    private BackupChannelService service;
    private BackupChannelProperties properties;

    @BeforeEach
    void setUp() {
        tbClient = mock(TbClient.class);
        repository = mock(RegisteredDeviceRepository.class);
        properties = new BackupChannelProperties();
        properties.setProjectWhitelist(List.of(219L));
        service = new BackupChannelService(properties, tbClient,
                new TbProperties(), new ObjectMapper(), repository);
    }

    // ------------------------------------------------------------ helpers

    private void handle(String topic, String payload) {
        service.handle(topic, new MqttMessage(payload.getBytes(StandardCharsets.UTF_8)));
    }

    private void handle(BackupChannelService target, String topic, String payload) {
        target.handle(topic, new MqttMessage(payload.getBytes(StandardCharsets.UTF_8)));
    }

    private boolean await(BooleanSupplier condition) throws InterruptedException {
        long deadline = System.currentTimeMillis() + 3000;
        while (System.currentTimeMillis() < deadline) {
            if (condition.getAsBoolean()) return true;
            Thread.sleep(20);
        }
        return false;
    }

    private RegisteredDevice device(RegistrationStatus status, UUID tbId) {
        RegisteredDevice d = new RegisteredDevice();
        d.setDevEui(EUI);
        d.setStatus(status);
        d.setTbDeviceId(tbId);
        return d;
    }

    private void activeDevice() {
        when(repository.findByDevEui(EUI)).thenReturn(
                List.of(device(RegistrationStatus.ACTIVE, UUID.fromString(TB_ID))));
    }

    /** Whitelist is snapshotted at construction — tests that need a different one build a fresh service. */
    private BackupChannelService serviceWithWhitelist(long... projects) {
        BackupChannelProperties p = new BackupChannelProperties();
        java.util.List<Long> list = new java.util.ArrayList<>();
        for (long project : projects) list.add(project);
        p.setProjectWhitelist(list);
        return new BackupChannelService(p, tbClient, new TbProperties(), new ObjectMapper(), repository);
    }

    private final String payload = "{\"project\": 219, \"channel\": \"LoRaWAN\", "
            + "\"ts\": 1790588627.5496006, \"devEUI\": \"" + EUI + "\", \"fPort\": 10, "
            + "\"data\": \"aGt0AA8BBAQDZBABrv5WEQa5+bUVAAAL/2cM/5oNAJk5AAAAAAAAAAAAAAAAAAAe\", "
            + "\"object\": {}}";

    private final String topic = "org/1/project/219/device/" + EUI + "/dat/up";

    // ------------------------------------------------------------ cases

    @Test
    void nonDatUpTopicsAreCountedAndIgnored() {
        handle("org/1/project/17/gateway/aa555a1100000018/stats", "{}");
        assertEquals(1, service.status().counters().otherIgnored());
        verify(repository, never()).findByDevEui(anyString());
    }

    @Test
    void whitelistedActiveFrameIsWrittenWithExplicitTs() throws Exception {
        activeDevice();
        handle(topic, payload);
        assertTrue(await(() -> service.status().counters().written() == 1));
        // ts normalization locked by spike ④: round(1790588627.5496006 * 1000)
        verify(tbClient, times(1)).writeDeviceTelemetry(eq(TB_ID), eq(1790588627550L), anyString());
        assertEquals(1, service.status().counters().todayFrames());
    }

    @Test
    void writtenDataHexMatchesBridgeHexForm() throws Exception {
        activeDevice();
        handle(topic, payload);
        assertTrue(await(() -> service.status().counters().written() == 1));
        org.mockito.ArgumentCaptor<String> hex = org.mockito.ArgumentCaptor.forClass(String.class);
        verify(tbClient).writeDeviceTelemetry(eq(TB_ID), anyLong(), hex.capture());
        assertTrue(hex.getValue().startsWith("686b74"));
        assertEquals(96, hex.getValue().length());
    }

    @Test
    void frameOutsideWhitelistIsCountedNotWritten() {
        handle("org/1/project/130/device/" + EUI + "/dat/up",
                payload.replace("\"project\": 219", "\"project\": 130"));
        assertEquals(1, service.status().counters().notWhitelisted());
        assertEquals(0, service.status().counters().written());
        verify(repository, never()).findByDevEui(anyString());
    }

    @Test
    void emptyWhitelistMeansCountOnly() {
        BackupChannelService countOnly = serviceWithWhitelist();
        handle(countOnly, topic, payload);
        assertEquals(1, countOnly.status().counters().notWhitelisted());
        assertEquals("COUNT_ONLY", countOnly.status().mode());
        verify(tbClient, never()).writeDeviceTelemetry(anyString(), anyLong(), anyString());
    }

    @Test
    void unknownDeviceIsCountedNeverWritten() {
        when(repository.findByDevEui(EUI)).thenReturn(List.of());
        handle(topic, payload);
        assertEquals(1, service.status().counters().unknownDevice());
        verify(tbClient, never()).writeDeviceTelemetry(anyString(), anyLong(), anyString());
    }

    @Test
    void registeredButNotActiveIsCountedSeparately() {
        when(repository.findByDevEui(EUI)).thenReturn(
                List.of(device(RegistrationStatus.DECOMMISSIONED, UUID.fromString(TB_ID))));
        handle(topic, payload);
        assertEquals(1, service.status().counters().inactiveDevice());
        verify(tbClient, never()).writeDeviceTelemetry(anyString(), anyLong(), anyString());
    }

    @Test
    void twoActiveRowsAreAmbiguousNeverAutoBound() {
        when(repository.findByDevEui(EUI)).thenReturn(List.of(
                device(RegistrationStatus.ACTIVE, UUID.fromString(TB_ID)),
                device(RegistrationStatus.ACTIVE, UUID.fromString(
                        "0e883280-bae5-11f1-8ac2-9b57e1be74c1"))));
        handle(topic, payload);
        assertEquals(1, service.status().counters().ambiguousMatch());
        verify(tbClient, never()).writeDeviceTelemetry(anyString(), anyLong(), anyString());
    }

    @Test
    void payloadProjectWinsOverTopicAndMismatchIsCounted() throws Exception {
        // 219 复盘实见：topic project/219 携带 payload project:89 的样本；
        // 白名单按 payload 项目（89）判定 → 写入，同时计数 mismatch
        BackupChannelService p89 = serviceWithWhitelist(89L);
        activeDevice();
        handle(p89, "org/1/project/219/device/" + EUI + "/dat/up",
                payload.replace("\"project\": 219", "\"project\": 89"));
        assertTrue(await(() -> p89.status().counters().written() == 1));
        assertEquals(1, p89.status().counters().projectMismatch());
    }

    @Test
    void matchCacheAvoidsRepeatedRegistryLookups() {
        activeDevice();
        handle(topic, payload);
        handle(topic, payload);
        assertEquals(2, service.status().counters().matched());
        verify(repository, times(1)).findByDevEui(EUI);
    }

    @Test
    void undecodablePayloadIsCounted() {
        activeDevice();
        handle(topic, "{\"project\":219,\"devEUI\":\"" + EUI + "\"}");
        assertEquals(1, service.status().counters().parseFailed());
    }

    @Test
    void statusBeforeStartIsStoppedAndDisabled() {
        BackupChannelService.Status st = service.status();
        assertFalse(st.enabled());
        assertEquals("STOPPED", st.state());
        assertFalse(st.subscribed());
        assertFalse(st.connected());
        assertEquals("WRITE", st.mode());
    }
}
