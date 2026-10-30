
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "systick.h"
#include "uart.h"
#include "control_center.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void TIM6_DAC_IRQHandler(void)
{
    if (LL_TIM_IsActiveFlag_UPDATE(TIM6))
    {
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
    NVIC_SetPriority(TIM6_DAC_IRQn, 0);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

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
 * @brief  RTC中断事件
 * @param
 * @retval
 */
void RTC_IRQHandler(void)
{
    /* Get the Alarm interrupt source enable status */
    if (LL_RTC_IsEnabledIT_ALRA(RTC) != 0)
    {
        /* Get the pending status of the Alarm Interrupt */
        if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0)
        {
            /* Alarm callback */
            device_t.rtcWakeEvent = 1;
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
            ledBlueBlinkTime = SEC_DELAY / 2;
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
    while (LL_RTC_IsActiveFlag_RS(RTC) != 1)
    {
    }
    return RTC_ERROR_NONE;
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
    while (LL_RTC_IsActiveFlag_INIT(RTC) != 1)
    {
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
 * @brief  初始化RTC闹钟唤醒时钟 year,month,date:年(0~99),月(1~12),日(0~31),week:星期(1~7)
 * @param
 * @retval
 */
void RTC_AlarmInit(struct tim *alarmTime)
{
    LL_RTC_DateTypeDef rtc_date_initstruct;
    LL_RTC_TimeTypeDef rtc_time_initstruct;
    LL_RTC_AlarmTypeDef rtc_alarm_initstruct;

    LL_RTC_DeInit(RTC);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_EnableBkUpAccess();

#if RTC_CLOCK_SOURCE_LSE
    /* Enable LSE only if disabled.*/
    //  if (LL_RCC_LSE_IsReady() == 0)
    {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        LL_RCC_LSE_Enable();
        while (LL_RCC_LSE_IsReady() != 1)
        {
            while (!LL_SYSTICK_IsActiveCounterFlag())
                ;
        }
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

        /*##-3- Enable RTC peripheral Clocks #######################################*/
        /* Enable RTC Clock */
        LL_RCC_EnableRTC();
    }
#else
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

    /*##-3- Enable RTC peripheral Clocks #######################################*/
    /* Enable RTC Clock */
    LL_RCC_EnableRTC();
#endif

    /*##-4- Disable RTC registers write protection ##############################*/
    LL_RTC_DisableWriteProtection(RTC);

    /*##-5- Enter in initialization mode #######################################*/
    while (Enter_RTC_InitMode() != RTC_ERROR_NONE)
        ;

    /*##-6- Configure RTC ######################################################*/
    /* Configure RTC prescaler and RTC data registers */
    /* Set Hour Format */
    LL_RTC_SetHourFormat(RTC, LL_RTC_HOURFORMAT_24HOUR);
    /* Set Asynch Prediv (value according to source clock) */
    LL_RTC_SetAsynchPrescaler(RTC, RTC_ASYNCH_PREDIV);
    /* Set Synch Prediv (value according to source clock) */
    LL_RTC_SetSynchPrescaler(RTC, RTC_SYNCH_PREDIV);
    /* Set OutPut */
    /* Reset value is LL_RTC_ALARMOUT_DISABLE */
    // LL_RTC_SetAlarmOutEvent(RTC, LL_RTC_ALARMOUT_DISABLE);
    /* Set OutPutPolarity */
    /* Reset value is LL_RTC_OUTPUTPOLARITY_PIN_HIGH */
    // LL_RTC_SetOutputPolarity(RTC, LL_RTC_OUTPUTPOLARITY_PIN_HIGH);
    /* Set OutPutType */
    /* Reset value is LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN */
    // LL_RTC_SetAlarmOutputType(RTC, LL_RTC_ALARM_OUTPUTTYPE_OPENDRAIN);

    /*##-7- Exit of initialization mode #######################################*/
    Exit_RTC_InitMode();

    /*##-8- Enable RTC registers write protection #############################*/
    LL_RTC_EnableWriteProtection(RTC);

    LL_RTC_DisableWriteProtection(RTC);
    while (Enter_RTC_InitMode() != RTC_ERROR_NONE)
        ;

    /*## Configure the Date ################################################*/

    rtc_date_initstruct.WeekDay = systime.tm_wday==0?7:systime.tm_wday;
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

    /*## Configure the RTC Alarm peripheral #################################*/

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

    /* RTC Alarm Interrupt Configuration: EXTI configuration */
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);

    /*## Configure the NVIC for RTC Alarm ##################################*/
    NVIC_SetPriority(RTC_IRQn, 0x0F);
    NVIC_EnableIRQ(RTC_IRQn);

    while (Exit_RTC_InitMode() != RTC_ERROR_NONE)
        ;
    /*##-8- Enable RTC registers write protection #############################*/
    LL_RTC_EnableWriteProtection(RTC);

    /* Disable RTC registers write protection */
    LL_RTC_DisableWriteProtection(RTC);

    LL_RTC_ALMA_Disable(RTC);
    /* Enable Alarm*/
    LL_RTC_ALMA_Enable(RTC);

    /* Enable IT Alarm */
    LL_RTC_EnableIT_ALRA(RTC);

    /* Enable RTC registers write protection */
    LL_RTC_EnableWriteProtection(RTC);

    /* Clear the Alarm interrupt pending bit */
    LL_RTC_ClearFlag_ALRA(RTC);

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
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    LL_LPM_EnableDeepSleep();

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
    DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{

    systime.tm_wday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC))==7 ? 0:__LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC));
    systime.tm_year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC)) + 2000;
    systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC)) - 1;
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));

    u32 time = LL_RTC_TIME_Get(RTC);
    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));
    systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
    systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));
    //systime.tm_wday = whatday(systime.tm_year, systime.tm_mon+1, systime.tm_mday);

    getTimestamp();

    DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
}

