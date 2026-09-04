/**
 * Payload Decoder for The Chirpstack v4
 *
 * Copyright 2025 HKT SmartHard
 *
 * @product PIR-100
 *
 * 协议依据：《HKT PIR Motion Sensor User Manual v1.2》第 5 章 Communication Format
 *   - FPort 210（解码器不强制校验端口，仅按负载格式解析）
 *   - 上行为通道式格式：每条记录 = 1 字节通道号 + 定长值（电池电压为 2 字节大端，其余 1 字节）
 *   - 通道 0x00 为保留通道，仅 1 字节占位，无值
 *
 * 通道长度表（手册 5.1 Heartbeat frame）：
 *   0x00 保留 | 0x01 产品型号 | 0x03 防拆告警 | 0x04 电池电压(2B, mV)
 *   0x05 低电告警 | 0x17 红外告警 | 0x77 防拆状态 | 0x7D 电压状态
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2025    初版（len==5 门槛 + bytes[1..6] 布局假设，与手册协议不符）。
 * 2026-09-04 按手册 v1.2 §5 重写：
 *   1. 删除 len==5 门槛与错误字段假设，改为通道式循环解析（通道号+定长值）；
 *   2. 电池电压修正为 2 字节大端 ÷1000 得伏特（原 1 字节 ÷10）；
 *   3. 补齐手册全部通道字段：productModel/batteryVoltage/voltageStatus/
 *      tamperStatus/lowBatteryAlarm/tamperAlarm/infraredAlarm；
 *   4. 防御性解析：截断/未知通道报 error 并停止，空负载报错，不再越界读；
 *   5. decodeUplink 错误时返回标准 errors 数组（原任何输入恒返回 {data:{}}）；
 *   6. 字段名为破坏性变更（原 infraredStatus/antiTamperSwitch 等已废弃），
 *      平台侧规则/看板需同步映射。
 *   验证：手册示例帧 000104040c857d0077011701 及三条实测上行
 *   （3.610V/3.612V/3.608V，含变长 10B 帧与红外告警帧）解码全部正确。
 * ─────────────────────────────────────────────────────────
 */
var CHANNEL_LENGTHS = {
    0x00: 0, // Reserved channel (marker only, no value)
    0x01: 1, // Product Model
    0x03: 1, // Tamper Alarm: 00 Normal; 01 Tamper Alarm
    0x04: 2, // Battery Voltage (big-endian, unit: mV)
    0x05: 1, // Low Battery Alarm: 00 Normal; 01 Alarm
    0x17: 1, // Infrared Alarm: 00 Normal; 01 Alarm
    0x77: 1, // Tamper Status: 00 Normal; 01 Tampered
    0x7D: 1, // Voltage Status: 00 Normal; 01 Low Voltage
};

function easy_decode(bytes) {
    var decoded = {};

    if (!bytes || bytes.length === 0) {
        decoded.error = "empty payload";
        return decoded;
    }

    var i = 0;
    while (i < bytes.length) {
        var channel = bytes[i];
        i++;
        var len = CHANNEL_LENGTHS[channel];
        if (len === undefined) {
            decoded.error = "unknown channel 0x" + channel.toString(16) + " at offset " + (i - 1) + ", stop parsing";
            return decoded;
        }
        if (i + len > bytes.length) {
            decoded.error = "truncated channel 0x" + channel.toString(16) + ", need " + len + " byte(s), got " + (bytes.length - i);
            return decoded;
        }
        switch (channel) {
            case 0x00: // Reserved
                break;
            case 0x01: // Product Model (04 = PIR sensor)
                decoded.productModel = bytes[i];
                break;
            case 0x03: // Tamper Alarm
                decoded.tamperAlarm = bytes[i] === 0 ? "Normal" : "Tamper Alarm";
                break;
            case 0x04: // Battery Voltage, 2 bytes big-endian, mV -> V
                decoded.batteryVoltage = ((bytes[i] << 8) | bytes[i + 1]) / 1000;
                break;
            case 0x05: // Low Battery Alarm
                decoded.lowBatteryAlarm = bytes[i] === 0 ? "Normal" : "Alarm";
                break;
            case 0x17: // Infrared Alarm
                decoded.infraredAlarm = bytes[i] === 0 ? "Normal" : "Alarm";
                break;
            case 0x77: // Tamper Status
                decoded.tamperStatus = bytes[i] === 0 ? "Normal" : "Tampered";
                break;
            case 0x7D: // Voltage Status
                decoded.voltageStatus = bytes[i] === 0 ? "Normal" : "Low Voltage";
                break;
        }
        i += len;
    }
    return decoded;
}

function decodeUplink(input) {
    var decoded = easy_decode(input.bytes);
    if (decoded.error) {
        return { data: decoded, errors: [decoded.error] };
    }
    return { data: decoded };
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { decodeUplink, easy_decode };
}
