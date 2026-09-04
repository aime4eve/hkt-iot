/**
  ******************************************************************************
  * @file    fm33a0xxev_lpuart.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the LPUART firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_LPUART_H
#define __FM33A0XXEV_LPUART_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
	 
#define LPUARTx_MCTL_FOR9600 (0x00000552<<16)
#define LPUARTx_MCTL_FOR4800 (0x00001EFB<<16)
#define LPUARTx_MCTL_FOR2400 (0x000016DB<<16)
#define LPUARTx_MCTL_FOR1200 (0x00000492<<16)
#define LPUARTx_MCTL_FOR600  (0x000016D6<<16)
#define LPUARTx_MCTL_FOR300  (0x00000842<<16)	 
	 
#define	LPUARTx_CSR_BUSY_Pos	24	/* LPUART通信标志，只读 (Busy)
1：LPUART正在通信中
0：LPUART空闲 */
#define	LPUARTx_CSR_BUSY_Msk	(0x1U << LPUARTx_CSR_BUSY_Pos)

#define	LPUARTx_CSR_WKBYTECFG_Pos	19	/* 数据接收唤醒条件配置 (Wakeup Byte Config)
1：接收完1字节，并且奇偶校验和STOP位都正确，才触发唤醒中断
0：接收完1字节，不检查校验位和STOP位，直接触发唤醒中断 */
#define	LPUARTx_CSR_WKBYTECFG_Msk	(0x1U << LPUARTx_CSR_WKBYTECFG_Pos)

#define	LPUARTx_CSR_RXEV_Pos	16	/* 唤醒中断事件配置，用于控制何种事件下向CPU提供唤醒中断
(Receive Wakeup Event)
00：START位检测唤醒
01：1byte数据接收完成
10：接收数据匹配成功
11：RXD下降沿检测 */
#define	LPUARTx_CSR_RXEV_Msk	(0x3U << LPUARTx_CSR_RXEV_Pos)
#define	LPUARTx_CSR_RXEV_START	(0x0U << LPUARTx_CSR_RXEV_Pos)	/* START位检测唤醒 */
#define	LPUARTx_CSR_RXEV_1BYTE	(0x1U << LPUARTx_CSR_RXEV_Pos)	/* 1byte数据接收完成 */
#define	LPUARTx_CSR_RXEV_MATCH	(0x2U << LPUARTx_CSR_RXEV_Pos)	/* 接收数据匹配成功 */
#define	LPUARTx_CSR_RXEV_FALLING	(0x3U << LPUARTx_CSR_RXEV_Pos)	/* RXD下降沿检测 */

#define	LPUARTx_CSR_IOSWAP_Pos	11	/* RX与TX引脚交换
0：默认
1：交换 */
#define	LPUARTx_CSR_IOSWAP_Msk	(0x1U << LPUARTx_CSR_IOSWAP_Pos)
#define	LPUARTx_CSR_IOSWAP_DEFUALT	(0x0U << LPUARTx_CSR_IOSWAP_Pos)	/* 默认 */
#define	LPUARTx_CSR_IOSWAP_SWAP		(0x1U << LPUARTx_CSR_IOSWAP_Pos)	/* 交换 */


#define	LPUARTx_CSR_DMATXIFCFG_Pos	10	/* DMA发送完成中断使能，仅在LPUART通过DMA进行发送时有效 (DMA Transmit Interrupt Config)
1：IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出
0：是否允许中断信号输出仅由IE决定 */
#define	LPUARTx_CSR_DMATXIFCFG_Msk	(0x1U << LPUARTx_CSR_DMATXIFCFG_Pos)
	/* IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出 */
	/* 是否允许中断信号输出仅由IE决定 */

#define	LPUARTx_CSR_BITORD_Pos	9	/* 数据发送/接收时的位顺序 (Bit Order)
0：LSB first
1：MSB first */
#define	LPUARTx_CSR_BITORD_Msk	(0x1U << LPUARTx_CSR_BITORD_Pos)
#define	LPUARTx_CSR_BITORD_LSB	(0x0U << LPUARTx_CSR_BITORD_Pos)	/* LSB first */
#define	LPUARTx_CSR_BITORD_MSB	(0x1U << LPUARTx_CSR_BITORD_Pos)	/* MSB first */

