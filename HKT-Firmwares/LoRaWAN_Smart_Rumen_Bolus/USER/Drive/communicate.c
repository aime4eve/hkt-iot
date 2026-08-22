
/* Includes ------------------------------------------------------------------*/
#include "stdlib.h"
#include <stdio.h>
#include <string.h>

#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "cJSON.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "lwrb.h"
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
u8 sendBuffer[120];

u8 factoryMode;
u8 keepRunFlag;
u8 packSyncNumber;
char tempBuf[500];

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
    case DATATYPE_DEVICE_TEMPERATURE_INFO: // 温度数据 数据放大100倍上传
        dest[1] = device_t.sensor_t.temperature >> 16;
        dest[2] = (device_t.sensor_t.temperature >> 8) & 0xFF;
        dest[3] = device_t.sensor_t.temperature & 0xFF;
        dataLen += 3;
        break;
    case DATATYPE_DEVICE_TEMPERATURE_GROUP_INFO: {
        dest[1] = device_t.temperature_group;
        for (u8 i = 0; i < device_t.temperature_group; i++) {
            dest[2 + i * 2] = (device_t.temperature[i] >> 8) & 0xFF;
            dest[3 + i * 2] = device_t.temperature[i] & 0xFF;
        }
        dataLen += 1 + 2 * device_t.temperature_group;
        device_t.temperature_group = 0;
    } break;
    case DATATYPE_DEVICE_X_ACC_VALUE: // x轴加速度值
        dest[1] = device_t.sensor_t.ax;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_Y_ACC_VALUE: // y轴加速度值
        dest[1] = device_t.sensor_t.ay;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_Z_ACC_VALUE: // z轴加速度值
        dest[1] = device_t.sensor_t.az;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_GASTRIC_MOTILITY: // 胃动量
        dest[1] = (device_t.sensor_t.gastricMotility >> 24) & 0xFF;
        dest[2] = (device_t.sensor_t.gastricMotility >> 16) & 0xFF;
        dest[3] = (device_t.sensor_t.gastricMotility >> 8) & 0xFF;
        dest[4] = device_t.sensor_t.gastricMotility & 0xFF;
        dataLen += 4;
        break;
    case DATATYPE_DEVICE_BATTERY_VOLTAGE: // 电池电压mV
        dest[1] = (device_t.sensor_t.vol >> 8) & 0xFF;
        dest[2] = device_t.sensor_t.vol & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_BATTERY_INFO: // 电量信息
        dest[1] = batteryLevel;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：10- 1440 （10分钟到24小时）。
        dest[1] = device_t.reportInterval >> 8;
        dest[2] = device_t.reportInterval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_TIMESTAMP:
        dest[1] = (Timestamp >> 24) & 0xFF;
        dest[2] = (Timestamp >> 16) & 0xFF;
        dest[3] = (Timestamp >> 8) & 0xFF;
        dest[4] = Timestamp & 0xFF;
        dataLen += 4;
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
#if FUNC_LORA_VERSION_LIR
    if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
        setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
#endif
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
#if FUNC_LORA_VERSION_LIR
    /* 等待模组就绪 */
    while (!getLoRaWANBusyIOLevel())
        ;
#endif

#if ZN_COM_ENABLE
    uartLoRa.sendData[0] = 0xC1;
    memcpy(uartLoRa.sendData + 1, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 1;
#else
    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
#if LORAWAN_CNF_ENABLE
    uartLoRa.sendData[3] = 4;
#else
    uartLoRa.sendData[3] = 0;
#endif

    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 5;
#endif

#if FUNC_LORA_VERSION_LIR
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
        device_t.sendFailCnt++;
        packSyncNumber--;
        if (device_t.sendFailCnt >= 5) {
            device_t.sendFailCnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        tx_mutex_put(&l_mutex_lock);
        device_t.sending = 0;
        return false;
    }
    device_t.sendFailCnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 5 * SEC_DELAY;
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

#else

#if 0
    // 先查询入网状态 再进行数据发送
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        AT_LoRaWAN_setATExit();
        mDelay(500);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(500);
    } else {
        rstLoRaWANModule();
        LoRa_DelayMs(500);
        AT_LoRaWAN_setATExit();
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        reconnect_stamp = Timestamp;
        DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        return;
    }
#else

