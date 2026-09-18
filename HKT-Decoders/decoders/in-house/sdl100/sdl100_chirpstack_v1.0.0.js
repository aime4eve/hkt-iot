/**
 * Payload Decoder for HKT SDL100/SDL200 智能门锁（ChirpStack）
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_SmartDoorLock（ref 41ad0d9）
 *   - 上行组包：USER/Drive/communicate.c → sendLoRaWANData + setDataPackage + Device_PeriodicReport
 *   - 类型枚举：USER/Drive/include/communicate.h → enum DataType
 *   - 编译开关：USER/config.h → FUNC_OPERATIONAL_VERSION_ENABLE=1（HARDWARE_VER=0x11, SOFTWARE_VER=0x19）
 *
 * 帧格式（fPort 固件默认 10 = LoRaWAN_DEFAULT_PORT，config.h 两个条件分支均为 10）：
 *   68 6B 74 | 02 | seq(u8, packSyncNumber++ 自增回绕) | TLV... | checksum
 *   - 同步头 "hkt"（SYNC_HEAD）
 *   - 特殊类型字节固定 0x02（sendLoRaWANData 中 txbuf[3]=2；bit1 同时表示启用校验）
 *   - checksum = 前面全部字节累加 mod 256（GetCheckSum，FUNC_OPERATIONAL_VERSION_ENABLE 才追加）
 *   - TLV = 类型字节 + 定长值；0x32 临时密码应答与 0x80 时间同步请求为 0 字节值（仅类型字节）
 *
 * 上行形态（ENABLE 版固件，lwrb 缓冲一条 = 一个空口帧）：
 *   1) 周期上报（Device_PeriodicReport，唯一组合帧）：FF <项数=10> + 固定顺序 10 个 TLV
 *      FF 0A | 01 版本(2) | 03 电量(1) | 8A 时区(1) | 57 音量(1) | 53 回锁(1) |
 *      52 常开模式(11) | 56 绑定(1) | 84 防拆(1) | 86 上报周期(2,大端) | 54 时间戳(4,大端)
 *   2) 单 TLV 帧：下行查询/设置应答或事件（01/02/03/2F/30/32/4E/4F/52/53/54/56/57/80/84/85/86/8A）
 *
 * 固件怪癖（均已按固件源码忠实解码）：
 *   - 0x32/0x80：setDataPackage 走 default 分支仅返回类型字节 → 0 字节值 TLV
 *   - 0x57 音量设置应答成功时固件硬编码回 0x50（并非实际音量值）；0x86/0x85 应答成功值为
 *     未赋值的 0x00（86 不回显间隔值）；0x4E/0x4F 删除/失败短格式带未清零的尾随 0x00
 *   - 0x53 自动回锁应答中 buffer[0]=DATATYPE_DEVICE_OPEN_MODE 的赋值被随后 memcpy 覆盖，
 *     实际上行仍为 0x53
 *   - 0x80 触发标志 syncLocalTime 在本快照内无赋值方（疑似预留/由蓝牙侧置位）
 *   - enum 中 0x2E/0x31/0x55/0x81 等类型在 ENABLE 版空口路径无发送方 → 解码器按未知类型拒收
 */
'use strict';

/* 时区原始字节 → UTC 偏移小时（setDataPackage/FLASH 互逆编码） */
function decodeTimezone(raw) {
  if (raw === 25) return 3.5;   // UTC+3.5
  if (raw === 26) return 5.5;   // UTC+5.5
  if (raw <= 12) return raw;    // 0..12 → 东区
  if (raw <= 24) return 12 - raw; // 13..24 → 西区 -1..-12
  return null;
}

function pad2(n) {
  return (n < 10 ? '0' : '') + n;
}

function hhmmss(h, m, s) {
  return pad2(h) + ':' + pad2(m) + ':' + pad2(s);
}

function isoTime(ts) {
  var d = new Date(ts * 1000);
  if (isNaN(d.getTime())) return null;
  return d.toISOString();
}

function toHex(bytes) {
  var s = '';
  for (var i = 0; i < bytes.length; i++) {
    s += (bytes[i] < 16 ? '0' : '') + bytes[i].toString(16).toUpperCase();
  }
  return s;
}

/* 4E 密码域：8 字节右对齐 ASCII（高位补 0x00） */
function decodePassword(word) {
  var start = 0;
  while (start < 8 && word[start] === 0) start++;
  var s = '';
  for (var i = start; i < 8; i++) {
    if (word[i] < 0x20 || word[i] > 0x7E) return toHex(word);
    s += String.fromCharCode(word[i]);
  }
  return s;
}

