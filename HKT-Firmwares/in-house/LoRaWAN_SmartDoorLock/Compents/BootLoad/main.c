
/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "tim.h"
#include "uart.h"
#include "gpio.h"
#include "systick.h"
#include "flash.h"
#include "crc.h"

/**
 * @brief  程序入口
 * @param
 * @retval
 */
int main(void)
{
    /*系统初始化*/
    Init_System();
    Uartx_Init(UART0, 115200); // 串口打印初始化
    FLASH_Read_Update_Flag();
    BSTIM_Init(1000, RCHFCLKCFG); // 定时1ms中断
    Init_CRC_CRC_CCITT();
    App_PortCfg();
    DEBUG_TRACE(LOG_TAG, "Run BootLoad");
    while (1)
    {
        fromInfoDataHandle();
        IWDT_Clr();
    }
}
