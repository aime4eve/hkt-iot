
#ifndef __SHT40X_H__
#define __SHT40X_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void SHT40X_Init(void);
    void readSHT40XConvertResult(void);
    u8 SHT40X_Check(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
