
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "communicate.h"
#include "systick.h"
#include "uart.h"

u16 batteryVoltageConvertInterval; // 电量检测转换间隔
bool gADC_CycleEndOfConversion;

/**
 * @brief   This function handles ADC interrupt.
 * @retval  None
 */
void ADC_IRQHandler(void)
{
    if (ADC_GetIntStatus(HT_ADC0, ADC_INT_SINGLE_EOC) == SET) {
        ADC_ClearIntPendingBit(HT_ADC0, ADC_FLAG_SINGLE_EOC);
        gADC_CycleEndOfConversion = TRUE;
    }
}

#define R_REF 100000.0 // 参考电阻值，单位为欧姆
#define B_VALUE 3950.0 // B值

double calculate_temperature(double resistance)
{
    double temperature;
    double r_inf = R_REF * exp(-B_VALUE / 298.15);   // 计算25°C参考电阻值
    temperature = B_VALUE / log(resistance / r_inf); // 计算温度
    return temperature - 273.15;                     // 转换为摄氏温度并返回
}

float calculateVoltageDivider(float r1, float r2, float v_in)
{
    // 计算电压分压公式：v_out = (r2 / (r1 + r2)) * v_in
    float v_out = (r2 / (r1 + r2)) * v_in;
    return v_out;
}

// 已知电阻R2, 计算分压电阻R1的阻值
double calculate_resistance(double r2, float v_in, float v_out)
{
    // 使用电压分压公式：Vout = Vin * R2 / (R1 + R2)
    // 解方程得到 R1 = R2 * (Vin / Vout - 1)
    double r1 = r2 * (v_in / v_out - 1);
    return r1;
}

/**
 * @brief
 * @param adc_reading NTC ADC Value
 * @brief: Rt = R *EXP(B*(1/T1-1/T2))
 * @return: ntc temperature
 */
float convertNtcTemperature(uint32_t adc_reading) // 60摄氏度时电阻值为11K左右
{
    float temp = 0.0;
    float Rp = 100000.0;
    float T2 = (273.15 + 25.0);
    float Bx = 3950.0;
    float Ka = 273.15;
    double Rt = 0;
    if (adc_reading == 0)
        adc_reading = 1; // handling of ADC conversion errors
    Rt = calculate_resistance(Rp, 3000, ((double)adc_reading * 3000 / 4096));
    temp = calculate_temperature(Rt);
    return temp;
}

/**
 * @brief  通过温敏电阻转换环境温度
 * @param  None
 * @retval None
 */
void ADC_NtcSglConvert(void)
{
    gADC_CycleEndOfConversion = FALSE;

    /* Enable peripheral clock                                                                              */
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.AFIO = 1;
    CKCUClock.Bit.ADC0 = 1;
    CKCUClock.Bit.PA = 1;
    CKCUClock.Bit.PB = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // 拉低控制IO
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_6, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(HT_GPIOB, GPIO_PIN_6, GPIO_PR_DISABLE);
    GPIO_DirectionConfig(HT_GPIOB, GPIO_PIN_6, GPIO_DIR_OUT);
    GPIO_WriteOutBits(HT_GPIOB, GPIO_PIN_6, SET);
    delay_1us(10);

    // 初始化ADC端口                                                    */
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_0, AFIO_FUN_ADC0);
    CKCU_SetADCnPrescaler(CKCU_ADCPRE_ADC0, CKCU_ADCPRE_DIV64);
    ADC_RegularGroupConfig(HT_ADC0, ONE_SHOT_MODE, 1, 0);
    ADC_SamplingTimeConfig(HT_ADC0, 36);
    ADC_RegularChannelConfig(HT_ADC0, ADC_CH_2, 0, 36);
    /* Set software trigger as ADC trigger source                                                           */
    ADC_RegularTrigConfig(HT_ADC0, ADC_TRIG_SOFTWARE);
    /* Enable ADC single end of conversion interrupt                                                          */
    ADC_IntConfig(HT_ADC0, ADC_INT_SINGLE_EOC, ENABLE);
    NVIC_EnableIRQ(ADC_IRQn);

    /* Enable ADC                                                                                             */
    ADC_Cmd(HT_ADC0, ENABLE);
    ADC_SoftwareStartConvCmd(HT_ADC0, ENABLE);

    while (!gADC_CycleEndOfConversion)
        ;

    u32 ntcADCValue = (HT_ADC0->DR[0] & 0x0FFF);
    device_t.temperature = convertNtcTemperature(ntcADCValue);
    DEBUG_TRACE(LOG_TAG, "Covert Ntc Temperature %.2fC, ADC Value %d", device_t.temperature, ntcADCValue);

    if (device_t.temperature > 55 && !device_t.temp_state) {
        device_t.sync_state = 1;
        device_t.temp_state = 1;
    } else if (device_t.temperature < 50 && device_t.temp_state) {
        device_t.sync_state = 1;
        device_t.temp_state = 0;
    }

    ADC_Reset(HT_ADC0);
    ADC_DeInit(HT_ADC0);

    // 关闭控制IO 省电
    GPIO_WriteOutBits(HT_GPIOB, GPIO_PIN_6, RESET);
    // 关闭ADC端口                                                    */
    AFIO_GPxConfig(GPIO_PA, AFIO_PIN_0, AFIO_FUN_GPIO);
    GPIO_PullResistorConfig(HT_GPIOA, GPIO_PIN_0, GPIO_PR_DISABLE);
    GPIO_WriteOutBits(HT_GPIOA, GPIO_PIN_0, RESET);
    GPIO_DirectionConfig(HT_GPIOA, GPIO_PIN_0, GPIO_DIR_OUT);
}
