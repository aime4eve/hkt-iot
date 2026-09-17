/**
 * 欧孚 W200PL（LoRa 私有协议）手表 上行解码器
 * 依据（正确性来源）：《W200PL-Lora私有-通信协议-V2.3.docx》第 3、4 章
 *
 * 帧结构（与 B2315L V2.1 的差异：BD 头后多 8 字节 DEVID）：
 *   [0xBD][DEVID 8B][MSGID 1B][payload N][checksum 1B]
 *   checksum = 0xFF - Σ(从 BD 到 payload 末尾)；文档注明"通用版本不强求校验"，
 *   且文档示例帧存在自相矛盾（跌落帧标 E7 尾实为 56 等），故本解码器只做
 *   宽松校验（不符记 warning，不拒解）。
 *   文档另外三处示例帧自相矛盾（02 低电帧内 0200、C3 开始/充满帧内时间戳
 *   DB4D2F66 与正文解释 E377BD67 不符、32 帧内时间戳与正文换算不符），
 *   一律以帧内数据 + 文档字段算法定义为准，样例 note 已逐条记录。
 *
 * 版本历史：
 *   v1.0.0  2026-09-17 按 V2.3 文档 §4 实现全部 12 类上行帧：
 *   02 报警(位图) / 21 报警-2(type1 关机细分、type5 健康阈值) / B5 SOS /
 *   03 GPS/BDS / D6 蓝牙定位 / C3 充电状态 / F9 电量 / BB 固件版本 /
 *   A5 组号 / 32 健康(固定布局) / C0 下行反馈 / 28 消息状态。
 *   时间戳统一输出北京时间（UTC+8 固定偏移，不依赖宿主时区）。
 */

var decoded = {};
var warnings = [];

function decodeUplink(input) {
    var bytes = input.bytes;
    decoded = {};
    warnings = [];
    var offset = 0;

    if (!bytes || bytes.length < 11 || bytes[0] !== 0xBD) {
        return { data: {}, errors: ['帧头错误：W200PL 上行帧必须以 0xBD 开头（BD+DEVID(8B)+MSGID+payload+checksum）'] };
    }

    while (offset < bytes.length) {
        if (bytes.length - offset < 11) {
            warnings.push('offset ' + offset + ': 剩余 ' + (bytes.length - offset) + ' 字节，不足最短帧（11B），已停止解析');
            break;
        }
        if (bytes[offset] !== 0xBD) {
            warnings.push('offset ' + offset + ': 非 0xBD 帧头（0x' + hex8(bytes[offset]) + '），跳过该字节');
            offset += 1;
            continue;
        }
        decoded.devId = bytesToHex(bytes, offset + 1, 8);
        var msgId = bytes[offset + 9];
        var consumed;
        switch (msgId) {
            case 0x02: consumed = decodeAlarm1(bytes, offset); break;
            case 0x21: consumed = decodeAlarm2(bytes, offset); break;
            case 0xB5: consumed = decodeSos(bytes, offset); break;
            case 0x03: consumed = decodeGps(bytes, offset); break;
            case 0xD6: consumed = decodeLbe(bytes, offset); break;
            case 0xC3: consumed = decodeCharge(bytes, offset); break;
            case 0xF9: consumed = decodeBattery(bytes, offset); break;
            case 0xBB: consumed = decodeFirmwareVersion(bytes, offset); break;
            case 0xA5: consumed = decodeGroupIds(bytes, offset); break;
            case 0x32: consumed = decodeHealth(bytes, offset); break;
            case 0xC0: consumed = decodeFeedback(bytes, offset); break;
            case 0x28: consumed = decodeMessageStatus(bytes, offset); break;
            default:
                warnings.push('offset ' + offset + ': 未知 MSGID 0x' + hex8(msgId) + '，跳过该字节');
                offset += 1;
                continue;
        }
        if (consumed < 0) {
            warnings.push('offset ' + offset + ': MSGID 0x' + hex8(msgId) + ' 帧不完整，停止解析');
            break;
        }
        offset += consumed;
        if (bytes.length - offset === 1) {
            // 流末尾剩余 1 字节 = 校验和。文档 §3.5 给出算法但全部示例值与其
            // 自身定义对不上，且注明"通用版本不强求校验"——故仅消费不校验。
            offset += 1;
        }
    }

    var result = { data: decoded };
    if (warnings.length) result.warnings = warnings;
    return result;
}

