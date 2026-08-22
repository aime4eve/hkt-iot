/**
  ******************************************************************************
  * @file    fm33a0xxev_lptim.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_lptim.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup LPTIM 
  * @brief LPTIM driver modules
  * @{
  */ 

/* LPT_ETR输入模拟滤波使能(External Trigger input Analog Filter Enable)
0：关闭模拟滤波
1：使能模拟滤波，滤波宽度约100ns 相关函数 */
void LPTIM_CFGR_ETR_AFEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->CFGR |= (LPTIM_CFGR_ETR_AFEN_Msk);
	}
	else
	{
		LPTIM->CFGR &= ~(LPTIM_CFGR_ETR_AFEN_Msk);
	}
}

FunState LPTIM_CFGR_ETR_AFEN_Getable(void)
{
	if (LPTIM->CFGR & (LPTIM_CFGR_ETR_AFEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 时钟预分频输入选择(Prescaler input Select)
0：CLKSEL选择的时钟
1：LPTETRF 相关函数 */
void LPTIM_CFGR_PSCSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_PSCSEL_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_PSCSEL_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_PSCSEL_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_PSCSEL_Msk);
}

/* 计数时钟分频选择(Counter Clock Divider Select)
000：1分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110：64分频
111：128分频 相关函数 */
void LPTIM_CFGR_DIVSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_DIVSEL_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_DIVSEL_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_DIVSEL_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_DIVSEL_Msk);
}

/* ETR输入边沿选择(ETR Clock Edge Select)
0：LPT_ETR的上升沿计数
1：LPT_ETR的下降沿计数 相关函数 */
void LPTIM_CFGR_EDGESEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_EDGESEL_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_EDGESEL_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_EDGESEL_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_EDGESEL_Msk);
}

/* 外部触发边沿选择（需使用内部时钟同步采样LPT_ETR）(ETR trigger Configuration)
00: LPT_ETR输入信号上升沿触发
01: LPT_ETR输入信号下降沿触发
10/11: 外部输入信号上升下降沿触发 相关函数 */
void LPTIM_CFGR_TRIGCFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_TRIGCFG_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_TRIGCFG_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_TRIGCFG_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_TRIGCFG_Msk);
}

/* 单次计数模式使能(One State Timer)
0：连续计数模式：计数器被触发后保持运行，直到被关闭为止。计数器达到目标值后回到0重新开始计数，并产生溢出中断。
1：单次计数模式：计数器被触发后计数到目标值后回到0，并自动停止，产生溢出中断。 相关函数 */
void LPTIM_CFGR_ONST_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_ONST_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_ONST_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_ONST_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_ONST_Msk);
}

/* 工作模式选择(Timer operation Mode)
00：普通定时器模式
01：Trigger脉冲触发计数模式
10：外部异步脉冲计数模式
11：Timeout模式 相关函数 */
void LPTIM_CFGR_TMODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CFGR;
	tmpreg &= ~(LPTIM_CFGR_TMODE_Msk);
	tmpreg |= (SetValue & LPTIM_CFGR_TMODE_Msk);
	LPTIM->CFGR = tmpreg;
}

uint32_t LPTIM_CFGR_TMODE_Get(void)
{
	return (LPTIM->CFGR & LPTIM_CFGR_TMODE_Msk);
}

/* 32bit计数器当前计数值(Counter 32bits-wide) 相关函数 */
uint32_t LPTIM_CNTR_Read(void)
{
	return (LPTIM->CNTR);
}

/* 通道1捕捉源选择(Capture channel 1 source select)，仅在CH1通道配置为输入捕捉时有效
00：LPT_CH1输入
01：XTLF
10：RCLP
11：RCMF 相关函数 */
void LPTIM_CCSR_CAP1SSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAP1SSEL_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAP1SSEL_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAP1SSEL_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAP1SSEL_Msk);
}

/* 通道4当前被捕捉的边沿，在CC2IF置位时更新(Channel 4 Captured Edge)
0：上升沿
1：下降沿 相关函数 */
void LPTIM_CCSR_CAP4EDGE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAP4EDGE_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAP4EDGE_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAP4EDGE_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAP4EDGE_Msk);
}

void LPTIM_CCSR_CAP3EDGE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAP3EDGE_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAP3EDGE_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAP3EDGE_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAP3EDGE_Msk);
}

void LPTIM_CCSR_CAP2EDGE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAP2EDGE_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAP2EDGE_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAP2EDGE_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAP2EDGE_Msk);
}

void LPTIM_CCSR_CAP1EDGE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAP1EDGE_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAP1EDGE_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAP1EDGE_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAP1EDGE_Msk);
}

