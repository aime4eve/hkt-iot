/**
 * Payload Decoder — 智能空开（ChirpStack v4）
 *
 * Copyright 2026 HKT SmartHard
 *
 * @product SCB100（智能空开，LoRaWAN_Air_Switch 固件）
 *
 * 支持两类上行载荷：
 * 1. HKT 固件帧：AA | cmd | ... | CRC16 | 55
 *    - 自研上报/应答帧按 v2.0.0 语义解码；
 *    - 0xEE 透传帧先剥掉 HKT 封包，再按曼顿 485 协议 V1.5 解码。
 * 2. 直接 Modbus RTU 响应帧：ADDR | CMD | DATA... | CRC16L | CRC16H
 *    - 覆盖 V1.5 的 0x01/02/03/04/05/06/0F/10 应答和标准异常帧。
 *
 * CRC-16 均为 Modbus 低前格式（init 0xFFFF，poly 0xA001）。
 */
'use strict';

function decodeUplink(input) {
    var b = input.bytes;
    if (!b || !b.length) return err("empty payload");

    if (b[0] === 0xAA) {
        return decodeHktFrame(b);
    }
    return decodeModbusFrame(b, "modbus-rtu");
}

/** Decode HKT firmware framing. AA..55 frames carry either internal reports or AS Modbus data. */
function decodeHktFrame(b) {
    if (b.length < 7) {
        return err("frame too short (" + b.length + " bytes)，最小帧结构为 AA+cmd+addr+len+CRC16+55");
    }
    if (b[b.length - 1] !== 0x55) {
        return err("invalid frame tail 0x" + hex8(b[b.length - 1]) + "（固件帧尾恒为 0x55）");
    }
    var crc = crc16(b.slice(0, b.length - 3));
    if ((crc & 0xFF) !== b[b.length - 3] || ((crc >> 8) & 0xFF) !== b[b.length - 2]) {
        return err("HKT frame CRC check failed（Modbus CRC-16 低字节在前）");
    }

    var cmd = b[1];
    if (cmd === 0xEE) {
        var asData = b.slice(2, b.length - 3);
        if (!asData.length) return err("cmd 0xEE passthrough payload is empty");
        return decodeModbusFrame(asData, "hkt-0xee");
    }

    var isReport = (cmd === 0x05 || cmd === 0x06 || cmd === 0x07);
    var addr = b[2];
    var model = null, itemType = null, lenPos;
    if (isReport) {
        model = b[3];
        itemType = b[4];
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
        case 0x00:
        case 0x01:
        case 0x03:
        case 0x04:
            if (dataLen !== 0) return err("cmd 0x" + hex8(cmd) + " 应答帧 len 应为 0");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen } };

        case 0x05: {
            if (dataLen !== 40) return err("cmd 0x05 阈值帧 len 应为 40");
            var th = ["maxVoltage", "minVoltage", "maxLeakage", "maxPower", "maxTemperature", "maxElectricity",
                "maxAElectricity", "maxBElectricity", "maxCElectricity", "maxAPower", "maxBPower", "maxCPower",
                "maxVoltageAlarm", "minVoltageAlarm", "maxLeakageAlarm", "maxTemperatureAlarm",
                "aElectricityAlarm", "bElectricityAlarm", "cElectricityAlarm", "electricityAlarm"];
            var out = { cmd: cmd, addr: addr, model: model, itemType: itemType, dataLen: dataLen };
            for (var i = 0; i < th.length; i++) out[th[i]] = readUInt16BE(d, i * 2);
            return { data: out };
        }

        case 0x06:
        case 0x07: {
            if (dataLen !== 59) return err("cmd 0x" + hex8(cmd) + " 实时数据帧 len 应为 59（29×u16+1）");
            var rt = ["totalVoltage", "leakageElectricity", "power", "temperature", "electricity", "alarm",
                "powerLowByte", "powerHighByte", "aPhaseVoltage", "bPhaseVoltage", "cPhaseVoltage",
                "aElectricity", "bElectricity", "cElectricity", "nElectricity",
                "aPhasePower", "bPhasePower", "cPhasePower", "aPhaseAlarm", "bPhaseAlarm", "cPhaseAlarm",
                "bitState", "aPhasePowerFactor", "bPhasePowerFactor", "cPhasePowerFactor",
                "aPhaseTemperature", "bPhaseTemperature", "cPhaseTemperature", "nPhaseTemperature"];
            var out2 = { cmd: cmd, addr: addr, model: model, itemType: itemType, dataLen: dataLen };
            for (var j = 0; j < rt.length; j++) out2[rt[j]] = readUInt16BE(d, j * 2);
            out2.remoteControl = d[58];
            return { data: out2 };
        }

        case 0x08:
            if (dataLen !== 2) return err("cmd 0x08 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, aliveMask: readUInt16BE(d, 0) } };

        case 0x09:
            if (dataLen !== 2) return err("cmd 0x09 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, openStateMask: readUInt16BE(d, 0) } };

        case 0x0A:
            if (dataLen !== 4) return err("cmd 0x0A 帧 len 应为 4");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, openStateMask: readUInt16BE(d, 0), remoteControlMask: readUInt16BE(d, 2) } };

        case 0x0B:
            if (dataLen !== 2) return err("cmd 0x0B 帧 len 应为 2");
            return { data: { cmd: cmd, addr: addr, dataLen: dataLen, leakageFlag: readUInt16BE(d, 0) } };

        default:
            return err("unknown HKT cmd 0x" + hex8(cmd) + " at offset 1, stop parsing");
    }
}

