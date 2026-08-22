/**
  ******************************************************************************
  * @file    fm33a0xxev_qspi.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the QSPI firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_QSPI_H
#define __FM33A0XXEV_QSPI_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 

typedef struct
{
    uint32_t Instruction;
    uint32_t Address;
    uint32_t AlternateByte;
    
    uint32_t AddressSize;
    uint32_t AlternateByteSize;
    uint32_t DummyCycles;
    
    uint32_t InstructionMode;
    uint32_t AddressMode;
    uint32_t AlternateByteMode;
    uint32_t DataMode;
}QSPI_CmdTypeDef;
     
#define	QSPI_CR_PRESCALER_Pos	24
#define	QSPI_CR_PRESCALER_Msk	(0xffU << QSPI_CR_PRESCALER_Pos)

#define	QSPI_CR_PMM_Pos	23	/* 轮询匹配模式
0：AND模式，所有bit都匹配才置位SMF
1：OR模式，至少1bit匹配就会置位SMF */
#define	QSPI_CR_PMM_Msk	(0x1U << QSPI_CR_PMM_Pos)
#define	QSPI_CR_PMM_AND	(0x0U << QSPI_CR_PMM_Pos)	/* AND模式，所有bit都匹配才置位SMF */
#define	QSPI_CR_PMM_OR	(0x1U << QSPI_CR_PMM_Pos)	/* OR模式，至少1bit匹配就会置位SMF */

#define	QSPI_CR_TOIE_Pos	20	/* 超时中断使能
0：禁止超时中断
1：允许超时中断 */
#define	QSPI_CR_TOIE_Msk	(0x1U << QSPI_CR_TOIE_Pos)
	/* 禁止超时中断 */
	/* 允许超时中断 */

#define	QSPI_CR_SMIE_Pos	19	/* 状态匹配中断使能
0：禁止状态匹配中断
1：允许状态匹配中断 */
#define	QSPI_CR_SMIE_Msk	(0x1U << QSPI_CR_SMIE_Pos)
	/* 禁止状态匹配中断 */
	/* 允许状态匹配中断 */

#define	QSPI_CR_FTIE_Pos	18	/* FIFO水位中断使能
0：禁止FIFO水位中断
1：允许FIFO水位中断 */
#define	QSPI_CR_FTIE_Msk	(0x1U << QSPI_CR_FTIE_Pos)
	/* 禁止FIFO水位中断 */
	/* 允许FIFO水位中断 */

#define	QSPI_CR_TCIE_Pos	17	/* 传输完成中断使能
0：禁止传输完成中断
1：允许传输完成中断 */
#define	QSPI_CR_TCIE_Msk	(0x1U << QSPI_CR_TCIE_Pos)
	/* 禁止传输完成中断 */
	/* 允许传输完成中断 */

#define	QSPI_CR_TEIE_Pos	16	/* 传输错误中断使能
0：禁止传输错误中断
1：允许传输错误中断 */
#define	QSPI_CR_TEIE_Msk	(0x1U << QSPI_CR_TEIE_Pos)
	/* 禁止传输错误中断 */
	/* 允许传输错误中断 */

