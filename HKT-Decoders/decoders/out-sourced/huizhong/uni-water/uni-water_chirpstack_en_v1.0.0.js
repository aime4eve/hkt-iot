const IS_LITTLE_ENDIAN = true;
const CODE_VERSION_INFO = {
  version: "v1.0.2",
  author: "Zhiyong Wu",
  releaseDate: "2026-03-20",
  description: "LoRaWAN unified protocol payload decoder, outputs ChirpStack v4 flat business object, supports major data parsing for AFN=1/3"
};

const FRAME_CONFIG = [
  { name: "Frame Start Flag", bytes: 1, type: "HEX", expected: "66" },
  { name: "Length L", bytes: 1, type: "UINT8" },
  { name: "Protocol Version V", bytes: 1, type: "UINT8", expected: "01" },
  { name: "Address Field A", bytes: 7, type: "HEX" },
  { name: "Frame Sequence SEQ", bytes: 1, type: "UINT8" },
  { name: "Device Type", bytes: 1, type: "UINT8" },
  { name: "Control Field C", bytes: 1, type: "HEX" },
  { name: "Auxiliary Control Field CI", bytes: 1, type: "HEX" }
];

const AFN_DEFINITIONS = {
  0x01: { name: "Real-time Data Report" },
  0x03: { name: "Other Data Report" },
  0x04: { name: "Parameter Setting" },
  0x05: { name: "Parameter Reading" },
  0x06: { name: "Control Command" }
};

const DEVICE_TYPES = {
  0x01: "Valve",
  0x02: "Collector",
  0x03: "Pressure",
  0x04: "Water Meter"
};

const DATA_CATEGORIES = {
  1: { name: "Terminal Status" },
  2: { name: "Real-time Data" },
  3: { name: "Frozen Data" },
  4: { name: "Daily Settlement Data" },
  5: { name: "Curve Data" },
  6: { name: "Alarm Info" },
  30: { name: "Log Data" }
};

const DATA_ID_CONFIGS = {
  1: {
    1: { name: "Supply Voltage", type: "UINT16", unit: "V", scale: 0.001 },
    4: { name: "Current Clock", type: "BCD_TIME" },
    7: { name: "DEVEUI", type: "HEX", bytes: 16 },
    9: { name: "Manufacturer Code", type: "UINT16" },
    15: { name: "Device Type", type: "UINT8" },
    19: { name: "LoRaWAN Network RSSI", type: "UINT8", unit: "dB" }
  },
  2: {
    1: { name: "Instant Flow Rate", type: "FLOAT", unit: "m³/h" },
    2: { name: "Positive Cumulative Flow", type: "BS8", unit: "m³" },
    3: { name: "Negative Cumulative Flow", type: "BS8", unit: "m³" },
    4: { name: "Positive Cumulative Runtime", type: "UINT32", unit: "h" },
    5: { name: "Negative Cumulative Runtime", type: "UINT32", unit: "h" },
    9: { name: "Temperature", type: "INT16", unit: "℃", scale: 0.01 },
    11: { name: "Pressure", type: "UINT16", unit: "kPa" },
    16: { name: "Valve Status", type: "UINT16" },
    24: { name: "Alarm", type: "ALARM" },
    25: { name: "Net Cumulative Flow", type: "BS8", unit: "m³" }
  }
};

const LID_LENGTH_MAP = {
  1: 1,
  2: 2,
  3: 3,
  4: 4,
  5: 5,
  6: 6,
  7: 16
};

const VALVE_STATUS_MAP = {
  0: "Valve Closed",
  10000: "Valve Open",
  10001: "Valve Open Timeout",
  10002: "Valve Close Timeout",
  10003: "Valve Open Stalled",
  10004: "Valve Close Stalled",
  10005: "Valve Open Failed",
  10006: "Valve Closed, Flow Present",
  10007: "Valve Opening",
  10008: "Valve Closing",
  10009: "Valve Forced Open",
  10010: "Valve Close Failed",
  10011: "Valve Fault",
  10012: "No Valve"
};

