
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

#include "SI12T.h"
#include "TB_03F.h"
#include "W25X40CLSNIG.h"
#include "ZS902R.h"
#include "cJSON.h"
#include "mbi5120.h"
#include "sound.h"
#if (FSV9522_CARD_ENABLE)
#include "fsv9522.h"
#else
#include "nz3801.h"
#endif

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

u8 factoryMode;
u8 packSyncNumber;
u8 uart_touch_press = 0xFF;

char tempBuf[500];
time_t dataReportTimestamp;
time_t batteryTimestamp;
time_t wakeupTimestamp;

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
    case DATATYPE_DEVICE_REMOTE_UNLOCK: // 远程开锁
        dest[1] = 0;
        dataLen += 1;
        break;
#if FUNC_OPERATIONAL_VERSION_ENABLE
    case DATATYPE_DEVICE_OPEN_MODE: // 常开模式
        dest[1] = coded_open_mode_t.mode;
        dest[2] = (coded_open_mode_t.lock_delay >> 8) & 0xFF;
        dest[3] = coded_open_mode_t.lock_delay & 0xFF;
        dest[4] = coded_open_mode_t.loop_duty;
        dest[5] = coded_open_mode_t.loop_cnt;
        dest[6] = coded_open_mode_t.loop_start_hour;
        dest[7] = coded_open_mode_t.loop_start_min;
        dest[8] = coded_open_mode_t.loop_start_sec;
        dest[9] = coded_open_mode_t.loop_over_hour;
        dest[10] = coded_open_mode_t.loop_over_min;
        dest[11] = coded_open_mode_t.loop_over_sec;
        dataLen += 11;
        break;
    case DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME: // 门锁自动回锁时间
        dest[1] = device_t.auto_lock_time;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_VOLUME_SET: // 音量信息
        dest[1] = device_t.volume;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_TIME_ZONE: // 设备时区信息
    {
        u8 timezone = 0;
        if (device_t.timezone == 3.5) {
            timezone = 25;
        } else if (device_t.timezone == 5.5) {
            timezone = 26;
        } else if (device_t.timezone < 13 && device_t.timezone >= 0) {
            timezone = device_t.timezone;
        } else if (device_t.timezone < 24) {
            timezone = -(device_t.timezone - 12);
        }
        dest[1] = timezone;
        dataLen += 1;
    } break;
    case DATATYPE_DEVICE_USER_BIND: {
        if (device_t.isBand == 1) {
            dest[1] = 0x56;
        } else {
            dest[1] = 0;
        }
        dataLen += 1;
    } break;
#endif
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆状态
        dest[1] = device_t.tamperAlarm;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_PERIOD: // 数据上报周期
        dest[1] = (device_t.reportInterval >> 8) & 0xFF;
        dest[2] = device_t.reportInterval & 0xFF;
        dataLen += 2;
        break;
    case DATATYPE_DEVICE_TIMESTAMP:
        dest[1] = (Timestamp >> 24) & 0xFF;
        dest[2] = (Timestamp >> 16) & 0xFF;
        dest[3] = (Timestamp >> 8) & 0xFF;
        dest[4] = Timestamp & 0xFF;
        dataLen += 4;
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

ErrorStatus CalCheckSum(uint8_t *buf, int len)
{
    int total = 0;
    uint8_t sum = 0;
    ErrorStatus status = PASS;
    for (int i = 0; i < len - 1; i++) {
        total += buf[i];
    }
    sum = total % 256;
    if (sum != buf[len - 1]) {
        DEBUG_TRACE(WARN_TAG, "cal error sum[%d-%d]", sum, buf[len - 1]);
        status = FAIL;
    }
    return status;
}

uint8_t GetCheckSum(uint8_t *buf, int len)
{
    int total = 0;
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        total += buf[i];
    }
    sum = total % 256;
    return sum;
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
    tx_mutex_get(&l_mutex_lock, TX_WAIT_FOREVER);
    device_t.sending = 1;
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
    len = 0;
    memset(txbuf, 0, sizeof(txbuf));

    memcpy(txbuf, SYNC_HEAD, 3);
    txbuf[3] = 2;
    txbuf[4] = packSyncNumber++;

    len = txlen;
    memcpy(txbuf + 5, buf, txlen);
    len += 5;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    u8 sum = GetCheckSum(txbuf, len);
    txbuf[len++] = sum;
#endif

#if FUNC_LORA_VERSION_LIR
    tx_thread_sleep(20);
    LoRa_SendData(txbuf, len); // 发送数据
    DebugHexInfo("Send", txbuf, len);
    tx_thread_sleep(20);
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

            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.offline = 1;
            exc_lock_info_t.type = 0x64;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
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
    tx_mutex_put(&l_mutex_lock);
    device_t.sending = 0;
    return true;

#else
    // 先查询入网状态 再进行数据发送
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        AT_LoRaWAN_setATExit();
        tx_thread_sleep(500);
        LoRa_SendData(txbuf, len); // 发送数据
        DebugHexInfo("Send", txbuf, len);
        tx_thread_sleep(500);
    } else {
        
        // rstLoRaWANModule();
        LoRa_DelayMs(500);
        AT_LoRaWAN_setATExit();
        // AT_LoRaWAN_setATExit();
        // setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        // LoRaWAN.joinState = 0;
        LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        // reconnect_stamp = Timestamp + 30 * 60;
        // DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);

        LoRaWAN.joinState = 0;
        reconnect_stamp = Timestamp; // 开始发起入网请求

        struct lock_log exc_lock_info_t;
        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
        exc_lock_info_t.u_exec.isLocal = 1;
        exc_lock_info_t.u_exec.offline = 1;
        exc_lock_info_t.type = 0x64;
        exc_lock_info_t.stamp = Timestamp;
        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash

        uartLoRa.sendDelay = 6 * SEC_DELAY;
        tx_mutex_put(&l_mutex_lock);
        device_t.sending = 0;
        return false;
    }
    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 6 * SEC_DELAY;
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    tx_mutex_put(&l_mutex_lock);
    device_t.sending = 0;
    return true;
#endif
}

/**
 * @brief  发送填充好的数据给蓝牙模块
 * @param  buf 待发送有效数据缓冲区
 * @param  txlen 待发送有效数据长度
 * @retval
 */
void sendBTData(u8 *buf, u8 txlen)
{
    u8 txbuf[100];
    u8 len;

    len = 0;
    memset(txbuf, 0, sizeof(txbuf));
    memcpy(txbuf, SYNC_HEAD, 3);

    txbuf[3] = 2;
    txbuf[4] = packSyncNumber++;

    len = txlen;
    memcpy(txbuf + 5, buf, txlen);
    len += 5;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    u8 sum = GetCheckSum(txbuf, len);
    txbuf[len++] = sum;
#endif

    Ble_SendData(txbuf, len); // 发送数据
    DebugHexInfo("Send", txbuf, len);
    tx_thread_sleep(500);

    DEBUG_TRACE(LOG_TAG, "BT SendData Success");
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
    u8 buffer[100];
    u8 sendLen;

    /*  确认数据 */
    if (checkDataSyncHead(data) == FAIL) {
        DEBUG_TRACE(ERROR_TAG, "Data Sync Head ERROR!");
        return;
    }
    if (data[3] & 0x02) { // 通讯启用校验
        if (CalCheckSum(data, len) == FAIL) {
            DEBUG_TRACE(ERROR_TAG, "Data Check Sum ERROR!");
            return;
        }
    }
    LoRaWAN.joinState = 1;
    tx_event_flags_set(&event_group, BIT_SYNC, TX_OR);

#if (!FUNC_OPERATIONAL_VERSION_ENABLE)
    if ((data[3] & 0x01) == 1) { // 服务器需要应答
        u8 *dataHex = buffer;

        tx_mutex_get(&l_mutex_lock, TX_WAIT_FOREVER);
        device_t.sending = 1;
        memset(buffer, 0, sizeof(buffer));

        sendLen = setDataPackage(DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK, dataHex);
        dataHex += sendLen;

        memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
        memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
        uartLoRa.sendData[3] = 0;
        uartLoRa.sendData[4] = packSyncNumber++;
        memcpy(uartLoRa.sendData + 5, buffer, sendLen);
        uartLoRa.sendLen = sendLen + 5;

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
        tx_thread_sleep(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        tx_thread_sleep(20);
        if (getLoRaWANBusyStat()) {
            DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        }
        if (getLoRaWANSendStat()) {
            DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) {
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }
            device_t.sendFailCnt++;
            packSyncNumber--;
            if (device_t.sendFailCnt >= 5) {
                device_t.sendFailCnt = 0;
                LoRaWAN.joinState = 0;
                LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
                LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
                reconnect_stamp = Timestamp + 30 * 60;
                DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);

                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.offline = 1;
                exc_lock_info_t.type = 0x64;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
            tx_mutex_put(&l_mutex_lock);
            device_t.sending = 0;
            return;
        }
        device_t.sendFailCnt = 0;
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 5 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        tx_mutex_put(&l_mutex_lock);
        device_t.sending = 0;
    }

#else
        // 先查询入网状态 再进行数据发送
        AT_LoRaWAN_setATEnter();
        if (AT_LoRaWAN_getJOIN() == TRUE) {
            AT_LoRaWAN_setATExit();
            tx_thread_sleep(20);
            LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
            DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
            tx_thread_sleep(500);
        } else {
            // AT_LoRaWAN_setATExit();
            // setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            // LoRaWAN.joinState = 0;
            rstLoRaWANModule();
            tx_thread_sleep(500);
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            // reconnect_stamp = Timestamp + 30 * 60;
            // DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);

            LoRaWAN.joinState = 0;
            reconnect_stamp = Timestamp; // 开始发起入网请求

            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.offline = 1;
            exc_lock_info_t.type = 0x64;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash

            uartLoRa.sendDelay = 6 * SEC_DELAY;
            tx_mutex_put(&l_mutex_lock);
            device_t.sending = 0;
        }
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
        uartLoRa.sendDelay = 6 * SEC_DELAY;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        tx_mutex_put(&l_mutex_lock);
        device_t.sending = 0;
#endif
}

