
#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_LoRaWAN_TX_PIN LL_GPIO_PIN_10
#define GPIO_LoRaWAN_TX_PORT GPIOB

#define GPIO_LoRaWAN_RX_PIN LL_GPIO_PIN_11
#define GPIO_LoRaWAN_RX_PORT GPIOB

#define CMD_COMMON_DEBUG_ON 0

#if CMD_COMMON_DEBUG_ON
#define DEBUG_TRACE(tag, fmt, ...) \
    INFO("\n[%s]", tag);           \
    INFO(fmt, ##__VA_ARGS__);      \
    INFO(" (File: %s, Line: %d, Func: %s)", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_TRACE(tag, fmt, ...) \
    INFO("\n[%s]", tag);           \
    INFO(fmt, ##__VA_ARGS__);
#endif

#define uart1_recv_max_len 500
#define uart1_send_max_len 500

#define uart2_recv_max_len 100
#define uart2_send_max_len 100

    typedef struct uart1
    {
        u8 sendData[uart1_send_max_len];
        u16 sendLen;
        u8 sendRetry;  //重发次数
        u16 sendDelay; //发送延时
        u8 recvData[uart1_recv_max_len];
        u16 recvLen;
        u16 recvTimeout;
    } uart1_t;

    typedef struct uart2
    {
        u8 sendData[uart2_send_max_len];
        u16 sendLen;
        u8 recvData[uart2_recv_max_len];
        u16 recvLen;
        u16 recvTimeout;
    } uart2_t;

    extern struct uart1 uartLoRa;
    extern struct uart2 uartInfo;

    void App_LpUart1Cfg(void);
    void App_Uart1Cfg(void);
    void App_Uart2Cfg(void);

    void LoRa_SendData(uint8_t *buffer, u16 buffer_size);
    void LoRa_SendCMD(const char *cmd);
    void INFO_SendData(uint8_t *buffer, u16 buffer_size);
    void Uart2_Cmd_Process(u8 *cmd, u16 cmd_size);
    void INFO(const char *str, ...);
    void DebugHexInfo(char *tag, u8 *data, int len);

#ifdef __cplusplus
}
#endif

#endif