#define	LPUARTx_CSR_STOPCFG_Pos	8	/* 停止位宽度配置，仅对发送帧格式有效，接收时不判断停止位个数 (Stop bit Config)
0：1位停止位
1：2位停止位 */
#define	LPUARTx_CSR_STOPCFG_Msk	(0x1U << LPUARTx_CSR_STOPCFG_Pos)
#define	LPUARTx_CSR_STOPCFG_1STOP	(0x0U << LPUARTx_CSR_STOPCFG_Pos)	/* 1位停止位 */
#define	LPUARTx_CSR_STOPCFG_2STOP	(0x1U << LPUARTx_CSR_STOPCFG_Pos)	/* 2位停止位 */

#define	LPUARTx_CSR_PDSEL_Pos	6	/* 每帧数据长度选择；此寄存器对数据发送和接收同时有效
(Payload Data length Select)
00：7位数据
01：8位数据
10：9位数据
11：6位数据 */
#define	LPUARTx_CSR_PDSEL_Msk	(0x3U << LPUARTx_CSR_PDSEL_Pos)
#define	LPUARTx_CSR_PDSEL_7BITS	(0x0U << LPUARTx_CSR_PDSEL_Pos)	/* 7位数据 */
#define	LPUARTx_CSR_PDSEL_8BITS	(0x1U << LPUARTx_CSR_PDSEL_Pos)	/* 8位数据 */
#define	LPUARTx_CSR_PDSEL_9BITS	(0x2U << LPUARTx_CSR_PDSEL_Pos)	/* 9位数据 */
#define	LPUARTx_CSR_PDSEL_6BITS	(0x3U << LPUARTx_CSR_PDSEL_Pos)	/* 6位数据 */

#define	LPUARTx_CSR_PARITY_Pos	4	/* 校验位配置；此寄存器对数据发送和接收同时有效 (Parity)
00：无校验位
01：偶校验
10：奇校验
11：RFU */
#define	LPUARTx_CSR_PARITY_Msk	(0x3U << LPUARTx_CSR_PARITY_Pos)
#define	LPUARTx_CSR_PARITY_NON	(0x0U << LPUARTx_CSR_PARITY_Pos)	/* 无校验位 */
#define	LPUARTx_CSR_PARITY_EVEN	(0x1U << LPUARTx_CSR_PARITY_Pos)	/* 偶校验 */
#define	LPUARTx_CSR_PARITY_ODD	(0x2U << LPUARTx_CSR_PARITY_Pos)	/* 奇校验 */

#define	LPUARTx_CSR_RXPOL_Pos	3	/* 接收数据极性配置 (Receive Polarity)
0：正向
1：取反 */
#define	LPUARTx_CSR_RXPOL_Msk	(0x1U << LPUARTx_CSR_RXPOL_Pos)
#define	LPUARTx_CSR_RXPOL_POS	(0x0U << LPUARTx_CSR_RXPOL_Pos)	/* 正向 */
#define	LPUARTx_CSR_RXPOL_NEG	(0x1U << LPUARTx_CSR_RXPOL_Pos)	/* 取反 */

#define	LPUARTx_CSR_TXPOL_Pos	2	/* 发送数据极性配置 (Transmit Polarity)
0：正向
1：取反 */
#define	LPUARTx_CSR_TXPOL_Msk	(0x1U << LPUARTx_CSR_TXPOL_Pos)
#define	LPUARTx_CSR_TXPOL_POS	(0x0U << LPUARTx_CSR_TXPOL_Pos)	/* 正向 */
#define	LPUARTx_CSR_TXPOL_NEG	(0x1U << LPUARTx_CSR_TXPOL_Pos)	/* 取反 */

