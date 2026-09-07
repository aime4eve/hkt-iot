
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

u32 delay_ms_tick;
u32 sytick_tick;
time_t Timestamp;

/**
 * @brief  Configure the NVIC vector table.
 * @retval None
 */
void NVIC_Configuration(void)
{
    NVIC_SetVectorTable(NVIC_VECTTABLE_FLASH, 0x0); /* Set the Vector Table base location at 0x00000000   */
}

/**
 * @brief 初始化系统时钟 HSI 8M
 * @retval
 */
void SystemClock_Config(void)
{
    CKCU_ClocksTypeDef ClockFreq;
    /* Reset CKCU, SYSCLK = HSI */
    // CKCU_DeInit();
    /* Reset Backup Domain                                                                                    */
    // PWRCU_DeInit();

    /* Enable HSI */
    CKCU_HSICmd(ENABLE);
    // /* HCLK = SYSCLK/4 */
    // CKCU_SetHCLKPrescaler(CKCU_SYSCLK_DIV4);
    /* Configure system clock */
    CKCU_SysClockConfig(CKCU_SW_HSI);
    /* Get the current clocks setting */
    CKCU_GetClocksFrequency(&ClockFreq);

    CKCU_HSIAutoTrimClkConfig(CKCU_ATC_LSE);
    CKCU_HSIAutoTrimCmd(ENABLE);

    /* FLASH wait state configuration */
    FLASH_SetWaitState(FLASH_WAITSTATE_0); /* FLASH zero wait clock */

    /* Enable peripheral clock                                                                              */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.BKP = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    CKCU_MCUDBGConfig(CKCU_DBG_WDT_HALT, 1);
}

/**
 * @brief	初始化系统时钟
 * @param
 * @retval
 */
void Systick_Init(void)
{
    /* SYSTICK configuration */
    SYSTICK_ClockSourceConfig(SYSTICK_SRC_FCLK);
    SYSTICK_SetReloadValue(0x5);
    SYSTICK_CounterCmd(SYSTICK_COUNTER_CLEAR);
    SYSTICK_CounterCmd(SYSTICK_COUNTER_ENABLE);
    SysTick->CTRL; // Fix the timing issue when using delayMicroseconds() for the first time
}

/**
 * @brief  Configure the Watchdog
 * @retval None
 */
void WDT_Configuration(void)
{
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.WDT = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    /* Enable WDT APB clock                                                                                   */
    /* Reset WDT                                                                                              */
    WDT_DeInit();
    /* Set Prescaler Value, 32K/64 = 500 Hz                                                                   */
    WDT_SetPrescaler(WDT_PRESCALER_64);
    /* Set Prescaler Value, 500 Hz/4000 = 0.125 Hz                                                            */
    WDT_SetReloadValue(4000);
    /* Set Delta Value, 500 Hz/4000 = 0.125 Hz                                                                */
    WDT_SetDeltaValue(4000);
    WDT_Restart(); // Reload Counter as WDTV Value
#if 0
  WDT_ResetCmd(ENABLE);             // Enable the WDT Reset when WDT meets underflow or error.
#endif
    WDT_Cmd(ENABLE);        // Enable WDT
    WDT_ProtectCmd(ENABLE); // Enable WDT Protection
}

void delay_1us(uint32_t count)
{
    // while (count--)
    // {
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    //     __NOP();
    // }

    SysTick->LOAD = US2TICK(count);
    SysTick->VAL = 0;
    while (SysTick->CTRL == 5)
        ;
}

void delay_1ms(uint32_t count)
{
    delay_ms_tick = count;
    while (delay_ms_tick)
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
    sytick_tick++;
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
    if (delay_ms_tick)
        delay_ms_tick--;

    if (sys_OPTime_ms >= SEC_DELAY) {
        sys_OPTime_ms = 0;
        sys_OPTime_ss++;
    }
    if (SysTicker >= SEC_DELAY) {
        SysTicker = 0;
        if (device_t.sleep_delay)
            device_t.sleep_delay--;
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
