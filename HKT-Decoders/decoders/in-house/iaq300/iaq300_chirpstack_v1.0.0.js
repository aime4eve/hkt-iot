/**
 * Payload Decoder — 三合一室内环境监测传感器（ChirpStack v4）
 *
 * Copyright 2025 HKT SmartHard
 *
 * @product IAQ300
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_IAQ300
 *   USER/Drive/communicate.c 的 setDataPackage（上行组帧）/ fromLoRaWANDataHandle（下行解析）
 *   USER/Drive/include/communicate.h 的 enum DataType（ref 89d58fd，2026-10-26 逐字节核对）
 *   USER/Drive/control_center.h struct sensor / Compents/Sensor/Sensirion/SHT40X.c（温度湿度倍率）
 * 帧格式：[0x68 0x6B 0x74][特殊类型(BIT0=需ACK)][seqNo][TLV...]，TLV = 1 字节类型 + 定长值
 * fPort：固件默认 LoRaWAN 端口 10（解码器不校验端口）
 *
 * ── 与《用户手册 V1.2》的差异（以固件为准，2026-10-26 核对）──
 *   1. 温度/湿度 = 放大 1000 倍的 int，上行取补码低 3 字节（大端）。
 *      手册 §6.3 负温示例"80 62 8E"（符号位+幅值法）与固件组帧 (int>>16) 不符：
 *      -25.23℃ 固件实际发送 FF 9D 72（24 位补码）。
 *   2. 0x81 供电方式：固件 gpio.c readDCPowerInputStatus 生效分支为
 *      检测到 DC 插入 → powerIn=1。即 1=外部供电、0=电池；手册 §6.3 写反。
 *   3. 手册未列 0x05 传感器上报周期(2B) 与 0x86 设备同步周期(2B)（上行回读真实存在）；
 *      且两类型的下行配置分支在固件中已注释禁用——运行周期配置唯一通道是
 *      0x5C 传感器任务（1~1440 分钟）与 0x5D PIR 任务（1~60 分钟 + 状态同步间隔）。
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2026-10-26 v1.0.0 新建条目（非迁移件）：按 LoRaWAN_IAQ300 固件源码全量编写并
 *   核验；TLV 定长表 + 越界/未知类型显式报错；温度 24 位补码；投影 0x5C/0x5D
 *   配置回读与 0x58/0x5B 命令应答（供应商 Bench 的命令确认回报测试需要读这些字段）。
 * ─────────────────────────────────────────────────────────
 */
'use strict';

