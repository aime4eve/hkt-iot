
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "gpio.h"
#include "systick.h"
#include "uart.h"

/* Console-only UART: the new BootLoader has no BLE (upgrade transport lives
 * in the APP), USART3 stays as the debug/log port. The legacy bootloader BLE
 * OTA (hkt+len+cmd+crc16+"bootloader" frames) was removed with it. */

struct uart uartInfo;

/**
 * @brief  串口3接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART3_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART3)) // 检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART3); // 读取出来接收到的数据
        if (uartInfo.recvLen < uart_recv_max_len) {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 20; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_PE(USART3)) {
        LL_USART_ClearFlag_PE(USART3);
    }
    if (LL_USART_IsActiveFlag_ORE(USART3)) {
        LL_USART_ClearFlag_ORE(USART3);
    }
}

/**
 * @brief  失能异步串口3收发器
 * @param
 * @retval
 */
void App_Uart3_Deinit(void)
{
    LL_USART_DeInit(USART3);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    NVIC_DisableIRQ(USART3_IRQn);
}

/**
 * @brief  异步串口3收发器初始化，波特率115200，默认用来打印日志信息
 * @param
 * @retval
 */
void App_Uart3Cfg(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART3_CLKSOURCE_PCLK1);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /**USART3 GPIO Configuration
    PC4   ------> USART3_TX
    PC5   ------> USART3_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 1));
    NVIC_EnableIRQ(USART3_IRQn);

    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;

    LL_USART_Init(USART3, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART3);
    LL_USART_DisableOverrunDetect(USART3);
    LL_USART_DisableDMADeactOnRxErr(USART3);
    LL_USART_Enable(USART3);

    LL_USART_EnableIT_RXNE(USART3);
    LL_USART_EnableIT_PE(USART3);
    LL_USART_ClearFlag_PE(USART3);
}

/**
 * @brief  通过USART3外设传输多个数据
 * @param  buffer:要传输的数据的字节数组
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from USART3 */
        LL_USART_TransmitData8(USART3, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART3))
            ;
    }
}

/**
 * @brief  格式化打印日志信息
 * @param  str：待发送字符串源地址
 * @retval
 */
void INFO(const char *str, ...)
{
    static unsigned char s[600];
    unsigned char *p;
    va_list args;
    int str_len;

    if ((str == NULL) || (strlen(str) == 0)) {
        return;
    }

    va_start(args, str);
    str_len = (unsigned int)vsprintf((char *)s, str, args);
    va_end(args);
    p = s;
    INFO_SendData(p, str_len);
}

void DebugHexInfo(char *tag, u8 *data, int len)
{
    INFO("\nUart Data [%s]:", tag);
    for (int i = 0; i < len; i++) {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}

/**
 * @brief  处理来自debug口的数据（仅时间同步等调试命令）
 * @param  data：接收到数据的数组
 * @param  len：data 数据长度
 * @retval
 */
void fromInfoDataHandle(u8 *data, u16 len)
{
    if (strstr((const char *)data, "syncDeviceTimestamp")) {
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
            DEBUG_TRACE(LOG_TAG, "Recv Uart Sync Time CMD,%04d-%02d-%02d-%d:%d:%d", systime.tm_year, systime.tm_mon, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
        }
    } else {
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
    if (!uartInfo.recvTimeout && uartInfo.recvLen) // 来自debug口的数据
    {
        fromInfoDataHandle(uartInfo.recvData, uartInfo.recvLen);
        memset(uartInfo.recvData, 0, sizeof(uartInfo.recvData));
        uartInfo.recvLen = 0;
    }
}
