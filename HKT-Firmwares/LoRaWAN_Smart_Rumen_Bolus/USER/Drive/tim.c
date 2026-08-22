
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void TIM6_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM6)) {
        LL_TIM_ClearFlag_UPDATE(TIM6);
        Systick_1ms_Handler();
    }
}

/**
 * @brief  初始化通用定时器
 * @param  period：计数周期（实际值）
 * @param  prescaler：预分频值（实际值）
 * @retval
 */
void TIM6_Init(u16 period, u16 prescaler)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);

    /* TIM6 interrupt Init */
    NVIC_SetPriority(TIM6_IRQn, 1);
    NVIC_EnableIRQ(TIM6_IRQn);

    TIM_InitStruct.Prescaler = prescaler - 1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = period - 1;
    LL_TIM_Init(TIM6, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM6);
    LL_TIM_SetTriggerOutput(TIM6, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM6);
    LL_TIM_EnableCounter(TIM6);
    LL_TIM_EnableUpdateEvent(TIM6);
    LL_TIM_EnableIT_UPDATE(TIM6);
}

/**
 * @brief  RTC中断事件
 * @param
 * @retval
 */
void RTC_IRQHandler(void)
{
    /* Get the Alarm interrupt source enable status */
    if (LL_RTC_IsEnabledIT_ALRA(RTC) != 0) {
        /* Get the pending status of the Alarm Interrupt */
        if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
            /* Alarm callback */
            device_t.rtcWakeEvent = 1;
            // if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            //     device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            /* Clear the Alarm interrupt pending bit */
            LL_RTC_ClearFlag_ALRA(RTC);

            LL_RTC_DisableWriteProtection(RTC);
            LL_RTC_ALMA_Disable(RTC);
            LL_RTC_EnableWriteProtection(RTC);
        }
    }
    /* Clear the EXTI's Flag for RTC Alarm */
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
}

/**
 * @brief  Wait until the RTC Time and Date registers (RTC_TR and RTC_DR) are
 *         synchronized with RTC APB clock.
 * @param  None
 * @retval RTC_ERROR_NONE if no error (RTC_ERROR_TIMEOUT will occur if RTC is
 *         not synchronized)
 */
uint32_t WaitForSynchro_RTC(void)
{
    /* Clear RSF flag */
    LL_RTC_ClearFlag_RS(RTC);

    /* Wait the registers to be synchronised */
    while (LL_RTC_IsActiveFlag_RS(RTC) != 1) {
    }
    return RTC_ERROR_NONE;
}

/**
 * @brief   获取RTC日期、时间，并将BCD格式的时间转换为bin格式
 * @param   RtcDate:日期（*）
 * @param   RtcTime：时间（*）
 * @retval
 **/
void getBcdTime2BinTime(LL_RTC_DateTypeDef *RtcDate, LL_RTC_TimeTypeDef *RtcTime)
{
    RtcTime->Hours = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
    RtcTime->Minutes = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
    RtcTime->Seconds = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
    RtcDate->Year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC));
    RtcDate->Month = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC));
    RtcDate->Day = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
    RtcDate->WeekDay = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC));
}

/**
 * @brief  Enter in initialization mode
 * @note In this mode, the calendar counter is stopped and its value can be updated
 * @param  None
 * @retval RTC_ERROR_NONE if no error
 */
uint32_t Enter_RTC_InitMode(void)
{
    /* Set Initialization mode */
    LL_RTC_EnableInitMode(RTC);
    /* Check if the Initialization mode is set */
    while (LL_RTC_IsActiveFlag_INIT(RTC) != 1) {
    }
    return RTC_ERROR_NONE;
}

/**
 * @brief  Exit Initialization mode
 * @param  None
 * @retval RTC_ERROR_NONE if no error
 */
uint32_t Exit_RTC_InitMode(void)
{
    LL_RTC_DisableInitMode(RTC);

    /* Wait for synchro */
    /* Note: Needed only if Shadow registers is enabled           */
    /*       LL_RTC_IsShadowRegBypassEnabled function can be used */
    return (WaitForSynchro_RTC());
}