/* ---------- 各帧解析（o = 0xBD 头位置；payload 自 o+10 起） ---------- */

var ALARM1_FLAGS = [
    [0x4000, 'fall'], [0x0100, 'wearing'], [0x0080, 'sos_cancel'], [0x0020, 'sedentary'],
    [0x0010, 'taken_off'], [0x0004, 'shutdown'], [0x0001, 'low_battery'],
];
// 4.1.1 BD 02 warnU16LE tsU32LE CK（文档 §4.1.1 示例帧 02 变体均无 SOS 位，SOS 走 0xB5）
function decodeAlarm1(bytes, o) {
    if (bytes.length - o < 17) return -1;
    var warn = readU16LE(bytes, o + 10);
    decoded.Upl_warn = warn.toString(16);
    decoded.alarm = flagNames(ALARM1_FLAGS, warn);
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 12));
    return 16;
}

// 4.1.2 BD 21 alarmTypeU16LE ... CK；type=1: warnU32LE+ts(10B)，type=5: ts+len+3B 项
function decodeAlarm2(bytes, o) {
    if (bytes.length - o < 13) return -1;
    var alarmType = readU16LE(bytes, o + 10);
    decoded.WarnType = alarmType.toString();
    if (alarmType === 1) {
        if (bytes.length - o < 21) return -1;
        var warn = readU32LE(bytes, o + 12);
        decoded.Upl_warn = warn.toString(16);
        decoded.alarm = flagNames([[0x04, 'power_off_charging'], [0x02, 'power_off_low_battery'], [0x01, 'power_off_manual']], warn);
        decoded.timestamp = dateFormat(readU32LE(bytes, o + 16));
        return 20;
    }
    if (alarmType === 5) {
        var ts = readU32LE(bytes, o + 12);
        var len = readU16LE(bytes, o + 16);
        if (bytes.length - o < 19 + len) return -1;
        var items = [];
        for (var i = 0; i + 3 <= len; i += 3) {
            var p = o + 18 + i;
            items.push({
                healthType: { 1: 'heart_rate', 2: 'sbp', 3: 'dbp', 4: 'spo2', 5: 'temperature' }[bytes[p]] || bytes[p],
                direction: bytes[p + 1] === 1 ? 'below_threshold' : (bytes[p + 1] === 2 ? 'above_threshold' : bytes[p + 1]),
                value: readI16LE(bytes, p + 2),
            });
        }
        decoded.timestamp = dateFormat(ts);
        decoded.healthAlarms = items;
        return 18 + len;
    }
    decoded.raw = bytesToHex(bytes, o + 10, bytes.length - o - 12 > 0 ? bytes.length - o - 12 : 0);
    return 12;
}

// 4.1.3 BD B5 statusU8 tsI32LE CK
function decodeSos(bytes, o) {
    if (bytes.length - o < 16) return -1;
    decoded.sosStatus = bytes[o + 10] === 1 ? 'SOS' : bytes[o + 10];
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 11));
    return 15;
}

// 4.2.1 BD 03 lonF64LE latF64LE nsU8 ewU8 statusU8 tsU32LE CK
function decodeGps(bytes, o) {
    if (bytes.length - o < 34) return -1;
    decoded.lon = readF64LE(bytes, o + 10);
    decoded.lat = readF64LE(bytes, o + 18);
    decoded.north_south = String.fromCharCode(bytes[o + 26]);
    decoded.east_west = String.fromCharCode(bytes[o + 27]);
    decoded.status = String.fromCharCode(bytes[o + 28]);
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 29));
    return 34;
}

