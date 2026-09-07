
#include "uart.h"
#include "adc.h"
#include "systick.h"
#include "LoRaWAN_APPLY.h"
#include "control_center.h"

struct uart1 uartLoRa;
struct uart2 uartInfo;

/**
 * @brief  串口2接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART2_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART2)) //检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART2); //读取出来接收到的数据
        if (uartInfo.recvLen < uart2_recv_max_len)
        {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 20; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_PE(USART2))
    {
        LL_USART_ClearFlag_PE(USART2);
    }
    if (LL_USART_IsActiveFlag_ORE(USART2))
    {
        LL_USART_ClearFlag_ORE(USART2);
    }
}

/**
 * @brief  异步串口2收发器初始化，波特率115200，默认用来打印日志信息
 * @param
 * @retval
 */
void App_Uart2Cfg(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    /**USART2 GPIO Configuration
    PA2   ------> USART2_TX
    PA3   ------> USART2_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    NVIC_SetPriority(USART2_IRQn, 0);
    NVIC_EnableIRQ(USART2_IRQn);

    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART2, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_Enable(USART2);

    LL_USART_EnableIT_RXNE(USART2);
    LL_USART_EnableIT_PE(USART2);
    LL_USART_ClearFlag_PE(USART2);
}

#if 0

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1)) //检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART1); //读取出来接收到的数据
        if (uartLoRa.recvLen < uart1_recv_max_len)
        {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 100; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_PE(USART1))
    {
        LL_USART_ClearFlag_PE(USART1);
    }
}

/**
 * @brief  异步串口1收发器初始化，波特率9600，默认用来与设备之间通讯
 * @param
 * @retval
 */
void App_Uart1Cfg(void)
{

    LL_USART_InitTypeDef USART_InitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);

    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
    /**USART1 GPIO Configuration
    PA9   ------> USART1_TX
    PA10   ------> USART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    NVIC_SetPriority(USART1_IRQn, 0);
    NVIC_EnableIRQ(USART1_IRQn);

    /* USER CODE BEGIN USART1_Init 1 */

    /* USER CODE END USART1_Init 1 */
    USART_InitStruct.BaudRate = 9600;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
    LL_USART_Init(USART1, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART1);
    LL_USART_Enable(USART1);

    LL_USART_EnableIT_RXNE(USART1);
    LL_USART_EnableIT_PE(USART1);
    LL_USART_ClearFlag_PE(USART1);
}

/**
 * @brief  通过USART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size)
    {
        /* Send one byte */
        LL_USART_TransmitData8(USART1, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART1))
            ;
    }
    ADC_BatterySglConvert(); //每发射一次，电池容量减一
    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

#else

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void LPUART1_IRQHandler(void)
{
    if (LL_LPUART_IsActiveFlag_RXNE(LPUART1)) // 检测是否接收中断
    {
        u8 DataReceived = LL_LPUART_ReceiveData8(LPUART1); // 读取出来接收到的数据
        if (uartLoRa.recvLen < uart1_recv_max_len)
        {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 50;                              // 50ms超时
            if (uartLoRa.recvLen == 1 && uartLoRa.recvData[0] == 0) // 第一个字节接收到00丢掉
            {
                uartLoRa.recvLen--;
            }
        }
    }
    if (LL_LPUART_IsActiveFlag_PE(LPUART1))
    {
        LL_LPUART_ClearFlag_PE(LPUART1);
    }
    if (LL_LPUART_IsActiveFlag_ORE(LPUART1))
    {
        LL_LPUART_ClearFlag_ORE(LPUART1);
    }
}

/**
 * @brief  异步串口1收发器初始化，波特率9600，默认用来与设备之间通讯
 * @param
 * @retval
 */
void App_LpUart1Cfg(void)
{
    LL_LPUART_InitTypeDef LPUART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPUART1);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
    /**LPUART1 GPIO Configuration
    PB10   ------> LPUART1_TX
    PB11   ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_LoRaWAN_TX_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIO_LoRaWAN_TX_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_LoRaWAN_RX_PIN;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIO_LoRaWAN_RX_PORT, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, 0);
    NVIC_EnableIRQ(LPUART1_IRQn);

    LPUART_InitStruct.BaudRate = 9600;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);

    LL_LPUART_EnableIT_RXNE(LPUART1);
    LL_LPUART_EnableIT_PE(LPUART1);
    LL_LPUART_ClearFlag_PE(USART1);

    LL_LPUART_EnableIT_RXNE(LPUART1); // 启用 检测到接收非空 中断

    // 使能LPUART1
    LL_LPUART_Enable(LPUART1);
    // 查询LPUART1初始化
    while (!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1))))
    {
    }
}

/**
 * @brief  通过USART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    ADC_BatterySglConvert();
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size)
    {
        // INFO("%02x ",buffptr[TxCounter]);
        /* Send one byte */
        LL_LPUART_TransmitData8(LPUART1, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!LL_LPUART_IsActiveFlag_TC(LPUART1))
            ;
    }
}
#endif

/**
 * @brief  通过USART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void LoRa_SendCMD(const char *cmd)
{
    LoRa_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART2外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size)
    {
        /* Send one byte from USART2 to USARTx */
        LL_USART_TransmitData8(USART2, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART2))
            ;
    }
}

/**
 * @brief  格式化打印日志信息，打印字符串最大限制长度在600字节以内
 * @param  str：待发送字符串源地址
 * @retval
 */
void INFO(const char *str, ...)
{
    static unsigned char s[500]; // This needs to be large enough to store the string TODO Change magic number
    unsigned char *p;
    va_list args;
    int str_len;

    if ((str == NULL) || (strlen(str) == 0))
    {
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
    INFO("\nUart Data [%s]: %d, ", tag, len);
    for (int i = 0; i < len; i++)
    {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}
