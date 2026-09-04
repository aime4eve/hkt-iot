
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "control_center.h"
#include "flash.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#include "lwrb.h"

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
u8 keepMode;
u8 packSyncNumber;

char tempBuf[500];

time_t heart_stamp;

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
    case DATATYPE_DEVICE_IR_REPORT_MODE: // 红外线人流量计数器上报模式
        dest[1] = device_t.reportMode;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_IR_SET_REPORT_INTERVAL: // 红外线人流量计数器时间间隔设定
        dest[1] = (u8)(device_t.reportIRInterval >> 8);
        dest[2] = (u8)(device_t.reportIRInterval & 0xFF);
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_IR_THRESHOLD_TOTAL_COUNT: // 红外线人流量计数器累计人数阈值设定
        dest[1] = (u8)(device_t.reportTotalCount >> 8);
        dest[2] = (u8)(device_t.reportTotalCount & 0xFF);
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_IR_REPORT_COUNT: // 红外线人流量计数器上报人数
        dest[1] = (u8)(device_t.counter_a_value >> 8);
        dest[2] = (u8)(device_t.counter_a_value & 0xFF);
        dest[3] = (u8)(device_t.counter_b_value >> 8);
        dest[4] = (u8)(device_t.counter_b_value & 0xFF);

        dest[5] = (device_t.counter_a_count >> 24) & 0xFF;
        dest[6] = (device_t.counter_a_count >> 16) & 0xFF;
        dest[7] = (device_t.counter_a_count >> 8) & 0xFF;
        dest[8] = device_t.counter_a_count & 0xFF;

        dest[9] = (device_t.counter_b_count >> 24) & 0xFF;
        dest[10] = (device_t.counter_b_count >> 16) & 0xFF;
        dest[11] = (device_t.counter_b_count >> 8) & 0xFF;
        dest[12] = device_t.counter_b_count & 0xFF;

        device_t.counter_a_value = 0;
        device_t.counter_b_value = 0;

        dataLen += 12;
        break;
    case DATATYPE_DEVICE_WORK_MODE: // 设备工作模式 0正常模式 1节能模式（低功耗模式）
        dest[1] = 1;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_FAULT_STATE: // 故障状态 0故障解除 1设备故障
        dest[1] = device_t.faultState;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆状态 0设备已安装 1设备未安装
        dest[1] = device_t.tamperAlarm;
        dataLen += 1;
        break;
        
    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：1- 1440 （1分钟到24小时），默认8小时。
        dest[1] = (u8)(device_t.reportInterval >> 8);
        dest[2] = device_t.reportInterval & 0xFF;
        dataLen += 2;
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
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;
    mDelay(100);

    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, uartLoRa.sendLen);
    uartLoRa.sendLen += 5;

    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);

    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto fail;
    }
    if (getLoRaWANSendStat()) {
    fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        uartLoRa.sendLen -= 5;
        lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, uartLoRa.sendLen);
        lwrb_write_count++;
        device_t.send_failcnt++;
        if (device_t.send_failcnt >= 5) {
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
    ADC_BatterySglConvert();
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
    u8 *validStr;
    u8 dataLen = 0;
    dataLen = len;
    validStr = data;
    /*  确认数据 */
    if (checkDataSyncHead(validStr) == ERROR) {
        // DEBUG_TRACE(ERROR_TAG, "Data Sync Head ERROR!");
        return;
    }

    if ((validStr[3] & 0x01) == 1) // 服务器需要应答
    {
        u8 *dataHex = sendBuffer;
        memset(sendBuffer, 0, sizeof(sendBuffer));

        sendNum = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
        dataHex += sendNum;
        uartLoRa.sendLen = sendNum;

        sendLoRaWANData();
    }

    /*  解析数据  */
    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = validStr + 5;
    dataLen -= 5;

    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_IR_REPORT_MODE: // 红外线人流量计数器上报模式
        {
            device_t.reportMode = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            FLASH_Write_Report_Mode();
            DEBUG_TRACE(LOG_TAG, "Update Device Report Mode : %d", device_t.reportMode);
        } break;
        case DATATYPE_DEVICE_IR_SET_REPORT_INTERVAL: // 红外线人流量计数器时间间隔设定
        {
            device_t.reportIRInterval = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;
            FLASH_Write_IRReport_Interval();
            DEBUG_TRACE(LOG_TAG, "Update Device Report Interval : %d", device_t.reportIRInterval);
        } break;
        case DATATYPE_DEVICE_IR_THRESHOLD_TOTAL_COUNT: // 红外线人流量计数器累计人数阈值设定
        {
            device_t.reportTotalCount = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;
            FLASH_Write_Report_Total_Count();
            DEBUG_TRACE(LOG_TAG, "Update Device Report Count : %d", device_t.reportTotalCount);
        } break;
        case DATATYPE_DEVICE_IR_REPORT_COUNT: // 服务器要求设备主动上报红外计数器
        {
            u8 dataTypeReport = srcHex[1];
            if (dataTypeReport == 1) // 服务器主动清零总计数后并上报
            {
                dataLen -= 1;
                srcHex += 1;
                device_t.counter_a_value = 0;
                device_t.counter_b_value = 0;
                device_t.counter_a_count = 0;
                device_t.counter_b_count = 0;
                DEBUG_TRACE(LOG_TAG, "Server Ask Zero Count");
            }
            dataLen -= 1;
            srcHex += 1;
            device_t.updateCounter = 1;
            DEBUG_TRACE(LOG_TAG, "Server Ask Report Count");
        } break;
        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) // 复位LoRaWAN模块
            {
                LoRaWAN.port = 10;
                LoRaWAN.class = 0;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Write_LoRaWAN_Port_Class();
            } else if (restFactorySign == 2) {
                LoRaWAN.port = 10;
                LoRaWAN.class = 0;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Write_LoRaWAN_Port_Class();
                FLASH_Reset_Param();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);
        } break;
        case DATATYPE_DEVICE_SYNC_TIME: // 同步时间
        {
            if (srcHex[1] > 100 || srcHex[2] > 11 || srcHex[3] > 31 || srcHex[4] > 23 || srcHex[5] > 59 || srcHex[6] > 59) {
                DEBUG_TRACE(ERROR_TAG, "Wrong time,Check sent data!");
            } else {
                RtcDate.Year = srcHex[1];
                RtcDate.Month = srcHex[2] + 1;
                RtcDate.Day = srcHex[3];
                RtcTime.Hours = srcHex[4];
                RtcTime.Minutes = srcHex[5];
                RtcTime.Seconds = srcHex[6];
                DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%d-%d-%d-%d-%d-%d", RtcDate.Year, RtcDate.Month, RtcDate.Day, RtcTime.Hours, RtcTime.Minutes, RtcTime.Seconds);
                setRtcTime(&RtcDate, &RtcTime);
                systime.tm_year = srcHex[1] + 2000;
                systime.tm_mon = srcHex[2];
                systime.tm_mday = srcHex[3];
                systime.tm_hour = srcHex[4];
                systime.tm_min = srcHex[5];
                systime.tm_sec = srcHex[6];
                systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
                getTimestamp();
            }
            dataLen -= 7;
            srcHex += 7;
        } break;
        case DATATYPE_DEVICE_TIME_EVENT: {
            if (srcHex[1] == 0) {
                device_t.timeEvent = 0;
                offAlma();
                DEBUG_TRACE(LOG_TAG, "Alma:off");
            } else if (srcHex[1] == 1) {
                if (srcHex[2] > 23 || srcHex[3] > 59) {
                    DEBUG_TRACE(ERROR_TAG, "wrong time,Check sent data!");
                } else {
                    device_t.timeEvent = 1;
                    setAlmaTime(srcHex[2], srcHex[3]);
                }
            }
            FLASH_Write_Time_Event();
            DEBUG_TRACE(LOG_TAG, "Recv Device Time Event CMD : %d %d %d", srcHex[1], srcHex[2], srcHex[3]);
            dataLen -= 3;
            srcHex += 3;
        } break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
                device_t.reportInterval = reportInterval;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval : %d", reportInterval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval Error: %d", reportInterval);
            }
        }
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
    memset(sendBuffer, 0, sizeof(sendBuffer));
    uartLoRa.sendLen = 0;

    sendNum = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex); // 红外线人流量计数器上报模式
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_IR_REPORT_MODE, dataHex); // 红外线人流量计数器上报模式
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_IR_SET_REPORT_INTERVAL, dataHex); // 红外线人流量计数器时间间隔设定
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_IR_THRESHOLD_TOTAL_COUNT, dataHex); // 红外线人流量计数器累计人数阈值设定
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_FAULT_STATE, dataHex); // 故障状态 0故障解除 1设备故障
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendLoRaWANData();
}

