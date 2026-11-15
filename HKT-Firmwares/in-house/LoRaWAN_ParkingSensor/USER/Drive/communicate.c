
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "ble_upgrade.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "lwrb.h"
#include "p25q40.h"
#include "qmc5883l.h"
#include "rkb1145d.h"
#include "rtc.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

/***********            通讯协议数据结构             ***********/
/*--------------------------------------------------------------
|                                                               |
|   同步头   |   特殊类型   |   包序号   |   数据类型   |   数据   |
|                                                               |
---------------------------------------------------------------*/

const u8 SYNC_HEAD[3] = {0x68, 0x6B, 0x74};

u8 lwrb_write_count;
u8 lwrb_data[lwrb_buffer_size];
lwrb_t lwrbBuff_t;

u8 sendNum;
u8 sendBuffer[220];

u8 factoryMode;
u8 keepRunFlag;
u8 BLEDebugMode;
u8 packSyncNumber;

char tempBuf[500];
time_t dataReportTimestamp;

/**
 * @brief  快速分割短整型为字符串数组
 * @param  src：源数据
 * @param   hex: 输出字符串数组
 * @retval
 */

void quickSplitShort(u16 src, u8 *hex)
{
    unionDataStruct unionDataValue;
    unionDataValue.unionDataSrc = src;
    hex[0] = unionDataValue.unionData.DataH;
    hex[1] = unionDataValue.unionData.DataL;
}

/**
 * @brief  按规定协议封成数据包
 * @param  type：协议类型
 * @param  dest：接收封包后的数组
 * @retval 整条数据长度
 */
u8 setDataPackage(enum DataType type, u8 *dest)
{
    u8 dataLen = 1;
    /*  数据类型  */
    dest[0] = type;

    switch (type) {
    case DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION: // 设备软硬件版本
        dest[1] = HARDWARE_VER;
        dest[2] = SOFTWARE_VER;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_ADDR_ID: // 设备ID
        memcpy(dest + 1, LoRaWAN.DevEUI + 2, 6);
        dataLen += 6;
        break;
    case DATATYPE_DEVICE_BATTERY_INFO: // 电量信息
        dest[1] = batteryLevel;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_PARK_STATE: // 车辆状态
        dest[1] = device_t.park_state;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_PARK_MODE: // 车辆状态
        dest[1] = device_t.park_mode;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_PERIOD: // 数据上报周期
        dest[1] = (device_t.report_interval >> 8) & 0xFF;
        dest[2] = device_t.report_interval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆状态
        dest[1] = device_t.tamper_alarm;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_POWER: // 开关机状态
        dest[1] = device_t.power_on;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_MAG_X: // X轴磁场值
        dest[1] = ((uint16_t)qmc5883l_t.xyz.x >> 8) & 0xFF;
        dest[2] = (uint8_t)qmc5883l_t.xyz.x & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_MAG_Y: // Y轴磁场值
        dest[1] = ((uint16_t)qmc5883l_t.xyz.y >> 8) & 0xFF;
        dest[2] = (uint8_t)qmc5883l_t.xyz.y & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_MAG_Z: // Z轴磁场值
        dest[1] = ((uint16_t)qmc5883l_t.xyz.z >> 8) & 0xFF;
        dest[2] = (uint8_t)qmc5883l_t.xyz.z & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_RADAR_SPECTRUM: // 雷达频谱值
        for (uint8_t i = 0; i < 10; i++) {
            dest[1 + i * 2] = ((uint16_t)rkb1145d_t.power[i] >> 8) & 0xFF;
            dest[1 + i * 2 + 1] = (uint8_t)rkb1145d_t.power[i] & 0xFF;
        }
        dataLen += 20;
        break;
    case DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK: // 设备应答ACK
        dest[1] = 0xFF;
        dataLen += 1;
        break;
    default:
        break;
    }
    return dataLen;
}

/**
 * @brief  发送填充好的数据给LoRa模块
 * @param  buf 待发送有效数据缓冲区
 * @param  txlen 待发送有效数据长度
 * @retval true 发送成功 false 发送失败
 */
bool sendLoRaWANData(u8 *buf, u8 txlen)
{
    u8 txbuf[100];
    u8 len;

    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;

    len = 0;
    memset(txbuf, 0, sizeof(txbuf));

    memcpy(txbuf, SYNC_HEAD, 3);
    txbuf[3] = 0;
    txbuf[4] = packSyncNumber++;

    len = txlen;
    memcpy(txbuf + 5, buf, txlen);
    len += 5;

    mDelay(20);
    LoRa_SendData(txbuf, len); // 发送数据
    DebugHexInfo("LoRaWAN Send", txbuf, len);
    mDelay(20);
    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __fail;
    }
    if (getLoRaWANSendStat()) {
    __fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        device_t.send_fail_cnt++;
        packSyncNumber--;
        if (device_t.send_fail_cnt >= 5) {
            device_t.send_fail_cnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        return false;
    }
    device_t.send_fail_cnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    return true;
}