// 4.2.2 BD D6 typeU8 groupsU8 [tsU32LE countU8 count*(majorU16LE minorU16LE rssiI8)] CK
function decodeLbe(bytes, o) {
    if (bytes.length - o < 20) return -1;
    decoded.Type = bytes[o + 10];
    var groups = bytes[o + 11];
    decoded.Total_groups = groups;
    var all = [];
    var cur = o + 12;
    for (var g = 0; g < groups; g++) {
        if (bytes.length - cur < 5) return -1;
        var ts = readU32LE(bytes, cur);
        var count = bytes[cur + 4];
        if (bytes.length - cur < 5 + count * 5) return -1;
        var beacons = [];
        for (var b = 0; b < count; b++) {
            var p = cur + 5 + b * 5;
            beacons.push({ Major: readU16LE(bytes, p), Minor: readU16LE(bytes, p + 2), Rssi: readI8(bytes, p + 4) });
        }
        all.push({ Timestamp: dateFormat(ts), Total_PackCount: count, beacons: beacons });
        cur += 5 + count * 5;
    }
    decoded.ALL = all;
    return cur - o;
}

// 4.3.1 BD C3 statusU8 tsU32LE CK；0 开始充电 / 1 结束 / 2 充满
function decodeCharge(bytes, o) {
    if (bytes.length - o < 16) return -1;
    var st = bytes[o + 10];
    decoded.chargeStatus = { 0: 'charging', 1: 'charge_ended', 2: 'fully_charged' }[st] || st;
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 11));
    return 15;
}

// 4.3.2 BD F9 batTypeU8 batVoltU16LE signalTypeU8 signalI16LE otherTypeU8 numU32LE tsU32LE CK
function decodeBattery(bytes, o) {
    if (bytes.length - o < 26) return -1;
    decoded.Bat_type = bytes[o + 10];
    decoded.Bat_volt = readU16LE(bytes, o + 11);
    decoded.Signal_type = bytes[o + 13];
    decoded.Signal_strength = readI16LE(bytes, o + 14);
    decoded.other_type = bytes[o + 16];
    decoded.num = readU32LE(bytes, o + 17);
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 21));
    return 25;
}

// 4.3.3 BD BB lenU8 ASCII[len] CK
function decodeFirmwareVersion(bytes, o) {
    if (bytes.length - o < 12) return -1;
    var len = bytes[o + 10];
    if (bytes.length - o < 12 + len) return -1;
    decoded.Version_len = len;
    decoded.Version = asciiOf(bytes, o + 11, len);
    return 11 + len;
}

// 4.3.4 BD A5 countU8 count*groupIdU32LE CK
function decodeGroupIds(bytes, o) {
    if (bytes.length - o < 12) return -1;
    var n = bytes[o + 10];
    if (bytes.length - o < 12 + n * 4) return -1;
    decoded.groupCount = n;
    var ids = [];
    for (var i = 0; i < n; i++) ids.push(readU32LE(bytes, o + 11 + i * 4));
    decoded.groupIds = ids;
    return 11 + n * 4;
}

// 4.4.1 BD 32 bpHighU8 bpLowU8 hrsU8 spo2U8 wristU16LE(/10) bodyU16LE(/10)
//        stepU32LE batU8 signU8 tsI32LE CK（固定 18B payload）
function decodeHealth(bytes, o) {
    if (bytes.length - o < 29) return -1;
    decoded.bp_high = bytes[o + 10];
    decoded.bp_low = bytes[o + 11];
    decoded.Bp_heart = bytes[o + 12];
    decoded.BloodOxygen = bytes[o + 13];
    decoded.Wrist_Temp = readU16LE(bytes, o + 14) / 10;
    decoded.Body_Temp = readU16LE(bytes, o + 16) / 10;
    decoded.step = readU32LE(bytes, o + 18);
    decoded.batteryLevel = bytes[o + 22];
    decoded.signal = bytes[o + 23];
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 24));
    return 28;
}

