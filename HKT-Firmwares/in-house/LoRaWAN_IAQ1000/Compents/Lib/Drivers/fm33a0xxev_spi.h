/**
  ******************************************************************************
  * @file    fm33a0xxev_spi.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the SPI firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_SPI_H
#define __FM33A0XXEV_SPI_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
//------------------------------------------------------------------------------
     
#define	SPIx_CR1_IOSWAP_Pos	11	/* MOSI和MISO引脚交换 (IO swapping)
0：默认引脚顺序
1：交换引脚顺序 */
#define	SPIx_CR1_IOSWAP_Msk	(0x1U << SPIx_CR1_IOSWAP_Pos)
#define	SPIx_CR1_IOSWAP_DEFAULT	(0x0U << SPIx_CR1_IOSWAP_Pos)	/* 默认引脚顺序 */
#define	SPIx_CR1_IOSWAP_EXCHANGE	(0x1U << SPIx_CR1_IOSWAP_Pos)	/* 交换引脚顺序 */

#define	SPIx_CR1_MSPA_Pos	10	/* Master Sampling Position Adjustment，Master对MISO信号的采样位置调整，用于高速通信时补偿PCB走线延迟
1：采样点延迟半个SCK周期
0：不调整 */
#define	SPIx_CR1_MSPA_Msk	(0x1U << SPIx_CR1_MSPA_Pos)
#define	SPIx_CR1_MSPA_DELAY	(0x1U << SPIx_CR1_MSPA_Pos)	/* 采样点延迟半个SCK周期 */
#define	SPIx_CR1_MSPA_NORMAL	(0x0U << SPIx_CR1_MSPA_Pos)	/* 不调整 */

#define	SPIx_CR1_SSPA_Pos	9	/* Slave Sending Position Adjustment，Slave MISO发送位置调整
1：提前半个SCK周期发送
0：不调整 */
#define	SPIx_CR1_SSPA_Msk	(0x1U << SPIx_CR1_SSPA_Pos)
#define	SPIx_CR1_SSPA_EARLY	(0x1U << SPIx_CR1_SSPA_Pos)	/* 提前半个SCK周期发送 */
#define	SPIx_CR1_SSPA_NORMAL	(0x0U << SPIx_CR1_SSPA_Pos)	/* 不调整 */

#define	SPIx_CR1_MM_Pos	8	/* Master/Slave模式选择。(Master Mode)1：Master模式0：Slave模式 */
#define	SPIx_CR1_MM_Msk	(0x1U << SPIx_CR1_MM_Pos)
#define	SPIx_CR1_MM_MASTER	(0x1U << SPIx_CR1_MM_Pos)
#define	SPIx_CR1_MM_SLAVE	(0x0U << SPIx_CR1_MM_Pos)

#define	SPIx_CR1_WAIT_Pos	6	/* Master模式下，每发送完一帧后加入至少(1+WAIT)个SCK cycle等待时间，再传输下一帧的数据。如果SSN由硬件控制，并且SSNM=1，则硬件会自动拉高SSN。 */
#define	SPIx_CR1_WAIT_Msk	(0x3U << SPIx_CR1_WAIT_Pos)
#define	SPIx_CR1_WAIT_1WAIT	(0x0U << SPIx_CR1_WAIT_Pos)
#define	SPIx_CR1_WAIT_2WAIT	(0x1U << SPIx_CR1_WAIT_Pos)
#define	SPIx_CR1_WAIT_3WAIT	(0x2U << SPIx_CR1_WAIT_Pos)
#define	SPIx_CR1_WAIT_4WAIT	(0x3U << SPIx_CR1_WAIT_Pos)

#define	SPIx_CR1_BAUD_Pos	3	/* Master模式波特率配置位: (Baud rate)000： fAPBCLK/2001： fAPBCLK/4010： fAPBCLK/8011： fAPBCLK/16100： fAPBCLK/32101： fAPBCLK/64110： fAPBCLK/128111： fAPBCLK/256当通信正在进行的时候，不能修改这些位。 */
#define	SPIx_CR1_BAUD_Msk	(0x7U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV2	(0x0U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV4	(0x1U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV8	(0x2U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV16	(0x3U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV32	(0x4U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV64	(0x5U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV128	(0x6U << SPIx_CR1_BAUD_Pos)
#define	SPIx_CR1_BAUD_DIV256	(0x7U << SPIx_CR1_BAUD_Pos)