/**
 * @brief  校验同步头
 * @param  data：接收到数据的数组
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDataSyncHead(u8 *data)
{
    ErrorStatus status = SUCCESS;
    if (memcmp(data, SYNC_HEAD, 3) != 0)
        status = ERROR;
    return status;
}

/**
 * @brief  处理来自LoRaWAN模组的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromLoRaWANDataHandle(u8 *data, u16 len)
{
    /*  确认数据 */
    if (checkDataSyncHead(data) == ERROR) {
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        return;
    }

    if ((data[3] & 0x01) == 1) // 服务器需要应答
    {
        u8 *dataHex = sendBuffer;
        memset(sendBuffer, 0, sizeof(sendBuffer));

        sendNum = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
        dataHex += sendNum;

        memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
        memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
        uartLoRa.sendData[3] = 0;
        uartLoRa.sendData[4] = packSyncNumber++;
        memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
        uartLoRa.sendLen = sendNum + 5;

        // ADC_BatterySglConvert();

        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
        if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
        /* 等待模组就绪 */
        while (!getLoRaWANBusyIOLevel())
            ;
        mDelay(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        while (!getLoRaWANStatIOLevel())
            ;
        uartLoRa.sendDelay = 5 * SEC_DELAY;
    }
    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_SYNC_TIME: { // 同步时间
            systime.tm_year = srcHex[1] + 2000;
            systime.tm_mon = srcHex[2];
            systime.tm_mday = srcHex[3];
            systime.tm_hour = srcHex[4];
            systime.tm_min = srcHex[5];
            systime.tm_sec = srcHex[6];
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            RTC_TimeSet();
            dataReportTimestamp = Timestamp;
            dataLen -= 7;
            srcHex += 7;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD, %04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        } break;
        case DATATYPE_DEVICE_PARK_MODE: // 工作模式
            if (srcHex[1] >= 0 && srcHex[1] <= 2) {
                device_t.park_mode = srcHex[1];
                FLASH_Write_Park_Mode();
                DEBUG_TRACE(LOG_TAG, "Update Device Park Mode Success: %d", device_t.park_mode);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update  Device Park Mode Error: %d", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;
            break;
        case DATATYPE_DEVICE_BATTERY_CALIB: // 电量校准（0-100%，与BLE 0x8E同码值）
            if (srcHex[1] <= 100) {
                batteryInitialCapacity = (u32)(BATTERY_MAX_PUSH_CNT / 100) * (100 - srcHex[1]);
                if (FLASH_Write_Battery() == SUCCESS) {
                    ADC_BatterySglConvert(); /* 重算百分比并触发上报 */
                    DEBUG_TRACE(LOG_TAG, "LoRa Battery Calibrated To %d%%", srcHex[1]);
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Battery Calib Flash Write FAIL");
                }
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update Battery Calib Error: %d", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;
            break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 report_interval = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;
            if ((report_interval >= 1 && report_interval <= 1440) || report_interval == 0) {
                device_t.report_interval = report_interval;
                dataReportTimestamp = Timestamp;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Device Sync Interval Over Threshold: %d", report_interval);
            }
        } break;
        default:
            dataLen -= 2;
            srcHex += 2;
            break;
        }
        if (dataLen > 200) // 长度异常，数据出错，退出数据解析
            break;
    }
}

/**
 * @brief  周期同步上报数据
 * @param
 * @retval
 */
void Device_PeriodicReport(void)
{
    u8 buffer[50] = {0};
    u8 *dataHex = buffer;
    u8 dataLen = 0;
    u8 sendLen = 0;

    memset(buffer, 0, sizeof(buffer));
    sendLen = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PARK_STATE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PARK_MODE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendLen + 1)) {
        lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, buffer, sendLen);
        lwrb_write_count++;
        upg_ble_confirm(); // 版本上报成功即确认新固件（升级确认机制）
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
    }
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeviceEvent_Process(void)
{
    u8 buffer[50] = {0};
    u8 sendLen = 0;

    /* 请求服务端同步本地时间 */
    if (device_t.sync_local_time) {
        device_t.sync_local_time = 0;

        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_SYNC_TIME, buffer);

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }

    /* 更新电量信息 */
    if (device_t.sync_battery) {
        device_t.sync_battery = 0;
        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, buffer);

        lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, buffer, sendLen);
        lwrb_write_count++;

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }

    if (device_t.ble_wake_event) {
        device_t.ble_wake_event = 0;
        if (device_t.ble_connected != BLE_CONNECT_STA) {
            mDelay(20);
            if (device_t.ble_connected != BLE_CONNECT_STA) {
                device_t.ble_connected = BLE_CONNECT_STA;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                if (device_t.ble_connected) {
                    PWR_CTRL_H;
                    mDelay(500);
                } else if (BLEDebugMode || device_t.ble_sleep_mode) {
                    /* 断开蓝牙：0xA2 调试态自动全关（RAM态不留存到下次连接/复位） */
                    BLEDebugMode = 0;
                    device_t.ble_sleep_mode = 0;
                    DEBUG_TRACE(LOG_TAG, "BLE Disconnected: Debug Mode Auto Off");
                }
                DEBUG_TRACE(LOG_TAG, "BLE Connect State Change To %d", device_t.ble_connected);
            }
        }
    }

    if (dataReportTimestamp <= Timestamp) {
        device_t.sync_state = 1;
        dataReportTimestamp = Timestamp + device_t.report_interval * 60;
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    u8 buffer[100] = {0};
    u8 sendLen = 0;

    DeviceEvent_Process();

    if (!uartLoRa.sendDelay) {
        if (LoRaWAN.joinState) {
            /* 同步设备数据 */
            if (device_t.sync_state) {
                device_t.sync_state = 0;
                device_t.sync_battery = 0;
                Device_PeriodicReport();
            }

            /* 检测缓冲器是否存在待发射数据 */
            if (lwrb_write_count) { // 缓冲区内有数据
                u8 *dataHex = buffer;
                lwrb_write_count--;
                memset(buffer, 0, sizeof(buffer));
                lwrb_read(&lwrbBuff_t, &sendLen, 1);
                lwrb_read(&lwrbBuff_t, dataHex, sendLen);

                sendLoRaWANData(buffer, sendLen);

                if (lwrb_write_count == 0) {
                    memset(lwrb_data, 0, sizeof(lwrb_data));
                    lwrb_reset(&lwrbBuff_t);
                }
            }
        }
    }
}

