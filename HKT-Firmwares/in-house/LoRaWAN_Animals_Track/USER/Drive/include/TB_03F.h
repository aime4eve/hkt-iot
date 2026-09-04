
#ifndef __TB_03F_H__
#define __TB_03F_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    bool AT_Ble_checkAlive(void);
    bool AT_Ble_restFactory(void);
    bool AT_Ble_restart(void);
    bool AT_Ble_setSleep(u8 mode);
    bool AT_Ble_getVer(void);
    bool AT_Ble_getMac(void);
    bool AT_Ble_setName(void);
    bool AT_Ble_getName(void);
    bool AT_Ble_setTxUUID(void);
    bool AT_Ble_setRxUUID(void);
    bool AT_Ble_getTxUUID(void);
    bool AT_Ble_getRxUUID(void);
    bool AT_Ble_setAdvIntv(u16 interval);
    bool AT_Ble_getBleState(void);

    void ble_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
