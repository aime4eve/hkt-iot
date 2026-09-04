/**
  ******************************************************************************
  * @file    fm33a0xxev_et.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_et.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup ET 
  * @brief ET driver modules
  * @{
  */ 

/* 输出极性选择 (Output Polarity)
0：输出不取反
1：输出取反 相关函数 */
void ETx_CR_OPOL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_OPOL_Msk);
	tmpreg |= (SetValue & ETx_CR_OPOL_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_OPOL_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_OPOL_Msk);
}

/* 外部引脚输入数字滤波使能
1 = 打开引脚输入信号数字滤波
0 = 关闭引脚输入信号数字滤波 相关函数 */
void ETx_CR_EXFLT_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_EXFLT_Msk);
	tmpreg |= (SetValue & ETx_CR_EXFLT_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_EXFLT_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_EXFLT_Msk);
}

/* PWM输出控制
1 = PWM输出使能
0 = PWM输出禁止 相关函数 */
void ETx_CR_PWM_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->CR  |= (ETx_CR_PWM_Msk);
	}
	else
	{
		ETx->CR  &= ~(ETx_CR_PWM_Msk);
	}
}

FunState ETx_CR_PWM_Getable(ET_Type* ETx)
{
	if (ETx->CR  & (ETx_CR_PWM_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 启动控制
1 = 启动定时器，在计数器模式下启动时将计数初值加载至计数器和工作寄存器；在捕捉模式下，启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数
0 = 停止计数器计数 相关函数 */
void ETx_CR_CEN_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->CR  |= (ETx_CR_CEN_Msk);
	}
	else
	{
		ETx->CR  &= ~(ETx_CR_CEN_Msk);
	}
}

FunState ETx_CR_CEN_Getable(ET_Type* ETx)
{
	if (ETx->CR  & (ETx_CR_CEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 工作模式选择
1 = 捕捉模式
0 = 定时/计数模式 相关函数 */
void ETx_CR_MOD_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_MOD_Msk);
	tmpreg |= (SetValue & ETx_CR_MOD_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_MOD_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_MOD_Msk);
}

/* Cascade Enable，扩展定时器级联使能
1 = ET1（ET3）和ET2（ET4）级联成32bit定时器
0 = 16bit定时器独立工作 相关函数 */
void ETx_CR_CASEN_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_CASEN_Msk);
	tmpreg |= (SetValue & ETx_CR_CASEN_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_CASEN_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_CASEN_Msk);
}

/* 计数模式采沿方式选择
（计数时钟选择mcu_clk时该位无效，总是采用mcu_clk时钟上升沿计数）
1 = 计数模式采下降沿
0 = 计数模式采上升沿 相关函数 */
void ETx_CR_EDGESEL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_EDGESEL_Msk);
	tmpreg |= (SetValue & ETx_CR_EDGESEL_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_EDGESEL_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_EDGESEL_Msk);
}

/* 捕捉模式控制
1 = 脉宽捕捉
0 = 脉冲周期捕捉 相关函数 */
void ETx_CR_CAPMOD_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_CAPMOD_Msk);
	tmpreg |= (SetValue & ETx_CR_CAPMOD_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_CAPMOD_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_CAPMOD_Msk);
}

/* 带清零捕捉模式控制
1 = 事件触发捕捉：使能后计数器保持0，捕捉到第一个有效沿之后timer才开始计数
0 = 捕捉不清零，计数器一直自由计数 相关函数 */
void ETx_CR_CAPCLR_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->CR  |= (ETx_CR_CAPCLR_Msk);
	}
	else
	{
		ETx->CR  &= ~(ETx_CR_CAPCLR_Msk);
	}
}

FunState ETx_CR_CAPCLR_Getable(ET_Type* ETx)
{
	if (ETx->CR  & (ETx_CR_CAPCLR_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 单次捕捉控制
1 = 单次捕捉有效，在捕捉到一次脉冲周期后计数器停止，若需要再次捕捉需重新启动
0 = 连续捕捉 相关函数 */
void ETx_CR_CAPONCE_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_CAPONCE_Msk);
	tmpreg |= (SetValue & ETx_CR_CAPONCE_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_CAPONCE_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_CAPONCE_Msk);
}

/* 捕捉沿选择
1 = 周期捕捉模式时下沿捕捉
0 = 周期捕捉模式时上沿捕捉 相关函数 */
void ETx_CR_CAPEDGE_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->CR ;
	tmpreg &= ~(ETx_CR_CAPEDGE_Msk);
	tmpreg |= (SetValue & ETx_CR_CAPEDGE_Msk);
	ETx->CR  = tmpreg;
}