/* 常开模式 11 字节值：mode(1) lock_delay(2 大端) loop_duty(1) loop_cnt(1) start(3) over(3) */
function decodeOpenMode(v) {
  return {
    mode: v[0],
    lock_delay: v[1] * 256 + v[2],
    loop_duty: v[3],
    loop_count: v[4],
    loop_start: hhmmss(v[5], v[6], v[7]),
    loop_over: hhmmss(v[8], v[9], v[10]),
  };
}

var STATUS_TEXT = { 0: 'failed', 1: 'success', 2: 'deleted' };

/* 单 TLV 解码：返回字段对象（不含 seq），未知类型返回 null */
function decodeTlv(type, v) {
  switch (type) {
    case 0x01: // 软硬件版本（HARDWARE_VER=0x11, SOFTWARE_VER=0x19）
      return { frame_type: 'version_info', hardware_version: v[0], software_version: v[1] };
    case 0x02: // 设备 ID：DevEUI[2..7]
      return { frame_type: 'device_id', device_id: toHex(v) };
    case 0x03: // 电量百分比 0..100
      return { frame_type: 'battery_report', battery: v[0] };
    case 0x2F: // 远程开关锁执行结果（回显命令参数 0=开锁 1=关锁）
      return { frame_type: 'remote_unlock_result', action: v[0] === 0 ? 'unlock' : (v[0] === 1 ? 'lock' : null) };
    case 0x30: // 开锁记录：[0xC5, 用户号, 时间戳4B 大端]
      return {
        frame_type: 'unlock_record',
        record_code: v[0],
        record_type: v[0] === 0xC5 ? 'remote_unlock' : 'unknown',
        user_number: v[1],
        timestamp: v[2] * 16777216 + v[3] * 65536 + v[4] * 256 + v[5],
        time: isoTime(v[2] * 16777216 + v[3] * 65536 + v[4] * 256 + v[5]),
      };
    case 0x32: // 临时密码设置应答（0 字节值，收到即成功）
      return { frame_type: 'temp_password_ack', ok: true };
    case 0x4E: // 管理开锁密码(带时效)应答：短格式 [status,0x00] / 成功 [status,user,isable,len,word8,start4,end4,cnt2]
      if (v.length >= 22) {
        return {
          frame_type: 'password_key_ack',
          status: v[0],
          status_text: STATUS_TEXT[v[0]] || null,
          user_number: v[1],
          enabled: v[2] === 1,
          key_length: v[3],
          password: decodePassword(v.slice(4, 12)),
          start_timestamp: v[12] * 16777216 + v[13] * 65536 + v[14] * 256 + v[15],
          end_timestamp: v[16] * 16777216 + v[17] * 65536 + v[18] * 256 + v[19],
          valid_count: v[20] * 256 + v[21],
        };
      }
      return { frame_type: 'password_key_ack', status: v[0], status_text: STATUS_TEXT[v[0]] || null };
    case 0x4F: // 管理开锁卡片(带时效)应答：短格式 [status,0x00] / 成功 [status,user,isable,id4,key16,start4,end4,cnt2]
      if (v.length >= 33) {
        return {
          frame_type: 'card_key_ack',
          status: v[0],
          status_text: STATUS_TEXT[v[0]] || null,
          user_number: v[1],
          enabled: v[2] === 1,
          card_id: toHex(v.slice(3, 7)),
          card_key: toHex(v.slice(7, 23)),
          start_timestamp: v[23] * 16777216 + v[24] * 65536 + v[25] * 256 + v[26],
          end_timestamp: v[27] * 16777216 + v[28] * 65536 + v[29] * 256 + v[30],
          valid_count: v[31] * 256 + v[32],
        };
      }
      return { frame_type: 'card_key_ack', status: v[0], status_text: STATUS_TEXT[v[0]] || null };
    case 0x52: // 常开模式设置应答：参数非法为 [0xFE]，成功回显完整 11 字节
      if (v.length >= 11) {
        return { frame_type: 'open_mode_ack', ok: true, open_mode: decodeOpenMode(v) };
      }
      return { frame_type: 'open_mode_ack', ok: v[0] !== 0xFE };
    case 0x53: // 自动回锁时间应答：合法值 3..30，非法 0xFE
      return { frame_type: 'auto_lock_time_ack', ok: v[0] !== 0xFE, auto_lock_time: v[0] === 0xFE ? null : v[0] };
    case 0x54: // 时间戳（Unix 秒，大端）
      var ts = v[0] * 16777216 + v[1] * 65536 + v[2] * 256 + v[3];
      return { frame_type: 'timestamp_report', timestamp: ts, time: isoTime(ts) };
    case 0x56: // 绑定状态：周期上报 0x56=已绑定/0=未绑定；命令应答固定 0x01
      if (v[0] === 0x56) return { frame_type: 'bind_state', bound: true };
      if (v[0] === 0x00) return { frame_type: 'bind_state', bound: false };
      if (v[0] === 0x01) return { frame_type: 'bind_state', bound: null, ok: true };
      return { frame_type: 'bind_state', bound: null };
    case 0x57: // 音量设置应答：成功固件硬编码 0x50（非实际音量），非法 0xFE
      return { frame_type: 'volume_ack', ok: v[0] !== 0xFE, reported_value: v[0] };
    case 0x80: // 请求服务端同步本地时间（0 字节值）
      return { frame_type: 'time_sync_request' };
    case 0x84: // 防拆状态：0 正常 / 1 报警（拆开瞬间即发 84 01）
      return { frame_type: 'tamper_state', tamper_alarm: v[0] === 1 };
    case 0x85: // 恢复出厂应答：成功 0x00（固件未显式赋值）/ 失败 0xFE
      return { frame_type: 'factory_reset_ack', ok: v[0] !== 0xFE };
    case 0x86: // 上报周期设置应答：成功 0x00（固件不回显间隔值）/ 失败 0xFE
      return { frame_type: 'report_interval_ack', ok: v[0] !== 0xFE };
    case 0x8A: // 时区设置应答：成功回显编码值 / 非法 0xFE
      return { frame_type: 'timezone_ack', ok: v[0] !== 0xFE, timezone: v[0] === 0xFE ? null : decodeTimezone(v[0]), timezone_raw: v[0] };
    default:
      return null;
  }
}

