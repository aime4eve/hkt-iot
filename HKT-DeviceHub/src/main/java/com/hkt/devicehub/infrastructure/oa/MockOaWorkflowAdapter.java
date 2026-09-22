package com.hkt.devicehub.infrastructure.oa;

import com.hkt.devicehub.domain.model.DisposalTicket;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;

import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * Mock OA adapter (本期): generates DD-yyyyMMdd-NNNN instance ids and logs
 * notifications; the console simulates the DingTalk approve/reject buttons.
 * TODO phase 2: real DingTalk workflow/processInstances implementation.
 */
@Component
@Slf4j
public class MockOaWorkflowAdapter implements OaWorkflowPort {

    private final AtomicInteger sequence = new AtomicInteger();

    @Override
    public String startApproval(DisposalTicket ticket, String assignee) {
        String instanceId = "DD-" + LocalDate.now().format(DateTimeFormatter.BASIC_ISO_DATE)
                + "-" + String.format("%04d", sequence.incrementAndGet() % 10_000);
        log.info("[OA-mock] approval instance {} started for ticket {} ({}), assignee {}",
                instanceId, ticket.getId(), ticket.getDevEui(), assignee);
        return instanceId;
    }

    @Override
    public void notifyAssignee(DisposalTicket ticket, String assignee, String message) {
        log.info("[OA-mock] notify {} about ticket {} ({}): {}",
                assignee, ticket.getId(), ticket.getDevEui(), message);
    }
}
