package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import com.hkt.devicehub.infrastructure.thingsboard.TbWebSocketChannel;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.ObjectProvider;

import java.util.List;
import java.util.Map;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * The topology gateway-mapping probe must distinguish "cannot read the OC
 * shared attributes" (readable=false → console is-down) from "read OK, N
 * projects mapped". Regression: the probe used to go through the swallowing
 * mappedProjectIds() cache, so a TB outage surfaced as "0 mappings" instead
 * of a read failure.
 */
class TopologyServiceTest {

    private GatewayMappingService gatewayMapping;
    private TopologyService service;

    @BeforeEach
    void setUp() {
        TbClient tbClient = mock(TbClient.class);
        TbProperties tbProperties = new TbProperties();
        tbProperties.setEnabled(false); // auth probe short-circuits; not under test
        DeviceHubProperties properties = new DeviceHubProperties();
        properties.getTopology().setNsHost("127.0.0.1");
        properties.getTopology().setNsPort(9); // discard port: fast refusal
        gatewayMapping = mock(GatewayMappingService.class);
        RegisteredDeviceRepository deviceRepository = mock(RegisteredDeviceRepository.class);
        when(deviceRepository.countByStatus(RegistrationStatus.ACTIVE)).thenReturn(0L);
        when(deviceRepository.findByStatus(RegistrationStatus.ACTIVE)).thenReturn(List.of());
        TelemetryChannelMetrics metrics = mock(TelemetryChannelMetrics.class);
        when(metrics.channelStateSnapshot()).thenReturn(Map.of());
        @SuppressWarnings("unchecked")
        ObjectProvider<TbWebSocketChannel> wsChannel = mock(ObjectProvider.class);
        service = new TopologyService(tbClient, tbProperties, properties, gatewayMapping,
                deviceRepository, metrics, wsChannel);
    }

    @Test
    void mappingReadFailureIsReportedUnreadable() {
        when(gatewayMapping.refreshMappedProjectIds())
                .thenThrow(new IllegalStateException("TB 401"));
        TopologyService.TopologyResponse t = service.topology();
        assertFalse(t.gatewayMapping().readable());
        assertEquals("TB 401", t.gatewayMapping().error());
        assertEquals(0, t.gatewayMapping().mappedProjectCount());
    }

    @Test
    void successfulReadReportsMappedProjects() {
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of(148, 219));
        TopologyService.TopologyResponse t = service.topology();
        assertTrue(t.gatewayMapping().readable());
        assertEquals(2, t.gatewayMapping().mappedProjectCount());
        assertEquals(List.of(148, 219), t.gatewayMapping().mappedProjectIds());
    }
}
