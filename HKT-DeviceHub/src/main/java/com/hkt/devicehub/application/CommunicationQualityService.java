package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.NsCounterSnapshot;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.NsCounterSnapshotRepository;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.domain.repository.TelemetryFrameRecordRepository;
import com.hkt.devicehub.infrastructure.config.QualityProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.TreeSet;

/**
 * Communication quality summary (MVP "solve existence first"): loss rate from
 * the NS f_cnt_up differential against retained-frame counts, signal
 * distribution from retained rssi/snr, silence-gap detection, and offline
 * judgement. Metrics are computed at query time over a bounded window — the
 * fleet is small (periodic-reporting devices), so no pre-aggregation yet.
 * <p>
 * Methodology notes (research note 2026-11-03):
 * <ul>
 *   <li>Loss windows need two consecutive ok snapshots; a non-positive or
 *   wildly negative f_cnt_up delta is a device reset (ABP reboots zero the
 *   counter — normal, not loss) and is reported as resetEvents.</li>
 *   <li>NS-side air loss is NOT computable here (no per-frame NS history
 *   endpoint); the rate below is end-to-end (device → gateway → NS → TB →
 *   DeviceHub), which mixes in platform-side drops. Documented, not hidden.</li>
 *   <li>Online = last_frame_at within 3× the device's expected report
 *   interval (offlineFactor fixed at 3).</li>
 * </ul>
 */
@Service
@RequiredArgsConstructor
@Slf4j
public class CommunicationQualityService {

    static final int OFFLINE_FACTOR = 3;

    private final RegisteredDeviceRepository deviceRepository;
    private final NsCounterSnapshotRepository snapshotRepository;
    private final TelemetryFrameRecordRepository frameRepository;
    private final QualityProperties properties;
    private final FrameRetentionRecorder retentionRecorder;
    private final NsClient nsClient;

    private volatile Summary cached;
    private volatile Instant cachedAt;
    private final Object cacheLock = new Object();

    public Summary summary() {
        Summary snapshot = cached;
        if (snapshot != null && cachedAt != null
                && Instant.now().isBefore(cachedAt.plusMillis(properties.getSummaryCacheMs()))) {
            return snapshot;
        }
        synchronized (cacheLock) {
            if (cached != null && cachedAt != null
                    && Instant.now().isBefore(cachedAt.plusMillis(properties.getSummaryCacheMs()))) {
                return cached;
            }
            Summary fresh = compute();
            cached = fresh;
            cachedAt = Instant.now();
            return fresh;
        }
    }

    private Summary compute() {
        Instant now = Instant.now();
        Instant since = now.minus(Duration.ofHours(properties.getSummaryWindowHours()));
        List<RegisteredDevice> devices = deviceRepository.findByStatus(RegistrationStatus.ACTIVE);
        Map<String, List<SnapPoint>> snapsByEui = new LinkedHashMap<>();
        boolean nsDegraded = false;
        for (NsCounterSnapshot snap : snapshotRepository.findByPolledAtGreaterThanEqualOrderByPolledAtAsc(since)) {
            nsDegraded |= !snap.isNsOk();
            snapsByEui.computeIfAbsent(snap.getDevEui(), k -> new ArrayList<>())
                    .add(new SnapPoint(snap.getPolledAt(), snap.isNsOk(), snap.getFCntUp(),
                            snap.getNFCntDown(), snap.getAFCntDown(), snap.getLatestGateway(),
                            snap.getLastSeen()));
        }
        Map<String, List<FramePoint>> framesByEui = new LinkedHashMap<>();
        for (var record : frameRepository.findByTsGreaterThanEqualOrderByTsAsc(since)) {
            framesByEui.computeIfAbsent(record.getDevEui(), k -> new ArrayList<>())
                    .add(new FramePoint(record.getTs(), record.getRssi(), record.getSnr()));
        }

        List<DeviceQuality> rows = new ArrayList<>();
        for (RegisteredDevice device : devices) {
            rows.add(derive(device.getDevEui(), device.getProject().name(), device.getDeviceType(),
                    device.getExpectedReportIntervalSeconds(), device.getLastFrameAt(),
                    snapsByEui.getOrDefault(device.getDevEui(), List.of()),
                    framesByEui.getOrDefault(device.getDevEui(), List.of()),
                    now, properties.getGapThresholdMs()));
        }
        rows.sort(Comparator.comparing(DeviceQuality::project).thenComparing(DeviceQuality::devEui));

        long online = rows.stream().filter(DeviceQuality::online).count();
        List<Double> lossRates = rows.stream().map(DeviceQuality::lossRatePercent)
                .filter(Objects::nonNull).toList();
        DeviceQuality worst = rows.stream()
                .filter(r -> r.lossRatePercent() != null)
                .max(Comparator.comparing(DeviceQuality::lossRatePercent))
                .orElse(null);
        Totals totals = new Totals(rows.size(), (int) online, lossRates.size(),
                lossRates.isEmpty() ? null : round1(lossRates.stream().mapToDouble(Double::doubleValue).average().orElse(0)),
                worst == null ? null : worst.lossRatePercent(),
                worst == null ? null : worst.devEui());
        return new Summary(now, properties.getSummaryWindowHours(), nsClient.isEnabled(), nsDegraded,
                retentionRecorder.status(), totals, rows);
    }