uint32_t ETx_CR_CAPEDGE_Get(ET_Type* ETx)
{
	return (ETx->CR  & ETx_CR_CAPEDGE_Msk);
}

/* 内部信号2源选择(捕捉源)
1 = 扩展定时器3的内部信号2选择Group1
0 = 扩展定时器3的内部信号2选择Group2 相关函数 */
void ETx_INSR_SIG2SEL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->INSR ;
	tmpreg &= ~(ETx_INSR_SIG2SEL_Msk);
	tmpreg |= (SetValue & ETx_INSR_SIG2SEL_Msk);
	ETx->INSR  = tmpreg;
}

uint32_t ETx_INSR_SIG2SEL_Get(ET_Type* ETx)
{
	return (ETx->INSR  & ETx_INSR_SIG2SEL_Msk);
}

/* 内部信号1源选择（在计数模式下计数源仅由此选择，捕捉模式下计数源）
1 = 扩展定时器3的内部信号1选择Group2
0 = 扩展定时器3的内部信号1选择Group1 相关函数 */
void ETx_INSR_SIG1SEL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->INSR ;
	tmpreg &= ~(ETx_INSR_SIG1SEL_Msk);
	tmpreg |= (SetValue & ETx_INSR_SIG1SEL_Msk);
	ETx->INSR  = tmpreg;
}

uint32_t ETx_INSR_SIG1SEL_Get(ET_Type* ETx)
{
	return (ETx->INSR  & ETx_INSR_SIG1SEL_Msk);
}

/* GROUP2 信号选择控制
ET1
000 = UART0_RX
001 = UART1_RX
010 = XTLF
011 = ET1_IN1
100 = ET1_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET2
000 = UART2_RX
001 = UART3_RX
010 = XTLF
011 = ET2_IN1
100 = ET2_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET3
000 = ET3_IN1
001 = XTLF 
010 = UART4_RX
011 = UART5_RX
100 = RTCSEC
101~111 = RFU
ET4
000 = ET4_IN1
001 = XTLF
010 = UART_RX2
011 = UART_RX0
100 = CMP1O（比较器1输出）
101 = CMP2O（比较器2输出）
110= RTCSEC
111= RCMF? 相关函数 */
void ETx_INSR_GRP2SEL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->INSR ;
	tmpreg &= ~(ETx_INSR_GRP2SEL_Msk);
	tmpreg |= (SetValue & ETx_INSR_GRP2SEL_Msk);
	ETx->INSR  = tmpreg;
}

uint32_t ETx_INSR_GRP2SEL_Get(ET_Type* ETx)
{
	return (ETx->INSR  & ETx_INSR_GRP2SEL_Msk);
}

/* GROUP1 信号选择控制
ET1
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET1_IN0
ET2
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET2_IN0
ET3
00 = APBCLK
01 = ET3_IN0
10 = RTCSEC
11 = RCLP
ET4
00 = APBCLK
01 = ET4_IN0
10 = RTC64HZ
11 = RCMF 相关函数 */
void ETx_INSR_GRP1SEL_Set(ET_Type* ETx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = ETx->INSR ;
	tmpreg &= ~(ETx_INSR_GRP1SEL_Msk);
	tmpreg |= (SetValue & ETx_INSR_GRP1SEL_Msk);
	ETx->INSR  = tmpreg;
}

uint32_t ETx_INSR_GRP1SEL_Get(ET_Type* ETx)
{
	return (ETx->INSR  & ETx_INSR_GRP1SEL_Msk);
}

/* 输入Signal1（计数源）的预分频寄存器
00表示1分频，FF表示256分频 相关函数 */
void ETx_PSCR1_Write(ET_Type* ETx, uint32_t SetValue)
{
	ETx->PSCR1  = (SetValue & ETx_PSCR1_PRESCALE1_Msk);
}

uint32_t ETx_PSCR1_Read(ET_Type* ETx)
{
	return (ETx->PSCR1  & ETx_PSCR1_PRESCALE1_Msk);
}

/* 输入Signal2（捕捉源）的预分频寄存器
00表示1分频，FF表示256分频。 相关函数 */
void ETx_PSCR2_Write(ET_Type* ETx, uint32_t SetValue)
{
	ETx->PSCR2  = (SetValue & ETx_PSCR2_PRESCALE2_Msk);
}

uint32_t ETx_PSCR2_Read(ET_Type* ETx)
{
	return (ETx->PSCR2  & ETx_PSCR2_PRESCALE2_Msk);
}

/* 扩展定时器初值寄存器 相关函数 */
void ETx_IVR_Write(ET_Type* ETx, uint32_t SetValue)
{
	ETx->IVR  = (SetValue & ETx_IVR_INITVALUE_Msk);
}

