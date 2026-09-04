
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "gpio.h"
#include "tim.h"
#include "uart.h"
#include "adc.h"
#include "flash.h"
#include "iic.h"
#include "systick.h"
#include "control_center.h"
#include "communicate.h"
#include "lwrb.h"
#include "gps.h"
#include "spi.h"
#include "ens210.h"
#include "lis2dh12.h"
#include <LoRaWAN_APPLY.h>

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef hlpuart1;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
RTC_HandleTypeDef hrtc;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim6;

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();
    /* Configure the system clock */
    SystemClock_Config();
    TIM6_Init(1000, 4);
    MX_GPIO_Init();
    MX_LPUART1_UART_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_ADC_Init();
    BEEP_RUN(500, 50);
    FLASH_Read_Param();
    ENS210_Convert();
    systemTime_Init();
    ADC_BatterySglConvert();
    LoRaWAN_Init();
    // device_t.powerState = 1;
#if FUNC_ACC_ENABLE
    Lis2dh12_Init();
#endif
    /* Infinite loop */
    while (1)
    {
        systemLowPowerMode_Config();
        LoRaWAN_JoinInit();
        DeviceStateProcess();
        fromUartDataHandle();
#if FUNC_GPS_ENABLE
        Gps_Handler();
#endif
#if FUNC_ACC_ENABLE
        lis2dh12_handler();
#endif
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
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
