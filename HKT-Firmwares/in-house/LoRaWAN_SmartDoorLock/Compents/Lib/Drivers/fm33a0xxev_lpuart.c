/**
  ******************************************************************************
  * @file    fm33a0xxev_lpuart.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_lpuart.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup LPUART 
  * @brief LPUART driver modules
  * @{
  */ 

/* LPUART通信标志，只读 (Busy)
1：LPUART正在通信中
0：LPUART空闲 相关函数 */
FlagStatus LPUARTx_CSR_BUSY_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->CSR & LPUARTx_CSR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 数据接收唤醒条件配置 相关函数 */
void LPUARTx_CSR_WKBYTECFG_Setable(LPUART_Type* LPUARTx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx->CSR |= (LPUARTx_CSR_WKBYTECFG_Msk);
	}
	else
	{
		LPUARTx->CSR &= ~(LPUARTx_CSR_WKBYTECFG_Msk);
	}
}

/* 数据接收唤醒条件配置 (Wakeup Byte Config)
1：接收完1字节，并且奇偶校验和STOP位都正确，才触发唤醒中断
0：接收完1字节，不检查校验位和STOP位，直接触发唤醒中断 相关函数 */
FlagStatus LPUARTx_CSR_WKBYTECFG_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->CSR & LPUARTx_CSR_WKBYTECFG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 唤醒中断事件配置，用于控制何种事件下向CPU提供唤醒中断
(Receive Wakeup Event)
00：START位检测唤醒
01：1byte数据接收完成
10：接收数据匹配成功
11：RXD下降沿检测 相关函数 */
void LPUARTx_CSR_RXEV_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_RXEV_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_RXEV_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_RXEV_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_RXEV_Msk);
}

/* RX与TX引脚交换
0：默认
1：交换 */
void LPUARTx_CSR_IOSWAP_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_IOSWAP_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_IOSWAP_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_IOSWAP_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_IOSWAP_Msk);
}

/* DMA发送完成中断使能，仅在LPUART通过DMA进行发送时有效 (DMA Transmit Interrupt Config)
1：IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出
0：是否允许中断信号输出仅由IE决定 相关函数 */
void LPUARTx_CSR_DMATXIFCFG_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->CSR |= (LPUARTx_CSR_DMATXIFCFG_Msk);
	}
	else
	{
		LPUARTx ->CSR &= ~(LPUARTx_CSR_DMATXIFCFG_Msk);
	}
}

FunState LPUARTx_CSR_DMATXIFCFG_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->CSR & (LPUARTx_CSR_DMATXIFCFG_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 数据发送/接收时的位顺序 (Bit Order)
0：LSB first
1：MSB first 相关函数 */
void LPUARTx_CSR_BITORD_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_BITORD_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_BITORD_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_BITORD_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_BITORD_Msk);
}

/* 停止位宽度配置，仅对发送帧格式有效，接收时不判断停止位个数 (Stop bit Config)
0：1位停止位
1：2位停止位 相关函数 */
void LPUARTx_CSR_STOPCFG_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_STOPCFG_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_STOPCFG_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_STOPCFG_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_STOPCFG_Msk);
}

/* 每帧数据长度选择；此寄存器对数据发送和接收同时有效
(Payload Data length Select)
00：7位数据
01：8位数据
10：9位数据
11：6位数据 相关函数 */
void LPUARTx_CSR_PDSEL_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_PDSEL_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_PDSEL_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_PDSEL_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_PDSEL_Msk);
}

/* 校验位配置；此寄存器对数据发送和接收同时有效 (Parity)
00：无校验位
01：偶校验
10：奇校验
11：RFU 相关函数 */
void LPUARTx_CSR_PARITY_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_PARITY_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_PARITY_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_PARITY_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_PARITY_Msk);
}

/* 接收数据极性配置 (Receive Polarity)
0：正向
1：取反 相关函数 */
void LPUARTx_CSR_RXPOL_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_RXPOL_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_RXPOL_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_RXPOL_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_RXPOL_Msk);
}

/* 发送数据极性配置 (Transmit Polarity)
0：正向
1：取反 相关函数 */
void LPUARTx_CSR_TXPOL_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->CSR;
	tmpreg &= ~(LPUARTx_CSR_TXPOL_Msk);
	tmpreg |= (SetValue & LPUARTx_CSR_TXPOL_Msk);
	LPUARTx ->CSR = tmpreg;
}

