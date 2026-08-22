
#ifndef __LoRAWAN_APPLY_H__
#define __LoRAWAN_APPLY_H__

///** 使用ADR功能，模块在运行过程中，服务器会根据ADR机制动态调整模块的发送功率，通讯速率

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define LoRaWAN_MODE_PIN GPIO_PIN_14
#define LoRaWAN_MODE_PORT GPIOB

#define LoRaWAN_BUSY_PIN GPIO_PIN_15
#define LoRaWAN_BUSY_PORT GPIOB

#define LoRaWAN_NRST_PIN GPIO_PIN_8
#define LoRaWAN_NRST_PORT GPIOA

#define LoRaWAN_STAT_PIN GPIO_PIN_11
#define LoRaWAN_STAT_PORT GPIOA

#if FUNC_LoRaWAN_FIRM_SX
#define LoRaWAN_WAKE_PIN GPIO_PIN_6
#define LoRaWAN_WAKE_PORT GPIOB
#else
#define LoRaWAN_WAKE_PIN GPIO_PIN_12
#define LoRaWAN_WAKE_PORT GPIOA
#endif

#define LoRaWAN_PASSTHROUGH_MODE GPIO_PIN_RESET // 透传模式
#define LoRaWAN_CMD_MODE GPIO_PIN_SET           // 命令模式

#define LoRaWAN_SLEEP_STATUS GPIO_PIN_RESET // 睡眠状态
#define LoRaWAN_WAKE_STATUS GPIO_PIN_SET    // 激活状态

    extern u8 appEui[];
    extern u8 appKey[];
    extern u8 appSKey[];
    extern u8 nwkSKey[];
    extern u8 devAddr[];
    extern u8 jion_fail_cnt;
    extern time_t reconnect_stamp;
    extern struct LoRaParam LoRaWAN;

    void LoRa_DelayMs(u32 delay_time);
    void rstLoRaWANModule(void);
    int getLoRaWANMode(void);
    void setLoRaWANMode(GPIO_PinState status);
    int getLoRaWANStatus(void);
    void setLoRaWANStatus(GPIO_PinState status);
    void setLoRaWANResetIOLevel(GPIO_PinState status);
    int getLoRaWANBusyIOLevel(void);
    int getLoRaWANStatIOLevel(void);
    int getLoRaWANSendStat(void);
    int getLoRaWANBusyStat(void);

    void LoRaWAN_CreatAPPKEY(void);
    void LoRaWAN_Init(void);

    void LoRaWAN_JoinInit(void);

#ifdef __cplusplus
}
#endif

#endif