/** Decode a Modbus RTU response defined by the 485 protocol V1.5. */
function decodeModbusFrame(m, transport) {
    if (m.length < 5) return err("Modbus frame too short (" + m.length + " bytes)");
    var crc = crc16(m.slice(0, m.length - 2));
    if ((crc & 0xFF) !== m[m.length - 2] || ((crc >> 8) & 0xFF) !== m[m.length - 1]) {
        return err("Modbus CRC check failed（CRC-16 低字节在前）");
    }
    var addr = m[0];
    if (addr < 1 || addr > 247) return err("invalid Modbus slave address " + addr + "（协议允许 1-247）");

    var func = m[1];
    var base = func & 0x7F;
    if (func & 0x80) {
        if (m.length !== 5) return err("Modbus 异常帧长度应为 5 字节，实际 " + m.length);
        return { data: {
            transport: transport,
            modbusAddr: addr,
            functionCode: base,
            isError: true,
            exceptionCode: m[2],
            exception: modbusException(m[2])
        } };
    }

    var data = {
        transport: transport,
        modbusAddr: addr,
        functionCode: func,
        isError: false
    };

    switch (func) {
        case 0x01:
        case 0x02:
            return decodeBitResponse(m, data, func);

        case 0x03:
        case 0x04:
            return decodeRegisterResponse(m, data, func);

        case 0x05: {
            if (m.length !== 8) return err("cmd 0x05 应答帧长度应为 8 字节，实际 " + m.length);
            data.requestAddress = readUInt16BE(m, 2);
            data.switchAddress = m[3] + 1;
            data.rawValue = readUInt16BE(m, 4);
            if (data.rawValue === 0xFF00) {
                data.action = "close";
                data.actionCode = 1;
            } else if (data.rawValue === 0x0000) {
                data.action = "open";
                data.actionCode = 0;
            } else {
                data.action = "unknown";
            }
            return { data: data };
        }

        case 0x06: {
            if (m.length !== 8) return err("cmd 0x06 应答帧长度应为 8 字节，实际 " + m.length);
            data.parameterAddress = m[2];
            data.switchRequestAddress = m[3];
            data.switchAddress = m[3] === 0xF0 ? 0xF0 : m[3] + 1;
            data.rawValue = readUInt16BE(m, 4);
            data.target = m[3] === 0xF0 ? "485-module" : "air-switch";
            var meta = parameterRegister(m[2], data.rawValue, m[3] === 0xF0);
            if (meta) data.parameter = meta;
            return { data: data };
        }

        case 0x0F: {
            if (m.length !== 8) return err("cmd 0x0F 应答帧长度应为 8 字节，实际 " + m.length);
            data.switchAddressStart = m[3] + 1;
            data.switchCount = readUInt16BE(m, 4);
            return { data: data };
        }

        case 0x10: {
            if (m.length !== 8) return err("cmd 0x10 应答帧长度应为 8 字节，实际 " + m.length);
            data.parameterAddress = m[2];
            data.switchRequestAddress = m[3];
            data.switchAddress = m[3] === 0xF0 ? 0xF0 : m[3] + 1;
            data.wordCount = readUInt16BE(m, 4);
            return { data: data };
        }

        default:
            return err("unknown Modbus function code 0x" + hex8(func) + "（V1.5 支持 01/02/03/04/05/06/0F/10）");
    }
}

