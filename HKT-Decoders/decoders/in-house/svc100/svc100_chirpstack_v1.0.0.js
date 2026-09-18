/**
 * Payload Decoder — 电磁阀控制器 SVC100（ChirpStack v4）
 *
 * Copyright 2026 HKT SmartHard
 *
 * @product SVC100（电磁阀控制器，LoRaWAN_Solenoid_Valve_Controller 固件）
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_Solenoid_Valve_Controller
 *   USER/Drive/communicate.c setDataPackage + sendLoRaWANData + Device_PeriodicReport
 *   + Device_ReportLocalTaskData + fromLoRaWANDataHandle；USER/config.h（ref 41ad0d9，
 *   2026-09-18 对照固件核验，对应发行版 SVC100_EU868_V13.14_26.9.8）
 * 帧格式（模组透传，整帧即 LoRaWAN 应用负载）：
 *   68 6B 74 | 特殊类型 | 包序号 | TLV...
 *   - 同步头恒为 68 6B 74（SYNC_HEAD）
 *   - 上行特殊类型恒为 0x00（sendLoRaWANData: sendData[3]=0；bit0/bit1 仅下行应答/校验语义）
 *   - 包序号 packSyncNumber 每次 +1，u8 回绕
 *   - TLV = 类型(1B) + 值；上行无校验和（CalCheckSum 仅用于下行接收方向）
 * fPort：固件默认 LoRaWAN_DEFAULT_PORT=10（config.h），解码器不校验端口
 *
 * 上行 TLV 表（固件实际会在 LoRaWAN 上行发送的全部类型）：
 *   0x01 软硬件版本      值 2B：硬件版本、软件版本（0x0D/0x0E，即 V13.14）
 *   0x03 电量            值 1B：0~100 百分比
 *   0x3C 设备状态        值 8B：阀1状态、接口1连接、脉冲1(u16 大端)、阀2状态、接口2连接、脉冲2(u16 大端)
 *   0x3D 本地任务上报    值 1+n×10B：任务数(1~4) + 每任务[id,阀号,状态,脉冲(u16 大端),起始时,起始分,结束时,结束分,重复周日掩码]
 *   0x40 输出电压等级    值 1B：0=12V 1=9V 2=5V
 *   0x41 接口功能        值 1B：port_mode 位掩码（下行校验仅 bit3~bit6 非法）
 *   0x42 接口稳定时长    值 1B：秒
 *   0x43 自动开关机      值 1B：0=手动 1=自动
 *   0x86 数据同步周期    值 2B：分钟，u16 大端（10~1440）
 *   0x8A 时区            值 1B：25=UTC+3.5 26=UTC+5.5 0~12=东N区 13~23=西(v-12)区
 *   0xFF 通讯应答 ACK    值 1B：恒 0xFF（下行特殊类型 bit0=1 时的应答帧，整帧仅此一条 TLV）
 * 固件怪癖：
 *   - 0x02 设备 ID、0x8D 开关机状态仅在蓝牙通道组包（callback_BLEQuery/callback_BLEPower），
 *     0x80/0x85 为纯下行类型，均不会出现在 LoRaWAN 上行，解码器按未知类型拒绝；
 *   - 本固件上行无 0 字节值 TLV；家族怪例中的"只发类型字节"出现在下行方向
 *     （服务器发单字节 0x3D 即请求任务上报，见 fromLoRaWANDataHandle dataLen==1 分支）；
 *   - 0x3D 每帧最多 4 个任务（Device_ReportLocalTaskData 每 4 条冲刷一次缓冲），
 *     任务数为 0 时固件不入队、不发帧；
 *   - 0x3C 中阀状态/接口连接为位域 u8，脉冲计数为 u16 大端（quickSplitShort 高字节在前）。
 * ── 修改日志 ─────────────────────────────────────────────
 * 2026-09-18 v1.0.0 首版：按固件 setDataPackage 全量实现 11 种实际上行 TLV，
 *   组合帧（周期上报 9×TLV）与单 TLV 帧均支持；坏同步头/特殊类型非 0/未知类型/
 *   TLV 截断/0x3D 任务数越界/0xFF 应答值异常一律返回 {data:{},errors:[...]}。
 * ─────────────────────────────────────────────────────────
 */
'use strict';

