package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.application.TelemetryFrame;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Component;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.TreeMap;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Normalizes ThingsBoard timeseries (REST pages and WS points) into
 * {@link TelemetryFrame} events.
 * <p>
 * Same-frame rule (parking NIX-80): result and dataHex are two keys of one
 * physical frame. result frames with decodeStatus=true are authoritative; a
 * dataHex point within 2s of a result frame is the same frame and is skipped.
 * A dataHex point further away is a result-less frame and is published raw.
 * <p>
 * Fault tolerance (livestock NIX-179): frames that cannot be decoded (e.g. a
 * modified TB rule chain saving decodeStatus:false results) are reported as
 * skipped instead of failing the whole page — the channel advances its cursor
 * past them so bad frames can never stall a device permanently.
 */
@Component
@Slf4j
public class TbFrameNormalizer {

    static final long SAME_FRAME_WINDOW_MS = 2000;
    private static final long WS_CACHE_RETENTION_MS = 30_000;

    private final ObjectMapper objectMapper;

    /** WS only: recent authoritative result timestamps per device, for 2s dedup. */
    private final Map<String, TreeMap<Long, Boolean>> recentResultTs = new ConcurrentHashMap<>();
    /** WS only: transport points (rssi/snr/downLinkGateway) awaiting their frame. */
    private final Map<String, TreeMap<Long, Map<String, Object>>> recentTransport = new ConcurrentHashMap<>();

    public TbFrameNormalizer(ObjectMapper objectMapper) {
        this.objectMapper = objectMapper;
    }

    /** Device identity carried into every frame. */
    public record DeviceRef(String tbDeviceId, String devEui, String project) {}

    /** Parsed page: frames to publish plus the timestamps of dropped frames. */
    public record ParseResult(List<TelemetryFrame> frames, List<Long> skippedTs) {}

    // ---------------------------------------------------------------- REST page

    public ParseResult normalizePage(DeviceRef device, JsonNode timeseries) {
        TreeMap<Long, Map<String, Object>> resultFrames = new TreeMap<>();
        TreeMap<Long, String> hexFrames = new TreeMap<>();
        TreeMap<Long, String> invalidFrames = new TreeMap<>();
        Map<Long, Map<String, Object>> transport = new TreeMap<>();

        collectPoints(timeseries, "result", (ts, value) -> {
            Map<String, Object> properties = parseResultProperties(value);
            if (properties == null) {
                invalidFrames.put(ts, value.isTextual() ? value.asText() : value.toString());
            } else {
                resultFrames.put(ts, properties);
            }
        });
        collectPoints(timeseries, "dataHex", (ts, value) -> hexFrames.put(ts, value.asText()));
        collectPoints(timeseries, "rssi", (ts, value) ->
                transport.computeIfAbsent(ts, k -> new LinkedHashMap<>()).put("rssi", value.asInt()));
        collectPoints(timeseries, "snr", (ts, value) ->
                transport.computeIfAbsent(ts, k -> new LinkedHashMap<>()).put("snr", value.asInt()));
        collectPoints(timeseries, "downLinkGateway", (ts, value) ->
                transport.computeIfAbsent(ts, k -> new LinkedHashMap<>())
                        .put("gatewayId", value.asText()));

        List<TelemetryFrame> frames = new ArrayList<>();
        for (Map.Entry<Long, Map<String, Object>> entry : resultFrames.entrySet()) {
            frames.add(buildFrame(device, entry.getKey(), entry.getValue(), null,
                    transport.get(entry.getKey())));
        }
        for (Map.Entry<Long, String> entry : hexFrames.entrySet()) {
            if (hasFrameNearby(resultFrames, entry.getKey())) {
                continue;
            }
            removeInvalidResultNearby(invalidFrames, entry.getKey());
            frames.add(buildFrame(device, entry.getKey(), Map.of(), entry.getValue(),
                    transport.get(entry.getKey())));
        }
        if (!invalidFrames.isEmpty()) {
            log.warn("[TB] {} undecodable frame(s) skipped at timestamps {}",
                    invalidFrames.size(), invalidFrames.keySet());
        }
        frames.sort(Comparator.comparingLong(TelemetryFrame::ts));
        return new ParseResult(List.copyOf(frames), List.copyOf(invalidFrames.keySet()));
    }

