
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "systick.h"

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void BFTM0_IRQHandler(void)
{
    BFTM_ClearFlag(HT_BFTM0);
    Systick_1ms_Handler();
}

/**
 * @brief  初始化通用定时器 1ms中断
 * @retval
 */
void BFTIM_Init(void)
{
    /* Enable peripheral clock                                                                              */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.BFTM0 = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    /* BFTM as Repetitive mode, every 1 ms to trigger the match interrupt                               */
    BFTM_SetCompare(HT_BFTM0, 8000000 / 1000);
    BFTM_SetCounter(HT_BFTM0, 0);
    BFTM_IntConfig(HT_BFTM0, ENABLE);
    BFTM_EnaCmd(HT_BFTM0, ENABLE);

    NVIC_EnableIRQ(BFTM0_IRQn);
}
