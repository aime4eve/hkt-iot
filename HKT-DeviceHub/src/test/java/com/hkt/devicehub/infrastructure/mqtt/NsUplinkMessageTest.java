package com.hkt.devicehub.infrastructure.mqtt;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Phase 0 spike locked the wire format (2026-09-29): ts is epoch seconds as a
 * float with sub-second precision, data is base64 of the raw frame bytes. The
 * normalization round(ts*1000) and the hex form of dataHex are contract — the
 * old-generation 1970 bug came from storing seconds as milliseconds.
 */
class NsUplinkMessageTest {

    private final ObjectMapper mapper = new ObjectMapper();

    @Test
    void parsesCompactBusPayload() {
        String payload = "{\"project\": 219, \"app\": 36, \"channel\": \"LoRaWAN\", "
                + "\"ts\": 1790588627.5496006, \"devEUI\": \"0095690E00003588\", \"fPort\": 10, "
                + "\"data\": \"aGt0AA8BBAQDZBABrv5WEQa5+bUVAAAL/2cM/5oNAJk5AAAAAAAAAAAAAAAAAAAe\", "
                + "\"object\": {}, \"rssi\": -33, \"rxInfo\": []}";
        NsUplinkMessage msg = NsUplinkMessage.parse(mapper, 219, payload);
        assertNotNull(msg);
        assertEquals(219, msg.topicProject());
        assertEquals(219L, msg.payloadProject());
        assertEquals("0095690e00003588", msg.devEui());
        assertEquals(1790588627550L, msg.tsMs());
        assertFalse(msg.tsUnitAnomaly());
        // "aGt0" is base64 of "hkt" — the frame magic
        assertTrue(msg.dataHex().startsWith("686b74"));
        assertEquals(96, msg.dataHex().length()); // 64 base64 chars (no padding) = 48 bytes
    }

    @Test
    void hexEncodingIsLowercaseAndExact() {
        assertEquals("686b7400", NsUplinkMessage.toHex(
                java.util.Base64.getDecoder().decode("aGt0AA==")));
        assertEquals("00ff10", NsUplinkMessage.toHex(new byte[]{0, (byte) 0xff, 0x10}));
    }

    @Test
    void parsesPrettyPrintedPayload() {
        String payload = "{\n  \"project\": 89,\n  \"channel\": \"LoRaWAN\",\n"
                + "  \"ts\": 1788150673.5004249,\n"
                + "  \"devEUI\": \"001a0103ff000262\",\n"
                + "  \"data\": \"aGt0AHsBEA==\",\n  \"object\": {}\n}";
        NsUplinkMessage msg = NsUplinkMessage.parse(mapper, 219, payload);
        assertNotNull(msg);
        assertEquals(89L, msg.payloadProject());
        assertEquals("001a0103ff000262", msg.devEui());
        assertEquals(1788150673500L, msg.tsMs());
        assertEquals("686b74007b0110" , msg.dataHex());
    }

    @Test
    void tsAlreadyInMillisecondsIsFlaggedNotConverted() {
        // defensive path: if NS ever emits ms, write as-is and mark the anomaly
        NsUplinkMessage msg = NsUplinkMessage.parse(mapper, 219,
                "{\"project\":219,\"ts\":1790588627549,\"devEUI\":\"a\",\"data\":\"aGt0\"}");
        assertNotNull(msg);
        assertTrue(msg.tsUnitAnomaly());
        assertEquals(1790588627549L, msg.tsMs());
    }

    @Test
    void missingFieldsOrGarbageReturnNull() {
        assertNull(NsUplinkMessage.parse(mapper, 219, "not json"));
        assertNull(NsUplinkMessage.parse(mapper, 219, "{\"project\":219}"));
        assertNull(NsUplinkMessage.parse(mapper, 219, "{\"project\":219,\"devEUI\":\"a\",\"ts\":1.0}"));
        assertNull(NsUplinkMessage.parse(mapper, 219,
                "{\"project\":219,\"devEUI\":\"a\",\"ts\":1.0,\"data\":\"%%%not-base64%%%\"}"));
        assertNull(NsUplinkMessage.parse(mapper, 219,
                "{\"project\":219,\"devEUI\":\"a\",\"ts\":1.0,\"data\":\"\"}"));
    }
}
