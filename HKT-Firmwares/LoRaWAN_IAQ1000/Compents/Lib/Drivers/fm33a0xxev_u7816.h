/**
  ******************************************************************************
  * @file    fm33a0xxev_u7816.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the U7816 firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_U7816_H
#define __FM33A0XXEV_U7816_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
 
typedef struct
{
	FunState				TXEN;		//发送使能
	FunState				RXEN;		//接受使能
	FunState				CKOEN;		//U7816时钟CLK输出使能控制
	FunState				HPUAT;		//U7816通道数据发送强上拉电阻自动有效控制
	FunState				HPUEN;		//U7816通道强上拉使能控制
	
	uint32_t				ERSW;		//ERROR SIGNAL宽度选择
	uint32_t				ERSGD;		//ERROR SIGNAL后GUARDTIME宽度选择（仅在发送时有效）
	FunState				BGTEN;		//BGT（block guard time）控制
	uint32_t				REP_T;		//控制接收数据奇偶校验出错时自动重发次数
	uint32_t				PAR;		//奇偶校验类型选择
	uint32_t				SFREN;		//Guard Time长度控制位（发送时严格按照协议2etu）
  uint32_t				RFREN;    //接收Guard Time长度控制位
	FunState				TREPEN;		//缓发送数据奇偶校验错的处理方式选择
	FunState				RREPEN;		//接收数据奇偶校验错的处理方式选择
	FunState				DICONV;		//传输次序反向编码使能
	
	uint32_t				TXEGT;		//发送时插入的EGT时间（以ETU为单位）
	uint32_t				CLKDIV;		//U7816时钟输出分频控制寄存器
	uint32_t				PDIV;		//U7816预分频控制寄存器，控制7816通信分频比（波特率）
	
	FunState				RXIE;		//数据接收中断使能位。对应RX_FLAG中断标志位
	FunState				TXIE;		//数据发送中断使能位。对应TX_FLAG中断标志位
	FunState				LSIE;		//线路状态中断使能位。对应ERROR_FLAG中断标志位
	
}U7816_InitTypeDef;


#define	U7816_CR_TXEN_Pos	5	/* U7816通道发送使能控制位 (Transmit Enable)1：通道发送使能，可发送数据0：通道发送禁止，不可发送数据，并关断输出端口，将SCL信号转化为低电平 */
#define	U7816_CR_TXEN_Msk	(0x1U << U7816_CR_TXEN_Pos)

#define	U7816_CR_RXEN_Pos	4	/* U7816通道接收使能控制位 (Receive Enable)1：通道接收使能，可接收数据0：通道接收禁止，不可接收数据，并关断输入端口 */
#define	U7816_CR_RXEN_Msk	(0x1U << U7816_CR_RXEN_Pos)

#define	U7816_CR_CKOEN_Pos	3	/* U7816时钟CLK输出使能控制位 (Clock output Enable)1：7816时钟输出使能0：7816时钟输出禁止 */
#define	U7816_CR_CKOEN_Msk	(0x1U << U7816_CR_CKOEN_Pos)

#define	U7816_CR_HPUAT_Pos	2	/* U7816通道数据发送强上拉电阻自动有效控制位 (High-Pullup Automatically)1：数据发送时上拉电阻自动有效，接收态上拉电阻无效0：数据发送时上拉电阻自动有效功能禁止，上拉电阻由HPUEN，LPUEN控制 */
#define	U7816_CR_HPUAT_Msk	(0x1U << U7816_CR_HPUAT_Pos)

#define	U7816_CR_HPUEN_Pos	1	/* U7816通道强上拉使能控制位 (High-Pullup Enable)1：强上拉有效0：强上拉无效 */
#define	U7816_CR_HPUEN_Msk	(0x1U << U7816_CR_HPUEN_Pos)