#endif

/*  解析数据  */
u8 dataLen = len - 5;
if (data[3] & 0x02) { // 通讯启用校验
    dataLen -= 1;
}
DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d", dataLen);
u8 *srcHex = data + 5;
while (dataLen) {
    u8 dataType = srcHex[0];
    DEBUG_TRACE(LOG_TAG, "Analysis Data Type: 0x%02X", dataType);
    switch (dataType) {
#if FUNC_OPERATIONAL_VERSION_ENABLE
    case DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION: {
        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, buffer);
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 1;
        srcHex += 1;
    } break;
    case DATATYPE_DEVICE_ADDR_ID: {
        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_ADDR_ID, buffer);
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 1;
        srcHex += 1;
    } break;
    case DATATYPE_DEVICE_BATTERY_INFO: {
        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, buffer);
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 2;
        srcHex += 2;
    } break;
    case DATATYPE_DEVICE_REMOTE_UNLOCK: {
        memset(buffer, 0, sizeof(buffer));
        sendLen = setDataPackage(DATATYPE_DEVICE_REMOTE_UNLOCK, buffer);
        buffer[1] = srcHex[1];
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
         
        if (srcHex[1] == 1) { // 关锁
            /* 反转 */
            MOTOR_REVERSAL();
            tx_thread_sleep(150);
            /* 刹车 */
            MOTOR_BRAKE();
            /* 待机 */
            MOTOR_STANDBY();
        } else if (srcHex[1] == 0) {
            if (!device_t.lockDoorTimeout) // 已开锁时不允许重复开锁
                tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
            memset(buffer, 0, sizeof(buffer));
            sendLen = 7;

            buffer[0] = DATATYPE_DEVICE_REPORT_UNLOCK_RECORD;
            buffer[1] = 0xC5;
            buffer[2] = 0x00;
            buffer[3] = (Timestamp >> 24) & 0xFF;
            buffer[4] = (Timestamp >> 16) & 0xFF;
            buffer[5] = (Timestamp >> 8) & 0xFF;
            buffer[6] = Timestamp & 0xFF;

            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) {
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }
            {
                // 写入远程开锁操作记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 0;
                exc_lock_info_t.u_exec.unlockSuccess = 1;
                exc_lock_info_t.type = 0;
                exc_lock_info_t.number = 0;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            }
        }
        dataLen -= 2;
        srcHex += 2;
    } break;
    case DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD: {
        memset(&coded_lock_t.temp_pass, 0, sizeof(coded_lock_t.temp_pass));
        coded_lock_t.temp_pass.isable = 1;
        coded_lock_t.temp_pass.time = Timestamp + (srcHex[1] << 8 | srcHex[2]) * 60;
        memcpy(coded_lock_t.temp_pass.word, srcHex + 4 + 8 - (srcHex[3]), srcHex[3]);
        DEBUG_TRACE(LOG_TAG, "Recv Server Temp Password Len[%d] [%s]", srcHex[3], coded_lock_t.temp_pass.word);
        memset(buffer, 0, sizeof(buffer));
         
        sendLen = setDataPackage(DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD, buffer);
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 12;
        srcHex += 12;
    } break;
    case DATATYPE_DEVICE_MANAGE_CODED_LOCK_TIMELINESS: {
        struct secret_key_timeliness key_info = {0};
        u8 arg = srcHex[1];
        key_info.user_num = srcHex[2];
        key_info.isable = srcHex[3];
        key_info.len = srcHex[4];
        memcpy(key_info.word, srcHex + 5 + (8 - key_info.len), key_info.len);
        key_info.start_timestamp = srcHex[13] << 24 | srcHex[14] << 16 | srcHex[15] << 8 | srcHex[16];
        key_info.end_timestamp = srcHex[17] << 24 | srcHex[18] << 16 | srcHex[19] << 8 | srcHex[20];
        key_info.valid_cnt = srcHex[21] << 8 | srcHex[22];

        memset(buffer, 0, sizeof(buffer));
        sendLen = 0;
        buffer[0] = DATATYPE_DEVICE_MANAGE_CODED_LOCK_TIMELINESS;
        memcpy(buffer + 2, srcHex + 2, 21);

        DEBUG_TRACE(LOG_TAG, "Key Word Arg[%d] Num[%d] state[%d] Len[%d] [%s] [%d-%d] cnt[%d]", arg, key_info.user_num, key_info.isable, key_info.len,
                    key_info.word, key_info.start_timestamp, key_info.end_timestamp, key_info.valid_cnt);

         

        u8 index = find_coded_key_num(key_info.user_num, arg); // 查找当前用户编号位置
        if (arg == 0) {                                        // 新增修改
            if (index >= MAX_SECRET_TIMELINESS_NUM + 1) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                if (key_info.len > 8 || key_info.len < 6) {
                    DEBUG_TRACE(ERROR_TAG, "Key Password Len Error", key_info.len);
                    buffer[1] = 0; // 操作失败
                    sendLen = 3;
                } else {
                    buffer[1] = 1; // 操作成功
                    sendLen = 23;
                    add_coded_key_num(index, &key_info);
                }
            }
            dataLen -= 23;
            srcHex += 23;
        } else if (arg == 1) { // 删除
            if (index >= MAX_SECRET_TIMELINESS_NUM + 1) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                delete_coded_key_num(index);
                buffer[1] = 2; // 删除成功
                sendLen = 3;
            }
            dataLen -= 3;
            srcHex += 3;
        } else if (arg == 2) { // 读取
            if (index >= MAX_SECRET_TIMELINESS_NUM + 1) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                read_coded_key_num(index, buffer + 2);
                buffer[1] = 1; // 读取成功
                sendLen = 23;
            }
            dataLen -= 3;
            srcHex += 3;
        }
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_MANAGE_CARD_LOCK_TIMELINESS: {
        struct secret_card_timeliness card_info = {0};
        u8 arg = srcHex[1];
        card_info.user_num = srcHex[2];
        card_info.isable = srcHex[3];
        memcpy(card_info.id, srcHex + 4, 4);
        memcpy(card_info.id_key, srcHex + 8, 16);
        card_info.start_timestamp = srcHex[24] << 24 | srcHex[25] << 16 | srcHex[26] << 8 | srcHex[27];
        card_info.end_timestamp = srcHex[28] << 24 | srcHex[29] << 16 | srcHex[30] << 8 | srcHex[31];
        card_info.valid_cnt = srcHex[32] << 8 | srcHex[33];

         
        memset(buffer, 0, sizeof(buffer));
        sendLen = 0;
        buffer[0] = DATATYPE_DEVICE_MANAGE_CARD_LOCK_TIMELINESS;
        memcpy(buffer + 2, srcHex + 2, 32);
        DEBUG_TRACE(LOG_TAG,
                    "Card Arg[%d] Num[%d] state[%d] id[%02X%02X%02X%02X] key[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X] "
                    "[%d-%d] cnt[%d]",
                    arg, card_info.user_num, card_info.isable, card_info.id[0], card_info.id[1], card_info.id[2], card_info.id[3],
                    card_info.id_key[0], card_info.id_key[1], card_info.id_key[2], card_info.id_key[3], card_info.id_key[4], card_info.id_key[5],
                    card_info.id_key[6], card_info.id_key[7], card_info.id_key[8], card_info.id_key[9], card_info.id_key[10], card_info.id_key[11],
                    card_info.id_key[12], card_info.id_key[13], card_info.id_key[14], card_info.id_key[15], card_info.start_timestamp,
                    card_info.end_timestamp, card_info.valid_cnt);

        u8 index = find_coded_card_num(card_info.user_num, arg); // 查找当前用户编号位置
        if (arg == 0) {                                          // 新增修改
            if (index >= MAX_SECRET_TIMELINESS_NUM) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                buffer[1] = 1; // 操作成功
                sendLen = 34;
                add_coded_card_num(index, &card_info);
            }
            dataLen -= 34;
            srcHex += 34;
        } else if (arg == 1) { // 删除
            if (index >= MAX_SECRET_TIMELINESS_NUM) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                delete_coded_card_num(index);
                buffer[1] = 2; // 删除成功
                sendLen = 3;
            }
            dataLen -= 3;
            srcHex += 3;
        } else if (arg == 2) { // 读取
            if (index >= MAX_SECRET_TIMELINESS_NUM) {
                buffer[1] = 0; // 无效（失败）
                sendLen = 3;
            } else {
                read_coded_card_num(index, buffer + 2);
                buffer[1] = 1; // 读取成功
                sendLen = 34;
            }
            dataLen -= 3;
            srcHex += 3;
        }
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_OPEN_MODE: { // 常开模式
        struct coded_lock_open_mode open_mode;
        open_mode.mode = srcHex[1];
        open_mode.lock_delay = srcHex[2] << 8 | srcHex[3];
        open_mode.loop_duty = srcHex[4];
        open_mode.loop_cnt = srcHex[5];
        open_mode.loop_start_hour = srcHex[6];
        open_mode.loop_start_min = srcHex[7];
        open_mode.loop_start_sec = srcHex[8];
        open_mode.loop_over_hour = srcHex[9];
        open_mode.loop_over_min = srcHex[10];
        open_mode.loop_over_sec = srcHex[11];

        DEBUG_TRACE(LOG_TAG, "Recv Open Mode[%d]", open_mode.mode);
        memset(buffer, 0, sizeof(buffer));
        sendLen = 0;
        buffer[0] = DATATYPE_DEVICE_OPEN_MODE;
        memcpy(buffer, srcHex, 12);

         

        if (open_mode.mode == 0) { // 关闭常开模式
            sendLen = 12;
            set_coded_open_mode(&open_mode);
        } else if (open_mode.mode == 1) { // 打开常开模式1，需手动上锁（按35#上锁）
            sendLen = 12;
            set_coded_open_mode(&open_mode);
        } else if (open_mode.mode == 2) { // 打开常开模式2，延时一段时间上锁
            sendLen = 12;
            set_coded_open_mode(&open_mode);
        } else if (open_mode.mode == 3) { // 打开常开模式3，某个时间段自动处于常开模式
            if (open_mode.loop_duty > 0x7F || open_mode.loop_duty < 1) {
                DEBUG_TRACE(ERROR_TAG, "Open Mode Loop Duty Error [%d]", open_mode.loop_duty);
                buffer[1] = 0xFE;
                sendLen = 2;
            }
            if (open_mode.loop_start_hour > 24 || open_mode.loop_start_hour >= 60 || open_mode.loop_start_sec >= 60 ||
                open_mode.loop_over_hour > 24 || open_mode.loop_over_min >= 60 || open_mode.loop_over_sec >= 60) {
                DEBUG_TRACE(ERROR_TAG, "Open Mode Loop Time Error");
                buffer[1] = 0xFE;
                sendLen = 2;
            } else {
                set_coded_open_mode(&open_mode);
            }
        }
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 12;
        srcHex += 12;
    } break;
    case DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME: { // 门锁自动回锁时间
        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
         
        buffer[0] = DATATYPE_DEVICE_OPEN_MODE;
        memcpy(buffer, srcHex, 2);
        if (srcHex[1] < 3 || srcHex[1] > 30) {
            DEBUG_TRACE(ERROR_TAG, "Recv Auto Lock Time Error[%d]", srcHex[1]);
            buffer[1] = 0xFE;
        } else {
            DEBUG_TRACE(LOG_TAG, "Recv Auto Lock Time [%d]", srcHex[1]);
            device_t.auto_lock_time = srcHex[1];
            FLASH_Write_Device_Param();
        }
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
        dataLen -= 2;
        srcHex += 2;
    } break;
    case DATATYPE_DEVICE_TIMESTAMP: { // 时间戳
         
        if (srcHex[1] == 0xFF) {
            dataLen -= 2;
            srcHex += 2;
            DEBUG_TRACE(LOG_TAG, "Recv Read Timestamp[%d]", Timestamp);
        } else {
            Timestamp = srcHex[1] << 24 | srcHex[2] << 16 | srcHex[3] << 8 | srcHex[4];
            DEBUG_TRACE(LOG_TAG, "Recv Sync Timestamp[%d]", Timestamp);
            convertTimestampToSystime();
            RTC_Init();
            dataLen -= 5;
            srcHex += 5;
        }
        memset(buffer, 0, sizeof(buffer));
        sendLen = 5;
        buffer[0] = DATATYPE_DEVICE_TIMESTAMP;
        buffer[1] = (Timestamp >> 24) & 0xFF;
        buffer[2] = (Timestamp >> 16) & 0xFF;
        buffer[3] = (Timestamp >> 8) & 0xFF;
        buffer[4] = Timestamp & 0xFF;
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_USER_BIND: { // 用户绑定状态
         
        if (srcHex[1] == 0x56) {
            DEBUG_TRACE(LOG_TAG, "Recv User Unband CMD");
            device_t.isBand = 1;
            FLASH_Write_Device_Param();
        } else if (srcHex[1] == 0) {
            DEBUG_TRACE(LOG_TAG, "Recv User Band CMD");
            device_t.isBand = 0;
            FLASH_Write_Device_Param();
        } else {
            DEBUG_TRACE(ERROR_TAG, "Recv User Band CMD Error [%d]", srcHex[1]);
        }
        dataLen -= 2;
        srcHex += 2;

        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
        buffer[0] = DATATYPE_DEVICE_USER_BIND;
        buffer[1] = 1;
        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_VOLUME_SET: { // 音量设置
         
        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
        buffer[0] = DATATYPE_DEVICE_VOLUME_SET;
        if (srcHex[1] > 100) {
            buffer[1] = 0xFE;
            DEBUG_TRACE(ERROR_TAG, "Recv Set Volume CMD Error [%d]", srcHex[1]);
        } else {
            device_t.volume = srcHex[1];
            set_coded_lock_volume();
            buffer[1] = 0x50;
            DEBUG_TRACE(LOG_TAG, "Recv Set Volume CMD [%d]", srcHex[1]);
        }
        dataLen -= 2;
        srcHex += 2;

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_TIME_ZONE: { // 时区设置
         
        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
        buffer[0] = DATATYPE_DEVICE_TIME_ZONE;
        u8 timezone = srcHex[1];
        if (timezone > 26) {
            DEBUG_TRACE(ERROR_TAG, "Recv Set Time Zone Err[%d]", timezone);
            buffer[1] = 0xFE;
        } else {
            if (timezone == 25) {
                device_t.timezone = 3.5;
            } else if (timezone == 26) {
                device_t.timezone = 5.5;
            } else if (timezone < 13) {
                device_t.timezone = timezone;
            } else {
                device_t.timezone = -(timezone - 12);
            }
            DEBUG_TRACE(LOG_TAG, "Config Time Zone [%.1f]", device_t.timezone);
            convertTimestampToSystime();
            RTC_Init();
            FLASH_Write_Device_Param();
            dataReportTimestamp = Timestamp;
        }
        dataLen -= 2;
        srcHex += 2;

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
    case DATATYPE_DEVICE_REST_FACTORY: {
         
        u8 restFactorySign = srcHex[1];
        DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);

        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
        buffer[0] = DATATYPE_DEVICE_REST_FACTORY;
        if (restFactorySign == 1 && device_t.isBand == 0) {
            FLASH_Reset_Param();
            dataReportTimestamp = Timestamp;

            // 写入操作记录
            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.factory = 1;
            exc_lock_info_t.type = 4;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
        } else {
            buffer[1] = 0xFE;
        }

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }

        dataLen -= 2;
        srcHex += 2;
    } break;
    case DATATYPE_DEVICE_SYNC_PERIOD: {
        u16 reportInterval = srcHex[1] << 8 | srcHex[2];
        dataLen -= 3;
        srcHex += 3;

         
        
        memset(buffer, 0, sizeof(buffer));
        sendLen = 2;
        buffer[0] = DATATYPE_DEVICE_SYNC_PERIOD;
        if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            FLASH_Write_Device_Param();
            dataReportTimestamp = Timestamp;
            DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.reportInterval);
        } else {
            buffer[1] = 0xFE;
            DEBUG_TRACE(ERROR_TAG, "Device Sync Interval Over Threshold: %d", reportInterval);
        }

        int free_number = lwrb_get_free(&lwrbBuff_t);
        if (free_number >= (sendLen + 1)) {
            lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
            lwrb_write(&lwrbBuff_t, buffer, sendLen);
            lwrb_write_count++;
        } else {
            DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
        }
    } break;
