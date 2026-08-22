
#include "math.h"
#include <stdio.h>
#include <stdlib.h>

#include "systick.h"
#include "tim.h"
#include "time.h"
#include "uart.h"
#include "gpio.h"

static u32 sys_pant_ms; // 系统开启时间,单位ms
static u32 sys_OPTime_ss;
static u32 sys_OPTime_ms;
static u16 systick_ms;

const static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct tim systime;
time_t Timestamp;

void SystemClock_Config(void)
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0) {
    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSI_Enable();

    /* Wait till HSI is ready */
    while (LL_RCC_HSI_IsReady() != 1) {
    }
    LL_RCC_HSI_SetCalibTrimming(16);
    LL_RCC_LSI_Enable();

    /* Wait till LSI is ready */
    while (LL_RCC_LSI_IsReady() != 1) {
    }
    LL_PWR_EnableBkUpAccess();
//    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_LOW);
//    LL_RCC_LSE_Enable();

    // /* Wait till LSE is ready */
    // while (LL_RCC_LSE_IsReady() != 1) {
    // }
//    LL_RCC_LSE_EnableCSS();
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
    }

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    LL_Init1msTick(16000000);
    LL_SetSystemCoreClock(16000000);

    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
}

/**
 * @brief Peripherals Common Clock Configuration
 * @retval None
 */
void PeriphCommonClock_Config(void)
{
    LL_RCC_PLLSAI1_ConfigDomain_ADC(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLSAI1R_DIV_2);
    LL_RCC_PLLSAI1_EnableDomain_ADC();
    LL_RCC_PLLSAI1_Enable();

    /* Wait till PLLSAI1 is ready */
    while (LL_RCC_PLLSAI1_IsReady() != 1) {
    }
}

/**
 * @brief	初始化系统时钟
 * @param
 * @retval
 */
void Systick_Init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

    /* System interrupt init*/
    /* SysTick_IRQn interrupt configuration */
    NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 15, 0));
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
 * @brief	初始化系统时间
 * @param
 * @retval
 */
void systemTime_Init(void)
{
    systime.tm_year = 2023;
    systime.tm_mday = 1;
    systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
    getTimestamp();
    // RTC_Init();
}

/**
 * @brief	系统时钟，年月日分秒
 * @param
 * @retval
 */
void SystemTimer(void)
{
    static char tem_month;
    systime.tm_sec++;

    if (systime.tm_sec > 59) {
        systime.tm_sec = 0;
        systime.tm_min++;

        if (systime.tm_min > 59) {
            systime.tm_min = 0;
            systime.tm_hour++;
            if (systime.tm_hour > 23) {
                systime.tm_hour = 0;
                systime.tm_mday++;
                systime.tm_wday++;
                if (systime.tm_wday > 6)
                    systime.tm_wday = 0;
                tem_month = month[systime.tm_mon];
                if ((systime.tm_year % 4 == 0 && systime.tm_year % 100 != 0) || (systime.tm_year % 400 == 0)) {
                    if (systime.tm_mon == 1) // 2月
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
    ;
}

/**
 * @brief	滴答mS中断事件
 * @param
 * @retval
 */
void Systick_1ms_Handler(void)
{
    static u32 SysTicker;
    static u8 button_tick_start;
    SysTicker++;
    systick_ms++;
    sys_pant_ms++;
    sys_OPTime_ms++;

    if (uartInfo.recvTimeout)
        uartInfo.recvTimeout--;
    if (uartBle.recvTimeout)
        uartBle.recvTimeout--;
    if (uartBle.sendDelay)
        uartBle.sendDelay--;
    if (sys_OPTime_ms >= SEC_DELAY) {
        sys_OPTime_ms = 0;
        sys_OPTime_ss++;
    }

    if (sys_pant_ms % 500 == 0){
        LL_GPIO_TogglePin(LED_BLUE_PORT, LED_BLUE_PIN);
        LL_GPIO_TogglePin(LED_RED_PORT, LED_RED_PIN);
    }

    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        Timestamp++;
        SystemTimer();
    }
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
