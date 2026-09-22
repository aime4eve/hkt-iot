package com.hkt.devicehub.infrastructure.oa;

import com.hkt.devicehub.domain.model.DisposalTicket;

/**
 * OA workflow abstraction (console v1.5 §3.4 DingTalk integration points).
 * Real DingTalk open-platform implementation (processInstances /
 * bpms_instance_change callback / oToMessages) is phase 2; the disposal state
 * machine never sees the difference.
 */
public interface OaWorkflowPort {

    /** Start an approval instance for the ticket; returns the instance id. */
    String startApproval(DisposalTicket ticket, String assignee);

    /** Work notification to the assignee. */
    void notifyAssignee(DisposalTicket ticket, String assignee, String message);
}
