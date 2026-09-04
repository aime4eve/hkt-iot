/**
  ******************************************************************************
  * @file    fm33a0xxev_crc.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_crc.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup CRC 
  * @brief CRC driver modules
  * @{
  */ 

/* CRCDR用于作为数据输入寄存器，并且在运算结束后保存CRC计算结果。(CRC Data Register)
作为输入时：若word操作使能，则对CRCDR[31:0]进行计算，共4次byte运算（由低到高）；否则对CRCDR[7:0]进行计算，共1次byte运算。
保存结果时：若为7位多项式结果保存在CRCDR[6:0]，若为8位多项式结果保存在CRCDR[7:0]，若为16位多项式结果保存在CRCDR[15:0]，若为32位多项式结果保存在CRCDR[31:0]。 相关函数 */
void CRC_DR_Write(uint32_t SetValue)
{
	CRC->DR = (SetValue);
}

uint32_t CRC_DR_Read(void)
{ 
	return (CRC->DR);
}

/* WORD操作使能 (Operation by Word)
0：字节操作，CRC计算仅针对CRCDR最低字节进行
1：字操作，CRC计算针对CRCDR全部4字节进行 相关函数 */
void CRC_CR_OPWD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CRC->CR;
	tmpreg &= ~(CRC_CR_OPWD_Msk);
	tmpreg |= (SetValue & CRC_CR_OPWD_Msk);
	CRC->CR = tmpreg;
}

uint32_t CRC_CR_OPWD_Get(void)
{
	return (CRC->CR & CRC_CR_OPWD_Msk);
}

/* CRC快速计算使能 (Parallel Calculation)
0：串行运算，计算1个字节需要8个时钟周期
1：并行计算，计算1个字节需要1个时钟周期 相关函数 */
void CRC_CR_PARA_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CRC->CR;
	tmpreg &= ~(CRC_CR_PARA_Msk);
	tmpreg |= (SetValue & CRC_CR_PARA_Msk);
	CRC->CR = tmpreg;
}

uint32_t CRC_CR_PARA_Get(void)
{
	return (CRC->CR & CRC_CR_PARA_Msk);
}

/* CRC输入反转控制 (Reflected Input)
00：输入不反转
01：输入按字节反转
10：输入按半字反转
11：输入按字反转
例如：计算数据为0x11223344，
如果RFLTIN==01，则将数据变为0x8844CC22，再进行计算如果RFLTIN==10，则将数据变为0x448822CC，再进行计算
如果RFLTIN==11，则将数据变为0x22CC4488，再进行计算 相关函数 */
void CRC_CR_RFLTIN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CRC->CR;
	tmpreg &= ~(CRC_CR_RFLTIN_Msk);
	tmpreg |= (SetValue & CRC_CR_RFLTIN_Msk);
	CRC->CR = tmpreg;
}

uint32_t CRC_CR_RFLTIN_Get(void)
{
	return (CRC->CR & CRC_CR_RFLTIN_Msk);
}

/* CRC输出反转控制 (Reflected Output)
0：输入不反转
1：输入按字节反转
例如：
如果RFLTO==1，若当前计算的CRC结果为0x1234，则输出的结果为0x2C48如果 RFLTO==0，则直接输出0x1234注意：此结果不一定为最终输出结果，还需要看XOR是否为1，详见本寄存器bit2说明 相关函数 */
void CRC_CR_RFLTO_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CRC->CR;
	tmpreg &= ~(CRC_CR_RFLTO_Msk);
	tmpreg |= (SetValue & CRC_CR_RFLTO_Msk);
	CRC->CR = tmpreg;
}

uint32_t CRC_CR_RFLTO_Get(void)
{
	return (CRC->CR & CRC_CR_RFLTO_Msk);
}

/* CRC结果标志位，只读 (Result Flag)
0：CRC结果为0
1：CRC结果非全0 相关函数 */
FlagStatus CRC_CR_RES_Chk(void)
{
	if (CRC->CR & CRC_CR_RES_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* CRC运算标志位，只读 (Busy)
0：CRC运算结束
1：CRC运算进行中 相关函数 */
FlagStatus CRC_CR_BUSY_Chk(void)
{
	if (CRC->CR & CRC_CR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 输出异或使能 (Output XORed with CRC_XOR register enable)
0：输出不异或CRC_XOR寄存器
1：输出异或CRC_XOR寄存器 相关函数 */
void CRC_CR_XOR_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CRC->CR |= (CRC_CR_XOR_Msk);
	}
	else
	{
		CRC->CR &= ~(CRC_CR_XOR_Msk);
	}
}

FunState CRC_CR_XOR_Getable(void)
{
	if (CRC->CR & (CRC_CR_XOR_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CRC多项式位宽选择 (Polynomial width Selection)
00：32位
01：16位
10：8位
11：7位 相关函数 */
void CRC_CR_SEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CRC->CR;
	tmpreg &= ~(CRC_CR_SEL_Msk);
	tmpreg |= (SetValue & CRC_CR_SEL_Msk);
	CRC->CR = tmpreg;
}

uint32_t CRC_CR_SEL_Get(void)
{
	return (CRC->CR & CRC_CR_SEL_Msk);
}

/* CRC线性反馈移位寄存器 (Linear Feedback Shift Register)
运算开始前可以由软件写入CRC初始值 相关函数 */
void CRC_LFSR_Write(uint32_t SetValue)
{
	CRC->LFSR = (SetValue);
}

uint32_t CRC_LFSR_Read(void)
{
	return (CRC->LFSR);
}

/* CRC运算结果异或寄存器 (eXclusive OR)
当CRC_CR.XOR为1时，CRC结果输出前将异或此寄存器的数据。 相关函数 */
void CRC_XOR_Write(uint32_t SetValue)
{
	CRC->XOR = (SetValue);
}

uint32_t CRC_XOR_Read(void)
{
	return (CRC->XOR);
}

/* CRC运算多项式系数 (CRC Polynominals) 相关函数 */
void CRC_POLY_Write(uint32_t SetValue)
{
	CRC->POLY = (SetValue);
}

uint32_t CRC_POLY_Read(void)
{
	return (CRC->POLY);
}


void CRC_Deinit(void)
{
	//CRC->DR = 0xFFFFFFFF;
	//CRC->CR = 0x00000002;
	//CRC->LFSR = 0xFFFFFFFF;
	//CRC->XOR = 0x00000000;
	//CRC->POLY = 0x00000000;
}
//Code_End

/* CRC初始化配置函数*/
void CRC_Init(CRC_InitTypeDef* para)
{
	CRC_LFSR_Write(para->LFSR);				/*!<初值寄存器*/
	CRC_CR_SEL_Set(para->CRCSEL);			/*!<CRC校验多项式位宽选择*/
	CRC_CR_OPWD_Set(para->OPWD);			/*!<WORD操作使能*/
	CRC_CR_PARA_Set(para->PARA);			/*!<CRC快速计算使能*/
	CRC_CR_RFLTIN_Set(para->RFLTIN);	/*!<CRC输入反转控制*/
	CRC_CR_RFLTO_Set(para->RFLTO);		/*!<CRC输出反转控制*/
	CRC_CR_XOR_Setable(para->XOR);		/*!<输出异或使能*/
	CRC_XOR_Write(para->CRC_XOR);			/*!<运算结果异或寄存器*/
	CRC_POLY_Write(para->CRCPOLY);		/*!<CRC多项式寄存器*/
}

/******END OF FILE****/
