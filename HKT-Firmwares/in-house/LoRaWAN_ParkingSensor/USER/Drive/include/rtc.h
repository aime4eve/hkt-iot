
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "time.h"
#include <config.h>

#define RTC_ERROR_NONE 0
#define RTC_ERROR_TIMEOUT 1

/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 256Hz with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV ((uint32_t)0x00FF)

void RTC_TimeSet(void);
void RTC_Init(void);
void RTC_AlarmInit(struct tm *alarmTime);
void lpmSleepTimeInit(void);
void EnterSleepMode(void);
void IWDG_Init(void);

#ifdef __cplusplus
}
#endif

#endif
