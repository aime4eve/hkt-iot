/**
  ******************************************************************************
  * @file    fm33a0xxev_bt.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_bt.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup BT
  * @brief BT driver modules
  * @{
  */ 
 
/* 高8位计数器（BT1H或BT2H）启动控制  相关函数 */
void BTx_CR1_CHEN_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR1  |= (BTx_CR1_CHEN_Msk);
	}
	else
	{
		BTx->CR1  &= ~(BTx_CR1_CHEN_Msk);
	}
}

FunState BTx_CR1_CHEN_Getable(BT_Type* BTx)
{
	if (BTx->CR1  & (BTx_CR1_CHEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 低8位计数器（BT1L或BT2L）启动控制
(Counter-Lowend enable)
1：启动低8bit计数器，在计数器模式下启动时将预置数值和加载值分别加载至计数器和比较寄存器；捕捉模式下启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数，捕捉功能只工作在16位模式；在16位的定时/计数和捕捉模式下CHEN作为计数器的启动控制，CLEN自动失效
0：停止低8bit计数器计数 相关函数 */
void BTx_CR1_CLEN_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR1  |= (BTx_CR1_CLEN_Msk);
	}
	else
	{
		BTx->CR1  &= ~(BTx_CR1_CLEN_Msk);
	}
}

FunState BTx_CR1_CLEN_Getable(BT_Type* BTx)
{
	if (BTx->CR1  & (BTx_CR1_CLEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 工作模式选择 (work mode)
1：16位捕捉模式
0：8位定时/计数模式，若高位计数器计数源选择为低位计数器的溢出信号，则可实现16位定时/计数模式 相关函数 */
void BTx_CR1_MODE_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR1 ;
	tmpreg &= ~(BTx_CR1_MODE_Msk);
	tmpreg |= (SetValue & BTx_CR1_MODE_Msk);
	BTx->CR1  = tmpreg;
}

uint32_t BTx_CR1_MODE_Get(BT_Type* BTx)
{
	return (BTx->CR1  & BTx_CR1_MODE_Msk);
}

/* 计数模式下的计数沿和周期捕捉时的捕捉沿选择位 (edge select)
1：计数模式采样计数源下降沿，周期捕捉模式时下沿捕捉
0：计数模式采样计数源上升沿，周期捕捉模式时上沿捕捉
注：不支持系统时钟的下降沿计数，捕捉源和计数源为系统时钟时选择下降沿将不会有效计数。 相关函数 */
void BTx_CR1_EDGESEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR1 ;
	tmpreg &= ~(BTx_CR1_EDGESEL_Msk);
	tmpreg |= (SetValue & BTx_CR1_EDGESEL_Msk);
	BTx->CR1  = tmpreg;
}

uint32_t BTx_CR1_EDGESEL_Get(BT_Type* BTx)
{
	return (BTx->CR1  & BTx_CR1_EDGESEL_Msk);
}

/* 捕捉模式控制（只在捕捉模式下有效）(capture mode)
1：脉冲宽度捕捉
0：脉冲周期捕捉 相关函数 */
void BTx_CR1_CAPMOD_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR1 ;
	tmpreg &= ~(BTx_CR1_CAPMOD_Msk);
	tmpreg |= (SetValue & BTx_CR1_CAPMOD_Msk);
	BTx->CR1  = tmpreg;
}

uint32_t BTx_CR1_CAPMOD_Get(BT_Type* BTx)
{
	return (BTx->CR1  & BTx_CR1_CAPMOD_Msk);
}

/* 带清零捕捉模式控制 (capture clear)
1：不论在脉冲宽度还是周期捕捉情况下，捕捉到第一个沿后将计数器清零产生中断，捕捉到第二个沿后锁存（锁存到高低位预置数寄存器）计数值并同时清零计数器
0：捕捉不清零，计数器一直自由计数 相关函数 */
void BTx_CR1_CAPCLR_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR1  |= (BTx_CR1_CAPCLR_Msk);
	}
	else
	{
		BTx->CR1  &= ~(BTx_CR1_CAPCLR_Msk);
	}
}

