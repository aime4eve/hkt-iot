
#ifndef __CM1106SL_H__
#define __CM1106SL_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void CM1106SL_Init(void);
void CM1106SL_Sleep(void);
void cm_search_co2(u8 save);
bool cm_forced(u16 forced);
bool cm_cal_store(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
