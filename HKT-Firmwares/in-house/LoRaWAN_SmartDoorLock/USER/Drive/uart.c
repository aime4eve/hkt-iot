
#include "stdarg.h"
#include "stdio.h"
#include "string.h"

#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "control_center.h"
#include "uart.h"

uart_t uartLoRa;
uart_t uartInfo;
uart_t uartBle;
uart_t uartZs;

/**
 * @brief  串口0接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART0_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART0)) && (SET == UARTx_ISR_RXBF_Chk(UART0))) {
        u8 DataReceived = UARTx_RXBUF_Read(UART0); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
        if (uartInfo.recvLen < uart_recv_max_len) {
            uartInfo.recvData[uartInfo.recvLen++] = DataReceived;
            uartInfo.recvTimeout = 100 / SYSTEM_TIMER_BASE; // 100ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART0)) && (SET == UARTx_ISR_TXSE_Chk(UART0))) {
        // 发送中断标志可通过写TXBUF寄存器清除或TXSE写1清除
        // 发送指定长度的数据
        if (uartInfo.txIndex < uartInfo.sendLen) {
            UARTx_TXBUF_Write(UART0, uartInfo.sendData[uartInfo.txIndex]); // 发送一个数据
            uartInfo.txIndex++;
        }
        UARTx_ISR_TXSE_Clr(UART0); // 清除发送中断标志
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  串口5接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART5_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART5)) && (SET == UARTx_ISR_RXBF_Chk(UART5))) {
        u8 DataReceived = UARTx_RXBUF_Read(UART5); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
        if (uartBle.recvLen < uart_recv_max_len) {
            uartBle.recvData[uartBle.recvLen++] = DataReceived;
            uartBle.recvTimeout = 100 / SYSTEM_TIMER_BASE; // 100ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART5)) && (SET == UARTx_ISR_TXSE_Chk(UART5))) {
        if (uartBle.txIndex < uartBle.sendLen) {
            UARTx_TXBUF_Write(UART5, uartBle.sendData[uartBle.txIndex]); // 发送一个数据
            uartBle.txIndex++;
        }
        UARTx_ISR_TXSE_Clr(UART5); // 清除发送中断标志
    }
    // if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
    //     device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  串口2接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART2_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART2)) && (SET == UARTx_ISR_RXBF_Chk(UART2))) {
        u8 DataReceived = UARTx_RXBUF_Read(UART2); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
        if (uartZs.recvLen < uart_recv_max_len) {
            uartZs.recvData[uartZs.recvLen++] = DataReceived;
            uartZs.recvTimeout = 100 / SYSTEM_TIMER_BASE; // 100ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART2)) && (SET == UARTx_ISR_TXSE_Chk(UART2))) {
        UARTx_ISR_TXSE_Clr(UART2); // 清除发送中断标志
    }
    // if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
    //     device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  串口4接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART4_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART4)) && (SET == UARTx_ISR_RXBF_Chk(UART4))) {
        u8 DataReceived = UARTx_RXBUF_Read(UART4); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART4)) && (SET == UARTx_ISR_TXSE_Chk(UART4))) {
        UARTx_ISR_TXSE_Clr(UART4); // 清除发送中断标志
    }
}

/**
 * @brief  串口3接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART3_IRQHandler(void)
{
    // 接收中断处理
    if ((ENABLE == UARTx_IER_RXBF_IE_Getable(UART3)) && (SET == UARTx_ISR_RXBF_Chk(UART3))) {
        u8 DataReceived = UARTx_RXBUF_Read(UART3); // 读取出来接收到的数据,接收中断标志仅可通过读取RXBUF寄存器清除
        if (uartLoRa.recvLen < uart_recv_max_len) {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 50; // 50ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == UARTx_IER_TXSE_IE_Getable(UART3)) && (SET == UARTx_ISR_TXSE_Chk(UART3))) {
        UARTx_ISR_TXSE_Clr(UART3); // 清除发送中断标志
    }

    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  异步串口收发器初始化，默认用来打印日志信息
 * @param   UARTx: 串口号
 * @param   BaudRate: 波特率
 * @retval
 */
