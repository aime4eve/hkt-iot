package com.hkt.devicehub.application;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.AuditLog;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.AuditLogRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import java.util.List;
import java.util.UUID;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Empty-copy delete hard rules (R-04): telemetry-bearing or locally bound TB
 * devices are refused; refusals and deletions are audited.
 */
class GovernanceServiceTest {

    private static final String TB_ID = "3f6f1b2c-9a4d-4e2f-8c1a-2b3c4d5e6f70";

    private TbClient tbClient;
    private RegisteredDeviceRepository deviceRepository;
    private AuditLogRepository auditLogRepository;
    private GovernanceService service;

    @BeforeEach
    void setUp() {
        tbClient = mock(TbClient.class);
        deviceRepository = mock(RegisteredDeviceRepository.class);
        auditLogRepository = mock(AuditLogRepository.class);
        service = new GovernanceService(tbClient, deviceRepository, auditLogRepository,
                new ObjectMapper());
    }

    @Test
    void telemetryBearingCopyIsRefusedWith422Exception() {
        when(deviceRepository.findAll()).thenReturn(List.of());
        when(tbClient.hasAnyTelemetry(TB_ID)).thenReturn(true);

        assertThrows(UnprocessableEntityException.class,
                () -> service.deleteEmptyCopy(TB_ID, "ops-li"));
        verify(tbClient, never()).deleteDevice(TB_ID);

        ArgumentCaptor<AuditLog> audit = ArgumentCaptor.forClass(AuditLog.class);
        verify(auditLogRepository).save(audit.capture());
        assertEquals("DELETE_TB_DEVICE_REJECTED", audit.getValue().getAction());
        assertEquals("ops-li", audit.getValue().getOperator());
    }

    @Test
    void locallyBoundCopyIsRefused() {
        RegisteredDevice bound = new RegisteredDevice();
        bound.setId(7L);
        bound.setProject(DeviceProject.LIVESTOCK);
        bound.setDevEui("001a0102ff000644");
        bound.setStatus(RegistrationStatus.ACTIVE);
        bound.setTbDeviceId(UUID.fromString(TB_ID));
        when(deviceRepository.findAll()).thenReturn(List.of(bound));

        assertThrows(UnprocessableEntityException.class,
                () -> service.deleteEmptyCopy(TB_ID, null));
        verify(tbClient, never()).deleteDevice(TB_ID);
    }

    @Test
    void emptyUnboundCopyIsDeletedAndAudited() {
        when(deviceRepository.findAll()).thenReturn(List.of());
        when(tbClient.hasAnyTelemetry(TB_ID)).thenReturn(false);

        service.deleteEmptyCopy(TB_ID, null);

        verify(tbClient).deleteDevice(TB_ID);
        ArgumentCaptor<AuditLog> audit = ArgumentCaptor.forClass(AuditLog.class);
        verify(auditLogRepository).save(audit.capture());
        assertEquals("DELETE_TB_DEVICE", audit.getValue().getAction());
        assertEquals("console", audit.getValue().getOperator());
        assertEquals(TB_ID, audit.getValue().getTarget());
    }
}
