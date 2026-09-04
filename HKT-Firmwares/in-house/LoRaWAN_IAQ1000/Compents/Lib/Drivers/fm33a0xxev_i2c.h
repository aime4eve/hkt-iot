/**
  ******************************************************************************
  * @file    fm33a0xxev_i2c.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the I2C firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_I2C_H
#define __FM33A0XXEV_I2C_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define I2C_SEND_STARTBIT(I2Cx)			I2Cx_CR_SEN_Setable(I2Cx,ENABLE)
#define I2C_SEND_RESTARTBIT(I2Cx)		I2Cx_CR_RSEN_Setable(I2Cx,ENABLE)
#define I2C_SEND_STOPBIT(I2Cx)			I2Cx_CR_PEN_Setable(I2Cx,ENABLE)
	 
#define I2C_SEND_ACK_0(I2Cx)				I2Cx_SR_ACKMO_Set(I2Cx,I2Cx_SR_ACKMO_RESET)
#define I2C_SEND_ACK_1(I2Cx)				I2Cx_SR_ACKMO_Set(I2Cx,I2Cx_SR_ACKMO_SET)	 
	 
	 
#define	I2Cx_CFGR_AUTOEND_Pos	17	/* 主机DMA自动终止 (Automatic Ending)
1：DMA指定长度传输完成后，自动发送STOP时序
0：DMA指定长度传输完成后，等待软件接管 */
#define	I2Cx_CFGR_AUTOEND_Msk	(0x1U << I2Cx_CFGR_AUTOEND_Pos)
#define	I2Cx_CFGR_AUTOEND_AUTO	(0x1U << I2Cx_CFGR_AUTOEND_Pos)	/* DMA指定长度传输完成后，自动发送STOP时序 */
#define	I2Cx_CFGR_AUTOEND_MANUAL	(0x0U << I2Cx_CFGR_AUTOEND_Pos)	/* DMA指定长度传输完成后，等待软件接管 */

#define	I2Cx_CFGR_MSP_DMAEN_Pos	16	/* 主机DMA使能 (Master DMA Enable)
0：关闭DMA功能
1：使能DMA功能 */
#define	I2Cx_CFGR_MSP_DMAEN_Msk	(0x1U << I2Cx_CFGR_MSP_DMAEN_Pos)
	/* 关闭DMA功能 */
	/* 使能DMA功能 */

#define	I2Cx_CFGR_TOEN_Pos	1	/* SCL拉低超时使能（TimeOut）
1：使能超时功能，超时周期由MSPTO寄存器定义
0：关闭超时功能 */
#define	I2Cx_CFGR_TOEN_Msk	(0x1U << I2Cx_CFGR_TOEN_Pos)
#define	I2Cx_CFGR_TOEN_ENABLE	(0x1U << I2Cx_CFGR_TOEN_Pos)	/* 使能超时功能，超时周期由MSPTO寄存器定义 */
#define	I2Cx_CFGR_TOEN_DISABLE	(0x0U << I2Cx_CFGR_TOEN_Pos)	/* 关闭超时功能 */

#define	I2Cx_CFGR_MSPEN_Pos	0	/* I2C主机模块使能控制位 (Master Enable) 1 = I2C主机使能 0 = I2C主机禁止 */
#define	I2Cx_CFGR_MSPEN_Msk	(0x1U << I2Cx_CFGR_MSPEN_Pos)

#define	I2Cx_CR_RCEN_Pos	3	/* 主控接收模式下，接收使能位 (Receive Enable)
1 = 主机接收使能
0 = 接收禁止 */
#define	I2Cx_CR_RCEN_Msk	(0x1U << I2Cx_CR_RCEN_Pos)

#define	I2Cx_CR_PEN_Pos	2	/* STOP时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Stop Enable) */
#define	I2Cx_CR_PEN_Msk	(0x1U << I2Cx_CR_PEN_Pos)

#define	I2Cx_CR_RSEN_Pos	1	/* Repeated START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (ReStart Enable) */
#define	I2Cx_CR_RSEN_Msk	(0x1U << I2Cx_CR_RSEN_Pos)

#define	I2Cx_CR_SEN_Pos	0	/* START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Start Enable) */
#define	I2Cx_CR_SEN_Msk	(0x1U << I2Cx_CR_SEN_Pos)