    // ---------------------------------------------------------------- WS points

    /**
     * Normalize one WS latest-value point. Returns empty for transport-only
     * points (rssi/snr/downLinkGateway are cached and attached to the frame
     * sharing their ts) and for dataHex points suppressed by the 2s same-frame
     * window.
     */
    public Optional<TelemetryFrame> normalizeWsPoint(DeviceRef device, String key, long ts,
                                                     JsonNode value) {
        switch (key) {
            case "rssi", "snr", "downLinkGateway" -> {
                cacheTransport(device.tbDeviceId(), ts, key, value);
                return Optional.empty();
            }
            case "result" -> {
                Map<String, Object> properties = parseResultProperties(value);
                if (properties == null) {
                    log.warn("[TB] ws result frame of {} at {} not decodable, skipped",
                            device.devEui(), ts);
                    return Optional.empty();
                }
                rememberResultTs(device.tbDeviceId(), ts);
                return Optional.of(buildFrame(device, ts, properties, null,
                        takeTransport(device.tbDeviceId(), ts)));
            }
            case "dataHex" -> {
                String hex = value.asText();
                if (hex == null || hex.isBlank()) return Optional.empty();
                if (hasRecentResult(device.tbDeviceId(), ts)) {
                    return Optional.empty();
                }
                return Optional.of(buildFrame(device, ts, Map.of(), hex,
                        takeTransport(device.tbDeviceId(), ts)));
            }
            default -> {
                return Optional.empty();
            }
        }
    }

    // ---------------------------------------------------------------- shared

    /** frameId = UUID name-based (SHA-1) of "tbDeviceId:ts" — stable across channels and replays. */
    public static String frameId(String tbDeviceId, long ts) {
        return UUID.nameUUIDFromBytes(
                (tbDeviceId + ":" + ts).getBytes(StandardCharsets.UTF_8)).toString();
    }

    /**
     * Authoritative decode path: result values are JSON (text or object) with
     * decodeStatus=true; the whole decodeData.properties map is passed through
     * verbatim — DeviceHub does not interpret business fields.
     */
    Map<String, Object> parseResultProperties(JsonNode resultValue) {
        JsonNode decoded;
        if (resultValue.isTextual()) {
            try {
                decoded = objectMapper.readTree(resultValue.asText());
            } catch (Exception e) {
                log.warn("[TB] result is not valid JSON: {}", e.getMessage());
                return null;
            }
        } else {
            decoded = resultValue;
        }
        if (!decoded.path("decodeStatus").asBoolean(false)) {
            return null;
        }
        JsonNode props = decoded.path("decodeData").path("properties");
        if (!props.isObject()) {
            return new LinkedHashMap<>();
        }
        return objectMapper.convertValue(props, LinkedHashMap.class);
    }

    private TelemetryFrame buildFrame(DeviceRef device, long ts, Map<String, Object> properties,
                                      String dataHex, Map<String, Object> transport) {
        Integer rssi = transport == null ? null : toInt(transport.get("rssi"));
        Integer snr = transport == null ? null : toInt(transport.get("snr"));
        String gatewayId = transport == null ? null
                : transport.get("gatewayId") instanceof String g ? g : null;
        return new TelemetryFrame(
                TelemetryFrame.CONTRACT_VERSION,
                frameId(device.tbDeviceId(), ts),
                device.devEui(),
                device.tbDeviceId(),
                ts,
                properties,
                dataHex,
                rssi,
                snr,
                gatewayId);
    }

