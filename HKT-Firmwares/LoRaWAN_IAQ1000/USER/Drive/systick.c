
#include "math.h"
#include <stdio.h>
#include <stdlib.h>

#include "LoRaWAN_APPLY.h"
#include "adc.h"
#include "cb_hcho_v4c.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "lwrb.h"
#include "multi_button.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

#if FUNC_HCHO
#include "CB_HCHO_V4C.h"
#endif

u32 sys_pant_ms; // 系统开启时间,单位ms
u32 sys_OPTime_ss;
u32 sys_OPTime_ms;
u32 tm_tick_ms;

const static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct tim systime;
time_t Timestamp;

extern void Error_Handler(void);

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

/**
 * @brief	初始化系统时间
 * @param
 * @retval
 */
void systemTime_Init(void)
{
    sysTimeSwitchToRtcTime();
    // 2023010000
    if (Timestamp < 1672502400 || systime.tm_year >= 2050) {
        memset(&systime, 0, sizeof(systime));
        systime.tm_year = 2022;
        systime.tm_mday = 1;
        systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
        getTimestamp();
    }
    RTC_Init();
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

void tx_TimerCallback(void)
{
    static u16 SysTicker;
    SysTicker++;
    if (uartInfo.recvTimeout)
        uartInfo.recvTimeout--;
    if (uartLoRa.recvTimeout)
        uartLoRa.recvTimeout--;
    if (uartPmsa.recvTimeout)
        uartPmsa.recvTimeout--;
    if (uartHCHO.recvTimeout)
        uartHCHO.recvTimeout--;
    if (uartLoRa.sendDelay)
        uartLoRa.sendDelay--;
    if (device_t.epdShowModeWriteDelay > 0)
        device_t.epdShowModeWriteDelay--;
    if (device_t.ledShowModeWriteDelay > 0)
        device_t.ledShowModeWriteDelay--;
    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        SystemTimer();
        Timestamp++;
        if (!uartLoRa.sendDelay) {
            if (device_t.sleepDelay)
                device_t.sleepDelay--;
        }
        if (heater_wait_time)
            heater_wait_time--;
    }
    // IWDT_Clr();
}

/**
 * @brief	mS级别延时，使用系统中断定时器，延时较准
 * @param	延时的时间
 * @retval
 */
void mDelay(u16 ms)
{
    u16 delay_ms = ms;
    tm_tick_ms = 0;
    while (tm_tick_ms < delay_ms)
        ;
}

/**
 * @brief	获取系统运行mS
 * @param
 * @retval	返回系统运行mS
 */
u32 get_syspant_ms(void)
{
    if (sys_pant_ms > 86400000)
        sys_pant_ms = 0;
    return sys_pant_ms;
}

/**
 * @brief This function handles System tick timer.
 */
// void SysTick_Handler(void)
//{
//	// Systick_1ms_Handler();
// }

/**
 * @brief	滴答mS中断事件
 * @param
 * @retval
 */
void Systick_1ms_Handler(void)
{
    ;
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

void NMI_Handler(void)
{
    // NWKUP默认连接到了cpu的NMI不可屏蔽中断，不受NVIC控制，不受全局中断使能控制，唤醒后必然进NMI中断
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PD6)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PD6);
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PE9)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PE9);
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PE2)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PE2);
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PA13)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PA13);
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PG7)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PG7);
        device_t.extiWakeEvent = 1;
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PC13)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PC13);
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PB0)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PB0);
        device_t.extiWakeEvent = 1;
    }
    if (SET == PMU_WKFR_WKPxF_Chk(PINWKEN_PF5)) {
        PMU_WKFR_WKPxF_Clr(PINWKEN_PF5);
    }
}

void PMU_SleepCfg(void)
{
    PMU_SleepCfg_InitTypeDef SleepCfg_InitStruct;
    CDIF_CR_INTF_EN_Setable(ENABLE);
    /*下电复位配置*/
    // pdr和bor两个下电复位至少要打开一个
    // 当电源电压低于下电复位时，芯片会被复位住
    // pdr电压档位不准但是功耗极低(几乎无测量）
    // bor电压档位准确但是需要增加2uA功耗
    RMU_PDRCR_PDREN_Setable(ENABLE);             // 打开PDR
    RMU_PDRCR_PDRCFG_Set(RMU_PDRCR_PDRCFG_1P4V); // pdr电压调整到1.4V
    RMU_BORCR_OFF_BOR_Setable(ENABLE);           // 关闭BOR
    VRTC_RCMFCR_EN_Setable(DISABLE);             // RCMF关闭
    // VRTC_RCLPCR_RCLP_OFF_Setable(ENABLE);		 // RCLP关闭

    CDIF_CR_INTF_EN_Setable(DISABLE);
    SleepCfg_InitStruct.PMOD = PMU_CR_PMOD_SLEEP;       // 功耗模式配置
    SleepCfg_InitStruct.SLPDP = PMU_CR_SLPDP_DEEPSLEEP; // deepsleep
    SleepCfg_InitStruct.CVS = DISABLE;                  // 内核电压降低控制
    SleepCfg_InitStruct.SCR = 0;                        // M0系统控制寄存器，一般配置为0即可
    SleepCfg_InitStruct.TIA = PMU_WKTR_T1A_8US;         // 可编程额外唤醒延迟8us

    PMU_SleepCfg_Init(&SleepCfg_InitStruct); // 休眠配置
    PMU_CR_WKFSEL_Set(clkmode - 1);
}

void PMU_LpRunCfg(void)
{
    CMU_SYSCLKCR_SYSCLKSEL_Set(CMU_SYSCLKCR_SYSCLKSEL_LSCLK); // 选择LSCLK做主时钟

    PMU_SleepCfg_InitTypeDef SleepCfg_InitStruct;
    CDIF_CR_INTF_EN_Setable(ENABLE);
    /*下电复位配置*/
    // pdr和bor两个下电复位至少要打开一个
    // 当电源电压低于下电复位时，芯片会被复位住
    // pdr电压档位不准但是功耗极低(几乎无测量）
    // bor电压档位准确但是需要增加2uA功耗
    RMU_PDRCR_PDREN_Setable(ENABLE);             // 打开PDR
    RMU_PDRCR_PDRCFG_Set(RMU_PDRCR_PDRCFG_1P4V); // pdr电压调整到1.4V
    RMU_BORCR_OFF_BOR_Setable(ENABLE);           // 关闭BOR
    VRTC_RCMFCR_EN_Setable(DISABLE);             // RCMF关闭
    // VRTC_RCLPCR_RCLP_OFF_Setable(ENABLE);		 // RCLP关闭

    CDIF_CR_INTF_EN_Setable(DISABLE);
    SleepCfg_InitStruct.PMOD = PMU_CR_PMOD_LPRUN;       // 功耗模式配置
    SleepCfg_InitStruct.SLPDP = PMU_CR_SLPDP_DEEPSLEEP; // deepsleep
    SleepCfg_InitStruct.CVS = DISABLE;                  // 内核电压降低控制
    SleepCfg_InitStruct.SCR = 0;                        // M0系统控制寄存器，一般配置为0即可
    SleepCfg_InitStruct.TIA = PMU_WKTR_T1A_8US;         // 可编程额外唤醒延迟8us

    PMU_SleepCfg_Init(&SleepCfg_InitStruct); // 休眠配置
}
