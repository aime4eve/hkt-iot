
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "systick.h"


/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void TIM6_DAC_IRQHandler(void)
{
 //   if (LL_TIM_IsActiveFlag_UPDATE(TIM6))
    {
        LL_TIM_ClearFlag_UPDATE(TIM6);
        Systick_1ms_Handler();
    }
}

/**
 * @brief  初始化通用定时器
 * @param  period：计数周期（实际值）
 * @param  prescaler：预分频值（实际值）
 * @retval
 */
void TIM6_Init(u16 period, u16 prescaler)
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);

    /* TIM6 interrupt Init */
    NVIC_SetPriority(TIM6_DAC_IRQn, 0);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    TIM_InitStruct.Prescaler = prescaler - 1;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = period - 1;
    LL_TIM_Init(TIM6, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM6);
    LL_TIM_SetTriggerOutput(TIM6, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM6);
    LL_TIM_EnableCounter(TIM6);
    LL_TIM_EnableUpdateEvent(TIM6);
    LL_TIM_EnableIT_UPDATE(TIM6);
}



/**
 * @brief  Function to enter in Stop mode.
 * @param  None
 * @retval None
 */
void EnterStopMode(void)
{
    /** Request to enter Stop mode
     * Following procedure describe in STM32L0xx Reference Manual
     * See PWR part, section Low-power modes, Stop mode
     */
    /** Set the regulator to low power before setting MODE_STOP.
     * If the regulator remains in "main mode", it consumes
     * more power without providing any additional feature. */
    LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);
    /* Set STOP mode when CPU enters deepsleep */
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);

    /* Set SLEEPDEEP bit of Cortex System Control Register */
    LL_LPM_EnableDeepSleep();

    /* Request Wait For Interrupt */
    __WFI();
}

