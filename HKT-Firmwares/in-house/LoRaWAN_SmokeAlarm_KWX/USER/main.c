
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "communicate.h"
#include "config.h"
#include "flash.h"
#include "gpio.h"
#include "ht32_time.h"
#include "rtc.h"
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
    SystemClock_Config();
    Systick_Init();
    LoRa_UartCfg();
    Info_UartCfg();
    BFTIM_Init();
    App_PortCfg();
    RTC_Configuration(); 
    App_Init();
    LoRaWAN_Init();
    lwrbDataInit();
    /* Infinite loop */
    while (1) {
        LoRaWAN_JoinInit();
        fromUartDataHandle();
        DeviceStateProcess();
        Gpio_Event();
        DeviceSleepEventProcess();
        WDT_Restart();
    }
}
