/**
 * HKT GM-100 LoRaWAN Smart Gas Meter — payload decoder v1.1.0 (ChirpStack v4)
 *
 * Protocol authority: docs/HKT GM-100 LoRaWAN Smart Gas Meter User Manual.md, §5.1–§5.4.
 * v1.0.0 was a mis-migrated PAFDS product file (only decoded bytes[0]===0x02 and
 * returned {} for every real GM-100 frame); this version replaces it and implements
 * the GM-100 TLV protocol with the FLAT golden-sample output contract:
 *
 *   data = {
 *     sync_head:        "686B74"                (uppercase hex, §5.2 fixed header)
 *     special_type_raw: <int>                   (§5.2 special type byte)
 *     ack_required:     <bool>                  (§5.2 BIT0)
 *     packet_serial:    <int>
 *     blocks: [                                 (one entry per §5.3 TLV block, in frame order)
 *       0x01 { data_type, hardware_version, software_version }
 *       0x03 { data_type, battery_percentage }
 *       0x33 { data_type, unit_price }            (raw/100, §5.3 unit 0.01/m^3)
 *       0x34 { data_type, recharge_amount }       (raw/100)
 *       0x35 { data_type, total_usage_m3 }        (raw/100)
 *       0x36 { data_type, remaining_m3 }          (raw/100)
 *       0x37 { data_type, balance }               (raw/100; §5.3 unit label ambiguous, see manual)
 *       0x38 { data_type, valve }                 ("closed" | "open")
 *       0x80 { data_type, time_request, data_hex } (uplink request form)
 *            { data_type, time }                  (5/6-byte y m d h m [s] form)
 *       0x83 { data_type, fault }                 ("cleared" | "failure_alarm")
 *       0x85 { data_type, raw_code }              (downlink-only type, lossless passthrough)
 *       0x86 { data_type, sync_period_minutes }
 *       0x89 { data_type, battery_status }        ("under_voltage_alarm" | "normal")
 *     ]
 *   }
 *
 * Error policy: structural violations (§5.2 sync head mismatch, frame shorter than
 * §5.1 minimum, TLV data truncation vs §5.3 fixed lengths, data type absent from the
 * §5.3 table) and undefined 1-byte status enum values (0x38/0x83/0x89 outside the
 * documented 0/1 codes) throw internally and surface as { data:{}, errors:[...] }.
 * Soft anomaly: 0x86 outside 10–1440 min still decodes and adds a "warnings" array
 * (key present only when non-empty, so in-range golden samples match exactly).
 *
 * Direction note: §5.3 marks 0x33/0x34/0x85 downlink-only, yet §5.4's uplink example
 * frame contains 0x33 — no direction enforcement is applied (documented contradiction).
 *
 * Sandbox constraints honoured: no Buffer / TypedArray / require — pure JS only.
 * Entry: decodeUplink(input{bytes:number[], fPort}) -> { data } | { data:{}, errors:[...] }.
 */

var GM100_SYNC = [0x68, 0x6b, 0x74];

/** §5.3 data-type table -> fixed data length in bytes; -1 = variable (0x80). */
var GM100_TYPE_LENGTH = {
  0x01: 2,
  0x03: 1,
  0x33: 4,
  0x34: 4,
  0x35: 4,
  0x36: 4,
  0x37: 4,
  0x38: 1,
  0x80: -1,
  0x83: 1,
  0x85: 1,
  0x86: 2,
  0x89: 1
};

function gm100HexByte(v) {
  var h = (v & 0xff).toString(16);
  if (h.length < 2) h = "0" + h;
  return h.toUpperCase();
}

function gm100ToHex(bytes, from, to) {
  var out = "";
  for (var i = from; i < to; i++) out += gm100HexByte(bytes[i]);
  return out;
}

function gm100Pad2(n) {
  return n < 10 ? "0" + n : "" + n;
}

function gm100ParseUintBE(bytes, from, len) {
  var v = 0;
  for (var i = 0; i < len; i++) v = v * 256 + bytes[from + i];
  return v;
}