/* 通道4比较输出波形极性选择 (Channel 4 compare output Polarity)
0：正极性波形，起始为低，计数值==比较值时置高，计数值==ARR时恢复为低
1：负极性波形，正极性波形取反 相关函数 */
void LPTIM_CCSR_POLAR4_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_POLAR4_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_POLAR4_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_POLAR4_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_POLAR4_Msk);
}

void LPTIM_CCSR_POLAR3_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_POLAR3_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_POLAR3_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_POLAR3_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_POLAR3_Msk);
}

void LPTIM_CCSR_POLAR2_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_POLAR2_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_POLAR2_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_POLAR2_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_POLAR2_Msk);
}

void LPTIM_CCSR_POLAR1_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_POLAR1_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_POLAR1_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_POLAR1_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_POLAR1_Msk);
}

/* 通道4捕捉边沿选择(Channel 4 Capture edge Config)
00：上升沿捕捉
01：下降沿捕捉
10：上升下降沿捕捉
11：RFU 相关函数 */
void LPTIM_CCSR_CAPCFG4_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAPCFG4_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAPCFG4_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAPCFG4_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAPCFG4_Msk);
}

void LPTIM_CCSR_CAPCFG3_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAPCFG3_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAPCFG3_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAPCFG3_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAPCFG3_Msk);
}

void LPTIM_CCSR_CAPCFG2_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAPCFG2_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAPCFG2_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAPCFG2_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAPCFG2_Msk);
}

void LPTIM_CCSR_CAPCFG1_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CAPCFG1_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CAPCFG1_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CAPCFG1_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CAPCFG1_Msk);
}

/* 通道4捕捉/比较功能使能(Channel 4 Capture/Compare Select)
00,11：禁止通道4捕捉/比较功能
01：使能通道4捕捉功能（LPT_CH4为输入）
10：使能通道4比较功能（LPT_CH4为输出） 相关函数 */
void LPTIM_CCSR_CC4S_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CC4S_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CC4S_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CC4S_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CC4S_Msk);
}

void LPTIM_CCSR_CC3S_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CC3S_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CC3S_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CC3S_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CC3S_Msk);
}

void LPTIM_CCSR_CC2S_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CC2S_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CC2S_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CC2S_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CC2S_Msk);
}

void LPTIM_CCSR_CC1S_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPTIM->CCSR;
	tmpreg &= ~(LPTIM_CCSR_CC1S_Msk);
	tmpreg |= (SetValue & LPTIM_CCSR_CC1S_Msk);
	LPTIM->CCSR = tmpreg;
}

uint32_t LPTIM_CCSR_CC1S_Get(void)
{
	return (LPTIM->CCSR & LPTIM_CCSR_CC1S_Msk);
}

/* 自动重载目标寄存器(Auto-Reload Register)
当计数器计数值等于ARR时，计数器回到初值重新开始向上计数 相关函数 */
void LPTIM_ARR_Write(uint32_t SetValue)
{
	LPTIM->ARR = (SetValue);
}

uint32_t LPTIM_ARR_Read(void)
{
	return (LPTIM->ARR);
}

/* 通道4捕捉溢出中断使能(Channel 4 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
void LPTIM_IER_OVR4IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_OVR4IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_OVR4IE_Msk);
	}
}

FunState LPTIM_IER_OVR4IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_OVR4IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 通道3捕捉溢出中断使能(Channel 3 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
void LPTIM_IER_OVR3IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_OVR3IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_OVR3IE_Msk);
	}
}

FunState LPTIM_IER_OVR3IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_OVR3IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 通道2捕捉溢出中断使能(Channel 2 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
void LPTIM_IER_OVR2IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_OVR2IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_OVR2IE_Msk);
	}
}

FunState LPTIM_IER_OVR2IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_OVR2IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 通道1捕捉溢出中断使能(Channel 1 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
void LPTIM_IER_OVR1IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_OVR1IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_OVR1IE_Msk);
	}
}

FunState LPTIM_IER_OVR1IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_OVR1IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 外部触发到来中断使能位(External Trigger Interrupt Enable)
1：外部触发到来中断使能
0：外部触发到来中断禁止 相关函数 */
void LPTIM_IER_TRIGIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_TRIGIE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_TRIGIE_Msk);
	}
}

FunState LPTIM_IER_TRIGIE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_TRIGIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 计数器溢出中断使能位(Counter Over-Flow Interrupt Enable)
1：计数器溢出中断使能
0：计数器溢出中断禁止 相关函数 */
void LPTIM_IER_OVIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_OVIE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_OVIE_Msk);
	}
}

FunState LPTIM_IER_OVIE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_OVIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 捕捉/比较通道4中断使能位(Capture/Compare channel 4 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 相关函数 */
void LPTIM_IER_CC4IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_CC4IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_CC4IE_Msk);
	}
}

