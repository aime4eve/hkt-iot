
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "multi_button.h"
#include "systick.h"
#include "uart.h"

uint16_t freq_tab[12] = {262, 277, 294, 311, 330, 349, 369, 392, 415, 440, 466, 494}; // 原始频率表 CDEFGAB

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void BSTIM_IRQHandler(void)
{
    static u8 button_tick_start;
    if (BSTIM_ISR_UIF_Chk() != RESET) {
        BSTIM_ISR_UIF_Clr();
        button_tick_start++;
        tm_tick_ms++;
        sys_pant_ms++;
        if (button_tick_start > 5) {
            button_tick_start = 0;
            button_ticks();
        }
    }
}

/**
 * @brief  初始化通用定时器
 * @param  period：计数周期（实际值）
 * @param  prescaler：预分频值（实际值）
 * @retval
 */
void BSTIM_Init(u16 period, u16 prescaler)
{
    CMU_PERCLK_SetableEx(BSTIMCLK, ENABLE);          // 外设总线时钟使能;
    CMU_OPCCR2_BSTCKS_Set(CMU_OPCCR2_BSTCKS_SYSCLK); // 选择工作时钟源
    CMU_OPCCR2_BSTCKE_Setable(ENABLE);               // 工作时钟源使能

    BSTIM_CR1_ARPE_Setable(ENABLE);            // 预装载使能
    BSTIM_CR1_OPM_Set(BSTIM_CR1_OPM_CONTINUE); // UE产生时计数器不停止

    BSTIM_PSCR_Write(prescaler - 1); //
    BSTIM_ARR_Write(period - 1);     //
    // 0x10000 - u16Period;

    NVIC_DisableIRQ(BSTIM_IRQn);
    NVIC_SetPriority(BSTIM_IRQn, 2); // 中断优先级配置
    NVIC_EnableIRQ(BSTIM_IRQn);

    BSTIM_IER_UIE_Setable(ENABLE); // Update中断使能
    BSTIM_CR1_CEN_Setable(ENABLE); // 计数器使能
}

/**
 * @brief  低功耗定时器中断事件
 * @param
 * @retval
 */
void LPTIM_IRQHandler(void) // PWM输出中断服务程序
{
    if (LPTIM_ISR_OVIF_Chk() == SET) // 溢出中断标志被置起
    {
        LPTIM_ISR_OVIF_Clr(); // 清除中断标志
        if (device_t.sleepState)
            device_t.lptWakeEvent = 1;
    }
}

/**
 * @brief  初始化低功耗定时器 500ms中断周期
 * @param
 * @retval
 */
void LPTIM_Init(void)
{
    CMU_PERCLK_SetableEx(LPTCLK, ENABLE);           // 外设总线时钟使能
    CMU_OPCCR2_LPTCKS_Set(CMU_OPCCR2_LPTCKS_LSCLK); // 选择工作时钟源
    CMU_OPCCR2_LPTCKE_Setable(ENABLE);              // 工作时钟源使能

    LPTIM_CFGR_ETR_AFEN_Setable(DISABLE);            // ETR输入滤波
    LPTIM_CFGR_PSCSEL_Set(LPTIM_CFGR_PSCSEL_CLKSEL); // 时钟选择
    LPTIM_CFGR_DIVSEL_Set(LPTIM_CFGR_DIVSEL_DIV1);   // 时钟分频选择
    LPTIM_CFGR_ONST_Set(LPTIM_CFGR_ONST_CONTINUE);   // 发生计数器不停止
    LPTIM_CFGR_TMODE_Set(LPTIM_CFGR_TMODE_COUNTER);  // 普通定时器模式

    // LPTIM_ARR_Write(16384); // 500ms
    LPTIM_ARR_Write(26214); // 800ms

    LPTIM_ISR_OVIF_Clr();           /* 清除计数器中断标志位 */
    LPTIM_IER_OVIE_Setable(ENABLE); /* 开启计数器中断 */

    NVIC_DisableIRQ(LPTIM_IRQn);
    NVIC_SetPriority(LPTIM_IRQn, 2); // 中断优先级配置
    NVIC_EnableIRQ(LPTIM_IRQn);

    LPTIM_CR_EN_Setable(ENABLE); /* 使能计数器: */
}

/**
 * @brief  RTC中断事件
 * @param
 * @retval
 */
void RTC_IRQHandler(void)
{
    if (RTC_ISR_ALARM_IF_Chk() == SET) // 查询闹钟钟断标志是否置起
    {
        /* Alarm callback */
        device_t.rtcWakeEvent = 1;
        if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
            device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
        RTC_ISR_ALARM_IF_Clr(); // 清除闹钟中断标志
    }
}

