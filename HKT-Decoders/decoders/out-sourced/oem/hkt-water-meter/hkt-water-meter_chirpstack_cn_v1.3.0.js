/**
 * HKT LoRaWAN 水表（214C / 213E）上报与阀控协议解码器
 * 协议依据：《214C and 213E water meter lorawan协议_0X1E_V101_20211231.pdf》
 *
 * 帧结构：[FE FE FE] 68 [T] [A0-A6] [C] [L] [DATA...] [CS] 16
 *   - FE FE FE 为前导（协议下行/应答帧示例均带，出现则剥离，最多 3 字节）
 *   - CS = 从 68 到数据域末字节的累加和低 8 位
 *   - AT 文本帧（协议 §2 配置通道）以 ASCII "AT" 开头，原样透传
 *
 * C=81 数据上报，数据域 30 字节（小端序 BCD，低字节在前）：
 *   DI0(1) DI1(1) SER(1)
 *   单位码(1)+正向累积流量(4) ×0.001 m³
 *   单位码(1)+反向累积流量(4) ×0.001 m³
 *   单位码(1)+瞬时流量(4)     ×0.0001 m³/h
 *   温度(3) ×0.01 ℃
 *   [扩展字节(1)]  仅 L=1F/DI=93 1F 的 31 字节真机帧存在（2026-10-21 实证），语义未文档化，原样输出 extraByte
 *   实时时间(7) 秒分时日月年(低前)
 *   状态字(2) 低字节=Table 3.1，高字节=Table 3.2
 *
 * C=84 阀控指令正常应答，数据域 5 字节：A0 17 SER ST0 ST1（ST 同上两表）
 * C=04 阀控指令（下行回显），数据域 4 字节：A0 17 SER OP（OP 见 VALVE_OPS）
 * 其它控制码：数据域原样输出 rawData 并附 warning（真机实证 C=24 心跳/状态帧，语义未文档化）
 *
 * 下行阀控（encodeDownlink，平台侧暂不执行，供 ChirpStack 下发使用）：
 *   FE FE FE 68 10 [A0-A6] 04 04 A0 17 01 [OP] [CS] 16
 *   OP：55 强制开阀 / 99 强制关阀 / FF 强制撤销 / 66 自由开阀 / 77 自由关阀
 */

var VALVE_OPS = {
  forceOpen:  0x55,
  forceClose: 0x99,
  forceUndo:  0xFF,
  freeOpen:   0x66,
  freeClose:  0x77
};

var VALVE_OP_NAMES = {
  0x55: ['forceOpen',  '强制开阀'],
  0x99: ['forceClose', '强制关阀'],
  0xFF: ['forceUndo',  '强制撤销'],
  0x66: ['freeOpen',   '自由开阀'],
  0x77: ['freeClose',  '自由关阀']
};

function decodeBCDLE(bytes) {
  const reversed = [...bytes].reverse();
  let str = '';
  for (const b of reversed) {
    const high = (b >> 4) & 0x0f;
    const low = b & 0x0f;
    if (high > 9 || low > 9) {
      throw new Error('BCD 数据非法，存在非十进制半字节');
    }
    str += (b >> 4).toString(16) + (b & 0x0f).toString(16);
  }
  return str;
}

function decodeScaledBCDLE(bytes, scale) {
  return parseInt(decodeBCDLE(bytes), 10) * scale;
}

function decodeDatetimeBCDLE(bytes) {
  const timeBcd = decodeBCDLE(bytes);
  const year = parseInt(timeBcd.slice(0, 4), 10);
  const month = parseInt(timeBcd.slice(4, 6), 10);
  const day = parseInt(timeBcd.slice(6, 8), 10);
  const hour = parseInt(timeBcd.slice(8, 10), 10);
  const minute = parseInt(timeBcd.slice(10, 12), 10);
  const second = parseInt(timeBcd.slice(12, 14), 10);
  return `${year}-${String(month).padStart(2, '0')}-${String(day).padStart(2, '0')} ` +
         `${String(hour).padStart(2, '0')}:${String(minute).padStart(2, '0')}:${String(second).padStart(2, '0')}`;
}

