/**
  ******************************************************************************
  * @file    fm33a0xxev_rmu.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_rmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup RMU 
  * @brief RMU driver modules
  * @{
  */ 

/* 下电复位电压配置00—1.5V01—1.25V（禁止使用）10—1.35V11—1.4V（默认） 相关函数 */
void RMU_PDRCR_PDRCFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RMU->PDRCR;
	tmpreg &= ~(RMU_PDRCR_PDRCFG_Msk);
	tmpreg |= (SetValue & RMU_PDRCR_PDRCFG_Msk);
	RMU->PDRCR = tmpreg;
}

uint32_t RMU_PDRCR_PDRCFG_Get(void)
{
	return (RMU->PDRCR & RMU_PDRCR_PDRCFG_Msk);
}

/* 下电复位使能0：关闭下电复位1：使能下电复位 相关函数 */
void RMU_PDRCR_PDREN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->PDRCR |= (RMU_PDRCR_PDREN_Msk);
	}
	else
	{
		RMU->PDRCR &= ~(RMU_PDRCR_PDREN_Msk);
	}
}

FunState RMU_PDRCR_PDREN_Getable(void)
{
	if (RMU->PDRCR & (RMU_PDRCR_PDREN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 下电复位电压配置00—1.7V01—1.6V（默认）10—1.65V11—1.75V 相关函数 */
void RMU_BORCR_BOR_PDRCFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RMU->BORCR;
	tmpreg &= ~(RMU_BORCR_BOR_PDRCFG_Msk);
	tmpreg |= (SetValue & RMU_BORCR_BOR_PDRCFG_Msk);
	RMU->BORCR = tmpreg;
}

uint32_t RMU_BORCR_BOR_PDRCFG_Get(void)
{
	return (RMU->BORCR & RMU_BORCR_BOR_PDRCFG_Msk);
}

/* BOR使能控制寄存器0：使能BOR1：关闭BOR 相关函数 */
void RMU_BORCR_OFF_BOR_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
    RMU->BORCR |= (RMU_BORCR_OFF_BOR_Msk);
	}
	else
	{
		RMU->BORCR &= ~(RMU_BORCR_OFF_BOR_Msk);
	}
}

FunState RMU_BORCR_OFF_BOR_Getable(void)
{
	if (RMU->BORCR & (RMU_BORCR_OFF_BOR_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LOCKUP复位使能1：使能SC000 LOCKUP复位0：屏蔽SC000 LOCKUP复位 相关函数 */
void RMU_RSTCFGR_LKUPRST_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->RSTCFGR |= (RMU_RSTCFGR_LKUPRST_EN_Msk);
	}
	else
	{
		RMU->RSTCFGR &= ~(RMU_RSTCFGR_LKUPRST_EN_Msk);
	}
}

FunState RMU_RSTCFGR_LKUPRST_EN_Getable(void)
{
	if (RMU->RSTCFGR & (RMU_RSTCFGR_LKUPRST_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 软件写0x5C5C_AABB触发全局复位 相关函数 */
void RMU_SOFTRST_Write(uint32_t SetValue)
{
	RMU->SOFTRST = (SetValue);
}

/* NRST引脚复位标志，高有效 相关函数 */
void RMU_RSR_NRSTN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_NRSTN_FLAG_Msk;
}

FlagStatus RMU_RSR_NRSTN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_NRSTN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* TESTN引脚复位标志，高有效 相关函数 */
void RMU_RSR_TESTN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_TESTN_FLAG_Msk;
}

FlagStatus RMU_RSR_TESTN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_TESTN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 上电复位标志，高有效 相关函数 */
void RMU_RSR_PORN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_PORN_FLAG_Msk;
}

FlagStatus RMU_RSR_PORN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_PORN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 下电复位标志，高有效 相关函数 */
void RMU_RSR_PDRN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_PDRN_FLAG_Msk;
}

