/**
 * Payload Decoder — 智能空开（ChirpStack v4）
 *
 * Copyright 2026 HKT SmartHard
 *
 * @product SCB100（智能空开，LoRaWAN_Air_Switch 固件）
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_Air_Switch 与 LoRaWAN_Air_Switch_CL
 *   （两工程上行组包逐字节一致）：USER/Drive/communicate.c + an001.c（ref 41ad0d9）
 * 帧格式：AA | cmd | addr | len | data... | CRC16L | CRC16H | 55
 *   report 类（0x05/0x06/0x07）在 addr 与 len 之间多两字节：model(=version>>8)、itemType(=item&0xFF)
 *   0xEE 透传帧无 addr/len：AA | EE | AS原始数据... | CRC16L | CRC16H | 55
 *   CRC = Modbus CRC-16（init 0xFFFF, poly 0xA001），覆盖帧内除 CRC 与尾 0x55 外全部字节，低字节在前
 * fPort：固件默认 LoRaWAN_DEFAULT_PORT=10（模组侧可改，解码器不校验端口）
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2026-09-17 v1.0.0 迁移占位 —— 实为 DMS01 门磁产品解码器（帧头 68 6B 74 + TLV），
 *            与本产品固件的 AA..55 帧完全不兼容，所有真实帧均被拒绝（误迁移，保留存档）。
 * 2026-09-18 v2.0.0 按固件协议全量重写（黄金样例 × 固件逐字节互证）：
 *   覆盖固件全部 12 种上行 cmd：0x00/01/03/04 ACK（len=0）、0x05 阈值（20×u16）、
 *   0x06/07 实时数据（29×u16 + remoteControl）、0x08 在线掩码、0x09 批量通断掩码、
 *   0x0A 双掩码、0x0B 漏保自检、0xEE 透传；
 *   坏帧头/坏帧尾/长度不符/未知 cmd/CRC 错误一律报错（CRC 语义照抄 an001.c crc_cal_value）。
 * ─────────────────────────────────────────────────────────
 */
'use strict';

