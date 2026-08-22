
#include "systick.h"
#include "uart.h"
#include "communicate.h"
#include "LoRaWAN_APPLY.h"
#include "tim.h"
#include "an001.h"

static u32 sys_pant_ms; // 系统开启时间,单位ms
static u32 sys_OPTime_ss;
static u32 sys_OPTime_ms;
static u16 systick_ms;
time_t Timestamp;

volatile static float count_1us = 0;
volatile static float count_1ms = 0;

#if FUNC_BOARD_VERSION_CL
void SystemClock_Config(void)
{
    /* 1. 设置 Flash 等待周期 */
    /* 频率达到16MHz，需要1个等待周期[reference:4] */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_1) {}

    /* 2. 配置电源电压范围 */
    /* STM32L051在16MHz下需确保电压范围在Range 1 (1.8V ~ 3.6V)[reference:5] */
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1); 

    /* 3. 启用并等待 HSI 就绪 */
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1) {}

    /* 4. 配置 AHB/APB 总线分频器 (不分频) */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_1);

    /* 5. 将系统时钟源切换为 HSI */
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

    /* 等待切换完成 */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI) {}

    /* 6. 更新 SysTick 和系统核心时钟变量 */
    /* 系统时钟频率为 16,000,000 Hz */
    LL_Init1msTick(16000000);
    LL_SetSystemCoreClock(16000000);

    /* 7. (可选) 如果不再需要 MSI，可以将其关闭以省电 */
    LL_RCC_MSI_Disable();

    /* 以下为外设时钟源配置，保持不变或根据实际需求调整 */
    LL_RCC_SetUSARTClockSource(LL_RCC_USART1_CLKSOURCE_PCLK2);
    LL_RCC_SetUSARTClockSource(LL_RCC_USART2_CLKSOURCE_PCLK1);
    LL_RCC_SetLPUARTClockSource(LL_RCC_LPUART1_CLKSOURCE_PCLK1);
    LL_RCC_SetI2CClockSource(LL_RCC_I2C1_CLKSOURCE_PCLK1);
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

	/*
	 * 初始化延时校准值.
	 * LL_Init1msTick 已将 SysTick 时钟源设为 HCLK (16MHz),
	 * 但 delay_1us/delay_1ms 函数会强制使用 HCLK/8 模式,
	 * 因此 count_1us = SystemCoreClock / 8000000.
	 * 参考: SysTick->CTRL CLKSOURCE=0 → HCLK/8
	 */
	count_1us = (float)SystemCoreClock / 8000000;
	count_1ms = (float)count_1us * 1000;
}
#else
/**
 * @brief 初始化系统时钟 PLL 108M
 * @retval
 */
void SystemClock_Config(void)
{
    uint32_t timeout = 0U;
    uint32_t stab_flag = 0U;

    /* select IRC8M as system clock source, deinitialize the RCU */
    rcu_system_clock_source_config(RCU_CKSYSSRC_IRC8M);
    rcu_deinit();

    /* enable IRC8M */
    RCU_CTL |= RCU_CTL_IRC8MEN;

    /* wait until IRC8M is stable or the startup time is longer than IRC8M_STARTUP_TIMEOUT */
    do
    {
        timeout++;
        stab_flag = (RCU_CTL & RCU_CTL_IRC8MSTB);
    } while ((0U == stab_flag) && (IRC8M_STARTUP_TIMEOUT != timeout));

    /* if fail */
    if (0U == (RCU_CTL & RCU_CTL_IRC8MSTB))
    {
        while (1)
        {
        }
    }

    /* IRC8M is stable */
    /* AHB = SYSCLK */
    RCU_CFG0 |= RCU_AHB_CKSYS_DIV1;
    /* APB2 = AHB/1 */
    RCU_CFG0 |= RCU_APB2_CKAHB_DIV8;
    /* APB1 = AHB/2 */
    RCU_CFG0 |= RCU_APB1_CKAHB_DIV8;

    /* CK_PLL = (CK_IRC8M/2) * 27 = 108 MHz */
    RCU_CFG0 &= ~(RCU_CFG0_PLLMF | RCU_CFG0_PLLMF_4);
    RCU_CFG0 |= RCU_PLL_MUL27;

    /* enable PLL */
    RCU_CTL |= RCU_CTL_PLLEN;

    /* wait until PLL is stable */
    while (0U == (RCU_CTL & RCU_CTL_PLLSTB))
    {
    }

    /* select PLL as system clock */
    RCU_CFG0 &= ~RCU_CFG0_SCS;
    RCU_CFG0 |= RCU_CKSYSSRC_PLL;

    /* wait until PLL is selected as system clock */
    while (RCU_SCSS_PLL != (RCU_CFG0 & RCU_CFG0_SCSS))
    {
    }
}

