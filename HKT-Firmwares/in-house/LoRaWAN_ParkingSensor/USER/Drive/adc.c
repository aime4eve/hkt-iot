
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "control_center.h"
#include "gpio.h"
#include "p25q40.h"
#include "systick.h"
#include "uart.h"

u8 batteryLevel;
bool batteryStatus;

u16 batteryVoltageConvertInterval; // 电量检测转换间隔
u32 convertedValue;

u32 batteryInitialCapacity; // 电池上报次数

static u8 valid_cnt, invalid_cnt;

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
		device_t.sync_battery = 1;
	}
	else if (batteryStatus && batteryLevel > 25)
	{
		batteryStatus = 0; //正常
		device_t.sync_battery = 1;
	}
	else if (batteryLevel != tempBattery)
	{
		tempBattery = batteryLevel; //电量信息更新
		device_t.sync_battery = 1;
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
    if (batteryInitialCapacity <= BATTERY_MAX_PUSH_CNT) {
        batteryLevel = (BATTERY_MAX_PUSH_CNT - batteryInitialCapacity) * 100 / BATTERY_MAX_PUSH_CNT;
        if (batteryLevel != tempBattery) {
            tempBattery = batteryLevel; // 电量信息更新
            device_t.sync_battery = 1;
        }
    } else {
        device_t.sync_battery = 1;
        batteryLevel = 0;
    }
}
#endif

/**
 * @brief  ADC 初始化
 * @param
 * @retval
 */
void ADC1_Init(void)
{
    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_SYSCLK);

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC);

    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /**ADC1 GPIO Configuration
    PA6   ------> ADC1_IN11
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /** Common config
     */
    ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
    ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
    ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
    LL_ADC_Init(ADC1, &ADC_InitStruct);
    ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
    ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
    ADC_REG_InitStruct.SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE;
    ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_SINGLE;
    ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
    ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_PRESERVED;
    LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);

    ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_ASYNC_DIV128;
    LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC1), &ADC_CommonInitStruct);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_NONE);

    /* Disable ADC deep power down (enabled by default after reset state) */
    LL_ADC_DisableDeepPowerDown(ADC1);
    /* Enable ADC internal voltage regulator */
    LL_ADC_EnableInternalRegulator(ADC1);
    /* Delay for ADC internal voltage regulator stabilization. */
    /* Compute number of CPU cycles to wait for, from delay in us. */
    /* Note: Variable divided by 2 to compensate partially */
    /* CPU processing cycles (depends on compilation optimization). */
    /* Note: If system core clock frequency is below 200kHz, wait time */
    /* is only a few CPU processing cycles. */
    uint32_t wait_loop_index;
    wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US * (SystemCoreClock / (100000 * 2))) / 10);
    while (wait_loop_index != 0) {
        wait_loop_index--;
    }
    /** Configure Regular Channel
     */
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_11);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_11, LL_ADC_SAMPLINGTIME_12CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_11, LL_ADC_SINGLE_ENDED);

    LL_ADC_Enable(ADC1);
    if (invalid_cnt < 1)
        Delay_Ms(1);
    else if (invalid_cnt < 2)
        mDelay(100);
    else
        mDelay(1000);

    LL_ADC_REG_StartConversion(ADC1);
    // 等待转换完成
    while (!LL_ADC_IsActiveFlag_EOS(ADC1))
        ;
    LL_ADC_ClearFlag_EOS(ADC1);
    convertedValue = LL_ADC_REG_ReadConversionData12(ADC1);

    LL_ADC_REG_StopConversion(ADC1);
    while (LL_ADC_REG_IsConversionOngoing(ADC1) != 0)
        ;
    LL_ADC_Disable(ADC1);
}

/**
 * @brief  DAC 初始化
 * @param  None
 * @retval None
 */
