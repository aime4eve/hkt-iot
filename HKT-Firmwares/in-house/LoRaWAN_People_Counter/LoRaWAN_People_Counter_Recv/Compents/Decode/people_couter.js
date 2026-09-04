/**
 * Payload Decoder for The Reports
 *
 * Copyright 2022 HKT SmartHard
 *
 * @product HKT-IRC01
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2022    初版。
 * 2026-09-04 防御性修复（对照固件 communicate.c/communicate.h）：
 *   1. 修复截断帧死循环：原 while(dataLen--) 在 dataLen 为负时永不退出，
 *      改为 i < bytes.length + TLV_LENGTHS 长度表前置越界检查；
 *   2. 修复 byteToUint32 符号溢出：追加 >>> 0，累计人数不再变负数；
 *   3. 修复未知 TLV 类型失步：查表失败记录 error 并停止解析；
 *   4. 修复越界静默错值：不完整 TLV 报 error，不输出字段；
 *   5. 补齐固件会发但缺失的类型 0x02/0x82/0x84/0x86/0xFF；
 *   6. 新增输出 ackRequested（flags bit0）与 seqNo（包序号，防重复计数）；
 *   7. 坏帧头/空输入恒返回对象（原裸 return undefined）；
 *   8. 删除死代码 hexToString。字段名保持原拼写（兼容平台消费方）。
 * ─────────────────────────────────────────────────────────
 * 帧格式（与固件 communicate.c 的 sendLoRaWANData 对应）：
 *   [0x68 0x6B 0x74][flags][seqNo][TLV...]
 *   flags: bit0 = 服务器要求应答(ACK)
 *   TLV:   1 字节类型 + 定长值（无长度字段，类型长度表见 TLV_LENGTHS）
 *
 * 防御性约束：
 *   - 任何 TLV 越过帧尾 => 返回 error，绝不越界/死循环
 *   - 未知类型无法定位下一个 TLV => 记录 error 并停止解析
 *   - 多字节整数一律无符号
 */
var TLV_LENGTHS = {
    0x01: 2,  // hardware_ver + software_ver
    0x02: 6,  // DevEUI
    0x03: 1,  // battery
    0x04: 1,  // IR report mode
    0x05: 2,  // IR report interval
    0x06: 2,  // threshold of total number of users
    0x07: 12, // counterA/counterB (u16) + totalCounterA/totalCounterB (u32)
    0x82: 1,  // work mode
    0x83: 1,  // fault status
    0x84: 1,  // tamper state
    0x86: 2,  // sync/report interval
    0xFF: 1,  // ACK response
};

function Decoder(bytes, port) {

    var decoded = {};

    if (!bytes || bytes.length < 5 || !checkReportSync(bytes)) {
        decoded.error = "invalid frame header";
        return decoded;
    }

    decoded.ackRequested = (bytes[3] & 0x01) == 1;
    decoded.seqNo = bytes[4];

    var i = 5;
    while (i < bytes.length) {
        var type = bytes[i];
        i++;
        var len = TLV_LENGTHS[type];
        if (len === undefined) {
            decoded.error = "unknown TLV type 0x" + type.toString(16) + " at offset " + (i - 1) + ", stop parsing";
            return decoded;
        }
        if (i + len > bytes.length) {
            decoded.error = "truncated TLV type 0x" + type.toString(16) + ", need " + len + " byte(s), got " + (bytes.length - i);
            return decoded;
        }
        switch (type) {
            case 0x01: //software_ver and hardware_ver
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                break;
            case 0x02: // device ID (DevEUI)
                decoded.devEUI = bytesToHex(bytes.slice(i, i + 6));
                break;
            case 0x03:// battery
                decoded.battery = bytes[i];
                break;
            case 0x04:// IR report mode
                decoded.IRreportMode = bytes[i];
                break;
            case 0x05:// IR report intervel
                decoded.IRreportIntervel = byteToUint16(bytes.slice(i, i + 2));
                break;
            case 0x06:// Threshold of total number of users
                decoded.totalNumber = byteToUint16(bytes.slice(i, i + 2));
                break;
            case 0x07:// people counter report
                decoded.counterA = byteToUint16(bytes.slice(i, i + 2));
                decoded.counterB = byteToUint16(bytes.slice(i + 2, i + 4));
                decoded.totalCounterA = byteToUint32(bytes.slice(i + 4, i + 8));
                decoded.totalCounterB = byteToUint32(bytes.slice(i + 8, i + 12));
                break;
            case 0x82:// work mode
                decoded.workMode = bytes[i];
                break;
            case 0x83:// fault status
                decoded.faultStatus = bytes[i];
                break;
            case 0x84:// tamper state
                decoded.tamperState = bytes[i];
                break;
            case 0x86:// report interval
                decoded.reportInterval = byteToUint16(bytes.slice(i, i + 2));
                break;
            case 0xFF:// ACK response
                decoded.ackResponse = bytes[i];
                break;
        }
        i += len;
    }
    return decoded;
}


function byteToUint16(bytes) {
    var value = (bytes[0] << 8) | bytes[1];
    return value;
}

function byteToUint32(bytes) {
    var value = ((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | (bytes[3] << 0)) >>> 0;
    return value;
}

function bytesToHex(bytes) {
    var value = "";
    for (var i = 0; i < bytes.length; i++) {
        var hex = bytes[i].toString(16);
        value += hex.length < 2 ? "0" + hex : hex;
    }
    return value;
}

function checkReportSync(bytes) {
    if (bytes[0] == 0x68 && bytes[1] == 0x6B && bytes[2] == 0x74) {
        return true;
    }
    return false;
}


var info_report = [0x68, 0x6B, 0x74, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x64, 0x86, 0x01, 0xE1, 0x04, 0x01, 0x05, 0x00, 0x1E, 0x06, 0x00, 0x64, 0x83, 0x00];
var data_report = [0x68, 0x6B, 0x74, 0x00, 0x59, 0x07, 0x00, 0x33, 0x00, 0x22, 0x00, 0x00, 0x12, 0x22, 0x00, 0x00, 0x22, 0x22];

console.log(Decoder(info_report, 10))
console.log(Decoder(data_report, 10))

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { Decoder, byteToUint16, byteToUint32, bytesToHex, checkReportSync };
}
