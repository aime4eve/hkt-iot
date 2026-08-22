
/* Includes ------------------------------------------------------------------*/
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "iic.h"
#include "lwrb.h"
#include "p25q40.h"
#include "rtc.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    Systick_Init();
    SystemClock_Config();
    // PeriphCommonClock_Config();
    App_PortCfg();
    App_Uart1Cfg();
    App_LpUart1Cfg();
    App_Uart3Cfg();
    Spi3Init();
    I2C1_Init();
    TIM16_Init(1000, 16);
    __enable_irq();
    IWDG_Init();
    FLASH_Read_Param();
#if NOR_FLASH_ENABLE
    flash_db_init();
#endif

    lwrbDataInit();
    
    LoRaWAN_Init();
    BEEP_RUN(200, 50);

    // back_1_control();
    // back_2_control();
    // LL_IWDG_ReloadCounter(IWDG);
    // forward_1_control();
    // forward_2_control();
    LL_IWDG_ReloadCounter(IWDG);
    memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
    device_t.sleep_delay = 0;
    // device_t.power_on = 1;
    // LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;

    /* Infinite loop */
    while (1) {
#if (!FUNC_DISSLEEP)
        systemLowPowerMode_Config();
#endif
        LoRaWAN_JoinInit();
        DeviceStateProcess();
        fromUartDataHandle();
        local_schedule_event();
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
