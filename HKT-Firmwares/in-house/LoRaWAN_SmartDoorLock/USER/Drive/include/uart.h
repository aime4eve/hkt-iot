
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

#define uart_recv_max_len 1000
#define uart_send_max_len 500

    typedef struct uart 
    {
        u8 sendData[uart_recv_max_len];
        u16 sendLen;
        u16 txIndex;
        u8 sendRetry;  // 重发次数
        u16 sendDelay; // 发送延时
        u8 recvData[uart_recv_max_len];
        u16 recvLen;
        u16 recvTimeout;
    } uart_t;

    extern uart_t uartLoRa;
    extern uart_t uartInfo;
    extern uart_t uartBle;
    extern uart_t uartZs;

    void LPUART0_Init(void);
    void Uartx_Init(UART_Type *UARTx, u32 BaudRate);

    void LoRa_SendData(uint8_t *buffer, u16 buffer_size);
    void LoRa_SendCMD(const char *cmd);
    void INFO_SendData(uint8_t *buffer, u16 buffer_size);
    void Ble_SendData(uint8_t *buffer, u16 buffer_size);
    void Ble_SendCMD(const char *cmd);
    void Zs_SendData(uint8_t *buffer, u16 buffer_size);
    void Uart2_Cmd_Process(u8 *cmd, u16 cmd_size);
    void INFO(const char *str, ...);
    void DebugHexInfo(char *tag, u8 *data, int len);
    void PrintInfoDumpExt(u8 *info, u16 len, char *name);

#ifdef __cplusplus
}
#endif

#endif