/** Decode one §5.3 TLV data block into the flat golden-contract shape. */
function gm100DecodeBlock(type, bytes, from, len, warnings) {
  var block = { data_type: "0x" + gm100HexByte(type) };
  var v;

  switch (type) {
    case 0x01: // §5.3: first byte HW version, last byte SW version (uplink only)
      block.hardware_version = bytes[from];
      block.software_version = bytes[from + 1];
      break;
    case 0x03: // §5.3: battery percentage (uplink only)
      block.battery_percentage = bytes[from];
      break;
    case 0x33: // §5.3: gas unit price, 0.01/m^3, 4 bytes
      block.unit_price = gm100ParseUintBE(bytes, from, 4) / 100;
      break;
    case 0x34: // §5.3: gas recharge payment, 4 bytes
      block.recharge_amount = gm100ParseUintBE(bytes, from, 4) / 100;
      break;
    case 0x35: // §5.3: total usage, 0.01 m^3, 4 bytes
      block.total_usage_m3 = gm100ParseUintBE(bytes, from, 4) / 100;
      break;
    case 0x36: // §5.3: gas remaining amount, 0.01 m^3, 4 bytes
      block.remaining_m3 = gm100ParseUintBE(bytes, from, 4) / 100;
      break;
    case 0x37: // §5.3: gas balance, 4 bytes
      block.balance = gm100ParseUintBE(bytes, from, 4) / 100;
      break;
    case 0x38: // §5.3: 00 = valve closed, 01 = valve open
      v = bytes[from];
      if (v === 0x00) block.valve = "closed";
      else if (v === 0x01) block.valve = "open";
      else throw new Error("§5.3 type 0x38: undefined valve status value 0x" + gm100HexByte(v));
      break;
    case 0x80: // §5.3: uplink = time sync request; 5/6-byte payload = y m d h m [s]
      if (len === 5 || len === 6) {
        var yy = bytes[from];
        var mo = bytes[from + 1];
        var dd = bytes[from + 2];
        var hh = bytes[from + 3];
        var mi = bytes[from + 4];
        var ss = len === 6 ? bytes[from + 5] : 0;
        block.time = 2000 + yy + "-" + gm100Pad2(mo) + "-" + gm100Pad2(dd) + " " + gm100Pad2(hh) + ":" + gm100Pad2(mi) + ":" + gm100Pad2(ss);
      } else {
        block.time_request = true;
        if (len > 0) block.data_hex = gm100ToHex(bytes, from, from + len);
      }
      break;
    case 0x83: // §5.3: 0 = fault cleared, 1 = device failure / counting interference alarm
      v = bytes[from];
      if (v === 0) block.fault = "cleared";
      else if (v === 1) block.fault = "failure_alarm";
      else throw new Error("§5.3 type 0x83: undefined fault status value 0x" + gm100HexByte(v));
      break;
    case 0x85: // §5.3: resume factory settings (downlink only); lossless passthrough in uplink
      block.raw_code = bytes[from];
      break;
    case 0x86: // §5.3: data synchronization period, minutes, range 10–1440
      block.sync_period_minutes = gm100ParseUintBE(bytes, from, 2);
      v = block.sync_period_minutes;
      if (v !== 0 && (v < 10 || v > 1440)) {
        warnings.push("§5.3 type 0x86: sync period " + v + " min is outside manual range 10-1440");
      }
      break;
    case 0x89: // §5.3: 0 = under voltage alarm, 1 = normal (uplink only)
      v = bytes[from];
      if (v === 0) block.battery_status = "under_voltage_alarm";
      else if (v === 1) block.battery_status = "normal";
      else throw new Error("§5.3 type 0x89: undefined battery status value 0x" + gm100HexByte(v));
      break;
    default:
      throw new Error("Data type 0x" + gm100HexByte(type) + " is not defined in §5.3");
  }
  return block;
}

/**
 * Decode one GM-100 uplink frame (§5.1 structure) into the flat contract.
 * Throws Error on any structural or enum violation (see header error policy).
 */
function gm100Decode(bytes) {
  var i, b;

  if (!Array.isArray(bytes) || bytes.length === 0) {
    throw new Error("Invalid input: bytes must be a non-empty array");
  }
  for (i = 0; i < bytes.length; i++) {
    b = bytes[i];
    if (typeof b !== "number" || !isFinite(b) || b % 1 !== 0 || b < 0 || b > 255) {
      throw new Error("Invalid input: bytes[" + i + "] is not an octet");
    }
  }
  if (bytes.length < 6) {
    throw new Error(
      "Payload too short: §5.1 needs sync(3) + special type(1) + serial(1) + at least 1 data-type byte, got " + bytes.length + " byte(s)"
    );
  }
  if (bytes[0] !== GM100_SYNC[0] || bytes[1] !== GM100_SYNC[1] || bytes[2] !== GM100_SYNC[2]) {
    throw new Error("§5.2 sync head must be 0x68 0x6B 0x74 (HKT), got 0x" + gm100ToHex(bytes, 0, 3));
  }

  var warnings = [];
  var blocks = [];
  var pos = 5;

  while (pos < bytes.length) {
    var type = bytes[pos];
    pos += 1;

    if (!Object.prototype.hasOwnProperty.call(GM100_TYPE_LENGTH, type)) {
      throw new Error("Data type 0x" + gm100HexByte(type) + " is not defined in §5.3");
    }

    var defLen = GM100_TYPE_LENGTH[type];
    var dataLen;
    if (defLen === -1) {
      // 0x80: 6-byte y m d h m s form when available, 5-byte form, otherwise the
      // remaining bytes are the request payload (§5.3 uplink example carries 4).
      var rest = bytes.length - pos;
      dataLen = rest >= 6 ? 6 : rest;
    } else {
      dataLen = defLen;
    }

    if (pos + dataLen > bytes.length) {
      throw new Error(
        "§5.3 type 0x" + gm100HexByte(type) + " requires " + dataLen + " data byte(s), only " + (bytes.length - pos) + " left"
      );
    }

    blocks.push(gm100DecodeBlock(type, bytes, pos, dataLen, warnings));
    pos += dataLen;
  }

  var data = {
    sync_head: gm100ToHex(bytes, 0, 3),
    special_type_raw: bytes[3],
    ack_required: (bytes[3] & 0x01) === 1,
    packet_serial: bytes[4],
    blocks: blocks
  };
  if (warnings.length > 0) data.warnings = warnings;
  return data;
}

function decodeUplink(input) {
  try {
    return { data: gm100Decode(input && input.bytes) };
  } catch (e) {
    return { data: {}, errors: [e && e.message ? e.message : String(e)] };
  }
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = { decodeUplink: decodeUplink, gm100Decode: gm100Decode };
}

if (typeof window !== "undefined") {
  window.decodeUplink = decodeUplink;
}