#define	I2Cx_IER_WCOLE_Pos	6	/* WCOL中断使能寄存器 (Write Collide Enable)
1：允许写冲突中断
0：禁止写冲突中断 */
#define	I2Cx_IER_WCOLE_Msk	(0x1U << I2Cx_IER_WCOLE_Pos)
	/* 允许写冲突中断 */
	/* 禁止写冲突中断 */

#define	I2Cx_IER_TOE_Pos	5	/* SCL超时中断使能寄存器 (Time-Out Enable)
1：允许超时中断
0：禁止超时中断 */
#define	I2Cx_IER_TOE_Msk	(0x1U << I2Cx_IER_TOE_Pos)

#define	I2Cx_IER_SE_Pos	4	/* START时序中断使能寄存器 (START interrupt Enable)
1：允许START时序中断
0：禁止START时序中断 */
#define	I2Cx_IER_SE_Msk	(0x1U << I2Cx_IER_SE_Pos)
	/* 允许START时序中断 */
	/* 禁止START时序中断 */

#define	I2Cx_IER_PE_Pos	3	/* STOP时序中断使能寄存器 (STOP interrupt Enable)
1：允许STOP时序中断
0：禁止STOP时序中断 */
#define	I2Cx_IER_PE_Msk	(0x1U << I2Cx_IER_PE_Pos)
	/* 允许STOP时序中断 */
	/* 禁止STOP时序中断 */

#define	I2Cx_IER_NACKE_Pos	2	/* 主机发送模式下NACK中断使能寄存器 (Non-ACK interrupt Enable)
1：允许收到NACK产生中断
0：禁止产生NACK中断 */
#define	I2Cx_IER_NACKE_Msk	(0x1U << I2Cx_IER_NACKE_Pos)
	/* 允许收到NACK产生中断 */
	/* 禁止产生NACK中断 */

#define	I2Cx_IER_TXIE_Pos	1	/* I2C主机发送完成中断使能 (Transmit done interrupt enable)
1：允许发送完成中断
0：禁止发送完成中断 */
#define	I2Cx_IER_TXIE_Msk	(0x1U << I2Cx_IER_TXIE_Pos)
	/* 允许发送完成中断 */
	/* 禁止发送完成中断 */

#define	I2Cx_IER_RXIE_Pos	0	/* I2C主机接收完成中断使能 (Receive done interrupt enable)
1：允许接收完成中断
0：禁止接收完成中断 */
#define	I2Cx_IER_RXIE_Msk	(0x1U << I2Cx_IER_RXIE_Pos)
	/* 允许接收完成中断 */
	/* 禁止接收完成中断 */

#define	I2Cx_ISR_WCOL_Pos	6	/* 写冲突检测位，MCU只能在完成START时序或发送完成一帧读写之后才能写SSPBUF，否则发生写冲突；硬件置位，软件写1清零
(Write Collide)
1 = 发送写冲突
0 = 未发生冲突 */
#define	I2Cx_ISR_WCOL_Msk	(0x1U << I2Cx_ISR_WCOL_Pos)

#define	I2Cx_ISR_TO_Pos	5	/* SCL中断标志*/
#define	I2Cx_ISR_TO_Msk	(0x1U << I2Cx_ISR_TO_Pos)


#define	I2Cx_ISR_S_Pos	4	/* START时序发送完成中断标志，硬件置位，软件读取后清零
(START done) */
#define	I2Cx_ISR_S_Msk	(0x1U << I2Cx_ISR_S_Pos)

#define	I2Cx_ISR_P_Pos	3	/* STOP时序发送完成中断标志，硬件置位，软件读取后清零
(STOP done) */
#define	I2Cx_ISR_P_Msk	(0x1U << I2Cx_ISR_P_Pos)

#define	I2Cx_ISR_ACKSTA_Pos	2	/* 主控发送模式下，来自从机的回应信号；当主机发送后收到NACK，此标志可以产生中断；硬件置位，软件写1清零。(Acknowledge Status)
1：从机回应NACK
0：从机回应ACK */
#define	I2Cx_ISR_ACKSTA_Msk	(0x1U << I2Cx_ISR_ACKSTA_Pos)

#define	I2Cx_ISR_TXIF_Pos	1	/* I2C主机发送完成中断标志，硬件置位，软件写1清零 (Transmit done interrupt flag) */
#define	I2Cx_ISR_TXIF_Msk	(0x1U << I2Cx_ISR_TXIF_Pos)

