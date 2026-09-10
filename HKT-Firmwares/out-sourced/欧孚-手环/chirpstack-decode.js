// ============================================================================
// Oufu B2315L LoRaWAN band - ChirpStack uplink payload codec.
// Protocol reference: B2315L LoRaWAN communication protocol V2.1.
//
// ================================ CHANGELOG =================================
// 2026-09-09 v2.1 - Aligned decoder with B2315L LoRaWAN protocol V2.1:
//   1. [add] 0xBB firmware version (4.3.1): Version_len + ASCII string; this
//      handler was missing entirely.
//   2. [fix] 0x03 GPS (4.2.1): replaced the hand-written double parser with a
//      correct IEEE-754 little-endian reader (the old one assembled the
//      mantissa backwards and dropped the sign); verified against the doc
//      examples (lon 121.6131622, lat 31.2059137).
//   3. [fix] 0xD6 BLE location (4.2.2): multi-group frames were parsed at
//      fixed offsets and consumed a wrong length; now walks every group
//      (Utc + count + 5*count bytes). Output shape changed: decoded.ALL is
//      now [{ Timestamp, Total_PackCount, beacons: [{Major, Minor, Rssi}] }].
//   4. [fix] 0x32 health (4.4.1): step value (ID=01) was read from a wrong
//      offset from the 2nd TLV on (9+intVal*2+1 -> cur+1); diastolic (06) and
//      systolic (07) field names were swapped (now bp_low / bp_high); added
//      HRV (ID=09).
//   5. [fix] 0x21 alarm-2 (4.1.2): Upl_warn is U32 (4 bytes); the old code
//      read U16 and truncated it. Alarm names now decoded to decoded.alarm.
//   6. [fix] 0x02 alarm-1 (4.1.1): alarm bit names decoded to decoded.alarm
//      (fall / sos / sos_cancel / low_battery / shutdown / taken_off /
//      sedentary / wearing).
//   7. [fix] 0xF9 (4.3.2): Signal_strength is I16 (signed) per spec.
//   8. [fix] 0xC0 feedback (4.5.1): all N echoed Message IDs are collected
//      now (the old loop mutated the byte array mid-read and kept only the
//      last one); decoded.feedback is now an array of hex strings.
//   9. [fix] Timestamps use full U32 reads; the old (byte << 24) form breaks
//      for values >= 2^31.
//  10. [fix] An unknown MessageId no longer wipes the whole packet: the codec
//      scans forward to the next 0xBD header, records a warning and keeps
//      decoding trailing frames. FF00FF time sync (5.2.1) is detected at any
//      position, so it no longer collides with the frame loop.
//  11. [keep] 0xB5 SOS, 0xC3 charge warn, 0x28 message feedback and the
//      legacy non-TLV 0x32 layout: not defined in V2.1 chapter 4 but observed
//      on real devices; kept for compatibility.
//  12. [note] Checksum (3.5) is intentionally not verified: the general
//      version does not require it and downlink may fill any byte.
// ============================================================================

// 2026-09-09 v2.2 - Support the real A2313 (AS923) firmware captured from
//   production wristbands (version string "A2313.923.O.01"), which deviates
//   from the V2.1 document framing:
//  13. [add] Compact framing auto-detection: when the payload does not start
//      with 0xBD, frames are parsed as MessageId + payload without the BD
//      header and without the checksum byte (verified against captured
//      uplinks; the V2.1 BD-framed format still decodes unchanged).
//  14. [fix] 0xD6 BLE location: accept both layouts - the documented one with
//      a Total_groups byte and the A2313 compact one without it (validated
//      against real frames before falling back).
//  15. [add] Undocumented A2313 frames (tentative, verified on 2 real samples
//      each, semantics to be confirmed with the vendor):
//      - C2: systolic / diastolic / heart rate + timestamp (118/75/80);
//      - BA: type + timestamp + flag + two temperatures /10 (27.7 and
//        35.8 deg C, wrist and body);
//      - F6: four counters (1 / 56..168 / 0 / 100) + timestamp; meaning
//        unknown, fields exported as F6_v1..F6_v4.
// ============================================================================

