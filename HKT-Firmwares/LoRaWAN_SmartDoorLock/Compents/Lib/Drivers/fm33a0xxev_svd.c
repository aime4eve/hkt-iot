/**
  ******************************************************************************
  * @file    fm33a0xxev_svd.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_svd.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup SVD 
  * @brief SVD driver modules
  * @{
  */ 
 
/* SVD报警阈值档位设置，参见11.3.3检测阈值
(SVD threshold level) 相关函数 */
void SVD_CFGR_LVL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SVD->CFGR;
	tmpreg &= ~(SVD_CFGR_LVL_Msk);
	tmpreg |= (SetValue & SVD_CFGR_LVL_Msk);
	SVD->CFGR = tmpreg;
}

uint32_t SVD_CFGR_LVL_Get(void)
{
	return (SVD->CFGR & SVD_CFGR_LVL_Msk);
}

/* 数字滤波使能（MOD=1时必须置1）
(Digital Filter Enable)
1：启动SVD输出的数字滤波
0：关闭SVD输出的数字滤波 相关函数 */
void SVD_CFGR_DFEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->CFGR |= (SVD_CFGR_DFEN_Msk);
	}
	else
	{
		SVD->CFGR &= ~(SVD_CFGR_DFEN_Msk);
	}
}

FunState SVD_CFGR_DFEN_Getable(void)
{
	if (SVD->CFGR & (SVD_CFGR_DFEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SVD工作模式选择，配置模式后还要置位SVD_CR.EN才会启动SVD
(SVD Mode)
1：间歇使能模式
0：常使能模式
注意：间歇使能模式下必须开启数字滤波 相关函数 */
void SVD_CFGR_MOD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SVD->CFGR;
	tmpreg &= ~(SVD_CFGR_MOD_Msk);
	tmpreg |= (SetValue & SVD_CFGR_MOD_Msk);
	SVD->CFGR = tmpreg;
}

uint32_t SVD_CFGR_MOD_Get(void)
{
	return (SVD->CFGR & SVD_CFGR_MOD_Msk);
}

/* SVD Interval，SVD间歇使能间隔 (SVD periodic interval)
00：62.5ms
01：256ms
10：1s
11：4s 相关函数 */
void SVD_CFGR_ITVL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SVD->CFGR;
	tmpreg &= ~(SVD_CFGR_ITVL_Msk);
	tmpreg |= (SetValue & SVD_CFGR_ITVL_Msk);
	SVD->CFGR = tmpreg;
}

uint32_t SVD_CFGR_ITVL_Get(void)
{
	return (SVD->CFGR & SVD_CFGR_ITVL_Msk);
}

/* SVD测试使能，避免写1 (SVD test enable) 相关函数 */
void SVD_CR_TE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->CR |= (SVD_CR_TE_Msk);
	}
	else
	{
		SVD->CR &= ~(SVD_CR_TE_Msk);
	}
}

