/**
 * Payload Decoder for Cattle Ear Tag
 *
 * @product  Cattle Ear Tag (LoRaWAN Ear Tag Communication Protocol V6.0)
 * @platform ChirpStack v4 device-profile codec
 *           (for ChirpStack v3 use Decode(fPort, bytes) / Encode(data, fPort) instead)
 *
 * Uplink frame (exactly 10 bytes):
 *   [0]    0xAA   header
 *   [1]    protocol version (e.g. 0x06)
 *   [2-3]  battery voltage, BCD, unit mV (e.g. 0x3002 -> 3002 mV)
 *   [4-5]  temperature, BCD / 100, unit C (e.g. 0x3626 -> 36.26 C)
 *   [6-7]  step count, big-endian hexadecimal (e.g. 0x0060 -> 96)
 *   [8]    report interval, unit hour
 *   [9]    0x55   tail
 *
 * Downlink frame (set report interval): AA <hours> 55
 *
 * Note: the protocol does not define negative temperatures;
 * below-zero operation should be confirmed with the vendor.
 */

var FRAME_LENGTH = 10;
var FRAME_HEADER = 0xAA;
var FRAME_TAIL = 0x55;
var DOWNLINK_FPORT = 1; // adjust to the fPort the device actually listens on

function decodeUplink(input) {
    var bytes = input.bytes;

    if (bytes.length !== FRAME_LENGTH) {
        return { errors: ["Invalid payload length: expected " + FRAME_LENGTH + " bytes, got " + bytes.length] };
    }
    if (bytes[0] !== FRAME_HEADER || bytes[FRAME_LENGTH - 1] !== FRAME_TAIL) {
        return { errors: ["Invalid packet header or tail"] };
    }

    var voltage = bcdToDecimal((bytes[2] << 8) | bytes[3]);
    if (voltage === null) {
        return { errors: ["Invalid BCD encoding in battery voltage field"] };
    }

    var temperature = bcdToDecimal((bytes[4] << 8) | bytes[5]);
    if (temperature === null) {
        return { errors: ["Invalid BCD encoding in temperature field"] };
    }

    return {
        data: {
            protocol_version: bytes[1],
            battery_voltage: voltage,        // mV
            temperature: temperature / 100,  // C
            step_count: (bytes[6] << 8) | bytes[7],
            report_interval: bytes[8]        // hours
        }
    };
}

// Set the report interval: schedule a downlink with JSON payload
// { "report_interval": <hours> }; the device receives AA <hours> 55.
function encodeDownlink(input) {
    var hours = input.data.report_interval;
    if (typeof hours !== "number" || !isFinite(hours) || Math.floor(hours) !== hours || hours < 1 || hours > 255) {
        return { errors: ["report_interval must be an integer between 1 and 255 (hours)"] };
    }
    return {
        fPort: DOWNLINK_FPORT,
        bytes: [FRAME_HEADER, hours, FRAME_TAIL]
    };
}

// BCD to decimal; returns null when any nibble is not a valid BCD digit (0-9).
function bcdToDecimal(bcd) {
    var decimal = 0;
    var multiplier = 1;
    while (bcd > 0) {
        var digit = bcd & 0x0F;
        if (digit > 9) {
            return null;
        }
        decimal += digit * multiplier;
        multiplier *= 10;
        bcd = bcd >> 4;
    }
    return decimal;
}
