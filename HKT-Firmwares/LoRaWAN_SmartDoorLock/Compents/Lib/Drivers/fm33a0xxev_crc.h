/**
  ******************************************************************************
  * @file    fm33a0xxev_crc.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the CRC firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_CRC_H
#define __FM33A0XXEV_CRC_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
	 
typedef struct
{       	
	uint32_t			CRCSEL;		/*!<CRC校验多项式位宽选择*/
	uint32_t 			OPWD;			/*!<WORD操作使能*/
	uint32_t      PARA;			/*!<CRC快速计算使能*/
	uint32_t			RFLTIN;		/*!<CRC输入反转控制*/
	uint32_t			RFLTO;		/*!<CRC输出反转控制*/
	FunState			XOR;			/*!<输出异或使能*/
	uint32_t			CRC_XOR;	/*!<运算结果异或寄存器*/
	uint32_t 			CRCPOLY;	/*!<CRC多项式寄存器*/
	uint32_t			LFSR;			/*!<CRC初始值*/
}CRC_InitTypeDef;

#define	CRC_DR_DR_Pos	0	/* CRCDR用于作为数据输入寄存器，并且在运算结束后保存CRC计算结果。(CRC Data Register)
作为输入时：若word操作使能，则对CRCDR[31:0]进行计算，共4次byte运算（由低到高）；否则对CRCDR[7:0]进行计算，共1次byte运算。
保存结果时：若为7位多项式结果保存在CRCDR[6:0]，若为8位多项式结果保存在CRCDR[7:0]，若为16位多项式结果保存在CRCDR[15:0]，若为32位多项式结果保存在CRCDR[31:0]。 */
#define	CRC_DR_DR_Msk	(0xffffffffU << CRC_DR_DR_Pos)

#define	CRC_CR_OPWD_Pos	9	/* WORD操作使能 (Operation by Word)
0：字节操作，CRC计算仅针对CRCDR最低字节进行
1：字操作，CRC计算针对CRCDR全部4字节进行 */
#define	CRC_CR_OPWD_Msk	(0x1U << CRC_CR_OPWD_Pos)
#define	CRC_CR_OPWD_BYTE	(0x0U << CRC_CR_OPWD_Pos)	/* 字节操作，CRC计算仅针对CRCDR最低字节进行 */
#define	CRC_CR_OPWD_WORD	(0x1U << CRC_CR_OPWD_Pos)	/* 字操作，CRC计算针对CRCDR全部4字节进行 */

#define	CRC_CR_PARA_Pos	8	/* CRC快速计算使能 (Parallel Calculation)
0：串行运算，计算1个字节需要8个时钟周期
1：并行计算，计算1个字节需要1个时钟周期 */
#define	CRC_CR_PARA_Msk	(0x1U << CRC_CR_PARA_Pos)
#define	CRC_CR_PARA_SERIAL	(0x0U << CRC_CR_PARA_Pos)	/* 串行运算，计算1个字节需要8个时钟周期 */
#define	CRC_CR_PARA_PARALLEL	(0x1U << CRC_CR_PARA_Pos)	/* 并行计算，计算1个字节需要1个时钟周期 */

#define	CRC_CR_RFLTIN_Pos	6	/* CRC输入反转控制 (Reflected Input)
00：输入不反转
01：输入按字节反转
10：输入按半字反转
11：输入按字反转
例如：计算数据为0x11223344，
如果RFLTIN==01，则将数据变为0x8844CC22，再进行计算如果RFLTIN==10，则将数据变为0x448822CC，再进行计算
如果RFLTIN==11，则将数据变为0x22CC4488，再进行计算 */
#define	CRC_CR_RFLTIN_Msk	(0x3U << CRC_CR_RFLTIN_Pos)
#define	CRC_CR_RFLTIN_NONE	(0x0U << CRC_CR_RFLTIN_Pos)	/* 输入不反转 */
#define	CRC_CR_RFLTIN_BYTE	(0x1U << CRC_CR_RFLTIN_Pos)	/* 输入按字节反转 */
#define	CRC_CR_RFLTIN_HALFWORD	(0x2U << CRC_CR_RFLTIN_Pos)	/* 输入按半字反转 */
#define	CRC_CR_RFLTIN_WORD	(0x3U << CRC_CR_RFLTIN_Pos)	/* 输入按字反转 */