#define	I2Cx_ISR_RXIF_Pos	0	/* I2C主机接收完成中断标志，硬件置位，软件写1清零 (Receive done interrupt flag) */
#define	I2Cx_ISR_RXIF_Msk	(0x1U << I2Cx_ISR_RXIF_Pos)

#define	I2Cx_SR_BUSY_Pos	5	/* I2C通信状态位 (I2C is busy)
1：接口处于读写状态，正在进行数据传输，
0：已完成数据传输 */
#define	I2Cx_SR_BUSY_Msk	(0x1U << I2Cx_SR_BUSY_Pos)

#define	I2Cx_SR_RW_Pos	4	/* I2C传输方向状态位 (Read or Write Bar)
1：主机从从机读取数据
0：主机向从机写入数据 */
#define	I2Cx_SR_RW_Msk	(0x1U << I2Cx_SR_RW_Pos)

#define	I2Cx_SR_BF_Pos	2	/* 缓冲器满状态位 (Buffer full)
接收：
1 = 接收完成，SSPBUF满
0 = 接收未完成，SSPBUF空
发送：
1 = 正在发送，SSPBUF满
0 = 发送完成，SSPBUF空 */
#define	I2Cx_SR_BF_Msk	(0x1U << I2Cx_SR_BF_Pos)

#define	I2Cx_SR_ACKMO_Pos	0	/* 主控接收模式下，主机回应信号的状态 (Acknowledge mode)
1：主机回发NACK
0：主机回发ACK

注意：必须在P标志寄存器被清零的情况下，软件才能置位ACKMO
 */
#define	I2Cx_SR_ACKMO_Msk	(0x1U << I2Cx_SR_ACKMO_Pos)
#define	I2Cx_SR_ACKMO_RESET	(0x0U << I2Cx_SR_ACKMO_Pos)	/* 0：主机回发ACK */
#define	I2Cx_SR_ACKMO_SET	(0x1U << I2Cx_SR_ACKMO_Pos)	/* 1：主机回发NACK */

#define	I2Cx_BRG_MSPBRGH_Pos	16	/* 主机发送的SCL时钟高电平宽度，以I2C工作时钟计数 */
#define	I2Cx_BRG_MSPBRGH_Msk	(0x1ffU << I2Cx_BRG_MSPBRGH_Pos)

#define	I2Cx_BRG_MSPBRGL_Pos	0	/* 主机发送的SCL时钟低电平宽度，以I2C工作时钟计数 */
#define	I2Cx_BRG_MSPBRGL_Msk	(0x1ffU << I2Cx_BRG_MSPBRGL_Pos)

#define	I2Cx_BUF_WR_Pos	0	/* SSPBUF[7:0]：数据的读写通过对SSPBUF的操作完成。发送时，对SSPBUF执行写操作，同时也载入数据收发移位寄存器(SSPSR)；接收时，SSPBUF与SSPSR组成双缓冲结构，读出数据为SSPBUF的数据。接收完一个字节的数据，SSPSR将数据载入SSPBUF，同时置位I2CIF。SSPSR不是直接寄存器，没有物理地址 */
#define	I2Cx_BUF_WR_Msk	(0xffU << I2Cx_BUF_WR_Pos)

#define	I2Cx_TIMING_SDAHD_Pos	0	/* 定义SDA相对于SCL下降沿的保持时间参数，以I2C工作时钟计数
(SDA hold time) */
#define	I2Cx_TIMING_SDAHD_Msk	(0x1ffU << I2Cx_TIMING_SDAHD_Pos)

#define	I2Cx_TO_TIMEOUT_Pos	0	/* 定义从机SCL低电平延展超时周期，软件可以在MSPEN=0的情况下改写
TSCL_STRETCHING_TIMEOUT=TIMEOUT[11:0] * TSCL */
#define	I2Cx_TO_TIMEOUT_Msk	(0xfffU << I2Cx_TO_TIMEOUT_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void I2Cx_Deinit(I2C_Type* I2Cx);