#define	SPIx_CR1_LSBF_Pos	2	/* 帧格式 (LSB First) 0：先发送MSB1：先发送LSB注：当通信在进行时不能改变该位的值。 */
#define	SPIx_CR1_LSBF_Msk	(0x1U << SPIx_CR1_LSBF_Pos)
#define	SPIx_CR1_LSBF_MSB	(0x0U << SPIx_CR1_LSBF_Pos)
#define	SPIx_CR1_LSBF_LSB	(0x1U << SPIx_CR1_LSBF_Pos)

#define	SPIx_CR1_CPHOL_Pos	1
#define	SPIx_CR1_CPHOL_Msk	(0x1U << SPIx_CR1_CPHOL_Pos)
#define	SPIx_CR1_CPHOL_LOW	(0x0U << SPIx_CR1_CPHOL_Pos)
#define	SPIx_CR1_CPHOL_HIGH	(0x1U << SPIx_CR1_CPHOL_Pos)

#define	SPIx_CR1_CPHA_Pos	0	/* 时钟相位选择 (Clock Phase)1：第二个时钟边沿是第一个捕捉边沿0：第一个时钟边沿是第一个捕捉边沿注：当通信在进行时不能改变该位的值。 */
#define	SPIx_CR1_CPHA_Msk	(0x1U << SPIx_CR1_CPHA_Pos)
#define	SPIx_CR1_CPHA_1CLOCK	(0x0U << SPIx_CR1_CPHA_Pos)
#define	SPIx_CR1_CPHA_2CLOCK	(0x1U << SPIx_CR1_CPHA_Pos)

#define	SPIx_CR2_DUMMY_EN_Pos	15	/* 4线半双工协议下是否在读操作中插入dummy cycle
(Dummy cycle Enable)
0：不插入dummy cycle
1：在读命令之后插入一个dummy cycle */
#define	SPIx_CR2_DUMMY_EN_Msk	(0x1U << SPIx_CR2_DUMMY_EN_Pos)
	/* 不插入dummy cycle */
	/* 在读命令之后插入一个dummy cycle */

#define	SPIx_CR2_RXO_Pos	11	/* RXONLY控制位，此寄存器置位时，SPI可以连续接收，无需软件写TXBUF (Receive Only mode)
1：启动Master的单接收模式
0：关闭单接收模式（收发全双工） */
#define	SPIx_CR2_RXO_Msk	(0x1U << SPIx_CR2_RXO_Pos)
	/* 启动Master的单接收模式 */
	/* 关闭单接收模式（收发全双工） */

#define	SPIx_CR2_DLEN_Pos	9	/* 通信数据字长配置 (Data Length)
00：8bit
01：16bit
10：24bit
11：32bit */
#define	SPIx_CR2_DLEN_Msk	(0x3U << SPIx_CR2_DLEN_Pos)
#define	SPIx_CR2_DLEN_8BIT	(0x0U << SPIx_CR2_DLEN_Pos)	/* 8bit */
#define	SPIx_CR2_DLEN_16BIT	(0x1U << SPIx_CR2_DLEN_Pos)	/* 16bit */
#define	SPIx_CR2_DLEN_24BIT	(0x2U << SPIx_CR2_DLEN_Pos)	/* 24bit */
#define	SPIx_CR2_DLEN_32BIT	(0x3U << SPIx_CR2_DLEN_Pos)	/* 32bit */

#define	SPIx_CR2_HALFDUPLEX_Pos	8	/* 通信模式选择 (Half-Duplex mode)
0：标准SPI模式，4线全双工
1：DCN模式，4线半双工 */
#define	SPIx_CR2_HALFDUPLEX_Msk	(0x1U << SPIx_CR2_HALFDUPLEX_Pos)
#define	SPIx_CR2_HALFDUPLEX_SPI	(0x0U << SPIx_CR2_HALFDUPLEX_Pos)	/* 标准SPI模式，4线全双工 */
#define	SPIx_CR2_HALFDUPLEX_DCN	(0x1U << SPIx_CR2_HALFDUPLEX_Pos)	/* DCN模式，4线半双工 */

