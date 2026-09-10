
#include "systick.h"
#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "tim.h"
#include "uart.h"

static u32 sys_pant_ms; // 系统开启时间,单位ms
static u32 sys_OPTime_ss;
static u32 sys_OPTime_ms;
static u16 systick_ms;

u32 runHours; // 设备运行小时计数
u32 runMins;

const static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct tim systime;
time_t Timestamp;

LL_RTC_AlarmTypeDef RTC_AlarmStruct; // 定时闹钟
LL_RTC_DateTypeDef RtcDate;
LL_RTC_TimeTypeDef RtcTime;

extern void Error_Handler(void);


void IWDG_Configuration(void) 
{
    // 启用IWDG时钟（LSI）
    RCC->CSR |= RCC_CSR_LSION; // 开启LSI
    while ((RCC->CSR & RCC_CSR_LSIRDY) == 0); // 等待LSI就绪

    // 解锁IWDG_PR和IWDG_RLR寄存器
    IWDG->KR = 0x5555;

    // 设置预分频器为256
    IWDG->PR = 0x06; // PR[2:0] = 0x06 (256分频)

    // 设置重装载值为3749（0xEA5）
    IWDG->RLR = 0xEA5; // 30秒超时

    // 启动看门狗
    IWDG->KR = 0xCCCC;
}

void IWDG_Feed(void)
{
    IWDG->KR = 0xAAAA; // 重装载计数器
}

/**
 * @brief	初始化系统时钟
 * @param
 * @retval
 */
void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0) {
    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE3);
    LL_RCC_MSI_Enable();

    /* Wait till MSI is ready */
    while (LL_RCC_MSI_IsReady() != 1) {
    }
    LL_RCC_MSI_SetRange(LL_RCC_MSIRANGE_5);
    LL_RCC_MSI_SetCalibTrimming(0);
    LL_PWR_EnableBkUpAccess();
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
    }
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
    LL_RCC_LSE_Enable();

    /* Wait till LSE is ready */
    while (LL_RCC_LSE_IsReady() != 1) {
    }
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    }
    LL_RCC_EnableRTC();
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_MSI);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_MSI) {
    }

    LL_Init1msTick(2097000);

    LL_SetSystemCoreClock(2097000);
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_SYSCLK);
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_MSI);
}

/**
 * @brief	初始化系统时钟
 * @param
 * @retval
 */
void Systick_Init(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    LL_PWR_EnableBkUpAccess();

    /* System interrupt init*/
    /* SysTick_IRQn interrupt configuration */
    NVIC_SetPriority(SysTick_IRQn, 3);
}

// 根据日期推算星期
int whatday(int year, int mon, int day)
{
    int m = mon;
    int d = day;
    // 根据月份对年份和月份进行调整
    if (m <= 2) {
        year -= 1;
        m += 12;
    }
    int c = year / 100; // 取得年份前两位
    int y = year % 100; // 取得年份后两位

    // 根据泰勒公式计算星期
    int w = (int)(c / 4) - 2 * c + y + (int)(y / 4) + (int)(13 * (m + 1) / 5) + d - 1;

    return w % 7; // 返回星期
}

