
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define RTC_ERROR_NONE    0
#define RTC_ERROR_TIMEOUT 1

#define RTC_CONVERT_BIN2BCD(__VALUE__) (uint8_t)((((__VALUE__) / 10U) << 4U) | ((__VALUE__) % 10U))
#define RTC_CONVERT_BCD2BIN(__VALUE__) (uint8_t)((((uint8_t)((__VALUE__) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U ) + ((__VALUE__) & (uint8_t)0x0FU))


    void BSTIM_Init(u16 period, u16 prescaler);
    void LPTIM_Init(void);
    void RTC_Init(void);
    void RTC_AlarmInit(RTC_AlarmTmieTypeDef *alarmTime);
    void getTimestamp(void);
    void sysTimeSwitchToRtcTime(void);
    void lpmSleepTimeInit(void);
    void BeepTimerInit(u16 period, u16 pulse);
    void beep_set(uint16_t freq, uint8_t volume);
    void BEEP_ON(void);
    void BEEP_OFF(void);
    void BeepTest(void);

#ifdef __cplusplus
}
#endif

#endif
