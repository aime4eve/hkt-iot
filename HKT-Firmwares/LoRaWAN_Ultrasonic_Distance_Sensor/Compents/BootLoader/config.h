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
#include "stm32l4xx_ll_crc.h"
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

#define HARDWARE_VER 0x0A
#define SOFTWARE_VER 0x00

// 全局类型定义
typedef enum {
    FALSE = 0,
    TRUE = !FALSE
} BOOL;

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
