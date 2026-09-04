
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define BATTERY_MAX_PUSH_CNT  150000

    extern u16 batteryVoltageConvertInterval;
    extern u8 batteryLevel;
    extern bool batteryStatus;
    extern u32 batteryInitialCapacity;

    void ADC_BatterySglConvert(void);

#ifdef __cplusplus
}
#endif

#endif
