
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "cJSON.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "gps.h"
#include "lwrb.h"
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

time_t dataReportTimestamp;
time_t dataConvertTimestamp;

u8 sendNum;
u8 sendBuffer[220];
u8 recvBuffer[220];

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
    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时）
        dest[1] = device_t.report_interval >> 8;
        dest[2] = device_t.report_interval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_GPS_PERIOD: // 取值范围：1- 1440 （1分钟到24小时），一般默认为1小时，设置为0时不进行GPS定位
        dest[1] = device_t.gps_convert_interval >> 8;
        dest[2] = device_t.gps_convert_interval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_LATITUDE: // 纬度
        dest[1] = device_t.latitude >> 24;
        dest[2] = (device_t.latitude >> 16) & 0xFF;
        dest[3] = (device_t.latitude >> 8) & 0xFF;
        dest[4] = device_t.latitude & 0xFF;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_LONGITUDE: // 经度
        dest[1] = device_t.longitude >> 24;
        dest[2] = (device_t.longitude >> 16) & 0xFF;
        dest[3] = (device_t.longitude >> 8) & 0xFF;
        dest[4] = device_t.longitude & 0xFF;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_TEMPERATURE_INFO: // 温度数据 数据放大1000倍上传
        dest[1] = device_t.temperature >> 16;
        dest[2] = (device_t.temperature >> 8) & 0xFF;
        dest[3] = device_t.temperature & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_HUMIDITY_INFO: // 湿度数据 数据放大1000倍上传
        dest[1] = device_t.humidity >> 16;
        dest[2] = (device_t.humidity >> 8) & 0xFF;
        dest[3] = device_t.humidity & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_BATTERY_VOLTAGE: // 电池电压mV
        dest[1] = (device_t.battery_voltage >> 8) & 0xFF;
        dest[2] = device_t.battery_voltage & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_POWER: // 开关机状态
        dest[1] = device_t.power_on;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_ANGLE_INFO: // 设备当前俯仰角
    {
        u16 angle = device_t.angle * 100;
        dest[1] = (angle >> 8) & 0xFF;
        dest[2] = angle & 0xFF;
        dataLen += 2;
    } break;
    case DATATYPE_DEVICE_SLANT_STATE: // 设备倾斜状态
        dest[1] = device_t.is_slant;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_HT_ALARM: // 高温报警
        dest[1] = device_t.ht_alarm;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_UD_DISTANCE: // 超声波距离
    {
        u16 distance = device_t.ud_distance;
        if (device_t.threshold_state == 0xFF) { // 数据无效时上报为0
            distance = 0;
        }
        dest[1] = (distance >> 8) & 0xFF;
        dest[2] = distance & 0xFF;
        dataLen += 2;
    } break;
    case DATATYPE_DEVICE_OVERFLOW_STATE: // 垃圾桶满溢状态
    {
        dest[1] = device_t.threshold_state;
        dataLen += 1;
    } break;
    case DATATYPE_DEVICE_OVERFLOW_CONFIG: // 超声波测距告警阈值设置
    {
        u16 low_distance = device_t.low_threshold_distance;
        u16 high_distance = device_t.high_threshold_distance;
        dest[1] = (low_distance >> 8) & 0xFF;
        dest[2] = low_distance & 0xFF;
        dest[3] = (high_distance >> 8) & 0xFF;
        dest[4] = high_distance & 0xFF;
        dataLen += 4;
    } break;
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
    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 5;

    mDelay(20);
    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
    mDelay(20);
    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __fail;
    }
    if (getLoRaWANSendStat()) {
    __fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        uint8_t readLen = 0;
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            do {
                lwrb_write_count--;
                lwrb_read(&lwrbBuff_t, &readLen, 1);
                lwrb_read(&lwrbBuff_t, recvBuffer, readLen);
                free_number = lwrb_get_free(&lwrbBuff_t);
            } while (free_number < (sendNum + 1));

            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        packSyncNumber--;
        device_t.send_fail_cnt++;
        if (device_t.send_fail_cnt >= 5) {
            device_t.send_fail_cnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        return;
    }
    device_t.send_fail_cnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 3 * SEC_DELAY;
    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
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
        DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
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
        mDelay(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(20);
        if (getLoRaWANBusyStat()) {
            DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
            goto __send_fail;
        }
        if (getLoRaWANSendStat()) {
        __send_fail:
            DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
            packSyncNumber--;
            device_t.send_fail_cnt++;
            if (device_t.send_fail_cnt >= 5) {
                device_t.send_fail_cnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                reconnect_stamp = Timestamp + 30 * 60;
                DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            }
            return;
        }
        device_t.send_fail_cnt = 0;
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 3 * SEC_DELAY;
        if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) {
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                FLASH_Reset_Param();
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);
        } break;
        case DATATYPE_DEVICE_ACC_REFERENCE: {
            u8 offset = srcHex[1];
            if (offset == 1) {
                device_t.angle_offset_mode = offset;
            }
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(WARN_TAG, "Recv Device Acc Reference CMD : %d", offset);
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
            RTC_TimeSet();
            device_t.sync_state = 1;
            gps_stamp = Timestamp;
            dataConvertTimestamp = Timestamp + 5 * 60;
            dataLen -= 7;
            srcHex += 7;
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        } break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if (reportInterval <= 1440 && reportInterval >= 1) {
                device_t.report_interval = reportInterval;
                device_t.sync_state = 1;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.report_interval);
            }
            dataLen -= 3;
            srcHex += 3;
        } break;
        case DATATYPE_DEVICE_GPS_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
                device_t.gps_convert_interval = reportInterval;
                gps_stamp = Timestamp;
                FLASH_Write_Gps_Period();
                DEBUG_TRACE(LOG_TAG, "Update Device Gps Interval: %dmin", device_t.gps_convert_interval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Gps Interval ERROR: %dmin", device_t.gps_convert_interval);
            }
            dataLen -= 3;
            srcHex += 3;
        } break;
        case DATATYPE_DEVICE_OVERFLOW_CONFIG: {
            u16 low_distance = srcHex[1] << 8 | srcHex[2];
            u16 high_distance = srcHex[3] << 8 | srcHex[4];

            if (low_distance < 30 || low_distance > 4500) {
                DEBUG_TRACE(ERROR_TAG, "Config Device Low Threshold Distance ERROR: %dmin", low_distance);
                return;
            }
            if ((high_distance < 30 && high_distance != 0) || high_distance > 4500) {
                DEBUG_TRACE(ERROR_TAG, "Config Device High Threshold Distance ERROR: %dmin", high_distance);
                return;
            }

            device_t.low_threshold_distance = low_distance;
            device_t.high_threshold_distance = high_distance;
            FLASH_Write_Ud_Reference();
            DEBUG_TRACE(LOG_TAG, "Update Device Threshold Distance Low[%dmm] High[%dmm]", low_distance, high_distance);
            dataLen -= 5;
            srcHex += 5;
        } break;

        default:
            return;
        }
        if (dataLen > 200) // 长度异常，数据出错，退出数据解析
            break;
    }
    DEBUG_TRACE(WARN_TAG, "Recv Device Data Out");
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

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_VOLTAGE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HUMIDITY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_ANGLE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SLANT_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HT_ALARM, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_GPS_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    if (!device_t.is_slant) {
        dataLen = setDataPackage(DATATYPE_DEVICE_UD_DISTANCE, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;

        dataLen = setDataPackage(DATATYPE_DEVICE_OVERFLOW_STATE, dataHex);
        dataHex += dataLen;
        sendNum += dataLen;
    }

    dataLen = setDataPackage(DATATYPE_DEVICE_OVERFLOW_CONFIG, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    uint8_t readLen = 0;
    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        do {
            lwrb_write_count--;
            lwrb_read(&lwrbBuff_t, &readLen, 1);
            lwrb_read(&lwrbBuff_t, recvBuffer, readLen);
            free_number = lwrb_get_free(&lwrbBuff_t);
        } while (free_number < (sendNum + 1));

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }
}