#define	LPUARTx_CSR_RXEN_Pos	1	/* 接收使能，1有效 (Receive Enable) */
#define	LPUARTx_CSR_RXEN_Msk	(0x1U << LPUARTx_CSR_RXEN_Pos)

#define	LPUARTx_CSR_TXEN_Pos	0	/* 发送使能，1有效 (Transmit Enable) */
#define	LPUARTx_CSR_TXEN_Msk	(0x1U << LPUARTx_CSR_TXEN_Pos)

#define	LPUARTx_IER_RXEV_IE_Pos	12	/* 接收唤醒事件中断使能，1有效 (Receive Event Interrupt Enable) */
#define	LPUARTx_IER_RXEV_IE_Msk	(0x1U << LPUARTx_IER_RXEV_IE_Pos)

#define	LPUARTx_IER_RXERR_IE_Pos	10	/* 接收错误中断使能，1有效 (Receive Error Interrupt Enable) */
#define	LPUARTx_IER_RXERR_IE_Msk	(0x1U << LPUARTx_IER_RXERR_IE_Pos)

#define	LPUARTx_IER_RXBF_IE_Pos	8	/* 接收缓存满中断使能，1有效 (Receive Buffer Full Interrupt Enable) */
#define	LPUARTx_IER_RXBF_IE_Msk	(0x1U << LPUARTx_IER_RXBF_IE_Pos)

#define	LPUARTx_IER_TXBE_IE_Pos	1	/* 发送缓存空中断使能，1有效 (Transmit Buffer Empty Interrupt Enable) */
#define	LPUARTx_IER_TXBE_IE_Msk	(0x1U << LPUARTx_IER_TXBE_IE_Pos)

#define	LPUARTx_IER_TXSE_IE_Pos	0	/* 发送缓存空且发送移位寄存器空中断使能，1有效 (Transmit Shift register Interrupt Enable) */
#define	LPUARTx_IER_TXSE_IE_Msk	(0x1U << LPUARTx_IER_TXSE_IE_Pos)

#define	LPUARTx_ISR_RXEVF_Pos	24	/* 接收唤醒事件中断标志，硬件置位，软件写1清零 (Receive Event Interrupt Flag)
中断标志触发源由LPUxCR.RXEV寄存器配置。 */
#define	LPUARTx_ISR_RXEVF_Msk	(0x1U << LPUARTx_ISR_RXEVF_Pos)

#define	LPUARTx_ISR_TXOV_Pos	19	/* 发送缓存溢出错误，硬件置位，软件写1清零 (Transmit Overflow Error)
当发送缓存中的数据还未进入移位寄存器发送时，软件向发送缓存写入新数据，将触发TXOV标志置位。 */
#define	LPUARTx_ISR_TXOV_Msk	(0x1U << LPUARTx_ISR_TXOV_Pos)

#define	LPUARTx_ISR_PERR_Pos	18	/* 奇偶校验错误中断标志，硬件置位，软件写1清零 (Parity Error) */
#define	LPUARTx_ISR_PERR_Msk	(0x1U << LPUARTx_ISR_PERR_Pos)

#define	LPUARTx_ISR_FERR_Pos	17	/* 帧格式错误中断标志，硬件置位，软件写1清零 (Frame Error) */
#define	LPUARTx_ISR_FERR_Msk	(0x1U << LPUARTx_ISR_FERR_Pos)

#define	LPUARTx_ISR_OERR_Pos	16	/* 接收缓存溢出错误中断标志，当接收缓存满的情况下，收到新的数据时置位；硬件置位，软件写1清零 (Receive Buffer Overflow Error) */
#define	LPUARTx_ISR_OERR_Msk	(0x1U << LPUARTx_ISR_OERR_Pos)

#define	LPUARTx_ISR_RXBF_Pos	8	/* 接收缓存满中断标志，硬件置位，软件写1或者读取RXBUF时清零 (Receive Buffer Full) */
#define	LPUARTx_ISR_RXBF_Msk	(0x1U << LPUARTx_ISR_RXBF_Pos)

