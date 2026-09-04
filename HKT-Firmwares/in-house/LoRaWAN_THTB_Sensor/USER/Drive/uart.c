
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "uart.h"
#include "adc.h"
#include "control_center.h"
#include "gpio.h"

#include <LoRaWAN_APPLY.h>

struct uart uartLoRa;
struct uart uartInfo;
struct uart_gps uartGps;

u8 t_uart1, t_uart2, t_lpuart1;

/**
 * @brief UART MSP Initialization
 * This function configures the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (huart->Instance == LPUART1)
    {
        /* USER CODE BEGIN LPUART1_MspInit 0 */

        /* USER CODE END LPUART1_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_LPUART1_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**LPUART1 GPIO Configuration
        PB10     ------> LPUART1_TX
        PB11     ------> LPUART1_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_LPUART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* LPUART1 interrupt Init */
        HAL_NVIC_SetPriority(LPUART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);
        /* USER CODE BEGIN LPUART1_MspInit 1 */

        /* USER CODE END LPUART1_MspInit 1 */
    }
    else if (huart->Instance == USART1)
    {
        /* USER CODE BEGIN USART1_MspInit 0 */

        /* USER CODE END USART1_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART1 interrupt Init */
        HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspInit 1 */

        /* USER CODE END USART1_MspInit 1 */
    }
    else if (huart->Instance == USART2)
    {
        /* USER CODE BEGIN USART2_MspInit 0 */

        /* USER CODE END USART2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**USART2 GPIO Configuration
        PA2     ------> USART2_TX
        PA3     ------> USART2_RX
        */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* USART2 interrupt Init */
        HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspInit 1 */

        /* USER CODE END USART2_MspInit 1 */
    }
}

/**
 * @brief UART MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1)
    {
        /* USER CODE BEGIN LPUART1_MspDeInit 0 */

        /* USER CODE END LPUART1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_LPUART1_CLK_DISABLE();

        /**LPUART1 GPIO Configuration
        PB10     ------> LPUART1_TX
        PB11     ------> LPUART1_RX
        */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10 | GPIO_PIN_11);

        /* LPUART1 interrupt DeInit */
        HAL_NVIC_DisableIRQ(LPUART1_IRQn);
        /* USER CODE BEGIN LPUART1_MspDeInit 1 */

        /* USER CODE END LPUART1_MspDeInit 1 */
    }
    else if (huart->Instance == USART1)
    {
        /* USER CODE BEGIN USART1_MspDeInit 0 */

        /* USER CODE END USART1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART1_CLK_DISABLE();

        /**USART1 GPIO Configuration
        PA9     ------> USART1_TX
        PA10     ------> USART1_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);

        /* USART1 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART1_IRQn);
        /* USER CODE BEGIN USART1_MspDeInit 1 */

        /* USER CODE END USART1_MspDeInit 1 */
    }
    else if (huart->Instance == USART2)
    {
        /* USER CODE BEGIN USART2_MspDeInit 0 */

        /* USER CODE END USART2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_USART2_CLK_DISABLE();

        /**USART2 GPIO Configuration
        PA2     ------> USART2_TX
        PA3     ------> USART2_RX
        */
        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);

        /* USART2 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART2_IRQn);
        /* USER CODE BEGIN USART2_MspDeInit 1 */

        /* USER CODE END USART2_MspDeInit 1 */
    }
}

/**
 * @brief This function handles USART1 global interrupt / USART1 wake-up interrupt through EXTI line 25.
 */
