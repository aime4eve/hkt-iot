
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

void IWDG_Init(void)
{
    /* Configure the IWDG with window option disabled */
    /* ------------------------------------------------------- */
    /* (1) Enable the IWDG by writing 0x0000 CCCC in the IWDG_KR register */
    /* (2) Enable register access by writing 0x0000 5555 in the IWDG_KR register */
    /* (3) Write the IWDG prescaler by programming IWDG_PR from 0 to 7 - LL_IWDG_PRESCALER_4 (0) is lowest divider*/
    /* (4) Write the reload register (IWDG_RLR) */
    /* (5) Wait for the registers to be updated (IWDG_SR = 0x0000 0000) */
    /* (6) Refresh the counter value with IWDG_RLR (IWDG_KR = 0x0000 AAAA) */
    LL_IWDG_Enable(IWDG);                              /* (1) */
    LL_IWDG_EnableWriteAccess(IWDG);                   /* (2) */
    LL_IWDG_SetPrescaler(IWDG, LL_IWDG_PRESCALER_256); /* (3) */
    LL_IWDG_SetReloadCounter(IWDG, 0xFFFF);            /* (4) */
    while (LL_IWDG_IsReady(IWDG) != 1)                 /* (5) */
    {
    }
    LL_IWDG_ReloadCounter(IWDG); /* (6) */

//    FLASH_OBProgramInitTypeDef OptionsBytesStruct;
//    /* Get the configuration status */
//    HAL_FLASHEx_OBGetConfig(&OptionsBytesStruct);
//    if ((OptionsBytesStruct.USERConfig & (FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)) != 0) {
//        // 没有配置过就配置一次，有必要判断一下是否配置过，因为每次配置完都会导致重启，不能每次上电都无条件配置一次
//        OptionsBytesStruct.OptionType = OPTIONBYTE_USER;
//        OptionsBytesStruct.USERType = OB_USER_IWDG_STOP | OB_USER_IWDG_SW;
//        OptionsBytesStruct.USERConfig &= (~(FLASH_OPTR_IWDG_STOP | FLASH_OPTR_IWDG_SW)); // STOP模式下停止看门狗计数
//        HAL_FLASH_Unlock();
//        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
//        HAL_FLASH_OB_Unlock();
//        if (HAL_FLASHEx_OBProgram(&OptionsBytesStruct) != HAL_OK) {
//            // 配置失败，都说重启大法好，我就重启下试试...
//            NVIC_SystemReset();
//        }
//        if (HAL_FLASH_OB_Launch() != HAL_OK) // 加载flash配置，这里会导致重启
//        {
//            NVIC_SystemReset();
//        }
//        HAL_FLASH_OB_Lock();
//        HAL_FLASH_Lock();
//    }
}

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    Systick_Init();
    SystemClock_Config();
    systemTime_Init();
    App_PortCfg();
    App_LpUart1Cfg();
    App_Uart3Cfg();
    TIM16_Init(1000, 16);
    FLASH_Read_Update_Flag();
    IWDG_Init();
    memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
    /* Infinite loop */
    while (1) {
        fromUartDataHandle();
        LL_IWDG_ReloadCounter(IWDG);
    }
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}
