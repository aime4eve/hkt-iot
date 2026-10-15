// ============================================================================
// DA51LD Sub-G split air conditioner actuator - ChirpStack uplink codec.
// Protocol reference: "UM DA51LD Sub-G分体空调执行器 用户说明书.docx",
// chapter "ModBus-LoRaPMAC协议说明".
//
// Frame format (identical to ModBus-RTU framing, carried over LoRaWAN):
//   0x41 active report : [addr 1B][0x41][startReg 2B][dataLen 1B][data dataLen][CRC16 2B]
//   0x10 write response: [addr 1B][0x10][startReg 2B][numRegs 2B][CRC16 2B]
//   0x03 read response : [addr 1B][0x03][byteCount 1B][data byteCount][CRC16 2B]
//
// LoRaWAN port assignment (manual chapter 13.3):
//   8   : periodic heartbeat (active report)
//   220 : ModBus downlink
//   221 : remote AT command
//   222 : ModBus remote command response / report
//   223 : vendor reserved
//
// Active report payload (sequential register layout, startReg 0x0100):
//   0x0100 8B  DevEUI (hex string)
//   0x0104 4B  UTC timestamp (u32 BE, epoch seconds)
//   0x0106 2B  RSSI (i16 BE, dBm)
//   0x0107 2B  SNR (i16 BE, 0.1 dB)
//   0x0108 2B  on/off + key value (lo 8 = power, hi 8 = key index)
//   0x0109 2B  mode (1 auto / 2 cool / 3 heat / 4 fan / 5 dry)
//   0x010A 2B  temperature setpoint (0.1 deg C)
//   0x010B 2B  fan speed (0 auto / 1 low / 2 mid / 3 high)
//   0x010C 2B  vertical swing (0 off / 1 on)
//   0x010D 2B  horizontal swing (0 off / 1 on)
//   0x010E 4B  current RMS (0.001 A)
//   0x0110 4B  cumulative energy (0.01 kWh)
//   0x0112 4B  total power-on runtime (s, reserved)
//   0x0114 4B  total on runtime (s, reserved)
//   0x0116 4B  24 h on runtime (s, reserved)
//   0x0118 2B  last control method
//   0x0119 2B  temperature sensor 1 (0.1 deg C, signed i16 for error codes)
//   0x011A 2B  temperature sensor 2 (optional, reserved)
//   0x011B 2B  internal temperature (optional, reserved)
//
// The device can be configured to report fewer registers
// (register 0x2A16, default 25 = stops before internal temperature);
// the decoder walks the layout and stops when data runs out.
// ============================================================================

var FRAME_HEADER_LEN = 5; // addr + func + startReg(2) + dataLen  (0x41)
var FRAME_CRC_LEN = 2;

var MODES = { 1: 'auto', 2: 'cool', 3: 'heat', 4: 'fan', 5: 'dry' };
var FAN_SPEEDS = { 0: 'auto', 1: 'low', 2: 'mid', 3: 'high' };
var CONTROL_METHODS = {
    0: 'ir_remote',
    1: 'sub_g_wireless',
    2: 'bluetooth',
    3: 'device_permission_correct',
    4: 'ct_detection',
    5: 'rs485',
    6: 'lte',
};

