
/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "lwrb.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "CM1106SL.h"
#include "EPD.h"
#include "LTR_3XXALS_01.h"
#include "PMSA003.h"
#include "W25X40CLSNIG.h"
#include "cb_hcho_v4c.h"
#include "hp203b.h"
#include "im8601pa.h"
#include "scd4x.h"
#include "scd4x_i2c.h"
#include "sgp40_i2c.h"
#include "sgp4x.h"
#include "sht40x.h"
#include "sht4x_i2c.h"
#include "zmod441x.h"
#include "zmod451x.h"

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
u8 packSyncNumber;

char tempBuf[500];
time_t sensorConvertTimestamp;
time_t dataReportTimestamp;
time_t pirConvertTimestamp;
time_t getRTCTimestamp;

void thread_50ms_timer_init(void);

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
    case DATATYPE_DEVICE_SENSOR_REPORT_INTERVAL: // 传感器数据上报周期
        dest[1] = (device_t.sensorInterval >> 8) & 0xFF;
        dest[2] = device_t.sensorInterval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_TEMPERATURE_INFO: // 温度数据 数据放大100倍上传
        dest[1] = (device_t.sensor_t.temperature >> 16) & 0xFF;
        dest[2] = (device_t.sensor_t.temperature >> 8) & 0xFF;
        dest[3] = device_t.sensor_t.temperature & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_HUMIDITY_INFO: // 湿度数据 数据放大100倍上传
        dest[1] = (device_t.sensor_t.humidity >> 16) & 0xFF;
        dest[2] = (device_t.sensor_t.humidity >> 8) & 0xFF;
        dest[3] = device_t.sensor_t.humidity & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_PRESSURE: // 大气压
        dest[1] = (device_t.sensor_t.pressure >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.pressure & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_PIR_SIGNAL: // PIR信号
        dest[1] = device_t.sensor_t.pir_signal;
        dataLen += 1;
        device_t.sensor_t.pir_signal = 0; // 上报后清除
        break;
    case DATATYPE_DEVICE_AMBIENT_LIGHT_LEVEL: // 环境光强度等级
        dest[1] = device_t.sensor_t.light_level;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_PM2_5: // 标准大气压环境下，空气中PM2.5浓度
        dest[1] = (device_t.sensor_t.atmos_pm_2_5 >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.atmos_pm_2_5 & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_PM10: // 标准大气压环境下，空气中PM10浓度
        dest[1] = (device_t.sensor_t.atmos_pm_10_0 >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.atmos_pm_10_0 & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_HCHO_INFO: // 空气中甲醛浓度 数据放大1000倍上传
        dest[1] = (device_t.sensor_t.hcho_ppm >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.hcho_ppm & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_O3_LEVEL: // 空气中臭氧含量等级
        dest[1] = device_t.sensor_t.o3_level;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_CO2_INFO: // 空气中二氧化碳浓度
        dest[1] = (device_t.sensor_t.co2 >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.co2 & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_VOC_LEVEL: // 空气中TVOC含量等级
        dest[1] = device_t.sensor_t.tvoc_level;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_POWER_WAY: // 设备供电方式 0电池 1市电
        dest[1] = device_t.powerIn;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时），默认8小时。
        dest[1] = (device_t.reportInterval >> 8) & 0xFF;
        dest[2] = device_t.reportInterval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_EPD_SHOW_MODE:
        dest[1] = device_t.epdShowMode;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_LED_SHOW_MODE:
        dest[1] = device_t.ledShowMode;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_BEEP_STATUS:
        dest[1] = device_t.beepEnable;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_TEMPERATURE_SHOW_WAY:
        dest[1] = device_t.epdShowTempWay;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_EPD_OVERTURN_MODE:
        dest[1] = device_t.epdOverturnShow;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SENSOR_TASK_PERIOD:
        dest[1] = device_t.start_hour;
        dest[2] = device_t.start_min;
        dest[3] = device_t.end_hour;
        dest[4] = device_t.end_min;
        dest[5] = (device_t.sensorIntervalTask >> 8) & 0xFF;
        dest[6] = device_t.sensorIntervalTask & 0xFF;
        dest[7] = (device_t.sensorInterval >> 8) & 0xFF;
        dest[8] = device_t.sensorInterval & 0xFF;
        dataLen += 8;
        break;
    case DATATYPE_DEVICE_PIR_TASK_PERIOD:
        dest[1] = device_t.pirInterval;
        dest[2] = (device_t.reportInterval >> 8) & 0xFF;
        dest[3] = device_t.reportInterval & 0xFF;
        dataLen += 3;
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
 * @param
 * @retval
 */
void sendLoRaWANData(void)
{
    device_t.sleepLock = 1;
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;

#if FUNC_THREE_IN_ONE
    Calculate_Battery();
#endif

    memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    uartLoRa.sendLen = sendNum;
    memcpy(uartLoRa.sendData + 5, sendBuffer, uartLoRa.sendLen);
    uartLoRa.sendLen += 5;

    // 发送数据过程中防止出现PIR干扰
    if (pirConvertTimestamp < Timestamp + 15)
        pirConvertTimestamp = Timestamp + 15;

    tx_thread_sleep(20);
    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
    tx_thread_sleep(20);
    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "TX_FAIL_BUSY_TIMEOUT");
        LoRaWAN_ReportTxFail(TX_FAIL_BUSY_TIMEOUT);
        goto __fail;
    }
    if (getLoRaWANSendStat()) {
    __fail:
        DEBUG_TRACE(WARN_TAG, "TX_FAIL_UPLINK (uplink=%d, ack=%d)",
                    txRecover.uplink_fail_cnt, txRecover.ack_fail_cnt);
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        packSyncNumber--;
        if (txRecover.lastFailType != TX_FAIL_BUSY_TIMEOUT)
            LoRaWAN_ReportTxFail(TX_FAIL_UPLINK);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
        device_t.sleepLock = 0;
        return;
    }
    LoRaWAN_ReportTxSuccess();
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    device_t.sleepLock = 0;
}

/**
 * @brief  校验同步头
 * @param  data：接收到数据的数组
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDataSyncHead(u8 *data)
{
    ErrorStatus status = PASS;
    if (memcmp(data, SYNC_HEAD, 3) != 0)
        status = FAIL;
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
    if (checkDataSyncHead(data) == FAIL) {
        DEBUG_TRACE(ERROR_TAG, "Data Sync Head ERROR!");
        return;
    }

    if ((data[3] & 0x01) == 1) { // 服务器需要应答
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

        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
        if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
        /* 等待模组就绪 */
        while (!getLoRaWANBusyIOLevel())
            ;

        if (pirConvertTimestamp < Timestamp + 15)
            pirConvertTimestamp = Timestamp + 15;
        tx_thread_sleep(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        tx_thread_sleep(20);
        if (getLoRaWANBusyStat()) {
            DEBUG_TRACE(WARN_TAG, "TX_FAIL_BUSY_TIMEOUT");
            LoRaWAN_ReportTxFail(TX_FAIL_BUSY_TIMEOUT);
            goto __fail;
        }
        if (getLoRaWANSendStat()) {
        __fail:
            DEBUG_TRACE(WARN_TAG, "TX_FAIL_ACK (uplink=%d, ack=%d)",
                        txRecover.uplink_fail_cnt, txRecover.ack_fail_cnt);
            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendNum + 1)) {
                lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }
            packSyncNumber--;
            if (txRecover.lastFailType != TX_FAIL_BUSY_TIMEOUT)
                LoRaWAN_ReportTxFail(TX_FAIL_ACK);
            return;
        }
        LoRaWAN_ReportTxSuccess();
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_REBOOT: {
            if (srcHex[1] == 1) {
                NVIC_SystemReset();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Reboot CMD : %d", srcHex[1]);
        } break;
        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) {
                FLASH_Reset_Param();
                mDelay(100);
                NVIC_SystemReset();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);
        } break;
        case DATATYPE_DEVICE_SYNC_TIME: { // 同步时间
            systime.tm_year = srcHex[1] + 2000;
            systime.tm_mon = srcHex[2];
            systime.tm_mday = srcHex[3];
            systime.tm_hour = srcHex[4];
            systime.tm_min = srcHex[5];
            systime.tm_sec = srcHex[6];
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            RTC_Init();
            dataLen -= 7;
            srcHex += 7;
            sensorConvertTimestamp = Timestamp;
            pirConvertTimestamp =
                dataReportTimestamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
            if (device_t.epdShowMode != 0xFF) {
                device_t.epdShowDuCnt = 0;
                tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
            }
        } break;
        // case DATATYPE_DEVICE_SENSOR_REPORT_INTERVAL: { // 传感器数据上报周期
        //     u16 sensorInterval = srcHex[1] << 8 | srcHex[2];
        //     dataLen -= 3;
        //     srcHex += 3;
        //     if (sensorInterval >= 1 && sensorInterval <= 1440) {
        //         device_t.sensorInterval = sensorInterval;
        //         FLASH_Write_Report_Interval();
        //         DEBUG_TRACE(LOG_TAG, "Update Device Sensor Report Interval: %dmin", device_t.sensorInterval);
        //     } else {
        //         DEBUG_TRACE(ERROR_TAG, "Device Sensor Report Interval Over Threshold: %d", sensorInterval);
        //     }
        // } break;
        case DATATYPE_DEVICE_CO2_CALIBRATION: {
            u16 value = srcHex[1] << 8 | srcHex[2];
            if (value >= 400 && value <= 2000) {
                device_t.co2_calibration_value = value;
                device_t.co2_calibration_mode = 1;
            } else {
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendBuffer[0] = DATATYPE_DEVICE_CO2_CALIBRATION;
                sendBuffer[1] = 0xFE;
                sendNum = 2;
                sendLoRaWANData();
            }
            dataLen -= 3;
            srcHex += 3;
        } break;
        case DATATYPE_DEVICE_TEMPERATURE_OFFSET: {
            int8_t value = (int8_t)srcHex[1];

            memset(sendBuffer, 0, sizeof(sendBuffer));
            sendBuffer[0] = DATATYPE_DEVICE_TEMPERATURE_OFFSET;
            sendBuffer[1] = (unsigned char)value; // 显式转换为 unsigned char
            sendNum = 2;

            if (value >= -100 && value <= 100) {
                device_t.temperature_offset = value;
                FLASH_Write_Temperature_Offset();
                DEBUG_TRACE(LOG_TAG, "Update Device Temperature Offset: %d", value);
            } else {
                sendBuffer[1] = 0xFE; // 错误码
                DEBUG_TRACE(ERROR_TAG, "Device Sync Temperature Offset Over Threshold: %d", value);
            }
            sendLoRaWANData();
            dataLen -= 2;
            srcHex += 2;
        } break;
        // case DATATYPE_DEVICE_SYNC_PERIOD: {
        //     u16 reportInterval = srcHex[1] << 8 | srcHex[2];
        //     dataLen -= 3;
        //     srcHex += 3;
        //     if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
        //         device_t.reportInterval = reportInterval;
        //         FLASH_Write_Report_Interval();
        //         DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.reportInterval);
        //     } else {
        //         DEBUG_TRACE(ERROR_TAG, "Device Sync Interval Over Threshold: %d", reportInterval);
        //     }
        // } break;
        case DATATYPE_DEVICE_SENSOR_TASK_PERIOD: {
            u8 start_hour = srcHex[1];
            u8 start_min = srcHex[2];
            u8 end_hour = srcHex[3];
            u8 end_min = srcHex[4];
            u16 sensorIntervalTask = srcHex[5] << 8 | srcHex[6];
            u16 sensorInterval = srcHex[7] << 8 | srcHex[8];

            uint16_t start_time = start_hour * 60 + start_min;
            uint16_t end_time = end_hour * 60 + end_min;

            if (start_hour > 24 || start_min > 60 || end_hour > 24 || end_min > 60 || sensorInterval > 1440 ||
                sensorInterval < 1 || sensorIntervalTask > 1440 || sensorIntervalTask < 1 || start_time >= end_time) {
                DEBUG_TRACE(ERROR_TAG,
                            "Update Device Over Threshold Sensor Report Interval: %dmin, Task: %dmin ,[%02d:%02d-%02d:%02d]",
                            sensorInterval, sensorIntervalTask, start_hour, start_min, end_hour, end_min);
            } else {
                device_t.sensorInterval = sensorInterval;
                device_t.sensorIntervalTask = sensorIntervalTask;
                device_t.start_hour = start_hour;
                device_t.start_min = start_min;
                device_t.end_hour = end_hour;
                device_t.end_min = end_min;

                FLASH_Write_Report_Interval();
                DEBUG_TRACE(
                    LOG_TAG,
                    "Update Device Report Interval: %dmin, Sensor Report Interval: %dmin, Pir Interval: %dmin, Task: %dmin [%02d:%02d-%02d:%02d]",
                    device_t.reportInterval, device_t.sensorInterval, device_t.pirInterval, device_t.sensorIntervalTask, device_t.start_hour,
                    device_t.start_min, device_t.end_hour, device_t.end_min);
            }
            dataLen -= 9;
            srcHex += 9;
        } break;
        case DATATYPE_DEVICE_PIR_TASK_PERIOD: {
            u16 pirInterval = srcHex[1];
            u16 reportInterval = srcHex[2] << 8 | srcHex[3];
            if (pirInterval > 60 || pirInterval < 1 || reportInterval > 1440) {
                DEBUG_TRACE(ERROR_TAG, "Update Device Over Threshold Report Interval: %dmin, Pir Interval: %dmin", reportInterval, pirInterval);
            } else {
                device_t.reportInterval = reportInterval;
                device_t.pirInterval = pirInterval;

                FLASH_Write_Report_Interval();
                DEBUG_TRACE(
                    LOG_TAG,
                    "Update Device Report Interval: %dmin, Sensor Report Interval: %dmin, Pir Interval: %dmin, Task: %dmin [%02d:%02d-%02d:%02d]",
                    device_t.reportInterval, device_t.sensorInterval, device_t.pirInterval, device_t.sensorIntervalTask, device_t.start_hour,
                    device_t.start_min, device_t.end_hour, device_t.end_min);
            }
            dataLen -= 4;
            srcHex += 4;
        } break;
        case DATATYPE_DEVICE_LED_SHOW_MODE: {
            if (srcHex[1] <= 1) {
                device_t.ledShowMode = srcHex[1];
                device_t.ledShowModeWriteDelay = 5 * SEC_DELAY;
                DEBUG_TRACE(LOG_TAG, "Update Device Led Show Mode: %d", device_t.ledShowMode);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Led Show Mode Fail: %d", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_BEEP_STATUS: {
            if (srcHex[1] <= 1) {
                device_t.ledShowModeWriteDelay = 5 * SEC_DELAY;
                device_t.beepEnable = srcHex[1];
                DEBUG_TRACE(LOG_TAG, "Update Device Beep Enable: %d", device_t.beepEnable);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Beep Enable Fail: %d", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_TEMPERATURE_SHOW_WAY: {
            device_t.epdShowTempWay = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            DEBUG_TRACE(LOG_TAG, "Update Device Epd Show Temp Way: %d", device_t.epdShowTempWay);
            if (device_t.epdShowMode != 0xFF) {
                device_t.epdShowDuCnt = 0;
                tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
            }
        } break;
        case DATATYPE_DEVICE_EPD_SHOW_MODE: {
            device_t.epdShowMode = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            DEBUG_TRACE(LOG_TAG, "Update Device Epd Show Mode: %d", device_t.epdShowMode);
            device_t.epdShowDuCnt = 0;
            tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
        } break;
        case DATATYPE_DEVICE_EPD_OVERTURN_MODE: {
            device_t.epdOverturnShow = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            DEBUG_TRACE(LOG_TAG, "Update Device Epd Overturn Mode: %d", device_t.epdOverturnShow);
            device_t.epdShowDuCnt = 0;
            tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
        } break;
        default: // 遇到错误直接退出不解析
            dataLen = 0;
            break;
        }
        if (dataLen > 200) // 长度异常，数据出错，退出数据解析
            break;
    }
}

