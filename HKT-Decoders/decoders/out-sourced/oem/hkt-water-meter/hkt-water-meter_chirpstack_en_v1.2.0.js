/**
 * HKT LoRaWAN Water Meter (214C / 213E) Decoder (English)
 * Author: Zhiyong Wu
 * Protocol: "214C and 213E water meter lorawan protocol 0X1E V101 20211231"
 *
 * Frame format: [FE FE FE] 68 [T] [A0-A6] [C] [L] [DATA...] [CS] 16
 *   - FE FE FE preamble (present in the protocol downlink/ACK examples; the
 *     decoder strips it when present, up to 3 bytes)
 *   - CS = low byte of the sum of all bytes from 0x68 to the end of DATA
 *   - AT text frames (protocol §2 config channel) start with ASCII "AT"; passed through verbatim
 *
 * C=81 data upload, 30-byte DATA (BCD little-endian):
 *   DI0(1) DI1(1) SER(1)
 *   Unit(1)+ForwardTotal(4) x0.001 m³
 *   Unit(1)+ReverseTotal(4) x0.001 m³
 *   Unit(1)+InstantFlow(4)  x0.0001 m³/h
 *   Temperature(3) x0.01 ℃
 *   [ExtraByte(1)]  only in the 31-byte field frame (L=1F / DI=93 1F, observed 2026-10-21);
 *                   undocumented, emitted verbatim as extraByte
 *   Realtime(7) ss:mm:hh:DD:MM:YY (little-endian)
 *   Status(2) low byte=Table 3.1, high byte=Table 3.2
 *
 * C=84 valve-control normal reply, 5-byte DATA: A0 17 SER ST0 ST1 (same status tables)
 * C=04 valve-control command (downlink echo), 4-byte DATA: A0 17 SER OP (see VALVE_OPS)
 * Other control codes: DATA emitted verbatim as rawData with a warning
 * (field-verified C=24 heartbeat/status frame, semantics undocumented)
 *
 * Downlink valve control (encodeDownlink; not exercised by the platform yet):
 *   FE FE FE 68 10 [A0-A6] 04 04 A0 17 01 [OP] [CS] 16
 *   OP: 55 force open / 99 force close / FF force undo / 66 free open / 77 free close
 */

var VALVE_OPS = {
  forceOpen:  0x55,
  forceClose: 0x99,
  forceUndo:  0xFF,
  freeOpen:   0x66,
  freeClose:  0x77
};

var VALVE_OP_NAMES = {
  0x55: ['forceOpen',  'Force open'],
  0x99: ['forceClose', 'Force close'],
  0xFF: ['forceUndo',  'Force undo'],
  0x66: ['freeOpen',   'Free open'],
  0x77: ['freeClose',  'Free close']
};

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

function hex2(b) {
  return b.toString(16).padStart(2, '0');
}

// Status word: st0 = Table 3.1 (first byte), st1 = Table 3.2 (second byte); wording per protocol tables
function parseStatus(st0, st1) {
  return {
    valve: (st0 & 0x01) ? 'Closed' : 'Open',
    battery: (st0 & 0x02) ? 'Low' : 'Normal',
    pipeBurst: (st0 & 0x04) ? 'Fault' : 'Normal',
    pipeLeak: (st0 & 0x08) ? 'Fault' : 'Normal',
    tempSensor: (st0 & 0x10) ? 'Fault' : 'Normal',
    flowSensor: (st0 & 0x20) ? 'Fault' : 'Normal',
    installDir: (st0 & 0x40) ? 'Reverse' : 'Forward',
    openValveMode: (st1 & 0x01) ? 'Normal open' : 'Abnormal open',
    closeValveMode: (st1 & 0x02) ? 'Normal close' : 'Abnormal close',
    externalPower: (st1 & 0x04) ? 'Low voltage' : 'Abnormal',
    openValveError: (st1 & 0x08) ? 'Fault' : 'Normal',
    closeValveError: (st1 & 0x10) ? 'Fault' : 'Normal',
    forcedOpen: (st1 & 0x20) ? 'Forced open' : 'Not forced',
    forcedClose: (st1 & 0x40) ? 'Forced close' : 'Not forced',
    externalPowerSupply: (st1 & 0x80) ? 'Low voltage' : 'Normal',
    rawHex: '0x' + hex2(st0) + hex2(st1)
  };
}

