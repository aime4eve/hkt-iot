package com.hkt.devicehub.infrastructure.thingsboard;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.application.TelemetryFrame;
import org.junit.jupiter.api.Test;

import java.util.Optional;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * TbFrameNormalizer contract tests: authoritative result path, dataHex
 * fallback path, 2s same-frame dedup, frameId stability.
 */
class TbFrameNormalizerTest {

    private static final String TB_ID = "3f6f1b2c-9a4d-4e2f-8c1a-2b3c4d5e6f70";
    private static final String EUI = "0018b20000001122";

    private final ObjectMapper mapper = new ObjectMapper();
    private final TbFrameNormalizer normalizer = new TbFrameNormalizer(mapper);
    private final TbFrameNormalizer.DeviceRef ref =
            new TbFrameNormalizer.DeviceRef(TB_ID, EUI, "LIVESTOCK");

    private JsonNode timeseries(String json) throws Exception {
        return mapper.readTree(json);
    }

    @Test
    void resultWithDecodeStatusTrueIsAuthoritative() throws Exception {
        JsonNode ts = timeseries("""
                {
                  "result": [{"ts": 1000, "value": "{\\"decodeStatus\\":true,\\"decodeData\\":{\\"properties\\":{\\"battery\\":87,\\"latitude\\":22.5}}}"}],
                  "dataHex": [{"ts": 1100, "value": "aabbcc"}],
                  "rssi":  [{"ts": 1000, "value": -80}],
                  "snr":   [{"ts": 1000, "value": 9}],
                  "downLinkGateway": [{"ts": 1000, "value": "gw-01"}]
                }
                """);
        TbFrameNormalizer.ParseResult result = normalizer.normalizePage(ref, ts);
        assertEquals(1, result.frames().size(), "dataHex within 2s of a result frame must be suppressed");
        assertTrue(result.skippedTs().isEmpty());

        TelemetryFrame frame = result.frames().get(0);
        assertEquals(1, frame.version());
        assertEquals(1000, frame.ts());
        assertEquals(EUI, frame.devEui());
        assertEquals(TB_ID, frame.tbDeviceId());
        assertEquals(87, ((Number) frame.properties().get("battery")).intValue());
        assertNull(frame.dataHex());
        assertEquals(-80, frame.rssi());
        assertEquals(9, frame.snr());
        assertEquals("gw-01", frame.gatewayId());
    }

    @Test
    void dataHexBeyond2sBecomesFallbackFrame() throws Exception {
        JsonNode ts = timeseries("""
                {
                  "result": [{"ts": 1000, "value": "{\\"decodeStatus\\":true,\\"decodeData\\":{\\"properties\\":{\\"battery\\":87}}}"}],
                  "dataHex": [{"ts": 5000, "value": "aabbcc"}]
                }
                """);
        TbFrameNormalizer.ParseResult result = normalizer.normalizePage(ref, ts);
        assertEquals(2, result.frames().size());
        TelemetryFrame fallback = result.frames().get(1);
        assertEquals(5000, fallback.ts());
        assertEquals("aabbcc", fallback.dataHex());
        assertTrue(fallback.properties().isEmpty());
    }

    @Test
    void undecodableResultIsSkippedAndReported() throws Exception {
        JsonNode ts = timeseries("""
                {
                  "result": [{"ts": 2000, "value": "{\\"decodeStatus\\":false}"}]
                }
                """);
        TbFrameNormalizer.ParseResult result = normalizer.normalizePage(ref, ts);
        assertTrue(result.frames().isEmpty());
        assertEquals(1, result.skippedTs().size());
        assertEquals(2000, result.skippedTs().get(0));
    }

    @Test
    void frameIdIsStableAndDistinct() {
        String first = TbFrameNormalizer.frameId(TB_ID, 1000);
        String second = TbFrameNormalizer.frameId(TB_ID, 1000);
        assertEquals(first, second, "same tbDeviceId + ts must yield the same frameId");
        assertNotEquals(first, TbFrameNormalizer.frameId(TB_ID, 1001));
        assertNotEquals(first, TbFrameNormalizer.frameId(EUI, 1000));
    }

    @Test
    void wsDataHexWithin2sOfResultIsSuppressed() {
        // WS dedup cache prunes by wall clock — use near-now timestamps.
        long now = System.currentTimeMillis();
        JsonNode resultValue = mapper.valueToTree(
                java.util.Map.of("decodeStatus", true,
                        "decodeData", java.util.Map.of("properties", java.util.Map.of("battery", 50))));
        Optional<TelemetryFrame> resultFrame = normalizer.normalizeWsPoint(ref, "result", now, resultValue);
        assertTrue(resultFrame.isPresent());

        JsonNode hexValue = mapper.valueToTree("aabbcc");
        assertTrue(normalizer.normalizeWsPoint(ref, "dataHex", now + 500, hexValue).isEmpty(),
                "dataHex within 2s of a WS result frame must be suppressed");
        assertTrue(normalizer.normalizeWsPoint(ref, "dataHex", now + 3_000, hexValue).isPresent(),
                "dataHex beyond the 2s window is a fallback frame");
    }

    @Test
    void wsUndecodableResultDoesNotSuppressLaterDataHex() {
        long now = System.currentTimeMillis();
        JsonNode badResult = mapper.valueToTree(java.util.Map.of("decodeStatus", false));
        assertTrue(normalizer.normalizeWsPoint(ref, "result", now, badResult).isEmpty());
        JsonNode hexValue = mapper.valueToTree("aabbcc");
        // No authoritative result was remembered, so the hex survives.
        assertFalse(normalizer.normalizeWsPoint(ref, "dataHex", now + 500, hexValue).isEmpty());
    }
}
