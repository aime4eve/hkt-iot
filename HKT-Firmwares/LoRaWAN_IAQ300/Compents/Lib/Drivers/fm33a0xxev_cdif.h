/**
  ******************************************************************************
  * @file    fm33a0xxev_cdif.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the CDIF firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_CDIF_H
#define __FM33A0XXEV_CDIF_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 


#define	CDIF_CR_INTF_EN_Pos	0	/* 跨电源域接口使能 */
#define	CDIF_CR_INTF_EN_Msk	(0xfU << CDIF_CR_INTF_EN_Pos)
#define	CDIF_CR_INTF_EN_ENABLE	(0xAU << CDIF_CR_INTF_EN_Pos)
#define	CDIF_CR_INTF_EN_DISABLE	(0x5U << CDIF_CR_INTF_EN_Pos)
	/* 其他：关闭接口 */
	/* 1010：使能接口 */

#define	CDIF_PSCR_PRSC_Pos	0	/* 跨电源域时序控制，配置相对于APBCLK的定时比例 */
#define	CDIF_PSCR_PRSC_Msk	(0xfU << CDIF_PSCR_PRSC_Pos)
#define	CDIF_PSCR_PRSC_DIV1	(0x0U)	/* 000：1分频 */
#define	CDIF_PSCR_PRSC_DIV2	(0x1U)	/* 001：2分频 */
#define	CDIF_PSCR_PRSC_DIV4	(0x2U)	/* 010：4分频 */
#define	CDIF_PSCR_PRSC_DIV8	(0x3U)	/* 011：8分频 */
#define	CDIF_PSCR_PRSC_DIV16	(0x4U)	/* 100：16分频 */
#define	CDIF_PSCR_PRSC_DIV32	(0x5U)	/* 101：32分频 */
#define	CDIF_PSCR_PRSC_DIV64	(0x6U)	/* 110：64分频 */
#define	CDIF_PSCR_PRSC_DIV128	(0x7U)	/* 111：128分频 */
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void CDIF_Deinit(void);

/* 跨电源域接口使能 相关函数 */
extern void CDIF_CR_INTF_EN_Setable(FunState NewState);
extern FunState CDIF_CR_INTF_EN_Getable(void);

/* 跨电源域时序控制，配置相对于APBCLK的定时比例 相关函数 */
extern void CDIF_PSCR_Write(uint32_t SetValue);
extern uint32_t CDIF_PSCR_Read(void);
//Announce_End


#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_CDIF_H */
