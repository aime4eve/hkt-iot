#ifndef __CONFIG_H__
#define __CONFIG_H__

//-------------------------------------
#ifdef MAIN_CONFIG //头文件被多个C调用时，避免变量冲突问题
#define EXT
#else
#define EXT extern
#endif

/* Includes ------------------------------------------------------------------*/
#include "ht32.h"

#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "stdlib.h"
#include <math.h>
#include "time.h"
#include <assert.h>
#include "lwrb.h"

//无符号类型的定义
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

#define HARDWARE_VER 0x0E
#define SOFTWARE_VER 0x03

#define FUNC_NTC_ENABLE 1
#define FUNC_TAMPER_ENABLE 1

#define FUNC_LoRaWAN_ABP 0

#define FUNC_LoRaWAN_SAMPLE 1

#define FUNC_LoRaWAN_CN470_0 0 // 通道0-7
#define FUNC_LoRaWAN_CN470_8 0 // 通道8-15
#define FUNC_LoRaWAN_CN470A 0  // 通道25-32
#define FUNC_LoRaWAN_IN865 0
#define FUNC_LoRaWAN_EU868 0
#define FUNC_LoRaWAN_US915 0
#define FUNC_LoRaWAN_AU915          1
#define FUNC_LoRaWAN_AU915_0          1
#define FUNC_LoRaWAN_KR920 0
#define FUNC_LoRaWAN_AS923_AS1 0
#define FUNC_LoRaWAN_AS923_AS2 0
#define FUNC_LoRaWAN_AS923_3 0
#define FUNC_LoRaWAN_AS923_2 0
#define FUNC_LoRaWAN_EU868_TO_US915 0
#define FUNC_LoRaWAN_EU868_TO_AU915 1
#define FUNC_LoRaWAN_EU868_TO_KR920 0

#define LoRaWAN_DEFAULT_PORT 10
#define LoRaWAN_DEFAULT_CLASS 0 // 0 – ClassA  2 – ClassC

#define LoRaWAN_DEBUG_ENABLE 0
#define HIGH_LEVEL_DEBUG_ENABLE 1

#define DEFAULT_REPORT_INTERVAL (24 * 60)      //数据上报间隔

// 2482 1659  2111

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
