/**
  ******************************************************************************
  * @file    fm33a0xxev_rng.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_rng.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup RNG 
  * @brief RNG driver modules
  * @{
  */ 

/* RNG使能寄存器，软件写1启动 (RNG enable)
1：启动RNG
0：关闭RNG 相关函数 */
void RNG_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RNG->CR |= (RNG_CR_EN_Msk);
	}
	else
	{
		RNG->CR &= ~(RNG_CR_EN_Msk);
	}
}

FunState RNG_CR_EN_Getable(void)
{
	if (RNG->CR & (RNG_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 随机数生成结果或CRC运算结果寄存器，只读
(RNG output) 相关函数 */
uint32_t RNG_DOR_Read(void)
{
	return (RNG->DOR);
}

/* LFSR状态标志，只读 (LFSR enable)
1：LFSR在运行中，不可进行CRC验证
0：LFSR不在运行中，可进行CRC验证
注：本寄存器不会引起模块中断，仅供查询 相关函数 */
FlagStatus RNG_SR_LFSREN_Chk(void)
{
	if (RNG->SR & (RNG_SR_LFSREN_Msk))
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 随机数生成失败标志，软件写1清零 (Random Number Fail)
1：随机数未能通过质量检测
0：随机数通过质量检测 相关函数 */
void RNG_SR_RNF_Clr(void)
{
	RNG->SR = RNG_SR_RNF_Msk;
}

FlagStatus RNG_SR_RNF_Chk(void)
{
	if (RNG->SR & RNG_SR_RNF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* CRC使能控制寄存器，软件写1启动CRC，运算完成后硬件自动清零 (CRC enable)
1：CRC使能
0：CRC关闭 相关函数 */
void RNG_CRC_CR_CRCEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RNG->CRC_CR |= (RNG_CRC_CR_CRCEN_Msk);
	}
	else
	{
		RNG->CRC_CR &= ~(RNG_CRC_CR_CRCEN_Msk);
	}
}

FunState RNG_CRC_CR_CRCEN_Getable(void)
{
	if (RNG->CRC_CR & (RNG_CRC_CR_CRCEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CRC运算数据输入寄存器 (CRC data input) 相关函数 */
void RNG_CRC_DIR_Write(uint32_t SetValue)
{
	RNG->CRC_DIR = (SetValue);
}

uint32_t RNG_CRC_DIR_Read(void)
{
	return (RNG->CRC_DIR);
}

/* CRC计算完成标志，软件写0清零 (CRC calculation done)
1：CRC计算完成
0：CRC计算未完成 相关函数 */
void RNG_CRC_SR_CRCDONE_Clr(void)
{
	RNG->CRC_SR &= ~(RNG_CRC_SR_CRCDONE_Msk);
}

FlagStatus RNG_CRC_SR_CRCDONE_Chk(void)
{
	if (RNG->CRC_SR & RNG_CRC_SR_CRCDONE_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void RNG_Deinit(void)
{
	//RNG->CR = 0x00000000;
	//RNG->DOR = 0x00000000;
	//RNG->SR = 0x00000000;
	//RNG->CRC_CR = 0x00000000;
	//RNG->CRC_DIR = 0xFFFFFFFF;
	//RNG->CRC_SR = 0x00000000;
}

/******END OF FILE****/
