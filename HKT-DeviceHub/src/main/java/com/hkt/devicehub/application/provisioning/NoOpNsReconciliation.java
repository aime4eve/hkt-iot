package com.hkt.devicehub.application.provisioning;

import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

/**
 * Default no-op NS reconciliation — active only when the real NS client is
 * disabled (devicehub.ns.enabled absent or false).
 */
@Component
@ConditionalOnProperty(name = "devicehub.ns.enabled", havingValue = "false", matchIfMissing = true)
public class NoOpNsReconciliation implements NsReconciliationPort {
}
