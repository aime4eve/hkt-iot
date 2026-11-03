package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.NsCounterSnapshot;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.NsCounterSnapshotRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Component;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * NS device/detail counter poller (communication quality MVP, 15 min cycle).
 * One snapshot per unique DevEUI per cycle — the same EUI can be registered
 * in two projects but NS counters are per EUI. NS unavailability is recorded
 * as an ns_ok=false snapshot row (unknown != healthy), never skipped
 * silently; the f_cnt_up differential across snapshots is what makes the
 * loss rate exact.
 */
@Component
@RequiredArgsConstructor
@Slf4j
public class NsCounterPoller {

    private final NsClient nsClient;
    private final RegisteredDeviceRepository deviceRepository;
    private final NsCounterSnapshotRepository snapshotRepository;

    @Scheduled(fixedDelayString = "${devicehub.quality.ns-poll-interval-ms:900000}",
            initialDelayString = "${devicehub.quality.ns-poll-initial-delay-ms:30000}")
    public void poll() {
        if (!nsClient.isEnabled()) return;
        List<RegisteredDevice> devices = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        Set<String> polled = new HashSet<>();
        int failures = 0;
        for (RegisteredDevice device : devices) {
            String eui = device.getDevEui();
            if (!polled.add(eui)) continue;
            try {
                NsClient.NsDeviceDetail detail = nsClient.fetchDeviceDetail(eui);
                snapshotRepository.save(NsCounterSnapshot.ok(eui, detail.fCntUp(), detail.nFCntDown(),
                        detail.aFCntDown(), detail.latestGateway(), detail.lastSeen()));
            } catch (Exception e) {
                failures++;
                snapshotRepository.save(NsCounterSnapshot.failed(eui, truncate(e.getMessage())));
                log.warn("[NsCounterPoller] detail {} failed: {}", eui, e.getMessage());
            }
        }
        if (!polled.isEmpty()) {
            log.info("[NsCounterPoller] polled {} devices, {} failures", polled.size(), failures);
        }
    }

    private static String truncate(String message) {
        if (message == null) return "unknown error";
        return message.length() > 250 ? message.substring(0, 250) : message;
    }
}