#define	CRC_CR_RFLTO_Pos	5	/* CRC输出反转控制 (Reflected Output)
0：输入不反转
1：输入按字节反转
例如：
如果RFLTO==1，若当前计算的CRC结果为0x1234，则输出的结果为0x2C48如果 RFLTO==0，则直接输出0x1234注意：此结果不一定为最终输出结果，还需要看XOR是否为1，详见本寄存器bit2说明 */
#define	CRC_CR_RFLTO_Msk	(0x1U << CRC_CR_RFLTO_Pos)
#define	CRC_CR_RFLTO_NON	(0x0U << CRC_CR_RFLTO_Pos)	/* 输入不反转 */
#define	CRC_CR_RFLTO_BYTE	(0x1U << CRC_CR_RFLTO_Pos)	/* 输入按字节反转 */

#define	CRC_CR_RES_Pos	4	/* CRC结果标志位，只读 (Result Flag)
0：CRC结果为0
1：CRC结果非全0 */
#define	CRC_CR_RES_Msk	(0x1U << CRC_CR_RES_Pos)

#define	CRC_CR_BUSY_Pos	3	/* CRC运算标志位，只读 (Busy)
0：CRC运算结束
1：CRC运算进行中 */
#define	CRC_CR_BUSY_Msk	(0x1U << CRC_CR_BUSY_Pos)

#define	CRC_CR_XOR_Pos	2	/* 输出异或使能 (Output XORed with CRC_XOR register enable)
0：输出不异或CRC_XOR寄存器
1：输出异或CRC_XOR寄存器 */
#define	CRC_CR_XOR_Msk	(0x1U << CRC_CR_XOR_Pos)
	/* 输出不异或CRC_XOR寄存器 */
	/* 输出异或CRC_XOR寄存器 */

#define	CRC_CR_SEL_Pos	0	/* CRC多项式位宽选择 (Polynomial width Selection)
00：32位
01：16位
10：8位
11：7位 */
#define	CRC_CR_SEL_Msk	(0x3U << CRC_CR_SEL_Pos)
#define	CRC_CR_SEL_CRC32	(0x0U << CRC_CR_SEL_Pos)	/* 32位 */
#define	CRC_CR_SEL_CRC16	(0x1U << CRC_CR_SEL_Pos)	/* 16位 */
#define	CRC_CR_SEL_CRC8	(0x2U << CRC_CR_SEL_Pos)	/* 8位 */
#define	CRC_CR_SEL_CRC7	(0x3U << CRC_CR_SEL_Pos)	/* 7位 */

#define	CRC_LFSR_LFSR_Pos	0	/* CRC线性反馈移位寄存器 (Linear Feedback Shift Register)
运算开始前可以由软件写入CRC初始值 */
#define	CRC_LFSR_LFSR_Msk	(0xffffffffU << CRC_LFSR_LFSR_Pos)

#define	CRC_XOR_XOR_Pos	0	/* CRC运算结果异或寄存器 (eXclusive OR)
当CRC_CR.XOR为1时，CRC结果输出前将异或此寄存器的数据。 */
#define	CRC_XOR_XOR_Msk	(0xffffffffU << CRC_XOR_XOR_Pos)

#define	CRC_POLY_POLY_Pos	0	/* CRC运算多项式系数 (CRC Polynominals) */
#define	CRC_POLY_POLY_Msk	(0xffffffffU << CRC_POLY_POLY_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void CRC_Deinit(void);