const ALARM_TYPE_MAP = {
  1: "Continuous Low Flow Alarm",
  2: "Continuous High Flow Alarm",
  3: "Data Tampering Alarm",
  4: "Low Battery Alarm",
  5: "Battery Critically Low, Near Depletion",
  6: "Internal High Temperature Alarm",
  7: "Backflow Alarm",
  8: "High Pressure Alarm",
  9: "Low Pressure Alarm",
  10: "High Water Temperature Alarm",
  11: "Low Water Temperature Alarm",
  12: "Empty Pipe Alarm",
  13: "Storage Fault Warning",
  14: "Water Temperature Sensor Fault",
  15: "Internal Temperature Sensor Fault",
  16: "Pressure Sensor Fault",
  17: "Long-duration Flow Alarm",
  18: "Pressure Surge Alarm",
  19: "Reverse Continuous Low Flow Alarm",
  20: "Reverse Continuous High Flow Alarm",
  21: "Meter Removal Alarm",
  22: "Valve Fault Alarm (Valve-controlled Water Meter)",
  33: "Valve Fault Alarm (Smart Control Valve)"
};

function addRecord(records, key, value, meta = {}) {
  records.push({ key, value: `${value}`, ...meta });
}

function bytesToHexString(bytes) {
  if (!Array.isArray(bytes) || bytes.length === 0) {
    throw new Error("Invalid input");
  }
  return bytes
    .map((byte) => {
      if (!Number.isInteger(byte) || byte < 0 || byte > 255) {
        throw new Error("Invalid input");
      }
      return byte.toString(16).toUpperCase().padStart(2, "0");
    })
    .join("");
}

function hexToBytes(hex) {
  const cleanHex = String(hex || "")
    .trim()
    .replace(/\s/g, "")
    .replace(/0x/g, "");
  if (!cleanHex) return [];
  if (cleanHex.length % 2 !== 0) {
    throw new Error("Hex string length must be even");
  }
  const bytes = [];
  for (let i = 0; i < cleanHex.length; i += 2) {
    bytes.push(cleanHex.substring(i, i + 2).toUpperCase());
  }
  return bytes;
}

function parseUint(bytes, littleEndian = true) {
  if (!bytes || bytes.length === 0) return 0;
  let value = 0;
  if (littleEndian) {
    for (let i = 0; i < bytes.length; i++) {
      value |= parseInt(bytes[i], 16) << (i * 8);
    }
  } else {
    for (let i = 0; i < bytes.length; i++) {
      value = (value << 8) | parseInt(bytes[i], 16);
    }
  }
  return value;
}

function parseFloat32(bytes) {
  if (bytes.length !== 4) return 0;
  const buf = new ArrayBuffer(4);
  const view = new DataView(buf);
  for (let i = 0; i < 4; i++) {
    view.setUint8(i, parseInt(bytes[i], 16));
  }
  return view.getFloat32(0, IS_LITTLE_ENDIAN);
}

function parseInt16(bytes) {
  if (bytes.length !== 2) return 0;
  const value = parseUint(bytes, IS_LITTLE_ENDIAN);
  return value > 32767 ? value - 65536 : value;
}

function parseBCDByte(byteHex) {
  const v = parseInt(byteHex, 16);
  return ((v >> 4) & 0x0f) * 10 + (v & 0x0f);
}

function parseBCDTime(bytes) {
  if (bytes.length !== 6) return `0x${bytes.join("")}`;
  const year = parseBCDByte(bytes[0]);
  const month = parseBCDByte(bytes[1]);
  const day = parseBCDByte(bytes[2]);
  const hour = parseBCDByte(bytes[3]);
  const minute = parseBCDByte(bytes[4]);
  const second = parseBCDByte(bytes[5]);
  return `20${year.toString().padStart(2, "0")}-${month.toString().padStart(2, "0")}-${day.toString().padStart(2, "0")} ${hour.toString().padStart(2, "0")}:${minute.toString().padStart(2, "0")}:${second.toString().padStart(2, "0")}`;
}

