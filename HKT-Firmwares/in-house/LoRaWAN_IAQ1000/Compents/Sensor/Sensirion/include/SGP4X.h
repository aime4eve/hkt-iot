
#ifndef __SGP4X_H__
#define __SGP4X_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void SGP4X_Init(void);
    void readSGP4XConvertResult(void);
    u8 SGP4X_Check(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
