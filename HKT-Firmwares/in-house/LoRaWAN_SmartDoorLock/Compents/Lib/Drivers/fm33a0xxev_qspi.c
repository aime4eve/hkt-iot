/**
  ******************************************************************************
  * @file    fm33a0xxev_qspi.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_qspi.h"
#include "fm33a0xxev_rmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup QSPI 
  * @brief QSPI driver modules
  * @{
  */ 

void QSPI_CR_PRESCALER_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
    
    SetValue <<= QSPI_CR_PRESCALER_Pos;
    
	tmpreg = QSPI->CR;
	tmpreg &= ~(QSPI_CR_PRESCALER_Msk);
	tmpreg |= (SetValue & QSPI_CR_PRESCALER_Msk);
	QSPI->CR = tmpreg;
}

uint32_t QSPI_CR_PRESCALER_Get(void)
{
	return (QSPI->CR & QSPI_CR_PRESCALER_Msk);
}

/* 轮询匹配模式
0：AND模式，所有bit都匹配才置位SMF
1：OR模式，至少1bit匹配就会置位SMF 相关函数 */
void QSPI_CR_PMM_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = QSPI->CR;
	tmpreg &= ~(QSPI_CR_PMM_Msk);
	tmpreg |= (SetValue & QSPI_CR_PMM_Msk);
	QSPI->CR = tmpreg;
}

uint32_t QSPI_CR_PMM_Get(void)
{
	return (QSPI->CR & QSPI_CR_PMM_Msk);
}

/* 超时中断使能
0：禁止超时中断
1：允许超时中断 相关函数 */
void QSPI_CR_TOIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_TOIE_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_TOIE_Msk);
	}
}

FunState QSPI_CR_TOIE_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_TOIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 状态匹配中断使能
0：禁止状态匹配中断
1：允许状态匹配中断 相关函数 */
void QSPI_CR_SMIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_SMIE_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_SMIE_Msk);
	}
}

FunState QSPI_CR_SMIE_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_SMIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* FIFO水位中断使能
0：禁止FIFO水位中断
1：允许FIFO水位中断 相关函数 */
void QSPI_CR_FTIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_FTIE_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_FTIE_Msk);
	}
}

FunState QSPI_CR_FTIE_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_FTIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 传输完成中断使能
0：禁止传输完成中断
1：允许传输完成中断 相关函数 */
void QSPI_CR_TCIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_TCIE_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_TCIE_Msk);
	}
}

FunState QSPI_CR_TCIE_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_TCIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 传输错误中断使能
0：禁止传输错误中断
1：允许传输错误中断 相关函数 */
void QSPI_CR_TEIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_TEIE_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_TEIE_Msk);
	}
}

FunState QSPI_CR_TEIE_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_TEIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* FIFO水位寄存器，仅外设模式下起效
外设写模式：
0：FTF在FIFO空字节大于等于1时置位
1：FTF在FIFO空字节大于等于2时置位
……
15：FTF在FIFO空字节等于16时置位
外设读模式：
0：FTF在FIFO中字节数大于等于1时置位
1：FTF在FIFO中字节数大于等于2时置位
……
15：FTF在FIFO中字节数等于16时置位 相关函数 */
void QSPI_CR_FIFOTHR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = QSPI->CR;
	tmpreg &= ~(QSPI_CR_FIFOTHR_Msk);
	tmpreg |= (SetValue & QSPI_CR_FIFOTHR_Msk);
	QSPI->CR = tmpreg;
}

uint32_t QSPI_CR_FIFOTHR_Get(void)
{
	return (QSPI->CR & QSPI_CR_FIFOTHR_Msk);
}

/* 延迟采样使能
0：关闭延迟采样功能
1：使能延迟采样功能 相关函数 */
void QSPI_CR_SSHFT_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_SSHFT_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_SSHFT_Msk);
	}
}

FunState QSPI_CR_SSHFT_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_SSHFT_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 总线超时使能，此寄存器仅在存储器映射模式下有效
当BUSY置位后，如果QuadSPI不发起对QSPI存储器的访问，超时寄存器开始计数，计数溢出长度由TIMEOUT寄存器定义。当QSPI总线长时间无动作，计数器溢出，nCS被自动拉高，强制结束当前传输过程。
0：关闭超时功能
1：使能超时功能 相关函数 */
void QSPI_CR_TCEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_TCEN_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_TCEN_Msk);
	}
}