#define	QSPI_CR_FIFOTHR_Pos	8	/* FIFO水位寄存器，仅外设模式下起效
外设写模式：
0：FTF在FIFO空字节大于等于1时置位
1：FTF在FIFO空字节大于等于2时置位
……
15：FTF在FIFO空字节等于16时置位
外设读模式：
0：FTF在FIFO中字节数大于等于1时置位
1：FTF在FIFO中字节数大于等于2时置位
……
15：FTF在FIFO中字节数等于16时置位 */
#define	QSPI_CR_FIFOTHR_Msk	(0xfU << QSPI_CR_FIFOTHR_Pos)
#define	QSPI_CR_FIFOTHR_1	(0x0U << QSPI_CR_FIFOTHR_Pos)   /* FTF在FIFO空字节大于等于1时置位 */
#define	QSPI_CR_FIFOTHR_2	(0x1U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于2时置位 */
#define	QSPI_CR_FIFOTHR_3	(0x2U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于3时置位 */
#define	QSPI_CR_FIFOTHR_4	(0x3U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于4时置位 */
#define	QSPI_CR_FIFOTHR_5	(0x4U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于5时置位 */
#define	QSPI_CR_FIFOTHR_6	(0x5U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于6时置位 */
#define	QSPI_CR_FIFOTHR_7	(0x6U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于7时置位 */
#define	QSPI_CR_FIFOTHR_8	(0x7U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于8时置位 */
#define	QSPI_CR_FIFOTHR_9	(0x8U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于9时置位 */
#define	QSPI_CR_FIFOTHR_10	(0x9U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于10时置位 */
#define	QSPI_CR_FIFOTHR_11	(0x10U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于11时置位 */
#define	QSPI_CR_FIFOTHR_12	(0x11U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于12时置位 */
#define	QSPI_CR_FIFOTHR_13	(0x12U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于13时置位 */
#define	QSPI_CR_FIFOTHR_14	(0x13U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于14时置位 */
#define	QSPI_CR_FIFOTHR_15	(0x14U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节大于等于15时置位 */
#define	QSPI_CR_FIFOTHR_16	(0x15U << QSPI_CR_FIFOTHR_Pos)	/* FTF在FIFO空字节等于16时置位 */

#define	QSPI_CR_SSHFT_Pos	4	/* 延迟采样使能
0：关闭延迟采样功能
1：使能延迟采样功能 */
#define	QSPI_CR_SSHFT_Msk	(0x1U << QSPI_CR_SSHFT_Pos)
	/* 关闭延迟采样功能 */
	/* 使能延迟采样功能 */

#define	QSPI_CR_TCEN_Pos	3	/* 总线超时使能，此寄存器仅在存储器映射模式下有效
当BUSY置位后，如果QuadSPI不发起对QSPI存储器的访问，超时寄存器开始计数，计数溢出长度由TIMEOUT寄存器定义。当QSPI总线长时间无动作，计数器溢出，nCS被自动拉高，强制结束当前传输过程。
0：关闭超时功能
1：使能超时功能 */
#define	QSPI_CR_TCEN_Msk	(0x1U << QSPI_CR_TCEN_Pos)
	/* 关闭超时功能 */
	/* 使能超时功能 */

#define	QSPI_CR_DMAEN_Pos	2	/* DMA使能
0：DMA功能关闭，QuadSPI不会发送DMA请求
1：DMA功能开启，QuadSPI在满足条件时发送DMA请求 */
#define	QSPI_CR_DMAEN_Msk	(0x1U << QSPI_CR_DMAEN_Pos)
	/* DMA功能关闭，QuadSPI不会发送DMA请求 */
	/* DMA功能开启，QuadSPI在满足条件时发送DMA请求 */

#define	QSPI_CR_ABORT_Pos	1	/* 当前传输终止寄存器，软件写1终止传输，硬件拉高nCS后自动清零 */
#define	QSPI_CR_ABORT_Msk	(0x1U << QSPI_CR_ABORT_Pos)

#define	QSPI_CR_EN_Pos	0	/* QuadSPI模块使能
0：关闭QuadSPI
1：使能QuadSPI */
#define	QSPI_CR_EN_Msk	(0x1U << QSPI_CR_EN_Pos)
	/* 关闭QuadSPI */
	/* 使能QuadSPI */

