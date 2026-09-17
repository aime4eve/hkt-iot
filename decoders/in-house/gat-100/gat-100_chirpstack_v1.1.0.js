/**
 * Payload Decoder — GPS 动物追踪器（ChirpStack v4）
 *
 * Copyright 2023 HKT SmartHard
 *
 * @product GAT-100（ChirpStack）
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_Animals_Track
 *   USER/Drive/communicate.c 的 setDataPackage / Device_PeriodicReport（ref 41ad0d9）
 * 帧格式：[0x68 0x6B 0x74][flags][seqNo][TLV...]，TLV = 1 字节类型 + 定长值（表见 TLV_LENGTHS）
 * fPort：固件默认 LoRaWAN_DEFAULT_PORT=10（模组侧可改，解码器不校验端口）
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2023    v1.0.0 初版（easy_decode）。
 * 2026-09-18 v1.1.0 防御性重写（黄金样例 × 固件逐字节互证）：
 *   1. 坏帧头/过短帧/TLV 截断/未知类型一律返回 error（原版静默返回空对象、
 *      用残字节拼值或把数据字节误当类型号继续解析）；
 *   2. 补齐固件会发但缺失的 TLV：0x32 主动同步(1B)、0x80 时间同步回执
 *      (0B，固件 setDataPackage 无该分支只发类型字节的 quirk)、0xFF ACK 回执(1B)；
 *   3. 0x39 运行配置 11 字段与固件大端序逐一对应（原 readUInt16LE 实为大端，已正名）；
 *   4. 负经纬度符号判断改用 >>>0 后判 bit31（原对 JS 带符号数比较 0x7FFFFFFF 永假）。
 * ─────────────────────────────────────────────────────────
 */
var TLV_LENGTHS = {
    0x01: 2,  // hard_ver + soft_ver
    0x03: 1,  // battery（百分比 0~100）
    0x09: 3,  // temperature（毫℃，符号位 bit23）
    0x0A: 3,  // humidity（毫 %RH）
    0x0B: 2,  // X 轴加速度原始计数（int16 大端，±2g 满量程 16384 计数/g）
    0x0C: 2,  // Y 轴加速度原始计数
    0x0D: 2,  // Z 轴加速度原始计数
    0x10: 4,  // latitude（度×1e6 大端，负值=绝对值|bit31）
    0x11: 4,  // longitude
    0x15: 2,  // step（u16 大端）
    0x32: 1,  // 主动同步数据（固件固定写 0，无业务字段）
    0x39: 15, // 运行配置：mode + interval1 + 时段1(4B) + interval2 + 时段2(4B) + idle
    0x80: 0,  // 时间同步回执（固件 quirk：只发类型字节本身）
    0x84: 1,  // 防拆告警
    0xFF: 1,  // ACK 回执（固件固定写 0xFF）
};

function decodeUplink(input) {
    var bytes = input.bytes;
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
            case 0x03: // battery
                decoded.battery = bytes[i];
                break;
            case 0x09: // temperature
                decoded.temperature = readInt24SignMag(bytes, i) / 1000;
                break;
            case 0x0A: // humidity
                decoded.humidity = readUInt24(bytes, i) / 1000;
                break;
            case 0x0B: // X-axis acceleration → g
                decoded.x_acc = readInt16(bytes, i) / 16384;
                break;
            case 0x0C: // Y-axis acceleration → g
                decoded.y_acc = readInt16(bytes, i) / 16384;
                break;
            case 0x0D: // Z-axis acceleration → g
                decoded.z_acc = readInt16(bytes, i) / 16384;
                break;
            case 0x10: // GPS latitude
                decoded.latitude = readCoord(bytes, i);
                break;
            case 0x11: // GPS longitude
                decoded.longitude = readCoord(bytes, i);
                break;
            case 0x15: // step
                decoded.step = readUInt16(bytes, i);
                break;
            case 0x32: // 主动同步（固定 0，无业务字段）
            case 0x80: // 时间同步回执（无值）
                break;
            case 0x39: // work mode and report interval
                decoded.work_mode = bytes[i];
                decoded.report_interval_time1 = readUInt16(bytes, i + 1);
                decoded.start_hour_time1 = bytes[i + 3];
                decoded.start_min_time1 = bytes[i + 4];
                decoded.end_hour_time1 = bytes[i + 5];
                decoded.end_min_time1 = bytes[i + 6];
                decoded.report_interval_time2 = readUInt16(bytes, i + 7);
                decoded.start_hour_time2 = bytes[i + 9];
                decoded.start_min_time2 = bytes[i + 10];
                decoded.end_hour_time2 = bytes[i + 11];
                decoded.end_min_time2 = bytes[i + 12];
                decoded.idle_interval = readUInt16(bytes, i + 13);
                break;
            case 0x84: // tamper alarm
                decoded.tamper_alarm = bytes[i];
                break;
            case 0xFF: // ACK 回执
                decoded.ackResponse = bytes[i];
                break;
        }
        i += len;
    }
    return { data: decoded };
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

function readInt16(b, i) {
    var v = readUInt16(b, i);
    return v > 0x7FFF ? v - 0x10000 : v;
}

function readUInt24(b, i) {
    return ((b[i] << 16) | (b[i + 1] << 8) | b[i + 2]) >>> 0;
}

function readInt24SignMag(b, i) {
    var v = readUInt24(b, i);
    return v > 0x7FFFFF ? -(v & 0x7FFFFF) : v;
}

/** 度×1e6 大端，bit31=1 表示负（固件 gps.c：(u32)(fabs(度)*1e6)|0x80000000） */
function readCoord(b, i) {
    var v = ((b[i] << 24) | (b[i + 1] << 16) | (b[i + 2] << 8) | b[i + 3]) >>> 0;
    return (v & 0x80000000) ? -((v & 0x7FFFFFFF) / 1000000) : v / 1000000;
}
