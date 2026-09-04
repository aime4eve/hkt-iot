
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
#include "qmc5883l.h"
#include "rkb1145d.h"
#include "rtc.h"
#include "spi.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <cm_backtrace.h>

#define HARDWARE_VERSION "V1.0.0"
#define SOFTWARE_VERSION "V0.1.0"

#include "stm32l4xx.h"

void check_reset_cause(void)
{
    uint32_t csr_value = RCC->CSR;
    if (csr_value & RCC_CSR_FWRSTF) {
        // 闪存看门狗复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From FW");
    }
    if (csr_value & RCC_CSR_OBLRSTF) {
        // 选项字节加载复位标志
        DEBUG_TRACE(WARN_TAG, "System Restart From OBL");
    }
    if (csr_value & RCC_CSR_PINRSTF) {
        // 引脚复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From RST PIN");
    }
    if (csr_value & RCC_CSR_BORRSTF) {
        // 欠压复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From BOR");
    }
    if (csr_value & RCC_CSR_SFTRSTF) {
        // 软件复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From SOFTER");
    }
    if (csr_value & RCC_CSR_IWDGRSTF) {
        // 独立看门狗复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From IWD");
    }
    if (csr_value & RCC_CSR_WWDGRSTF) {
        // 窗口看门狗复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From WWD");
    }
    if (csr_value & RCC_CSR_LPWRRSTF) {
        // 低功耗复位发生
        DEBUG_TRACE(WARN_TAG, "System Restart From LPWR");
    }

    // 使用RCC_CSR_RMVF清除复位标志位
    RCC->CSR |= RCC_CSR_RMVF;
}

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    /*系统初始化*/
    Systick_Init();
    SystemClock_Config();
    PeriphCommonClock_Config();
    App_PortCfg();
    App_Uart1Cfg();
    // App_LpUart1Cfg();
    App_Uart2Cfg();
    App_Uart3Cfg();
    // I2C1_DeInit();
    I2C1_Init();
    TIM16_Init(1000, 16);
    // LPTIM1_Init();
    Spi3Init();
    cm_backtrace_init("LoRaWAN_ParkingSensor", HARDWARE_VERSION, SOFTWARE_VERSION);
    __enable_irq();
    systemTime_Init();
    FLASH_Read_Param();

    IWDG_Init();
#if NOR_FLASH_ENABLE
    flash_db_init();
#endif
    lwrbDataInit();
    magnetometer_init(); // 磁力计初始化
    convert_light_sensor();
    rkb_init();     // 雷达模块初始化
    LoRaWAN_Init(); // LoRaWAN模块初始化

    Delay_Ms(100);
    memset(&uartBle, 0, sizeof(uartBle));
    memset(&uartLoRa, 0, sizeof(uartLoRa));
    memset(&uartRd, 0, sizeof(uartRd));
    App_LpUart1Cfg();
    check_reset_cause();
    // rkb_response_time_test();
    // rkb_period_read_test();
    // device_t.sleep_delay = 0;
    // device_t.power_on = 1;
    // LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
    // LPTIM1_Init();
    /* Infinite loop */
    while (1) {
        systemLowPowerMode_Process();
        LoRaWAN_JoinInit();
        DeviceStateProcess();
        fromUartDataHandle();
        magnetometer_work_process();
        rkb_period_event();
        rkb_calibration_event();
        park_process();
        system_debug();
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