void DAC1_Init(void)
{
    LL_DAC_InitTypeDef DAC_InitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DAC1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /**DAC1 GPIO Configuration
    PA4   ------> DAC1_OUT1
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /** DAC channel OUT1 config
     */
    DAC_InitStruct.TriggerSource = LL_DAC_TRIG_SOFTWARE;
    DAC_InitStruct.WaveAutoGeneration = LL_DAC_WAVE_AUTO_GENERATION_NONE;
    DAC_InitStruct.OutputBuffer = LL_DAC_OUTPUT_BUFFER_ENABLE;
    DAC_InitStruct.OutputConnection = LL_DAC_OUTPUT_CONNECT_GPIO;
    DAC_InitStruct.OutputMode = LL_DAC_OUTPUT_MODE_NORMAL;
    LL_DAC_Init(DAC1, LL_DAC_CHANNEL_1, &DAC_InitStruct);
    LL_DAC_DisableTrigger(DAC1, LL_DAC_CHANNEL_1);
}

void convert_light_sensor(void)
{
    // static u32 covert_time;
    // if (covert_time >= get_syspant_ms() || !device_t.power_on)
    //     return;

    // covert_time = get_syspant_ms() + 1000;

    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Peripheral clock enable */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    /**ADC1 GPIO Configuration
    PA6   ------> ADC1_IN11
    */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LIGHT_PWR_H;

    ADC1_Init();

    if (convertedValue < 2500) {
        if (invalid_cnt < 3) {
            invalid_cnt++;
        }
        valid_cnt = 0;
        if (!device_t.tamper_alarm && invalid_cnt >= 3) {
            device_t.tamper_alarm = 1;
            device_t.sync_state = 1;
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(WARN_TAG, "Tamper Alarm!!!");
            if (device_t.ble_connected) {
                BLE_SendCMD("Tamper Alarm!!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
    } else if (convertedValue > 3000) {
        if (valid_cnt < 3) {
            valid_cnt++;
        }
        invalid_cnt = 0;
        if (device_t.tamper_alarm && valid_cnt >= 3) {
            device_t.tamper_alarm = 0;
            device_t.sync_state = 1;
            device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
            DEBUG_TRACE(WARN_TAG, "Tamper DisAlarm!!!");
            if (device_t.ble_connected) {
                BLE_SendCMD("Tamper DisAlarm!!!");
            }
#if NOR_FLASH_ENABLE
            nor_flash_write(&tsdb);
#endif
        }
    }

    //     if (convertedValue < 2500 && !device_t.tamper_alarm) {
    //         device_t.tamper_alarm = 1;
    //         device_t.sync_state   = 1;
    //         device_t.sleep_delay  = WAIT_SLEEP_TIME_DELAY;
    //         DEBUG_TRACE(WARN_TAG, "Tamper Alarm!!!");
    //         if (device_t.ble_connected) {
    //             BLE_SendCMD("Tamper Alarm!!!");
    //         }
    // #if NOR_FLASH_ENABLE
    //         nor_flash_write(&tsdb);
    // #endif
    //     } else if (convertedValue > 3000 && device_t.tamper_alarm) {
    //         device_t.tamper_alarm = 0;
    //         device_t.sync_state   = 1;
    //         device_t.sleep_delay  = WAIT_SLEEP_TIME_DELAY;
    //         DEBUG_TRACE(WARN_TAG, "Tamper DisAlarm!!!");
    //         if (device_t.ble_connected) {
    //             BLE_SendCMD("Tamper DisAlarm!!!");
    //         }
    // #if NOR_FLASH_ENABLE
    //         nor_flash_write(&tsdb);
    // #endif
    //     }

    DEBUG_TRACE(LOG_TAG, "Read Light Sensor Value %d, Tamper State %d", convertedValue, device_t.tamper_alarm);

    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    // GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_6);

    LIGHT_PWR_L;
    LL_AHB2_GRP1_DisableClock(LL_AHB2_GRP1_PERIPH_ADC);
}
