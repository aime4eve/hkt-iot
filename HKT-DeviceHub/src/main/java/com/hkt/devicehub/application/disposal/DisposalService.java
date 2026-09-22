package com.hkt.devicehub.application.disposal;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.DisposalTicket;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.DisposalTicketRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import com.hkt.devicehub.infrastructure.oa.OaWorkflowPort;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * No-data device disposal workflow (console v1.5 §3.4):
 * auto-ticket + auto-triage → DingTalk(mock) approval → handling →
 * observation window (auto-resolve on first frame, escalate on timeout) →
 * resolved / decommissioned. Every transition appends a timeline entry.
 */
@Service
@Slf4j
public class DisposalService {

    private static final Set<DisposalTicket.Status> OPEN_STATUSES = Set.of(
            DisposalTicket.Status.PENDING_ASSIGN, DisposalTicket.Status.APPROVING,
            DisposalTicket.Status.PROCESSING, DisposalTicket.Status.OBSERVING);
    private static final long CRITICAL_AFTER_MS = 24 * 3_600_000L;

    private final DisposalTicketRepository ticketRepository;
    private final RegisteredDeviceRepository deviceRepository;
    private final NsClient nsClient;
    private final OaWorkflowPort oaWorkflow;
    private final ObjectMapper objectMapper;

    public DisposalService(DisposalTicketRepository ticketRepository,
                           RegisteredDeviceRepository deviceRepository,
                           NsClient nsClient, OaWorkflowPort oaWorkflow,
                           ObjectMapper objectMapper) {
        this.ticketRepository = ticketRepository;
        this.deviceRepository = deviceRepository;
        this.nsClient = nsClient;
        this.oaWorkflow = oaWorkflow;
        this.objectMapper = objectMapper;
    }

    // ------------------------------------------------------------- auto-create

    /**
     * Scan for devices registered longer than 2×their expected interval with
     * zero frames. Idempotent: one open ticket per device at any time.
     */
    @Scheduled(fixedDelayString = "${devicehub.disposal.scan-interval-ms:300000}",
            initialDelayString = "${devicehub.disposal.scan-initial-delay-ms:60000}")
    @Transactional
    public void scanNoDataDevices() {
        long now = System.currentTimeMillis();
        for (RegisteredDevice device : deviceRepository.findByStatus(RegistrationStatus.ACTIVE)) {
            if (device.getLastFrameAt() != null) continue;
            long silenceMs = now - device.getCreatedAt().toEpochMilli();
            long windowMs = 2L * device.getExpectedReportIntervalSeconds() * 1000L;
            if (silenceMs < windowMs) continue;
            if (ticketRepository.findFirstByDevEuiAndStatusIn(device.getDevEui(), OPEN_STATUSES)
                    .isPresent()) {
                continue;
            }
            DisposalTicket ticket = new DisposalTicket();
            ticket.setDevEui(device.getDevEui());
            ticket.setProject(device.getProject());
            ticket.setSeverity(silenceMs > CRITICAL_AFTER_MS
                    ? DisposalTicket.Severity.CRITICAL : DisposalTicket.Severity.NOTICE);
            triage(device, ticket);
            ticket.setTimeline(appendTimeline("[]", "CREATED", "auto-scanner",
                    Map.of("silenceMs", silenceMs, "windowMs", windowMs)));
            ticket.setTimeline(appendTimeline(ticket.getTimeline(), "TRIAGED", "auto-triage",
                    Map.of("category", ticket.getCategory().name())));
            ticketRepository.save(ticket);
            log.info("[Disposal] ticket created for {} ({} {}, {})",
                    device.getDevEui(), ticket.getSeverity(), ticket.getCategory(),
                    ticket.getProject());
        }
    }

    /**
     * Auto-triage (console §3.4 ①): NS unreachable / offline / zero frames →
     * FIELD; NS has uplinks but TB has none → PLATFORM; NS lookup failure
     * defaults to FIELD with the failure recorded as evidence.
     */
    void triage(RegisteredDevice device, DisposalTicket ticket) {
        Map<String, Object> evidence = new LinkedHashMap<>();
        evidence.put("expectedReportIntervalSeconds", device.getExpectedReportIntervalSeconds());
        evidence.put("tbLastFrameAt", null);
        if (!nsClient.isEnabled()) {
            evidence.put("nsCheck", "NS client disabled");
            ticket.setCategory(DisposalTicket.Category.FIELD);
        } else {
            try {
                var nsDevice = nsClient.findDeviceByEui(device.getDevEui());
                if (nsDevice.isEmpty()) {
                    evidence.put("nsCheck", "device not found in NS");
                    ticket.setCategory(DisposalTicket.Category.FIELD);
                } else {
                    var ns = nsDevice.get();
                    evidence.put("nsProjectId", ns.projectId());
                    evidence.put("nsOnline", ns.online());
                    evidence.put("nsFrameCountUp", ns.fCntUp());
                    boolean hasUplink = ns.fCntUp() != null && ns.fCntUp() > 0;
                    ticket.setCategory(hasUplink
                            ? DisposalTicket.Category.PLATFORM : DisposalTicket.Category.FIELD);
                }
            } catch (Exception e) {
                evidence.put("nsCheck", "NS query failed: " + e.getMessage());
                ticket.setCategory(DisposalTicket.Category.FIELD);
            }
        }
        try {
            ticket.setTriageEvidence(objectMapper.writeValueAsString(evidence));
        } catch (Exception e) {
            ticket.setTriageEvidence("{}");
        }
    }

