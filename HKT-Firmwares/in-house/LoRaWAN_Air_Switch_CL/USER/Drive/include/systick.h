
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "config.h"

extern time_t Timestamp;

/* Exported functions ------------------------------------------------------- */
void SystemClock_Config(void);
void Systick_Init(void);
void Systick_1ms_Handler(void);
void delay_1us(uint32_t count);
void delay_1ms(uint32_t count);
u16 random_syspant(u16 n);
u32 get_syspant_ms(void);
u32 get_sysopentime(void);

#endif /* __SYSTICK_H */