#define	SPIx_CR2_HD_RW_Pos	7	/* 半双工模式下主机读写操作配置 (Read/Write config for Half-Duplex mode)
0：4线半双工协议下主机写入从机
1：4线半双工协议下主机读取从机 */
#define	SPIx_CR2_HD_RW_Msk	(0x1U << SPIx_CR2_HD_RW_Pos)
#define	SPIx_CR2_HD_RW_WRITE	(0x0U << SPIx_CR2_HD_RW_Pos)	/* 4线半双工协议下主机写入从机 */
#define	SPIx_CR2_HD_RW_READ	(0x1U << SPIx_CR2_HD_RW_Pos)	/* 4线半双工协议下主机读取从机 */

#define	SPIx_CR2_CMD8b_Pos	6	/* 半双工模式下定义command帧长度 (Command 8 bits)
1：command帧固定为8bit
0：command帧长度由DLEN定义 */
#define	SPIx_CR2_CMD8b_Msk	(0x1U << SPIx_CR2_CMD8b_Pos)
#define	SPIx_CR2_CMD8b_8BIT	(0x1U << SPIx_CR2_CMD8b_Pos)	/* command帧固定为8bit */
#define	SPIx_CR2_CMD8b_DLEN	(0x0U << SPIx_CR2_CMD8b_Pos)	/* command帧长度由DLEN定义 */

#define	SPIx_CR2_SSNM_Pos	5	/* Master模式下SSN控制模式选择 (SSN mode)
1：每发送完一帧后Master拉高SSN，维持高电平时间由WAIT寄存器控制
0：每发送完一帧后Master保持SSN为低 */
#define	SPIx_CR2_SSNM_Msk	(0x1U << SPIx_CR2_SSNM_Pos)
#define	SPIx_CR2_SSNM_HIGH	(0x1U << SPIx_CR2_SSNM_Pos)	/* 每发送完一帧后Master拉高SSN，维持高电平时间由WAIT寄存器控制 */
#define	SPIx_CR2_SSNM_LOW	(0x0U << SPIx_CR2_SSNM_Pos)	/* 每发送完一帧后Master保持SSN为低 */

#define	SPIx_CR2_TXO_AC_Pos	4	/* TXONLY硬件自动清空的使能 (TXONLY auto-clear enable)1：TXONLY硬件自动清零有效，软件使能TXO后，等待发送完毕后，硬件清零0：关闭TXONLY硬件自动清零 */
#define	SPIx_CR2_TXO_AC_Msk	(0x1U << SPIx_CR2_TXO_AC_Pos)

#define	SPIx_CR2_TXO_Pos	3	/* TXONLY控制位 (Transmit Only mode enable)
1：启动Master的单发送模式
0：关闭单发送模式（收发全双工） */
#define	SPIx_CR2_TXO_Msk	(0x1U << SPIx_CR2_TXO_Pos)
	/* 启动Master的单发送模式 */
	/* 关闭单发送模式（收发全双工） */

#define	SPIx_CR2_SSN_Pos	2	/* Master模式下，如果SSNSEN为1，软件可以通过此位控制SSN输出电平 1：SSN输出高电平
0：SSN输出低电平 */
#define	SPIx_CR2_SSN_Msk	(0x1U << SPIx_CR2_SSN_Pos)
#define	SPIx_CR2_SSN_LOW	(0x0U << SPIx_CR2_SSN_Pos)	/* SSN输出高电平 */
#define	SPIx_CR2_SSN_HIGH	(0x1U << SPIx_CR2_SSN_Pos)	/* SSN输出低电平 */

