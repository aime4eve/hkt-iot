/**
  ******************************************************************************
  * @file    fm33a0xxev_cmu.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_cmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup CMU 
  * @brief CMU driver modules
  * @{
  */ 

void CMU_SYSCLKCR_SLP_ENEXTI_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->SYSCLKCR |= (CMU_SYSCLKCR_SLP_ENEXTI_Msk);
	}
	else
	{
		CMU->SYSCLKCR &= ~(CMU_SYSCLKCR_SLP_ENEXTI_Msk);
	}
}

FunState CMU_SYSCLKCR_SLP_ENEXTI_Getable(void)
{
	if (CMU->SYSCLKCR & (CMU_SYSCLKCR_SLP_ENEXTI_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* APB时钟分频选择 (APBbus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 相关函数 */
void CMU_SYSCLKCR_APBPRES_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->SYSCLKCR;
	tmpreg &= ~(CMU_SYSCLKCR_APBPRES_Msk);
	tmpreg |= (SetValue & CMU_SYSCLKCR_APBPRES_Msk);
	CMU->SYSCLKCR = tmpreg;
}

uint32_t CMU_SYSCLKCR_APBPRES_Get(void)
{
	return (CMU->SYSCLKCR & CMU_SYSCLKCR_APBPRES_Msk);
}

/* AHB时钟分频选择 (AHB bus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 相关函数 */
void CMU_SYSCLKCR_AHBPRES_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->SYSCLKCR;
	tmpreg &= ~(CMU_SYSCLKCR_AHBPRES_Msk);
	tmpreg |= (SetValue & CMU_SYSCLKCR_AHBPRES_Msk);
	CMU->SYSCLKCR = tmpreg;
}

uint32_t CMU_SYSCLKCR_AHBPRES_Get(void)
{
	return (CMU->SYSCLKCR & CMU_SYSCLKCR_AHBPRES_Msk);
}

/* CPU内核systick工作时钟选择 (Systick clock select)
00：SCLK
01：LSCLK
10：RC4M
11：SYSCLK 相关函数 */
void CMU_SYSCLKCR_STCLKSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->SYSCLKCR;
	tmpreg &= ~(CMU_SYSCLKCR_STCLKSEL_Msk);
	tmpreg |= (SetValue & CMU_SYSCLKCR_STCLKSEL_Msk);
	CMU->SYSCLKCR = tmpreg;
}

uint32_t CMU_SYSCLKCR_STCLKSEL_Get(void)
{
	return (CMU->SYSCLKCR & CMU_SYSCLKCR_STCLKSEL_Msk);
}

/* 系统时钟源选择 相关函数 */
void CMU_SYSCLKCR_SYSCLKSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->SYSCLKCR;
	tmpreg &= ~(CMU_SYSCLKCR_SYSCLKSEL_Msk);
	tmpreg |= (SetValue & CMU_SYSCLKCR_SYSCLKSEL_Msk);
	CMU->SYSCLKCR = tmpreg;
}

uint32_t CMU_SYSCLKCR_SYSCLKSEL_Get(void)
{
	return (CMU->SYSCLKCR & CMU_SYSCLKCR_SYSCLKSEL_Msk);
}

/* RCHF频率选择寄存器0000：8MHz
0001：16MHz
0010：24MHz
0011：32MHz
0111：40MHz
1111：48MHz
其他：RFU 相关函数 */
void CMU_RCHFCR_FSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->RCHFCR;
	tmpreg &= ~(CMU_RCHFCR_FSEL_Msk);
	tmpreg |= (SetValue & CMU_RCHFCR_FSEL_Msk);
	CMU->RCHFCR = tmpreg;
}

uint32_t CMU_RCHFCR_FSEL_Get(void)
{
	return (CMU->RCHFCR & CMU_RCHFCR_FSEL_Msk);
}

/* RCHF使能寄存器 (RCHF Enable)
1：使能RCHF
0：关闭RCHF 相关函数 */
void CMU_RCHFCR_RCHFEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->RCHFCR |= (CMU_RCHFCR_RCHFEN_Msk);
	}
	else
	{
		CMU->RCHFCR &= ~(CMU_RCHFCR_RCHFEN_Msk);
	}
}