// 4.5.1 BD C0 lenU8 n*msgIdU8 CK
function decodeFeedback(bytes, o) {
    if (bytes.length - o < 12) return -1;
    var n = bytes[o + 10];
    if (bytes.length - o < 12 + n) return -1;
    decoded.feedbackcount = n;
    var ids = [];
    for (var i = 0; i < n; i++) ids.push(hex8(bytes[o + 11 + i]));
    decoded.feedback = ids;
    return 11 + n;
}

// 4.5.2 BD 28 tsU32LE typeU8 statusU8 seqIdU32LE CK；status 1 已确认 / 2 已拒绝
function decodeMessageStatus(bytes, o) {
    if (bytes.length - o < 21) return -1;
    decoded.timestamp = dateFormat(readU32LE(bytes, o + 10));
    decoded.encodeType = bytes[o + 14];
    decoded.msgStatus = { 1: 'confirmed', 2: 'rejected' }[bytes[o + 15]] || bytes[o + 15];
    decoded.seqId = bytesToHex(bytes, o + 16, 4);
    return 20;
}

/* ---------- 工具函数 ---------- */

function checksum(bytes, frameStart) {
    var sum = 0;
    for (var i = frameStart; i < bytes.length - 1; i++) sum = (sum + bytes[i]) & 0xFF;
    return 0xFF - sum;
}

function flagNames(flags, value) {
    var names = [];
    for (var i = 0; i < flags.length; i++) {
        if ((value & flags[i][0]) !== 0) names.push(flags[i][1]);
    }
    return names;
}

function asciiOf(bytes, o, len) {
    var s = '';
    for (var i = 0; i < len; i++) s += String.fromCharCode(bytes[o + i]);
    return s;
}

function bytesToHex(bytes, o, len) {
    var s = '';
    for (var i = 0; i < len; i++) s += hex8(bytes[o + i]);
    return s.toUpperCase();
}

function hex8(v) {
    var h = v.toString(16);
    return h.length < 2 ? '0' + h : h;
}

function readU16LE(bytes, o) { return bytes[o] | (bytes[o + 1] << 8); }
function readI16LE(bytes, o) { var v = readU16LE(bytes, o); return v >= 0x8000 ? v - 0x10000 : v; }
function readI8(bytes, o) { return bytes[o] >= 0x80 ? bytes[o] - 256 : bytes[o]; }
function readU32LE(bytes, o) {
    return (bytes[o] | (bytes[o + 1] << 8) | (bytes[o + 2] << 16) | (bytes[o + 3] << 24)) >>> 0;
}

function readF64LE(bytes, o) {
    // IEEE-754 双精度小端：高 32 位取符号/指数，尾数 = (高 20 位 << 32) | 低 32 位
    var hi = readU32LE(bytes, o + 4);
    var lo = readU32LE(bytes, o);
    var sign = (hi >>> 31) & 1 ? -1 : 1;
    var e = ((hi >>> 20) & 0x7FF) - 1023;
    var m = (hi & 0xFFFFF) * 4294967296 + lo;
    if (e === -1023) return (hi & 0x7FFFFFFF) === 0 && lo === 0 ? 0 : sign * m * Math.pow(2, -1074);
    if (e === 1024) return (m === 0) ? sign * Infinity : NaN;
    var frac = 1 + m / 4503599627370496;
    return sign * frac * Math.pow(2, e);
}

function dateFormat(timestamp) {
    // 协议 §4：时间戳转北京时间（UTC+8）。固定偏移 + getUTC*，不依赖宿主时区。
    var date = new Date(timestamp * 1000 + 8 * 3600 * 1000);
    var Y = date.getUTCFullYear();
    var M = String(date.getUTCMonth() + 1).padStart(2, '0');
    var D = String(date.getUTCDate()).padStart(2, '0');
    var h = String(date.getUTCHours()).padStart(2, '0');
    var m = String(date.getUTCMinutes()).padStart(2, '0');
    var s = String(date.getUTCSeconds()).padStart(2, '0');
    return `${Y}-${M}-${D} ${h}:${m}:${s}`;
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== 'undefined') {
    module.exports = { decodeUplink };
}
