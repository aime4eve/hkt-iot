package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * Preflight five-check verdict matrix (R-01).
 */
class PreflightServiceTest {

    private static final String EUI = "001a0102ff000644";
    private static final String TB_ID = "3f6f1b2c-9a4d-4e2f-8c1a-2b3c4d5e6f70";
    private static final String PROFILE_ID = "profile-uuid-1";

    private NsClient nsClient;
    private TbClient tbClient;
    private GatewayMappingService gatewayMapping;
    private RegisteredDeviceRepository deviceRepository;
    private DeviceHubProperties properties;
    private PreflightService service;

    @BeforeEach
    void setUp() {
        nsClient = mock(NsClient.class);
        tbClient = mock(TbClient.class);
        gatewayMapping = mock(GatewayMappingService.class);
        deviceRepository = mock(RegisteredDeviceRepository.class);
        properties = new DeviceHubProperties();
        DeviceHubProperties.ProfileRef capsule = new DeviceHubProperties.ProfileRef();
        capsule.setName("瘤胃胶囊-OC-配置-v2");
        properties.getProfiles().put("CAPSULE", capsule);
        service = new PreflightService(nsClient, tbClient, gatewayMapping,
                deviceRepository, properties);
    }

    private void nsDevicePresent(int projectId, boolean online, int fCntUp) {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.findDeviceByEui(EUI)).thenReturn(Optional.of(
                new NsClient.NsDevice(EUI, projectId, 10, "瘤胃胶囊-644", online, fCntUp)));
    }

    // ① NS existence

    @Test
    void nsDisabledYieldsWarn() {
        when(nsClient.isEnabled()).thenReturn(false);
        assertEquals(PreflightService.Verdict.WARN, service.checkNs(EUI).verdict());
    }

    @Test
    void nsMissingYieldsFail() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.findDeviceByEui(EUI)).thenReturn(Optional.empty());
        assertEquals(PreflightService.Verdict.FAIL, service.checkNs(EUI).verdict());
    }

    @Test
    void nsPresentYieldsPassWithOwnershipEvidence() {
        nsDevicePresent(148, true, 34);
        PreflightService.CheckResult check = service.checkNs(EUI);
        assertEquals(PreflightService.Verdict.PASS, check.verdict());
        assertEquals(148, check.evidence().get("nsProjectId"));
    }

    // ② gateway mapping

    @Test
    void unmappedNsProjectYieldsFailNamingTheProject() {
        nsDevicePresent(148, false, 0);
        when(gatewayMapping.isMapped(148)).thenReturn(false);
        PreflightService.CheckResult nsCheck = service.checkNs(EUI);
        PreflightService.CheckResult check = service.checkGatewayMapping(nsCheck);
        assertEquals(PreflightService.Verdict.FAIL, check.verdict());
        assertTrue(check.advice().contains("项目 148 无网关映射"));
    }

    @Test
    void mappedNsProjectYieldsPass() {
        nsDevicePresent(217, true, 5);
        when(gatewayMapping.isMapped(217)).thenReturn(true);
        PreflightService.CheckResult check = service.checkGatewayMapping(service.checkNs(EUI));
        assertEquals(PreflightService.Verdict.PASS, check.verdict());
    }

    @Test
    void mappingCheckSkippedWhenNsFailed() {
        when(nsClient.isEnabled()).thenReturn(true);
        when(nsClient.findDeviceByEui(EUI)).thenReturn(Optional.empty());
        PreflightService.CheckResult check = service.checkGatewayMapping(service.checkNs(EUI));
        assertEquals(PreflightService.Verdict.WARN, check.verdict());
    }

    // ③ TB uniqueness

    @Test
    void zeroTbMatchesYieldsWarnCreatable() {
        when(tbClient.findDevices(EUI)).thenReturn(List.of());
        assertEquals(PreflightService.Verdict.WARN, service.checkTbUniqueness(EUI).verdict());
    }

    @Test
    void exactlyOneTbMatchYieldsPass() {
        when(tbClient.findDevices(EUI)).thenReturn(
                List.of(new TbClient.TbDeviceView(TB_ID, EUI, 0, PROFILE_ID)));
        assertEquals(PreflightService.Verdict.PASS, service.checkTbUniqueness(EUI).verdict());
    }

    @Test
    void multipleTbMatchesYieldFailWithCandidates() {
        when(tbClient.findDevices(EUI)).thenReturn(List.of(
                new TbClient.TbDeviceView(TB_ID, EUI, 0, PROFILE_ID),
                new TbClient.TbDeviceView("other-id", EUI.toUpperCase(), 0, PROFILE_ID)));
        PreflightService.CheckResult check = service.checkTbUniqueness(EUI);
        assertEquals(PreflightService.Verdict.FAIL, check.verdict());
        assertEquals(List.of(TB_ID, "other-id"), check.evidence().get("candidates"));
    }

    // ④ profile matching

    @Test
    void profileCheckWarnsWithoutDeviceType() {
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of());
        when(tbClient.findDevices(EUI)).thenReturn(List.of());
        PreflightService.CheckResult check =
                service.checkProfile(EUI, null, service.checkTbUniqueness(EUI));
        assertEquals(PreflightService.Verdict.WARN, check.verdict());
    }

    @Test
    void profileMissingInTbYieldsFail() {
        when(tbClient.findDevices(EUI)).thenReturn(List.of());
        when(tbClient.fetchDeviceProfiles()).thenReturn(Map.of("other-id", "别的profile"));
        PreflightService.CheckResult check =
                service.checkProfile(EUI, "CAPSULE", service.checkTbUniqueness(EUI));
        assertEquals(PreflightService.Verdict.FAIL, check.verdict());
        assertTrue(check.advice().contains("瘤胃胶囊-OC-配置-v2"));
    }

    @Test
    void profileMismatchOnExistingDeviceYieldsFail() {
        when(tbClient.findDevices(EUI)).thenReturn(
                List.of(new TbClient.TbDeviceView(TB_ID, EUI, 0, "wrong-profile")));
        when(tbClient.fetchDeviceProfiles()).thenReturn(Map.of(PROFILE_ID, "瘤胃胶囊-OC-配置-v2"));
        PreflightService.CheckResult check =
                service.checkProfile(EUI, "CAPSULE", service.checkTbUniqueness(EUI));
        assertEquals(PreflightService.Verdict.FAIL, check.verdict());
    }

    @Test
    void profileMatchYieldsPass() {
        when(tbClient.findDevices(EUI)).thenReturn(
                List.of(new TbClient.TbDeviceView(TB_ID, EUI, 0, PROFILE_ID)));
        when(tbClient.fetchDeviceProfiles()).thenReturn(Map.of(PROFILE_ID, "瘤胃胶囊-OC-配置-v2"));
        PreflightService.CheckResult check =
                service.checkProfile(EUI, "CAPSULE", service.checkTbUniqueness(EUI));
        assertEquals(PreflightService.Verdict.PASS, check.verdict());
    }

    // ⑤ local binding

    @Test
    void unregisteredYieldsWarn() {
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of());
        assertEquals(PreflightService.Verdict.WARN, service.checkLocalBinding(EUI).verdict());
    }

    @Test
    void activeBindingYieldsPass() {
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of(local(RegistrationStatus.ACTIVE)));
        assertEquals(PreflightService.Verdict.PASS, service.checkLocalBinding(EUI).verdict());
    }

    @Test
    void failedBindingYieldsWarn() {
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of(local(RegistrationStatus.FAILED)));
        assertEquals(PreflightService.Verdict.WARN, service.checkLocalBinding(EUI).verdict());
    }

    @Test
    void overallVerdictIsTheWorstCheck() {
        nsDevicePresent(148, true, 1);
        when(gatewayMapping.isMapped(148)).thenReturn(false); // FAIL
        when(tbClient.findDevices(EUI)).thenReturn(List.of()); // WARN
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of()); // WARN
        PreflightService.PreflightReport report = service.preflight(EUI, null);
        assertEquals(PreflightService.Verdict.FAIL, report.overall());
        assertEquals(5, report.checks().size());
    }

    private static RegisteredDevice local(RegistrationStatus status) {
        RegisteredDevice device = new RegisteredDevice();
        device.setDevEui(EUI);
        device.setProject(DeviceProject.LIVESTOCK);
        device.setStatus(status);
        return device;
    }
}
