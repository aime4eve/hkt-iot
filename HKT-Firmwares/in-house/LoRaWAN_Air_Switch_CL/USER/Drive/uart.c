
#include "uart.h"
#include "LoRaWAN_APPLY.h"
#include "gpio.h"
#include "systick.h"

struct uart uartLoRa;
struct uart uartAS;
struct uart uartInfo;
uint8_t dma_buf[200];

#if FUNC_BOARD_VERSION_CL

/**
 * @brief  串口2接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART2_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART2)) {            // 检测是否接收中断
        u8 DataReceived = LL_USART_ReceiveData8(USART2); // 读取出来接收到的数据
        if (uartInfo.recvLen < uart_recv_max_len) {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 20; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_ORE(USART2)) {
        LL_USART_ClearFlag_ORE(USART2);
    }
    if (LL_USART_IsActiveFlag_PE(USART2)) {
        LL_USART_ClearFlag_PE(USART2);
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
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
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
    NVIC_SetPriority(USART2_IRQn, 1);
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
    LL_USART_DisableOverrunDetect(USART2);
    LL_USART_DisableDMADeactOnRxErr(USART2);
    LL_USART_Enable(USART2);

    LL_USART_EnableIT_RXNE(USART2);
    LL_USART_EnableIT_PE(USART2);
    LL_USART_ClearFlag_PE(USART2);
}

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART1_IRQHandler(void)
{
    if (LL_USART_IsActiveFlag_RXNE(USART1)) {            // 检测是否接收中断
        u8 DataReceived = LL_USART_ReceiveData8(USART1); // 读取出来接收到的数据
        if (uartLoRa.recvLen < uart_recv_max_len) {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 50; // 20ms超时
        }
    }
    if (LL_USART_IsActiveFlag_ORE(USART1)) {
        LL_USART_ClearFlag_ORE(USART1);
    }
    if (LL_USART_IsActiveFlag_PE(USART1)) {
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
    GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    NVIC_SetPriority(USART1_IRQn, 1);
    NVIC_EnableIRQ(USART1_IRQn);

    USART_InitStruct.BaudRate = 115200;
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
    if (LL_LPUART_IsActiveFlag_WKUP(LPUART1) && LL_LPUART_IsEnabledIT_WKUP(LPUART1)) {
        /* Configure LPUART1 transfer interrupts : */
        /* Disable the UART Wake UP from stop mode Interrupt */
        LL_LPUART_DisableIT_WKUP(LPUART1);
        /* WUF flag clearing */
        LL_LPUART_ClearFlag_WKUP(LPUART1);

        u8 DataReceived = LL_LPUART_ReceiveData8(LPUART1); // 读取出来接收到的数据
        if (uartAS.recvLen < uart_recv_max_len) {
            uartAS.recvData[uartAS.recvLen++] = DataReceived;
            uartAS.recvTimeout = 50; // 50ms超时
        }
    }

    if (LL_LPUART_IsActiveFlag_RXNE(LPUART1)) {            // 检测是否接收中断
        u8 DataReceived = LL_LPUART_ReceiveData8(LPUART1); // 读取出来接收到的数据
        if (uartAS.recvLen < uart_recv_max_len) {
            uartAS.recvData[uartAS.recvLen++] = DataReceived;
            uartAS.recvTimeout = 50; // 50ms超时
        }
    }

    if (LL_LPUART_IsActiveFlag_PE(LPUART1)) {
        LL_LPUART_ClearFlag_PE(LPUART1);
    }
    if (LL_LPUART_IsActiveFlag_ORE(LPUART1)) {
        LL_LPUART_ClearFlag_ORE(LPUART1);
    }
}

