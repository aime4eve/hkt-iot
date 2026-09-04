
/* Includes ------------------------------------------------------------------*/
#include "LoRaWAN_APPLY.h"
#include "LoRaWAN_ATCMD.h"
#include "acc.h"
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "control_center.h"
#include "flash.h"
#include "gpio.h"
#include "gps.h"
#include "iic.h"
#include "lwrb.h"
#include "p25q40.h"
#include "rtc.h"
#include "sht40x.h"
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
    PeriphCommonClock_Config();
    App_PortCfg();
    App_Uart1Cfg();
    App_LpUart1Cfg();
    App_Uart2Cfg();
    App_Uart3Cfg();
    Spi2Init();
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
    SHT40X_Init();
    
    Acc_Init();
    LoRaWAN_Init();
    ADC_BatterySglConvert();
    Device_SglConvert();
    memset(uartBle.recvData, 0, sizeof(uartBle.recvData));
    memset(uartLoRa.recvData, 0, sizeof(uartLoRa.recvData));
    device_t.sleep_delay = 0;
    
    /* Infinite loop */
    while (1) {
        systemLowPowerMode_Config();
        LoRaWAN_JoinInit();
        Gps_Handler();
        Acc_Handler();
        fromUartDataHandle();
        DeviceStateProcess();
        LL_IWDG_ReloadCounter(IWDG);
        // lwrbTest();
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