uint32_t LPUARTx_CSR_TXPOL_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->CSR & LPUARTx_CSR_TXPOL_Msk);
}

/* 接收使能，1有效 (Receive Enable) 相关函数 */
void LPUARTx_CSR_RXEN_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->CSR |= (LPUARTx_CSR_RXEN_Msk);
	}
	else
	{
		LPUARTx ->CSR &= ~(LPUARTx_CSR_RXEN_Msk);
	}
}

FunState LPUARTx_CSR_RXEN_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->CSR & (LPUARTx_CSR_RXEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 发送使能，1有效 (Transmit Enable) 相关函数 */
void LPUARTx_CSR_TXEN_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->CSR |= (LPUARTx_CSR_TXEN_Msk);
	}
	else
	{
		LPUARTx ->CSR &= ~(LPUARTx_CSR_TXEN_Msk);
	}
}

FunState LPUARTx_CSR_TXEN_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->CSR & (LPUARTx_CSR_TXEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收唤醒事件中断使能，1有效 (Receive Event Interrupt Enable) 相关函数 */
void LPUARTx_IER_RXEV_IE_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->IER |= (LPUARTx_IER_RXEV_IE_Msk);
	}
	else
	{
		LPUARTx ->IER &= ~(LPUARTx_IER_RXEV_IE_Msk);
	}
}

FunState LPUARTx_IER_RXEV_IE_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->IER & (LPUARTx_IER_RXEV_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收错误中断使能，1有效 (Receive Error Interrupt Enable) 相关函数 */
void LPUARTx_IER_RXERR_IE_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->IER |= (LPUARTx_IER_RXERR_IE_Msk);
	}
	else
	{
		LPUARTx ->IER &= ~(LPUARTx_IER_RXERR_IE_Msk);
	}
}

FunState LPUARTx_IER_RXERR_IE_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->IER & (LPUARTx_IER_RXERR_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收缓存满中断使能，1有效 (Receive Buffer Full Interrupt Enable) 相关函数 */
void LPUARTx_IER_RXBF_IE_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->IER |= (LPUARTx_IER_RXBF_IE_Msk);
	}
	else
	{
		LPUARTx ->IER &= ~(LPUARTx_IER_RXBF_IE_Msk);
	}
}

FunState LPUARTx_IER_RXBF_IE_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->IER & (LPUARTx_IER_RXBF_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 发送缓存空中断使能，1有效 (Transmit Buffer Empty Interrupt Enable) 相关函数 */
void LPUARTx_IER_TXBE_IE_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->IER |= (LPUARTx_IER_TXBE_IE_Msk);
	}
	else
	{
		LPUARTx ->IER &= ~(LPUARTx_IER_TXBE_IE_Msk);
	}
}

FunState LPUARTx_IER_TXBE_IE_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->IER & (LPUARTx_IER_TXBE_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 发送缓存空且发送移位寄存器空中断使能，1有效 (Transmit Shift register Interrupt Enable) 相关函数 */
void LPUARTx_IER_TXSE_IE_Setable(LPUART_Type* LPUARTx,FunState NewState)
{
	if (NewState == ENABLE)
	{
		LPUARTx ->IER |= (LPUARTx_IER_TXSE_IE_Msk);
	}
	else
	{
		LPUARTx ->IER &= ~(LPUARTx_IER_TXSE_IE_Msk);
	}
}

FunState LPUARTx_IER_TXSE_IE_Getable(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->IER & (LPUARTx_IER_TXSE_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收唤醒事件中断标志，硬件置位，软件写1清零 (Receive Event Interrupt Flag)
中断标志触发源由LPUxCR.RXEV寄存器配置。 相关函数 */
void LPUARTx_ISR_RXEVF_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_RXEVF_Msk;
}

FlagStatus LPUARTx_ISR_RXEVF_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_RXEVF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送缓存溢出错误，硬件置位，软件写1清零 (Transmit Overflow Error)
当发送缓存中的数据还未进入移位寄存器发送时，软件向发送缓存写入新数据，将触发TXOV标志置位。 相关函数 */
void LPUARTx_ISR_TXOV_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_TXOV_Msk;
}

FlagStatus LPUARTx_ISR_TXOV_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_TXOV_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 奇偶校验错误中断标志，硬件置位，软件写1清零 (Parity Error) 相关函数 */
void LPUARTx_ISR_PERR_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_PERR_Msk;
}

