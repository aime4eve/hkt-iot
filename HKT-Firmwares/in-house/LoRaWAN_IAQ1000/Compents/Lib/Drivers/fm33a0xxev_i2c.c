/**
  ******************************************************************************
  * @file    fm33a0xxev_i2c.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_i2c.h"
#include "fm33a0xxev_rmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup I2C 
  * @brief I2C driver modules
  * @{
  */

/* 主机DMA自动终止 (Automatic Ending)
1：DMA指定长度传输完成后，自动发送STOP时序
0：DMA指定长度传输完成后，等待软件接管 相关函数 */
void I2Cx_CFGR_AUTOEND_Set(I2C_Type* I2Cx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = I2Cx->CFGR ;
	tmpreg &= ~(I2Cx_CFGR_AUTOEND_Msk);
	tmpreg |= (SetValue & I2Cx_CFGR_AUTOEND_Msk);
	I2Cx->CFGR  = tmpreg;
}

uint32_t I2Cx_CFGR_AUTOEND_Get(I2C_Type* I2Cx)
{
	return (I2Cx->CFGR  & I2Cx_CFGR_AUTOEND_Msk);
}

/* 主机DMA使能 (Master DMA Enable)
0：关闭DMA功能
1：使能DMA功能 相关函数 */
void I2Cx_CFGR_MSP_DMAEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CFGR  |= (I2Cx_CFGR_MSP_DMAEN_Msk);
	}
	else
	{
		I2Cx->CFGR  &= ~(I2Cx_CFGR_MSP_DMAEN_Msk);
	}
}

FunState I2Cx_CFGR_MSP_DMAEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CFGR  & (I2Cx_CFGR_MSP_DMAEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SCL拉低超时使能（TimeOut）
1：使能超时功能，超时周期由MSPTO寄存器定义
0：关闭超时功能 相关函数 */
void I2Cx_CFGR_TOEN_Setable(I2C_Type* I2Cx, FunState NewState)
{	
	if (NewState == ENABLE)
	{
		I2Cx->CFGR  |= (I2Cx_CFGR_TOEN_Msk);
	}
	else
	{
		I2Cx->CFGR  &= ~(I2Cx_CFGR_TOEN_Msk);
	}
}

FunState I2Cx_CFGR_TOEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CFGR  & (I2Cx_CFGR_TOEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C主机模块使能控制位 (Master Enable)1 = I2C主机使能0 = I2C主机禁止 相关函数 */
void I2Cx_CFGR_MSPEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CFGR  |= (I2Cx_CFGR_MSPEN_Msk);
	}
	else
	{
		I2Cx->CFGR  &= ~(I2Cx_CFGR_MSPEN_Msk);
	}
}

FunState I2Cx_CFGR_MSPEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CFGR  & (I2Cx_CFGR_MSPEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 主控接收模式下，接收使能位 (Receive Enable)
1 = 主机接收使能
0 = 接收禁止 相关函数 */
void I2Cx_CR_RCEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CR  |= (I2Cx_CR_RCEN_Msk);
	}
	else
	{
		I2Cx->CR  &= ~(I2Cx_CR_RCEN_Msk);
	}
}

FunState I2Cx_CR_RCEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CR  & (I2Cx_CR_RCEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* STOP时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Stop Enable) 相关函数 */
void I2Cx_CR_PEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CR  |= (I2Cx_CR_PEN_Msk);
	}
	else
	{
		I2Cx->CR  &= ~(I2Cx_CR_PEN_Msk);
	}
}

FunState I2Cx_CR_PEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CR  & (I2Cx_CR_PEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Repeated START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (ReStart Enable) 相关函数 */
void I2Cx_CR_RSEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CR  |= (I2Cx_CR_RSEN_Msk);
	}
	else
	{
		I2Cx->CR  &= ~(I2Cx_CR_RSEN_Msk);
	}
}

FunState I2Cx_CR_RSEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CR  & (I2Cx_CR_RSEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Start Enable) 相关函数 */
void I2Cx_CR_SEN_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->CR  |= (I2Cx_CR_SEN_Msk);
	}
	else
	{
		I2Cx->CR  &= ~(I2Cx_CR_SEN_Msk);
	}
}

FunState I2Cx_CR_SEN_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->CR  & (I2Cx_CR_SEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* WCOL中断使能寄存器 (Write Collide Enable)
1：允许写冲突中断
0：禁止写冲突中断 相关函数 */
void I2Cx_IER_WCOLE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_WCOLE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_WCOLE_Msk);
	}
}

FunState I2Cx_IER_WCOLE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_WCOLE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* SCL超时中断使能寄存器 (Time-Out Enable)
1：允许超时中断
0：禁止超时中断 相关函数 */
void I2Cx_IER_TOE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_TOE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_TOE_Msk);
	}
}

FunState I2Cx_IER_TOE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_TOE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* START时序中断使能寄存器 (START interrupt Enable)
1：允许START时序中断
0：禁止START时序中断 相关函数 */
void I2Cx_IER_SE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_SE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_SE_Msk);
	}
}

