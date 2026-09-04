
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define GPIO_INFRARED_A_PIN    LL_GPIO_PIN_6
#define GPIO_INFRARED_A_PORT   GPIOA

#define GPIO_INFRARED_B_PIN    LL_GPIO_PIN_7
#define GPIO_INFRARED_B_PORT   GPIOA

#define GPIO_SENSE_SENSOR_PIN    LL_GPIO_PIN_0
#define GPIO_SENSE_SENSOR_PORT   GPIOA

// ** LED IO ** //
#define GPIO_LED_GREEN_PIN    LL_GPIO_PIN_6
#define GPIO_LED_GREEN_PORT   GPIOB

#define GPIO_LED_BLUE_PIN    LL_GPIO_PIN_5
#define GPIO_LED_BLUE_PORT   GPIOB

#define GPIO_LED_ORANGE_PIN    LL_GPIO_PIN_4
#define GPIO_LED_ORANGE_PORT   GPIOB

#define GPIO_LED_RED_PIN    LL_GPIO_PIN_3
#define GPIO_LED_RED_PORT   GPIOB

#define LoRaWAN_MODE_PIN LL_GPIO_PIN_14
#define LoRaWAN_MODE_PORT GPIOB

#define LoRaWAN_BUSY_PIN LL_GPIO_PIN_15
#define LoRaWAN_BUSY_PORT GPIOB

#define LoRaWAN_NRST_PIN LL_GPIO_PIN_8
#define LoRaWAN_NRST_PORT GPIOA

#define LoRaWAN_STAT_PIN LL_GPIO_PIN_11
#define LoRaWAN_STAT_PORT GPIOA

#define LoRaWAN_WAKE_PIN LL_GPIO_PIN_12
#define LoRaWAN_WAKE_PORT GPIOA

void App_PortCfg(void);
int getDoorSensorState(void);
int getPeopleCounterAState(void);
int getPeopleCounterBState(void);

u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue);

#ifdef __cplusplus
}
#endif

#endif