function decodeUplink(input) {
    var b = input.bytes;
    if (!b || b.length < 7) {
        return err("frame too short (" + (b ? b.length : 0) + " bytes)，最小帧结构为 AA+cmd+addr+len+CRC16+55");
    }
    if (b[0] !== 0xAA) {
        return err("invalid frame header 0x" + hex8(b[0]) + "（固件帧头恒为 0xAA）");
    }
    if (b[b.length - 1] !== 0x55) {
        return err("invalid frame tail 0x" + hex8(b[b.length - 1]) + "（固件帧尾恒为 0x55）");
    }
    var crc = crc16(b.slice(0, b.length - 3));
    if ((crc & 0xFF) !== b[b.length - 3] || ((crc >> 8) & 0xFF) !== b[b.length - 2]) {
        return err("CRC check failed（Modbus CRC-16 低字节在前）");
    }

    var cmd = b[1];
    if (cmd === 0xEE) {
        // 透传上行：无 addr/len 字节（固件 fromASDataHandle 现状）
        return { data: { cmd: cmd, asDataHex: bytesToHex(b.slice(2, b.length - 3)) } };
    }

    var isReport = (cmd === 0x05 || cmd === 0x06 || cmd === 0x07);
    var addr = b[2];
    var model = null, itemType = null, lenPos;
    if (isReport) {
        model = b[3];           // = version >> 8
        itemType = b[4];        // = item & 0xFF
        lenPos = 5;
    } else {
        lenPos = 3;
    }
    var dataLen = b[lenPos];
    var dataStart = lenPos + 1;
    if (dataStart + dataLen + 3 !== b.length) {
        return err("length mismatch: len 字段=" + dataLen + "，实际数据 " + (b.length - dataStart - 3) + " 字节");
    }
    var d = b.slice(dataStart, dataStart + dataLen);

    switch (cmd) {
        case 0x00: // 设置空开地址应答
        case 0x01: // 设置上报间隔应答
        case 0x03: // 单空开通断应答（addr 回显，0xFF=广播）
        case 0x04: // 恢复出厂应答
            if (dataLen !== 0) return err("cmd 0x" + hex8(cmd) + " 应答帧 len 应为 0");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen } };

        case 0x05: { // 阈值上报（20×u16 大端，原始寄存器值）
            if (dataLen !== 40) return err("cmd 0x05 阈值帧 len 应为 40");
            var th = ["maxVoltage", "minVoltage", "maxLeakage", "maxPower", "maxTemperature", "maxElectricity",
                "maxAElectricity", "maxBElectricity", "maxCElectricity", "maxAPower", "maxBPower", "maxCPower",
                "maxVoltageAlarm", "minVoltageAlarm", "maxLeakageAlarm", "maxTemperatureAlarm",
                "aElectricityAlarm", "bElectricityAlarm", "cElectricityAlarm", "electricityAlarm"];
            var out = { cmd: cmd, addr: addr, model: model, itemType: itemType, dataLen: dataLen };
            for (var i = 0; i < th.length; i++) out[th[i]] = readUInt16(d, i * 2);
            return { data: out };
        }

        case 0x06: // 实时数据-查询应答
        case 0x07: // 实时数据-主动周期上报（组包布局与 0x06 相同）
        {
            if (dataLen !== 59) return err("cmd 0x" + hex8(cmd) + " 实时数据帧 len 应为 59（29×u16+1）");
            var rt = ["totalVoltage", "leakageElectricity", "power", "temperature", "electricity", "alarm",
                "powerLowByte", "powerHighByte", "aPhaseVoltage", "bPhaseVoltage", "cPhaseVoltage",
                "aElectricity", "bElectricity", "cElectricity", "nElectricity",
                "aPhasePower", "bPhasePower", "cPhasePower", "aPhaseAlarm", "bPhaseAlarm", "cPhaseAlarm",
                "bitState", "aPhasePowerFactor", "bPhasePowerFactor", "cPhasePowerFactor",
                "aPhaseTemperature", "bPhaseTemperature", "cPhaseTemperature", "nPhaseTemperature"];
            var out2 = { cmd: cmd, addr: addr, model: model, itemType: itemType, dataLen: dataLen };
            for (var j = 0; j < rt.length; j++) out2[rt[j]] = readUInt16(d, j * 2);
            out2.remoteControl = d[58];
            return { data: out2 };
        }

        case 0x08: // 在线状态掩码（s0→bit15 … s9→bit6，bit5..0 恒 0）
            if (dataLen !== 2) return err("cmd 0x08 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, aliveMask: readUInt16(d, 0) } };

        case 0x09: // 批量通断结果掩码
            if (dataLen !== 2) return err("cmd 0x09 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, openStateMask: readUInt16(d, 0) } };

        case 0x0A: // 分合闸 + 远程使能双掩码
            if (dataLen !== 4) return err("cmd 0x0A 帧 len 应为 4");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, openStateMask: readUInt16(d, 0), remoteControlMask: readUInt16(d, 2) } };

        case 0x0B: // 漏保自检结果（u16 承载 0/1）
            if (dataLen !== 2) return err("cmd 0x0B 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, leakageFlag: readUInt16(d, 0) } };

        default:
            return err("unknown cmd 0x" + hex8(cmd) + " at offset 1, stop parsing");
    }
}

function err(msg) {
    return { data: {}, errors: [msg] };
}

function hex8(n) {
    return (n < 16 ? "0" : "") + n.toString(16);
}

/** u16 大端（固件 an001.c 组包：高字节在前，无缩放） */
function readUInt16(d, i) {
    return ((d[i] << 8) | d[i + 1]) & 0xFFFF;
}

/** Modbus CRC-16（an001.c crc_cal_value：init 0xFFFF，poly 0xA001，低字节在前存放） */
function crc16(arr) {
    var crc = 0xFFFF;
    for (var i = 0; i < arr.length; i++) {
        crc ^= arr[i];
        for (var j = 0; j < 8; j++) {
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
        }
    }
    return crc & 0xFFFF;
}

function bytesToHex(bytes) {
    var s = "";
    for (var i = 0; i < bytes.length; i++) {
        var h = bytes[i].toString(16);
        s += h.length < 2 ? "0" + h : h;
    }
    return s;
}
