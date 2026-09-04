
#ifndef __NZ3801_H__
#define __NZ3801_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#if (!FSV9522_CARD_ENABLE)
    u8 singleCardRead(void);
    void thread_card(ULONG thread_input);
    void nz3801_sleep(void);
    void nz3801_init(void);
    void nz3801_rst(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