void callback_BLEQuery(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    convert_light_sensor();
    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PARK_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PARK_MODE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_X, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_Y, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_Z, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_RADAR_SPECTRUM, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);

    upg_ble_confirm(); // 版本上报成功即确认新固件（升级确认机制）
}

void callback_BLEAck(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEPower(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEMagData(void)
{
    int16_t x = qmc5883l_t.xyz.x;
    int16_t y = qmc5883l_t.xyz.y;
    int16_t z = qmc5883l_t.xyz.z;

    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[sendNum++] = 0x07;

    // 使用位与操作提取高字节和低字节
    sendBuffer[sendNum++] = ((uint16_t)x >> 8) & 0xFF; // 高字节
    sendBuffer[sendNum++] = (uint8_t)x & 0xFF; // 低字节

    sendBuffer[sendNum++] = ((uint16_t)y >> 8) & 0xFF; // 高字节
    sendBuffer[sendNum++] = (uint8_t)y & 0xFF; // 低字节

    sendBuffer[sendNum++] = ((uint16_t)z >> 8) & 0xFF; // 高字节
    sendBuffer[sendNum++] = (uint8_t)z & 0xFF; // 低字节

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLERadarData(void)
{
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    u16 power[10];
    uint8_t i = 0;

    sendBuffer[sendNum++] = 0x08;

    for (i = 0; i < 10; i++) {
        power[i] = rkb1145d_t.power[i];
    }

    for (uint8_t i = 0; i < 10; i++) {
        sendBuffer[sendNum++] = ((uint16_t)power[i] >> 8) & 0xFF;
        sendBuffer[sendNum++] = (uint8_t)power[i] & 0xFF;
    }

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEDebugData(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    /* X/Y/Z三轴磁场原始值 */
    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_X, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_Y, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_MAG_Z, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    /* 雷达频谱值 */
    dataLen = setDataPackage(DATATYPE_DEVICE_RADAR_SPECTRUM, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen);
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

void callback_BLEJoinData(void)
{
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    sendBuffer[sendNum++] = 0xFC;

    sendBuffer[sendNum++] = LoRaWAN.joinState;

    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 24) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 16) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.SNR >> 8) & 0xFF;
    sendBuffer[sendNum++] = LoRaWAN.SNR & 0xFF;

    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 24) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 16) & 0xFF;
    sendBuffer[sendNum++] = (LoRaWAN.RSSI >> 8) & 0xFF;
    sendBuffer[sendNum++] = LoRaWAN.RSSI & 0xFF;

    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    memcpy(uartBle.sendData + 5, sendBuffer, sendNum);

    uartBle.sendLen = sendNum + 5;

    BLE_SendData(uartBle.sendData, uartBle.sendLen); // 发送数据
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

/* ---- 0xA0/0xA1/0x8E/0xA2 LoRaWAN配置与调试协议（BLE） ----
 * 帧形：hkt(3) 特殊类型(1)=0x00 包序号(1) 数据类型(1)=0xA0/0xA1 数据(n) 累加校验(1)
 * 校验=自帧头逐字节求和 mod 256（与升级帧同族），上下行都带校验。
 * 0xA0 查询：data=01（8字节帧）；0xA0 配置：data=33字节（40字节帧）
 * 0xA1 SR0：data=01（8字节帧），应答为模组原始文本透传
 * 0x8E 电量校准：data=百分比(1字节0-100)（8字节帧，68 6B 74 00 01 8E 64 3A=校准100%）。
 *   反算计数落盘环形槽后回 0xFF ACK；LoRa 下行同码值 TLV（dataType 0x8E+1字节百分比）
 * 0xA2 BLE调试模式：data=10字节定长（17字节帧，仅蓝牙无LoRa下行）——
 *   data[0]设备类型(0=地磁,非0拒绝) data[1]调试开关(0/1) data[2]连接时允许睡眠(0/1)
 *   data[3..9]预留忽略；生效回 0xFF ACK；断开蓝牙自动全关（见 ble_wake_event 处理）。
 *   代替旧字符串命令 switchBLEDebugON/OFF、switchBLESleepON/OFF
 * 33字节配置载荷（大端多字节）：
 *   band(1) chmask位图(9) rx2_dr(1) rx2_freq(4) adr(1) confirm(1) confirm_cnt(1)
 *   power(2) dr(2) activation(1) class(1) port(1) deveui(8只读) */
#define BLE_FR_LORAWAN_CFG 0xA0
#define BLE_FR_LORAWAN_SR 0xA1
#define BLE_FR_BATTERY_CALIB 0x8E
#define BLE_FR_BLE_DEBUG 0xA2
#define BLE_CFG_PAYLOAD_LEN 33

/**
 * @brief  组包0xA0配置载荷（33字节，查询应答复用）
 * @param  payload：输出缓冲（33字节）
 * @retval
 */
static void lorawan_cfg_to_payload(u8 *payload)
{
    payload[0] = lorawanCfg.band;
    memcpy(payload + 1, lorawanCfg.chmask, 9);
    payload[10] = lorawanCfg.rx2_dr;
    payload[11] = (lorawanCfg.rx2_freq >> 24) & 0xFF;
    payload[12] = (lorawanCfg.rx2_freq >> 16) & 0xFF;
    payload[13] = (lorawanCfg.rx2_freq >> 8) & 0xFF;
    payload[14] = lorawanCfg.rx2_freq & 0xFF;
    payload[15] = lorawanCfg.adr;
    payload[16] = lorawanCfg.confirm;
    payload[17] = lorawanCfg.confirm_cnt;
    payload[18] = (lorawanCfg.power >> 8) & 0xFF;
    payload[19] = lorawanCfg.power & 0xFF;
    payload[20] = (lorawanCfg.dr >> 8) & 0xFF;
    payload[21] = lorawanCfg.dr & 0xFF;
    payload[22] = lorawanCfg.activation;
    payload[23] = lorawanCfg.class;
    payload[24] = lorawanCfg.port;
    memcpy(payload + 25, LoRaWAN.DevEUI, 8); /* DevEUI只读，回读上报 */
}

/**
 * @brief  0xA0查询应答：hkt 00 seq A0 <33B配置> sum，共40字节（缓存值）
 * @param
 * @retval
 */
static void callback_BLELoRaWANCfg(void)
{
    u8 sum = 0;
    u8 i;

    memset(uartBle.sendData, 0, 40);
    memcpy(uartBle.sendData, SYNC_HEAD, 3);
    uartBle.sendData[3] = 0;
    uartBle.sendData[4] = packSyncNumber++;
    uartBle.sendData[5] = BLE_FR_LORAWAN_CFG;
    lorawan_cfg_to_payload(uartBle.sendData + 6);

    for (i = 0; i < 39; i++)
        sum += uartBle.sendData[i];
    uartBle.sendData[39] = sum;

    uartBle.sendLen = 40;
    BLE_SendData(uartBle.sendData, uartBle.sendLen);
    DebugHexInfo("BLESend", uartBle.sendData, uartBle.sendLen);
}

/**
 * @brief  应用0xA0配置：校验→写flash→ACK→复位重启（重启后LoRaWAN_Init按新配置下发并重新入网）
 * @param  payload：33字节配置载荷
 * @retval
 */
static void ble_apply_lorawan_cfg(u8 *payload)
{
    struct LoRaWANLinkCfg tmp;

    memcpy(&tmp, &lorawanCfg, sizeof(tmp));
    tmp.band = payload[0];
    memcpy(tmp.chmask, payload + 1, 9);
    tmp.rx2_dr = payload[10];
    tmp.rx2_freq = ((u32)payload[11] << 24) | ((u32)payload[12] << 16) | ((u32)payload[13] << 8) | payload[14];
    tmp.adr = payload[15];
    tmp.confirm = payload[16];
    tmp.confirm_cnt = payload[17];
    tmp.power = ((u16)payload[18] << 8) | payload[19];
    tmp.dr = ((u16)payload[20] << 8) | payload[21];
    tmp.activation = payload[22];
    tmp.class = payload[23];
    tmp.port = payload[24];
    /* payload[25..32] DevEUI只读，写配置时忽略 */

    if (!LoRaWAN_CfgValidate(&tmp)) {
        return; /* 校验失败：不应答，APP超时保持旧配置 */
    }

    tmp.magic = LORAWAN_CFG_MAGIC;
    memcpy(&lorawanCfg, &tmp, sizeof(lorawanCfg));
    if (FLASH_Write_LoRaWAN_Cfg() != SUCCESS) {
        DEBUG_TRACE(ERROR_TAG, "LoRaWAN Cfg Flash Write FAIL");
        return;
    }

    DEBUG_TRACE(LOG_TAG, "LoRaWAN Cfg Updated band[%d], Reboot...", lorawanCfg.band);
    callback_BLEAck();
    if (batterySavedCapacity != batteryInitialCapacity) /* 复位前补落盘未存的电量消耗 */
        FLASH_Write_Battery();
    mDelay(500); /* 等ACK发完再复位 */
    NVIC_SystemReset();
}

/**
 * @brief  0xA1模组状态透传：AT+SR0?应答裸文本原样转发蓝牙（用完恢复模组原模式）
 * @param
 * @retval
 */
static void ble_forward_module_sr(void)
{
    int prev_mode = getLoRaWANMode();
    int prev_status = getLoRaWANStatus();

    setLoRaWANStatus(LoRaWAN_WAKE_STATUS); // 唤醒模块
    setLoRaWANMode(LoRaWAN_CMD_MODE); // 配置进入命令模式

    if (AT_LoRaWAN_getSR0()) {
        if (uartLoRa.recvLen < sizeof(uartLoRa.recvData))
            uartLoRa.recvData[uartLoRa.recvLen] = '\0'; /* 补字符串结束符 */
        else
            uartLoRa.recvData[sizeof(uartLoRa.recvData) - 1] = '\0';
        BLE_SendCMD((const char *)uartLoRa.recvData);
    } else {
        BLE_SendCMD("AT+SR0? no response");
    }

    setLoRaWANMode(prev_mode); // 恢复模组原工作模式
    setLoRaWANStatus(prev_status);
}

/**
 * @brief  0x8E电量校准：按百分比反算计数→立即落盘→ACK（100%即换新电池归零校准）
 * @param  percent：目标百分比 0-100
 * @retval
 */
static void ble_apply_battery_calib(u8 percent)
{
    if (percent > 100) {
        DEBUG_TRACE(ERROR_TAG, "Battery Calib Range Error: %d", percent);
        return; /* 非法值不应答，APP 超时 */
    }
    batteryInitialCapacity = (u32)(BATTERY_MAX_PUSH_CNT / 100) * (100 - percent);
    if (FLASH_Write_Battery() != SUCCESS) {
        DEBUG_TRACE(ERROR_TAG, "Battery Calib Flash Write FAIL");
        return;
    }
    ADC_BatterySglConvert(); /* 落盘判脏差=0 不重复写；重算百分比并触发上报 */
    DEBUG_TRACE(LOG_TAG, "Battery Calibrated To %d%% (used %u)", percent, batteryInitialCapacity);
    callback_BLEAck();
}

/**
 * @brief  0xA0/0xA1/0x8E帧入口（校验+分发）
 * @param  data：完整帧（含帧头与校验）
 * @param  len：帧长度
 * @retval
 */
static void ble_handle_lorawan_frame(u8 *data, u16 len)
{
    u16 i;
    u8 sum = 0;

    for (i = 0; i < len - 1; i++)
        sum += data[i];
    if (sum != data[len - 1]) {
        DEBUG_TRACE(ERROR_TAG, "BLE LoRaWAN Frame Checksum ERROR");
        return;
    }
    DebugHexInfo("RevBLE", data, len);

    if (data[5] == BLE_FR_LORAWAN_CFG) {
        if (len == 8 && data[6] == 0x01) {
            callback_BLELoRaWANCfg(); /* 查询：上报缓存配置 */
        } else if (len == 6 + BLE_CFG_PAYLOAD_LEN + 1) {
            if (upg_ble_active()) {
                DEBUG_TRACE(ERROR_TAG, "BLE Cfg Rejected: Upgrade In Progress");
                return; /* 升级会话期间禁止复位类操作 */
            }
            ble_apply_lorawan_cfg(data + 6);
        } else {
            DEBUG_TRACE(ERROR_TAG, "BLE Cfg Frame Length ERROR [%d]", len);
        }
    } else if (data[5] == BLE_FR_BATTERY_CALIB) {
        if (len == 8) {
            ble_apply_battery_calib(data[6]);
        } else {
            DEBUG_TRACE(ERROR_TAG, "BLE Battery Calib Frame Length ERROR [%d]", len);
        }
    } else if (data[5] == BLE_FR_BLE_DEBUG) {
        /* 0xA2 BLE调试模式：10字节定长载荷（帧总长17），仅蓝牙 */
        if (len == 6 + 10 + 1) {
            if (upg_ble_active()) {
                DEBUG_TRACE(ERROR_TAG, "BLE Debug Rejected: Upgrade In Progress");
                return; /* 升级会话期间禁止调试流（与0xA0/0xA1同纪律） */
            }
            if (data[6] != 0x00) {
                DEBUG_TRACE(ERROR_TAG, "BLE Debug Device Type ERROR [%d]", data[6]); /* 0=地磁，其他预留 */
                return;
            }
            if ((data[7] | data[8]) & ~0x01) {
                DEBUG_TRACE(ERROR_TAG, "BLE Debug Switch Value ERROR [%d %d]", data[7], data[8]); /* 开关仅0/1 */
                return;
            }
            BLEDebugMode = data[7];          /* 1=开调试透传（system_debug 周期输出） */
            device_t.ble_sleep_mode = data[8]; /* 1=连接状态时允许进睡眠 */
            DEBUG_TRACE(LOG_TAG, "BLE Debug Mode=%d SleepAllow=%d", BLEDebugMode, device_t.ble_sleep_mode);
            callback_BLEAck(); /* 0xFF ACK */
        } else {
            DEBUG_TRACE(ERROR_TAG, "BLE Debug Frame Length ERROR [%d]", len);
        }
    } else { /* BLE_FR_LORAWAN_SR */
        if (len == 8 && data[6] == 0x01) {
            if (upg_ble_active()) {
                DEBUG_TRACE(ERROR_TAG, "BLE SR Rejected: Upgrade In Progress");
                return; /* AT交互会阻塞主循环，升级会话期间禁用 */
            }
            ble_forward_module_sr();
        } else {
            DEBUG_TRACE(ERROR_TAG, "BLE SR Frame Length ERROR [%d]", len);
        }
    }
}

/**
 * @brief  处理来自蓝牙的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    if (upg_ble_rx(data, len)) { // 蓝牙升级帧（hkt+BIT3升级位）由升级状态机独占处理
        return;
    }

    /* 0xA0/0xA1/0x8E/0xA2 LoRaWAN配置/调试/电量校准/BLE调试帧：hkt+特殊类型0x00，与旧帧形(len在data[4..5])互不冲突 */
    if (len >= 8 && data[0] == SYNC_HEAD[0] && data[1] == SYNC_HEAD[1] && data[2] == SYNC_HEAD[2] && data[3] == 0x00 &&
        (data[5] == BLE_FR_LORAWAN_CFG || data[5] == BLE_FR_LORAWAN_SR || data[5] == BLE_FR_BATTERY_CALIB ||
         data[5] == BLE_FR_BLE_DEBUG)) {
        ble_handle_lorawan_frame(data, len);
        return;
    }

    if (device_t.power_on) {
        /* BLEDebugMode/ble_sleep_mode 已由 0xA2 帧控制（switchBLEDebug/Sleep 字符串命令已废弃） */
        if (strstr((const char *)data, "enterMagCalibrationMode")) // 进入地磁校准模式
        {
            magnetometer_enter_cal();
            PWR_CTRL_H;
            mDelay(500);
            BLE_SendCMD("CalibrationMode");
        } else if (strstr((const char *)data, "enterRKBCalibrationMode")) // 进入雷达波校准模式
        {
            rkb1145d_t.is_calibration = 1;
            PWR_CTRL_H;
            mDelay(500);
            BLE_SendCMD("RKBCalibrationMode");
        } else if (strstr((const char *)data, "switchRdON")) // 打开雷达模块
        {
            PWR_CTRL_H;
            mDelay(500);
            BLE_SendCMD("RdON");
        } else if (strstr((const char *)data, "switchRdOFF")) // 关闭雷达模块
        {
            PWR_CTRL_L;
            BLE_SendCMD("RdOFF");
        }
    }

    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10")) {
        char *valid = strstr((const char *)data, ":");
        u16 report_interval = atoi(valid + 1);
        if ((report_interval >= 1 && report_interval <= 1440) || report_interval == 0) {
            device_t.report_interval = report_interval;
						dataReportTimestamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    } else if (strstr((const char *)data, "getDeviceStatus")) { // 获取设备信息
        convert_light_sensor();
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X", HARDWARE_VER, SOFTWARE_VER);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "batteryLevel: %d", batteryLevel);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "powerStatus: %d", device_t.power_on);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "dataSyncPeriod: %d", device_t.report_interval);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "LoRaWANJoin: %s", "Class A");
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "DEVEUI: %02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
                LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "APPEUI: %02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3],
                LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "APPKEY: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1],
                LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8],
                LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14],
                LoRaWAN.AppKEY[15]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Join State: %d", LoRaWAN.joinState);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Park State: %d", device_t.park_state);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Park Mode: %d", device_t.park_mode);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "Tamper State: %d, Light Value: %d", device_t.tamper_alarm, convertedValue);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "MAG Base[%d, %d, %d], Cur[%d, %d, %d]", qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z, qmc5883l_t.xyz.x,
                qmc5883l_t.xyz.y, qmc5883l_t.xyz.z);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        // memset(tempBuf, 0, sizeof(tempBuf));
        // sprintf(tempBuf, "RKB [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", rkb1145d_t.power[0], rkb1145d_t.power[1], rkb1145d_t.power[2],
        // rkb1145d_t.power[3], rkb1145d_t.power[4],
        //         rkb1145d_t.power[5], rkb1145d_t.power[6], rkb1145d_t.power[7], rkb1145d_t.power[8], rkb1145d_t.power[9]);
        // BLE_SendCMD(tempBuf);
        // mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "RKB [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", rkb1145d_t.adjust_power[0], rkb1145d_t.adjust_power[1],
                rkb1145d_t.adjust_power[2], rkb1145d_t.adjust_power[3], rkb1145d_t.adjust_power[4], rkb1145d_t.adjust_power[5],
                rkb1145d_t.adjust_power[6], rkb1145d_t.adjust_power[7], rkb1145d_t.adjust_power[8], rkb1145d_t.adjust_power[9]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "BSE [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d]", rkb1145d_t.base_power[0], rkb1145d_t.base_power[1], rkb1145d_t.base_power[2],
                rkb1145d_t.base_power[3], rkb1145d_t.base_power[4], rkb1145d_t.base_power[5], rkb1145d_t.base_power[6], rkb1145d_t.base_power[7],
                rkb1145d_t.base_power[8], rkb1145d_t.base_power[9]);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "MAG State[%d], RKB State[%d]", qmc5883l_t.is_changed, rkb1145d_t.is_changed);
        BLE_SendCMD(tempBuf);
        mDelay(100);
    } else if (strstr((const char *)data, "getDevEui")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "DEVEUI:%02X%02X%02X%02X%02X%02X%02X%02X", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
                LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
        DEBUG_TRACE(LOG_TAG, "%s", tempBuf);
        if (!strstr(tempBuf, "DEVEUI:0000000000000000")) {
            BLE_SendCMD(tempBuf);
        }
        mDelay(100);
    } else if (strstr((const char *)data, "getProctName")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "ProctName:%s", PROCT_NAME);
        DEBUG_TRACE(LOG_TAG, "%s", tempBuf);
        BLE_SendCMD(tempBuf);
        mDelay(100);
    } else if (strstr((const char *)data, "switchPowerON")) { // 激活
        if (!device_t.power_on) {
            device_t.power_on = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            LPTIM1_Init();
            FLASH_Write_Park_Power();
        }
        BLE_SendCMD("PowerON");
    } else if (strstr((const char *)data, "switchPowerOFF")) { // 关机
        if (device_t.power_on) {
            device_t.power_on = 0;
            LPTIM1_Stop();
            FLASH_Write_Park_Power();
        }
        BLE_SendCMD("PowerOFF");
    } else if (strstr((const char *)data, "reboot")) {
        if (batterySavedCapacity != batteryInitialCapacity) /* 复位前补落盘 */
            FLASH_Write_Battery();
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "hkt") && (data[5] == 5 || data[5] == 4)) { // APP操作指令
        DebugHexInfo("RevBLE", data, len);
        // APP查询指令
        if (data[6] == 0xFF) {
            callback_BLEQuery();
        } else if (data[6] == 0x07) {
            callback_BLEMagData();
        } else if (data[6] == 0x08) {
            callback_BLERadarData();
        } else if (data[6] == 0x76) {
            callback_BLEDebugData();
        } else if (data[6] == 0xFC) {
            callback_BLEJoinData();
        } else if (data[6] == 0x09 || data[6] == 0x0A) { // APP开关机控制
            if (data[6] == 0x09) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                    LPTIM1_Init();
                    FLASH_Write_Park_Power();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power ON");
                }
            } else {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    LPTIM1_Stop();
                    FLASH_Write_Park_Power();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power OFF");
                }
            }
            callback_BLEPower();
        }

        // APP开关机控制
        else if (data[6] == 0xFE) {
            if (data[10] == 0x01) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                    LPTIM1_Init();
                    FLASH_Write_Park_Power();
                }
            } else if (data[10] == 0x00) {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    LPTIM1_Stop();
                    FLASH_Write_Park_Power();
                }
            }
            DEBUG_TRACE(LOG_TAG, "Recv BLE Power CMD [%02X]", data[10]);
            callback_BLEAck();
        }
        // APP执行校准命令
        else if (data[6] == 0xFD) {
            magnetometer_enter_cal();
            PWR_CTRL_H;
            DEBUG_TRACE(LOG_TAG, "Recv BLE MAG Reference CMD");
            callback_BLEAck();
        }
        // APP配置周期与工作模式
        else if (data[6] == 2 && data[5] == 4) {
            u16 reportInterval = data[7] << 8 | data[8];
            if ((reportInterval >= 1 && reportInterval <= 1440) || reportInterval == 0) {
                device_t.report_interval = reportInterval;
								dataReportTimestamp = Timestamp;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.report_interval);
            }
            u8 parkMode = data[9];
            if (parkMode >= 0 && parkMode <= 2) {
                device_t.park_mode = parkMode;
                FLASH_Write_Park_Mode();
                DEBUG_TRACE(LOG_TAG, "Update Device Park Mode Success: %d", device_t.park_mode);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update  Device Park Mode Error: %d", parkMode);
            }
            callback_BLEAck();
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 4 && data[6] == 6) { // 同步设备时间
        time_t stamp = data[7] << 24 | data[8] << 16 | data[9] << 8 | data[10];
        if (stamp != 0) {
            stamp += 8 * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            RTC_TimeSet();
            callback_BLEAck();
            DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time[%d], %04d-%02d-%02d-%d:%d:%d", stamp, systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "syncDeviceTimestamp")) { // 同步设备时间
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0) {
            stamp += 8 * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon + 1;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
            getTimestamp();
            RTC_TimeSet();
            DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon, systime.tm_mday, systime.tm_hour,
                        systime.tm_min, systime.tm_sec);
        }
    }
    // else {
    //     BLE_SendCMD(data);
    // }
}

