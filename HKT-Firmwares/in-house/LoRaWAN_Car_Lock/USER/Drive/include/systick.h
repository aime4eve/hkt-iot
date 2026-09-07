
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "config.h"

extern time_t Timestamp;

#define RTC_CLOCK_SOURCE_LSE 1

/* Exported functions ------------------------------------------------------- */
void SystemClock_Config(void);
void Systick_Init(void);
void Systick_1ms_Handler(void);
void Delay_Us(__IO u16 myUs);
void Delay_Ms(__IO u16 myMs);
void mDelay(u16 ms);
u16 random_syspant(u16 n);
u32 get_syspant_ms(void);
u32 get_sysopentime(void);

#endif /* __SYSTICK_H */