/**
 * @brief  空开通讯串口，波特率19200
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
    GPIO_InitStruct.Pin = LL_GPIO_PIN_10;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_11;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_4;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* LPUART1 interrupt Init */
    NVIC_SetPriority(LPUART1_IRQn, 0);
    NVIC_EnableIRQ(LPUART1_IRQn);

    LPUART_InitStruct.BaudRate = 19200;
    LPUART_InitStruct.DataWidth = LL_LPUART_DATAWIDTH_8B;
    LPUART_InitStruct.StopBits = LL_LPUART_STOPBITS_1;
    LPUART_InitStruct.Parity = LL_LPUART_PARITY_NONE;
    LPUART_InitStruct.TransferDirection = LL_LPUART_DIRECTION_TX_RX;
    LPUART_InitStruct.HardwareFlowControl = LL_LPUART_HWCONTROL_NONE;
    LL_LPUART_Init(LPUART1, &LPUART_InitStruct);

    LL_LPUART_SetWKUPType(LPUART1, LL_LPUART_WAKEUP_ON_STARTBIT);
    LL_LPUART_EnableIT_RXNE(LPUART1);
    LL_LPUART_EnableIT_PE(LPUART1);

    LL_LPUART_ClearFlag_PE(LPUART1);

    /* 使能LPUART1 */
    LL_LPUART_Enable(LPUART1);
    /* 等待 LPUART1 发送和接收就绪 */
    while (!(LL_LPUART_IsActiveFlag_TEACK(LPUART1)) || (!(LL_LPUART_IsActiveFlag_REACK(LPUART1)))) {
    }
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* 等待 TDR 为空后再写入下一字节，比 TC 轮询吞吐量更高 */
        while (!LL_USART_IsActiveFlag_TXE(USART1))
            ;
        LL_USART_TransmitData8(USART1, buffptr[TxCounter++]);
    }
    /* 最后一字节发送完成后等待 TC，确保数据完全移出移位寄存器 */
    while (!LL_USART_IsActiveFlag_TC(USART1))
        ;
}

/**
 * @brief  通过USART外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void LoRa_SendCMD(const char *cmd)
{
    LoRa_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void AS_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size) {
        // INFO("%02x ",buffptr[TxCounter]);
        /* 等待 TDR 为空后再写入下一字节 */
        while (!LL_LPUART_IsActiveFlag_TXE(LPUART1))
            ;
        LL_LPUART_TransmitData8(LPUART1, buffptr[TxCounter++]);
    }
    /* 最后一字节发送完成后等待 TC */
    while (!LL_LPUART_IsActiveFlag_TC(LPUART1))
        ;
}

/**
 * @brief  通过USART外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void AS_SendCMD(const char *cmd)
{
    DEBUG_TRACE(LOG_TAG, "Send BT Data: %s", cmd);
    AS_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* 等待 TDR 为空后再写入下一字节，比 TC 轮询吞吐量更高 */
        while (!LL_USART_IsActiveFlag_TXE(USART2))
            ;
        LL_USART_TransmitData8(USART2, buffptr[TxCounter++]);
    }
    /* 最后一字节发送完成后等待 TC，确保数据完全移出移位寄存器 */
    while (!LL_USART_IsActiveFlag_TC(USART2))
        ;
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    /* 等待 TDR 空闲后再写入，避免覆盖未发送完的数据 */
    while (!LL_USART_IsActiveFlag_TXE(USART2))
        ;
    LL_USART_TransmitData8(USART2, (uint8_t)ch);

    /* 等待当前字节完全发送完成 */
    while (!LL_USART_IsActiveFlag_TC(USART2))
        ;
    return ch;
}

#else

FlagStatus uart0_transfer_complete;
FlagStatus uart1_transfer_complete;
FlagStatus uart2_transfer_complete;

/**
 * @brief  串口0接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART0_IRQHandler(void)
{
    // if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_RBNE))
    // {
    //     /* clear IDLE flag */
    //     u8 DataReceived = usart_data_receive(USART0); //读取出来接收到的数据
    //     if (uartLoRa.recvLen < uart_recv_max_len - 1)
    //     {
    //         uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
    //         uartLoRa.recvTimeout = 100; // 100ms超时
    //     }
    // }

    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)) {
        usart_interrupt_flag_clear(USART0, USART_FLAG_IDLEF);
        usart_data_receive(USART0);
        uartLoRa.recvTimeout = 20; // 20ms超时
    }
    /* number of data received */
    uint8_t recv_len = uart_recv_max_len - (dma_transfer_number_get(DMA0, DMA_CH4));
    memcpy(uartLoRa.recvData + uartLoRa.recvLen, dma_buf, recv_len);
    uartLoRa.recvLen += recv_len;

    /* disable DMA and reconfigure */
    dma_channel_disable(DMA0, DMA_CH4);
    dma_transfer_number_config(DMA0, DMA_CH4, uart_recv_max_len);
    dma_channel_enable(DMA0, DMA_CH4);
}

/**
 * @brief  串口1接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART1_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_RBNE)) {
        /* clear IDLE flag */
        u8 DataReceived = usart_data_receive(USART1); // 读取出来接收到的数据
        if (uartAS.recvLen < uart_recv_max_len) {
            uartAS.recvData[uartAS.recvLen++] = DataReceived;
            uartAS.recvTimeout = 50; // 50ms超时
        }
    }
}