/* CRCDR用于作为数据输入寄存器，并且在运算结束后保存CRC计算结果。(CRC Data Register)
作为输入时：若word操作使能，则对CRCDR[31:0]进行计算，共4次byte运算（由低到高）；否则对CRCDR[7:0]进行计算，共1次byte运算。
保存结果时：若为7位多项式结果保存在CRCDR[6:0]，若为8位多项式结果保存在CRCDR[7:0]，若为16位多项式结果保存在CRCDR[15:0]，若为32位多项式结果保存在CRCDR[31:0]。 相关函数 */
extern void CRC_DR_Write(uint32_t SetValue);
extern uint32_t CRC_DR_Read(void);

/* WORD操作使能 (Operation by Word)
0：字节操作，CRC计算仅针对CRCDR最低字节进行
1：字操作，CRC计算针对CRCDR全部4字节进行 相关函数 */
extern void CRC_CR_OPWD_Set(uint32_t SetValue);
extern uint32_t CRC_CR_OPWD_Get(void);

/* CRC快速计算使能 (Parallel Calculation)
0：串行运算，计算1个字节需要8个时钟周期
1：并行计算，计算1个字节需要1个时钟周期 相关函数 */
extern void CRC_CR_PARA_Set(uint32_t SetValue);
extern uint32_t CRC_CR_PARA_Get(void);

/* CRC输入反转控制 (Reflected Input)
00：输入不反转
01：输入按字节反转
10：输入按半字反转
11：输入按字反转
例如：计算数据为0x11223344，
如果RFLTIN==01，则将数据变为0x8844CC22，再进行计算如果RFLTIN==10，则将数据变为0x448822CC，再进行计算
如果RFLTIN==11，则将数据变为0x22CC4488，再进行计算 相关函数 */
extern void CRC_CR_RFLTIN_Set(uint32_t SetValue);
extern uint32_t CRC_CR_RFLTIN_Get(void);

/* CRC输出反转控制 (Reflected Output)
0：输入不反转
1：输入按字节反转
例如：
如果RFLTO==1，若当前计算的CRC结果为0x1234，则输出的结果为0x2C48如果 RFLTO==0，则直接输出0x1234注意：此结果不一定为最终输出结果，还需要看XOR是否为1，详见本寄存器bit2说明 相关函数 */
extern void CRC_CR_RFLTO_Set(uint32_t SetValue);
extern uint32_t CRC_CR_RFLTO_Get(void);

/* CRC结果标志位，只读 (Result Flag)
0：CRC结果为0
1：CRC结果非全0 相关函数 */
extern FlagStatus CRC_CR_RES_Chk(void);

/* CRC运算标志位，只读 (Busy)
0：CRC运算结束
1：CRC运算进行中 相关函数 */
extern FlagStatus CRC_CR_BUSY_Chk(void);

/* 输出异或使能 (Output XORed with CRC_XOR register enable)
0：输出不异或CRC_XOR寄存器
1：输出异或CRC_XOR寄存器 相关函数 */
extern void CRC_CR_XOR_Setable(FunState NewState);
extern FunState CRC_CR_XOR_Getable(void);

/* CRC多项式位宽选择 (Polynomial width Selection)
00：32位
01：16位
10：8位
11：7位 相关函数 */
extern void CRC_CR_SEL_Set(uint32_t SetValue);
extern uint32_t CRC_CR_SEL_Get(void);

/* CRC线性反馈移位寄存器 (Linear Feedback Shift Register)
运算开始前可以由软件写入CRC初始值 相关函数 */
extern void CRC_LFSR_Write(uint32_t SetValue);
extern uint32_t CRC_LFSR_Read(void);

/* CRC运算结果异或寄存器 (eXclusive OR)
当CRC_CR.XOR为1时，CRC结果输出前将异或此寄存器的数据。 相关函数 */
extern void CRC_XOR_Write(uint32_t SetValue);
extern uint32_t CRC_XOR_Read(void);

/* CRC运算多项式系数 (CRC Polynominals) 相关函数 */
extern void CRC_POLY_Write(uint32_t SetValue);
extern uint32_t CRC_POLY_Read(void);
//Announce_End
extern void CRC_Init(CRC_InitTypeDef* para);
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_CRC_H */
