/**
  ******************************************************************************
  * @file    fm33a0xxev_vrtc.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_vrtc.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup VRTC 
  * @brief VRTC driver modules
  * @{
  */ 

/* RCMF使能寄存器
0：关闭RCMF
1：使能RCMF
【注】当使能了RTC自动温补后，RCMF受硬件控制自行启动和关闭 相关函数 */
void VRTC_RCMFCR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		VRTC->RCMFCR |= (VRTC_RCMFCR_EN_Msk);
	}
	else
	{
		VRTC->RCMFCR &= ~(VRTC_RCMFCR_EN_Msk);
	}
}

FunState VRTC_RCMFCR_EN_Getable(void)
{
	if (VRTC->RCMFCR & (VRTC_RCMFCR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RCMF频率调校寄存器 相关函数 */
void VRTC_RCMFCR_TRIM_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = VRTC->RCMFCR;
	tmpreg &= ~(VRTC_RCMFCR_TRIM_Msk);
	tmpreg |= (SetValue & VRTC_RCMFCR_TRIM_Msk);
	VRTC->RCMFCR = tmpreg;
}

uint32_t VRTC_RCMFCR_TRIM_Get(void)
{
	return (VRTC->RCMFCR & VRTC_RCMFCR_TRIM_Msk);
}

/* Chopper使能 相关函数 */
void VRTC_RCLPCR_CHOPEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		VRTC->RCLPCR |= (VRTC_RCLPCR_CHOPEN_Msk);
	}
	else
	{
		VRTC->RCLPCR &= ~(VRTC_RCLPCR_CHOPEN_Msk);
	}
}

FunState VRTC_RCLPCR_CHOPEN_Getable(void)
{
	if (VRTC->RCLPCR & (VRTC_RCLPCR_CHOPEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RCLP使能信号
0：使能RCLP 
1：关闭RCLP
【注】XTLF异常停振时，自动使能RCLP 相关函数 */
void VRTC_RCLPCR_RCLP_OFF_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
    VRTC->RCLPCR |= (VRTC_RCLPCR_RCLP_OFF_Msk);
	}
	else
	{
		VRTC->RCLPCR &= ~(VRTC_RCLPCR_RCLP_OFF_Msk);
	}
}

FunState VRTC_RCLPCR_RCLP_OFF_Getable(void)
{
	if (VRTC->RCLPCR & (VRTC_RCLPCR_RCLP_OFF_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RCLP调校值寄存器
0000 0000：频率最低
1111 1111：频率最高 相关函数 */
void VRTC_RCLPTR_Write(uint32_t SetValue)
{
	VRTC->RCLPTR = (SetValue & VRTC_RCLPTR_RCLP_TRIM_Msk);
}

uint32_t VRTC_RCLPTR_Read(void)
{
	return (VRTC->RCLPTR & VRTC_RCLPTR_RCLP_TRIM_Msk);
}

void VRTC_XTLFCR_XTLFIPW_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = VRTC->XTLFCR;
	tmpreg &= ~(VRTC_XTLFCR_XTLFIPW_Msk);
	tmpreg |= (SetValue & VRTC_XTLFCR_XTLFIPW_Msk);
	VRTC->XTLFCR = tmpreg;
}

uint32_t VRTC_XTLFCR_XTLFIPW_Get(void)
{
	return (VRTC->XTLFCR & VRTC_XTLFCR_XTLFIPW_Msk);
}

/* ADC工作时钟选择寄存器
000：RCMF
001：RCMF/2
010：RCMF/4
011：RCMF/8
100：RCMF/16
101：RCMF/32
110：RCLP
111：XTLF 相关函数 */
void VRTC_ADCCR_CKS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = VRTC->ADCCR;
	tmpreg &= ~(VRTC_ADCCR_CKS_Msk);
	tmpreg |= (SetValue & VRTC_ADCCR_CKS_Msk);
	VRTC->ADCCR = tmpreg;
}

uint32_t VRTC_ADCCR_CKS_Get(void)
{
	return (VRTC->ADCCR & VRTC_ADCCR_CKS_Msk);
}

/* ADC工作时钟使能
0：关闭ADC工作时钟
1：使能ADC工作时钟 相关函数 */
void VRTC_ADCCR_CKE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		VRTC->ADCCR |= (VRTC_ADCCR_CKE_Msk);
	}
	else
	{
		VRTC->ADCCR &= ~(VRTC_ADCCR_CKE_Msk);
	}
}

FunState VRTC_ADCCR_CKE_Getable(void)
{
	if (VRTC->ADCCR & (VRTC_ADCCR_CKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* XTLF低频检测报警中断使能，1有效 相关函数 */
void VRTC_LFDIER_LFDET_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		VRTC->LFDIER |= (VRTC_LFDIER_LFDET_IE_Msk);
	}
	else
	{
		VRTC->LFDIER &= ~(VRTC_LFDIER_LFDET_IE_Msk);
	}
}

FunState VRTC_LFDIER_LFDET_IE_Getable(void)
{
	if (VRTC->LFDIER & (VRTC_LFDIER_LFDET_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 低频停振检测状态输出
1：XTLF正常
0：XTLF停振 相关函数 */
FlagStatus VRTC_LFDISR_LFDETO_Chk(void)
{
	if (VRTC->LFDISR & VRTC_LFDISR_LFDETO_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 低频停振检测中断标志寄存器，XTLF停振时硬件异步置位，软件写1清零；只有在LFDETO不为0的情况下才能够清除此寄存器 相关函数 */
void VRTC_LFDISR_LFDET_IF_Clr(void)
{
	VRTC->LFDISR = VRTC_LFDISR_LFDET_IF_Msk;
}

FlagStatus VRTC_LFDISR_LFDET_IF_Chk(void)
{
	if (VRTC->LFDISR & VRTC_LFDISR_LFDET_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/*
rcmf校准值载入函数
功能:rcmf校准值载入
输入：要设置的RCMF频率
*/
void VRTC_Init_RCMF_Trim(void)
{
	uint32_t Temp32;
	
	Temp32 = const_rcmf_Trim;
	
	if( ((Temp32>>16)&0x0000FFFF) == ((~Temp32)&0x0000FFFF) )	//正反码校验判断
	{
		VRTC_RCMFCR_TRIM_Set(Temp32&0x000000FF);
	}	
	else
	{
		VRTC_RCMFCR_TRIM_Set(0x0000001F);
	}
}

void VRTC_Deinit(void)
{
	//VRTC->RCMFCR = 0x00000040;
	//VRTC->RCLPCR = 0x00000000;
	//VRTC->RCLPTR = 0x00000000;
	//VRTC->XTLFCR = ;
	//VRTC->ADCCR = 0x00000004;
	//VRTC->LFDIER = 0x00000000;
	//VRTC->LFDISR = 0x00000000;
}

/******END OF FILE****/
