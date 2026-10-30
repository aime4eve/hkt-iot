
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "control_center.h"
#include "LoRaWAN_APPLY.h"

/**
 * @brief  This function handles External line 0 to 1 interrupt request.
 * @param  None
 * @retval None
 */
void EXTI0_1_IRQHandler(void)
{
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_0))
	{
		/* Clear the EXTI line 0 pending bit */
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
		// 触发干簧管
		device_t.doorEvent = 1;
		if (device_t.sleepState)
		{
			device_t.sleepState = 0;
			device_t.extiWakeEvent = 1;
		}
		// if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		// 	device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
		ledBlueBlinkTime = SEC_DELAY / 2;
	}
}

void EXTI4_15_IRQHandler(void)
{
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_6))
	{
		/* Clear the EXTI line 0 pending bit */
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_6);
		device_t.tamperEvent = 1;
		if (device_t.sleepState)
		{
			device_t.sleepState = 0;
			device_t.extiWakeEvent = 1;
		}
		// if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
		// 	device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
		ledBlueBlinkTime = SEC_DELAY / 2;
	}
}

/**
 * @brief  设备GPIO初始化
 * @param
 * @retval
 */
void App_PortCfg(void)
{
	LL_EXTI_InitTypeDef EXTI_InitStruct = {0};
	LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* GPIO Ports Clock Enable */
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);

	// LL_GPIO_DeInit(GPIOA);
	// LL_GPIO_DeInit(GPIOB);
	// LL_GPIO_DeInit(GPIOC);

	/** 初始化LED IO **/
	LL_GPIO_SetOutputPin(GPIO_LED_BLUE_PORT, GPIO_LED_BLUE_PIN);

	GPIO_InitStruct.Pin = GPIO_LED_BLUE_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIO_LED_BLUE_PORT, &GPIO_InitStruct);

	/** 初始化LoRaWAN 模块IO **/
	LL_GPIO_SetOutputPin(LoRaWAN_MODE_PORT, LoRaWAN_MODE_PIN);
	LL_GPIO_SetOutputPin(LoRaWAN_NRST_PORT, LoRaWAN_NRST_PIN);
	LL_GPIO_SetOutputPin(LoRaWAN_WAKE_PORT, LoRaWAN_WAKE_PIN);

	GPIO_InitStruct.Pin = LoRaWAN_MODE_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(LoRaWAN_MODE_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LoRaWAN_NRST_PIN;
	LL_GPIO_Init(LoRaWAN_NRST_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LoRaWAN_WAKE_PIN;
	LL_GPIO_Init(LoRaWAN_WAKE_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LoRaWAN_BUSY_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(LoRaWAN_BUSY_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = LoRaWAN_STAT_PIN;
	LL_GPIO_Init(LoRaWAN_STAT_PORT, &GPIO_InitStruct);

	// ** 初始化干簧管中断 IO ** //
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
	LL_GPIO_SetPinPull(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	// ** 初始化防拆中断 IO ** //
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE6);
	LL_GPIO_SetPinPull(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_6;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	/* EXTI interrupt init*/
	NVIC_SetPriority(EXTI4_15_IRQn, 0);
	NVIC_EnableIRQ(EXTI4_15_IRQn);
	/* EXTI interrupt init*/
	NVIC_SetPriority(EXTI0_1_IRQn, 0);
	NVIC_EnableIRQ(EXTI0_1_IRQn);
}

/**
 * @brief  获取防拆报警状态
 * @param
 * @retval 防拆报警状态
 */
int getTamperState(void)
{
	if(LL_GPIO_ReadInputPin(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN) != GPIO_PIN_STATUS_RESET)
		return GPIO_PIN_STATUS_SET;
	else
		return GPIO_PIN_STATUS_RESET;
}

/**
 * @brief  获取门磁触发状态
 * @param
 * @retval 门磁触发状态
 */
int getDoorSensorState(void)
{
	if(LL_GPIO_ReadInputPin(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN) != GPIO_PIN_STATUS_RESET)
		return GPIO_PIN_STATUS_SET;
	else
		return GPIO_PIN_STATUS_RESET;
}

//读取输入IO状态
u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
	return (READ_BIT(GPIOx->IDR, PinMask));
}

//读取输出IO状态
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
	return (READ_BIT(GPIOx->ODR, PinMask));
}

//设置IO状态
void LL_GPIO_WriteOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask, uint32_t PinValue)
{
	if (PinValue != 0)
	{
		WRITE_REG(GPIOx->BSRR, PinMask);
	}
	else
	{
		WRITE_REG(GPIOx->BRR, PinMask);
	}
}