FunState I2Cx_IER_SE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_SE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* STOP时序中断使能寄存器 (STOP interrupt Enable)
1：允许STOP时序中断
0：禁止STOP时序中断 相关函数 */
void I2Cx_IER_PE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_PE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_PE_Msk);
	}
}

FunState I2Cx_IER_PE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_PE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 主机发送模式下NACK中断使能寄存器 (Non-ACK interrupt Enable)
1：允许收到NACK产生中断
0：禁止产生NACK中断 相关函数 */
void I2Cx_IER_NACKE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_NACKE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_NACKE_Msk);
	}
}

FunState I2Cx_IER_NACKE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_NACKE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C主机发送完成中断使能 (Transmit done interrupt enable)
1：允许发送完成中断
0：禁止发送完成中断 相关函数 */
void I2Cx_IER_TXIE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_TXIE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_TXIE_Msk);
	}
}

FunState I2Cx_IER_TXIE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_TXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* I2C主机接收完成中断使能 (Receive done interrupt enable)
1：允许接收完成中断
0：禁止接收完成中断 相关函数 */
void I2Cx_IER_RXIE_Setable(I2C_Type* I2Cx, FunState NewState)
{
	if (NewState == ENABLE)
	{
		I2Cx->IER  |= (I2Cx_IER_RXIE_Msk);
	}
	else
	{
		I2Cx->IER  &= ~(I2Cx_IER_RXIE_Msk);
	}
}