uint32_t ETx_IVR_Read(ET_Type* ETx)
{
	return (ETx->IVR  & ETx_IVR_INITVALUE_Msk);
}

/* 扩展定时器比较寄存器
该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
void ETx_CMPR_Write(ET_Type* ETx, uint32_t SetValue)
{
	ETx->CMPR  = (SetValue & ETx_CMPR_CMP_Msk);
}

uint32_t ETx_CMPR_Read(ET_Type* ETx)
{
	return (ETx->CMPR  & ETx_CMPR_CMP_Msk);
}

/* 扩展定时器比较中断使能
1 = 使能
0 = 禁止 相关函数 */
void ETx_IER_CMPIE_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->IER  |= (ETx_IER_CMPIE_Msk);
	}
	else
	{
		ETx->IER  &= ~(ETx_IER_CMPIE_Msk);
	}
}

FunState ETx_IER_CMPIE_Getable(ET_Type* ETx)
{
	if (ETx->IER  & (ETx_IER_CMPIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 扩展定时器捕捉中断使能
1 = 使能
0 = 禁止 相关函数 */
void ETx_IER_CAPIE_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->IER  |= (ETx_IER_CAPIE_Msk);
	}
	else
	{
		ETx->IER  &= ~(ETx_IER_CAPIE_Msk);
	}
}

FunState ETx_IER_CAPIE_Getable(ET_Type* ETx)
{
	if (ETx->IER  & (ETx_IER_CAPIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 扩展定时器3溢出中断使能
1 = 使能
0 = 禁止 相关函数 */
void ETx_IER_OVIE_Setable(ET_Type* ETx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		ETx->IER  |= (ETx_IER_OVIE_Msk);
	}
	else
	{
		ETx->IER  &= ~(ETx_IER_OVIE_Msk);
	}
}

FunState ETx_IER_OVIE_Getable(ET_Type* ETx)
{
	if (ETx->IER  & (ETx_IER_OVIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 比较状态，写1清0
1 =当前计数器的值大于等于比较寄存器的值
0 =当前计数器的值小于比较寄存器的值 相关函数 */
void ETx_ISR_CMPIF_Clr(ET_Type* ETx)
{
	ETx->ISR  = ETx_ISR_CMPIF_Msk;
}

FlagStatus ETx_ISR_CMPIF_Chk(ET_Type* ETx)
{
	if (ETx->ISR  & ETx_ISR_CMPIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 扩展定时器捕捉产生信号
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 相关函数 */
void ETx_ISR_CAPIF_Clr(ET_Type* ETx)
{
	ETx->ISR  = ETx_ISR_CAPIF_Msk;
}

FlagStatus ETx_ISR_CAPIF_Chk(ET_Type* ETx)
{
	if (ETx->ISR  & ETx_ISR_CAPIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 扩展定时器3溢出信号，当计数器的值由0xFFFF再增加时将置位
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
void ETx_ISR_OVIF_Clr(ET_Type* ETx)
{
	ETx->ISR  = ETx_ISR_OVIF_Msk;
}

FlagStatus ETx_ISR_OVIF_Chk(ET_Type* ETx)
{
	if (ETx->ISR  & ETx_ISR_OVIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/*********************************
获取ET 计数器数值函数
功能：获取ET 计数器数值
输入: ET1/ET2/ET3/ET4  	定时器名称
输出: 00-FFFF
      其余为无效值
*********************************/
uint32_t ETxCNT_CNTx_Read(ETCNT_Type* ETIMx)
{
	volatile uint8_t tmp08;
  uint32_t tmp32;
  tmp08 = ((uint32_t)ETIMx - ET1_BASE)>>5;	//获取ETIM
  switch(tmp08)
  {
    case 0:
      tmp32 = (ETCNT->CNT1 & ETCNTx_CNT_Msk);
      break;
    case 1:
      tmp32 = (ETCNT->CNT2 & ETCNTx_CNT_Msk);
      break;
    case 2:
      tmp32 = (ETCNT->CNT3 & ETCNTx_CNT_Msk);
      break;
    case 3:
      tmp32 = (ETCNT->CNT4 & ETCNTx_CNT_Msk);
      break;
    default:
      tmp32 = 0x1fffffff;
			break;
  }
  return tmp32;
}

void ETx_Deinit(ET_Type* ETx)
{
	ETx->CR  = 0x00000000;
	ETx->INSR  = 0x00000000;
	ETx->PSCR1  = 0x00000000;
	ETx->PSCR2  = 0x00000000;
	ETx->IVR  = 0x00000000;
	ETx->CMPR  = 0x00000000;
	ETx->IER  = 0x00000000;
	ETx->ISR  = 0x00000000;
}

/******END OF FILE****/
