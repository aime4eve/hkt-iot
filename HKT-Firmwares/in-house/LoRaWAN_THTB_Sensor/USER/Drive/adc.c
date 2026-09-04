
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "uart.h"
#include "systick.h"
#include "control_center.h"
#include "gpio.h"

u8 batteryLevel;
bool batteryStatus;

u16 batteryVoltageConvertInterval; //电量检测转换间隔
u32 convertedValue;

u32 batteryInitialCapacity; //电池上报次数

/**
 * @brief ADC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (hadc->Instance == ADC1)
	{
		/* USER CODE BEGIN ADC1_MspInit 0 */

		/* USER CODE END ADC1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_ADC1_CLK_ENABLE();

		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**ADC GPIO Configuration
		PB0     ------> ADC_IN8
		*/
		GPIO_InitStruct.Pin = LIGHT_SENSOR_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(LIGHT_SENSOR_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN ADC1_MspInit 1 */

		/* USER CODE END ADC1_MspInit 1 */
	}
}

/**
 * @brief ADC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc)
{
	if (hadc->Instance == ADC1)
	{
		/* USER CODE BEGIN ADC1_MspDeInit 0 */

		/* USER CODE END ADC1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_ADC1_CLK_DISABLE();

		/**ADC GPIO Configuration
		PB0     ------> ADC_IN8
		*/
		HAL_GPIO_DeInit(LIGHT_SENSOR_GPIO_Port, LIGHT_SENSOR_Pin);

		/* USER CODE BEGIN ADC1_MspDeInit 1 */

		/* USER CODE END ADC1_MspDeInit 1 */
	}
}

/**
 * @brief ADC Initialization Function
 * @param None
 * @retval None
 */
void MX_ADC_Init(void)
{
	__HAL_RCC_ADC1_CLK_ENABLE();
	ADC_ChannelConfTypeDef sConfig = {0};
	/** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc.Instance = ADC1;
	hadc.Init.OversamplingMode = DISABLE;
	hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.SamplingTime = ADC_SAMPLETIME_79CYCLES_5;
	hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.DMAContinuousRequests = DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
	hadc.Init.LowPowerAutoWait = DISABLE;
	hadc.Init.LowPowerFrequencyMode = DISABLE;
	hadc.Init.LowPowerAutoPowerOff = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK)
	{
		Error_Handler();
	}
	/** Configure for the selected ADC regular channel to be converted.
	 */
	sConfig.Channel = ADC_CHANNEL_8;
	sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

void Stop_ADC(void)
{
	HAL_ADC_Stop(&hadc);
}

void StartAdCollect(void)
{
	u32 convert_value = 0;
	HAL_GPIO_WritePin(ENS210_PWR_GPIO_Port, ENS210_PWR_Pin, GPIO_PIN_SET);
	HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);

	for (int i = 0; i < 30; i++)
	{
		HAL_ADC_Start(&hadc);				  //启动ADC转换
		HAL_ADC_PollForConversion(&hadc, 50); //等待转换完成，50为最大等待时间，单位为ms
		if (HAL_IS_BIT_SET(HAL_ADC_GetState(&hadc), HAL_ADC_STATE_REG_EOC))
		{
			u32 ADC_Value = HAL_ADC_GetValue(&hadc); //获取AD值
			convert_value += ADC_Value;
		}
	}

	DEBUG_TRACE(LOG_TAG, "Covert Tamper ADC Value Result: %d, %d", convert_value, convert_value / 30);
	Stop_ADC();
	// HAL_Delay(100);
	if (convert_value / 30 < 2500)
	{
		device_t.sensor_t.tamper_status = 1;
		DEBUG_TRACE(WARN_TAG, "Device Has Been Removed!!!");
	}
	else
	{
		if (device_t.sensor_t.tamper_status)
		{
			device_t.sensor_t.tamper_status = 0;
		}
	}
	HAL_GPIO_WritePin(ENS210_PWR_GPIO_Port, ENS210_PWR_Pin, GPIO_PIN_RESET);
}

#if 0

void ADC1_COMP_IRQHandler(void)
{
	if (LL_ADC_IsActiveFlag_EOS(ADC1))
	{
		convertedValue = LL_ADC_REG_ReadConversionData32(ADC1);
		LL_ADC_ClearFlag_EOS(ADC1);
	}
}


