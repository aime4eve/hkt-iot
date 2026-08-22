
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "systick.h"


    void BEEP_RUN(int time, int percent);
    void TIM6_Init(u16 period, u16 prescaler);

#ifdef __cplusplus
}
#endif

#endif
