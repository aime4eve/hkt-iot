/**
  ******************************************************************************
  * @file    fm33a0xxev_spi.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_spi.h"
#include "fm33a0xxev_rmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup SPI 
  * @brief SPI driver modules
  * @{
  */ 

/* MOSI和MISO引脚交换 (IO swapping)
0：默认引脚顺序
1：交换引脚顺序 相关函数 */
void SPIx_CR1_IOSWAP_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_IOSWAP_Msk);
	tmpreg |= (SetValue & SPIx_CR1_IOSWAP_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_IOSWAP_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_IOSWAP_Msk);
}

/* Master Sampling Position Adjustment，Master对MISO信号的采样位置调整，用于高速通信时补偿PCB走线延迟
1：采样点延迟半个SCK周期
0：不调整 相关函数 */
void SPIx_CR1_MSPA_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_MSPA_Msk);
	tmpreg |= (SetValue & SPIx_CR1_MSPA_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_MSPA_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_MSPA_Msk);
}

/* Slave Sending Position Adjustment，Slave MISO发送位置调整
1：提前半个SCK周期发送
0：不调整 相关函数 */
void SPIx_CR1_SSPA_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_SSPA_Msk);
	tmpreg |= (SetValue & SPIx_CR1_SSPA_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_SSPA_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_SSPA_Msk);
}

/* Master/Slave模式选择。(Master Mode)1：Master模式0：Slave模式 相关函数 */
void SPIx_CR1_MM_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_MM_Msk);
	tmpreg |= (SetValue & SPIx_CR1_MM_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_MM_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_MM_Msk);
}

/* Master模式下，每发送完一帧后加入至少(1+WAIT)个SCK cycle等待时间，再传输下一帧的数据。如果SSN由硬件控制，并且SSNM=1，则硬件会自动拉高SSN。 相关函数 */
void SPIx_CR1_WAIT_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_WAIT_Msk);
	tmpreg |= (SetValue & SPIx_CR1_WAIT_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_WAIT_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_WAIT_Msk);
}

/* Master模式波特率配置位: (Baud rate)000： fAPBCLK/2001： fAPBCLK/4010： fAPBCLK/8011： fAPBCLK/16100： fAPBCLK/32101： fAPBCLK/64110： fAPBCLK/128111： fAPBCLK/256当通信正在进行的时候，不能修改这些位。 相关函数 */
void SPIx_CR1_BAUD_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_BAUD_Msk);
	tmpreg |= (SetValue & SPIx_CR1_BAUD_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_BAUD_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_BAUD_Msk);
}

/* 帧格式 (LSB First) 0：先发送MSB1：先发送LSB注：当通信在进行时不能改变该位的值。 相关函数 */
void SPIx_CR1_LSBF_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_LSBF_Msk);
	tmpreg |= (SetValue & SPIx_CR1_LSBF_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_LSBF_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_LSBF_Msk);
}

void SPIx_CR1_CPHOL_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_CPHOL_Msk);
	tmpreg |= (SetValue & SPIx_CR1_CPHOL_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_CPHOL_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_CPHOL_Msk);
}

/* 时钟相位选择 (Clock Phase)1：第二个时钟边沿是第一个捕捉边沿0：第一个时钟边沿是第一个捕捉边沿注：当通信在进行时不能改变该位的值。 相关函数 */
void SPIx_CR1_CPHA_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR1 ;
	tmpreg &= ~(SPIx_CR1_CPHA_Msk);
	tmpreg |= (SetValue & SPIx_CR1_CPHA_Msk);
	SPIx->CR1  = tmpreg;
}

uint32_t SPIx_CR1_CPHA_Get(SPI_Type* SPIx)
{
	return (SPIx->CR1  & SPIx_CR1_CPHA_Msk);
}

/* 4线半双工协议下是否在读操作中插入dummy cycle
(Dummy cycle Enable)
0：不插入dummy cycle
1：在读命令之后插入一个dummy cycle 相关函数 */
void SPIx_CR2_DUMMY_EN_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_DUMMY_EN_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_DUMMY_EN_Msk);
	}
}

FunState SPIx_CR2_DUMMY_EN_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_DUMMY_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* RXONLY控制位，此寄存器置位时，SPI可以连续接收，无需软件写TXBUF (Receive Only mode)
1：启动Master的单接收模式
0：关闭单接收模式（收发全双工） 相关函数 */
void SPIx_CR2_RXO_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_RXO_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_RXO_Msk);
	}
}

