
#ifndef __CB_HCHO_V4C_H__
#define __CB_HCHO_V4C_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    extern u8 heater_wait_time;

    void HCHO_Init(void);
    void Select_PPM_From_HCHO(void);
    void HCHO_Cmd_Process(u8 *cmd, u16 cmd_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