FunState BTx_CR1_CAPCLR_Getable(BT_Type* BTx)
{
	if (BTx->CR1  & (BTx_CR1_CAPCLR_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 单次捕捉控制 (capture once)
1：单次捕捉有效，在捕捉到一次脉冲宽度或脉冲周期后计数器停止，若需要再次捕捉需重新启动
0：连续捕捉 相关函数 */
void BTx_CR1_CAPONCE_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR1 ;
	tmpreg &= ~(BTx_CR1_CAPONCE_Msk);
	tmpreg |= (SetValue & BTx_CR1_CAPONCE_Msk);
	BTx->CR1  = tmpreg;
}

uint32_t BTx_CR1_CAPONCE_Get(BT_Type* BTx)
{
	return (BTx->CR1  & BTx_CR1_CAPONCE_Msk);
}

/* PWM模式输出 (pulse width modulation)
1：PWM输出使能
0：PWM 输出不使能 相关函数 */
void BTx_CR1_PWM_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR1  |= (BTx_CR1_PWM_Msk);
	}
	else
	{
		BTx->CR1  &= ~(BTx_CR1_PWM_Msk);
	}
}

FunState BTx_CR1_PWM_Getable(BT_Type* BTx)
{
	if (BTx->CR1  & (BTx_CR1_PWM_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 计数器内部计数源信号选择 (signal group2 select)
1：内部计数源信号选择Group2
0：内部计数源信号选择Group1 相关函数 */
void BTx_CR2_SIG2SEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR2 ;
	tmpreg &= ~(BTx_CR2_SIG2SEL_Msk);
	tmpreg |= (SetValue & BTx_CR2_SIG2SEL_Msk);
	BTx->CR2  = tmpreg;
}

uint32_t BTx_CR2_SIG2SEL_Get(BT_Type* BTx)
{
	return (BTx->CR2  & BTx_CR2_SIG2SEL_Msk);
}

/* 计数器内部捕捉源信号选择 (signal group1 select)
1：内部捕捉源信号选择Group1
0：内部捕捉源信号选择Group2 相关函数 */
void BTx_CR2_SIG1SEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR2 ;
	tmpreg &= ~(BTx_CR2_SIG1SEL_Msk);
	tmpreg |= (SetValue & BTx_CR2_SIG1SEL_Msk);
	BTx->CR2  = tmpreg;
}

uint32_t BTx_CR2_SIG1SEL_Get(BT_Type* BTx)
{
	return (BTx->CR2  & BTx_CR2_SIG1SEL_Msk);
}

/* 高8位计数器计数源选择 (Counter Highend source select)
00/11：选择ET1的低位计数器的溢出信号，与低位计数器组成16位计数器
01：选择内部捕捉源信号
10：选择内部计数源信号或外部DIR输入组合信号 相关函数 */
void BTx_CR2_CNTHSEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR2 ;
	tmpreg &= ~(BTx_CR2_CNTHSEL_Msk);
	tmpreg |= (SetValue & BTx_CR2_CNTHSEL_Msk);
	BTx->CR2  = tmpreg;
}

uint32_t BTx_CR2_CNTHSEL_Get(BT_Type* BTx)
{
	return (BTx->CR2  & BTx_CR2_CNTHSEL_Msk);
}

/* 外部输入DIR控制使能。通常电量脉冲输出时同时会输出一个由高低电平指示正反向的方向信号DIR。电路将通过DIR信号电平的高低，分别控制高位计数器和低位计数器计数使能，以实现针对正向、反向脉冲的各种计数功能 (direction bit enable)
1：外部输入的DIR信号有效，此时高低位计数器是否计数可由外部输入的DIR信号控制。
0：外部输入的DIR信号无效，此时高低位计数器是否计数将由内部控制信号控制。 相关函数 */
void BTx_CR2_DIREN_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR2  |= (BTx_CR2_DIREN_Msk);
	}
	else
	{
		BTx->CR2  &= ~(BTx_CR2_DIREN_Msk);
	}
}

