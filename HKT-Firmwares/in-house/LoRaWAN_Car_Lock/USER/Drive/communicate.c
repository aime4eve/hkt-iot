
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

u8     lwrb_write_count;
u8     lwrb_data[1024];
lwrb_t lwrbBuff_t;

u8 sendNum;
u8 sendBuffer[256];

u8            packSyncNumber;
u8            send_failcnt;
struct device device_t;

/**
 * @brief  发送填充好的数据给LoRa模块
 * @param
 * @retval
 */
void sendLoRaWANData(void)
{
    if (getLoRaWANStatus() == LoRaWAN_SLEEP_STATUS)
        setLoRaWANStatus(LoRaWAN_WAKE_STATUS);

    uartLoRa.sendLen = sendNum;
    memcpy(uartLoRa.sendData, sendBuffer, uartLoRa.sendLen);

    // 先查询入网状态 再进行数据发送
    AT_LoRaWAN_setATEnter();
    if (AT_LoRaWAN_getJOIN() == TRUE) {
        AT_LoRaWAN_setATExit();
        mDelay(20);
        LoRa_SendData(uartLoRa.sendData, uartLoRa.sendLen); // 发送数据
        DebugHexInfo("Send", uartLoRa.sendData, uartLoRa.sendLen);
        mDelay(500);
    } else {
        AT_LoRaWAN_setATExit();
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
        LoRaWAN.joinState = 0;
        LoRaWAN.status    = LoRaWAN_STATUS_NET_ERROR;
        LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_IDLE;
        reconnect_stamp   = Timestamp + 30 * 60;
        DEBUG_TRACE(LOG_TAG, "Update Reconnect Timestamp: %ld", reconnect_stamp);
        uartLoRa.sendDelay = 6 * SEC_DELAY;

        lwrb_write(&lwrbBuff_t, &uartLoRa.sendLen, 1); // 写入数据长度
        lwrb_write(&lwrbBuff_t, uartLoRa.sendData, uartLoRa.sendLen);
        lwrb_write_count++;
        return;
    }
    setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);
    DEBUG_TRACE(LOG_TAG, "LoRa SendData Success");
    uartLoRa.sendDelay = 6 * SEC_DELAY;
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
    return;
}

/**
 * @brief  处理来自LoRaWAN模组的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromLoRaWANDataHandle(u8 *data, u16 len)
{
    if (LoRaWAN.joinState) {
        BT_SendCMD((const char *)data);
        BT_SendCMD("\r\n");
    }
}

/**
 * @brief  处理来自车位锁的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromBTDataHandle(u8 *data, u16 len)
{
    memset(sendBuffer, 0, sizeof(sendBuffer)); // 将接收到的数据写入缓冲区
    if (len > 255)
        sendNum = 255;
    sendNum = len;
    memcpy(sendBuffer, data, sendNum);
    lwrb_write(&lwrbBuff_t, &sendNum, 1); // 写入数据长度
    lwrb_write(&lwrbBuff_t, sendBuffer, sendNum);
    lwrb_write_count++;
}

/**
 * @brief  处理来自debug口的数据
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    BT_SendCMD((const char *)data);
    BT_SendCMD("\r\n");
}

/**
 * @brief  处理来自串口接收到的数据
 * @param
 * @retval
 */
void fromUartDataHandle(void)
{
    if (!uartLoRa.recvTimeout && uartLoRa.recvLen) {
        // DebugHexInfo("LoRaWAN", uartLoRa.recvData, uartLoRa.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart LoRaWAN Data: %s", uartLoRa.recvData);

        fromLoRaWANDataHandle(uartLoRa.recvData, uartLoRa.recvLen);
        memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
        uartLoRa.recvLen = 0;
    }
    if (!uartBT.recvTimeout && uartBT.recvLen) {
        // DebugHexInfo("BT", uartBT.recvData, uartBT.recvLen);
        DEBUG_TRACE(LOG_TAG, "Recv Uart BT Data: %s", uartBT.recvData);

        fromBTDataHandle(uartBT.recvData, uartBT.recvLen);
        memset(uartBT.recvData, 0, sizeof(uartBT.recvData));
        uartBT.recvLen = 0;
    }
    if (!uartInfo.recvTimeout && uartInfo.recvLen) { // 来自debug口的数据
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
        }
    }
}

/**
 * @brief  系统执行低功耗模式配置
 * @param
 * @retval
 */
void systemLowPowerMode_Config(void)
{
#if FUNC_STANDARD_CLASSC_ENABLE
    return;
#endif
    if (!uartLoRa.sendDelay && !device_t.sleepDelay &&
        ((LoRaWAN.joinState && !lwrb_write_count) || (!LoRaWAN.joinState && LoRaWAN.status != LoRaWAN_STATUS_NORMAL))) {
        setLoRaWANStatus(LoRaWAN_SLEEP_STATUS);

        LL_LPUART_ClearFlag_WKUP(LPUART1);
        LL_LPUART_EnableIT_WKUP(LPUART1);
        LL_LPUART_EnableInStopMode(LPUART1);

        DEBUG_TRACE(LOG_TAG, "Device Enter Stop Mode");
        GPIO_LED_H;
        device_t.sleepState = 1;
        EnterStopMode();

        // TIM6_Init(1000, 4);
        // App_PortCfg();
        // App_Uart1Cfg();
        // App_Uart2Cfg();
        // App_LpUart1Cfg();

        if (device_t.loraWakeEvent || device_t.extiWakeEvent) {
            if (device_t.loraWakeEvent) {
                device_t.loraWakeEvent = 0;
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From LoRa");
            }
            if (device_t.extiWakeEvent) {
                DEBUG_TRACE(LOG_TAG, "Device Wake Up From EXTI");
                device_t.extiWakeEvent = 0;
            }
            if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
                device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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
