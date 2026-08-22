/**
  ******************************************************************************
  * @file    fm33a0xxev_lcd.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_lcd.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup LCD 
  * @brief LCD driver modules
  * @{
  */ 

/* 电容驱动模式，驱动频率
000：频率为帧频*COM数
001：频率为LSCLK/8
010：频率为LSCLK/16
011：频率为LSCLK/32
100：频率为LSCLK/64
101：频率为LSCLK/128
110：频率为LSCLK/256
111：频率为LSCLK/512相关函数 */
void LCD_CR_SCSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_SCSEL_Msk);
	tmpreg |= (SetValue & LCD_CR_SCSEL_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_SCSEL_Get(void)
{
	return (LCD->CR & LCD_CR_SCSEL_Msk);
}

/* 电容驱动模式，驱动方式控制 
00：单次驱动
01：连续驱动2次
10：连续驱动4次
11：多次驱动 相关函数 */
void LCD_CR_SC_CTRL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_SC_CTRL_Msk);
	tmpreg |= (SetValue & LCD_CR_SC_CTRL_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_SC_CTRL_Get(void)
{
	return (LCD->CR & LCD_CR_SC_CTRL_Msk);
}

/* 偏置电路输入电流源大小控制 (Input bias Current Control)
00：电流最大
01：电流次大
10：电流次小
11：电流最小 相关函数 */
void LCD_CR_IC_CTRL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_IC_CTRL_Msk);
	tmpreg |= (SetValue & LCD_CR_IC_CTRL_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_IC_CTRL_Get(void)
{
	return (LCD->CR & LCD_CR_IC_CTRL_Msk);
}

/* 驱动模式选择 (LCD Enabling Mode)
0：RFU
1：片内电阻型驱动 相关函数 */
void LCD_CR_ENMODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_ENMODE_Msk);
	tmpreg |= (SetValue & LCD_CR_ENMODE_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_ENMODE_Get(void)
{
	return (LCD->CR & LCD_CR_ENMODE_Msk);
}

/* 显示闪烁使能位 (LCD Flick Enable)
1：显示闪烁，闪烁频率由TON和TOFF寄存器设置
0：关闭闪烁 相关函数 */
void LCD_CR_FLICK_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->CR |= (LCD_CR_FLICK_Msk);
	}
	else
	{
		LCD->CR &= ~(LCD_CR_FLICK_Msk);
	}
}

FunState LCD_CR_FLICK_Getable(void)
{
	if (LCD->CR & (LCD_CR_FLICK_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LCD偏置电平选择位，用于显示灰度控制 (LCD Bias Voltage Select) 相关函数 */
void LCD_CR_BIAS_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_BIAS_Msk);
	tmpreg |= ((SetValue<<LCD_CR_BIAS_Pos) & LCD_CR_BIAS_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_BIAS_Get(void)
{
	return ((LCD->CR & LCD_CR_BIAS_Msk)>>LCD_CR_BIAS_Pos);
}

/* 偏置类型选择 (Bias Mode)
1：1/3 Bias
0：1/4 Bias 相关函数 */
void LCD_CR_BIASMD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_BIASMD_Msk);
	tmpreg |= (SetValue & LCD_CR_BIASMD_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_BIASMD_Get(void)
{
	return (LCD->CR & LCD_CR_BIASMD_Msk);
}

/* 防极化使能 (Anti-Polarization)
1：COM和SEG在LCD关闭情况下接地
0：COM和SEG在LCD关闭情况下浮空 相关函数 */
void LCD_CR_ANTIPOLAR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_ANTIPOLAR_Msk);
	tmpreg |= (SetValue & LCD_CR_ANTIPOLAR_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_ANTIPOLAR_Get(void)
{
	return (LCD->CR & LCD_CR_ANTIPOLAR_Msk);
}

/* 驱动波形选择 (Waveform Format)
1：B类波形
0：A类波形 相关函数 */
void LCD_CR_WFT_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_WFT_Msk);
	tmpreg |= (SetValue & LCD_CR_WFT_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_WFT_Get(void)
{
	return (LCD->CR & LCD_CR_WFT_Msk);
}

/* COM数量选择 (Segment Line Mux)
00：4COM
01：6COM
10/11：8COM 相关函数 */
void LCD_CR_LMUX_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->CR;
	tmpreg &= ~(LCD_CR_LMUX_Msk);
	tmpreg |= (SetValue & LCD_CR_LMUX_Msk);
	LCD->CR = tmpreg;
}

uint32_t LCD_CR_LMUX_Get(void)
{
	return (LCD->CR & LCD_CR_LMUX_Msk);
}

/* LCD显示使能位 (LCD Enable)
1：启动LCD显示
0：关闭LCD显示 相关函数 */
void LCD_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->CR |= (LCD_CR_EN_Msk);
	}
	else
	{
		LCD->CR &= ~(LCD_CR_EN_Msk);
	}
}