FunState BTx_CR2_DIREN_Getable(BT_Type* BTx)
{
	if (BTx->CR2  & (BTx_CR2_DIREN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 内部DIR控制信号，当DIREN为0，即外部输入DIR控制无效时，可由该信号代替DIR输入，直接控制内部计数器的计数。当需要外部DIR输入，即DIREN为1时，该位应设置为0 (set direction)
1：内部DIR信号为高电平，则高8位计数器计数 相关函数 */
void BTx_CR2_STDIR_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR2  |= (BTx_CR2_STDIR_Msk);
	}
	else
	{
		BTx->CR2  &= ~(BTx_CR2_STDIR_Msk);
	}
}

FunState BTx_CR2_STDIR_Getable(BT_Type* BTx)
{
	if (BTx->CR2  & (BTx_CR2_STDIR_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 低位计数器计数使能控制选择信号 (source select)
1低位计数器计数使能端选则常使能。此时低位计数器计数不受DIR控制，可将正反向所有脉冲一并计数
0：低位计数器计数使能端选则由寄存器STDIR或外部EX_SIG2输入控制。 相关函数 */
void BTx_CR2_SRCSEL_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->CR2  |= (BTx_CR2_SRCSEL_Msk);
	}
	else
	{
		BTx->CR2  &= ~(BTx_CR2_SRCSEL_Msk);
	}
}

FunState BTx_CR2_SRCSEL_Getable(BT_Type* BTx)
{
	if (BTx->CR2  & (BTx_CR2_SRCSEL_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 输入信号2极性选择 (direction polarity)
1：对外部输入DIR信号EX_SIG2反向
0：对外部输入DIR信号EX_SIG2不反向 相关函数 */
void BTx_CR2_DIRPO_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CR2 ;
	tmpreg &= ~(BTx_CR2_DIRPO_Msk);
	tmpreg |= (SetValue & BTx_CR2_DIRPO_Msk);
	BTx->CR2  = tmpreg;
}

uint32_t BTx_CR2_DIRPO_Get(BT_Type* BTx)
{
	return (BTx->CR2  & BTx_CR2_DIRPO_Msk);
}

/* RTCOUT2信号选择控制2 (RTCOUT2 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU 相关函数 */
void BTx_CFGR1_RTCSEL2_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR1 ;
	tmpreg &= ~(BTx_CFGR1_RTCSEL2_Msk);
	tmpreg |= (SetValue & BTx_CFGR1_RTCSEL2_Msk);
	BTx->CFGR1  = tmpreg;
}

uint32_t BTx_CFGR1_RTCSEL2_Get(BT_Type* BTx)
{
	return (BTx->CFGR1  & BTx_CFGR1_RTCSEL2_Msk);
}

/* RTCOUT1信号选择控制1 (RTCOUT1 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU 相关函数 */
void BTx_CFGR1_RTCSEL1_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR1 ;
	tmpreg &= ~(BTx_CFGR1_RTCSEL1_Msk);
	tmpreg |= (SetValue & BTx_CFGR1_RTCSEL1_Msk);
	BTx->CFGR1  = tmpreg;
}

uint32_t BTx_CFGR1_RTCSEL1_Get(BT_Type* BTx)
{
	return (BTx->CFGR1  & BTx_CFGR1_RTCSEL1_Msk);
}

/* Group2信号选择控制 (Group2 source select)
00 = APBCLK
01 = RTCOUT2
10 = IN_SIG2，内部输入信号2
11 = EX_SIG2，外部输入信号2
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 相关函数 */
void BTx_CFGR1_GRP2SEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR1 ;
	tmpreg &= ~(BTx_CFGR1_GRP2SEL_Msk);
	tmpreg |= (SetValue & BTx_CFGR1_GRP2SEL_Msk);
	BTx->CFGR1  = tmpreg;
}

uint32_t BTx_CFGR1_GRP2SEL_Get(BT_Type* BTx)
{
	return (BTx->CFGR1  & BTx_CFGR1_GRP2SEL_Msk);
}

/* Group1信号选择控制（可作为捕捉模式下采样时钟选择，同时可作为信号捕捉源）(Group1 source select)
00 = APBCLK
01 = RTCOUT1
10 = IN_SIG1，内部输入信号1
11 = EX_SIG1，外部输入信号1
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 相关函数 */
void BTx_CFGR1_GRP1SEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR1 ;
	tmpreg &= ~(BTx_CFGR1_GRP1SEL_Msk);
	tmpreg |= (SetValue & BTx_CFGR1_GRP1SEL_Msk);
	BTx->CFGR1  = tmpreg;
}