#else
        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            if (restFactorySign == 1) {
                reset_coded_lock();
                reset_card_binding();
                reset_fingerprint();
                coded_lock_init();

                device_t.factoryFlag = 1;
                FLASH_Write_Factory_Flag();
                FLASH_Reset_Param();

                // 写入操作记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.factory = 1;
                exc_lock_info_t.type = 4;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
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
            DEBUG_TRACE(LOG_TAG, "Recv Server Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        } break;
        case DATATYPE_DEVICE_MANAGE_CODED_LOCK: { // 管理开锁密码
            u8 arg = srcHex[1];
            u8 userNum = srcHex[2];
            u8 pass_len = srcHex[3];
            char password[10] = {0};
            u8 event = 0;

            memset(buffer, 0, sizeof(buffer));
            sendLen = 0;

            if ((pass_len > 8 || pass_len < 6) && arg == 0)
                return;

            memcpy(password, srcHex + 4 + (8 - pass_len), pass_len);
            DEBUG_TRACE(LOG_TAG, "Arg[%d] Num[%d] Len[%d] [%s]", arg, userNum, pass_len, password);

            buffer[0] = DATATYPE_DEVICE_MANAGE_CODED_LOCK;
            buffer[1] = 0xFF; // 操作成功
            switch (arg) {
            case 0: // 新增/修改
                for (int i = 0; i < pass_len; i++) {
                    if (password[i] < 0x30 || password[i] > 0x39) {
                        DEBUG_TRACE(ERROR_TAG, "PassWord ERROR!!!");
                        return;
                    }
                }
                set_coded_lock(userNum, password, 0);
                memset(coded_lock_t.pass[userNum].word, 0, sizeof(coded_lock_t.pass[userNum].word));
                memcpy(coded_lock_t.pass[userNum].word, password, pass_len);
                if (!coded_lock_t.pass[userNum].isable) {
                    coded_lock_t.pass[userNum].isable = 1;
                    coded_lock_t.total_num_pass++;
                }
                 

                {
                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.inputAction = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = userNum;
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[userNum].word);
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
                break;
            case 1: // 删除
                del_coded_lock(userNum, 0);
                if (coded_lock_t.pass[userNum].isable) {
                    coded_lock_t.pass[userNum].isable = 0;
                    coded_lock_t.total_num_pass--;
                }
                 
                {
                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.deleteAction = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = userNum;
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[userNum].word);
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
                break;
            case 2: // 读取
                memset(password, 0, sizeof(password));
                read_coded_lock(userNum, password, &event);
                pass_len = strlen(password);
                if (pass_len < 6) {
                    buffer[1] = 0xFE; // 操作失败
                    DEBUG_TRACE(ERROR_TAG, "Server Read Password Fail");
                }
                 
                {
                    // 写入操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.readAction = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = userNum;
                    strcat(exc_lock_info_t.word, coded_lock_t.pass[userNum].word);
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
                break;
            default:
                return;
            }

            buffer[2] = userNum;
            buffer[3] = pass_len;
            memcpy(buffer + 4 + (8 - pass_len), password, pass_len);
            sendLen = 12;

            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) {
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }

            dataLen -= 12;
            srcHex += 12;
        } break;
        case DATATYPE_DEVICE_REMOTE_UNLOCK: { // 远程开锁
            if (srcHex[1] == 0) {
                DEBUG_TRACE(LOG_TAG, "Server Remote Unlock");
                 
                tx_thread_sleep(200);
                if (!device_t.lockDoorTimeout) // 已开锁时不允许重复开锁
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
                memset(buffer, 0, sizeof(buffer));
                sendLen = 7;

                buffer[0] = 0x30;
                buffer[1] = 0xC5;
                buffer[2] = 0x00;
                buffer[3] = (Timestamp >> 24) & 0xFF;
                buffer[4] = (Timestamp >> 16) & 0xFF;
                buffer[5] = (Timestamp >> 8) & 0xFF;
                buffer[6] = Timestamp & 0xFF;

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
                {
                    // 写入远程开锁操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = 0;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            }
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_MANAGE_CARD_FINGER: { // 管理卡和指纹状态
            u8 arg = srcHex[1];
            u8 userNum = srcHex[2];
            u8 etype = srcHex[3];
            char card[10] = {0};
            u8 finger_state;
            u8 event = 0;

            memset(buffer, 0, sizeof(buffer));
            sendLen = 0;
            buffer[0] = 0x31;
            buffer[1] = 0xFF;
            buffer[3] = 0;
            switch (arg) {
            case 1:             // 删除
                if (etype == 0) // 删除卡
                {
                    // read_card_binding(userNum, card, &event);
                    if (coded_lock_t.card[userNum].isable) {
                        coded_lock_t.card[userNum].isable = 0;
                        coded_lock_t.total_num_card--;
                        del_card_binding(userNum, 0);
                    } else {
                        buffer[1] = 0xFE; // 操作失败
                        buffer[3] = 1;    // 无效
                    }

                    {
                        struct lock_log exc_lock_info_t;
                        memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                        exc_lock_info_t.u_exec.isLocal = 0;
                        exc_lock_info_t.u_exec.deleteAction = 1;
                        exc_lock_info_t.type = 2;
                        exc_lock_info_t.number = userNum;
                        exc_lock_info_t.stamp = Timestamp;
                        nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                    }
                } else if (etype == 1) // 删除指纹
                {
#if FUNC_FINGERPRINT_ENABLE
                    // read_fingerprint(userNum, (char *)&finger_state, &event);
                    del_fingerprint(userNum, 0);
                    if (coded_lock_t.finger[userNum].isable) {
                        coded_lock_t.finger[userNum].isable = 0;
                        coded_lock_t.total_num_finger--;
                        if (read_template(userNum) == 0) // 读取模组有效 强制删除操作
                        {
                            delete_template(userNum, 1);
                        }
                    }
                    // if (finger_state) // 本地有效
                    // {
                    //     buffer[3] = 0; // 有效
                    //     buffer[1] = 0; // 操作成功

                    //     if (read_template(userNum) == 0) // 读取模组有效 强制删除操作
                    //     {
                    //         delete_template(userNum, 1);
                    //     }
                    // }
                    else {
                        buffer[1] = 0xFE; // 操作失败
                        buffer[3] = 1;    // 无效
                    }
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.deleteAction = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
#endif
                }
                 
                break;
            case 2:             // 读取
                if (etype == 0) // 读取卡状态
                {
                    // memset(card, 0, sizeof(card));
                    // read_card_binding(userNum, card, &event);
                    // if (strlen(card) > 3)
                    if (!coded_lock_t.card[userNum].isable) {
                        buffer[1] = 0xFE; // 操作失败
                        buffer[3] = 1;    // 无效
                    }

                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.readAction = 1;
                    exc_lock_info_t.type = 2;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                } else if (etype == 1)                        // 读取指纹状态
                {
                    // read_fingerprint(userNum, (char *)&finger_state, &event);
                    // if (finger_state)
                    if (!coded_lock_t.finger[userNum].isable) {
                        buffer[1] = 0xFE; // 操作失败
                        buffer[3] = 1;    // 无效
                    }
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.readAction = 1;
                    exc_lock_info_t.type = 1;
                    exc_lock_info_t.number = userNum;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
                 
                break;
            default:
                return;
            }

            buffer[2] = userNum;
            sendLen = 4;

            int free_number = lwrb_get_free(&lwrbBuff_t);
            if (free_number >= (sendLen + 1)) {
                lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                lwrb_write(&lwrbBuff_t, buffer, sendLen);
                lwrb_write_count++;
            } else {
                DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
            }

            dataLen -= 4;
            srcHex += 4;
        } break;
        case DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD: {
            memset(&coded_lock_t.temp_pass, 0, sizeof(coded_lock_t.temp_pass));
            coded_lock_t.temp_pass.isable = 1;
            coded_lock_t.temp_pass.time = Timestamp + (srcHex[1] << 8 | srcHex[2]) * 60;
            memcpy(coded_lock_t.temp_pass.word, srcHex + 4 + 8 - (srcHex[3]), srcHex[3]);
            DEBUG_TRACE(LOG_TAG, "Recv Server Temp Password Len[%d] [%s]", srcHex[3], coded_lock_t.temp_pass.word);
             
        }
            dataLen -= 12;
            srcHex += 12;
            break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;
            if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
                device_t.reportInterval = reportInterval;
                dataReportTimestamp = Timestamp;
                FLASH_Write_Device_Param();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.reportInterval);
                 
            } else {
                DEBUG_TRACE(ERROR_TAG, "Device Sync Interval Over Threshold: %d", reportInterval);
            }
        } break;
#endif
    default: // 遇到错误直接退出不解析
        dataLen = 0;
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
    u8 tpye_cnt = 0;

    memset(buffer, 0, sizeof(buffer));
    sendLen = 0;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    buffer[0] = DATATYPE_DEVICE_COMMUNICATE_RESPONSE_ACK;
    buffer[1] = 0;
    dataHex += 2;
    sendLen = 2;
#endif

    dataLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    dataLen = setDataPackage(DATATYPE_DEVICE_TIME_ZONE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_VOLUME_SET, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_OPEN_MODE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_USER_BIND, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

#endif

    dataLen = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

    dataLen = setDataPackage(DATATYPE_DEVICE_TIMESTAMP, dataHex);
    dataHex += dataLen;
    sendLen += dataLen;
    tpye_cnt++;

#if FUNC_OPERATIONAL_VERSION_ENABLE
    buffer[1] = tpye_cnt;
#endif

    int free_number = lwrb_get_free(&lwrbBuff_t);
    if (free_number >= (sendLen + 1)) {
        lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, buffer, sendLen);
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
    u8 buffer[50] = {0};
    u8 sendLen = 0;

    /* 请求服务端同步本地时间 */
    if (device_t.syncLocalTime) {
        device_t.syncLocalTime = 0;

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

    if (device_t.reportInterval) {
        if (dataReportTimestamp <= Timestamp) {
            dataReportTimestamp = Timestamp + device_t.reportInterval * 60;
            device_t.syncDeviceState = 1;
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
    u8 buffer[100] = {0};
    u8 sendLen = 0;

    DeciceEvent_Process();

#if !LOCAL_LOCK_ENABLE
    if (!uartLoRa.sendDelay) {
        // if (LoRaWAN.joinState)
        {

#if !FUNC_OPERATIONAL_VERSION_ENABLE
            // 同步状态恢复出厂设置给云端
            if (device_t.factoryFlag) {
                buffer[0] = 0x85;
                buffer[1] = 1;
                if (sendLoRaWANData(buffer, 2) == true) {
                    device_t.factoryFlag = 0;
                    FLASH_Write_Factory_Flag();
                }
            }
#endif
            /* 同步设备数据 */
            if (device_t.syncDeviceState) {
                device_t.syncDeviceState = 0;
                device_t.updateBattery = 0;
                Device_PeriodicReport();
            }
            /* 更新电量信息 */
            if (device_t.updateBattery) {
                device_t.updateBattery = 0;
                memset(buffer, 0, sizeof(buffer));
                sendLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, buffer);

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
            }
            /* 检测缓冲器是否存在待发射数据 */
            if (lwrb_write_count) // 缓冲区内有数据
            {
                u8 *dataHex = buffer;
                device_t.sending = 1;
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
#endif
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
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(LOG_TAG, "Recv Uart Report Interval : %dm", device_t.reportInterval);
            FLASH_Write_Device_Param();
        } else
            DEBUG_TRACE(ERROR_TAG, "Recv Uart Report Interval Param ERROR");
    }

    // 上位机软件配置
    /* 设置数据同步周期 */
    else if (strstr((const char *)data, "setDataSyncPeriod:") && len <= strlen("setDataSyncPeriod:1440") && len >= strlen("setDataSyncPeriod:1")) {
        valid = strstr((const char *)data, ":");
        u16 reportInterval = atoi(valid + 1);
        if ((reportInterval <= 1440 && reportInterval >= 10) || reportInterval == 0) {
            device_t.reportInterval = reportInterval;
            DEBUG_TRACE(ACK_TAG, "setDataSyncPeriod: %dm \r\n OK", device_t.reportInterval);
            FLASH_Write_Device_Param();
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
            DEBUG_TRACE(ACK_TAG, "Smart Door Lock startFactoryTest OK");
            factoryMode = 1;
        }
    } else if (strstr((const char *)data, "getDeviceStatue") || strstr((const char *)data, "getDeviceStatus")) {
        memset(tempBuf, 0, sizeof(tempBuf));
        if (LoRaWAN.class == 0) {
            sprintf(tempBuf, "deviceState");
            sprintf(tempBuf + strlen(tempBuf), ",batteryStatus:%d", batteryLevel);
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
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
            sprintf(tempBuf + strlen(tempBuf), ",powerStatus:%d", 1);
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

    else if (strstr((const char *)data, "getDeviceNorFlash")) // 获取设备历史数据
    {
        nor_flash_read_export(&tsdb);
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
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD, %04d-%02d-%02d-%d:%d:%d\r\n OK", systime.tm_year, systime.tm_mon, systime.tm_mday,
                        systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else if (strstr((const char *)data, "hkt") && data[5] == 1) // 更新固件
    {
        FLASH_Write_Update_Flag();
        mDelay(100);
        NVIC_SystemReset(); // 复位
    } else if (strstr((const char *)data, "touch:") && strlen((const char *)data) == 7) {
        char *valid = strstr((const char *)data, ":") + 1;
        uart_touch_press = valid[0];
        tx_event_flags_set(&event_group, BIT_TOUCH, TX_OR);
    } else if (strstr((const char *)data, "reboot")) {
        NVIC_SystemReset();
    }
#if !FUNC_OPERATIONAL_VERSION_ENABLE
    else if (strstr((const char *)data, "setPassword")) {
        char pass[10] = {0};
        char *valid = strstr((const char *)data, ":") + 1;
        int validNum = 0;
        memset(&splitBuf, 0, sizeof(splitBuf));
        cmd_split(valid, ",", splitBuf, &validNum);
        if (validNum >= 2) {
            u8 userNum = atoi(splitBuf[0]);
            char param;
            u8 event = 0;
            u8 plen = strlen(splitBuf[1]);
            memcpy(pass, splitBuf[1], plen);
            for (int i = 0; i < plen; i++) {
                param = pass[i];
                if (param < '0' || param > '9') {
                    DEBUG_TRACE(WARN_TAG, "password format error: %s", pass);
                    return;
                }
            }
            if (plen > 10 || plen < 6) {
                DEBUG_TRACE(WARN_TAG, "password len error: %d", plen);
                return;
            }
            if (userNum > 99) {
                DEBUG_TRACE(WARN_TAG, "user num more than max: %d", userNum);
                return;
            }
            set_coded_lock(userNum, splitBuf[1], 1);
            read_coded_lock(userNum, pass, &event);
            coded_lock_init();
        }
    } else if (strstr((const char *)data, "readPassword")) {
        char pass[10] = {0};
        char *valid = strstr((const char *)data, ":") + 1;
        u8 userNum = atoi(valid);
        u8 event = 0;
        if (userNum > 99) {
            DEBUG_TRACE(WARN_TAG, "user num format error");
            return;
        }
        read_coded_lock(userNum, pass, &event);
    } else if (strstr((const char *)data, "deletePassword")) {
        char *valid = strstr((const char *)data, ":") + 1;
        u8 userNum = atoi(valid);
        if (userNum > 99) {
            DEBUG_TRACE(WARN_TAG, "user num format error");
            return;
        }
        del_coded_lock(userNum, 2);
        coded_lock_init();
    } else if (strstr((const char *)data, "readLocalInfo")) {
        printf_local_info();
    }
#endif
    else {
        DEBUG_TRACE(WARN_TAG, "Uart Failed To Parse Data!");
    }
}

/**
 * @brief  处理来自蓝牙端的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBleDataHandle(u8 *data, u16 len)
{
    u8 buffer[100];
    u8 sendLen;

#if FUNC_BT_ENABLE
    /*  确认数据 */
    if (checkDataSyncHead(data) == FAIL) {
        DEBUG_TRACE(ERROR_TAG, "Data Sync Head ERROR!");
        return;
    }
    if (data[3] & 0x02) { // 通讯启用校验
        if (CalCheckSum(data, len) == FAIL) {
            DEBUG_TRACE(ERROR_TAG, "Data Check Sum ERROR!");
            return;
        }
    }

    /*  解析数据  */
    u8 dataLen = len - 5;
    if (data[3] & 0x02) { // 通讯启用校验
        dataLen -= 1;
    }
    DEBUG_TRACE(LOG_TAG, "Recv LoRaWAN CMD, Total Data Len : %d", dataLen);
    u8 *srcHex = data + 5;
    while (dataLen) {
        u8 dataType = srcHex[0];
        DEBUG_TRACE(LOG_TAG, "Analysis Data Type: 0x%02X", dataType);
        switch (dataType) {
        case DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION: {
            memset(buffer, 0, sizeof(buffer));
            sendLen = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, buffer);
            sendBTData(buffer, sendLen);
            dataLen -= 1;
            srcHex += 1;
        } break;
        case DATATYPE_DEVICE_ADDR_ID: {
            memset(buffer, 0, sizeof(buffer));
            sendLen = setDataPackage(DATATYPE_DEVICE_ADDR_ID, buffer);
            sendBTData(buffer, sendLen);
            dataLen -= 1;
            srcHex += 1;
        } break;
        case DATATYPE_DEVICE_BATTERY_INFO: {
            memset(buffer, 0, sizeof(buffer));
            sendLen = setDataPackage(DATATYPE_DEVICE_BATTERY_INFO, buffer);
            sendBTData(buffer, sendLen);
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_REMOTE_UNLOCK: {
            memset(buffer, 0, sizeof(buffer));
            sendLen = setDataPackage(DATATYPE_DEVICE_REMOTE_UNLOCK, buffer);
            buffer[1] = srcHex[1];
            sendBTData(buffer, sendLen);
            if (srcHex[1] == 1) { // 关锁
                /* 反转 */
                MOTOR_REVERSAL();
                tx_thread_sleep(150);
                /* 刹车 */
                MOTOR_BRAKE();
                /* 待机 */
                MOTOR_STANDBY();
            } else if (srcHex[1] == 0) {
                if (!device_t.lockDoorTimeout) { // 已开锁时不允许重复开锁
                    tx_event_flags_set(&event_group, BIT_MOTOR, TX_OR);
#if FUNC_FINGERPRINT_ENABLE
                    tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_STOP, TX_OR);
#endif
                }
                memset(buffer, 0, sizeof(buffer));
                sendLen = 7;

                buffer[0] = DATATYPE_DEVICE_REPORT_UNLOCK_RECORD;
                buffer[1] = 0xC6;
                buffer[2] = 0x00;
                buffer[3] = (Timestamp >> 24) & 0xFF;
                buffer[4] = (Timestamp >> 16) & 0xFF;
                buffer[5] = (Timestamp >> 8) & 0xFF;
                buffer[6] = Timestamp & 0xFF;

                int free_number = lwrb_get_free(&lwrbBuff_t);
                if (free_number >= (sendLen + 1)) {
                    lwrb_write(&lwrbBuff_t, &sendLen, 1); // 写入数据长度
                    lwrb_write(&lwrbBuff_t, buffer, sendLen);
                    lwrb_write_count++;
                } else {
                    DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                }
                {
                    // 写入远程开锁操作记录
                    struct lock_log exc_lock_info_t;
                    memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                    exc_lock_info_t.u_exec.isLocal = 0;
                    exc_lock_info_t.u_exec.unlockSuccess = 1;
                    exc_lock_info_t.type = 0;
                    exc_lock_info_t.number = 0;
                    exc_lock_info_t.stamp = Timestamp;
                    nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
                }
            }
            dataLen -= 2;
            srcHex += 2;
        } break;

#if FUNC_OPERATIONAL_VERSION_ENABLE
        case DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD: {
            memset(&coded_lock_t.temp_pass, 0, sizeof(coded_lock_t.temp_pass));
            coded_lock_t.temp_pass.isable = 1;
            coded_lock_t.temp_pass.time = Timestamp + (srcHex[1] << 8 | srcHex[2]) * 60;
            memcpy(coded_lock_t.temp_pass.word, srcHex + 4 + 8 - (srcHex[3]), srcHex[3]);
            DEBUG_TRACE(LOG_TAG, "Recv Server Temp Password Len[%d] [%s]", srcHex[3], coded_lock_t.temp_pass.word);
            memset(buffer, 0, sizeof(buffer));
            sendLen = setDataPackage(DATATYPE_DEVICE_MANAGE_TEMP_PASSWORD, buffer);
            sendBTData(buffer, sendLen);
            dataLen -= 12;
            srcHex += 12;
        } break;
        case DATATYPE_DEVICE_MANAGE_CODED_LOCK_TIMELINESS: {
            struct secret_key_timeliness key_info = {0};
            u8 arg = srcHex[1];
            key_info.user_num = srcHex[2];
            key_info.isable = srcHex[3];
            key_info.len = srcHex[4];
            memcpy(key_info.word, srcHex + 5 + (8 - key_info.len), key_info.len);
            key_info.start_timestamp = srcHex[13] << 24 | srcHex[14] << 16 | srcHex[15] << 8 | srcHex[16];
            key_info.end_timestamp = srcHex[17] << 24 | srcHex[18] << 16 | srcHex[19] << 8 | srcHex[20];
            key_info.valid_cnt = srcHex[21] << 8 | srcHex[22];

            memset(buffer, 0, sizeof(buffer));
            sendLen = 0;
            buffer[0] = DATATYPE_DEVICE_MANAGE_CODED_LOCK_TIMELINESS;
            memcpy(buffer + 2, srcHex + 2, 21);

            DEBUG_TRACE(LOG_TAG, "Key Word Arg[%d] Num[%d] state[%d] Len[%d] [%s] [%d-%d] cnt[%d]", arg, key_info.user_num, key_info.isable,
                        key_info.len, key_info.word, key_info.start_timestamp, key_info.end_timestamp, key_info.valid_cnt);

            u8 index = find_coded_key_num(key_info.user_num, arg); // 查找当前用户编号位置
            if (arg == 0) {                                        // 新增修改
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    if (key_info.len > 8 || key_info.len < 6) {
                        DEBUG_TRACE(ERROR_TAG, "Key Password Len Error", key_info.len);
                        buffer[1] = 0; // 操作失败
                        sendLen = 3;
                    } else {
                        buffer[1] = 1; // 操作成功
                        sendLen = 23;
                        add_coded_key_num(index, &key_info);
                    }
                }
                dataLen -= 23;
                srcHex += 23;
            } else if (arg == 1) { // 删除
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    delete_coded_key_num(index);
                    buffer[1] = 2; // 删除成功
                    sendLen = 3;
                }
                dataLen -= 3;
                srcHex += 3;
            } else if (arg == 2) { // 读取
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    read_coded_key_num(index, buffer + 2);
                    buffer[1] = 1; // 读取成功
                    sendLen = 23;
                }
                dataLen -= 3;
                srcHex += 3;
            }
            sendBTData(buffer, sendLen);
        } break;
        case DATATYPE_DEVICE_MANAGE_CARD_LOCK_TIMELINESS: {
            struct secret_card_timeliness card_info = {0};
            u8 arg = srcHex[1];
            card_info.user_num = srcHex[2];
            card_info.isable = srcHex[3];
            memcpy(card_info.id, srcHex + 4, 4);
            memcpy(card_info.id_key, srcHex + 8, 16);
            card_info.start_timestamp = srcHex[24] << 24 | srcHex[25] << 16 | srcHex[26] << 8 | srcHex[27];
            card_info.end_timestamp = srcHex[28] << 24 | srcHex[29] << 16 | srcHex[30] << 8 | srcHex[31];
            card_info.valid_cnt = srcHex[32] << 8 | srcHex[33];

            memset(buffer, 0, sizeof(buffer));
            sendLen = 0;
            buffer[0] = DATATYPE_DEVICE_MANAGE_CARD_LOCK_TIMELINESS;
            memcpy(buffer + 2, srcHex + 2, 32);
            DEBUG_TRACE(LOG_TAG,
                        "Card Arg[%d] Num[%d] state[%d] id[%02X%02X%02X%02X] key[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X] "
                        "[%d-%d] cnt[%d]",
                        arg, card_info.user_num, card_info.isable, card_info.id[0], card_info.id[1], card_info.id[2], card_info.id[3],
                        card_info.id_key[0], card_info.id_key[1], card_info.id_key[2], card_info.id_key[3], card_info.id_key[4], card_info.id_key[5],
                        card_info.id_key[6], card_info.id_key[7], card_info.id_key[8], card_info.id_key[9], card_info.id_key[10],
                        card_info.id_key[11], card_info.id_key[12], card_info.id_key[13], card_info.id_key[14], card_info.id_key[15],
                        card_info.start_timestamp, card_info.end_timestamp, card_info.valid_cnt);

            u8 index = find_coded_card_num(card_info.user_num, arg); // 查找当前用户编号位置
            if (arg == 0) {                                          // 新增修改
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    buffer[1] = 1; // 操作成功
                    sendLen = 34;
                    add_coded_card_num(index, &card_info);
                }
                dataLen -= 34;
                srcHex += 34;
            } else if (arg == 1) { // 删除
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    delete_coded_card_num(index);
                    buffer[1] = 2; // 删除成功
                    sendLen = 3;
                }
                dataLen -= 3;
                srcHex += 3;
            } else if (arg == 2) { // 读取
                if (index >= MAX_SECRET_TIMELINESS_NUM) {
                    buffer[1] = 0; // 无效（失败）
                    sendLen = 3;
                } else {
                    read_coded_card_num(index, buffer + 2);
                    buffer[1] = 1; // 读取成功
                    sendLen = 34;
                }
                dataLen -= 3;
                srcHex += 3;
            }
            sendBTData(buffer, sendLen);
        } break;
        case DATATYPE_DEVICE_OPEN_MODE: { // 常开模式
            struct coded_lock_open_mode open_mode;
            open_mode.mode = srcHex[1];
            open_mode.lock_delay = srcHex[2] << 8 | srcHex[3];
            open_mode.loop_duty = srcHex[4];
            open_mode.loop_cnt = srcHex[5];
            open_mode.loop_start_hour = srcHex[6];
            open_mode.loop_start_min = srcHex[7];
            open_mode.loop_start_sec = srcHex[8];
            open_mode.loop_over_hour = srcHex[9];
            open_mode.loop_over_min = srcHex[10];
            open_mode.loop_over_sec = srcHex[11];

            DEBUG_TRACE(LOG_TAG, "Recv Open Mode[%d]", open_mode.mode);
            memset(buffer, 0, sizeof(buffer));
            sendLen = 0;
            buffer[0] = DATATYPE_DEVICE_OPEN_MODE;
            memcpy(buffer, srcHex, 12);
            if (open_mode.mode == 0) { // 关闭常开模式
                sendLen = 12;
                set_coded_open_mode(&open_mode);
            } else if (open_mode.mode == 1) { // 打开常开模式1，需手动上锁（按35#上锁）
                sendLen = 12;
                set_coded_open_mode(&open_mode);
            } else if (open_mode.mode == 2) { // 打开常开模式2，延时一段时间上锁
                sendLen = 12;
                set_coded_open_mode(&open_mode);
            } else if (open_mode.mode == 3) { // 打开常开模式3，某个时间段自动处于常开模式
                if (open_mode.loop_duty > 0x7F || open_mode.loop_duty < 1) {
                    DEBUG_TRACE(ERROR_TAG, "Open Mode Loop Duty Error [%d]", open_mode.loop_duty);
                    buffer[1] = 0xFE;
                    sendLen = 2;
                }
                if (open_mode.loop_start_hour > 24 || open_mode.loop_start_hour >= 60 || open_mode.loop_start_sec >= 60 ||
                    open_mode.loop_over_hour > 24 || open_mode.loop_over_min >= 60 || open_mode.loop_over_sec >= 60) {
                    DEBUG_TRACE(ERROR_TAG, "Open Mode Loop Time Error");
                    buffer[1] = 0xFE;
                    sendLen = 2;
                } else {
                    set_coded_open_mode(&open_mode);
                }
            }
            sendBTData(buffer, sendLen);
            dataLen -= 12;
            srcHex += 12;
        } break;
        case DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME: { // 门锁自动回锁时间
            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_LOCK_AUTO_BACK_TIME;
            memcpy(buffer, srcHex, 2);
            if (srcHex[1] < 3 || srcHex[1] > 30) {
                DEBUG_TRACE(ERROR_TAG, "Recv Auto Lock Time Error[%d]", srcHex[1]);
                buffer[1] = 0xFE;
            } else {
                DEBUG_TRACE(LOG_TAG, "Recv Auto Lock Time [%d]", srcHex[1]);
                device_t.auto_lock_time = srcHex[1];
                FLASH_Write_Device_Param();
            }
            sendBTData(buffer, sendLen);
            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_TIMESTAMP: { // 时间戳
            if (srcHex[1] == 0xFF) {
                dataLen -= 2;
                srcHex += 2;
                DEBUG_TRACE(LOG_TAG, "Recv Read Timestamp[%d]", Timestamp);
            } else {
                Timestamp = srcHex[1] << 24 | srcHex[2] << 16 | srcHex[3] << 8 | srcHex[4];
                DEBUG_TRACE(LOG_TAG, "Recv Sync Timestamp[%d]", Timestamp);
                convertTimestampToSystime();
                dataLen -= 5;
                srcHex += 5;
            }
            memset(buffer, 0, sizeof(buffer));
            sendLen = 5;
            buffer[0] = DATATYPE_DEVICE_TIMESTAMP;
            buffer[1] = (Timestamp >> 24) & 0xFF;
            buffer[2] = (Timestamp >> 16) & 0xFF;
            buffer[3] = (Timestamp >> 8) & 0xFF;
            buffer[4] = Timestamp & 0xFF;
            sendBTData(buffer, sendLen);
        } break;
        case DATATYPE_DEVICE_USER_BIND: { // 用户绑定状态
            if (srcHex[1] == 0x56) {
                DEBUG_TRACE(LOG_TAG, "Recv User Unband CMD");
                device_t.isBand = 1;
                FLASH_Write_Device_Param();
            } else if (srcHex[1] == 0) {
                DEBUG_TRACE(LOG_TAG, "Recv User Band CMD");
                device_t.isBand = 0;
                FLASH_Write_Device_Param();
            } else {
                DEBUG_TRACE(ERROR_TAG, "Recv User Band CMD Error [%d]", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;

            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_USER_BIND;
            buffer[1] = 1;
            sendBTData(buffer, sendLen);
        } break;
        case DATATYPE_DEVICE_VOLUME_SET: { // 音量设置
            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_VOLUME_SET;
            if (srcHex[1] > 100) {
                buffer[1] = 0xFE;
                DEBUG_TRACE(ERROR_TAG, "Recv Set Volume CMD Error [%d]", srcHex[1]);
            } else {
                device_t.volume = srcHex[1];
                set_coded_lock_volume();
                buffer[1] = 0x50;
                DEBUG_TRACE(LOG_TAG, "Recv Set Volume CMD [%d]", srcHex[1]);
            }
            dataLen -= 2;
            srcHex += 2;

            sendBTData(buffer, sendLen);
        } break;
        case DATATYPE_DEVICE_TIME_ZONE: { // 时区设置
            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_TIME_ZONE;
            u8 timezone = srcHex[1];
            if (timezone > 26) {
                DEBUG_TRACE(ERROR_TAG, "Recv Set Time Zone Err[%d]", timezone);
                buffer[1] = 0xFE;
            } else {
                if (timezone == 25) {
                    device_t.timezone = 3.5;
                } else if (timezone == 26) {
                    device_t.timezone = 5.5;
                } else if (timezone < 13) {
                    device_t.timezone = timezone;
                } else {
                    device_t.timezone = -(timezone - 12);
                }
                DEBUG_TRACE(LOG_TAG, "Config Time Zone [%.1f]", device_t.timezone);
                convertTimestampToSystime();
                RTC_Init();
                FLASH_Write_Device_Param();
                dataReportTimestamp = Timestamp;
            }
            sendBTData(buffer, sendLen);

            dataLen -= 2;
            srcHex += 2;
        } break;

#endif

        case DATATYPE_DEVICE_REST_FACTORY: {
            u8 restFactorySign = srcHex[1];
            DEBUG_TRACE(WARN_TAG, "Recv Device Rest Factory CMD : %d", restFactorySign);

            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_REST_FACTORY;
            if (restFactorySign == 1 && device_t.isBand == 0) {
                FLASH_Reset_Param();

                // 写入操作记录
                struct lock_log exc_lock_info_t;
                memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
                exc_lock_info_t.u_exec.isLocal = 1;
                exc_lock_info_t.u_exec.factory = 1;
                exc_lock_info_t.type = 4;
                exc_lock_info_t.stamp = Timestamp;
                nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
            } else {
                buffer[1] = 0xFE;
            }
            sendBTData(buffer, sendLen);

            dataLen -= 2;
            srcHex += 2;
        } break;
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            dataLen -= 3;
            srcHex += 3;

            memset(buffer, 0, sizeof(buffer));
            sendLen = 2;
            buffer[0] = DATATYPE_DEVICE_SYNC_PERIOD;
            if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
                device_t.reportInterval = reportInterval;
                FLASH_Write_Device_Param();
                dataReportTimestamp = Timestamp;
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", device_t.reportInterval);

            } else {
                buffer[1] = 0xFE;
                DEBUG_TRACE(ERROR_TAG, "Device Sync Interval Over Threshold: %d", reportInterval);
            }
            sendBTData(buffer, sendLen);
        } break;

        default: // 遇到错误直接退出不解析
            dataLen = 0;
            break;
        }
        if (dataLen > 200) // 长度异常，数据出错，退出数据解析
            break;
    }
#endif
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        DebugHexInfo("Recv LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }

    if (!uartBle.recvTimeout && uartBle.recvLen) {
        fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }

    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/**
 * @brief  处理来自LoRa串口接收到的数据
 * @param
 * @retval
 */
void fromLoRaUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        DebugHexInfo("Recv LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY) {
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        }
        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);

        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
}

/**
 * @brief  处理来自蓝牙串口接收到的数据
 * @param
 * @retval
 */
void fromBtUartDataHandle(void)
{
    if (!uartBle.recvTimeout && uartBle.recvLen) {
        // DebugHexInfo("Recv BT", uartBle.recvData, uartBle.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart BT Data: %s", (char *)uartBle.recvData);
        char *data = (char *)uartBle.recvData;
        if (strstr((const char *)data, "getDevEui")) {
            memset(tempBuf, 0, sizeof(tempBuf));
            sprintf(tempBuf, "DEVEUI:%02X%02X%02X%02X%02X%02X%02X%02X", LoRaWAN.DevEUI[0], LoRaWAN.DevEUI[1], LoRaWAN.DevEUI[2], LoRaWAN.DevEUI[3],
                    LoRaWAN.DevEUI[4], LoRaWAN.DevEUI[5], LoRaWAN.DevEUI[6], LoRaWAN.DevEUI[7]);
            DEBUG_TRACE(LOG_TAG, "%s", tempBuf);
            if (!strstr(tempBuf, "DEVEUI:0000000000000000")) {
                Ble_SendCMD(tempBuf);
            }
            tx_thread_sleep(100);
        } else if (strstr((const char *)data, "getProctName")) {
            memset(tempBuf, 0, sizeof(tempBuf));
            sprintf(tempBuf, "ProctName:%s", PROCT_NAME);
            Ble_SendCMD(tempBuf);
            tx_thread_sleep(100);
        } else if (strstr((const char *)data, "reboot")) {
            NVIC_SystemReset(); // 复位
        } else {
            fromBleDataHandle(uartBle.recvData, uartBle.recvLen);
        }
        memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
        uartBle.recvLen = 0;
    }
}

/**
 * @brief  处理来自打印串口接收到的数据
 * @param
 * @retval
 */
void fromInfoUartDataHandle(void)
{
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/**
 * @brief  通讯管理任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_communicate(ULONG thread_input)
{
    (void)thread_input;

    u8 gp_buffer[10];
    u8 gp_num;

    union u_mbi5120 t_mbi5120;
    t_mbi5120.dat = 0;
#if EU_TOUCH_LED
    t_mbi5120.s_mbi5120_t.led_4 = 1;
    t_mbi5120.s_mbi5120_t.led_8 = 1;
    t_mbi5120.s_mbi5120_t.led_6 = 1;
    t_mbi5120.s_mbi5120_t.led_x = 1;
    t_mbi5120.s_mbi5120_t.led_j = 1;
#else
    t_mbi5120.s_mbi5120_t.led_1 = 1;
    t_mbi5120.s_mbi5120_t.led_3 = 1;
    t_mbi5120.s_mbi5120_t.led_5 = 1;
    t_mbi5120.s_mbi5120_t.led_7 = 1;
    t_mbi5120.s_mbi5120_t.led_9 = 1;
#endif
    MBI5120_Display(t_mbi5120.dat);
#if !FUNC_OPERATIONAL_VERSION_ENABLE
    coded_lock_init();
#endif
    tx_event_flags_set(&event_group, BIT_SOUND, TX_OR);

#if LOCAL_LOCK_ENABLE
    device_t.sleepDelay = 10;
#else
    if (device_t.network == 0) {
        device_t.sleepDelay = 10;
    } else {
        if (device_t.sleepDelay < MAX_WAIT_SLEEP_TIMEOUT)
            device_t.sleepDelay = MAX_WAIT_SLEEP_TIMEOUT;
    }
    // LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
    // LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
#endif

    LoRaWAN_Init();
    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
    uartLoRa.recvLen = 0;

    while (1) {
    __back:
        tx_thread_sleep(100);
        if (device_t.sleepState) {
            goto __back;
        }
#if !LOCAL_LOCK_ENABLE
        LoRaWAN_JoinInit();
        fromLoRaUartDataHandle();
#endif
        DeviceStateProcess();

#if FUNC_TAMPER_ENABLE
        /* 未录入用户信息时触发无效 */
        if (!device_t.tamperAlarm &&
#if FUNC_OPERATIONAL_VERSION_ENABLE
            (coded_lock_t.total_num_pass > 0 || coded_lock_t.total_num_card > 0)) {
#else
            (coded_lock_t.total_num_pass > 0 || coded_lock_t.total_num_card > 0 || coded_lock_t.total_num_finger > 0)) {
#endif
            if (tamperio_state != READ_FUNC_KEY_STATUS) // 变更一次状态后才能触发防拆报警
            {
                tx_thread_sleep(1000);
                if (!READ_FUNC_KEY_STATUS) // 防拆按键松开
                {
                    device_t.tamperAlarm = 1;
                    gp_num = 2;
                    gp_buffer[0] = 0x84;
                    gp_buffer[1] = 1;
                    int free_number = lwrb_get_free(&lwrbBuff_t);
                    if (free_number >= (gp_num + 1)) {
                        lwrb_write(&lwrbBuff_t, &gp_num, 1); // 写入数据长度
                        lwrb_write(&lwrbBuff_t, gp_buffer, gp_num);
                        lwrb_write_count++;
                    } else {
                        DEBUG_TRACE(ERROR_TAG, "Lwrb Already Full");
                    }
                } else {
                    tamperio_state = READ_FUNC_KEY_STATUS;
                }
            }
        }
#endif
#if !FUNC_OPERATIONAL_VERSION_ENABLE
        if (system_rst) {
            INFO("\nPress rst key long handler\n");
            /* 语音 */
            // 系统初始化
            Sound_Insert_Number(1, VOICE_SYSTEM_INIT);

            system_rst = 0;
            reset_coded_lock();
            reset_card_binding();
            reset_fingerprint();
            coded_lock_init();

            device_t.factoryFlag = 1;
            FLASH_Write_Factory_Flag();
            FLASH_Reset_Param();

            GPIO_EXTI_Close(FINGER_DETECT_PORT, FINGER_DETECT_PIN);
            FINGER_POWER_H;
            tx_thread_sleep(200);
            GPIO_EXTI_Init(FINGER_DETECT_PORT, FINGER_DETECT_PIN, EXTI_RISING, ENABLE);
            // W25X40_ChipErase();

            // 写入操作记录
            struct lock_log exc_lock_info_t;
            memset(&exc_lock_info_t, 0, sizeof(exc_lock_info_t));
            exc_lock_info_t.u_exec.isLocal = 1;
            exc_lock_info_t.u_exec.factory = 1;
            exc_lock_info_t.type = 4;
            exc_lock_info_t.stamp = Timestamp;
            nor_flash_write(&tsdb, &exc_lock_info_t); // 将操作记录写入到外置nor flash
        }
#endif
    }
}

/**
 * @brief  串口事件任务
 * @param thread_input 创建该任务时传递的形参
 * @retval
 */
void thread_uart(ULONG thread_input)
{
    (void)thread_input;

    u32 cnt = 0;
    extern void DispthreadInfo(void);

    while (1) {
        fromInfoUartDataHandle();
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
void thread_bt(ULONG thread_input)
{
    (void)thread_input;
    // ULONG actual_events;
    // UINT status = tx_event_flags_get(&event_group,     /* 事件标志控制块 */
    //                                  BIT_BLE,          /* 等待标志 */
    //                                  TX_AND_CLEAR,     /* 等待特定bit满足 */
    //                                  &actual_events,   /* 获取实际值 */
    //                                  TX_WAIT_FOREVER); /* 永久等待 */
    // ble_init();
    while (1) {
        fromBtUartDataHandle();
        tx_thread_sleep(100);
        if (READ_BLE_STATUS) {
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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
    ULONG actual_events;
    u8 displayFlag;

#if FUNC_STANDARD_CLASSC_ENABLE
    while (1) {
        tx_thread_sleep(100);
    }
#else
    while (1) {
        tx_thread_sleep(100);
#if !LOCAL_LOCK_ENABLE
        if (!device_t.bleConnectState && !device_t.tamperAlarm && !device_t.sending && !uartLoRa.sendDelay && !device_t.sleepDelay &&
            !s_touch.timeout && !device_t.lockTimeout && !device_t.operateTimeout && !device_t.lockDoorTimeout &&
            (((LoRaWAN.joinState && !device_t.syncDeviceState && !lwrb_write_count) || device_t.network == 0) ||
             (!LoRaWAN.joinState && LoRaWAN.status != LoRaWAN_STATUS_NORMAL))) {
#else
        if (!device_t.bleConnectState && !device_t.tamperAlarm && !device_t.sleepDelay && !s_touch.timeout && !device_t.lockTimeout &&
            !device_t.operateTimeout && !device_t.lockDoorTimeout) {
#endif
            GPIO_EXTI_Close(TCH_IRQ_PORT, TCH_IRQ_PIN);
            setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
            MBI5120_Display(0);
            si12t_sleep();
            tx_event_flags_set(&main_event_group, MAIN_BIT_SLEEP, TX_OR);

#if FSV9522_CARD_ENABLE
            tx_event_flags_set(&fsv9522_event_group, FSV9522_BIT_SLEEP, TX_OR);
            // 等待读卡芯片空闲
            tx_event_flags_get(&fsv9522_event_group, FSV9522_BIT_SLEEP_DONE, TX_AND_CLEAR, &actual_events, TX_WAIT_FOREVER);
#endif

#if FUNC_FINGERPRINT_ENABLE
            tx_event_flags_set(&zs_display_event_group, FINGER_BIT_SLEEP, TX_OR);
            // 等待指纹模块空闲
            tx_event_flags_get(&zs_display_event_group, FINGER_BIT_SLEEP_DONE, TX_AND_CLEAR, &actual_events, TX_WAIT_FOREVER);
#endif
            // 置事件位为睡眠状态
            tx_event_flags_set(&main_event_group, MAIN_BIT_SLEEP, TX_AND);
            si12t_clear_read();
            ADC_BatterySglConvert();

            device_t.sleepState = 1;
            W25X40_PowerDown();
            SpiDeint();
            Spi1Deint();

            // 关闭IIC与相关IO
            I2Cx_Deinit(I2C0);
            CloseIO(GPIOA, GPIO_Pin_14);
            CloseIO(GPIOA, GPIO_Pin_15);

            BSTIM_CR1_CEN_Setable(DISABLE);
            CMU_PERCLK_SetableEx(BSTIMCLK, DISABLE);

            lpmSleepTimeInit();
        __sleep:
            displayFlag = 1;
            device_t.rtcWakeEvent = 0;
            device_t.touchWakeEvent = 0;
            device_t.cardWakeEvent = 0;
            device_t.fingerWakeEvent = 0;
            device_t.tamperWakeEvent = 0;
            device_t.isTouchInit = 0;
            device_t.loraWakeEvent = 0;
#if FUNC_FINGERPRINT_ENABLE
            GPIO_EXTI_Init(FINGER_DETECT_PORT, FINGER_DETECT_PIN, EXTI_RISING, ENABLE);
#endif
            GPIO_EXTI_Init(CARD_IRQ_PORT, CARD_IRQ_PIN, EXTI_FALLING, ENABLE);
            GPIO_EXTI_Init(TCH_IRQ_PORT, TCH_IRQ_PIN, EXTI_FALLING, ENABLE);

            GPIO_Sleep_Config();
            __enable_irq();

            PMU_SleepCfg();
            IWDT_Clr();
            __WFI(); // 进入休眠

            // 唤醒后重新初始化各外设
            __disable_irq();
            Init_SysClk_Gen();

#if FUNC_FINGERPRINT_ENABLE
            GPIO_EXTI_Close(FINGER_DETECT_PORT, FINGER_DETECT_PIN);
#endif

            Uartx_Init(UART0, 115200); // 串口打印初始化
            Spi1Init();
            sysTimeSwitchToRtcTime();
            IWDT_Clr();

            if (device_t.lptWakeEvent) {
                displayFlag = 0;
                device_t.lptWakeEvent = 0;
                IWDT_Clr();
                // DEBUG_TRACE(LOG_TAG, "Device Wake Up From LPT");
#if (FSV9522_CARD_ENABLE)
#if !LOCAL_LOCK_ENABLE
                if (Timestamp > dataReportTimestamp || (Timestamp > reconnect_stamp && !LoRaWAN.joinState)) {
                    DEBUG_TRACE(LOG_TAG, "Reconnect...");
#else
                if (0) {
                    DEBUG_TRACE(LOG_TAG, "Reconnect...");
#endif
#else
#if !LOCAL_LOCK_ENABLE
                if (singleCardRead() || Timestamp > dataReportTimestamp || (Timestamp > reconnect_stamp && !LoRaWAN.joinState)) {
#else
                if (singleCardRead()) {
                    DEBUG_TRACE(LOG_TAG, "single Read Card Success");
#endif
#endif
                } else
                    goto __sleep;
            } else if (device_t.rtcWakeEvent) {
                displayFlag = 0;
                device_t.rtcWakeEvent = 0; // 周期唤醒时间未到时,立马进入睡眠
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From RTC");
#if LOCAL_LOCK_ENABLE
                lpmSleepTimeInit();
                goto __sleep;
#else
                if (Timestamp >= dataReportTimestamp || (Timestamp >= reconnect_stamp && !LoRaWAN.joinState)) {
                    // DEBUG_TRACE(LOG_TAG, "Device Wake Up Action Event");
                } else {
                    lpmSleepTimeInit();
                    goto __sleep;
                }
#endif
            } else if (device_t.touchWakeEvent) {
                device_t.touchWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From Touch");
            } else if (device_t.cardWakeEvent) {
                device_t.cardWakeEvent = 0;
#if (FSV9522_CARD_ENABLE)
                Clear_lpcd_irq();
                fsv9522_LPCD_Stop();
#endif
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From Card");
            } else if (device_t.fingerWakeEvent) {
                device_t.fingerWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From Finger");
            } else if (device_t.tamperWakeEvent) {
                device_t.tamperWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From Tamper Key");
            } else if (device_t.rstKeyWakeEvent) {
                device_t.rstKeyWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From Rst Key");
            } else if (device_t.loraWakeEvent) {
                displayFlag = 0;
                device_t.loraWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From LoRa Mode");
            }

            BSTIM_Init(1000, RCHFCLKCFG); // 定时1ms中断
#if FUNC_BT_ENABLE
            Uartx_Init(UART5, 115200); // 蓝牙串口初始化
#endif
#if FUNC_FINGERPRINT_ENABLE
            Uartx_Init(UART2, 57600); // 指纹串口初始化
#endif
#if FUNC_LORA_VERSION_LIR
            LPUART0_Init();
#else
            Uartx_Init(UART3, 115200);
#endif
            App_PortCfg();
            I2C_Init(I2C0);
            SpiInit();

            __enable_irq();
#if FUNC_FINGERPRINT_ENABLE
            FINGER_POWER_H;
#endif

            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            device_t.sleepState = 0;

            if (displayFlag) {
                MBI5120_OE_L;
                MBI5120_Display(0xFFFF);
                tx_thread_sleep(20);
                memset(&s_touch, 0, sizeof(s_touch));
#if FUNC_FINGERPRINT_ENABLE
                tx_event_flags_set(&zs_display_event_group, FINGER_BIT_DISPLAY_BLUE, TX_OR);
                // finger_led_control(3, 1, 1, 0); // 蓝色常亮
#endif
            }
            // 置事件位为运行状态
            tx_event_flags_get(&main_event_group, MAIN_BIT_SLEEP, TX_OR_CLEAR, &actual_events, TX_NO_WAIT);
            tx_event_flags_set(&main_event_group, MAIN_BIT_RUN, TX_OR);
        }
    }
#endif
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
