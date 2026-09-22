package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import lombok.extern.slf4j.Slf4j;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.web.client.HttpStatusCodeException;
import org.springframework.web.client.RestOperations;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriUtils;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;

/**
 * ThingsBoard REST client.
 * <p>
 * TB conventions (verified on 172.22.3.105, TB 3.8.0):
 * - login: POST /api/auth/login {username,password} → {token, refreshToken}
 * - every call carries "X-Authorization: Bearer <token>" (NOT "Authorization")
 * - 401 → re-login once and replay
 */
@Component
@Slf4j
public class TbClient {

    private static final String TB_KEYS = "result,dataHex,rssi,snr,downLinkGateway";

    private final TbProperties properties;
    private final ObjectMapper objectMapper;
    private final RestOperations rest;

    @Autowired
    public TbClient(TbProperties properties, ObjectMapper objectMapper) {
        this(properties, objectMapper, new RestTemplate());
    }

    TbClient(TbProperties properties, ObjectMapper objectMapper, RestOperations rest) {
        this.properties = properties;
        this.objectMapper = objectMapper;
        this.rest = rest;
    }

    private final Map<String, CachedToken> tokenCache = new ConcurrentHashMap<>();

    public JsonNode fetchTimeseries(String tbDeviceId, long startTs, long endTs, int limit) {
        String path = "/api/plugins/telemetry/DEVICE/" + tbDeviceId + "/values/timeseries"
                + "?keys=" + TB_KEYS
                + "&startTs=" + startTs + "&endTs=" + endTs
                + "&orderBy=ASC&limit=" + limit;
        return exchangeForJson(path, HttpMethod.GET, null);
    }

    /**
     * Resolve a DevEUI to its TB device id using the three-variant exact match
     * (as-is → upper → lower, parking NIX-80 D11). TB textSearch is fuzzy, so
     * results are filtered to exact name equality per variant. Multiple distinct
     * matches mean ambiguous data and must not be silently auto-bound.
     */
    public String resolveDeviceId(String eui) {
        LinkedHashSet<String> matches = new LinkedHashSet<>();
        for (String variant : variantsOf(eui)) {
            String path = "/api/tenant/devices?pageSize=100&page=0&textSearch="
                    + UriUtils.encodeQueryParam(variant, StandardCharsets.UTF_8);
            JsonNode page = exchangeForJson(path, HttpMethod.GET, null);
            for (JsonNode device : page.path("data")) {
                if (variant.equals(device.path("name").asText())) {
                    matches.add(device.path("id").path("id").asText());
                }
            }
        }
        if (matches.isEmpty()) {
            throw new IllegalStateException("TB device not found for EUI " + eui);
        }
        if (matches.size() > 1) {
            throw new IllegalStateException("Ambiguous TB devices for EUI " + eui + ": " + matches);
        }
        return matches.iterator().next();
    }

    public List<TbDeviceView> findDevices(String eui) {
        Map<String, TbDeviceView> matches = new LinkedHashMap<>();
        for (String variant : variantsOf(eui)) {
            String path = "/api/tenant/devices?pageSize=100&page=0&textSearch="
                    + UriUtils.encodeQueryParam(variant, StandardCharsets.UTF_8);
            JsonNode page = exchangeForJson(path, HttpMethod.GET, null);
            for (JsonNode device : page.path("data")) {
                if (variant.equals(device.path("name").asText())) {
                    String id = device.path("id").path("id").asText();
                    matches.putIfAbsent(id, toView(device));
                }
            }
        }
        return List.copyOf(matches.values());
    }

    /** Paged listing of all tenant devices (for reconcile / conflicts). */
    public List<TbDeviceView> listTenantDevices() {
        List<TbDeviceView> devices = new ArrayList<>();
        int page = 0;
        while (true) {
            JsonNode result = exchangeForJson(
                    "/api/tenant/devices?pageSize=100&page=" + page, HttpMethod.GET, null);
            JsonNode data = result.path("data");
            for (JsonNode device : data) {
                devices.add(toView(device));
            }
            if (data.size() < 100) break;
            page++;
        }
        return devices;
    }

    /** All TB device profiles as profileId → name (for profile matching). */
    public Map<String, String> fetchDeviceProfiles() {
        Map<String, String> profiles = new LinkedHashMap<>();
        int page = 0;
        while (true) {
            JsonNode result = exchangeForJson("/api/deviceProfiles?pageSize=100&page=" + page,
                    HttpMethod.GET, null);
            JsonNode data = result.path("data");
            for (JsonNode profile : data) {
                profiles.put(profile.path("id").path("id").asText(), profile.path("name").asText());
            }
            if (data.size() < 100) break;
            page++;
        }
        return profiles;
    }

