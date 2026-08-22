
#ifndef __SCD40X_H__
#define __SCD40X_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void SCD4X_DeInit(void);
void SCD4X_ReStart(void);
void SCD4X_Start_Periodic(void);
void SCD4X_Read_Periodic_Data(u8 save);
bool SCD4X_ForcedPPM(u16 forced);
void SCD4X_Init(void);
void readSCD4XConvertResult(u8 save);
u8 SCD4X_Check(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