FlagStatus RMU_RSR_PDRN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_PDRN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 软件复位标志，高有效 相关函数 */
void RMU_RSR_SOFTN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_SOFTN_FLAG_Msk;
}

FlagStatus RMU_RSR_SOFTN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_SOFTN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* IWDT复位标志，高有效 相关函数 */
void RMU_RSR_IWDTN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_IWDTN_FLAG_Msk;
}

FlagStatus RMU_RSR_IWDTN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_IWDTN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* WWDT复位标志，高有效 相关函数 */
void RMU_RSR_WWDTN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_WWDTN_FLAG_Msk;
}

FlagStatus RMU_RSR_WWDTN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_WWDTN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LOOKUP复位标志，高有效 相关函数 */
void RMU_RSR_LKUPN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_LKUPN_FLAG_Msk;
}

FlagStatus RMU_RSR_LKUPN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_LKUPN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* NVIC复位标志，高有效 相关函数 */
void RMU_RSR_NVICN_FLAG_Clr(void)
{
	RMU->RSR = RMU_RSR_NVICN_FLAG_Msk;
}

FlagStatus RMU_RSR_NVICN_FLAG_Chk(void)
{
	if (RMU->RSR & RMU_RSR_NVICN_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 外设模块复位使能，32bit虚寄存器，只写软件对此地址写0x1357_9BDF，使能外设复位功能，此后可以通过外设模块复位寄存器复位各个模块软件对此地址写任意其他数据，将关闭外设复位功能 相关函数 */
void RMU_PRSTEN_Write(uint32_t SetValue)
{
	RMU->PRSTEN = (SetValue);
}

/* HASH模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_AHBRST_HASHRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->AHBRST |= (RMU_AHBRST_HASHRST_Msk);
	}
	else
	{
		RMU->AHBRST &= ~(RMU_AHBRST_HASHRST_Msk);
	}
}

FunState RMU_AHBRST_HASHRST_Getable(void)
{
	if (RMU->AHBRST & (RMU_AHBRST_HASHRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* PAE模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_AHBRST_PAERST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->AHBRST |= (RMU_AHBRST_PAERST_Msk);
	}
	else
	{
		RMU->AHBRST &= ~(RMU_AHBRST_PAERST_Msk);
	}
}

FunState RMU_AHBRST_PAERST_Getable(void)
{
	if (RMU->AHBRST & (RMU_AHBRST_PAERST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* DMA模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_AHBRST_DMARST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->AHBRST |= (RMU_AHBRST_DMARST_Msk);
	}
	else
	{
		RMU->AHBRST &= ~(RMU_AHBRST_DMARST_Msk);
	}
}

FunState RMU_AHBRST_DMARST_Getable(void)
{
	if (RMU->AHBRST & (RMU_AHBRST_DMARST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART5模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_UART5RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_UART5RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_UART5RST_Msk);
	}
}

FunState RMU_APBRST1_UART5RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_UART5RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART4模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_UART4RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_UART4RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_UART4RST_Msk);
	}
}

FunState RMU_APBRST1_UART4RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_UART4RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_UART3RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_UART3RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_UART3RST_Msk);
	}
}

FunState RMU_APBRST1_UART3RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_UART3RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_UART2RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_UART2RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_UART2RST_Msk);
	}
}

FunState RMU_APBRST1_UART2RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_UART2RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Timer array复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_TIMARST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_TIMARST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_TIMARST_Msk);
	}
}

FunState RMU_APBRST1_TIMARST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_TIMARST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LCD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_LCDRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_LCDRST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_LCDRST_Msk);
	}
}

FunState RMU_APBRST1_LCDRST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_LCDRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_U7816RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_U7816RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_U7816RST_Msk);
	}
}

FunState RMU_APBRST1_U7816RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_U7816RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_SPI4RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_SPI4RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_SPI4RST_Msk);
	}
}

FunState RMU_APBRST1_SPI4RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_SPI4RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_SPI3RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_SPI3RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_SPI3RST_Msk);
	}
}

FunState RMU_APBRST1_SPI3RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_SPI3RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_SPI2RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_SPI2RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_SPI2RST_Msk);
	}
}

FunState RMU_APBRST1_SPI2RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_SPI2RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* EUART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_LPUART0RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_LPUART0RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_LPUART0RST_Msk);
	}
}

FunState RMU_APBRST1_LPUART0RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_LPUART0RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_I2C1RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_I2C1RST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_I2C1RST_Msk);
	}
}

FunState RMU_APBRST1_I2C1RST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_I2C1RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/*  _32模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST1_LPTRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST1 |= (RMU_APBRST1_LPTRST_Msk);
	}
	else
	{
		RMU->APBRST1 &= ~(RMU_APBRST1_LPTRST_Msk);
	}
}

FunState RMU_APBRST1_LPTRST_Getable(void)
{
	if (RMU->APBRST1 & (RMU_APBRST1_LPTRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_UART1RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_UART1RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_UART1RST_Msk);
	}
}

FunState RMU_APBRST2_UART1RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_UART1RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_UART0RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_UART0RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_UART0RST_Msk);
	}
}

FunState RMU_APBRST2_UART0RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_UART0RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* UART红外模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_UARTIRRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_UARTIRRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_UARTIRRST_Msk);
	}
}

FunState RMU_APBRST2_UARTIRRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_UARTIRRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* BSTIM模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_BSTRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_BSTRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_BSTRST_Msk);
	}
}

FunState RMU_APBRST2_BSTRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_BSTRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CIC滤波模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_CICRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_CICRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_CICRST_Msk);
	}
}

FunState RMU_APBRST2_CICRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_CICRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_ADCRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_ADCRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_ADCRST_Msk);
	}
}

FunState RMU_APBRST2_ADCRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_ADCRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* AES模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_AESRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_AESRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_AESRST_Msk);
	}
}

FunState RMU_APBRST2_AESRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_AESRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CRC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_CRCRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_CRCRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_CRCRST_Msk);
	}
}

FunState RMU_APBRST2_CRCRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_CRCRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RNG模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_RNGRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_RNGRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_RNGRST_Msk);
	}
}

FunState RMU_APBRST2_RNGRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_RNGRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_SPI1RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_SPI1RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_SPI1RST_Msk);
	}
}

FunState RMU_APBRST2_SPI1RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_SPI1RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_SPI0RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_SPI0RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_SPI0RST_Msk);
	}
}

FunState RMU_APBRST2_SPI0RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_SPI0RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* EUART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_LPUART1RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_LPUART1RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_LPUART1RST_Msk);
	}
}

FunState RMU_APBRST2_LPUART1RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_LPUART1RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_I2C0RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_I2C0RST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_I2C0RST_Msk);
	}
}

FunState RMU_APBRST2_I2C0RST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_I2C0RST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SVD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_SVDRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_SVDRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_SVDRST_Msk);
	}
}

FunState RMU_APBRST2_SVDRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_SVDRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 比较器模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
void RMU_APBRST2_COMPRST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RMU->APBRST2 |= (RMU_APBRST2_COMPRST_Msk);
	}
	else
	{
		RMU->APBRST2 &= ~(RMU_APBRST2_COMPRST_Msk);
	}
}

FunState RMU_APBRST2_COMPRST_Getable(void)
{
	if (RMU->APBRST2 & (RMU_APBRST2_COMPRST_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


void RMU_Deinit(void)
{
	//RMU->PDRCR = 0x00000007;
	//RMU->BORCR = 0x00000002;
	//RMU->RSTCFGR = 0x00000000;
	//RMU->SOFTRST = 0x00000000;
	//RMU->RSR = 0x00000000;
	//RMU->PRSTEN = 0x00000000;
	//RMU->AHBRST = 0x00000000;
	//RMU->APBRST1 = 0x00000000;
	//RMU->APBRST2 = 0x00000000;
}

/******END OF FILE****/
