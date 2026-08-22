
/* Includes ------------------------------------------------------------------*/
#include "communicate.h"
#include "config.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

void App_Init(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("*************** HKT : Car Lock ***************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa
}

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    SystemClock_Config();
    TIM6_Init(1000, 4);
    App_PortCfg();
    App_Uart1Cfg();
    App_Uart2Cfg();
    App_LpUart1Cfg();

    App_Init();
    LoRaWAN_Init();
    lwrbDataInit();
    /* Infinite loop */
    while (1) {
        LoRaWAN_JoinInit();
        fromUartDataHandle();
        DeviceStateProcess();
        // systemLowPowerMode_Config();
    }
}
