/**
 * Payload Decoder for The Reports
 *
 * Copyright 2022 HKT SmartHard
 *
 * @product GT-30
 *
 * 正确性来源（固件）：HKT-Firmwares/in-house/LoRaWAN_THTB_Sensor
 *   USER/Drive/communicate.c（ASCII CSV 帧经 LoRa_SendCMD 原样上送，ref 41ad0d9）
 * 帧格式（fPort 固件默认 10）：
 *   普通帧（11 字段）  T,S1,DEVEUI,软版本,包序号,事件,状态,温度,湿度,moved,battery
 *   GPS 帧（10 字段）  T,S1,DEVEUI,软版本,包序号,21,E/W经度,N/S纬度,时间,温度
 *   配置响应（R 开头） R,DEVEUI,软版本,包序号,101,类型,值;类型,值;...
 *   数值字段为 %.1f 格式（"25.4"）；moved/battery/GPS 无值时为 "*" 占位 → NaN → 平台判定为 null
 *
 * ── 修改日志 ─────────────────────────────────────────────
 * 2026-08-31 v1.0.0 字节数组转 ASCII 解析。
 * 2026-09-18 v1.1.0 黄金样例 × 固件互证：
 *   1. 新增 101 配置响应帧（SendConfigInfo 真实上行，原版只认 T,S1 直接拒绝）；
 *   2. 字段数既非 10/11 也非配置响应的帧一律报错（原版静默输出公共头 4 字段）；
 *   3. 坏同步头/过短帧返回 error（原版裸 return undefined）。
 * ─────────────────────────────────────────────────────────
 */
'use strict';

function Decoder(bytes, port) {

    var decoded = {};
    if (Array.isArray(bytes) || ArrayBuffer.isView(bytes)) {
        bytes = String.fromCharCode.apply(null, new Uint8Array(bytes));
    }

    if (typeof bytes !== "string" || bytes.length < 4) {
        return { data: {}, errors: ["payload too short (" + (bytes ? bytes.length : 0) + " bytes)，帧最小为 4 字节同步头 T,S1"] };
    }

    var isConfigResponse = bytes.slice(0, 2) === "R,";

    if (!isConfigResponse && checkReportSync(bytes.slice(0, 4)) == false) {
        return { data: {}, errors: ["invalid frame header（同步头必须为 T,S1，配置响应帧以 R, 开头）"] };
    }

    var field = bytes.split(",");

    // 101 配置响应帧：固件 SendConfigInfo 下行请求的应答，经空口上行
    if (isConfigResponse) {
        if (field.length < 6) {
            return { data: {}, errors: ["truncated config response frame（R 帧字段不足）"] };
        }
        var params = {};
        var kv = field.slice(5).join(",").split(";");
        for (var p = 0; p < kv.length; p++) {
            var pair = kv[p].split(",");
            if (pair.length === 2 && pair[0] !== "") {
                params[pair[0]] = parseFloat(pair[1]);
            }
        }
        return { data: {
            frame_type: "config_response",
            deveui: field[1],
            soft_ver: parseInt(field[2]),
            que_num: parseInt(field[3]),
            config_type: parseInt(field[4]),
            params: params,
        } };
    }

    if (field.length == 10) //gps data
    {
        // Longitude of GPS
        decoded.lon = parseFloat(field[6].slice(1, field[6].length));
        if (field[6].slice(0, 1) == "W")  //E,W
            decoded.lon = -decoded.lon;

        // Latitude of GPS.
        decoded.lat = parseFloat(field[7].slice(1, field[7].length));
        if (field[7].slice(0, 1) == "S")  //N,S
            decoded.lat = -decoded.lat;

        // UTC time
        decoded.time = field[8];

        //This is the temperature of the unit in Celsius
        decoded.temperature = parseFloat(field[9]);
    }
    else if (field.length == 11)    //normal data
    {
        //This is the state of any alarm sent. When an alarm threshold hasreached, the unit must send State 1.
        //If the alarm continues when thenext send interval reaches, then the unit must send State 2.
        // When the end of the alarm occurs then a state of 3 must be sent.
        /* 0 Normal Reading (Also be sent for low battery and movement)
        * 1 Alarm Started
        * 2 Alarm Still Active
        * 3 End of Alarm
        */
        decoded.state = parseInt(field[6]);

        //This is the temperature of the unit in Celsius
        decoded.temperature = parseFloat(field[7]);

        //This is the Relative Humidity measured in Percentage (%)
        decoded.humidity = parseFloat(field[8]);

        //This is a flag to indicate if the unit has moved
        decoded.moved = parseInt(field[9]);
        decoded.battery = parseFloat(field[10]);
    }
    else {
        // 固件上行只存在 10/11 字段普通帧与 R 配置响应；其余形态必为坏帧
        return { data: {}, errors: ["unknown frame structure: " + field.length + " field(s)（固件仅发送 10/11 字段数据帧与 R 配置响应）"] };
    }

    // The Dev EUI is the number which uniquely identifies each unit
    decoded.deveui = field[2];

    // This field contains the current Software Version on the sensor
    decoded.soft_ver = parseInt(field[3]);

    // This is a number from 1 to 65535 to indicate the message number. Onceit reaches 65535 it will reset to 1.
    decoded.que_num = parseInt(field[4]);

    //The unit must send the reason for the data transfer and can be the following
    /* Event Code Reason
    *  1 Interval Reading
    *  2 Temperature Alarm – Low Threshold
    *  3 Temperature Alarm – Upper Threshold
    *  4 Relative Humidity – Low Threshold
    *  5 Relative Humidity – Upper Threshold
    *  6 Low unit Battery
    *  7 Unit movement
    *  8 Dismantle Alarm
    *  21 GPS positioning
    *  22 Cached Temperature
    */
    decoded.event = parseInt(field[5]);
    return decoded;
}


function checkReportSync(bytes) {
    if (bytes == "T,S1") {
        return true;
    }
    return false;
}

/* ==== HKT-Decoders 平台适配层（2026-09-17 规范化迁移追加） ====
 * 统一入口 decodeUplink(input)：包装历史 Decoder(bytes, port) 入口。
 */
function decodeUplink(input) {
    var __bytes = input.bytes;
    var __port = (input.fPort === undefined || input.fPort === null) ? 0 : input.fPort;
    var __r;
    try {
        __r = Decoder(__bytes, __port);
    } catch (e) {
        return { data: {}, errors: ["decoder threw: " + (e && e.message ? e.message : String(e))] };
    }
    if (__r === undefined || __r === null) return { data: {}, errors: ["decoder returned nothing"] };
    if (typeof __r === "object" && !Array.isArray(__r) && (__r.data !== undefined || __r.errors !== undefined)) return __r;
    return { data: __r };
}

// 供本地 Node 单元测试使用；平台 codec 沙箱中无 module，自动跳过
if (typeof module !== "undefined") {
    module.exports = { Decoder };
}