function hex2(b) {
  return b.toString(16).padStart(2, '0');
}

// 状态字：st0 = Table 3.1（第一字节），st1 = Table 3.2（第二字节），文案按协议表格原文直译
function parseStatus(st0, st1) {
  return {
    valve: (st0 & 0x01) ? '阀门关' : '阀门开',
    battery: (st0 & 0x02) ? '欠压' : '正常',
    pipeBurst: (st0 & 0x04) ? '故障' : '正常',
    pipeLeak: (st0 & 0x08) ? '故障' : '正常',
    tempSensor: (st0 & 0x10) ? '故障' : '正常',
    flowSensor: (st0 & 0x20) ? '故障' : '正常',
    installDir: (st0 & 0x40) ? '反向' : '正向',
    openValveMode: (st1 & 0x01) ? '正常开' : '异常开',
    closeValveMode: (st1 & 0x02) ? '正常关' : '异常关',
    externalPower: (st1 & 0x04) ? '欠压' : '异常',
    openValveError: (st1 & 0x08) ? '故障' : '正常',
    closeValveError: (st1 & 0x10) ? '故障' : '正常',
    forcedOpen: (st1 & 0x20) ? '强制开' : '非强制',
    forcedClose: (st1 & 0x40) ? '强制关' : '非强制',
    externalPowerSupply: (st1 & 0x80) ? '欠压' : '正常',
    rawHex: '0x' + hex2(st0) + hex2(st1)
  };
}

function ensureAvailable(source, offset, size, fieldName) {
  if (offset + size > source.length) {
    throw new Error(`${fieldName} 长度不足`);
  }
}

function stripPreamble(buf) {
  let start = 0;
  while (start < 3 && start < buf.length && buf[start] === 0xFE) start++;
  return start ? buf.slice(start) : buf;
}

function parseAtText(buf) {
  let text = '';
  for (const b of buf) text += (b >= 0x20 && b <= 0x7e) ? String.fromCharCode(b) : '\\x' + hex2(b);
  return {
    messageType: 'AT 文本',
    text
  };
}

function parseMeterData(data) {
  let di = 0;
  const take = (size, fieldName) => {
    ensureAvailable(data, di, size, fieldName);
    const part = data.slice(di, di + size);
    di += size;
    return part;
  };

  const di0 = take(1, '数据标识 DI0')[0];
  const di1 = take(1, '数据标识 DI1')[0];
  const serialNumber = take(1, '序列号')[0];
  take(1, '正向流量单位');
  const forwardFlow = decodeScaledBCDLE(take(4, '正向累积流量'), 0.001);
  take(1, '反向流量单位');
  const reverseFlow = decodeScaledBCDLE(take(4, '反向累积流量'), 0.001);
  take(1, '瞬时流量单位');
  const instantFlow = decodeScaledBCDLE(take(4, '瞬时流量'), 0.0001);
  const temperature = decodeScaledBCDLE(take(3, '温度'), 0.01);

  // 温度后应剩 时间7+状态2=9 字节；31 字节真机帧多 1 个未文档化字节，按剩余长度分流
  const remaining = data.length - di;
  let extraByte = null;
  if (remaining === 10) {
    extraByte = take(1, '扩展字节')[0];
  } else if (remaining !== 9) {
    throw new Error(`温度后剩余 ${remaining} 字节，无法识别帧型（期望 9 或 10）`);
  }

  const datetime = decodeDatetimeBCDLE(take(7, '实时时间'));
  const st0 = take(1, '状态字第一字节')[0];
  const st1 = take(1, '状态字第二字节')[0];

  if (di !== data.length) {
    throw new Error(`数据域未完全消费，剩余 ${data.length - di} 字节`);
  }

  const out = {
    dataIdentifier: '0x' + hex2(di0) + hex2(di1),
    serialNumber,
    forwardFlow:  { value: forwardFlow,  unit: 'm³' },
    reverseFlow:  { value: reverseFlow,  unit: 'm³' },
    instantFlow:  { value: instantFlow,  unit: 'm³/h' },
    temperature:  { value: temperature,  unit: '℃' }
  };
  if (extraByte !== null) out.extraByte = '0x' + hex2(extraByte);
  out.datetime = datetime;
  out.status = parseStatus(st0, st1);
  return out;
}

