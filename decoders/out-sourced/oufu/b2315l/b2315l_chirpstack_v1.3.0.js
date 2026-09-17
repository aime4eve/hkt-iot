// ============================================================================
// Oufu/Oviphone B2315L LoRaWAN wristband - ChirpStack uplink payload codec.
// Protocol reference: "Oviphone B2315L LoRaWAN Protocal" (V1.3, 2023-12-14).
//
// ================================ CHANGELOG =================================
// 2026-09-09 v1.3 - New codec implementing protocol V1.3 (2023-12-14) as a
//   separate file; chirpstack-decode.js remains the V2.1 (BD-framed) codec.
//   This V1.3 firmware sends compact frames: MessageId + payload, NO 0xBD
//   header and NO checksum byte (matches captured A2313.923.O.01 devices).
//   Uplink messages implemented per document:
//   - 0xBB firmware version: Version_len + ASCII (2.1)
//   - 0xF6 battery/steps/signal: Bat_volt U16 + Step_num U32 +
//     Signal_strength U8 + timestamp (2.2); Bat_percent maps the documented
//     battery bars (0..3 -> 0/30/60/100%)
//   - 0x03 GPS/BDS: lon/lat IEEE-754 double LE + N/S + E/W + A/V +
//     timestamp (2.3)
//   - 0xC2 heart rate / blood pressure: bp_high U16 + bp_low U16 +
//     Bp_heart U16 + timestamp (2.4)
//   - 0xBA temperature: timestamp flag + optional timestamp + temp type +
//     wrist/body (+environment) S16 x10 (2.5)
//   - 0x02 alarm: Upl_warn U16 bitfield + timestamp (2.6); bits per this
//     document: 0x4000 fall, 0x0100 wearing, 0x0010 taken_off, 0x0004
//     shutdown, 0x0002 sos, 0x0001 low_battery (V2.1 later added 0x0020
//     sedentary / 0x0080 sos_cancel - not part of this spec)
//   - 0xD6 BLE location: type + utc + Total_PackCount + N*(major, minor,
//     rssi); the documented layout has NO Total_groups byte (2.7)
//   - FF 00 FF time-sync request (4.1)
//   0xC1 (download message status check) is listed in the Message ID table
//   but has no payload definition in the document; frames starting with it
//   are skipped with a warning.
//   Multiple frames concatenated in one uplink are supported (2.7 example
//   shows single frames, but captured devices concatenate).
//   Output field names follow the document; unknown/garbage bytes are
//   reported through output.warnings without aborting remaining frames.
// ============================================================================

var decoded = {};

var MSG_IDS = {
    0x02: 1, 0x03: 1, 0xB5: 0, 0xBA: 1, 0xBB: 1,
    0xC1: 1, 0xC2: 1, 0xD6: 1, 0xF6: 1
};

function decodeUplink(input) {
    var bytes = input.bytes;
    decoded = {};
    var warnings = [];
    var offset = 0;

    while (offset < bytes.length) {
        // 4.1 time-sync request has no Message ID: FF 00 FF.
        if (bytes[offset] === 0xFF && bytes[offset + 1] === 0x00 && bytes[offset + 2] === 0xFF) {
            decoded.time_request = 1;
            offset += 3;
            continue;
        }
        if (!MSG_IDS[bytes[offset]]) {
            warnings.push("offset " + offset + ": skipped unknown byte 0x" + hex8(bytes[offset]));
            offset += 1;
            continue;
        }
        if (bytes.length - offset < 2) {
            warnings.push("truncated frame header at offset " + offset);
            break;
        }

        var consumed;
        switch (bytes[offset]) {
            case 0xBB: consumed = decodeFirmwareVersion(bytes, offset); break;
            case 0xF6: consumed = decodeBatteryStepSignal(bytes, offset); break;
            case 0x03: consumed = decodeGps(bytes, offset); break;
            case 0xC2: consumed = decodeHeartRateBp(bytes, offset); break;
            case 0xBA: consumed = decodeTemperature(bytes, offset, warnings); break;
            case 0x02: consumed = decodeAlarm(bytes, offset); break;
            case 0xD6: consumed = decodeLbe(bytes, offset); break;
            case 0xC1:
                warnings.push("offset " + offset + ": 0xC1 has no payload definition in V1.3");
                offset += 1;
                continue;
            default:
                warnings.push("offset " + offset + ": unknown MessageId 0x" + hex8(bytes[offset]));
                offset += 1;
                continue;
        }
        if (consumed < 0) {
            warnings.push("truncated MessageId 0x" + hex8(bytes[offset]) + " frame at offset " + offset);
            break;
        }
        offset += consumed;
    }

    return { data: decoded, warnings: warnings };
}

