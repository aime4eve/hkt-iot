/**
 * Payload Decoder for The Things Network
 * 
 * Copyright 2023 HKT SmartHard
 * 
 * @product HKT-SD-300
 * 
 * Frame structure (all HEX):
 *   Sync header (3B: 0x68 0x6B 0x74) | special type (1B) | serial number (1B) | [data type (1B) + data (nB)]...
 * 
 * Data types:
 *   0x01  version            2 bytes: hardware version + software version (uplink)
 *   0x09  temperature        3 bytes: big-endian, unit Celsius x1000, highest bit 1 = negative (sign-magnitude)
 *   0x27  smoke status       1 byte : 0 = alarm recovery, 1 = smoke alarm triggered (uplink)
 *   0x28  temperature alarm  1 byte : 0 = alarm recovery, 1 = high temperature alarm triggered (uplink)
 *   0x80  sync system time   uplink: request, no data; downlink: YY MM DD hh mm ss
 *   0x85  factory reset      1 byte : 1 = restore factory settings (downlink only)
 *   0x86  report interval    2 bytes: big-endian, unit minutes, 10-1440, 0 = disable (uplink & downlink)
 *   0x89  battery status     1 byte : 0 = undervoltage alarm, 1 = battery normal (uplink)
 */

function easy_decode(bytes) {
    var decoded = {};

    if (checkReportSync(bytes) == false)
        return { error: "invalid sync header" };

    var temp;
    var dataLen = bytes.length - 5;
    var i = 5;
    while (dataLen > 0) {
        var type = bytes[i];
        i++;
        dataLen--;
        switch (type) {
            case 0x01:  //software_ver and hardware_ver
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                dataLen -= 2;
                i += 2;
                break;
            case 0x09:// TEMPERATURE
                // 3 bytes big-endian, unit 0.001 Celsius
                // negative value: highest bit is 1 (sign-magnitude, NOT two's complement)
                temp = (bytes[i] << 16) | (bytes[i + 1] << 8) | bytes[i + 2];
                if (temp & 0x800000)
                    temp = -(temp & 0x7FFFFF);
                // ℃
                decoded.temperature = temp / 1000;

                // ℉
                // decoded.temperature = temp / 1000 * 1.8 + 32;

                dataLen -= 3;
                i += 3;
                break;
            case 0x27:// SMOKE_WARNING  0 = alarm recovery, 1 = smoke alarm
                decoded.smoke_warning = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
            case 0x28:// TEMPERATURE_WARNING  0 = alarm recovery, 1 = high temperature alarm
                decoded.temperature_warning = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
            case 0x80:// SYNC_SYSTEM_TIME  uplink request, no data
                decoded.time_sync_request = 1;
                break;
            case 0x85:// FACTORY_RESET  downlink only, 1 byte
                decoded.factory_reset = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
            case 0x86:// REPORT_INTERVAL  2 bytes big-endian, unit: minutes
                decoded.report_interval = (bytes[i] << 8) | bytes[i + 1];
                dataLen -= 2;
                i += 2;
                break;
            case 0x89:// BATTERY_STATUS  0 = undervoltage alarm, 1 = battery normal
                decoded.battery = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
            default:// unknown data type, stop parsing to avoid misalignment
                return decoded;
        }
    }
    return decoded;
}

function byteToUint16(bytes) {
    var value = (bytes[0] << 8) | bytes[1];
    return value;
}

function byteToInt16(bytes) {
    var value = (bytes[0] << 8) | bytes[1];
    return value > 0x7fff ? value - 0x10000 : value;
}

function byteToInt32(bytes) {
    var value = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
    return value > 0x7fffff ? value - 0x1000000 : value;
}

function hexToString(bytes) {
    var value = "";
    for (var i = 0; i < bytes.length; i++) {
        value += ("0" + bytes[i].toString(16)).slice(-2);
    }
    return value;
}

function checkReportSync(bytes) {
    if (bytes[0] == 0x68 && bytes[1] == 0x6B && bytes[2] == 0x74) {
        return true;
    }
    return false;
}

function Decoder(bytes, port) {
    return easy_decode(bytes);
}


/* ==== HKT-Decoders 平台适配层（2026-09-17 规范化迁移追加） ====
 * 统一入口 decodeUplink(input)：包装历史 Decoder(bytes, port) 入口。
 */
function decodeUplink(input) {
    var __bytes = input.bytes;
    var __port = (input.fPort === undefined || input.fPort === null) ? 0 : input.fPort;
    var __r;
    try {
        __r = Decoder(__bytes, __port);
    } catch (e) {
        return { data: {}, errors: ["decoder threw: " + (e && e.message ? e.message : String(e))] };
    }
    if (__r === undefined || __r === null) return { data: {}, errors: ["decoder returned nothing"] };
    if (typeof __r === "object" && !Array.isArray(__r) && (__r.data !== undefined || __r.errors !== undefined)) return __r;
    return { data: __r };
}