#define	U7816_FFR_SFREN_Pos	11	/* Guard Time发送长度控制位 (Send long Frame Enable)1：Guard time为3 etu0：Guard time为2 etu */
#define	U7816_FFR_SFREN_Msk	(0x1U << U7816_FFR_SFREN_Pos)
#define	U7816_FFR_SFREN_3ETU	(0x1U << U7816_FFR_SFREN_Pos)
#define	U7816_FFR_SFREN_2ETU	(0x0U << U7816_FFR_SFREN_Pos)

#define	U7816_FFR_ERSW_Pos	9	/* ERROR SIGNAL宽度选择 (Error Signal Width)11：ERROR SIGNAL宽度为1ETU;10：ERROR SIGNAL宽度为1.5ETU;01：ERROR SIGNAL宽度为2ETU;00：ERROR SIGNAL宽度为2ETU; */
#define	U7816_FFR_ERSW_Msk	(0x3U << U7816_FFR_ERSW_Pos)
#define	U7816_FFR_ERSW_1ETU	(0x0U << U7816_FFR_ERSW_Pos)
#define	U7816_FFR_ERSW_1P5ETU	(0x1U << U7816_FFR_ERSW_Pos)
#define	U7816_FFR_ERSW_2ETU	(0x2U << U7816_FFR_ERSW_Pos)

#define	U7816_FFR_ERSGD_Pos	8	/* ERROR SIGNAL后GUARDTIME宽度选择（仅在发送时有效）
(Error Signal Guard Time)1：ERROR SIGNAL后GUARDTIME为1~1.5ETU。0：ERROR SIGNAL后GUARDTIME为2~2.5ETU。ERROR SIGNAL宽度为整数ETU时GUARDTIME为1.5或2.5ETU；ERROR SIGNAL宽度为1.5ETU时GUARDTIME为1或2ETU */
#define	U7816_FFR_ERSGD_Msk	(0x1U << U7816_FFR_ERSGD_Pos)
#define	U7816_FFR_ERSGD_1ETU	(0x1U << U7816_FFR_ERSGD_Pos)	/* ERROR SIGNAL后GUARDTIME为1~1.5ETU。0 */
#define	U7816_FFR_ERSGD_2ETU	(0x0U << U7816_FFR_ERSGD_Pos)

#define	U7816_FFR_BGTEN_Pos	7	/* BGT控制位。控制接收->发送之间是否插入BGT。BGT是接收->发送之间需要的最小时间（block guard time enable）1：BGT使能，插入Block guard time(12 etu);0：BGT禁止，不插入Block guard time(12 etu); */
#define	U7816_FFR_BGTEN_Msk	(0x1U << U7816_FFR_BGTEN_Pos)

#define	U7816_FFR_REP_T_Pos	6	/* 控制接收数据奇偶校验出错时自动重发次数 (Repeated Times)1：3次0：1次 */
#define	U7816_FFR_REP_T_Msk	(0x1U << U7816_FFR_REP_T_Pos)
#define	U7816_FFR_REP_T_1TIMES	(0x0U << U7816_FFR_REP_T_Pos)
#define	U7816_FFR_REP_T_3TIMES	(0x1U << U7816_FFR_REP_T_Pos)

#define	U7816_FFR_PAR_Pos	4	/* 奇偶校验类型选择 (Parity)00：Even01：Odd10：Always 111：不校验，处理 */
#define	U7816_FFR_PAR_Msk	(0x3U << U7816_FFR_PAR_Pos)
#define	U7816_FFR_PAR_EVEN	(0x0U << U7816_FFR_PAR_Pos)
#define	U7816_FFR_PAR_ODD	(0x1U << U7816_FFR_PAR_Pos)
#define	U7816_FFR_PAR_1ALAWAYS	(0x2U << U7816_FFR_PAR_Pos)
#define	U7816_FFR_PAR_NON	(0x3U << U7816_FFR_PAR_Pos)

#define	U7816_FFR_RFREN_Pos	3	/* Guard Time接收长度控制位 (Receive short Frame )1：Guard time为1 etu0：Guard time为2 etu */
#define	U7816_FFR_RFREN_Msk	(0x1U << U7816_FFR_RFREN_Pos)
#define	U7816_FFR_RFREN_1ETU	(0x1U << U7816_FFR_RFREN_Pos)
#define	U7816_FFR_RFREN_2ETU	(0x0U << U7816_FFR_RFREN_Pos)