// A2313 compact frames carry the MessageId where BD-framed streams carry 0xBD.
var COMPACT_MSG_IDS = {
    0x02: 1, 0x03: 1, 0x21: 1, 0x28: 1, 0x32: 1, 0xB5: 1, 0xBA: 1,
    0xBB: 1, 0xC0: 1, 0xC2: 1, 0xC3: 1, 0xD6: 1, 0xF6: 1, 0xF9: 1
};

// Compact frames omit the checksum byte, so every frame-length guard shrinks
// by one byte relative to the BD-framed protocol layout.
var compactFrame = false;

function frameNeed(normalLen) {
    return normalLen - (compactFrame ? 1 : 0);
}

var decoded = {};

function decodeUplink(input) {
    var bytes = input.bytes;
    decoded = {};
    var warnings = [];
    var offset = 0;
    compactFrame = bytes.length > 0 && bytes[0] !== 0xBD &&
        !(bytes[0] === 0xFF && bytes[1] === 0x00 && bytes[2] === 0xFF);

    while (offset < bytes.length) {
        // 5.2.1 time sync request is the only frame without a 0xBD header.
        if (bytes[offset] === 0xFF && bytes[offset + 1] === 0x00 && bytes[offset + 2] === 0xFF) {
            decoded.time_request = 1;
            offset += 3;
            continue;
        }
        var normal = bytes[offset] === 0xBD;
        if (!normal && !COMPACT_MSG_IDS[bytes[offset]]) {
            warnings.push("offset " + offset + ": skipped unknown byte 0x" + hex8(bytes[offset]));
            offset += 1;
            continue;
        }
        var hOffset = normal ? offset : offset - 1;
        var idPos = normal ? offset + 1 : offset;
        if (bytes.length - offset < (normal ? 2 : 1)) {
            warnings.push("truncated frame header at offset " + offset);
            break;
        }

        var consumed;
        switch (bytes[idPos]) {
            case 0x02: consumed = decodeAlarm1(bytes, hOffset); break;
            case 0x21: consumed = decodeAlarm2(bytes, hOffset); break;
            case 0x03: consumed = decodeGps(bytes, hOffset); break;
            case 0xD6: consumed = decodeLbe(bytes, hOffset); break;
            case 0xBB: consumed = decodeFirmwareVersion(bytes, hOffset); break;
            case 0xF9: consumed = decodeBattery(bytes, hOffset); break;
            case 0x32: consumed = decodeHealth(bytes, hOffset, warnings); break;
            case 0xB5: consumed = decodeSos(bytes, hOffset); break;
            case 0xC3: consumed = decodeChargeWarn(bytes, hOffset); break;
            case 0xC2: consumed = decodeBloodPressure(bytes, hOffset); break;
            case 0xBA: consumed = decodeTemperature(bytes, hOffset); break;
            case 0xF6: consumed = decodeF6Counters(bytes, hOffset); break;
            case 0xC0: consumed = decodeDownlinkFeedback(bytes, hOffset); break;
            case 0x28: consumed = decodeMessageFeedback(bytes, hOffset); break;
            default: {
                warnings.push("offset " + offset + ": unknown MessageId 0x" + hex8(bytes[idPos]));
                if (normal) {
                    var nextFrame2 = bytes.indexOf(0xBD, offset + 2);
                    offset = nextFrame2 === -1 ? bytes.length : nextFrame2;
                } else {
                    offset += 1;
                }
                continue;
            }
        }
        if (consumed < 0) {
            warnings.push("truncated MessageId 0x" + hex8(bytes[idPos]) + " frame at offset " + offset);
            break;
        }
        offset += normal ? consumed : consumed - 2;
    }

    return { data: decoded, warnings: warnings };
}

