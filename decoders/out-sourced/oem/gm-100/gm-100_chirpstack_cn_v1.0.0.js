/**
 * HKT GM-100 LoRaWAN gas meter — payload decoder
 * Data-type parsing strictly follows User Manual **section 5.3** only.
 * decodeUplink returns a **hierarchical** `data` object (meta / frame / tlv_blocks / warnings).
 */

const CODE_VERSION_INFO = {
  version: "v1.3.0",
  author: "",
  releaseDate: "2026-04-01",
  description: "HKT GM-100 hierarchical decode per User Manual §5.3"
};

const SYNC_BYTES = ["68", "6B", "74"];

const DATA_TYPE_LENGTH = {
  0x01: 2,
  0x03: 1,
  0x33: 4,
  0x34: 4,
  0x35: 4,
  0x36: 4,
  0x37: 4,
  0x38: 1,
  0x80: null,
  0x83: 1,
  0x85: 1,
  0x86: 2,
  0x89: 1
};

const DATA_TYPE_FUNCTION_5_3 = {
  0x01: "Device software and hardware version",
  0x03: "Device Battery Information",
  0x33: "Gas Unit Price",
  0x34: "Gas recharge payment",
  0x35: "Total usage",
  0x36: "Gas remaining amount",
  0x37: "Gas balance",
  0x38: "Valve status",
  0x80: "Synchronize system time",
  0x83: "Fault Status",
  0x85: "Resume factory settings",
  0x86: "Data Synchronization Period",
  0x89: "Battery status"
};

const DEFINED_5_3 = new Set(Object.keys(DATA_TYPE_LENGTH).map((k) => parseInt(k, 10)));

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

function parseUintBE(bytes) {
  if (!bytes || bytes.length === 0) return 0;
  let value = 0;
  for (let i = 0; i < bytes.length; i++) {
    value = (value << 8) | parseInt(bytes[i], 16);
  }
  return value;
}

function parseSpecialType(byteHex) {
  const v = parseInt(byteHex, 16);
  return {
    raw: v,
    ackRequired: (v & 0x01) === 1,
    reservedBits: (v >> 1) & 0x7f
  };
}

function parseTimePayload80(bytes) {
  if (bytes.length === 0) return { decoded: "no payload", format: "§5.3 note" };
  if (bytes.length >= 6) {
    const y = parseInt(bytes[0], 16);
    const mo = parseInt(bytes[1], 16);
    const d = parseInt(bytes[2], 16);
    const h = parseInt(bytes[3], 16);
    const mi = parseInt(bytes[4], 16);
    const s = parseInt(bytes[5], 16);
    const year = 2000 + (y >= 0 && y <= 99 ? y : y % 100);
    const decoded = `${year}-${String(mo).padStart(2, "0")}-${String(d).padStart(2, "0")} ${String(h).padStart(2, "0")}:${String(mi).padStart(2, "0")}:${String(s).padStart(2, "0")}`;
    return { decoded, format: "year,month,day,hour,minute,second (§5.3 downlink)" };
  }
  if (bytes.length === 5) {
    const y = parseInt(bytes[0], 16);
    const mo = parseInt(bytes[1], 16);
    const d = parseInt(bytes[2], 16);
    const h = parseInt(bytes[3], 16);
    const mi = parseInt(bytes[4], 16);
    const year = 2000 + (y >= 0 && y <= 99 ? y : y % 100);
    const decoded = `${year}-${String(mo).padStart(2, "0")}-${String(d).padStart(2, "0")} ${String(h).padStart(2, "0")}:${String(mi).padStart(2, "0")}:00`;
    return { decoded, format: "§5.3 downlink example (5 bytes, second not in frame)" };
  }
  return { decoded: `0x${bytes.join("")}`, format: "§5.3 uplink request (raw)" };
}

/**
 * @returns {{ data_type_hex: string, function_5_3: string, payload_hex: string, data_byte_length: number, decoded: object }}
 */
