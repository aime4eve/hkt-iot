
#ifndef __REAL_TIME_H__
#define __REAL_TIME_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "systick.h"
#include <config.h>

void real_time_init(void);
void read_real_time(void);
void set_real_time(void);

#ifdef __cplusplus
}
#endif

#endif
