/**
  ******************************************************************************
  * @file    fm33a0xxev_wwdt.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the WWDT firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_WWDT_H
#define __FM33A0XXEV_WWDT_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	WWDT_CR_CON_Pos	0	/* 当CPU向此地址写入0x5A时启动WWDT定时器 (WWDT Control,write only)
在启动WWDT后，当CPU向此地址写入0xAC时清零计数器 */
#define	WWDT_CR_CON_Msk	(0xffU << WWDT_CR_CON_Pos)

#define	WWDT_CFGR_CFG_Pos	0	/* 配置WWDT看门狗溢出时间，复位值011，由于上电后系统时钟默认为8Mhz，所以默认溢出周期大约32ms (WWDT Config)
000：TPCLK * 4096 * 1
001：TPCLK * 4096 * 4
010：TPCLK * 4096 * 16
011：TPCLK * 4096 * 64
100：TPCLK * 4096 * 128
101：TPCLK * 4096 * 256
110：TPCLK * 4096 * 512
111：TPCLK * 4096 * 1024 */
#define	WWDT_CFGR_CFG_Msk	(0x7U << WWDT_CFGR_CFG_Pos)

#define	WWDT_CNTR_CNT_Pos	0	/* WWDT计数寄存器值，软件可通过查询此寄存器了解WWDT计时进度 (WWDT Counter value,read only) */
#define	WWDT_CNTR_CNT_Msk	(0x3ffU << WWDT_CNTR_CNT_Pos)

#define	WWDT_IER_IE_Pos	0	/* WWDT中断使能 (WWDT Interrupt Enable)
0：中断使能禁止
1：中断使能打开 */
#define	WWDT_IER_IE_Msk	(0x1U << WWDT_IER_IE_Pos)
	/* 中断使能禁止 */
	/* 中断使能打开 */

#define	WWDT_ISR_NOVF_Pos	0	/* WWDT 75%计时中断标志，写1清零 (Near Overflow Flag,write 1 to clear)
0：无中断产生
1：中断标志置位
如果IE=1，则此寄存器置位将触发中断 */
#define	WWDT_ISR_NOVF_Msk	(0x1U << WWDT_ISR_NOVF_Pos)

#define	WWDT_PSCR_DIV_CNT_Pos	0	/* WWDT的4096预分频计数器当前计数值，只读 (WWDT prescaler Divider Counte,read only) */
#define	WWDT_PSCR_DIV_CNT_Msk	(0xfffU << WWDT_PSCR_DIV_CNT_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void WWDT_Deinit(void);

/* 当CPU向此地址写入0x5A时启动WWDT定时器 (WWDT Control,write only)
在启动WWDT后，当CPU向此地址写入0xAC时清零计数器 相关函数 */
extern void WWDT_CR_Write(uint32_t SetValue);

/* 配置WWDT看门狗溢出时间，复位值011，由于上电后系统时钟默认为8Mhz，所以默认溢出周期大约32ms (WWDT Config)
000：TPCLK * 4096 * 1
001：TPCLK * 4096 * 4
010：TPCLK * 4096 * 16
011：TPCLK * 4096 * 64
100：TPCLK * 4096 * 128
101：TPCLK * 4096 * 256
110：TPCLK * 4096 * 512
111：TPCLK * 4096 * 1024 相关函数 */
extern void WWDT_CFGR_Write(uint32_t SetValue);
extern uint32_t WWDT_CFGR_Read(void);

/* WWDT计数寄存器值，软件可通过查询此寄存器了解WWDT计时进度 (WWDT Counter value,read only) 相关函数 */
extern uint32_t WWDT_CNTR_Read(void);

/* WWDT中断使能 (WWDT Interrupt Enable)
0：中断使能禁止
1：中断使能打开 相关函数 */
extern void WWDT_IER_IE_Setable(FunState NewState);
extern FunState WWDT_IER_IE_Getable(void);

/* WWDT 75%计时中断标志，写1清零 (Near Overflow Flag,write 1 to clear)
0：无中断产生
1：中断标志置位
如果IE=1，则此寄存器置位将触发中断 相关函数 */
extern void WWDT_ISR_NOVF_Clr(void);
extern FlagStatus WWDT_ISR_NOVF_Chk(void);

/* WWDT的4096预分频计数器当前计数值，只读 (WWDT prescaler Divider Counte,read only) 相关函数 */
extern uint32_t WWDT_PSCR_Read(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_WWDT_H */
