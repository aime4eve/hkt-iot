/**
  ******************************************************************************
  * @file    fm33a0xxev_adc.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_adc.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */ 

/** @defgroup ADC 
  * @brief ADC driver modules
  * @{
  */ 

/* ADC内部累加模式中断使能 相关函数 */
void ADC_CR_ADC_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		ADC->CR |= (ADC_CR_ADC_IE_Msk);
	}
	else
	{
		ADC->CR &= ~(ADC_CR_ADC_IE_Msk);
	}
}

FunState ADC_CR_ADC_IE_Getable(void)
{
	if (ADC->CR & (ADC_CR_ADC_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC外部累加模式中断使能 相关函数 */
void ADC_CR_ACC_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		ADC->CR |= (ADC_CR_ACC_IE_Msk);
	}
	else
	{
		ADC->CR &= ~(ADC_CR_ACC_IE_Msk);
	}
}

FunState ADC_CR_ACC_IE_Getable(void)
{
	if (ADC->CR & (ADC_CR_ACC_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC功耗模式
0：低功耗模式，工作时钟最高1MHz
1：高功耗模式，工作时钟最高2MHz 相关函数 */
void ADC_CR_HPEN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ADC->CR;
	tmpreg &= ~(ADC_CR_HPEN_Msk);
	tmpreg |= (SetValue & ADC_CR_HPEN_Msk);
	ADC->CR = tmpreg;
}

uint32_t ADC_CR_HPEN_Get(void)
{
	return (ADC->CR & ADC_CR_HPEN_Msk);
}

/* ADC工作模式
0：内部累加器模式
1：外部累加器或CIC模式 相关函数 */
void ADC_CR_MODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ADC->CR;
	tmpreg &= ~(ADC_CR_MODE_Msk);
	tmpreg |= (SetValue & ADC_CR_MODE_Msk);
	ADC->CR = tmpreg;
}

uint32_t ADC_CR_MODE_Get(void)
{
	return (ADC->CR & ADC_CR_MODE_Msk);
}

/* 积分器复位使能，在MODE=1并且使用外部累加器时，必须置位，其他条件下必须保持为0
0：禁止积分器外部复位
1：允许积分器外部复位 相关函数 */
void ADC_CR_RSTCTRL_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		ADC->CR |= (ADC_CR_RSTCTRL_EN_Msk);
	}
	else
	{
		ADC->CR &= ~(ADC_CR_RSTCTRL_EN_Msk);
	}
}