FunState CMU_RCHFCR_RCHFEN_Getable(void)
{
	if (CMU->RCHFCR & (CMU_RCHFCR_RCHFEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RCHF频率调校寄存器，8’h00表示频率最低，8’hFF表示频率最高，调校范围为中心频率+/-30%，调校步长为中心频率0.25%
上电后芯片自动从LDT0读取8MHz调校值并写入此寄存器
软件使用其他频率时，可以自行从LDT0指定地址读取调校信息并写入此寄存器，从而确保输出频率准确。 相关函数 */
void CMU_RCHFTR_RCHFTRIM_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->RCHFTR;
	tmpreg &= ~(CMU_RCHFTR_RCHFTRIM_Msk);
	tmpreg |= (SetValue & CMU_RCHFTR_RCHFTRIM_Msk);
	CMU->RCHFTR = tmpreg;
}

uint32_t CMU_RCHFTR_RCHFTRIM_Get(void)
{
	return (CMU->RCHFTR & CMU_RCHFTR_RCHFTRIM_Msk);
}

void CMU_PLLLCR_PLLDB_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->PLLLCR;
	tmpreg &= ~(CMU_PLLLCR_PLLDB_Msk);
	tmpreg |= (SetValue & CMU_PLLLCR_PLLDB_Msk);
	CMU->PLLLCR = tmpreg;
}

uint32_t CMU_PLLLCR_PLLDB_Get(void)
{
	return (CMU->PLLLCR & CMU_PLLLCR_PLLDB_Msk);
}

/* PLL_L锁定标志
1：PLL_L已锁定
0：PLL_L未锁定 相关函数 */
FlagStatus CMU_PLLLCR_LOCKED_Chk(void)
{
	if (CMU->PLLLCR & CMU_PLLLCR_LOCKED_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* PLL使能寄存器
1：使能PLL
0：关闭PLL 相关函数 */
void CMU_PLLLCR_PLLEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->PLLLCR |= (CMU_PLLLCR_PLLEN_Msk);
	}
	else
	{
		CMU->PLLLCR &= ~(CMU_PLLLCR_PLLEN_Msk);
	}
}

FunState CMU_PLLLCR_PLLEN_Getable(void)
{
	if (CMU->PLLLCR & (CMU_PLLLCR_PLLEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* PLLH倍频比，参考时钟1Mhz
0000011111：输出32倍频
0101111：输出48倍频 相关函数 */
void CMU_PLLHCR_PLLHDB_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->PLLHCR;
	tmpreg &= ~(CMU_PLLHCR_PLLHDB_Msk);
	tmpreg |= (SetValue & CMU_PLLHCR_PLLHDB_Msk);
	CMU->PLLHCR = tmpreg;
}

uint32_t CMU_PLLHCR_PLLHDB_Get(void)
{
	return (CMU->PLLHCR & CMU_PLLHCR_PLLHDB_Msk);
}

/* PLLH锁定标志，软件通过查询此寄存器确认PLL已经处于锁定状态
1：PLL已锁定
0：PLL未锁定 相关函数 */
FlagStatus CMU_PLLHCR_LOCKED_Chk(void)
{
	if (CMU->PLLHCR & CMU_PLLHCR_LOCKED_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* PLLH参考时钟预分频（目标是产生1MHz参考时钟给PLL）
000：不分频
001：2分频
010：4分频
011：8分频
100：12分频
101：16分频
110：24分频
111：32分频 相关函数 */
void CMU_PLLHCR_REFPRSC_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->PLLHCR;
	tmpreg &= ~(CMU_PLLHCR_REFPRSC_Msk);
	tmpreg |= (SetValue & CMU_PLLHCR_REFPRSC_Msk);
	CMU->PLLHCR = tmpreg;
}

uint32_t CMU_PLLHCR_REFPRSC_Get(void)
{
	return (CMU->PLLHCR & CMU_PLLHCR_REFPRSC_Msk);
}

/* PLLH输出选择寄存器
0：选择PLLH一倍输出作为数字电路内的PLL时钟
1：选择PLLH两倍输出作为数字电路内的PLL时钟 相关函数 */
void CMU_PLLHCR_OSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->PLLHCR;
	tmpreg &= ~(CMU_PLLHCR_OSEL_Msk);
	tmpreg |= (SetValue & CMU_PLLHCR_OSEL_Msk);
	CMU->PLLHCR = tmpreg;
}

uint32_t CMU_PLLHCR_OSEL_Get(void)
{
	return (CMU->PLLHCR & CMU_PLLHCR_OSEL_Msk);
}

/* PLLH输入选择寄存器
0：RCHF
1：XTHF */
void CMU_PLLHCR_INSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->PLLHCR;
	tmpreg &= ~(CMU_PLLHCR_INSEL_Msk);
	tmpreg |= (SetValue & CMU_PLLHCR_INSEL_Msk);
	CMU->PLLHCR = tmpreg;
}

uint32_t CMU_PLLHCR_INSEL_Get(void)
{
	return (CMU->PLLHCR & CMU_PLLHCR_INSEL_Msk);
}

/* PLLH使能寄存器
1：使能PLLH
0：关闭PLLH 相关函数 */
void CMU_PLLHCR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->PLLHCR |= (CMU_PLLHCR_EN_Msk);
	}
	else
	{
		CMU->PLLHCR &= ~(CMU_PLLHCR_EN_Msk);
	}
}

