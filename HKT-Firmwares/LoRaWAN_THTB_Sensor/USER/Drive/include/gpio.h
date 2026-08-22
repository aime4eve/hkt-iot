
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

/* Private defines -----------------------------------------------------------*/
#define LIGHT_SENSOR_Pin GPIO_PIN_0
#define LIGHT_SENSOR_GPIO_Port GPIOB
#define LIS2DH12_INT_Pin GPIO_PIN_1
#define LIS2DH12_INT_GPIO_Port GPIOB
#define LIS2DH12_INT_EXTI_IRQn EXTI0_1_IRQn
#define LIS2DH12_VDDIO_Pin GPIO_PIN_2
#define LIS2DH12_VDDIO_GPIO_Port GPIOB
#define GPS_PWR_CTRL_Pin GPIO_PIN_13
#define GPS_PWR_CTRL_GPIO_Port GPIOB
#define LoRaWAN_MODE_Pin GPIO_PIN_14
#define LoRaWAN_MODE_GPIO_Port GPIOB
#define LoRaWAN_BUSY_Pin GPIO_PIN_15
#define LoRaWAN_BUSY_GPIO_Port GPIOB
#define LoRaWAN_RST_Pin GPIO_PIN_8
#define LoRaWAN_RST_GPIO_Port GPIOA
#define LoRaWAN_STAT_Pin GPIO_PIN_11
#define LoRaWAN_STAT_GPIO_Port GPIOA

#if FUNC_LoRaWAN_FIRM_SX
#define LoRaWAN_WAKE_Pin GPIO_PIN_6
#define LoRaWAN_WAKE_GPIO_Port GPIOB
#else
#define LoRaWAN_WAKE_Pin GPIO_PIN_12
#define LoRaWAN_WAKE_GPIO_Port GPIOA
#endif

#define BEEP_CTRL_Pin GPIO_PIN_15
#define BEEP_CTRL_GPIO_Port GPIOA
#define GPS_VBACK_Pin GPIO_PIN_12
#define GPS_VBACK_GPIO_Port GPIOB
#define ENS210_PWR_Pin GPIO_PIN_5
#define ENS210_PWR_GPIO_Port GPIOB
#define REED_SWITCH_GPIO_Port GPIOA
#define REED_SWITCH_Pin GPIO_PIN_0

#define GPS_PWR_CTRL_H HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_SET)
#define GPS_PWR_CTRL_L HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, GPS_PWR_CTRL_Pin, GPIO_PIN_RESET)

#define BEEP_CTRL_H HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, BEEP_CTRL_Pin, GPIO_PIN_SET)
#define BEEP_CTRL_L HAL_GPIO_WritePin(GPS_PWR_CTRL_GPIO_Port, BEEP_CTRL_Pin, GPIO_PIN_RESET)

    extern struct button door_sensor;
    extern u8 reed_switch_long;

    void MX_GPIO_Init(void);
    u8 getReedSwitchState(void);

#ifdef __cplusplus
}
#endif

#endif
