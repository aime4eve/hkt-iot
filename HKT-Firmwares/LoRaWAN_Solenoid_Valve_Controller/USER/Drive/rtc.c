
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "real_time.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>
#include <LoRaWAN_ATCMD.h>

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
 * @brief  RTC闹钟中断事件
 * @param
 * @retval
 */

void RTC_Alarm_IRQHandler(void)
{
    /* Get the Alarm interrupt source enable status */
    if (LL_RTC_IsEnabledIT_ALRA(RTC) != 0) {
        /* Get the pending status of the Alarm Interrupt */
        if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
            /* Alarm callback */
            device_t.rtc_wake_event = 1;
            device_t.sleep_delay    = WAIT_SLEEP_TIME_DELAY;

            /* Clear the Alarm interrupt pending bit */
            LL_RTC_ClearFlag_ALRA(RTC);
            LL_RTC_DisableWriteProtection(RTC);
            LL_RTC_ALMA_Disable(RTC);
            LL_RTC_EnableWriteProtection(RTC);
        }
    }
    /* Clear the EXTI's Flag for RTC Alarm */
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_18);
}

/**
 * @brief  配置RTC时间
 * @param
 * @retval
 */
void RTC_TimeSet(void)
{
    LL_RTC_DateTypeDef rtc_date_initstruct = {0};
    LL_RTC_TimeTypeDef rtc_time_initstruct = {0};

    rtc_date_initstruct.WeekDay = systime.tm_wday;
    rtc_date_initstruct.Day     = systime.tm_mday;
    rtc_date_initstruct.Month   = systime.tm_mon + 1;
    rtc_date_initstruct.Year    = systime.tm_year - 2000;
    /* Initialize RTC date according to parameters defined in initialization structure. */
    LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_date_initstruct);

    rtc_time_initstruct.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
    rtc_time_initstruct.Hours      = systime.tm_hour;
    rtc_time_initstruct.Minutes    = systime.tm_min;
    rtc_time_initstruct.Seconds    = systime.tm_sec;
    /* Initialize RTC time according to parameters defined in initialization structure. */
    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_time_initstruct);

#define RTC_BKP_DATE_TIME_UPDTATED ((uint32_t)0x32F2)
    LL_RTC_BAK_SetRegister(RTC, LL_RTC_BKP_DR0, RTC_BKP_DATE_TIME_UPDTATED);

    sysTimeSwitchToRtcTime();
}

/**
 * @brief  初始化RTC时间
 * @param
 * @retval
 */
void RTC_Init(void)
{
    LL_RTC_DateTypeDef rtc_date_initstruct = {0};
    LL_RTC_TimeTypeDef rtc_time_initstruct = {0};

#if FUNC_RCC_BYPASS_ENABLE
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
        FlagStatus pwrclkchanged = RESET;
        /* Update LSE configuration in Backup Domain control register */
        /* Requires to enable write access to Backup Domain if necessary */
        if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR) != 1U) {
            /* Enables the PWR Clock and Enables access to the backup domain */
            LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
            pwrclkchanged = SET;
        }
        if (LL_PWR_IsEnabledBkUpAccess() != 1U) {
            /* Enable write access to Backup domain */
            LL_PWR_EnableBkUpAccess();
            while (LL_PWR_IsEnabledBkUpAccess() == 0U) {
            }
        }
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        LL_RCC_LSE_EnableBypass();
        LL_RCC_LSE_Enable();

        /* Wait till LSE is ready */
        while (LL_RCC_LSE_IsReady() != 1) {
        }
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
        /* Restore clock configuration if changed */
        if (pwrclkchanged == SET) {
            LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
        }
    }
