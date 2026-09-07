
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

#define LED_ORANGE_PORT GPIOC
#define LED_ORANGE_PIN GPIO_Pin_8
#define LED_ORANGE_H GPIO_SetBits(LED_ORANGE_PORT, LED_ORANGE_PIN)
#define LED_ORANGE_L GPIO_ResetBits(LED_ORANGE_PORT, LED_ORANGE_PIN)
#define LED_ORANGE_TOGGLEBITS GPIO_ToggleBits(LED_ORANGE_PORT, LED_ORANGE_PIN)

#define LED_RED_PORT GPIOC
#define LED_RED_PIN GPIO_Pin_9
#define LED_RED_H GPIO_SetBits(LED_RED_PORT, LED_RED_PIN)
#define LED_RED_L GPIO_ResetBits(LED_RED_PORT, LED_RED_PIN)
#define LED_RED_TOGGLEBITS GPIO_ToggleBits(LED_RED_PORT, LED_RED_PIN)


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