FunState CMU_PLLHCR_EN_Getable(void)
{
	if (CMU->PLLHCR & (CMU_PLLHCR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* XTHF振荡强度配置
000：最弱
111：最强 相关函数 */
void CMU_XTHFCR_XTHF_CFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->XTHFCR;
	tmpreg &= ~(CMU_XTHFCR_XTHF_CFG_Msk);
	tmpreg |= (SetValue & CMU_XTHFCR_XTHF_CFG_Msk);
	CMU->XTHFCR = tmpreg;
}

uint32_t CMU_XTHFCR_XTHF_CFG_Get(void)
{
	return (CMU->XTHFCR & CMU_XTHFCR_XTHF_CFG_Msk);
}

/* XTHF使能寄存器
0：关闭XTHF
1：使能XTHF 相关函数 */
void CMU_XTHFCR_XTHFEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->XTHFCR |= (CMU_XTHFCR_XTHFEN_Msk);
	}
	else
	{
		CMU->XTHFCR &= ~(CMU_XTHFCR_XTHFEN_Msk);
	}
}

FunState CMU_XTHFCR_XTHFEN_Getable(void)
{
	if (CMU->XTHFCR & (CMU_XTHFCR_XTHFEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void CMU_IER_SYSCSE_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->IER |= (CMU_IER_SYSCSE_IE_Msk);
	}
	else
	{
		CMU->IER &= ~(CMU_IER_SYSCSE_IE_Msk);
	}
}

FunState CMU_IER_SYSCSE_IE_Getable(void)
{
	if (CMU->IER & (CMU_IER_SYSCSE_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void CMU_IER_HFDET_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->IER |= (CMU_IER_HFDET_IE_Msk);
	}
	else
	{
		CMU->IER &= ~(CMU_IER_HFDET_IE_Msk);
	}
}

FunState CMU_IER_HFDET_IE_Getable(void)
{
	if (CMU->IER & (CMU_IER_HFDET_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


/* 高频晶体停振检测模块输出
1：XTHF未停振
0：XTHF停振 相关函数 */
FlagStatus CMU_ISR_HFDETO_Chk(void)
{
	if (CMU->ISR & CMU_ISR_HFDETO_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


/* SYSCLK时钟选择错误中断标志寄存器；当被选中的时钟源没有使能时，此中断标志置位，软件写1清零。 相关函数 */
void CMU_ISR_SYSCSE_IF_Clr(void)
{
	CMU->ISR = CMU_ISR_SYSCSE_IF_Msk;
}

FlagStatus CMU_ISR_SYSCSE_IF_Chk(void)
{
	if (CMU->ISR & CMU_ISR_SYSCSE_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 高频停振检测中断标志寄存器，XTHF停振时硬件异步置位，软件写1清零；只有在FFDETO不为0的情况下才能够清除此寄存器 相关函数 */
void CMU_ISR_HFDET_IF_Clr(void)
{
	CMU->ISR = CMU_ISR_HFDET_IF_Msk;
}

FlagStatus CMU_ISR_HFDET_IF_Chk(void)
{
	if (CMU->ISR & CMU_ISR_HFDET_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* EXTI工作时钟使能，高有效 相关函数 */
void CMU_OPCCR1_EXTICKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_EXTICKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_EXTICKE_Msk);
	}
}

FunState CMU_OPCCR1_EXTICKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_EXTICKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* EXTI中断采样时钟选择 */
void CMU_OPCCR1_EXTICKSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_EXTICKSEL_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_EXTICKSEL_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_EXTICKSEL_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_EXTICKSEL_Msk);
}

/* LPUART1工作时钟使能，高有效 相关函数 */
void CMU_OPCCR1_LPUART1CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_LPUART1CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_LPUART1CKE_Msk);
	}
}

FunState CMU_OPCCR1_LPUART1CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_LPUART1CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LPUART0工作时钟使能，高有效 相关函数 */
void CMU_OPCCR1_LPUART0CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_LPUART0CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_LPUART0CKE_Msk);
	}
}

