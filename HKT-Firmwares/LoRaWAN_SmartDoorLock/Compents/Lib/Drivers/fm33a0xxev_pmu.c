/**
  ******************************************************************************
  * @file    fm33a0xxev_pmu.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_pmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup PMU 
  * @brief PMU driver modules
  * @{
  */ 

/* LDO15使能标志位
1：LDO15处于工作状态
0：LDO15被关闭 相关函数 */
FlagStatus PMU_CR_LDO15EN_Chk(void)
{
	if (PMU->CR & PMU_CR_LDO15EN_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LDO15使能标志反码校验位 相关函数 */
FlagStatus PMU_CR_LDO15EN_B_Chk(void)
{
	if (PMU->CR & PMU_CR_LDO15EN_B_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Sleep/DeepSleep唤醒后的系统频率
00：RCHF-8MHz
01：RCHF-16MHz
10：RCHF-24MHz
11：RCHF-32MHz 相关函数 */
void PMU_CR_WKFSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->CR;
	tmpreg &= ~(PMU_CR_WKFSEL_Msk);
	tmpreg |= (SetValue & PMU_CR_WKFSEL_Msk);
	PMU->CR = tmpreg;
}

uint32_t PMU_CR_WKFSEL_Get(void)
{
	return (PMU->CR & PMU_CR_WKFSEL_Msk);
}

/* DeepSleep控制寄存器
1：DeepSleep模式使能，下关闭基准电压源
0：常规Sleep模式
在Sleep下，如果置位了SLPDP位即为DeepSleep模式；
该位仅在Sleep下有效 相关函数 */
void PMU_CR_SLPDP_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->CR;
	tmpreg &= ~(PMU_CR_SLPDP_Msk);
	tmpreg |= (SetValue & PMU_CR_SLPDP_Msk);
	PMU->CR = tmpreg;
}

uint32_t PMU_CR_SLPDP_Get(void)
{
	return (PMU->CR & PMU_CR_SLPDP_Msk);
}

/* CoreVoltageScaling配置
0：低功耗模式下不使能内核电压调整
1：低功耗模式下降低内核电压
该位仅在Sleep/DeepSleep/RTCBKP模式下起作用 相关函数 */
void PMU_CR_CVS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->CR;
	tmpreg &= ~(PMU_CR_CVS_Msk);
	tmpreg |= (SetValue & PMU_CR_CVS_Msk);
	PMU->CR = tmpreg;
}

uint32_t PMU_CR_CVS_Get(void)
{
	return (PMU->CR & PMU_CR_CVS_Msk);
}

/* 低功耗模式配置寄存器
00：Active mode / LP Active mode
01：LPRUN mode
10：Sleep mode / DeepSleep mode
11：RTCBKP mode 相关函数 */
void PMU_CR_PMOD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->CR;
	tmpreg &= ~(PMU_CR_PMOD_Msk);
	tmpreg |= (SetValue & PMU_CR_PMOD_Msk);
	PMU->CR = tmpreg;
}

uint32_t PMU_CR_PMOD_Get(void)
{
	return (PMU->CR & PMU_CR_PMOD_Msk);
}

/* Flash Stop唤醒控制
0：Stop信号等待时钟建立后同步清零
1：Stop信号异步清零 相关函数 */
void PMU_WKTR_STPCLR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->WKTR;
	tmpreg &= ~(PMU_WKTR_STPCLR_Msk);
	tmpreg |= (SetValue & PMU_WKTR_STPCLR_Msk);
	PMU->WKTR = tmpreg;
}

uint32_t PMU_WKTR_STPCLR_Get(void)
{
	return (PMU->WKTR & PMU_WKTR_STPCLR_Msk);
}

/* 可编程额外唤醒延迟
在DeepSleep模式下，RCHF时钟到来后，根据此寄存器配置等待额外延迟时间后，再读取Flash校验字
00：0us
01：2us
10：4us
11：8us 相关函数 */
void PMU_WKTR_T1A_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PMU->WKTR;
	tmpreg &= ~(PMU_WKTR_T1A_Msk);
	tmpreg |= (SetValue & PMU_WKTR_T1A_Msk);
	PMU->WKTR = tmpreg;
}

uint32_t PMU_WKTR_T1A_Get(void)
{
	return (PMU->WKTR & PMU_WKTR_T1A_Msk);
}

/* ADC中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_ADCWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_ADCWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* RTC中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_RTCWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_RTCWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SVD中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_SVDWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_SVDWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 32768Hz晶体停振中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_LFDETWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_LFDETWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* IO中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_IOWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_IOWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LPUART1中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_LPU1WKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_LPU1WKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LPUART0中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_LPU0WKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_LPU0WKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 比较器out-of-window唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_COMP_OOWF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_COMP_OOWF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 比较器window唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_COMP_WINF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_COMP_WINF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 比较器2中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_COMP2WKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_COMP2WKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 比较器1中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_COMP1WKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_COMP1WKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/*  _32中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_LPTWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_LPTWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* BSTIM32中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
FlagStatus PMU_WKFR_BSTWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_BSTWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* CPU Debugger唤醒标志，软件写1清零 相关函数 */
void PMU_WKFR_DBGWKF_Clr(void)
{
	PMU->WKFR = PMU_WKFR_DBGWKF_Msk;
}