/**
 * @brief  获取RTC时间
 * @param   para ： RTC时间结构体
 * @retval  0 获取成功 1 获取失败
 */
u8 RTC_GetRTC(RTC_TimeDateTypeDef *para)
{
    u8 n, i;
    u8 Result = 1;

    RTC_TimeDateTypeDef getRtcTime1, getRtcTime2;

    for (n = 0; n < 3; n++) {
        RTC_TimeDate_GetEx(&getRtcTime1); // 读一次时间
        RTC_TimeDate_GetEx(&getRtcTime2); // 再读一次时间

        for (i = 0; i < 7; i++) // 两者一致, 表示读取成功
        {
            if (((u8 *)(&getRtcTime1))[i] != ((u8 *)(&getRtcTime2))[i])
                break;
        }
        if (i == 7) {
            Result = 0;
            memcpy((u8 *)(para), (u8 *)(&getRtcTime1), 7); // 读取正确则更新新的时间
            break;
        }
    }
    return Result;
}

u8 RTC_SetRTC(RTC_TimeDateTypeDef *para)
{
    u8 n, i;
    u8 Result;
    RTC_TimeDateTypeDef getRtcTime1;

    for (n = 0; n < 3; n++) {
        RTC_WER_Write(RTC_WRITE_ENABLE);  // 解除RTC写保护
        RTC_TimeDate_SetEx(para);         // 设置RTC
        RTC_WER_Write(RTC_WRITE_DISABLE); // 打开RTC写保护

        Result = RTC_GetRTC(&getRtcTime1); // 读取确认设置结果
        if (Result == 0) {
            Result = 1;
            for (i = 0; i < 7; i++) // 两者一致, 表示设置成功
            {
                if (((u8 *)(&getRtcTime1))[i] != ((u8 *)(para))[i])
                    break;
            }
            if (i == 7) {
                Result = 0;
                break;
            }
        }
    }
    return Result;
}

/**
 * @brief  初始化RTC时钟
 * @param
 * @retval
 */
void RTC_Init(void)
{
    CMU_PERCLK_SetableEx(RTCCLK, ENABLE); // RTC总线时钟使能
    RTC_VCAL_PR1SEN_Setable(DISABLE);     // 虚拟调校使能关闭

    RTC_TimeDateTypeDef rtc_date_initstruct = {0};
    rtc_date_initstruct.Year = RTC_CONVERT_BIN2BCD(systime.tm_year - 2000);
    rtc_date_initstruct.Month = RTC_CONVERT_BIN2BCD(systime.tm_mon + 1);
    rtc_date_initstruct.Date = RTC_CONVERT_BIN2BCD(systime.tm_mday);
    rtc_date_initstruct.Hour = RTC_CONVERT_BIN2BCD(systime.tm_hour);
    rtc_date_initstruct.Minute = RTC_CONVERT_BIN2BCD(systime.tm_min);
    rtc_date_initstruct.Second = RTC_CONVERT_BIN2BCD(systime.tm_sec);
    rtc_date_initstruct.Week = systime.tm_wday;
    RTC_SetRTC(&rtc_date_initstruct);

    RTC_CR_RTC_EN_Setable(ENABLE); // 使能RTC_A
    // RTC_ISR_SEC_IF_Clr();		//清除秒中断标志
    // RTC_IER_SEC_IE_Setable(ENABLE);//打开RTC秒中断

    NVIC_DisableIRQ(RTC_IRQn); // NVIC中断控制器配置
    NVIC_SetPriority(RTC_IRQn, 2);
    NVIC_EnableIRQ(RTC_IRQn);
}

/**
 * @brief  初始化RTC闹钟唤醒时钟
 * @param
 * @retval
 */