FunState SPIx_CR2_RXO_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_RXO_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 通信数据字长配置 (Data Length)
00：8bit
01：16bit
10：24bit
11：32bit 相关函数 */
void SPIx_CR2_DLEN_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_DLEN_Msk);
	tmpreg |= (SetValue & SPIx_CR2_DLEN_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_DLEN_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_DLEN_Msk);
}

/* 通信模式选择 (Half-Duplex mode)
0：标准SPI模式，4线全双工
1：DCN模式，4线半双工 相关函数 */
void SPIx_CR2_HALFDUPLEX_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_HALFDUPLEX_Msk);
	tmpreg |= (SetValue & SPIx_CR2_HALFDUPLEX_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_HALFDUPLEX_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_HALFDUPLEX_Msk);
}

/* 半双工模式下主机读写操作配置 (Read/Write config for Half-Duplex mode)
0：4线半双工协议下主机写入从机
1：4线半双工协议下主机读取从机 相关函数 */
void SPIx_CR2_HD_RW_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_HD_RW_Msk);
	tmpreg |= (SetValue & SPIx_CR2_HD_RW_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_HD_RW_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_HD_RW_Msk);
}

/* 半双工模式下定义command帧长度 (Command 8 bits)
1：command帧固定为8bit
0：command帧长度由DLEN定义 相关函数 */
void SPIx_CR2_CMD8b_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_CMD8b_Msk);
	tmpreg |= (SetValue & SPIx_CR2_CMD8b_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_CMD8b_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_CMD8b_Msk);
}

/* Master模式下SSN控制模式选择 (SSN mode)
1：每发送完一帧后Master拉高SSN，维持高电平时间由WAIT寄存器控制
0：每发送完一帧后Master保持SSN为低 相关函数 */
void SPIx_CR2_SSNM_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_SSNM_Msk);
	tmpreg |= (SetValue & SPIx_CR2_SSNM_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_SSNM_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_SSNM_Msk);
}

/* TXONLY硬件自动清空的使能 (TXONLY auto-clear enable)1：TXONLY硬件自动清零有效，软件使能TXO后，等待发送完毕后，硬件清零0：关闭TXONLY硬件自动清零 相关函数 */
void SPIx_CR2_TXO_AC_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_TXO_AC_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_TXO_AC_Msk);
	}
}

FunState SPIx_CR2_TXO_AC_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_TXO_AC_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* TXONLY控制位 (Transmit Only mode enable)
1：启动Master的单发送模式
0：关闭单发送模式（收发全双工） 相关函数 */
void SPIx_CR2_TXO_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_TXO_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_TXO_Msk);
	}
}

FunState SPIx_CR2_TXO_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_TXO_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Master模式下，如果SSNSEN为1，软件可以通过此位控制SSN输出电平 1：SSN输出高电平
0：SSN输出低电平 相关函数 */
void SPIx_CR2_SSN_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->CR2 ;
	tmpreg &= ~(SPIx_CR2_SSN_Msk);
	tmpreg |= (SetValue & SPIx_CR2_SSN_Msk);
	SPIx->CR2  = tmpreg;
}

uint32_t SPIx_CR2_SSN_Get(SPI_Type* SPIx)
{
	return (SPIx->CR2  & SPIx_CR2_SSN_Msk);
}

/* Master模式下，软件控制SSN使能 (SSN Software Enable)
1：Master模式下SSN输出由软件控制
0：Master模式下SSN输出由硬件自动控制 相关函数 */
void SPIx_CR2_SSNSEN_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_SSNSEN_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_SSNSEN_Msk);
	}
}

FunState SPIx_CR2_SSNSEN_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_SSNSEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SPI使能 (SPI enable)1：使能SPI0：关闭SPI，清空发送接收缓存 相关函数 */
void SPIx_CR2_SPIEN_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->CR2  |= (SPIx_CR2_SPIEN_Msk);
	}
	else
	{
		SPIx->CR2  &= ~(SPIx_CR2_SPIEN_Msk);
	}
}

