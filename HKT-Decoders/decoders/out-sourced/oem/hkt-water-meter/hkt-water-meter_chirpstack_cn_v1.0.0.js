/**
 * HKT LoRaWAN 水表协议解码器
 * 
 * 帧结构：68 [T] [A0-A6] [C] [L] [DATA...] [CS] 16
 * 
 * 数据域字段（小端序 BCD，低字节在前）：
 * - DI0(1B) DI1(1B) SER(1B)
 * - 单位码(1B) + 正向累积流量(4B)
 * - 单位码(1B) + 反向累积流量(4B)
 * - 单位码(1B) + 瞬时流量(4B)
 * - 温度(3B)
 * - 实时时间(7B)
 * - 状态字(2B)
 */

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

function parseStatus(st0, st1) {
  return {
    valve: (st0 & 0x01) ? '阀门关' : '阀门开',
    battery: (st0 & 0x02) ? '欠压' : '正常',
    pipeBurst: (st0 & 0x04) ? '故障' : '正常',
    pipeLeak: (st0 & 0x08) ? '故障' : '正常',
    tempSensor: (st0 & 0x10) ? '故障' : '正常',
    flowSensor: (st0 & 0x20) ? '故障' : '正常',
    installDir: (st0 & 0x40) ? '反向' : '正向',
    rawHex: '0x' + st0.toString(16).padStart(2, '0') + st1.toString(16).padStart(2, '0')
  };
}

function ensureAvailable(source, offset, size, fieldName) {
  if (offset + size > source.length) {
    throw new Error(`${fieldName} 长度不足`);
  }
}

function decodeBuffer(buf) {
  if (buf.length < 13) throw new Error('帧长度不足');

  if (buf[0] !== 0x68) throw new Error('帧头错误，期望 68H，实际 ' + buf[0].toString(16).padStart(2, '0'));
  if (buf[buf.length - 1] !== 0x16) throw new Error('帧尾错误，期望 16H，实际 ' + buf[buf.length - 1].toString(16).padStart(2, '0'));

  let checksum = 0;
  for (let i = 0; i < buf.length - 2; i++) {
    checksum = (checksum + buf[i]) & 0xff;
  }
  const cs = buf[buf.length - 2];
  if (checksum !== cs) {
    console.warn(`[警告] 校验不匹配，计算=0x${checksum.toString(16).padStart(2, '0')}，帧内=0x${cs.toString(16).padStart(2, '0')}`);
  }

  let offset = 1;
  const meterType = buf[offset++];
  const meterTypeNames = { 0x10: '水表' };

  ensureAvailable(buf, offset, 7, '地址域');
  const addressField = buf.slice(offset, offset + 7);
  const Address = [...addressField.slice(0, 4)]
    .reverse()
    .map((b) => b.toString(16).padStart(2, '0'))
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

  let di = 0;
  const take = (size, fieldName) => {
    ensureAvailable(data, di, size, fieldName);
    const part = data.slice(di, di + size);
    di += size;
    return part;
  };

  const di0 = data[di++];
  const di1 = data[di++];
  const serialNumber = data[di++];
  const forwardUnit = take(1, '正向流量单位')[0];
  const forwardFlow = decodeScaledBCDLE(take(4, '正向累积流量'), 0.001);
  const reverseUnit = take(1, '反向流量单位')[0];
  const reverseFlow = decodeScaledBCDLE(take(4, '反向累积流量'), 0.001);
  const instantUnit = take(1, '瞬时流量单位')[0];
  const instantFlow = decodeScaledBCDLE(take(4, '瞬时流量'), 0.0001);
  const temperature = decodeScaledBCDLE(take(3, '温度'), 0.01);
  const datetime = decodeDatetimeBCDLE(take(7, '实时时间'));
  const st0 = take(1, '状态字第一字节')[0];
  const st1 = take(1, '状态字第二字节')[0];

  if (di !== data.length) {
    throw new Error(`数据域未完全消费，剩余 ${data.length - di} 字节`);
  }

  return {
    meterType: meterTypeNames[meterType] || `未知(0x${meterType.toString(16).padStart(2, '0')})`,
    Address,
    controlCode: '0x' + controlCode.toString(16).padStart(2, '0'),
    serialNumber,
    forwardFlow:  { value: forwardFlow,  unit: 'm³' },
    reverseFlow:  { value: reverseFlow,  unit: 'm³' },
    instantFlow:  { value: instantFlow,  unit: 'm³/h' },
    temperature:  { value: temperature,  unit: '℃' },
    datetime,
    status: parseStatus(st0, st1)
  };
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
  return decodeBuffer(buf);
}

function decodeUplink(input) {
  try {
    if (!input || !input.bytes) {
      return { data: { error: 'Invalid input' } };
    }
    const buf = input.bytes; // 规范化（2026-09-17）：不再经 Buffer 转换
    const decoded = decodeBuffer(buf);
    return { data: decoded };
  } catch (e) {
    return { data: { error: e.message } };
  }
}

// ========= 导出（供模块引用） =========
// module.exports = { decodeFrame, decodeUplink };

/*
if (require.main === module) {
  const sample = '68 10 78 56 34 12 00 00 00 81 1E 92 1F 06 2C 25 00 00 00 2C 25 00 00 00 35 70 01 00 00 00 18 00 11 43 00 11 05 19 20 40 00 25 16';

  try {
    const r = decodeFrame(sample);
    console.log(JSON.stringify(r, null, 2));
    console.log('\n========== 解码结果 ==========');
    console.log(`表类型：${r.meterType}`);
    console.log(`表地址：${r.meterAddress} (${r.addressHex})`);
    console.log(`控制码：${r.controlCode}`);
    console.log(`序列号：${r.serialNumber}`);
    console.log(`正向累积流量：${r.forwardFlow.value} ${r.forwardFlow.unit}`);
    console.log(`反向累积流量：${r.reverseFlow.value} ${r.reverseFlow.unit}`);
    console.log(`瞬时流量：${r.instantFlow.value} ${r.instantFlow.unit}`);
    console.log(`温度：${r.temperature.value} ${r.temperature.unit}`);
    console.log(`时间：${r.datetime}`);
    console.log(`阀门状态：${r.status.valve}`);
    console.log(`电池电压：${r.status.battery}`);
    console.log(`爆管故障：${r.status.pipeBurst}`);
    console.log(`漏水故障：${r.status.pipeLeak}`);
    console.log(`温度传感器：${r.status.tempSensor}`);
    console.log(`流量传感器：${r.status.flowSensor}`);
    console.log(`安装方向：${r.status.installDir}`);
  } catch (e) {
    console.error('解码失败:', e.message);
  }
}
*/
