
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define ReadInputPin(x, PinMask) (READ_BIT(x->IDR, PinMask) ? GPIO_PIN_STATUS_SET : GPIO_PIN_STATUS_RESET)
#define ReadOutputPin(x, PinMask) (READ_BIT(x->ODR, PinMask) ? GPIO_PIN_STATUS_SET : GPIO_PIN_STATUS_RESET)

// ** 蓝牙连接状态 ** //
#define BLE_CONNECT_PIN LL_GPIO_PIN_2
#define BLE_CONNECT_PORT GPIOB
#define BLE_CONNECT_STA ReadInputPin(BLE_CONNECT_PORT, BLE_CONNECT_PIN)

// ** LED ** //
#define LED_RED_PIN LL_GPIO_PIN_0
#define LED_RED_PORT GPIOB
#define LED_RED_L LL_GPIO_ResetOutputPin(LED_RED_PORT, LED_RED_PIN)
#define LED_RED_H LL_GPIO_SetOutputPin(LED_RED_PORT, LED_RED_PIN)

#define LED_BLUE_PIN LL_GPIO_PIN_1
#define LED_BLUE_PORT GPIOB
#define LED_BLUE_L LL_GPIO_ResetOutputPin(LED_BLUE_PORT, LED_BLUE_PIN)
#define LED_BLUE_H LL_GPIO_SetOutputPin(LED_BLUE_PORT, LED_BLUE_PIN)

void App_PortCfg(void);

#ifdef __cplusplus
}
#endif

#endif
