package com.hkt.devicehub.application.provisioning;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/** Device type correction for already-active registrations. */
class DeviceProvisioningServiceTest {

    private static final String EUI = "00956906000288cf";
    private static final String TB_ID = "c53883c0-b498-11f1-8ac2-9b57e1be74c1";

    private TbClient tbClient;
    private RegisteredDeviceRepository deviceRepository;
    private DeviceProvisioningService service;

    @BeforeEach
    void setUp() {
        tbClient = mock(TbClient.class);
        deviceRepository = mock(RegisteredDeviceRepository.class);
        service = new DeviceProvisioningService(tbClient, deviceRepository,
                new NoOpNsReconciliation(), new ObjectMapper(), new DeviceHubProperties());
    }

    @Test
    void activeRegistrationBackfillsMissingDeviceType() {
        RegisteredDevice device = new RegisteredDevice();
        device.setDevEui(EUI);
        device.setProject(DeviceProject.PARKING);
        device.setStatus(RegistrationStatus.ACTIVE);
        when(deviceRepository.findByProjectAndDevEui(DeviceProject.PARKING, EUI))
                .thenReturn(Optional.of(device));

        var result = service.register(new DeviceProvisioningService.RegisterCommand(
                EUI, "PARKING", null, "geomagnetic", null));

        assertEquals("ALREADY_REGISTERED", result.result());
        assertEquals("GEOMAGNETIC", device.getDeviceType());
        assertEquals(3600, device.getExpectedReportIntervalSeconds());
        verify(deviceRepository).save(device);
    }
}
