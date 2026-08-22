
#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_GPS_TX_PIN LL_GPIO_PIN_10
#define GPIO_GPS_TX_PORT GPIOB

#define GPIO_GPS_RX_PIN LL_GPIO_PIN_11
#define GPIO_GPS_RX_PORT GPIOB

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

#define uart_recv_max_len 255
#define uart_send_max_len 255

    typedef struct uart
    {
        u8 sendData[uart_send_max_len];
        u16 sendLen;
        u8 sendRetry;  // 重发次数
        u16 sendDelay; // 发送延时
        u8 recvData[uart_recv_max_len];
        u16 recvLen;
        u16 recvTimeout;
    } uart_t;

    typedef struct uart_gps
    {
        u8 sendData[100];
        u16 sendLen;
        u8 sendRetry;  // 重发次数
        u16 sendDelay; // 发送延时
        u8 recvData[1000];
        u16 recvLen;
        u16 recvTimeout;
    } uart_gps;

    extern struct uart uartLoRa;
    extern struct uart_gps uartGps;
    extern struct uart uartInfo;

    void App_LpUart1Cfg(void);
    void App_Uart1Cfg(void);
    void App_Uart2Cfg(void);
    void App_LpUart1DeInit(void);
    void App_Uart1_Deinit(void);
    void App_Uart2_Deinit(void);
    void IOUart_Init(void);

    void Gps_SendData(uint8_t *buffer, u16 buffer_size);
    void Gps_SendCMD(const char *cmd);
    void LoRa_SendData(uint8_t *buffer, u16 buffer_size);
    void LoRa_SendCMD(const char *cmd);
    void Ble_SendData(uint8_t *buffer, u16 buffer_size);
    void Ble_SendCMD(const char *cmd);
    void IOUart_SendByte(u8 datatoSend);
    void IOUart_SendNByte(uint8_t *buffer, u16 buffer_size);
    void INFO(const char *str, ...);

    void DebugHexInfo(char *tag, u8 *data, int len);

#ifdef __cplusplus
}
#endif

#endif
