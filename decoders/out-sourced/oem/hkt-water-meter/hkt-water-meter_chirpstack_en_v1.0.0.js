/**
 * HKT LoRaWAN Water Meter Decoder (English)
 * Author: Zhiyong Wu
 * Version: v2.0.3
 * Release Date: 2026-03-25
 *
 * Frame format:
 * 68 [T] [A0-A6] [C] [L] [DATA...] [CS] 16
 *
 * DATA fields (BCD little-endian):
 * DI0(1) DI1(1) SER(1)
 * Unit(1)+ForwardTotal(4)
 * Unit(1)+ReverseTotal(4)
 * Unit(1)+InstantFlow(4)
 * Temperature(3)
 * Realtime(7)
 * Status(2)
 */
function decodeBCDLE(bytes) {
  const reversed = [...bytes].reverse();
  let str = '';
  for (const b of reversed) {
    const high = (b >> 4) & 0x0f;
    const low = b & 0x0f;
    if (high > 9 || low > 9) {
      throw new Error('Invalid BCD data: non-decimal nibble found');
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
    valve: (st0 & 0x01) ? 'Closed' : 'Open',
    battery: (st0 & 0x02) ? 'Low' : 'Normal',
    pipeBurst: (st0 & 0x04) ? 'Fault' : 'Normal',
    pipeLeak: (st0 & 0x08) ? 'Fault' : 'Normal',
    tempSensor: (st0 & 0x10) ? 'Fault' : 'Normal',
    flowSensor: (st0 & 0x20) ? 'Fault' : 'Normal',
    installDir: (st0 & 0x40) ? 'Reverse' : 'Forward',
    rawHex: '0x' + st0.toString(16).padStart(2, '0') + st1.toString(16).padStart(2, '0')
  };
}

function ensureAvailable(source, offset, size, fieldName) {
  if (offset + size > source.length) {
    throw new Error(`${fieldName} length is insufficient`);
  }
}

function decodeBuffer(buf) {
  // Basic frame checks: minimum size + start/end flags
  if (buf.length < 13) throw new Error('Frame length is too short');
  if (buf[0] !== 0x68) throw new Error('Invalid frame start, expected 0x68');
  if (buf[buf.length - 1] !== 0x16) throw new Error('Invalid frame end, expected 0x16');

  // CS is the low byte of the sum from 0x68 to the end of status bytes
  let checksum = 0;
  for (let i = 0; i < buf.length - 2; i++) {
    checksum = (checksum + buf[i]) & 0xff;
  }
  const cs = buf[buf.length - 2];
  if (checksum !== cs) {
    console.warn(`[Warning] Checksum mismatch, calculated=0x${checksum.toString(16).padStart(2, '0')}, frame=0x${cs.toString(16).padStart(2, '0')}`);
  }

  let offset = 1;
  const meterType = buf[offset++];
  const meterTypeNames = { 0x10: 'Water Meter' };

  const addressBytes = buf.slice(offset, offset + 7);
  ensureAvailable(buf, offset, 7, 'Address');
  const address = decodeBCDLE(addressBytes.slice(0, 4));
  offset += 7;

  const controlCode = buf[offset++];
  const dataLength = buf[offset++];
  const expectedFrameLength = 13 + dataLength;
  if (buf.length !== expectedFrameLength) {
    throw new Error(`Frame length mismatch, expected ${expectedFrameLength}, actual ${buf.length}`);
  }
  ensureAvailable(buf, offset, dataLength, 'Data section');
  const data = buf.slice(offset, offset + dataLength);

  let di = 0;
  // Read helper for DATA section with bounds checking
  const take = (size, fieldName) => {
    ensureAvailable(data, di, size, fieldName);
    const part = data.slice(di, di + size);
    di += size;
    return part;
  };

  di++;
  di++;
  const serialNumber = data[di++];
  take(1, 'Forward flow unit');
  const forwardFlow = decodeScaledBCDLE(take(4, 'Forward cumulative flow'), 0.001);
  take(1, 'Reverse flow unit');
  const reverseFlow = decodeScaledBCDLE(take(4, 'Reverse cumulative flow'), 0.001);
  take(1, 'Instant flow unit');
  const instantFlow = decodeScaledBCDLE(take(4, 'Instant flow'), 0.0001);
  const temperature = decodeScaledBCDLE(take(3, 'Temperature'), 0.01);
  const datetime = decodeDatetimeBCDLE(take(7, 'Realtime time'));
  const st0 = take(1, 'Status byte 1')[0];
  const st1 = take(1, 'Status byte 2')[0];

  if (di !== data.length) {
    throw new Error(`Data section not fully consumed, remaining ${data.length - di} bytes`);
  }

  return {
    meterType: meterTypeNames[meterType] || `Unknown(0x${meterType.toString(16).padStart(2, '0')})`,
    address,
    controlCode: '0x' + controlCode.toString(16).padStart(2, '0'),
    serialNumber,
    forwardFlow: { value: forwardFlow, unit: 'm³' },
    reverseFlow: { value: reverseFlow, unit: 'm³' },
    instantFlow: { value: instantFlow, unit: 'm³/h' },
    temperature: { value: temperature, unit: '℃' },
    datetime,
    status: parseStatus(st0, st1)
  };
}

// Normalization (2026-09-17): dropped Buffer dependency (no Buffer in ChirpStack/TTN runtimes), plain byte arrays
function hexToBytes(hexString) {
  const out = [];
  for (let i = 0; i < hexString.length; i += 2) out.push(parseInt(hexString.slice(i, i + 2), 16));
  return out;
}

function decodeFrame(hexString) {
  const hex = hexString.replace(/\s+/g, '').replace(/^0x/i, '');
  if (hex.length % 2 !== 0) throw new Error('Hex string length must be even');
  const buf = hexToBytes(hex);
  return decodeBuffer(buf);
}

function decodeUplink(input) {
  // ChirpStack v4 standard entry: input.bytes -> { data: ... }
  try {
    if (!input || !input.bytes) {
      return { data: { error: 'Invalid input' } };
    }
    const buf = input.bytes; // Normalization (2026-09-17): no Buffer round-trip
    const decoded = decodeBuffer(buf);
    return { data: decoded };
  } catch (e) {
    return { data: { error: e.message } };
  }
}

// Node.js/CommonJS exports for local testing and module reuse
// module.exports = { decodeFrame, decodeUplink };