uint32_t BTx_CFGR1_GRP1SEL_Get(BT_Type* BTx)
{
	return (BTx->CFGR1  & BTx_CFGR1_GRP1SEL_Msk);
}

/* 外部输入信号选择控制2 (external source select2)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 相关函数 */
void BTx_CFGR2_EXSEL2_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR2 ;
	tmpreg &= ~(BTx_CFGR2_EXSEL2_Msk);
	tmpreg |= (SetValue & BTx_CFGR2_EXSEL2_Msk);
	BTx->CFGR2  = tmpreg;
}

uint32_t BTx_CFGR2_EXSEL2_Get(BT_Type* BTx)
{
	return (BTx->CFGR2  & BTx_CFGR2_EXSEL2_Msk);
}

/* 外部输入信号选择控制1 (external source select1)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 相关函数 */
void BTx_CFGR2_EXSEL1_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR2 ;
	tmpreg &= ~(BTx_CFGR2_EXSEL1_Msk);
	tmpreg |= (SetValue & BTx_CFGR2_EXSEL1_Msk);
	BTx->CFGR2  = tmpreg;
}

uint32_t BTx_CFGR2_EXSEL1_Get(BT_Type* BTx)
{
	return (BTx->CFGR2  & BTx_CFGR2_EXSEL1_Msk);
}

/* 内部输入信号选择控制2 (internal source select 2)
00 = UART_RX3/UART_RX3
01 = UART_RX4/UART_RX4
10 = UART_RX5/UART_RX5
11 = RCLP/BT1_OUT 相关函数 */
void BTx_CFGR2_INSEL2_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR2 ;
	tmpreg &= ~(BTx_CFGR2_INSEL2_Msk);
	tmpreg |= (SetValue & BTx_CFGR2_INSEL2_Msk);
	BTx->CFGR2  = tmpreg;
}

uint32_t BTx_CFGR2_INSEL2_Get(BT_Type* BTx)
{
	return (BTx->CFGR2  & BTx_CFGR2_INSEL2_Msk);
}

/* 内部输入信号选择控制1 (internal source select 1)
00 = UART_RX0/UART_RX0
01 = UART_RX1/UART_RX1
10 = UART_RX2/UART_RX2
11 = RCLP/RCLP 相关函数 */
void BTx_CFGR2_INSEL1_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->CFGR2 ;
	tmpreg &= ~(BTx_CFGR2_INSEL1_Msk);
	tmpreg |= (SetValue & BTx_CFGR2_INSEL1_Msk);
	BTx->CFGR2  = tmpreg;
}

uint32_t BTx_CFGR2_INSEL1_Get(BT_Type* BTx)
{
	return (BTx->CFGR2  & BTx_CFGR2_INSEL1_Msk);
}

/* 输入Group1的预分频寄存器 (Group1 input signal prescaler)
分频数=（X+1），即00表示1分频，FF表示256分频。预分频后的信号都为单周期脉冲的形式，占空比1:X 相关函数 */
void BTx_PRES_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->PRES  = (SetValue & BTx_PRES_PRESCALE_Msk);
}

uint32_t BTx_PRES_Read(BT_Type* BTx)
{
	return (BTx->PRES  & BTx_PRES_PRESCALE_Msk);
}