function crc16(buf, len) {
    var crc = 0xFFFF;
    for (var i = 0; i < len; i++) {
        crc ^= buf[i];
        for (var j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

function toHex(bytes, start, len) {
    var s = '';
    for (var i = 0; i < len; i++) {
        s += ('0' + (bytes[start + i] & 0xFF).toString(16)).slice(-2).toUpperCase();
    }
    return s;
}

function u16(bytes, off) {
    return ((bytes[off] & 0xFF) << 8) | (bytes[off + 1] & 0xFF);
}

function u16le(bytes, off) {
    return (bytes[off] & 0xFF) | ((bytes[off + 1] & 0xFF) << 8);
}

function i16(bytes, off) {
    var v = u16(bytes, off);
    return v >= 0x8000 ? v - 0x10000 : v;
}

function u32(bytes, off) {
    return ((u16(bytes, off) << 16) | u16(bytes, off + 2)) >>> 0;
}

// Fixed UTC+8 to avoid host-timezone dependency (same convention as B2315L).
function formatTimestamp(epochSec) {
    if (epochSec === 0 || epochSec === 0xFFFFFFFF) return null;
    var d = new Date((epochSec + 8 * 3600) * 1000);
    function p(n) { return ('0' + n).slice(-2); }
    return d.getUTCFullYear() + '-' + p(d.getUTCMonth() + 1) + '-' + p(d.getUTCDate()) +
        ' ' + p(d.getUTCHours()) + ':' + p(d.getUTCMinutes()) + ':' + p(d.getUTCSeconds());
}

function decodeActiveReport(bytes) {
    var dataLen = bytes[4];
    var dataStart = FRAME_HEADER_LEN;
    var crcOff = dataStart + dataLen;

    if (bytes.length < crcOff + FRAME_CRC_LEN) {
        return { error: 'active report truncated: need ' + (crcOff + FRAME_CRC_LEN) + ' bytes, got ' + bytes.length };
    }

    var warnings = [];
    var frameCrc = u16le(bytes, crcOff);
    var calcCrc = crc16(bytes, crcOff);
    if (frameCrc !== calcCrc) {
        warnings.push('CRC mismatch: frame 0x' + frameCrc.toString(16) + ' vs calculated 0x' + calcCrc.toString(16));
    }

    var data = {
        frameType: 'active_report',
        deviceAddr: bytes[0],
        startRegister: '0x' + u16(bytes, 2).toString(16).padStart(4, '0'),
    };

    var off = dataStart;
    var end = dataStart + dataLen;

    // Walk the register layout sequentially; stop cleanly at dataLen.
    function take(n) {
        if (off + n > end) return false;
        off += n;
        return true;
    }

    if (take(8)) data.devEui = toHex(bytes, off - 8, 8);
    if (take(4)) { var ts = u32(bytes, off - 4); var s = formatTimestamp(ts); if (s) data.timestamp = s; }
    if (take(2)) data.rssi = i16(bytes, off - 2);
    if (take(2)) data.snr = i16(bytes, off - 2) / 10;
    if (take(2)) {
        var reg = u16(bytes, off - 2);
        data.power = (reg & 0x01) ? 'on' : 'off';
        var key = reg >> 8;
        if (key > 0) data.keyValue = key;
    }
    if (take(2)) { var m = u16(bytes, off - 2); data.mode = MODES[m] || m; }
    if (take(2)) data.tempSetpoint = u16(bytes, off - 2) / 10;
    if (take(2)) { var f = u16(bytes, off - 2); data.fanSpeed = FAN_SPEEDS[f] || f; }
    if (take(2)) data.verticalSwing = u16(bytes, off - 2) ? 'on' : 'off';
    if (take(2)) data.horizontalSwing = u16(bytes, off - 2) ? 'on' : 'off';
    if (take(4)) data.current = u32(bytes, off - 4) / 1000;
    if (take(4)) data.energy = u32(bytes, off - 4) / 100;
    if (take(4)) { var v = u32(bytes, off - 4); if (v > 0) data.powerOnTimeSec = v; }
    if (take(4)) { var v = u32(bytes, off - 4); if (v > 0) data.onTimeSec = v; }
    if (take(4)) { var v = u32(bytes, off - 4); if (v > 0) data.onTime24hSec = v; }
    if (take(2)) { var c = u16(bytes, off - 2); data.controlMethod = CONTROL_METHODS[c] || c; }
    if (take(2)) {
        var t = i16(bytes, off - 2);
        if (t === -3005) data.tempSensor1 = 'sensor_error';
        else if (t === -3019) data.tempSensor1 = 'not_installed';
        else if (t === -3134) data.tempSensor1 = 'not_onewire_mode';
        else if (t > 0) data.tempSensor1 = t / 10;
    }
    if (take(2)) {
        var t = i16(bytes, off - 2);
        if (t > 0) data.tempSensor2 = t / 10;
    }
    if (take(2)) {
        var t = i16(bytes, off - 2);
        if (t > 0) data.internalTemp = t / 10;
    }

    if (off < end) {
        warnings.push((end - off) + ' trailing byte(s) after known register layout');
    }

    if (warnings.length > 0) data.warnings = warnings;
    return data;
}

function decodeWriteResponse(bytes) {
    if (bytes.length < 8) {
        return { error: 'write response truncated: need 8 bytes, got ' + bytes.length };
    }
    var warnings = [];
    var frameCrc = u16le(bytes, 6);
    var calcCrc = crc16(bytes, 6);
    if (frameCrc !== calcCrc) {
        warnings.push('CRC mismatch: frame 0x' + frameCrc.toString(16) + ' vs calculated 0x' + calcCrc.toString(16));
    }
    var data = {
        frameType: 'write_response',
        deviceAddr: bytes[0],
        startRegister: '0x' + u16(bytes, 2).toString(16).padStart(4, '0'),
        numRegisters: u16(bytes, 4),
    };
    if (warnings.length > 0) data.warnings = warnings;
    return data;
}

function decodeReadResponse(bytes) {
    var byteCount = bytes[2];
    if (bytes.length < 3 + byteCount + FRAME_CRC_LEN) {
        return { error: 'read response truncated: need ' + (3 + byteCount + FRAME_CRC_LEN) + ' bytes, got ' + bytes.length };
    }
    var warnings = [];
    var crcOff = 3 + byteCount;
    var frameCrc = u16le(bytes, crcOff);
    var calcCrc = crc16(bytes, crcOff);
    if (frameCrc !== calcCrc) {
        warnings.push('CRC mismatch: frame 0x' + frameCrc.toString(16) + ' vs calculated 0x' + calcCrc.toString(16));
    }
    var registers = [];
    for (var i = 0; i < byteCount; i += 2) {
        registers.push(u16(bytes, 3 + i));
    }
    var data = {
        frameType: 'read_response',
        deviceAddr: bytes[0],
        byteCount: byteCount,
        registers: registers,
    };
    if (warnings.length > 0) data.warnings = warnings;
    return data;
}

function decodeUplink(input) {
    var result = { data: {} };
    var bytes = input.bytes;
    var errors = [];

    if (!bytes || bytes.length === 0) {
        result.errors = ['empty payload'];
        return result;
    }
    if (bytes.length < 2) {
        result.errors = ['payload too short: ' + bytes.length + ' byte(s)'];
        return result;
    }

    var func = bytes[1];
    var decoded;
    if (func === 0x41) {
        decoded = decodeActiveReport(bytes);
    } else if (func === 0x10) {
        decoded = decodeWriteResponse(bytes);
    } else if (func === 0x03) {
        decoded = decodeReadResponse(bytes);
    } else {
        result.errors = ['unknown function code 0x' + func.toString(16).padStart(2, '0')];
        return result;
    }

    if (decoded.error) {
        result.errors = [decoded.error];
        result.data = decoded;
        return result;
    }

    result.data = decoded;
    if (decoded.warnings && decoded.warnings.length > 0) {
        result.warnings = decoded.warnings;
        delete decoded.warnings;
    }
    return result;
}
