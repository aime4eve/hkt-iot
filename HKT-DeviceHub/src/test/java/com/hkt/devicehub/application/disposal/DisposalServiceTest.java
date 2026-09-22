package com.hkt.devicehub.application.disposal;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.DisposalTicket;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.DisposalTicketRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import com.hkt.devicehub.infrastructure.oa.MockOaWorkflowAdapter;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;

import java.time.Instant;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.when;

/**
 * Disposal state machine: submit → approve/reject → handled → observation
 * auto-resolve / decommission. Timeline entries accumulate per transition.
 */
class DisposalServiceTest {

    private static final String EUI = "001a0102ff000644";

    private DisposalTicketRepository ticketRepository;
    private RegisteredDeviceRepository deviceRepository;
    private NsClient nsClient;
    private DisposalService service;
    private DisposalTicket ticket;

    @BeforeEach
    void setUp() {
        ticketRepository = Mockito.mock(DisposalTicketRepository.class);
        deviceRepository = Mockito.mock(RegisteredDeviceRepository.class);
        nsClient = Mockito.mock(NsClient.class);
        service = new DisposalService(ticketRepository, deviceRepository, nsClient,
                new MockOaWorkflowAdapter(), new ObjectMapper());

        ticket = new DisposalTicket();
        ticket.setId(1L);
        ticket.setDevEui(EUI);
        ticket.setProject(DeviceProject.LIVESTOCK);
        ticket.setStatus(DisposalTicket.Status.PENDING_ASSIGN);
        ticket.setTimeline("[]");
        when(ticketRepository.findById(1L)).thenReturn(Optional.of(ticket));
        when(ticketRepository.save(any())).thenAnswer(inv -> inv.getArgument(0));
    }

    @Test
    void submitApprovalMovesToApprovingWithDingtalkInstance() {
        DisposalTicket result = service.submitApproval(1L, "oncall-zhang");
        assertEquals(DisposalTicket.Status.APPROVING, result.getStatus());
        assertNotNull(result.getDingtalkInstanceId());
        assertTrue(result.getDingtalkInstanceId().matches("DD-\\d{8}-\\d{4}"));
        assertEquals("oncall-zhang", result.getAssignee());
        assertTrue(result.getTimeline().contains("SUBMIT_APPROVAL"));
    }

    @Test
    void submitApprovalRequiresAssignee() {
        assertThrows(IllegalArgumentException.class, () -> service.submitApproval(1L, " "));
    }

    @Test
    void submitApprovalFromWrongStateRejected() {
        ticket.setStatus(DisposalTicket.Status.PROCESSING);
        assertThrows(IllegalStateException.class, () -> service.submitApproval(1L, "x"));
    }

    @Test
    void approveCallbackApprovedMovesToProcessing() {
        service.submitApproval(1L, "oncall-zhang");
        DisposalTicket result = service.approveCallback(1L, true);
        assertEquals(DisposalTicket.Status.PROCESSING, result.getStatus());
        assertTrue(result.getTimeline().contains("APPROVED"));
    }

    @Test
    void approveCallbackRejectedFallsBackToPendingAssign() {
        service.submitApproval(1L, "oncall-zhang");
        DisposalTicket result = service.approveCallback(1L, false);
        assertEquals(DisposalTicket.Status.PENDING_ASSIGN, result.getStatus());
        assertTrue(result.getTimeline().contains("REJECTED"));
    }

    @Test
    void markHandledMovesToObservingWithDeadline() {
        service.submitApproval(1L, "oncall-zhang");
        service.approveCallback(1L, true);
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of(device(RegistrationStatus.ACTIVE)));
        DisposalTicket result = service.markHandled(1L, "console");
        assertEquals(DisposalTicket.Status.OBSERVING, result.getStatus());
        assertTrue(result.getTimeline().contains("OBSERVING_STARTED"));
        assertTrue(result.getTimeline().contains("observationDeadlineAt"));
    }

    @Test
    void frameArrivalAutoResolvesObservingTicket() {
        service.submitApproval(1L, "oncall-zhang");
        service.approveCallback(1L, true);
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of(device(RegistrationStatus.ACTIVE)));
        service.markHandled(1L, "console");
        when(ticketRepository.findFirstByDevEuiAndStatusIn(anyString(), any(Set.class)))
                .thenAnswer(inv -> {
                    @SuppressWarnings("unchecked")
                    Set<DisposalTicket.Status> statuses = inv.getArgument(1);
                    return statuses.contains(ticket.getStatus())
                            ? Optional.of(ticket) : Optional.empty();
                });
        Instant frameAt = Instant.now();
        service.onFrameArrived(EUI, frameAt);
        assertEquals(DisposalTicket.Status.RESOLVED, ticket.getStatus());
        assertEquals(frameAt, ticket.getFirstFrameAt());
        assertTrue(ticket.getTimeline().contains("AUTO_RESOLVED"));
    }

    @Test
    void decommissionRequiresReason() {
        assertThrows(IllegalArgumentException.class,
                () -> service.decommission(1L, " ", "console"));
    }

    @Test
    void decommissionClosesTicketAndRetiresDevice() {
        RegisteredDevice device = device(RegistrationStatus.ACTIVE);
        when(deviceRepository.findByDevEui(EUI)).thenReturn(List.of(device));
        DisposalTicket result = service.decommission(1L, "设备硬件报废", "console");
        assertEquals(DisposalTicket.Status.DECOMMISSIONED, result.getStatus());
        assertEquals("设备硬件报废", result.getDecommissionReason());
        assertEquals(RegistrationStatus.DECOMMISSIONED, device.getStatus());
        assertTrue(result.getTimeline().contains("DECOMMISSIONED"));
    }

    @Test
    void decommissionOnClosedTicketRejected() {
        ticket.setStatus(DisposalTicket.Status.RESOLVED);
        assertThrows(IllegalStateException.class,
                () -> service.decommission(1L, "reason", "console"));
    }

    private static RegisteredDevice device(RegistrationStatus status) {
        RegisteredDevice device = new RegisteredDevice();
        device.setDevEui(EUI);
        device.setProject(DeviceProject.LIVESTOCK);
        device.setStatus(status);
        device.setExpectedReportIntervalSeconds(14400);
        return device;
    }
}
