
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define GPIO_KEY_PIN GPIO_PIN_0
#define GPIO_KEY_PORT GPIOA

// ** LED IO ** //
#define GPIO_LED_PIN GPIO_PIN_3
#define GPIO_LED_PORT GPIOB
#define GPIO_LED_L LL_GPIO_ResetOutputPin(GPIO_LED_PORT, GPIO_LED_PIN)
#define GPIO_LED_H LL_GPIO_SetOutputPin(GPIO_LED_PORT, GPIO_LED_PIN)

#define GPIO_LED1_PIN GPIO_PIN_8
#define GPIO_LED1_PORT GPIOB
#define GPIO_LED1_L LL_GPIO_ResetOutputPin(GPIO_LED1_PORT, GPIO_LED1_PIN)
#define GPIO_LED1_H LL_GPIO_SetOutputPin(GPIO_LED1_PORT, GPIO_LED1_PIN)

#define GPIO_LED2_PIN GPIO_PIN_9
#define GPIO_LED2_PORT GPIOB
#define GPIO_LED2_L LL_GPIO_ResetOutputPin(GPIO_LED2_PORT, GPIO_LED2_PIN)
#define GPIO_LED2_H LL_GPIO_SetOutputPin(GPIO_LED2_PORT, GPIO_LED2_PIN)

#define LoRaWAN_WAKE_PIN GPIO_PIN_12
#define LoRaWAN_WAKE_PORT GPIOA

#define LoRaWAN_NRST_PIN GPIO_PIN_8
#define LoRaWAN_NRST_PORT GPIOA

#define LoRaWAN_IRQ_PIN GPIO_PIN_12
#define LoRaWAN_IRQ_PORT GPIOB

void App_PortCfg(void);
u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue);

#ifdef __cplusplus
} 
#endif

#endif
 