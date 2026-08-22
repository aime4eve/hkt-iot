
#ifndef __GPIO_H__
#define __GPIO_H__

/* Includes ------------------------------------------------------------------*/
#include <config.h>

// ** LED IO ** //
#define LED_GREEN_PORT GPIOC
#define LED_GREEN_PIN GPIO_Pin_7
#define LED_GREEN_H GPIO_SetBits(LED_GREEN_PORT, LED_GREEN_PIN)
#define LED_GREEN_L GPIO_ResetBits(LED_GREEN_PORT, LED_GREEN_PIN)
#define LED_GREEN_TOGGLEBITS GPIO_ToggleBits(LED_GREEN_PORT, LED_GREEN_PIN)



#ifdef __cplusplus
extern "C"
{
#endif
	
  void App_PortCfg(void);
  void Led_Blink(void);

#ifdef __cplusplus
}
#endif

#endif