void RTC_AlarmInit(RTC_AlarmTmieTypeDef *alarmTime)
{
    RTC_AlarmTmieTypeDef AlarmTime_InitStruct;
    CMU_PERCLK_SetableEx(RTCCLK, ENABLE); // RTC总线时钟使能
    RTC_VCAL_PR1SEN_Setable(DISABLE);     // 虚拟调校使能关闭

    // RTC_TimeDateTypeDef rtc_date_initstruct = {0};
    // rtc_date_initstruct.Year = RTC_CONVERT_BIN2BCD(systime.tm_year - 2000);
    // rtc_date_initstruct.Month = RTC_CONVERT_BIN2BCD(systime.tm_mon + 1);
    // rtc_date_initstruct.Date = RTC_CONVERT_BIN2BCD(systime.tm_mday);
    // rtc_date_initstruct.Hour = RTC_CONVERT_BIN2BCD(systime.tm_hour);
    // rtc_date_initstruct.Minute = RTC_CONVERT_BIN2BCD(systime.tm_min);
    // rtc_date_initstruct.Second = RTC_CONVERT_BIN2BCD(systime.tm_sec);
    // rtc_date_initstruct.Week = systime.tm_wday;
    // RTC_SetRTC(&rtc_date_initstruct);

    AlarmTime_InitStruct.Hour = RTC_CONVERT_BIN2BCD(alarmTime->Hour);
    AlarmTime_InitStruct.Minute = RTC_CONVERT_BIN2BCD(alarmTime->Minute);
    AlarmTime_InitStruct.Second = RTC_CONVERT_BIN2BCD(alarmTime->Second);
    RTC_AlarmTime_SetEx(&AlarmTime_InitStruct); // 设置闹钟时间

    RTC_ISR_ALARM_IF_Clr();           // 清除闹钟中断标志
    RTC_IER_ALARM_IE_Setable(ENABLE); // 打开闹钟中断
    RTC_ALARM_ALMEN_Setable(ENABLE);  // 闹钟功能使能
    RTC_CR_RTC_EN_Setable(ENABLE);    // 使能RTC_A

    // RTC_ISR_SEC_IF_Clr();		//清除秒中断标志
    // RTC_IER_SEC_IE_Setable(ENABLE);//打开RTC秒中断

    NVIC_DisableIRQ(RTC_IRQn); // NVIC中断控制器配置
    NVIC_SetPriority(RTC_IRQn, 2);
    NVIC_EnableIRQ(RTC_IRQn);

    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d, Timestamp: %ld-%ld",
                alarmTime->Hour, alarmTime->Minute, alarmTime->Second,
                systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, wakeupTimestamp, Timestamp);
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
    int temp = device_t.timezone * 60 * 60;
    Timestamp = mktime(&stm_stamp) - temp;
    // #if HIGH_LEVEL_DEBUG_ENABLE
    //     DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
    //                 systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
    // #endif
}

/**
 * @brief  时间戳转时间
 * @param
 * @retval
 */
void convertTimestampToSystime(void)
{
    int temp = device_t.timezone * 60 * 60;
    time_t rawtime = Timestamp + temp;
    struct tm *info = localtime(&rawtime);
    memcpy(&systime, info, sizeof(systime));
    systime.tm_year += 1900;
    DEBUG_TRACE(LOG_TAG, "Timezone[%.1f] Conver Timestamp To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld",
                device_t.timezone, systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
}

/**
 * @brief  切换系统时间为RTC时间
 * @param
 * @retval
 */
void sysTimeSwitchToRtcTime(void)
{
    RTC_TimeDateTypeDef rtcTime = {0};
    if (!RTC_GetRTC(&rtcTime)) {
        systime.tm_year = 2000 + RTC_CONVERT_BCD2BIN(rtcTime.Year);
        systime.tm_mon = RTC_CONVERT_BCD2BIN(rtcTime.Month) - 1;
        systime.tm_mday = RTC_CONVERT_BCD2BIN(rtcTime.Date);
        systime.tm_hour = RTC_CONVERT_BCD2BIN(rtcTime.Hour);
        systime.tm_min = RTC_CONVERT_BCD2BIN(rtcTime.Minute);
        systime.tm_sec = RTC_CONVERT_BCD2BIN(rtcTime.Second);
        systime.tm_wday = RTC_CONVERT_BCD2BIN(rtcTime.Week);

        getTimestamp();
        DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                    systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
    } else {
        DEBUG_TRACE(ERROR_TAG, "RTC Get RTC Time ERROR");
    }
}

/**
 * @brief  设置下次自动唤醒时间 （芯片限制最大睡眠时间为4096s，否则看门狗自动复位）
 * @param
 * @retval
 */
void lpmSleepTimeInit(void)
{
    struct tm timeinfo = {0};
    time_t stamp = 0;

    /* 唤醒间隔处理 */
    if (device_t.reportInterval && dataReportTimestamp > Timestamp)
        stamp = dataReportTimestamp;
    else if (!device_t.reportInterval) {
        stamp = Timestamp + 4000;
    }
    if (reconnect_stamp < stamp && !LoRaWAN.joinState && reconnect_stamp > Timestamp) {
        stamp = reconnect_stamp;
    }
    // 最长睡眠4000s
    if ((stamp - Timestamp) >= 4000) {
        stamp = Timestamp + 4000;
    }
    // stamp += 8 * 60 * 60;
    wakeupTimestamp = stamp;
    int temp = device_t.timezone * 60 * 60;
    stamp += temp;

    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    RTC_AlarmTmieTypeDef alarmTime;
    alarmTime.Hour = timeinfo.tm_hour;
    alarmTime.Minute = timeinfo.tm_min;
    alarmTime.Second = timeinfo.tm_sec;
    RTC_AlarmInit(&alarmTime);
}