void Uartx_Init(UART_Type *UARTx, u32 BaudRate)
{
    UART_SInitTypeDef USART_InitStruct;
    CMU_ClocksType CMU_Clocks;
    u8 uartNumber;

    CMU_PERCLK_SetableEx(PADCLK, ENABLE);
    if (((uint32_t)UARTx - UART0_BASE) == 0)
        uartNumber = 0;
    else
        uartNumber = ((((uint32_t)UARTx - UART1_BASE) >> 10) + 1); // 获取uart

    switch (uartNumber) {
    case 0:                                                     // 进行UART0的初始化配置
        USART_InitStruct.ClockSrc = CMU_OPCCR1_UART0CKS_APBCLK; // UART0工作时钟选择
        /*UART0 IO 配置*/
        AltFunIO(GPIOF, GPIO_Pin_3, 2); // PF3 UART0 RX
        AltFunIO(GPIOF, GPIO_Pin_4, 0); // PF4 UART0 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART0_IRQn);
        NVIC_SetPriority(UART0_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART0_IRQn);
        break;
    case 1: // 进行UART1的初始化配置
        USART_InitStruct.ClockSrc = CMU_OPCCR1_UART1CKS_APBCLK;

        /*UART1 IO 配置*/
        AltFunIO(GPIOE, GPIO_Pin_3, 2); // PE3 UART1 RX
        AltFunIO(GPIOE, GPIO_Pin_4, 0); // PE4 UART1 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART1_IRQn);
        NVIC_SetPriority(UART1_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART1_IRQn);
        break;

    case 2: // 进行UART2的初始化配置
        /*UART2 IO 配置*/
        AltFunIO(GPIOB, GPIO_Pin_2, 2); // PB2 UART2 RX
        AltFunIO(GPIOB, GPIO_Pin_3, 0); // PB3 UART2 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART2_IRQn);
        NVIC_SetPriority(UART2_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART2_IRQn);
        break;
    case 3: // 进行UART3的初始化配置
        /*UART3 IO 配置*/
        AltFunIO(GPIOC, GPIO_Pin_10, 2); // PC10 UART3 RX
        AltFunIO(GPIOC, GPIO_Pin_11, 0); // PC11 UART3 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART3_IRQn);
        NVIC_SetPriority(UART3_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART3_IRQn);
        break;

    case 4: // 进行UART4的初始化配置
        /*UART4 IO 配置*/
        AltFunIO(GPIOD, GPIO_Pin_0, 2); // PD0 UART4 RX
        AltFunIO(GPIOD, GPIO_Pin_1, 0); // PD1 UART4 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART4_IRQn);
        NVIC_SetPriority(UART4_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART4_IRQn);
        break;

    case 5: // 进行UART5的初始化配置
        /*UART5 IO 配置*/
        AltFunIO(GPIOA, GPIO_Pin_8, 2); // PA8 UART5 RX
        AltFunIO(GPIOA, GPIO_Pin_9, 0); // PA9 UART5 TX

        /*NVIC中断配置*/
        NVIC_DisableIRQ(UART5_IRQn);
        NVIC_SetPriority(UART5_IRQn, 2); // 中断优先级配置
        NVIC_EnableIRQ(UART5_IRQn);
        break;

    default:
        break;
    }

    // UART初始化配置
    USART_InitStruct.BaudRate = BaudRate; // 波特率
    USART_InitStruct.DataBit = Eight8Bit; // 数据位数
    USART_InitStruct.ParityBit = NONE;    // 奇偶校验
    USART_InitStruct.StopBit = OneBit;    // 停止位

    CMU_GetClocksFreq(&CMU_Clocks);
    UART_SInit(UARTx, &USART_InitStruct, &CMU_Clocks); // 初始化uart

    UARTx_IER_RXBF_IE_Setable(UARTx, ENABLE); // 接收中断使能
    UARTx_CSR_RXEN_Setable(UARTx, ENABLE);    // 接收使能
    UARTx_CSR_TXEN_Setable(UARTx, ENABLE);    // 发送使能
    UARTx->ISR = 0xFFFFFFFF;                  // 清除标志
}

