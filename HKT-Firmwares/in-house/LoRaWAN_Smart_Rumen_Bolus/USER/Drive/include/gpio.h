
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define GPIO_DA_INT_PIN LL_GPIO_PIN_7
#define GPIO_DA_INT_PORT GPIOB

#define GPIO_LED_PIN LL_GPIO_PIN_15
#define GPIO_LED_PORT GPIOA
#define GPIO_LED_L LL_GPIO_ResetOutputPin(GPIO_LED_PORT, GPIO_LED_PIN)
#define GPIO_LED_H LL_GPIO_SetOutputPin(GPIO_LED_PORT, GPIO_LED_PIN)

#define LoRaWAN_MODE_PIN LL_GPIO_PIN_14
#define LoRaWAN_MODE_PORT GPIOB

#define LoRaWAN_BUSY_PIN LL_GPIO_PIN_15
#define LoRaWAN_BUSY_PORT GPIOB

#define LoRaWAN_STAT_PIN LL_GPIO_PIN_11
#define LoRaWAN_STAT_PORT GPIOA

#define LoRaWAN_NRST_PIN LL_GPIO_PIN_8
#define LoRaWAN_NRST_PORT GPIOA

#define LoRaWAN_WAKE_PIN LL_GPIO_PIN_12
#define LoRaWAN_WAKE_PORT GPIOA

#define LoRaWAN_IRQ_PIN LL_GPIO_PIN_15
#define LoRaWAN_IRQ_PORT GPIOB

void App_PortCfg(void);
int getTamperState(void);
int getButtonPressState(void);
void GpioEventProcess(void);

u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue);

#ifdef __cplusplus
}
#endif

#endif
