/**
  ******************************************************************************
  * @file    fm33a0xxev_u7816.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_u7816.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup U7816 
  * @brief U7816 driver modules
  * @{
  */ 

/* U7816通道发送使能控制位 (Transmit Enable)1：通道发送使能，可发送数据0：通道发送禁止，不可发送数据，并关断输出端口，将SCL信号转化为低电平 相关函数 */
void U7816_CR_TXEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->CR |= (U7816_CR_TXEN_Msk);
	}
	else
	{
		U7816->CR &= ~(U7816_CR_TXEN_Msk);
	}
}

FunState U7816_CR_TXEN_Getable(void)
{
	if (U7816->CR & (U7816_CR_TXEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816通道接收使能控制位 (Receive Enable)1：通道接收使能，可接收数据0：通道接收禁止，不可接收数据，并关断输入端口 相关函数 */
void U7816_CR_RXEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->CR |= (U7816_CR_RXEN_Msk);
	}
	else
	{
		U7816->CR &= ~(U7816_CR_RXEN_Msk);
	}
}

FunState U7816_CR_RXEN_Getable(void)
{
	if (U7816->CR & (U7816_CR_RXEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816时钟CLK输出使能控制位 (Clock output Enable)1：7816时钟输出使能0：7816时钟输出禁止 相关函数 */
void U7816_CR_CKOEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->CR |= (U7816_CR_CKOEN_Msk);
	}
	else
	{
		U7816->CR &= ~(U7816_CR_CKOEN_Msk);
	}
}

FunState U7816_CR_CKOEN_Getable(void)
{
	if (U7816->CR & (U7816_CR_CKOEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816通道数据发送强上拉电阻自动有效控制位 (High-Pullup Automatically)1：数据发送时上拉电阻自动有效，接收态上拉电阻无效0：数据发送时上拉电阻自动有效功能禁止，上拉电阻由HPUEN，LPUEN控制 相关函数 */
void U7816_CR_HPUAT_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->CR |= (U7816_CR_HPUAT_Msk);
	}
	else
	{
		U7816->CR &= ~(U7816_CR_HPUAT_Msk);
	}
}

FunState U7816_CR_HPUAT_Getable(void)
{
	if (U7816->CR & (U7816_CR_HPUAT_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816通道强上拉使能控制位 (High-Pullup Enable)1：强上拉有效0：强上拉无效 相关函数 */
void U7816_CR_HPUEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->CR |= (U7816_CR_HPUEN_Msk);
	}
	else
	{
		U7816->CR &= ~(U7816_CR_HPUEN_Msk);
	}
}

FunState U7816_CR_HPUEN_Getable(void)
{
	if (U7816->CR & (U7816_CR_HPUEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Guard Time发送长度控制位 (Send long Frame Enable)1：Guard time为3 etu0：Guard time为2 etu 相关函数 */
void U7816_FFR_SFREN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_SFREN_Msk);
	tmpreg |= (SetValue & U7816_FFR_SFREN_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_SFREN_Get(void)
{
	return (U7816->FFR & U7816_FFR_SFREN_Msk);
}

/* ERROR SIGNAL宽度选择 (Error Signal Width)11：ERROR SIGNAL宽度为1ETU;10：ERROR SIGNAL宽度为1.5ETU;01：ERROR SIGNAL宽度为2ETU;00：ERROR SIGNAL宽度为2ETU; 相关函数 */
void U7816_FFR_ERSW_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_ERSW_Msk);
	tmpreg |= (SetValue & U7816_FFR_ERSW_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_ERSW_Get(void)
{
	return (U7816->FFR & U7816_FFR_ERSW_Msk);
}

/* ERROR SIGNAL后GUARDTIME宽度选择（仅在发送时有效）
(Error Signal Guard Time)1：ERROR SIGNAL后GUARDTIME为1~1.5ETU。0：ERROR SIGNAL后GUARDTIME为2~2.5ETU。ERROR SIGNAL宽度为整数ETU时GUARDTIME为1.5或2.5ETU；ERROR SIGNAL宽度为1.5ETU时GUARDTIME为1或2ETU 相关函数 */
void U7816_FFR_ERSGD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_ERSGD_Msk);
	tmpreg |= (SetValue & U7816_FFR_ERSGD_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_ERSGD_Get(void)
{
	return (U7816->FFR & U7816_FFR_ERSGD_Msk);
}

/* BGT控制位。控制接收->发送之间是否插入BGT。BGT是接收->发送之间需要的最小时间（block guard time enable）1：BGT使能，插入Block guard time(12 etu);0：BGT禁止，不插入Block guard time(12 etu); 相关函数 */
void U7816_FFR_BGTEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->FFR |= (U7816_FFR_BGTEN_Msk);
	}
	else
	{
		U7816->FFR &= ~(U7816_FFR_BGTEN_Msk);
	}
}

FunState U7816_FFR_BGTEN_Getable(void)
{
	if (U7816->FFR & (U7816_FFR_BGTEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 控制接收数据奇偶校验出错时自动重发次数 (Repeated Times)1：3次0：1次 相关函数 */
void U7816_FFR_REP_T_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_REP_T_Msk);
	tmpreg |= (SetValue & U7816_FFR_REP_T_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_REP_T_Get(void)
{
	return (U7816->FFR & U7816_FFR_REP_T_Msk);
}

/* 奇偶校验类型选择 (Parity)00：Even01：Odd10：Always 111：不校验，处理 相关函数 */
void U7816_FFR_PAR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_PAR_Msk);
	tmpreg |= (SetValue & U7816_FFR_PAR_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_PAR_Get(void)
{
	return (U7816->FFR & U7816_FFR_PAR_Msk);
}

/* Guard Time接收长度控制位 (Receive short Frame )1：Guard time为1 etu0：Guard time为2 etu 相关函数 */
void U7816_FFR_RFREN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_RFREN_Msk);
	tmpreg |= (SetValue & U7816_FFR_RFREN_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_RFREN_Get(void)
{
	return (U7816->FFR & U7816_FFR_RFREN_Msk);
}

/* 发送数据奇偶校验错的处理方式选择 (Transmit Repeat Enable)1：收到奇偶校验出错标志（error signal），根据T＝0协议自动进行回发。在单一byte重复发送次数超过REP_T后，置tx_parity_err标志，进行中断0：收到Error signal时不进行自动回发，置tx_parity_err标志，直接中断 相关函数 */
void U7816_FFR_TREPEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->FFR |= (U7816_FFR_TREPEN_Msk);
	}
	else
	{
		U7816->FFR &= ~(U7816_FFR_TREPEN_Msk);
	}
}

FunState U7816_FFR_TREPEN_Getable(void)
{
	if (U7816->FFR & (U7816_FFR_TREPEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收数据奇偶校验错的处理方式选择 (Receive Repeat Enable)1：奇偶校验错，根据T=0协议自动回发ERROR SIGNAL。单一BYTE连续接收次数超过REP_T后，置RX_PARITY_ERR标志，进行中断0：奇偶校验错，不自动发送ERROR SIGNAL，置RX_PARITY_ERR标志，进行中断 相关函数 */
void U7816_FFR_RREPEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->FFR |= (U7816_FFR_RREPEN_Msk);
	}
	else
	{
		U7816->FFR &= ~(U7816_FFR_RREPEN_Msk);
	}
}

FunState U7816_FFR_RREPEN_Getable(void)
{
	if (U7816->FFR & (U7816_FFR_RREPEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 传输次序，编码方式选择 (bit Direction Conversion)1：反向编码，先收发MSB；(收发数据+校验位)反逻辑电平0：正向编码，先收发LSB ； (收发数据+校验位)正逻辑电平 相关函数 */
void U7816_FFR_DICONV_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = U7816->FFR;
	tmpreg &= ~(U7816_FFR_DICONV_Msk);
	tmpreg |= (SetValue & U7816_FFR_DICONV_Msk);
	U7816->FFR = tmpreg;
}

uint32_t U7816_FFR_DICONV_Get(void)
{
	return (U7816->FFR & U7816_FFR_DICONV_Msk);
}

/* 发送时插入的Extra Guard Time时间（以ETU为单位）
(Transmit Extra Guard Time) 相关函数 */
void U7816_EGTR_Write(uint32_t SetValue)
{
	U7816->EGTR = (SetValue & U7816_EGTR_TXEGT_Msk);
}

uint32_t U7816_EGTR_Read(void)
{
	return (U7816->EGTR & U7816_EGTR_TXEGT_Msk);
}

/* U7816时钟输出分频控制寄存器(Clock Divider)，控制7816工作时钟分频数。U7816工作时钟与APBCLK的分频关系：F7816=FAPBCLK/(CLKDIV+1)特殊情况：CLK_DIV设置成0或1时，F7816=FAPBCLK/2注：7816协议规定的工作时钟范围是1~5MHZ。 相关函数 */
void U7816_PSC_Write(uint32_t SetValue)
{
	U7816->PSC = (SetValue & U7816_PSC_CLKDIV_Msk);
}

uint32_t U7816_PSC_Read(void)
{
	return (U7816->PSC & U7816_PSC_CLKDIV_Msk);
}

/* U7816预分频控制寄存器(Pre-Divider)，控制7816通信分频比（波特率）Baud ＝ F7816/(PDIV ＋ 1)

注意：PDIV最小可用值是0x1，应用禁止配置0x0 相关函数 */
void U7816_BGR_Write(uint32_t SetValue)
{
	U7816->BGR = (SetValue & U7816_BGR_PDIV_Msk);
}

uint32_t U7816_BGR_Read(void)
{
	return (U7816->BGR & U7816_BGR_PDIV_Msk);
}

/* U7816数据接收缓存寄存器 (Receive Buffer) 相关函数 */
uint32_t U7816_RXBUF_Read(void)
{
	return (U7816->RXBUF & U7816_RXBUF_RXBUF_Msk);
}

/* U7816数据发送缓存寄存器 (Transmit Buffer) 相关函数 */
void U7816_TXBUF_Write(uint32_t SetValue)
{
	U7816->TXBUF = (SetValue & U7816_TXBUF_TXBUF_Msk);
}

/* 数据接收中断使能位。对应RXIF中断标志位 (Receive Interrupt Enable)1：当RXIF寄存器置位时产生接收完成中断0：禁止接收完成中断 相关函数 */
void U7816_IER_RXIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->IER |= (U7816_IER_RXIE_Msk);
	}
	else
	{
		U7816->IER &= ~(U7816_IER_RXIE_Msk);
	}
}

FunState U7816_IER_RXIE_Getable(void)
{
	if (U7816->IER & (U7816_IER_RXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 数据发送中断使能位。对应TXIF中断标志位(Transmit Interrupt Enable)1：当TXIF寄存器置位时产生发送完成中断0：禁止发送完成中断 相关函数 */
void U7816_IER_TXIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->IER |= (U7816_IER_TXIE_Msk);
	}
	else
	{
		U7816->IER &= ~(U7816_IER_TXIE_Msk);
	}
}

FunState U7816_IER_TXIE_Getable(void)
{
	if (U7816->IER & (U7816_IER_TXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 线路状态中断使能位。对应ERRIF中断标志位(Line Status Interrupt Enable)1：当ERRIF寄存器置位时产生线路错误中断0：禁止线路错误中断 相关函数 */
void U7816_IER_LSIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		U7816->IER |= (U7816_IER_LSIE_Msk);
	}
	else
	{
		U7816->IER &= ~(U7816_IER_LSIE_Msk);
	}
}

FunState U7816_IER_LSIE_Getable(void)
{
	if (U7816->IER & (U7816_IER_LSIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* U7816接口发送了错误信号，正在等待对方重发数据；(Waiting for Repeat flag)状态机进入发送错误信号状态时置位，收到数据起始位或者进入发送状态时硬件清零；软件只读。 相关函数 */
FlagStatus U7816_ISR_WAIT_RPT_Chk(void)
{
	if (U7816->ISR & U7816_ISR_WAIT_RPT_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送数据忙标志。（发送完成后自动清零）(Transmission busy flag)1：处于数据发送状态，发送移位寄存器正在发送数据。（开始发送起始位置1，停止位中间清零）0：数据发送空闲 相关函数 */
FlagStatus U7816_ISR_TXBUSY_Chk(void)
{
	if (U7816->ISR & U7816_ISR_TXBUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收数据忙标志。（接收完成后自动清零）(Receiving busy flag)1：处于数据接收状态，接收移位寄存器正在接收数据。（收到起始位置1，收到停止位清零，若接收数据出错需重发，则回发error signal时清零。即数据及校验位接收之后，无论是否需要重发，都需要及时清除该标志）0：数据接收空闲 相关函数 */
FlagStatus U7816_ISR_RXBUSY_Chk(void)
{
	if (U7816->ISR & U7816_ISR_RXBUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送数据奇偶校验错误标志位。硬件置位，写1清零 (Transmit Parity Error,write 1 to clear) 相关函数 */
void U7816_ISR_TPARERR_Clr(void)
{
	U7816->ISR = U7816_ISR_TPARERR_Msk;
}

FlagStatus U7816_ISR_TPARERR_Chk(void)
{
	if (U7816->ISR & U7816_ISR_TPARERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收数据奇偶校验错误标志位。硬件置位，写1清零 (Receive Parity Error flag,write 1 to clear) 相关函数 */
void U7816_ISR_RPARERR_Clr(void)
{
	U7816->ISR = U7816_ISR_RPARERR_Msk;
}

FlagStatus U7816_ISR_RPARERR_Chk(void)
{
	if (U7816->ISR & U7816_ISR_RPARERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收帧格式错误标志位。硬件置位，写1清零 (Frame Error flag,write 1 to clear)1：帧格式有错误，接收到的frame字节长度有误或接收到的frame或者stop位有误0：接收数据时无奇偶校验错误 相关函数 */
void U7816_ISR_FRERR_Clr(void)
{
	U7816->ISR = U7816_ISR_FRERR_Msk;
}

FlagStatus U7816_ISR_FRERR_Chk(void)
{
	if (U7816->ISR & U7816_ISR_FRERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收溢出错误标志位。硬件置位，写1清零 (Receive Overflow Error,write 1 to clear)1：接收缓冲寄存器未被读出，又接收到新的数据，溢出错误标志有效。原接收缓冲寄存器内数据被新覆盖0：无溢出错误 相关函数 */
void U7816_ISR_OVERR_Clr(void)
{
	U7816->ISR = U7816_ISR_OVERR_Msk;
}

FlagStatus U7816_ISR_OVERR_Chk(void)
{
	if (U7816->ISR & U7816_ISR_OVERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收完成标志(Receive interrupt flag)，U7816接口控制器每收到1byte数据，根据接收的通道相应发出一次中断。硬件置位，读数据接收缓冲寄存器清零1：接收到1byte数据，数据接收缓冲器满0：未接收到数据，数据接收缓冲器空 相关函数 */
FlagStatus U7816_ISR_RXIF_Chk(void)
{
	if (U7816->ISR & U7816_ISR_RXIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送缓冲区空标志(Transmit interrupt flag)，上电复位后此标志就自动置位，表示缓冲区空，可以写入数据。软件写入数据后标志自动清除，数据从发送缓存移入移位寄存器后置11：数据发送缓冲器空0：数据发送缓冲器内有数据待发送 相关函数 */
FlagStatus U7816_ISR_TXIF_Chk(void)
{
	if (U7816->ISR & U7816_ISR_TXIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 错误标志(Error interrupt flag)，寄存器配置出错或传输过程中出错。此bit是TPARERR、RPARERR、FRERR、OVERR的或。软件通过清除以上错误标志寄存器来清除此bit。 相关函数 */
FlagStatus U7816_ISR_ERRIF_Chk(void)
{
	if (U7816->ISR & U7816_ISR_ERRIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void U7816_Deinit(void)
{
	U7816->CR = 0x00000000;
	U7816->FFR = 0x00000006;
	//U7816->EGTR = 0x00000000;
	//U7816->PSC = 0x00000000;
	//U7816->BGR = 0x00000100;
	//U7816->RXBUF = 0x00000000;
	//U7816->TXBUF = 0x00000000;
	//U7816->IER = 0x00000000;
	//U7816->ISR = 0x00000002;
}


/* U7816完整参数初始化函数 */
/*******************************************
函数名：U7816_Init
函数功能: U7816完整参数初始化函数
输入参数：U7816x：U78160、U78161
          para：U7816_InitTypeDef数组
输出参数: 无
*******************************************/
void U7816_Init(U7816_InitTypeDef* para)
{
	
	U7816_CR_TXEN_Setable(para->TXEN);		//发送使能
	U7816_CR_RXEN_Setable(para->RXEN);		//接受使能
	U7816_CR_CKOEN_Setable(para->CKOEN);		//U7816时钟CLK输出使能控制
	U7816_CR_HPUAT_Setable(para->HPUAT);		//U7816通道数据发送强上拉电阻自动有效控制
	U7816_CR_HPUEN_Setable(para->HPUEN);		//U7816通道强上拉使能控制
		
	U7816_FFR_ERSW_Set(para->ERSW);		//ERROR SIGNAL宽度选择
	U7816_FFR_ERSGD_Set(para->ERSGD);		//ERROR SIGNAL后GUARDTIME宽度选择（仅在发送时有效）
	U7816_FFR_BGTEN_Setable(para->BGTEN);		//BGT（block guard time）控制
	U7816_FFR_REP_T_Set(para->REP_T);		//控制接收数据奇偶校验出错时自动重发次数
	U7816_FFR_PAR_Set(para->PAR);		//奇偶校验类型选择
	U7816_FFR_SFREN_Set(para->SFREN);		//Guard Time长度控制位（发送时严格按照协议2etu） 
  U7816_FFR_RFREN_Set(para->RFREN);   //接收Guard Time长度控制位
	U7816_FFR_TREPEN_Setable(para->TREPEN);		//缓发送数据奇偶校验错的处理方式选择
	U7816_FFR_RREPEN_Setable(para->RREPEN);		//接收数据奇偶校验错的处理方式选择
	U7816_FFR_DICONV_Set(para->DICONV);		//传输次序反向编码使能
	
	U7816_EGTR_Write(para->TXEGT);		//发送时插入的EGT时间（以ETU为单位）
	U7816_PSC_Write(para->CLKDIV);		//U7816时钟输出分频控制寄存器
	U7816_BGR_Write(para->PDIV);		//U7816预分频控制寄存器，控制7816通信分频比（波特率）
	
	U7816_IER_RXIE_Setable(para->RXIE);		//数据接收中断使能位。对应RX_FLAG中断标志位
	U7816_IER_TXIE_Setable(para->TXIE);		//数据发送中断使能位。对应TX_FLAG中断标志位
	U7816_IER_LSIE_Setable(para->LSIE);		//线路状态中断使能位。对应ERROR_FLAG中断标志位
	
}

/******END OF FILE****/