#define	LPUARTx_ISR_TXBE_Pos	1	/* 发送缓存空中断标志，硬件置位，写入TXBUF时清零 (Transmit Buffer Empty) */
#define	LPUARTx_ISR_TXBE_Msk	(0x1U << LPUARTx_ISR_TXBE_Pos)

#define	LPUARTx_ISR_TXSE_Pos	0	/* 发送缓存空且发送移位寄存器空中断标志，硬件置位，软件写1或者发送数据被载入移位寄存器时清零 (Transmit Shift register Empty) */
#define	LPUARTx_ISR_TXSE_Msk	(0x1U << LPUARTx_ISR_TXSE_Pos)

#define	LPUARTx_BMR_MCTL_Pos	16	/* LPUART每个bit的位宽调制控制信号，参见错误!未找到引用源。错误!未找到引用源。 (Bit Modulation Control) */
#define	LPUARTx_BMR_MCTL_Msk	(0x1fffU << LPUARTx_BMR_MCTL_Pos)

#define	LPUARTx_BMR_BAUD_Pos	0	/* 波特率控制（bps）
000：9600
001：4800
010：2400
011：1200
100：600
101/110/111：300 */
#define	LPUARTx_BMR_BAUD_Msk	(0x7U << LPUARTx_BMR_BAUD_Pos)
#define	LPUARTx_BMR_BAUD_9600	(0x0U << LPUARTx_BMR_BAUD_Pos)	/* 9600 */
#define	LPUARTx_BMR_BAUD_4800	(0x1U << LPUARTx_BMR_BAUD_Pos)	/* 4800 */
#define	LPUARTx_BMR_BAUD_2400	(0x2U << LPUARTx_BMR_BAUD_Pos)	/* 2400 */
#define	LPUARTx_BMR_BAUD_1200	(0x3U << LPUARTx_BMR_BAUD_Pos)	/* 1200 */
#define	LPUARTx_BMR_BAUD_600	(0x4U << LPUARTx_BMR_BAUD_Pos)	/* 600 */
#define	LPUARTx_BMR_BAUD_300	(0x5U << LPUARTx_BMR_BAUD_Pos)	/* 300 */

#define	LPUARTx_RXBUF_RXBUF_Pos	0	/* 接收数据缓存寄存器 (Receive Buffer) */
#define	LPUARTx_RXBUF_RXBUF_Msk	(0x1ffU << LPUARTx_RXBUF_RXBUF_Pos)

#define	LPUARTx_TXBUF_TXBUF_Pos	0	/* 发送数据缓存寄存器 (Transmit Buffer) */
#define	LPUARTx_TXBUF_TXBUF_Msk	(0x1ffU << LPUARTx_TXBUF_TXBUF_Pos)

#define	LPUARTx_DMR_MATD_Pos	0	/* 第一帧接收比较数据，如果RXEV=10，当接收到的第一帧数据与MATD相同时，触发RXEVF中断，可以用于休眠模式下的数据接收唤醒。
(Matched Data) */
#define	LPUARTx_DMR_MATD_Msk	(0x1ffU << LPUARTx_DMR_MATD_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void LPUARTx_Deinit(LPUART_Type* LPUARTx);

/* LPUART通信标志，只读 (Busy)
1：LPUART正在通信中
0：LPUART空闲 相关函数 */
extern FlagStatus LPUARTx_CSR_BUSY_Chk(LPUART_Type* LPUARTx);

/* 数据接收唤醒条件配置 相关函数 */
extern void LPUARTx_CSR_WKBYTECFG_Setable(LPUART_Type* LPUARTx, FunState NewState);
	
/* 数据接收唤醒条件配置 (Wakeup Byte Config)
1：接收完1字节，并且奇偶校验和STOP位都正确，才触发唤醒中断
0：接收完1字节，不检查校验位和STOP位，直接触发唤醒中断 相关函数 */
extern FlagStatus LPUARTx_CSR_WKBYTECFG_Chk(LPUART_Type* LPUARTx);