FunState LCD_CR_EN_Getable(void)
{
	if (LCD->CR & (LCD_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* LCD测试控制位，仅在测试模式下有效 (Line Constant Control)
COM、SEG输出电平由测试模式下的引脚输出数据寄存器决定。不同设置下SEG或COM输出的结果参见后文表格。 相关函数 */
void LCD_TEST_LCCTRL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->TEST;
	tmpreg &= ~(LCD_TEST_LCCTRL_Msk);
	tmpreg |= (SetValue & LCD_TEST_LCCTRL_Msk);
	LCD->TEST = tmpreg;
}

uint32_t LCD_TEST_LCCTRL_Get(void)
{
	return (LCD->TEST & LCD_TEST_LCCTRL_Msk);
}

/* 测试模式使能位 (Test mode Enable)
1：LCD测试模式使能。在LCD测试模式下，LCD引脚静态输出模拟直流电平，所有与动态扫描时间以及扫描波形相关寄存器设置无效
0：正常工作模式，测试模式无效，相关测试寄存器控制无效 相关函数 */
void LCD_TEST_TESTEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->TEST |= (LCD_TEST_TESTEN_Msk);
	}
	else
	{
		LCD->TEST &= ~(LCD_TEST_TESTEN_Msk);
	}
}

FunState LCD_TEST_TESTEN_Getable(void)
{
	if (LCD->TEST & (LCD_TEST_TESTEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 显示预分频寄存器 (Display Frequency) 相关函数 */
void LCD_FCR_Write(uint32_t SetValue)
{
	LCD->FCR = (SetValue & LCD_FCR_DF_Msk);
}

uint32_t LCD_FCR_Read(void)
{
	return (LCD->FCR & LCD_FCR_DF_Msk);
}

/* 闪烁显示熄灭时间寄存器 (Display-Off Time)
TOFF最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际OFF时间为TOFF * Tstep 相关函数 */
void LCD_FLKT_TOFF_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->FLKT;
	tmpreg &= ~(LCD_FLKT_TOFF_Msk);
	tmpreg |= (SetValue & LCD_FLKT_TOFF_Msk);
	LCD->FLKT = tmpreg;
}

uint32_t LCD_FLKT_TOFF_Get(void)
{
	return (LCD->FLKT & LCD_FLKT_TOFF_Msk);
}

/* 闪烁显示点亮时间寄存器 (Display-On Time)
TON最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际ON时间为TON * Tstep 相关函数 */
void LCD_FLKT_TON_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->FLKT;
	tmpreg &= ~(LCD_FLKT_TON_Msk);
	tmpreg |= (SetValue & LCD_FLKT_TON_Msk);
	LCD->FLKT = tmpreg;
}

uint32_t LCD_FLKT_TON_Get(void)
{
	return (LCD->FLKT & LCD_FLKT_TON_Msk);
}

/* 显示点亮中断使能位 (Display-On Interrupt Enable)
1：显示点亮中断使能
0：显示点亮中断禁止 相关函数 */
void LCD_IER_DONIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->IER |= (LCD_IER_DONIE_Msk);
	}
	else
	{
		LCD->IER &= ~(LCD_IER_DONIE_Msk);
	}
}

