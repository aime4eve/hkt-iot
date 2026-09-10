
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>


#define RTC_ERROR_NONE    0
#define RTC_ERROR_TIMEOUT 1


#if RTC_CLOCK_SOURCE_LSE
/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 256Hz with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV ((uint32_t)0x00FF)
#else
// /* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=37 kHz RC */
// #define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
// /* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
// #define RTC_SYNCH_PREDIV ((uint32_t)0x122)

/* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=37 kHz RC */
#define RTC_ASYNCH_PREDIV ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV ((uint32_t)0x148)

#endif

    void RTC_AlarmInit(struct tm *alarmTime);
    void getTimestamp(void);
    void sysTimeSwitchToRtcTime(void);
    void lpmSleepTimeInit(void);
    void EnterSleepMode(u32 mode);

#ifdef __cplusplus
}
#endif

#endif
