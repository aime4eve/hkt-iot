
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "systick.h"
#include <config.h>

void BEEP_RUN(int time, int percent);
void TIM16_Init(u16 period, u16 prescaler);
void LPTIM1_Init(void);
void getTimestamp(void);
void sysTimeSwitchToRtcTime(void);
void convertTimestampToSystime(void);

#ifdef __cplusplus
}
#endif

#endif
