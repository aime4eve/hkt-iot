
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "control_center.h"
#include "uart.h"
#include "multi_button.h"
#include "systick.h"

#include <LoRaWAN_APPLY.h>

struct button door_sensor;
u8 reed_switch_long;

void EXTI0_1_IRQHandler(void)
{
	if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
	{
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
	}
	if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET)
		HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
	// HAL_GPIO_EXTI_IRQHandler(LIS2DH12_INT_Pin);
	// HAL_GPIO_EXTI_IRQHandler(REED_SWITCH_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == LIS2DH12_INT_Pin)
	{
		device_t.accIntState = 1;
		device_t.extiWakeEvent = 1;
		// INFO("\r\nLIS2DH12 INT1 EXTI\r\n");
	}
	if (GPIO_Pin == REED_SWITCH_Pin)
	{
		INFO("\r\nREED SWITCH EXTI\r\n");
	}
	if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

void DOOR_SENSOR_SINGLE_CLICK_Handler(void)
{
	// INFO("Decetion Reed Switch Single Handler\n");
	if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

void DOOR_SENSOR_DOUBLE_CLICK_Handler(void)
{
	// INFO("Decetion Reed Switch Double Handler\n");
	if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

void DOOR_SENSOR_LONG_RRESS_START_Handler(void)
{
	reed_switch_long = 1;
	// INFO("Decetion Reed Switch Long Handler\n");
	if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	// __HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPS_PWR_CTRL_Pin | GPS_VBACK_Pin | ENS210_PWR_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin : LIS2DH12_INT_Pin */
	GPIO_InitStruct.Pin = LIS2DH12_INT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LIS2DH12_INT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LIS2DH12_VDDIO_Pin */
	GPIO_InitStruct.Pin = LIS2DH12_VDDIO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : GPS_PWR_CTRL_Pin GPS_RST_Pin ENS210_PWR_Pin */
	GPIO_InitStruct.Pin = GPS_PWR_CTRL_Pin | GPS_VBACK_Pin | ENS210_PWR_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, LoRaWAN_RST_Pin | LoRaWAN_WAKE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, LoRaWAN_MODE_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : LoRaWAN_MODE_Pin */
	GPIO_InitStruct.Pin = LoRaWAN_MODE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pins : LoRaWAN_RST_Pin LoRaWAN_WAKE_Pin */
	GPIO_InitStruct.Pin = LoRaWAN_RST_Pin | LoRaWAN_WAKE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = LoRaWAN_BUSY_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : LoRaWAN_STAT_Pin */
	GPIO_InitStruct.Pin = LoRaWAN_STAT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(LoRaWAN_STAT_GPIO_Port, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = REED_SWITCH_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(REED_SWITCH_GPIO_Port, &GPIO_InitStruct);

	/* EXTI interrupt init*/
	HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

	button_init(&door_sensor, getReedSwitchState, 1);

	button_attach(&door_sensor, SINGLE_CLICK, (BtnCallback)DOOR_SENSOR_SINGLE_CLICK_Handler);
	button_attach(&door_sensor, DOUBLE_CLICK, (BtnCallback)DOOR_SENSOR_DOUBLE_CLICK_Handler);
	button_attach(&door_sensor, LONG_RRESS_START, (BtnCallback)DOOR_SENSOR_LONG_RRESS_START_Handler);
	button_start(&door_sensor);
}

/**
 * @brief  获取门磁触发状态
 * @param
 * @retval 门磁触发状态
 */
u8 getReedSwitchState(void)
{
	return HAL_GPIO_ReadPin(REED_SWITCH_GPIO_Port, REED_SWITCH_Pin);
}
