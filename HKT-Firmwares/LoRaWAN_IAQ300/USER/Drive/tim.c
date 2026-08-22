
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
void LPTIM_IRQHandler(void)
{
    static u16 time_tick, beep_tick, led_tick;
    if (LPTIM_ISR_OVIF_Chk() == SET) // 溢出中断标志被置起
    {
        LPTIM_ISR_OVIF_Clr(); // 清除中断标志
        time_tick++;
        if (time_tick >= 10) {
            time_tick = 0;
            Timestamp++;
        }

        led_tick++;
        /* LED控制逻辑 */
        if (device_t.ledShowMode == 1) { // 模式1：常闭
            LED_GREEN_H;
            LED_ORANGE_H;
            LED_RED_H;
        } else if (device_t.ledShowMode == 2) { // 模式2：常亮
            switch (device_t.sensor_t.air_level) {
            case AIR_LEVEL_NORMAL:
                LED_GREEN_L;
                LED_ORANGE_H;
                LED_RED_H;
                break;
            case AIR_LEVEL_MILD:
                LED_ORANGE_L;
                LED_GREEN_H;
                LED_RED_H;
                break;
            case AIR_LEVEL_BAD:
            default:
                LED_RED_L;
                LED_ORANGE_H;
                LED_GREEN_H;
                break;
            }
        } else { // 模式0：10秒周期闪烁
            if (led_tick >= 100)
                led_tick = 0; // 10秒周期复位
            // 在周期末尾亮1秒（最后10个tick）
            if (led_tick >= 99) {
                switch (device_t.sensor_t.air_level) {
                case AIR_LEVEL_NORMAL:
                    LED_GREEN_L;
                    LED_ORANGE_H;
                    LED_RED_H;
                    break;
                case AIR_LEVEL_MILD:
                    LED_ORANGE_L;
                    LED_GREEN_H;
                    LED_RED_H;
                    break;
                case AIR_LEVEL_BAD:
                default:
                    LED_RED_L;
                    LED_ORANGE_H;
                    LED_GREEN_H;
                    break;
                }
            } else {
                LED_GREEN_H;
                LED_ORANGE_H;
                LED_RED_H; // 熄灭
            }
        }

        if (device_t.beepEnable) {
            if (device_t.sensor_t.air_level == AIR_LEVEL_MILD) {
                beep_tick++;
                if (beep_tick == 300) {
                    PMU_CR_PMOD_Set(PMU_CR_PMOD_ACTIVE);
                    Init_SysClk_Gen();
                    if (pirConvertTimestamp < Timestamp + 3)
                        pirConvertTimestamp = Timestamp + 3;
                    beep_set(2000, 50);
                } else if (beep_tick == 301) {
                    beep_tick = 0;
                    CMU_SYSCLKCR_SYSCLKSEL_Set(CMU_SYSCLKCR_SYSCLKSEL_LSCLK); // 选择LSCLK做主时钟
                    PMU_CR_PMOD_Set(PMU_CR_PMOD_LPRUN);
                    BEEP_OFF();
                }
            } else if (device_t.sensor_t.air_level >= AIR_LEVEL_BAD) {
                beep_tick++;
                if (beep_tick == 300) {
                    PMU_CR_PMOD_Set(PMU_CR_PMOD_ACTIVE);
                    Init_SysClk_Gen();
                    if (pirConvertTimestamp < Timestamp + 3)
                        pirConvertTimestamp = Timestamp + 3;
                    beep_set(2000, 50);
                } else if (beep_tick == 301)
                    BEEP_OFF();
                else if (beep_tick == 302)
                    beep_set(2000, 50);
                else if (beep_tick == 303) {
                    beep_tick = 0;
                    CMU_SYSCLKCR_SYSCLKSEL_Set(CMU_SYSCLKCR_SYSCLKSEL_LSCLK); // 选择LSCLK做主时钟
                    PMU_CR_PMOD_Set(PMU_CR_PMOD_LPRUN);
                    BEEP_OFF();
                }
            } else {
                beep_tick = 0;
                BEEP_OFF();
            }
        }
    }
}

