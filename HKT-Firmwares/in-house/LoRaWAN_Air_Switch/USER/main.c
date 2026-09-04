
/* Includes ------------------------------------------------------------------*/
#include "an001.h"
#include "communicate.h"
#include "config.h"
#include "flash.h"
#include "gpio.h"
#include "systick.h"
#include "tim.h"
#include "uart.h"
#include <LoRaWAN_APPLY.h>

void App_Init(void)
{
    INFO("\n************************************************************\n");
    INFO("************************************************************\n");
    INFO("*************** HKT : Air Switch ***************\n");
    INFO("******** HARDWARE_VER: 0x%02x ,SOFTWARE_VER: 0x%02x ********\n", HARDWARE_VER, SOFTWARE_VER);

    memcpy(LoRaWAN.AppKEY, appKey, sizeof(LoRaWAN.AppKEY));
    memcpy(LoRaWAN.AppEUI, appEui, sizeof(LoRaWAN.AppEUI));

    memcpy(LoRaWAN.DevADDR, devAddr, sizeof(LoRaWAN.DevADDR));
    memcpy(LoRaWAN.AppSKEY, appSKey, sizeof(LoRaWAN.AppSKEY));
    memcpy(LoRaWAN.NwkSKEY, nwkSKey, sizeof(LoRaWAN.NwkSKEY));
    LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT; // 初始化LoRa

    fmc_read_report_interval();
}

void Fwdgt_Init(void)
{
#if !FUNC_BOARD_VERSION_CL
    /* enable IRC40K */
    rcu_osci_on(RCU_IRC40K);
    /* wait till IRC40K is ready */
    while (SUCCESS != rcu_osci_stab_wait(RCU_IRC40K))
        ;

    /* confiure FWDGT counter clock: 40KHz(IRC40K) / 256 = 0.156 KHz */
    fwdgt_config(1560, FWDGT_PSC_DIV256); // 看门狗时间设置，大概10S(1560/0.156)不喂狗，芯片复位
    fwdgt_enable();                       // 看门狗使能
#endif
}

void fwdgt_reload(void)
{
#if !FUNC_BOARD_VERSION_CL
    fwdgt_counter_reload();
#endif
}

/**
 * @brief  Main program.
 * @param  None
 * @retval None
 */
int main(void)
{
    SystemClock_Config();
    Systick_Init();
#if FUNC_BOARD_VERSION_CL
    TIM6_Init(4000, 4); /* 16MHz / 4 / 4000 = 1000Hz = 1ms 系统节拍 */
    App_Uart1Cfg();
    App_Uart2Cfg();
    App_LpUart1Cfg();
#else
    TIM1_Init(100, 135 * 2); // 如果APBx（x=0,1）的分频系数不为1，TIMER时钟为CK_APBx（x=0,1）的两倍
    App_Uart0Cfg();
    App_Uart1Cfg();
    App_Uart2Cfg();
#endif

    App_PortCfg();

    App_Init();
    LoRaWAN_Init();
    lwrbDataInit();
#if !FUNC_BOARD_VERSION_CL
    Fwdgt_Init();
#endif

    /* Infinite loop */
    while (1) {
        LoRaWAN_JoinInit();
        fromUartDataHandle();
        DeviceStateProcess();
        as_event();
        fwdgt_reload(); // 喂狗
    }
}
