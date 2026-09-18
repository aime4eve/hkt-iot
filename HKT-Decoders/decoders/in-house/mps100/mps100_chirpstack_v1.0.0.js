/**
 * Payload Decoder — 地磁车位传感器 MPS100/EPS100（ChirpStack v4）
 *
 * Copyright 2026 HKT SmartHard
 *
 * @product MPS100（主型号）/ EPS100（同固件别名）
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_ParkingSensor
 *   USER/Drive/communicate.c 的 setDataPackage / sendLoRaWANData / Device_PeriodicReport /
 *   DeviceEvent_Process / fromLoRaWANDataHandle（ref 41ad0d9，2026-09-18 对照核验）
 *   USER/Drive/include/communicate.h 的 enum DataType；USER/config.h 常量。
 *
 * 帧格式：[0x68 0x6B 0x74][特殊类型固定 0x00][seq=packSyncNumber++][TLV...]
 *   TLV = 1 字节类型 + 定长值（长度与编码见 TLV_LENGTHS，全部大端）
 * fPort：固件 LoRaWAN_DEFAULT_PORT=10（模组侧可改，解码器不校验端口）
 *
 * 实际会上行的 TLV（逐调用方核对）：
 *   - Device_PeriodicReport（周期到点 / 车位状态变化置 sync_state）：
 *       0x01 版本 + 0x03 电量 + 0x3A 车位状态 + 0x3B 模式 + 0x84 防拆 + 0x86 周期，固定顺序合为一帧；
 *   - DeviceEvent_Process 的 sync_battery（电量变化/低电，adc.c 置位）：单发 0x03（固件重复入队两次，连发两帧）；
 *   - fromLoRaWANDataHandle（服务器帧 b3 bit0=1 要求应答）：单发 0xFF|0xFF ACK 帧。
 *   0x02/0x8D/0x5D/0x5E/0x5F/0x60 仅走 BLE 查询/调试通道（callback_BLEQuery/callback_BLEDebugData），
 *   不上 LoRaWAN，但 TLV 语法相同，解码器同样支持以防透传/后续固件纳入。
 *
 * 固件怪癖：0x80 时间同步请求 — setDataPackage 无该 case，走 default 分支 dataLen=1，
 *   只发类型字节不带值（0 字节值 TLV）；且本固件 build 中 sync_local_time 标志无置位方（死路径），
 *   保留解析兼容。0x85 恢复出厂同属无 case 枚举，仅下行语义。
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2026-09-18 v1.0.0 初版（依固件独立推导帧格式与黄金样例互证）。
 * ─────────────────────────────────────────────────────────
 */
var TLV_LENGTHS = {
    0x01: 2,  // 软硬件版本：hard_ver + soft_ver（本 build 0x0B/0x1C）
    0x02: 6,  // 设备 ID：DevEUI[2..7]（仅 BLE 通道发送）
    0x03: 1,  // 电量百分比 0~100
    0x3A: 1,  // 车位状态：0 无车 / 1 有车 / 255 异常或被水覆盖
    0x3B: 1,  // 检测模式：0 融合 / 1 仅磁力计 / 2 雷达优先
    0x80: 0,  // 时间同步请求（固件 quirk：只发类型字节本身）
    0x84: 1,  // 防拆状态：0 已安装 / 1 未安装
    0x85: 0,  // 恢复出厂（枚举存在但无封包分支，本固件从不上行）
    0x86: 2,  // 上报周期（分钟，u16 大端，0 或 1~1440，出厂默认 1440）
    0x8D: 1,  // 开关机状态：0 关 / 1 开（仅 BLE 通道发送）
    0x5D: 2,  // X 轴磁场原始值（int16 大端，仅 BLE 通道）
    0x5E: 2,  // Y 轴磁场原始值（仅 BLE 通道）
    0x5F: 2,  // Z 轴磁场原始值（仅 BLE 通道）
    0x60: 20, // 雷达频谱：10 × u16 大端（仅 BLE 通道）
    0xFF: 1,  // ACK 应答（固件固定写 0xFF）
};

function decodeUplink(input) {
    var bytes = input.bytes;
    if (!bytes || bytes.length < 5) {
        return err("frame too short (" + (bytes ? bytes.length : 0) + " bytes)，至少需要 5 字节同步头");
    }
    if (bytes[0] !== 0x68 || bytes[1] !== 0x6B || bytes[2] !== 0x74) {
        return err("invalid sync head " + hex3(bytes) + "，应为 68 6B 74");
    }
    if (bytes.length === 5) {
        return err("frame has no TLV payload");
    }
    var decoded = {};
    var i = 5;
    while (i < bytes.length) {
        var type = bytes[i];
        i++;
        var len = TLV_LENGTHS[type];
        if (len === undefined) {
            return err("unknown TLV type 0x" + hex2(type) + " at offset " + (i - 1) + ", stop parsing");
        }
        if (i + len > bytes.length) {
            return err("truncated TLV type 0x" + hex2(type) + ", need " + len + " byte(s), got " + (bytes.length - i));
        }
        switch (type) {
            case 0x01: // 软硬件版本
                decoded.hard_ver = bytes[i];
                decoded.soft_ver = bytes[i + 1];
                break;
            case 0x02: // 设备 ID（DevEUI 后 6 字节）
                decoded.dev_eui = readHex(bytes, i, 6);
                break;
            case 0x03: // 电量百分比
                decoded.battery = bytes[i];
                break;
            case 0x3A: // 车位状态
                decoded.park_state = bytes[i];
                break;
            case 0x3B: // 检测模式
                decoded.park_mode = bytes[i];
                break;
            case 0x84: // 防拆状态
                decoded.tamper_state = bytes[i];
                break;
            case 0x86: // 上报周期（分钟）
                decoded.report_interval = readUInt16(bytes, i);
                break;
            case 0x8D: // 开关机状态
                decoded.power_on = bytes[i];
                break;
            case 0x5D: // X 轴磁场原始计数
                decoded.mag_x = readInt16(bytes, i);
                break;
            case 0x5E: // Y 轴磁场原始计数
                decoded.mag_y = readInt16(bytes, i);
                break;
            case 0x5F: // Z 轴磁场原始计数
                decoded.mag_z = readInt16(bytes, i);
                break;
            case 0x60: // 雷达频谱 10 × u16
                decoded.radar_spectrum = [];
                for (var k = 0; k < 10; k++) decoded.radar_spectrum.push(readUInt16(bytes, i + k * 2));
                break;
            case 0xFF: // ACK 应答
                decoded.ack = bytes[i];
                break;
            case 0x80: // 时间同步请求（无值）
            case 0x85: // 恢复出厂（无值，本固件不会上行）
                break;
        }
        i += len;
    }
    return { data: decoded };
}

function err(msg) {
    return { data: {}, errors: [msg] };
}

function hex2(n) {
    return (n < 16 ? "0" : "") + n.toString(16);
}

function hex3(b) {
    return hex2(b[0]) + " " + hex2(b[1]) + " " + hex2(b[2]);
}

function readUInt16(b, i) {
    return ((b[i] << 8) | b[i + 1]) & 0xFFFF;
}

function readInt16(b, i) {
    var v = readUInt16(b, i);
    return v > 0x7FFF ? v - 0x10000 : v;
}

function readHex(b, i, n) {
    var s = "";
    for (var k = 0; k < n; k++) s += hex2(b[i + k]);
    return s;
}
