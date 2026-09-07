/**
  ******************************************************************************
  * @file    fm33a0xxev_uart.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the UART firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_UART_H
#define __FM33A0XXEV_UART_H

#ifdef __cplusplus
 extern "C" {
#endif
   
/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
#include "fm33a0xxev_CMU.h" 
   
typedef enum
{
	RxInt,
	TxInt	
}UART_IntTypeDef;

typedef enum
{
	Seven7Bit,		
	Eight8Bit,
	Nine9Bit,
	Six6Bit	
}UART_DataBitTypeDef;

typedef enum
{
	NONE,
	EVEN,
	ODD	
}UART_ParityBitTypeDef;

typedef enum
{
	OneBit,
	TwoBit
}UART_StopBitTypeDef;

typedef struct
{
	uint32_t				      BaudRate;		//波特率
	UART_DataBitTypeDef		DataBit;		//数据位数
	UART_ParityBitTypeDef	ParityBit; 	//校验位
	UART_StopBitTypeDef		StopBit;		//停止位
	uint32_t 				      ClockSrc;   // 时钟源选择
}UART_SInitTypeDef;	

typedef struct
{
	FunState				RXBF_IE;		    //接收中断
	FunState				TXBE_IE;		//发送缓存空中断
	FunState				TXSE_IE;		//发送缓存空且发送寄存器空中断
	uint32_t				SPBRG;		  //波特率产生器寄存器
	uint32_t				PDSEL;		  //数据长度选择
	uint32_t				PARITY;		  //校验位配置
  FunState				RXTO_IE;		//接收超时中断
	FunState				ERRIE;		  //错误中断使能控制
	FunState				RXEN;		    //接收模块使能控制
	uint32_t				STOPSEL;	  //停止位选择
	FunState				TXEN;		    //发送模块使能控制
	FunState				IREN;		    //发送红外调制使能位
	FunState				RXDFLAG;	  //接收数据取反控制位
	FunState				TXDFLAG;	  //发送数据取反控制位	
}UART_InitTypeDef;	

#define	UARTIR_CR_IRFLAG_Pos	15	/* 控制红外调制发送数据时的默认输出极性 (Infra Red Flag)
0：正极性
1：负极性 */
#define	UARTIR_CR_IRFLAG_Msk	(0x1U << UARTIR_CR_IRFLAG_Pos)
#define	UARTIR_CR_IRFLAG_POSITIVE	(0x0U << UARTIR_CR_IRFLAG_Pos)	/* 正极性 */
#define	UARTIR_CR_IRFLAG_NEGTIVE	(0x1U << UARTIR_CR_IRFLAG_Pos)	/* 负极性 */

#define	UARTIR_CR_TH_Pos	11	/* 红外占空比调制参数 (Tranmission High Duty) */
#define	UARTIR_CR_TH_Msk	(0xfU << UARTIR_CR_TH_Pos)

#define	UARTIR_CR_TZBRG_Pos	0	/* 红外调制频率 (Transmission Baud Rate) */
#define	UARTIR_CR_TZBRG_Msk	(0x7ffU << UARTIR_CR_TZBRG_Pos)

#define	UARTx_CSR_BUSY_Pos	24	/* UART通信标志，只读
1：UART正在通信中
0：UART空闲 */
#define	UARTx_CSR_BUSY_Msk	(0x1U << UARTx_CSR_BUSY_Pos)

#define	UARTx_CSR_TXIREN_Pos	17	/* 发送红外调制使能
1：使能红外调制发送
0：关闭红外调制发送 */
#define	UARTx_CSR_TXIREN_Msk	(0x1U << UARTx_CSR_TXIREN_Pos)
	/* 使能红外调制发送 */
	/* 关闭红外调制发送 */

#define	UARTx_CSR_RXTOEN_Pos	16	/* 接收超时使能
1：使能接收超时功能
0：关闭接收超时功能 */
#define	UARTx_CSR_RXTOEN_Msk	(0x1U << UARTx_CSR_RXTOEN_Pos)
	/* 使能接收超时功能 */
	/* 关闭接收超时功能 */

#define	UARTx_CSR_IOSWAP_Pos	12	/* RX和TX引脚交换
0：默认引脚顺序（与封装图一致）
1：交换引脚顺序 */
#define	UARTx_CSR_IOSWAP_Msk	(0x1U << UARTx_CSR_IOSWAP_Pos)
#define	UARTx_CSR_IOSWAP_DEFAULT	(0x0U << UARTx_CSR_IOSWAP_Pos)	/* 默认引脚顺序（与封装图一致） */
#define	UARTx_CSR_IOSWAP_EXCHANGE	(0x1U << UARTx_CSR_IOSWAP_Pos)	/* 交换引脚顺序 */