/**
 * @brief  初始化RTC时间
 * @param
 * @retval
 */
void RTC_Init(void)
{
    LL_RTC_DateTypeDef rtc_date_initstruct;
    LL_RTC_TimeTypeDef rtc_time_initstruct;

    LL_RTC_DeInit(RTC);

    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    LL_RCC_EnableRTC();

    /*##-4- Disable RTC registers write protection ##############################*/
    LL_RTC_DisableWriteProtection(RTC);

    /*##-5- Enter in initialization mode #######################################*/
    if (Enter_RTC_InitMode() != RTC_ERROR_NONE) {
        ;
    }
    /*##-6- Configure RTC ######################################################*/
    /* Configure RTC prescaler and RTC data registers */
    /* Set Hour Format */
    LL_RTC_SetHourFormat(RTC, LL_RTC_HOURFORMAT_24HOUR);
    /* Set Asynch Prediv (value according to source clock) */
    LL_RTC_SetAsynchPrescaler(RTC, RTC_ASYNCH_PREDIV);
    /* Set Synch Prediv (value according to source clock) */
    LL_RTC_SetSynchPrescaler(RTC, RTC_SYNCH_PREDIV);
    /*## Configure the Date ################################################*/
    rtc_date_initstruct.WeekDay = systime.tm_wday;
    rtc_date_initstruct.Day = __LL_RTC_CONVERT_BIN2BCD(systime.tm_mday);
    rtc_date_initstruct.Month = __LL_RTC_CONVERT_BIN2BCD(systime.tm_mon + 1);
    rtc_date_initstruct.Year = __LL_RTC_CONVERT_BIN2BCD(systime.tm_year - 2000);

    /* Initialize RTC date according to parameters defined in initialization structure. */
    LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BCD, &rtc_date_initstruct);

    /*## Configure the Time ################################################*/

    rtc_time_initstruct.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
    rtc_time_initstruct.Hours = __LL_RTC_CONVERT_BIN2BCD(systime.tm_hour);
    rtc_time_initstruct.Minutes = __LL_RTC_CONVERT_BIN2BCD(systime.tm_min);
    rtc_time_initstruct.Seconds = __LL_RTC_CONVERT_BIN2BCD(systime.tm_sec);

    /* Initialize RTC time according to parameters defined in initialization structure. */
    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &rtc_time_initstruct);

    /*##-7- Exit of initialization mode #######################################*/
    Exit_RTC_InitMode();

    /*##-8- Enable RTC registers write protection #############################*/
    LL_RTC_EnableWriteProtection(RTC);
    // LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR1, RTC_BKP_DATE_TIME_UPDTATED);
}

/**
 * @brief  初始化RTC闹钟唤醒时钟
 * @param
 * @retval
 */
void RTC_AlarmInit(struct tm *alarmTime)
{
    LL_RTC_AlarmTypeDef rtc_alarm_initstruct;

    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ALMA_Disable(RTC);
    LL_RTC_DisableIT_ALRA(RTC);
    LL_RTC_EnableWriteProtection(RTC);

    /* Set Alarm */
    rtc_alarm_initstruct.AlarmTime.TimeFormat = LL_RTC_ALMA_TIME_FORMAT_AM;
    rtc_alarm_initstruct.AlarmTime.Hours = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_hour);
    rtc_alarm_initstruct.AlarmTime.Minutes = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_min);
    rtc_alarm_initstruct.AlarmTime.Seconds = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_sec);

    /* RTC Alarm Generation: Alarm on Hours, Minutes and Seconds (ignore date/weekday)*/
    rtc_alarm_initstruct.AlarmMask = LL_RTC_ALMA_MASK_DATEWEEKDAY; // 设置不匹配日期
    // rtc_alarm_initstruct.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
    // rtc_alarm_initstruct.AlarmDateWeekDay = 0x01;

    /* Initialize ALARM A according to parameters defined in initialization structure. */
    LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BCD, &rtc_alarm_initstruct);

    LL_RTC_DisableWriteProtection(RTC);
    /* Enable Alarm*/
    LL_RTC_ALMA_Enable(RTC);

    /* Clear the Alarm interrupt pending bit */
    LL_RTC_ClearFlag_ALRA(RTC);

    /* Enable IT Alarm */
    LL_RTC_EnableIT_ALRA(RTC);

    /* RTC Alarm Interrupt Configuration: EXTI configuration */
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);

    /*##-6- Configure the NVIC for RTC Alarm ###############################*/
    NVIC_SetPriority(RTC_IRQn, 0x0F);
    NVIC_EnableIRQ(RTC_IRQn);

    LL_RTC_EnableWriteProtection(RTC);
    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d",
                alarmTime->tm_hour, alarmTime->tm_min, alarmTime->tm_sec,
                systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
}

