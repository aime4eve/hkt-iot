
#ifndef __UART_H__
#define __UART_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define CMD_COMMON_DEBUG_ON 0

#if CMD_COMMON_DEBUG_ON
#define DEBUG_TRACE(tag, fmt, ...) \
    INFO("\n[%s]", tag);           \
    INFO(fmt, ##__VA_ARGS__);      \
    INFO(" (File: %s, Line: %d, Func: %s)", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_TRACE(tag, fmt, ...) \
    //INFO("\n[%s]", tag);           \
   //INFO(fmt, ##__VA_ARGS__);
#endif

#define uart_recv_max_len 2200
#define uart_send_max_len 500

    typedef struct uart
    {
        u8 sendData[uart_send_max_len];
        u16 sendLen;
        u16 txIndex;
        u8 sendRetry;  // 重发次数
        u16 sendDelay; // 发送延时
        u8 recvData[uart_recv_max_len];
        u16 recvLen;
        u16 recvTimeout;
    } uart_t;

    extern uart_t uartInfo;

    void Uartx_Init(UART_Type *UARTx, u32 BaudRate);
    void INFO_SendData(uint8_t *buffer, u16 buffer_size);
    void INFO(const char *str, ...);
    void DebugHexInfo(char *tag, u8 *data, int len);
    void fromInfoDataHandle(void);
    void AppProgramRun(void);

#ifdef __cplusplus
}
#endif

#endif