/* 固件 Device_PeriodicReport 的固定 TLV 顺序（FUNC_OPERATIONAL_VERSION_ENABLE=1） */
var PERIODIC_ORDER = [
  { type: 0x01, len: 2 },  // 软硬件版本
  { type: 0x03, len: 1 },  // 电量
  { type: 0x8A, len: 1 },  // 时区
  { type: 0x57, len: 1 },  // 音量
  { type: 0x53, len: 1 },  // 自动回锁时间
  { type: 0x52, len: 11 }, // 常开模式
  { type: 0x56, len: 1 },  // 绑定状态
  { type: 0x84, len: 1 },  // 防拆状态
  { type: 0x86, len: 2 },  // 上报周期（大端，注意与单帧应答的 1 字节状态值不同）
  { type: 0x54, len: 4 },  // 时间戳
];

function decodeUplink(input) {
  var bytes = input && input.bytes;
  if (!Array.isArray(bytes)) {
    return { data: {}, errors: ['invalid input: bytes 必须是字节数组'] };
  }
  var n = bytes.length;
  // 最短合法帧 = 同步头3 + 特殊类型1 + seq1 + 类型字节1 + 校验和1 = 7 字节
  if (n < 7) {
    return { data: {}, errors: ['frame too short (' + n + ' bytes)，最短合法帧为 7 字节（同步头+类型+seq+TLV+校验和）'] };
  }
  if (bytes[0] !== 0x68 || bytes[1] !== 0x6B || bytes[2] !== 0x74) {
    return { data: {}, errors: ['invalid sync head（同步头必须为 68 6B 74）'] };
  }
  if (bytes[3] !== 0x02) {
    return { data: {}, errors: ['invalid special type 0x' + bytes[3].toString(16) + '（固件 sendLoRaWANData 固定发送 0x02）'] };
  }
  var sum = 0;
  for (var i = 0; i < n - 1; i++) sum += bytes[i];
  if ((sum % 256) !== bytes[n - 1]) {
    return { data: {}, errors: ['checksum mismatch（校验和应为 0x' + (sum % 256).toString(16) + '，实际 0x' + bytes[n - 1].toString(16) + '）'] };
  }

  var out = { seq: bytes[4] };
  var p = bytes.slice(5, n - 1); // TLV 区
  var pos = 0;

  /* 周期上报组合帧：FF <项数> + 固定顺序 TLV */
  if (p[0] === 0xFF) {
    if (p.length < 2) {
      return { data: {}, errors: ['truncated periodic header（FF 帧头缺少项数字节）'] };
    }
    var count = p[1];
    if (count !== PERIODIC_ORDER.length) {
      return { data: {}, errors: ['unexpected periodic type count ' + count + '（固件固定上报 ' + PERIODIC_ORDER.length + ' 项）'] };
    }
    pos = 2;
    out.frame_type = 'periodic_report';
    out.type_count = count;
    for (var k = 0; k < PERIODIC_ORDER.length; k++) {
      var item = PERIODIC_ORDER[k];
      if (pos + 1 + item.len > p.length) {
        return { data: {}, errors: ['truncated periodic TLV 0x' + item.type.toString(16) + '（需要 ' + (1 + item.len) + ' 字节，剩余 ' + (p.length - pos) + ' 字节）'] };
      }
      if (p[pos] !== item.type) {
        return { data: {}, errors: ['unexpected periodic TLV order: 期望 0x' + item.type.toString(16) + ' 实际 0x' + p[pos].toString(16) + '（固件 Device_PeriodicReport 顺序固定）'] };
      }
      var pv = p.slice(pos + 1, pos + 1 + item.len);
      if (item.type === 0x01) {
        out.hardware_version = pv[0];
        out.software_version = pv[1];
      } else if (item.type === 0x03) {
        out.battery = pv[0];
      } else if (item.type === 0x8A) {
        out.timezone = decodeTimezone(pv[0]);
        out.timezone_raw = pv[0];
      } else if (item.type === 0x57) {
        out.volume = pv[0];
      } else if (item.type === 0x53) {
        out.auto_lock_time = pv[0];
      } else if (item.type === 0x52) {
        out.open_mode = decodeOpenMode(pv);
      } else if (item.type === 0x56) {
        out.bound = pv[0] === 0x56;
      } else if (item.type === 0x84) {
        out.tamper_alarm = pv[0] === 1;
      } else if (item.type === 0x86) {
        out.report_interval = pv[0] * 256 + pv[1];
      } else if (item.type === 0x54) {
        var ts = pv[0] * 16777216 + pv[1] * 65536 + pv[2] * 256 + pv[3];
        out.timestamp = ts;
        out.time = isoTime(ts);
      }
      pos += 1 + item.len;
    }
    if (pos !== p.length) {
      return { data: {}, errors: ['trailing bytes after periodic frame（周期帧末尾多余 ' + (p.length - pos) + ' 字节）'] };
    }
    return { data: out, errors: [] };
  }

  /* 单 TLV（或多 TLV 顺序拼接）帧 */
  while (pos < p.length) {
    var type = p[pos];
    var vlen;
    if (type === 0x4E) {
      vlen = (p.length - pos - 1) >= 22 ? 22 : 2; // 成功回显 22 字节值，否则短格式
    } else if (type === 0x4F) {
      vlen = (p.length - pos - 1) >= 33 ? 33 : 2;
    } else if (type === 0x52) {
      vlen = (p.length - pos - 1) >= 11 ? 11 : 1;
    } else {
      var FIXED = {
        0x01: 2, 0x02: 6, 0x03: 1, 0x2F: 1, 0x30: 6, 0x32: 0, 0x53: 1, 0x54: 4,
        0x56: 1, 0x57: 1, 0x80: 0, 0x84: 1, 0x85: 1, 0x86: 1, 0x8A: 1,
      };
      if (FIXED[type] === undefined) {
        return { data: {}, errors: ['unknown TLV type 0x' + type.toString(16) + '（固件空口不会发送该类型）'] };
      }
      vlen = FIXED[type];
    }
    if (pos + 1 + vlen > p.length) {
      return { data: {}, errors: ['truncated TLV 0x' + type.toString(16) + '（值需要 ' + vlen + ' 字节，剩余 ' + (p.length - pos - 1) + ' 字节）'] };
    }
    var v = p.slice(pos + 1, pos + 1 + vlen);
    var part = decodeTlv(type, v);
    if (part === null) {
      return { data: {}, errors: ['unknown TLV type 0x' + type.toString(16) + '（固件空口不会发送该类型）'] };
    }
    if (out.frame_type === undefined) out.frame_type = part.frame_type;
    for (var key in part) {
      if (key !== 'frame_type' || out.frame_type === undefined) out[key] = part[key];
    }
    pos += 1 + vlen;
  }
  return { data: out, errors: [] };
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== 'undefined') {
  module.exports = { decodeUplink };
}