/**
 * @brief  初始化低功耗定时器 周期1ms
 * @retval
 */
void LPTIM_Init(void)
{
    CMU_PERCLK_SetableEx(LPTCLK, ENABLE);           // 外设总线时钟使能
    CMU_OPCCR2_LPTCKS_Set(CMU_OPCCR2_LPTCKS_LSCLK); // 选择工作时钟源
    CMU_OPCCR2_LPTCKE_Setable(ENABLE);              // 工作时钟源使能

    LPTIM_CFGR_ETR_AFEN_Setable(DISABLE);            // ETR输入滤波
    LPTIM_CFGR_PSCSEL_Set(LPTIM_CFGR_PSCSEL_CLKSEL); // 时钟选择
    LPTIM_CFGR_DIVSEL_Set(LPTIM_CFGR_DIVSEL_DIV4);   // 时钟分频选择
    LPTIM_CFGR_ONST_Set(LPTIM_CFGR_ONST_CONTINUE);   // 发生计数器不停止
    LPTIM_CFGR_TMODE_Set(LPTIM_CFGR_TMODE_COUNTER);  // 普通定时器模式

    LPTIM_ARR_Write(820 - 1); // 100ms

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
        uartLoRa.sendDelay = 3 * SEC_DELAY; // 唤醒后延时3秒发送数据，保证传感器全部转换完成
        RTC_ISR_ALARM_IF_Clr();             // 清除闹钟中断标志
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
    RTC_AlarmTmieTypeDef AlarmTime_InitStruct;
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

    // NVIC_DisableIRQ(RTC_IRQn); // NVIC中断控制器配置
    // NVIC_SetPriority(RTC_IRQn, 2);
    // NVIC_EnableIRQ(RTC_IRQn);

    DEBUG_TRACE(LOG_TAG, "Next Wake Up Time: %02d:%02d:%02d, Time Now: %04d-%02d-%02d:%02d:%02d:%02d, sensorConvertTimestamp: %d", alarmTime->Hour,
                alarmTime->Minute, alarmTime->Second, systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min,
                systime.tm_sec, sensorConvertTimestamp);
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
    DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1, systime.tm_mday,
                systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
#endif
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
        // DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d, week: %02d, Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
        //             systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, systime.tm_wday, Timestamp);
    } else {
        DEBUG_TRACE(ERROR_TAG, "RTC Get RTC Time ERROR");
    }
}

/**
 * @brief  设置下次自动唤醒时间（芯片限制最大睡眠时间为4096s，否则看门狗自动复位）
 * @param
 * @retval
 */
void lpmSleepTimeInit(void)
{
    struct tm timeinfo = {0};
    time_t stamp = 0;

    /* 唤醒间隔处理 */
    if (sensorConvertTimestamp < dataReportTimestamp)
        stamp = sensorConvertTimestamp;
    else
        stamp = dataReportTimestamp;

    if (!LoRaWAN.joinState) {
        if (stamp > reconnect_stamp)
            stamp = reconnect_stamp;
    }

    if ((stamp - Timestamp) >= 4000) { // 最长睡眠4000s
        stamp = Timestamp + 4000;
    }

    if (stamp < Timestamp + 3) {
        stamp = Timestamp + 3;
    }

    if (poweroff_status) {
        stamp = Timestamp + 30;
    }

    stamp += 8 * 60 * 60;

    localtime_r(&stamp, &timeinfo); // 获取下次唤醒时间
    RTC_AlarmTmieTypeDef alarmTime;
    alarmTime.Hour = timeinfo.tm_hour;
    alarmTime.Minute = timeinfo.tm_min;
    alarmTime.Second = timeinfo.tm_sec;
    RTC_AlarmInit(&alarmTime);
}

