const IS_LITTLE_ENDIAN = true;
const CODE_VERSION_INFO = {
  version: "v1.0.2",
  author: "伍志勇",
  releaseDate: "2026-03-20",
  description: "LoRaWAN统一协议Payload解码，输出ChirpStack v4扁平业务对象，支持AFN=1/3主要数据解析"
};

const FRAME_CONFIG = [
  { name: "帧起始符", bytes: 1, type: "HEX", expected: "66" },
  { name: "长度L", bytes: 1, type: "UINT8" },
  { name: "协议版本V", bytes: 1, type: "UINT8", expected: "01" },
  { name: "地址域A", bytes: 7, type: "HEX" },
  { name: "帧序号SEQ", bytes: 1, type: "UINT8" },
  { name: "设备类型", bytes: 1, type: "UINT8" },
  { name: "控制域C", bytes: 1, type: "HEX" },
  { name: "辅助控制域CI", bytes: 1, type: "HEX" }
];

const AFN_DEFINITIONS = {
  0x01: { name: "实时数据上报" },
  0x03: { name: "其他数据上报" },
  0x04: { name: "参数设置" },
  0x05: { name: "参数读取" },
  0x06: { name: "控制命令" }
};

const DEVICE_TYPES = {
  0x01: "阀门",
  0x02: "采集器",
  0x03: "压力",
  0x04: "水表"
};

const DATA_CATEGORIES = {
  1: { name: "终端状态" },
  2: { name: "实时数据" },
  3: { name: "冻结数据" },
  4: { name: "日结数据" },
  5: { name: "曲线数据" },
  6: { name: "告警信息" },
  30: { name: "日志数据" }
};

