package com.hkt.devicehub.infrastructure.ns;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.HttpMethod;
import org.springframework.http.MediaType;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Component;
import org.springframework.web.client.RestOperations;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.util.UriComponentsBuilder;

import java.net.URI;
import java.time.Instant;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.time.format.DateTimeFormatter;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Read-only NS inventory client (livestock NsClient blueprint). Login via
 * POST /backend/api/login/ → token carried as "x-token" header; device list
 * via /backend/org_api/lora_wan/device/list/?org=&page=&limit=&dev_eui=.
 * <p>
 * Caution (L6): NS list API fields like f_cnt_up can disagree with the NS UI
 * counters — expose them as evidence, never as sole verdicts.
 */
@Component
public class NsClient {

    private static final String LOGIN_API_PREFIX = "/backend/api/";
    private static final String ORG_API_PREFIX = "/backend/org_api/";
    /** NS list API renders last_seen in the NS host's local time (UTC+8). */
    private static final ZoneId NS_ZONE = ZoneId.of("Asia/Shanghai");
    private static final DateTimeFormatter SEEN_FORMAT = DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss");

    private final NsProperties properties;
    private final ObjectMapper objectMapper;
    private final RestOperations rest;
    private final Map<String, CachedToken> tokenCache = new ConcurrentHashMap<>();

    @Autowired
    public NsClient(NsProperties properties, ObjectMapper objectMapper) {
        this(properties, objectMapper, new RestTemplate());
    }

    NsClient(NsProperties properties, ObjectMapper objectMapper, RestOperations rest) {
        this.properties = properties;
        this.objectMapper = objectMapper;
        this.rest = rest;
    }

    public boolean isEnabled() {
        return properties.isEnabled();
    }

    public Optional<NsDevice> findDeviceByEui(String eui) {
        return listDevices(null, eui).stream()
                .filter(item -> item.eui().equalsIgnoreCase(eui))
                .findFirst();
    }

    public List<NsDevice> listDevices(Integer projectId) {
        return listDevices(projectId, null);
    }

    private List<NsDevice> listDevices(Integer projectId, String eui) {
        requireEnabled();
        List<NsDevice> devices = new ArrayList<>();
        int page = 1;
        int count = Integer.MAX_VALUE;
        while (devices.size() < count) {
            UriComponentsBuilder builder = UriComponentsBuilder
                    .fromHttpUrl(properties.getBaseUrl() + ORG_API_PREFIX + "lora_wan/device/list/")
                    .queryParam("org", properties.getOrgId())
                    .queryParam("page", page)
                    .queryParam("limit", properties.getPageSize());
            if (projectId != null) {
                builder.queryParam("project", projectId);
            }
            if (eui != null) {
                builder.queryParam("dev_eui", eui);
            }
            JsonNode response = exchangeForJson(builder.build().toUriString());
            requireSuccess(response, "NS device list");
            JsonNode data = response.path("data");
            count = response.path("count").asInt(data.size());
            for (JsonNode item : data) {
                String rawEui = firstText(item, "dev_eui", "devEui", "devEUI");
                if (rawEui == null || rawEui.isBlank()) continue;
                devices.add(new NsDevice(
                        rawEui,
                        item.path("project").asInt(item.path("project_id").asInt(0)),
                        item.path("app").asInt(item.path("app_id").asInt(0)),
                        firstText(item, "name", "device_name"),
                        firstBool(item, "is_online", "online"),
                        firstInt(item, "f_cnt_up", "frame_count_up"),
                        parseLastSeen(item.get("last_seen"))));
            }
            if (data.size() < properties.getPageSize()) break;
            page++;
        }
        return devices;
    }

    /**
     * device/detail counters — the authoritative per-device frame counters.
     * The list endpoint's f_cnt_up has been observed to render as 0 for whole
     * fleets (2026-11-02 p43 check), so loss-rate differential polling must
     * use this call, not the list.
     */
    public NsDeviceDetail fetchDeviceDetail(String eui) {
        requireEnabled();
        UriComponentsBuilder builder = UriComponentsBuilder
                .fromHttpUrl(properties.getBaseUrl() + ORG_API_PREFIX + "lora_wan/device/detail/")
                .queryParam("org", properties.getOrgId())
                .queryParam("dev_eui", eui);
        JsonNode response = exchangeForJson(builder.build().toUriString());
        requireSuccess(response, "NS device detail");
        JsonNode data = response.path("data");
        if (data.isArray()) {
            data = data.size() > 0 ? data.get(0) : objectMapper.createObjectNode();
        }
        return new NsDeviceDetail(
                firstLong(data, "f_cnt_up", "frame_count_up"),
                firstLong(data, "n_f_cnt_down", "nf_cnt_down"),
                firstLong(data, "a_f_cnt_down", "af_cnt_down"),
                firstText(data, "latest_gateway", "downlink_gateway", "gateway"),
                parseLastSeenIn(data.get("last_seen"), ZoneId.of("UTC")),
                firstBool(data, "skip_f_cnt_check"));
    }