#define	QSPI_CFG_CSHT_Pos	8	/* nCS最小高电平时间，定义了连续两个帧之间nCS所需保持高电平的最短时间，以QSPI_CLK周期计数
0：至少1 cycle
1：至少2 cycles
……
7：至少8 cycles */
#define	QSPI_CFG_CSHT_Msk	(0x7U << QSPI_CFG_CSHT_Pos)
#define	QSPI_CFG_CSHT_1CYCLE	(0x0U << QSPI_CFG_CSHT_Pos)	/* 至少1 cycle */
#define	QSPI_CFG_CSHT_2CYCLES	(0x1U << QSPI_CFG_CSHT_Pos)	/* 至少2 cycles */
#define	QSPI_CFG_CSHT_3CYCLES	(0x2U << QSPI_CFG_CSHT_Pos)	/* 至少3 cycles */
#define	QSPI_CFG_CSHT_4CYCLES	(0x3U << QSPI_CFG_CSHT_Pos)	/* 至少4 cycles */
#define	QSPI_CFG_CSHT_5CYCLES	(0x4U << QSPI_CFG_CSHT_Pos)	/* 至少5 cycles */
#define	QSPI_CFG_CSHT_6CYCLES	(0x5U << QSPI_CFG_CSHT_Pos)	/* 至少6 cycles */
#define	QSPI_CFG_CSHT_7CYCLES	(0x6U << QSPI_CFG_CSHT_Pos)	/* 至少7 cycles */
#define	QSPI_CFG_CSHT_8CYCLES	(0x7U << QSPI_CFG_CSHT_Pos)	/* 至少8 cycles */

#define	QSPI_CFG_CKMODE_Pos	0	/* SPI Clock Mode寄存器
0：mode 0
1：mode 3 */
#define	QSPI_CFG_CKMODE_Msk	(0x1U << QSPI_CFG_CKMODE_Pos)
#define	QSPI_CFG_CKMODE_MODE0	(0x0U << QSPI_CFG_CKMODE_Pos)	/* mode 0 */
#define	QSPI_CFG_CKMODE_MODE3	(0x1U << QSPI_CFG_CKMODE_Pos)	/* mode 3 */

#define	QSPI_SR_FIFOLVL_Pos	8	/* FIFO水位标志
此寄存器表示当前FIFO中保存的数据字节数，0表示FIFO空，16表示FIFO满
自动查询模式下此寄存器保持0 */
#define	QSPI_SR_FIFOLVL_Msk	(0x1fU << QSPI_SR_FIFOLVL_Pos)

#define	QSPI_SR_BUSY_Pos	5	/* 1表示QuadSPI传输进行中，通信结束后自动清零 */
#define	QSPI_SR_BUSY_Msk	(0x1U << QSPI_SR_BUSY_Pos)

#define	QSPI_SR_TOF_Pos	4	/* 超时标志，硬件置位，软件写1清零 */
#define	QSPI_SR_TOF_Msk	(0x1U << QSPI_SR_TOF_Pos)

#define	QSPI_SR_SMF_Pos	3	/* 自动查询模式下表征状态寄存器匹配成功，硬件置位，软件写1清零 */
#define	QSPI_SR_SMF_Msk	(0x1U << QSPI_SR_SMF_Pos)

#define	QSPI_SR_FTF_Pos	2	/* FIFO threshold标志，FIFO水位高于设定阈值时自动置位，低于阈值时自动清零
自动查询模式下，每次读回一组状态值后都会自动置位FTF，如果软件读取QSPI_DATA寄存器则FTF清零 */
#define	QSPI_SR_FTF_Msk	(0x1U << QSPI_SR_FTF_Pos)

#define	QSPI_SR_TCF_Pos	1	/* 传输完成标志，硬件置位，软件写1清零 */
#define	QSPI_SR_TCF_Msk	(0x1U << QSPI_SR_TCF_Pos)

#define	QSPI_DATALEN_QSPI_DATALEN_Pos	0	/* 传输数据长度为DATALEN+1（bytes） */
#define	QSPI_DATALEN_QSPI_DATALEN_Msk	(0xffffffffU << QSPI_DATALEN_QSPI_DATALEN_Pos)

