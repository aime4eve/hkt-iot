
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "lwrb.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

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
    // TIM6_Init(1000, 2);
    TIM6_Init(2097, 1); // 2097
    App_PortCfg();
    App_Uart2Cfg();
    App_LpUart1Cfg();
    IWDG_Configuration();
    FLASH_Read_Param();
    lwrbDataInit();
    LoRaWAN_Init();
    NVIC_EnableIRQ(EXTI4_15_IRQn);
    device_t.mode = ALIGNMENT_MODE;
    device_t.mode_timeout = SEC_DELAY * 60; // 维持一分钟的对准模式
    /* Infinite loop */
    while (1) {
        LoRaWAN_JoinInit();
        fromUartDataHandle();
        DeviceStateProcess();
        Run_People_Counter_Handle();
        IWDG_Feed();
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