#define	U7816_FFR_TREPEN_Pos	2	/* 发送数据奇偶校验错的处理方式选择 (Transmit Repeat Enable)1：收到奇偶校验出错标志（error signal），根据T＝0协议自动进行回发。在单一byte重复发送次数超过REP_T后，置tx_parity_err标志，进行中断0：收到Error signal时不进行自动回发，置tx_parity_err标志，直接中断 */
#define	U7816_FFR_TREPEN_Msk	(0x1U << U7816_FFR_TREPEN_Pos)

#define	U7816_FFR_RREPEN_Pos	1	/* 接收数据奇偶校验错的处理方式选择 (Receive Repeat Enable)1：奇偶校验错，根据T=0协议自动回发ERROR SIGNAL。单一BYTE连续接收次数超过REP_T后，置RX_PARITY_ERR标志，进行中断0：奇偶校验错，不自动发送ERROR SIGNAL，置RX_PARITY_ERR标志，进行中断 */
#define	U7816_FFR_RREPEN_Msk	(0x1U << U7816_FFR_RREPEN_Pos)

#define	U7816_FFR_DICONV_Pos	0	/* 传输次序，编码方式选择 (bit Direction Conversion)1：反向编码，先收发MSB；(收发数据+校验位)反逻辑电平0：正向编码，先收发LSB ； (收发数据+校验位)正逻辑电平 */
#define	U7816_FFR_DICONV_Msk	(0x1U << U7816_FFR_DICONV_Pos)
#define	U7816_FFR_DICONV_FORWARD	(0x0U << U7816_FFR_DICONV_Pos)
#define	U7816_FFR_DICONV_REVERSE	(0x1U << U7816_FFR_DICONV_Pos)

#define	U7816_EGTR_TXEGT_Pos	0	/* 发送时插入的Extra Guard Time时间（以ETU为单位）
(Transmit Extra Guard Time) */
#define	U7816_EGTR_TXEGT_Msk	(0xffU << U7816_EGTR_TXEGT_Pos)

#define	U7816_PSC_CLKDIV_Pos	0	/* U7816时钟输出分频控制寄存器(Clock Divider)，控制7816工作时钟分频数。U7816工作时钟与APBCLK的分频关系：F7816=FAPBCLK/(CLKDIV+1)特殊情况：CLK_DIV设置成0或1时，F7816=FAPBCLK/2注：7816协议规定的工作时钟范围是1~5MHZ。 */
#define	U7816_PSC_CLKDIV_Msk	(0x1fU << U7816_PSC_CLKDIV_Pos)

#define	U7816_BGR_PDIV_Pos	0	/* U7816预分频控制寄存器(Pre-Divider)，控制7816通信分频比（波特率）Baud ＝ F7816/(PDIV ＋ 1)

注意：PDIV最小可用值是0x1，应用禁止配置0x0 */
#define	U7816_BGR_PDIV_Msk	(0xfffU << U7816_BGR_PDIV_Pos)

#define	U7816_RXBUF_RXBUF_Pos	0	/* U7816数据接收缓存寄存器 (Receive Buffer) */
#define	U7816_RXBUF_RXBUF_Msk	(0xffU << U7816_RXBUF_RXBUF_Pos)

#define	U7816_TXBUF_TXBUF_Pos	0	/* U7816数据发送缓存寄存器 (Transmit Buffer) */
#define	U7816_TXBUF_TXBUF_Msk	(0xffU << U7816_TXBUF_TXBUF_Pos)

#define	U7816_IER_RXIE_Pos	2	/* 数据接收中断使能位。对应RXIF中断标志位 (Receive Interrupt Enable)1：当RXIF寄存器置位时产生接收完成中断0：禁止接收完成中断 */
#define	U7816_IER_RXIE_Msk	(0x1U << U7816_IER_RXIE_Pos)