#define	QSPI_CCR_CRM_Pos	28	/* Continuous Read Mode
0：每次通信发起时都要发送指令
1：只在第一次通信时发送指令 */
#define	QSPI_CCR_CRM_Msk	(0x1U << QSPI_CCR_CRM_Pos)
#define	QSPI_CCR_CRM_ALWAYS	(0x0U << QSPI_CCR_CRM_Pos)	/* 每次通信发起时都要发送指令 */
#define	QSPI_CCR_CRM_ONLY	(0x1U << QSPI_CCR_CRM_Pos)	/* 只在第一次通信时发送指令 */

#define	QSPI_CCR_OPMODE_Pos	26	/* 操作模式
00：外设写模式
01：外设读模式
10：自动查询模式
11：存储器映射模式 */
#define	QSPI_CCR_OPMODE_Msk	    (0x3U << QSPI_CCR_OPMODE_Pos)
#define	QSPI_CCR_OPMODE_WRITE	(0x0U << QSPI_CCR_OPMODE_Pos)	/* 外设写模式 */
#define	QSPI_CCR_OPMODE_READ	(0x1U << QSPI_CCR_OPMODE_Pos)	/* 外设读模式 */
#define	QSPI_CCR_OPMODE_QUERY	(0x2U << QSPI_CCR_OPMODE_Pos)	/* 自动查询模式 */
#define	QSPI_CCR_OPMODE_MAP	    (0x3U << QSPI_CCR_OPMODE_Pos)	/* 存储器映射模式 */

#define	QSPI_CCR_DMODE_Pos	24	/* 数据通信模式（data phase）
00：无数据
01：单线
10：双线
11：四线 */
#define	QSPI_CCR_DMODE_Msk	    (0x3U << QSPI_CCR_DMODE_Pos)
#define	QSPI_CCR_DMODE_NONE	    (0x0U << QSPI_CCR_DMODE_Pos)	/* 无数据 */
#define	QSPI_CCR_DMODE_SINGLE	(0x1U << QSPI_CCR_DMODE_Pos)	/* 单线 */
#define	QSPI_CCR_DMODE_DOUBLE	(0x2U << QSPI_CCR_DMODE_Pos)	/* 双线 */
#define	QSPI_CCR_DMODE_FOUR	    (0x3U << QSPI_CCR_DMODE_Pos)	/* 四线 */

#define	QSPI_CCR_DUMCYC_Pos	18	/* Dummy cycle个数配置（以QSPI_CLK周期计算），0~31 */
#define	QSPI_CCR_DUMCYC_Msk	(0x1fU << QSPI_CCR_DUMCYC_Pos)

#define	QSPI_CCR_ABSIZE_Pos	16	/* Alternate bytes个数
00：8bits alternate bytes
01：16bits alternate bytes
10：24bits alternate bytes
11：32bits alternate bytes */
#define	QSPI_CCR_ABSIZE_Msk	    (0x3U << QSPI_CCR_ABSIZE_Pos)
#define	QSPI_CCR_ABSIZE_8BITS	(0x0U << QSPI_CCR_ABSIZE_Pos)	/* 8bits alternate bytes */
#define	QSPI_CCR_ABSIZE_16BITS	(0x1U << QSPI_CCR_ABSIZE_Pos)	/* 16bits alternate bytes */
#define	QSPI_CCR_ABSIZE_24BITS	(0x2U << QSPI_CCR_ABSIZE_Pos)	/* 24bits alternate bytes */
#define	QSPI_CCR_ABSIZE_32BITS	(0x3U << QSPI_CCR_ABSIZE_Pos)	/* 32bits alternate bytes */

#define	QSPI_CCR_ABMODE_Pos	14	/* Alternate bytes发送模式
00：无alternate bytes
01：单线
10：双线
11：四线 */
#define	QSPI_CCR_ABMODE_Msk	    (0x3U << QSPI_CCR_ABMODE_Pos)
#define	QSPI_CCR_ABMODE_NONE	(0x0U << QSPI_CCR_ABMODE_Pos)	/* 无alternate bytes */
#define	QSPI_CCR_ABMODE_SINGLE	(0x1U << QSPI_CCR_ABMODE_Pos)	/* 单线 */
#define	QSPI_CCR_ABMODE_DOUBLE	(0x2U << QSPI_CCR_ABMODE_Pos)	/* 双线 */
#define	QSPI_CCR_ABMODE_FOUR	(0x3U << QSPI_CCR_ABMODE_Pos)	/* 四线 */