/**
 * @brief	初始化系统时钟
 * @param
 * @retval
 */
void Systick_Init(void)
{
    /* systick clock source is from HCLK/8 */
    systick_clksource_set(SYSTICK_CLKSOURCE_HCLK_DIV8);
    count_1us = (float)SystemCoreClock / 8000000;
    count_1ms = (float)count_1us * 1000;
}

#endif

/*!
    \brief      delay a time in microseconds in polling mode
    \param[in]  count: count in microseconds
    \param[out] none
    \retval     none
*/
void delay_1us(uint32_t count)
{
    uint32_t ctl;
    uint32_t saved_load = SysTick->LOAD;
    uint32_t saved_ctrl = SysTick->CTRL;

    /* reload the count value */
    SysTick->LOAD = (uint32_t)(count * count_1us);
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* enable the systick timer (CLKSOURCE=0 → HCLK/8, no interrupt) */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do
    {
        ctl = SysTick->CTRL;
    } while ((ctl & SysTick_CTRL_ENABLE_Msk) && !(ctl & SysTick_CTRL_COUNTFLAG_Msk));
    /* disable the systick timer */
    SysTick->CTRL = 0x0000U;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* restore systick configuration (preserves 1ms tick if previously enabled) */
    SysTick->LOAD = saved_load;
    SysTick->CTRL = saved_ctrl;
}

/*!
    \brief      delay a time in milliseconds in polling mode
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void delay_1ms(uint32_t count)
{
    uint32_t ctl;
    uint32_t saved_load = SysTick->LOAD;
    uint32_t saved_ctrl = SysTick->CTRL;

    /* reload the count value */
    SysTick->LOAD = (uint32_t)(count * count_1ms);
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* enable the systick timer (CLKSOURCE=0 → HCLK/8, no interrupt) */
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk;
    /* wait for the COUNTFLAG flag set */
    do
    {
        ctl = SysTick->CTRL;
    } while ((ctl & SysTick_CTRL_ENABLE_Msk) && !(ctl & SysTick_CTRL_COUNTFLAG_Msk));
    /* disable the systick timer */
    SysTick->CTRL = 0x0000U;
    /* clear the current count value */
    SysTick->VAL = 0x0000U;
    /* restore systick configuration (preserves 1ms tick if previously enabled) */
    SysTick->LOAD = saved_load;
    SysTick->CTRL = saved_ctrl;
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
    if (uartAS.recvTimeout)
        uartAS.recvTimeout--;
    if (uartAS.sendDelay)
        uartAS.sendDelay--;

    if (sys_OPTime_ms >= SEC_DELAY)
    {
        sys_OPTime_ms = 0;
        sys_OPTime_ss++;
    }
    if (SysTicker >= SEC_DELAY)
    {
        SysTicker = 0;
        Timestamp++;

        if (as_an001_t.period_upload_real_timeout)
            as_an001_t.period_upload_real_timeout--;
        if (as_an001_t.period_refresh_real_time)
            as_an001_t.period_refresh_real_time--;
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