// 4.1.1 Upl_warn bits (values as read after little-endian byte swap).
var ALARM1_FLAGS = [
    [0x0001, "low_battery"],
    [0x0002, "sos"],
    [0x0004, "shutdown"],
    [0x0010, "taken_off"],
    [0x0020, "sedentary"],
    [0x0080, "sos_cancel"],
    [0x0100, "wearing"],
    [0x4000, "fall"]
];

// 4.1.2 Alarm type=1 shutdown reason bits.
var ALARM2_FLAGS = [
    [0x0001, "power_off_manual"],
    [0x0002, "power_off_low_battery"],
    [0x0004, "power_off_charging"]
];

function flagNames(flags, value) {
    var names = [];
    for (var i = 0; i < flags.length; i++) {
        if (value & flags[i][0]) names.push(flags[i][1]);
    }
    return names;
}

// 4.1.1 BD 02 warnU16 tsU32 CK (9 bytes)
function decodeAlarm1(bytes, offset) {
    if (bytes.length - offset < frameNeed(9)) return -1;
    var warn = readU16LE(bytes, offset + 2);
    decoded.Upl_warn = warn.toString(16);
    decoded.alarm = flagNames(ALARM1_FLAGS, warn);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 4));
    return 9;
}

// 4.1.2 BD 21 alarmTypeU16 warnU32 tsU32 CK (13 bytes)
function decodeAlarm2(bytes, offset) {
    if (bytes.length - offset < frameNeed(13)) return -1;
    var alarmType = readU16LE(bytes, offset + 2);
    var warn = readU32LE(bytes, offset + 4);
    decoded.WarnType = alarmType.toString(16);
    decoded.Upl_warn = warn.toString(16);
    decoded.alarm = alarmType === 1 ? flagNames(ALARM2_FLAGS, warn) : [];
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 8));
    return 13;
}

// 4.2.1 BD 03 lonF64 latF64 ns ew status tsU32 CK (26 bytes)
function decodeGps(bytes, offset) {
    if (bytes.length - offset < frameNeed(26)) return -1;
    decoded.lon = readF64LE(bytes, offset + 2);
    decoded.lat = readF64LE(bytes, offset + 10);
    decoded.north_south = String.fromCharCode(bytes[offset + 18]);
    decoded.east_west = String.fromCharCode(bytes[offset + 19]);
    decoded.status = String.fromCharCode(bytes[offset + 20]);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 21));
    return 26;
}

// 4.2.2 BD D6 type groups... CK. Two layouts are accepted:
// - documented: type groups (tsU32 countU8 count*5 bytes) x groups CK
// - A2313 compact: type tsU32 countU8 count*5 bytes (no Total_groups byte)
function decodeLbe(bytes, offset) {
    if (bytes.length - offset < frameNeed(9)) return -1;
    decoded.Type = bytes[offset + 2];
    var groups = bytes[offset + 3];
    if (groups >= 1 && groups <= 8) {
        var pos = offset + 4;
        var fits = true;
        for (var g = 0; g < groups; g++) {
            if (bytes.length - pos < 5) { fits = false; break; }
            var need = 5 + bytes[pos + 4] * 5;
            if (bytes.length - pos < need) { fits = false; break; }
            pos += need;
        }
        if (fits) {
            decoded.Total_groups = groups;
            decoded.ALL = readLbeGroups(bytes, offset + 4, groups);
            return pos - offset + 1;
        }
    }
    var count = bytes[offset + 7];
    if (bytes.length - offset < frameNeed(9 + count * 5)) return -1;
    decoded.Total_groups = 1;
    decoded.ALL = readLbeGroups(bytes, offset + 3, 1);
    return 9 + count * 5;
}

function readLbeGroups(bytes, pos, groups) {
    var out = [];
    for (var g = 0; g < groups; g++) {
        var count = bytes[pos + 4];
        var beacons = [];
        for (var i = 0; i < count; i++) {
            var base = pos + 5 + i * 5;
            beacons.push({
                Major: readU16LE(bytes, base),
                Minor: readU16LE(bytes, base + 2),
                Rssi: readI8(bytes, base + 4)
            });
        }
        out.push({
            Timestamp: dateFormat(readU32LE(bytes, pos)),
            Total_PackCount: count,
            beacons: beacons
        });
        pos += 5 + count * 5;
    }
    return out;
}

