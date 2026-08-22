
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
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void TIM7_IRQHandler(void)
{
    static u8 recvData;
    if (LL_TIM_IsActiveFlag_UPDATE(TIM7)) {
        LL_TIM_ClearFlag_UPDATE(TIM7);

        recvStat++;
        // COM_D7_BIT
        if (recvStat == COM_STOP_BIT) {
            // 到这里接收完成一个字节数据
            LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_6); // 开启下降沿中断
            LL_TIM_DisableCounter(TIM7);           // 关闭定时器
            LL_TIM_DisableIT_UPDATE(TIM7);

            LL_TIM_SetCounter(TIM7, 0); // 定时器清零
            LL_TIM_EnableIT_UPDATE(TIM7);
            LL_TIM_EnableCounter(TIM7);
            if (uartGps.recvLen < uart_recv_max_len) {
                uartGps.recvData[uartGps.recvLen++] = recvData; // 存入缓冲区
                uartGps.recvTimeout = 50;                       // 50ms超时
            }
            recvData = 0;
            return;
        }
        if (GPIO_GPS_RXD_STA) {
            recvData |= (1 << (recvStat - 1));
        } else {
            recvData &= ~(1 << (recvStat - 1));
        }
    }
}

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void LPTIM1_IRQHandler(void)
{
    if (LL_LPTIM_IsActiveFlag_ARRM(LPTIM1)) {
        LL_LPTIM_ClearFLAG_ARRM(LPTIM1);
        if (device_t.sleep_state)
            device_t.lptime_wake_event = 1;
    }
}

/**
 * @brief  初始化低功耗定时器，固定唤醒周期8s
 * @param
 * @retval
 */
void LPTIM1_Init(void)
{
    // LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);
    LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);

    /* LPTIM1 interrupt Init */
    NVIC_SetPriority(LPTIM1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(LPTIM1_IRQn);

    LL_LPTIM_Disable(LPTIM1);
    LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
    LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV8);
    LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
    LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
    LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
    LL_LPTIM_TrigSw(LPTIM1);

    /* Enable the Peripheral */
    LL_LPTIM_Enable(LPTIM1);

    // LL_LPTIM_SetAutoReload(LPTIM1, 32768);
    LL_LPTIM_SetAutoReload(LPTIM1, 32000);

    /* Clear flag */
    LL_LPTIM_ClearFLAG_ARRM(LPTIM1); // Clear the autoreload register update interrupt flag
    LL_LPTIM_EnableIT_ARRM(LPTIM1);
    LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
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
 * @brief  时间戳转时间
 * @param
 * @retval
 */
void convertTimestampToSystime(void)
{
    time_t rawtime = Timestamp + (8 * 60 * 60);
    struct tm *info = localtime(&rawtime);
    memcpy(&systime, info, sizeof(systime));
    systime.tm_year += 1900;
    DEBUG_TRACE(LOG_TAG, "Timezone[%d] Conver Timestamp To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", 8, systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{
    // u32 time = LL_RTC_TIME_Get(RTC);
    // systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));
    // systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
    // systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));

    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetHour(RTC));
    systime.tm_min = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetMinute(RTC));
    systime.tm_sec = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_TIME_GetSecond(RTC));
    systime.tm_year = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetYear(RTC)) + 2000;
    systime.tm_mon = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetMonth(RTC)) - 1;
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetDay(RTC));
    systime.tm_wday = __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC)) == 7 ? 0 : __LL_RTC_CONVERT_BCD2BIN(LL_RTC_DATE_GetWeekDay(RTC));

    getTimestamp();
    DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
}