function decodeBitResponse(m, data, func) {
    var byteCount = m[2];
    if (byteCount < 1 || byteCount > 255) return err("cmd 0x01/0x02 byte count must be 1-255");
    if (m.length !== byteCount + 5) {
        return err("cmd 0x" + hex8(func) + " 长度不符：byteCount=" + byteCount + "，期望帧长 " + (byteCount + 5));
    }

    data.byteCount = byteCount;
    data.bitCount = byteCount * 8;
    data.states = [];
    var maskHex = "";
    for (var i = 0; i < byteCount; i++) {
        var value = m[3 + i];
        maskHex += hex8(value);
        for (var bit = 0; bit < 8; bit++) {
            data.states.push((value >> bit) & 1);
        }
    }
    data.stateMaskHex = maskHex;
    data.stateMeaning = func === 0x01 ? "1=合闸，0=分闸" : "1=不能远控，0=允许远控";
    return { data: data };
}

function decodeRegisterResponse(m, data, func) {
    var normalCount = m[2];
    var normalExact = normalCount > 0 && normalCount % 2 === 0 && m.length === normalCount + 5;
    var remoteCount = m.length >= 7 ? m[4] : 0;
    var remoteExact = remoteCount > 0 && remoteCount % 2 === 0 && m.length === remoteCount + 7;

    if (normalExact) {
        data.remoteMode = false;
        data.byteCount = normalCount;
        data.values = readWords(m, 3, normalCount / 2);
        data.note = "标准响应不含请求地址，无法安全映射寄存器名称";
        return { data: data };
    }
    if (remoteExact) {
        data.remoteMode = true;
        data.parameterAddress = m[2];
        data.switchRequestAddress = m[3];
        data.switchAddress = m[3] === 0xF0 ? 0xF0 : m[3] + 1;
        data.byteCount = m[4];
        data.values = readWords(m, 5, m[4] / 2);

        if (m[3] === 0xF0) {
            data.target = "485-module";
            data.registers = namedRegisters(data.parameterAddress, data.values, true, func === 0x03);
        } else {
            data.target = "air-switch";
            data.registers = namedRegisters(data.parameterAddress, data.values, false, func === 0x03);
        }
        addEnergy(data);
        return { data: data };
    }
    return err("cmd 0x" + hex8(func) + " 长度既不符合标准响应，也不符合远程模式响应");
}

function addEnergy(data) {
    var start = data.parameterAddress;
    var highIndex = 0x06 - start;
    var lowIndex = 0x07 - start;
    if (highIndex < 0 || lowIndex >= data.values.length) return;
    var raw = data.values[highIndex] * 0x10000 + data.values[lowIndex];
    data.energy = { raw: raw, value: raw * 0.001, unit: "kWh" };
}

function namedRegisters(start, values, own, realtime) {
    var result = [];
    for (var i = 0; i < values.length; i++) {
        var address = start + i;
        var item = {
            address: address,
            addressHex: "0x" + hex8(address),
            raw: values[i]
        };
        var meta = own
            ? ownRegister(address, values[i], realtime)
            : standardRegister(address, values[i], realtime);
        if (meta) {
            item.name = meta.name;
            item.value = meta.value;
            if (meta.unit) item.unit = meta.unit;
        } else {
            item.name = "register_" + item.addressHex;
            item.value = values[i];
        }
        result.push(item);
    }
    return result;
}