// 4.3.1 BD BB lenU8 ascii[len] CK (4 + len bytes)
function decodeFirmwareVersion(bytes, offset) {
    if (bytes.length - offset < frameNeed(3)) return -1;
    var len = bytes[offset + 2];
    if (bytes.length - offset < frameNeed(4 + len)) return -1;
    var version = "";
    for (var i = 0; i < len; i++) version += String.fromCharCode(bytes[offset + 3 + i]);
    decoded.Version_len = len;
    decoded.Version = version;
    return 4 + len;
}

// 4.3.2 BD F9 batTypeU8 batVoltU16 sigTypeU8 sigI16 otherTypeU8 numU32 tsU32 CK
function decodeBattery(bytes, offset) {
    if (bytes.length - offset < frameNeed(18)) return -1;
    decoded.Bat_type = bytes[offset + 2];
    decoded.Bat_volt = readU16LE(bytes, offset + 3);
    decoded.Signal_type = bytes[offset + 5];
    decoded.Signal_strength = readI16LE(bytes, offset + 6);
    decoded.other_type = bytes[offset + 8];
    decoded.num = readU32LE(bytes, offset + 9);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 13));
    return 18;
}

// 4.4.1 BD 32 type tsU32 contentLenU16 TLV... CK (10 + contentLen bytes)
// TLV item: id = dataType << 3 | valueLen, then the value (little-endian).
function decodeHealth(bytes, offset, warnings) {
    if (bytes.length - offset < frameNeed(9)) return -1;
    if (bytes[offset + 2] !== 0x00) return decodeHealthLegacy(bytes, offset);
    decoded.health_type = 0;
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 3));
    var contentLen = readU16LE(bytes, offset + 7);
    if (bytes.length - offset < frameNeed(10 + contentLen)) return -1;
    var end = offset + 9 + contentLen;
    var cur = offset + 9;
    while (cur < end) {
        var id = bytes[cur];
        var dataType = id >> 3;
        var len = id & 0x07;
        if (cur + 1 + len > end) {
            warnings.push("health TLV at offset " + cur + " overruns content");
            break;
        }
        switch (dataType) {
            case 0x01: decoded.step = len >= 2 ? readU16LE(bytes, cur + 1) : readU8(bytes, cur + 1); break;
            case 0x02: decoded.Bp_heart = readU8(bytes, cur + 1); break;
            case 0x03: decoded.Body_Temp = readU16LE(bytes, cur + 1) / 10; break;
            case 0x04: decoded.Wrist_Temp = readU16LE(bytes, cur + 1) / 10; break;
            case 0x06: decoded.bp_low = readU8(bytes, cur + 1); break;   // 06 diastolic
            case 0x07: decoded.bp_high = readU8(bytes, cur + 1); break;  // 07 systolic
            case 0x08: decoded.BloodOxygen = readU8(bytes, cur + 1); break;
            case 0x09: decoded.hrv = len >= 2 ? readU16LE(bytes, cur + 1) : readU8(bytes, cur + 1); break;
            default: warnings.push("health dataType 0x" + hex8(dataType) + " not defined in V2.1");
        }
        cur += 1 + len;
    }
    return 10 + contentLen;
}

// Legacy fixed-layout health frame from older firmware (not part of V2.1).
function decodeHealthLegacy(bytes, offset) {
    if (bytes.length - offset < frameNeed(21)) return -1;
    decoded.bp_high = readU8(bytes, offset + 2);
    decoded.bp_low = readU8(bytes, offset + 3);
    decoded.Bp_heart = readU8(bytes, offset + 4);
    decoded.BloodOxygen = readU8(bytes, offset + 5);
    decoded.wrist_Temp = readU16LE(bytes, offset + 6) / 10;
    decoded.Body_Temp = readU16LE(bytes, offset + 8) / 10;
    decoded.step = readU32LE(bytes, offset + 10);
    decoded.Bat_volt = readU8(bytes, offset + 14);
    decoded.Signal_strength = readU8(bytes, offset + 15);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 16));
    return 21;
}