#define	U7816_IER_TXIE_Pos	1	/* 数据发送中断使能位。对应TXIF中断标志位(Transmit Interrupt Enable)1：当TXIF寄存器置位时产生发送完成中断0：禁止发送完成中断 */
#define	U7816_IER_TXIE_Msk	(0x1U << U7816_IER_TXIE_Pos)

#define	U7816_IER_LSIE_Pos	0	/* 线路状态中断使能位。对应ERRIF中断标志位(Line Status Interrupt Enable)1：当ERRIF寄存器置位时产生线路错误中断0：禁止线路错误中断 */
#define	U7816_IER_LSIE_Msk	(0x1U << U7816_IER_LSIE_Pos)

#define	U7816_ISR_WAIT_RPT_Pos	18	/* U7816接口发送了错误信号，正在等待对方重发数据；(Waiting for Repeat flag)状态机进入发送错误信号状态时置位，收到数据起始位或者进入发送状态时硬件清零；软件只读。 */
#define	U7816_ISR_WAIT_RPT_Msk	(0x1U << U7816_ISR_WAIT_RPT_Pos)

#define	U7816_ISR_TXBUSY_Pos	17	/* 发送数据忙标志。（发送完成后自动清零）(Transmission busy flag)1：处于数据发送状态，发送移位寄存器正在发送数据。（开始发送起始位置1，停止位中间清零）0：数据发送空闲 */
#define	U7816_ISR_TXBUSY_Msk	(0x1U << U7816_ISR_TXBUSY_Pos)

#define	U7816_ISR_RXBUSY_Pos	16	/* 接收数据忙标志。（接收完成后自动清零）(Receiving busy flag)1：处于数据接收状态，接收移位寄存器正在接收数据。（收到起始位置1，收到停止位清零，若接收数据出错需重发，则回发error signal时清零。即数据及校验位接收之后，无论是否需要重发，都需要及时清除该标志）0：数据接收空闲 */
#define	U7816_ISR_RXBUSY_Msk	(0x1U << U7816_ISR_RXBUSY_Pos)

#define	U7816_ISR_TPARERR_Pos	11	/* 发送数据奇偶校验错误标志位。硬件置位，写1清零 (Transmit Parity Error,write 1 to clear) */
#define	U7816_ISR_TPARERR_Msk	(0x1U << U7816_ISR_TPARERR_Pos)

#define	U7816_ISR_RPARERR_Pos	10	/* 接收数据奇偶校验错误标志位。硬件置位，写1清零 (Receive Parity Error flag,write 1 to clear) */
#define	U7816_ISR_RPARERR_Msk	(0x1U << U7816_ISR_RPARERR_Pos)

#define	U7816_ISR_FRERR_Pos	9	/* 接收帧格式错误标志位。硬件置位，写1清零 (Frame Error flag,write 1 to clear)1：帧格式有错误，接收到的frame字节长度有误或接收到的frame或者stop位有误0：接收数据时无奇偶校验错误 */
#define	U7816_ISR_FRERR_Msk	(0x1U << U7816_ISR_FRERR_Pos)

#define	U7816_ISR_OVERR_Pos	8	/* 接收溢出错误标志位。硬件置位，写1清零 (Receive Overflow Error,write 1 to clear)1：接收缓冲寄存器未被读出，又接收到新的数据，溢出错误标志有效。原接收缓冲寄存器内数据被新覆盖0：无溢出错误 */
#define	U7816_ISR_OVERR_Msk	(0x1U << U7816_ISR_OVERR_Pos)

#define	U7816_ISR_RXIF_Pos	2	/* 接收完成标志(Receive interrupt flag)，U7816接口控制器每收到1byte数据，根据接收的通道相应发出一次中断。硬件置位，读数据接收缓冲寄存器清零1：接收到1byte数据，数据接收缓冲器满0：未接收到数据，数据接收缓冲器空 */
#define	U7816_ISR_RXIF_Msk	(0x1U << U7816_ISR_RXIF_Pos)

