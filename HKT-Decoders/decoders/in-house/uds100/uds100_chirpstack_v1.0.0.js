/**
 * Payload Decoder for The Reports — UDS100 垃圾桶满溢监测传感器
 *
 * Copyright 2026 HKT SmartHard
 *
 * @product HKT-UDS100 (uds100)
 *
 * 正确性来源固件（唯一权威）：
 *   HKT-Firmwares/in-house/LoRaWAN_Ultrasonic_Distance_Sensor
 *   - USER/Drive/communicate.c  setDataPackage() / sendLoRaWANData() /
 *     Device_PeriodicReport() / Device_GpsReport() / fromLoRaWANDataHandle()
 *   - USER/Drive/include/communicate.h  enum DataType（类型→hex 映射）
 *   - USER/config.h  HARDWARE_VER=0x02, SOFTWARE_VER=0x0A,
 *     LoRaWAN_DEFAULT_PORT=10, DEFAULT_REPORT_INTERVAL=30
 * （2026-09-18 对照固件源码逐字段核验）
 *
 * 帧格式（LoRaWAN 应用负载，fPort 默认 10，可经调试口改配）：
 *   [0x68 0x6B 0x74][flags][seq][TLV...]
 *   - flags: 上行恒 0x00（sendLoRaWANData 固定写 0；bit0 仅在下行方向表示"服务器要求应答"）
 *   - seq:   packSyncNumber 自增包序号（0~255 回绕）
 *   - TLV:   1 字节类型 + 定长值（无长度字段，长度由类型表约定，全部大端）
 *
 * 上行实际会发送的 TLV（按固件调用方枚举）：
 *   Device_PeriodicReport（周期到点 / 满溢或倾斜或高温状态变化触发）：
 *     0x01 版本, 0x8B 电池电压, 0x09 温度, 0x0A 湿度, 0x0E 俯仰角,
 *     0x44 倾斜状态, 0x28 高温报警, 0x45 GPS 周期,
 *     [0x46 超声波距离 + 0x47 满溢状态 —— 仅未倾斜时],
 *     0x48 满溢阈值配置, 0x86 上报周期
 *   Device_GpsReport（GPS 定位成功时）：0x10 纬度, 0x11 经度
 *   fromLoRaWANDataHandle（下行 flags bit0=1 时应答）：0xFF ACK
 *   注：0x02 设备ID / 0x8D 开关机状态虽在 setDataPackage 中实现，
 *       但 LoRaWAN 上行无调用方（仅 BLE 通道使用），本表保留兼容。
 *
 * 缩放与字节序（全部大端）：
 *   0x01 [硬件版本 u8][软件版本 u8]（默认 0x02 / 0x0A）
 *   0x09 温度 3 字节 ×1000，bit23=负数标志（负温时 固件写 (-t)|0x800000）
 *   0x0A 湿度 3 字节 ×1000（%RH）
 *   0x0E 俯仰角 2 字节 ×100（度，固件 float 截断为 u16）
 *   0x10/0x11 纬度/经度 4 字节 = 绝对值(度)×1e6，bit31=南纬/西经标志
 *   0x45/0x86 GPS 周期/上报周期 2 字节（分钟，0 表示 GPS 定位关闭）
 *   0x46 超声波距离 2 字节（mm）；测量无效（阈值状态 0xFF）时固件强制发 0
 *   0x47 满溢状态：0 正常 / 1 低于低阈值(满溢) / 2 高于高阈值 / 255(0xFF) 无效
 *   0x48 [低阈值 u16 mm][高阈值 u16 mm]（高阈值 0 表示未启用；有效域 30~4500）
 *   0x8B 电池电压 2 字节（mV）
 *   0xFF ACK 值恒 0xFF
 *
 * 防御性约束（沙箱内无 Buffer/TypedArray/require，纯 JS）：
 *   - 坏帧头 / 过短帧 / TLV 截断 / 未知类型 => 返回 {data:{}, errors:[...]}，绝不越界
 *   - 已知类型长度表前置检查，杜绝失步与死循环
 */
var TLV_LENGTHS = {
    0x01: 2,  // 软硬件版本 [HW][SW]
    0x02: 6,  // 设备 ID（DevEUI 后 6 字节；LoRaWAN 上行当前不发送）
    0x09: 3,  // 温度 ×1000，bit23 负数标志
    0x0a: 3,  // 湿度 ×1000
    0x0e: 2,  // 俯仰角 ×100
    0x10: 4,  // 纬度 绝对值×1e6，bit31 南纬标志
    0x11: 4,  // 经度 绝对值×1e6，bit31 西经标志
    0x28: 1,  // 高温报警（≥50.000°C 置 1）
    0x44: 1,  // 倾斜状态
    0x45: 2,  // GPS 定位周期（分钟）
    0x46: 2,  // 超声波距离（mm）
    0x47: 1,  // 垃圾桶满溢状态
    0x48: 4,  // 满溢阈值 [低 u16][高 u16] mm
    0x86: 2,  // 上报周期（分钟）
    0x8b: 2,  // 电池电压（mV）
    0x8d: 1,  // 开关机状态（LoRaWAN 上行当前不发送）
    0xff: 1,  // 通讯应答 ACK（恒 0xFF）
};

