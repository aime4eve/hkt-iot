
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

    void TIM1_Init(u16 period, u16 prescaler);
		void TIM6_Init(u16 period, u16 prescaler);

#ifdef __cplusplus
}
#endif

#endif