FunState CMU_OPCCR1_LPUART0CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_LPUART0CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LPUART1工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU 相关函数 */
void CMU_OPCCR1_LPUART1CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_LPUART1CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_LPUART1CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_LPUART1CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_LPUART1CKS_Msk);
}

/* LPUART0工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU 相关函数 */
void CMU_OPCCR1_LPUART0CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_LPUART0CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_LPUART0CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_LPUART0CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_LPUART0CKS_Msk);
}

/* I2C1工作时钟使能 相关函数 */
void CMU_OPCCR1_I2C1CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_I2C1CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_I2C1CKE_Msk);
	}
}

FunState CMU_OPCCR1_I2C1CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_I2C1CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C0工作时钟使能 相关函数 */
void CMU_OPCCR1_I2C0CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_I2C0CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_I2C0CKE_Msk);
	}
}

FunState CMU_OPCCR1_I2C0CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_I2C0CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C1主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK 相关函数 */
void CMU_OPCCR1_I2C1CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_I2C1CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_I2C1CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_I2C1CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_I2C1CKS_Msk);
}


/* I2C0主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK 相关函数 */
void CMU_OPCCR1_I2C0CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_I2C0CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_I2C0CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_I2C0CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_I2C0CKS_Msk);
}

/* UART1工作时钟使能，高有效 相关函数 */
void CMU_OPCCR1_UART1CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_UART1CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_UART1CKE_Msk);
	}
}

FunState CMU_OPCCR1_UART1CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_UART1CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART0工作时钟使能，高有效 相关函数 */
void CMU_OPCCR1_UART0CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR1 |= (CMU_OPCCR1_UART0CKE_Msk);
	}
	else
	{
		CMU->OPCCR1 &= ~(CMU_OPCCR1_UART0CKE_Msk);
	}
}

FunState CMU_OPCCR1_UART0CKE_Getable(void)
{
	if (CMU->OPCCR1 & (CMU_OPCCR1_UART0CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART1工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU 相关函数 */
void CMU_OPCCR1_UART1CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_UART1CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_UART1CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_UART1CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_UART1CKS_Msk);
}

/* UART0工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU 相关函数 */
void CMU_OPCCR1_UART0CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR1;
	tmpreg &= ~(CMU_OPCCR1_UART0CKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR1_UART0CKS_Msk);
	CMU->OPCCR1 = tmpreg;
}

uint32_t CMU_OPCCR1_UART0CKS_Get(void)
{
	return (CMU->OPCCR1 & CMU_OPCCR1_UART0CKS_Msk);
}

/* 随机数发生器工作时钟分频
000：不分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110, 111：RFU 相关函数 */
void CMU_OPCCR2_RNGPRSC_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR2;
	tmpreg &= ~(CMU_OPCCR2_RNGPRSC_Msk);
	tmpreg |= (SetValue & CMU_OPCCR2_RNGPRSC_Msk);
	CMU->OPCCR2 = tmpreg;
}

uint32_t CMU_OPCCR2_RNGPRSC_Get(void)
{
	return (CMU->OPCCR2 & CMU_OPCCR2_RNGPRSC_Msk);
}

