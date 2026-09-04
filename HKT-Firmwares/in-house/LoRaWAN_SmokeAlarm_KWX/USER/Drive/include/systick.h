
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "config.h"

/* Settings ------------------------------------------------------------------------------------------------*/
#define TICK_OFFSET                               (13) // HT32F50231/50241
//#define TICK_OFFSET                               (15) // HT32F52342/52352 (Branch Cache)

#define TICK_US                                   (LIBCFG_MAX_SPEED / 1000000)

/* Private macro -------------------------------------------------------------------------------------------*/
#define US2TICK(us)                               (us * TICK_US - TICK_OFFSET)

#define delayMicroseconds(cnt)                    {SysTick->LOAD = cnt;\
                                                   SysTick->VAL = 0;\
                                                   while (SysTick->CTRL == 5);}

extern time_t Timestamp;

/* Exported functions ------------------------------------------------------- */
void SystemClock_Config(void);
void Systick_Init(void);
void WDT_Configuration(void);
void Systick_1ms_Handler(void);
void delay_1us(uint32_t count);
void delay_1ms(uint32_t count);
u16 random_syspant(u16 n);
u32 get_syspant_ms(void);
u32 get_sysopentime(void);

#endif /* __SYSTICK_H */