FunState LCD_IER_DONIE_Getable(void)
{
	if (LCD->IER & (LCD_IER_DONIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 显示熄灭中断使能位 (Display-OFF Interrupt Enable)
1：显示熄灭中断使能
0：显示熄灭中断禁止 相关函数 */
void LCD_IER_DOFFIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->IER |= (LCD_IER_DOFFIE_Msk);
	}
	else
	{
		LCD->IER &= ~(LCD_IER_DOFFIE_Msk);
	}
}

FunState LCD_IER_DOFFIE_Getable(void)
{
	if (LCD->IER & (LCD_IER_DOFFIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 显示点亮中断标志 (Display-On Interrupt Flag,write 1 to clear)
显示由灭变亮时硬件产生中断标志，硬件置位，软件清零 相关函数 */
void LCD_ISR_DONIF_Clr(void)
{
	LCD->ISR = LCD_ISR_DONIF_Msk;
}

FlagStatus LCD_ISR_DONIF_Chk(void)
{
	if (LCD->ISR & LCD_ISR_DONIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 显示熄灭中断标志 (Display-OFF Interrupt Flag,write 1 to clear)
显示由亮变灭时硬件产生中断标志，硬件置位，软件清零 相关函数 */
void LCD_ISR_DOFFIF_Clr(void)
{
	LCD->ISR = LCD_ISR_DOFFIF_Msk;
}

FlagStatus LCD_ISR_DOFFIF_Chk(void)
{
	if (LCD->ISR & LCD_ISR_DOFFIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void LCD_BSTCR_OSC_CFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->BSTCR;
	tmpreg &= ~(LCD_BSTCR_OSC_CFG_Msk);
	tmpreg |= (SetValue & LCD_BSTCR_OSC_CFG_Msk);
	LCD->BSTCR = tmpreg;
}

uint32_t LCD_BSTCR_OSC_CFG_Get(void)
{
	return (LCD->BSTCR & LCD_BSTCR_OSC_CFG_Msk);
}

void LCD_BSTCR_BUF_OFF_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->BSTCR |= (LCD_BSTCR_BUF_OFF_Msk);
	}
	else
	{
		 LCD->BSTCR &= ~(LCD_BSTCR_BUF_OFF_Msk);		
	}
}

FunState LCD_BSTCR_BUF_OFF_Getable(void)
{
	if (LCD->BSTCR & (LCD_BSTCR_BUF_OFF_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void LCD_BSTCR_BUFBYP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->BSTCR |= (LCD_BSTCR_BUFBYP_Msk);
	}
	else
	{
		LCD->BSTCR &= ~(LCD_BSTCR_BUFBYP_Msk);
	}
}

FunState LCD_BSTCR_BUFBYP_Getable(void)
{
	if (LCD->BSTCR & (LCD_BSTCR_BUFBYP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void LCD_BSTCR_VLCDPD_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->BSTCR |= (LCD_BSTCR_VLCDPD_Msk);
	}
	else
	{
		LCD->BSTCR &= ~(LCD_BSTCR_VLCDPD_Msk);
	}
}

FunState LCD_BSTCR_VLCDPD_Getable(void)
{
	if (LCD->BSTCR & (LCD_BSTCR_VLCDPD_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void LCD_BSTCR_VLCDCFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LCD->BSTCR;
	tmpreg &= ~(LCD_BSTCR_VLCDCFG_Msk);
	tmpreg |= (SetValue & LCD_BSTCR_VLCDCFG_Msk);
	LCD->BSTCR = tmpreg;
}

uint32_t LCD_BSTCR_VLCDCFG_Get(void)
{
	return (LCD->BSTCR & LCD_BSTCR_VLCDCFG_Msk);
}

void LCD_BSTCR_BOOST_TEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->BSTCR |= (LCD_BSTCR_BOOST_TEN_Msk);
	}
	else
	{
		LCD->BSTCR &= ~(LCD_BSTCR_BOOST_TEN_Msk);
	}
}

FunState LCD_BSTCR_BOOST_TEN_Getable(void)
{
	if (LCD->BSTCR & (LCD_BSTCR_BOOST_TEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

void LCD_BSTCR_BOOST_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		LCD->BSTCR |= (LCD_BSTCR_BOOST_EN_Msk);
	}
	else
	{
		LCD->BSTCR &= ~(LCD_BSTCR_BOOST_EN_Msk);
	}
}

FunState LCD_BSTCR_BOOST_EN_Getable(void)
{
	if (LCD->BSTCR & (LCD_BSTCR_BOOST_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


/*********************************
LCD 显示控制函数 
功能：配置LCD COM0~3输出使能控制
输入: 00-0F COM0-COM3
输出: 无
*********************************/
void LCD_COM_EN_Write(uint32_t SetValue)
{
	LCD->COMEN = (SetValue & LCD_COM_EN_COMEN_Msk);
}
/*********************************
LCD 显示控制函数 
功能：获取LCD COM0~3输出使能状态
输入: 无
输出: 00-0F COM0-COM3
*********************************/
uint32_t LCD_COM_EN_Read(void)
{
	return (LCD->COMEN & LCD_COM_EN_COMEN_Msk);
}


/*********************************
LCD 显示控制函数 
功能：配置LCD SEG0-31输出使能控制
输入: 00000000-ffffffff	SEG0-SEG31
输出: 无
*********************************/
void LCD_SEG_EN0_Write(uint32_t SetValue)
{
	LCD->SEGEN0 = (SetValue);
}
/*********************************
LCD 显示控制函数 
功能：获取LCD SEG0-31输出使能状态
输入: 无
输出: 00000000-ffffffff	SEG0-SEG31
*********************************/
uint32_t LCD_SEG_EN0_Read(void)
{
	return (LCD->SEGEN0);
}

/*********************************
LCD 显示控制函数 
功能：配置LCD SEG32~43(COM4-7)输出使能控制
输入: 00000000-00000fff	SEG32~43(COM4-7)
输出: 无
*********************************/
void LCD_SEG_EN1_Write(uint32_t SetValue)
{
	LCD->SEGEN1 = (SetValue & LCD_SEG_EN1_SEGENx_Msk);
}
/*********************************
LCD 显示控制函数 
功能：获取LCD SEG32~43(COM4-7)输出使能状态
输入: 无
输出: 00000000-00000fff	SEG32~43(COM4-7)
*********************************/
uint32_t LCD_SEG_EN1_Read(void)
{
	return (LCD->SEGEN1 & LCD_SEG_EN1_SEGENx_Msk);
}

/*DISPDATAx显示数据寄存器刷新*/
void LCD_DISPDATAx_Refresh(uint32_t* DispBuf)
{
    LCD->DATA0 = DispBuf[0];
    LCD->DATA1 = DispBuf[1];
    LCD->DATA2 = DispBuf[2];
    LCD->DATA3 = DispBuf[3];
    LCD->DATA4 = DispBuf[4];
    LCD->DATA5 = DispBuf[5];
    LCD->DATA6 = DispBuf[6];
    LCD->DATA7 = DispBuf[7];
    LCD->DATA8 = DispBuf[8];
    LCD->DATA9 = DispBuf[9];
}

void LCD_Deinit(void)
{
	//LCD->CR = 0x00010e00;
	//LCD->TEST = 0x00000000;
	//LCD->FCR = 0x00000000;
	//LCD->FLKT = 0x00000000;
	//LCD->IER = 0x00000000;
	//LCD->ISR = 0x00000000;
	//LCD->BSTCR = 0x00000000;
}

/******END OF FILE****/
