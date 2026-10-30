
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "gpio.h"
#include "tim.h"
#include "uart.h"
#include "adc.h"
#include "flash.h"
#include "systick.h"
#include "control_center.h"
#include "communicate.h"
#include <LoRaWAN_APPLY.h>
#include "lwrb.h"

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
    TIM6_Init(1000, 4);
    App_PortCfg();
    App_Uart2Cfg();
    App_LpUart1Cfg();
    //App_Uart1Cfg();
    FLASH_Read_Param();
    lwrbDataInit();
    LoRaWAN_Init();
    /* Infinite loop */
    while (1)
    {
        LoRaWAN_JoinInit();
        fromUartDataHandle();
        DeviceStateProcess();
        DeviceSleepEventProcess();
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

/*
 * 客户定制软件需求
 * 设备共有两种模式，分为模式一与模式二，默认模式二
 * 模式二状态下为触发后立即上报，即触发门磁或者防拆后，立马上报门磁状态、防拆状态、电量状态
 * 模式一为周期上报，默认间隔为30分钟，且仅在设定时间段内有效，默认运行时间段为8：00-16：00，监控时间段外执行模式二，模式一上报数据内容为30分钟内开关门的次数+历史触发总数
 * 设备心跳间隔默认为24小时，上报数据为软硬件版本号、电量信息、门磁状态、运行模式、计数时间段、计数上报间隔、心跳间隔（0-24hour,1-12hour,2-8hour）、防拆状态
 */