function parseDataBlock(dataType, dataBytes, warnings) {
  const dataTypeHex = `0x${dataType.toString(16).toUpperCase().padStart(2, "0")}`;
  const functionName = DATA_TYPE_FUNCTION_5_3[dataType];
  const payloadHex = `0x${dataBytes.join("")}`;
  const decoded = {};

  switch (dataType) {
    case 0x01:
      decoded.hardware_version = parseInt(dataBytes[0], 16);
      decoded.software_version = parseInt(dataBytes[1], 16);
      break;
    case 0x03:
      decoded.battery_percentage = parseInt(dataBytes[0], 16);
      break;
    case 0x33: {
      const raw = parseUintBE(dataBytes);
      decoded.raw_0_01_per_m3 = raw;
      decoded.value_m3 = raw / 100;
      break;
    }
    case 0x34: {
      const raw = parseUintBE(dataBytes);
      decoded.raw_0_01_per_m3 = raw;
      decoded.value_m3 = raw / 100;
      break;
    }
    case 0x35: {
      const raw = parseUintBE(dataBytes);
      decoded.raw_0_01_m3 = raw;
      decoded.total_usage_m3 = raw / 100;
      break;
    }
    case 0x36: {
      const raw = parseUintBE(dataBytes);
      decoded.raw_0_01_m3 = raw;
      decoded.gas_remaining_amount_m3 = raw / 100;
      break;
    }
    case 0x37: {
      const raw = parseUintBE(dataBytes);
      decoded.raw_0_01_m3 = raw;
      decoded.gas_balance_m3 = raw / 100;
      break;
    }
    case 0x38: {
      const st = parseInt(dataBytes[0], 16);
      decoded.code = st;
      decoded.text_5_3 =
        st === 0x01 ? "Valve open" : st === 0x00 ? "Valve closed" : `reserved value 0x${st.toString(16)}`;
      break;
    }
    case 0x80: {
      const t = parseTimePayload80(dataBytes);
      decoded.time = t.decoded;
      decoded.format_note = t.format;
      break;
    }
    case 0x83: {
      const st = parseInt(dataBytes[0], 16);
      decoded.code = st;
      decoded.text_5_3 =
        st === 0 ? "fault cleared" : st === 1 ? "device failure / counting interference alarm" : `reserved value 0x${st.toString(16)}`;
      break;
    }
    case 0x85: {
      const v = parseInt(dataBytes[0], 16);
      decoded.code = v;
      decoded.text_5_3 =
        v === 1 ? "restore factory settings (§5.3)" : `value ${v} (§5.3 defines 1: restore)`;
      break;
    }
    case 0x86: {
      const minutes = parseUintBE(dataBytes);
      decoded.minutes = minutes;
      if (minutes !== 0 && (minutes < 10 || minutes > 1440)) {
        warnings.push(
          `§5.3 type 0x86: value ${minutes} min is outside manual range 10–1440 minutes`
        );
      }
      break;
    }
    case 0x89: {
      const st = parseInt(dataBytes[0], 16);
      decoded.code = st;
      decoded.text_5_3 =
        st === 0 ? "under voltage alarm" : st === 1 ? "normal" : `reserved value 0x${st.toString(16)}`;
      break;
    }
    default:
      break;
  }

  return {
    data_type_hex: dataTypeHex,
    function_5_3: functionName,
    payload_hex: payloadHex,
    data_byte_length: dataBytes.length,
    decoded
  };
}

function parseDataFromHex(hexInput) {
  const bytes = hexToBytes(hexInput);
  const warnings = [];

  if (bytes.length < 6) {
    throw new Error(
      `Payload too short: need §5.1 sync (3) + special type (1) + serial (1) + at least one data type byte; got ${bytes.length} bytes`
    );
  }

  if (bytes[0] !== SYNC_BYTES[0] || bytes[1] !== SYNC_BYTES[1] || bytes[2] !== SYNC_BYTES[2]) {
    warnings.push(`Sync header is not HKT: expected 686B74 per §5.2, got ${bytes.slice(0, 3).join("")}`);
  }

  const specialHex = bytes[3];
  const serialHex = bytes[4];
  const sp = parseSpecialType(specialHex);

  const tlv_blocks = [];
  let i = 5;
  let blockIndex = 0;

  while (i < bytes.length) {
    blockIndex += 1;
    const dataType = parseInt(bytes[i], 16);
    i += 1;

    if (!DEFINED_5_3.has(dataType)) {
      throw new Error(
        `Data type 0x${dataType.toString(16).toUpperCase()} is not defined in User Manual section 5.3`
      );
    }

    let dataLen = DATA_TYPE_LENGTH[dataType];

    if (dataLen === null) {
      const rest = bytes.length - i;
      if (rest >= 6) dataLen = 6;
      else if (rest === 5) dataLen = 5;
      else if (rest >= 4) dataLen = 4;
      else dataLen = rest;
    }

    if (i + dataLen > bytes.length) {
      throw new Error(
        `§5.3 type 0x${dataType.toString(16).toUpperCase()} requires ${dataLen} data byte(s); only ${bytes.length - i} left`
      );
    }

    const dataBytes = bytes.slice(i, i + dataLen);
    i += dataLen;

    const block = parseDataBlock(dataType, dataBytes, warnings);
    tlv_blocks.push({
      block_index: blockIndex,
      ...block
    });
  }

  const payloadHex = bytes.join("");

  return {
    meta: {
      byte_length: bytes.length,
      payload_hex: payloadHex
    },
    frame: {
      sync_head: `0x${bytes.slice(0, 3).join("")}`,
      special_type: {
        hex: `0x${specialHex}`,
        decimal: parseInt(specialHex, 16),
        bit0_ack_required: sp.ackRequired,
        reserved_bits_1_to_7: sp.reservedBits
      },
      packet_serial: parseInt(serialHex, 16)
    },
    tlv_blocks,
    warnings
  };
}

function decodeUplink(input) {
  try {
    if (!input || !Array.isArray(input.bytes)) {
      return { data: { error: "Invalid input" } };
    }
    const hexInput = bytesToHexString(input.bytes);
    const structured = parseDataFromHex(hexInput);
    return { data: structured };
  } catch (error) {
    return { data: { error: error.message || "Decode exception" } };
  }
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { decodeUplink, parseDataFromHex, CODE_VERSION_INFO, DEFINED_5_3 };
}

if (typeof window !== "undefined") {
  window.decodeUplink = decodeUplink;
  window.CODE_VERSION_INFO = CODE_VERSION_INFO;
}