FlagStatus LPUARTx_ISR_PERR_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_PERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 帧格式错误中断标志，硬件置位，软件写1清零 (Frame Error) 相关函数 */
void LPUARTx_ISR_FERR_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_FERR_Msk;
}

FlagStatus LPUARTx_ISR_FERR_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_FERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收缓存溢出错误中断标志，当接收缓存满的情况下，收到新的数据时置位；硬件置位，软件写1清零 (Receive Buffer Overflow Error) 相关函数 */
void LPUARTx_ISR_OERR_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_OERR_Msk;
}

FlagStatus LPUARTx_ISR_OERR_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_OERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 接收缓存满中断标志，硬件置位，软件写1或者读取RXBUF时清零 (Receive Buffer Full) 相关函数 */
void LPUARTx_ISR_RXBF_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_RXBF_Msk;
}

FlagStatus LPUARTx_ISR_RXBF_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_RXBF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送缓存空中断标志，硬件置位，写入TXBUF时清零 (Transmit Buffer Empty) 相关函数 */
FlagStatus LPUARTx_ISR_TXBE_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_TXBE_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送缓存空且发送移位寄存器空中断标志，硬件置位，软件写1或者发送数据被载入移位寄存器时清零 (Transmit Shift register Empty) 相关函数 */
void LPUARTx_ISR_TXSE_Clr(LPUART_Type* LPUARTx)
{
	LPUARTx ->ISR = LPUARTx_ISR_TXSE_Msk;
}

FlagStatus LPUARTx_ISR_TXSE_Chk(LPUART_Type* LPUARTx)
{
	if (LPUARTx ->ISR & LPUARTx_ISR_TXSE_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* LPUART每个bit的位宽调制控制信号，参见错误!未找到引用源。错误!未找到引用源。 (Bit Modulation Control) 相关函数 */
void LPUARTx_BMR_MCTL_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->BMR;
	tmpreg &= ~(LPUARTx_BMR_MCTL_Msk);
	tmpreg |= (SetValue & LPUARTx_BMR_MCTL_Msk);
	LPUARTx ->BMR = tmpreg;
}

uint32_t LPUARTx_BMR_MCTL_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->BMR & LPUARTx_BMR_MCTL_Msk);
}

/* 波特率控制（bps）
000：9600
001：4800
010：2400
011：1200
100：600
101/110/111：300 相关函数 */
void LPUARTx_BMR_BAUD_Set(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = LPUARTx ->BMR;
	tmpreg &= ~(LPUARTx_BMR_BAUD_Msk);
	tmpreg |= (SetValue & LPUARTx_BMR_BAUD_Msk);
	LPUARTx ->BMR = tmpreg;
}

uint32_t LPUARTx_BMR_BAUD_Get(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->BMR & LPUARTx_BMR_BAUD_Msk);
}

/* 接收数据缓存寄存器 (Receive Buffer) 相关函数 */
uint32_t LPUARTx_RXBUF_Read(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->RXBUF & LPUARTx_RXBUF_RXBUF_Msk);
}

/* 发送数据缓存寄存器 (Transmit Buffer) 相关函数 */
void LPUARTx_TXBUF_Write(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	LPUARTx ->TXBUF = (SetValue & LPUARTx_TXBUF_TXBUF_Msk);
}

/* 第一帧接收比较数据，如果RXEV=10，当接收到的第一帧数据与MATD相同时，触发RXEVF中断，可以用于休眠模式下的数据接收唤醒。
(Matched Data) 相关函数 */
void LPUARTx_DMR_Write(LPUART_Type* LPUARTx,uint32_t SetValue)
{
	LPUARTx ->DMR = (SetValue & LPUARTx_DMR_MATD_Msk);
}

uint32_t LPUARTx_DMR_Read(LPUART_Type* LPUARTx)
{
	return (LPUARTx ->DMR & LPUARTx_DMR_MATD_Msk);
}


void LPUARTx_Deinit(LPUART_Type* LPUARTx)
{
	//LPUARTx ->CSR = 0x00000000;
	//LPUARTx ->IER = 0x00000000;
	//LPUARTx ->ISR = 0x00000002;
	//LPUARTx ->BMR = 0x00000000;
	//LPUARTx ->RXBUF = ;
	//LPUARTx ->TXBUF = ;
	//LPUARTx ->DMR = ;
}

/******END OF FILE****/
