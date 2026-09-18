/**
 * Payload Decoder — 多参数空气质量传感器（ChirpStack v4）
 *
 * Copyright 2022 HKT SmartHard
 *
 * @product AQS1000
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_IAQ1000
 *   USER/Drive/communicate.c 的 setDataPackage / Device_SensorDataReport /
 *   Device_PeriodicReport（ref 41ad0d9；IAQ300/600/900 与 AQS1000 共用协议）
 * 帧格式：[0x68 0x6B 0x74][特殊类型(固定0)][seqNo][TLV...]，TLV = 1 字节类型 + 定长值
 * fPort：MCU 透传模组，固件默认 LoRaWAN_DEFAULT_PORT=10
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2022    v1.0.0 初版。
 * 2026-09-18 v1.1.0 全量重写（黄金样例 × 固件逐字节互证）：
 *   1. 修复 byteToUint16/byteToInt32 高位权重 0xFF→0x100（v1.0.0 所有多字节
 *      字段在中/高字节非零时系统性偏小，如 CO2 03 2C→809 而非 812）；
 *   2. 温度按 24 位补码解析（-15.5℃ 帧 FF C3 74→-15500/1000）；
 *   3. 修复 while(dataLen--) 截断死循环（含默认配置周期帧必挂场景），
 *      改为类型定长表 + 越界/未知类型显式报错；
 *   4. 修复 hexToString 每字节前导零丢失（设备 ID 60 05 0A 00 2B 7F 曾解成 9 字符）；
 *   5. 补齐固件真实上行类型：05 传感器周期(2B)/29~2D 显示配置(各1B)/58 校准应答(1B)/
 *      5B 温度偏移应答(1B)/80 时间同步请求(0B)/FF ACK 回执(1B)，按契约静默投影；
 *   6. 删除枚举中不存在的 0x82 Eco 死代码（疑来自旧协议）。
 * ─────────────────────────────────────────────────────────
 */
'use strict';

var TLV_LENGTHS = {
    0x01: 2,  // hard_ver + soft_ver
    0x02: 6,  // 设备 ID（DevEUI 后 6 字节）
    0x03: 1,  // battery（百分比 0~100，仅市电时携带）
    0x05: 2,  // 传感器上报周期（分钟，大端）
    0x09: 3,  // temperature（毫℃，24 位补码）
    0x0A: 3,  // humidity（毫 %RH）
    0x19: 2,  // pressure（hPa 整数，HP203B Pa/100）
    0x1A: 1,  // pir（触发置 1，上报后清 0）
    0x1B: 1,  // light_level（等级）
    0x1C: 2,  // pm2_5（μg/m³）
    0x1D: 2,  // pm10（μg/m³）
    0x1E: 2,  // hcho（ppb，=ppm×1000）
    0x1F: 1,  // o3_level（等级，O3 编译变体）
    0x20: 2,  // co2（ppm）
    0x21: 1,  // tvoc（等级）
    0x29: 1,  // LED 模式
    0x2A: 1,  // 蜂鸣
    0x2B: 1,  // 温度单位
    0x2C: 1,  // EPD 模式
    0x2D: 1,  // 翻转
    0x58: 1,  // CO2 校准应答（FF 成功 / FE 失败）
    0x5B: 1,  // 温度偏移设置应答（回显值，越界为 FE）
    0x80: 0,  // 时间同步请求（setDataPackage 无 case，帧体仅类型字节）
    0x81: 1,  // pwr_way（供电方式）
    0x86: 2,  // 同步周期（分钟，大端）
    0xFF: 1,  // ACK 回执（值固定 0xFF）
};

function Decoder(bytes, port) {

    if (!bytes || bytes.length < 5) {
        return err("frame too short (" + (bytes ? bytes.length : 0) + " bytes)，至少需要 5 字节帧头");
    }
    if (bytes[0] !== 0x68 || bytes[1] !== 0x6B || bytes[2] !== 0x74) {
        return err("invalid frame header");
    }

    var decoded = {};
    var i = 5;
    while (i < bytes.length) {
        var type = bytes[i];
        i++;
        var len = TLV_LENGTHS[type];
        if (len === undefined) {
            return err("unknown TLV type 0x" + hex8(type) + " at offset " + (i - 1) + ", stop parsing");
        }
        if (i + len > bytes.length) {
            return err("truncated TLV type 0x" + hex8(type) + ", need " + len + " byte(s), got " + (bytes.length - i));
        }
        switch (type) {
            case 0x01: // software_ver and hardware_ver
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                break;
            case 0x02: // 设备 ID
                decoded.id = bytesToHex(bytes.slice(i, i + 6));
                break;
            case 0x03: // battery
                decoded.battery = bytes[i];
                break;
            case 0x09: // TEMPERATURE ℃
                decoded.temperature = readInt24(bytes, i) / 1000;
                break;
            case 0x0A: // HUMIDITY
                decoded.humidity = readUInt24(bytes, i) / 1000;
                break;
            case 0x19: // PRESSURE
                decoded.pressure = readUInt16(bytes, i);
                break;
            case 0x1A: // PIR
                decoded.pir = bytes[i];
                break;
            case 0x1B: // LIGHT
                decoded.light_level = bytes[i];
                break;
            case 0x1C: // PM2.5
                decoded.pm2_5 = readUInt16(bytes, i);
                break;
            case 0x1D: // PM10
                decoded.pm10 = readUInt16(bytes, i);
                break;
            case 0x1E: // HCHO
                decoded.hcho = readUInt16(bytes, i) / 1000;
                break;
            case 0x1F: // O3
                decoded.o3_level = bytes[i];
                break;
            case 0x20: // CO2
                decoded.co2 = readUInt16(bytes, i);
                break;
            case 0x21: // TVOC
                decoded.tvoc = bytes[i];
                break;
            case 0x81: // power way
                decoded.pwr_way = bytes[i];
                break;
            case 0x05: // 传感器周期 / 0x29~0x2D 显示配置 / 0x58 / 0x5B / 0x80 / 0x86 / 0xFF：
            case 0x29: // 固件真实上行，但契约约定不投影为业务字段（静默消费）
            case 0x2A:
            case 0x2B:
            case 0x2C:
            case 0x2D:
            case 0x58:
            case 0x5B:
            case 0x80:
            case 0x86:
            case 0xFF:
                break;
        }
        i += len;
    }
    return decoded;
}

function err(msg) {
    return { data: {}, errors: [msg] };
}

function hex8(n) {
    return (n < 16 ? "0" : "") + n.toString(16);
}

function readUInt16(b, i) {
    return ((b[i] << 8) | b[i + 1]) & 0xFFFF;
}

function readUInt24(b, i) {
    return ((b[i] << 16) | (b[i + 1] << 8) | b[i + 2]) >>> 0;
}

/** 24 位补码（固件 int32 补码取低 3 字节，如 -15.5℃ = FF C3 74） */
function readInt24(b, i) {
    var v = readUInt24(b, i);
    return v > 0x7FFFFF ? v - 0x1000000 : v;
}

function bytesToHex(bytes) {
    var s = "";
    for (var i = 0; i < bytes.length; i++) {
        var h = bytes[i].toString(16);
        s += h.length < 2 ? "0" + h : h;
    }
    return s;
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

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { Decoder };
}
