
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

#include "communicate.h"
#include "uart.h"
#include "flash.h"
#include "systick.h"
#include "tim.h"
#include "control_center.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "gpio.h"
#include "lwrb.h"
#include "gps.h"
#include "cJSON.h"
#include "tb_03f.h"
#include "acc.h"

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

struct run_mode system_schedule_t;

u8 sendNum;
u8 sendBuffer[220];

u8 factoryMode;
u8 keepRunFlag;
u8 packSyncNumber;

char tempBuf[500];

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

    switch (type)
    {
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
    case DATATYPE_DEVICE_TEMPERATURE_INFO: // 温度数据 数据放大100倍上传
        dest[1] = device_t.sensor_t.temperature >> 16;
        dest[2] = (device_t.sensor_t.temperature >> 8) & 0xFF;
        dest[3] = device_t.sensor_t.temperature & 0xFF;
        dataLen += 3;
        break;

    case DATATYPE_DEVICE_HUMIDITY_INFO: // 湿度数据 数据放大100倍上传
        dest[1] = device_t.sensor_t.humidity >> 16;
        dest[2] = (device_t.sensor_t.humidity >> 8) & 0xFF;
        dest[3] = device_t.sensor_t.humidity & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_X_ACC_VALUE: // x轴加速度值
        dest[1] = (acc_current.x >> 8) & 0xFF;
        dest[2] = acc_current.x & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_Y_ACC_VALUE: // y轴加速度值
        dest[1] = (acc_current.y >> 8) & 0xFF;
        dest[2] = acc_current.y & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_Z_ACC_VALUE: // z轴加速度值
        dest[1] = (acc_current.z >> 8) & 0xFF;
        dest[2] = acc_current.z & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_ANGLE_INFO: // 设备当前俯仰角
        break;
    case DATATYPE_DEVICE_UTC_TIMESTAMP: // UTC时间戳（GPS）
        break;
    case DATATYPE_DEVICE_LATITUDE: // 纬度
        dest[1] = device_t.sensor_t.latitude >> 24;
        dest[2] = (device_t.sensor_t.latitude >> 16) & 0xFF;
        dest[3] = (device_t.sensor_t.latitude >> 8) & 0xFF;
        dest[4] = device_t.sensor_t.latitude & 0xFF;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_LONGITUDE: // 经度
        dest[1] = device_t.sensor_t.longitude >> 24;
        dest[2] = (device_t.sensor_t.longitude >> 16) & 0xFF;
        dest[3] = (device_t.sensor_t.longitude >> 8) & 0xFF;
        dest[4] = device_t.sensor_t.longitude & 0xFF;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_GPS_MOVE_SPEED: // GPS移动速度
        break;
    case DATATYPE_DEVICE_GPS_AZIMUTH: // GPS方位角
        break;
    case DATATYPE_DEVICE_GPS_ALTITUDE: // 海拔
        break;
    case DATATYPE_DEVICE_STEP_NUMBER: // 步数
        dest[1] = device_t.sensor_t.step >> 8;
        dest[2] = device_t.sensor_t.step & 0xFF;
        dataLen += 2;
        device_t.sensor_t.step = 0;
        break;
    // case DATATYPE_DEVICE_BLE_RSSI: // 蓝牙信号强度
    //     break;
    // case DATATYPE_DEVICE_BLE_TAG_NUMBER: // 蓝牙标签数量
    //     break;
    // case DATATYPE_DEVICE_BLE_MAJOR_MINOR: // 蓝牙Major,Minor值
    //     break;
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆状态 0设备已安装 1设备未安装
        dest[1] = device_t.sensor_t.tamper_status;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_DATA: // 同步数据
        dest[1] = 0;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_RUN_CONFIG: // 运行模式上报
        dest[1] = system_schedule_t.runMode;
        dest[2] = (system_schedule_t.reportInterval[0] >> 8) & 0xFF;
        dest[3] = system_schedule_t.reportInterval[0] & 0xFF;
        dest[4] = system_schedule_t.start_hour[0];
        dest[5] = system_schedule_t.start_min[0];
        dest[6] = system_schedule_t.end_hour[0];
        dest[7] = system_schedule_t.end_min[0];

        dest[8] = (system_schedule_t.reportInterval[1] >> 8) & 0xFF;
        dest[9] = system_schedule_t.reportInterval[1] & 0xFF;
        dest[10] = system_schedule_t.start_hour[1];
        dest[11] = system_schedule_t.start_min[1];
        dest[12] = system_schedule_t.end_hour[1];
        dest[13] = system_schedule_t.end_min[1];

        dest[14] = (system_schedule_t.idleReportInterval >> 8) & 0xFF;
        dest[15] = system_schedule_t.idleReportInterval & 0xFF;
        dataLen += 15;
        break;
    // case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时）。
    //     // dest[1] = device_t.reportInterval >> 8;
    //     // dest[2] = device_t.reportInterval & 0xFF;

    //     dest[1] = device_t.gpsLocateInterval >> 8;
    //     dest[2] = device_t.gpsLocateInterval & 0xFF;
    //     dataLen += 2;
    //     break;
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
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;

    ADC_BatterySglConvert();

    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 5;

    mDelay(20);
    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
    mDelay(20);
    if (getLoRaWANBusyStat())
    {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __fail;
    }
    if (getLoRaWANSendStat())
    {
    __fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1))
        {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        packSyncNumber--;
        device_t.sendFailCnt++;
        if (device_t.sendFailCnt >= 5)
        {
            device_t.sendFailCnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        return;
    }
    device_t.sendFailCnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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
 * @brief  确认设备类型
 * @param  data：接收到数据的设备类型
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDeviceType(u8 *data)
{
    ErrorStatus status = SUCCESS;
    u16 type = data[0] << 8 | data[1];
    if (type != LoRaWAN_IR_PEOPLE_COUNTER)
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
    if (checkDataSyncHead(data) == ERROR)
    {
        DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
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

        ADC_BatterySglConvert();

        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
        if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
        /* 等待模组就绪 */
        while (!getLoRaWANBusyIOLevel())
            ;
        mDelay(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(20);
        if (getLoRaWANBusyStat())
        {
            DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
            goto __send_fail;
        }
        if (getLoRaWANSendStat())
        {
        __send_fail:
            DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
            packSyncNumber--;
            device_t.sendFailCnt++;
            if (device_t.sendFailCnt >= 5)
            {
                device_t.sendFailCnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                reconnect_stamp = Timestamp + 30 * 60;
                DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            }
            return;
        }
        device_t.sendFailCnt = 0;
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen)
    {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType)
        {
        case DATATYPE_DEVICE_REST_FACTORY:
        {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) // 复位LoRaWAN模块
            {
                LoRaWAN.port = 2;
                LoRaWAN.class = 0;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Write_LoRaWAN_Port_Class();
            }
            else if (restFactorySign == 2)
            {
                LoRaWAN.port = 2;
                LoRaWAN.class = 0;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Write_LoRaWAN_Port_Class();
                FLASH_Reset_Param();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);
        }
        break;
        case DATATYPE_DEVICE_SYNC_TIME: // 同步时间
        {
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
            gps_stamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
        break;
        case DATATYPE_DEVICE_RUN_CONFIG: // 运行模式配置
        {
            u8 vaild = 1;
            struct run_mode schedule_t;

            schedule_t.runMode = srcHex[1];
            schedule_t.reportInterval[0] = srcHex[2] << 8 | srcHex[3];
            schedule_t.start_hour[0] = srcHex[4];
            schedule_t.start_min[0] = srcHex[5];
            schedule_t.end_hour[0] = srcHex[6];
            schedule_t.end_min[0] = srcHex[7];

            schedule_t.reportInterval[1] = srcHex[8] << 8 | srcHex[9];
            schedule_t.start_hour[1] = srcHex[10];
            schedule_t.start_min[1] = srcHex[11];
            schedule_t.end_hour[1] = srcHex[12];
            schedule_t.end_min[1] = srcHex[13];

            schedule_t.idleReportInterval = srcHex[14] << 8 | srcHex[15];

            if (schedule_t.runMode == 0)
            {
                if (schedule_t.idleReportInterval > 1440 || schedule_t.idleReportInterval < 1)
                {
                    goto __config_fail;
                }
            }
            else if (schedule_t.runMode == 1)
            {
                // 时间段1
                if (schedule_t.reportInterval[0] > 1440 || schedule_t.reportInterval[0] < 1 ||
                    schedule_t.start_hour[0] >= 24 || schedule_t.start_min[0] >= 60 ||
                    schedule_t.end_hour[0] >= 24 || schedule_t.end_min[0] >= 60)
                {
                    goto __config_fail;
                }

                // 时间段2
                if (schedule_t.reportInterval[1] > 1440 || schedule_t.reportInterval[1] < 1 ||
                    schedule_t.start_hour[1] >= 24 || schedule_t.start_min[1] >= 60 ||
                    schedule_t.end_hour[1] >= 24 || schedule_t.end_min[1] >= 60)
                {
                    goto __config_fail;
                }

                if (schedule_t.idleReportInterval > 1440 || schedule_t.idleReportInterval < 1)
                {
                    goto __config_fail;
                }
            }
            else
            {
            __config_fail:
                vaild = 0;
                DEBUG_TRACE(ERROR_TAG, "Config Schedule Function Fail");
            }

            INFO("\r\nDevice Sync Schedule Mode[%d]:\n\tSchedule[1] %dmin [%d:%d-%d:%d]\n\tSchedule[2] %dmin [%d:%d-%d:%d]\n\tIdle Report Interval %dmin",
                 schedule_t.runMode, schedule_t.reportInterval[0], schedule_t.start_hour[0], schedule_t.start_min[0], schedule_t.end_hour[0], schedule_t.end_min[0],
                 schedule_t.reportInterval[1], schedule_t.start_hour[1], schedule_t.start_min[1], schedule_t.end_hour[1], schedule_t.end_min[1], schedule_t.idleReportInterval);

            if (vaild) // 数据有效
            {
                memcpy(&system_schedule_t, &schedule_t, sizeof(schedule_t));
                FLASH_Write_Gps_Locate_Interval();

                device_t.gpsLocateInterval = system_schedule_t.idleReportInterval;
                gps_stamp = Timestamp;
                DEBUG_TRACE(LOG_TAG, "Current GPS Report Interval %dmin", device_t.gpsLocateInterval);
            }

            dataLen -= 16;
            srcHex += 16;
        }
        break;
        // case DATATYPE_DEVICE_SYNC_PERIOD:
        // {
        //     u16 reportInterval = srcHex[1] << 8 | srcHex[2];
        //     if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0)
        //     {
        //         // device_t.reportInterval = reportInterval;
        //         device_t.gpsLocateInterval = reportInterval;
        //         // FLASH_Write_Report_Interval();
        //         FLASH_Write_Gps_Locate_Interval();
        //         DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.gpsLocateInterval);
        //     }
        //     else
        //     {
        //         DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.reportInterval);
        //     }
        //     dataLen -= 3;
        //     srcHex += 3;
        // }
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
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;
#if FUNC_THTB_ENABLE
    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HUMIDITY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;
#endif

#if FUNC_TAMPER_ENABLE
    dataLen = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex); // 防拆状态 0设备已安装 1设备未安装
    dataHex += dataLen;
    sendNum += dataLen;
#endif
    // if (device_t.sensor_t.latitude && device_t.sensor_t.longitude) //要求未获取到GPS信息是上报为0
    {
        dataLen = setDataPackage(DATATYPE_DEVICE_LATITUDE, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;

        dataLen = setDataPackage(DATATYPE_DEVICE_LONGITUDE, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;
    }
#if TRACK_ACC_ENABLE
    // if (device_t.sensor_t.step)
    {
        dataLen = setDataPackage(DATATYPE_DEVICE_STEP_NUMBER, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;
    }

    dataLen = setDataPackage(DATATYPE_DEVICE_X_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_Y_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_Z_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;


#endif
    // dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex); // 防拆状态 0设备已安装 1设备未安装
    // dataHex += dataLen;
    // sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_RUN_CONFIG, dataHex); // 运行状态上报
    dataHex += dataLen;
    sendNum += dataLen;

    sendLoRaWANData();
}

/**
 * @brief  主动同步数据
 * @param
 * @retval
 */
void Device_SyncData(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_DATA, dataHex); // 主动同步数据
    dataHex += dataLen;
    sendNum += dataLen;

    sendLoRaWANData();
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeciceEvent_Process(void)
{
    /* 更新电量信息 */
    if (device_t.updateBattery)
    {
        device_t.updateBattery = 0;
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, sendBuffer);

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1))
        {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }

    /* 发送防拆状态 */
    if (device_t.updateTamper)
    {
        device_t.updateTamper = 0;
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, sendBuffer);

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1))
        {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }

    /* 请求服务端同步本地时间 */
    if (device_t.syncLocalTime)
    {
        device_t.syncLocalTime = 0;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_SYNC_TIME, sendBuffer); // 回应ACK

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1))
        {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        else
        {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    if (LoRaWAN.joinState && !uartLoRa.sendDelay)
    {
        /* 检测缓冲器是否存在待发射数据 */
        if (lwrb_write_count) // 缓冲区内有数据
        {
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);

            sendLoRaWANData();

            if (--lwrb_write_count == 0)
            {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }
        }
        else if (device_t.syncDeviceState)
        {
            device_t.syncDeviceState = 0;
            Device_PeriodicReport();
            uartLoRa.sendDelay = 5 * SEC_DELAY;
        }
        else if (device_t.syncState)
        {
            device_t.syncState = 0;
            Device_SyncData();
            uartLoRa.sendDelay = 5 * SEC_DELAY;
        }
    }
    GpioEventProcess();
    DeciceEvent_Process();
}

/**
 * @brief  处理来自蓝牙端的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    u8 buffer[200] = {0};
    if (strstr((const char *)data, "+BLE_CONNECTED")) // 蓝牙连接
    {
        if (device_t.bleConnectState)
            return;
        device_t.bleConnectState = 1;

        // 发送设备信息给手机 DevEui 时间戳
        char DevEui[16];
        snprintf(DevEui, sizeof(DevEui), "%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);

        cJSON *JsonBuffer = NULL;
        JsonBuffer = cJSON_CreateObject();
        cJSON_AddStringToObject(JsonBuffer, "DevEui", DevEui);
        char ver[] = "v09.11";
        snprintf(ver, sizeof(ver), "v%2d.%2d", HARDWARE_VER, SOFTWARE_VER);
        cJSON_AddStringToObject(JsonBuffer, "versionCode", ver);
        cJSON_AddNumberToObject(JsonBuffer, "timestamp", Timestamp);
        char *s = cJSON_PrintUnformatted(JsonBuffer);
        Ble_SendCMD(s);

        if (s)
            cJSON_free((void *)s);
        if (JsonBuffer)
            cJSON_Delete(JsonBuffer);
    }
    else if (strstr((const char *)data, "+BLE_DISCONNECTED")) // 蓝牙断开
    {
        if (!device_t.bleConnectState)
            return;
        device_t.bleConnectState = 0;
        AT_Ble_setSleep(0);
    }
    else if (strstr((const char *)data, "+WAKEUP")) // 蓝牙唤醒
    {
        AT_Ble_setSleep(0);
    }
    else if (strstr((const char *)data, "+DATA="))
    {
        /* 获取有效数据 */
        memset(&splitBuf, 0, sizeof(splitBuf));
        char *validStr = strstr((const char *)data, "+DATA=") + 6;
        int validNum = 0;
        cmd_split(validStr, ",", splitBuf, &validNum);
        if (validNum == 2)
        {
            validNum = atoi(splitBuf[0]);
            /* 转换字符串为hex */
            cmd_get_hexstr2hex(buffer, sizeof(buffer), (const u8 *)splitBuf[1], strlen(splitBuf[1]));
        }
    }
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

    device_t.sleepDelay = 30; // 串口有新命令时刷新进入睡眠时长
    /* 设置数据同步周期 */
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:0"))
    {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0)
        {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval: %dm", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        }
        else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1"))
    {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0)
        {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dmin \r\n OK", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        }
        else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    }
    /* 设置LoRa参数 */
    else if (strstr((const char *)data, "LoRaWANPort") && strstr((const char *)data, "LoRaWANFreq") && strstr((const char *)data, "LoRaWANJoin"))
    {
        memset(&splitBuf, 0, sizeof(splitBuf));
        char *validStr = strstr((const char *)data, "LoRaWANPort:");
        int validNum = 0, i = 0;
        cmd_split(validStr, ",", splitBuf, &validNum);
        if (validNum == 8)
        {
            DEBUG_TRACE(ACK_TAG, "setLoRaWANParam OK");

            // LoRaWAN Port
            valid = strstr((const char *)splitBuf[0], ":");
            if (strlen(valid + 1) > 0 && strlen(valid + 1) <= 3)
            {
                LoRaWAN.port = atoi(valid + 1);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort: %d \r\n OK", LoRaWAN.port);
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Freq
            valid = strstr((const char *)splitBuf[1], ":");
            if (strlen(valid + 1) == 6)
            {
                char value[] = "868";
                snprintf(value, 3, "%s", valid + 1);
                LoRaWAN.frequency = atoi(value) * 1000;
                DEBUG_TRACE(ACK_TAG, "setLoRaWANFreq: %d \r\n OK", LoRaWAN.frequency);
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANPort Param ERROR");

            // LoRaWAN Join
            valid = strstr((const char *)splitBuf[2], ":");
            if (strlen(valid + 1) == 7)
            {
                if (strstr((const char *)valid + 1, "A"))
                    LoRaWAN.class = 0; // class A
                else
                    LoRaWAN.class = 2; // class C
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin: %d \r\n OK", LoRaWAN.class);
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANJoin Param ERROR");

            // LoRaWAN APPEUI
            valid = strstr((const char *)splitBuf[3], ":") + 1;
            if (strlen(valid) == 8)
            {
                /* 转换字符串为hex */
                for (i = 0; i < 8; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppEUI[i]);

                DEBUG_TRACE(LOG_TAG, "AppEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppKEY[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPEUI \r\n OK");
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAppEUI Param ERROR");

            // LoRaWAN APPKEY
            valid = strstr((const char *)splitBuf[4], ":") + 1;
            if (strlen(valid) == 16)
            {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
                            LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY \r\n OK");
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPKEY Param ERROR");

            // LoRaWAN DEVADDR
            valid = strstr((const char *)splitBuf[5], ":") + 1;
            if (strlen(valid) == 4)
            {
                /* 转换字符串为hex */
                for (i = 0; i < 4; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.DevADDR[i]);

                DEBUG_TRACE(LOG_TAG, "DEVADDR: %02x %02x %02x %02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR \r\n OK");
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANDEVADDR Param ERROR");

            // LoRaWAN APPSKEY
            valid = strstr((const char *)splitBuf[6], ":") + 1;
            if (strlen(valid) == 16)
            {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
                            LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY \r\n OK");
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY Param ERROR");

            // LoRaWAN NWKSKEY
            valid = strstr((const char *)splitBuf[7], ":") + 1;
            if (strlen(valid) == 16)
            {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.NwkSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "NWKSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
                            LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY \r\n OK");
            }
            else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANNWKSKEY Param ERROR");

            FLASH_Write_DevEUI_AppEUI_AppKEY();
            FLASH_Write_DevADDR_AppSKEY_NwkSKEY();
            FLASH_Write_LoRaWAN_Port_Class();
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
        }
        else
            DEBUG_TRACE(ACK_TAG, "LoRaWAN PARAM ERROR");
    }
    else if (strstr((const char *)data, "startFactoryTest"))
    {
        if (!factoryMode)
        {
            DEBUG_TRACE(ACK_TAG, "doorSensor startFactoryTest OK");
            factoryMode = 1;
            factoryTestMode();
        }
    }
    else if (strstr((const char *)data, "getDeviceStatue"))
    {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0)
        {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class A");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
                    LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:");
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:");
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:");
        }
        else
        {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class C");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.AppEUI[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
                    LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:%02x%02x%02x%02x",
                    LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1], LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
                    LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
                    LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
        }
        DEBUG_TRACE(ACK_TAG, "%s", tempBuf);
    }
    else if (strstr((const char *)data, "syncDeviceTimestamp")) // 同步设备时间
    {
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0)
        {
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
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD OK,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    }
    else if (strstr((const char *)data, "setGpsMode")) // 设置GPS模式
    {
        char *valid = strstr((const char *)data, ":") + 1;
        u8 mode = atoi(valid);
        if (mode >= GALAXY_CONFIG_GPS && mode <= GALAXY_CONFIG_GPS_BEIDOU_GLONASS)
        {
            DEBUG_TRACE(LOG_TAG, "Recv Uart Set Gps Mode: %d", mode);
            set_gps_galaxy(mode);
        }
    }
    else if (strstr((const char *)data, "keepMainRun")) // 保持不进入睡眠
    {
        keepRunFlag = 1;
    }
    else if (strstr((const char *)data, "leaveMainRun"))
    {
        keepRunFlag = 0;
    }
    else
    {
        fromBleDataHandle(data, len);
        // DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY; // 错误命令
    }
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen)
    {
        // if (getLoRaWANMode() == LoRaWAN_PASSTHROUGH_MODE)
        {
            fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);

            // DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
            // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

            memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
            uartLoRa.recvLen = 0;
        }
    }
    if (!uartGps.recvTimeout && uartGps.recvLen)
    {
        fromGpsDataHandle(uartGps.recvData, uartGps.recvLen); //
        memset(uartGps.recvData, 0, sizeof(uartGps.recvData));
        uartGps.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug

        // DebugHexInfo("LoRaWAN", uartInfo.recvData, uartInfo.recvLen);
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
    switch (type)
    {
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
