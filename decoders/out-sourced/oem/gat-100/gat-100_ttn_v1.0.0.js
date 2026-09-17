/**
 * Payload Decoder for The Things Network
 * 
 * Copyright 2023 HKT SmartHard
 * 
 * @product GAT-100
 */

function easy_decode(bytes) {
    var decoded = {};

    if (checkReportSync(bytes) == false)
        return;

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
            case 0x03:// BATTERY
                decoded.battery = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
            case 0x09:// tamperature
                var value = bytes[i] << 16 | bytes[i + 1] << 8 | bytes[i + 2];
                if (value > 0x7FFFFF)
                    value = -(value & 0x7FFFFF);
                decoded.temperature = value / 1000;
                dataLen -= 3;
                i += 3;
                break;
            case 0x0A:// humidity
                var value = bytes[i] << 16 | bytes[i + 1] << 8 | bytes[i + 2];
                decoded.humidity = value / 1000;
                dataLen -= 3;
                i += 3;
                break;
            case 0x0B:  // X-axis acceleration
                var x_acc = bytes[i] << 8 | bytes[i + 1];
                if (x_acc > 0x7FFF) x_acc = -(0x10000 - x_acc);
                decoded.x_acc = x_acc;
                dataLen -= 2;
                i += 2;
                break;
            case 0x0C:  // Y-axis acceleration
                var y_acc = bytes[i] << 8 | bytes[i + 1];
                if (y_acc > 0x7FFF) y_acc = -(0x10000 - y_acc);
                decoded.y_acc = y_acc;
                dataLen -= 2;
                i += 2;
                break;
            case 0x0D:  // Z-axis acceleration
                var z_acc = bytes[i] << 8 | bytes[i + 1];
                if (z_acc > 0x7FFF) z_acc = -(0x10000 - z_acc);
                decoded.z_acc = z_acc;
                dataLen -= 2;
                i += 2;
                break;
            case 0x10:// GPS latitude
                var ref = (bytes[i] << 24) | (bytes[i + 1] << 16) | (bytes[i + 2] << 8) | bytes[i + 3];
                if (ref & 0x80000000) {
                    ref = -(ref & 0x7FFFFFFF);
                }
                decoded.latitude = ref / 1000000;
                dataLen -= 4;
                i += 4;
                break;
            case 0x11:// GPS longitude
                var ref = (bytes[i] << 24) | (bytes[i + 1] << 16) | (bytes[i + 2] << 8) | bytes[i + 3];
                if (ref & 0x80000000) {
                    ref = -(ref & 0x7FFFFFFF);
                }
                decoded.longitude = ref / 1000000;
                dataLen -= 4;
                i += 4;
                break;
            case 0x15:// step
                decoded.step = bytes[i] << 8 | bytes[i + 1];
                dataLen -= 2;
                i += 2;
                break;
            case 0x39:// work mode and report interval
                decoded.work_mode = bytes[i];
                i += 1;
                decoded.report_interval_time1 = readUInt16LE(bytes.slice(i, i + 2));
                i += 2;
                decoded.start_hour_time1 = bytes[i++];
                decoded.start_min_time1 = bytes[i++];
                decoded.end_hour_time1 = bytes[i++];
                decoded.end_min_time1 = bytes[i++];

                decoded.report_interval_time2 = readUInt16LE(bytes.slice(i, i + 2));
                i += 2;
                decoded.start_hour_time2 = bytes[i++];
                decoded.start_min_time2 = bytes[i++];
                decoded.end_hour_time2 = bytes[i++];
                decoded.end_min_time2 = bytes[i++];

                decoded.idle_interval = readUInt16LE(bytes.slice(i, i + 2));
                dataLen -= 15;
                i += 2;
                break;
            case 0x84:// tamper alarm
                decoded.tamper_alarm = bytes[i];
                dataLen -= 1;
                i += 1;
                break;
        }
    }
    return decoded;
}


function hexToString(bytes) {
    var value = "";
    var arr = bytes.toString(16).split(",");
    for (var i = 0; i < arr.length; i++) {
        value += parseInt(arr[i]).toString(16);
    }
    return value;
}

function checkReportSync(bytes) {
    if (bytes[0] == 0x68 && bytes[1] == 0x6B && bytes[2] == 0x74) {
        return true;
    }
    return false;
}

function readUInt16LE(bytes) {
    var value = (bytes[0] << 8) + bytes[1];
    return (value & 0xFFFF);
}

function readInt16LE(bytes) {
    var ref = readUInt16LE(bytes);
    return (ref > 0x7FFF) ? ref - 0x10000 : ref;
}

function readUInt32LE(bytes) {
    var value = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return (value & 0xFFFFFFFF);
}

function readInt32LE(bytes) {
    var ref = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return (ref > 0x7FFFFFFF) ? ref - 0x100000000 : ref;
}

function readUInt8LE_SWP8(bytes) {
    return (value & 0xFF);
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
