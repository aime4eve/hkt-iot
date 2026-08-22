
#ifndef __TRNG_H__
#define __TRNG_H__

#include "config.h"

void TRNG_Init(void);
uint32_t TRNG_Get_Crc32(uint32_t DataIn);
uint32_t TRNG_Get_RandNum(void);
uint32_t RNG_ClkCalc(uint32_t RCHFClk);
uint32_t Calc_Crc32_MPEG2(const uint8_t *data, uint32_t len);

#endif /* __SYSTICK_H */