/**
 * @brief  Function to enter in Stop mode.
 * @param  None
 * @retval None
 */
void EnterStopMode(void)
{
    /** Request to enter Stop mode
     * Following procedure describe in STM32L0xx Reference Manual
     * See PWR part, section Low-power modes, Stop mode
     */
    /* Enable ultra low power mode */
    LL_PWR_EnableUltraLowPower();
    /** Set the regulator to low power before setting MODE_STOP.
     * If the regulator remains in "main mode", it consumes
     * more power without providing any additional feature. */
    LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);
    /* Set STOP mode when CPU enters deepsleep */
    LL_PWR_ClearFlag_WU();
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    LL_LPM_EnableDeepSleep();

    /* Request Wait For Interrupt */
    __WFI();
}

/**
 * @brief  Function to configure and enter in STANDBY Mode.
 * @param  None
 * @retval None
 */
void EnterStandbyMode(void)
{
    /* Clear Standby flag*/
    LL_PWR_ClearFlag_SB();

    /* Disable all used wakeup sources */
    LL_PWR_DisableWakeUpPin(LL_PWR_WAKEUP_PIN1);

    /* Clear all wake up Flag */
    LL_PWR_ClearFlag_WU();

    /* Enable wakeup pin */
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);

    /* As default User push-button state is high level, need to clear all wake up Flag again */
    LL_PWR_ClearFlag_WU();

    /* Enable ultra low power mode */
    LL_PWR_EnableUltraLowPower();

    /** Request to enter STANDBY mode
     * Following procedure describe in STM32L0xx Reference Manual
     * See PWR part, section Low-power modes, Standby mode
     */
    /* Set STANDBY mode when CPU enters deepsleep */
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    LL_LPM_EnableDeepSleep();

    /* This option is used to ensure that store operations are completed */
#if defined(__CC_ARM)
    __force_stores();
#endif
    /* Request Wait For Interrupt */
    __WFI();
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
    // DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
    //             systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{
    systime.tm_wday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC)) == 7 ? 0 : __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC));
    systime.tm_year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC)) + 2000;
    systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC)) - 1;
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));

    u32 time = LL_RTC_TIME_Get(RTC);
    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));
    systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
    systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));

    getTimestamp();

    // DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
    //             systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
}

/**
 * @brief  设置下次自动唤醒时间(最大设置间隔18个小时 < 1080 分钟)
 * @param
 * @retval
 */
void lpmSleepTimeInit(void)
{
    struct tm timeinfo = {0};
    time_t stamp = dataConvertTimestamp;

    /* 唤醒间隔处理 */
    if (!LoRaWAN.joinState && reconnect_stamp > Timestamp) {
        stamp = reconnect_stamp;
    }
    if (dataConvertTimestamp < stamp) {
        stamp = dataConvertTimestamp;
    }
    if (dataReportTimestamp < stamp && dataReportTimestamp > Timestamp) {
        stamp = dataReportTimestamp;
    }
    if (stamp < Timestamp + 3) {
        stamp = Timestamp + 5;
    }
    stamp += 8 * 60 * 60;
    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    RTC_AlarmInit(&timeinfo);
}