const DATA_ID_CONFIGS = {
  1: {
    1: { name: "供电电压", type: "UINT16", unit: "V", scale: 0.001 },
    4: { name: "当前时钟", type: "BCD_TIME" },
    7: { name: "DEVEUI", type: "HEX", bytes: 16 },
    9: { name: "厂商代码", type: "UINT16" },
    15: { name: "设备类型", type: "UINT8" },
    19: { name: "LoRaWAN网络RSSI", type: "UINT8", unit: "dB" }
  },
  2: {
    1: { name: "瞬时流量", type: "FLOAT", unit: "m³/h" },
    2: { name: "(正)累积流量", type: "BS8", unit: "m³" },
    3: { name: "负累积流量", type: "BS8", unit: "m³" },
    4: { name: "(正)累积运行时间", type: "UINT32", unit: "h" },
    5: { name: "负累积运行时间", type: "UINT32", unit: "h" },
    9: { name: "温度", type: "INT16", unit: "℃", scale: 0.01 },
    11: { name: "压力", type: "UINT16", unit: "kPa" },
    16: { name: "阀门状态", type: "UINT16" },
    24: { name: "告警", type: "ALARM" },
    25: { name: "净累积流量", type: "BS8", unit: "m³" }
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
  0: "阀门关",
  10000: "阀门开",
  10001: "阀门开超时",
  10002: "阀门关超时",
  10003: "阀门开堵转",
  10004: "阀门关堵转",
  10005: "阀门开失败",
  10006: "阀门关，有流量",
  10007: "阀门正在开",
  10008: "阀门正在关",
  10009: "阀门强制开",
  10010: "关阀门失败",
  10011: "阀门异常",
  10012: "没有阀门"
};

const ALARM_TYPE_MAP = {
  1: "持续低流量告警",
  2: "持续高流量告警",
  3: "数据被篡改告警",
  4: "低电量告警",
  5: "电量严重不足即将耗尽",
  6: "内部高温告警",
  7: "反流告警",
  8: "高压告警",
  9: "低压告警",
  10: "高水温告警",
  11: "低水温告警",
  12: "空管告警",
  13: "存储故障警告",
  14: "水温传感器故障",
  15: "内部温度传感器故障",
  16: "压力传感器故障",
  17: "长时间有流量告警",
  18: "压力突变告警",
  19: "反向持续低流量告警",
  20: "反向持续高流量告警",
  21: "仪表拆卸告警",
  22: "阀门异常告警（阀控水表）",
  33: "阀门异常告警（智能调节阀）"
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
    throw new Error("十六进制字符串长度必须是偶数");
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

function parseAddressA0ToA3(bytes) {
  if (!Array.isArray(bytes) || bytes.length < 4) return "";
  return bytes
    .slice(0, 4)
    .reverse()
    .join("");
}

function parseControlByte(byte) {
  const value = parseInt(byte, 16);
  const afn = value & 0x0f;
  const dir = (value >> 7) & 0x01;
  const prm = (value >> 6) & 0x01;
  return {
    afn,
    afnText: AFN_DEFINITIONS[afn]?.name || `未知AFN(0x${afn.toString(16)})`,
    dirText: dir === 1 ? "上行(终端→主站)" : "下行(主站→终端)",
    prmText: prm === 1 ? "启动站" : "从动站"
  };
}

function parseControlByteCI(byte) {
  const value = parseInt(byte, 16);
  const hasMore = (value >> 5) & 0x01;
  const encrypt = (value >> 4) & 0x01;
  const fir = (value >> 3) & 0x01;
  const fin = (value >> 2) & 0x01;
  let frameType = "多帧中间帧";
  if (fir === 1 && fin === 1) frameType = "单帧";
  if (fir === 1 && fin === 0) frameType = "多帧首帧";
  if (fir === 0 && fin === 1) frameType = "多帧末帧";
  return `${frameType}, ${encrypt === 1 ? "加密" : "不加密"}, ${hasMore === 1 ? "有后续指令" : "无后续指令"}`;
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
    typeName = `类型${dataType}`;
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
    return `状态:${parseInt(bytes[0], 16) === 1 ? "产生" : "消除"}, 门限:${parseInt(bytes[1], 16) * 0.1}L/Min, 持续时间:${parseInt(bytes[2], 16)}min`;
  }
  if (alarmId === 4 && bytes.length >= 5) {
    const status = parseInt(bytes[0], 16) === 1 ? "产生" : "消除";
    const threshold = parseUint(bytes.slice(1, 3), IS_LITTLE_ENDIAN) * 0.01;
    const voltage = parseUint(bytes.slice(3, 5), IS_LITTLE_ENDIAN) * 0.01;
    return `状态:${status}, 门限:${threshold}V, 当前:${voltage}V`;
  }
  if (alarmId === 22 && bytes.length >= 1) {
    const statusMap = { 0: "告警消除", 1: "开阀异常", 2: "关阀异常", 3: "洗阀异常" };
    return `状态:${statusMap[parseInt(bytes[0], 16)] || "未知状态"}`;
  }
  return bytes.length > 0 ? `${bytes.length}字节原始数据` : null;
}

function parseAlarmDataBlock(bytes) {
  const records = [];
  let idx = 0;
  let alarmCount = 0;
  while (idx < bytes.length) {
    const alarmId = parseInt(bytes[idx], 16);
    const alarmName = ALARM_TYPE_MAP[alarmId] || `告警${alarmId}`;
    addRecord(records, "告警数据ID", `${alarmId} (${alarmName})`);
    idx += 1;
    if (idx >= bytes.length) break;
    const alarmLength = parseInt(bytes[idx], 16);
    addRecord(records, "告警数据长度", alarmLength);
    idx += 1;
    if (idx + 4 > bytes.length) break;
    const timeBytes = bytes.slice(idx, idx + 4);
    addRecord(records, "告警发生时间", parseBCDTime4(timeBytes));
    idx += 4;
    const contentLength = alarmLength - 4;
    if (contentLength > 0 && idx + contentLength <= bytes.length) {
      const contentBytes = bytes.slice(idx, idx + contentLength);
      addRecord(records, "告警数据内容", `0x${contentBytes.join("")}`);
      const detail = getAlarmContentInfo(alarmId, contentBytes);
      if (detail) addRecord(records, "告警详情", detail);
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
    addRecord(records, "ID头", `0x${bytes[idx]}`);
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
        VALVE_STATUS_MAP[statusVal] || (statusVal >= 1 && statusVal <= 9999 ? `阀门开度: ${statusVal}` : `未知状态(${statusVal})`);
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
    addRecord(records, "采集时间", parseUnixTimestamp(timestampBytes) || `0x${timestampBytes.join("")}`);
    idx += 4;
    if (idx >= bytes.length) break;
    const categoryByte = parseInt(bytes[idx], 16);
    const hasTwoByteLength = (categoryByte & 0x80) !== 0;
    const categoryId = categoryByte & 0x7f;
    addRecord(records, "数据类别", DATA_CATEGORIES[categoryId]?.name || `未知类别(${categoryId})`);
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
    addRecord(records, "数据内容", `0x${dataBytes.join("")}`);
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
  if (idx < bytes.length) addRecord(records, "数据帧号", parseInt(bytes[idx++], 16));
  if (idx < bytes.length) {
    const reportFlag = parseInt(bytes[idx++], 16);
    addRecord(records, "上报标识", `${reportFlag} (${reportFlag === 0 ? "手动上报" : "自动上报"})`);
  }
  if (idx < bytes.length) {
    const alarmBytes = bytes.slice(idx);
    addRecord(records, "告警信息", `0x${alarmBytes.join("")}`);
    const alarmResult = parseAlarmDataBlock(alarmBytes);
    records.push(...alarmResult.records);
    summary.alarmCount = alarmResult.alarmCount;
  }
  return { records, summary };
}

function parseDataFromHex(hexInput) {
  const bytes = hexToBytes(hexInput);
  if (bytes.length < 16) {
    throw new Error(`数据长度不足: 至少需要16字节帧头，实际${bytes.length}字节`);
  }
  const records = [];
  const warnings = [];
  let currentIndex = 0;
  let controlInfo = null;
  const frame = {};
  for (const field of FRAME_CONFIG) {
    const fieldBytes = bytes.slice(currentIndex, currentIndex + field.bytes);
    if (fieldBytes.length < field.bytes) {
      throw new Error(`字段${field.name}数据不足`);
    }
    let value = "";
    if (field.type === "UINT8") {
      const v = parseInt(fieldBytes[0], 16);
      if (field.name === "设备类型") {
        value = `${v} (${DEVICE_TYPES[v] || `未知(0x${v.toString(16)})`})`;
      } else {
        value = `${v}`;
      }
    } else {
      if (field.name === "地址域A") {
        value = parseAddressA0ToA3(fieldBytes);
      } else {
        value = `0x${fieldBytes.join("")}`;
      }
      if (field.name === "控制域C") {
        controlInfo = parseControlByte(fieldBytes[0]);
      }
      if (field.name === "辅助控制域CI") {
        value = `${value} (${parseControlByteCI(fieldBytes[0])})`;
      }
    }
    if (field.expected && fieldBytes.join("") !== field.expected) {
      warnings.push(`${field.name}期望0x${field.expected}，实际0x${fieldBytes.join("")}`);
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
    addRecord(dataRecords, "数据区", `0x${dataAreaBytes.join("")}`);
  }
  records.push(...dataRecords);
  const cs = bytes[bytes.length - 2];
  const endFlag = bytes[bytes.length - 1];
  addRecord(records, "帧校验和CS", `0x${cs}`);
  addRecord(records, "帧结束符", `0x${endFlag}`);
  if (endFlag !== "16") {
    warnings.push("帧结束符错误，应为0x16");
  }
  return {
    input: { byteLength: bytes.length, hex: bytes.join("") },
    frame: {
      startFlag: frame["帧起始符"] || null,
      length: frame["长度L"] || null,
      version: frame["协议版本V"] || null,
      address: frame["地址域A"] || null,
      seq: frame["帧序号SEQ"] || null,
      deviceType: frame["设备类型"] || null,
      control: frame["控制域C"] || null,
      ci: frame["辅助控制域CI"] || null,
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