/**
 * @brief  初始化蜂鸣器发声定时器
 * @param  period：计数周期（实际值）
 * @param  prescaler：预分频值（实际值）
 * @retval
 */
void BeepTimerInit(u16 period, u16 pulse)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    OutputIO(BEEP_CTRL_PORT, BEEP_CTRL_PIN, OUT_PUSHPULL);

    CMU_PERCLK_SetableEx(BT1CLK, ENABLE);
    AltFunIO(GPIOB, GPIO_Pin_6, 0);
    GPIOx_DFS_Setable(GPIOB, GPIO_Pin_6, ENABLE);

    BTx_CR1_MODE_Set(BT1, BTx_CR1_MODE_COUNTER);          // 计数器
    BTx_CR1_EDGESEL_Set(BT1, BTx_CR1_EDGESEL_FALLING);    // 下降沿
    BTx_CR2_SIG2SEL_Set(BT1, BTx_CR2_SIG2SEL_GROUP1);     // 计数源选择group1
    BTx_CFGR1_GRP1SEL_Set(BT1, BTx_CFGR1_GRP1SEL_APBCLK); // 计数源选择group1-APB
    // BTx_CFGR1_GRP1SEL_Set(BT1, BTx_CFGR1_GRP1SEL_RTCOUT1);
    // BTx_CFGR1_RTCSEL1_Set(BT1, BTx_CFGR1_RTCSEL1_32768HZ);

    BTx_PRES_Write(BT1, 0);           // 预分频
    BTx_LOADL_Write(BT1, period - 1); // 周期
    BTx_OUTCNT_Write(BT1, pulse - 1); // 脉冲

    BTx_CR1_PWM_Setable(BT1, DISABLE);
    BTx_CR2_CNTHSEL_Set(BT1, BTx_CR2_CNTHSEL_INNER); // 8位
    BTx_CR2_STDIR_Setable(BT1, ENABLE);              //

    BTx_OCR_OUTCLR_Set(BT1, BTx_OCR_OUTCLR_CLEAR);
    BTx_OCR_OUTINV_Set(BT1, BTx_OCR_OUTINV_NORMAL);
    BTx_OCR_OUTMOD_Set(BT1, BTx_OCR_OUTMOD_PULSE_SHIFT); // 脉宽可调
    BTx_OCR_OUTSEL_Set(BT1, BTx_OCR_OUTSEL_HIGH);        // 输出高比较器

    BTx_LOADCR_LLEN_Setable(BT1, ENABLE);
    BEEP_ON();
}

void BEEP_ON() { BTx_CR1_CLEN_Setable(BT1, ENABLE); }

void BEEP_OFF()
{
    BTx_CR1_CLEN_Setable(BT1, DISABLE);
    BTx_CR2_STDIR_Setable(BT1, DISABLE);
    BTx_LOADCR_LLEN_Setable(BT1, DISABLE);
    CMU_PERCLK_SetableEx(BT1CLK, DISABLE);

    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    // GPIOx_DFS_Setable(GPIOB, GPIO_Pin_6, DISABLE);

    /** 初始化按键 IO **/
    OutputIO(BEEP_CTRL_PORT, BEEP_CTRL_PIN, OUT_PUSHPULL);
    BEEP_CTRL_L;
}

void beep_set(uint16_t freq, uint8_t volume)
{
    u16 period, pulse;

    /* 将频率转化为周期 周期单位:ns 频率单位:HZ */
    period = 32768 / freq; // unit:ns 1/HZ*10^9 = ns

    /* 根据声音大小计算占空比 蜂鸣器低电平触发 */
    pulse = period - period * volume / 100;

    /* 利用 PWM API 设定 周期和占空比 */
    BeepTimerInit(period, pulse);
}

void BeepTest(void)
{
    for (int i = 0; i < sizeof(freq_tab); i++) {
        beep_set(freq_tab[i], 1);
        for (int j = 0; j < 100; j++) {
            beep_set(freq_tab[i], j);
            mDelay(500);
        }
    }
}