    private static Integer toInt(Object value) {
        if (value instanceof Number number) return number.intValue();
        if (value instanceof String text && text.chars().allMatch(Character::isDigit)) {
            return Integer.parseInt(text);
        }
        return null;
    }

    private void cacheTransport(String tbDeviceId, long ts, String key, JsonNode value) {
        TreeMap<Long, Map<String, Object>> byTs =
                recentTransport.computeIfAbsent(tbDeviceId, k -> new TreeMap<>());
        Object parsed = switch (key) {
            case "rssi", "snr" -> value.isNumber() ? value.asInt() : value.asText();
            default -> value.asText();
        };
        synchronized (byTs) {
            byTs.computeIfAbsent(ts, k -> new LinkedHashMap<>()).put(
                    "downLinkGateway".equals(key) ? "gatewayId" : key, parsed);
            prune(byTs);
        }
    }

    private Map<String, Object> takeTransport(String tbDeviceId, long ts) {
        TreeMap<Long, Map<String, Object>> byTs = recentTransport.get(tbDeviceId);
        if (byTs == null) return null;
        synchronized (byTs) {
            Map<String, Object> exact = byTs.get(ts);
            if (exact != null) return exact;
            Long floor = byTs.floorKey(ts);
            if (floor != null && ts - floor <= SAME_FRAME_WINDOW_MS) return byTs.get(floor);
            Long ceiling = byTs.ceilingKey(ts);
            if (ceiling != null && ceiling - ts <= SAME_FRAME_WINDOW_MS) return byTs.get(ceiling);
            return null;
        }
    }

    private void rememberResultTs(String tbDeviceId, long ts) {
        TreeMap<Long, Boolean> set = recentResultTs.computeIfAbsent(tbDeviceId, k -> new TreeMap<>());
        synchronized (set) {
            set.put(ts, Boolean.TRUE);
            prune(set);
        }
    }

    private boolean hasRecentResult(String tbDeviceId, long ts) {
        TreeMap<Long, Boolean> set = recentResultTs.get(tbDeviceId);
        if (set == null) return false;
        synchronized (set) {
            Long floor = set.floorKey(ts);
            if (floor != null && ts - floor <= SAME_FRAME_WINDOW_MS) return true;
            Long ceiling = set.ceilingKey(ts);
            return ceiling != null && ceiling - ts <= SAME_FRAME_WINDOW_MS;
        }
    }

    private static void prune(TreeMap<Long, ?> map) {
        long cutoff = System.currentTimeMillis() - WS_CACHE_RETENTION_MS;
        map.headMap(cutoff).clear();
    }

    private static void removeInvalidResultNearby(TreeMap<Long, String> invalidFrames, long ts) {
        Long floor = invalidFrames.floorKey(ts);
        if (floor != null && ts - floor <= SAME_FRAME_WINDOW_MS) {
            invalidFrames.remove(floor);
            return;
        }
        Long ceiling = invalidFrames.ceilingKey(ts);
        if (ceiling != null && ceiling - ts <= SAME_FRAME_WINDOW_MS) {
            invalidFrames.remove(ceiling);
        }
    }

    private static boolean hasFrameNearby(TreeMap<Long, ?> frames, long ts) {
        Long floor = frames.floorKey(ts);
        if (floor != null && ts - floor <= SAME_FRAME_WINDOW_MS) return true;
        Long ceiling = frames.ceilingKey(ts);
        return ceiling != null && ceiling - ts <= SAME_FRAME_WINDOW_MS;
    }

    private interface PointConsumer {
        void accept(long ts, JsonNode value);
    }

    private static void collectPoints(JsonNode timeseries, String key, PointConsumer consumer) {
        JsonNode points = timeseries.path(key);
        if (!points.isArray()) return;
        for (JsonNode point : points) {
            consumer.accept(point.path("ts").asLong(), point.path("value"));
        }
    }
}
