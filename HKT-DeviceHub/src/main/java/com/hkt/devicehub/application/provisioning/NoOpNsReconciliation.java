package com.hkt.devicehub.application.provisioning;

import org.springframework.stereotype.Component;

/**
 * Default no-op NS reconciliation — DeviceHub works without any network
 * server dependency until a project supplies a real implementation.
 */
@Component
public class NoOpNsReconciliation implements NsReconciliationPort {
}
