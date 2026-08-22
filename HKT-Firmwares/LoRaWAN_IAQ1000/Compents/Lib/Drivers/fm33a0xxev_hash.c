/**
  ******************************************************************************
  * @file    fm33a0xxev_hash.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_hash.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup HASH 
  * @brief HASH driver modules
  * @{
  */ 

/* HASH运算启动寄存器，软件写1启动运算，运算结束后硬件自动清零 相关函数 */
void HASH_CSR_START_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		HASH->CSR |= (HASH_CSR_START_Msk);
	}
	else
	{
		HASH->CSR &= ~(HASH_CSR_START_Msk);
	}
}

FunState HASH_CSR_START_Getable(void)
{
	if (HASH->CSR & (HASH_CSR_START_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SHA算法选择 相关函数 */
void HASH_CSR_HASHSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = HASH->CSR;
	tmpreg &= ~(HASH_CSR_HASHSEL_Msk);
	tmpreg |= (SetValue & HASH_CSR_HASHSEL_Msk);
	HASH->CSR = tmpreg;
}

uint32_t HASH_CSR_HASHSEL_Get(void)
{
	return (HASH->CSR & HASH_CSR_HASHSEL_Msk);
}

/* 输入或输出数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48
 相关函数 */
void HASH_DTR_DATA_TYPE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = HASH->DTR;
	tmpreg &= ~(HASH_DTR_DATA_TYPE_Msk);
	tmpreg |= (SetValue & HASH_DTR_DATA_TYPE_Msk);
	HASH->DTR = tmpreg;
}

uint32_t HASH_DTR_DATA_TYPE_Get(void)
{
	return (HASH->DTR & HASH_DTR_DATA_TYPE_Msk);
}


void HASH_Deinit(void)
{
	//HASH->CSR = 0x00000000;
	//HASH->DTR = 0x00000000;
}

/******END OF FILE****/