function decodeUplink(input) {
    var b = input.bytes;
    if (!b || b.length === 0) {
        return err("empty payload");
    }
    if (b.length < 7) {
        return err("frame too short (" + b.length + " bytes)，最小帧结构为 同步头3+特殊类型1+包序号1+TLV(类型1+值1)=7 字节");
    }
    if (b[0] !== 0x68 || b[1] !== 0x6B || b[2] !== 0x74) {
        return err("invalid sync head " + hex3(b[0], b[1], b[2]) + "（固件同步头恒为 68 6B 74）");
    }
    if (b[3] !== 0x00) {
        return err("invalid special type byte 0x" + hex8(b[3]) + "（上行恒为 0x00，bit0/bit1 应答与校验语义仅存在于下行）");
    }

    var out = { seq: b[4] };
    var i = 5;
    while (i < b.length) {
        var type = b[i++];
        switch (type) {
            case 0x01: // 设备软硬件版本
                if (!need(b, i, 2, type)) return err(lastErr);
                out.hardwareVersion = b[i];
                out.softwareVersion = b[i + 1];
                i += 2;
                break;
            case 0x03: // 电量百分比
                if (!need(b, i, 1, type)) return err(lastErr);
                out.battery = b[i];
                i += 1;
                break;
            case 0x3C: // 设备状态（双阀）
                if (!need(b, i, 8, type)) return err(lastErr);
                out.valve1State = b[i];
                out.insert1Connected = b[i + 1];
                out.pulse1Count = readUInt16(b, i + 2);
                out.valve2State = b[i + 4];
                out.insert2Connected = b[i + 5];
                out.pulse2Count = readUInt16(b, i + 6);
                i += 8;
                break;
            case 0x3D: // 本地定时任务上报（应服务器单字节 0x3D 查询）
                if (!need(b, i, 1, type)) return err(lastErr);
                var taskCount = b[i];
                if (taskCount === 0) return err("TLV 0x3D 任务数为 0（固件 task_num=0 时不出帧）");
                if (taskCount > 4) return err("TLV 0x3D 任务数 " + taskCount + " 超过固件单帧上限 4");
                if (!need(b, i + 1, taskCount * 10, type)) return err(lastErr);
                var tasks = [];
                for (var t = 0; t < taskCount; t++) {
                    var o = i + 1 + t * 10;
                    tasks.push({
                        id: b[o],
                        valve: b[o + 1],
                        state: b[o + 2],
                        pulseCount: readUInt16(b, o + 3),
                        startHour: b[o + 5],
                        startMin: b[o + 6],
                        endHour: b[o + 7],
                        endMin: b[o + 8],
                        repeatDuty: b[o + 9] // 位掩码：bit0=周一 … bit6=周日
                    });
                }
                out.taskCount = taskCount;
                out.tasks = tasks;
                i += 1 + taskCount * 10;
                break;
            case 0x40: // 输出电压等级
                if (!need(b, i, 1, type)) return err(lastErr);
                out.volLevel = b[i]; // 0=12V 1=9V 2=5V
                i += 1;
                break;
            case 0x41: // 接口功能
                if (!need(b, i, 1, type)) return err(lastErr);
                out.portMode = b[i];
                i += 1;
                break;
            case 0x42: // 接口稳定时长（秒）
                if (!need(b, i, 1, type)) return err(lastErr);
                out.stableTime = b[i];
                i += 1;
                break;
            case 0x43: // 自动开关机
                if (!need(b, i, 1, type)) return err(lastErr);
                out.powerMode = b[i];
                i += 1;
                break;
            case 0x86: // 数据同步周期（分钟，u16 大端）
                if (!need(b, i, 2, type)) return err(lastErr);
                out.reportInterval = readUInt16(b, i);
                i += 2;
                break;
            case 0x8A: // 时区
                if (!need(b, i, 1, type)) return err(lastErr);
                var tz = b[i];
                if (tz > 26) return err("TLV 0x8A 时区编码 0x" + hex8(tz) + " 非法（固件编码域为 0~26）");
                if (tz === 25) out.timezone = 3.5;
                else if (tz === 26) out.timezone = 5.5;
                else if (tz < 13) out.timezone = tz;        // 东 0~12 区
                else out.timezone = -(tz - 12);             // 13→西1区 … 23→西11区
                i += 1;
                break;
            case 0xFF: // 通讯应答 ACK
                if (!need(b, i, 1, type)) return err(lastErr);
                if (b[i] !== 0xFF) return err("TLV 0xFF 应答值 0x" + hex8(b[i]) + " 异常（固件恒回填 0xFF）");
                out.ack = true;
                i += 1;
                break;
            default:
                return err("unknown TLV type 0x" + hex8(type) + " at offset " + (i - 1) +
                    "（0x02/0x8D 仅蓝牙通道使用，0x80/0x85 仅下行，固件不会在 LoRaWAN 上行发送）");
        }
    }
    return { data: out };
}

var lastErr = "";

/** 校验 TLV 值区完整，不完整时写入截断错误信息 */
function need(b, pos, n, type) {
    if (pos + n > b.length) {
        lastErr = "TLV 0x" + hex8(type) + " 值区截断：需要 " + n + " 字节，实际仅剩 " + (b.length - pos) + " 字节";
        return false;
    }
    return true;
}

/** u16 大端（固件 quickSplitShort：高字节在前） */
function readUInt16(b, i) {
    return ((b[i] << 8) | b[i + 1]) & 0xFFFF;
}

function hex8(n) {
    return (n < 16 ? "0" : "") + n.toString(16);
}

function hex3(a, c, d) {
    return hex8(a) + " " + hex8(c) + " " + hex8(d);
}

function err(msg) {
    return { data: {}, errors: [msg] };
}