#define	U7816_ISR_TXIF_Pos	1	/* 发送缓冲区空标志(Transmit interrupt flag)，上电复位后此标志就自动置位，表示缓冲区空，可以写入数据。软件写入数据后标志自动清除，数据从发送缓存移入移位寄存器后置11：数据发送缓冲器空0：数据发送缓冲器内有数据待发送 */
#define	U7816_ISR_TXIF_Msk	(0x1U << U7816_ISR_TXIF_Pos)

#define	U7816_ISR_ERRIF_Pos	0	/* 错误标志(Error interrupt flag)，寄存器配置出错或传输过程中出错。此bit是TPARERR、RPARERR、FRERR、OVERR的或。软件通过清除以上错误标志寄存器来清除此bit。 */
#define	U7816_ISR_ERRIF_Msk	(0x1U << U7816_ISR_ERRIF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void U7816_Deinit(void);

/* U7816通道发送使能控制位 (Transmit Enable)1：通道发送使能，可发送数据0：通道发送禁止，不可发送数据，并关断输出端口，将SCL信号转化为低电平 相关函数 */
extern void U7816_CR_TXEN_Setable(FunState NewState);
extern FunState U7816_CR_TXEN_Getable(void);

/* U7816通道接收使能控制位 (Receive Enable)1：通道接收使能，可接收数据0：通道接收禁止，不可接收数据，并关断输入端口 相关函数 */
extern void U7816_CR_RXEN_Setable(FunState NewState);
extern FunState U7816_CR_RXEN_Getable(void);

/* U7816时钟CLK输出使能控制位 (Clock output Enable)1：7816时钟输出使能0：7816时钟输出禁止 相关函数 */
extern void U7816_CR_CKOEN_Setable(FunState NewState);
extern FunState U7816_CR_CKOEN_Getable(void);

/* U7816通道数据发送强上拉电阻自动有效控制位 (High-Pullup Automatically)1：数据发送时上拉电阻自动有效，接收态上拉电阻无效0：数据发送时上拉电阻自动有效功能禁止，上拉电阻由HPUEN，LPUEN控制 相关函数 */
extern void U7816_CR_HPUAT_Setable(FunState NewState);
extern FunState U7816_CR_HPUAT_Getable(void);

/* U7816通道强上拉使能控制位 (High-Pullup Enable)1：强上拉有效0：强上拉无效 相关函数 */
extern void U7816_CR_HPUEN_Setable(FunState NewState);
extern FunState U7816_CR_HPUEN_Getable(void);

/* Guard Time发送长度控制位 (Send long Frame Enable)1：Guard time为3 etu0：Guard time为2 etu 相关函数 */
extern void U7816_FFR_SFREN_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_SFREN_Get(void);

/* ERROR SIGNAL宽度选择 (Error Signal Width)11：ERROR SIGNAL宽度为1ETU;10：ERROR SIGNAL宽度为1.5ETU;01：ERROR SIGNAL宽度为2ETU;00：ERROR SIGNAL宽度为2ETU; 相关函数 */
extern void U7816_FFR_ERSW_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_ERSW_Get(void);

/* ERROR SIGNAL后GUARDTIME宽度选择（仅在发送时有效）
(Error Signal Guard Time)1：ERROR SIGNAL后GUARDTIME为1~1.5ETU。0：ERROR SIGNAL后GUARDTIME为2~2.5ETU。ERROR SIGNAL宽度为整数ETU时GUARDTIME为1.5或2.5ETU；ERROR SIGNAL宽度为1.5ETU时GUARDTIME为1或2ETU 相关函数 */
extern void U7816_FFR_ERSGD_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_ERSGD_Get(void);

/* BGT控制位。控制接收->发送之间是否插入BGT。BGT是接收->发送之间需要的最小时间（block guard time enable）1：BGT使能，插入Block guard time(12 etu);0：BGT禁止，不插入Block guard time(12 etu); 相关函数 */
extern void U7816_FFR_BGTEN_Setable(FunState NewState);
extern FunState U7816_FFR_BGTEN_Getable(void);