/**
 * @brief  转换电池电量，计算电量信息
 * @param  None
 * @retval None
 */
void ADC_BatterySglConvert(void)
{
	static u8 tempBattery = 100;
	if (batteryVoltageConvertInterval)
		return;

	LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
	LL_ADC_InitTypeDef ADC_InitStruct = {0};

	/* Peripheral clock enable */
	LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);

	/* ADC interrupt Init */
	NVIC_SetPriority(ADC1_COMP_IRQn, 0);
	NVIC_EnableIRQ(ADC1_COMP_IRQn);

	/** Configure Regular Channel
	 */
	LL_ADC_REG_SetSequencerChAdd(ADC1, LL_ADC_CHANNEL_VREFINT);
	LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_VREFINT);
	/** Common config
	 */
	ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
	ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
	ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
	ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
	ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
	LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
	LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_39CYCLES_5);
	LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
	LL_ADC_REG_SetSequencerScanDirection(ADC1, LL_ADC_REG_SEQ_SCAN_DIR_FORWARD);
	LL_ADC_SetCommonFrequencyMode(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_CLOCK_FREQ_MODE_HIGH);
	LL_ADC_DisableIT_EOC(ADC1);
	LL_ADC_DisableIT_EOS(ADC1);
	ADC_InitStruct.Clock = LL_ADC_CLOCK_SYNC_PCLK_DIV2;
	ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
	ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
	ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
	LL_ADC_Init(ADC1, &ADC_InitStruct);

	/* Enable ADC internal voltage regulator */
	LL_ADC_EnableInternalRegulator(ADC1);

	LL_ADC_EnableIT_EOS(ADC1);

	convertedValue = 0;
	/* Enable ADC internal voltage regulator */
	LL_ADC_StartCalibration(ADC1);
	while (LL_ADC_IsCalibrationOnGoing(ADC1))
		;
	LL_ADC_Enable(ADC1);
	LL_ADC_REG_StartConversion(ADC1);
	HAL_Delay(10);

	while (convertedValue == 0)
		; //等待转换完成

	LL_ADC_REG_StopConversion(ADC1);
	while (LL_ADC_REG_IsConversionOngoing(ADC1) != 0)
		;
	LL_ADC_Disable(ADC1);
	batteryVoltageConvertInterval = 60 * SEC_DELAY;

	/* Compute the voltage */
	// batteryVoltage = (convertedValue * 3300) / 0xFFF;

	u16 VREFINT_CAL = *(__IO u16 *)(0x1FF80078); //获取内部基准
	float batteryVoltage = 3000 * VREFINT_CAL / convertedValue;

	if (batteryVoltage - 2500 > 0 && batteryVoltage < 3100)
	{
		batteryLevel = (batteryVoltage - 2600) * 2 / 1000;
	}
	else if (batteryVoltage >= 3100)
		batteryLevel = 100;
	else
		batteryLevel = 0;
	if (batteryLevel <= 20 && !batteryStatus)
	{
		batteryStatus = 1; //低电量  上报平台报警
		device_t.updateBattery = 1;
	}
	else if (batteryStatus && batteryLevel > 25)
	{
		batteryStatus = 0; //正常
		device_t.updateBattery = 1;
	}
	else if (batteryLevel != tempBattery)
	{
		tempBattery = batteryLevel; //电量信息更新
		device_t.updateBattery = 1;
	}
	DEBUG_TRACE(LOG_TAG, "Get Battery %.2fv, Level %d\n", batteryVoltage, batteryLevel);
}

#else

/**
 * @brief  转换电池电量，计算电量信息（以前期预估上报次数来计算剩余容量）
 * @param  None
 * @retval None
 */
void ADC_BatterySglConvert(void)
{
	static u8 tempBattery = 100;
	if (batteryInitialCapacity++ <= BATTERY_MAX_PUSH_CNT)
	{
		batteryLevel = (double)((BATTERY_MAX_PUSH_CNT - batteryInitialCapacity) * 100) / BATTERY_MAX_PUSH_CNT + 0.5;
		if (batteryLevel != tempBattery)
		{
			tempBattery = batteryLevel; //电量信息更新
			device_t.updateBattery = 1;
		}
	}
	else
	{
		device_t.updateBattery = 1;
		batteryLevel = 0;
	}
}
#endif