void stamp_to_date(time_t stampTime, struct tm *stm)
{
    struct tm *info;
    // char buffer[80];
    //	time(&stampTime);
    info = localtime(&stampTime);

    char nowtime[24];
    memset(nowtime, 0, sizeof(nowtime));
    strftime(nowtime, 24, "%Y-%m-%d %H:%M:%S", info);
    INFO("\r\nnowtime = %s\n", nowtime);

    stm->tm_year = atoi(nowtime) - 1900;
    stm->tm_mon = atoi(nowtime + 5) - 1;
    stm->tm_mday = atoi(nowtime + 8);
    stm->tm_hour = atoi(nowtime + 11);
    stm->tm_min = atoi(nowtime + 14);
    stm->tm_sec = atoi(nowtime + 17);
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
    stm_stamp.tm_mon = systime.tm_mon - 1;
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
 * @brief	初始化系统时间
 * @param
 * @retval
 */
void systemTime_Init(void)
{
    systime.tm_year = 2022;
    systime.tm_mon = 0;
    systime.tm_mday = 1;
    systime.tm_hour = 0;
    systime.tm_min = 0;
    systime.tm_sec = 0;
    systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
    getTimestamp();
    RTC_Init();
}

/**
 * @brief	系统时钟，年月日分秒
 * @param
 * @retval
 */
static void SystemTimer(void)
{
    static char tem_month;
    systime.tm_sec++;
    Timestamp++;

    // 每小时时间同步一次
    if (30 == systime.tm_min && 30 == systime.tm_sec) {
        device_t.syncRTCTime = 1;
    }
    if (systime.tm_sec > 59) {
        systime.tm_sec = 0;
        systime.tm_min++;
        if (++runMins >= device_t.reportInterval && device_t.reportInterval != 0) {
            runMins = 0;
            device_t.syncDeviceState = 1;
        }
        if (systime.tm_min > 59) {
            systime.tm_min = 0;
            systime.tm_hour++;
            runHours++;
            if (systime.tm_hour > 23) {
                systime.tm_hour = 0;
                systime.tm_mday++;
                systime.tm_wday++;
                if (systime.tm_wday > 6)
                    systime.tm_wday = 0;
                tem_month = month[systime.tm_mon];
                if ((systime.tm_year % 4 == 0 && systime.tm_year % 100 != 0) || (systime.tm_year % 400 == 0)) {
                    tem_month += 1;
                }
                if (systime.tm_mday > tem_month) {
                    systime.tm_mday = 1;
                    systime.tm_mon++;
                    if (systime.tm_mon > 11) {
                        systime.tm_mon = 0;
                        systime.tm_year++;
                    }
                }
            }
        }
    }
}

/**
 * @brief	uS级别延时，实际延时比理论稍长
 * @param	延时的时间
 * @retval
 */
void Delay_Us(__IO u16 myUs)
{
    u16 i = 0;
    while (myUs--) {
        i = 100;
        while (i--)
            ;
    }
}

/**
 * @brief	Ms级别延时，实际延时较准
 * @param	延时的时间
 * @retval
 */
void Delay_Ms(__IO u16 myMs)
{
    u16 i = 0;
    while (myMs--) {
        i = 8000;
        while (i--)
            ;
    }
}

/**
 * @brief	mS级别延时，使用系统中断定时器，延时较准
 * @param	延时的时间
 * @retval
 */
void mDelay(u16 ms)
{
    u16 delay_ms = ms;
    systick_ms = 0;
    while (systick_ms < delay_ms)
        ;
}

/**
 * @brief	获取系统运行mS
 * @param
 * @retval	返回系统运行mS
 */
u32 get_syspant_ms(void)
{
    return sys_pant_ms;
}

/**
 * @brief This function handles System tick timer.
 */
void SysTick_Handler(void)
{
    // Systick_1ms_Handler();
}

/**
 * @brief	滴答mS中断事件
 * @param
 * @retval
 */
void Systick_1ms_Handler(void)
{
    static u32 SysTicker;
    SysTicker++;
    systick_ms++;
    sys_pant_ms++;
    sys_OPTime_ms++;

    if (uartInfo.recvTimeout)
        uartInfo.recvTimeout--;
    if (uartLoRa.recvTimeout)
        uartLoRa.recvTimeout--;
    if (uartLoRa.sendDelay)
        uartLoRa.sendDelay--;

    if (batteryVoltageConvertInterval)
        batteryVoltageConvertInterval--;

    if (sys_OPTime_ms >= SEC_DELAY) {
        sys_OPTime_ms = 0;
        sys_OPTime_ss++;
    }

    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        if ((device_t.reportMode & PERIOD_REPORT_MODE) == 1) // 仅在周期上报模式开启时计时
            device_t.counter_run_time++;
        else
            device_t.counter_run_time = 0;
        if (device_t.heartbeatDelay)
            device_t.heartbeatDelay--;
        SystemTimer();
        // getBcdTime2BinTime(&RtcDate,&RtcTime);
        // DEBUG_TRACE(LOG_TAG, "Systime Now :%04d-%02d-%02d %02d:%02d:%02d week:%02d, Timestamp :%ld", 2000+RtcDate.Year, RtcDate.Month ,
        // RtcDate.Day, RtcTime.Hours, RtcTime.Minutes, RtcTime.Seconds,RtcDate.WeekDay, Timestamp);
    }
    People_Counter_Handle();
    Led_Blink_Process();
    People_Counter_Mode_Process();
}

/**
 * @brief   随机函数发生，seed:sys_pant_ms
 * @param   参数设定随机数范围1~n
 * @retval  返回生成的随机数value;
 **/
u16 random_syspant(u16 n)
{
    u16 value;
    srand(sys_pant_ms);
    value = rand() % n + 1; // 获取一个随机数(1~n)
    return value;
}

/**
 * @brief   系统运行时间计时器
 * @param
 * @retval  返回系统运行时间mS
 **/
u32 get_sysopentime(void)
{
    u32 sys_ms;
    sys_ms = sys_OPTime_ss * 1000 + sys_OPTime_ms;
    return sys_ms;
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
 * @brief   设置RTC日期、时间
 * @param   RtcDate:日期（*）
 * @param   RtcTime：时间（*）
 * @retval
 **/
void setRtcTime(LL_RTC_DateTypeDef *RtcDate, LL_RTC_TimeTypeDef *RtcTime)
{
    RtcTime->TimeFormat = LL_RTC_HOURFORMAT_24HOUR;
    RtcDate->WeekDay = whatday(RtcDate->Year + 2000, RtcDate->Month, RtcDate->Day) == 0 ? 7 : whatday(RtcDate->Year + 2000, RtcDate->Month, RtcDate->Day);
    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, RtcTime);
    LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, RtcDate);
    getBcdTime2BinTime(RtcDate, RtcTime);
    DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d %02d:%02d:%02d week:%02d", 2000 + RtcDate->Year, RtcDate->Month,
                RtcDate->Day, RtcTime->Hours, RtcTime->Minutes, RtcTime->Seconds, RtcDate->WeekDay);
}