FunState I2Cx_IER_RXIE_Getable(I2C_Type* I2Cx)
{
	if (I2Cx->IER  & (I2Cx_IER_RXIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 写冲突检测位，MCU只能在完成START时序或发送完成一帧读写之后才能写SSPBUF，否则发生写冲突；硬件置位，软件写1清零
(Write Collide)
1 = 发送写冲突
0 = 未发生冲突 相关函数 */
void I2Cx_ISR_WCOL_Clr(I2C_Type* I2Cx)
{
	I2Cx->ISR  = I2Cx_ISR_WCOL_Msk;
}

FlagStatus I2Cx_ISR_WCOL_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_WCOL_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SCL超时中断标志 相关函数 */
void I2Cx_ISR_OVT_Clr(I2C_Type* I2Cx)
{
	I2Cx->ISR = I2Cx_ISR_TO_Msk;
}

FlagStatus I2Cx_ISR_OVT_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR & I2Cx_ISR_TO_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* START时序发送完成中断标志，硬件置位，软件读取后清零
(START done) 相关函数 */
FlagStatus I2Cx_ISR_S_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_S_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* STOP时序发送完成中断标志，硬件置位，软件读取后清零
(STOP done) 相关函数 */
FlagStatus I2Cx_ISR_P_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_P_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 主控发送模式下，来自从机的回应信号；当主机发送后收到NACK，此标志可以产生中断；硬件置位，软件写1清零。(Acknowledge Status)
1：从机回应NACK
0：从机回应ACK 相关函数 */
void I2Cx_ISR_ACKSTA_Clr(I2C_Type* I2Cx)
{
	I2Cx->ISR  = I2Cx_ISR_ACKSTA_Msk;
}

FlagStatus I2Cx_ISR_ACKSTA_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_ACKSTA_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* I2C主机发送完成中断标志，硬件置位，软件写1清零 (Transmit done interrupt flag) 相关函数 */
void I2Cx_ISR_TXIF_Clr(I2C_Type* I2Cx)
{
	I2Cx->ISR  = I2Cx_ISR_TXIF_Msk;
}

FlagStatus I2Cx_ISR_TXIF_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_TXIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* I2C主机接收完成中断标志，硬件置位，软件写1清零 (Receive done interrupt flag) 相关函数 */
void I2Cx_ISR_RXIF_Clr(I2C_Type* I2Cx)
{
	I2Cx->ISR  = I2Cx_ISR_RXIF_Msk;
}

FlagStatus I2Cx_ISR_RXIF_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->ISR  & I2Cx_ISR_RXIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* I2C通信状态位 (I2C is busy)
1：接口处于读写状态，正在进行数据传输，
0：已完成数据传输 相关函数 */
FlagStatus I2Cx_SR_BUSY_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->SR  & I2Cx_SR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* I2C传输方向状态位 (Read or Write Bar)
1：主机从从机读取数据
0：主机向从机写入数据 相关函数 */
FlagStatus I2Cx_SR_RW_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->SR  & I2Cx_SR_RW_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 缓冲器满状态位 (Buffer full)
接收：
1 = 接收完成，SSPBUF满
0 = 接收未完成，SSPBUF空
发送：
1 = 正在发送，SSPBUF满
0 = 发送完成，SSPBUF空 相关函数 */
FlagStatus I2Cx_SR_BF_Chk(I2C_Type* I2Cx)
{
	if (I2Cx->SR  & I2Cx_SR_BF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 主控接收模式下，主机回应信号的状态 (Acknowledge mode)
1：主机回发NACK
0：主机回发ACK

注意：必须在P标志寄存器被清零的情况下，软件才能置位ACKMO
 相关函数 */
void I2Cx_SR_ACKMO_Set(I2C_Type* I2Cx,uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = I2Cx->SR;
	tmpreg &= ~(I2Cx_SR_ACKMO_Msk);
	tmpreg |= (SetValue & I2Cx_SR_ACKMO_Msk);
	I2Cx->SR = tmpreg;
}

uint32_t I2Cx_SR_ACKMO_Get(I2C_Type* I2Cx)
{
	return (I2Cx->SR & I2Cx_SR_ACKMO_Msk);
}

/* 主机发送的SCL时钟高电平宽度，以I2C工作时钟计数 相关函数 */
void I2Cx_BRG_MSPBRGH_Set(I2C_Type* I2Cx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = I2Cx->BRG;
	tmpreg &= ~(I2Cx_BRG_MSPBRGH_Msk);
	tmpreg |= (SetValue & I2Cx_BRG_MSPBRGH_Msk);
	I2Cx->BRG = tmpreg;
}

uint32_t I2Cx_BRG_MSPBRGH_Get(I2C_Type* I2Cx)
{
	return (I2Cx->BRG & I2Cx_BRG_MSPBRGH_Msk);
}

/* 主机发送的SCL时钟低电平宽度，以I2C工作时钟计数 相关函数 */
void I2Cx_BRG_MSPBRGL_Set(I2C_Type* I2Cx, uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = I2Cx->BRG;
	tmpreg &= ~(I2Cx_BRG_MSPBRGL_Msk);
	tmpreg |= (SetValue & I2Cx_BRG_MSPBRGL_Msk);
	I2Cx->BRG = tmpreg;
}

uint32_t I2Cx_BRG_MSPBRGL_Get(I2C_Type* I2Cx)
{
	return (I2Cx->BRG & I2Cx_BRG_MSPBRGL_Msk);
}

/* SSPBUF[7:0]：数据的读写通过对SSPBUF的操作完成。发送时，对SSPBUF执行写操作，同时也载入数据收发移位寄存器(SSPSR)；接收时，SSPBUF与SSPSR组成双缓冲结构，读出数据为SSPBUF的数据。接收完一个字节的数据，SSPSR将数据载入SSPBUF，同时置位I2CIF。SSPSR不是直接寄存器，没有物理地址 相关函数 */
void I2Cx_BUF_Write(I2C_Type* I2Cx, uint32_t SetValue)
{
	I2Cx->BUF = (SetValue & I2Cx_BUF_WR_Msk);
}

uint32_t I2Cx_BUF_Read(I2C_Type* I2Cx)
{
	return (I2Cx->BUF & I2Cx_BUF_WR_Msk);
}

/* 定义SDA相对于SCL下降沿的保持时间参数，以I2C工作时钟计数
(SDA hold time) 相关函数 */
void I2Cx_TIMING_Write(I2C_Type* I2Cx, uint32_t SetValue)
{
	I2Cx->TIMING  = (SetValue & I2Cx_TIMING_SDAHD_Msk);
}

uint32_t I2Cx_TIMING_Read(I2C_Type* I2Cx)
{
	return (I2Cx->TIMING  & I2Cx_TIMING_SDAHD_Msk);
}

/* 定义从机SCL低电平延展超时周期，软件可以在MSPEN=0的情况下改写
TSCL_STRETCHING_TIMEOUT=TIMEOUT[11:0] * TSCL 相关函数 */
void I2Cx_TO_Write(I2C_Type* I2Cx, uint32_t SetValue)
{
	I2Cx->TO = (SetValue & I2Cx_TO_TIMEOUT_Msk);
}

uint32_t I2Cx_TO_Read(I2C_Type* I2Cx)
{
	return (I2Cx->TO & I2Cx_TO_TIMEOUT_Msk);
}


void I2Cx_Deinit(I2C_Type* I2Cx)
{
	RMU_PRSTEN_Write(0x13579BDF);
	if(I2Cx == I2C0)
	{
		RMU_APBRST2_I2C0RST_Setable(ENABLE);  
		RMU_APBRST2_I2C0RST_Setable(DISABLE);
	}
	if(I2Cx == I2C1)
	{
		RMU_APBRST1_I2C1RST_Setable(ENABLE);  
		RMU_APBRST1_I2C1RST_Setable(DISABLE);
	}
  RMU_PRSTEN_Write(0x00000000);
}

 /********************************
 I2C波特率寄存器值计算函数
功能：I2C波特率寄存器值计算
输入：期望波特率@Hz， APBClk @Hz
输出：I2CBRG对应期望波特率的配置值
********************************/
uint32_t I2C_BaudREG_Calc(uint32_t I2CClk, uint32_t APBClk)
{
	uint32_t TempREG;
	
	TempREG = APBClk/(2*I2CClk);
	if( TempREG >= 1 ) TempREG = TempREG - 1;
	
	return TempREG;
}

/******END OF FILE****/