#if LORAWAN_CNF_ENABLE
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_TxHEX(uartLoRa.sendData, uartLoRa.sendLen, 10) == TRUE) {
        // DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 1 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    } else {
        LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        device_t.sendFailCnt++;
        packSyncNumber--;
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
            DEBUG_TRACE(LOG_TAG, "Lwrb Write Cache %d", lwrb_write_count);
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full %d", lwrb_write_count);

            // 循环丢弃旧数据直到有足够空间
            while (free_number < (sendNum + 1)) {
                uint8_t temp_len = 0;

                // 尝试读取一个数据包并丢弃
                if (lwrb_read(&lwrbBuff_t, &temp_len, 1) == 1) {
                    lwrb_read(&lwrbBuff_t, (uint8_t *)tempBuf, temp_len); // 丢弃数据
                    lwrb_write_count--;
                    free_number = lwrb_get_free(&lwrbBuff_t);
                    DEBUG_TRACE(LWRB_TAG, "Lwrb Discard Packet, Count: %d", lwrb_write_count);
                } else {
                    // 缓冲区已空但空间仍不足？这不应该发生
                    DEBUG_TRACE(ERROR_TAG, "Unexpected empty buffer");
                    break;
                }
            }

            // 写入新数据
            lwrb_write(&lwrbBuff_t, &sendNum, 1);
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        return;
    }
#else
    static uint8_t tx_cnt;
    tx_cnt++;
    if (tx_cnt >= 96) {
        tx_cnt = 0;
        AT_LoRaWAN_setATEnter();
        if (AT_LoRaWAN_getJOIN() == TRUE) {
            AT_LoRaWAN_setATExit();
            mDelay(500);
            LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
            DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
            mDelay(500);
        } else {
            rstLoRaWANModule();
            LoRa_DelayMs(500);
            AT_LoRaWAN_setATExit();
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            return;
        }
    } else {
        mDelay(100);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(100);
    }
#endif

#endif

    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    if (lwrb_write_count) {
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
#endif
}

/**
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void txLoRaWANData(uint8_t *sendBuffer, uint8_t sendNum)
{
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);

#if ZN_COM_ENABLE
    uartLoRa.sendData[0] = 0xC1;
    memcpy(uartLoRa.sendData + 1, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 1;
#else
    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    uartLoRa.sendLen = sendNum + 5;
#endif

#if LORAWAN_CNF_ENABLE
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_TxHEX(uartLoRa.sendData, uartLoRa.sendLen, 10) == TRUE) {
        // DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 1 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    } else {
        LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        device_t.sendFailCnt++;
        packSyncNumber--;

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendNum + 1)) {
            lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
            DEBUG_TRACE(LOG_TAG, "Lwrb Write Cache %d", lwrb_write_count);
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full %d", lwrb_write_count);

            // 循环丢弃旧数据直到有足够空间
            while (free_number < (sendNum + 1)) {
                uint8_t temp_len = 0;

                // 尝试读取一个数据包并丢弃
                if (lwrb_read(&lwrbBuff_t, &temp_len, 1) == 1) {
                    lwrb_read(&lwrbBuff_t, (uint8_t *)tempBuf, temp_len); // 丢弃数据
                    lwrb_write_count--;
                    free_number = lwrb_get_free(&lwrbBuff_t);
                    DEBUG_TRACE(LWRB_TAG, "Lwrb Discard Packet, Count: %d, Read Len: %d", lwrb_write_count, temp_len);
                } else {
                    // 缓冲区已空但空间仍不足？这不应该发生
                    DEBUG_TRACE(ERROR_TAG, "Unexpected empty buffer");
                    break;
                }
            }

            // 写入新数据
            lwrb_write(&lwrbBuff_t, &sendNum, 1);
            lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
            lwrb_write_count++;
        }
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        return;
    }
#else
    static uint8_t tx_cnt;
    tx_cnt++;
    if (tx_cnt >= 96) {
        tx_cnt = 0;
        AT_LoRaWAN_setATEnter();
        if (AT_LoRaWAN_getJOIN() == TRUE) {
            AT_LoRaWAN_setATExit();
            mDelay(500);
            LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
            DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
            mDelay(500);
        } else {
            rstLoRaWANModule();
            LoRa_DelayMs(500);
            AT_LoRaWAN_setATExit();
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            return;
        }
    } else {
        mDelay(100);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(100);
    }
#endif

    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    if (lwrb_write_count) {
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    }
}

// 发送在那协议版本数据
// C1 80 0D 46 FF FE FF FE FE FE 00 21 AB 9F
// C1 00 0D 4D FE FD FF FE FF FE 00 21 AB 9F
// C1 00 0D 4D FE FF FF FE FD FF 00 1D F2 DE
// C1 00 0D 4D FE FF FF FE FD FF 00 1D F2 DE
// C1 00 0D 4F FF FF FE FE FF FE 00 1D F2 DE
void zn_sendLoRaWANData(void)
{
    static u8 send_cnt;
    u8 data[] = {0xC1, 0x00, 0x0D, 0x4F, 0xFF, 0xFF, 0xFE, 0xFE, 0xFF, 0xFE, 0x00, 0x1D, 0xF2, 0xDE};
    if (send_cnt == 0) {
        data[1] = 0x80;
    }
    send_cnt++;
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);

    // memcpy(uartLoRa.sendData, SYNC_HEAD, 3);

    // uartLoRa.sendData[3] = 0;
    // uartLoRa.sendData[4] = packSyncNumber++;

    // memcpy(uartLoRa.sendData + 5, sendBuffer, sendNum);
    // uartLoRa.sendLen = sendNum + 5;

    mDelay(100);
    LoRa_SendData(data, sizeof(data)); // 发送数据
    DebugHexInfo("Send", data, sizeof(data));
    mDelay(100);

    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    if (lwrb_write_count) {
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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

    // 服务器需要应答
    if ((data[3] & 0x01) == 1) {
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

#if FUNC_LORA_VERSION_LIR
        if (getLoRaWANMode() == LoRaWAN_CMD_MODE)
            setLoRaWANMode(LoRaWAN_PASSTHROUGH_MODE);
#endif
        if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
            setLoRaWANStatus(LoRaWAN_WAKE_STATUS);
#if FUNC_LORA_VERSION_LIR
        /* 等待模组就绪 */
        while (!getLoRaWANBusyIOLevel())
            ;
