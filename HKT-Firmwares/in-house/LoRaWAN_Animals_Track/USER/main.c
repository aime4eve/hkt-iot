
/* Includes ------------------------------------------------------------------*/
#include "LTR_3XXALS_01.h"
#include "TB_03F.h"
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
#include "sht40x.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

static time_t debugstamp;

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    Systick_Init();
    SystemClock_Config();
    TIM6_Init(1000, 1);
    App_PortCfg();
    App_Uart1Cfg();
    App_LpUart1Cfg();
    App_Uart2Cfg();
    I2C1_Init();

    FLASH_Read_Param();
    systemTime_Init();
    lwrbDataInit();
#if FUNC_THTB_ENABLE
    SHT40X_Init();
#endif
    // device_t.powerState = 1;
#if TRACK_ACC_ENABLE
    Acc_Init();
#endif
    LoRaWAN_Init();
    BEEP_RUN(200, 50);
    // device_t.gpsLocateInterval = 1;
    // ble_init();
    /* Infinite loop */
    while (1) {
        if (debugstamp <= Timestamp) {
            debugstamp = Timestamp + 10;
            DEBUG_TRACE(LOG_TAG,
                        "Current GPS Report Interval %dmin, GPS Reopen Timestamp %d, Systime Now :%04d-%02d-%02d:%02d:%02d:%02d , Timestamp :%ld",
                        device_t.gpsLocateInterval, gps_stamp, systime.tm_year, systime.tm_mon + 1, systime.tm_mday, systime.tm_hour, systime.tm_min,
                        systime.tm_sec, Timestamp);
        }
        systemLowPowerMode_Config();
        LoRaWAN_JoinInit();
        DeviceStateProcess();
        fromUartDataHandle();
        Gps_Handler();
#if TRACK_ACC_ENABLE
        Acc_Handler();
#endif
#if FUNC_THTB_ENABLE
        readSHT40XConvertResult();
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
    while (1) {
    }
    /* USER CODE END Error_Handler_Debug */
}