function parseValveAck(data) {
  if (data.length !== 5) {
    throw new Error(`阀控应答帧数据域长度异常，期望 5 字节，实际 ${data.length} 字节`);
  }
  if (data[0] !== 0xA0 || data[1] !== 0x17) {
    throw new Error('阀控应答帧数据标识异常，期望 A0 17，实际 ' + hex2(data[0]) + ' ' + hex2(data[1]));
  }
  return {
    dataIdentifier: '0xa017',
    message: '阀控指令应答（正常）',
    serialNumber: data[2],
    status: parseStatus(data[3], data[4])
  };
}

function parseValveCommand(data) {
  if (data.length !== 4) {
    throw new Error(`阀控指令帧数据域长度异常，期望 4 字节，实际 ${data.length} 字节`);
  }
  if (data[0] !== 0xA0 || data[1] !== 0x17) {
    throw new Error('阀控指令帧数据标识异常，期望 A0 17，实际 ' + hex2(data[0]) + ' ' + hex2(data[1]));
  }
  const op = data[3];
  const known = VALVE_OP_NAMES[op];
  const body = {
    dataIdentifier: '0xa017',
    message: '阀控指令（下行回显）',
    serialNumber: data[2],
    command: known ? known[0] : 'unknown',
    commandName: known ? known[1] : `未知(0x${hex2(op)})`
  };
  return known ? { body, warnings: [] } : { body, warnings: [`未知阀控操作码 0x${hex2(op)}`] };
}

function parseUnknownFrame(controlCode, data) {
  const body = { rawData: data.map(hex2).join(' ') };
  // DI0/DI1/SER 按 CJ/T 188 惯例提取，仅作参考，全量以 rawData 为准
  if (data.length >= 2) body.dataIdentifier = '0x' + hex2(data[0]) + hex2(data[1]);
  if (data.length >= 3) body.serialNumber = data[2];
  return {
    body,
    warnings: [`未知控制码 0x${hex2(controlCode)}：数据域未解析，原样输出 rawData（DI/SER 按 CJ/T 188 惯例提取）`]
  };
}

