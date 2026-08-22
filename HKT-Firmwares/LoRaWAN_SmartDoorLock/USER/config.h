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

#define uint8  unsigned char·
#define uint16 unsigned short int
#define uint32 unsigned int

#define u8  unsigned char
#define s8  char
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

#define FUNC_OPERATIONAL_VERSION_ENABLE 1
#define FUNC_CONFIG_NETWORK_ENABLE      0 // 配置通过系统菜单开启网络功能是否启用
#define FUNC_DEFAULT_NETWORK_ON         1
#define FUNC_LORA_VERSION_LIR           0
#define LPT_CARD_ENABLE                 0
#define FUNC_BT_ENABLE                  1
#define LOCAL_LOCK_ENABLE               0
#define LPCD_ENABLE                     0
#define EU_TOUCH_LED                    0
#define FSV9522_CARD_ENABLE             1
#define FUNC_TAMPER_ENABLE              1 // 防拆功能开关
#define FUNC_DOORBELL_ENABLE            0

#if FUNC_OPERATIONAL_VERSION_ENABLE

#define HARDWARE_VER                0x11
#define LoRaWAN_DEFAULT_PORT        10
#define LoRaWAN_DEFAULT_CLASS       0 // 0 – ClassA  2 – ClassC
#define PROCT_NAME                  "SDL100"
#define FUNC_FINGERPRINT_ENABLE     0
#define FUNC_STANDARD_CLASSC_ENABLE 0
#else
#define HARDWARE_VER            0x09
#define LoRaWAN_DEFAULT_PORT    10
#define LoRaWAN_DEFAULT_CLASS   0 // 0 – ClassA  2 – ClassC
#define PROCT_NAME              "SDL200"
#define FUNC_FINGERPRINT_ENABLE 0
#endif
#define SOFTWARE_VER 0x19

#define FUNC_LoRaWAN_ABP    0
#define FUNC_LoRaWAN_SAMPLE 1

#define FUNC_LoRaWAN_CN470_0 0 // 通道0-7
#define FUNC_LoRaWAN_CN470_8 0 // 通道8-15
#define FUNC_LoRaWAN_CN470A  0 // 通道25-32
#define FUNC_LoRaWAN_IN865   0
#define FUNC_LoRaWAN_EU868   0
#define FUNC_LoRaWAN_US915   0
#define FUNC_LoRaWAN_AU915   1
#define FUNC_LoRaWAN_AU915_0   0

#define FUNC_LoRaWAN_KR920          0
#define FUNC_LoRaWAN_AS923_AS1      0
#define FUNC_LoRaWAN_AS923_AS2      0
#define FUNC_LoRaWAN_AS923_3        0
#define FUNC_LoRaWAN_EU868_TO_US915 0
#define FUNC_LoRaWAN_EU868_TO_AU915 1
#define FUNC_LoRaWAN_EU868_TO_KR920 0

#define HIGH_LEVEL_DEBUG_ENABLE 1
#define NORFLASH_ITER_PRINTF    0
#define NORFLASH_DEBUG_PRINTF   1
// #define NFC_DEBUG_PRINTF 1
#define FUNC_THREAD_DEBUG       0

#define BIT_MOTOR  (1 << 0)
#define BIT_FINGER (1 << 1)
#define BIT_TOUCH  (1 << 2)
#define BIT_CARD   (1 << 3)
#define BIT_SOUND  (1 << 4)
#define BIT_LED    (1 << 5)
#define BIT_SYNC   (1 << 6)
#define BIT_BLE    (1 << 7)
#define BIT_LORA   (1 << 8)

/*include*/
#include "define_all.h"

#include "tx_api.h"
#include "tx_timer.h"

typedef enum { FALSE = 0, TRUE = !FALSE } BOOL;

extern TX_MUTEX u_mutex_lock; /* 用于printf互斥 */
extern TX_MUTEX f_mutex_lock; /* 用于nor flash互斥 */
extern TX_MUTEX l_mutex_lock; /* 用于lora互斥 */
extern TX_SEMAPHORE Semaphore;
extern TX_EVENT_FLAGS_GROUP event_group;

/********************************************************************************************************
**                            End Of File
********************************************************************************************************/
#endif