// 2.6 Upl_warn bitfield (values as read after little-endian byte swap).
var ALARM_FLAGS = [
    [0x0001, "low_battery"],
    [0x0002, "sos"],
    [0x0004, "shutdown"],
    [0x0010, "taken_off"],
    [0x0100, "wearing"],
    [0x4000, "fall"]
];

function flagNames(value) {
    var names = [];
    for (var i = 0; i < ALARM_FLAGS.length; i++) {
        if (value & ALARM_FLAGS[i][0]) names.push(ALARM_FLAGS[i][1]);
    }
    return names;
}

// 2.1 BB Version_lenU8 ascii[len] (2 + len bytes)
function decodeFirmwareVersion(bytes, offset) {
    if (bytes.length - offset < 2) return -1;
    var len = bytes[offset + 1];
    if (bytes.length - offset < 2 + len) return -1;
    var version = "";
    for (var i = 0; i < len; i++) version += String.fromCharCode(bytes[offset + 2 + i]);
    decoded.Version_len = len;
    decoded.Version = version;
    return 2 + len;
}

// 2.2 F6 Bat_voltU16 Step_numU32 Signal_strengthU8 tsU32 (12 bytes)
function decodeBatteryStepSignal(bytes, offset) {
    if (bytes.length - offset < 12) return -1;
    var bat = readU16LE(bytes, offset + 1);
    decoded.Bat_volt = bat;
    decoded.Bat_percent = bat === 3 ? 100 : bat === 2 ? 60 : bat === 1 ? 30 : 0;
    decoded.Step_num = readU32LE(bytes, offset + 3);
    decoded.Signal_strength = readU8(bytes, offset + 7);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 8));
    return 12;
}

// 2.3 03 lonF64 latF64 ns ew status tsU32 (24 bytes)
function decodeGps(bytes, offset) {
    if (bytes.length - offset < 24) return -1;
    decoded.lon = readF64LE(bytes, offset + 1);
    decoded.lat = readF64LE(bytes, offset + 9);
    decoded.north_south = String.fromCharCode(bytes[offset + 17]);
    decoded.east_west = String.fromCharCode(bytes[offset + 18]);
    decoded.status = String.fromCharCode(bytes[offset + 19]);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 20));
    return 24;
}

// 2.4 C2 bp_highU16 bp_lowU16 Bp_heartU16 tsU32 (11 bytes)
function decodeHeartRateBp(bytes, offset) {
    if (bytes.length - offset < 11) return -1;
    decoded.bp_high = readU16LE(bytes, offset + 1);
    decoded.bp_low = readU16LE(bytes, offset + 3);
    decoded.Bp_heart = readU16LE(bytes, offset + 5);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 7));
    return 11;
}

// 2.5 BA Temp_flagU8 [tsU32] Temp_typeU8 WristS16 BodyS16 [EnvS16]
// flag 00 = timestamp present, 01 = absent; type 1 = wrist+body,
// type 2 = wrist+body+environment. Temperatures are x10 signed.
function decodeTemperature(bytes, offset, warnings) {
    if (bytes.length - offset < 2) return -1;
    var flag = bytes[offset + 1];
    if (flag !== 0x00 && flag !== 0x01) {
        warnings.push("temperature flag 0x" + hex8(flag) + " not defined in V1.3");
        return -1;
    }
    var pos = offset + 2;
    decoded.Timestamp_flag = flag;
    if (flag === 0x00) {
        if (bytes.length - pos < 4) return -1;
        decoded.timestamp = dateFormat(readU32LE(bytes, pos));
        pos += 4;
    }
    if (bytes.length - pos < 1) return -1;
    var type = bytes[pos];
    if (type !== 0x01 && type !== 0x02) {
        warnings.push("temperature type 0x" + hex8(type) + " not defined in V1.3");
        return -1;
    }
    decoded.Temp_type = type;
    var temps = type === 0x02 ? 3 : 2;
    if (bytes.length - pos < 1 + temps * 2) return -1;
    decoded.Wrist_Temp = readI16LE(bytes, pos + 1) / 10;
    decoded.Body_Temp = readI16LE(bytes, pos + 3) / 10;
    if (type === 0x02) decoded.Env_Temp = readI16LE(bytes, pos + 5) / 10;
    return pos + 1 + temps * 2 - offset;
}

