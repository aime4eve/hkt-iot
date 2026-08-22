
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "flash.h"
#include "gpio.h"
#include "rtc.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

const u8 SYNC_HEAD[3] = {0x68, 0x6B, 0x74};
u8 lwrb_write_count;
u8 lwrb_data[1024];
lwrb_t lwrbBuff_t;

u8 sendNum;
u8 sendBuffer[100];

u8 packSyncNumber;
u8 send_failcnt;

struct communicate device_t;

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
    case DATATYPE_DEVICE_TEMPERATURE_INFO: {
        int temp = device_t.temperature * 1000;
        if (temp < 0)
            temp = abs(temp) | 0x800000;
        dest[1] = (temp >> 16) & 0xFF;
        dest[2] = (temp >> 8) & 0xFF;
        dest[3] = temp & 0xFF;
        dataLen += 3;
    } break;
    case DATATYPE_DEVICE_SMOKE_ALARM: // 烟雾报警 0报警恢复 1触发烟雾报警
        dest[1] = device_t.alarm_state;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_TEMPERATURE_ALARM: // 温度报警 0温度正常 1温度异常
        dest[1] = device_t.temp_state;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_SYNC_BATTERY_STATE: // 电量信息
        dest[1] = device_t.battery_state;
        dataLen += 1;
        break;
    case DATATYPE_DEVICE_TAMPER_STATE: // 防拆报警
        dest[1] = device_t.tamper_state;
        dataLen += 1;
        break;

    case DATATYPE_DEVICE_SYNC_PERIOD: // 设备数据同步周期 取值范围：1- 1440 （1分钟到24小时），默认8小时。
        dest[1] = (u8)(device_t.report_interval >> 8);
        dest[2] = device_t.report_interval & 0xFF;
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
    delay_1ms(20);
    memcpy(uartLoRa.sendData, SYNC_HEAD, 3);
    uartLoRa.sendData[3] = 0;
    uartLoRa.sendData[4] = packSyncNumber++;

    uartLoRa.sendLen = sendNum;
    memcpy(uartLoRa.sendData + 5, sendBuffer, uartLoRa.sendLen);
    uartLoRa.sendLen += 5;

    LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
    DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
    WDT_Restart();

    if (getLoRaWANBusyStat()) {
        DEBUG_TRACE(WARN_TAG, "Wait Busy IO Timeout");
        goto __send_fail;
    }
    if (getLoRaWANSendStat()) {
    __send_fail:
        DEBUG_TRACE(WARN_TAG, "LoRa SendData Fail, Write Date To Cache");
        lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
        lwrb_write_count++;
        send_failcnt++;
        if (send_failcnt >= 5) {
            send_failcnt = 0;
            LoRaWAN.joinState = 0;
            LoRaWAN.status = LoRaWAN_STATUS_NET_ERROR;
            LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
            reconnect_stamp = Timestamp + 30 * 60;
            DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        }
        return;
    }
    send_failcnt = 0;
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 3 * SEC_DELAY;
}

/**
 * @brief  校验同步头
 * @param  data：接收到数据的数组
 * @retval  检验状态 1：成功 0：失败
 */
bool checkDataSyncHead(u8 *data)
{
    bool status = SUCCESS;
    if (memcmp(data, SYNC_HEAD, 3) != 0)
        status = FALSE;
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
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);
        DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
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
        case DATATYPE_DEVICE_SYNC_PERIOD: {
            u16 reportInterval = srcHex[1] << 8 | srcHex[2];
            if ((reportInterval >= 10 && reportInterval <= 1440) || reportInterval == 0) {
                device_t.report_interval = reportInterval;
                device_t.sync_state = 1;
                Flash_Write_Report_Interval();
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval: %dmin", reportInterval);
            } else {
                DEBUG_TRACE(LOG_TAG, "Update Device Sync Interval Error: %dmin", reportInterval);
            }
            dataLen -= 3;
            srcHex += 3;
        } break;
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
        default:
            return;
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
    u8 len = 0;

    memset(sendBuffer, 0, sizeof(sendBuffer));
    sendNum = 0;

    len = setDataPackage(DATATYPE_DEVICE_HARDWARE_SOFTWARE_VERSION, dataHex);
    dataHex += len;
    sendNum += len;

    len = setDataPackage(DATATYPE_DEVICE_SYNC_BATTERY_STATE, dataHex);
    dataHex += len;
    sendNum += len;

    len = setDataPackage(DATATYPE_DEVICE_SMOKE_ALARM, dataHex); // 烟雾报警
    dataHex += len;
    sendNum += len;

    len = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_ALARM, dataHex); // 温度状态
    dataHex += len;
    sendNum += len;

    len = setDataPackage(DATATYPE_DEVICE_TEMPERATURE_INFO, dataHex); // 温度信息
    dataHex += len;
    sendNum += len;

