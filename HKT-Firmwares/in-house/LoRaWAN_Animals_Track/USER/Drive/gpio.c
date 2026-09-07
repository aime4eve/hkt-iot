
/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include "control_center.h"
#include "LoRaWAN_APPLY.h"
#include "multi_button.h"
#include "uart.h"
#include "systick.h"
#include "gps.h"
#include "tim.h"
#include "acc.h"

static struct button button_1;
u8 repeat_cnt;
time_t repeat_stamp;

uint8_t button_read_pin_status(void)
{
	return getButtonPressState();
}

void BUTTON1_SINGLE_CLICK_Handler(void)
{
	device_t.pressShortEvent = 1;
	if (repeat_cnt++ == 0)
		repeat_stamp = Timestamp + 10;

	if (Timestamp > repeat_stamp && repeat_cnt)
	{
		repeat_cnt = 0;
	}
	else if (device_t.powerState)
	{
		if (repeat_cnt >= 7)
		{
			repeat_cnt = 0;
			device_t.waitDafaultMode = 1;
		}
	}
}

void BUTTON1_DOUBLE_CLICK_Handler(void)
{
	if (!device_t.waitDafaultMode)
		device_t.pressDoubleEvent = 1;
	else
	{
		GPIO_GREEN_LED_H;
		GPIO_RED_LED_H;
		device_t.waitDafaultMode = 0;
	}
}

void BUTTON1_LONG_RRESS_START_Handler(void)
{
	if (!device_t.waitDafaultMode)
		device_t.pressLongEvent = 1;
	else
	{
		device_t.factoryEvent = 1;
		device_t.waitDafaultMode = 0;
		GPIO_GREEN_LED_H;
		GPIO_RED_LED_H;
	}
}

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
		if (device_t.sleepState)
		{
			device_t.sleepState = 0;
			device_t.extiWakeEvent = 1;
		}
		if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
			device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
	}
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_1))
	{
		/* Clear the EXTI line 0 pending bit */
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_1);
		if (device_t.sleepState)
		{
			device_t.sleepState = 0;
			device_t.extiWakeEvent = 1;
		}
		if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
			device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
	}
}