/**
 * @brief  处理来自雷达模块的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromRdDataHandle(u8 *data, u16 len)
{
    if (data[0] == 0x52 && data[len - 1] == 0x46) // 确认为雷达模块返回数据
    {
        u8 sum = rkb_cal_sum(data, len - 3);
        // u16 datelen = len - 5;
        u16 distance;
        if (data[len - 2] == sum) {
            u8 cmd = data[2];
            u8 *valid = data + 3;
#if HIGH_LEVEL_DEBUG_ENABLE
            DEBUG_TRACE(LOG_TAG, "Recv RD Data CMD[%02X] Len[%d]", cmd, datelen);
#endif
            switch (cmd) {
            case 0x01: {
                distance = valid[0] << 8 | valid[1];
                valid += 2;
                rkb1145d_t.power[0] = valid[0] << 8 | valid[1];
                DEBUG_TRACE(LOG_TAG, "RD Distance[%dcm] Power[%d]", distance, rkb1145d_t.power[0]);
            } break;
            case 0x03: {
                u8 max_num;
                u16 max_power = 0;
                INFO("\r\n[");
                for (u8 i = 0; i < 10; i++) {
                    rkb1145d_t.power[i] = valid[0] << 8 | valid[1];
                    valid += 2;
                    INFO(" %d", rkb1145d_t.power[i]);

                    if (max_power < rkb1145d_t.power[i]) {
                        max_power = rkb1145d_t.power[i];
                        max_num = i;
                    }
                }
                rkb1145d_t.max_power_distance = max_num * 10;
                INFO(" ] [%d] [%d] power, max, distance", max_power, rkb1145d_t.max_power_distance);
            } break;
            default:
                break;
            }
        } else {
            DEBUG_TRACE(WARN_TAG, "Recv RD Data Sum Error[%02x]: %02x", data[len - 2], sum);
        }
    }
    // else
    // {
    //     DEBUG_TRACE(WARN_TAG, "Recv RD Unkown Data : %s", data);
    // }
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    char *valid;

    // device_t.sleep_delay = 30; // 串口有新命令时刷新进入睡眠时长
    /* 设置数据同步周期 */
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10")) {
        valid = strstr((const char *)data, ":");
        u16 report_interval = atoi(valid + 1);
        if (report_interval <= 1440 && report_interval >= 10) {
            device_t.report_interval = report_interval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 report_interval = atoi(valid + 1);
        if (report_interval <= 1440 && report_interval >= 10) {
            device_t.report_interval = report_interval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dm \r\n OK", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    }else if (strstr((const char *)data, "getDeviceStatus")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.report_interval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class A");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
                    LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                    LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                    LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                    LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:");
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:");
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:");
        } else {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.report_interval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class C");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2],
                    LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                    LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                    LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                    LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:%02x%02x%02x%02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2],
                    LoRaWAN.DevADDR[3]);
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppSKEY[0],
                    LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6],
                    LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12],
                    LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.NwkSKEY[0],
                    LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6],
                    LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12],
                    LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
        }
        DEBUG_TRACE(ACK_TAG, "%s", tempBuf);
    } else if (strstr((const char *)data, "syncDeviceTimestamp")) // 同步设备时间
    {
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0) {
            stamp += 8 * 60 * 60;
            struct tm *timeP;
            timeP = localtime(&stamp); // 转换
            systime.tm_year = timeP->tm_year + 1900;
            systime.tm_mon = timeP->tm_mon + 1;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
            getTimestamp();
            RTC_TimeSet();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon, systime.tm_mday, systime.tm_hour,
                        systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "keepMainRun")) // 保持不进入睡眠
    {
        keepRunFlag = 1;
    } else if (strstr((const char *)data, "leaveMainRun")) {
        keepRunFlag = 0;
    } else if (strstr((const char *)data, "switchBLEDebugON")) // 切换BLE Debug模式
    {
        BLEDebugMode = 1;
    } else if (strstr((const char *)data, "switchBLEDebugOFF")) // 切换BLE Debug模式
    {
        BLEDebugMode = 0;
    } else if (strstr((const char *)data, "enterMagCalibrationMode"))
    {
        memset(&qmc5883l_t, 0, sizeof(qmc5883l_t));
        magnetometer_enter_cal();
    } else if (strstr((const char *)data, "getDeviceNorFlash")) // 获取设备历史数据
    {
        nor_flash_read_export(&tsdb);
    } else {
        DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
        // device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY; // 错误命令
    }
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);

        // DebugHexInfo("LoRaWAN Recv", uartLoRa.recvData, uartLoRa.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartBle.recvTimeout && uartBle.recvLen) {
        DEBUG_TRACE(LOG_TAG, "Recv Uart BLE Data: %s", uartBle.recvData);
        fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }
    if (!uartRd.recvTimeout && uartRd.recvLen) {
        fromRdDataHandle(uartRd.recvData, uartRd.recvLen);
        memset(uartRd.recvData, 0, sizeof(uartRd.recvData));
        uartRd.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug

        // DebugHexInfo("Info Recv", uartInfo.recvData, uartInfo.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartInfo.recvData);

        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/******************************************************************************/
/******************************************************************************/

/**
 * @brief  环形缓冲区事件打印函数
 * @param
 * @retval
 */
void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len)
{
    switch (type) {
    case LWRB_EVT_RESET:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer reset event!");
        break;
    case LWRB_EVT_READ:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer read event: %d byte(s)!", (int)len);
        break;
    case LWRB_EVT_WRITE:
        DEBUG_TRACE(LWRB_TAG, "[EVT] Buffer write event: %d byte(s)!", (int)len);
        break;
    default:
        break;
    }
}

/**
 * @brief  环形缓冲区数据初始化
 * @param
 * @retval
 */
void lwrbDataInit(void)
{
    lwrb_init(&lwrbBuff_t, lwrb_data, sizeof(lwrb_data));
    lwrb_set_evt_fn(&lwrbBuff_t, my_buff_evt_fn);
}