    /**
     * Pure metric derivation — unit-testable without Spring or repositories.
     */
    static DeviceQuality derive(String devEui, String project, String deviceType,
                                int expectedIntervalSeconds, Instant lastFrameAt,
                                List<SnapPoint> snapshots, List<FramePoint> frames,
                                Instant now, long gapThresholdMs) {
        List<SnapPoint> snaps = snapshots.stream()
                .sorted(Comparator.comparing(SnapPoint::polledAt)).toList();

        long totalExpected = 0;
        long received = 0;
        long totalLost = 0;
        int lossWindows = 0;
        int resetEvents = 0;
        Long nFCntDownGrowth = null;
        Long aFCntDownGrowth = null;
        SnapPoint latestOk = null;

        for (int i = 1; i < snaps.size(); i++) {
            SnapPoint s1 = snaps.get(i - 1);
            SnapPoint s2 = snaps.get(i);
            if (!s1.nsOk() || !s2.nsOk() || s1.fCntUp() == null || s2.fCntUp() == null) continue;
            if (latestOk == null || s2.polledAt().isAfter(latestOk.polledAt())) latestOk = s2;
            long expected = s2.fCntUp() - s1.fCntUp();
            if (expected <= 0) {
                // ABP reboot zeroes the counter — a reset event, never loss.
                resetEvents++;
                continue;
            }
            lossWindows++;
            long got = frames.stream()
                    .filter(f -> f.ts().isAfter(s1.polledAt()) && !f.ts().isAfter(s2.polledAt()))
                    .count();
            received += got;
            totalExpected += expected;
            totalLost += Math.max(0, expected - got);
        }
        // downlink growth: last minus first nsOk value (evidence only)
        Long nFirst = null;
        Long nLast = null;
        Long aFirst = null;
        Long aLast = null;
        for (SnapPoint snap : snaps) {
            if (!snap.nsOk()) continue;
            if (snap.nFCntDown() != null) {
                if (nFirst == null) nFirst = snap.nFCntDown();
                nLast = snap.nFCntDown();
            }
            if (snap.aFCntDown() != null) {
                if (aFirst == null) aFirst = snap.aFCntDown();
                aLast = snap.aFCntDown();
            }
        }
        nFCntDownGrowth = (nFirst != null && nLast != null) ? nLast - nFirst : null;
        aFCntDownGrowth = (aFirst != null && aLast != null) ? aLast - aFirst : null;

        List<Integer> rssiValues = frames.stream().map(FramePoint::rssi).filter(Objects::nonNull).toList();
        Double rssiMedian = median(rssiValues);
        List<Integer> snrValues = frames.stream().map(FramePoint::snr).filter(Objects::nonNull).toList();
        Double snrNegativePercent = snrValues.isEmpty() ? null
                : round1(snrValues.stream().filter(v -> v < 0).count() * 100.0 / snrValues.size());

        int gapCount = 0;
        Integer longestGapMinutes = null;
        TreeSet<Instant> frameTs = new TreeSet<>();
        for (FramePoint frame : frames) frameTs.add(frame.ts());
        Instant previous = null;
        for (Instant ts : frameTs) {
            if (previous != null) {
                long gapMs = Duration.between(previous, ts).toMillis();
                if (gapMs > gapThresholdMs) {
                    gapCount++;
                    int minutes = (int) Math.round(gapMs / 60000.0);
                    if (longestGapMinutes == null || minutes > longestGapMinutes) longestGapMinutes = minutes;
                }
            }
            previous = ts;
        }

        boolean online = lastFrameAt != null
                && Duration.between(lastFrameAt, now).getSeconds() <= OFFLINE_FACTOR * (long) expectedIntervalSeconds;

        return new DeviceQuality(devEui, project, deviceType, online, lastFrameAt,
                latestOk == null ? null : latestOk.lastSeen(),
                latestOk == null ? null : latestOk.fCntUp(),
                totalExpected, received, totalLost,
                totalExpected > 0 ? round1(totalLost * 100.0 / totalExpected) : null,
                resetEvents, lossWindows,
                rssiMedian, snrNegativePercent, rssiValues.size(),
                gapCount, longestGapMinutes, nFCntDownGrowth, aFCntDownGrowth);
    }

    private static Double median(List<Integer> values) {
        if (values.isEmpty()) return null;
        List<Integer> sorted = new ArrayList<>(values);
        Collections.sort(sorted);
        int mid = sorted.size() / 2;
        return sorted.size() % 2 == 1
                ? (double) sorted.get(mid)
                : (sorted.get(mid - 1) + sorted.get(mid)) / 2.0;
    }

    private static double round1(double value) {
        return Math.round(value * 10.0) / 10.0;
    }

    public record SnapPoint(Instant polledAt, boolean nsOk, Long fCntUp, Long nFCntDown,
                            Long aFCntDown, String latestGateway, Instant lastSeen) {}

    public record FramePoint(Instant ts, Integer rssi, Integer snr) {}

    public record DeviceQuality(String devEui, String project, String deviceType, boolean online,
                                Instant lastFrameAt, Instant lastSeen, Long fCntUp,
                                long expectedFrames, long receivedFrames, long lostFrames,
                                Double lossRatePercent, int resetEvents, int lossWindows,
                                Double rssiMedian, Double snrNegativePercent, int signalFrames,
                                int gapCount, Integer longestGapMinutes,
                                Long nFCntDownGrowth, Long aFCntDownGrowth) {}

    public record Totals(int totalDevices, int onlineDevices, int devicesWithLossData,
                         Double meanLossPercent, Double worstLossPercent, String worstDevEui) {}

    public record Summary(Instant generatedAt, int windowHours, boolean nsEnabled, boolean nsPollDegraded,
                          FrameRetentionRecorder.Status retention, Totals totals, List<DeviceQuality> devices) {}
}
