package com.hkt.devicehub.application;

import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.time.Instant;
import java.util.List;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * R-07 mapping gap derivation: NS projects with devices minus OC-mapped set.
 * Unknown-≠-healthy semantics: NS disabled/unreadable or mapping unreadable
 * must surface as availability flags, never as an empty gap list.
 * Since 2026-10-09 gaps split active (heard within gapActiveWindowMs — live
 * frame loss) from dormant (stale/never-heard inventory on this shared NS).
 */
class MappingGapServiceTest {

    private static final Instant NOW = Instant.parse("2026-10-09T00:00:00Z");
    private static final Instant RECENT = Instant.parse("2026-10-08T20:00:00Z");
    private static final Instant STALE = Instant.parse("2026-09-01T00:00:00Z");

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
        return device(eui, projectId, RECENT);
    }

    private NsClient.NsDevice device(String eui, int projectId, Instant lastSeen) {
        return new NsClient.NsDevice(eui, projectId, 10, eui, true, 1, lastSeen);
    }

    @Test
    void derivesGapForProjectWithDevicesButNoMapping() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("001a0102ff000644", 148),
                        device("0095690e00003588", 219),
                        device("0095690e00003599", 219)),
                Set.of(148), NOW);

        assertTrue(report.nsAvailable());
        assertTrue(report.mappingReadable());
        assertEquals(List.of(148), report.mappedProjectIds());
        assertEquals(1, report.gaps().size());
        MappingGapService.MappingGap gap = report.gaps().get(0);
        assertEquals(219, gap.nsProjectId());
        assertEquals(2, gap.deviceCount());
        assertEquals(List.of("0095690e00003588", "0095690e00003599"), gap.sampleDevEuis());
        assertEquals(RECENT, gap.lastSeenAt());
        assertTrue(report.dormantGaps().isEmpty());
    }

    @Test
    void staleProjectsAreDormantNotActiveGaps() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("0095690e00003588", 219, STALE),
                        device("001a0102ff000644", 148, RECENT)),
                Set.of(), NOW);
        assertEquals(List.of(148), report.gaps().stream().map(MappingGapService.MappingGap::nsProjectId).toList());
        assertEquals(1, report.dormantGaps().size());
        assertEquals(219, report.dormantGaps().get(0).nsProjectId());
        assertEquals(STALE, report.dormantGaps().get(0).lastSeenAt());
    }

    @Test
    void neverHeardProjectsAreDormant() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("0095690e00003588", 219, null)), Set.of(), NOW);
        assertTrue(report.gaps().isEmpty());
        assertEquals(1, report.dormantGaps().size());
        assertNull(report.dormantGaps().get(0).lastSeenAt());
    }

    @Test
    void projectIsActiveWhenAnyDeviceHeardWithinWindow() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("0095690e00003588", 219, STALE),
                        device("0095690e00003599", 219, RECENT)),
                Set.of(), NOW);
        assertEquals(1, report.gaps().size());
        assertEquals(RECENT, report.gaps().get(0).lastSeenAt());
        assertTrue(report.dormantGaps().isEmpty());
    }

    @Test
    void capsSampleEuisAtFive() {
        List<NsClient.NsDevice> many = new java.util.ArrayList<>();
        for (int i = 0; i < 8; i++) {
            many.add(device(String.format("0095690e0000%04x", i), 219));
        }
        MappingGapService.MappingGapReport report = service.derive(many, Set.of(), NOW);
        assertEquals(8, report.gaps().get(0).deviceCount());
        assertEquals(MappingGapService.SAMPLE_EUI_LIMIT,
                report.gaps().get(0).sampleDevEuis().size());
    }

    @Test
    void skipsProjectZeroFromMissingNsFields() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("001a0102ff000644", 0)), Set.of(), NOW);
        assertTrue(report.gaps().isEmpty());
        assertTrue(report.dormantGaps().isEmpty());
    }

    @Test
    void fullyMappedProjectSetYieldsNoGaps() {
        MappingGapService.MappingGapReport report = service.derive(
                List.of(device("001a0102ff000644", 148)), Set.of(148), NOW);
        assertTrue(report.gaps().isEmpty());
        assertTrue(report.dormantGaps().isEmpty());
    }

    @Test
    void nsDisabledReportsUnavailableWithoutGaps() {
        when(nsClient.isEnabled()).thenReturn(false);
        MappingGapService.MappingGapReport report = service.refreshNow();
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
    void reportIsNonBlockingAndServesStaleWhileRevalidating() throws Exception {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.listDevices(null)).thenReturn(List.of(device("0095690e00003588", 219)));
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of());

        // cold start: no cache yet → computing placeholder, refresh kicked async
        MappingGapService.MappingGapReport first = service.report();
        assertFalse(first.nsAvailable());

        // async refresh lands; subsequent report() serves the real scan
        long deadline = System.currentTimeMillis() + 5_000;
        MappingGapService.MappingGapReport later = first;
        while (System.currentTimeMillis() < deadline) {
            later = service.report();
            if (later.nsAvailable()) break;
            Thread.sleep(20);
        }
        assertTrue(later.nsAvailable());
        assertEquals(1, later.gaps().size());
    }

    @Test
    void invalidateTriggersAsyncRecompute() throws Exception {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.listDevices(null)).thenReturn(List.of(
                device("0095690e00003588", 219)));
        when(gatewayMapping.refreshMappedProjectIds()).thenReturn(Set.of());

        service.refreshNow();
        service.refreshNow(); // cache fresh: no second NS call
        service.invalidate();

        long deadline = System.currentTimeMillis() + 5_000;
        while (System.currentTimeMillis() < deadline) {
            try {
                verify(nsClient, times(2)).listDevices(null);
                return;
            } catch (AssertionError notYet) {
                Thread.sleep(20);
            }
        }
        verify(nsClient, times(2)).listDevices(null);
    }
}