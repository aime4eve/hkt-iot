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
                        firstInt(item, "f_cnt_up", "frame_count_up")));
            }
            if (data.size() < properties.getPageSize()) break;
            page++;
        }
        return devices;
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

    private static String redact(String uri) {
        return uri.replaceAll("(?i)(token=)[^&]+", "$1***");
    }

    /**
     * @param online  NS-reported online flag; null when the API omits it
     * @param fCntUp  NS uplink frame counter; null when omitted. L6: may
     *                disagree with NS UI — evidence only.
     */
    public record NsDevice(String eui, int projectId, int appId, String name,
                           Boolean online, Integer fCntUp) {}

    private record CachedToken(String token, Instant expiresAt) {}
}