/**
 * @brief  低功耗串口0接收中断事件,中断服务函数
 * @param
 * @retval
 */
void LPUART0_IRQHandler(void)
{
    static u16 txIndex;
    // 接收中断处理
    if ((ENABLE == LPUARTx_IER_RXBF_IE_Getable(LPUART0)) && (SET == LPUARTx_ISR_RXBF_Chk(LPUART0))) // 接收数据匹配中断
    {
        u8 DataReceived = LPUARTx_RXBUF_Read(LPUART0); // 读取出来接收到的数据
        LPUARTx_ISR_RXBF_Clr(LPUART0);                 // 清除中断标志
        if (uartLoRa.recvLen < uart_recv_max_len) {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 100; // 50ms超时
        }
    }

    // 发送中断处理
    if ((ENABLE == LPUARTx_IER_TXSE_IE_Getable(LPUART0)) && (SET == LPUARTx_ISR_TXSE_Chk(LPUART0))) {
        // 发送指定长度的数据
        if (uartLoRa.txIndex < uartLoRa.sendLen) {
            LPUARTx_TXBUF_Write(LPUART0, uartLoRa.sendData[uartLoRa.txIndex]); // 发送一个数据
            uartLoRa.txIndex++;
        }
        // 清除发送中断标志
        LPUARTx_ISR_TXSE_Clr(LPUART0);
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
        device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief  初始化低功耗串口收发器，波特率9600，默认用来与设备之间通讯
 * @param
 * @retval
 */
void LPUART0_Init(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // PAD时钟使能

    CMU_PERCLK_SetableEx(LPUART0CLK, ENABLE);               // LPUART0总线时钟使能
    CMU_OPCCR1_LPUART0CKE_Setable(ENABLE);                  // LPUART0工作时钟使能
    CMU_OPCCR1_LPUART0CKS_Set(CMU_OPCCR1_LPUART0CKS_LSCLK); // LPUART0工作时钟选择LSCLK
    // CMU_OPCCR1_LPUART0CKS_Set(CMU_OPCCR1_LPUART0CKS_RCHF);//LPUART0工作时钟选择LSCLK

    AltFunIO(GPIOC, GPIO_Pin_0, 2); // PC0 LPUART RX 假如外部没有上拉电阻建议配置上拉电阻
    AltFunIO(GPIOC, GPIO_Pin_1, 0); // PC1 LPUART TX
    // GPIOx_DFS_Setable(GPIOC, GPIO_Pin_0, ENABLE); // PC0 数字功能选择LPUART
    // GPIOx_DFS_Setable(GPIOC, GPIO_Pin_1, ENABLE); // PC1 数字功能选择LPUART

    /*NVIC中断配置*/
    NVIC_DisableIRQ(LPUART0_IRQn);
    NVIC_SetPriority(LPUART0_IRQn, 2); // 中断优先级配置
    NVIC_EnableIRQ(LPUART0_IRQn);

    // 波特率等
    LPUARTx_BMR_BAUD_Set(LPUART0, LPUARTx_BMR_BAUD_9600); // 波特率控制
    LPUARTx_BMR_MCTL_Set(LPUART0, LPUARTx_MCTL_FOR9600);  // 调制控制寄存器

    LPUARTx_CSR_STOPCFG_Set(LPUART0, LPUARTx_CSR_STOPCFG_1STOP); // 停止位长度
    LPUARTx_CSR_PDSEL_Set(LPUART0, LPUARTx_CSR_PDSEL_8BITS);     // 数据长度
    LPUARTx_CSR_PARITY_Set(LPUART0, LPUARTx_CSR_PARITY_NON);     // 校验位类型

    LPUARTx_IER_RXBF_IE_Setable(LPUART0, ENABLE); // 接收中断使能
    LPUARTx_CSR_RXEN_Setable(LPUART0, ENABLE);    // 接收使能
    LPUARTx_CSR_TXEN_Setable(LPUART0, ENABLE);    // 发送使能
    LPUART0->ISR = 0xFFFFFFFF;                    // 清除标志
}

/**
 * @brief  通过LPUART0外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void LoRa_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size) {
        // INFO("%02x ",buffptr[TxCounter]);
#if FUNC_LORA_VERSION_LIR
        /* Send one byte */
        LPUARTx_TXBUF_Write(LPUART0, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!LPUARTx_ISR_TXBE_Chk(LPUART0))
            ;
#else
        /* Send one byte from UART0 to USARTx */
        UARTx_TXBUF_Write(UART3, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!UARTx_ISR_TXBE_Chk(UART3))
            ;
#endif
    }
    if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
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
 * @brief  通过USART0外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from UART0 to USARTx */
        UARTx_TXBUF_Write(UART0, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!UARTx_ISR_TXBE_Chk(UART0))
            ;
    }
}

