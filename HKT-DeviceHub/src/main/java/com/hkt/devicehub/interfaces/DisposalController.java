package com.hkt.devicehub.interfaces;

import com.fasterxml.jackson.databind.JsonNode;
import com.hkt.devicehub.application.disposal.DisposalService;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.DisposalTicket;
import io.swagger.v3.oas.annotations.Operation;
import io.swagger.v3.oas.annotations.tags.Tag;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestHeader;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RestController;

import java.util.List;
import java.util.Map;

/**
 * No-data device disposal workflow (console v1.5 §3.4). DingTalk approval is
 * mocked this phase via OaWorkflowPort; the console's approve/reject buttons
 * simulate the bpms_instance_change callback.
 */
@RestController
@RequestMapping("/api/v1/disposals")
@Tag(name = "Disposals", description = "No-data device disposal queue (console §3.4)")
public class DisposalController {

    private final DisposalService disposalService;

    public DisposalController(DisposalService disposalService) {
        this.disposalService = disposalService;
    }

    public record DisposalView(Long id, String devEui, String project, String severity,
                               String category, String status, String assignee,
                               String dingtalkInstanceId, String decommissionReason,
                               java.time.Instant firstFrameAt, java.time.Instant createdAt,
                               java.time.Instant updatedAt, JsonNode triageEvidence,
                               JsonNode timeline) {}

    @GetMapping
    @Operation(summary = "Disposal ticket list (status/project filters)")
    public List<DisposalView> list(@RequestParam(required = false) DisposalTicket.Status status,
                                   @RequestParam(required = false) DeviceProject project) {
        return disposalService.list(status, project).stream().map(this::toView).toList();
    }

    @GetMapping("/{id}")
    @Operation(summary = "Ticket detail with timeline")
    public DisposalView detail(@PathVariable long id) {
        return toView(disposalService.detail(id));
    }

    @PostMapping("/{id}/submit-approval")
    @Operation(summary = "Assign + push DingTalk approval (mock): PENDING_ASSIGN → APPROVING")
    public DisposalView submitApproval(@PathVariable long id, @RequestParam String assignee) {
        return toView(disposalService.submitApproval(id, assignee));
    }

    @PostMapping("/{id}/approve-callback")
    @Operation(summary = "Mock DingTalk bpms_instance_change callback: approved → PROCESSING, rejected → PENDING_ASSIGN")
    public DisposalView approveCallback(@PathVariable long id, @RequestParam boolean approved) {
        return toView(disposalService.approveCallback(id, approved));
    }

    @PostMapping("/{id}/mark-handled")
    @Operation(summary = "Handled on site: PROCESSING → OBSERVING (deadline = now + 2×interval)")
    public DisposalView markHandled(
            @PathVariable long id,
            @RequestHeader(name = "X-Operator", required = false) String operator) {
        return toView(disposalService.markHandled(id, operator));
    }

    @PostMapping("/{id}/decommission")
    @Operation(summary = "Decommission (reason mandatory): any open state → DECOMMISSIONED, device retired")
    public DisposalView decommission(
            @PathVariable long id,
            @RequestBody Map<String, String> body,
            @RequestHeader(name = "X-Operator", required = false) String operator) {
        return toView(disposalService.decommission(id, body.get("reason"), operator));
    }

    private DisposalView toView(DisposalTicket ticket) {
        JsonNode triage;
        try {
            triage = new com.fasterxml.jackson.databind.ObjectMapper()
                    .readTree(ticket.getTriageEvidence() == null ? "{}" : ticket.getTriageEvidence());
        } catch (Exception e) {
            triage = null;
        }
        return new DisposalView(
                ticket.getId(), ticket.getDevEui(), ticket.getProject().name(),
                ticket.getSeverity().name(),
                ticket.getCategory() == null ? null : ticket.getCategory().name(),
                ticket.getStatus().name(), ticket.getAssignee(), ticket.getDingtalkInstanceId(),
                ticket.getDecommissionReason(), ticket.getFirstFrameAt(),
                ticket.getCreatedAt(), ticket.getUpdatedAt(),
                triage, disposalService.timelineOf(ticket));
    }
}
