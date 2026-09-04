/**
  ******************************************************************************
  * @file    fm33a0xxev_iwdt.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the IWDT firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_IWDT_H
#define __FM33A0XXEV_IWDT_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	IWDT_SERV_IWDTSERV_Pos	0	/* IWDT启动后，软件向此地址写入0x1234_5A5A时清狗 */
#define	IWDT_SERV_IWDTSERV_Msk	(0xffffffffU << IWDT_SERV_IWDTSERV_Pos)

#define	IWDT_CFGR_IWDTOVP_Pos	0	/* IWDT溢出周期设置
x00：125ms
x01：500ms
x10：2s
x11：8s
说明：当bit2为0时，休眠后仍使用短周期，而bit2为1时，休眠后自动使用4096s；非休眠状态下仅支持125ms/500ms/2s/8s四种周期。 */
#define	IWDT_CFGR_IWDTOVP_Msk	(0x7U << IWDT_CFGR_IWDTOVP_Pos)

#define	IWDT_CNTR_IWDTCNT_Pos	0	/* IWDT当前计数值 */
#define	IWDT_CNTR_IWDTCNT_Msk	(0xfffffU << IWDT_CNTR_IWDTCNT_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void IWDT_Deinit(void);

/* IWDT启动后，软件向此地址写入0x1234_5A5A时清狗 相关函数 */
extern void IWDT_SERV_IWDTSERV_Set(uint32_t SetValue);

/* IWDT溢出周期设置
x00：125ms
x01：500ms
x10：2s
x11：8s
说明：当bit2为0时，休眠后仍使用短周期，而bit2为1时，休眠后自动使用4096s；非休眠状态下仅支持125ms/500ms/2s/8s四种周期。 相关函数 */
extern void IWDT_CFGR_Write(uint32_t SetValue);
extern uint32_t IWDT_CFGR_Read(void);

/* IWDT当前计数值 相关函数 */
extern uint32_t IWDT_CNTR_IWDTCNT_Get(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_IWDT_H */
