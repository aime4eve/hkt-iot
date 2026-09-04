
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"
#include "systick.h"
#include "uart.h"
#include "control_center.h"
#include "gps.h"
#include "communicate.h"

#include <LoRaWAN_APPLY.h>

/**
 * @brief RTC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspInit 0 */

        /* USER CODE END RTC_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_RTC_ENABLE();
        /* RTC interrupt Init */
        HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(RTC_IRQn);
        /* USER CODE BEGIN RTC_MspInit 1 */

        /* USER CODE END RTC_MspInit 1 */
    }
}

/**
 * @brief RTC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc)
{
    if (hrtc->Instance == RTC)
    {
        /* USER CODE BEGIN RTC_MspDeInit 0 */

        /* USER CODE END RTC_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();

        /* RTC interrupt DeInit */
        HAL_NVIC_DisableIRQ(RTC_IRQn);
        /* USER CODE BEGIN RTC_MspDeInit 1 */

        /* USER CODE END RTC_MspDeInit 1 */
    }
}

/**
 * @brief  RTC中断事件
 * @param
 * @retval
 */
void RTC_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&hrtc);
    HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}

uint32_t count = 0;
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    count++;
    // INFO("\r\nRTC AlarmA Enter Count%d\r\n", count);
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    count++;
    // INFO("\r\nRTC WakeUp Enter Count%d\r\n", count);
}

/**
 * @brief  初始化RTC闹钟唤醒时钟 year,month,date:年(0~99),月(1~12),日(0~31),week:星期(1~7)
 * @param
 * @retval
 */
void RTC_AlarmInit(struct tm *alarmTime)
{
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;
    RTC_AlarmTypeDef salarmstructure;

    /*##-1- Configure the RTC peripheral #######################################*/
    /* Configure RTC prescaler and RTC data registers */
    /* RTC configured as follow:
        - Hour Format    = Format 24
        - Asynch Prediv  = Value according to source clock
        - Synch Prediv   = Value according to source clock
        - OutPut         = Output Disable
        - OutPutPolarity = High Polarity
        - OutPutType     = Open Drain */
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    hrtc.Init.SynchPrediv = RTC_SYNCH_PREDIV;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&hrtc) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-2- Configure the RTC Alarm peripheral #################################*/
    salarmstructure.Alarm = RTC_ALARM_A;
    salarmstructure.AlarmDateWeekDay = RTC_WEEKDAY_MONDAY;
    salarmstructure.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    salarmstructure.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
    salarmstructure.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
    salarmstructure.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;

    salarmstructure.AlarmTime.Hours = alarmTime->tm_hour;
    salarmstructure.AlarmTime.Minutes = alarmTime->tm_min;
    salarmstructure.AlarmTime.Seconds = alarmTime->tm_sec;
    salarmstructure.AlarmTime.SubSeconds = 0;

    if (HAL_RTC_SetAlarm_IT(&hrtc, &salarmstructure, RTC_FORMAT_BIN) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-3- Configure the Date #################################################*/
    sdatestructure.Year = systime.tm_year - 2000;
    sdatestructure.Month = systime.tm_mon + 1;
    sdatestructure.Date = systime.tm_mday - 1;
    sdatestructure.WeekDay = systime.tm_wday + 1;

    if (HAL_RTC_SetDate(&hrtc, &sdatestructure, RTC_FORMAT_BIN) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    /*##-4- Configure the Time #################################################*/
    stimestructure.Hours = systime.tm_hour;
    stimestructure.Minutes = systime.tm_min;
    stimestructure.Seconds = systime.tm_sec;
    stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&hrtc, &stimestructure, RTC_FORMAT_BIN) != HAL_OK)
    {
        /* Initialization Error */
        Error_Handler();
    }

    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);

    __HAL_RCC_RTC_ENABLE();

    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d",
                alarmTime->tm_hour, alarmTime->tm_min, alarmTime->tm_sec,
                systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec);
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
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
#endif
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{
    RTC_DateTypeDef sdatestructureget;
    RTC_TimeTypeDef stimestructureget;
    /* Get the RTC current Time */
    HAL_RTC_GetTime(&hrtc, &stimestructureget, RTC_FORMAT_BIN);
    /* Get the RTC current Date */
    HAL_RTC_GetDate(&hrtc, &sdatestructureget, RTC_FORMAT_BIN);

    systime.tm_year = sdatestructureget.Year + 2000;
    systime.tm_mon = sdatestructureget.Month - 1;
    systime.tm_mday = sdatestructureget.Date + 1;
    systime.tm_wday = sdatestructureget.WeekDay - 1;

    systime.tm_hour = stimestructureget.Hours;
    systime.tm_min = stimestructureget.Minutes;
    systime.tm_sec = stimestructureget.Seconds;
    systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);

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
    struct tm alarm_time = {0};
    struct tm timeinfo = {0};
    time_t stamp;

    /* 唤醒间隔处理 */

    // 取最短唤醒间隔
#if FUNC_GPS_ENABLE
    if (gps_stamp < sensor_stamp)
    {
        stamp = gps_stamp;
    }
    else
#endif
    {
        stamp = sensor_stamp;
    }
    if (reconnect_stamp < stamp && !LoRaWAN.joinState)
    {
        stamp = reconnect_stamp;
    }
    stamp += 8 * 60 * 60;

    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    alarm_time.tm_hour = timeinfo.tm_hour;
    alarm_time.tm_min = timeinfo.tm_min;
    alarm_time.tm_sec = timeinfo.tm_sec;
    RTC_AlarmInit(&alarm_time);
}