void EXTI4_15_IRQHandler(void)
{
	if (LL_EXTI_IsActiveFlag_0_31(LL_EXTI_LINE_7))
	{
		/* Clear the EXTI line 0 pending bit */
		LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_7);
		if (device_t.sleepState)
		{
			device_t.sleepState = 0;
			device_t.extiWakeEvent = 1;
		}
		device_t.lis3d_int = 1;
		if (device_t.sleepDelay < WAIT_SLEEP_TIME_DELAY)
			device_t.sleepDelay = WAIT_SLEEP_TIME_DELAY;
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

	LL_GPIO_DeInit(GPIOA);
	LL_GPIO_DeInit(GPIOB);
	LL_GPIO_DeInit(GPIOC);

	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Pin = GPIO_GPS_PWR_CTRL_PIN;
	LL_GPIO_Init(GPIO_GPS_PWR_CTRL_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_GPS_BPWR_CTRL_PIN;
	LL_GPIO_Init(GPIO_GPS_BPWR_CTRL_PORT, &GPIO_InitStruct);
	GPIO_GPS_PWR_CTRL_L;
	GPIO_GPS_BPWR_CTRL_L;

	GPIO_InitStruct.Pin = GPIO_BEEP_PIN;
	LL_GPIO_Init(GPIO_BEEP_PORT, &GPIO_InitStruct);
	GPIO_BEEP_L;

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

	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_UP;
	GPIO_InitStruct.Pin = GPIO_SENSE_SENSOR_PIN;
	LL_GPIO_Init(GPIO_SENSE_SENSOR_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_TAMPER_PIN;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIO_TAMPER_PORT, &GPIO_InitStruct);

	// ** 初始化LED IO ** //
	GPIO_InitStruct.Pin = GPIO_GREEN_LED_PIN;
	GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	LL_GPIO_Init(GPIO_GREEN_LED_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_RED_LED_PIN;
	LL_GPIO_Init(GPIO_RED_LED_PORT, &GPIO_InitStruct);
	GPIO_GREEN_LED_H;
	GPIO_RED_LED_H;

	GPIO_InitStruct.Pin = GPIO_BLE_RST_PIN;
	LL_GPIO_Init(GPIO_BLE_RST_PORT, &GPIO_InitStruct);
	GPIO_BLE_RST_H;

	// ** 初始化加速计 IO ** //
	GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	GPIO_InitStruct.Pin = GPIO_LIS3D_INT_PIN;
	LL_GPIO_Init(GPIO_LIS3D_INT_PORT, &GPIO_InitStruct);

	// ** 初始化加速计中断 IO ** //
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTB, LL_SYSCFG_EXTI_LINE7);
	LL_GPIO_SetPinPull(GPIO_LIS3D_INT_PORT, GPIO_LIS3D_INT_PIN, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIO_LIS3D_INT_PORT, GPIO_LIS3D_INT_PIN, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_7;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING;
	LL_EXTI_Init(&EXTI_InitStruct);

	// ** 初始化按键中断 IO ** //
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE0);
	LL_GPIO_SetPinPull(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_PULL_UP);
	LL_GPIO_SetPinMode(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_0;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	// ** 初始化防拆中断 IO ** //
	LL_SYSCFG_SetEXTISource(LL_SYSCFG_EXTI_PORTA, LL_SYSCFG_EXTI_LINE1);
	LL_GPIO_SetPinPull(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, LL_GPIO_PULL_NO);
	LL_GPIO_SetPinMode(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN, LL_GPIO_MODE_INPUT);

	EXTI_InitStruct.Line_0_31 = LL_EXTI_LINE_1;
	EXTI_InitStruct.LineCommand = ENABLE;
	EXTI_InitStruct.Mode = LL_EXTI_MODE_IT;
	EXTI_InitStruct.Trigger = LL_EXTI_TRIGGER_RISING_FALLING;
	LL_EXTI_Init(&EXTI_InitStruct);

	/* EXTI interrupt init*/
	NVIC_SetPriority(EXTI0_1_IRQn, 1);
	NVIC_EnableIRQ(EXTI0_1_IRQn);

	NVIC_SetPriority(EXTI4_15_IRQn, 1);
	NVIC_EnableIRQ(EXTI4_15_IRQn);

	button_init(&button_1, button_read_pin_status, 0);

	button_attach(&button_1, SINGLE_CLICK, (BtnCallback)BUTTON1_SINGLE_CLICK_Handler);
	button_attach(&button_1, DOUBLE_CLICK, (BtnCallback)BUTTON1_DOUBLE_CLICK_Handler);
	button_attach(&button_1, LONG_RRESS_START, (BtnCallback)BUTTON1_LONG_RRESS_START_Handler);
	button_start(&button_1);
}

/**
 * @brief  获取防拆报警状态
 * @param
 * @retval 防拆报警状态
 */
int getTamperState(void)
{
	if (LL_GPIO_ReadInputPin(GPIO_TAMPER_PORT, GPIO_TAMPER_PIN) != GPIO_PIN_STATUS_RESET)
		return GPIO_PIN_STATUS_SET;
	else
		return GPIO_PIN_STATUS_RESET;
}

/**
 * @brief  获取按键状态
 * @param
 * @retval 按键状态
 */
int getButtonPressState(void)
{
	if (LL_GPIO_ReadInputPin(GPIO_SENSE_SENSOR_PORT, GPIO_SENSE_SENSOR_PIN) != GPIO_PIN_STATUS_RESET)
		return GPIO_PIN_STATUS_SET;
	else
		return GPIO_PIN_STATUS_RESET;
}

// 读取输入IO状态
u32 LL_GPIO_ReadInputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
	return (READ_BIT(GPIOx->IDR, PinMask));
}

// 读取输出IO状态
u32 LL_GPIO_ReadOutputPin(GPIO_TypeDef *GPIOx, uint32_t PinMask)
{
	return (READ_BIT(GPIOx->ODR, PinMask));
}

// 设置IO状态
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

void Beep(int time)
{
	while (time--)
	{
		GPIO_BEEP_H;
		Delay_Us(400);
		GPIO_BEEP_L;
		Delay_Us(400);
	}
}

void GpioEventProcess(void)
{
	
#if FUNC_TAMPER_ENABLE
	if (!getTamperState() && !device_t.sensor_t.tamper_status)
	{
		mDelay(20);
		if (!getTamperState()) // 防抖
		{
			device_t.updateTamper = 1;
			device_t.sensor_t.tamper_status = 1; // 防拆报警
		}
	}
	else if (getTamperState() && device_t.sensor_t.tamper_status)
	{
		mDelay(20);
		if (getTamperState()) // 防抖
		{
			device_t.updateTamper = 1;
			device_t.sensor_t.tamper_status = 0;
		}
	}
#endif

	if (!device_t.waitDafaultMode)
	{
		if (device_t.pressShortEvent)
		{
			device_t.pressShortEvent = 0;
			INFO("\nButton Press Short Handler\n");
			if (device_t.powerState) // 开机状态下短按亮绿灯
			{
				ledGreenBlinkTime = SEC_DELAY;
			}
			else // 关机状态下短按亮红灯
			{
				ledRedBlinkTime = SEC_DELAY;
			}
		}

		// 双击主动同步数据
		if (device_t.pressDoubleEvent)
		{
			device_t.pressDoubleEvent = 0;
			INFO("\nButton Press Double Handler\n");
			GPIO_GREEN_LED_L;
			mDelay(500);
			GPIO_GREEN_LED_H;
			GPIO_RED_LED_L;
			mDelay(500);
			GPIO_RED_LED_H;
			if (LoRaWAN.joinState)
				device_t.syncState = 1;
		}

		if (device_t.pressLongEvent)
		{
			device_t.pressLongEvent = 0;
			INFO("\nButton Press Long Handler\n");
			if (device_t.powerState)
			{
				device_t.powerState = 0;
				BEEP_RUN(3000, 50); // 关机长鸣
			}
			else
			{
				device_t.powerState = 1;
				BEEP_RUN(1000, 50); // 开机短鸣
				LoRaWAN.joinEvent = LoRaWAN_JOINEVENT_INIT;
				gps_stamp = Timestamp; // 开机后重新开始定位
			}
#if TRACK_ACC_ENABLE
			if (!device_t.powerState) // 切换待机状态
				lis3dh_set_mode(acc_sensor, lis3dh_power_down, lis3dh_low_power, true, true, true);
			else
				lis3dh_set_mode(acc_sensor, lis3dh_odr_50, lis3dh_low_power, true, true, true);
#endif
		}
	}

	if (device_t.factoryEvent)
	{
		device_t.factoryEvent = 0;
		// INFO("\nButton Repeat 7 Tick, Device Default\n");
		INFO("\nDevice Default\n");
		FLASH_Reset_Param();
		ledGreenBlinkTime = 3 * SEC_DELAY;
		ledRedBlinkTime = 3 * SEC_DELAY;
		BEEP_RUN(200, 50);
		mDelay(500);
		BEEP_RUN(200, 50);
	}
}