#else
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSI) {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    }
#endif
    // if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
    //     FlagStatus pwrclkchanged = RESET;
    //     /* Update LSE configuration in Backup Domain control register */
    //     /* Requires to enable write access to Backup Domain if necessary */
    //     if (LL_APB1_GRP1_IsEnabledClock(LL_APB1_GRP1_PERIPH_PWR) != 1U) {
    //         /* Enables the PWR Clock and Enables access to the backup domain */
    //         LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    //         pwrclkchanged = SET;
    //     }
    //     if (LL_PWR_IsEnabledBkUpAccess() != 1U) {
    //         /* Enable write access to Backup domain */
    //         LL_PWR_EnableBkUpAccess();
    //         while (LL_PWR_IsEnabledBkUpAccess() == 0U) {
    //         }
    //     }
    //     LL_RCC_ForceBackupDomainReset();
    //     LL_RCC_ReleaseBackupDomainReset();
    //     LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
    //     LL_RCC_LSE_Enable();

    //     /* Wait till LSE is ready */
    //     while (LL_RCC_LSE_IsReady() != 1) {
    //     }
    //     LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    //     /* Restore clock configuration if changed */
    //     if (pwrclkchanged == SET) {
    //         LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_PWR);
    //     }
    // }
    /* Enable RTC Clock */
    LL_RCC_EnableRTC();

    NVIC_SetPriority(RTC_Alarm_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(RTC_Alarm_IRQn);

    /* Configure RTC prescaler and RTC data registers */
    /* Set Hour Format */
    LL_RTC_SetHourFormat(RTC, LL_RTC_HOURFORMAT_24HOUR);
    /* Set Asynch Prediv (value according to source clock) */
    LL_RTC_SetAsynchPrescaler(RTC, RTC_ASYNCH_PREDIV);
    /* Set Synch Prediv (value according to source clock) */
    LL_RTC_SetSynchPrescaler(RTC, RTC_SYNCH_PREDIV);

    RTC_TimeSet();
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
    LL_RTC_EnableWriteProtection(RTC);

    /* Set Alarm */
    rtc_alarm_initstruct.AlarmTime.TimeFormat = LL_RTC_ALMA_TIME_FORMAT_AM;
    // rtc_alarm_initstruct.AlarmTime.Hours = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_hour);
    // rtc_alarm_initstruct.AlarmTime.Minutes = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_min);
    // rtc_alarm_initstruct.AlarmTime.Seconds = __LL_RTC_CONVERT_BIN2BCD(alarmTime->tm_sec);
    rtc_alarm_initstruct.AlarmTime.Hours   = alarmTime->tm_hour;
    rtc_alarm_initstruct.AlarmTime.Minutes = alarmTime->tm_min;
    rtc_alarm_initstruct.AlarmTime.Seconds = alarmTime->tm_sec;

    /* RTC Alarm Generation: Alarm on Hours, Minutes and Seconds (ignore date/weekday)*/
    rtc_alarm_initstruct.AlarmMask           = LL_RTC_ALMA_MASK_DATEWEEKDAY; // 设置不匹配日期
    rtc_alarm_initstruct.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_WEEKDAY;
    rtc_alarm_initstruct.AlarmDateWeekDay    = 0x1;

    /* Initialize ALARM A according to parameters defined in initialization structure. */
    LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm_initstruct);

    LL_RTC_DisableWriteProtection(RTC);
    /* Enable Alarm*/
    LL_RTC_ALMA_Enable(RTC);

    /* Clear the Alarm interrupt pending bit */
    LL_RTC_ClearFlag_ALRA(RTC);

    /* Enable IT Alarm */
    LL_RTC_EnableIT_ALRA(RTC);
    LL_RTC_EnableWriteProtection(RTC);

    /* RTC Alarm Interrupt Configuration: EXTI configuration */
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_18);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_18);

    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d",
                alarmTime->tm_hour, alarmTime->tm_min, alarmTime->tm_sec,
                systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
}

/**
 * @brief  Function to enter in specical mode.
 * @param  None
 * @retval None
 */
void EnterSleepMode(void)
{
    /* Enable Power Clock */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    /* Ensure that MSI is wake-up system clock from low-power mode Stop */
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);

    /* Set STOP2 mode when CPU enters deepsleep */
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP2);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    LL_LPM_EnableDeepSleep();

    /* Request Wait For Interrupt */
    __WFI();
}

/**
 * @brief  设置下次自动唤醒时间
 * @param
 * @retval
 */