/**
 * @brief  设置下次自动唤醒时间(最大设置间隔18个小时 < 1080 分钟)
 * @param
 * @retval
 */
void lpmSleepTimeInit(void)
{
    struct tim alarm_time = {0};
    struct tm timeinfo = {0};
    time_t stamp = Timestamp, t_stamp = 0;

    /* 唤醒间隔处理 */
    if (device_t.run_mode == 1 && Timestamp > 1659196800 &&
        systime.tm_hour * 60 + systime.tm_min >= device_t.c_time.start_hour * 60 + device_t.c_time.start_min &&
        systime.tm_hour * 60 + systime.tm_min <= device_t.c_time.end_hour * 60 + device_t.c_time.end_min)
    {
        stamp = report_stamp;
    }
    else
    {
        /* 判断下次监控时间段与下次心跳间隔的差值，取偏小值 */
        struct tm t = {0};
        memcpy(&t, &systime, sizeof(t));

        t.tm_year = systime.tm_year - 1900;
        t.tm_mon = systime.tm_mon;
        t.tm_hour = device_t.c_time.start_hour;
        t.tm_min = device_t.c_time.start_min;
        t.tm_sec = 0;
        t.tm_wday = systime.tm_wday;
        t.tm_isdst = -1;
        t_stamp = mktime(&t) - 8 * 60 * 60;

        if (t_stamp < Timestamp) // 当天监控时间已过
        {
            t_stamp += 24 * 60 * 60;
        }
        if (heart_stamp > t_stamp && device_t.run_mode == 1)
            stamp = t_stamp;
        else
            stamp = heart_stamp;
    }
    if (reconnect_stamp < stamp && !LoRaWAN.joinState)
    {
        stamp = reconnect_stamp;
    }
    stamp += 8 * 60 * 60;
    DEBUG_TRACE(LOG_TAG, "Lpm Sleep Time Intervel Stamp: %ld, Report Stamp: %ld, Heart Stamp: %ld", t_stamp, report_stamp, heart_stamp);
    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    alarm_time.tm_hour = timeinfo.tm_hour;
    alarm_time.tm_min = timeinfo.tm_min;
    alarm_time.tm_sec = timeinfo.tm_sec;
    //RTC_AlarmInit(&alarm_time);
    setAlmaTime(alarm_time.tm_hour,alarm_time.tm_min,alarm_time.tm_sec);
    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d",
                alarm_time.tm_hour, alarm_time.tm_min, alarm_time.tm_sec,
                systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
}