function decodeUplink(input) {
    var bytes = input && input.bytes ? input.bytes : null;
    if (!bytes || bytes.length < 5) {
        return { data: {}, errors: ["frame too short: need at least 5 bytes (sync head 3 + flags + seq), got " + (bytes ? bytes.length : 0)] };
    }
    if (bytes[0] !== 0x68 || bytes[1] !== 0x6b || bytes[2] !== 0x74) {
        return { data: {}, errors: ["invalid frame header: expected 68 6B 74, got " +
            hex2(bytes[0]) + " " + hex2(bytes[1]) + " " + hex2(bytes[2])] };
    }

    var data = {};
    data.ackRequested = (bytes[3] & 0x01) === 1; // 上行恒 false；下行 bit0=1 时设备才回 ACK
    data.seqNo = bytes[4];

    var i = 5;
    while (i < bytes.length) {
        var type = bytes[i];
        i += 1;
        var len = TLV_LENGTHS[type];
        if (len === undefined) {
            return { data: {}, errors: ["unknown TLV type 0x" + hex2(type) + " at offset " + (i - 1) + ", stop parsing"] };
        }
        if (i + len > bytes.length) {
            return { data: {}, errors: ["truncated TLV type 0x" + hex2(type) + " at offset " + (i - 1) + ", need " + len + " value byte(s), got " + (bytes.length - i)] };
        }
        switch (type) {
            case 0x01: // 软硬件版本
                data.hard_ver = bytes[i];
                data.soft_ver = bytes[i + 1];
                break;
            case 0x02: // 设备 ID（DevEUI 后 6 字节）
                data.devEUI = bytesToHex(bytes, i, 6);
                break;
            case 0x09: { // 温度：×1000，bit23 负数标志
                var t = readUint24(bytes, i);
                var tNeg = (t & 0x800000) !== 0;
                var tVal = (t & 0x7fffff) / 1000;
                data.temperature = tNeg ? -tVal : tVal;
                break;
            }
            case 0x0a: // 湿度：×1000
                data.humidity = readUint24(bytes, i) / 1000;
                break;
            case 0x0e: // 俯仰角：×100
                data.angle = readUint16(bytes, i) / 100;
                break;
            case 0x10: { // 纬度：绝对值×1e6，bit31 南纬
                var lat = readUint32(bytes, i);
                data.latitudeSouth = lat >= 2147483648;
                data.latitude = (lat - (data.latitudeSouth ? 2147483648 : 0)) / 1000000;
                break;
            }
            case 0x11: { // 经度：绝对值×1e6，bit31 西经
                var lon = readUint32(bytes, i);
                data.longitudeWest = lon >= 2147483648;
                data.longitude = (lon - (data.longitudeWest ? 2147483648 : 0)) / 1000000;
                break;
            }
            case 0x28: // 高温报警
                data.htAlarm = bytes[i];
                break;
            case 0x44: // 倾斜状态
                data.slantState = bytes[i];
                break;
            case 0x45: // GPS 定位周期（分钟，0=关闭）
                data.gpsPeriod = readUint16(bytes, i);
                break;
            case 0x46: // 超声波距离（mm，无效测量时固件发 0）
                data.distance = readUint16(bytes, i);
                break;
            case 0x47: // 满溢状态：0 正常 / 1 满溢 / 2 超高阈值 / 255 无效
                data.overflowState = bytes[i];
                break;
            case 0x48: // 满溢阈值 [低][高] mm
                data.thresholdLow = readUint16(bytes, i);
                data.thresholdHigh = readUint16(bytes, i + 2);
                break;
            case 0x86: // 上报周期（分钟）
                data.syncPeriod = readUint16(bytes, i);
                break;
            case 0x8b: // 电池电压（mV）
                data.batteryVoltage = readUint16(bytes, i);
                break;
            case 0x8d: // 开关机状态
                data.powerState = bytes[i];
                break;
            case 0xff: // 通讯应答 ACK（恒 0xFF）
                data.ackResponse = bytes[i];
                break;
        }
        i += len;
    }
    return { data: data };
}

/* ---- 纯 JS 数值助手（不依赖 Buffer/TypedArray） ---- */

function readUint16(b, o) {
    return (b[o] << 8) | b[o + 1];
}

function readUint24(b, o) {
    return ((b[o] << 16) | (b[o + 1] << 8) | b[o + 2]) >>> 0;
}

function readUint32(b, o) {
    // 用乘法避免 <<24 的 int32 符号位陷阱，结果为安全整数（< 2^32）
    return b[o] * 16777216 + (b[o + 1] << 16 | b[o + 2] << 8 | b[o + 3]);
}

function hex2(v) {
    var h = (v & 0xff).toString(16);
    return h.length < 2 ? "0" + h : h;
}

function bytesToHex(b, o, n) {
    var s = "";
    for (var k = 0; k < n; k++) s += hex2(b[o + k]);
    return s;
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module 时自动跳过
if (typeof module !== "undefined") {
    module.exports = { decodeUplink, readUint16, readUint24, readUint32 };
}