void USART1_IRQHandler(void)
{
    volatile u8 recvive;
    // HAL_UART_IRQHandler(&huart1);
    if (USART1->ISR & (1 << 5))
    {
        if (uartLoRa.recvLen < uart_recv_max_len - 1)
        {
            uartLoRa.recvData[uartLoRa.recvLen++] = USART1->RDR;
            uartLoRa.recvTimeout = 50; // 50ms超时
        }
        else
        {
            recvive = USART1->RDR;
        }
    }
    //溢出错误时（接收下一次数据前未读取数据），清除标志位
    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((USART1->ISR & USART_ISR_ORE) != 0U)
    {
        USART1->ICR = UART_CLEAR_OREF;
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief This function handles USART2 global interrupt / USART2 wake-up interrupt through EXTI line 26.
 */
void USART2_IRQHandler(void)
{
    volatile u8 recvive;
    // HAL_UART_IRQHandler(&huart2);
    if (USART2->ISR & (1 << 5))
    {
        if (uartInfo.recvLen < uart_recv_max_len - 1)
        {
            uartInfo.recvData[uartInfo.recvLen++] = USART2->RDR;
            uartInfo.recvTimeout = 50; // 50ms超时
        }
        else
        {
            recvive = USART2->RDR;
        }
    }
    //溢出错误时（接收下一次数据前未读取数据），清除标志位
    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((USART2->ISR & USART_ISR_ORE) != 0U)
    {
        USART2->ICR = UART_CLEAR_OREF;
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

//   uint32_t isrflags   = READ_REG(huart->Instance->ISR);
//   uint32_t errorflags;
//   /* If no error occurs */
//   errorflags = (isrflags & (uint32_t)(USART_ISR_PE | USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE | USART_ISR_RTOF));

/**
 * @brief This function handles LPUART1 global interrupt / LPUART1 wake-up interrupt through EXTI line 28.
 */
void LPUART1_IRQHandler(void)
{
    volatile u8 recvive;
    // HAL_UART_IRQHandler(&hlpuart1);
    //接收中断，接收数据
    if (LPUART1->ISR & (1 << 5))
    {
        if (uartGps.recvLen < 799)
        {
            uartGps.recvData[uartGps.recvLen++] = LPUART1->RDR;
            uartGps.recvTimeout = 50;
        }
        else
        {
            recvive = LPUART1->RDR;
        }
    }
    //溢出错误时（接收下一次数据前未读取数据），清除标志位
    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((LPUART1->ISR & USART_ISR_ORE) != 0U)
    {
        LPUART1->ICR = UART_CLEAR_OREF;
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *aHuart)
{
    volatile HAL_StatusTypeDef status;

    if (aHuart->Instance == LPUART1)
    {
        if (uartGps.recvLen < 1000)
        {
            uartGps.recvData[uartGps.recvLen++] = t_lpuart1;
            uartGps.recvTimeout = 100;
        }
        status = HAL_UART_Receive_IT(aHuart, &t_lpuart1, 1);
    }
    if (aHuart->Instance == USART2)
    {
        if (uartInfo.recvLen < uart_recv_max_len)
        {
            uartInfo.recvData[uartInfo.recvLen++] = t_uart2;
            uartInfo.recvTimeout = 50; // 50ms超时
        }
        status = HAL_UART_Receive_IT(aHuart, &t_uart2, 1);
    }
    if (aHuart->Instance == USART1)
    {
        if (uartLoRa.recvLen < uart_recv_max_len)
        {
            uartLoRa.recvData[uartLoRa.recvLen++] = t_uart1;
            uartLoRa.recvTimeout = 50; // 50ms超时
        }
        status = HAL_UART_Receive_IT(aHuart, &t_uart1, 1);
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT | UART_ADVFEATURE_DMADISABLEONERROR_INIT;
    huart2.AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;
    huart2.AdvancedInit.DMADisableonRxError = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    // HAL_UART_Receive_IT(&huart2, &t_uart2, 1);
    ATOMIC_SET_BIT(USART2->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 9600;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
    // HAL_UART_Receive_IT(&huart1, &t_uart1, 1);
    ATOMIC_SET_BIT(USART1->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
}

/**
 * @brief LPUART1 Initialization Function
 * @param None
 * @retval None
 */
void MX_LPUART1_UART_Init(void)
{
    hlpuart1.Instance = LPUART1;
#if FUNC_GPS_FIRM_L76K
    hlpuart1.Init.BaudRate = 9600;
#else
    hlpuart1.Init.BaudRate = 115200;
#endif
    hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
    hlpuart1.Init.StopBits = UART_STOPBITS_1;
    hlpuart1.Init.Parity = UART_PARITY_NONE;
    hlpuart1.Init.Mode = UART_MODE_TX_RX;
    hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&hlpuart1) != HAL_OK)
    {
        Error_Handler();
    }
    ATOMIC_SET_BIT(LPUART1->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);
    // HAL_UART_Receive_IT(&hlpuart1, &t_lpuart1, 1);
}

void App_LpUart1DeInit(void)
{
    HAL_UART_MspDeInit(&hlpuart1);
}

/**
 * @brief  通过USART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    HAL_UART_Transmit(&huart1, buffer, buffer_size, 1000);

    // ADC_BatterySglConvert(); //每发射一次，电池容量减一
    device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

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
 * @brief  通过LPUART1外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void Gps_SendData(uint8_t *buffer, u16 buffer_size)
{
    HAL_UART_Transmit(&hlpuart1, buffer, buffer_size, 1000);

    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  通过LPUART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void Gps_SendCMD(const char *cmd)
{
    Gps_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART2外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    HAL_UART_Transmit(&huart2, buffer, buffer_size, 1000);
}

/**
 * @brief  格式化打印日志信息，打印字符串最大限制长度在600字节以内
 * @param  str：待发送字符串源地址
 * @retval
 */
void INFO(const char *str, ...)
{
    static unsigned char s[1100]; // This needs to be large enough to store the string TODO Change magic number
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
    INFO("\n Recv Uart Data [%s]:", tag);
    for (int i = 0; i < len; i++)
    {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}