#define	QSPI_CCR_ADSIZE_Pos	12	/* 地址字节长度
00：8bits地址
01：16bits地址
10：24bits地址
11：32bits地址 */
#define	QSPI_CCR_ADSIZE_Msk	    (0x3U << QSPI_CCR_ADSIZE_Pos)
#define	QSPI_CCR_ADSIZE_8BITS	(0x0U << QSPI_CCR_ADSIZE_Pos)	/* 8bits地址 */
#define	QSPI_CCR_ADSIZE_16BITS	(0x1U << QSPI_CCR_ADSIZE_Pos)	/* 16bits地址 */
#define	QSPI_CCR_ADSIZE_24BITS	(0x2U << QSPI_CCR_ADSIZE_Pos)	/* 24bits地址 */
#define	QSPI_CCR_ADSIZE_32BITS	(0x3U << QSPI_CCR_ADSIZE_Pos)	/* 32bits地址 */

#define	QSPI_CCR_ADMODE_Pos	10	/* 地址字节发送模式
00：无地址字节
01：单线
10：双线
11：四线 */
#define	QSPI_CCR_ADMODE_Msk	    (0x3U << QSPI_CCR_ADMODE_Pos)
#define	QSPI_CCR_ADMODE_NONE	(0x0U << QSPI_CCR_ADMODE_Pos)	/* 无地址字节 */
#define	QSPI_CCR_ADMODE_SINGLE	(0x1U << QSPI_CCR_ADMODE_Pos)	/* 单线 */
#define	QSPI_CCR_ADMODE_DOUBLE	(0x2U << QSPI_CCR_ADMODE_Pos)	/* 双线 */
#define	QSPI_CCR_ADMODE_FOUR	(0x3U << QSPI_CCR_ADMODE_Pos)	/* 四线 */

#define	QSPI_CCR_IMODE_Pos	8	/* 指令发送模式
00：无指令字节
01：单线
10：双线
11：四线 */
#define	QSPI_CCR_IMODE_Msk	    (0x3U << QSPI_CCR_IMODE_Pos)
#define	QSPI_CCR_IMODE_NONE	    (0x0U << QSPI_CCR_IMODE_Pos)	/* 无指令字节 */
#define	QSPI_CCR_IMODE_SINGLE	(0x1U << QSPI_CCR_IMODE_Pos)	/* 单线 */
#define	QSPI_CCR_IMODE_DOUBLE	(0x2U << QSPI_CCR_IMODE_Pos)	/* 双线 */
#define	QSPI_CCR_IMODE_FOUR	    (0x3U << QSPI_CCR_IMODE_Pos)	/* 四线 */

#define	QSPI_CCR_INSTRUCTION_Pos	0	/* QuadSPI发送的指令字节 */
#define	QSPI_CCR_INSTRUCTION_Msk	(0xffU << QSPI_CCR_INSTRUCTION_Pos)

#define	QSPI_ADDR_QSPI_ADDR_Pos	0	/* 发送给QSPI存储器的地址，在存储器映射模式下无效 */
#define	QSPI_ADDR_QSPI_ADDR_Msk	(0xffffffffU << QSPI_ADDR_QSPI_ADDR_Pos)

#define	QSPI_ABR_QSPI_ABR_Pos	0	/* 发送给QSPI存储器的alternate bytes */
#define	QSPI_ABR_QSPI_ABR_Msk	(0xffffffffU << QSPI_ABR_QSPI_ABR_Pos)