function decodeBuffer(rawBuf) {
  const buf = stripPreamble(rawBuf);

  if (buf.length >= 2 && buf[0] === 0x41 && buf[1] === 0x54) {
    return { data: parseAtText(buf), warnings: ['AT 文本帧（协议 §2 配置/反馈通道），仅透传未做结构化解析'] };
  }

  if (buf.length < 13) throw new Error('帧长度不足');
  if (buf[0] !== 0x68) throw new Error('帧头错误，期望 68H，实际 ' + hex2(buf[0]));
  if (buf[buf.length - 1] !== 0x16) throw new Error('帧尾错误，期望 16H，实际 ' + hex2(buf[buf.length - 1]));

  let checksum = 0;
  for (let i = 0; i < buf.length - 2; i++) {
    checksum = (checksum + buf[i]) & 0xff;
  }
  const cs = buf[buf.length - 2];
  if (checksum !== cs) {
    console.warn(`[警告] 校验不匹配，计算=0x${hex2(checksum)}，帧内=0x${hex2(cs)}`);
  }

  let offset = 1;
  const meterType = buf[offset++];
  const meterTypeNames = { 0x10: '水表' };

  ensureAvailable(buf, offset, 7, '地址域');
  const addressField = buf.slice(offset, offset + 7);
  const address = [...addressField.slice(0, 4)]
    .reverse()
    .map(hex2)
    .join('');
  offset += 7;

  const controlCode = buf[offset++];
  const dataLength = buf[offset++];
  const expectedFrameLength = 13 + dataLength;
  if (buf.length !== expectedFrameLength) {
    throw new Error(`帧长与数据长度不一致，期望 ${expectedFrameLength} 字节，实际 ${buf.length} 字节`);
  }
  ensureAvailable(buf, offset, dataLength, '数据域');
  const data = buf.slice(offset, offset + dataLength);

  const head = {
    meterType: meterTypeNames[meterType] || `未知(0x${hex2(meterType)})`,
    address,
    controlCode: '0x' + hex2(controlCode)
  };

  let body;
  let warnings = [];
  if (controlCode === 0x81) {
    body = parseMeterData(data);
  } else if (controlCode === 0x84) {
    body = parseValveAck(data);
  } else if (controlCode === 0x04) {
    const r = parseValveCommand(data);
    body = r.body;
    warnings = r.warnings;
  } else {
    const r = parseUnknownFrame(controlCode, data);
    body = r.body;
    warnings = r.warnings;
  }
  return { data: Object.assign(head, body), warnings };
}

// 规范化（2026-09-17）：去除 Buffer 依赖（ChirpStack/TTN 运行时无 Buffer），改用纯字节数组
function hexToBytes(hexString) {
  const out = [];
  for (let i = 0; i < hexString.length; i += 2) out.push(parseInt(hexString.slice(i, i + 2), 16));
  return out;
}

function decodeFrame(hexString) {
  const hex = hexString.replace(/\s+/g, '').replace(/^0x/i, '');
  if (hex.length % 2 !== 0) throw new Error('十六进制字符串长度必须为偶数');
  const buf = hexToBytes(hex);
  return decodeBuffer(buf).data;
}

function decodeUplink(input) {
  try {
    if (!input || !input.bytes) {
      return { data: { error: 'Invalid input' } };
    }
    const buf = input.bytes;
    const r = decodeBuffer(buf);
    const out = { data: r.data };
    if (r.warnings && r.warnings.length) out.warnings = r.warnings;
    return out;
  } catch (e) {
    return { data: { error: e.message } };
  }
}

// 阀控下行编码：input.data = { command: 'forceOpen'|'forceClose'|'forceUndo'|'freeOpen'|'freeClose', address: '12345678' }
// address 为 8 位十进制表地址（对应 A0-A3 BCD 低字节在前），A4-A6 固定 00，SER 固定 01（协议示例值）
function encodeDownlink(input) {
  const d = (input && input.data) || {};
  const op = VALVE_OPS[d.command];
  if (op === undefined) {
    return { errors: ['command 必须为 forceOpen/forceClose/forceUndo/freeOpen/freeClose 之一'] };
  }
  const addr = String(d.address == null ? '' : d.address);
  if (!/^[0-9]{8}$/.test(addr)) {
    return { errors: ['address 必须为 8 位十进制表地址字符串，如 "12345678"'] };
  }
  const addrBytes = [];
  for (let i = 6; i >= 0; i -= 2) addrBytes.push(parseInt(addr.slice(i, i + 2), 16));
  const frame = [0x68, 0x10].concat(addrBytes, [0x00, 0x00, 0x00, 0x04, 0x04, 0xA0, 0x17, 0x01, op]);
  let cs = 0;
  for (const b of frame) cs = (cs + b) & 0xff;
  return { bytes: [0xFE, 0xFE, 0xFE].concat(frame, [cs, 0x16]) };
}

// ========= 导出（供模块引用） =========
// module.exports = { decodeFrame, decodeUplink, encodeDownlink };
