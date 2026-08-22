
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "communicate.h"
#include "control_center.h"
#include "gpio.h"
#include "sound.h"
#include "systick.h"
#include "uart.h"

u8 batteryLevel;
bool batteryStatus;

u16 batteryVoltageConvertInterval; // 电量检测转换间隔
u32 convertedValue;
u32 batteryInitialCapacity; // 电池上报次数

#define const_adc_Slope (*((u16 *)(0x1FFFFA84)))  // ADC斜率，除1000后使用         FA08  EF08  F408
#define const_adc_Offset (*((int16_t *)(0x1FFFFA86))) // ADC截距，除100后使用，单位mV   80FF  4B02  9601

struct ADC_Struct {
    u16 *Bufu16;    // 数据缓冲区地址	//注意内存对齐
    u8 SampleNeed;  // 需要采样的数据个数
    u8 SampleCount; // 已经采样的数据个数
};

struct ADC_Struct ADC_Struct;

uint32_t ANAC_ADC_VoltageCalc(uint32_t ADCData)
{
    uint32_t Volt = 0;

    Volt = (ADCData * const_adc_Slope / 1000) + const_adc_Offset / 100;
    return Volt;
}

u8 ADC_Wait_Finish(void)
{
    for (int i = 0; i < 10; i++) {
        if (SET == ADC_ISR_ADC_IF_Chk())
            return 0;
        tx_thread_sleep(1);
    }
    return 1; // 超时
}

/**
 * @brief  转换电池电量，计算电量信息
 * @param  None
 * @retval None
 */
void ADC_BatterySglConvert(void)
{
    static u8 tempBattery = 100;
    if (batteryTimestamp > Timestamp)
        return;
    batteryTimestamp = Timestamp + 24 * 3600;
    CMU_PERCLK_SetableEx(PADCLK, ENABLE); // IO控制时钟寄存器使能
    OutputIO(BAT_CTRL_PORT, BAT_CTRL_PIN, OUT_PUSHPULL);
    BAT_CTRL_H;

    AnalogIO(GPIOC, GPIO_Pin_15);
    GPIOx_ANEN_Setable(GPIOC, GPIO_Pin_15, ENABLE);

    CDIF_CR_INTF_EN_Setable(ENABLE); // 跨电源域接口使能
    VRTC_Init_RCMF_Trim();
    VRTC_RCMFCR_EN_Setable(ENABLE);
    VRTC_ADCCR_CKS_Set(VRTC_ADCCR_CKS_RCMF_2);    // ADC工作时钟选择
    VRTC_ADCCR_CKE_Setable(ENABLE);               // ADC工作时钟使能
    ADC_CFGR_BUFSEL_Set(ADC_CFGR_BUFSEL_ADC_IN6); // ADC输入通道选择

    ADC_CFGR_BUFEN_Setable(ENABLE);     // ADC输入通道buffer使能/禁止
    ADC_CR_MODE_Set(ADC_CR_MODE_INNER); // ADC工作模式选择内部累加器
    //  ADC_TRIM_Write(0X7FF);			//adc频率1M 时 计算时间4ms
    ADC_TRIM_Write(0x3FF); // adc频率1M 时 计算时间2ms
    //	ADC_TRIM_Write(0X1FF);			//adc频率1M 时 计算时间1ms

    ADC_CR_ADC_IE_Setable(DISABLE); // 内部累加模式中断禁止
    // ADC_CR_EN_Setable(DISABLE);		// ADC关闭
    ADC_CR_EN_Setable(ENABLE); // ADC启动

    ADC_ISR_ADC_IF_Clr(); // 清除中断标志

    tx_thread_sleep(20);

    u16 ADCData[12];
    u32 fTempADC = 0;
    u16 max, min;
    int i = 0;
    for (i = 0; i < 12; i++) {
        ADC_ISR_ADC_IF_Clr(); // 清除中断标志

        ADCData[i] = 0;
        if (0 == ADC_Wait_Finish()) // 等待转换完成
        {
            ADCData[i] = ADC_DR_Read(); // 读取AD值
            fTempADC += ADCData[i];
        } else {
            break;
        }
    }

    if (i == 12) {
        max = ADCData[0];
        min = ADCData[0];
        for (i = 0; i < 12; i++) {
            max = (ADCData[i] < max) ? max : ADCData[i];
            min = (min < ADCData[i]) ? min : ADCData[i];
        }
        fTempADC = (fTempADC - max - min) / 10;
    } else {
        DEBUG_TRACE(LOG_TAG, "Get Battery Fail[%d]", i);
    }

    GPIOx_ANEN_Setable(GPIOC, GPIO_Pin_15, DISABLE);
    ADC_CFGR_BUFEN_Setable(DISABLE);
    ADC_CR_EN_Setable(DISABLE); // ADC关闭
    VRTC_RCMFCR_EN_Setable(DISABLE);
    CDIF_CR_INTF_EN_Setable(DISABLE);

    GPIO_OUTPUT_L(GPIOC, GPIO_Pin_15);
    // AnalogIO(GPIOC, GPIO_Pin_15);
    BAT_CTRL_L;

    u32 batteryVoltage = ANAC_ADC_VoltageCalc(fTempADC) * 1020 / 510 * 2;

    if (batteryVoltage >= 5000 && batteryVoltage < 6200) {
        batteryLevel = (batteryVoltage - 5000) * 100 / 1200;
    } else if (batteryVoltage >= 6200 && batteryVoltage < 7000)
        batteryLevel = 100;
    else
        batteryLevel = 0;
    if (batteryLevel <= 10 && !batteryStatus) {
        batteryStatus = 1; // 低电量  上报平台报警
        device_t.updateBattery = 1;
    } else if (batteryStatus && batteryLevel > 25) {
        batteryStatus = 0; // 正常
        device_t.updateBattery = 1;
    } else if (batteryLevel != tempBattery) {
        tempBattery = batteryLevel; // 电量信息更新
        device_t.updateBattery = 1;
    }
    // if (batteryLevel <= 10) {
    //     // 电量低
    //     Sound_Insert_Number(1, VOICE_LOW_BATTERY);
    // }

    device_t.updateBattery = 0; // 新版本为避免反复上报，关闭电量变更后重复上报数据功能
    DEBUG_TRACE(LOG_TAG, "Get Battery %dmV, Level %d", batteryVoltage, batteryLevel);
}
