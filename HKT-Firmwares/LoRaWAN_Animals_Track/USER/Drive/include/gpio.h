
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define GPIO_TAMPER_PIN LL_GPIO_PIN_1
#define GPIO_TAMPER_PORT GPIOA

#define GPIO_SENSE_SENSOR_PIN LL_GPIO_PIN_0
#define GPIO_SENSE_SENSOR_PORT GPIOA

#define GPIO_LIS3D_INT_PIN LL_GPIO_PIN_7
#define GPIO_LIS3D_INT_PORT GPIOB

#define GPIO_BEEP_PIN LL_GPIO_PIN_15
#define GPIO_BEEP_PORT GPIOA
#define GPIO_BEEP_L LL_GPIO_ResetOutputPin(GPIO_BEEP_PORT, GPIO_BEEP_PIN)
#define GPIO_BEEP_H LL_GPIO_SetOutputPin(GPIO_BEEP_PORT, GPIO_BEEP_PIN)

#define GPIO_GPS_PWR_CTRL_PIN LL_GPIO_PIN_13
#define GPIO_GPS_PWR_CTRL_PORT GPIOB
#define GPIO_GPS_PWR_CTRL_L LL_GPIO_ResetOutputPin(GPIO_GPS_PWR_CTRL_PORT, GPIO_GPS_PWR_CTRL_PIN)
#define GPIO_GPS_PWR_CTRL_H LL_GPIO_SetOutputPin(GPIO_GPS_PWR_CTRL_PORT, GPIO_GPS_PWR_CTRL_PIN)

#define GPIO_GPS_BPWR_CTRL_PIN LL_GPIO_PIN_12
#define GPIO_GPS_BPWR_CTRL_PORT GPIOB
#define GPIO_GPS_BPWR_CTRL_L LL_GPIO_ResetOutputPin(GPIO_GPS_BPWR_CTRL_PORT, GPIO_GPS_BPWR_CTRL_PIN)
#define GPIO_GPS_BPWR_CTRL_H LL_GPIO_SetOutputPin(GPIO_GPS_BPWR_CTRL_PORT, GPIO_GPS_BPWR_CTRL_PIN)

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
    int getTamperState(void);
    int getButtonPressState(void);
    void GpioEventProcess(void);

    u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
    u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask);
    void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue);

#define GPIO_GREEN_LED_PIN LL_GPIO_PIN_3
#define GPIO_GREEN_LED_PORT GPIOB
#define GPIO_GREEN_LED_L LL_GPIO_WriteOutputPin(GPIO_GREEN_LED_PORT, GPIO_GREEN_LED_PIN, 0)
#define GPIO_GREEN_LED_H LL_GPIO_WriteOutputPin(GPIO_GREEN_LED_PORT, GPIO_GREEN_LED_PIN, 1)

#define GPIO_RED_LED_PIN LL_GPIO_PIN_4
#define GPIO_RED_LED_PORT GPIOB
#define GPIO_RED_LED_L LL_GPIO_WriteOutputPin(GPIO_RED_LED_PORT, GPIO_RED_LED_PIN, 0)
#define GPIO_RED_LED_H LL_GPIO_WriteOutputPin(GPIO_RED_LED_PORT, GPIO_RED_LED_PIN, 1)

#define GPIO_BLE_RST_PIN LL_GPIO_PIN_5
#define GPIO_BLE_RST_PORT GPIOB
#define GPIO_BLE_RST_L LL_GPIO_WriteOutputPin(GPIO_BLE_RST_PORT, GPIO_BLE_RST_PIN, 0)
#define GPIO_BLE_RST_H LL_GPIO_WriteOutputPin(GPIO_BLE_RST_PORT, GPIO_BLE_RST_PIN, 1)

#ifdef __cplusplus
}
#endif

#endif
