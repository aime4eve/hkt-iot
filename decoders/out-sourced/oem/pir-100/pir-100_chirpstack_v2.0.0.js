/**
 * PIR-100 人存感应器 上行解码器（ChirpStack v4）
 *
 * 依据（正确性来源）：《HKT PIR Motion Sensor User Manual v1.2》§5 Communication Format
 *   - fPort = 210；上行帧为通道式结构：1 字节通道号 + 定长值（通道可按报告类型省略）
 *   - 通道表：00 保留(1B 占位，无值) | 01 产品型号(1B) | 04 电池电压(2B 大端 mV，÷1000=V)
 *             | 7d 电压状态(1B：00 Normal / 01 Low Voltage)
 *             | 77 防拆状态(1B：00 Normal / 01 Tampered)
 *             | 05 低电告警(1B：00 Normal / 01 Alarm)
 *             | 03 防拆告警(1B：00 Normal / 01 Tamper Alarm)
 *             | 17 红外告警(1B：00 Normal / 01 Alarm)
 *   - 手册示例帧 000104040c857d0077011701
 *     => 电池 3.205V、电压正常、防拆已触发、红外告警
 *
 * 版本历史：
 *   v1.0.0  迁移存档（5 字节帧假设与手册不符：越界读、电池换算错误，勿用）
 *   v2.0.0  2026-09-17 按手册 §5 重写：通道长度表 + 越界/未知通道防护
 */

var CHANNEL_LENGTHS = {
    0x00: 0, // 保留通道，仅占位
    0x01: 1, // 产品型号
    0x04: 2, // 电池电压 mV（大端）
    0x7d: 1, // 电压状态
    0x77: 1, // 防拆状态
    0x05: 1, // 低电告警
    0x03: 1, // 防拆告警
    0x17: 1, // 红外告警
};

function decodeUplink(input) {
    var bytes = input.bytes;
    var decoded = {};
    var errors = [];
    var i = 0;

    while (i < bytes.length) {
        var ch = bytes[i];
        i++;
        var len = CHANNEL_LENGTHS[ch];
        if (len === undefined) {
            errors.push("unknown channel 0x" + ch.toString(16) + " at offset " + (i - 1) + ", stop parsing");
            break;
        }
        if (i + len > bytes.length) {
            errors.push("truncated channel 0x" + ch.toString(16) + ", need " + len + " byte(s), got " + (bytes.length - i));
            break;
        }
        switch (ch) {
            case 0x00: // 保留，无值
                break;
            case 0x01:
                decoded.productModel = bytes[i];
                break;
            case 0x04:
                decoded.batteryVoltage = ((bytes[i] << 8) | bytes[i + 1]) / 1000;
                break;
            case 0x7d:
                decoded.voltageStatus = bytes[i] === 0 ? "Normal" : "Low Voltage";
                break;
            case 0x77:
                decoded.tamperStatus = bytes[i] === 0 ? "Normal" : "Tampered";
                break;
            case 0x05:
                decoded.lowBatteryAlarm = bytes[i] === 0 ? "Normal" : "Alarm";
                break;
            case 0x03:
                decoded.tamperAlarm = bytes[i] === 0 ? "Normal" : "Tamper Alarm";
                break;
            case 0x17:
                decoded.infraredAlarm = bytes[i] === 0 ? "Normal" : "Alarm";
                break;
        }
        i += len;
    }

    if (errors.length === 0 && Object.keys(decoded).length === 0) {
        errors.push("no known channel decoded");
    }
    return errors.length ? { data: decoded, errors: errors } : { data: decoded };
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { decodeUplink };
}