    /** Shared attributes of a device as key → value (gateway OC mapping lives here). */
    public Map<String, JsonNode> fetchSharedAttributes(String tbDeviceId) {
        JsonNode attrs = exchangeForJson("/api/plugins/telemetry/DEVICE/" + tbDeviceId
                + "/values/attributes/SHARED_SCOPE", HttpMethod.GET, null);
        Map<String, JsonNode> out = new LinkedHashMap<>();
        if (attrs.isArray()) {
            for (JsonNode attr : attrs) {
                out.put(attr.path("key").asText(), attr.path("value"));
            }
        }
        return out;
    }

    /**
     * Recent frame summaries (DESC, null-artifact filtered) for the device
     * detail drawer. Values are truncated to keep the payload small.
     */
    public List<FrameSummary> fetchRecentFrames(String tbDeviceId, int limit) {
        String path = "/api/plugins/telemetry/DEVICE/" + tbDeviceId + "/values/timeseries"
                + "?keys=result,dataHex&startTs=0&endTs=" + System.currentTimeMillis()
                + "&orderBy=DESC&limit=" + limit;
        JsonNode timeseries = exchangeForJson(path, HttpMethod.GET, null);
        List<FrameSummary> frames = new ArrayList<>();
        for (String key : List.of("result", "dataHex")) {
            for (JsonNode point : timeseries.path(key)) {
                JsonNode value = point.path("value");
                if (value.isNull() || value.isMissingNode()) continue;
                String text = value.isTextual() ? value.asText() : value.toString();
                frames.add(new FrameSummary(point.path("ts").asLong(), key,
                        text.length() > 200 ? text.substring(0, 200) + "…" : text));
            }
        }
        frames.sort((a, b) -> Long.compare(b.ts(), a.ts()));
        return frames.size() > limit ? frames.subList(0, limit) : frames;
    }

    /**
     * Zero-telemetry probe (governance delete precondition): any non-null
     * point on result/data/dataHex counts as telemetry.
     */
    public boolean hasAnyTelemetry(String tbDeviceId) {
        String path = "/api/plugins/telemetry/DEVICE/" + tbDeviceId + "/values/timeseries"
                + "?keys=result,data,dataHex&startTs=0&endTs=" + System.currentTimeMillis()
                + "&orderBy=DESC&limit=5";
        JsonNode timeseries = exchangeForJson(path, HttpMethod.GET, null);
        for (String key : List.of("result", "data", "dataHex")) {
            for (JsonNode point : timeseries.path(key)) {
                if (!point.path("value").isNull()) return true;
            }
        }
        return false;
    }

    /** Approximate telemetry volume: non-null points across keys, capped at limit per key. */
    public int approximateTelemetryCount(String tbDeviceId, int limitPerKey) {
        String path = "/api/plugins/telemetry/DEVICE/" + tbDeviceId + "/values/timeseries"
                + "?keys=result,dataHex&startTs=0&endTs=" + System.currentTimeMillis()
                + "&orderBy=DESC&limit=" + limitPerKey;
        JsonNode timeseries = exchangeForJson(path, HttpMethod.GET, null);
        int count = 0;
        for (String key : List.of("result", "dataHex")) {
            for (JsonNode point : timeseries.path(key)) {
                if (!point.path("value").isNull()) count++;
            }
        }
        return count;
    }

    /** Governance: delete a TB device (only after the zero-telemetry check). */
    public void deleteDevice(String tbDeviceId) {
        exchangeForJson("/api/device/" + tbDeviceId, HttpMethod.DELETE, null);
        log.info("[TB] deleted device {}", tbDeviceId);
    }

    private static TbDeviceView toView(JsonNode device) {
        return new TbDeviceView(
                device.path("id").path("id").asText(),
                device.path("name").asText(),
                device.path("createdTime").asLong(0),
                device.path("deviceProfileId").path("id").asText(null));
    }

    /**
     * Create a TB device named by DevEUI (IoT gateway uplinks land on a device
     * whose name is the EUI). Returns the created device id.
     */
    public String createDevice(String devEui) {
        ObjectNode body = objectMapper.createObjectNode();
        body.put("name", devEui);
        body.putObject("additionalInfo").put("gateway", false);
        JsonNode created = exchangeForJson("/api/device", HttpMethod.POST, body);
        String id = created.path("id").path("id").asText();
        if (id.isBlank()) {
            throw new IllegalStateException("TB create device returned no id for " + devEui);
        }
        log.info("[TB] created device {} -> {}", devEui, id);
        return id;
    }

    /**
     * Latest frame ts of a device (cheap limit=1 DESC probe). TB artifact:
     * explicit-keys queries return {ts:now, value:null} entries for keys with
     * no data — null values must be ignored.
     */
    public Instant fetchLatestTelemetryTs(String tbDeviceId) {
        String path = "/api/plugins/telemetry/DEVICE/" + tbDeviceId + "/values/timeseries"
                + "?keys=result,dataHex&startTs=0&endTs=" + System.currentTimeMillis()
                + "&orderBy=DESC&limit=1";
        JsonNode timeseries = exchangeForJson(path, HttpMethod.GET, null);
        long maxTs = Long.MIN_VALUE;
        for (String key : List.of("result", "dataHex")) {
            JsonNode values = timeseries.path(key);
            if (!values.isArray()) continue;
            for (JsonNode point : values) {
                if (point.path("value").isNull()) continue;
                maxTs = Math.max(maxTs, point.path("ts").asLong());
            }
        }
        return maxTs == Long.MIN_VALUE ? null : Instant.ofEpochMilli(maxTs);
    }

