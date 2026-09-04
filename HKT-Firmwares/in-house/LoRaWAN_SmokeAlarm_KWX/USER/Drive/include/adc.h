
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define BATTERY_MAX_PUSH_CNT 150000

extern u16 batteryVoltageConvertInterval;
extern bool gADC_CycleEndOfConversion;

void ADC_BatterySglConvert(void);
void ADC_NtcSglConvert(void);

#ifdef __cplusplus
}
#endif

#endif