#define	UARTx_CSR_DMATXIFCFG_Pos	10	/* DMA发送完成中断使能，仅在UART通过DMA进行发送时有效
1：IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出
0：是否允许中断信号输出仅由IE决定 */
#define	UARTx_CSR_DMATXIFCFG_Msk	(0x1U << UARTx_CSR_DMATXIFCFG_Pos)
	/* IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出 */
	/* 是否允许中断信号输出仅由IE决定 */

#define	UARTx_CSR_BITORD_Pos	9	/* 数据发送/接收时的位顺序
0：LSB first
1：MSB first */
#define	UARTx_CSR_BITORD_Msk	(0x1U << UARTx_CSR_BITORD_Pos)
#define	UARTx_CSR_BITORD_LSB	(0x0U << UARTx_CSR_BITORD_Pos)	/* LSB first */
#define	UARTx_CSR_BITORD_MSB	(0x1U << UARTx_CSR_BITORD_Pos)	/* MSB first */

#define	UARTx_CSR_STOPCFG_Pos	8	/* 停止位宽度配置，仅对发送帧格式有效，接收时不判断停止位个数
0：1位停止位
1：2位停止位 */
#define	UARTx_CSR_STOPCFG_Msk	(0x1U << UARTx_CSR_STOPCFG_Pos)
#define	UARTx_CSR_STOPCFG_1STOPBIT	(0x0U << UARTx_CSR_STOPCFG_Pos)	/* 1位停止位 */
#define	UARTx_CSR_STOPCFG_2STOPBIT	(0x1U << UARTx_CSR_STOPCFG_Pos)	/* 2位停止位 */

#define	UARTx_CSR_PDSEL_Pos	6	/* 每帧的数据长度选择；此寄存器对数据发送和接收同时有效
00：7位数据
01：8位数据
10：9位数据
11：6位数据 */
#define	UARTx_CSR_PDSEL_Msk	(0x3U << UARTx_CSR_PDSEL_Pos)
#define	UARTx_CSR_PDSEL_7BIT	(0x0U << UARTx_CSR_PDSEL_Pos)	/* 7位数据 */
#define	UARTx_CSR_PDSEL_8BIT	(0x1U << UARTx_CSR_PDSEL_Pos)	/* 8位数据 */
#define	UARTx_CSR_PDSEL_9BIT	(0x2U << UARTx_CSR_PDSEL_Pos)	/* 9位数据 */
#define	UARTx_CSR_PDSEL_6BIT	(0x3U << UARTx_CSR_PDSEL_Pos)	/* 6位数据 */

#define	UARTx_CSR_PARITY_Pos	4	/* 校验位配置；此寄存器对数据发送和接收同时有效
00：无校验位
01：偶校验
10：奇校验
11：RFU */
#define	UARTx_CSR_PARITY_Msk	(0x3U << UARTx_CSR_PARITY_Pos)
#define	UARTx_CSR_PARITY_NONE	(0x0U << UARTx_CSR_PARITY_Pos)	/* 无校验位 */
#define	UARTx_CSR_PARITY_EVEN	(0x1U << UARTx_CSR_PARITY_Pos)	/* 偶校验 */
#define	UARTx_CSR_PARITY_ODD	(0x2U << UARTx_CSR_PARITY_Pos)	/* 奇校验 */

#define	UARTx_CSR_RXPOL_Pos	3	/* 接收数据极性配置
0：正向
1：取反 */
#define	UARTx_CSR_RXPOL_Msk	(0x1U << UARTx_CSR_RXPOL_Pos)
#define	UARTx_CSR_RXPOL_POSITIVE	(0x0U << UARTx_CSR_RXPOL_Pos)	/* 正向 */
#define	UARTx_CSR_RXPOL_NEGTIVE	(0x1U << UARTx_CSR_RXPOL_Pos)	/* 取反 */

#define	UARTx_CSR_TXPOL_Pos	2	/* 发送数据极性配置
0：正向
1：取反 */
#define	UARTx_CSR_TXPOL_Msk	(0x1U << UARTx_CSR_TXPOL_Pos)
#define	UARTx_CSR_TXPOL_POSITIVE	(0x0U << UARTx_CSR_TXPOL_Pos)	/* 正向 */
#define	UARTx_CSR_TXPOL_NEGTIVE	(0x1U << UARTx_CSR_TXPOL_Pos)	/* 取反 */

