package com.hkt.devicehub.application;

import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * R-07 mapping gap derivation: NS projects with devices minus OC-mapped set.
 * Unknown-≠-healthy semantics: NS disabled/unreadable or mapping unreadable
 * must surface as availability flags, never as an empty gap list.
 */
class MappingGapServiceTest {

    private NsClient nsClient;
    private GatewayMappingService gatewayMapping;
    private MappingGapService service;

    @BeforeEach
    void setUp() {
        nsClient = mock(NsClient.class);
        gatewayMapping = mock(GatewayMappingService.class);
        service = new MappingGapService(nsClient, gatewayMapping, new DeviceHubProperties());
    }

    private NsClient.NsDevice device(String eui, int projectId) {
        return new NsClient.NsDevice(eui, projectId, 10, eui, true, 1);
    }

    @Test
    void derivesGapForProjectWithDevicesButNoMapping() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.listDevices(null)).thenReturn(List.of(
                device("001a0102ff000644", 148),
                device("0095690e00003588", 219),
                device("0095690e00003599", 219)));
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of(148));

        MappingGapService.MappingGapReport report = service.derive(
                nsClient.listDevices(null), Set.of(148));

        assertTrue(report.nsAvailable());
        assertTrue(report.mappingReadable());
        assertEquals(List.of(148), report.mappedProjectIds());
        assertEquals(1, report.gaps().size());
        MappingGapService.MappingGap gap = report.gaps().get(0);
        assertEquals(219, gap.nsProjectId());
        assertEquals(2, gap.deviceCount());
        assertEquals(List.of("0095690e00003588", "0095690e00003599"), gap.sampleDevEuis());
    }

    @Test
    void capsSampleEuisAtFive() {
        when(nsClient.isEnabled()).thenReturn(true);
        List<NsClient.NsDevice> many = new java.util.ArrayList<>();
        for (int i = 0; i < 8; i++) {
            many.add(device(String.format("0095690e0000%04x", i), 219));
        }
        MappingGapService.MappingGapReport report = service.derive(many, Set.of());
        assertEquals(8, report.gaps().get(0).deviceCount());
        assertEquals(MappingGapService.SAMPLE_EUI_LIMIT,
                report.gaps().get(0).sampleDevEuis().size());
    }

    @Test
    void skipsProjectZeroFromMissingNsFields() {
        when(nsClient.isEnabled()).thenReturn(true);
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("001a0102ff000644", 0)), Set.of());
        assertTrue(report.gaps().isEmpty());
    }

    @Test
    void fullyMappedProjectSetYieldsNoGaps() {
        when(nsClient.isEnabled()).thenReturn(true);
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("001a0102ff000644", 148)), Set.of(148));
        assertTrue(report.gaps().isEmpty());
    }

    @Test
    void nsDisabledReportsUnavailableWithoutGaps() {
        when(nsClient.isEnabled()).thenReturn(false);
        MappingGapService.MappingGapReport report = service.report();
        assertFalse(report.nsAvailable());
        assertFalse(report.mappingReadable());
        assertTrue(report.gaps().isEmpty());
    }

    @Test
    void mappingReadFailureIsNotReportedAsNoGap() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(gatewayMapping.refreshMappedProjectIds())
                .thenThrow(new IllegalStateException("TB 401"));
        MappingGapService.MappingGapReport report = service.inspect();
        assertTrue(report.nsAvailable());
        assertFalse(report.mappingReadable());
        assertTrue(report.gaps().isEmpty());
        assertEquals("TB 401", report.mappingError());
    }

    @Test
    void nsListFailureKeepsMappingReadableFlag() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of(148));
        when(nsClient.listDevices(null)).thenThrow(new IllegalStateException("NS timeout"));
        MappingGapService.MappingGapReport report = service.inspect();
        assertFalse(report.nsAvailable());
        assertEquals("NS timeout", report.nsError());
        assertTrue(report.mappingReadable());
        assertTrue(report.gaps().isEmpty());
    }

    @Test
    void invalidateForcesRecomputeOnNextReport() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.listDevices(null)).thenReturn(List.of(
                device("0095690e00003588", 219)));
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of());

        service.report();
        service.invalidate();
        service.report();

        verify(nsClient, times(2)).listDevices(null);
    }
}