/* 主机DMA自动终止 (Automatic Ending)
1：DMA指定长度传输完成后，自动发送STOP时序
0：DMA指定长度传输完成后，等待软件接管 相关函数 */
extern void I2Cx_CFGR_AUTOEND_Set(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_CFGR_AUTOEND_Get(I2C_Type* I2Cx);

/* 主机DMA使能 (Master DMA Enable)
0：关闭DMA功能
1：使能DMA功能 相关函数 */
extern void I2Cx_CFGR_MSP_DMAEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CFGR_MSP_DMAEN_Getable(I2C_Type* I2Cx);

/* SCL拉低超时使能（TimeOut）
1：使能超时功能，超时周期由MSPTO寄存器定义
0：关闭超时功能 相关函数 */
extern void I2Cx_CFGR_TOEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CFGR_TOEN_Getable(I2C_Type* I2Cx);

/* I2C主机模块使能控制位 (Master Enable)1 = I2C主机使能0 = I2C主机禁止 相关函数 */
extern void I2Cx_CFGR_MSPEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CFGR_MSPEN_Getable(I2C_Type* I2Cx);

/* 主控接收模式下，接收使能位 (Receive Enable)
1 = 主机接收使能
0 = 接收禁止 相关函数 */
extern void I2Cx_CR_RCEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CR_RCEN_Getable(I2C_Type* I2Cx);

/* STOP时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Stop Enable) 相关函数 */
extern void I2Cx_CR_PEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CR_PEN_Getable(I2C_Type* I2Cx);

/* Repeated START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (ReStart Enable) 相关函数 */
extern void I2Cx_CR_RSEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CR_RSEN_Getable(I2C_Type* I2Cx);

/* START时序产生使能控制位，软件写1发送STOP时序，发送完成后硬件自动清零 (Start Enable) 相关函数 */
extern void I2Cx_CR_SEN_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_CR_SEN_Getable(I2C_Type* I2Cx);

/* WCOL中断使能寄存器 (Write Collide Enable)
1：允许写冲突中断
0：禁止写冲突中断 相关函数 */
extern void I2Cx_IER_WCOLE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_WCOLE_Getable(I2C_Type* I2Cx);

/* SCL超时中断使能寄存器 (Time-Out Enable)
1：允许超时中断
0：禁止超时中断 相关函数 */
extern void I2Cx_IER_TOE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_TOE_Getable(I2C_Type* I2Cx);

/* START时序中断使能寄存器 (START interrupt Enable)
1：允许START时序中断
0：禁止START时序中断 相关函数 */
extern void I2Cx_IER_SE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_SE_Getable(I2C_Type* I2Cx);

/* STOP时序中断使能寄存器 (STOP interrupt Enable)
1：允许STOP时序中断
0：禁止STOP时序中断 相关函数 */
extern void I2Cx_IER_PE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_PE_Getable(I2C_Type* I2Cx);

/* 主机发送模式下NACK中断使能寄存器 (Non-ACK interrupt Enable)
1：允许收到NACK产生中断
0：禁止产生NACK中断 相关函数 */
extern void I2Cx_IER_NACKE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_NACKE_Getable(I2C_Type* I2Cx);

/* I2C主机发送完成中断使能 (Transmit done interrupt enable)
1：允许发送完成中断
0：禁止发送完成中断 相关函数 */
extern void I2Cx_IER_TXIE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_TXIE_Getable(I2C_Type* I2Cx);

/* I2C主机接收完成中断使能 (Receive done interrupt enable)
1：允许接收完成中断
0：禁止接收完成中断 相关函数 */
extern void I2Cx_IER_RXIE_Setable(I2C_Type* I2Cx, FunState NewState);
extern FunState I2Cx_IER_RXIE_Getable(I2C_Type* I2Cx);

/* 写冲突检测位，MCU只能在完成START时序或发送完成一帧读写之后才能写SSPBUF，否则发生写冲突；硬件置位，软件写1清零
(Write Collide)
1 = 发送写冲突
0 = 未发生冲突 相关函数 */
extern void I2Cx_ISR_WCOL_Clr(I2C_Type* I2Cx);
extern FlagStatus I2Cx_ISR_WCOL_Chk(I2C_Type* I2Cx);

extern void I2Cx_ISR_OVT_Clr(I2C_Type* I2Cx);
extern FlagStatus I2Cx_ISR_OVT_Chk(I2C_Type* I2Cx);

/* START时序发送完成中断标志，硬件置位，软件读取后清零
(START done) 相关函数 */
extern FlagStatus I2Cx_ISR_S_Chk(I2C_Type* I2Cx);

/* STOP时序发送完成中断标志，硬件置位，软件读取后清零
(STOP done) 相关函数 */
extern FlagStatus I2Cx_ISR_P_Chk(I2C_Type* I2Cx);

/* 主控发送模式下，来自从机的回应信号；当主机发送后收到NACK，此标志可以产生中断；硬件置位，软件写1清零。(Acknowledge Status)
1：从机回应NACK
0：从机回应ACK 相关函数 */
extern void I2Cx_ISR_ACKSTA_Clr(I2C_Type* I2Cx);
extern FlagStatus I2Cx_ISR_ACKSTA_Chk(I2C_Type* I2Cx);

/* I2C主机发送完成中断标志，硬件置位，软件写1清零 (Transmit done interrupt flag) 相关函数 */
extern void I2Cx_ISR_TXIF_Clr(I2C_Type* I2Cx);
extern FlagStatus I2Cx_ISR_TXIF_Chk(I2C_Type* I2Cx);

/* I2C主机接收完成中断标志，硬件置位，软件写1清零 (Receive done interrupt flag) 相关函数 */
extern void I2Cx_ISR_RXIF_Clr(I2C_Type* I2Cx);
extern FlagStatus I2Cx_ISR_RXIF_Chk(I2C_Type* I2Cx);

/* I2C通信状态位 (I2C is busy)
1：接口处于读写状态，正在进行数据传输，
0：已完成数据传输 相关函数 */
extern FlagStatus I2Cx_SR_BUSY_Chk(I2C_Type* I2Cx);

/* I2C传输方向状态位 (Read or Write Bar)
1：主机从从机读取数据
0：主机向从机写入数据 相关函数 */
extern FlagStatus I2Cx_SR_RW_Chk(I2C_Type* I2Cx);

/* 缓冲器满状态位 (Buffer full)
接收：
1 = 接收完成，SSPBUF满
0 = 接收未完成，SSPBUF空
发送：
1 = 正在发送，SSPBUF满
0 = 发送完成，SSPBUF空 相关函数 */
extern FlagStatus I2Cx_SR_BF_Chk(I2C_Type* I2Cx);

/* 主控接收模式下，主机回应信号的状态 (Acknowledge mode)
1：主机回发NACK
0：主机回发ACK

注意：必须在P标志寄存器被清零的情况下，软件才能置位ACKMO
 相关函数 */
extern void I2Cx_SR_ACKMO_Set(I2C_Type* I2Cx,uint32_t SetValue);
extern uint32_t I2Cx_SR_ACKMO_Get(I2C_Type* I2Cx);

/* 主机发送的SCL时钟高电平宽度，以I2C工作时钟计数 相关函数 */
extern void I2Cx_BRG_MSPBRGH_Set(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_BRG_MSPBRGH_Get(I2C_Type* I2Cx);

/* 主机发送的SCL时钟低电平宽度，以I2C工作时钟计数 相关函数 */
extern void I2Cx_BRG_MSPBRGL_Set(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_BRG_MSPBRGL_Get(I2C_Type* I2Cx);

/* SSPBUF[7:0]：数据的读写通过对SSPBUF的操作完成。发送时，对SSPBUF执行写操作，同时也载入数据收发移位寄存器(SSPSR)；接收时，SSPBUF与SSPSR组成双缓冲结构，读出数据为SSPBUF的数据。接收完一个字节的数据，SSPSR将数据载入SSPBUF，同时置位I2CIF。SSPSR不是直接寄存器，没有物理地址 相关函数 */
extern void I2Cx_BUF_Write(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_BUF_Read(I2C_Type* I2Cx);

/* 定义SDA相对于SCL下降沿的保持时间参数，以I2C工作时钟计数
(SDA hold time) 相关函数 */
extern void I2Cx_TIMING_Write(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_TIMING_Read(I2C_Type* I2Cx);

/* 定义从机SCL低电平延展超时周期，软件可以在MSPEN=0的情况下改写
TSCL_STRETCHING_TIMEOUT=TIMEOUT[11:0] * TSCL 相关函数 */
extern void I2Cx_TO_Write(I2C_Type* I2Cx, uint32_t SetValue);
extern uint32_t I2Cx_TO_Read(I2C_Type* I2Cx);

extern void I2Cx_Deinit(I2C_Type* I2Cx);
extern uint32_t I2C_BaudREG_Calc(uint32_t I2CClk, uint32_t APBClk);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_I2C_H */
