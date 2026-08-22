
#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include <config.h>

void bma456_init(void);
void bma456_convertData(void);

#ifdef __cplusplus
}
#endif

#endif