#endif

#if FUNC_LORA_VERSION_LIR
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
            device_t.sendFailCnt++;
            packSyncNumber--;
            if (device_t.sendFailCnt >= 5) {
                device_t.sendFailCnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                reconnect_stamp = Timestamp + 30 * 60;
                DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
            }
            uartLoRa.sendDelay = 5 * SEC_DELAY;
            device_t.sending = 0;
            return false;
        }
        device_t.sendFailCnt = 0;
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;

#else
#if 0
    // 先查询入网状态 再进行数据发送
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        AT_LoRaWAN_setATExit();
        mDelay(500);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(500);
    } else {
        AT_LoRaWAN_setATExit();
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        // reconnect_stamp = Timestamp + 30 * 60;
        reconnect_stamp = Timestamp;
        DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        // uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        return;
    }
#else
        mDelay(100);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(100);
#endif
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
#endif
    }
    /*  解析数据  */
    u8 dataLen = len - 5;

    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d,", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type : %02x", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
                device_t.reportInterval = reportInterval;
                FLASH_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.reportInterval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval ERROR: %dmin", device_t.reportInterval);
            }
            dataLen -= 3;
            srcHex += 3;
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

#define TEMP_OFFSET 4000.0
#define TEMP_SCALE (3100.0 / 2048.0)
#define ROUND(x) lrint(((x) >= 0.0 ? (x) + 0.5 : (x) - 0.5))

/**
 * @brief 将温度值转换为12位有符号数
 * @param temperature 原始温度值
 * @return int16_t 转换后的12位有符号数(-2048 到 2047)
 */
static int16_t convertTemperatureTo12Bit(float temperature)
{
    // 温度转换公式: ((temperature + 0.4) * 100 - 4000) * 2048 / 3100
    double adjusted_temp = (temperature + 0.4) * 100;
    double raw_temp = (adjusted_temp - TEMP_OFFSET) * 2048 / 3100;
    int16_t raw_int = ROUND(raw_temp); // 四舍五入到整数

    // 处理负数情况，确保12位值是有符号的
    if (raw_int < 0) {
        raw_int += 0x1000; // 设置符号位并保留12位（最高位为1表示负数）
    }
    return raw_int;
}

/**
 * @brief  周期同步上报数据
 * @param
 * @retval
 */
