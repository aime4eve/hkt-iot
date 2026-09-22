package com.hkt.devicehub.application.provisioning;

import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import lombok.extern.slf4j.Slf4j;
import org.springframework.boot.autoconfigure.condition.ConditionalOnProperty;
import org.springframework.stereotype.Component;

import java.util.ArrayList;
import java.util.List;

/**
 * Real NS reconciliation step (replaces the no-op default when
 * devicehub.ns.enabled=true): verifies the device exists in NS and reports
 * evidence differences instead of vetoing registration.
 */
@Component
@ConditionalOnProperty(name = "devicehub.ns.enabled", havingValue = "true")
@Slf4j
public class NsReconciliationAdapter implements NsReconciliationPort {

    private final NsClient nsClient;

    public NsReconciliationAdapter(NsClient nsClient) {
        this.nsClient = nsClient;
    }

    @Override
    public void afterRegister(RegisteredDevice device) {
        nsClient.findDeviceByEui(device.getDevEui()).ifPresentOrElse(
                ns -> log.info("[NS] {} present in NS project {} app {}",
                        device.getDevEui(), ns.projectId(), ns.appId()),
                () -> log.warn("[NS] {} not found in NS at registration time", device.getDevEui()));
    }

    @Override
    public List<String> reconcileDifferences(RegisteredDevice device) {
        List<String> diffs = new ArrayList<>();
        try {
            if (nsClient.findDeviceByEui(device.getDevEui()).isEmpty()) {
                diffs.add("NS_MISSING");
            }
        } catch (Exception e) {
            log.warn("[NS] reconcile check for {} failed: {}", device.getDevEui(), e.getMessage());
            diffs.add("NS_CHECK_FAILED");
        }
        return diffs;
    }
}