FunState LPTIM_IER_CC4IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_CC4IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 捕捉/比较通道3中断使能位(Capture/Compare channel 3 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 相关函数 */
void LPTIM_IER_CC3IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_CC3IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_CC3IE_Msk);
	}
}

FunState LPTIM_IER_CC3IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_CC3IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 捕捉/比较通道2中断使能位(Capture/Compare channel 2 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 相关函数 */
void LPTIM_IER_CC2IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_CC2IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_CC2IE_Msk);
	}
}

FunState LPTIM_IER_CC2IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_CC2IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 捕捉/比较通道1中断使能位(Capture/Compare channel 1 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 相关函数 */
void LPTIM_IER_CC1IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->IER |= (LPTIM_IER_CC1IE_Msk);
	}
	else
	{
		LPTIM->IER &= ~(LPTIM_IER_CC1IE_Msk);
	}
}

FunState LPTIM_IER_CC1IE_Getable(void)
{
	if (LPTIM->IER & (LPTIM_IER_CC1IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void LPTIM_ISR_CAP4OVR_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CAP4OVR_Msk;
}

FlagStatus LPTIM_ISR_CAP4OVR_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CAP4OVR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CAP3OVR_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CAP3OVR_Msk;
}

FlagStatus LPTIM_ISR_CAP3OVR_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CAP3OVR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CAP2OVR_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CAP2OVR_Msk;
}

FlagStatus LPTIM_ISR_CAP2OVR_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CAP2OVR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CAP1OVR_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CAP1OVR_Msk;
}

FlagStatus LPTIM_ISR_CAP1OVR_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CAP1OVR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_TRIGIF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_TRIGIF_Msk;
}

FlagStatus LPTIM_ISR_TRIGIF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_TRIGIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_OVIF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_OVIF_Msk;
}

FlagStatus LPTIM_ISR_OVIF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_OVIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CC4IF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CC4IF_Msk;
}

FlagStatus LPTIM_ISR_CC4IF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CC4IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CC3IF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CC3IF_Msk;
}

FlagStatus LPTIM_ISR_CC3IF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CC3IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CC2IF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CC2IF_Msk;
}

FlagStatus LPTIM_ISR_CC2IF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CC2IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LPTIM_ISR_CC1IF_Clr(void)
{
	LPTIM->ISR = LPTIM_ISR_CC1IF_Msk;
}

FlagStatus LPTIM_ISR_CC1IF_Chk(void)
{
	if (LPTIM->ISR & LPTIM_ISR_CC1IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LPTIM使能位(LPTIM Enable)
1：使能计数器计数
0：禁止计数器计数 相关函数 */
void LPTIM_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPTIM->CR |= (LPTIM_CR_EN_Msk);
	}
	else
	{
		LPTIM->CR &= ~(LPTIM_CR_EN_Msk);
	}
}

FunState LPTIM_CR_EN_Getable(void)
{
	if (LPTIM->CR & (LPTIM_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 捕捉/比较值寄存器1 (Channel1 Capture/Compare Register) 相关函数 */
void LPTIM_CCR1_Write(uint32_t SetValue)
{
	LPTIM->CCR1 = (SetValue);
}

uint32_t LPTIM_CCR1_Read(void)
{
	return (LPTIM->CCR1);
}

/* 捕捉/比较值寄存器2 (Channel2 Capture/Compare Register) 相关函数 */
void LPTIM_CCR2_Write(uint32_t SetValue)
{
	LPTIM->CCR2 = (SetValue);
}

uint32_t LPTIM_CCR2_Read(void)
{
	return (LPTIM->CCR2);
}

/* 捕捉/比较值寄存器3 (Channel3 Capture/Compare Register) 相关函数 */
void LPTIM_CCR3_Write(uint32_t SetValue)
{
	LPTIM->CCR3 = (SetValue);
}

uint32_t LPTIM_CCR3_Read(void)
{
	return (LPTIM->CCR3);
}

/* 捕捉/比较值寄存器4 (Channel4 Capture/Compare Register) 相关函数 */
void LPTIM_CCR4_Write(uint32_t SetValue)
{
	LPTIM->CCR4 = (SetValue);
}

uint32_t LPTIM_CCR4_Read(void)
{
	return (LPTIM->CCR4);
}

void LPTIM_Deinit(void)
{
	//LPTIM->CFGR = 0x00000000;
	//LPTIM->CNTR = 0x00000000;
	//LPTIM->CCSR = ;
	//LPTIM->ARR = 0x00000000;
	//LPTIM->IER = 0x00000000;
	//LPTIM->ISR = ;
	//LPTIM->CR = 0x00000000;
	//LPTIM->CCR1 = 0x00000000;
}

/******END OF FILE****/