void Device_PeriodicReport(void)
{
    static u8 cnt;
    u8 *dataHex = sendBuffer;
    u8 dataLen = 0;
    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

#if ZN_COM_ENABLE
    if (cnt == 0) {
        cnt = 1;
        dataHex[0] = 0x80; // 传感器状态
    } else {
        dataHex[0] = 0x00; // 传感器状态
    }

    float temperature = device_t.temperature[0] / 100.0f + 0.005f; // 四舍五入
    int16_t temp_12bit = convertTemperatureTo12Bit(temperature);   // c1000cfec2fd0b071302470a8500  35°
    device_t.temperature_group = 0;

    dataHex[1] = (temp_12bit >> 8) & 0xFF;
    dataHex[2] = temp_12bit & 0xFF;

    float temp, diff;
    int8_t value;

    for (u8 i = 0; i < 6; i++) {
        temp = device_t.temperature[i + 1] / 100.0f + 0.005f; // 温度差值
        diff = temp - temperature;
        temperature = temp;
        value = diff * 127;
        dataHex[3 + i] = value;
    }

    dataHex[9] = (device_t.sensor_t.gastricMotility >> 24) & 0xFF;
    dataHex[10] = (device_t.sensor_t.gastricMotility >> 16) & 0xFF;
    dataHex[11] = (device_t.sensor_t.gastricMotility >> 8) & 0xFF;
    dataHex[12] = device_t.sensor_t.gastricMotility & 0xFF;
    sendNum = 13;

#else
    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_GROUP_INFO, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_VOLTAGE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_GASTRIC_MOTILITY, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_X_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_Y_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_Z_ACC_VALUE, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendNum += dataLen;

    // dataLen = setDataPackage(DATATYPE_DEVICE_TIMESTAMP, dataHex);
    // dataHex += dataLen;
    // sendNum += dataLen;
#endif

#if LORAWAN_CNF_ENABLE
    txLoRaWANData(sendBuffer, sendNum);
#else

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendNum + 1)) {
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
    } else {
        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
    }