var TLV_LENGTHS = {
    0x01: 2,  // hard_ver + soft_ver（setDataPackage: HARDWARE_VER, SOFTWARE_VER）
    0x02: 6,  // 设备 ID（DevEUI[2..7]，大端拷贝）
    0x03: 1,  // battery（百分比 0~100）
    0x05: 2,  // 传感器上报周期（分钟，大端；下行配置分支已禁用，仅上行回读）
    0x09: 3,  // temperature（毫℃，24 位补码，int 放大 1000 倍取低 3 字节）
    0x0A: 3,  // humidity（毫 %RH，物理量非负）
    0x19: 2,  // pressure（大端；IAQ300 三合一硬件通常不含气压计，字段保留）
    0x1A: 1,  // pir（触发置 1，上报后固件清 0）
    0x1B: 1,  // light_level（等级）
    0x1C: 2,  // pm2_5（μg/m³）
    0x1D: 2,  // pm10（μg/m³）
    0x1E: 2,  // hcho（ppb，=ppm×1000）
    0x1F: 1,  // o3_level（等级）
    0x20: 2,  // co2（ppm，CM1106SL NDIR）
    0x21: 1,  // tvoc（等级）
    0x29: 1,  // LED 显示模式（0=闪烁 10s 间隔，1=常闭）
    0x2A: 1,  // 蜂鸣器启用（0=禁用，1=启用）
    0x2B: 1,  // 温度单位（0=℃，1=℉）
    0x2C: 1,  // 电子纸显示模式
    0x2D: 1,  // 电子纸翻转显示
    0x58: 1,  // CO2 强制校准应答（仅非法值 400~2000 之外时回 0xFE；合法值执行校准无回包）
    0x5B: 1,  // 温度偏移设置应答（有符号字节回显，单位 0.1℃；越界回 0xFE）
    0x5C: 8,  // 传感器任务配置回读：start_hh start_mm end_hh end_mm + u16 段内间隔 + u16 段外间隔
    0x5D: 3,  // PIR 任务配置回读：pir 间隔(1B) + 状态同步间隔(u16)
    0x80: 0,  // 时间同步请求（上行仅类型字节，体长 0）
    0x81: 1,  // pwr_way（1=外部供电，0=电池；手册 V1.2 写反，以固件为准）
    0x86: 2,  // 设备同步周期（分钟，大端 10~1440 默认 480；下行配置分支已禁用）
    0xFF: 1,  // ACK 回执（值固定 0xFF，对下行 BIT0=1 的整帧应答）
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
            case 0x01:
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                break;
            case 0x02:
                decoded.id = bytesToHex(bytes.slice(i, i + 6));
                break;
            case 0x03:
                decoded.battery = bytes[i];
                break;
            case 0x05:
                decoded.sensor_interval_min = readUInt16(bytes, i);
                break;
            case 0x09:
                decoded.temperature = readInt24(bytes, i) / 1000;
                break;
            case 0x0A:
                decoded.humidity = readUInt24(bytes, i) / 1000;
                break;
            case 0x19:
                decoded.pressure = readUInt16(bytes, i);
                break;
            case 0x1A:
                decoded.pir = bytes[i];
                break;
            case 0x1B:
                decoded.light_level = bytes[i];
                break;
            case 0x1C:
                decoded.pm2_5 = readUInt16(bytes, i);
                break;
            case 0x1D:
                decoded.pm10 = readUInt16(bytes, i);
                break;
            case 0x1E:
                decoded.hcho = readUInt16(bytes, i) / 1000;
                break;
            case 0x1F:
                decoded.o3_level = bytes[i];
                break;
            case 0x20:
                decoded.co2 = readUInt16(bytes, i);
                break;
            case 0x21:
                decoded.tvoc = bytes[i];
                break;
            case 0x29:
                decoded.led_show_mode = bytes[i];
                break;
            case 0x2A:
                decoded.beep_enable = bytes[i];
                break;
            case 0x2B:
                decoded.temp_unit = bytes[i];
                break;
            case 0x2C:
                decoded.epd_show_mode = bytes[i];
                break;
            case 0x2D:
                decoded.epd_overturn = bytes[i];
                break;
            case 0x58:
                decoded.co2_calib_ack = bytes[i]; // 0xFE=失败；合法校准无回包，此类型上行仅失败应答
                break;
            case 0x5B:
                decoded.temp_offset_ack = bytes[i] === 0xFE ? -1 : readInt8(bytes, i); // 单位 0.1℃；-1 仅表示 0xFE 失败码
                break;
            case 0x5C:
                decoded.task_start_hh = bytes[i];
                decoded.task_start_mm = bytes[i + 1];
                decoded.task_end_hh = bytes[i + 2];
                decoded.task_end_mm = bytes[i + 3];
                decoded.task_interval_min = readUInt16(bytes, i + 4);
                decoded.sensor_interval_min = readUInt16(bytes, i + 6);
                break;
            case 0x5D:
                decoded.pir_interval_min = bytes[i];
                decoded.sync_interval_min = readUInt16(bytes, i + 1);
                break;
            case 0x80:
                decoded.time_sync_request = true;
                break;
            case 0x81:
                decoded.pwr_way = bytes[i]; // 1=外部供电 0=电池（固件口径）
                break;
            case 0x86:
                decoded.sync_interval_min = readUInt16(bytes, i);
                break;
            case 0xFF:
                decoded.ack = bytes[i];
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

function readUInt24(b, i) {
    return ((b[i] << 16) | (b[i + 1] << 8) | b[i + 2]) >>> 0;
}

/** 24 位补码（固件 int 温度取低 3 字节，如 -25.23℃ = FF 9D 72） */
function readInt24(b, i) {
    var v = readUInt24(b, i);
    return v > 0x7FFFFF ? v - 0x1000000 : v;
}

function readInt8(b, i) {
    return b[i] > 0x7F ? b[i] - 0x100 : b[i];
}

function bytesToHex(bytes) {
    var s = "";
    for (var i = 0; i < bytes.length; i++) {
        var h = bytes[i].toString(16);
        s += h.length < 2 ? "0" + h : h;
    }
    return s;
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { decodeUplink };
}
