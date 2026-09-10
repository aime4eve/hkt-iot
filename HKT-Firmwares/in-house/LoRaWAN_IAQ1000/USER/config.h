#ifndef __CONFIG_H__
#define __CONFIG_H__

//-------------------------------------
#ifdef MAIN_CONFIG // 头文件被多个C调用时，避免变量冲突问题
#define EXT
#else
#define EXT extern
#endif

/* Includes ------------------------------------------------------------------*/

// 无符号类型的定义
#define uchar unsigned char
#define uint  unsigned int

#define uint8  unsigned char
#define uint16 unsigned short int
#define uint32 unsigned int

#define u8  unsigned char
#define u16 unsigned short
#define u32 unsigned int

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

#define CMD_TAG   "CMD"
#define LOG_TAG   "LOG"
#define ERROR_TAG "ERROR"
#define WARN_TAG  "WARN"
#define FLASH_TAG "FLASH"
#define LWRB_TAG  "LWRB"
#define ACK_TAG   "ACK"

#define SYSTEM_TIMER_BASE 50
#define SEC_DELAY         (1000 / SYSTEM_TIMER_BASE) // u16 不能超过65秒

#define HARDWARE_VER 0x03
#define SOFTWARE_VER 0x0F

#define FUNC_THREE_IN_ONE 0 // 客户定制版本固件

#define FUNC_AMBIENT_LIGHT 1
#define FUNC_TVOC          1
#define FUNC_PRESSURE      1
#define FUNC_PM2_5         1
#define FUNC_HCHO          1
#define FUNC_O3            0

#define FUNC_BATTERY 0
#define CM1106SL     0
#define EPD_E042A162 1     // NEW屏

#define EPD_SW_CO_HCHO_SHOW 0
#define EPD_SHOW_MODE_FIXED 1
#define FUNC_MINIRF_ENABLE 1

#if FUNC_THREE_IN_ONE
#undef FUNC_PRESSURE
#undef FUNC_TVOC
#undef FUNC_AMBIENT_LIGHT
#undef FUNC_PM2_5
#undef FUNC_HCHO
#undef FUNC_O3
#endif

#if FUNC_HCHO
#undef FUNC_O3
#endif
#define FUNC_SHOW_TIME 0
#define FUNC_SHOW_LOGO 0

#define FUNC_LoRaWAN_ABP    0
#define FUNC_LoRaWAN_SAMPLE 1

#define FUNC_LoRaWAN_CN470_0        0 // 通道0-7
#define FUNC_LoRaWAN_CN470_8        0 // 通道8-15
#define FUNC_LoRaWAN_CN470A         0 // 通道25-32
#define FUNC_LoRaWAN_IN865          0
#define FUNC_LoRaWAN_EU868          1
#define FUNC_LoRaWAN_US915          0
#define FUNC_LoRaWAN_AU915          0
#define FUNC_LoRaWAN_KR920          0
#define FUNC_LoRaWAN_AS923_AS1      0
#define FUNC_LoRaWAN_AS923_AS2      0
#define FUNC_LoRaWAN_EU868_TO_US915 0
#define FUNC_LoRaWAN_EU868_TO_AU915 0
#define FUNC_LoRaWAN_EU868_TO_KR920 0

#define LoRaWAN_DEFAULT_PORT  10
#define LoRaWAN_DEFAULT_CLASS 2 // 0 – ClassA  2 – ClassC

#define HIGH_LEVEL_DEBUG_ENABLE 1

#define PMSA_DEBUG_PRINTF  0
#define LIGHT_DEBUG_PRINTF 1

#define NORFLASH_DEBUG_PRINTF 0
#define PIR_DEBUG_PRINTF      0
#define FUNC_THREAD_DEBUG     0

#define AIR_LEVEL_NORMAL 0
#define AIR_LEVEL_MILD   1
#define AIR_LEVEL_BAD    2

#define BIT_CO2     (1 << 0)
#define BIT_THTB    (1 << 1)
#define BIT_TVOC    (1 << 2)
#define BIT_LIGHT   (1 << 3)
#define BIT_PIR     (1 << 4)
#define BIT_DISPLAY (1 << 5)
#define BIT_O3      (1 << 6)

#define BIT_ALL (BIT_CO2 | BIT_THTB | BIT_TVOC | BIT_LIGHT | BIT_PIR | BIT_DISPLAY | BIT_O3)

/*include*/
#include "define_all.h"

#include "tx_api.h"
#include "tx_timer.h"

// 全局类型定义
typedef enum { FALSE = 0, TRUE = !FALSE } BOOL;

/* 方便RTOS里面使用 */
// extern void SysTick_ISR(void);

// #define bsp_ProPer1ms  SysTick_ISR
extern TX_EVENT_FLAGS_GROUP event_group;
extern TX_MUTEX mutex_info; /* 用于printf互斥 */
extern TX_MUTEX mutex_iic;  /* 用于IIC总线互斥 */
extern TX_TIMER AppTimer;

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