#define	UARTx_CSR_RXEN_Pos	1	/* 接收使能，1有效 */
#define	UARTx_CSR_RXEN_Msk	(0x1U << UARTx_CSR_RXEN_Pos)
#define	UARTx_CSR_RXEN_DISBALE	(0x0U << UARTx_CSR_RXEN_Pos)
#define	UARTx_CSR_RXEN_ENABLE	(0x1U << UARTx_CSR_RXEN_Pos)

#define	UARTx_CSR_TXEN_Pos	0	/* 发送使能，1有效 */
#define	UARTx_CSR_TXEN_Msk	(0x1U << UARTx_CSR_TXEN_Pos)
#define	UARTx_CSR_TXEN_DISBALE	(0x0U << UARTx_CSR_TXEN_Pos)
#define	UARTx_CSR_TXEN_ENABLE	(0x1U << UARTx_CSR_TXEN_Pos)

#define	UARTx_IER_RXTO_IE_Pos	11	/* 接收超时中断使能，1有效
（仅UART0和UART1有效） */
#define	UARTx_IER_RXTO_IE_Msk	(0x1U << UARTx_IER_RXTO_IE_Pos)

#define	UARTx_IER_RXERR_IE_Pos	10	/* 接收错误中断使能，1有效 */
#define	UARTx_IER_RXERR_IE_Msk	(0x1U << UARTx_IER_RXERR_IE_Pos)

#define	UARTx_IER_RXBF_IE_Pos	8	/* 接收缓存满中断使能，1有效 */
#define	UARTx_IER_RXBF_IE_Msk	(0x1U << UARTx_IER_RXBF_IE_Pos)

#define	UARTx_IER_NEWUP_IE_Pos	7	/* RX下降沿异步检测中断使能，1有效（仅UART0和UART1有效） */
#define	UARTx_IER_NEWUP_IE_Msk	(0x1U << UARTx_IER_NEWUP_IE_Pos)

#define	UARTx_IER_TXBE_IE_Pos	1	/* 发送缓存空中断使能，1有效 */
#define	UARTx_IER_TXBE_IE_Msk	(0x1U << UARTx_IER_TXBE_IE_Pos)

#define	UARTx_IER_TXSE_IE_Pos	0	/* 发送缓存空且发送移位寄存器空中断使能，1有效 */
#define	UARTx_IER_TXSE_IE_Msk	(0x1U << UARTx_IER_TXSE_IE_Pos)

#define	UARTx_ISR_PERR_Pos	18	/* 奇偶校验错误中断标志，硬件置位，软件写1清零 */
#define	UARTx_ISR_PERR_Msk	(0x1U << UARTx_ISR_PERR_Pos)

#define	UARTx_ISR_FERR_Pos	17	/* 帧格式错误中断标志，硬件置位，软件写1清零 */
#define	UARTx_ISR_FERR_Msk	(0x1U << UARTx_ISR_FERR_Pos)

#define	UARTx_ISR_OERR_Pos	16	/* 接收缓存溢出错误中断标志，当接收缓存满的情况下，收到新的数据时置位；硬件置位，软件写1或者读取RXBUF时清零
接收溢出时，接收缓冲器中原有的数据被新数据覆盖。 */
#define	UARTx_ISR_OERR_Msk	(0x1U << UARTx_ISR_OERR_Pos)

#define	UARTx_ISR_RXTO_Pos	11	/* 接收超时中断标志，硬件置位，软件写1清零
（仅UART0和UART1有效） */
#define	UARTx_ISR_RXTO_Msk	(0x1U << UARTx_ISR_RXTO_Pos)

#define	UARTx_ISR_RXBF_Pos	8	/* 接收缓存满中断标志，硬件置位，软件写1或者读取RXBUF时清零 */
#define	UARTx_ISR_RXBF_Msk	(0x1U << UARTx_ISR_RXBF_Pos)

#define	UARTx_ISR_TXBE_Pos	1	/* 发送缓存空中断标志，硬件置位，软件写入TXBUF时清零 */
#define	UARTx_ISR_TXBE_Msk	(0x1U << UARTx_ISR_TXBE_Pos)

#define	UARTx_ISR_TXSE_Pos	0	/* 发送缓存空且移位寄存器发送完成中断标志，硬件置位，软件写1或者软件写发送缓存时清零 */
#define	UARTx_ISR_TXSE_Msk	(0x1U << UARTx_ISR_TXSE_Pos)

#define	UARTx_TODR_TXDLY_LEN_Pos	8	/* 发送延迟，最大255baud */
#define	UARTx_TODR_TXDLY_LEN_Msk	(0xffU << UARTx_TODR_TXDLY_LEN_Pos)

