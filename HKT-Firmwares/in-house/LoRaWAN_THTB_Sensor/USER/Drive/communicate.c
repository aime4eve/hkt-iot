
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

#include "communicate.h"
#include "flash.h"
#include "uart.h"
#include "systick.h"
#include "tim.h"
#include "control_center.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "gpio.h"
#include "lwrb.h"
#include "gps.h"
#include "ens210.h"
#include "lis2dh12.h"
#include "rtc.h"

#include <LoRaWAN_APPLY.h>

/***********            通讯协议数据结构             ***********/
/*--------------------------------------------------------------
|                                                               |
|   同步头   |   特殊类型   |   包序号   |   数据类型   |   数据   |
|                                                               |
---------------------------------------------------------------*/

u8 sendNum;
u8 sendBuffer[50];

u8 factoryMode;
u8 keepRunFlag;
u16 packSyncNumber;

char tempBuf[300];
char cacheBuf[50];

time_t sensor_stamp;

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
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void sendLoRaWANData(char *data)
{
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;
    HAL_Delay(20);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData: %s", data);
    LoRa_SendCMD(data);      // 发送数据
    ADC_BatterySglConvert(); // 每发射一次，电池容量减一

    if (getLoRaWANBusyStat())
    {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
    }
    if (getLoRaWANSendStat())
    {
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Cache Date To EEPROM: %s\r\n", cacheBuf);
        if (strlen(cacheBuf) > 3)
            FLASH_Write_Cache_Data((u8 *)cacheBuf, strlen(cacheBuf));
        device_t.send_failcnt++;
        if (device_t.send_failcnt >= 5)
        {
            device_t.send_failcnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
        return;
    }
    device_t.send_failcnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (packSyncNumber >= 65535)
    {
        packSyncNumber = 1;
    }
}

/**
 * @brief  校验同步头
 * @param  data：接收到数据的数组
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDataSyncHead(u8 *data)
{
    ErrorStatus status = SUCCESS;
    if (memcmp(data, "R", 1) != 0)
        status = ERROR;
    return status;
}

/**
 * @brief  确认设备类型
 * @param  data：接收到数据的设备类型
 * @retval  检验状态 1：成功 0：失败
 */
ErrorStatus checkDeviceDevEui(u8 *data)
{
    ErrorStatus status = SUCCESS;
    u8 deveui[8] = {0};
    cmd_get_hexstr2hex(deveui, 8, data, 16);
    if (memcmp(LoRaWAN.DevEUI, deveui, 8))
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
    u8 *validStr;
    int revlen;
#if FUNC_LoRaWAN_FIRM_HP
    u8 revbuf[100] = {0};
    u8 datalen = 0;

    validStr = (u8 *)strstr((const char *)data, "OK+RECV");
    if (validStr == NULL)
        return;
    memset(&splitBuf, 0, sizeof(splitBuf));
    cmd_split((char *)validStr, ",", splitBuf, &revlen);
    if (revlen >= 3)
    {
        cmd_get_hexstr2hex(&datalen, 1, (const u8 *)splitBuf[2], strlen(splitBuf[2]));
        cmd_get_hexstr2hex(revbuf, datalen, (const u8 *)splitBuf[3], datalen * 2);
        validStr = revbuf;
    }
#else
    validStr = (u8 *)strstr((const char *)data, "R,");
#endif
    /*  确认数据 */
    if (checkDataSyncHead(validStr) == ERROR)
    {
#if HIGH_LEVEL_DEBUG_ENABLE
        DEBUG_TRACE(ERROR_TAG, "Data Sync Head ERROR!");
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", validStr);
#endif
        return;
    }
    if (checkDeviceDevEui(validStr + 2) == ERROR)
    {
        DEBUG_TRACE(ERROR_TAG, "Data Check Device DevEui ERROR!");
#if HIGH_LEVEL_DEBUG_ENABLE
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", validStr);
#endif
        return;
    }

    int validNum = 0, i = 0, p = 0, type = 0, flash_signal = 0;
    char *splitRev[11] = {0};

    cmd_split((char *)validStr, ";", splitRev, &revlen);

    while (revlen >= p)
    {
        memset(&splitBuf, 0, sizeof(splitBuf));
        cmd_split(splitRev[p], ",", splitBuf, &validNum);

        if (p == 0)
        {
            i = 3;
            type = atoi(splitBuf[i]);
            DEBUG_TRACE(LOG_TAG, "Data First Recv Type %d", type);
            if (type == DATATYPE_DEVICE_101)
            {
                SendConfigInfo();
                return;
            }
            if (validNum < 5)
                return;
        }
        else
        {
            i = 0;
        }
        p++;
        type = atoi(splitBuf[i]);
        DEBUG_TRACE(LOG_TAG, "Data Recv Type %d", type);
        switch (type)
        {
        case DATATYPE_DEVICE_INTERVAL_READING: //
            device_t.reportInterval = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Report Interval %d", device_t.reportInterval);
            FLASH_Write_Report_Interval();
            break;
        case DATATYPE_DEVICE_LOW_THRESHOLD_TEMP_ALARM: // 温度过低报警
            device_t.system_config_t.temperature_alarm_low_threshold = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Temperature Alarm Low Threshold %d", device_t.system_config_t.temperature_alarm_low_threshold);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_HIGH_THRESHOLD_TEMP_ALARM: //  温度过高报警
            device_t.system_config_t.temperature_alarm_high_threshold = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Temperature Alarm High Threshold %d", device_t.system_config_t.temperature_alarm_high_threshold);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_LOW_THRESHOLD_HUMI_ALARM: // 湿度过低报警
            device_t.system_config_t.humidity_alarm_low_threshold = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Humidity Alarm Low Threshold %d", device_t.system_config_t.humidity_alarm_low_threshold);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_HIGH_THRESHOLD_HUMI_ALARM: //  湿度过高报警
            device_t.system_config_t.humidity_alarm_high_threshold = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Humidity Alarm High Threshold %d", device_t.system_config_t.humidity_alarm_high_threshold);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_BATTERY_LOW:
            device_t.system_config_t.battery_isable = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Battery Isable %d", device_t.system_config_t.battery_isable);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_MOVE_DETECTION: // 移动检测
            device_t.system_config_t.move_isable = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Move Isable %d", device_t.system_config_t.move_isable);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_SYSTEM_IS_RUN: // 解除报警
            device_t.system_config_t.system_isable = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv System Isable %d", device_t.system_config_t.system_isable);
            flash_signal = 1;
        case DATATYPE_DEVICE_GPS_REPORT_INTERVAL: // GPS上报间隔
            device_t.gpsLocateInterval = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv GPS Report Interval %d", device_t.gpsLocateInterval);
            FLASH_Write_Gps_Locate_Interval();
            break;
        case DATATYPE_DEVICE_ANGLE_THRESHOLD: // 变化角度
            device_t.system_config_t.angle_alarm_threshold = atoi(splitBuf[i + 1]);
            DEBUG_TRACE(LOG_TAG, "Data Recv Angle Alarm Threshold %d", device_t.system_config_t.angle_alarm_threshold);
            flash_signal = 1;
            break;
        case DATATYPE_DEVICE_OTA_RESET: // 空中复位
            FLASH_Reset_Param();
            break;
        default:
            break;
        }
    }

    if (flash_signal)
    {
        FLASH_Write_System_Config();
    }
    // SendConfigInfo();
}

/**
 * @brief  周期同步上报数据
 * @param
 * @retval
 */
void Device_PeriodicReport(void)
{
    sendPackFromSlave(DATATYPE_DEVICE_INTERVAL_READING);
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeciceEvent_Process(void)
{
    if (sensor_stamp <= Timestamp)
    {
        sensor_stamp = Timestamp + device_t.reportInterval * 60;
        ENS210_Convert(); // 温湿度转换
        StartAdCollect(); // 防拆检测
        device_t.syncDeviceState = 1;
        DEBUG_TRACE(LOG_TAG, "Next Temperature And Tamper Convert Timestamp: %d, Now Timestamp: %d", sensor_stamp, Timestamp);
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    DeciceEvent_Process();

    if (!uartLoRa.sendDelay)
    {
        if (device_t.syncDeviceState)
        {
            device_t.syncDeviceState = 0;
            Device_PeriodicReport();
            uartLoRa.sendDelay = 10 * SEC_DELAY;
        }
        else if (device_t.syncGpsState)
        {
            device_t.syncGpsState = 0;
            sendPackFromSlave(DATATYPE_DEVICE_GPS_INFO);
            uartLoRa.sendDelay = 10 * SEC_DELAY;
        }
        else if (LoRaWAN.joinState)
        {
            if (device_t.flash_data)
            {
                device_t.flash_data = 0;
                FLASH_Read_Cache_Data();
            }
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
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10"))
    {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if (reportInterval <= 1440 && reportInterval >= 10)
        {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.reportInterval);
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
        if (reportInterval <= 1440 && reportInterval >= 10)
        {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dm \r\n OK", device_t.reportInterval);
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
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", device_t.powerIn);
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
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", device_t.powerIn);
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
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
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
        DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
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
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
#if HIGH_LEVEL_DEBUG_ENABLE
        DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
#endif
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
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
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

// 发送GPS协议数据包
void sendPackFromSlave(int event)
{
    //"T,S1,0095690600002e25,1,27,21,E112.850830,N28.246389,2022.07.12 09:20:44,28.8"
    char send_buf[150] = {0};
    char dev_eui[16] = {0};
    char temp_buf[10] = {0};
    float battery_value;
    memset(cacheBuf, 0, sizeof(cacheBuf));

    // 拼接DEVEUI
    cmd_get_hex2hexstr((u8 *)dev_eui, 16, LoRaWAN.DevEUI, 8);
    strcat(send_buf, "T,S1,");
    strncat(send_buf, dev_eui, 16);
    strcat(send_buf, ",");

    // Software Version
    sprintf(temp_buf, "%d,", SOFTWARE_VER);
    strcat(send_buf, temp_buf);

    // 包序号
    sprintf(temp_buf, "%d,", packSyncNumber++);
    strcat(send_buf, temp_buf);

    switch (event)
    {
    case DATATYPE_DEVICE_GPS_INFO: // GPS数据
    {
        sprintf(temp_buf, "%d,", DATATYPE_DEVICE_GPS_INFO);
        strcat(send_buf, temp_buf);

        if (gps_l76k.longitude == 0 || gps_l76k.latitude == 0)
        {
            strcat(send_buf, "*,*,*,");
            strcat(cacheBuf, "*,*,*,");
        }
        else
        {
            sprintf(temp_buf, "%s%lf,", gps_l76k.longitude ? "E" : "W", fabs(gps_l76k.longitude));
            strcat(send_buf, temp_buf);
            strcat(cacheBuf, temp_buf);

            sprintf(temp_buf, "%s%lf,", gps_l76k.latitude ? "N" : "S", fabs(gps_l76k.latitude));
            strcat(send_buf, temp_buf);
            strcat(cacheBuf, temp_buf);

            sprintf(temp_buf, "%d.%02d.%02d %02d:%02d:%02d,", utctime.tm_year, utctime.tm_mon + 1, utctime.tm_mday,
                    utctime.tm_hour, utctime.tm_min, utctime.tm_sec);
            strcat(send_buf, temp_buf);
            strcat(cacheBuf, temp_buf);
        }
        sprintf(temp_buf, "%.1f", (float)device_t.sensor_t.temperature / 1000);
        strcat(send_buf, temp_buf);
        strcat(cacheBuf, temp_buf);
    }
    break;
    case DATATYPE_DEVICE_INTERVAL_READING:          //
    case DATATYPE_DEVICE_LOW_THRESHOLD_TEMP_ALARM:  // 温度过低报警
    case DATATYPE_DEVICE_HIGH_THRESHOLD_TEMP_ALARM: //  温度过高报警
    case DATATYPE_DEVICE_LOW_THRESHOLD_HUMI_ALARM:  // 湿度过低报警
    case DATATYPE_DEVICE_HIGH_THRESHOLD_HUMI_ALARM: //  湿度过高报警
    case DATATYPE_DEVICE_BATTERY_LOW:
    case DATATYPE_DEVICE_MOVE_DETECTION: // 移动检测
    case DATATYPE_DEVICE_TAMPER_ALARM:   // 解除报警
    case DATATYPE_DEVICE_CACHED_TEMP:    // 缓存温湿度数据
    {
        sprintf(temp_buf, "%d,", DATATYPE_DEVICE_INTERVAL_READING);
        battery_value = 2.6 + (float)batteryLevel / 100;
        if (device_t.sensor_t.move_state && device_t.system_config_t.move_isable) // 移动报警
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_MOVE_DETECTION);
        }
        else if (device_t.sensor_t.tamper_status)
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_TAMPER_ALARM);
        }
        else if (device_t.sensor_t.temperature < flash_config.temperature_alarm_low_threshold * 1000)
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_LOW_THRESHOLD_TEMP_ALARM);
        }
        else if (device_t.sensor_t.temperature > flash_config.temperature_alarm_high_threshold * 1000)
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_LOW_THRESHOLD_TEMP_ALARM);
        }
        else if (device_t.sensor_t.humidity < flash_config.temperature_alarm_low_threshold * 1000)
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_LOW_THRESHOLD_HUMI_ALARM);
        }
        else if (device_t.sensor_t.humidity > flash_config.temperature_alarm_high_threshold * 1000)
        {
            sprintf(temp_buf, "%d,", DATATYPE_DEVICE_HIGH_THRESHOLD_HUMI_ALARM);
        }
        else if (device_t.system_config_t.battery_isable)
        {
            if (battery_value < 2.8)
            {
                sprintf(temp_buf, "%d,", DATATYPE_DEVICE_BATTERY_LOW);
            }
        }
        strcat(send_buf, temp_buf); // 事件码

        // if (device_t.sensor_t.tamper_status || device_t.sensor_t.move_state || batteryLevel < 20)
        // {
        //     if (device_t.sensor_t.state < 2)
        //     {
        //         device_t.sensor_t.state++; //报警事件
        //     }
        // }
        // else
        // {
        //     if (device_t.sensor_t.state > 0 && device_t.sensor_t.state < 3)
        //     {
        //         device_t.sensor_t.state = 3; //停止报警
        //     }
        //     else
        //     {
        //         device_t.sensor_t.state = 0; //正常上报
        //     }
        // }
        if (device_t.sensor_t.tamper_status || device_t.sensor_t.move_state || batteryLevel < 20)
        {
            device_t.sensor_t.state = 1; // 报警事件
        }
        else
        {
            device_t.sensor_t.state = 0; // 正常上报
        }
        sprintf(temp_buf, "%d,", device_t.sensor_t.state);
        strcat(send_buf, temp_buf); // 报警状态
        strcat(cacheBuf, temp_buf);

        // 温湿度数据
        sprintf(temp_buf, "%.1f,", (float)device_t.sensor_t.temperature / 1000);
        strcat(send_buf, temp_buf);
        strcat(cacheBuf, temp_buf);
        sprintf(temp_buf, "%.1f,", (float)device_t.sensor_t.humidity / 1000);
        strcat(send_buf, temp_buf);
        strcat(cacheBuf, temp_buf);

        // 移动状态
        if (device_t.system_config_t.move_isable)
        {
            sprintf(temp_buf, "%d,", device_t.sensor_t.move_state);
            device_t.sensor_t.move_state = 0;
        }
        else
            sprintf(temp_buf, "%s,", "*");
        strcat(send_buf, temp_buf);
        strcat(cacheBuf, temp_buf);

        // 电量信息
        if (device_t.system_config_t.battery_isable)
        {
            // battery_value = 2.6 + batteryLevel / 100;
            sprintf(temp_buf, "%.1f", battery_value);
            // if (batteryLevel > 20)
            //     sprintf(temp_buf, "3.5");
            // else
            //     sprintf(temp_buf, "2.8");
        }
        else
            sprintf(temp_buf, "%s", "*");
        strcat(send_buf, temp_buf);
        strcat(cacheBuf, temp_buf);
    }
    break;
    default:
        break;
    }
    if (!LoRaWAN.joinState)
    {
        DEBUG_TRACE(WARN_TAG, "Write Cache Date To EEPROM: %s\r\n", cacheBuf);
        FLASH_Write_Cache_Data((u8 *)cacheBuf, strlen(cacheBuf));
        return;
    }
    sendLoRaWANData(send_buf);
}