FunState SPIx_CR2_SPIEN_Getable(SPI_Type* SPIx)
{
	if (SPIx->CR2  & (SPIx_CR2_SPIEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Transmit Buffer Clear，软件写1清除发送缓存，写0无效 相关函数 */
void SPIx_CR3_TXBFC_Clr(SPI_Type* SPIx)
{
	SPIx->CR3  = SPIx_CR3_TXBFC_Msk;
}

/* Receive Buffer Clear，软件写1清除发送缓存，写0无效 相关函数 */
void SPIx_CR3_RXBFC_Clr(SPI_Type* SPIx)
{
	SPIx->CR3  = SPIx_CR3_RXBFC_Msk;
}

/* Master Error Clear，软件写1清除HSPISTA.MERR寄存器 相关函数 */
void SPIx_CR3_MERRC_Clr(SPI_Type* SPIx)
{
	SPIx->CR3  = SPIx_CR3_MERRC_Msk;
}

/* Slave Error Clear，软件写1清除HSPISTA.SERR寄存器 相关函数 */
void SPIx_CR3_SERRC_Clr(SPI_Type* SPIx)
{
	SPIx->CR3  = SPIx_CR3_SERRC_Msk;
}

/* SPI错误中断使能 (Error Interrupt Enable) 相关函数 */
void SPIx_IER_ERRIE_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->IER  |= (SPIx_IER_ERRIE_Msk);
	}
	else
	{
		SPIx->IER  &= ~(SPIx_IER_ERRIE_Msk);
	}
}

FunState SPIx_IER_ERRIE_Getable(SPI_Type* SPIx)
{
	if (SPIx->IER  & (SPIx_IER_ERRIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 发送完成中断使能 (Transmit Interrupt Enable) 相关函数 */
void SPIx_IER_TXIE_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->IER  |= (SPIx_IER_TXIE_Msk);
	}
	else
	{
		SPIx->IER  &= ~(SPIx_IER_TXIE_Msk);
	}
}

FunState SPIx_IER_TXIE_Getable(SPI_Type* SPIx)
{
	if (SPIx->IER  & (SPIx_IER_TXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 接收完成中断使能 (Receive Interrupt Enable) 相关函数 */
void SPIx_IER_RXIE_Setable(SPI_Type* SPIx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		SPIx->IER  |= (SPIx_IER_RXIE_Msk);
	}
	else
	{
		SPIx->IER  &= ~(SPIx_IER_RXIE_Msk);
	}
}

FunState SPIx_IER_RXIE_Getable(SPI_Type* SPIx)
{
	if (SPIx->IER  & (SPIx_IER_RXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 半双工模式下（HALFDUPLEX=1），配置在每个数据帧的最后bit发送的DCN信号电平 (Data/Command transmit config)
0：DCN=0，表示命令帧
1：DCN=1，表示数据帧
软件应在发送前设置DCN_TX寄存器，如果DCN_TX=0，硬件在完成一帧发送后，自动将DCN_TX置1，即默认只会发送一个命令帧，后续都是数据帧。 相关函数 */
void SPIx_ISR_DCN_TX_Set(SPI_Type* SPIx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = SPIx->ISR ;
	tmpreg &= ~(SPIx_ISR_DCN_TX_Msk);
	tmpreg |= (SetValue & SPIx_ISR_DCN_TX_Msk);
	SPIx->ISR  = tmpreg;
}

uint32_t SPIx_ISR_DCN_TX_Get(SPI_Type* SPIx)
{
	return (SPIx->ISR  & SPIx_ISR_DCN_TX_Msk);
}

/* 接收缓存溢出，软件写1清零 (Receive Collision flag,write 1 to flag) 相关函数 */
void SPIx_ISR_RXCOL_Clr(SPI_Type* SPIx)
{
	SPIx->ISR  = SPIx_ISR_RXCOL_Msk;
}

FlagStatus SPIx_ISR_RXCOL_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_RXCOL_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 发送缓存溢出，软件写1清零 (Transmit Collision flag,write 1 to clear) 相关函数 */
void SPIx_ISR_TXCOL_Clr(SPI_Type* SPIx)
{
	SPIx->ISR  = SPIx_ISR_TXCOL_Msk;
}

FlagStatus SPIx_ISR_TXCOL_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_TXCOL_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SPI空闲标志，只读 (busy flag)
1：SPI传输进行中
0：SPI传输空闲 相关函数 */
FlagStatus SPIx_ISR_BUSY_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Master Error标志(Master Error flag)
当Master下传输未满8位SSN就被拉高时，MERR置位 相关函数 */
FlagStatus SPIx_ISR_MERR_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_MERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Slave Error标志(Slave Error flag)
当Slave下传输未满8位SSN就被拉高时，SERR置位 相关函数 */
FlagStatus SPIx_ISR_SERR_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_SERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* TX Buffer Empty标志位(TX Buffer Empty flag)
1：发送缓存空，软件写TXBUF清零
0：发送缓存满 相关函数 */
FlagStatus SPIx_ISR_TXBE_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_TXBE_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* RX Buffer Full标志位(RX Buffer Full flag)
1：接收缓存满，软件读RXBUF清零
0：接收缓存空 相关函数 */
FlagStatus SPIx_ISR_RXBF_Chk(SPI_Type* SPIx)
{
	if (SPIx->ISR  & SPIx_ISR_RXBF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SPI发送缓存 (Transmit Buffer) 相关函数 */
void SPIx_TXBUF_Write(SPI_Type* SPIx, uint32_t SetValue)
{
	SPIx->TXBUF  = (SetValue);
}

/* SPI接收缓存 (Receive Buffer) 相关函数 */
uint32_t SPIx_RXBUF_Read(SPI_Type* SPIx)
{
	return (SPIx->RXBUF );
}

void SPIx_Deinit(SPI_Type* SPIx)
{
    RMU_PRSTEN_Write(0x13579BDF);
    if (SPIx == SPI0)
    {
        RMU_APBRST2_SPI0RST_Setable(ENABLE);
        RMU_APBRST2_SPI0RST_Setable(DISABLE);
    }
    else if (SPIx == SPI1)
    {
        RMU_APBRST2_SPI1RST_Setable(ENABLE);
        RMU_APBRST2_SPI1RST_Setable(DISABLE);
    }
    else if (SPIx == SPI2)
    {
        RMU_APBRST1_SPI2RST_Setable(ENABLE);
        RMU_APBRST1_SPI2RST_Setable(DISABLE);
    }
    else if (SPIx == SPI3)
    {
        RMU_APBRST1_SPI3RST_Setable(ENABLE);
        RMU_APBRST1_SPI3RST_Setable(DISABLE);
    }
    else if (SPIx == SPI4)
    {
        RMU_APBRST1_SPI4RST_Setable(ENABLE);
        RMU_APBRST1_SPI4RST_Setable(DISABLE);
    }
    RMU_PRSTEN_Write(0x00000000);
}

/********************************
 SPI的SSN设置电平低的函数
********************************/
void SPI_SSN_Set_Low(SPI_Type* SPIx)//驱动级
{
	SPIx_CR2_SSN_Set(SPIx, SPIx_CR2_SSN_LOW);//拉低SSN信号
}

/********************************
 SPI的SSN设置电平高的函数
********************************/
void SPI_SSN_Set_High(SPI_Type* SPIx)//驱动级
{
	SPIx_CR2_SSN_Set(SPIx, SPIx_CR2_SSN_HIGH);//拉高SSN信号
}

/********************************
 SPI发送并接收一个字节的函数
功能：SPI发送和接收一个字节
输入：SPI要发送的字节，输出：SPI接收字节
********************************/
uint8_t SPI_RW_Byte(SPI_Type* SPIx, uint8_t data)//SPI发送并接收一字节
{
  unsigned char rx_data;
	uint32_t Temp32;
	uint32_t i;
	
	Temp32 = SPIx_CR1_BAUD_Get(SPIx);
	Temp32 = Temp32>>SPIx_CR1_BAUD_Pos;
	Temp32 = 0x1U<<(Temp32+1);
	Temp32 = Temp32*8;
	
    for(i=0; i<Temp32; i++)
    {
    	if (SPIx_ISR_TXBE_Chk(SPIx) == SET)
		{
			break;
		}		
    }
    SPIx_TXBUF_Write(SPIx, data);

    for(i=0; i<Temp32; i++)
    {
    	if (SPIx_ISR_RXBF_Chk(SPIx) == SET)
        {
            break;
        }
    }
    rx_data = SPIx_RXBUF_Read(SPIx);
    return rx_data;
}

/******END OF FILE****/