/* 高位加载控制 (Counter highend load enable)
写1将预置数寄存器ET1PRESETH和加载寄存器ET1LOADH分别加载到计数值寄存器ET1CNTH和比较寄存器ET1CMPH，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 相关函数 */
void BTx_LOADCR_LHEN_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->LOADCR |= (BTx_LOADCR_LHEN_Msk);
	}
	else
	{
		BTx->LOADCR &= ~(BTx_LOADCR_LHEN_Msk);
	}
}

FunState BTx_LOADCR_LHEN_Getable(BT_Type* BTx)
{
	if (BTx->LOADCR & (BTx_LOADCR_LHEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 低位加载控制 (Counter lowend load enable)
写1将预置数寄存器PRESETL和加载寄存器LOADL分别加载到计数值寄存器ET1CNTL和比较寄存器ET1CMPL，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 相关函数 */
void BTx_LOADCR_LLEN_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->LOADCR |= (BTx_LOADCR_LLEN_Msk);
	}
	else
	{
		BTx->LOADCR &= ~(BTx_LOADCR_LLEN_Msk);
	}
}

FunState BTx_LOADCR_LLEN_Getable(BT_Type* BTx)
{
	if (BTx->LOADCR & (BTx_LOADCR_LLEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 计数器低位计数值寄存器 (counter lowend)
LLEN有效时加载预置数到该寄存器。 相关函数 */
uint32_t BTx_CNTL_Read(BT_Type* BTx)
{
	return (BTx->CNTL  & BTx_CNTL_CNTL_Msk);
}

/* 计数器高位计数值寄存器 (counter highend)
LHEN有效时加载预置数到该寄存器。 相关函数 */
uint32_t BTx_CNTH_Read(BT_Type* BTx)
{
	return (BTx->CNTH  & BTx_CNTH_CNTH_Msk);
}

/* 计数器高位预置数寄存器 (preset highend)
用于保存高位计数器初值，或保存捕捉结果高8bit 相关函数 */
void BTx_PRESET_PRESETH_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->PRESET ;
	tmpreg &= ~(BTx_PRESET_PRESETH_Msk);
	tmpreg |= (SetValue & BTx_PRESET_PRESETH_Msk);
	BTx->PRESET  = tmpreg;
}

uint32_t BTx_PRESET_PRESETH_Get(BT_Type* BTx)
{
	return (BTx->PRESET  & BTx_PRESET_PRESETH_Msk);
}

/* 计数器低位预置数寄存器 (preset lowend)
用于保存低位计数器初值，或保存捕捉结果低8bit 相关函数 */
void BTx_PRESET_PRESETL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->PRESET ;
	tmpreg &= ~(BTx_PRESET_PRESETL_Msk);
	tmpreg |= (SetValue & BTx_PRESET_PRESETL_Msk);
	BTx->PRESET  = tmpreg;
}

uint32_t BTx_PRESET_PRESETL_Get(BT_Type* BTx)
{
	return (BTx->PRESET  & BTx_PRESET_PRESETL_Msk);
}

/* 计数器低位加载寄存器 (load lowend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。 相关函数 */
void BTx_LOADL_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->LOADL  = (SetValue & BTx_LOADL_LOADL_Msk);
}

uint32_t BTx_LOADL_Read(BT_Type* BTx)
{
	return (BTx->LOADL  & BTx_LOADL_LOADL_Msk);
}

/* 计数器高位加载寄存器 (load highend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。当工作在8位定时/计数器模式时，该高位加载寄存器不支持加载值为0x00的设置。 相关函数 */
void BTx_LOADH_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->LOADH  = (SetValue & BTx_LOADH_LOADH_Msk);
}

uint32_t BTx_LOADH_Read(BT_Type* BTx)
{
	return (BTx->LOADH  & BTx_LOADH_LOADH_Msk);
}

