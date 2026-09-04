
#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define CMD_COMMON_DEBUG_ON 0

#if CMD_COMMON_DEBUG_ON
#define DEBUG_TRACE(tag, fmt, ...)                                                                                                                   \
    INFO("\n[%s]", tag);                                                                                                                             \
    INFO(fmt, ##__VA_ARGS__);                                                                                                                        \
    INFO(" (File: %s, Line: %d, Func: %s)", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_TRACE(tag, fmt, ...)                                                                                                                   \
    INFO("\n[%s]", tag);                                                                                                                             \
    INFO(fmt, ##__VA_ARGS__);
#endif

#define uart_recv_max_len 128
#define uart_send_max_len 128

typedef struct uart {
    u8 sendData[uart_send_max_len];
    u16 sendLen;
    u8 sendRetry; // 重发次数
    u16 sendDelay; // 发送延时
    u8 recvData[uart_recv_max_len];
    u16 recvLen;
    u16 recvTimeout;
} uart_t __attribute__((aligned(4)));

extern struct uart uartLoRa;
extern struct uart uartBle;
extern struct uart uartRd;
extern struct uart uartInfo;

void App_LpUart1Cfg(void);
void App_Uart1Cfg(void);
void App_Uart2Cfg(void);
void App_Uart3Cfg(void);
void App_Uart2_Deinit(void);
void App_Uart3_Deinit(void);

void LoRa_SendCMD(const char *cmd);
void LoRa_SendData(uint8_t *buffer, u16 buffer_size);
void RD_SendData(uint8_t *buffer, u16 buffer_size);
void BLE_SendData(uint8_t *buffer, u16 buffer_size);
void BLE_SendCMD(const char *cmd);

void INFO_SendData(uint8_t *buffer, u16 buffer_size);
void Uart2_Cmd_Process(u8 *cmd, u16 cmd_size);
void INFO(const char *str, ...);
void DebugHexInfo(char *tag, u8 *data, int len);

#ifdef __cplusplus
}
#endif

#endif
