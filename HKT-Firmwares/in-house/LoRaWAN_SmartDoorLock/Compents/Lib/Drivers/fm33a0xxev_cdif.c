/**
  ******************************************************************************
  * @file    fm33a0xxev_cdif.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_cdif.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup CDIF 
  * @brief CDIF driver modules
  * @{
  */ 


/* 跨电源域接口使能 相关函数 */
void CDIF_CR_INTF_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CDIF->CR = CDIF_CR_INTF_EN_ENABLE;
	}
	else
	{
		CDIF->CR = CDIF_CR_INTF_EN_DISABLE;
	}
}

FunState CDIF_CR_INTF_EN_Getable(void)
{
	if ((CDIF->CR & (CDIF_CR_INTF_EN_Msk)) == CDIF_CR_INTF_EN_ENABLE)
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 跨电源域时序控制，配置相对于APBCLK的定时比例 相关函数 */
void CDIF_PSCR_Write(uint32_t SetValue)
{
	CDIF->PSCR = (SetValue & CDIF_PSCR_PRSC_Msk);
}

uint32_t CDIF_PSCR_Read(void)
{
	return (CDIF->PSCR & CDIF_PSCR_PRSC_Msk);
}


void CDIF_Deinit(void)
{
	//CDIF->CR = 0x00000101;
	//CDIF->PSCR = 0x00000000;
}

/******END OF FILE****/