/**
 * @brief  传感器数据上报
 * @param
 * @retval
 */
void Device_SensorDataReport(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;

    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HUMIDITY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_CO2_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PIR_SIGNAL, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
    }
}

/**
 * @brief  周期同步上报数据
 * @param
 * @retval
 */
void Device_PeriodicReport(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;

    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER_WAY, dataHex); // 设备供电方式 1电池 0市电
    dataHex += dataLen;
    sendNum += dataLen;

    if (device_t.powerIn) {
        dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;
    }

    dataLen = setDataPackage(DATATYPE_DEVICE_LED_SHOW_MODE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BEEP_STATUS, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_SHOW_WAY, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_EPD_SHOW_MODE, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_EPD_OVERTURN_MODE, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_SENSOR_REPORT_INTERVAL, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SENSOR_TASK_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_PIR_TASK_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    device_t.updateBattery = 0;
    device_t.syncLedShowMode = 0;
    device_t.syncBeepEnable = 0;
    device_t.syncEpdOverturnMode = 0;
    device_t.syncEpdShowMode = 0;
    device_t.syncTempShowWay = 0;

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
    }
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeciceEvent_Process(void)
{
    /* 请求服务端同步本地时间 */
    if (device_t.syncLocalTime) {
        device_t.syncLocalTime = 0;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_SYNC_TIME, sendBuffer);

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }

    if (device_t.epdShowModeWriteDelay == 0) {
        device_t.epdShowModeWriteDelay = -1;
        FLASH_Write_EPD_Show_Mode();
    }
    if (device_t.ledShowModeWriteDelay == 0) {
        device_t.ledShowModeWriteDelay = -1;
        FLASH_Write_Device_Work_Mode();
    }

    if (dataReportTimestamp <= Timestamp && LoRaWAN.joinState) {
        dataReportTimestamp = Timestamp + device_t.reportInterval * 60;
        device_t.syncDeviceState = 1;
    }

    if (getRTCTimestamp <= Timestamp && LoRaWAN.joinState) {
        getRTCTimestamp = Timestamp + 12 * 3600;
        setLoRaWANMode(LoRaWAN_CMD_MODE); // 配置进入命令模式
        AT_LoRaWAN_getRTC();
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE); // 配置进入透传模式
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    if (!uartLoRa.sendDelay) {
        if (LoRaWAN.joinState) {
            /* 检测缓冲器是否存在待发射数据 */
            if (lwrb_write_count) // 缓冲区内有数据
            {
                u8 *dataHex = sendBuffer;
                lwrb_write_count--;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                lwrb_read(&lwrbBuff_t, &sendNum, 1);
                lwrb_read(&lwrbBuff_t, dataHex, sendNum);

                sendLoRaWANData();

                if (lwrb_write_count == 0) {
                    memset(lwrb_data, 0, sizeof(lwrb_data));
                    lwrb_reset(&lwrbBuff_t);
                }
            }
            /* 同步设备数据 */
            if (device_t.syncDeviceState) {
                device_t.syncDeviceState = 0;
                Device_PeriodicReport();
                uartLoRa.sendDelay = 5 * SEC_DELAY;
            }
            /* 上报传感器数据 */
            if (device_t.syncSensorData) {
                device_t.syncSensorData = 0;
                Device_SensorDataReport();
                uartLoRa.sendDelay = 5 * SEC_DELAY;
            }
            /* 更新电量信息 */
            if (device_t.updateBattery) {
                device_t.updateBattery = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }

            if (device_t.syncLedShowMode) {
                device_t.syncLedShowMode = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_LED_SHOW_MODE, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }

            if (device_t.syncBeepEnable) {
                device_t.syncBeepEnable = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_BEEP_STATUS, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }

            if (device_t.syncTempShowWay) {
                device_t.syncTempShowWay = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_SHOW_WAY, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }

            if (device_t.syncEpdOverturnMode) {
                device_t.syncEpdOverturnMode = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_EPD_OVERTURN_MODE, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }

            if (device_t.syncEpdShowMode) {
                device_t.syncEpdShowMode = 0;
                memset(sendBuffer, 0, sizeof(sendBuffer));
                sendNum = setDataPackage(DATATYPE_DEVICE_EPD_SHOW_MODE, sendBuffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendNum + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }
        }
    }
    DeciceEvent_Process();
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

    /* 设置数据同步周期 */
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if (reportInterval <= 1440 && reportInterval >= 10) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    } else if (strstr((const char *)data, "Set Sensor Interval:") && len <= strlen("Set Sensor Interval:1440") &&
               len >= strlen("Set Sensor Interval:1")) {
        valid = strstr((const char *)data, ":");
        u16 sensorInterval = atoi(valid + 1);
        if (sensorInterval <= 1440 && sensorInterval >= 1) {
            device_t.sensorInterval = sensorInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sensor Interval : %dm", device_t.sensorInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Sensor Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if (reportInterval <= 1440 && reportInterval >= 10) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dm \r\n OK", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    }
    /* 设置LoRa参数 */
    else if (strstr((const char *)data, "LoRaWANPort") && strstr((const char *)data, "LoRaWANFreq") && strstr((const char *)data, "LoRaWANJoin")) {
        memset(&splitBuf, 0, sizeof(splitBuf));
        char *validStr = strstr((const char *)data, "LoRaWANPort:");
        int validNum = 0, i = 0;
        cmd_split(validStr, ",", splitBuf, &validNum);
        if (validNum == 8) {
            DEBUG_TRACE(ACK_TAG, "setLoRaWANParam OK");

            // LoRaWAN Port
            valid = strstr((const char *)splitBuf[0], ":");
            if (strlen(valid + 1) > 0 && strlen(valid + 1) <= 3) {
                LoRaWAN.port = atoi(valid + 1);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort: %d \r\n OK", LoRaWAN.port);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Freq
            valid = strstr((const char *)splitBuf[1], ":");
            if (strlen(valid + 1) == 6) {
                char value[] = "868";
                snprintf(value, 3, "%s", valid + 1);
                LoRaWAN.frequency = atoi(value) * 1000;
                DEBUG_TRACE(ACK_TAG, "setLoRaWANFreq: %d \r\n OK", LoRaWAN.frequency);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Join
            valid = strstr((const char *)splitBuf[2], ":");
            if (strlen(valid + 1) == 7) {
                if (strstr((const char *)valid + 1, "A"))
                    LoRaWAN.class = 0; // class A
                else
                    LoRaWAN.class = 2; // class C
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin: %d \r\n OK", LoRaWAN.class);
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin Param ERROR");

            // LoRaWAN APPEUI
            valid = strstr((const char *)splitBuf[3], ":") + 1;
            if (strlen(valid) == 8) {
                /* 转换字符串为hex */
                for (i = 0; i < 8; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppEUI[i]);

                DEBUG_TRACE(LOG_TAG, "AppEUI: %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppKEY[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2],
                            LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPEUI \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAppEUI Param ERROR");

            // LoRaWAN APPKEY
            valid = strstr((const char *)splitBuf[4], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppKEY[0],
                            LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6],
                            LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12],
                            LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY Param ERROR");

            // LoRaWAN DEVADDR
            valid = strstr((const char *)splitBuf[5], ":") + 1;
            if (strlen(valid) == 4) {
                /* 转换字符串为hex */
                for (i = 0; i < 4; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.DevADDR[i]);
                DEBUG_TRACE(LOG_TAG, "DEVADDR: %02x %02x %02x %02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR Param ERROR");

            // LoRaWAN APPSKEY
            valid = strstr((const char *)splitBuf[6], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.AppSKEY[0],
                            LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6],
                            LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12],
                            LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY Param ERROR");

            // LoRaWAN NWKSKEY
            valid = strstr((const char *)splitBuf[7], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.NwkSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "NWKSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", LoRaWAN.NwkSKEY[0],
                            LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6],
                            LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12],
                            LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY Param ERROR");

            FLASH_Write_DevEUI_AppEUI_AppKEY();
            FLASH_Write_DevADDR_AppSKEY_NwkSKEY();
            FLASH_Write_LoRaWAN_Port_Class();
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
        } else
            DEBUG_TRACE(ACK_TAG, "LoRaWAN PARAM ERROR");
    } else if (strstr((const char *)data, "startFactoryTest")) {
        if (!factoryMode) {
            DEBUG_TRACE(ACK_TAG, "Air Sensor startFactoryTest OK");
            factoryMode = 1;
            factoryTestMode();
        }
    } else if (strstr((const char *)data, "getDeviceStatue")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", device_t.powerIn);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
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
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", device_t.powerIn);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
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
    }
    // 设置电子纸显示模式
    else if (strstr((const char *)data, "setShowMode")) {
        valid = strstr((const char *)data, ":");
        u16 mode = atoi(valid + 1);
#if !FUNC_THREE_IN_ONE
#if !FUNC_PM2_5
        if (mode <= 1)
#else
        if (mode <= 2) // 数据出错，超出范围
#endif
        {
            device_t.epdShowMode = mode;
            DEBUG_TRACE(ACK_TAG, "Set Device EPD Show Mode: %d \r\n OK", mode);
            if (device_t.epdShowMode != 0xFF) {
                tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
                device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            }
        } else
#endif
            DEBUG_TRACE(ACK_TAG, "Set Device EPD Show Mode ERROR");
    }
    // 设置温度显示单位
    else if (strstr((const char *)data, "setTempShowMode")) {
        valid = strstr((const char *)data, ":");
        u16 mode = atoi(valid + 1);
        if (mode <= 1) {
            device_t.epdShowTempWay = mode;
            DEBUG_TRACE(ACK_TAG, "Set Device Temp Show Way: %d \r\n OK", mode);
            if (device_t.epdShowMode != 0xFF) {
                tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
                device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            }
        } else
            DEBUG_TRACE(ACK_TAG, "Set Device Temp Show Way ERROR");
    } else if (strstr((const char *)data, "getDeviceNorFlash")) { // 获取设备历史数据
        nor_flash_read_export(&tsdb);
    } else if (strstr((const char *)data, "syncDeviceTimestamp")) { // 同步设备时间
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
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
            RTC_Init();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD, %04d-%02d-%02d-%d:%d:%d\r\n OK", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
            if (device_t.epdShowMode != 0xFF) {
                tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
                device_t.epdShowModeWriteDelay = 5 * SEC_DELAY;
            }
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 1) {
        FLASH_Write_Update_Flag();
        EPD_ShowFirmUpdate();
        mDelay(100);
        NVIC_SystemReset();
        // 复位
    } else if (strstr((const char *)data, "reboot")) { // v4 新增
        NVIC_SystemReset();                            // 复位
    } else if (strstr((const char *)data, "setCO2Forced:")) {
        valid = strstr((const char *)data, ":");
        u16 value = atoi(valid + 1);
        if (value >= 400 && value <= 2000) {
            device_t.co2_calibration_mode = 1;
            device_t.co2_calibration_value = value;
            DEBUG_TRACE(LOG_TAG, "Recv Uart CO2 Forced: %d", value);
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart CO2 Forced: %d", value);
    }
    // 设置温度偏移
    else if (strstr((const char *)data, "setTempOffset")) {
        valid = strstr((const char *)data, ":");
        int8_t value = atoi(valid + 1);
        if (value >= -100 && value <= 100) {
            device_t.temperature_offset = value;
            FLASH_Write_Temperature_Offset();
            DEBUG_TRACE(LOG_TAG, "Update Device Temperature Offset: %d", value);
        } else {
            sendBuffer[1] = 0xFE; // 错误码
            DEBUG_TRACE(ERROR_TAG, "Device Sync Temperature Offset Over Threshold: %d", value);
        }
    } else {
        DEBUG_TRACE(WARN_TAG, "Uart Failed To Parse Data!");
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

        DebugHexInfo("Recv LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }

    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

void DispthreadInfo(void);
/**
 * @brief  通讯管理任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_communicate(ULONG thread_input)
{
    (void)thread_input;
#if FUNC_THREAD_DEBUG
    u32 cnt;
#endif
    LoRaWAN_Init();
    IWDT_Clr();
    while (1) {
        LoRaWAN_JoinInit();
        LoRaWAN_TxRecoverProcess();
        fromUartDataHandle();
        DeviceStateProcess();
        tx_thread_sleep(100);
#if FUNC_THREAD_DEBUG
        if (++cnt >= 100) {
            cnt = 0;
            DispthreadInfo();
        }
#endif
    }
}

/**
 * @brief  传感器触发转换任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_sensor(ULONG thread_input)
{
    (void)thread_input;

    int dec_delay = 599;
    u8 dec_cnt = 0;
    int convert_cnt = 0;
    u8 air_level = 0;

    SHT40X_Init(); // thtb
#if CM1106SL
    CM1106SL_Init();
#else
    SCD4X_Init(); // co2
#endif

    IWDT_Clr();
    if (device_t.sleepDelay < 10)
        device_t.sleepDelay = 10;
    while (1) {
        if (device_t.sleepState) {
            tx_thread_sleep(10);
        } else {
            if (device_t.co2_calibration_mode) {
                if (device_t.sleepDelay < 10)
                    device_t.sleepDelay = 10;

                if (convert_cnt == 0) {
#if CM1106SL
#else
                    SCD4X_Start_Periodic();
#endif
                }
                tx_thread_sleep(5000);
#if CM1106SL
                cm_search_co2(0);
#else
                SCD4X_Read_Periodic_Data(0);
#endif
                if (convert_cnt++ >= 60) {
#if CM1106SL
#else
                    scd4x_stop_periodic_measurement();
#endif
                    tx_thread_sleep(500);
                    memset(sendBuffer, 0, sizeof(sendBuffer));
                    sendBuffer[0] = DATATYPE_DEVICE_CO2_CALIBRATION;
                    sendBuffer[1] = 0xFE;
                    sendNum = 2;
#if CM1106SL
                    if (cm_forced(device_t.co2_calibration_value) == true) {
                        sendBuffer[1] = 0xFF;
                    }
#else
                    if (SCD4X_ForcedPPM(device_t.co2_calibration_value) == true) {
                        sendBuffer[1] = 0xFF;
                    }
#endif
                    if (LoRaWAN.joinState) {
                        sendLoRaWANData();
                    } else {
                        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
                        int free_number = lwrb_get_free(&lwrbBuff_t);
                        if (free_number >= (sendNum + 1)) {
                            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
                            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
                            lwrb_write_count++;
                        } else {
                            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                        }
                    }

                    device_t.co2_calibration_mode = 0;
                    convert_cnt = 0;
                }
            } else {
                dec_delay++;
                if (dec_delay % 100 == 0 || sensorConvertTimestamp <= Timestamp) { // 唤醒时 10秒刷新一次
                    readSHT40XConvertResult();
                    readDCPowerInputStatus(); // 更新电源接入状态

                    // if (device_t.sensor_t.co2 < 1000 && device_t.sensor_t.temperature < 35000 && device_t.sensor_t.humidity < 60000) {
                    //     device_t.sensor_t.air_level = AIR_LEVEL_NORMAL;
                    // } else if (device_t.sensor_t.co2 < 1500 && device_t.sensor_t.temperature < 40000 && device_t.sensor_t.humidity < 70000) {
                    //     device_t.sensor_t.air_level = AIR_LEVEL_MILD;
                    // } else if (device_t.sensor_t.co2 >= 1500 || device_t.sensor_t.temperature >= 40000 || device_t.sensor_t.humidity >= 70000) {
                    //     device_t.sensor_t.air_level = AIR_LEVEL_BAD;
                    // }

                    if (device_t.sensor_t.co2 < 1000) {
                        device_t.sensor_t.air_level = AIR_LEVEL_NORMAL;
                    } else if (device_t.sensor_t.co2 < 1500) {
                        device_t.sensor_t.air_level = AIR_LEVEL_MILD;
                    } else if (device_t.sensor_t.co2 >= 1500) {
                        device_t.sensor_t.air_level = AIR_LEVEL_BAD;
                    }

                    if (!device_t.powerIn) {
                        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
                    }

                    // 状态发生变更后立马刷新屏幕
                    // if (sensorConvertTimestamp <= Timestamp || air_level != device_t.sensor_t.air_level || dec_delay >= 600) {
                    if (sensorConvertTimestamp <= Timestamp || air_level != device_t.sensor_t.air_level) {
                        air_level = device_t.sensor_t.air_level;
                        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
                        if (dec_delay >= 600)
                            dec_delay = 0;
#if CM1106SL
                        cm_search_co2(1);
#else
                        readSCD4XConvertResult(1);
#endif
                        tx_event_flags_set(&event_group, BIT_DISPLAY, TX_OR);
                        W25X40_ReleasePowerDown();
                        nor_flash_write(&tsdb); // 将传感器数据写入到外置nor flash
                        device_t.syncSensorData = 1;

                        if (!LoRaWAN.joinState) {
                            dec_cnt++;
                            if (dec_cnt > 2) {
                                dec_cnt = 0;
                                Calculate_Battery();
                            }
                        }

                        uint16_t start_time = device_t.start_hour * 60 + device_t.start_min;
                        uint16_t end_time = device_t.end_hour * 60 + device_t.end_min;
                        uint16_t cur_time = systime.tm_hour * 60 + systime.tm_min;
                        bool is_time_period_valid = (start_time + end_time) != 0; // 检查时间段是否有效

                        if (cur_time >= start_time && cur_time <= end_time && is_time_period_valid) {
                            sensorConvertTimestamp = Timestamp + device_t.sensorIntervalTask * 60;
                            pirConvertTimestamp = Timestamp + device_t.pirInterval * 60;
                        } else {

                            // 非工作时段，计算下次触发时间
                            uint16_t next_time = cur_time + device_t.sensorInterval;
                            if (is_time_period_valid && next_time >= start_time && cur_time < start_time) {
                                // 计算到工作开始时间的剩余分钟数
                                uint16_t minutes_until_start = start_time - cur_time;
                                sensorConvertTimestamp = Timestamp + minutes_until_start * 60;
                                pirConvertTimestamp = Timestamp + device_t.pirInterval * 60;
                            } else {
                                // 正常使用短间隔
                                sensorConvertTimestamp = Timestamp + device_t.sensorInterval * 60;
                                pirConvertTimestamp = Timestamp + device_t.pirInterval * 60;
                            }
                        }
                        DEBUG_TRACE(LOG_TAG, "Next Sensor Convert Timetstamp: %d", sensorConvertTimestamp);
                    } else {
                        if (device_t.powerIn) {
#if CM1106SL
                            cm_search_co2(0);
#else
                            readSCD4XConvertResult(0);
#endif
                        } else {
#if CM1106SL
                            cm_search_co2(1);
#else
                            readSCD4XConvertResult(1);
#endif
                        }
                    }
                }
                tx_thread_sleep(100);
            }
        }
    }
}

/**
 * @brief  低功耗模式任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_deepsleep(ULONG thread_input)
{
    (void)thread_input;

    u32 deep_tick = 0;
    time_t delay_stamp = 0;

    while (1) {
        tx_thread_sleep(100);
        if ((!device_t.pirEvent && !uartLoRa.sendDelay && !device_t.sleepDelay && (device_t.powerIn || poweroff_status) && !device_t.sleepLock &&
             ((LoRaWAN.joinState && !lwrb_write_count) || (!LoRaWAN.joinState && LoRaWAN.status != LoRaWAN_STATUS_NORMAL)))) {
            device_t.sleepState = 1;
            device_t.rtcWakeEvent = 0;
            device_t.extiWakeEvent = 0;
            IWDT_Clr();
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            // SCD4X_DeInit();
            // sht4x_soft_reset();

            if (poweroff_status) {
                EPD_ShowClean();
                EPD_PWR_CTRL_H;
            }

            W25X40_PowerDown();
            ENTER_DEEP_SLEEP();
            tx_timer_delete(&AppTimer);
            BSTIM_CR1_CEN_Setable(DISABLE); // 计数器失能
            LPTIM_Init();
            BEEP_OFF();
            lpmSleepTimeInit();
            GPIO_Sleep_Config();
            device_t.epdInit = 1;

        __sleep:
            if (poweroff_status) {
                poweroff_status = 2;
                PMU_SleepCfg();
                __WFI(); // 进入休眠
            } else {
                PIR_ReadEnd();
                device_t.pirEvent = 0;
                PMU_LpRunCfg();
                while (1) {
                    if (device_t.pirEvent) {
                        if (pirConvertTimestamp <= Timestamp) {
                            break;
                        } else {
                            PMU_CR_PMOD_Set(PMU_CR_PMOD_ACTIVE);
                            Init_SysClk_Gen();
                            PIR_ReadEnd();
                            // PMU_LpRunCfg();
                            CMU_SYSCLKCR_SYSCLKSEL_Set(CMU_SYSCLKCR_SYSCLKSEL_LSCLK);
                            PMU_CR_PMOD_Set(PMU_CR_PMOD_LPRUN);
                            device_t.pirEvent = 0;
                            IWDT_Clr();
                        }
                    }

                    if (!READ_FUNC_KEY_STATUS || !READ_RST_KEY_STATUS) {
                        device_t.extiWakeEvent = 1;
                    }

                    if (device_t.extiWakeEvent || device_t.rtcWakeEvent) {
                        break;
                    }

                    if (delay_stamp <= Timestamp) {
                        delay_stamp = Timestamp + 2;
                        IWDT_Clr();
                    }
                }
            }

            // 唤醒后重新初始化各外设
            // __disable_irq();
            IWDT_Clr();

            if (poweroff_status) {
                // if (!device_t.extiWakeEvent) {
                //  device_t.extiWakeEvent = 0;
                if (!READ_FUNC_KEY_STATUS) {
                    TicksDelayMs(500, NULL);
                    if (!READ_FUNC_KEY_STATUS) {
                        NVIC_SystemReset();
                    }
                }
                //}
                goto __sleep;
            }
            PMU_CR_PMOD_Set(PMU_CR_PMOD_ACTIVE);
            Init_SysClk_Gen();
            Uartx_Init(UART0, 115200); // 串口打印初始化
            sysTimeSwitchToRtcTime();
            if (device_t.rtcWakeEvent) {
                device_t.rtcWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
            } else if (device_t.extiWakeEvent) {
                device_t.extiWakeEvent = 0;
                if (!LoRaWAN.joinState && !poweroff_status) {
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
                    if (device_t.sleepDelay < MAX_WAIT_SLEEP_TIMEOUT)
                        device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
                }
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI");
            } else if (device_t.pirEvent) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From PIR");
                if (pirConvertTimestamp > Timestamp) {
                    device_t.pirEvent = 0;
                    IWDT_Clr();
                    goto __sleep;
                }
            } else {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From UnKnown");
                goto __sleep;
            }

            BSTIM_Init(1000, RCHFCLKCFG); // 定时1ms中断
            LPUART0_Init();
            // App_PortCfg();
            I2C_Init(I2C0);
#if CM1106SL
            Uartx_Init(UART3, 9600);
#endif

            BEEP_OFF();
            LPTIM_CR_EN_Setable(DISABLE); // 禁止低功耗定时器
            thread_50ms_timer_init();     // 重新启动定时器

            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            uartLoRa.sendDelay = 3 * SEC_DELAY; // 唤醒后延时8秒发送数据，保证传感器全部转换完成
            device_t.sleepState = 0;
        }
    }
}

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

/******************************************************************************/
/******************************************************************************/