/* 计数器低位比较寄存器 (compare lowend )
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
void BTx_CMPL_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->CMPL  = (SetValue & BTx_CMPL_CMPL_Msk);
}

uint32_t BTx_CMPL_Read(BT_Type* BTx)
{
	return (BTx->CMPL  & BTx_CMPL_CMPL_Msk);
}

/* 计数器高位比较寄存器 (compare highend)
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
void BTx_CMPH_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->CMPH  = (SetValue & BTx_CMPH_CMPH_Msk);
}

uint32_t BTx_CMPH_Read(BT_Type* BTx)
{
	return (BTx->CMPH  & BTx_CMPH_CMPH_Msk);
}

/* 计数器输出脉冲宽度计数器 (output pulse width counter)
该寄存器用于调整输出脉冲宽度。计数时钟为32768Hz，对应的输出脉冲宽度范围为30.5uS~125mS。输出脉冲宽度=(OUTCNT+1)/32768秒 相关函数 */
void BTx_OUTCNT_Write(BT_Type* BTx, uint32_t SetValue)
{
	BTx->OUTCNT  = (SetValue & BTx_OUTCNT_OUTCNT_Msk);
}

uint32_t BTx_OUTCNT_Read(BT_Type* BTx)
{
	return (BTx->OUTCNT  & BTx_OUTCNT_OUTCNT_Msk);
}

void BTx_OCR_OUTCLR_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->OCR;
	tmpreg &= ~(BTx_OCR_OUTCLR_Msk);
	tmpreg |= (SetValue & BTx_OCR_OUTCLR_Msk);
	BTx->OCR = tmpreg;
}

uint32_t BTx_OCR_OUTCLR_Get(BT_Type* BTx)
{
	return (BTx->OCR & BTx_OCR_OUTCLR_Msk);
}

void BTx_OCR_OUTINV_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->OCR;
	tmpreg &= ~(BTx_OCR_OUTINV_Msk);
	tmpreg |= (SetValue & BTx_OCR_OUTINV_Msk);
	BTx->OCR = tmpreg;
}

uint32_t BTx_OCR_OUTINV_Get(BT_Type* BTx)
{
	return (BTx->OCR & BTx_OCR_OUTINV_Msk);
}

void BTx_OCR_OUTMOD_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->OCR;
	tmpreg &= ~(BTx_OCR_OUTMOD_Msk);
	tmpreg |= (SetValue & BTx_OCR_OUTMOD_Msk);
	BTx->OCR = tmpreg;
}

uint32_t BTx_OCR_OUTMOD_Get(BT_Type* BTx)
{
	return (BTx->OCR & BTx_OCR_OUTMOD_Msk);
}

void BTx_OCR_OUTSEL_Set(BT_Type* BTx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BTx->OCR;
	tmpreg &= ~(BTx_OCR_OUTSEL_Msk);
	tmpreg |= (SetValue & BTx_OCR_OUTSEL_Msk);
	BTx->OCR = tmpreg;
}

uint32_t BTx_OCR_OUTSEL_Get(BT_Type* BTx)
{
	return (BTx->OCR & BTx_OCR_OUTSEL_Msk);
}