/* 唤醒中断事件配置，用于控制何种事件下向CPU提供唤醒中断
(Receive Wakeup Event)
00：START位检测唤醒
01：1byte数据接收完成
10：接收数据匹配成功
11：RXD下降沿检测 相关函数 */
extern void LPUARTx_CSR_RXEV_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_RXEV_Get(LPUART_Type* LPUARTx);

/* RX与TX引脚交换
0：默认
1：交换 */
extern void LPUARTx_CSR_IOSWAP_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_IOSWAP_Get(LPUART_Type* LPUARTx);


/* DMA发送完成中断使能，仅在LPUART通过DMA进行发送时有效 (DMA Transmit Interrupt Config)
1：IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出
0：是否允许中断信号输出仅由IE决定 相关函数 */
extern void LPUARTx_CSR_DMATXIFCFG_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_CSR_DMATXIFCFG_Getable(LPUART_Type* LPUARTx);

/* 数据发送/接收时的位顺序 (Bit Order)
0：LSB first
1：MSB first 相关函数 */
extern void LPUARTx_CSR_BITORD_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_BITORD_Get(LPUART_Type* LPUARTx);

/* 停止位宽度配置，仅对发送帧格式有效，接收时不判断停止位个数 (Stop bit Config)
0：1位停止位
1：2位停止位 相关函数 */
extern void LPUARTx_CSR_STOPCFG_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_STOPCFG_Get(LPUART_Type* LPUARTx);

/* 每帧数据长度选择；此寄存器对数据发送和接收同时有效
(Payload Data length Select)
00：7位数据
01：8位数据
10：9位数据
11：6位数据 相关函数 */
extern void LPUARTx_CSR_PDSEL_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_PDSEL_Get(LPUART_Type* LPUARTx);

/* 校验位配置；此寄存器对数据发送和接收同时有效 (Parity)
00：无校验位
01：偶校验
10：奇校验
11：RFU 相关函数 */
extern void LPUARTx_CSR_PARITY_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_PARITY_Get(LPUART_Type* LPUARTx);

/* 接收数据极性配置 (Receive Polarity)
0：正向
1：取反 相关函数 */
extern void LPUARTx_CSR_RXPOL_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_RXPOL_Get(LPUART_Type* LPUARTx);

/* 发送数据极性配置 (Transmit Polarity)
0：正向
1：取反 相关函数 */
extern void LPUARTx_CSR_TXPOL_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_CSR_TXPOL_Get(LPUART_Type* LPUARTx);

/* 接收使能，1有效 (Receive Enable) 相关函数 */
extern void LPUARTx_CSR_RXEN_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_CSR_RXEN_Getable(LPUART_Type* LPUARTx);

/* 发送使能，1有效 (Transmit Enable) 相关函数 */
extern void LPUARTx_CSR_TXEN_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_CSR_TXEN_Getable(LPUART_Type* LPUARTx);

/* 接收唤醒事件中断使能，1有效 (Receive Event Interrupt Enable) 相关函数 */
extern void LPUARTx_IER_RXEV_IE_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_IER_RXEV_IE_Getable(LPUART_Type* LPUARTx);

/* 接收错误中断使能，1有效 (Receive Error Interrupt Enable) 相关函数 */
extern void LPUARTx_IER_RXERR_IE_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_IER_RXERR_IE_Getable(LPUART_Type* LPUARTx);

/* 接收缓存满中断使能，1有效 (Receive Buffer Full Interrupt Enable) 相关函数 */
extern void LPUARTx_IER_RXBF_IE_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_IER_RXBF_IE_Getable(LPUART_Type* LPUARTx);

/* 发送缓存空中断使能，1有效 (Transmit Buffer Empty Interrupt Enable) 相关函数 */
extern void LPUARTx_IER_TXBE_IE_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_IER_TXBE_IE_Getable(LPUART_Type* LPUARTx);

