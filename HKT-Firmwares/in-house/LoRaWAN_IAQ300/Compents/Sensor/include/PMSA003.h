
#ifndef __PMSA003_H__
#define __PMSA003_H__

#include "config.h"

// ** IO ** //
#define PMSA_RST_PORT GPIOB
#define PMSA_RST_PIN GPIO_Pin_5
#define PMSA_RST_H GPIO_SetBits(PMSA_RST_PORT, PMSA_RST_PIN)
#define PMSA_RST_L GPIO_ResetBits(PMSA_RST_PORT, PMSA_RST_PIN)

#define PMSA_SET_PORT GPIOB
#define PMSA_SET_PIN GPIO_Pin_4
#define PMSA_SET_H GPIO_SetBits(PMSA_SET_PORT, PMSA_SET_PIN)
#define PMSA_SET_L GPIO_ResetBits(PMSA_SET_PORT, PMSA_SET_PIN)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    void PMSA003_PortCfg(void);
    void PMSA003_Init(void);
    void PMSA003_SendReadCmd(void);
    u16 PMSA_CRC_Calculate(u8 *data, u16 count);
    void sendPMSA_DataPackage(u16 cmd, u16 data);
    void PMSA_Cmd_Process(u8 *cmd, u16 cmd_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