/**
 * @brief  通过USART5外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void Ble_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from UART0 to USARTx */
        UARTx_TXBUF_Write(UART5, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!UARTx_ISR_TXBE_Chk(UART5))
            ;
    }
}

/**
 * @brief  通过USART1外设传输多个字符串
 * @param  cmd:要传输的字符串命令
 * @retval
 */
void Ble_SendCMD(const char *cmd)
{
    Ble_SendData((u8 *)cmd, strlen(cmd));
}

/**
 * @brief  通过USART2外设传输多个数据
 * @param  buffer:要传输的数据
 * @param  buffer_size:要传输的数据的字节大小
 * @retval
 */
void Zs_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        /* Send one byte from UART0 to USARTx */
        UARTx_TXBUF_Write(UART2, buffptr[TxCounter++]);
        /* Loop until register is empty */
        while (!UARTx_ISR_TXBE_Chk(UART2))
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
	    /* 互斥操作 */
    tx_mutex_get(&u_mutex_lock, TX_WAIT_FOREVER);
    static unsigned char s[1500]; // This needs to be large enough to store the string TODO Change magic number
    // unsigned char s[500]; // This needs to be large enough to store the string TODO Change magic number
    unsigned char *p;
    va_list args;
    int str_len;

    if ((str == NULL) || (strlen(str) == 0)) {
        return;
    }

    va_start(args, str);
    str_len = (unsigned int)vsnprintf((char *)s, sizeof(s), str, args);
    va_end(args);
    p = s;

    /* 互斥操作 */
    // tx_mutex_get(&u_mutex_lock, TX_WAIT_FOREVER);
    INFO_SendData(p, str_len);
    tx_mutex_put(&u_mutex_lock);
}

void DebugHexInfo(char *tag, u8 *data, int len)
{
    INFO("\n Uart Data [%s]:", tag);
    for (int i = 0; i < len; i++) {
        INFO(" %02x", data[i]);
    }
    INFO("\n");
}

void PrintInfoDumpExt(u8 *info, u16 len, char *name)
{
    u16 i = 0, j = 0;
    u8 bCR = 0;
    INFO("\r\n%s\r\n", name);
    for (i = 0; i < len / 16; i++) {
        for (j = 0; j < 16; j++) {
            INFO(" %02x", info[i * 16 + j]);
        }
        INFO("\r\n");
    }
    for (j = 0; j < len % 16; j++) {
        bCR = 1;
        INFO(" %02x", info[i * 16 + j]);
    }
    if (bCR) {
        INFO("\r\n");
    }
}