FunState ADC_CR_RSTCTRL_EN_Getable(void)
{
	if (ADC->CR & (ADC_CR_RSTCTRL_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC使能信号
0：ADC不使能
1：ADC使能

注意：RTC执行自动温补时，硬件会周期性自动使能ADC 相关函数 */
void ADC_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		ADC->CR |= (ADC_CR_EN_Msk);
	}
	else
	{
		ADC->CR &= ~(ADC_CR_EN_Msk);
	}
}

FunState ADC_CR_EN_Getable(void)
{
	if (ADC->CR & (ADC_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC TRIM值，仅针对内部累加器模式输出
内部累加器累加周期：Period = TRIM*2*TADC_CLK
累加周期决定了输出数据位宽，当TRIM=0x7FF最大值时，实际累加周期为4095 cycle，即ADC输出数据最大有效位宽是12bit 相关函数 */
void ADC_TRIM_Write(uint32_t SetValue)
{
	ADC->TRIM  = (SetValue & ADC_TRIM_ADC_TRIM_Msk);
}

uint32_t ADC_TRIM_Read(void)
{
	return (ADC->TRIM  & ADC_TRIM_ADC_TRIM_Msk);
}

/* ADC输出数据，为码流累加结果，未经降采样滤波器处理
当MODE=0时，输出为ADC内部累加结果，最高12bit，位数由ADC_TRIM决定
当MODE=1时，输出位ADC外部累加结果，最高14bit，位数由ACC_PERIOD决定 相关函数 */
uint32_t ADC_DR_Read(void)
{
	return (ADC->DR & ADC_DR_ADC_DATA_Msk);
}

/* ADC初始化完成标志，仅MODE=1时有效（不产生中断）
0：ADC还未完成初始化
1：ADC完成初始化

注：MODE=1，ADC_EN置位后，等待MODE_CTRL_DELAY时间之后，此标志置位，表示ADC内部积分器建立完成。MODE=0时，工作时序由ADC内部自行控制，此标志无效。 相关函数 */
FlagStatus ADC_ISR_INIT_RDY_Chk(void)
{
	if (ADC->ISR  & ADC_ISR_INIT_RDY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 累加器完成中断标志，硬件置位，软件写1清零，写0无效 相关函数 */
void ADC_ISR_ACC_IF_Clr(void)
{
	ADC->ISR  = ADC_ISR_ACC_IF_Msk;
}

FlagStatus ADC_ISR_ACC_IF_Chk(void)
{
	if (ADC->ISR  & ADC_ISR_ACC_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* ADC转换完成输出，软件只读（不产生中断）
转换完成后此信号保持为高电平，只有关闭ADC才会清0 相关函数 */
FlagStatus ADC_ISR_ADC_DONE_Chk(void)
{
	if (ADC->ISR  & ADC_ISR_ADC_DONE_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* ADC转换完成中断标志，硬件置位，软件写1清零，写0无效 相关函数 */
void ADC_ISR_ADC_IF_Clr(void)
{
	ADC->ISR  = ADC_ISR_ADC_IF_Msk;
}

FlagStatus ADC_ISR_ADC_IF_Chk(void)
{
	if (ADC->ISR  & ADC_ISR_ADC_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 外部累加器累加周期配置，单位ADC_CLK
000：1023（对应结果10bit）
001：2047（对应结果11bit）
010：4095（对应结果12bit）
011：8191（对应结果13bit）
100：16383（对应结果14bit）
Others：4095 相关函数 */
void ADC_CFGR_ACC_PERIOD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ADC->CFGR;
	tmpreg &= ~(ADC_CFGR_ACC_PERIOD_Msk);
	tmpreg |= (SetValue & ADC_CFGR_ACC_PERIOD_Msk);
	ADC->CFGR = tmpreg;
}

uint32_t ADC_CFGR_ACC_PERIOD_Get(void)
{
	return (ADC->CFGR & ADC_CFGR_ACC_PERIOD_Msk);
}

/* SDADC使能后mode_ctrl延迟长度配置，单位是ADC_CLK周期
0x00：不延迟
0xFF：延迟255个ADC_CLK

注意：复位值为16个时钟周期，不得修改为小于16的数值 相关函数 */
void ADC_CFGR_RST_CTRL_DELAY_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ADC->CFGR;
	tmpreg &= ~(ADC_CFGR_RST_CTRL_DELAY_Msk);
	tmpreg |= (SetValue & ADC_CFGR_RST_CTRL_DELAY_Msk);
	ADC->CFGR = tmpreg;
}

uint32_t ADC_CFGR_RST_CTRL_DELAY_Get(void)
{
	return (ADC->CFGR & ADC_CFGR_RST_CTRL_DELAY_Msk);
}

/* ADC输入通道Buffer使能 相关函数 */
void ADC_CFGR_BUFEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		ADC->CFGR |= (ADC_CFGR_BUFEN_Msk);
	}
	else
	{
		ADC->CFGR &= ~(ADC_CFGR_BUFEN_Msk);
	}
}

FunState ADC_CFGR_BUFEN_Getable(void)
{
	if (ADC->CFGR & (ADC_CFGR_BUFEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* ADC输入通道选择
0001：ADC_IN1
0010：ADC_IN2
0011：ADC_IN3
0100：ADC_IN4
0101：ADC_IN5
0110：ADC_IN6
0111：ADC_IN7
1000：ADC_IN8
1001：ADC_IN9
1010：ADC_IN10
1011：ADC_IN11
1100：ADC_IN12
1110：VBAT
1111：TS 相关函数 */
void ADC_CFGR_BUFSEL_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ADC->CFGR;
	tmpreg &= ~(ADC_CFGR_BUFSEL_Msk);
	tmpreg |= (SetValue & ADC_CFGR_BUFSEL_Msk);
	ADC->CFGR = tmpreg;
}

uint32_t ADC_CFGR_BUFSEL_Get(void)
{
	return (ADC->CFGR & ADC_CFGR_BUFSEL_Msk);
}

/* CIC滤波器输出数据，更新频率由ADC工作时钟和OSR共同决定；注意这个数据是有符合数，格式为二进制补码。相关函数 */
uint32_t CIC_DR_Read(void)
{
	return (CIC->DR & CIC_DR_ADC_CIC_DATA_Msk);
}

/* CIC滤波器输出数据 offset调制寄存器，软件写入offset值，硬件将CIC_DR加上OS后可以得到无符合结果，相关函数 */
void CIC_OS_Write(uint32_t SetValue)
{
	CIC ->OS = (SetValue & CIC_OS_ADC_CIC_OS_Msk);
}

uint32_t CIC_OS_Read(void)
{
	return (CIC ->OS & CIC_OS_ADC_CIC_OS_Msk);
}

/* CIC_DR + CIC_OS得到的无符合数结果 相关函数 */
uint32_t CIC_USDR_Read(void)
{
	return (CIC ->USDR  & CIC_USDR_ADC_CIC_USDR_Msk);
}

/* CIC滤波器使能 (CIC enable)
0:关闭CIC
1:使能CIC 相关函数 */
void CIC_CR_CIC_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CIC ->CR |= (CIC_CR_CIC_EN_Msk);
	}
	else
	{
		CIC ->CR &= ~(CIC_CR_CIC_EN_Msk);
	}
}

FunState CIC_CR_CIC_EN_Getable(void)
{
	if (CIC ->CR & (CIC_CR_CIC_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CIC使能后丢弃的输出点数 (Number of Samples to be Discarded)
可以丢弃ADC使能后开头输出的0~255个采样点
相关函数 */
void CIC_CR_NS_DISC_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CIC->CR;
	tmpreg &= ~(CIC_CR_NS_DISC_Msk);
	tmpreg |= (SetValue & CIC_CR_NS_DISC_Msk);
	CIC ->CR = tmpreg;
}

uint32_t CIC_CR_NS_DISC_Get(void)
{
	return (CIC ->CR & CIC_CR_NS_DISC_Msk);
}

/* CIC Overrun Interrupt enable
0:禁止CIC溢出中断
1:允许CIC溢出中断 相关函数 */
void CIC_CR_OVR_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CIC ->CR |= (CIC_CR_OVR_IE_Msk);
	}
	else
	{
		CIC ->CR &= ~(CIC_CR_OVR_IE_Msk);
	}
}

FunState CIC_CR_OVR_IE_Getable(void)
{
	if (CIC ->CR & (CIC_CR_OVR_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* CIC中断使能(CIC interrupt enable)
0:禁止CIC中断
1:允许CIC中断,当CIC_IF置位时产生中断事件 相关函数 */
void CIC_CR_CIC_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		CIC ->CR |= (CIC_CR_CIC_IE_Msk);
	}
	else
	{
		CIC ->CR &= ~(CIC_CR_CIC_IE_Msk);
	}
}

FunState CIC_CR_CIC_IE_Getable(void)
{
	if (CIC ->CR & (CIC_CR_CIC_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 输出数据截取控制 (Truncate),配置最终结果中丢弃最低位的位数;
相关函数 */
void CIC_CR_TRUNC_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CIC ->CR;
	tmpreg &= ~(CIC_CR_TRUNC_Msk);
	tmpreg |= (SetValue & CIC_CR_TRUNC_Msk);
	CIC ->CR = tmpreg;
}

uint32_t CIC_CR_TRUNC_Get(void)
{
	return (CIC ->CR & CIC_CR_TRUNC_Msk);
}

/* 过采样率配置 (Over Sampling Rate)
000:x8
001:x16
010:x32
011:x64
100:x128
101:x256
110:x512
111:x1024 相关函数 */
void CIC_CR_OSR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = CIC ->CR;
	tmpreg &= ~(CIC_CR_OSR_Msk);
	tmpreg |= (SetValue & CIC_CR_OSR_Msk);
	CIC ->CR = tmpreg;
}

uint32_t CIC_CR_OSR_Get(void)
{
	return (CIC ->CR & CIC_CR_OSR_Msk);
}

void CIC_ISR_CIC_OVR_Clr(void)
{
	CIC ->ISR = CIC_ISR_CIC_OVR_Msk;
}

FlagStatus CIC_ISR_CIC_OVR_Chk(void)
{
	if (CIC ->ISR & CIC_ISR_CIC_OVR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void CIC_ISR_CIC_IF_Clr(void)
{
	CIC ->ISR = CIC_ISR_CIC_IF_Msk;
}

FlagStatus CIC_ISR_CIC_IF_Chk(void)
{
	if (CIC ->ISR & CIC_ISR_CIC_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void ADC_Deinit(void)
{
	//ADC->CR = 0x00000000;
	//ADC->TRIM  = 0x00000000;
	//ADC->DR = 0x00000000;
	//ADC->ISR  = 0x00000000;
	//ADC->CFGR = 0x00000000;
}

/******END OF FILE****/
