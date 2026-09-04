
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

void TIM6_Init(u16 period, u16 prescaler);
void EnterStopMode(void);

#ifdef __cplusplus
}
#endif

#endif
