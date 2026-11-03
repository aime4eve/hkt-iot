/**
 * Payload Decoder for The Reports
 *
 * Copyright 2022 HKT SmartHard
 *
 * @product HKT-DMS01 (LoRaWAN Wireless Door Sensor)
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2022       初版（固件自带 Compents/Decode/door_sensor.js，存档形态见 v1.0.0）。
 * 2026-11-03 按固件全量重写为 v2.0.0（对照 USER/Drive/communicate.c 与
 *            USER/Drive/include/communicate.h，ref eaa1d9d2）：
 *   1. 修复 0x24 计数间隔：固件写 2 字节（值=255×高+低，x/0xFF、x%0xFF 拆包，
 *      communicate.c:95-98），原脚本只读 1 字节 → 周期帧含 0x24 时后续 TLV 全部错位；
 *   2. 0x26 前两个 2 字节字段（周期内开/关计数）同按 255 基还原；
 *      后两个 4 字节（累计开/关计数）为大端 uint32（>>> 0 防符号溢出）；
 *   3. irc01 式防御守卫：坏同步头/未知类型/截断 TLV 报错并停止，
 *      不死循环、不静默错值、不越界读（长度表前置检查）；
 *   4. 新增输出 ackRequested（byte[3] bit0）与 seqNo（byte[4] 包序号）、
 *      deviceAck（收到 0xFF 设备应答帧）；
 *   5. 0x80（服务器对时）/ 0xFF（设备 ACK）识别为零值长度类型
 *      （setDataPackage 对二者只写 1 字节类型）；
 *   6. 字段名沿用原脚本拼写（hard_ver/hearterIntervel 等）兼容平台消费方。
 * ─────────────────────────────────────────────────────────
 * 帧格式（对应固件 communicate.c sendLoRaWANData）：
 *   [0x68 0x6B 0x74][flags][seqNo][TLV...]
 *   flags: bit0 = 服务器要求应答（上行帧固定写 0，此处按位如实解码）
 *   seqNo: 包序号 packSyncNumber++（非长度、非校验，帧无校验和）
 *   TLV:   1 字节类型 + 定长值（无长度字段，类型长度表见 TLV_LENGTHS）
 *
 * 防御性约束：
 *   - 任何 TLV 越过帧尾 => 报 error，绝不越界
 *   - 未知类型无法定位下一个 TLV => 记录 error 并停止解析
 *   - 多字节整数一律无符号；0x24/0x26 间隔字段按固件 255 基算术还原
 */
var TLV_LENGTHS = {
    0x01: 2,  // hard_ver + soft_ver
    0x03: 1,  // battery
    0x08: 1,  // doorStatus
    0x22: 1,  // runMode
    0x23: 4,  // startHours + startMins + endHours + endMins
    0x24: 2,  // counterIntervel（固件 x/0xFF、x%0xFF 拆包，255 基还原）
    0x25: 1,  // hearterIntervel
    0x26: 12, // counterA/counterB（255 基 2B）+ totalCounterA/totalCounterB（大端 u32）
    0x80: 0,  // 服务器对时（下行方向专属，零值长度）
    0x84: 1,  // tamperStatus
    0xFF: 0   // 设备通讯应答 ACK（零值长度）
};

function decodeUplink(input) {
    var bytes = input.bytes;
    var decoded = {};
    var errors = [];

    if (!Array.isArray(bytes) || bytes.length < 5) {
        return { data: {}, errors: ['帧过短：至少需 5 字节头（3 同步头字节 + flags + seqNo）'] };
    }
    if (bytes[0] !== 0x68 || bytes[1] !== 0x6B || bytes[2] !== 0x74) {
        return { data: {}, errors: ['同步头错误：期望 68 6B 74'] };
    }

    decoded.ackRequested = (bytes[3] & 0x01) === 1;
    decoded.seqNo = bytes[4];

    var i = 5;
    while (i < bytes.length) {
        var type = bytes[i];
        i++;
        var len = TLV_LENGTHS[type];
        if (len === undefined) {
            errors.push('未知数据类型 0x' + type.toString(16) + '（第 ' + (i - 1) + ' 字节），无法定位下一个 TLV，停止解析');
            break;
        }
        if (i + len > bytes.length) {
            errors.push('数据类型 0x' + type.toString(16) + ' 值截断：需 ' + len + ' 字节，仅剩 ' + (bytes.length - i) + ' 字节');
            break;
        }
        switch (type) {
            case 0x01:
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                break;
            case 0x03:
                decoded.battery = bytes[i];
                break;
            case 0x08:
                decoded.doorStatus = bytes[i];
                break;
            case 0x22:
                decoded.runMode = bytes[i];
                break;
            case 0x23:
                decoded.startHours = bytes[i];
                decoded.startMins = bytes[i + 1];
                decoded.endHours = bytes[i + 2];
                decoded.endMins = bytes[i + 3];
                break;
            case 0x24:
                decoded.counterIntervel = bytes[i] * 255 + bytes[i + 1];
                break;
            case 0x25:
                decoded.hearterIntervel = bytes[i];
                break;
            case 0x26:
                decoded.counterA = bytes[i] * 255 + bytes[i + 1];
                decoded.counterB = bytes[i + 2] * 255 + bytes[i + 3];
                decoded.totalCounterA = readUint32(bytes, i + 4);
                decoded.totalCounterB = readUint32(bytes, i + 8);
                break;
            case 0x80: // 对时类型：零值长度，无字段输出
                break;
            case 0x84:
                decoded.tamperStatus = bytes[i];
                break;
            case 0xFF:
                decoded.deviceAck = true;
                break;
        }
        i += len;
    }

    if (errors.length) return { data: decoded, errors: errors };
    return { data: decoded };
}

function readUint32(bytes, pos) {
    return ((bytes[pos] << 24) | (bytes[pos + 1] << 16) | (bytes[pos + 2] << 8) | bytes[pos + 3]) >>> 0;
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { decodeUplink, readUint32, TLV_LENGTHS };
}