#define	SPIx_CR2_SSNSEN_Pos	1	/* Master模式下，软件控制SSN使能 (SSN Software Enable)
1：Master模式下SSN输出由软件控制
0：Master模式下SSN输出由硬件自动控制 */
#define	SPIx_CR2_SSNSEN_Msk	(0x1U << SPIx_CR2_SSNSEN_Pos)
	/* Master模式下SSN输出由软件控制 */
	/* Master模式下SSN输出由硬件自动控制 */

#define	SPIx_CR2_SPIEN_Pos	0	/* SPI使能 (SPI enable)1：使能SPI0：关闭SPI，清空发送接收缓存 */
#define	SPIx_CR2_SPIEN_Msk	(0x1U << SPIx_CR2_SPIEN_Pos)

#define	SPIx_CR3_TXBFC_Pos	3	/* Transmit Buffer Clear，软件写1清除发送缓存，写0无效 */
#define	SPIx_CR3_TXBFC_Msk	(0x1U << SPIx_CR3_TXBFC_Pos)

#define	SPIx_CR3_RXBFC_Pos	2	/* Receive Buffer Clear，软件写1清除发送缓存，写0无效 */
#define	SPIx_CR3_RXBFC_Msk	(0x1U << SPIx_CR3_RXBFC_Pos)

#define	SPIx_CR3_MERRC_Pos	1	/* Master Error Clear，软件写1清除HSPISTA.MERR寄存器 */
#define	SPIx_CR3_MERRC_Msk	(0x1U << SPIx_CR3_MERRC_Pos)

#define	SPIx_CR3_SERRC_Pos	0	/* Slave Error Clear，软件写1清除HSPISTA.SERR寄存器 */
#define	SPIx_CR3_SERRC_Msk	(0x1U << SPIx_CR3_SERRC_Pos)

#define	SPIx_IER_ERRIE_Pos	2	/* SPI错误中断使能 (Error Interrupt Enable) */
#define	SPIx_IER_ERRIE_Msk	(0x1U << SPIx_IER_ERRIE_Pos)

#define	SPIx_IER_TXIE_Pos	1	/* 发送完成中断使能 (Transmit Interrupt Enable) */
#define	SPIx_IER_TXIE_Msk	(0x1U << SPIx_IER_TXIE_Pos)

#define	SPIx_IER_RXIE_Pos	0	/* 接收完成中断使能 (Receive Interrupt Enable) */
#define	SPIx_IER_RXIE_Msk	(0x1U << SPIx_IER_RXIE_Pos)

#define	SPIx_ISR_DCN_TX_Pos	12	/* 半双工模式下（HALFDUPLEX=1），配置在每个数据帧的最后bit发送的DCN信号电平 (Data/Command transmit config)
0：DCN=0，表示命令帧
1：DCN=1，表示数据帧
软件应在发送前设置DCN_TX寄存器，如果DCN_TX=0，硬件在完成一帧发送后，自动将DCN_TX置1，即默认只会发送一个命令帧，后续都是数据帧。 */
#define	SPIx_ISR_DCN_TX_Msk	(0x1U << SPIx_ISR_DCN_TX_Pos)
#define	SPIx_ISR_DCN_TX_COMMAND	(0x0U << SPIx_ISR_DCN_TX_Pos)	/* DCN=0，表示命令帧 */
#define	SPIx_ISR_DCN_TX_DATA	(0x1U << SPIx_ISR_DCN_TX_Pos)	/* DCN=1，表示数据帧 */

#define	SPIx_ISR_RXCOL_Pos	10	/* 接收缓存溢出，软件写1清零 (Receive Collision flag,write 1 to flag) */
#define	SPIx_ISR_RXCOL_Msk	(0x1U << SPIx_ISR_RXCOL_Pos)

#define	SPIx_ISR_TXCOL_Pos	9	/* 发送缓存溢出，软件写1清零 (Transmit Collision flag,write 1 to clear) */
#define	SPIx_ISR_TXCOL_Msk	(0x1U << SPIx_ISR_TXCOL_Pos)

#define	SPIx_ISR_BUSY_Pos	8	/* SPI空闲标志，只读 (busy flag)
1：SPI传输进行中
0：SPI传输空闲 */
#define	SPIx_ISR_BUSY_Msk	(0x1U << SPIx_ISR_BUSY_Pos)