#define	QSPI_DR_QSPI_DATA_Pos	0	/* QSPI数据寄存器
外设模式写操作时，对QSPI_DR寄存器写入的数据将被push FIFO，支持字节、半字、字写入，分别对FIFO压入1、2、4字节；如果写入字节数大于FIFO中空字节数，当前写操作被挂起，直到FIFO中有足够空间容纳当前写入数据。
外设模式读操作时，读取QSPI_DR寄存器将pop FIFO，支持字节、半字、字读取，分别从FIFO弹出1、2、4字节；如果读取字节数大于FIFO中有效字节数，当前读操作被挂起，直到FIFO中有足够字节可以被读取，或者传输完成，后一种情况下只弹出最后几个实际有效字节。
对QSPI_DR的访问必须对齐低地址，即字节访问必须对齐QSPI_DR[7:0]，半字访问必须对齐QSPI_DR[15:0] */
#define	QSPI_DR_QSPI_DATA_Msk	(0xffffffffU << QSPI_DR_QSPI_DATA_Pos)

#define	QSPI_SMSK_QSPI_SMSK_Pos	0	/* 自动状态查询模式下的状态mask寄存器，对应bit写0屏蔽相应状态位 */
#define	QSPI_SMSK_QSPI_SMSK_Msk	(0xffffffffU << QSPI_SMSK_QSPI_SMSK_Pos)

#define	QSPI_SMAT_QSPI_SMAT_Pos	0	/* 自动状态查询模式下的状态匹配寄存器
比较对象是QSPI_DATA & QSPI_SMSK */
#define	QSPI_SMAT_QSPI_SMAT_Msk	(0xffffffffU << QSPI_SMAT_QSPI_SMAT_Pos)

#define	QSPI_PITV_QSPI_PITV_Pos	0	/* 自动状态查询模式下的轮询间隔（polling interval），定义为QSPI_CLK周期数 */
#define	QSPI_PITV_QSPI_PITV_Msk	(0xffffU << QSPI_PITV_QSPI_PITV_Pos)

#define	QSPI_TO_QSPI_TO_Pos	0	/* 超时周期设置，定义为QSPI_CLK周期数，仅在存储器映射模式下有效
当FIFO满之后，QSPI总线行为停止，超时计数器开始计数，计数值到达QSPI_TO设定值之后，拉高nCS */
#define	QSPI_TO_QSPI_TO_Msk	(0xffffU << QSPI_TO_QSPI_TO_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void QSPI_Deinit(void);
extern void QSPI_CR_PRESCALER_Set(uint32_t SetValue);
extern uint32_t QSPI_CR_PRESCALER_Get(void);

/* 轮询匹配模式
0：AND模式，所有bit都匹配才置位SMF
1：OR模式，至少1bit匹配就会置位SMF 相关函数 */
extern void QSPI_CR_PMM_Set(uint32_t SetValue);
extern uint32_t QSPI_CR_PMM_Get(void);

/* 超时中断使能
0：禁止超时中断
1：允许超时中断 相关函数 */
extern void QSPI_CR_TOIE_Setable(FunState NewState);
extern FunState QSPI_CR_TOIE_Getable(void);

/* 状态匹配中断使能
0：禁止状态匹配中断
1：允许状态匹配中断 相关函数 */
extern void QSPI_CR_SMIE_Setable(FunState NewState);
extern FunState QSPI_CR_SMIE_Getable(void);

/* FIFO水位中断使能
0：禁止FIFO水位中断
1：允许FIFO水位中断 相关函数 */
extern void QSPI_CR_FTIE_Setable(FunState NewState);
extern FunState QSPI_CR_FTIE_Getable(void);

/* 传输完成中断使能
0：禁止传输完成中断
1：允许传输完成中断 相关函数 */
extern void QSPI_CR_TCIE_Setable(FunState NewState);
extern FunState QSPI_CR_TCIE_Getable(void);