/**
 * @brief  上报GPS数据
 * @param
 * @retval
 */
void Device_GpsReport(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    dataLen = setDataPackage(DATATYPE_DEVICE_LATITUDE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_LONGITUDE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    uint8_t readLen = 0;
    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        do {
            lwrb_write_count--;
            lwrb_read(&lwrbBuff_t, &readLen, 1);
            lwrb_read(&lwrbBuff_t, recvBuffer, readLen);
            free_number = lwrb_get_free(&lwrbBuff_t);
        } while (free_number < (sendNum + 1));

        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    }
}

/**
 * @brief  设备事件处理
 * @param
 * @retval
 */
void DeviceEvent_Process(void)
{
    if (device_t.ble_wake_event) {
        device_t.ble_wake_event = 0;
        if (device_t.ble_connected != BLE_CONNECT_STA) {
            mDelay(20);
            if (device_t.ble_connected != BLE_CONNECT_STA) {
                device_t.ble_connected = BLE_CONNECT_STA;
                if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
                    device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
                DEBUG_TRACE(LOG_TAG, "BLE Connect State Change To %d", device_t.ble_connected);
            }
        }
    }

    if (dataReportTimestamp <= Timestamp) {
        device_t.sync_state = 1;
    }

    // if (gps_stamp <= Timestamp) {
    //     device_t.gps_state = 1;
    // }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    GpioEventProcess();
    DeviceEvent_Process();
    if (LoRaWAN.joinState && !uartLoRa.sendDelay) {
        /* 检测缓冲器是否存在待发射数据 */
        if (lwrb_write_count) { // 缓冲区内有数据
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);

            sendLoRaWANData();

            if (--lwrb_write_count == 0) {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }
        }
    }
    if (device_t.sync_state) {
        device_t.sync_state = 0;
        dataReportTimestamp = Timestamp + device_t.report_interval * 60;
        // dataReportTimestamp = Timestamp + 10;
        Device_PeriodicReport();
        // uartLoRa.sendDelay = 5 * SEC_DELAY;
    }
    if (device_t.gps_state) {
        device_t.gps_state = 0;
        gps_stamp = Timestamp + device_t.gps_convert_interval * 60;
        Device_GpsReport();
        // uartLoRa.sendDelay = 5 * SEC_DELAY;
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

    device_t.sleep_delay = 30; // 串口有新命令时刷新进入睡眠时长
    /* 设置数据同步周期 */
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:0")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.report_interval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval: %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.report_interval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dmin \r\n OK", device_t.report_interval);
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

            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 重新初始化设备
        } else
            DEBUG_TRACE(ACK_TAG, "LoRaWAN PARAM ERROR");
    } else if (strstr((const char *)data, "getDeviceStatue") || strstr((const char *)data, "getDeviceStatus")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            // sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
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
            // sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
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
            RTC_TimeSet();
            device_t.sync_state = 1;
            gps_stamp = Timestamp;
            dataConvertTimestamp = Timestamp + 5 * 60;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD OK,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "keepMainRun")) // 保持不进入睡眠
    {
        keepRunFlag = 1;
    } else if (strstr((const char *)data, "leaveMainRun")) {
        keepRunFlag = 0;
    } else if (strstr((const char *)data, "reboot")) {
        NVIC_SystemReset(); // 复位
    } else {
        // DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY; // 错误命令
    }
}