#endif
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{

#if LORAWAN_CNF_ENABLE

#if ZN_COM_ENABLE
    if ((dataReportTimestamp <= Timestamp || device_t.temperature_group >= 7))
#else
    if ((dataReportTimestamp <= Timestamp || device_t.temperature_group >= 5))
#endif
    {
#if ZN_COM_ENABLE
        if (device_t.reportInterval == DEFAULT_REPORT_INTERVAL && device_t.temperature_group < 7 && dataReportTimestamp != 0)
#else
        if (device_t.reportInterval == DEFAULT_REPORT_INTERVAL && device_t.temperature_group < 5 && dataReportTimestamp != 0)
#endif
        {
            return;
        }
				
				if(dataReportTimestamp == 0){
						mDelay(5000);
				}
				
        if (device_t.reportInterval == 1) {
            dataReportTimestamp = Timestamp + 10;
        } else {
            dataReportTimestamp = Timestamp + device_t.reportInterval * 60;
        }
				
        Device_PeriodicReport();  // 保证离线时也能上报数据
        device_t.syncDeviceState = 0;
    }

#else

#if ZN_COM_ENABLE
    if ((dataReportTimestamp <= Timestamp || device_t.temperature_group >= 7) && LoRaWAN.joinState)
#else
    if ((dataReportTimestamp <= Timestamp || device_t.temperature_group >= 5) && LoRaWAN.joinState)
#endif
    {
        dataReportTimestamp = Timestamp + device_t.reportInterval * 60;

#if ZN_COM_ENABLE
        if (device_t.reportInterval == DEFAULT_REPORT_INTERVAL && device_t.temperature_group < 7 && dataReportTimestamp != 0)
#else
        if (device_t.reportInterval == DEFAULT_REPORT_INTERVAL && device_t.temperature_group < 5 && dataReportTimestamp != 0)
#endif
        {
            return;
        }
		
        device_t.syncDeviceState = 1;
    }

#endif

    if (LoRaWAN.joinState && !uartLoRa.sendDelay) {
        /* 检测缓冲器是否存在待发射数据 */
        if (lwrb_write_count) { // 缓冲区内有数据0
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);
            if (--lwrb_write_count == 0) {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }

            DEBUG_TRACE(LOG_TAG, "Lwrb Data Cnt %d", lwrb_write_count);
            sendLoRaWANData();

        } else if (device_t.syncDeviceState) {
            device_t.syncDeviceState = 0;
            Device_PeriodicReport();
            // uartLoRa.sendDelay = 5 * SEC_DELAY;
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
    if (strstr((const char *)data, "Set Report Interval:") && len <= strlen("Set Report Interval:1440") &&
        len >= strlen("Set Report Interval:0")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval: %dm", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") &&
             len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dmin \r\n OK", device_t.reportInterval);
            FLASH_Write_Report_Interval();
        } else
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod Param ERROR");
    } else if (strstr((const char *)data, "getDeviceStatue")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class A");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0],
                    LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5],
                    LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0],
                    LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5],
                    LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf),
                    ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5],
                    LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10],
                    LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:");
            sprintf(tempBuf + strlen(tempBuf), ",APPSKEY:");
            sprintf(tempBuf + strlen(tempBuf), ",NWKSKEY:");
        } else {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 0);
            sprintf(tempBuf + strlen(tempBuf), ",dataSyncPeriod:%d", device_t.reportInterval);
            sprintf(tempBuf + strlen(tempBuf), ",LoRaWANJoin:%s", "Class C");
            sprintf(tempBuf + strlen(tempBuf), ",DEVEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.DevEUI[0],
                    LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3], LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5],
                    LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            sprintf(tempBuf + strlen(tempBuf), ",APPEUI:%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppEUI[0],
                    LoRaWAN.AppEUI[1], LoRaWAN.AppEUI[2], LoRaWAN.AppEUI[3], LoRaWAN.AppEUI[4], LoRaWAN.AppEUI[5],
                    LoRaWAN.AppEUI[6], LoRaWAN.AppEUI[7]);
            sprintf(tempBuf + strlen(tempBuf),
                    ",APPKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppKEY[0],
                    LoRaWAN.AppKEY[1], LoRaWAN.AppKEY[2], LoRaWAN.AppKEY[3], LoRaWAN.AppKEY[4], LoRaWAN.AppKEY[5],
                    LoRaWAN.AppKEY[6], LoRaWAN.AppKEY[7], LoRaWAN.AppKEY[8], LoRaWAN.AppKEY[9], LoRaWAN.AppKEY[10],
                    LoRaWAN.AppKEY[11], LoRaWAN.AppKEY[12], LoRaWAN.AppKEY[13], LoRaWAN.AppKEY[14], LoRaWAN.AppKEY[15]);
            sprintf(tempBuf + strlen(tempBuf), ",DEVADDR:%02x%02x%02x%02x", LoRaWAN.DevADDR[0], LoRaWAN.DevADDR[1],
                    LoRaWAN.DevADDR[2], LoRaWAN.DevADDR[3]);
            sprintf(tempBuf + strlen(tempBuf),
                    ",APPSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.AppSKEY[0],
                    LoRaWAN.AppSKEY[1], LoRaWAN.AppSKEY[2], LoRaWAN.AppSKEY[3], LoRaWAN.AppSKEY[4], LoRaWAN.AppSKEY[5],
                    LoRaWAN.AppSKEY[6], LoRaWAN.AppSKEY[7], LoRaWAN.AppSKEY[8], LoRaWAN.AppSKEY[9], LoRaWAN.AppSKEY[10],
                    LoRaWAN.AppSKEY[11], LoRaWAN.AppSKEY[12], LoRaWAN.AppSKEY[13], LoRaWAN.AppSKEY[14],
                    LoRaWAN.AppSKEY[15]);
            sprintf(tempBuf + strlen(tempBuf),
                    ",NWKSKEY:%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", LoRaWAN.NwkSKEY[0],
                    LoRaWAN.NwkSKEY[1], LoRaWAN.NwkSKEY[2], LoRaWAN.NwkSKEY[3], LoRaWAN.NwkSKEY[4], LoRaWAN.NwkSKEY[5],
                    LoRaWAN.NwkSKEY[6], LoRaWAN.NwkSKEY[7], LoRaWAN.NwkSKEY[8], LoRaWAN.NwkSKEY[9], LoRaWAN.NwkSKEY[10],
                    LoRaWAN.NwkSKEY[11], LoRaWAN.NwkSKEY[12], LoRaWAN.NwkSKEY[13], LoRaWAN.NwkSKEY[14],
                    LoRaWAN.NwkSKEY[15]);
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
            systime.tm_mon = timeP->tm_mon;
            systime.tm_mday = timeP->tm_mday;
            systime.tm_hour = timeP->tm_hour;
            systime.tm_min = timeP->tm_min;
            systime.tm_sec = timeP->tm_sec;
            systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD OK,%04d-%02d-%02d-%d:%d:%d", systime.tm_year,
                        systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "keepMainRun")) // 保持不进入睡眠
    {
        keepRunFlag = 1;
    } else if (strstr((const char *)data, "leaveMainRun")) {
        keepRunFlag = 0;
    } else {
        // DEBUG_TRACE(WARN_TAG, "Uart Info Failed To Parse Data!");
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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

        // DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        // DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) {
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
