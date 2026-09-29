package com.hkt.devicehub.infrastructure.mqtt;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;

import java.util.Base64;

/**
 * One NS uplink message from the shared MQTT bus (topic
 * {@code org/{org}/project/{nsProjectId}/device/{devEui}/dat/up}).
 * <p>
 * Field semantics verified on the live bus (Phase 0 spike, 2026-09-29):
 * {@code ts} is <b>epoch seconds as a float with sub-second precision</b> —
 * the old-generation 1970 bug came from storing that value as milliseconds.
 * Normalization to TB milliseconds is locked here: {@code round(ts * 1000)}.
 * {@code data} is base64 of the raw LoRaWAN frame payload; the bridge path
 * stores the same bytes as lowercase hex in TB's {@code dataHex} key, so the
 * backup channel must write the identical form.
 *
 * @param topicProject NS project id from the topic segment
 * @param payloadProject NS project id from the payload (authoritative when present)
 * @param devEui lowercase DevEUI
 * @param tsMs frame timestamp in epoch milliseconds, normalized from payload ts
 * @param tsRaw raw payload ts value (for diagnostics)
 * @param dataHex raw frame bytes as lowercase hex (bridge-compatible format)
 * @param tsUnitAnomaly true when the payload ts was already in milliseconds
 */
public record NsUplinkMessage(long topicProject, Long payloadProject, String devEui,
                              long tsMs, double tsRaw, String dataHex, boolean tsUnitAnomaly) {

    /** Parses one dat/up payload; returns null when the message is not usable. */
    public static NsUplinkMessage parse(ObjectMapper mapper, long topicProject, String payload) {
        JsonNode node;
        try {
            node = mapper.readTree(payload);
        } catch (Exception e) {
            return null;
        }
        JsonNode euiNode = node.path("devEUI");
        JsonNode tsNode = node.path("ts");
        JsonNode dataNode = node.path("data");
        if (!euiNode.isTextual() || euiNode.asText().isBlank()
                || !tsNode.isNumber() || !dataNode.isTextual()) {
            return null;
        }
        String dataHex;
        try {
            dataHex = toHex(Base64.getDecoder().decode(dataNode.asText()));
        } catch (IllegalArgumentException e) {
            return null;
        }
        if (dataHex.isEmpty()) {
            return null;
        }
        double ts = tsNode.asDouble();
        boolean msAnomaly = ts >= 1e11;
        long tsMs = msAnomaly ? Math.round(ts) : Math.round(ts * 1000);
        Long payloadProject = node.path("project").isNumber() ? node.path("project").asLong() : null;
        return new NsUplinkMessage(topicProject, payloadProject,
                euiNode.asText().toLowerCase(), tsMs, ts, dataHex, msAnomaly);
    }

    static String toHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder(bytes.length * 2);
        for (byte b : bytes) {
            sb.append(Character.forDigit((b >> 4) & 0xF, 16));
            sb.append(Character.forDigit(b & 0xF, 16));
        }
        return sb.toString();
    }
}
