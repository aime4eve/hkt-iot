
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "stdbool.h"

#define BATTERY_MAX_PUSH_CNT	150000 //电池可上报次数

    extern u16 batteryVoltageConvertInterval;
    extern u8 batteryLevel;
    extern bool batteryStatus;
		extern u32 batteryInitialCapacity;
    extern u32 convertedValue;

    void ADC_BatterySglConvert(void);
    void ADC1_Init(void);
    void DAC1_Init(void);
    void convert_light_sensor(void);

#ifdef __cplusplus
}
#endif

#endif
