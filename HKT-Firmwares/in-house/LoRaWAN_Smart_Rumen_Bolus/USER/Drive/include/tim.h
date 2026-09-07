
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "systick.h"
#include <config.h>

#define RTC_ERROR_NONE 0
#define RTC_ERROR_TIMEOUT 1

#if RTC_CLOCK_SOURCE_LSE
/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 256Hz with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV ((uint32_t)0x00FF)

#else
/* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=37 kHz RC */
#define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV ((uint32_t)0x122)

#endif

void BEEP_RUN(int time, int percent);
void TIM6_Init(u16 period, u16 prescaler);
void RTC_Init(void);
void RTC_AlarmInit(struct tm *alarmTime);
void EnterStopMode(void);
void EnterStandbyMode(void);
void getTimestamp(void);
void sysTimeSwitchToRtcTime(void);
void lpmSleepTimeInit(void);

#ifdef __cplusplus
}
#endif

#endif