/* 传输错误中断使能
0：禁止传输错误中断
1：允许传输错误中断 相关函数 */
extern void QSPI_CR_TEIE_Setable(FunState NewState);
extern FunState QSPI_CR_TEIE_Getable(void);

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
extern void QSPI_CR_FIFOTHR_Set(uint32_t SetValue);
extern uint32_t QSPI_CR_FIFOTHR_Get(void);

/* 延迟采样使能
0：关闭延迟采样功能
1：使能延迟采样功能 相关函数 */
extern void QSPI_CR_SSHFT_Setable(FunState NewState);
extern FunState QSPI_CR_SSHFT_Getable(void);

/* 总线超时使能，此寄存器仅在存储器映射模式下有效
当BUSY置位后，如果QuadSPI不发起对QSPI存储器的访问，超时寄存器开始计数，计数溢出长度由TIMEOUT寄存器定义。当QSPI总线长时间无动作，计数器溢出，nCS被自动拉高，强制结束当前传输过程。
0：关闭超时功能
1：使能超时功能 相关函数 */
extern void QSPI_CR_TCEN_Setable(FunState NewState);
extern FunState QSPI_CR_TCEN_Getable(void);

/* DMA使能
0：DMA功能关闭，QuadSPI不会发送DMA请求
1：DMA功能开启，QuadSPI在满足条件时发送DMA请求 相关函数 */
extern void QSPI_CR_DMAEN_Setable(FunState NewState);
extern FunState QSPI_CR_DMAEN_Getable(void);

/* 当前传输终止寄存器，软件写1终止传输，硬件拉高nCS后自动清零 相关函数 */
extern void QSPI_CR_ABORT_Setable(FunState NewState);
extern FunState QSPI_CR_ABORT_Getable(void);

/* QuadSPI模块使能
0：关闭QuadSPI
1：使能QuadSPI 相关函数 */
extern void QSPI_CR_EN_Setable(FunState NewState);
extern FunState QSPI_CR_EN_Getable(void);

/* nCS最小高电平时间，定义了连续两个帧之间nCS所需保持高电平的最短时间，以QSPI_CLK周期计数
0：至少1 cycle
1：至少2 cycles
……
7：至少8 cycles 相关函数 */
extern void QSPI_CFG_CSHT_Set(uint32_t SetValue);
extern uint32_t QSPI_CFG_CSHT_Get(void);

/* SPI Clock Mode寄存器
0：mode 0
1：mode 3 相关函数 */
extern void QSPI_CFG_CKMODE_Set(uint32_t SetValue);
extern uint32_t QSPI_CFG_CKMODE_Get(void);

/* FIFO水位标志
此寄存器表示当前FIFO中保存的数据字节数，0表示FIFO空，16表示FIFO满
自动查询模式下此寄存器保持0 相关函数 */
extern uint32_t QSPI_SR_FIFOLVL_Get(void);

/* 1表示QuadSPI传输进行中，通信结束后自动清零 相关函数 */
extern FlagStatus QSPI_SR_BUSY_Chk(void);

/* 超时标志，硬件置位，软件写1清零 相关函数 */
extern void QSPI_SR_TOF_Clr(void);
extern FlagStatus QSPI_SR_TOF_Chk(void);

/* 自动查询模式下表征状态寄存器匹配成功，硬件置位，软件写1清零 相关函数 */
extern void QSPI_SR_SMF_Clr(void);
extern FlagStatus QSPI_SR_SMF_Chk(void);

/* FIFO threshold标志，FIFO水位高于设定阈值时自动置位，低于阈值时自动清零
自动查询模式下，每次读回一组状态值后都会自动置位FTF，如果软件读取QSPI_DATA寄存器则FTF清零 相关函数 */
extern FlagStatus QSPI_SR_FTF_Chk(void);

/* 传输完成标志，硬件置位，软件写1清零 相关函数 */
extern void QSPI_SR_TCF_Clr(void);
extern FlagStatus QSPI_SR_TCF_Chk(void);