#define	UARTx_TODR_RXTO_LEN_Pos	0	/* 接收超时溢出长度，最大255baud */
#define	UARTx_TODR_RXTO_LEN_Msk	(0xffU << UARTx_TODR_RXTO_LEN_Pos)

#define	UARTx_RXBUF_RXBUF_Pos	0	/* 接收数据缓冲寄存器数据 */
#define	UARTx_RXBUF_RXBUF_Msk	(0x1ffU << UARTx_RXBUF_RXBUF_Pos)

#define	UARTx_TXBUF_TXBUF_Pos	0	/* 发送数据缓冲寄存器数据 */
#define	UARTx_TXBUF_TXBUF_Msk	(0x1ffU << UARTx_TXBUF_TXBUF_Pos)

#define	UARTx_BRG_SPBRG_Pos	0	/* 波特率产生器寄存器值 */
#define	UARTx_BRG_SPBRG_Msk	(0xffffU << UARTx_BRG_SPBRG_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void UART_Deinit(void);

/* 控制红外调制发送数据时的默认输出极性 (Infra Red Flag)
0：正极性
1：负极性 相关函数 */
extern void UARTIR_CR_IRFLAG_Set(uint32_t SetValue);
extern uint32_t UARTIR_CR_IRFLAG_Get(void);

/* 红外占空比调制参数 (Tranmission High Duty) 相关函数 */
extern void UARTIR_CR_TH_Set(uint32_t SetValue);
extern uint32_t UARTIR_CR_TH_Get(void);

/* 红外调制频率 (Transmission Baud Rate) 相关函数 */
extern void UARTIR_CR_TZBRG_Set(uint32_t SetValue);
extern uint32_t UARTIR_CR_TZBRG_Get(void);
extern void UARTx_Deinit(UART_Type* UARTx);

/* UART通信标志，只读
1：UART正在通信中
0：UART空闲 相关函数 */
extern FlagStatus UARTx_CSR_BUSY_Chk(UART_Type* UARTx);

/* 发送红外调制使能
1：使能红外调制发送
0：关闭红外调制发送 相关函数 */
extern void UARTx_CSR_TXIREN_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_TXIREN_Getable(UART_Type* UARTx);

/* 接收超时使能
1：使能接收超时功能
0：关闭接收超时功能 相关函数 */
extern void UARTx_CSR_RXTOEN_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_RXTOEN_Getable(UART_Type* UARTx);

/* RX和TX引脚交换
0：默认引脚顺序（与封装图一致）
1：交换引脚顺序 相关函数 */
extern void UARTx_CSR_IOSWAP_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_CSR_IOSWAP_Get(UART_Type* UARTx);

/* DMA发送完成中断使能，仅在UART通过DMA进行发送时有效
1：IE=1的情况下，DMA模式下发送完最后一帧后，允许中断信号输出；最后一帧之前的数据帧发送完成后不允许中断信号输出
0：是否允许中断信号输出仅由IE决定 相关函数 */
extern void UARTx_CSR_DMATXIFCFG_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_DMATXIFCFG_Getable(UART_Type* UARTx);

/* 数据发送/接收时的位顺序
0：LSB first
1：MSB first 相关函数 */
extern void UARTx_CSR_BITORD_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_CSR_BITORD_Get(UART_Type* UARTx);

/* 停止位宽度配置，仅对发送帧格式有效，接收时不判断停止位个数
0：1位停止位
1：2位停止位 相关函数 */
extern void UARTx_CSR_STOPCFG_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_CSR_STOPCFG_Get(UART_Type* UARTx);

/* 每帧的数据长度选择；此寄存器对数据发送和接收同时有效
00：7位数据
01：8位数据
10：9位数据
11：6位数据 相关函数 */
extern void UARTx_CSR_PDSEL_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_CSR_PDSEL_Get(UART_Type* UARTx);

/* 校验位配置；此寄存器对数据发送和接收同时有效
00：无校验位
01：偶校验
10：奇校验
11：RFU 相关函数 */
extern void UARTx_CSR_PARITY_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_CSR_PARITY_Get(UART_Type* UARTx);

/* 接收数据极性配置
0：正向
1：取反 相关函数 */
extern void UARTx_CSR_RXPOL_Setable(UART_Type* UARTx,FunState NewState);
extern FunState UARTx_CSR_RXPOL_Getable(UART_Type* UARTx);

/* 发送数据极性配置
0：正向
1：取反 相关函数 */
extern void UARTx_CSR_TXPOL_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_TXPOL_Getable(UART_Type* UARTx);

/* 接收使能，1有效 相关函数 */
extern void UARTx_CSR_RXEN_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_RXEN_Getable(UART_Type* UARTx);

