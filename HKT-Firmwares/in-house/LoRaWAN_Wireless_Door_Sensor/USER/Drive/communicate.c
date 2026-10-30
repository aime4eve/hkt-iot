
/* Includes ------------------------------------------------------------------*/
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
u8 sendBuffer[100];

u8 tamperState;
u8 doorState;

u8 factoryMode;
u8 keepMode;
u8 packSyncNumber;

char tempBuf[500];

time_t report_stamp;
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

    switch (type)
    {
    case DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION: // 设备软硬件版本
        dest[1] = HARDWARE_VER;
        dest[2] = SOFTWARE_VER;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_BATTERY_INFO: // 电量信息
        dest[1] = batteryLevel;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_DOOR_STATE: // 门磁状态 0设备已安装 1设备未安装
        dest[1] = device_t.doorState;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_DOOR_RUN_MODE:
        dest[1] = device_t.run_mode;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_DOOR_COUNT_TIME:
        dest[1] = device_t.c_time.start_hour;
        dest[2] = device_t.c_time.start_min;
        dest[3] = device_t.c_time.end_hour;
        dest[4] = device_t.c_time.end_min;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_DOOR_COUNT_INTERVAL:
        dest[1] = device_t.counter_intervel / 0xFF;
        dest[2] = device_t.counter_intervel % 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_DOOR_HEART_INTERVAL:
        dest[1] = device_t.heater_intervel;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_REPORT_DOOR_COUNT:
        dest[1] = device_t.doorOpenCountIntervel / 0xFF;
        dest[2] = device_t.doorOpenCountIntervel % 0xFF;

        dest[3] = device_t.doorOffCountIntervel / 0xFF;
        dest[4] = device_t.doorOffCountIntervel % 0xFF;

        dest[5] = (device_t.doorOpenCount >> 24) & 0xFF;
        dest[6] = (device_t.doorOpenCount >> 16) & 0xFF;
        dest[7] = (device_t.doorOpenCount >> 8) & 0xFF;
        dest[8] = device_t.doorOpenCount & 0xFF;

        dest[9] = (device_t.doorOffCount >> 24) & 0xFF;
        dest[10] = (device_t.doorOffCount >> 16) & 0xFF;
        dest[11] = (device_t.doorOffCount >> 8) & 0xFF;
        dest[12] = device_t.doorOffCount & 0xFF;

        device_t.doorOpenCountIntervel = 0;
        device_t.doorOffCountIntervel = 0;
        dataLen += 12;
        break;
    case DATATYPE_SERVER_SYNC_TIME:
        break;
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆状态 0设备已安装 1设备未安装
        dest[1] = device_t.tamperState;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK: // 设备应答ACK
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
    mDelay(20);

    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, uartLoRa.sendLen);
    uartLoRa.sendLen += 5;

    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);

    if (getLoRaWANBusyStat())
    {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto fail;
    }
    if (getLoRaWANSendStat())
    {
    fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        uartLoRa.sendLen -= 5;
        lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, uartLoRa.sendLen);
        lwrb_write_count++;
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
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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
    if (checkDataSyncHead(validStr) == ERROR)
    {
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
    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d", dataLen);
    u8 *srcHex = validStr + 5;
    dataLen -= 5;

    while (dataLen)
    {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : 0x%02x", dataType);
        switch (dataType)
        {
        case DATATYPE_DEVICE_DOOR_RUN_MODE: // 设置门磁运行模式
        {
            device_t.run_mode = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            FLASH_Write_Run_Mode();
            DEBUG_TRACE(LOG_TAG, "Update Device Run Mode : %d", device_t.run_mode);
            if (device_t.run_mode == 1)
            {
                if (Timestamp > 1659196800 &&
                    systime.tm_hour * 60 + systime.tm_min >= device_t.c_time.start_hour * 60 + device_t.c_time.start_min &&
                    systime.tm_hour * 60 + systime.tm_min <= device_t.c_time.end_hour * 60 + device_t.c_time.end_min)
                {
                    report_stamp = Timestamp + 60 * device_t.counter_intervel;
                    DEBUG_TRACE(LOG_TAG, "Update Report Timestamp: %ld", report_stamp);
                }
            }
        }
        break;
        case DATATYPE_DEVICE_DOOR_COUNT_TIME: // 设置门磁计数时间段
        {
            device_t.c_time.start_hour = srcHex[1];
            device_t.c_time.start_min = srcHex[2];
            device_t.c_time.end_hour = srcHex[3];
            device_t.c_time.end_min = srcHex[4];

            dataLen -= 5;
            srcHex += 5;
            FLASH_Write_Count_Time();
            DEBUG_TRACE(FLASH_TAG, "Update Device Count Time : %.2d:%.2d-%.2d:%.2d",
                        device_t.c_time.start_hour, device_t.c_time.start_min, device_t.c_time.end_hour, device_t.c_time.end_min);
        }
        break;
        case DATATYPE_DEVICE_DOOR_COUNT_INTERVAL: // 设置门磁计数间隔
        {
            device_t.counter_intervel = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;
            FLASH_Write_Report_Interval();
            DEBUG_TRACE(LOG_TAG, "Update Device Count Intervel: %d", device_t.counter_intervel);
        }
        break;
        case DATATYPE_DEVICE_DOOR_HEART_INTERVAL: // 设置门磁心跳间隔
        {
            device_t.heater_intervel = srcHex[1];
            dataLen -= 2;
            srcHex += 2;
            FLASH_Write_Heart_Interval();
            DEBUG_TRACE(LOG_TAG, "Update Device Heart Intervel: %d(0-24h,1-12h,2-8h)", device_t.heater_intervel);
        }
        break;
        case DATATYPE_DEVICE_REST_FACTORY:
        {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) // 复位LoRaWAN模块
            {
                LoRaWAN.port = 10;
                LoRaWAN.class = 0;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Write_LoRaWAN_Port_Class();
            }
            else if (restFactorySign == 2)
            {
                LoRaWAN.port = 10;
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
        case DATATYPE_SERVER_SYNC_TIME: // 同步时间
        {
            LL_RTC_DateTypeDef RtcDate;
            LL_RTC_TimeTypeDef RtcTime;
            RtcDate.Year=srcHex[1];
            RtcDate.Month=srcHex[2]+1;
            RtcDate.Day=srcHex[3];
            RtcTime.Hours=srcHex[4];
            RtcTime.Minutes=srcHex[5];
            RtcTime.Seconds=srcHex[6];
            setRtcTime(&RtcDate,&RtcTime);
            systime.tm_year = srcHex[1] + 2000;
            systime.tm_mon = srcHex[2];
            systime.tm_mday = srcHex[3];
            systime.tm_hour = srcHex[4];
            systime.tm_min = srcHex[5];
            systime.tm_sec = srcHex[6];
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon+1, systime.tm_mday);
            dataLen -= 7;
            srcHex += 7;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%02d:%02d:%02d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
            getTimestamp();
        }
        break;
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

    sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_STATE, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_RUN_MODE, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_COUNT_TIME, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_COUNT_INTERVAL, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_HEART_INTERVAL, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendNum = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex);
    dataHex += sendNum;
    uartLoRa.sendLen += sendNum;

    sendLoRaWANData();
}

/**
 * @brief  上报门磁计数值
 * @param
 * @retval
 */
void Device_DoorCountReport(void)
{
    u8 *dataHex = sendBuffer;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    uartLoRa.sendLen = 0;

    sendNum = setDataPackage(DATATYPE_DEVICE_REPORT_DOOR_COUNT, dataHex);
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
    if (device_t.updateBattery)
    {
        device_t.updateBattery = 0;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, sendBuffer);

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }

    /* 发送防拆状态 */
    if (device_t.tamperEvent)
    {
        device_t.tamperEvent = 0;

        if (device_t.tamperState == getTamperState())
        {
            mDelay(20);
            if (device_t.tamperState == getTamperState())
            {
                device_t.tamperState = !getTamperState();
                device_t.touchEvent = 1;
            }
        }
        // memset(sendBuffer, 0, sizeof(sendBuffer));
        // sendNum = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, sendBuffer);

        // lwrb_write(&lwrbBuff_t, &sendNum, 1); //写入数据长度
        // lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        // lwrb_write_count++;
    }

    /* 发送门磁状态 */
    if (device_t.doorEvent)
    {
        device_t.doorEvent = 0;
        if (device_t.doorState == getDoorSensorState())
        {
            mDelay(20);
            if (device_t.doorState == getDoorSensorState())
            {
                device_t.doorState = !getDoorSensorState();
                // 计数模式且处于计数时间段内且设置好时间为2022.08后才开始计数
                if (device_t.run_mode == 1 && Timestamp > 1659196800 &&
                    systime.tm_hour * 60 + systime.tm_min >= device_t.c_time.start_hour * 60 + device_t.c_time.start_min &&
                    systime.tm_hour * 60 + systime.tm_min <= device_t.c_time.end_hour * 60 + device_t.c_time.end_min)
                {
                    if (device_t.doorState)
                    {
                        device_t.doorOpenCount++;
                        device_t.doorOpenCountIntervel++;
                    }
                    else
                    {
                        device_t.doorOffCount++;
                        device_t.doorOffCountIntervel++;
                    }
                }
                else
                    device_t.touchEvent = 1;
            }
        }
        // memset(sendBuffer, 0, sizeof(sendBuffer));
        // sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_STATE, sendBuffer);

        // lwrb_write(&lwrbBuff_t, &sendNum, 1); //写入数据长度
        // lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        // lwrb_write_count++;
    }
    /* 请求服务端同步本地时间 */
    if (device_t.syncLocalTime)
    {
        device_t.syncLocalTime = 0;

        memset(sendBuffer, 0, sizeof(sendBuffer));
        sendNum = setDataPackage(DATATYPE_SERVER_SYNC_TIME, sendBuffer);

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }

    if (device_t.touchEvent)
    {
        device_t.touchEvent = 0;
        /* 模式二下立即上报 */
        if (device_t.run_mode == 2 ||
            (systime.tm_hour * 60 + systime.tm_min < device_t.c_time.start_hour * 60 + device_t.c_time.start_min ||
             systime.tm_hour * 60 + systime.tm_min > device_t.c_time.end_hour * 60 + device_t.c_time.end_min))
        {
            memset(sendBuffer, 0, sizeof(sendBuffer));
            sendNum = setDataPackage(DATATYPE_DEVICE_DOOR_STATE, sendBuffer);
            sendNum += setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, sendBuffer + sendNum);
            sendNum += setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, sendBuffer + sendNum);

            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
            device_t.doorOpenCountIntervel = 0;
            device_t.doorOffCountIntervel = 0;
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
    // static u8 checkLinkNum;

    if (heart_stamp <= Timestamp)
    {
        switch (device_t.heater_intervel)
        {
        case 0:
            heart_stamp = Timestamp + 60 * 60 * 24;
            break;
        case 1:
            heart_stamp = Timestamp + 60 * 60 * 12;
            break;
        case 2:
            heart_stamp = Timestamp + 60 * 60 * 8;
            break;
        default:
            heart_stamp = Timestamp + 60 * 60 * 24;
            break;
        }
        DEBUG_TRACE(LOG_TAG, "Update Heart Timestamp: %ld", heart_stamp);
        device_t.syncDeviceState = 1;
    }

    // 时间正常才会开始计数  20220800
    if (device_t.run_mode == 1 && report_stamp <= Timestamp && Timestamp > 1659196800 &&
        systime.tm_hour * 60 + systime.tm_min >= device_t.c_time.start_hour * 60 + device_t.c_time.start_min &&
        systime.tm_hour * 60 + systime.tm_min <= device_t.c_time.end_hour * 60 + device_t.c_time.end_min)
    {
        report_stamp = Timestamp + 60 * device_t.counter_intervel;
        DEBUG_TRACE(LOG_TAG, "Update Report Timestamp: %ld", report_stamp);
        device_t.syncDoorCount = 1;
    }

    DeviceEvent_Process();

    if (!uartLoRa.sendDelay)
    {
        if (LoRaWAN.joinState)
        {
            /* 检测缓冲器是否存在待发射数据 */
            if (lwrb_write_count) // 缓冲区内有数据
            {
                uartLoRa.sendLen = 0;
                memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
                memset(sendBuffer, 0, sizeof(sendBuffer));
                u8 *dataHex = sendBuffer;
                while (lwrb_write_count)
                {
                    // sendNum = lwrb_read(&lwrbBuff_t, &sendNum, 1);
                    lwrb_read(&lwrbBuff_t, &sendNum, 1);
                    lwrb_read(&lwrbBuff_t, dataHex, sendNum);
                    if (uartLoRa.sendLen + sendNum < 100)
                    {
                        dataHex += sendNum;
                        lwrb_write_count--;
                        uartLoRa.sendLen += sendNum;
                    }
                    else
                        break;
                }

                sendLoRaWANData();

                lwrb_write_count = 0;
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }

            else if (device_t.syncDeviceState)
            {
                device_t.syncDeviceState = 0;
                Device_PeriodicReport();
            }

            else if (device_t.syncDoorCount)
            {
                device_t.syncDoorCount = 0;
                Device_DoorCountReport();
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
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:86400") && len >= strlen("Set Report Interval:60"))
    {
        valid = strstr((const char *)data, ":");
        u32 counter_intervel = atoi(valid + 1);
        if (counter_intervel <= 86400 && counter_intervel >= 60)
        {
            device_t.counter_intervel = counter_intervel;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %ds", device_t.counter_intervel);
            FLASH_Write_Report_Interval();
        }
        else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:86400") && len >= strlen("setDataSyncPeriod:60"))
    {
        valid = strstr((const char *)data, ":");
        u32 counter_intervel = atoi(valid + 1);
        if (counter_intervel <= 86400 && counter_intervel >= 60)
        {
            device_t.counter_intervel = counter_intervel;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %ds \r\n OK", device_t.counter_intervel);
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
    else if (strstr((const char *)data, "keepMainRun"))
    {
        if (!keepMode)
        {
            keepMode = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
        }
    }
    else if (strstr((const char *)data, "leaveMainRun"))
    {
        if (keepMode)
        {
            keepMode = 0;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
        }
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
        if (!keepMode)
        {
            keepMode = 1;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_ERROR;
        }
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0)
        {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.counter_intervel);
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
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.counter_intervel);
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
        sprintf(tempBuf + strlen(tempBuf), ",DMS01");
        DEBUG_TRACE(ACK_TAG, "%s", tempBuf);
    }
    else if (strstr((const char *)data, "syncDeviceTimestamp")) // 同步设备时间
    {
        char *valid = strstr((const char *)data, ":") + 1;
        time_t stamp = atoi(valid);
        if (stamp != 0)
        {
            stamp += 8 * 60 * 60;
            struct tm *t;
            t = localtime(&stamp); // 转换
            systime.tm_year = t->tm_year + 1900;
            systime.tm_mon = t->tm_mon;
            systime.tm_mday = t->tm_mday;
            systime.tm_hour = t->tm_hour;
            systime.tm_min = t->tm_min;
            systime.tm_sec = t->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon+1, systime.tm_mday);
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    }
    else if (strstr((const char *)data, "LoRaStream:"))
    {
        char *valid = strstr((const char *)data, ":") + 1;
        int state = 0;
        keepMode = 1;
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        {
            state = 1;
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
        }
        mDelay(20);
        LoRa_SendCMD(valid);
        LoRa_SendCMD("\r\n");
        if (state == 1)
        {
            setLoRaWANMode(LoRaWAN_CMD_MODE);
        }
    }
    else if (strstr((const char *)data, "LoRaAT:"))
    {
        char *valid = strstr((const char *)data, ":") + 1;
        int state = 0;
        keepMode = 1;
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
        if (getLoRaWANMode() == LoRaWAN_PASSTHROUGH_MODE)
        {
            state = 1;
            setLoRaWANMode(LoRaWAN_CMD_MODE);
        }
        mDelay(20);
        LoRa_SendCMD(valid);
        LoRa_SendCMD("\r\n");
        if (state == 1)
        {
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
        }
    }
    else
    {
        DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
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
        // DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        // DebugHexInfo("LoRaWAN", uartInfo.recvData, uartInfo.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartInfo.recvData);

        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/**
 * @brief  睡眠模式处理逻辑
 * @param
 * @retval
 */
void DeviceSleepEventProcess(void)
{
    if (!factoryMode && !keepMode &&
        !device_t.sleepDelay && !uartLoRa.sendDelay &&
        !device_t.touchEvent && !device_t.tamperEvent && !device_t.doorEvent &&
        (LoRaWAN.joinState || (!LoRaWAN.joinState && LoRaWAN.status != LoRaWAN_STATUS_NORMAL)))
    {
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        systemLowPowerMode_Config();
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