FunState SVD_CR_TE_Getable(void)
{
	if (SVD->CR & (SVD_CR_TE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


/* SVS外部电源检测通道控制信号 
(SVS external monitor channel enable)
0：SVS通道关闭
1：SVS通道使能
当EN=1时，根据SVDLVL寄存器可以设置SVS输入后是否经过内部电阻分压；如果LVL=1111，则SVS输入不做分压，如果LVL != 1111，则SVS输入经过内部电阻分压。 相关函数 */
void SVD_CR_SVSEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->CR |= (SVD_CR_SVSEN_Msk);
	}
	else
	{
		SVD->CR &= ~(SVD_CR_SVSEN_Msk);
	}
}

FunState SVD_CR_SVSEN_Getable(void)
{
	if (SVD->CR & (SVD_CR_SVSEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void SVD_CR_SVDEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->CR |= (SVD_CR_SVDEN_Msk);
	}
	else
	{
		SVD->CR &= ~(SVD_CR_SVDEN_Msk);
	}
}

FunState SVD_CR_SVDEN_Getable(void)
{
	if (SVD->CR & (SVD_CR_SVDEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 电源跌落中断使能寄存器，1有效 (Power Fall interrupt enable) 相关函数 */
void SVD_IER_PFIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->IER |= (SVD_IER_PFIE_Msk);
	}
	else
	{
		SVD->IER &= ~(SVD_IER_PFIE_Msk);
	}
}

FunState SVD_IER_PFIE_Getable(void)
{
	if (SVD->IER & (SVD_IER_PFIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 电源恢复中断使能寄存器，1有效 (Power Rise interrupt enable) 相关函数 */
void SVD_IER_PRIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->IER |= (SVD_IER_PRIE_Msk);
	}
	else
	{
		SVD->IER &= ~(SVD_IER_PRIE_Msk);
	}
}

FunState SVD_IER_PRIE_Getable(void)
{
	if (SVD->IER & (SVD_IER_PRIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SVD电源检测输出 (SVD output)
1：电源电压高于SVD当前阈值
0：电源电压低于SVD当前阈值 相关函数 */
FlagStatus SVD_ISR_SVDO_Chk(void)
{
	if (SVD->ISR & SVD_ISR_SVDO_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SVD输出锁存信号，数字电路锁存的SVD状态
(SVD registered output) 相关函数 */
FlagStatus SVD_ISR_SVDR_Chk(void)
{
	if (SVD->ISR & SVD_ISR_SVDR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 电源跌落中断标志寄存器，电源电压跌落到SVD阈值之下时置位，软件写1清零 (Power fall flag) 相关函数 */
void SVD_ISR_PFF_Clr(void)
{
	SVD->ISR = SVD_ISR_PFF_Msk;
}

FlagStatus SVD_ISR_PFF_Chk(void)
{
	if (SVD->ISR & SVD_ISR_PFF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 电源恢复中断标志寄存器，电源电压上升到SVD阈值之上时置位，软件写1清零 (Power rise flag) 相关函数 */
void SVD_ISR_PRF_Clr(void)
{
	SVD->ISR = SVD_ISR_PRF_Msk;
}

FlagStatus SVD_ISR_PRF_Chk(void)
{
	if (SVD->ISR & SVD_ISR_PRF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 1.0V基准输入使能信号 (1.0V reference enable)
1：使能1.0V基准输入
0：关闭1.0V基准输入 相关函数 */
void SVD_VSR_V1P0EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->VSR |= (SVD_VSR_V1P0EN_Msk);
	}
	else
	{
		SVD->VSR &= ~(SVD_VSR_V1P0EN_Msk);
	}
}

FunState SVD_VSR_V1P0EN_Getable(void)
{
	if (SVD->VSR & (SVD_VSR_V1P0EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 0.95V基准输入使能信号 (0.95V reference enable)
1：使能0.95V基准输入
0：关闭0.95V基准输入 相关函数 */
void SVD_VSR_V0P95EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->VSR |= (SVD_VSR_V0P95EN_Msk);
	}
	else
	{
		SVD->VSR &= ~(SVD_VSR_V0P95EN_Msk);
	}
}

FunState SVD_VSR_V0P95EN_Getable(void)
{
	if (SVD->VSR & (SVD_VSR_V0P95EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 0.9V基准输入使能信号 (0.9V reference enable)
1：使能0.9V基准输入
0：关闭0.9V基准输入 相关函数 */
void SVD_VSR_V0P9EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		SVD->VSR |= (SVD_VSR_V0P9EN_Msk);
	}
	else
	{
		SVD->VSR &= ~(SVD_VSR_V0P9EN_Msk);
	}
}

FunState SVD_VSR_V0P9EN_Getable(void)
{
	if (SVD->VSR & (SVD_VSR_V0P9EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


void SVD_Deinit(void)
{
	//SVD->CFGR = 0x00000008;
	//SVD->CR = 0x00000000;
	//SVD->IER = 0x00000000;
	//SVD->ISR = 0x00000000;
	//SVD->VSR = 0x00000000;
}

/*********************************
SVD初始化配置函数 
功能：SVD初始化配置
输入: ANAC_SVD_InitTypeDef* para，结构体函数中配置工作模式、阈值、数字滤波等功能
输出: 无
*********************************/
void SVD_Init(SVD_InitTypeDef* para)
{
	SVD_CFGR_MOD_Set(para->SVDMOD);	//SVD工作模式选择
	SVD_CFGR_ITVL_Set(para->SVDITVL);	//SVD间歇使能间隔设置
	SVD_CFGR_LVL_Set(para->SVDLVL);	//SVD报警阈值设置
	SVD_CFGR_DFEN_Setable(para->DFEN);	//SVD数字滤波设置（SVDMODE=1时必须置1）
	SVD_IER_PFIE_Setable(para->PFIE);	//SVD电源跌落中断设置
	SVD_IER_PRIE_Setable(para->PRIE);	//SVD电源恢复中断设置
	SVD_CR_TE_Setable(DISABLE);	//关闭测试功能	
}	

/******END OF FILE****/
