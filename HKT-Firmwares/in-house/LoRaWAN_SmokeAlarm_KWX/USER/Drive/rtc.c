
/* Includes ------------------------------------------------------------------*/
#include "rtc.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "systick.h"
#include "uart.h"

/* Private variables ---------------------------------------------------------------------------------------*/
u8 Day_Per_Month[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Global variables ----------------------------------------------------------------------------------------*/
rtc_time_t DateTime, CurTime;
vu8 CK_SECOND_Flag = 0;

u32 wake_time = 30;

/**
 * @brief  RTC中断服务入口
 * @param
 * @retval
 */
void RTC_IRQHandler(void)
{
    u8 bFlags = RTC_GetFlagStatus();
    device_t.rtc_event = 1;

    if (bFlags & RTC_FLAG_CM) {

        Timestamp += wake_time;

        // 周期上报
        if (device_t.report_interval) {
            if (device_t.report_interval_delay > wake_time) {
                device_t.report_interval_delay -= wake_time;
            } else {
                device_t.report_interval_delay = device_t.report_interval * 60;
                device_t.sync_state = 1;
                if (device_t.sleep_delay < 3)
                    device_t.sleep_delay = 3;
            }
        }
        if (reconnect_stamp <= Timestamp && LoRaWAN.joinEvent == LoRaWAN_JOINEVENT_IDLE && LoRaWAN.status == LoRaWAN_STATUS_NET_ERROR) {
            device_t.sleep_delay = 300;
        }
        wake_time = 60;
        if (wake_time > reconnect_stamp && !LoRaWAN.joinState && reconnect_stamp > 0)
            wake_time = reconnect_stamp;
        if (wake_time > device_t.report_interval_delay && device_t.report_interval_delay > 0)
            wake_time = device_t.report_interval_delay;
        RTC_SetCompare(wake_time);
    }
}

/**
 * @brief  Determine if a year is a leap year or not.
 * @param  year: a specific year.
 * @retval The status of the leap year check, TRUE or FALSE.
 */
static bool IsLeapYear(u32 year)
{
    if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
        return TRUE;
    else
        return FALSE;
}

/**
 * @brief  根据2014.1.1 00:00:00计算时间，并将秒的总和返回
 * @param  time: 时间结构体.
 * @retval 计算完成后的秒数
 */
u32 RTC_Calculate_Sec(rtc_time_t *time)
{
    u32 i, temp, secsum = 0;

    temp = time->year - 1;
    for (i = 0; i < (time->year - 2014); i++) {
        if (IsLeapYear(temp--) == TRUE) {
            secsum += (366 * 86400);
        } else {
            secsum += (365 * 86400);
        }
    }

    temp = 1;
    for (i = 0; i < (time->month - 1); i++) {
        if (temp == 2) {
            if (IsLeapYear(time->year) == TRUE)
                secsum += (29 * 86400);
            else
                secsum += (28 * 86400);
        } else {
            secsum += (Day_Per_Month[temp] * 86400);
        }
        temp++;
    }

    secsum += ((time->day - 1) * 86400);
    secsum += (time->hour * 3600);
    secsum += (time->minute * 60);
    secsum += (time->second);
    return secsum;
}

/**
 * @brief Configures RTC.
 * @retval None
 * @details
 *    - Enable the RTC Second interrupt.
 *    - RTC prescaler = 32768 to generate 1 second interrupt.
 */
void RTC_Configuration(void)
{
    /* NVIC configuration                                                                                     */
    NVIC_EnableIRQ(RTC_IRQn);
    /* Reset Backup Domain                                                                                    */
    PWRCU_DeInit();
#if 1
    /* Configure Low Speed External clock (LSE)                                                               */
    RTC_LSESMConfig(RTC_LSESM_FAST);
    // RTC_LSESMConfig(RTC_LSESM_NORMAL);
    RTC_LSECmd(ENABLE);
    while (CKCU_GetClockReadyStatus(CKCU_FLAG_LSERDY) == RESET)
        ;
    /* Configure the RTC                                                                                      */
    RTC_ClockSourceConfig(RTC_SRC_LSE);
#else
    RTC_ClockSourceConfig(RTC_SRC_LSI);
#endif
    RTC_IntConfig(RTC_INT_CM, ENABLE);
    RTC_WakeupConfig(RTC_WAKEUP_CM, ENABLE);
    RTC_SetPrescaler(RTC_RPRE_32768);
    RTC_CMPCLRCmd(ENABLE);
    // DateTime.year = 2022;
    // DateTime.month = 10;
    // DateTime.day = 24;
    // DateTime.hour = 0;
    // DateTime.minute = 0;
    // DateTime.second = 0;
    // u32 time_sec = RTC_Calculate_Sec(&DateTime);
    // HT_RTC->CNT = time_sec;
    RTC_SetCompare(wake_time);
    /* Enable RTC                                                                                           */
    RTC_Cmd(ENABLE);
}
