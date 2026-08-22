
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "stdbool.h"

#define BATTERY_MAX_PUSH_CNT	40000 //电池8000ma  每次10s，可以工作57600次

    extern u16 batteryVoltageConvertInterval;
    extern u8 batteryLevel;
    extern bool batteryStatus;
		extern u32 batteryInitialCapacity;

    void ADC_BatterySglConvert(void);
    void MX_ADC_Init(void);
    void Stop_ADC(void);
    void StartAdCollect(void);

#ifdef __cplusplus
}
#endif

#endif
