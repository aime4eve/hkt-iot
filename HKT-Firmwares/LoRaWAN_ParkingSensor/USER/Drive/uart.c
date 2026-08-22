
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "adc.h"
#include "control_center.h"
#include "gpio.h"
#include "uart.h"

struct uart uartLoRa;
struct uart uartInfo;
struct uart uartBle;
struct uart uartRd;

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
        // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        ledRedBlinkTime = 50;
    }
    if (LL_USART_IsActiveFlag_PE(USART3)) {
        LL_USART_ClearFlag_PE(USART3);
    }
    if (LL_USART_IsActiveFlag_ORE(USART3)) {
        LL_USART_ClearFlag_ORE(USART3);
    }
}

/**
 * @brief  失能异步串口2收发器
 * @param
 * @retval
 */
void App_Uart3_Deinit(void)
{
    LL_USART_DeInit(USART3);
    LL_USART_Disable(USART3);
    LL_AHB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART3);
    NVIC_DisableIRQ(USART3_IRQn);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4 | LL_GPIO_PIN_5;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);
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
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* USART3 interrupt Init */
    NVIC_SetPriority(USART3_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 0));
    NVIC_EnableIRQ(USART3_IRQn);

    /* USER CODE END USART3_Init 1 */
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
 * @brief  串口2接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART2_IRQHandler(void)
{
#if 1

    if (LL_USART_IsActiveFlag_IDLE(USART2)) {
        LL_USART_ClearFlag_IDLE(USART2);
        uartRd.recvLen = uart_recv_max_len - (LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6));
        uartRd.recvTimeout = 20; // 20ms超时
        ledRedBlinkTime = 50;

        // 重新配置 DMA
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, uart_recv_max_len);
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)uartRd.recvData);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
    }

#else
    if (LL_USART_IsActiveFlag_RXNE(USART2)) // 检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART2); // 读取出来接收到的数据
        if (uartRd.recvLen < uart_recv_max_len) {
            uartRd.recvData[uartRd.recvLen++] = DataReceived;
            uartRd.recvTimeout = 20; // 20ms超时
        }
        // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        ledRedBlinkTime = 50;
    }
#endif

    if (LL_USART_IsActiveFlag_PE(USART2)) {
        LL_USART_ClearFlag_PE(USART2);
    }
}

/**
 * @brief  失能异步串口2收发器
 * @param
 * @retval
 */
void App_Uart2_Deinit(void)
{
#if 1
    // 先确保 DMA 传输完成或暂停
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);
#endif

    LL_USART_DeInit(USART2);
    LL_USART_Disable(USART2);
    LL_USART_DisableIT_RXNE(USART2);
    LL_USART_DisableIT_PE(USART2);
    LL_AHB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_USART2);
    NVIC_DisableIRQ(USART2_IRQn);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_2 | LL_GPIO_PIN_3;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief  异步串口2收发器初始化，波特率57600，用于雷达模块通讯
 * @param
 * @retval
 */
void App_Uart2Cfg(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    /**USART2 GPIO Configuration
    PA2   ------> USART2_TX
    PA3   ------> USART2_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_2;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_3;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

#if 1
    /* DMA controller clock enable */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_DMA_SetPeriphRequest(DMA1, LL_DMA_CHANNEL_6, LL_DMA_REQUEST_2);
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)uartRd.recvData); // 接收目标地址
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)&(USART2->RDR)); // 设置外设地址，从哪里搬运数据
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6); // 使能通道
    LL_USART_EnableDMAReq_RX(USART2); // 使能串口DMA接收

    /* DMA interrupt init */
    /* DMA1_Channel6_IRQn interrupt configuration */
    NVIC_SetPriority(DMA1_Channel6_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(DMA1_Channel6_IRQn);
#endif

    /* USART2 interrupt Init */
    NVIC_SetPriority(USART2_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(USART2_IRQn);

    // USART_InitStruct.BaudRate = 57600;
    USART_InitStruct.BaudRate = 115200;
    USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;

    LL_USART_Init(USART2, &USART_InitStruct);
    LL_USART_ConfigAsyncMode(USART2);
    LL_USART_DisableOverrunDetect(USART2);
    LL_USART_DisableDMADeactOnRxErr(USART2);
    LL_USART_Enable(USART2);

    LL_USART_ClearFlag_PE(USART2);
    LL_USART_ClearFlag_CM(USART2);
    LL_USART_ClearFlag_EOB(USART2);

    LL_USART_EnableIT_RXNE(USART2);
#if 1
    LL_USART_EnableIT_IDLE(USART2);
#endif
    LL_USART_EnableIT_PE(USART2);
}

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1)) // 检测是否接收中断
    {
        u8 DataReceived = LL_USART_ReceiveData8(USART1); // 读取出来接收到的数据
        if (uartLoRa.recvLen < uart_recv_max_len) {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 20; // 20ms超时
        }
        // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        ledRedBlinkTime = 50;
    }
    if (LL_USART_IsActiveFlag_PE(USART1)) {
        LL_USART_ClearFlag_PE(USART1);
    }
}

