/**
 * Payload Decoder for HKT RBC100 LoRaWAN Rumen Biocapsule
 * 
 * Copyright 2024 HKT SmartHard
 * 
 * @product HKT_RBC100
 * @version 1.1
 * 
 * Revision History:
 * 2026-01-09  1.1  Add battery voltage (0x8B) parsing, stop on unknown
 *                  fixed-length fields, reject short payloads, and add legacy
 *                  acceleration mitigation fields while preserving original fields.
 * 2025-02-09  1.0  Initial version.
 */

function decodeRBC100(bytes) {
    var decoded = {};
    
    // 检查同步头
    if (!checkReportSync(bytes)) {
        decoded.error = "Invalid sync header";
        return decoded;
    }

    if (bytes.length < 5) {
        decoded.error = "Payload too short";
        return decoded;
    }
    
    // 解析固定头部
    decoded.sync_header = bytesToHexString(bytes.slice(0, 3));
    decoded.special_type = bytes[3];
    decoded.packet_seq = bytes[4];
    
    // 解析是否需要应答
    decoded.need_ack = (decoded.special_type & 0x01) === 0x01;
    
    // 解析数据部分（从第5字节开始）
    var dataLen = bytes.length - 5;
    var i = 5;
    
    while (dataLen > 0 && i < bytes.length) {
        var dataType = bytes[i];
        i++;
        dataLen--;
        
        switch (dataType) {
            case 0x01:  // 设备软硬件版本
                if (dataLen >= 2) {
                    decoded.hardware_version = bytes[i];
                    decoded.software_version = bytes[i + 1];
                    dataLen -= 2;
                    i += 2;
                }
                break;
                
            case 0x4D:  // 温度（多组）
                if (dataLen >= 1) {
                    var groupCount = bytes[i];
                    i++;
                    dataLen--;
                    
                    decoded.temperature_groups = groupCount;
                    decoded.temperatures = [];
                    
                    for (var j = 0; j < groupCount && dataLen >= 2; j++) {
                        var tempValue = (bytes[i] << 8) | bytes[i + 1];
                        var temperature = tempValue / 100.0;
                        
                        // 检查是否为负值（最高位为1）
                        if (tempValue & 0x8000) {
                            temperature = -(tempValue & 0x7FFF) / 100.0;
                        }
                        
                        decoded.temperatures.push(temperature);
                        i += 2;
                        dataLen -= 2;
                    }
                }
                break;
                
            case 0x49:  // 胃动量
                if (dataLen >= 4) {
                    decoded.rumen_motility = readUInt32BE(bytes.slice(i, i + 4));
                    dataLen -= 4;
                    i += 4;
                }
                break;

            case 0x8B:  // Battery voltage in mV
                if (dataLen >= 2) {
                    decoded.battery_voltage = readUInt16BE(bytes.slice(i, i + 2));
                    dataLen -= 2;
                    i += 2;
                }
                break;
                
            case 0x4A:  // X轴加速度值(g)
                if (dataLen >= 1) {
                    var rawX = bytes[i];
                    var mgX = rawX < 128 ? rawX : rawX - 256;
                    decoded.acceleration_x = rawX / 1.0;
                    decoded.acceleration_x_raw = rawX;
                    decoded.acceleration_x_mg = mgX;
                    decoded.acceleration_x_g = mgX / 1000.0;
                    dataLen -= 1;
                    i += 1;
                }
                break;
                
            case 0x4B:  // Y轴加速度值(g)
                if (dataLen >= 1) {
                    var rawY = bytes[i];
                    var mgY = rawY < 128 ? rawY : rawY - 256;
                    decoded.acceleration_y = rawY / 1.0;
                    decoded.acceleration_y_raw = rawY;
                    decoded.acceleration_y_mg = mgY;
                    decoded.acceleration_y_g = mgY / 1000.0;
                    dataLen -= 1;
                    i += 1;
                }
                break;
                
            case 0x4C:  // Z轴加速度值(g)
                if (dataLen >= 1) {
                    var rawZ = bytes[i];
                    var mgZ = rawZ < 128 ? rawZ : rawZ - 256;
                    decoded.acceleration_z = rawZ / 1.0;
                    decoded.acceleration_z_raw = rawZ;
                    decoded.acceleration_z_mg = mgZ;
                    decoded.acceleration_z_g = mgZ / 1000.0;
                    dataLen -= 1;
                    i += 1;
                }
                break;

            case 0x86:  // 数据同步周期
                if (dataLen >= 2) {
                    decoded.sync_interval = readUInt16BE(bytes.slice(i, i + 2)); // 单位：分钟
                    dataLen -= 2;
                    i += 2;
                }
                break;
                
            default:
                // Stop instead of skipping: an unknown fixed-length field cannot
                // be resynchronized safely without knowing its payload length.
                decoded.unknown_type = dataType;
                decoded.unknown_type_offset = i - 1;
                dataLen = 0;
                break;
        }
    }
    
    return decoded;
}

// 工具函数
function checkReportSync(bytes) {
    return bytes.length >= 3 && 
           bytes[0] === 0x68 && 
           bytes[1] === 0x6B && 
           bytes[2] === 0x74;
}

function bytesToHexString(bytes) {
    return Array.from(bytes, function(byte) {
        return ('0' + (byte & 0xFF).toString(16)).slice(-2);
    }).join(' ').toUpperCase();
}

function readUInt16BE(bytes) {
    return (bytes[0] << 8) + bytes[1];
}

function readUInt32BE(bytes) {
    return bytes[0] * 0x01000000 + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
}

// The Things Network 
function Decoder(bytes, port) {
    return decodeRBC100(bytes);
}