// 发送配置信息
void SendConfigInfo(void)
{
    char send_buf[150] = {0};
    char dev_eui[16] = {0};
    char temp_buf[10] = {0};
    memset(cacheBuf, 0, sizeof(cacheBuf));

    // 拼接DEVEUI
    cmd_get_hex2hexstr((u8 *)dev_eui, 16, LoRaWAN.DevEUI, 8);

    strcat(send_buf, "R,");
    strncat(send_buf, dev_eui, 16);
    strcat(send_buf, ",");

    // Software Version
    sprintf(temp_buf, "%d,", SOFTWARE_VER);
    strcat(send_buf, temp_buf);

    // 包序号
    sprintf(temp_buf, "%d,", packSyncNumber++);
    strcat(send_buf, temp_buf);

    strcat(send_buf, "101,");

    sprintf(temp_buf, "1,%d;", device_t.reportInterval);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "2,%d;", device_t.system_config_t.temperature_alarm_low_threshold);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "3,%d;", device_t.system_config_t.temperature_alarm_high_threshold);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "4,%d;", device_t.system_config_t.humidity_alarm_low_threshold);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "5,%d;", device_t.system_config_t.humidity_alarm_high_threshold);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "6,%d;", device_t.system_config_t.battery_isable);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "7,%d;", device_t.system_config_t.move_isable);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "8,%d;", device_t.system_config_t.system_isable);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "9,%d;", device_t.gpsLocateInterval);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "9,%d;", device_t.gpsLocateInterval);
    strcat(send_buf, temp_buf);

    sprintf(temp_buf, "10,%d", device_t.system_config_t.angle_alarm_threshold);
    strcat(send_buf, temp_buf);

    sendLoRaWANData(send_buf);
}