function standardRegister(address, raw, realtime) {
    if (realtime) {
        switch (address) {
            case 0x00: return scaled("lineVoltage", raw, "V");
            case 0x01: return scaled("leakageCurrent", raw * 0.1, "mA");
            case 0x02: return scaled("linePower", raw, "W");
            case 0x03: return scaled("moduleTemperature", signed16(raw) * 0.1, "°C");
            case 0x04: return scaled("lineCurrent", raw * 0.01, "A");
            case 0x05: return { name: "alarmBits", value: raw };
            case 0x06: return { name: "energyHigh", value: raw };
            case 0x07: return { name: "energyLow", value: raw };
            case 0x08: return scaled("aPhaseVoltage", raw, "V");
            case 0x09: return scaled("bPhaseVoltage", raw, "V");
            case 0x0A: return scaled("cPhaseVoltage", raw, "V");
            case 0x0B: return scaled("aPhaseCurrent", raw * 0.01, "A");
            case 0x0C: return scaled("bPhaseCurrent", raw * 0.01, "A");
            case 0x0D: return scaled("cPhaseCurrent", raw * 0.01, "A");
            case 0x0E: return scaled("nPhaseCurrent", raw * 0.01, "A");
            case 0x0F: return scaled("aPhasePower", raw, "W");
            case 0x10: return scaled("bPhasePower", raw, "W");
            case 0x11: return scaled("cPhasePower", raw, "W");
            case 0x12: return { name: "aPhaseAlarmBits", value: raw };
            case 0x13: return { name: "bPhaseAlarmBits", value: raw };
            case 0x14: return { name: "cPhaseAlarmBits", value: raw };
            case 0x15: return { name: "networkControlExtraFunctions", value: raw };
            case 0x16:
                if (raw === 0x5A) return { name: "switchState", value: "close" };
                if (raw === 0xA5) return { name: "switchState", value: "open" };
                return { name: "switchState", value: raw };
            case 0x17: return powerFactor("aPhasePowerFactor", raw);
            case 0x18: return powerFactor("bPhasePowerFactor", raw);
            case 0x19: return powerFactor("cPhasePowerFactor", raw);
            case 0x19: return scaled("terminalTemperatureA", signed16(raw) * 0.1, "°C");
            case 0x1A: return scaled("terminalTemperatureB", signed16(raw) * 0.1, "°C");
            case 0x1B: return scaled("terminalTemperatureC", signed16(raw) * 0.1, "°C");
            case 0x1C: return scaled("terminalTemperatureN", signed16(raw) * 0.1, "°C");
            case 0x1D: return { name: "openShortFaultData", value: raw };
            case 0x1E: return { name: "nPhaseAlarmBits", value: raw };
            case 0x1F: return { name: "openCloseCount", value: raw };
            case 0x20: return { name: "handleLockState", value: raw };
            case 0x21: return { name: "contactState", value: raw === 0 ? "close" : "open" };
            case 0x22: return { name: "remoteCloseForbidden", value: raw === 1 };
            case 0x23: return powerFactor("totalPowerFactor", raw);
            case 0x30: return scaled("alarmLeakageCurrent", raw * 0.1, "mA");
            case 0x31: return { name: "totalAlarmBits", value: raw };
            case 0x32: return scaled("alarmLineVoltage", raw, "V");
            case 0x33: return scaled("alarmLineCurrent", raw * 0.01, "A");
            case 0x34: return scaled("alarmLinePower", raw, "W");
            case 0x35: return scaled("alarmModuleTemperature", signed16(raw) * 0.1, "°C");
            case 0x36: return { name: "aPhaseAlarmBits", value: raw };
            case 0x37: return scaled("aPhaseAlarmVoltage", raw, "V");
            case 0x38: return scaled("aPhaseAlarmCurrent", raw * 0.01, "A");
            case 0x39: return scaled("aPhaseAlarmPower", raw, "W");
            case 0x3A: return scaled("aPhaseAlarmTemperature", signed16(raw) * 0.1, "°C");
            case 0x3B: return { name: "bPhaseAlarmBits", value: raw };
            case 0x3C: return scaled("bPhaseAlarmVoltage", raw, "V");
            case 0x3D: return scaled("bPhaseAlarmCurrent", raw * 0.01, "A");
            case 0x3E: return scaled("bPhaseAlarmPower", raw, "W");
            case 0x3F: return scaled("bPhaseAlarmTemperature", signed16(raw) * 0.1, "°C");
            case 0x40: return { name: "cPhaseAlarmBits", value: raw };
            case 0x41: return scaled("cPhaseAlarmVoltage", raw, "V");
            case 0x42: return scaled("cPhaseAlarmCurrent", raw * 0.01, "A");
            case 0x43: return scaled("cPhaseAlarmPower", raw, "W");
            case 0x44: return scaled("cPhaseAlarmTemperature", signed16(raw) * 0.1, "°C");
            }
        return null;
    }

    switch (address) {
        case 0x00: return scaled("maxVoltage", raw, "V");
        case 0x01: return scaled("minVoltage", raw, "V");
        case 0x02: return scaled("maxLeakageCurrent", raw * 0.1, "mA");
        case 0x03: return scaled("maxPower", raw, "W");
        case 0x04: return scaled("maxTemperature", signed16(raw) * 0.1, "°C");
        case 0x05: return scaled("maxCurrent", raw * 0.01, "A");
        case 0x06: return { name: "model", value: raw };
        case 0x07: return { name: "version", value: raw };
        case 0x08: return { name: "modelFunctionCode", value: raw };
        case 0x09: return scaled("aPhaseMaxCurrent", raw * 0.01, "A");
        case 0x0A: return scaled("bPhaseMaxCurrent", raw * 0.01, "A");
        case 0x0B: return scaled("cPhaseMaxCurrent", raw * 0.01, "A");
        case 0x0C: return scaled("aPhaseMaxPower", raw, "W");
        case 0x0D: return scaled("bPhaseMaxPower", raw, "W");
        case 0x0E: return scaled("cPhaseMaxPower", raw, "W");
        case 0x0F: return scaled("voltageWarningMax", raw, "V");
        case 0x10: return scaled("voltageWarningMin", raw, "V");
        case 0x11: return scaled("leakageWarningMax", raw * 0.1, "mA");
        case 0x12: return scaled("warningTemperature", signed16(raw) * 0.1, "°C");
        case 0x13: return scaled("aPhaseWarningCurrent", raw * 0.01, "A");
        case 0x14: return scaled("bPhaseWarningCurrent", raw * 0.01, "A");
        case 0x15: return scaled("cPhaseWarningCurrent", raw * 0.01, "A");
        case 0x16: return scaled("currentWarningLimit", raw * 0.01, "A");
        case 0x17: return scaled("nPhaseCurrentLimit", raw * 0.01, "A");
        case 0x18: return { name: "inverseTimeCurve", value: raw };
        case 0x19: return { name: "leakageGear", value: raw };
        case 0x1A: return { name: "longDelayArcSensitivity", value: raw };
        case 0x1B: return { name: "instantArcSensitivity", value: raw };
        case 0x1C: return scaled("malignantLoadPowerMin", raw, "W");
        case 0x1D: return scaled("malignantResistivePowerMin", raw, "W");
        case 0x1E: return { name: "voltageImbalanceLimit", value: raw };
        case 0x1F: return { name: "airSwitchType", value: raw };
        case 0x30: return { name: "id1", value: raw };
        case 0x31: return { name: "id2", value: raw };
        case 0x32: return { name: "id3", value: raw };
        case 0x33: return { name: "functionEnable0", value: raw };
        case 0x34: return { name: "functionEnable1", value: raw };
        case 0x35: return { name: "tripEnable0", value: raw };
        case 0x36: return { name: "tripEnable1", value: raw };
        case 0x37: return { name: "allowClose0", value: raw };
        case 0x38: return { name: "allowClose1", value: raw };
    }
    return null;
}

