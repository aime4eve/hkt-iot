
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "control_center.h"
#include "systick.h"
#include "uart.h"

u8 batteryLevel;
bool batteryStatus;

u16 batteryVoltageConvertInterval; // 电量检测转换间隔
u32 convertedValue;

u32 batteryInitialCapacity; // 电池上报次数

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
	mDelay(10);

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
    if (++batteryInitialCapacity <= BATTERY_MAX_PUSH_CNT) {
        batteryLevel = (double)(BATTERY_MAX_PUSH_CNT - batteryInitialCapacity) * 100 / BATTERY_MAX_PUSH_CNT + 0.5;
        if (batteryLevel != tempBattery) {
            tempBattery = batteryLevel; // 电量信息更新
        }
    } else {
        batteryLevel = 0;
    }
}
#endif