    /**
     * Downlink placeholder for phase 2 smoke verification only:
     * fire-and-forget server-side RPC. Not exposed via REST this phase.
     */
    public void sendRpcOneWay(String deviceId, Object payload) {
        JsonNode body = objectMapper.valueToTree(payload);
        exchangeForJson("/api/rpc/oneway/" + deviceId, HttpMethod.POST, body);
    }

    /**
     * Downlink placeholder for phase 2 smoke verification only:
     * blocking server-side RPC awaiting the device reply within timeoutMs.
     */
    public JsonNode sendRpcTwoWay(String deviceId, Object payload, long timeoutMs) {
        JsonNode body = objectMapper.valueToTree(payload);
        return exchangeForJson("/api/rpc/twoway/" + deviceId + "?timeout=" + timeoutMs,
                HttpMethod.POST, body);
    }

    /** Access token for the WS channel authCmd. */
    public String getAccessToken() {
        return getToken(false);
    }

    public static boolean isTbUuid(String externalId) {
        if (externalId == null) return false;
        try {
            UUID.fromString(externalId);
            return true;
        } catch (IllegalArgumentException e) {
            return false;
        }
    }

    static List<String> variantsOf(String eui) {
        List<String> variants = new ArrayList<>();
        if (eui != null && !eui.isBlank()) {
            variants.add(eui);
            String upper = eui.toUpperCase(Locale.ROOT);
            String lower = eui.toLowerCase(Locale.ROOT);
            if (!variants.contains(upper)) variants.add(upper);
            if (!variants.contains(lower)) variants.add(lower);
        }
        return variants;
    }

    private JsonNode exchangeForJson(String path, HttpMethod method, JsonNode body) {
        try {
            return doExchange(path, method, body, false);
        } catch (TbUnauthorizedException e) {
            log.info("[TB] 401 received, re-login and replay once: {}", path);
            return doExchange(path, method, body, true);
        }
    }

    private JsonNode doExchange(String path, HttpMethod method, JsonNode body, boolean forceRelogin) {
        String token = getToken(forceRelogin);
        try {
            HttpHeaders headers = new HttpHeaders();
            headers.set("X-Authorization", "Bearer " + token);
            if (body != null) headers.setContentType(MediaType.APPLICATION_JSON);
            ResponseEntity<String> resp = rest.exchange(
                    properties.getBaseUrl() + path, method, new HttpEntity<>(body, headers), String.class);
            return objectMapper.readTree(resp.getBody() == null ? "{}" : resp.getBody());
        } catch (HttpStatusCodeException e) {
            if (e.getStatusCode().value() == 401) {
                throw new TbUnauthorizedException(e);
            }
            throw new IllegalStateException("TB call failed: " + path + " -> " + e.getStatusCode(), e);
        } catch (Exception e) {
            if (e instanceof TbUnauthorizedException) throw (TbUnauthorizedException) e;
            throw new IllegalStateException("TB call failed: " + path, e);
        }
    }

    private synchronized String getToken(boolean forceRelogin) {
        String key = properties.getUsername();
        CachedToken cached = tokenCache.get(key);
        if (!forceRelogin && cached != null && Instant.now().isBefore(cached.expiresAt)) {
            return cached.token;
        }
        try {
            HttpHeaders headers = new HttpHeaders();
            headers.setContentType(MediaType.APPLICATION_JSON);
            Map<String, String> payload = Map.of("username", key, "password", properties.getPassword());
            ResponseEntity<String> resp = rest.postForEntity(
                    properties.getBaseUrl() + "/api/auth/login",
                    new HttpEntity<>(objectMapper.writeValueAsString(payload), headers),
                    String.class);
            JsonNode node = objectMapper.readTree(resp.getBody());
            // TB returns token + refreshToken with ~1h TTL; refresh at 50 min.
            String token = node.path("token").asText();
            if (token.isBlank()) {
                throw new IllegalStateException("TB login returned no token");
            }
            tokenCache.put(key, new CachedToken(token, Instant.now().plusSeconds(3000)));
            log.info("[TB] login ok for {}", key);
            return token;
        } catch (TbUnauthorizedException e) {
            throw e;
        } catch (Exception e) {
            throw new IllegalStateException("TB login failed for " + key, e);
        }
    }

    private record CachedToken(String token, Instant expiresAt) {}

    public record TbDeviceView(String id, String name, long createdTime, String profileId) {}

    public record FrameSummary(long ts, String key, String preview) {}

    private static class TbUnauthorizedException extends RuntimeException {
        TbUnauthorizedException(Throwable cause) {
            super("TB unauthorized", cause);
        }
    }
}
