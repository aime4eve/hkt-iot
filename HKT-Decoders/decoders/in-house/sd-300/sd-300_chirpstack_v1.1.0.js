/**
 * Payload Decoder for The Chirpstack v4
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
 *
 * Revision:
 * 2026-09-18 v1.1.0 黄金样例 × 手册互证：
 *   1. 过短帧（<5 字节，缺序号/数据段）返回 error（原静默返回空对象）；
 *   2. 0x85 恢复出厂为下行专属命令（手册 §6.3 "upstream command is invalid"），
 *      上行方向出现即报错（原解出 factory_reset 字段）。
 */

function easy_decode(bytes) {
    var decoded = {};

    if (!bytes || bytes.length < 5) {
        return { error: "payload too short (" + (bytes ? bytes.length : 0) + " bytes)，最少 5 字节（头3+特殊类型1+序号1）" };
    }

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
            case 0x85:// FACTORY_RESET  downlink only——上行方向收到即非法帧
                return { error: "0x85 factory reset is downlink-only, invalid in uplink" };
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

function decodeUplink(input) {
    var decoded = easy_decode(input.bytes);
    return { data: decoded };
}