#if FUNC_TAMPER_ENABLE
    len = setDataPackage(DATATYPE_DEVICE_TAMPER_STATE, dataHex); // 温度信息
    dataHex += len;
    sendNum += len;

#endif

    len = setDataPackage(DATATYPE_DEVICE_SYNC_PERIOD, dataHex); // 数据同步周期
    dataHex += len;
    sendNum += len;

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
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    ;
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
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        // DebugHexInfo("LoRaWAN", uartInfo.recvData, uartInfo.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart Info Data: %s", uartInfo.recvData);

        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen); // 主要用来debug
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}

/**
 * @brief  设备状态处理
 * @param
 * @retval
 */
void DeviceStateProcess(void)
{
    if (LoRaWAN.joinState && !uartLoRa.sendDelay) {
        /* 检测缓冲器是否存在待发射数据 */
        if (lwrb_write_count) // 缓冲区内有数据
        {
            uartLoRa.sendLen = 0;
            memset(uartLoRa.sendData, 0, sizeof(uartLoRa.sendData));
            memset(sendBuffer, 0, sizeof(sendBuffer));
            u8 *dataHex = sendBuffer;

            lwrb_read(&lwrbBuff_t, &sendNum, 1);
            lwrb_read(&lwrbBuff_t, dataHex, sendNum);
            lwrb_write_count--;

            sendLoRaWANData();

            if (lwrb_write_count == 0) {
                memset(lwrb_data, 0, sizeof(lwrb_data));
                lwrb_reset(&lwrbBuff_t);
            }
        } else if (device_t.sync_state) {
            device_t.sync_state = 0;
            device_t.report_interval_delay = device_t.report_interval * 60;
            Device_PeriodicReport();
        }
    }
}

/**
 * @brief
 * @param
 * @retval
 */
void DeviceSleepEventProcess(void)
{
    if (!uartLoRa.sendDelay && !device_t.sleep_delay && ((LoRaWAN.joinState && !lwrb_write_count && !device_t.sync_state) || (!LoRaWAN.joinState && LoRaWAN.status != LoRaWAN_STATUS_NORMAL))) {
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        device_t.rtc_event = 0;

        // 配置唤醒中断源
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_1);
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_6);
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_7);

#if FUNC_TAMPER_ENABLE
        EXTI_ClearWakeupFlag(EXTI_CHANNEL_0);
        if (READ_TAMPER_STATE)
            EXTI_WakeupEventConfig(EXTI_CHANNEL_0, EXTI_WAKEUP_LOW_LEVEL, ENABLE);
        else
            EXTI_WakeupEventConfig(EXTI_CHANNEL_0, EXTI_WAKEUP_HIGH_LEVEL, ENABLE);
#endif

        if (READ_AUE_STATE)
            EXTI_WakeupEventConfig(EXTI_CHANNEL_1, EXTI_WAKEUP_LOW_LEVEL, ENABLE);
        else
            EXTI_WakeupEventConfig(EXTI_CHANNEL_1, EXTI_WAKEUP_HIGH_LEVEL, ENABLE);

        if (READ_DAT1_STATE)
            EXTI_WakeupEventConfig(EXTI_CHANNEL_7, EXTI_WAKEUP_LOW_LEVEL, ENABLE);
        else
            EXTI_WakeupEventConfig(EXTI_CHANNEL_7, EXTI_WAKEUP_HIGH_LEVEL, ENABLE);

        if (READ_DAT2_STATE)
            EXTI_WakeupEventConfig(EXTI_CHANNEL_6, EXTI_WAKEUP_LOW_LEVEL, ENABLE);
        else
            EXTI_WakeupEventConfig(EXTI_CHANNEL_6, EXTI_WAKEUP_HIGH_LEVEL, ENABLE);

        EXTI_WakeupEventIntConfig(ENABLE);
        WDT_Restart();
        DEBUG_TRACE(LOG_TAG, ">>>>>>>> Device Enter Sleep, Next Wake Time %ds <<<<<<<<", wake_time);
        PWRCU_DeepSleep2(PWRCU_SLEEP_ENTRY_WFI);

        WDT_Restart();
        if (device_t.rtc_event) {
            device_t.rtc_event = 0;
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI RTC");
        } else if (device_t.dat1_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI DAT1 IO");
        } else if (device_t.dat2_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI DAT2 IO");
        } else if (device_t.aue_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI AUE IO");
        } else if (device_t.tamper_event) {
            DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI TAMPER IO");
        }
        ADC_NtcSglConvert();
        Gpio_Event();
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