void lpmSleepTimeInit(void)
{
    struct tm timeinfo = {0};
    time_t    stamp    = dataReportTimestamp;

    /* 唤醒间隔处理 */
    read_real_time();
    RTC_TimeSet();

    // 判断本地任务时间
    if (local_schedule_t.task_start_timestamp > 0) {
        if (local_schedule_t.task_start_timestamp < stamp && local_schedule_t.task_start_timestamp > Timestamp) {
            stamp = local_schedule_t.task_start_timestamp;
        } else {
            if (stamp > local_schedule_t.task_over_timestamp && local_schedule_t.task_over_timestamp > Timestamp)
                stamp = local_schedule_t.task_over_timestamp;
        }
    }
    if (real_time_task_t.is_start) {
        if (stamp > real_time_task_t.task_over_timestamp && real_time_task_t.task_over_timestamp > Timestamp)
            stamp = real_time_task_t.task_over_timestamp;
    }

    // 判断上报周期
    if (dataReportTimestamp < stamp && device_t.report_interval > 0 && dataReportTimestamp > Timestamp) {
        stamp = dataReportTimestamp;
    }

    // 判断重新入网时间
    if (!LoRaWAN.joinState && device_t.power_on && reconnect_stamp > Timestamp) {
        if (stamp > reconnect_stamp) {
            stamp = reconnect_stamp;
        }
    }

    struct tm time = {0};
    time_t    raw;
    time_t    zonestamp = device_t.timezone * 60 * 60;

    // 0点时间计算判断
    {
        //        struct tm time = {0};
        //        time_t raw;
        //        time_t zonestamp = device_t.timezone * 60 * 60;

        localtime_r(&Timestamp, &time);
        time.tm_hour = 0;
        time.tm_min  = 0;
        time.tm_sec  = 0;

        raw           = mktime(&time);
        zeroTimestamp = raw - zonestamp;

        if (zeroTimestamp < Timestamp) {
            zeroTimestamp += 86400;
        }
    }

    if (zeroTimestamp < stamp && zeroTimestamp > Timestamp) {
        stamp               = zeroTimestamp;
        device_t.zero_event = 1;
    }

    stamp += zonestamp;
    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    RTC_AlarmInit(&timeinfo);
}

void IWDG_Init(void)
{
    /* Configure the IWDG with window option disabled */
    /* ------------------------------------------------------- */
    /* (1) Enable the IWDG by writing 0x0000 CCCC in the IWDG_KR register */
    /* (2) Enable register access by writing 0x0000 5555 in the IWDG_KR register */
    /* (3) Write the IWDG prescaler by programming IWDG_PR from 0 to 7 - LL_IWDG_PRESCALER_4 (0) is lowest divider*/
    /* (4) Write the reload register (IWDG_RLR) */
    /* (5) Wait for the registers to be updated (IWDG_SR = 0x0000 0000) */
    /* (6) Refresh the counter value with IWDG_RLR (IWDG_KR = 0x0000 AAAA) */
    LL_IWDG_Enable(IWDG);                              /* (1) */
    LL_IWDG_EnableWriteAccess(IWDG);                   /* (2) */
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256); /* (3) */
    LL_IWDG_SetReloadCounter(IWDG, 0xFFFF);            /* (4) */
    while (LL_IWDG_IsReady(IWDG) != 1)                 /* (5) */
    {
    }
    LL_IWDG_ReloadCounter(IWDG); /* (6) */

    __HAL_DBGMCU_FREEZE_WWDG();
    __HAL_DBGMCU_FREEZE_IWDG(); // 调试模式时冻结看门狗

    //	 __HAL_DBGMCU_UNFREEZE_WWDG();
    //	 __HAL_DBGMCU_UNFREEZE_IWDG(); //调试模式时开启看门狗

    FLASH_OBProgramInitTypeDef OptionsBytesStruct;
    /* Get the configuration status */
    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
    if ((OptionsBytesStruct.USERConfig & (FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)) != 0) {
        // 没有配置过就配置一次，有必要判断一下是否配置过，因为每次配置完都会导致重启，不能每次上电都无条件配置一次
        OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
        OptionsBytesStruct.USERType   = OB_USER_IWDG_STOP | OB_USER_IWDG_SW;
        OptionsBytesStruct.USERConfig &= (~(FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)); // STOP模式下停止看门狗计数
        HAL_FLASH_Unlock();
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
        HAL_FLASH_OB_Unlock();
        if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) != HAL_OK) {
            // 配置失败，都说重启大法好，我就重启下试试...
            NVIC_SystemReset();
        }
        if (HAL_FLASH_OB_Launch() != HAL_OK) // 加载flash配置，这里会导致重启
        {
            NVIC_SystemReset();
        }
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
    }
}