/* 发送使能，1有效 相关函数 */
extern void UARTx_CSR_TXEN_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_CSR_TXEN_Getable(UART_Type* UARTx);

/* 接收超时中断使能，1有效
（仅UART0和UART1有效） 相关函数 */
extern void UARTx_IER_RXTO_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_RXTO_IE_Getable(UART_Type* UARTx);

/* 接收错误中断使能，1有效 相关函数 */
extern void UARTx_IER_RXERR_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_RXERR_IE_Getable(UART_Type* UARTx);

/* 接收缓存满中断使能，1有效 相关函数 */
extern void UARTx_IER_RXBF_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_RXBF_IE_Getable(UART_Type* UARTx);

/* RX下降沿异步检测中断使能，1有效（仅UART0和UART1有效） 相关函数 */
extern void UARTx_IER_NEWUP_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_NEWUP_IE_Getable(UART_Type* UARTx);

/* 发送缓存空中断使能，1有效 相关函数 */
extern void UARTx_IER_TXBE_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_TXBE_IE_Getable(UART_Type* UARTx);

/* 发送缓存空且发送移位寄存器空中断使能，1有效 相关函数 */
extern void UARTx_IER_TXSE_IE_Setable(UART_Type* UARTx, FunState NewState);
extern FunState UARTx_IER_TXSE_IE_Getable(UART_Type* UARTx);

/* 奇偶校验错误中断标志，硬件置位，软件写1清零 相关函数 */
extern void UARTx_ISR_PERR_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_PERR_Chk(UART_Type* UARTx);

/* 帧格式错误中断标志，硬件置位，软件写1清零 相关函数 */
extern void UARTx_ISR_FERR_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_FERR_Chk(UART_Type* UARTx);

/* 接收缓存溢出错误中断标志，当接收缓存满的情况下，收到新的数据时置位；硬件置位，软件写1或者读取RXBUF时清零
接收溢出时，接收缓冲器中原有的数据被新数据覆盖。 相关函数 */
extern void UARTx_ISR_OERR_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_OERR_Chk(UART_Type* UARTx);

/* 接收超时中断标志，硬件置位，软件写1清零
（仅UART0和UART1有效） 相关函数 */
extern void UARTx_ISR_RXTO_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_RXTO_Chk(UART_Type* UARTx);

/* 接收缓存满中断标志，硬件置位，软件写1或者读取RXBUF时清零 相关函数 */
extern void UARTx_ISR_RXBF_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_RXBF_Chk(UART_Type* UARTx);

/* RX下降沿异步检测中断标志，硬件置位，软件写1清零
（仅UART0和UART1有效） 相关函数 */
extern void UARTx_ISR_NEWKF_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_NEWKF_Chk(UART_Type* UARTx);

/* 发送缓存空中断标志，硬件置位，软件写入TXBUF时清零 相关函数 */
extern void UARTx_ISR_TXBE_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_TXBE_Chk(UART_Type* UARTx);

/* 发送缓存空且移位寄存器发送完成中断标志，硬件置位，软件写1或者软件写发送缓存时清零 相关函数 */
extern void UARTx_ISR_TXSE_Clr(UART_Type* UARTx);
extern FlagStatus UARTx_ISR_TXSE_Chk(UART_Type* UARTx);

/* 发送延迟，最大255baud 相关函数 */
extern void UARTx_TODR_TXDLY_LEN_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_TODR_TXDLY_LEN_Get(UART_Type* UARTx);

/* 接收超时溢出长度，最大255baud 相关函数 */
extern void UARTx_TODR_RXTO_LEN_Set(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_TODR_RXTO_LEN_Get(UART_Type* UARTx);

/* 接收数据缓冲寄存器数据 相关函数 */
extern uint32_t UARTx_RXBUF_Read(UART_Type* UARTx);

/* 发送数据缓冲寄存器数据 相关函数 */
extern void UARTx_TXBUF_Write(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_TXBUF_Read(UART_Type* UARTx);

/* 波特率产生器寄存器值 相关函数 */
extern void UARTx_BGR_Write(UART_Type* UARTx, uint32_t SetValue);
extern uint32_t UARTx_BGR_Read(UART_Type* UARTx);

extern uint32_t UART_BaudREGCalc(uint32_t BaudRate, uint32_t Clk);
extern void UART_IRModulation_Init( uint32_t ModuFreq, uint8_t ModuDutyCycle, uint32_t Clk );
extern void UART_Init(UART_Type* UARTx, UART_InitTypeDef* para);
extern void UART_SInit(UART_Type* UARTx, UART_SInitTypeDef* para,CMU_ClocksType* CMU_Clocks);

//Announce_End
#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_UART_H */
