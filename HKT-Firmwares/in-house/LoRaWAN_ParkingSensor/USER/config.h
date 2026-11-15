#ifndef __CONFIG_H__
#define __CONFIG_H__

//-------------------------------------
#ifdef MAIN_CONFIG // 头文件被多个C调用时，避免变量冲突问题
#define EXT
#else
#define EXT extern
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_cortex.h"
#include "stm32l4xx_hal_flash.h"
#include "stm32l4xx_hal_flash_ex.h"
#include "stm32l4xx_hal_flash_ramfunc.h"
#include "stm32l4xx_hal_pwr.h"
#include "stm32l4xx_ll_adc.h"
#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_cortex.h"
#include "stm32l4xx_ll_crs.h"
#include "stm32l4xx_ll_dac.h"
#include "stm32l4xx_ll_dma.h"
#include "stm32l4xx_ll_exti.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_i2c.h"
#include "stm32l4xx_ll_iwdg.h"
#include "stm32l4xx_ll_lptim.h"
#include "stm32l4xx_ll_lpuart.h"
#include "stm32l4xx_ll_pwr.h"
#include "stm32l4xx_ll_rcc.h"
#include "stm32l4xx_ll_rtc.h"
#include "stm32l4xx_ll_spi.h"
#include "stm32l4xx_ll_system.h"
#include "stm32l4xx_ll_tim.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l4xx_ll_utils.h"

#include "stdlib.h"
#include "time.h"
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

#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

#define CMD_TAG "CMD"
#define LOG_TAG "LOG"
#define ERROR_TAG "ERROR"
#define WARN_TAG "WARN"
#define FLASH_TAG "FLASH"
#define LWRB_TAG "LWRB"
#define ACK_TAG "ACK"

#define SYSTEM_TIMER_BASE 1
#define SEC_DELAY (1000 / SYSTEM_TIMER_BASE) // u16 不能超过65秒

// 旧升级方式 2026.11.4之前
// #if 0
// #define HARDWARE_VER 0x0A
// #define PROCT_NAME "EPS100" // 地埋式
// #else
// #define HARDWARE_VER 0x0B
// #define PROCT_NAME "MPS100" // 表贴式
// #endif

// 2026.11.4 以后使用新硬件版本区分升级方式
#if 0
#define HARDWARE_VER 0x3C
#define PROCT_NAME "EPS100" // 地埋式
#else
#define HARDWARE_VER 0x3D
#define PROCT_NAME "MPS100" // 表贴式
#endif

#define SOFTWARE_VER 0x1E

/* LoRaWAN region is a runtime configuration since the 0xA0 BLE protocol:
 * band comes from the flash-backed cfg struct (see LoRaWAN_APPLY.h), this
 * constant only seeds a blank/erased cfg page. Band codes (module AT+BAND):
 * 6=CN470 7=CN470B 84=RU864 85=IN865 86=EU868 90=US915 91=AU915
 * 92=KR920 93..96=AS923-1..4 */
#define LoRaWAN_DEFAULT_BAND 86

/* 1 = apply custom (non-standard) uplink/downlink FREQ tables per region,
 * for custom-network deployments only. Stock builds use the module's own
 * region defaults. */
#define LORAWAN_CUSTOM_FREQ_ENABLE 0

#define FUNC_LoRaWAN_SAMPLE 1

#define LoRaWAN_DEFAULT_PORT 10
#define LoRaWAN_DEFAULT_CLASS 0 // 0 – ClassA  2 – ClassC

#define HIGH_LEVEL_DEBUG_ENABLE 0
#define NOR_FLASH_ENABLE 0

#define NORFLASH_DEBUG_PRINTF 1

#define DATA_COLLECT_INTERVAL 500 // 检测间隔
#define MAG_CHANGED_CNT 3         // 磁力计变化持续次数
#define RD_CHANGED_CNT 3          // 雷达变化持续次数

// 全局类型定义
typedef enum { FALSE = 0,
               TRUE = !FALSE } BOOL;

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
