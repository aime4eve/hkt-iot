
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "tim.h"
#include "systick.h"

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
    CMU_PERCLK_SetableEx(PADCLK, ENABLE);                 // IO控制时钟寄存器使能

    /** 初始化LED IO **/
    OutputIO(LED_GREEN_PORT, LED_GREEN_PIN, OUT_PUSHPULL);
}

/**
 * @brief  用于LED闪烁
 * @param
 * @retval
 */
void Led_Blink(void)
{
    static u16 blinkcnt;
	
		blinkcnt++;
    if (blinkcnt == 500)
    {
        LED_GREEN_TOGGLEBITS;
    }
}
