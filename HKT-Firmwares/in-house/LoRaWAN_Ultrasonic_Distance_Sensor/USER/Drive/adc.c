
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "control_center.h"
#include "gpio.h"
#include "systick.h"
#include "uart.h"

u16 batteryVoltageConvertInterval; // 电量检测转换间隔
u32 convertedValue;

void ADC1_IRQHandler(void)
{
    if (LL_ADC_IsActiveFlag_EOS(ADC1)) {
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
    // if (batteryVoltageConvertInterval)
    //     return;
    PeriphCommonClock_Config();
    LL_ADC_InitTypeDef ADC_InitStruct = {0};
    LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};
    LL_ADC_CommonInitTypeDef ADC_CommonInitStruct = {0};
    LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

    LL_ADC_DeInit(ADC1);

    /* Peripheral clock enable */
    LL_RCC_SetADCClockSource(LL_RCC_ADC_CLKSOURCE_PLLSAI1);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_ADC);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);

    /**ADC GPIO 配置：PA0   ------> ADC_IN0  */
    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 interrupt Init */
    NVIC_SetPriority(ADC1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), 0, 0));
    NVIC_EnableIRQ(ADC1_IRQn);

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
    ADC_CommonInitStruct.CommonClock = LL_ADC_CLOCK_ASYNC_DIV8;
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

    /** Configure Regular Channel*/
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_12);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_12, LL_ADC_SAMPLINGTIME_47CYCLES_5);
    LL_ADC_SetChannelSingleDiff(ADC1, LL_ADC_CHANNEL_12, LL_ADC_SINGLE_ENDED);

    LL_ADC_StartCalibration(ADC1, LL_ADC_SINGLE_ENDED); // 开始校准
    while (!LL_ADC_IsCalibrationOnGoing(ADC1)) {
    }; // 等待校准完成

    GPIO_VOL_CTRL_H;
    LL_ADC_ClearFlag_EOS(ADC1);
    LL_ADC_EnableIT_EOS(ADC1); // 使能中断
    LL_ADC_Enable(ADC1);       // 使能ADC
                               /* 等待ADC使能 */
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) != SET) {
    }

    convertedValue = 0;
    LL_ADC_ClearFlag_EOC(ADC1);
    LL_ADC_REG_StartConversion(ADC1);
    // while (LL_ADC_REG_IsConversionOngoing(ADC1)) // 等待ADC转换完成
    //     ;
    // mDelay(2);

    while (convertedValue == 0)
        ; // 等待转换完成
    convertedValue = LL_ADC_REG_ReadConversionData32(ADC1);

    LL_ADC_REG_StopConversion(ADC1);
    // while (LL_ADC_REG_IsConversionOngoing(ADC1) != 0)
    //     ;
    LL_ADC_Disable(ADC1);
    LL_ADC_DisableInternalRegulator(ADC1);
    GPIO_VOL_CTRL_L;

    LL_RCC_PLLSAI1_DisableDomain_ADC();
    LL_RCC_PLLSAI1_Disable();

    GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_DOWN;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    // batteryVoltageConvertInterval = 60 * SEC_DELAY;

    /* Compute the voltage */
    // float batteryVoltage = 3300 * VREFINT_CAL / convertedValue;
    // vdd = __LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA_APPLI,convertedValue,LL_ADC_RESOLUTION_12B);
    // u16 batteryVoltage = __LL_ADC_CALC_VREFANALOG_VOLTAGE(convertedValue, LL_ADC_RESOLUTION_12B);
    u16 batteryVoltage = 3600 * convertedValue * 2 / 0xFFF + 92; // 计算电压 补偿0.92mV
    device_t.battery_voltage = batteryVoltage;
    DEBUG_TRACE(LOG_TAG, "Get Battery Voltage: %dmV\n", batteryVoltage);
}