function ensureAvailable(source, offset, size, fieldName) {
  if (offset + size > source.length) {
    throw new Error(`${fieldName} length is insufficient`);
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
    messageType: 'AT text',
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

  const di0 = take(1, 'Data identifier DI0')[0];
  const di1 = take(1, 'Data identifier DI1')[0];
  const serialNumber = take(1, 'Serial number')[0];
  take(1, 'Forward flow unit');
  const forwardFlow = decodeScaledBCDLE(take(4, 'Forward cumulative flow'), 0.001);
  take(1, 'Reverse flow unit');
  const reverseFlow = decodeScaledBCDLE(take(4, 'Reverse cumulative flow'), 0.001);
  take(1, 'Instant flow unit');
  const instantFlow = decodeScaledBCDLE(take(4, 'Instant flow'), 0.0001);
  const temperature = decodeScaledBCDLE(take(3, 'Temperature'), 0.01);

  // After temperature, 9 bytes (time 7 + status 2) are expected; the 31-byte field frame
  // carries one extra undocumented byte, so route on the remaining length
  const remaining = data.length - di;
  let extraByte = null;
  if (remaining === 10) {
    extraByte = take(1, 'Extra byte')[0];
  } else if (remaining !== 9) {
    throw new Error(`Unrecognized frame layout: ${remaining} bytes remain after temperature (expected 9 or 10)`);
  }

  const datetime = decodeDatetimeBCDLE(take(7, 'Realtime time'));
  const st0 = take(1, 'Status byte 1')[0];
  const st1 = take(1, 'Status byte 2')[0];

  if (di !== data.length) {
    throw new Error(`Data section not fully consumed, remaining ${data.length - di} bytes`);
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
    throw new Error(`Valve ACK data length mismatch, expected 5 bytes, got ${data.length}`);
  }
  if (data[0] !== 0xA0 || data[1] !== 0x17) {
    throw new Error('Valve ACK data identifier mismatch, expected A0 17, got ' + hex2(data[0]) + ' ' + hex2(data[1]));
  }
  return {
    dataIdentifier: '0xa017',
    message: 'Valve control ACK (normal)',
    serialNumber: data[2],
    status: parseStatus(data[3], data[4])
  };
}

function parseValveCommand(data) {
  if (data.length !== 4) {
    throw new Error(`Valve command data length mismatch, expected 4 bytes, got ${data.length}`);
  }
  if (data[0] !== 0xA0 || data[1] !== 0x17) {
    throw new Error('Valve command data identifier mismatch, expected A0 17, got ' + hex2(data[0]) + ' ' + hex2(data[1]));
  }
  const op = data[3];
  const known = VALVE_OP_NAMES[op];
  const body = {
    dataIdentifier: '0xa017',
    message: 'Valve control command (downlink echo)',
    serialNumber: data[2],
    command: known ? known[0] : 'unknown',
    commandName: known ? known[1] : `Unknown(0x${hex2(op)})`
  };
  return known ? { body, warnings: [] } : { body, warnings: [`Unknown valve operation code 0x${hex2(op)}`] };
}

function parseUnknownFrame(controlCode, data) {
  const body = { rawData: data.map(hex2).join(' ') };
  // DI0/DI1/SER extracted per CJ/T 188 convention, for reference only; rawData is authoritative
  if (data.length >= 2) body.dataIdentifier = '0x' + hex2(data[0]) + hex2(data[1]);
  if (data.length >= 3) body.serialNumber = data[2];
  return {
    body,
    warnings: [`Unknown control code 0x${hex2(controlCode)}: DATA not decoded, emitted verbatim as rawData (DI/SER per CJ/T 188 convention)`]
  };
}

function decodeBuffer(rawBuf) {
  const buf = stripPreamble(rawBuf);

  if (buf.length >= 2 && buf[0] === 0x41 && buf[1] === 0x54) {
    return { data: parseAtText(buf), warnings: ['AT text frame (protocol §2 config/feedback channel), passed through without structured parsing'] };
  }

  if (buf.length < 13) throw new Error('Frame length is too short');
  if (buf[0] !== 0x68) throw new Error('Invalid frame start, expected 0x68');
  if (buf[buf.length - 1] !== 0x16) throw new Error('Invalid frame end, expected 0x16');

  let checksum = 0;
  for (let i = 0; i < buf.length - 2; i++) {
    checksum = (checksum + buf[i]) & 0xff;
  }
  const cs = buf[buf.length - 2];
  if (checksum !== cs) {
    console.warn(`[Warning] Checksum mismatch, calculated=0x${hex2(checksum)}, frame=0x${hex2(cs)}`);
  }

  let offset = 1;
  const meterType = buf[offset++];
  const meterTypeNames = { 0x10: 'Water Meter' };

  ensureAvailable(buf, offset, 7, 'Address');
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
    throw new Error(`Frame length mismatch, expected ${expectedFrameLength}, actual ${buf.length}`);
  }
  ensureAvailable(buf, offset, dataLength, 'Data section');
  const data = buf.slice(offset, offset + dataLength);

  const head = {
    meterType: meterTypeNames[meterType] || `Unknown(0x${hex2(meterType)})`,
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
  return decodeBuffer(buf).data;
}

function decodeUplink(input) {
  // ChirpStack v4 standard entry: input.bytes -> { data: ... }
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

// Valve-control downlink encoder: input.data = { command: 'forceOpen'|'forceClose'|'forceUndo'|'freeOpen'|'freeClose', address: '12345678' }
// address is the 8-decimal-digit meter address (A0-A3 BCD little-endian); A4-A6 fixed 00, SER fixed 01 (protocol example value)
function encodeDownlink(input) {
  const d = (input && input.data) || {};
  const op = VALVE_OPS[d.command];
  if (op === undefined) {
    return { errors: ['command must be one of forceOpen/forceClose/forceUndo/freeOpen/freeClose'] };
  }
  const addr = String(d.address == null ? '' : d.address);
  if (!/^[0-9]{8}$/.test(addr)) {
    return { errors: ['address must be an 8-decimal-digit meter address string, e.g. "12345678"'] };
  }
  const addrBytes = [];
  for (let i = 6; i >= 0; i -= 2) addrBytes.push(parseInt(addr.slice(i, i + 2), 16));
  const frame = [0x68, 0x10].concat(addrBytes, [0x00, 0x00, 0x00, 0x04, 0x04, 0xA0, 0x17, 0x01, op]);
  let cs = 0;
  for (const b of frame) cs = (cs + b) & 0xff;
  return { bytes: [0xFE, 0xFE, 0xFE].concat(frame, [cs, 0x16]) };
}

// Node.js/CommonJS exports for local testing and module reuse
// module.exports = { decodeFrame, decodeUplink, encodeDownlink };