/**
 * @brief  串口2接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART2_IRQHandler(void)
{
    if (RESET != usart_interrupt_flag_get(USART2, USART_INT_FLAG_RBNE)) {
        /* clear IDLE flag */
        u8 DataReceived = usart_data_receive(USART2); // 读取出来接收到的数据
        if (uartInfo.recvLen < uart_recv_max_len) {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 50; // 50ms超时
        }
    }
}

/**
 * @brief  LoRaWAN模组通讯串口，波特率9600
 * @param
 * @retval
 */
void App_Uart0Cfg(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART0);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /* connect port to USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);

    dma_parameter_struct dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA0);

    dma_deinit(DMA0, DMA_CH4);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)dma_buf;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = uart_recv_max_len;
    dma_init_struct.periph_addr = ((uint32_t)&USART_DATA(USART0));
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH4, &dma_init_struct);

    dma_circulation_disable(DMA0, DMA_CH4);
    dma_channel_enable(DMA0, DMA_CH4);

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, 9600U);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART0, USART_DENR_ENABLE);
    // usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
    usart_enable(USART0);
    nvic_irq_enable(USART0_IRQn, 0, 0);

    usart_flag_clear(USART0, USART_FLAG_IDLEF);
    usart_flag_clear(USART0, USART_FLAG_RBNE);
    usart_flag_clear(USART0, USART_FLAG_IDLEF);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
    // usart_interrupt_enable(USART0, USART_INT_RBNE);
}

/**
 * @brief  空开通讯串口，波特率19200
 * @param
 * @retval
 */
void App_Uart1Cfg(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART1);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    /* connect port to USARTx_Rx */
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

    /* USART configure */
    usart_deinit(USART1);
    usart_baudrate_set(USART1, 19200U);
    usart_receive_config(USART1, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
    usart_enable(USART1);
    nvic_irq_enable(USART1_IRQn, 1, 1);

    usart_flag_clear(USART1, USART_FLAG_IDLEF);
    usart_flag_clear(USART1, USART_FLAG_RBNE);
    usart_interrupt_enable(USART1, USART_INT_RBNE);
}

/**
 * @brief  打印服务串口，波特率115200
 * @param
 * @retval
 */
void App_Uart2Cfg(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOB);
    /* enable USART clock */
    rcu_periph_clock_enable(RCU_USART2);

    /* connect port to USARTx_Tx */
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);
    /* connect port to USARTx_Rx */
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11);

    /* USART configure */
    usart_deinit(USART2);
    usart_baudrate_set(USART2, 115200U);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
    usart_enable(USART2);
    nvic_irq_enable(USART2_IRQn, 2, 2);

    usart_flag_clear(USART2, USART_FLAG_IDLEF);
    usart_flag_clear(USART2, USART_FLAG_RBNE);
    usart_interrupt_enable(USART2, USART_INT_RBNE);
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Send LoRa Data Len : %d, ", buffer_size);
#endif
    while (TxCounter < buffer_size) {
#if HIGH_LEVEL_DEBUG_ENABLE
        INFO("%02x ", buffptr[TxCounter]);
#endif
        /* Send one byte */
        usart_data_transmit(USART0, buffptr[TxCounter++]);
        while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
            ;
    }
}

/**
 * @brief  通过USART外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void LoRa_SendCMD(const char *cmd)
{
    LoRa_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void AS_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Send AS Data Len : %d, ", buffer_size);
#endif
    while (TxCounter < buffer_size) {
#if HIGH_LEVEL_DEBUG_ENABLE
        INFO("%02x ", buffptr[TxCounter]);
#endif
        /* Send one byte */
        usart_data_transmit(USART1, buffptr[TxCounter++]);
        while (RESET == usart_flag_get(USART1, USART_FLAG_TBE))
            ;
    }
    while (RESET == usart_flag_get(USART1, USART_FLAG_TC))
        ;
}

/**
 * @brief  通过USART外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void AS_SendCMD(const char *cmd)
{
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Send AS Data: %s", cmd);
#endif
    AS_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        usart_data_transmit(USART2, buffptr[TxCounter++]);
        while (RESET == usart_flag_get(USART2, USART_FLAG_TBE))
            ;
    }
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
    usart_data_transmit(USART2, (uint8_t)ch);
    while (RESET == usart_flag_get(USART2, USART_FLAG_TBE))
        ;
    return ch;
}

#endif

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