/**
 * @brief  上报红外计数值
 * @param
 * @retval
 */
void Device_IRCountReport(void)
{
    u8 *dataHex = sendBuffer;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    uartLoRa.sendLen = 0;

    sendNum = setDataPackage(DATATYPE_DEVICE_IR_REPORT_COUNT, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendLoRaWANData();
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeviceEvent_Process(void)
{
    /* 更新电量信息 */
    if (device_t.updateBattery) {
        device_t.updateBattery = 0;
        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, sendBuffer);

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }

    /* 发送故障状态 */
    if (device_t.updateFault) {
        device_t.updateFault = 0;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_FAULT_STATE, sendBuffer);

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }

    // if (device_t.syncTime)
    // {
    //     device_t.syncTime = 0;
    //     setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
    //     setLoRaWANMode(LoRaWAN_CMD_MODE); // 配置进入命令模式
    //     LoRa_DelayMs(500);
    //     AT_LoRaWAN_syncTime();
    //     setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE); // 配置进入透传模式
    //     setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    // }

    if (device_t.syncRTCTime) {
        device_t.syncRTCTime = 0;
        sysTimeSwitchToRtcTime();
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
                uartLoRa.sendLen = 0;
                memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
                memset(sendBuffer, 0, sizeof(sendBuffer));
                u8 *dataHex = sendBuffer;
                while (lwrb_write_count) {
                    // sendNum = lwrb_read(&lwrbBuff_t, &sendNum, 1);
                    lwrb_read(&lwrbBuff_t, &sendNum, 1);
                    lwrb_read(&lwrbBuff_t, dataHex, sendNum);
                    if (uartLoRa.sendLen + sendNum < 100) {
                        dataHex += sendNum;
                        lwrb_write_count--;
                        uartLoRa.sendLen += sendNum;
                    } else
                        break;
                }

                sendLoRaWANData();

                lwrb_write_count = 0;
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }

            /* 发送红外计数器 */
            else if (device_t.updateCounter) {
                device_t.updateCounter = 0;
                Device_IRCountReport();
                uartLoRa.sendDelay = 5 * SEC_DELAY;
            }

            else if (device_t.syncDeviceState) {
                device_t.syncDeviceState = 0;
                runMins = 0;
                Device_PeriodicReport();
                uartLoRa.sendDelay = 5 * SEC_DELAY;
            }
        }
    }
    DeviceEvent_Process();
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
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:0")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }
    /* 设置IR上报模式 */
    else if (strstr((const char *)data, "Set IR Report Mode:") && len == strlen("Set IR Report Mode::1")) {
        valid = strstr((const char *)data, ":");
        u8 reportMode = atoi(valid + 1);
        if (reportMode <= 4 && reportMode > 0) {
            device_t.reportMode = reportMode;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Mode : %d", device_t.reportMode);
            FLASH_Write_Report_Mode();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Mode Param ERROR");
    }
    /* 设置IR上报间隔 */
    else if (strstr((const char *)data, "Set IR Report Interval:") && len <= strlen("Set IR Report Interval:1440")) // 最大24小时
    {
        valid = strstr((const char *)data, ":");
        u16 reportIRInterval = atoi(valid + 1);
        if (reportIRInterval < 1440 && reportIRInterval > 0) {
            device_t.reportIRInterval = reportIRInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %d", device_t.reportIRInterval);
            FLASH_Write_IRReport_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interva Param ERROR");
    }
    /* 设置IR累计人数阈值*/
    else if (strstr((const char *)data, "Set IR Report Count:") && len <= strlen("Set IR Report Count:2000")) {
        valid = strstr((const char *)data, ":");
        u16 reportTotalCount = atoi(valid + 1);
        if (reportTotalCount < 2000 && reportTotalCount > 0) {
            device_t.reportTotalCount = reportTotalCount;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Count : %d", device_t.reportTotalCount);
            FLASH_Write_Report_Total_Count();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Count Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dm \r\n OK", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    }
    /* 设置IR上报模式 */
    else if (strstr((const char *)data, "setReportMode:") && len == strlen("setReportMode:1")) {
        valid = strstr((const char *)data, ":");
        u8 reportMode = atoi(valid + 1);
        if (reportMode <= 4 && reportMode > 0) {
            device_t.reportMode = reportMode;
            DEBUG_TRACE(ACK_TAG, "setReportMode: %d \r\n OK", device_t.reportMode);
            FLASH_Write_Report_Mode();
        } else
            DEBUG_TRACE(ACK_TAG, "setReportMode Param ERROR");
    }
    /* 设置IR上报间隔 */
    else if (strstr((const char *)data, "setReportInterval:") && len <= strlen("setReportInterval:1440")) // 最大24小时
    {
        valid = strstr((const char *)data, ":");
        u16 reportIRInterval = atoi(valid + 1);
        if (reportIRInterval < 1440 && reportIRInterval > 0) {
            device_t.reportIRInterval = reportIRInterval;
            DEBUG_TRACE(ACK_TAG, "setReportInterval: %ds \r\n OK", device_t.reportIRInterval);
            FLASH_Read_IRReport_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setReportInterval Param ERROR");
    }
    /* 设置IR累计人数阈值*/
    else if (strstr((const char *)data, "setPeopleThreshold:") && len <= strlen("setPeopleThreshold:2000")) {
        valid = strstr((const char *)data, ":");
        u16 reportTotalCount = atoi(valid + 1);
        if (reportTotalCount < 2000 && reportTotalCount > 0) {
            device_t.reportTotalCount = reportTotalCount;
            DEBUG_TRACE(ACK_TAG, "setPeopleThreshold: %d \r\n OK", device_t.reportTotalCount);
            FLASH_Write_Report_Total_Count();
        } else
            DEBUG_TRACE(ACK_TAG, "setPeopleThreshold Param ERROR");
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

                DEBUG_TRACE(LOG_TAG, "AppEUI: %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppKEY[0], LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5], LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPEUI \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAppEUI Param ERROR");

            // LoRaWAN APPKEY
            valid = strstr((const char *)splitBuf[4], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.AppKEY[i]);

                DEBUG_TRACE(LOG_TAG, "APPKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppKEY[0], LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5], LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7],
                            LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10], LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
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

                DEBUG_TRACE(LOG_TAG, "APPSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.AppSKEY[0], LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5], LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7],
                            LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10], LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14], LoRaWAN.AppSKEY[15]);
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY \r\n OK");
            } else
                DEBUG_TRACE(ACK_TAG, "setLoRaWANAPPSKEY Param ERROR");

            // LoRaWAN NWKSKEY
            valid = strstr((const char *)splitBuf[7], ":") + 1;
            if (strlen(valid) == 16) {
                /* 转换字符串为hex */
                for (i = 0; i < 16; i++)
                    cmd_string_to_hex(&valid[i], &LoRaWAN.NwkSKEY[i]);

                DEBUG_TRACE(LOG_TAG, "NWKSKEY: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
                            LoRaWAN.NwkSKEY[0], LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5], LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7],
                            LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10], LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14], LoRaWAN.NwkSKEY[15]);
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
            DEBUG_TRACE(ACK_TAG, "peopleCounter startFactoryTest OK");
            factoryMode = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
            factoryTestMode();
        }
    } else if (strstr((const char *)data, "getDeviceStatue")) {
        if (!keepMode) {
            keepMode = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
        }

        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
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
        } else {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
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
        sprintf(tempBuf + strlen(tempBuf), ",IRC01");
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
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
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
        DebugHexInfo("Recv", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
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