function ownRegister(address, raw, realtime) {
    if (realtime) {
        switch (address) {
            case 0x01: return scaled("moduleTemperature1", signed16(raw) * 0.1, "°C");
            case 0x02: return scaled("moduleTemperature2", signed16(raw) * 0.1, "°C");
            case 0x03: return { name: "moduleAlarmBits", value: raw };
        }
        return null;
    }
    switch (address) {
        case 0x03: return { name: "baudRateCode", value: raw };
        case 0x04: return { name: "communicationFormat", value: raw === 1 ? "RTU" : (raw === 0 ? "ASCII" : raw) };
        case 0x07: return { name: "remoteMode", value: raw === 1 };
        case 0x08: return { name: "disconnectTripTarget", value: raw };
        case 0x09: return { name: "moduleVersion", value: raw };
        case 0x0A: return { name: "energyStorageEnabled", value: raw === 1 };
        case 0x0C: return { name: "addressSwapEnabled", value: raw === 1 };
        case 0x10: return { name: "temperatureTripEnabled", value: (raw & 1) === 1 };
        case 0x11: return scaled("moduleTemperatureLimit", signed16(raw) * 0.1, "°C");
        case 0x50: return { name: "uniqueId1", value: raw };
        case 0x51: return { name: "uniqueId2", value: raw };
        case 0x52: return { name: "uniqueId3", value: raw };
        case 0x53: return { name: "imei7_8", value: raw };
        case 0x54: return { name: "imei5_6", value: raw };
        case 0x55: return { name: "imei3_4", value: raw };
        case 0x56: return { name: "imei1_2", value: raw };
        case 0x57: return { name: "imsi7_8", value: raw };
        case 0x58: return { name: "imsi5_6", value: raw };
        case 0x59: return { name: "imsi3_4", value: raw };
        case 0x5A: return { name: "imsi1_2", value: raw };
        case 0x5B: return { name: "iccid1", value: raw };
        case 0x5C: return { name: "iccid2", value: raw };
        case 0x5D: return { name: "iccid3", value: raw };
        case 0x5E: return { name: "iccid4", value: raw };
        case 0x5F: return { name: "iccid5", value: raw };
    }
    return null;
}