FunState QSPI_CR_TCEN_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_TCEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* DMA使能
0：DMA功能关闭，QuadSPI不会发送DMA请求
1：DMA功能开启，QuadSPI在满足条件时发送DMA请求 相关函数 */
void QSPI_CR_DMAEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_DMAEN_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_DMAEN_Msk);
	}
}

FunState QSPI_CR_DMAEN_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_DMAEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 当前传输终止寄存器，软件写1终止传输，硬件拉高nCS后自动清零 相关函数 */
void QSPI_CR_ABORT_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_ABORT_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_ABORT_Msk);
	}
}

FunState QSPI_CR_ABORT_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_ABORT_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* QuadSPI模块使能
0：关闭QuadSPI
1：使能QuadSPI 相关函数 */
void QSPI_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		QSPI->CR |= (QSPI_CR_EN_Msk);
	}
	else
	{
		QSPI->CR &= ~(QSPI_CR_EN_Msk);
	}
}

FunState QSPI_CR_EN_Getable(void)
{
	if (QSPI->CR & (QSPI_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* nCS最小高电平时间，定义了连续两个帧之间nCS所需保持高电平的最短时间，以QSPI_CLK周期计数
0：至少1 cycle
1：至少2 cycles
……
7：至少8 cycles 相关函数 */
void QSPI_CFG_CSHT_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = QSPI->CFG;
	tmpreg &= ~(QSPI_CFG_CSHT_Msk);
	tmpreg |= (SetValue & QSPI_CFG_CSHT_Msk);
	QSPI->CFG = tmpreg;
}

uint32_t QSPI_CFG_CSHT_Get(void)
{
	return (QSPI->CFG & QSPI_CFG_CSHT_Msk);
}

/* SPI Clock Mode寄存器
0：mode 0
1：mode 3 相关函数 */
void QSPI_CFG_CKMODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = QSPI->CFG;
	tmpreg &= ~(QSPI_CFG_CKMODE_Msk);
	tmpreg |= (SetValue & QSPI_CFG_CKMODE_Msk);
	QSPI->CFG = tmpreg;
}

uint32_t QSPI_CFG_CKMODE_Get(void)
{
	return (QSPI->CFG & QSPI_CFG_CKMODE_Msk);
}

/* FIFO水位标志
此寄存器表示当前FIFO中保存的数据字节数，0表示FIFO空，16表示FIFO满
自动查询模式下此寄存器保持0 相关函数 */
uint32_t QSPI_SR_FIFOLVL_Get(void)
{
	return (QSPI->SR & QSPI_SR_FIFOLVL_Msk) >> QSPI_SR_FIFOLVL_Pos;
}

/* 1表示QuadSPI传输进行中，通信结束后自动清零 相关函数 */
FlagStatus QSPI_SR_BUSY_Chk(void)
{
	if (QSPI->SR & QSPI_SR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 超时标志，硬件置位，软件写1清零 相关函数 */
void QSPI_SR_TOF_Clr(void)
{
	QSPI->SR = QSPI_SR_TOF_Msk;
}

FlagStatus QSPI_SR_TOF_Chk(void)
{
	if (QSPI->SR & QSPI_SR_TOF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 自动查询模式下表征状态寄存器匹配成功，硬件置位，软件写1清零 相关函数 */
void QSPI_SR_SMF_Clr(void)
{
	QSPI->SR = QSPI_SR_SMF_Msk;
}

FlagStatus QSPI_SR_SMF_Chk(void)
{
	if (QSPI->SR & QSPI_SR_SMF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* FIFO threshold标志，FIFO水位高于设定阈值时自动置位，低于阈值时自动清零
自动查询模式下，每次读回一组状态值后都会自动置位FTF，如果软件读取QSPI_DATA寄存器则FTF清零 相关函数 */
FlagStatus QSPI_SR_FTF_Chk(void)
{
	if (QSPI->SR & QSPI_SR_FTF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 传输完成标志，硬件置位，软件写1清零 相关函数 */
void QSPI_SR_TCF_Clr(void)
{
	QSPI->SR = QSPI_SR_TCF_Msk;
}

FlagStatus QSPI_SR_TCF_Chk(void)
{
	if (QSPI->SR & QSPI_SR_TCF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 传输数据长度为DATALEN+1（bytes） 相关函数 */
void QSPI_DATALEN_Write(uint32_t SetValue)
{
	QSPI->DATALEN = (SetValue);
}

uint32_t QSPI_DATALEN_Read(void)
{
	return (QSPI->DATALEN);
}

/* QuadSPI 通信控制寄存器 相关函数 */
void QSPI_CCR_Write(uint32_t SetValue)
{
    QSPI->CCR = (SetValue);
}

uint32_t QSPI_CCR_Read(void)
{
    return (QSPI->CCR);
}

/* 发送给QSPI存储器的地址，在存储器映射模式下无效 相关函数 */
void QSPI_ADDR_Write(uint32_t SetValue)
{
	QSPI->ADDR = (SetValue);
}

uint32_t QSPI_ADDR_Read(void)
{
	return (QSPI->ADDR);
}

/* 发送给QSPI存储器的alternate bytes 相关函数 */
void QSPI_ABR_Write(uint32_t SetValue)
{
	QSPI->ABR = (SetValue);
}

uint32_t QSPI_ABR_Read(void)
{
	return (QSPI->ABR);
}

/* QSPI数据寄存器
外设模式写操作时，对QSPI_DR寄存器写入的数据将被push FIFO，支持字节、半字、字写入，分别对FIFO压入1、2、4字节；如果写入字节数大于FIFO中空字节数，当前写操作被挂起，直到FIFO中有足够空间容纳当前写入数据。
外设模式读操作时，读取QSPI_DR寄存器将pop FIFO，支持字节、半字、字读取，分别从FIFO弹出1、2、4字节；如果读取字节数大于FIFO中有效字节数，当前读操作被挂起，直到FIFO中有足够字节可以被读取，或者传输完成，后一种情况下只弹出最后几个实际有效字节。
对QSPI_DR的访问必须对齐低地址，即字节访问必须对齐QSPI_DR[7:0]，半字访问必须对齐QSPI_DR[15:0] 相关函数 */
void QSPI_DR_WriteByte(uint8_t SetValue)
{
	*((uint8_t *)&(QSPI->DR)) = (SetValue);
}

uint8_t QSPI_DR_ReadByte(void)
{
	return (*((uint8_t *)&(QSPI->DR)));
}

void QSPI_DR_WriteHalfword(uint16_t SetValue)
{
	*((uint16_t *)&(QSPI->DR)) = (SetValue);
}

uint16_t QSPI_DR_ReadHalfword(void)
{
	return (*((uint16_t *)&(QSPI->DR)));
}

void QSPI_DR_WriteWord(uint32_t SetValue)
{
	*((uint32_t *)&(QSPI->DR)) = (SetValue);
}

uint32_t QSPI_DR_ReadWord(void)
{
	return (*((uint32_t *)&(QSPI->DR)));
}

/* 自动状态查询模式下的状态mask寄存器，对应bit写0屏蔽相应状态位 相关函数 */
void QSPI_SMSK_Write(uint32_t SetValue)
{
	QSPI->SMSK = (SetValue);
}

uint32_t QSPI_SMSK_Read(void)
{
	return (QSPI->SMSK);
}

/* 自动状态查询模式下的状态匹配寄存器
比较对象是QSPI_DATA & QSPI_SMSK 相关函数 */
void QSPI_SMAT_Write(uint32_t SetValue)
{
	QSPI->SMAT = (SetValue);
}

uint32_t QSPI_SMAT_Read(void)
{
	return (QSPI->SMAT);
}

/* 自动状态查询模式下的轮询间隔（polling interval），定义为QSPI_CLK周期数 相关函数 */
void QSPI_PITV_Write(uint32_t SetValue)
{
	QSPI->PITV = (SetValue & QSPI_PITV_QSPI_PITV_Msk);
}

uint32_t QSPI_PITV_Read(void)
{
	return (QSPI->PITV & QSPI_PITV_QSPI_PITV_Msk);
}

/* 超时周期设置，定义为QSPI_CLK周期数，仅在存储器映射模式下有效
当FIFO满之后，QSPI总线行为停止，超时计数器开始计数，计数值到达QSPI_TO设定值之后，拉高nCS 相关函数 */
void QSPI_TO_Write(uint32_t SetValue)
{
	QSPI->TO = (SetValue & QSPI_TO_QSPI_TO_Msk);
}

uint32_t QSPI_TO_Read(void)
{
	return (QSPI->TO & QSPI_TO_QSPI_TO_Msk);
}


void QSPI_Deinit(void)
{
    
}

void QSPI_Init(void)
{
    
}

/******END OF FILE****/
