
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

// ** LED IO ** //
#define GPIO_LED1_PIN GPIO_PIN_8
#define GPIO_LED1_PORT GPIOB

#define GPIO_LED2_PIN GPIO_PIN_9
#define GPIO_LED2_PORT GPIOB

#if FUNC_LORA_VERSION_LIR
#define LoRaWAN_MODE_PIN GPIO_PIN_14
#define LoRaWAN_MODE_PORT GPIOB
#else
#define LoRaWAN_IRQ_PIN GPIO_PIN_12
#define LoRaWAN_IRQ_PORT GPIOB
#endif

#define LoRaWAN_BUSY_PIN GPIO_PIN_15
#define LoRaWAN_BUSY_PORT GPIOB

#define LoRaWAN_NRST_PIN GPIO_PIN_8
#define LoRaWAN_NRST_PORT GPIOA

#define LoRaWAN_STAT_PIN GPIO_PIN_11
#define LoRaWAN_STAT_PORT GPIOA

#define LoRaWAN_WAKE_PIN GPIO_PIN_12
#define LoRaWAN_WAKE_PORT GPIOA


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

void App_PortCfg(void);

#ifdef __cplusplus
}
#endif

#endif
