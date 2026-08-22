
#ifndef __ADC_H__
#define __ADC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include <config.h>

extern u16 batteryVoltageConvertInterval;

void ADC_BatterySglConvert(void);

#ifdef __cplusplus
}
#endif

#endif