/**
 * @brief   RTC时钟初始化
 * @param
 * @retval
 **/
void RTC_Init(void)
{
    LL_RTC_InitTypeDef RTC_InitStruct = {0};
    LL_RTC_TimeTypeDef RTC_TimeStruct = {0};
    LL_RTC_DateTypeDef RTC_DateStruct = {0};

    /* Peripheral clock enable */
    LL_RCC_EnableRTC();

    /* RTC interrupt Init */
    NVIC_SetPriority(RTC_IRQn, 0);
    NVIC_EnableIRQ(RTC_IRQn);
    LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_17);
    LL_EXTI_EnableRisingTrig_0_31(LL_EXTI_LINE_17);

    /** Initialize RTC and set the Time and Date
     */
    RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
    RTC_InitStruct.AsynchPrescaler = 127;
    RTC_InitStruct.SynchPrescaler = 255;
    LL_RTC_Init(RTC, &RTC_InitStruct);

    /** Initialize RTC and set the Time and Date
     */
    RTC_TimeStruct.Hours = 0x00;
    RTC_TimeStruct.Minutes = 0x00;
    RTC_TimeStruct.Seconds = 0x00;
    LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_TimeStruct);
    RTC_DateStruct.WeekDay = LL_RTC_WEEKDAY_MONDAY;
    RTC_DateStruct.Month = LL_RTC_MONTH_JANUARY;
    RTC_DateStruct.Day = 0x01;
    RTC_DateStruct.Year = 0x22;
    LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_DateStruct);
}

void RTC_IRQHandler(void) // 定时闹钟中断函数
{
    LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_17);
    if (LL_RTC_IsActiveFlag_ALRA(RTC)) {
        LL_RTC_ClearFlag_ALRA(RTC);
        if (1 == device_t.timeEvent) {
            device_t.counter_a_count = 0;
            device_t.counter_b_count = 0;
            device_t.syncRTCTime = 1;
            // getBcdTime2BinTime(&RtcDate, &RtcTime);
            // DEBUG_TRACE(LOG_TAG, "Alarm event:Auto zero counter,Systime Now :%04d-%02d-%02d %02d:%02d:%02d week:%02d", 2000 + RtcDate.Year, RtcDate.Month,
            //             RtcDate.Day, RtcTime.Hours, RtcTime.Minutes, RtcTime.Seconds, RtcDate.WeekDay);
        }
    }
}

/**
 * @brief   设置定时闹钟时间并启动
 * @param hour-闹钟时
 * @param minute-闹钟分
 * @retval
 **/
void setAlmaTime(u8 hour, u8 minute)
{
    offAlma(); // 一定要先关闭闹钟再重设，否则无效
    RTC_AlarmStruct.AlarmTime.TimeFormat = LL_RTC_HOURFORMAT_24HOUR;
    RTC_AlarmStruct.AlarmTime.Seconds = 0;
    RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_DATEWEEKDAY;
    RTC_AlarmStruct.AlarmTime.Hours = hour;
    RTC_AlarmStruct.AlarmTime.Minutes = minute;
    LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &RTC_AlarmStruct);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_RTC_ALMA_Enable(RTC);
    LL_RTC_EnableIT_ALRA(RTC);
    LL_RTC_EnableWriteProtection(RTC);
    DEBUG_TRACE(LOG_TAG, "SET Alma time: %02d:%02d", hour, minute);
}

/**
 * @brief   关闭定时闹钟
 * @param
 * @retval
 **/
void offAlma(void)
{
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_ALMA_Disable(RTC);
    LL_RTC_DisableIT_ALRA(RTC);
    LL_RTC_EnableWriteProtection(RTC);
    // DEBUG_TRACE(LOG_TAG, "Alma:off");
}

/**
 * @brief   系统时间同步RTC时间
 * @param   每小时同步时的分钟数
 * @note    内部系统时间与RTC时间误差以1-5秒每小时级计算，每小时同步一次即可，边缘处同步会影响进位导致运行小时分钟重叠，故选择在分钟30s同步
 * @note    不可直接获取RTC同步，要考虑避开重叠计数
 * @retval
 **/
void syncRtcTime(u8 minutes)
{
    if (minutes == systime.tm_min && 30 == systime.tm_sec) {
        LL_RTC_TimeTypeDef nTime;
        LL_RTC_DateTypeDef nDate;
        getBcdTime2BinTime(&nDate, &nTime);
        if (minutes == nTime.Minutes) {
            // Timestamp -= (systime.tm_sec-nTime.Seconds);
            systime.tm_sec = nTime.Seconds;
            getTimestamp();
            DEBUG_TRACE(LOG_TAG, "syncRtcTime,Systime Now :%04d-%02d-%02d %02d:%02d:%02d week:%02d", 2000 + nDate.Year, nDate.Month,
                        nDate.Day, nTime.Hours, nTime.Minutes, nTime.Seconds, RtcDate.WeekDay);
        } else {
            DEBUG_TRACE(ERROR_TAG, "syncRtcTime,The time error is large. Check the configuration");
        }
    }
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

    DEBUG_TRACE(LOG_TAG, "Rtc To Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
}