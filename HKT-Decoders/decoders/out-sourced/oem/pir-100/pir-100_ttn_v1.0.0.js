/**
 * Payload Decoder for The Things Network
 * 
 * Copyright 2025 HKT SmartHard
 * 
 * @product PIR-100
 */

function easy_decode(bytes) {
    var decoded = {};

    var len = bytes.length;

    if (len == 5) {
        if (bytes[1] === 0x01) {
            decoded.reportType = "Heartbeat Report";
            decoded.sensorType = bytes[2];
            decoded.frameType = bytes[3];
            decoded.infraredStatus = bytes[4] === 0 ? "Not Triggered" : "Triggered";
            decoded.antiTamperSwitch = bytes[5] === 0 ? "Not Removed" : "Removed";
            decoded.batteryVoltage = bytes[6] / 10;
        } else if (bytes[1] === 0x02) {
            decoded.reportType = "Event Report";
            decoded.sensorType = bytes[2];
            decoded.frameType = bytes[3];
            decoded.infraredStatus = bytes[4] === 0 ? "Not Triggered" : "Triggered";
            decoded.antiTamperSwitch = bytes[5] === 0 ? "Not Removed" : "Removed";
            decoded.batteryVoltage = bytes[6] / 10;
        }
    }
    return decoded;
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
