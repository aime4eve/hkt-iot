
#ifndef __ZMOD441X_H__
#define __ZMOD441X_H__

#include "config.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void ZMOD4410_Init(void);
    void readZMOD4410ConvertResult(void);
    u8 ZMOD4410_Check(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