FlagStatus PMU_WKFR_DBGWKF_Chk(void)
{
	if (PMU->WKFR & PMU_WKFR_DBGWKF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


/* SLEEP错误中断使能 (Sleep mode Error Interrupt Enable)
1：使能SLEEP错误中断
0：禁止SLEEP错误中断 相关函数 */
void PMU_IER_SLPEIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		PMU->IER |= (PMU_IER_SLPEIE_Msk);
	}
	else
	{
		PMU->IER &= ~(PMU_IER_SLPEIE_Msk);
	}
}

FunState PMU_IER_SLPEIE_Getable(void)
{
	if (PMU->IER & (PMU_IER_SLPEIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RTCBKP错误中断使能(RTCBKP mode Error Interrupt Enable)
1：使能RTCBKP错误中断
0：禁止RTCBKP错误中断 相关函数 */
void PMU_IER_RTCEIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		PMU->IER |= (PMU_IER_RTCEIE_Msk);
	}
	else
	{
		PMU->IER &= ~(PMU_IER_RTCEIE_Msk);
	}
}

FunState PMU_IER_RTCEIE_Getable(void)
{
	if (PMU->IER & (PMU_IER_RTCEIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SLEEP错误中断标志，硬件置位，软件写1清零 (Sleep Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h2后，CPU执行WFI/WFE指令前置位了SLEEPDEEP寄存器时置位
0：在PMOD=2’h2后，CPU正确进入SLEEP 相关函数 */
void PMU_ISR_SLPEIF_Clr(void)
{
	PMU->ISR = PMU_ISR_SLPEIF_Msk;
}

FlagStatus PMU_ISR_SLPEIF_Chk(void)
{
	if (PMU->ISR & PMU_ISR_SLPEIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* RTCBKP错误中断标志，硬件置位，软件写1清零(RTC Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h3后，未改写CPU内部寄存器SLEEPDEEP=1，然后执行WFI/WFE指令；或者系统时钟来自USB PHY，试图进入RTCBKP模式
0：在PMOD=2’h3后，CPU自身正确进入DEEP SLEEP 相关函数 */
void PMU_ISR_RTCEIF_Clr(void)
{
	PMU->ISR = PMU_ISR_RTCEIF_Msk;
}

FlagStatus PMU_ISR_RTCEIF_Chk(void)
{
	if (PMU->ISR & PMU_ISR_RTCEIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void PMU_Deinit(void)
{
	//PMU->CR = 0x00020000;
	//PMU->WKTR = 0x00000001;
	//PMU->WKFR = 0x00000000;
	//PMU->IER = ;
	//PMU->ISR = ;
}

/********************************
清除NWKUPx Pin唤醒标志函数
功能:清除NWKUPx Pin唤醒标志，仅Sleep/DeepSleep模式下有效。
输入：设置NWKUPx Pin唤醒标志的设置值，写1.
 ********************************/
void PMU_WKFR_WKPxF_Clr(uint32_t NWKPinDef)
{
	PMU->WKFR = NWKPinDef;
}
/********************************
读取NWKUPx Pin唤醒标志函数
功能:读取NWKUPx Pin唤醒标志。
输出：NWKUPx Pin唤醒标志的值
 ********************************/
FlagStatus PMU_WKFR_WKPxF_Chk(uint32_t NWKPinDef)
{
	if (PMU->WKFR & NWKPinDef)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/********************************
低功耗模式的初始化函数
功能:低功耗模式下各种设置，SLEEP模式，RAM保持设置，内核电压，晶振电路等。
输入：低功耗功能开启前的设置.
 ********************************/
void PMU_SleepCfg_Init(PMU_SleepCfg_InitTypeDef* SleepCfg_InitStruct)
{
	PMU_CR_SLPDP_Set(SleepCfg_InitStruct->SLPDP);				/*!<DeepSleep控制寄存器  */	
	PMU_CR_CVS_Set(SleepCfg_InitStruct->CVS);				/*!<内核电压降低使能控制  */	
	if(SleepCfg_InitStruct->SLPDP == PMU_CR_SLPDP_DEEPSLEEP)
	{
		PMU_WKTR_T1A_Set(SleepCfg_InitStruct->TIA);
	}		
	SCB->SCR = SleepCfg_InitStruct->SCR;							/*!<M0系统控制寄存器，一般配置为0即可  */
	PMU_CR_PMOD_Set(SleepCfg_InitStruct->PMOD);					/*!<低功耗模式配置  */

}

/******END OF FILE****/
