
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void TIM1_UP_TIM16_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM16)) {
        LL_TIM_ClearFlag_UPDATE(TIM16);
        Systick_1ms_Handler();
        HAL_IncTick();
    }
}

/**
 * @brief  初始化通用定时器
 * @param  period：计数周期（实际值）
 * @param  prescaler：预分频值（实际值）
 * @retval
 */
void TIM16_Init(u16 period, u16 prescaler)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM16);

    /* TIM16 interrupt Init */
    NVIC_SetPriority(TIM1_UP_TIM16_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 1, 1));
    NVIC_EnableIRQ(TIM1_UP_TIM16_IRQn);

    TIM_InitStruct.Prescaler = prescaler - 1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = period - 1;
    LL_TIM_Init(TIM16, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM16);

    LL_TIM_EnableUpdateEvent(TIM16);
    LL_TIM_EnableIT_UPDATE(TIM16);
    LL_TIM_EnableCounter(TIM16);
}


/**
 * @brief  Display the current time and date.
 * @param  None
 * @retval None
 */
void Sync_RTC_Time(void)
{
    /* Note: need to convert in decimal value in using __LL_RTC_CONVERT_BCD2BIN helper macro */
    systime.tm_year = 2000 + __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC));
    systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC));
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
    systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
    systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
}

/**
 * @brief  时间戳转换
 * @param
 * @retval
 */
void getTimestamp(void)
{
    struct tm stm_stamp = {0};
    stm_stamp.tm_year = systime.tm_year - 1900;
    stm_stamp.tm_mon = systime.tm_mon;
    stm_stamp.tm_mday = systime.tm_mday;
    stm_stamp.tm_hour = systime.tm_hour;
    stm_stamp.tm_min = systime.tm_min;
    stm_stamp.tm_sec = systime.tm_sec;
    stm_stamp.tm_wday = systime.tm_wday;
    stm_stamp.tm_isdst = -1;
    Timestamp = mktime(&stm_stamp) - 8 * 60 * 60;
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{
    systime.tm_wday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC));
    systime.tm_year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC)) + 2000;
    systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC)) - 1;
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));

    u32 time = LL_RTC_TIME_Get(RTC);
    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));
    systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
    systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));

    getTimestamp();
    // DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
    //             systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
}