    // ------------------------------------------------------------- transitions

    @Transactional
    public DisposalTicket submitApproval(long id, String assignee) {
        DisposalTicket ticket = require(id);
        requireStatus(ticket, DisposalTicket.Status.PENDING_ASSIGN);
        if (assignee == null || assignee.isBlank()) {
            throw new IllegalArgumentException("assignee is required");
        }
        ticket.setAssignee(assignee);
        ticket.setDingtalkInstanceId(oaWorkflow.startApproval(ticket, assignee));
        ticket.setStatus(DisposalTicket.Status.APPROVING);
        ticket.setTimeline(appendTimeline(ticket.getTimeline(), "SUBMIT_APPROVAL", "console",
                Map.of("assignee", assignee, "dingtalkInstanceId", ticket.getDingtalkInstanceId())));
        oaWorkflow.notifyAssignee(ticket, assignee, "无数据设备处置审批：" + ticket.getDevEui());
        return ticketRepository.save(ticket);
    }

    @Transactional
    public DisposalTicket approveCallback(long id, boolean approved) {
        DisposalTicket ticket = require(id);
        requireStatus(ticket, DisposalTicket.Status.APPROVING);
        ticket.setStatus(approved
                ? DisposalTicket.Status.PROCESSING : DisposalTicket.Status.PENDING_ASSIGN);
        ticket.setTimeline(appendTimeline(ticket.getTimeline(),
                approved ? "APPROVED" : "REJECTED", "dingtalk(mock)",
                Map.of("dingtalkInstanceId",
                        ticket.getDingtalkInstanceId() == null ? "" : ticket.getDingtalkInstanceId())));
        return ticketRepository.save(ticket);
    }

    @Transactional
    public DisposalTicket markHandled(long id, String operator) {
        DisposalTicket ticket = require(id);
        requireStatus(ticket, DisposalTicket.Status.PROCESSING);
        ticket.setStatus(DisposalTicket.Status.OBSERVING);
        Instant deadline = Instant.now()
                .plusSeconds(2L * intervalSecondsOf(ticket.getDevEui()));
        ticket.setTimeline(appendTimeline(ticket.getTimeline(), "OBSERVING_STARTED",
                operator == null ? "console" : operator,
                Map.of("observationDeadlineAt", deadline.toString())));
        return ticketRepository.save(ticket);
    }

    @Transactional
    public DisposalTicket decommission(long id, String reason, String operator) {
        DisposalTicket ticket = require(id);
        if (reason == null || reason.isBlank()) {
            throw new IllegalArgumentException("decommission reason is required");
        }
        if (ticket.getStatus() == DisposalTicket.Status.RESOLVED
                || ticket.getStatus() == DisposalTicket.Status.DECOMMISSIONED) {
            throw new IllegalStateException("ticket " + id + " already closed: " + ticket.getStatus());
        }
        ticket.setStatus(DisposalTicket.Status.DECOMMISSIONED);
        ticket.setDecommissionReason(reason);
        ticket.setTimeline(appendTimeline(ticket.getTimeline(), "DECOMMISSIONED",
                operator == null ? "console" : operator, Map.of("reason", reason)));
        deviceRepository.findByDevEui(ticket.getDevEui()).forEach(device -> {
            device.setStatus(RegistrationStatus.DECOMMISSIONED);
            deviceRepository.save(device);
        });
        return ticketRepository.save(ticket);
    }

    // --------------------------------------------------- observation lifecycle