/* 扩展定时器高位比较发生信号 (compare highend interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
void BTx_IER_CMPHIE_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->IER |= (BTx_IER_CMPHIE_Msk);
	}
	else
	{
		BTx->IER &= ~(BTx_IER_CMPHIE_Msk);
	}
}

FunState BTx_IER_CMPHIE_Getable(BT_Type* BTx)
{
	if (BTx->IER & (BTx_IER_CMPHIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 扩展定时器低位比较发生信号 (compare lowend interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
void BTx_IER_CMPLIE_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->IER |= (BTx_IER_CMPLIE_Msk);
	}
	else
	{
		BTx->IER &= ~(BTx_IER_CMPLIE_Msk);
	}
}

FunState BTx_IER_CMPLIE_Getable(BT_Type* BTx)
{
	if (BTx->IER & (BTx_IER_CMPLIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 定时器高位溢出信号 (highend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
void BTx_IER_OVHIE_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->IER |= (BTx_IER_OVHIE_Msk);
	}
	else
	{
		BTx->IER &= ~(BTx_IER_OVHIE_Msk);
	}
}

FunState BTx_IER_OVHIE_Getable(BT_Type* BTx)
{
	if (BTx->IER & (BTx_IER_OVHIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 定时器低位溢出信号 (lowend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
void BTx_IER_OVLIE_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->IER |= (BTx_IER_OVLIE_Msk);
	}
	else
	{
		BTx->IER &= ~(BTx_IER_OVLIE_Msk);
	}
}

FunState BTx_IER_OVLIE_Getable(BT_Type* BTx)
{
	if (BTx->IER & (BTx_IER_OVLIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 定时器捕捉产生信号 (capture interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
void BTx_IER_CAPIE_Setable(BT_Type* BTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		BTx->IER |= (BTx_IER_CAPIE_Msk);
	}
	else
	{
		BTx->IER &= ~(BTx_IER_CAPIE_Msk);
	}
}

FunState BTx_IER_CAPIE_Getable(BT_Type* BTx)
{
	if (BTx->IER & (BTx_IER_CAPIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 定时器高位比较发生信号 (compare highend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 相关函数 */
void BTx_ISR_CMPHIF_Clr(BT_Type* BTx)
{
	BTx->ISR = BTx_ISR_CMPHIF_Msk;
}

FlagStatus BTx_ISR_CMPHIF_Chk(BT_Type* BTx)
{
	if (BTx->ISR & BTx_ISR_CMPHIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 定时器低位比较发生信号 (compare lowend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 相关函数 */
void BTx_ISR_CMPLIF_Clr(BT_Type* BTx)
{
	BTx->ISR = BTx_ISR_CMPLIF_Msk;
}

FlagStatus BTx_ISR_CMPLIF_Chk(BT_Type* BTx)
{
	if (BTx->ISR & BTx_ISR_CMPLIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 定时器高位溢出信号 (highend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
void BTx_ISR_OVHIF_Clr(BT_Type* BTx)
{
	BTx->ISR = BTx_ISR_OVHIF_Msk;
}

FlagStatus BTx_ISR_OVHIF_Chk(BT_Type* BTx)
{
	if (BTx->ISR & BTx_ISR_OVHIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 定时器低位溢出信号 (lowend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
void BTx_ISR_OVLIF_Clr(BT_Type* BTx)
{
	BTx->ISR = BTx_ISR_OVLIF_Msk;
}

FlagStatus BTx_ISR_OVLIF_Chk(BT_Type* BTx)
{
	if (BTx->ISR & BTx_ISR_OVLIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 定时器捕捉产生信号 (capture interrupt flag)
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 相关函数 */
void BTx_ISR_CAPIF_Clr(BT_Type* BTx)
{
	BTx->ISR = BTx_ISR_CAPIF_Msk;
}

FlagStatus BTx_ISR_CAPIF_Chk(BT_Type* BTx)
{
	if (BTx->ISR & BTx_ISR_CAPIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void BTx_Deinit(BT_Type* BTx)
{
	//BTx->CR1  = 0x00000000;
	//BTx->CR2  = 0x00000000;
	//BTx->CFGR1  = 0x00000000;
	//BTx->CFGR2  = 0x00000000;
	//BTx->PRES  = 0x00000000;
	//BTx->LOADCR = 0x00000000;
	//BTx->CNTL  = 0x00000000;
	//BTx->CNTH  = 0x00000000;
	//BTx->PRESET  = 0x00000000;
	//BTx->LOADL  = 0x00000000;
	//BTx->LOADH  = 0x00000000;
	//BTx->CMPL  = 0x00000000;
	//BTx->CMPH  = 0x00000000;
	//BTx->OUTCNT  = 0x00000000;
	//BTx->OCR = ;
	//BTx->IER = 0x00000000;
	//BTx->ISR = 0x00000000;
}
/******END OF FILE****/
