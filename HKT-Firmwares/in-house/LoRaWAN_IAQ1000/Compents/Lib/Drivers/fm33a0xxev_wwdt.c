/**
  ******************************************************************************
  * @file    fm33a0xxev_wwdt.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_wwdt.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup WWDT 
  * @brief WWDT driver modules
  * @{
  */ 

/* 当CPU向此地址写入0x5A时启动WWDT定时器 (WWDT Control,write only)
在启动WWDT后，当CPU向此地址写入0xAC时清零计数器 相关函数 */
void WWDT_CR_Write(uint32_t SetValue)
{
	WWDT->CR = (SetValue & WWDT_CR_CON_Msk);
}

/* 配置WWDT看门狗溢出时间，复位值011，由于上电后系统时钟默认为8Mhz，所以默认溢出周期大约32ms (WWDT Config)
000：TPCLK * 4096 * 1
001：TPCLK * 4096 * 4
010：TPCLK * 4096 * 16
011：TPCLK * 4096 * 64
100：TPCLK * 4096 * 128
101：TPCLK * 4096 * 256
110：TPCLK * 4096 * 512
111：TPCLK * 4096 * 1024 相关函数 */
void WWDT_CFGR_Write(uint32_t SetValue)
{
	WWDT->CFGR = (SetValue & WWDT_CFGR_CFG_Msk);
}

uint32_t WWDT_CFGR_Read(void)
{
	return (WWDT->CFGR & WWDT_CFGR_CFG_Msk);
}

/* WWDT计数寄存器值，软件可通过查询此寄存器了解WWDT计时进度 (WWDT Counter value,read only) 相关函数 */
uint32_t WWDT_CNTR_Read(void)
{
	return (WWDT->CNTR & WWDT_CNTR_CNT_Msk);
}

/* WWDT中断使能 (WWDT Interrupt Enable)
0：中断使能禁止
1：中断使能打开 相关函数 */
void WWDT_IER_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		WWDT->IER |= (WWDT_IER_IE_Msk);
	}
	else
	{
		WWDT->IER &= ~(WWDT_IER_IE_Msk);
	}
}

FunState WWDT_IER_IE_Getable(void)
{
	if (WWDT->IER & (WWDT_IER_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* WWDT 75%计时中断标志，写1清零 (Near Overflow Flag,write 1 to clear)
0：无中断产生
1：中断标志置位
如果IE=1，则此寄存器置位将触发中断 相关函数 */
void WWDT_ISR_NOVF_Clr(void)
{
	WWDT->ISR = WWDT_ISR_NOVF_Msk;
}

FlagStatus WWDT_ISR_NOVF_Chk(void)
{
	if (WWDT->ISR & WWDT_ISR_NOVF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* WWDT的4096预分频计数器当前计数值，只读 (WWDT prescaler Divider Counte,read only) 相关函数 */
uint32_t WWDT_PSCR_Read(void)
{
	return (WWDT->PSCR & WWDT_PSCR_DIV_CNT_Msk);
}


void WWDT_Deinit(void)
{
	//WWDT->CR = 0x00000000;
	//WWDT->CFGR = 0x00000003;
	//WWDT->CNTR = 0x00000000;
	//WWDT->IER = 0x00000000;
	//WWDT->ISR = 0x00000000;
	//WWDT->PSCR = 0x00000000;
}

/******END OF FILE****/