void callback_BLESearch(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    ADC_BatterySglConvert(); // 每次唤醒转换电量信息
    Device_SglConvert();
    // Angle_SglConvert();
    // gps_stamp = Timestamp;

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_POWER, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_VOLTAGE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HUMIDITY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_ANGLE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SLANT_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HT_ALARM, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_GPS_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_UD_DISTANCE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_OVERFLOW_STATE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_OVERFLOW_CONFIG, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_LATITUDE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_LONGITUDE, dataHex);
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

void callback_BLEInfo(void)
{
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    ADC_BatterySglConvert();
    Device_SglConvert();

    sendBuffer[0] = 0xFB;
    dataHex += 1;
    sendNum += 1;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_VOLTAGE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_HUMIDITY_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_ANGLE_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_UD_DISTANCE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_LATITUDE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_LONGITUDE, dataHex);
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

void callback_BLEJoinData(void)
{
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    u16 power[10];
    uint8_t i = 0;

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

/**
 * @brief  处理来自蓝牙的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") && len >= strlen("Set Report Interval:10")) {
        char *valid = strstr((const char *)data, ":");
        u16 report_interval = atoi(valid + 1);
        if (report_interval <= 1440 && report_interval >= 10) {
            device_t.report_interval = report_interval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.report_interval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
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
        BLE_SendCMD(tempBuf);
        mDelay(100);
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
            RTC_TimeSet();
            device_t.sync_state = 1;
            gps_stamp = Timestamp;
            dataConvertTimestamp = Timestamp + 5 * 60;
            DEBUG_TRACE(LOG_TAG, "Recv BLE Sync Time CMD, %04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "switchPowerON")) { // 激活
        if (!device_t.power_on) {
            device_t.power_on = 1;
            ledBlinkTime = 3 * SEC_DELAY;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
            FLASH_Write_Device_Param();
        }
        BLE_SendCMD("PowerON");

    } else if (strstr((const char *)data, "switchPowerOFF")) { // 关机
        if (device_t.power_on) {
            device_t.power_on = 0;
            ledBlinkTime = 1 * SEC_DELAY;
            FLASH_Write_Device_Param();
        }
        BLE_SendCMD("PowerOFF");
    } else if (strstr((const char *)data, "getDeviceStatus")) { // 获取设备信息
        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "HARDWARE_VER: 0x%02X ,SOFTWARE_VER: 0x%02X", HARDWARE_VER, SOFTWARE_VER);
        BLE_SendCMD(tempBuf);
        mDelay(100);

        memset(tempBuf, 0, sizeof(tempBuf));
        sprintf(tempBuf, "battery: %dMv", device_t.battery_voltage);
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
    } else if (strstr((const char *)data, "hkt") && data[5] == 1) { // 更新固件
        FLASH_Write_Update_Flag();
        mDelay(100);
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "reboot")) {
        NVIC_SystemReset();                                                           // 复位
    } else if (strstr((const char *)data, "hkt") && (data[5] == 5 || data[5] == 9)) { // APP操作指令
        DebugHexInfo("RevBLE", data, len);
        u8 packnum = data[6];
        // APP查询指令
        if (data[6] == 0xFF) {
            callback_BLESearch();
        } else if (data[6] == 0xFC) {
            callback_BLEJoinData();
        } else if (data[6] == 0xFB) {
            callback_BLEInfo();
        } else if (data[6] == 0x09 || data[6] == 0x0A) { // APP开关机控制
            if (data[6] == 0x09) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                    FLASH_Write_Device_Param();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power ON");
                }
            } else {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    FLASH_Write_Device_Param();
                    DEBUG_TRACE(LOG_TAG, "Recv BLE Power OFF");
                }
            }
            callback_BLEPower();
        } else if (data[6] == 0xFE) { // APP开关机控制
            if (data[10] == 0x01) {
                if (!device_t.power_on) {
                    device_t.power_on = 1;
                    ledBlinkTime = 3 * SEC_DELAY;
                    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
                    FLASH_Write_Device_Param();
                }
                // BLE_SendCMD("PowerON");
            } else if (data[10] == 0x00) {
                if (device_t.power_on) {
                    device_t.power_on = 0;
                    ledBlinkTime = 1 * SEC_DELAY;
                    FLASH_Write_Device_Param();
                }
                // BLE_SendCMD("PowerOFF");
            }
            DEBUG_TRACE(LOG_TAG, "Recv BLE Power CMD [%02X]", data[10]);
            callback_BLEAck();
        }
        // APP执行校准命令
        else if (data[6] == 0xFD) {
            device_t.angle_offset_mode = 1;
            DEBUG_TRACE(LOG_TAG, "Recv BLE Acc Reference CMD");
            callback_BLEAck();
        }
        // APP配置周期与高低阈值命令
        else if (data[6] == 2 && data[5] == 9) {
            u16 reportInterval = data[7] << 8 | data[8];
            if (reportInterval <= 1440 && reportInterval >= 1) {
                device_t.report_interval = reportInterval;
                device_t.sync_state = 1;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.report_interval);
            } else {
                DEBUG_TRACE(ERROR_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.report_interval);
            }

            u16 gpsInterval = data[9] << 8 | data[10];
            if ((gpsInterval <= 1440 && gpsInterval >= 10) || gpsInterval == 0) {
                device_t.gps_convert_interval = gpsInterval;
                gps_stamp = Timestamp;
                FLASH_Write_Gps_Period();
                DEBUG_TRACE(LOG_TAG, "Update Device Gps Interval: %dmin", device_t.gps_convert_interval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Gps Interval ERROR: %dmin", device_t.gps_convert_interval);
            }

            u16 low_distance = data[11] << 8 | data[12];
            u16 high_distance = data[13] << 8 | data[14];

            if (low_distance < 30 || low_distance > 4500) {
                DEBUG_TRACE(ERROR_TAG, "Config Device Low Threshold Distance ERROR: %dmin", low_distance);
                return;
            }
            if ((high_distance < 30 && high_distance != 0) || high_distance > 4500) {
                DEBUG_TRACE(ERROR_TAG, "Config Device High Threshold Distance ERROR: %dmin", high_distance);
                return;
            }

            device_t.low_threshold_distance = low_distance;
            device_t.high_threshold_distance = high_distance;
            FLASH_Write_Ud_Reference();
            DEBUG_TRACE(LOG_TAG, "Update Device Threshold Distance Low[%dmm] High[%dmm]", low_distance, high_distance);
            callback_BLEAck();
        }
        // APP更新指令
        else if (data[7] == 1) {
            FLASH_Write_Update_Flag();
            mDelay(100);
            NVIC_SystemReset();                                                         // 复位
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
        }
    }
    // else {
    //     BLE_SendCMD(data);
    // }
}

/**
 * @brief  处理来自超声波模块的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromUdDataHandle(u8 *data, u16 len)
{
    u8 threshold_state = 0;
    if (data[0] == 0xFF) {
        u16 sum = data[0] + data[1] + data[2];
        if ((sum & 0x00FF) == data[3]) {
            device_t.ud_distance = data[1] << 8 | data[2];
            if (device_t.ud_distance < 30 || device_t.ud_distance > 4500) {
                threshold_state = 0xFF;
                device_t.ud_distance = 0;
            } else if (device_t.ud_distance < device_t.low_threshold_distance) {
                threshold_state = 1;
            } else if (device_t.ud_distance > device_t.high_threshold_distance && device_t.high_threshold_distance > 0) {
                threshold_state = 2;
            } else {
                threshold_state = 0;
            }
            if (device_t.threshold_state != threshold_state) {
                device_t.threshold_state = threshold_state;
                device_t.sync_state = 1;
            }
            DEBUG_TRACE(LOG_TAG, "Convert UD Distance: %dmm, Threshold State: %d", device_t.ud_distance, device_t.threshold_state);
        }
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
        // DebugHexInfo("LoRaWAN Recv", uartLoRa.recvData, uartLoRa.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartBle.recvTimeout && uartBle.recvLen) {
        DebugHexInfo("BLE Recv", uartBle.recvData, uartBle.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart BLE Data: %s", uartBle.recvData);
        fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }
    if (!uartUd.recvTimeout && uartUd.recvLen) {
        // DEBUG_TRACE(LOG_TAG, "Recv Uart UD Data: %s", uartUd.recvData);
        fromUdDataHandle(uartUd.recvData, uartUd.recvLen);
        memset(uartUd.recvData, 0, sizeof(uartUd.recvData));
        uartUd.recvLen = 0;
    }
    if (!uartGps.recvTimeout && uartGps.recvLen) {
        // DEBUG_TRACE(LOG_TAG, "Recv Uart GPS Data: %s", uartGps.recvData);
        fromGpsDataHandle(uartGps.recvData, uartGps.recvLen);
        memset(uartGps.recvData, 0, sizeof(uartGps.recvData));
        uartGps.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug

        // DebugHexInfo("Info Recv", uartInfo.recvData, uartInfo.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart Info Data: %s", uartInfo.recvData);

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

void lwrbTest(void)
{
    if (!LoRaWAN.joinState)
        return;
    uint8_t readLen = 0;
    int free_number;
    while (1) {
        sendNum = 35;
        free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            do {
                lwrb_write_count--;
                lwrb_read(&lwrbBuff_t, &readLen, 1);
                lwrb_read(&lwrbBuff_t, recvBuffer, readLen);
                free_number = lwrb_get_free(&lwrbBuff_t);
            } while (free_number < (sendNum + 1));

            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
    }
}