// 2.6 02 Upl_warnU16 tsU32 (7 bytes)
function decodeAlarm(bytes, offset) {
    if (bytes.length - offset < 7) return -1;
    var warn = readU16LE(bytes, offset + 1);
    decoded.Upl_warn = warn.toString(16);
    decoded.alarm = flagNames(warn);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 3));
    return 7;
}

// 2.7 D6 typeU8 utcU32 countU8 count*(majorU16 minorU16 rssiI8)
// Documented layout has no Total_groups byte; captured devices may
// concatenate several such frames back to back.
function decodeLbe(bytes, offset) {
    if (bytes.length - offset < 7) return -1;
    decoded.Type = bytes[offset + 1];
    var ts = readU32LE(bytes, offset + 2);
    var count = bytes[offset + 6];
    if (bytes.length - offset < 7 + count * 5) return -1;
    var beacons = [];
    for (var i = 0; i < count; i++) {
        var base = offset + 7 + i * 5;
        beacons.push({
            Major: readU16LE(bytes, base),
            Minor: readU16LE(bytes, base + 2),
            Rssi: readI8(bytes, base + 4)
        });
    }
    decoded.ALL = [{
        Timestamp: dateFormat(ts),
        Total_PackCount: count,
        beacons: beacons
    }];
    return 7 + count * 5;
}

function readU8(bytes, o) {
    return bytes[o] & 0xFF;
}

function readI8(bytes, o) {
    var value = bytes[o] & 0xFF;
    return value > 0x7F ? value - 0x100 : value;
}

function readU16LE(bytes, o) {
    return ((bytes[o + 1] << 8) | bytes[o]) & 0xFFFF;
}

function readI16LE(bytes, o) {
    var value = readU16LE(bytes, o);
    return value > 0x7FFF ? value - 0x10000 : value;
}

function readU32LE(bytes, o) {
    return ((bytes[o + 3] * 0x1000000) + (bytes[o + 2] << 16) + (bytes[o + 1] << 8) + bytes[o]) >>> 0;
}

// IEEE-754 double, little-endian (2.3 GPS lon/lat).
function readF64LE(bytes, o) {
    var sign = (bytes[o + 7] & 0x80) ? -1 : 1;
    var exponent = ((bytes[o + 7] & 0x7F) << 4) | (bytes[o + 6] >> 4);
    var mantissa = bytes[o + 6] & 0x0F;
    for (var i = o + 5; i >= o; i--) {
        mantissa = mantissa * 256 + bytes[i];
    }
    if (exponent === 0x7FF) return sign * (mantissa === 0 ? Infinity : NaN);
    if (exponent === 0) return sign * mantissa * Math.pow(2, -1074);
    return sign * (1 + mantissa / Math.pow(2, 52)) * Math.pow(2, exponent - 1023);
}

function hex8(value) {
    return (value & 0xFF).toString(16);
}

function dateFormat(timestamp) {
    var date = new Date(timestamp * 1000);
    var Y = date.getFullYear();
    var M = String(date.getMonth() + 1).padStart(2, '0');
    var D = String(date.getDate()).padStart(2, '0');
    var h = String(date.getHours()).padStart(2, '0');
    var m = String(date.getMinutes()).padStart(2, '0');
    var s = String(date.getSeconds()).padStart(2, '0');
    return `${Y}-${M}-${D} ${h}:${m}:${s}`;
}