/* Flash擦写时钟使能，高有效 相关函数 */
void CMU_OPCCR2_NVMCKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR2 |= (CMU_OPCCR2_NVMCKE_Msk);
	}
	else
	{
		CMU->OPCCR2 &= ~(CMU_OPCCR2_NVMCKE_Msk);
	}
}

FunState CMU_OPCCR2_NVMCKE_Getable(void)
{
	if (CMU->OPCCR2 & (CMU_OPCCR2_NVMCKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 随机数发生器工作时钟使能，高有效 相关函数 */
void CMU_OPCCR2_RNGCKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR2 |= (CMU_OPCCR2_RNGCKE_Msk);
	}
	else
	{
		CMU->OPCCR2 &= ~(CMU_OPCCR2_RNGCKE_Msk);
	}
}

FunState CMU_OPCCR2_RNGCKE_Getable(void)
{
	if (CMU->OPCCR2 & (CMU_OPCCR2_RNGCKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/*  _工作时钟使能，高有效 相关函数 */
void CMU_OPCCR2_LPTCKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR2 |= (CMU_OPCCR2_LPTCKE_Msk);
	}
	else
	{
		CMU->OPCCR2 &= ~(CMU_OPCCR2_LPTCKE_Msk);
	}
}

FunState CMU_OPCCR2_LPTCKE_Getable(void)
{
	if (CMU->OPCCR2 & (CMU_OPCCR2_LPTCKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/*  _工作时钟选择
00：APBCLK
01：LSCLK
10：RCLP
11：PLL_L 相关函数 */
void CMU_OPCCR2_LPTCKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR2;
	tmpreg &= ~(CMU_OPCCR2_LPTCKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR2_LPTCKS_Msk);
	CMU->OPCCR2 = tmpreg;
}

uint32_t CMU_OPCCR2_LPTCKS_Get(void)
{
	return (CMU->OPCCR2 & CMU_OPCCR2_LPTCKS_Msk);
}

/* BSTIM工作时钟使能，高有效 相关函数 */
void CMU_OPCCR2_BSTCKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CMU->OPCCR2 |= (CMU_OPCCR2_BSTCKE_Msk);
	}
	else
	{
		CMU->OPCCR2 &= ~(CMU_OPCCR2_BSTCKE_Msk);
	}
}

FunState CMU_OPCCR2_BSTCKE_Getable(void)
{
	if (CMU->OPCCR2 & (CMU_OPCCR2_BSTCKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* BSTIM工作时钟源选择
00：APBCLK
01：LSCLK
10：RCLP
11：SYSCLK 相关函数 */
void CMU_OPCCR2_BSTCKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CMU->OPCCR2;
	tmpreg &= ~(CMU_OPCCR2_BSTCKS_Msk);
	tmpreg |= (SetValue & CMU_OPCCR2_BSTCKS_Msk);
	CMU->OPCCR2 = tmpreg;
}

uint32_t CMU_OPCCR2_BSTCKS_Get(void)
{
	return (CMU->OPCCR2 & CMU_OPCCR2_BSTCKS_Msk);
}

void CMU_Deinit(void)
{
	//CMU->SYSCLKCR = ;
	//CMU->RCHFCR = 0x00000000;
	//CMU->RCHFTR = 0x00000000;
	//CMU->PLLLCR = ;
	//CMU->PLLHCR = 0x00000000;
	//CMU->XTHFCR = 0x00000000;
	//CMU->IER = ;
	//CMU->ISR = ;
	//CMU->PCLKCR1 = 0x00000058;
	//CMU->PCLKCR2 = 0x00000000;
	//CMU->PCLKCR3 = 0x00000000;
	//CMU->PCLKCR4 = ;
	//CMU->OPCCR1 = 0x00000000;
	//CMU->OPCCR2 = 0x00000000;
}
//Code_End


/********************************
获取芯片已设置的不同时钟频率函数
功能:获取芯片已设置的不同时钟频率
RCHF_Frequency 
PLL_H_Frequency 
SYSCLK_Frequency 
AHBCLK_Frequency 
APBCLK_Frequency 
********************************/
void CMU_GetClocksFreq(CMU_ClocksType* para)
{
    uint32_t tmp32 = 0;
		para->RCHF_Frequency = 0;
		para->PLL_H_Frequency = 0;
		para->SYSCLK_Frequency = 0;
		para->AHBCLK_Frequency = 0;
		para->APBCLK_Frequency = 0;
		
	/* Get RCHF FSEL */
	if(ENABLE == CMU_RCHFCR_RCHFEN_Getable())
	{
		tmp32 = CMU_RCHFCR_FSEL_Get();
		tmp32 = tmp32 >> CMU_RCHFCR_FSEL_Pos;  
		/* RCHF clock frequency */
		if(tmp32==0xf)
		{
			/* 1111：48MHz */
			para->RCHF_Frequency = 48000000 ;	
		}
		else if(tmp32==0x07)
		{
			/* 0111：40MHz */
			para->RCHF_Frequency = 40000000 ;	
		}
		else
		{
			para->RCHF_Frequency = __RCHF_INITIAL_CLOCK * (tmp32 + 1);
		}
	}

	if(ENABLE == CMU_PLLHCR_EN_Getable())
	{
		/* PLLH倍频比 */
		tmp32 = CMU_PLLHCR_PLLHDB_Get();
		tmp32 = tmp32 >> CMU_PLLHCR_PLLHDB_Pos;
		para->PLL_H_Frequency = (tmp32 + 1) * 1000000;
		/* PLLH输出选择寄存器 */
		if( CMU_PLLHCR_OSEL_X2 == CMU_PLLHCR_OSEL_Get() )
		{
			para->PLL_H_Frequency = (para->PLL_H_Frequency)*2;
		}
	}
  
	/* Get SYSCLK source */
    tmp32 = CMU_SYSCLKCR_SYSCLKSEL_Get();
    tmp32 = tmp32 >> CMU_SYSCLKCR_SYSCLKSEL_Pos;
    switch (tmp32)
    {
        case 0x00:  /* RCHF used as system clock source */
            para->SYSCLK_Frequency = para->RCHF_Frequency;
            break;
        case 0x01:  /* XTHF used as system clock  source */
            para->SYSCLK_Frequency = __XTHF_CLOCK;
            break;
				case 0x02:  /* PLL_H used as system clock  source */
					para->SYSCLK_Frequency = para->PLL_H_Frequency;
					break;
        case 0x03:  /* LSCLK used as system clock  source */
            para->SYSCLK_Frequency = __XTLF_CLOCK;
            break;
        default:
            para->SYSCLK_Frequency = para->RCHF_Frequency;
            break;
    }
	
	/* Get AHB PRES */
    tmp32 = CMU_SYSCLKCR_AHBPRES_Get();
    tmp32 = tmp32 >> CMU_SYSCLKCR_AHBPRES_Pos;  
    switch(tmp32)
    {
        case 0x04: //100:DIV2
            para->AHBCLK_Frequency = para->SYSCLK_Frequency/2;
            break;
        case 0x05: //101:DIV4
            para->AHBCLK_Frequency = para->SYSCLK_Frequency/4;
            break;
        case 0x06: //110:DIV8
            para->AHBCLK_Frequency = para->SYSCLK_Frequency/8;
            break;
        case 0x07: //111:DIV16
            para->AHBCLK_Frequency = para->SYSCLK_Frequency/16;
            break;
        default: //0XX:DIV1
            para->AHBCLK_Frequency = para->SYSCLK_Frequency;
            break;
    }

	/* Get APB PRES */
    tmp32 = CMU_SYSCLKCR_APBPRES_Get();
    tmp32 = tmp32 >> CMU_SYSCLKCR_APBPRES_Pos; 
	/* APB clock frequency */
    switch(tmp32)
    {
        case 0x04: //100:DIV2
            para->APBCLK_Frequency = para->AHBCLK_Frequency/2;
            break;
        case 0x05: //101:DIV4
            para->APBCLK_Frequency = para->AHBCLK_Frequency/4;
            break;
        case 0x06: //110:DIV8
            para->APBCLK_Frequency = para->AHBCLK_Frequency/8;
            break;
        case 0x07: //111:DIV16
            para->APBCLK_Frequency = para->AHBCLK_Frequency/16;
            break;
        default: //0XX:DIV1
            para->APBCLK_Frequency = para->AHBCLK_Frequency;
            break;
    }
}


/********************************
外设时钟使能函数
功能:对外设模块所需要的对应时钟进行使能
输入：periph_def外部模块标号
      NewState使能位
 ********************************/
void CMU_PERCLK_SetableEx(uint32_t periph_def, FunState NewState)
{
	__IO uint32_t *p_reg = 0;
	uint32_t TempREG;
	
    switch((periph_def & 0x0F000000))
    {
		case 0x01000000:
			p_reg = &(CMU->PCLKCR1);
			break;
		
		case 0x02000000:
			p_reg = &(CMU->PCLKCR2);
			break;
		
		case 0x03000000:
			p_reg = &(CMU->PCLKCR3);
			break;
		
		case 0x04000000:
			p_reg = &(CMU->PCLKCR4);
			break;
		
		default:
			break;
    }
	if(p_reg)
	{
		periph_def &= 0xFFFF;
		TempREG = (0x1U << periph_def);
		
		if (NewState != DISABLE)
		{
			*p_reg |= TempREG;
		}
		else
		{
			*p_reg &= ~TempREG;
		}
	}
}


/********************************
RCHF初始化函数
功能:对RCHF进行初始化
输入：RCHF频率和使能
 ********************************/
void CMU_RCHF_Init(CMU_RCHF_InitTypeDef* para)
{
	CMU_RCHFCR_FSEL_Set(para->FSEL);
	CMU_RCHFCR_RCHFEN_Setable(para->RCHFEN);
}

/********************************
PLL H初始化函数
功能:对PLL H进行初始化
输入：对PLL H设置的相关参数
 ********************************/
void CMU_PLL_H_Init(CMU_PLL_H_InitTypeDef* para)
{
	CMU_PLLHCR_PLLHDB_Set(para->PLLHDB);
	CMU_PLLHCR_REFPRSC_Set(para->REFPRSC);
	CMU_PLLHCR_OSEL_Set(para->PLLH_OSEL);
	CMU_PLLHCR_INSEL_Set(para->PLLH_INSEL);
	CMU_PLLHCR_EN_Setable(para->PLLH_EN);
}

/* 系统时钟初始化 */
void CMU_SysClk_Init(CMU_SYSCLK_InitTypeDef* para)
{
	CMU_SYSCLKCR_SYSCLKSEL_Set(para->SYSCLKSEL);
	CMU_SYSCLKCR_AHBPRES_Set(para->AHBPRES);
	CMU_SYSCLKCR_APBPRES_Set(para->APBPRES);
	CMU_SYSCLKCR_SLP_ENEXTI_Setable(para->SLP_ENEXTI);
}

/********************************
rchf常温校准值载入函数
功能:rchf常温校准值载入(rchf常温校准值载入(芯片复位后自动载入8M的校准值))
输入：要设置的RCHF频率
			ClkMode 1 = 8M
			ClkMode 2 = 16M
			ClkMode 3 = 24M
			ClkMode 4 = 32M
 ********************************/
void CMU_Init_RCHF_Trim( uint8_t ClkMode )
{
	uint32_t Temp32;
	
	if( ClkMode == 1 )	//8M
	{
		Temp32 = const_rchf_Trim8;
	}
	else if( ClkMode == 2 )//16M
	{
		Temp32 = const_rchf_Trim16;
	}
	else if( ClkMode == 3 )//24M
	{
		Temp32 = const_rchf_Trim24;
	}
	else if( ClkMode == 4 )//32M
	{
		Temp32 = const_rchf_Trim32;
	}
  else if( ClkMode == 5 )//40M
	{
		Temp32 = const_rchf_Trim40;
	}
  else if( ClkMode == 6 )//48M
	{
		Temp32 = const_rchf_Trim48;
	}
	else//默认8M
	{
		Temp32 = const_rchf_Trim8;
	}
	
	if( ((Temp32>>16)&0x0000FFFF) == ((~Temp32)&0x0000FFFF) )	//正反码校验判断
	{
		CMU_RCHFTR_RCHFTRIM_Set(Temp32&0x000000FF);
	}	
	else
	{
		CMU_RCHFTR_RCHFTRIM_Set(0x0000006B);
	}
}

/******END OF FILE****/