#define	SPIx_ISR_MERR_Pos	6	/* Master Error标志(Master Error flag)
当Master下传输未满8位SSN就被拉高时，MERR置位 */
#define	SPIx_ISR_MERR_Msk	(0x1U << SPIx_ISR_MERR_Pos)

#define	SPIx_ISR_SERR_Pos	5	/* Slave Error标志(Slave Error flag)
当Slave下传输未满8位SSN就被拉高时，SERR置位 */
#define	SPIx_ISR_SERR_Msk	(0x1U << SPIx_ISR_SERR_Pos)

#define	SPIx_ISR_TXBE_Pos	1	/* TX Buffer Empty标志位(TX Buffer Empty flag)
1：发送缓存空，软件写TXBUF清零
0：发送缓存满 */
#define	SPIx_ISR_TXBE_Msk	(0x1U << SPIx_ISR_TXBE_Pos)

#define	SPIx_ISR_RXBF_Pos	0	/* RX Buffer Full标志位(RX Buffer Full flag)
1：接收缓存满，软件读RXBUF清零
0：接收缓存空 */
#define	SPIx_ISR_RXBF_Msk	(0x1U << SPIx_ISR_RXBF_Pos)

#define	SPIx_TXBUF_TXBUF_Pos	0	/* SPI发送缓存 (Transmit Buffer) */
#define	SPIx_TXBUF_TXBUF_Msk	(0xffffffffU << SPIx_TXBUF_TXBUF_Pos)

#define	SPIx_RXBUF_RXBUF_Pos	0	/* SPI接收缓存 (Receive Buffer) */
#define	SPIx_RXBUF_RXBUF_Msk	(0xffffffffU << SPIx_RXBUF_RXBUF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void SPIx_Deinit(SPI_Type* SPIx);

/* MOSI和MISO引脚交换 (IO swapping)
0：默认引脚顺序
1：交换引脚顺序 相关函数 */
extern void SPIx_CR1_IOSWAP_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_IOSWAP_Get(SPI_Type* SPIx);

/* Master Sampling Position Adjustment，Master对MISO信号的采样位置调整，用于高速通信时补偿PCB走线延迟
1：采样点延迟半个SCK周期
0：不调整 相关函数 */
extern void SPIx_CR1_MSPA_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_MSPA_Get(SPI_Type* SPIx);

/* Slave Sending Position Adjustment，Slave MISO发送位置调整
1：提前半个SCK周期发送
0：不调整 相关函数 */
extern void SPIx_CR1_SSPA_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_SSPA_Get(SPI_Type* SPIx);

/* Master/Slave模式选择。(Master Mode)1：Master模式0：Slave模式 相关函数 */
extern void SPIx_CR1_MM_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_MM_Get(SPI_Type* SPIx);

/* Master模式下，每发送完一帧后加入至少(1+WAIT)个SCK cycle等待时间，再传输下一帧的数据。如果SSN由硬件控制，并且SSNM=1，则硬件会自动拉高SSN。 相关函数 */
extern void SPIx_CR1_WAIT_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_WAIT_Get(SPI_Type* SPIx);

/* Master模式波特率配置位: (Baud rate)000： fAPBCLK/2001： fAPBCLK/4010： fAPBCLK/8011： fAPBCLK/16100： fAPBCLK/32101： fAPBCLK/64110： fAPBCLK/128111： fAPBCLK/256当通信正在进行的时候，不能修改这些位。 相关函数 */
extern void SPIx_CR1_BAUD_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_BAUD_Get(SPI_Type* SPIx);

/* 帧格式 (LSB First) 0：先发送MSB1：先发送LSB注：当通信在进行时不能改变该位的值。 相关函数 */
extern void SPIx_CR1_LSBF_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_LSBF_Get(SPI_Type* SPIx);
extern void SPIx_CR1_CPHOL_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_CPHOL_Get(SPI_Type* SPIx);