function parameterRegister(address, raw, own) {
    if (own) {
        var ownMeta = ownRegister(address, raw, false);
        if (ownMeta) return { address: address, addressHex: "0x" + hex8(address), name: ownMeta.name, value: ownMeta.value };
        return null;
    }
    var meta = writeParameterRegister(address, raw);
    if (meta) return { address: address, addressHex: "0x" + hex8(address), name: meta.name, value: meta.value };
    return null;
}

function writeParameterRegister(address, raw) {
    switch (address) {
        case 0x00: return scaled("maxVoltage", raw, "V");
        case 0x01: return scaled("minVoltage", raw, "V");
        case 0x02: return scaled("maxLeakageCurrent", raw * 0.1, "mA");
        case 0x03: return scaled("maxPower", raw, "W");
        case 0x04: return scaled("maxTemperature", signed16(raw) * 0.1, "°C");
        case 0x05: return scaled("maxCurrent", raw * 0.01, "A");
        case 0x08: return { name: "specialCommand", value: raw };
        case 0x09: return { name: "unlockCommand", value: raw };
        case 0x0A: return scaled("leakageWarning", raw * 0.1, "mA");
        case 0x0B: return scaled("temperatureWarning", signed16(raw) * 0.1, "°C");
        case 0x0C: return scaled("aPhaseWarningCurrent", raw * 0.01, "A");
        case 0x0D: return scaled("bPhaseWarningCurrent", raw * 0.01, "A");
        case 0x0E: return scaled("cPhaseWarningCurrent", raw * 0.01, "A");
        case 0x14: return scaled("voltageWarningMax", raw, "V");
        case 0x15: return scaled("voltageWarningMin", raw, "V");
        case 0x16: return { name: "energyHigh", value: raw };
        case 0x17: return { name: "energyLow", value: raw };
        case 0x28: return scaled("aPhaseMaxPower", raw, "W");
        case 0x29: return scaled("bPhaseMaxPower", raw, "W");
        case 0x2A: return scaled("cPhaseMaxPower", raw, "W");
        case 0x2B: return scaled("aPhaseMaxCurrent", raw * 0.01, "A");
        case 0x2C: return scaled("bPhaseMaxCurrent", raw * 0.01, "A");
        case 0x2D: return scaled("cPhaseMaxCurrent", raw * 0.01, "A");
        case 0x2E: return scaled("nPhaseMaxCurrent", raw * 0.01, "A");
        case 0x2F: return { name: "inverseTimeCurve", value: raw };
        case 0x30: return { name: "leakageProtectionGear", value: raw };
        case 0x31: return { name: "longDelayArcSensitivity", value: raw };
        case 0x32: return { name: "instantArcSensitivity", value: raw };
        case 0x33: return { name: "voltageImbalance", value: raw };
        case 0x34: return scaled("malignantLoadPowerMin", raw, "W");
        case 0x35: return scaled("malignantResistivePowerMin", raw, "W");
        case 0x49: return { name: "factoryReset2", value: raw };
        case 0x63: return { name: "functionEnable0", value: raw };
        case 0x64: return { name: "functionEnable1", value: raw };
        case 0x65: return { name: "tripEnable0", value: raw };
        case 0x66: return { name: "tripEnable1", value: raw };
        case 0x67: return { name: "allowClose0", value: raw };
        case 0x68: return { name: "allowClose1", value: raw };
    }
    return null;
}

function scaled(name, value, unit) {
    return { name: name, value: value, unit: unit };
}

function powerFactor(name, raw) {
    return { name: name, value: signed16(raw) / 32767 };
}

function modbusException(code) {
    if (code === 0x06) return "slave device busy";
    if (code === 0x01) return "illegal function";
    if (code === 0x02) return "illegal data address";
    if (code === 0x03) return "illegal data value";
    if (code === 0x04) return "slave device failure";
    return "unknown exception";
}

function readWords(m, offset, count) {
    var result = [];
    for (var i = 0; i < count; i++) result.push(readUInt16BE(m, offset + i * 2));
    return result;
}

function readUInt16BE(d, i) {
    return ((d[i] << 8) | d[i + 1]) & 0xFFFF;
}

function signed16(v) {
    return (v & 0x8000) ? v - 0x10000 : v;
}

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

function err(msg) {
    return { data: {}, errors: [msg] };
}

function hex8(n) {
    return (n < 16 ? "0" : "") + n.toString(16);
}