/* 控制接收数据奇偶校验出错时自动重发次数 (Repeated Times)1：3次0：1次 相关函数 */
extern void U7816_FFR_REP_T_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_REP_T_Get(void);

/* 奇偶校验类型选择 (Parity)00：Even01：Odd10：Always 111：不校验，处理 相关函数 */
extern void U7816_FFR_PAR_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_PAR_Get(void);

/* Guard Time接收长度控制位 (Receive short Frame )1：Guard time为1 etu0：Guard time为2 etu 相关函数 */
extern void U7816_FFR_RFREN_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_RFREN_Get(void);

/* 发送数据奇偶校验错的处理方式选择 (Transmit Repeat Enable)1：收到奇偶校验出错标志（error signal），根据T＝0协议自动进行回发。在单一byte重复发送次数超过REP_T后，置tx_parity_err标志，进行中断0：收到Error signal时不进行自动回发，置tx_parity_err标志，直接中断 相关函数 */
extern void U7816_FFR_TREPEN_Setable(FunState NewState);
extern FunState U7816_FFR_TREPEN_Getable(void);

/* 接收数据奇偶校验错的处理方式选择 (Receive Repeat Enable)1：奇偶校验错，根据T=0协议自动回发ERROR SIGNAL。单一BYTE连续接收次数超过REP_T后，置RX_PARITY_ERR标志，进行中断0：奇偶校验错，不自动发送ERROR SIGNAL，置RX_PARITY_ERR标志，进行中断 相关函数 */
extern void U7816_FFR_RREPEN_Setable(FunState NewState);
extern FunState U7816_FFR_RREPEN_Getable(void);

/* 传输次序，编码方式选择 (bit Direction Conversion)1：反向编码，先收发MSB；(收发数据+校验位)反逻辑电平0：正向编码，先收发LSB ； (收发数据+校验位)正逻辑电平 相关函数 */
extern void U7816_FFR_DICONV_Set(uint32_t SetValue);
extern uint32_t U7816_FFR_DICONV_Get(void);

/* 发送时插入的Extra Guard Time时间（以ETU为单位）
(Transmit Extra Guard Time) 相关函数 */
extern void U7816_EGTR_Write(uint32_t SetValue);
extern uint32_t U7816_EGTR_Read(void);

/* U7816时钟输出分频控制寄存器(Clock Divider)，控制7816工作时钟分频数。U7816工作时钟与APBCLK的分频关系：F7816=FAPBCLK/(CLKDIV+1)特殊情况：CLK_DIV设置成0或1时，F7816=FAPBCLK/2注：7816协议规定的工作时钟范围是1~5MHZ。 相关函数 */
extern void U7816_PSC_Write(uint32_t SetValue);
extern uint32_t U7816_PSC_Read(void);

/* U7816预分频控制寄存器(Pre-Divider)，控制7816通信分频比（波特率）Baud ＝ F7816/(PDIV ＋ 1)

注意：PDIV最小可用值是0x1，应用禁止配置0x0 相关函数 */
extern void U7816_BGR_Write(uint32_t SetValue);
extern uint32_t U7816_BGR_Read(void);

/* U7816数据接收缓存寄存器 (Receive Buffer) 相关函数 */
extern uint32_t U7816_RXBUF_Read(void);

/* U7816数据发送缓存寄存器 (Transmit Buffer) 相关函数 */
extern void U7816_TXBUF_Write(uint32_t SetValue);

/* 数据接收中断使能位。对应RXIF中断标志位 (Receive Interrupt Enable)1：当RXIF寄存器置位时产生接收完成中断0：禁止接收完成中断 相关函数 */
extern void U7816_IER_RXIE_Setable(FunState NewState);
extern FunState U7816_IER_RXIE_Getable(void);

/* 数据发送中断使能位。对应TXIF中断标志位(Transmit Interrupt Enable)1：当TXIF寄存器置位时产生发送完成中断0：禁止发送完成中断 相关函数 */
extern void U7816_IER_TXIE_Setable(FunState NewState);
extern FunState U7816_IER_TXIE_Getable(void);