/* 时钟相位选择 (Clock Phase)1：第二个时钟边沿是第一个捕捉边沿0：第一个时钟边沿是第一个捕捉边沿注：当通信在进行时不能改变该位的值。 相关函数 */
extern void SPIx_CR1_CPHA_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR1_CPHA_Get(SPI_Type* SPIx);

/* 4线半双工协议下是否在读操作中插入dummy cycle
(Dummy cycle Enable)
0：不插入dummy cycle
1：在读命令之后插入一个dummy cycle 相关函数 */
extern void SPIx_CR2_DUMMY_EN_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_DUMMY_EN_Getable(SPI_Type* SPIx);

/* RXONLY控制位，此寄存器置位时，SPI可以连续接收，无需软件写TXBUF (Receive Only mode)
1：启动Master的单接收模式
0：关闭单接收模式（收发全双工） 相关函数 */
extern void SPIx_CR2_RXO_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_RXO_Getable(SPI_Type* SPIx);

/* 通信数据字长配置 (Data Length)
00：8bit
01：16bit
10：24bit
11：32bit 相关函数 */
extern void SPIx_CR2_DLEN_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_DLEN_Get(SPI_Type* SPIx);

/* 通信模式选择 (Half-Duplex mode)
0：标准SPI模式，4线全双工
1：DCN模式，4线半双工 相关函数 */
extern void SPIx_CR2_HALFDUPLEX_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_HALFDUPLEX_Get(SPI_Type* SPIx);

/* 半双工模式下主机读写操作配置 (Read/Write config for Half-Duplex mode)
0：4线半双工协议下主机写入从机
1：4线半双工协议下主机读取从机 相关函数 */
extern void SPIx_CR2_HD_RW_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_HD_RW_Get(SPI_Type* SPIx);

/* 半双工模式下定义command帧长度 (Command 8 bits)
1：command帧固定为8bit
0：command帧长度由DLEN定义 相关函数 */
extern void SPIx_CR2_CMD8b_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_CMD8b_Get(SPI_Type* SPIx);

/* Master模式下SSN控制模式选择 (SSN mode)
1：每发送完一帧后Master拉高SSN，维持高电平时间由WAIT寄存器控制
0：每发送完一帧后Master保持SSN为低 相关函数 */
extern void SPIx_CR2_SSNM_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_SSNM_Get(SPI_Type* SPIx);

/* TXONLY硬件自动清空的使能 (TXONLY auto-clear enable)1：TXONLY硬件自动清零有效，软件使能TXO后，等待发送完毕后，硬件清零0：关闭TXONLY硬件自动清零 相关函数 */
extern void SPIx_CR2_TXO_AC_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_TXO_AC_Getable(SPI_Type* SPIx);

/* TXONLY控制位 (Transmit Only mode enable)
1：启动Master的单发送模式
0：关闭单发送模式（收发全双工） 相关函数 */
extern void SPIx_CR2_TXO_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_TXO_Getable(SPI_Type* SPIx);

/* Master模式下，如果SSNSEN为1，软件可以通过此位控制SSN输出电平 1：SSN输出高电平
0：SSN输出低电平 相关函数 */
extern void SPIx_CR2_SSN_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_CR2_SSN_Get(SPI_Type* SPIx);

/* Master模式下，软件控制SSN使能 (SSN Software Enable)
1：Master模式下SSN输出由软件控制
0：Master模式下SSN输出由硬件自动控制 相关函数 */
extern void SPIx_CR2_SSNSEN_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_SSNSEN_Getable(SPI_Type* SPIx);

/* SPI使能 (SPI enable)1：使能SPI0：关闭SPI，清空发送接收缓存 相关函数 */
extern void SPIx_CR2_SPIEN_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_CR2_SPIEN_Getable(SPI_Type* SPIx);

/* Transmit Buffer Clear，软件写1清除发送缓存，写0无效 相关函数 */
extern void SPIx_CR3_TXBFC_Clr(SPI_Type* SPIx);

/* Receive Buffer Clear，软件写1清除发送缓存，写0无效 相关函数 */
extern void SPIx_CR3_RXBFC_Clr(SPI_Type* SPIx);