    /**
     * Observation auto-close hook (console §3.4 ③): called on every published
     * frame; an OBSERVING ticket for the device is resolved with the first
     * frame timestamp. Fail-open — never blocks the ingest path.
     */
    @Transactional
    public void onFrameArrived(String devEui, Instant frameAt) {
        ticketRepository.findFirstByDevEuiAndStatusIn(devEui,
                        Set.of(DisposalTicket.Status.OBSERVING))
                .ifPresent(ticket -> {
                    ticket.setStatus(DisposalTicket.Status.RESOLVED);
                    ticket.setFirstFrameAt(frameAt);
                    ticket.setTimeline(appendTimeline(ticket.getTimeline(), "AUTO_RESOLVED",
                            "telemetry-hook", Map.of("firstFrameAt", frameAt.toString())));
                    ticketRepository.save(ticket);
                    log.info("[Disposal] ticket {} auto-resolved by first frame of {}",
                            ticket.getId(), devEui);
                });
    }

    /** Observation timeout escalation: severity → CRITICAL, timeline only. */
    @Scheduled(fixedDelayString = "${devicehub.disposal.scan-interval-ms:300000}",
            initialDelayString = "${devicehub.disposal.scan-initial-delay-ms:60000}")
    @Transactional
    public void escalateOverdueObservations() {
        Instant now = Instant.now();
        for (DisposalTicket ticket : ticketRepository.findByStatus(DisposalTicket.Status.OBSERVING)) {
            Instant deadline = observationDeadline(ticket);
            if (deadline == null || now.isBefore(deadline)) continue;
            if (ticket.getSeverity() != DisposalTicket.Severity.CRITICAL) {
                ticket.setSeverity(DisposalTicket.Severity.CRITICAL);
            }
            ticket.setTimeline(appendTimeline(ticket.getTimeline(), "OBSERVATION_OVERDUE",
                    "auto-scanner", Map.of("observationDeadlineAt", deadline.toString())));
            ticketRepository.save(ticket);
            log.warn("[Disposal] ticket {} observation overdue, escalated to CRITICAL",
                    ticket.getId());
        }
    }

    // ------------------------------------------------------------- queries

    @Transactional(readOnly = true)
    public List<DisposalTicket> list(DisposalTicket.Status status, DeviceProject project) {
        if (status != null && project != null) {
            return ticketRepository.findByStatusAndProject(status, project);
        }
        if (status != null) return ticketRepository.findByStatus(status);
        if (project != null) return ticketRepository.findByProject(project);
        return ticketRepository.findAll();
    }

    @Transactional(readOnly = true)
    public DisposalTicket detail(long id) {
        return require(id);
    }

    /** Timeline as a parsed JSON array for API responses. */
    public JsonNode timelineOf(DisposalTicket ticket) {
        try {
            return objectMapper.readTree(
                    ticket.getTimeline() == null ? "[]" : ticket.getTimeline());
        } catch (Exception e) {
            return objectMapper.createArrayNode();
        }
    }

    // ------------------------------------------------------------- helpers

    private DisposalTicket require(long id) {
        return ticketRepository.findById(id)
                .orElseThrow(() -> new IllegalArgumentException("disposal ticket not found: " + id));
    }

    private static void requireStatus(DisposalTicket ticket, DisposalTicket.Status expected) {
        if (ticket.getStatus() != expected) {
            throw new IllegalStateException("ticket " + ticket.getId() + " is "
                    + ticket.getStatus() + ", expected " + expected);
        }
    }

    private int intervalSecondsOf(String devEui) {
        return deviceRepository.findByDevEui(devEui).stream()
                .map(RegisteredDevice::getExpectedReportIntervalSeconds)
                .findFirst().orElse(3600);
    }

    private Instant observationDeadline(DisposalTicket ticket) {
        try {
            ArrayNode timeline = (ArrayNode) objectMapper.readTree(ticket.getTimeline());
            for (int i = timeline.size() - 1; i >= 0; i--) {
                JsonNode entry = timeline.get(i);
                if ("OBSERVING_STARTED".equals(entry.path("action").asText())) {
                    String deadline = entry.path("detail").path("observationDeadlineAt").asText(null);
                    return deadline == null ? null : Instant.parse(deadline);
                }
            }
        } catch (Exception e) {
            log.warn("[Disposal] timeline parse failed for ticket {}: {}",
                    ticket.getId(), e.getMessage());
        }
        return null;
    }

    String appendTimeline(String timeline, String action, String source, Map<String, ?> detail) {
        try {
            ArrayNode array = timeline == null || timeline.isBlank()
                    ? objectMapper.createArrayNode() : (ArrayNode) objectMapper.readTree(timeline);
            ObjectNode entry = array.addObject();
            entry.put("ts", Instant.now().toString());
            entry.put("action", action);
            entry.put("source", source);
            entry.set("detail", objectMapper.valueToTree(detail));
            return objectMapper.writeValueAsString(array);
        } catch (Exception e) {
            throw new IllegalStateException("timeline append failed", e);
        }
    }
}
