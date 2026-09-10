
#include "systick.h"
#include "LoRaWAN_APPLY.h"
#include "communicate.h"
#include "gpio.h"
#include "tim.h"
#include "uart.h"

static u32 sys_pant_ms; // 系统开启时间,单位ms
static u32 sys_OPTime_ss;
static u32 sys_OPTime_ms;
static u16 systick_ms;

time_t Timestamp;

void SystemClock_Config(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {
    }
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE2);
    LL_RCC_HSI_Enable();

    /* Wait till HSI is ready */
    while (LL_RCC_HSI_IsReady() != 1) {
    }
    LL_RCC_HSI_SetCalibTrimming(16);
    LL_PWR_EnableBkUpAccess();
#if RTC_CLOCK_SOURCE_LSE
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
    }
    LL_RCC_LSE_SetDriveCapability(LL_RCC_LSEDRIVE_HIGH);
    LL_RCC_LSE_Enable();

    /* Wait till LSE is ready */
    while (LL_RCC_LSE_IsReady() != 1) {
    }
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSE) {
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
    }
#else
    LL_RCC_LSI_Enable();

    /* Wait till LSI is ready */
    while (LL_RCC_LSI_IsReady() != 1) {
    }
    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSI) {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
    }
#endif
    // LL_RCC_EnableRTC();
    LL_RCC_HSI_EnableDivider();
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {
    }
    LL_Init1msTick(4000000);
    LL_SetSystemCoreClock(4000000);

    /* System interrupt init*/
    /* SysTick_IRQn interrupt configuration */
    NVIC_SetPriority(SysTick_IRQn, 3);

    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_LSE);
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
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
    systick_ms   = 0;
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
    static u16 led_tick;
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
    if (uartBT.recvTimeout)
        uartBT.recvTimeout--;
    if (uartBT.sendDelay)
        uartBT.sendDelay--;

    if (sys_OPTime_ms >= SEC_DELAY) {
        sys_OPTime_ms = 0;
        sys_OPTime_ss++;
    }
    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        if (device_t.sleepDelay)
            device_t.sleepDelay--;
        Timestamp++;
    }

    if (!LoRaWAN.joinState) {
        if (led_tick++ == 500) {
            GPIO_LED_H;
        } else if (led_tick++ == 1000) {
            led_tick = 0;
            GPIO_LED_L;
        }
    } else {
        GPIO_LED_L;
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