/* Master Error Clear，软件写1清除HSPISTA.MERR寄存器 相关函数 */
extern void SPIx_CR3_MERRC_Clr(SPI_Type* SPIx);

/* Slave Error Clear，软件写1清除HSPISTA.SERR寄存器 相关函数 */
extern void SPIx_CR3_SERRC_Clr(SPI_Type* SPIx);

/* SPI错误中断使能 (Error Interrupt Enable) 相关函数 */
extern void SPIx_IER_ERRIE_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_IER_ERRIE_Getable(SPI_Type* SPIx);

/* 发送完成中断使能 (Transmit Interrupt Enable) 相关函数 */
extern void SPIx_IER_TXIE_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_IER_TXIE_Getable(SPI_Type* SPIx);

/* 接收完成中断使能 (Receive Interrupt Enable) 相关函数 */
extern void SPIx_IER_RXIE_Setable(SPI_Type* SPIx, FunState NewState);
extern FunState SPIx_IER_RXIE_Getable(SPI_Type* SPIx);

/* 半双工模式下（HALFDUPLEX=1），配置在每个数据帧的最后bit发送的DCN信号电平 (Data/Command transmit config)
0：DCN=0，表示命令帧
1：DCN=1，表示数据帧
软件应在发送前设置DCN_TX寄存器，如果DCN_TX=0，硬件在完成一帧发送后，自动将DCN_TX置1，即默认只会发送一个命令帧，后续都是数据帧。 相关函数 */
extern void SPIx_ISR_DCN_TX_Set(SPI_Type* SPIx, uint32_t SetValue);
extern uint32_t SPIx_ISR_DCN_TX_Get(SPI_Type* SPIx);

/* 接收缓存溢出，软件写1清零 (Receive Collision flag,write 1 to flag) 相关函数 */
extern void SPIx_ISR_RXCOL_Clr(SPI_Type* SPIx);
extern FlagStatus SPIx_ISR_RXCOL_Chk(SPI_Type* SPIx);

/* 发送缓存溢出，软件写1清零 (Transmit Collision flag,write 1 to clear) 相关函数 */
extern void SPIx_ISR_TXCOL_Clr(SPI_Type* SPIx);
extern FlagStatus SPIx_ISR_TXCOL_Chk(SPI_Type* SPIx);

/* SPI空闲标志，只读 (busy flag)
1：SPI传输进行中
0：SPI传输空闲 相关函数 */
extern FlagStatus SPIx_ISR_BUSY_Chk(SPI_Type* SPIx);

/* Master Error标志(Master Error flag)
当Master下传输未满8位SSN就被拉高时，MERR置位 相关函数 */
extern FlagStatus SPIx_ISR_MERR_Chk(SPI_Type* SPIx);

/* Slave Error标志(Slave Error flag)
当Slave下传输未满8位SSN就被拉高时，SERR置位 相关函数 */
extern FlagStatus SPIx_ISR_SERR_Chk(SPI_Type* SPIx);

/* TX Buffer Empty标志位(TX Buffer Empty flag)
1：发送缓存空，软件写TXBUF清零
0：发送缓存满 相关函数 */
extern FlagStatus SPIx_ISR_TXBE_Chk(SPI_Type* SPIx);

/* RX Buffer Full标志位(RX Buffer Full flag)
1：接收缓存满，软件读RXBUF清零
0：接收缓存空 相关函数 */
extern FlagStatus SPIx_ISR_RXBF_Chk(SPI_Type* SPIx);

/* SPI发送缓存 (Transmit Buffer) 相关函数 */
extern void SPIx_TXBUF_Write(SPI_Type* SPIx, uint32_t SetValue);

/* SPI接收缓存 (Receive Buffer) 相关函数 */
extern uint32_t SPIx_RXBUF_Read(SPI_Type* SPIx);
//Announce_End

void SPIx_Deinit(SPI_Type* SPIx);
void SPI_SSN_Set_Low(SPI_Type* SPIx);
void SPI_SSN_Set_High(SPI_Type* SPIx);
uint8_t SPI_RW_Byte(SPI_Type* SPIx, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_SPI_H */