/**
 * @brief  异步串口1收发器初始化，波特率9600，用与LoRaWAN模组通讯
 * @param
 * @retval
 */
void App_Uart1Cfg(void)
{
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    /**USART1 GPIO Configuration
    PA9   ------> USART1_TX
    PA10   ------> USART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_9 | LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 1));
    NVIC_EnableIRQ(USART1_IRQn);

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
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void LPUART1_IRQHandler(void)
{
    if (LL_LPUART_IsActiveFlag_RXNE(LPUART1)) // 检测是否接收中断
    {
        u8 DataReceived = LL_LPUART_ReceiveData8(LPUART1); // 读取出来接收到的数据
        if (uartBle.recvLen < uart_recv_max_len - 1) {
            uartBle.recvData[uartBle.recvLen++] = DataReceived;
            uartBle.recvTimeout = 50; // 50ms超时
        }
        // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
        //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
        ledRedBlinkTime = 50;
    }
    if (LL_LPUART_IsActiveFlag_PE(LPUART1)) {
        LL_LPUART_ClearFlag_PE(LPUART1);
    }
    if (LL_LPUART_IsActiveFlag_ORE(LPUART1)) {
        LL_LPUART_ClearFlag_ORE(LPUART1);
    }
}

void App_LpUart1DeInit(void)
{
    LL_LPUART_DeInit(LPUART1);
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    /**LPUART1 GPIO Configuration
    PB10   ------> LPUART1_TX
    PB11   ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/**
 * @brief  低功耗串口1收发器初始化，波特率115200，用于蓝牙芯片通讯
 * @param
 * @retval
 */
void App_LpUart1Cfg(void)
{
    LL_LPUART_InitTypeDef LPUART_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_LPUART1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /**LPUART1 GPIO Configuration
    PB10   ------> LPUART1_TX
    PB11   ------> LPUART1_RX
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10 | LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_8;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    // GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    // LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(LPUART1_IRQn);

    LPUART_InitStruct.BaudRate = 115200;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);

    LL_LPUART_EnableIT_RXNE(LPUART1);
    LL_LPUART_EnableIT_PE(LPUART1);
    LL_LPUART_ClearFlag_PE(LPUART1);

    // 使能LPUART1
    LL_LPUART_Enable(LPUART1);
    // 查询LPUART1初始化
    while (!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1)))) {
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
    while (TxCounter < buffer_size) {
        /* Send one byte */
        LL_USART_TransmitData8(USART1, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART1))
            ;
    }
    ADC_BatterySglConvert(); // 每发射一次，电池容量减一
    // device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  通过USART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void LoRa_SendCMD(const char *cmd) { LoRa_SendData((u8 *)cmd, strlen(cmd)); }

/**
 * @brief  通过USART2外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void RD_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte */
        LL_USART_TransmitData8(USART2, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART2))
            ;
    }
}

/**
 * @brief  通过USART2外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void RD_SendCMD(const char *cmd) { RD_SendData((u8 *)cmd, strlen(cmd)); }

/**
 * @brief  通过LPUART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void BLE_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size) {
        // INFO("%02x ",buffptr[TxCounter]);
        /* Send one byte */
        LL_LPUART_TransmitData8(LPUART1, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_LPUART_IsActiveFlag_TC(LPUART1))
            ;
    }
    // if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY)
    //     device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    // batteryInitialCapacity++; // 按发包次数减少电量信息
}

/**
 * @brief  通过LPUART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void BLE_SendCMD(const char *cmd) { BLE_SendData((u8 *)cmd, strlen(cmd)); }

/**
 * @brief  通过USART3外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from USART2 to USARTx */
        LL_USART_TransmitData8(USART3, buffptr[TxCounter++]);

        /* Loop until register is empty */
        while (!LL_USART_IsActiveFlag_TC(USART3))
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
    static unsigned char s[512]; // This needs to be large enough to store the string TODO Change magic number
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