/* 传输数据长度为DATALEN+1（bytes） 相关函数 */
extern void QSPI_DATALEN_Write(uint32_t SetValue);
extern uint32_t QSPI_DATALEN_Read(void);

/* QuadSPI 通信控制寄存器 相关函数 */
#define QSPI_CCR_DUMCYC_SET(value)          ((value << QSPI_CCR_DUMCYC_Pos) & QSPI_CCR_DUMCYC_Msk)
#define QSPI_CCR_DUMCYC_GET(value)          ((value >> QSPI_CCR_DUMCYC_Pos) & QSPI_CCR_DUMCYC_Msk)
#define QSPI_CCR_INSTRUCTION_SET(value)     ((value << QSPI_CCR_INSTRUCTION_Pos) & QSPI_CCR_INSTRUCTION_Msk)
#define QSPI_CCR_INSTRUCTION_GET(value)     ((value >> QSPI_CCR_INSTRUCTION_Pos) & QSPI_CCR_INSTRUCTION_Msk)

extern void QSPI_CCR_Write(uint32_t SetValue);
extern uint32_t QSPI_CCR_Read(void);

/* 发送给QSPI存储器的地址，在存储器映射模式下无效 相关函数 */
extern void QSPI_ADDR_Write(uint32_t SetValue);
extern uint32_t QSPI_ADDR_Read(void);

/* 发送给QSPI存储器的alternate bytes 相关函数 */
extern void QSPI_ABR_Write(uint32_t SetValue);
extern uint32_t QSPI_ABR_Read(void);

/* QSPI数据寄存器
外设模式写操作时，对QSPI_DR寄存器写入的数据将被push FIFO，支持字节、半字、字写入，分别对FIFO压入1、2、4字节；如果写入字节数大于FIFO中空字节数，当前写操作被挂起，直到FIFO中有足够空间容纳当前写入数据。
外设模式读操作时，读取QSPI_DR寄存器将pop FIFO，支持字节、半字、字读取，分别从FIFO弹出1、2、4字节；如果读取字节数大于FIFO中有效字节数，当前读操作被挂起，直到FIFO中有足够字节可以被读取，或者传输完成，后一种情况下只弹出最后几个实际有效字节。
对QSPI_DR的访问必须对齐低地址，即字节访问必须对齐QSPI_DR[7:0]，半字访问必须对齐QSPI_DR[15:0] 相关函数 */
extern void QSPI_DR_WriteByte(uint8_t SetValue);
extern uint8_t QSPI_DR_ReadByte(void);
extern void QSPI_DR_WriteHalfword(uint16_t SetValue);
extern uint16_t QSPI_DR_ReadHalfword(void);
extern void QSPI_DR_WriteWord(uint32_t SetValue);
extern uint32_t QSPI_DR_ReadWord(void);

/* 自动状态查询模式下的状态mask寄存器，对应bit写0屏蔽相应状态位 相关函数 */
extern void QSPI_SMSK_Write(uint32_t SetValue);
extern uint32_t QSPI_SMSK_Read(void);

/* 自动状态查询模式下的状态匹配寄存器
比较对象是QSPI_DATA & QSPI_SMSK 相关函数 */
extern void QSPI_SMAT_Write(uint32_t SetValue);
extern uint32_t QSPI_SMAT_Read(void);

/* 自动状态查询模式下的轮询间隔（polling interval），定义为QSPI_CLK周期数 相关函数 */
extern void QSPI_PITV_Write(uint32_t SetValue);
extern uint32_t QSPI_PITV_Read(void);

/* 超时周期设置，定义为QSPI_CLK周期数，仅在存储器映射模式下有效
当FIFO满之后，QSPI总线行为停止，超时计数器开始计数，计数值到达QSPI_TO设定值之后，拉高nCS 相关函数 */
extern void QSPI_TO_Write(uint32_t SetValue);
extern uint32_t QSPI_TO_Read(void);

//Announce_End
void QSPI_Deinit(void);
void QSPI_Init(void);


#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_QSPI_H */