function parseBCDTime4(bytes) {
  if (bytes.length !== 4) return `0x${bytes.join("")}`;
  const year = parseBCDByte(bytes[0]);
  const month = parseBCDByte(bytes[1]);
  const day = parseBCDByte(bytes[2]);
  const hour = parseBCDByte(bytes[3]);
  return `20${year.toString().padStart(2, "0")}-${month.toString().padStart(2, "0")}-${day.toString().padStart(2, "0")} ${hour.toString().padStart(2, "0")}:00:00`;
}

function parseUnixTimestamp(bytes) {
  if (bytes.length !== 4) return null;
  const timestamp = parseUint(bytes, IS_LITTLE_ENDIAN) * 1000;
  if (timestamp <= 0) return null;
  const date = new Date(timestamp);
  const year = date.getUTCFullYear();
  const month = String(date.getUTCMonth() + 1).padStart(2, "0");
  const day = String(date.getUTCDate()).padStart(2, "0");
  const hours = String(date.getUTCHours()).padStart(2, "0");
  const minutes = String(date.getUTCMinutes()).padStart(2, "0");
  const seconds = String(date.getUTCSeconds()).padStart(2, "0");
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds} UTC`;
}

function parseDevEUI(bytes) {
  let devEuiStr = "0x";
  for (let i = 0; i < bytes.length; i++) {
    const hex = bytes[i].toString(16).toUpperCase().padStart(2, "0");
    devEuiStr += hex.startsWith("3") ? hex.substring(1) : hex;
  }
  return devEuiStr;
}

function parseControlByte(byte) {
  const value = parseInt(byte, 16);
  const afn = value & 0x0f;
  const dir = (value >> 7) & 0x01;
  const prm = (value >> 6) & 0x01;
  return {
    afn,
    afnText: AFN_DEFINITIONS[afn]?.name || `Unknown AFN(0x${afn.toString(16)})`,
    dirText: dir === 1 ? "Uplink (Terminal → Master)" : "Downlink (Master → Terminal)",
    prmText: prm === 1 ? "Initiating Station" : "Secondary Station"
  };
}

function parseControlByteCI(byte) {
  const value = parseInt(byte, 16);
  const hasMore = (value >> 5) & 0x01;
  const encrypt = (value >> 4) & 0x01;
  const fir = (value >> 3) & 0x01;
  const fin = (value >> 2) & 0x01;
  let frameType = "Multi-frame Middle Frame";
  if (fir === 1 && fin === 1) frameType = "Single Frame";
  if (fir === 1 && fin === 0) frameType = "Multi-frame First Frame";
  if (fir === 0 && fin === 1) frameType = "Multi-frame Last Frame";
  return `${frameType}, ${encrypt === 1 ? "Encrypted" : "Unencrypted"}, ${hasMore === 1 ? "Has Subsequent Command" : "No Subsequent Command"}`;
}

function parseBS8Format(bytes) {
  if (bytes.length < 1) return null;
  const formatByte = parseInt(bytes[0], 16);
  const dataType = (formatByte >> 4) & 0x0f;
  const decimalPos = (formatByte & 0x0f) - 6;
  const dataBytes = bytes.slice(1);
  let rawValue = 0;
  let typeName = "";
  if (dataType === 0 && dataBytes.length >= 1) {
    rawValue = parseInt(dataBytes[0], 16);
    typeName = "UINT8";
  } else if (dataType === 1 && dataBytes.length >= 2) {
    rawValue = parseUint(dataBytes.slice(0, 2), IS_LITTLE_ENDIAN);
    typeName = "UINT16";
  } else if (dataType === 2 && dataBytes.length >= 4) {
    rawValue = parseUint(dataBytes.slice(0, 4), IS_LITTLE_ENDIAN);
    typeName = "UINT32";
  } else if (dataType === 8 && dataBytes.length >= 4) {
    rawValue = parseFloat32(dataBytes.slice(0, 4));
    typeName = "Float";
  } else if (dataBytes.length > 0) {
    rawValue = parseUint(dataBytes, IS_LITTLE_ENDIAN);
    typeName = `Type${dataType}`;
  }
  return {
    typeName,
    decimalPos,
    finalValue: decimalPos !== 0 ? rawValue * Math.pow(10, decimalPos) : rawValue
  };
}

function parseIdHeader(byte) {
  const value = parseInt(byte, 16);
  const lid = (value >> 5) & 0x07;
  const id = value & 0x1f;
  return {
    lid,
    id,
    hasEid: id === 0,
    hasElid: lid === 0,
    dataLength: lid >= 1 && lid <= 7 ? LID_LENGTH_MAP[lid] || 0 : 0
  };
}

function getAlarmContentInfo(alarmId, bytes) {
  if (alarmId === 1 && bytes.length >= 3) {
    return `Status:${parseInt(bytes[0], 16) === 1 ? "Triggered" : "Cleared"}, Threshold:${parseInt(bytes[1], 16) * 0.1}L/Min, Duration:${parseInt(bytes[2], 16)}min`;
  }
  if (alarmId === 4 && bytes.length >= 5) {
    const status = parseInt(bytes[0], 16) === 1 ? "Triggered" : "Cleared";
    const threshold = parseUint(bytes.slice(1, 3), IS_LITTLE_ENDIAN) * 0.01;
    const voltage = parseUint(bytes.slice(3, 5), IS_LITTLE_ENDIAN) * 0.01;
    return `Status:${status}, Threshold:${threshold}V, Current:${voltage}V`;
  }
  if (alarmId === 22 && bytes.length >= 1) {
    const statusMap = { 0: "Alarm Cleared", 1: "Valve Open Exception", 2: "Valve Close Exception", 3: "Valve Flush Exception" };
    return `Status:${statusMap[parseInt(bytes[0], 16)] || "Unknown Status"}`;
  }
  return bytes.length > 0 ? `${bytes.length} bytes raw data` : null;
}

function parseAlarmDataBlock(bytes) {
  const records = [];
  let idx = 0;
  let alarmCount = 0;
  while (idx < bytes.length) {
    const alarmId = parseInt(bytes[idx], 16);
    const alarmName = ALARM_TYPE_MAP[alarmId] || `Alarm${alarmId}`;
    addRecord(records, "Alarm Data ID", `${alarmId} (${alarmName})`);
    idx += 1;
    if (idx >= bytes.length) break;
    const alarmLength = parseInt(bytes[idx], 16);
    addRecord(records, "Alarm Data Length", alarmLength);
    idx += 1;
    if (idx + 4 > bytes.length) break;
    const timeBytes = bytes.slice(idx, idx + 4);
    addRecord(records, "Alarm Occurrence Time", parseBCDTime4(timeBytes));
    idx += 4;
    const contentLength = alarmLength - 4;
    if (contentLength > 0 && idx + contentLength <= bytes.length) {
      const contentBytes = bytes.slice(idx, idx + contentLength);
      addRecord(records, "Alarm Data Content", `0x${contentBytes.join("")}`);
      const detail = getAlarmContentInfo(alarmId, contentBytes);
      if (detail) addRecord(records, "Alarm Details", detail);
      idx += contentLength;
    }
    alarmCount++;
  }
  return { records, alarmCount };
}

function parseIdListData(bytes, categoryId) {
  const records = [];
  let idx = 0;
  while (idx < bytes.length) {
    const idHeader = parseIdHeader(bytes[idx]);
    addRecord(records, "ID Header", `0x${bytes[idx]}`);
    idx += 1;
    let finalId = idHeader.id;
    if (idHeader.hasEid && idx < bytes.length) {
      finalId = parseInt(bytes[idx], 16) + 32;
      addRecord(records, "EID", finalId);
      idx += 1;
    }
    let dataLength = idHeader.dataLength;
    if (idHeader.hasElid && idx < bytes.length) {
      const elid = parseInt(bytes[idx], 16);
      dataLength = elid - 1 + 8;
      addRecord(records, "ELID", dataLength);
      idx += 1;
    }
    if (dataLength <= 0 || idx + dataLength > bytes.length) break;
    const dataBytes = bytes.slice(idx, idx + dataLength);
    const config = DATA_ID_CONFIGS[categoryId]?.[finalId];
    const itemName = config?.name || `ID${finalId}`;
    let displayValue = `0x${dataBytes.join("")}`;
    if (config?.type === "BS8") {
      const bs8 = parseBS8Format(dataBytes);
      if (bs8) {
        displayValue = `${typeof bs8.finalValue === "number" ? bs8.finalValue.toFixed(4) : bs8.finalValue}`;
        if (config.unit) displayValue += ` ${config.unit}`;
      }
    } else if (config?.type === "FLOAT") {
      displayValue = parseFloat32(dataBytes).toFixed(4) + (config.unit ? ` ${config.unit}` : "");
    } else if (config?.type === "UINT16") {
      const value = parseUint(dataBytes, IS_LITTLE_ENDIAN);
      displayValue = `${config.scale ? (value * config.scale).toFixed(3) : value}${config.unit ? ` ${config.unit}` : ""}`;
    } else if (config?.type === "UINT32") {
      displayValue = `${parseUint(dataBytes, IS_LITTLE_ENDIAN)}${config.unit ? ` ${config.unit}` : ""}`;
    } else if (config?.type === "INT16") {
      const value = parseInt16(dataBytes);
      displayValue = `${config.scale ? (value * config.scale).toFixed(2) : value}${config.unit ? ` ${config.unit}` : ""}`;
    } else if (config?.type === "BCD_TIME") {
      displayValue = parseBCDTime(dataBytes);
    } else if (config?.name === "DEVEUI") {
      displayValue = parseDevEUI(dataBytes.map((x) => parseInt(x, 16)));
    }
    if (finalId === 16) {
      const statusVal = parseUint(dataBytes, IS_LITTLE_ENDIAN);
      const statusText =
        VALVE_STATUS_MAP[statusVal] || (statusVal >= 1 && statusVal <= 9999 ? `Valve Opening: ${statusVal}` : `Unknown Status(${statusVal})`);
      displayValue = `${statusVal} (${statusText})`;
    }
    addRecord(records, itemName, displayValue);
    idx += dataLength;
  }
  return records;
}

function parseAFN1DataArea(bytes) {
  const records = [];
  const summary = { alarmCount: 0, dataBlocks: 0 };
  let idx = 0;
  if (idx < bytes.length) {
    addRecord(records, "Fn", parseInt(bytes[idx], 16));
    idx += 1;
  }
  while (idx < bytes.length) {
    if (bytes[idx] === "16") break;
    if (idx + 4 > bytes.length) break;
    const timestampBytes = bytes.slice(idx, idx + 4);
    addRecord(records, "Collection Time", parseUnixTimestamp(timestampBytes) || `0x${timestampBytes.join("")}`);
    idx += 4;
    if (idx >= bytes.length) break;
    const categoryByte = parseInt(bytes[idx], 16);
    const hasTwoByteLength = (categoryByte & 0x80) !== 0;
    const categoryId = categoryByte & 0x7f;
    addRecord(records, "Data Category", DATA_CATEGORIES[categoryId]?.name || `Unknown Category(${categoryId})`);
    idx += 1;
    let dataLength = 0;
    if (hasTwoByteLength) {
      if (idx + 2 > bytes.length) break;
      dataLength = parseUint(bytes.slice(idx, idx + 2), IS_LITTLE_ENDIAN);
      idx += 2;
    } else {
      if (idx >= bytes.length) break;
      dataLength = parseInt(bytes[idx], 16);
      idx += 1;
    }
    if (dataLength <= 0 || idx + dataLength > bytes.length) break;
    const dataBytes = bytes.slice(idx, idx + dataLength);
    addRecord(records, "Data Content", `0x${dataBytes.join("")}`);
    if (categoryId === 6) {
      const alarmResult = parseAlarmDataBlock(dataBytes);
      records.push(...alarmResult.records);
      summary.alarmCount += alarmResult.alarmCount;
    } else {
      records.push(...parseIdListData(dataBytes, categoryId));
    }
    summary.dataBlocks += 1;
    idx += dataLength;
  }
  return { records, summary };
}

function parseAFN3Fn6DataArea(bytes) {
  const records = [];
  const summary = { alarmCount: 0, dataBlocks: 1 };
  let idx = 0;
  if (idx < bytes.length) addRecord(records, "Fn", parseInt(bytes[idx++], 16));
  if (idx < bytes.length) addRecord(records, "Data Frame No", parseInt(bytes[idx++], 16));
  if (idx < bytes.length) {
    const reportFlag = parseInt(bytes[idx++], 16);
    addRecord(records, "Report Flag", `${reportFlag} (${reportFlag === 0 ? "Manual Report" : "Automatic Report"})`);
  }
  if (idx < bytes.length) {
    const alarmBytes = bytes.slice(idx);
    addRecord(records, "Alarm Info", `0x${alarmBytes.join("")}`);
    const alarmResult = parseAlarmDataBlock(alarmBytes);
    records.push(...alarmResult.records);
    summary.alarmCount = alarmResult.alarmCount;
  }
  return { records, summary };
}

function parseDataFromHex(hexInput) {
  const bytes = hexToBytes(hexInput);
  if (bytes.length < 16) {
    throw new Error(`Insufficient data length: at least 16-byte frame header required, actual ${bytes.length} bytes`);
  }
  const records = [];
  const warnings = [];
  let currentIndex = 0;
  let controlInfo = null;
  const frame = {};
  for (const field of FRAME_CONFIG) {
    const fieldBytes = bytes.slice(currentIndex, currentIndex + field.bytes);
    if (fieldBytes.length < field.bytes) {
      throw new Error(`Insufficient data for field ${field.name}`);
    }
    let value = "";
    if (field.type === "UINT8") {
      const v = parseInt(fieldBytes[0], 16);
      if (field.name === "Device Type") {
        value = `${v} (${DEVICE_TYPES[v] || `Unknown(0x${v.toString(16)})`})`;
      } else {
        value = `${v}`;
      }
    } else {
      value = `0x${fieldBytes.join("")}`;
      if (field.name === "Control Field C") {
        controlInfo = parseControlByte(fieldBytes[0]);
      }
      if (field.name === "Auxiliary Control Field CI") {
        value = `${value} (${parseControlByteCI(fieldBytes[0])})`;
      }
    }
    if (field.expected && fieldBytes.join("") !== field.expected) {
      warnings.push(`${field.name} expected 0x${field.expected}, actual 0x${fieldBytes.join("")}`);
    }
    addRecord(records, field.name, value);
    frame[field.name] = value;
    currentIndex += field.bytes;
  }
  const dataAreaBytes = bytes.slice(currentIndex, bytes.length - 2);
  let dataRecords = [];
  let dataSummary = { alarmCount: 0, dataBlocks: 0 };
  const afn = controlInfo?.afn;
  if (afn === 0x01) {
    const parsed = parseAFN1DataArea(dataAreaBytes);
    dataRecords = parsed.records;
    dataSummary = parsed.summary;
  } else if (afn === 0x03) {
    const parsed = parseAFN3Fn6DataArea(dataAreaBytes);
    dataRecords = parsed.records;
    dataSummary = parsed.summary;
  } else {
    addRecord(dataRecords, "Data Area", `0x${dataAreaBytes.join("")}`);
  }
  records.push(...dataRecords);
  const cs = bytes[bytes.length - 2];
  const endFlag = bytes[bytes.length - 1];
  addRecord(records, "Frame Checksum CS", `0x${cs}`);
  addRecord(records, "Frame End Flag", `0x${endFlag}`);
  if (endFlag !== "16") {
    warnings.push("Frame end flag error, should be 0x16");
  }
  return {
    input: { byteLength: bytes.length, hex: bytes.join("") },
    frame: {
      startFlag: frame["Frame Start Flag"] || null,
      length: frame["Length L"] || null,
      version: frame["Protocol Version V"] || null,
      address: frame["Address Field A"] || null,
      seq: frame["Frame Sequence SEQ"] || null,
      deviceType: frame["Device Type"] || null,
      control: frame["Control Field C"] || null,
      ci: frame["Auxiliary Control Field CI"] || null,
      cs: `0x${cs}`,
      endFlag: `0x${endFlag}`
    },
    summary: {
      totalBytes: bytes.length,
      afn: controlInfo?.afn ?? null,
      afnText: controlInfo?.afnText ?? null,
      dirText: controlInfo?.dirText ?? null,
      prmText: controlInfo?.prmText ?? null,
      alarmCount: dataSummary.alarmCount,
      dataBlocks: dataSummary.dataBlocks
    },
    records,
    warnings
  };
}

function flattenDecodeResult(parsed) {
  const flat = {};
  flat.byteLength = parsed?.input?.byteLength ?? 0;
  flat.payloadHex = parsed?.input?.hex || "";
  flat.startFlag = parsed?.frame?.startFlag || null;
  flat.length = parsed?.frame?.length || null;
  flat.version = parsed?.frame?.version || null;
  flat.address = parsed?.frame?.address || null;
  flat.seq = parsed?.frame?.seq || null;
  flat.deviceType = parsed?.frame?.deviceType || null;
  flat.control = parsed?.frame?.control || null;
  flat.ci = parsed?.frame?.ci || null;
  flat.cs = parsed?.frame?.cs || null;
  flat.endFlag = parsed?.frame?.endFlag || null;
  flat.afn = parsed?.summary?.afn ?? null;
  flat.afnText = parsed?.summary?.afnText ?? null;
  flat.dirText = parsed?.summary?.dirText ?? null;
  flat.prmText = parsed?.summary?.prmText ?? null;
  flat.alarmCount = parsed?.summary?.alarmCount ?? 0;
  flat.dataBlocks = parsed?.summary?.dataBlocks ?? 0;
  const counters = {};
  const records = Array.isArray(parsed?.records) ? parsed.records : [];
  for (const item of records) {
    if (!item || !item.key) continue;
    const baseKey = item.key;
    counters[baseKey] = (counters[baseKey] || 0) + 1;
    const finalKey = counters[baseKey] === 1 ? baseKey : `${baseKey}${counters[baseKey]}`;
    flat[finalKey] = item.value;
  }
  const warnings = Array.isArray(parsed?.warnings) ? parsed.warnings : [];
  if (warnings.length === 1) {
    flat.warning = warnings[0];
  } else if (warnings.length > 1) {
    for (let i = 0; i < warnings.length; i++) {
      flat[`warning${i + 1}`] = warnings[i];
    }
  }
  return flat;
}

function decodeUplink(input) {
  try {
    if (!input || !Array.isArray(input.bytes)) {
      return { data: { error: "Invalid input" } };
    }
    const hexInput = bytesToHexString(input.bytes);
    const parsed = parseDataFromHex(hexInput);
    return { data: flattenDecodeResult(parsed) };
  } catch (error) {
    return { data: { error: error.message || "Decode exception" } };
  }
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { decodeUplink, CODE_VERSION_INFO };
}

if (typeof window !== "undefined") {
  window.decodeUplink = decodeUplink;
  window.CODE_VERSION_INFO = CODE_VERSION_INFO;
}