/* 线路状态中断使能位。对应ERRIF中断标志位(Line Status Interrupt Enable)1：当ERRIF寄存器置位时产生线路错误中断0：禁止线路错误中断 相关函数 */
extern void U7816_IER_LSIE_Setable(FunState NewState);
extern FunState U7816_IER_LSIE_Getable(void);

/* U7816接口发送了错误信号，正在等待对方重发数据；(Waiting for Repeat flag)状态机进入发送错误信号状态时置位，收到数据起始位或者进入发送状态时硬件清零；软件只读。 相关函数 */
extern FlagStatus U7816_ISR_WAIT_RPT_Chk(void);

/* 发送数据忙标志。（发送完成后自动清零）(Transmission busy flag)1：处于数据发送状态，发送移位寄存器正在发送数据。（开始发送起始位置1，停止位中间清零）0：数据发送空闲 相关函数 */
extern FlagStatus U7816_ISR_TXBUSY_Chk(void);

/* 接收数据忙标志。（接收完成后自动清零）(Receiving busy flag)1：处于数据接收状态，接收移位寄存器正在接收数据。（收到起始位置1，收到停止位清零，若接收数据出错需重发，则回发error signal时清零。即数据及校验位接收之后，无论是否需要重发，都需要及时清除该标志）0：数据接收空闲 相关函数 */
extern FlagStatus U7816_ISR_RXBUSY_Chk(void);

/* 发送数据奇偶校验错误标志位。硬件置位，写1清零 (Transmit Parity Error,write 1 to clear) 相关函数 */
extern void U7816_ISR_TPARERR_Clr(void);
extern FlagStatus U7816_ISR_TPARERR_Chk(void);

/* 接收数据奇偶校验错误标志位。硬件置位，写1清零 (Receive Parity Error flag,write 1 to clear) 相关函数 */
extern void U7816_ISR_RPARERR_Clr(void);
extern FlagStatus U7816_ISR_RPARERR_Chk(void);

/* 接收帧格式错误标志位。硬件置位，写1清零 (Frame Error flag,write 1 to clear)1：帧格式有错误，接收到的frame字节长度有误或接收到的frame或者stop位有误0：接收数据时无奇偶校验错误 相关函数 */
extern void U7816_ISR_FRERR_Clr(void);
extern FlagStatus U7816_ISR_FRERR_Chk(void);

/* 接收溢出错误标志位。硬件置位，写1清零 (Receive Overflow Error,write 1 to clear)1：接收缓冲寄存器未被读出，又接收到新的数据，溢出错误标志有效。原接收缓冲寄存器内数据被新覆盖0：无溢出错误 相关函数 */
extern void U7816_ISR_OVERR_Clr(void);
extern FlagStatus U7816_ISR_OVERR_Chk(void);

/* 接收完成标志(Receive interrupt flag)，U7816接口控制器每收到1byte数据，根据接收的通道相应发出一次中断。硬件置位，读数据接收缓冲寄存器清零1：接收到1byte数据，数据接收缓冲器满0：未接收到数据，数据接收缓冲器空 相关函数 */
extern FlagStatus U7816_ISR_RXIF_Chk(void);

/* 发送缓冲区空标志(Transmit interrupt flag)，上电复位后此标志就自动置位，表示缓冲区空，可以写入数据。软件写入数据后标志自动清除，数据从发送缓存移入移位寄存器后置11：数据发送缓冲器空0：数据发送缓冲器内有数据待发送 相关函数 */
extern FlagStatus U7816_ISR_TXIF_Chk(void);

/* 错误标志(Error interrupt flag)，寄存器配置出错或传输过程中出错。此bit是TPARERR、RPARERR、FRERR、OVERR的或。软件通过清除以上错误标志寄存器来清除此bit。 相关函数 */
extern FlagStatus U7816_ISR_ERRIF_Chk(void);

/* U7816完整参数初始化函数 */
void U7816_Init( U7816_InitTypeDef* para);



//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_U7816_H */
