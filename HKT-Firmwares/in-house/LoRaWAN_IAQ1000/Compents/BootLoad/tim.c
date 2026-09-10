
/* Includes ------------------------------------------------------------------*/
#include "tim.h"
#include "systick.h"
#include "uart.h"
#include "gpio.h"

/**
 * @brief  定时器中断事件
 * @param
 * @retval
 */
void BSTIM_IRQHandler(void)
{
    if (BSTIM_ISR_UIF_Chk() != RESET)
    {
        BSTIM_ISR_UIF_Clr();
        Systick_1ms_Handler();
        Led_Blink();
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
    CMU_PERCLK_SetableEx(BSTIMCLK, ENABLE);          //外设总线时钟使能;
    CMU_OPCCR2_BSTCKS_Set(CMU_OPCCR2_BSTCKS_SYSCLK); //选择工作时钟源
    CMU_OPCCR2_BSTCKE_Setable(ENABLE);               //工作时钟源使能

    BSTIM_CR1_ARPE_Setable(ENABLE);            //预装载使能
    BSTIM_CR1_OPM_Set(BSTIM_CR1_OPM_CONTINUE); // UE产生时计数器不停止

    BSTIM_PSCR_Write(prescaler - 1); //
    BSTIM_ARR_Write(period - 1);     //
    // 0x10000 - u16Period;

    NVIC_DisableIRQ(BSTIM_IRQn);
    NVIC_SetPriority(BSTIM_IRQn, 2); //中断优先级配置
    NVIC_EnableIRQ(BSTIM_IRQn);

    BSTIM_IER_UIE_Setable(ENABLE); // Update中断使能
    BSTIM_CR1_CEN_Setable(ENABLE); //计数器使能
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
    DEBUG_TRACE(LOG_TAG, "Update Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld", systime.tm_year, systime.tm_mon + 1,
                systime.tm_mday, systime.tm_hour, systime.tm_min, systime.tm_sec, Timestamp);
#endif
}