    private void requireEnabled() {
        if (!properties.isEnabled()) {
            throw new IllegalStateException("NS client is disabled");
        }
    }

    private JsonNode exchangeForJson(String uri) {
        String token = getToken();
        try {
            HttpHeaders headers = new HttpHeaders();
            headers.set("x-token", token);
            ResponseEntity<String> response = rest.exchange(
                    URI.create(uri), HttpMethod.GET, new HttpEntity<>(headers), String.class);
            return objectMapper.readTree(response.getBody() == null ? "{}" : response.getBody());
        } catch (Exception e) {
            throw new IllegalStateException("NS call failed: " + redact(uri), e);
        }
    }

    private synchronized String getToken() {
        CachedToken cached = tokenCache.get(properties.getUsername());
        if (cached != null && Instant.now().isBefore(cached.expiresAt())) {
            return cached.token();
        }
        try {
            HttpHeaders headers = new HttpHeaders();
            headers.setContentType(MediaType.APPLICATION_JSON);
            Map<String, String> payload = Map.of(
                    "username", properties.getUsername(), "password", properties.getPassword());
            ResponseEntity<String> response = rest.postForEntity(
                    properties.getBaseUrl() + LOGIN_API_PREFIX + "login/",
                    new HttpEntity<>(objectMapper.writeValueAsString(payload), headers),
                    String.class);
            JsonNode root = objectMapper.readTree(response.getBody());
            JsonNode data = root.path("data");
            String token = data.path("token").asText(root.path("token").asText());
            if (token.isBlank()) {
                throw new IllegalStateException("NS login returned no token");
            }
            tokenCache.put(properties.getUsername(), new CachedToken(token, Instant.now().plusSeconds(3000)));
            return token;
        } catch (IllegalStateException e) {
            throw e;
        } catch (Exception e) {
            throw new IllegalStateException("NS login failed", e);
        }
    }

    private static void requireSuccess(JsonNode response, String operation) {
        if (response.path("code").asInt(-1) != 0) {
            throw new IllegalStateException(operation + " failed: " + response.path("msg").asText());
        }
    }

    private static String firstText(JsonNode node, String... fields) {
        for (String field : fields) {
            String value = node.path(field).asText(null);
            if (value != null && !value.isBlank()) return value;
        }
        return null;
    }

    private static Boolean firstBool(JsonNode node, String... fields) {
        for (String field : fields) {
            JsonNode value = node.path(field);
            if (value.isBoolean()) return value.asBoolean();
            if (value.isNumber()) return value.asInt() != 0;
        }
        return null;
    }

    private static Integer firstInt(JsonNode node, String... fields) {
        for (String field : fields) {
            JsonNode value = node.path(field);
            if (value.isNumber()) return value.asInt();
        }
        return null;
    }

    private static Long firstLong(JsonNode node, String... fields) {
        for (String field : fields) {
            JsonNode value = node.path(field);
            if (value.isNumber()) return value.asLong();
        }
        return null;
    }

    /** last_seen arrives as "yyyy-MM-dd HH:mm:ss" or "" when never seen; a
     * numeric epoch (s or ms) is tolerated for forward compatibility. */
    static Instant parseLastSeen(JsonNode node) {
        return parseLastSeenIn(node, NS_ZONE);
    }

    /** Zone-parameterized variant: the list endpoint renders NS-host local
     * time (UTC+8) while device/detail was measured returning UTC (2026-11-03,
     * capsule 000319: last_seen 02:33:28 ≈ 30 min prior). Same vendor, two
     * timezones — pass the zone explicitly per endpoint. */
    static Instant parseLastSeenIn(JsonNode node, ZoneId zone) {
        if (node == null || node.isNull()) return null;
        if (node.isNumber()) {
            long v = node.asLong();
            return Instant.ofEpochMilli(v > 1_000_000_000_000L ? v : v * 1000);
        }
        String text = node.asText(null);
        if (text == null || text.isBlank()) return null;
        try {
            return LocalDateTime.parse(text.trim(), SEEN_FORMAT).atZone(zone).toInstant();
        } catch (Exception e) {
            return null;
        }
    }

    private static String redact(String uri) {
        return uri.replaceAll("(?i)(token=)[^&]+", "$1***");
    }

    /**
     * @param online  NS-reported online flag; null when the API omits it
     * @param fCntUp  NS uplink frame counter; null when omitted. L6: may
     *                disagree with NS UI — evidence only.
     * @param lastSeen NS-reported last uplink time; null when the device has
     *                 never been heard. Same L6 evidence-only caveat.
     */
    public record NsDevice(String eui, int projectId, int appId, String name,
                           Boolean online, Integer fCntUp, Instant lastSeen) {}

    /**
     * @param fCntUp    authoritative uplink frame counter (device/detail);
     *                  null when the NS omits it
     * @param lastSeen  NS-reported last uplink time, null when never heard
     */
    public record NsDeviceDetail(Long fCntUp, Long nFCntDown, Long aFCntDown,
                                 String latestGateway, Instant lastSeen, Boolean skipFCntCheck) {}

    private record CachedToken(String token, Instant expiresAt) {}
}