// 2.2(3) SOS trigger feedback: BD B5 statusU8 tsU32 CK (8 bytes)
function decodeSos(bytes, offset) {
    if (bytes.length - offset < frameNeed(8)) return -1;
    decoded.Status = readU8(bytes, offset + 2);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 3));
    return 8;
}

// Charge warning observed on real devices (not in V2.1 chapter 4).
function decodeChargeWarn(bytes, offset) {
    if (bytes.length - offset < frameNeed(8)) return -1;
    decoded.charge_warn = readU8(bytes, offset + 2);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 3));
    return 8;
}

// Undocumented A2313 frame (tentative): C2 sysU16 diaU16 hrU16 tsU32 CK.
// Real samples: 118/75/80 and 117/74/74 - blood pressure + heart rate.
function decodeBloodPressure(bytes, offset) {
    if (bytes.length - offset < frameNeed(13)) return -1;
    decoded.bp_high = readU16LE(bytes, offset + 2);
    decoded.bp_low = readU16LE(bytes, offset + 4);
    decoded.Bp_heart = readU16LE(bytes, offset + 6);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 8));
    return 13;
}

// Undocumented A2313 frame (tentative): BA type tsU32 flagU8 temp1U16 temp2U16 CK.
// Values /10 look like wrist/skin temp and body temp (27.7 / 35.8 deg C).
function decodeTemperature(bytes, offset) {
    if (bytes.length - offset < frameNeed(13)) return -1;
    decoded.BA_type = bytes[offset + 2];
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 3));
    decoded.BA_flag = bytes[offset + 7];
    decoded.temp1 = readU16LE(bytes, offset + 8) / 10;
    decoded.temp2 = readU16LE(bytes, offset + 10) / 10;
    return 13;
}

// Undocumented A2313 frame: F6 v1U16 v2U16 v3U16 v4U8 tsU32 CK. Real samples
// carry (1, 56..168, 0, 100); meaning unknown, fields kept raw.
function decodeF6Counters(bytes, offset) {
    if (bytes.length - offset < frameNeed(14)) return -1;
    decoded.F6_v1 = readU16LE(bytes, offset + 2);
    decoded.F6_v2 = readU16LE(bytes, offset + 4);
    decoded.F6_v3 = readU16LE(bytes, offset + 6);
    decoded.F6_v4 = readU8(bytes, offset + 8);
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 9));
    return 14;
}

// 4.5.1 BD C0 lenU8 messageId[len] CK (4 + len bytes)
function decodeDownlinkFeedback(bytes, offset) {
    if (bytes.length - offset < frameNeed(3)) return -1;
    var len = bytes[offset + 2];
    if (bytes.length - offset < frameNeed(4 + len)) return -1;
    var ids = [];
    for (var i = 0; i < len; i++) ids.push(hex8(bytes[offset + 3 + i]));
    decoded.feedbackcount = len;
    decoded.feedback = ids;
    return 4 + len;
}

// Text message feedback observed on real devices (not in V2.1 chapter 4):
// BD 28 tsU32 typeU8 statusU8 messageIdU32(BE) CK (13 bytes)
function decodeMessageFeedback(bytes, offset) {
    if (bytes.length - offset < frameNeed(13)) return -1;
    decoded.timestamp = dateFormat(readU32LE(bytes, offset + 2));
    decoded.MessageType = hex8(bytes[offset + 6]);
    decoded.MessageStatus = hex8(bytes[offset + 7]);
    decoded.MessageId = readU32BE(bytes, offset + 8).toString(16);
    return 13;
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

function readU32BE(bytes, o) {
    return ((bytes[o] * 0x1000000) + (bytes[o + 1] << 16) + (bytes[o + 2] << 8) + bytes[o + 3]) >>> 0;
}

// IEEE-754 double, little-endian (4.2.1 GPS lon/lat).
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
