
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

// ** KEY IO ** //
#define FUNC_KEY_PORT GPIOG
#define FUNC_KEY_PIN GPIO_Pin_7
#define READ_FUNC_KEY_STATUS GPIO_ReadInputDataBit(FUNC_KEY_PORT, FUNC_KEY_PIN)

#define RST_KEY_PORT GPIOB
#define RST_KEY_PIN GPIO_Pin_0
#define READ_RST_KEY_STATUS GPIO_ReadInputDataBit(RST_KEY_PORT, RST_KEY_PIN)

// ** BEEP CTRL ** //
#define BEEP_CTRL_PORT GPIOB
#define BEEP_CTRL_PIN GPIO_Pin_6
#define BEEP_CTRL_H GPIO_SetBits(BEEP_CTRL_PORT, BEEP_CTRL_PIN)
#define BEEP_CTRL_L GPIO_ResetBits(BEEP_CTRL_PORT, BEEP_CTRL_PIN)

// ** DC INPUT ** //
#define DC_PWR_IN_PORT GPIOC
#define DC_PWR_IN_PIN GPIO_Pin_12
#define READ_DC_PWR_IN_STATUS GPIO_ReadInputDataBit(DC_PWR_IN_PORT, DC_PWR_IN_PIN)

// ** CO2 PWR CTRL ** //
// #define CO2_PWR_CTRL_PORT GPIOC
// #define CO2_PWR_CTRL_PIN GPIO_Pin_13
#define CO2_PWR_CTRL_PORT GPIOA
#define CO2_PWR_CTRL_PIN GPIO_Pin_10
#define CO2_PWR_CTRL_H GPIO_SetBits(CO2_PWR_CTRL_PORT, CO2_PWR_CTRL_PIN)
#define CO2_PWR_CTRL_L GPIO_ResetBits(CO2_PWR_CTRL_PORT, CO2_PWR_CTRL_PIN)
#define CO2_PWR_CTRL_STA GPIO_ReadOutputDataBit(CO2_PWR_CTRL_PORT, CO2_PWR_CTRL_PIN)


// ** EPD PWR CTRL ** //
#define EPD_PWR_CTRL_PIN GPIO_Pin_14
#define EPD_PWR_CTRL_PORT GPIOC
#define EPD_PWR_CTRL_H GPIO_SetBits(EPD_PWR_CTRL_PORT, EPD_PWR_CTRL_PIN)
#define EPD_PWR_CTRL_L GPIO_ResetBits(EPD_PWR_CTRL_PORT, EPD_PWR_CTRL_PIN)
#define READ_EPD_PWR_CTRL_STATUS GPIO_ReadOutputDataBit(EPD_PWR_CTRL_PORT, EPD_PWR_CTRL_PIN)

// ** TVOC INT ** //
#define TVOC_INT_PORT GPIOD
#define TVOC_INT_PIN GPIO_Pin_0
#define READ_TVOC_INT_STATUS GPIO_ReadInputDataBit(TVOC_INT_PORT, TVOC_INT_PIN)

#define TVOC_RST_PORT GPIOD
#define TVOC_RST_PIN GPIO_Pin_2
#define TVOC_RST_H GPIO_SetBits(TVOC_RST_PORT, TVOC_RST_PIN)
#define TVOC_RST_L GPIO_ResetBits(TVOC_RST_PORT, TVOC_RST_PIN)


// ** O3 INT ** //
#define O3_INT_PORT GPIOD
#define O3_INT_PIN GPIO_Pin_1
#define READ_O3_INT_STATUS GPIO_ReadInputDataBit(O3_INT_PORT, O3_INT_PIN)

#define BAT_CTRL_PORT GPIOD
#define BAT_CTRL_PIN GPIO_Pin_3
#define BAT_CTRL_H GPIO_SetBits(BAT_CTRL_PORT, BAT_CTRL_PIN)
#define BAT_CTRL_L GPIO_ResetBits(BAT_CTRL_PORT, BAT_CTRL_PIN)

#define LoRaWAN_MODE_PIN GPIO_Pin_4
#define LoRaWAN_MODE_PORT GPIOC

#define LoRaWAN_BUSY_PIN GPIO_Pin_5
#define LoRaWAN_BUSY_PORT GPIOC

#define LoRaWAN_NRST_PIN GPIO_Pin_6
#define LoRaWAN_NRST_PORT GPIOC

#define LoRaWAN_STAT_PIN GPIO_Pin_3
#define LoRaWAN_STAT_PORT GPIOC

#define LoRaWAN_WAKE_PIN GPIO_Pin_2
#define LoRaWAN_WAKE_PORT GPIOC

#ifdef __cplusplus
extern "C"
{
#endif

  extern u16 ledGreenBlinkTime;
  extern u16 ledOrangeBlinkTime;
  extern u16 ledRedBlinkTime;

  void App_PortCfg(void);
  void GPIO_Sleep_Config(void);
  void Test_Fout(void);
  void Led_Blink_Process(void);
  void readDCPowerInputStatus(void);
  void Beep(int time);

  void thread_led(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif
