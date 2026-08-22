
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

// ** 雷达模块 ** //
#define PWR_CTRL_PIN LL_GPIO_PIN_15
#define PWR_CTRL_PORT GPIOB
#define PWR_CTRL_L LL_GPIO_ResetOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)
#define PWR_CTRL_H LL_GPIO_SetOutputPin(PWR_CTRL_PORT, PWR_CTRL_PIN)

// ** 磁力计 ** //
#define QMC_INT_PIN LL_GPIO_PIN_13

#define QMC_INT_PORT GPIOC
#define QMC_INT_STA ReadInputPin(QMC_INT_PORT, QMC_INT_PIN)

// ** 磁敏开关 ** //
#define VCS_INT_PIN LL_GPIO_PIN_0
#define VCS_INT_PORT GPIOA
#define VCS_INT_STA ReadInputPin(VCS_INT_PORT, VCS_INT_PIN)

// ** 蓝牙连接状态 ** //
#define BLE_CONNECT_PIN LL_GPIO_PIN_2
#define BLE_CONNECT_PORT GPIOB
#define BLE_CONNECT_STA ReadInputPin(BLE_CONNECT_PORT, BLE_CONNECT_PIN)

// ** 光敏电阻 ** //
#define LIGHT_PWR_PIN LL_GPIO_PIN_5
#define LIGHT_PWR_PORT GPIOA
#define LIGHT_PWR_L LL_GPIO_ResetOutputPin(LIGHT_PWR_PORT, LIGHT_PWR_PIN)
#define LIGHT_PWR_H LL_GPIO_SetOutputPin(LIGHT_PWR_PORT, LIGHT_PWR_PIN)

#define LIGHT_OUT_PIN LL_GPIO_PIN_6
#define LIGHT_OUT_PORT GPIOA
#define LIGHT_OUT_STA ReadInputPin(LIGHT_OUT_PORT, LIGHT_OUT_PIN)
#define LIGHT_OUT_L LL_GPIO_ResetOutputPin(LIGHT_OUT_PORT, LIGHT_OUT_PIN)
#define LIGHT_OUT_H LL_GPIO_SetOutputPin(LIGHT_OUT_PORT, LIGHT_OUT_PIN)

// ** LED ** //
#define LED_RED_PIN LL_GPIO_PIN_0
#define LED_RED_PORT GPIOB
#define LED_RED_L LL_GPIO_ResetOutputPin(LED_RED_PORT, LED_RED_PIN)
#define LED_RED_H LL_GPIO_SetOutputPin(LED_RED_PORT, LED_RED_PIN)

#define LED_BLUE_PIN LL_GPIO_PIN_1
#define LED_BLUE_PORT GPIOB
#define LED_BLUE_L LL_GPIO_ResetOutputPin(LED_BLUE_PORT, LED_BLUE_PIN)
#define LED_BLUE_H LL_GPIO_SetOutputPin(LED_BLUE_PORT, LED_BLUE_PIN)

// ** FLASH ** //
#define FLASH_WP_PIN LL_GPIO_PIN_4
#define FLASH_WP_PORT GPIOB
#define FLASH_WP_L LL_GPIO_ResetOutputPin(FLASH_WP_PORT, FLASH_WP_PIN)
#define FLASH_WP_H LL_GPIO_SetOutputPin(FLASH_WP_PORT, FLASH_WP_PIN)

#define FLASH_HOLD_PIN LL_GPIO_PIN_3
#define FLASH_HOLD_PORT GPIOB
#define FLASH_HOLD_L LL_GPIO_ResetOutputPin(FLASH_HOLD_PORT, FLASH_HOLD_PIN)
#define FLASH_HOLD_H LL_GPIO_SetOutputPin(FLASH_HOLD_PORT, FLASH_HOLD_PIN)

// ** LoRaWAN ** //
#define LoRaWAN_WAKE_PIN LL_GPIO_PIN_8
#define LoRaWAN_WAKE_PORT GPIOA
#define LoRaWAN_WAKE_STA ReadOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN)
#define LoRaWAN_WAKE_L LL_GPIO_ResetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN)
#define LoRaWAN_WAKE_H LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN)

#define LoRaWAN_STAT_PIN LL_GPIO_PIN_9
#define LoRaWAN_STAT_PORT GPIOC
#define LoRaWAN_STAT_STA ReadInputPin(LoRaWAN_STAT_PORT, LoRaWAN_STAT_PIN)

#define LoRaWAN_BUSY_PIN LL_GPIO_PIN_8
#define LoRaWAN_BUSY_PORT GPIOC
#define LoRaWAN_BUSY_STA ReadInputPin(LoRaWAN_BUSY_PORT, LoRaWAN_BUSY_PIN)

#define LoRaWAN_MODE_PIN LL_GPIO_PIN_7
#define LoRaWAN_MODE_PORT GPIOC
#define LoRaWAN_MODE_STA ReadOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN)
#define LoRaWAN_MODE_L LL_GPIO_ResetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN)
#define LoRaWAN_MODE_H LL_GPIO_SetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN)

#define LoRaWAN_NRST_PIN LL_GPIO_PIN_6
#define LoRaWAN_NRST_PORT GPIOC
#define LoRaWAN_NRST_L LL_GPIO_ResetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN)
#define LoRaWAN_NRST_H LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN)

void App_PortCfg(void);
void port_sleep_init(void);


#ifdef __cplusplus
}
#endif

#endif
