#ifndef __CONFIG_H__
#define __CONFIG_H__

//-------------------------------------
#ifdef MAIN_CONFIG // 头文件被多个C调用时，避免变量冲突问题
#define EXT
#else
#define EXT extern
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
// #include "stm32l0xx_hal_flash.h"
// #include "stm32l0xx_hal_flash_ex.h"
// #include "stm32l0xx_hal_flash_ramfunc.h"
// #include "stm32l0xx_hal_pwr.h"

#include "stdlib.h"
#include "time.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// 无符号类型的定义
#define uchar unsigned char
#define uint unsigned int

#define uint8 unsigned char
#define uint16 unsigned short int
#define uint32 unsigned int

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define CMD_TAG "CMD"
#define LOG_TAG "LOG"
#define ERROR_TAG "ERROR"
#define WARN_TAG "WARN"
#define FLASH_TAG "FLASH"
#define LWRB_TAG "LWRB"
#define ACK_TAG "ACK"

#define SYSTEM_TIMER_BASE 1
#define SEC_DELAY (1000 / SYSTEM_TIMER_BASE) // u16 不能超过65秒

#define FUNC_GPS_FIRM_L76K 1
#define RTC_CLOCK_SOURCE_LSE 0
#define FUNC_GPS_ENABLE 1
#define FUNC_ACC_ENABLE 1

#define FUNC_LoRaWAN_ABP 0
#define FUNC_LoRaWAN_SAMPLE 1

#define FUNC_LoRaWAN_CN470_0 0 // 通道0-7
#define FUNC_LoRaWAN_CN470_8 0 // 通道8-15
#define FUNC_LoRaWAN_CN470A 0  // 通道25-32
#define FUNC_LoRaWAN_IN865 0
#define FUNC_LoRaWAN_EU868 0
#define FUNC_LoRaWAN_US915 0
#define FUNC_LoRaWAN_AU915 1
#define FUNC_LoRaWAN_AU915_0 0
#define FUNC_LoRaWAN_KR920 0
#define FUNC_LoRaWAN_AS923_AS1 0
#define FUNC_LoRaWAN_AS923_AS2 0
#define FUNC_LoRaWAN_EU868_TO_US915 0
#define FUNC_LoRaWAN_EU868_TO_AU915 1
#define FUNC_LoRaWAN_EU868_TO_KR920 0

#define LoRaWAN_DEFAULT_PORT 10
#define LoRaWAN_DEFAULT_CLASS 0 // 0 – ClassA  2 – ClassC

#define LoRaWAN_DEBUG_ENABLE 0
#define HIGH_LEVEL_DEBUG_ENABLE 0

#define DEFAULT_REPORT_INTERVAL  60
#define DEFAULT_TEMPERATURE_LOW_THRESHOLD -30 // 低温报警阈值
#define DEFAULT_TEMPERATURE_HIGH_THRESHOLD 99 // 高温报警阈值
#define DEFAULT_HUMIDITY_LOW_THRESHOLD 1      // 低湿度报警阈值
#define DEFAULT_HUMIDITY_HIGH_THRESHOLD 99    // 高湿度报警阈值
#define DEFAULT_ANGLE_THRESHOLD 25            // 角度变化阈值

#define HARDWARE_VER 0x03
#define SOFTWARE_VER 0x29

// 全局类型定义
typedef enum {
    FALSE = 0,
    TRUE = !FALSE
} BOOL;

typedef enum sfType {
    SF6,
    SF7,
    SF8,
    SF9,
    SF10,
    SF11,
    SF12
} sfType_t;
typedef enum bwType {
    BW62K,
    BW125K,
    BW250K,
    BW500K
} bwType_t;
typedef enum crType {
    CR4_5,
    CR4_6,
    CR4_7,
    CR4_8
} crType_t;

typedef union {
    struct
    {
        u8 FreqL : 8;
        u8 FreqM : 8;
        u8 FreqH : 8;
        u8 FreqX : 8;
    } freq;
    unsigned long Freq;
} FreqStruct;

typedef enum LoRaWAN_Status {
    LoRaWAN_STATUS_NORMAL,
    LoRaWAN_STATUS_NORESPONSES,
    LoRaWAN_STATUS_NET_ERROR,
} LoRaWAN_Status_t;

typedef enum LoRaWAN_JoinEvent {
    LoRaWAN_JOINEVENT_IDLE,
    LoRaWAN_JOINEVENT_INIT,     // 初始化
    LoRaWAN_JOINEVENT_REGISTER, // 注册
    LoRaWAN_JOINEVENT_JOINED,   // 完成入网
    LoRaWAN_JOINEVENT_ERROR,    // 入网失败

} LoRaWAN_JoinEvent_t;

struct LoRaParam {
    u8 joinState : 1; // 入网状态
    u8 waitJoin : 1;  // 等待设备入网
    u8 port;
    u8 class;     // 0 class A    2 class C
    u8 DevEUI[8]; // OTAA入网方式使用
    u8 AppEUI[8];
    u8 AppKEY[16];

    u8 DevADDR[4]; // ABP入网方式可直接使用如下三个参数
    u8 AppSKEY[16];
    u8 NwkSKEY[16];

    LoRaWAN_Status_t status;       // LoRa模块状态 0 正常 1入网失败
    LoRaWAN_JoinEvent_t joinEvent; // LoRa入网事件

    u16 bandWidth; // unit: KHz
    u32 frequency; // unit: KHz

    char outputPower; // unit: dBm   range: 2-20 [2dBm~+20dBm]

    enum sfType SFSel; // unit: none, range: SF6~SF12
    enum bwType BWSel;
    enum crType CRSel;

    FreqStruct FrequencyValue;
};

extern ADC_HandleTypeDef hadc;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef hlpuart1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern RTC_HandleTypeDef hrtc;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim6;

extern void Error_Handler(void);

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
