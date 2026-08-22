
#include "uart.h"
#include "LoRaWAN_APPLY.h"
#include "gpio.h"

struct uart uartLoRa;
struct uart uartInfo;

/**
 * @brief  串口接收中断事件,中断服务函数
 * @param
 * @retval
 */
void USART0_IRQHandler(void)
{
    if (USART_GetFlagStatus(HT_USART0, USART_FLAG_RXDR)) {
        /* clear IDLE flag */
        u8 DataReceived = USART_ReceiveData(HT_USART0); // 读取出来接收到的数据
        if (uartLoRa.recvLen < uart_recv_max_len) {
            uartLoRa.recvData[uartLoRa.recvLen++] = DataReceived;
            uartLoRa.recvTimeout = 100; // 100ms超时
        }
    }
}

/**
 * @brief  串口接收中断事件,中断服务函数
 * @param
 * @retval
 */
void UART1_IRQHandler(void)
{
    /* Rx: Move data from UART to buffer                                                                      */
    if (USART_GetFlagStatus(HT_UART1, USART_FLAG_RXDR)) {
        u8 DataReceived = USART_ReceiveData(HT_UART1);
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
void LoRa_UartCfg(void)
{
    /* Enable peripheral clock of AFIO, UxART                                                               */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.AFIO = 1;
    CKCUClock.Bit.PA = 1;
    CKCUClock.Bit.USART0 = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    /* Turn on UxART Rx internal pull up resistor to prevent unknow state                                     */
    GPIO_PullResistorConfig(HT_GPIOA, GPIO_PIN_3, GPIO_PR_UP);

    /* Config AFIO mode as UxART function.                                                                    */
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_2, AFIO_FUN_USART_UART);
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_3, AFIO_FUN_USART_UART);

    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
    USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
    USART_InitStructure.USART_Parity = USART_PARITY_NO;
    USART_InitStructure.USART_Mode = USART_MODE_NORMAL;

    USART_Init(HT_USART0, &USART_InitStructure);
    USART_TxCmd(HT_USART0, ENABLE);
    USART_RxCmd(HT_USART0, ENABLE);

    NVIC_EnableIRQ(USART0_IRQn);
    /* Enable UxART Rx interrupt                                                                              */
    USART_IntConfig(HT_USART0, USART_INT_RXDR, ENABLE);
}

/**
 * @brief  打印服务串口，波特率115200
 * @param
 * @retval
 */
void Info_UartCfg(void)
{
    /* Enable peripheral clock of AFIO, UxART                                                               */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.AFIO = 1;
    CKCUClock.Bit.PA = 1;
    CKCUClock.Bit.UART1 = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    /* Turn on UxART Rx internal pull up resistor to prevent unknow state                                     */
    GPIO_PullResistorConfig(HT_GPIOA, GPIO_PIN_5, GPIO_PR_UP);

    /* Config AFIO mode as UxART function.                                                                    */
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_4, AFIO_FUN_USART_UART);
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_5, AFIO_FUN_USART_UART);

    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WORDLENGTH_8B;
    USART_InitStructure.USART_StopBits = USART_STOPBITS_1;
    USART_InitStructure.USART_Parity = USART_PARITY_NO;
    USART_InitStructure.USART_Mode = USART_MODE_NORMAL;

    USART_Init(HT_UART1, &USART_InitStructure);
    USART_TxCmd(HT_UART1, ENABLE);
    USART_RxCmd(HT_UART1, ENABLE);
    NVIC_EnableIRQ(UART1_IRQn);
    /* Enable UxART Rx interrupt                                                                              */
    USART_IntConfig(HT_UART1, USART_INT_RXDR, ENABLE);
    // DEBUG_TRACE(LOG_TAG,"Uart Init Finish");
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
    // DEBUG_TRACE(LOG_TAG,"Send LoRa Data Len : %d, ",buffer_size);
    while (TxCounter < buffer_size) {
        // INFO("%02x ",buffptr[TxCounter]);
        /* Send one byte */
        while (USART_GetFlagStatus(HT_USART0, USART_FLAG_TXC) == RESET)
            ;
        USART_SendData(HT_USART0, buffptr[TxCounter++]);
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
void INFO_SendData(uint8_t *buffer, u16 buffer_size)
{
    u16 TxCounter = 0;
    u8 *buffptr = buffer;
    while (TxCounter < buffer_size) {
        while (USART_GetFlagStatus(HT_UART1, USART_FLAG_TXC) == RESET)
            ;
        USART_SendData(HT_UART1, buffptr[TxCounter++]);
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
