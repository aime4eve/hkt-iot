
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "config.h"

extern struct tm systime;
extern struct tm utctime;
extern time_t Timestamp;

/* Exported functions ------------------------------------------------------- */
void SystemClock_Config(void);
void Systick_1ms_Handler(void);
int whatday(int year, int mon, int day);
void stamp_to_date(time_t stampTime, struct tm *stm);
void systemTime_Init(void);
void Delay_Us(u16 myUs);
void Delay_Ms(u16 myMs);
u16 random_syspant(u16 n);
u32 get_syspant_ms(void);
u32 get_sysopentime(void);

#endif /* __SYSTICK_H */
