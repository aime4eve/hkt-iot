/**
  ******************************************************************************
  * @file    fm33a0xxev_bstim.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_bstim.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup BSTIM 
  * @brief BSTIM driver modules
  * @{
  */ 
  
/* Auto-reload预装载使能 (Auto-Reload Preload Enable)
0：ARR寄存器不使能preload
1：ARR寄存器使能preload 相关函数 */
void BSTIM_CR1_ARPE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		BSTIM->CR1 |= (BSTIM_CR1_ARPE_Msk);
	}
	else
	{
		BSTIM->CR1 &= ~(BSTIM_CR1_ARPE_Msk);
	}
}

FunState BSTIM_CR1_ARPE_Getable(void)
{
	if (BSTIM->CR1 & (BSTIM_CR1_ARPE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 单脉冲输出模式 (One Pulse Mode)
0：Update Event发生时计数器不停止
1：Update Event发生时计数器停止（自动清零CEN） 相关函数 */
void BSTIM_CR1_OPM_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BSTIM->CR1;
	tmpreg &= ~(BSTIM_CR1_OPM_Msk);
	tmpreg |= (SetValue & BSTIM_CR1_OPM_Msk);
	BSTIM->CR1 = tmpreg;
}

uint32_t BSTIM_CR1_OPM_Get(void)
{
	return (BSTIM->CR1 & BSTIM_CR1_OPM_Msk);
}

/* 更新请求选择 (Update Request Select)
0：以下事件能够产生update中断
计数器上溢出或下溢出
软件置位UG寄存器
从机控制器产生update
1：仅计数器上溢出或下溢出会产生update中断或DMA请求 相关函数 */
void BSTIM_CR1_URS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = BSTIM->CR1;
	tmpreg &= ~(BSTIM_CR1_URS_Msk);
	tmpreg |= (SetValue & BSTIM_CR1_URS_Msk);
	BSTIM->CR1 = tmpreg;
}

uint32_t BSTIM_CR1_URS_Get(void)
{
	return (BSTIM->CR1 & BSTIM_CR1_URS_Msk);
}

/* 禁止update (Update Disable)
0：使能update事件；以下事件发生时产生update事件
计数器上溢出或下溢出
软件置位UG寄存器
从机控制器产生update
1：禁止update事件，不更新shadow寄存器。当UG置位或从机控制器收到硬件reset时重新初始化计数器和预分频器。 相关函数 */
void BSTIM_CR1_UDIS_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		BSTIM->CR1 |= (BSTIM_CR1_UDIS_Msk);
	}
	else
	{
		BSTIM->CR1 &= ~(BSTIM_CR1_UDIS_Msk);
	}
}

FunState BSTIM_CR1_UDIS_Getable(void)
{
	if (BSTIM->CR1 & (BSTIM_CR1_UDIS_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 计数器使能 (Counter Enable)
0：计数器关闭
1：计数器使能
注意：外部触发模式可以自动置位CEN 相关函数 */
void BSTIM_CR1_CEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		BSTIM->CR1 |= (BSTIM_CR1_CEN_Msk);
	}
	else
	{
		BSTIM->CR1 &= ~(BSTIM_CR1_CEN_Msk);
	}
}

FunState BSTIM_CR1_CEN_Getable(void)
{
	if (BSTIM->CR1 & (BSTIM_CR1_CEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 主机模式选择，用于配置主机模式下向从机发送的同步触发信号（TRGO）源 (Master Mode Select)
000：BSTIM_EGR的UG寄存器被用作TRGO
001：计数器使能信号CNT_EN被用作TRGO，可用于同时启动多个定时器
010：UE（update event）信号被用作TRGO
011/100/111：RFU

注意：从机定时器或ADC必须事先使能工作时钟，才能接收主机定时器发送的TRGO 相关函数 */
void BSTIM_CR2_Write(uint32_t SetValue)
{
	BSTIM->CR2 = (SetValue & BSTIM_CR2_MMS_Msk);
}

uint32_t BSTIM_CR2_Read(void)
{
	return (BSTIM->CR2 & BSTIM_CR2_MMS_Msk);
}

/* Update事件中断使能 (Update event Interrupt Enable)
0：禁止Update事件中断
1：允许Update事件中断 相关函数 */
void BSTIM_IER_UIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		BSTIM->IER |= (BSTIM_IER_UIE_Msk);
	}
	else
	{
		BSTIM->IER &= ~(BSTIM_IER_UIE_Msk);
	}
}

FunState BSTIM_IER_UIE_Getable(void)
{
	if (BSTIM->IER & (BSTIM_IER_UIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Update事件中断标志，硬件置位，软件写1清零。(Update event Interrupt Flag,write 1 to flag)
当以下事件发生时，UIF置位，并更新shadow寄存器
-重复计数器=0，并且UDIS=0的情况下，计数器发生溢出
-URS=0且UDIS=0的情况下，软件置位UG寄存器初始化计数器
-URS=0且UDIS=0的情况下，触发事件初始化计数器 相关函数 */
void BSTIM_ISR_UIF_Clr(void)
{
	BSTIM->ISR = BSTIM_ISR_UIF_Msk;
}

FlagStatus BSTIM_ISR_UIF_Chk(void)
{
	if (BSTIM->ISR & BSTIM_ISR_UIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 软件Update事件，软件置位此寄存器产生Update事件，硬件自动清零 (User Generate)
软件置位UG时会重新初始化计数器并更新shadow寄存器，预分频计数器被清零。 相关函数 */
void BSTIM_EGR_UG_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		BSTIM->EGR |= (BSTIM_EGR_UG_Msk);
	}
	else
	{
		BSTIM->EGR &= ~(BSTIM_EGR_UG_Msk);
	}
}

/* 计数器值 (Counter) 相关函数 */
void BSTIM_CNTR_Write(uint32_t SetValue)
{
	BSTIM->CNTR = (SetValue);
}

uint32_t BSTIM_CNTR_Read(void)
{
	return (BSTIM->CNTR);
}

/* 计数器时钟（CK_CNT）预分频值 (Counter Clock Prescaler)
fCK_CNT=fCK_PSC/(PSC[15:0]+1)
这是一个preload寄存器，在update事件发生时其内容被载入shadow寄存器 相关函数 */
void BSTIM_PSCR_Write(uint32_t SetValue)
{
	BSTIM->PSCR = (SetValue);
}

uint32_t BSTIM_PSCR_Read(void)
{
	return (BSTIM->PSCR);
}

/* 计数溢出时的自动重载值 (Auto-Reload Register)
这是一个preload寄存器，在update事件发生时其内容被载入shadow寄存器 相关函数 */
void BSTIM_ARR_Write(uint32_t SetValue)
{
	BSTIM->ARR = (SetValue);
}

uint32_t BSTIM_ARR_Read(void)
{
	return (BSTIM->ARR);
}


void BSTIM_Deinit(void)
{
	//BSTIM->CR1 = 0x00000000;
	//BSTIM->CR2 = 0x00000000;
	//BSTIM->IER = 0x00000000;
	//BSTIM->ISR = 0x00000000;
	//BSTIM->EGR = 0x00000000;
	//BSTIM->CNTR = 0x00000000;
	//BSTIM->PSCR = 0x00000000;
	//BSTIM->ARR = 0x00000000;
}

/******END OF FILE****/