/* 发送缓存空且发送移位寄存器空中断使能，1有效 (Transmit Shift register Interrupt Enable) 相关函数 */
extern void LPUARTx_IER_TXSE_IE_Setable(LPUART_Type* LPUARTx,FunState NewState);
extern FunState LPUARTx_IER_TXSE_IE_Getable(LPUART_Type* LPUARTx);

/* 接收唤醒事件中断标志，硬件置位，软件写1清零 (Receive Event Interrupt Flag)
中断标志触发源由LPUxCR.RXEV寄存器配置。 相关函数 */
extern void LPUARTx_ISR_RXEVF_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_RXEVF_Chk(LPUART_Type* LPUARTx);

/* 发送缓存溢出错误，硬件置位，软件写1清零 (Transmit Overflow Error)
当发送缓存中的数据还未进入移位寄存器发送时，软件向发送缓存写入新数据，将触发TXOV标志置位。 相关函数 */
extern void LPUARTx_ISR_TXOV_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_TXOV_Chk(LPUART_Type* LPUARTx);

/* 奇偶校验错误中断标志，硬件置位，软件写1清零 (Parity Error) 相关函数 */
extern void LPUARTx_ISR_PERR_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_PERR_Chk(LPUART_Type* LPUARTx);

/* 帧格式错误中断标志，硬件置位，软件写1清零 (Frame Error) 相关函数 */
extern void LPUARTx_ISR_FERR_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_FERR_Chk(LPUART_Type* LPUARTx);

/* 接收缓存溢出错误中断标志，当接收缓存满的情况下，收到新的数据时置位；硬件置位，软件写1清零 (Receive Buffer Overflow Error) 相关函数 */
extern void LPUARTx_ISR_OERR_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_OERR_Chk(LPUART_Type* LPUARTx);

/* 接收缓存满中断标志，硬件置位，软件写1或者读取RXBUF时清零 (Receive Buffer Full) 相关函数 */
extern void LPUARTx_ISR_RXBF_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_RXBF_Chk(LPUART_Type* LPUARTx);

/* 发送缓存空中断标志，硬件置位，写入TXBUF时清零 (Transmit Buffer Empty) 相关函数 */
extern FlagStatus LPUARTx_ISR_TXBE_Chk(LPUART_Type* LPUARTx);

/* 发送缓存空且发送移位寄存器空中断标志，硬件置位，软件写1或者发送数据被载入移位寄存器时清零 (Transmit Shift register Empty) 相关函数 */
extern void LPUARTx_ISR_TXSE_Clr(LPUART_Type* LPUARTx);
extern FlagStatus LPUARTx_ISR_TXSE_Chk(LPUART_Type* LPUARTx);

/* LPUART每个bit的位宽调制控制信号，参见错误!未找到引用源。错误!未找到引用源。 (Bit Modulation Control) 相关函数 */
extern void LPUARTx_BMR_MCTL_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_BMR_MCTL_Get(LPUART_Type* LPUARTx);

/* 波特率控制（bps）
000：9600
001：4800
010：2400
011：1200
100：600
101/110/111：300 相关函数 */
extern void LPUARTx_BMR_BAUD_Set(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_BMR_BAUD_Get(LPUART_Type* LPUARTx);

/* 接收数据缓存寄存器 (Receive Buffer) 相关函数 */
extern uint32_t LPUARTx_RXBUF_Read(LPUART_Type* LPUARTx);

/* 发送数据缓存寄存器 (Transmit Buffer) 相关函数 */
extern void LPUARTx_TXBUF_Write(LPUART_Type* LPUARTx,uint32_t SetValue);

/* 第一帧接收比较数据，如果RXEV=10，当接收到的第一帧数据与MATD相同时，触发RXEVF中断，可以用于休眠模式下的数据接收唤醒。
(Matched Data) 相关函数 */
extern void LPUARTx_DMR_Write(LPUART_Type* LPUARTx,uint32_t SetValue);
extern uint32_t LPUARTx_DMR_Read(LPUART_Type* LPUARTx);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_LPUART_H */
