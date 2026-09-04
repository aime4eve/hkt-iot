
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define GPIO_PIN_STATUS_RESET 0
#define GPIO_PIN_STATUS_SET 1

#define GPIO_AD_PWR_PIN GPIO_PIN_6
#define GPIO_AD_PWR_PORT HT_GPIOB
#define GPIO_AD_PWR_H GPIO_SetOutBits(GPIO_AD_PWR_PORT, GPIO_AD_PWR_PIN)
#define GPIO_AD_PWR_L GPIO_ClearOutBits(GPIO_AD_PWR_PORT, GPIO_AD_PWR_PIN)

#define GPIO_DAT1_PIN GPIO_PIN_7
#define GPIO_DAT1_PORT HT_GPIOA
#define READ_DAT1_STATE GPIO_ReadInBit(GPIO_DAT1_PORT, GPIO_DAT1_PIN)

#define GPIO_DAT2_PIN GPIO_PIN_6
#define GPIO_DAT2_PORT HT_GPIOA
#define READ_DAT2_STATE GPIO_ReadInBit(GPIO_DAT2_PORT, GPIO_DAT2_PIN)

#define GPIO_AUE_PIN GPIO_PIN_1
#define GPIO_AUE_PORT HT_GPIOA
#define READ_AUE_STATE GPIO_ReadInBit(GPIO_AUE_PORT, GPIO_AUE_PIN)


#define GPIO_TAMPER_PIN GPIO_PIN_0
#define GPIO_TAMPER_PORT HT_GPIOC
#define READ_TAMPER_STATE GPIO_ReadInBit(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN)

#define LoRaWAN_MODE_PIN GPIO_PIN_4
#define LoRaWAN_MODE_PORT HT_GPIOB

#define LoRaWAN_BUSY_PIN GPIO_PIN_3
#define LoRaWAN_BUSY_PORT HT_GPIOB

#define LoRaWAN_STAT_PIN GPIO_PIN_2
#define LoRaWAN_STAT_PORT HT_GPIOB

#define LoRaWAN_NRST_PIN GPIO_PIN_14
#define LoRaWAN_NRST_PORT HT_GPIOA

#define LoRaWAN_WAKE_PIN GPIO_PIN_15
#define LoRaWAN_WAKE_PORT HT_GPIOA

void App_PortCfg(void);
void Gpio_Event(void);

#ifdef __cplusplus
}
#endif

#endif
