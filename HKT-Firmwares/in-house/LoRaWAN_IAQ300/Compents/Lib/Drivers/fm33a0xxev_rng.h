/**
  ******************************************************************************
  * @file    fm33a0xxev_rng.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the RNG firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_RNG_H
#define __FM33A0XXEV_RNG_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	RNG_CR_EN_Pos	0	/* RNG使能寄存器，软件写1启动 (RNG enable)
1：启动RNG
0：关闭RNG */
#define	RNG_CR_EN_Msk	(0x1U << RNG_CR_EN_Pos)
	/* 启动RNG */
	/* 关闭RNG */

#define	RNG_DOR_OUT_Pos	0	/* 随机数生成结果或CRC运算结果寄存器，只读
(RNG output) */
#define	RNG_DOR_OUT_Msk	(0xffffffffU << RNG_DOR_OUT_Pos)

#define	RNG_SR_LFSREN_Pos	1	/* LFSR状态标志，只读 (LFSR enable)
1：LFSR在运行中，不可进行CRC验证
0：LFSR不在运行中，可进行CRC验证
注：本寄存器不会引起模块中断，仅供查询 */
#define	RNG_SR_LFSREN_Msk	(0x1U << RNG_SR_LFSREN_Pos)
	/* LFSR在运行中，不可进行CRC验证 */
	/* LFSR不在运行中，可进行CRC验证 */
	/* 本寄存器不会引起模块中断，仅供查询 */

#define	RNG_SR_RNF_Pos	0	/* 随机数生成失败标志，软件写1清零 (Random Number Fail)
1：随机数未能通过质量检测
0：随机数通过质量检测 */
#define	RNG_SR_RNF_Msk	(0x1U << RNG_SR_RNF_Pos)

#define	RNG_CRC_CR_CRCEN_Pos	0	/* CRC使能控制寄存器，软件写1启动CRC，运算完成后硬件自动清零 (CRC enable)
1：CRC使能
0：CRC关闭 */
#define	RNG_CRC_CR_CRCEN_Msk	(0x1U << RNG_CRC_CR_CRCEN_Pos)
	/* CRC使能 */
	/* CRC关闭 */

#define	RNG_CRC_DIR_CRCIN_Pos	0	/* CRC运算数据输入寄存器 (CRC data input) */
#define	RNG_CRC_DIR_CRCIN_Msk	(0xffffffffU << RNG_CRC_DIR_CRCIN_Pos)

#define	RNG_CRC_SR_CRCDONE_Pos	0	/* CRC计算完成标志，软件写0清零 (CRC calculation done)
1：CRC计算完成
0：CRC计算未完成 */
#define	RNG_CRC_SR_CRCDONE_Msk	(0x1U << RNG_CRC_SR_CRCDONE_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void RNG_Deinit(void);

/* RNG使能寄存器，软件写1启动 (RNG enable)
1：启动RNG
0：关闭RNG 相关函数 */
extern void RNG_CR_EN_Setable(FunState NewState);
extern FunState RNG_CR_EN_Getable(void);

/* 随机数生成结果或CRC运算结果寄存器，只读
(RNG output) 相关函数 */
extern uint32_t RNG_DOR_Read(void);

/* LFSR状态标志，只读 (LFSR enable)
1：LFSR在运行中，不可进行CRC验证
0：LFSR不在运行中，可进行CRC验证
注：本寄存器不会引起模块中断，仅供查询 相关函数 */
extern FlagStatus RNG_SR_LFSREN_Chk(void);

/* 随机数生成失败标志，软件写1清零 (Random Number Fail)
1：随机数未能通过质量检测
0：随机数通过质量检测 相关函数 */
extern void RNG_SR_RNF_Clr(void);
extern FlagStatus RNG_SR_RNF_Chk(void);

/* CRC使能控制寄存器，软件写1启动CRC，运算完成后硬件自动清零 (CRC enable)
1：CRC使能
0：CRC关闭 相关函数 */
extern void RNG_CRC_CR_CRCEN_Setable(FunState NewState);
extern FunState RNG_CRC_CR_CRCEN_Getable(void);

/* CRC运算数据输入寄存器 (CRC data input) 相关函数 */
extern void RNG_CRC_DIR_Write(uint32_t SetValue);
extern uint32_t RNG_CRC_DIR_Read(void);

/* CRC计算完成标志，软件写0清零 (CRC calculation done)
1：CRC计算完成
0：CRC计算未完成 相关函数 */
extern void RNG_CRC_SR_CRCDONE_Clr(void);
extern FlagStatus RNG_CRC_SR_CRCDONE_Chk(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_RNG_H */
