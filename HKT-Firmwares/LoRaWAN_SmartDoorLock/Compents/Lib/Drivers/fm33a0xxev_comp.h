/**
  ******************************************************************************
  * @file    fm33a0xxev_comp.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the COMP firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_COMP_H
#define __FM33A0XXEV_COMP_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	COMP_CR1_DFLEN_Pos	19	/* 比较器1输出数字滤波长度配置寄存器。滤波长度周期是DFLEN+1
00010：3次APBCLK采样
……
11111：32次APBCLK采样 */
#define	COMP_CR1_DFLEN_Msk	(0x1fU << COMP_CR1_DFLEN_Pos)
#define	COMP_CR1_DFLEN_X3	(0x2U << COMP_CR1_DFLEN_Pos)	/* 3次APBCLK采样 */
#define	COMP_CR1_DFLEN_X32	(0x1fU << COMP_CR1_DFLEN_Pos)	/* 32次APBCLK采样 */

#define	COMP_CR1_WINMODE_Pos	18	/* 比较器1窗口模式控制寄存器
0：禁止窗口模式
1：使能窗口模式 */
#define	COMP_CR1_WINMODE_Msk	(0x1U << COMP_CR1_WINMODE_Pos)
	/* 禁止窗口模式 */
	/* 使能窗口模式 */

#define	COMP_CR1_POLAR_Pos	17	/* 比较器1输出极性控制
0：正向输出
1：取反输出 */
#define	COMP_CR1_POLAR_Msk	(0x1U << COMP_CR1_POLAR_Pos)
#define	COMP_CR1_POLAR_POS	(0x0U << COMP_CR1_POLAR_Pos)	/* 正向输出 */
#define	COMP_CR1_POLAR_NEG	(0x1U << COMP_CR1_POLAR_Pos)	/* 取反输出 */

#define	COMP_CR1_DFEN_Pos	16	/* 比较器1输出数字滤波使能
0：禁止输出数字滤波
1：使能输出数字滤波 */
#define	COMP_CR1_DFEN_Msk	(0x1U << COMP_CR1_DFEN_Pos)
	/* 禁止输出数字滤波 */
	/* 使能输出数字滤波 */

#define	COMP_CR1_CMP1OR_Pos	9	/* 比较器1输出（数字滤波前），软件只读 */
#define	COMP_CR1_CMP1OR_Msk	(0x1U << COMP_CR1_CMP1OR_Pos)

#define	COMP_CR1_CMP1OF_Pos	8	/* 比较器1输出（数字滤波后），软件只读 */
#define	COMP_CR1_CMP1OF_Msk	(0x1U << COMP_CR1_CMP1OF_Pos)

#define	COMP_CR1_V1NSEL_Pos	4	/* 比较器1负极输入选择
000：COMP1_INN1
001：COMP1_INN2
010：VREF
011：3/4 VREF
100：1/2 VREF
101：1/4 VREF
110：1/8 VREF
111：RFU */
#define	COMP_CR1_V1NSEL_Msk	(0x7U << COMP_CR1_V1NSEL_Pos)
#define	COMP_CR1_V1NSEL_COMP1_INN1	(0x0U << COMP_CR1_V1NSEL_Pos)	/* COMP1_INN1 */
#define	COMP_CR1_V1NSEL_COMP1_INN2	(0x1U << COMP_CR1_V1NSEL_Pos)	/* COMP1_INN2 */
#define	COMP_CR1_V1NSEL_VREF	(0x2U << COMP_CR1_V1NSEL_Pos)	/* VREF */
#define	COMP_CR1_V1NSEL_3_4_VREF	(0x3U << COMP_CR1_V1NSEL_Pos)	/* 3/4 VREF */
#define	COMP_CR1_V1NSEL_1_2_VREF	(0x4U << COMP_CR1_V1NSEL_Pos)	/* 1/2 VREF */
#define	COMP_CR1_V1NSEL_1_4_VREF	(0x5U << COMP_CR1_V1NSEL_Pos)	/* 1/4 VREF */
#define	COMP_CR1_V1NSEL_1_8_VREF	(0x6U << COMP_CR1_V1NSEL_Pos)	/* 1/8 VREF */

#define	COMP_CR1_V1PSEL_Pos	1	/* 比较器1正极输入选择
00：COMP1_INP1
01：COMP1_INP2
10：COMP1_INP3
11：COMP1_INP4 */
#define	COMP_CR1_V1PSEL_Msk	(0x3U << COMP_CR1_V1PSEL_Pos)
#define	COMP_CR1_V1PSEL_COMP1_INP1	(0x0U << COMP_CR1_V1PSEL_Pos)	/* COMP1_INP1 */
#define	COMP_CR1_V1PSEL_COMP1_INP2	(0x1U << COMP_CR1_V1PSEL_Pos)	/* COMP1_INP2 */
#define	COMP_CR1_V1PSEL_COMP1_INP3	(0x2U << COMP_CR1_V1PSEL_Pos)	/* COMP1_INP3 */
#define	COMP_CR1_V1PSEL_COMP1_INP4	(0x3U << COMP_CR1_V1PSEL_Pos)	/* COMP1_INP4 */

#define	COMP_CR1_CMP1EN_Pos	0	/* 比较器1使能位
0：关闭比较器1
1：使能比较器1 */
#define	COMP_CR1_CMP1EN_Msk	(0x1U << COMP_CR1_CMP1EN_Pos)
	/* 关闭比较器1 */
	/* 使能比较器1 */

#define	COMP_CR2_DFLEN_Pos	19	/* 比较器1输出数字滤波长度配置寄存器。滤波长度周期是DFLEN+1
00010：3次APBCLK采样
……
11111：32次APBCLK采样 */
#define	COMP_CR2_DFLEN_Msk	(0x1fU << COMP_CR2_DFLEN_Pos)
#define	COMP_CR2_DFLEN_X3	(0x2U << COMP_CR2_DFLEN_Pos)	/* 3次APBCLK采样 */
#define	COMP_CR2_DFLEN_X32	(0x1fU << COMP_CR2_DFLEN_Pos)	/* 32次APBCLK采样 */

#define	COMP_CR2_WINMODE_Pos	18	/* 比较器2窗口模式控制寄存器
0：禁止窗口模式
1：使能窗口模式 */
#define	COMP_CR2_WINMODE_Msk	(0x1U << COMP_CR2_WINMODE_Pos)
	/* 禁止窗口模式 */
	/* 使能窗口模式 */

#define	COMP_CR2_POLAR_Pos	17	/* 比较器2输出极性控制
0：正向输出
1：取反输出 */
#define	COMP_CR2_POLAR_Msk	(0x1U << COMP_CR2_POLAR_Pos)
#define	COMP_CR2_POLAR_POS	(0x0U << COMP_CR2_POLAR_Pos)	/* 正向输出 */
#define	COMP_CR2_POLAR_NEG	(0x1U << COMP_CR2_POLAR_Pos)	/* 取反输出 */

#define	COMP_CR2_DFEN_Pos	16	/* 比较器2输出数字滤波使能
0：禁止输出数字滤波
1：使能输出数字滤波 */
#define	COMP_CR2_DFEN_Msk	(0x1U << COMP_CR2_DFEN_Pos)
	/* 禁止输出数字滤波 */
	/* 使能输出数字滤波 */

#define	COMP_CR2_CMP2OR_Pos	9	/* 比较器2输出（数字滤波前），软件只读 */
#define	COMP_CR2_CMP2OR_Msk	(0x1U << COMP_CR2_CMP2OR_Pos)

#define	COMP_CR2_CMP2OF_Pos	8	/* 比较器2输出（数字滤波后），软件只读 */
#define	COMP_CR2_CMP2OF_Msk	(0x1U << COMP_CR2_CMP2OF_Pos)

#define	COMP_CR2_V2NSEL_Pos	4	/* 比较器2负极输入选择
000：COMP2_INN1
001：COMP2_INN2
010：VREF
011：3/4 VREF
100：1/2 VREF
101：1/4 VREF
110：1/8 VREF
111：RFU */
#define	COMP_CR2_V2NSEL_Msk	(0x7U << COMP_CR2_V2NSEL_Pos)
#define	COMP_CR2_V2NSEL_COMP2_INN1	(0x0U << COMP_CR2_V2NSEL_Pos)	/* COMP2_INN1 */
#define	COMP_CR2_V2NSEL_COMP2_INN2	(0x1U << COMP_CR2_V2NSEL_Pos)	/* COMP2_INN2 */
#define	COMP_CR2_V2NSEL_VREF	(0x2U << COMP_CR2_V2NSEL_Pos)	/* VREF */
#define	COMP_CR2_V2NSEL_3_4_VREF	(0x3U << COMP_CR2_V2NSEL_Pos)	/* 3/4 VREF */
#define	COMP_CR2_V2NSEL_1_2_VREF	(0x4U << COMP_CR2_V2NSEL_Pos)	/* 1/2 VREF */
#define	COMP_CR2_V2NSEL_1_4_VREF	(0x5U << COMP_CR2_V2NSEL_Pos)	/* 1/4 VREF */
#define	COMP_CR2_V2NSEL_1_8_VREF	(0x6U << COMP_CR2_V2NSEL_Pos)	/* 1/8 VREF */

#define	COMP_CR2_V2PSEL_Pos	1	/* 比较器2正极输入选择
000：COMP2_INP1
001：COMP2_INP2
010：COMP1_INP1
011：COMP1_INP2
100：AVREF
101：VDD15
110, 111：RFU */
#define	COMP_CR2_V2PSEL_Msk	(0x7U << COMP_CR2_V2PSEL_Pos)
#define	COMP_CR2_V2PSEL_COMP2_INP1	(0x0U << COMP_CR2_V2PSEL_Pos)	/* COMP2_INP1 */
#define	COMP_CR2_V2PSEL_COMP2_INP2	(0x1U << COMP_CR2_V2PSEL_Pos)	/* COMP2_INP2 */
#define	COMP_CR2_V2PSEL_COMP1_INP1	(0x2U << COMP_CR2_V2PSEL_Pos)	/* COMP1_INP1 */
#define	COMP_CR2_V2PSEL_COMP1_INP2	(0x3U << COMP_CR2_V2PSEL_Pos)	/* COMP1_INP2 */
#define	COMP_CR2_V2PSEL_AVREF	(0x4U << COMP_CR2_V2PSEL_Pos)	/* AVREF */
#define	COMP_CR2_V2PSEL_VDD15	(0x5U << COMP_CR2_V2PSEL_Pos)	/* VDD15 */
#define	COMP_CR2_V2PSEL_RFU		(0x7U << COMP_CR2_V2PSEL_Pos)	/* RFU */

#define	COMP_CR2_CMP2EN_Pos	0	/* 比较器2使能位
0：关闭比较器2
1：使能比较器2 */
#define	COMP_CR2_CMP2EN_Msk	(0x1U << COMP_CR2_CMP2EN_Pos)
	/* 关闭比较器2 */
	/* 使能比较器2 */

#define	COMP_ICR_OOW_IE_Pos	9	/* Out-Of-Window中断使能 */
#define	COMP_ICR_OOW_IE_Msk	(0x1U << COMP_ICR_OOW_IE_Pos)

#define	COMP_ICR_WIN_IE_Pos	8	/* Window中断使能 */
#define	COMP_ICR_WIN_IE_Msk	(0x1U << COMP_ICR_WIN_IE_Pos)

#define	COMP_ICR_BUFBYP_Pos	7	/* 比较器Buffer Bypass
0：不Bypass比较器Buffer
1：Bypass比较器Buffer */
#define	COMP_ICR_BUFBYP_Msk	(0x1U << COMP_ICR_BUFBYP_Pos)

#define	COMP_ICR_BUF_OFF_Pos	6	/* 比较器Buffer使能
0：使能比较器Buffer
1：禁止比较器Buffer */
#define	COMP_ICR_BUF_OFF_Msk	(0x1U << COMP_ICR_BUF_OFF_Pos)

#define	COMP_ICR_CMP2SEL_Pos	4	/* 比较器2中断源选择
00/11：比较器2输出上升或下降沿产生中断
01：比较器2输出上升沿产生中断
10：比较器2输出下降沿产生中断 */
#define	COMP_ICR_CMP2SEL_Msk	(0x3U << COMP_ICR_CMP2SEL_Pos)
#define	COMP_ICR_CMP2SEL_BOTH	(0x0U << COMP_ICR_CMP2SEL_Pos)
#define	COMP_ICR_CMP2SEL_RISING	(0x1U << COMP_ICR_CMP2SEL_Pos)
#define	COMP_ICR_CMP2SEL_FALLING	(0x2U << COMP_ICR_CMP2SEL_Pos)

#define	COMP_ICR_CMP1SEL_Pos	2	/* 比较器1中断源选择
00/11：比较器1输出上升或下降沿产生中断
01：比较器1输出上升沿产生中断
10：比较器1输出下降沿产生中断 */
#define	COMP_ICR_CMP1SEL_Msk	(0x3U << COMP_ICR_CMP1SEL_Pos)
#define	COMP_ICR_CMP1SEL_BOTH	(0x0U << COMP_ICR_CMP1SEL_Pos)
#define	COMP_ICR_CMP1SEL_RISING	(0x1U << COMP_ICR_CMP1SEL_Pos)
#define	COMP_ICR_CMP1SEL_FALLING	(0x2U << COMP_ICR_CMP1SEL_Pos)

#define	COMP_ICR_CMP2IE_Pos	1	/* 比较器2中断使能 */
#define	COMP_ICR_CMP2IE_Msk	(0x1U << COMP_ICR_CMP2IE_Pos)

#define	COMP_ICR_CMP1IE_Pos	0	/* 比较器1中断使能 */
#define	COMP_ICR_CMP1IE_Msk	(0x1U << COMP_ICR_CMP1IE_Pos)

#define	COMP_ISR_OOW_IF_Pos	3	/* Out-of-Window中断标志，硬件置位，软件写1清零 */
#define	COMP_ISR_OOW_IF_Msk	(0x1U << COMP_ISR_OOW_IF_Pos)

#define	COMP_ISR_WIN_IF_Pos	2	/* Window中断标志，硬件置位，软件写1清零 */
#define	COMP_ISR_WIN_IF_Msk	(0x1U << COMP_ISR_WIN_IF_Pos)

#define	COMP_ISR_CMP2IF_Pos	1	/* 比较器2中断标志，硬件置位，软件写1清零 */
#define	COMP_ISR_CMP2IF_Msk	(0x1U << COMP_ISR_CMP2IF_Pos)

#define	COMP_ISR_CMP1IF_Pos	0	/* 比较器1中断标志，硬件置位，软件写1清零 */
#define	COMP_ISR_CMP1IF_Msk	(0x1U << COMP_ISR_CMP1IF_Pos)

#define	COMP_PCR_HSCMPBUF_EN_Pos	2	/* 比较器BUFFER工作模式
0：低功耗模式
1：快速模式 */
#define	COMP_PCR_HSCMPBUF_EN_Msk	(0x1U << COMP_PCR_HSCMPBUF_EN_Pos)
#define	COMP_PCR_HSCMPBUF_EN_SLOW	(0x0U << COMP_PCR_HSCMPBUF_EN_Pos)	/* 低功耗模式 */
#define	COMP_PCR_HSCMPBUF_EN_FAST	(0x1U << COMP_PCR_HSCMPBUF_EN_Pos)	/* 快速模式 */

#define	COMP_PCR_HSCMP2_EN_Pos	1	/* 比较器2工作模式
0：低功耗模式
1：快速模式 */
#define	COMP_PCR_HSCMP2_EN_Msk	(0x1U << COMP_PCR_HSCMP2_EN_Pos)
#define	COMP_PCR_HSCMP2_EN_SLOW	(0x0U << COMP_PCR_HSCMP2_EN_Pos)	/* 低功耗模式 */
#define	COMP_PCR_HSCMP2_EN_FAST	(0x1U << COMP_PCR_HSCMP2_EN_Pos)	/* 快速模式 */

#define	COMP_PCR_HSCMP1_EN_Pos	0	/* 比较器1工作模式
0：低功耗模式
1：快速模式 */
#define	COMP_PCR_HSCMP1_EN_Msk	(0x1U << COMP_PCR_HSCMP1_EN_Pos)
#define	COMP_PCR_HSCMP1_EN_SLOW	(0x0U << COMP_PCR_HSCMP1_EN_Pos)	/* 低功耗模式 */
#define	COMP_PCR_HSCMP1_EN_FAST	(0x1U << COMP_PCR_HSCMP1_EN_Pos)	/* 快速模式*/

//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void COMP_Deinit(void);

/* 比较器1输出数字滤波长度配置寄存器。滤波长度周期是DFLEN+1
00010：3次APBCLK采样
……
11111：32次APBCLK采样 相关函数 */
extern void COMP_CR1_DFLEN_Set(uint32_t SetValue);
extern uint32_t COMP_CR1_DFLEN_Get(void);

/* 比较器1窗口模式控制寄存器
0：禁止窗口模式
1：使能窗口模式 相关函数 */
extern void COMP_CR1_WINMODE_Setable(FunState NewState);
extern FunState COMP_CR1_WINMODE_Getable(void);

/* 比较器1输出极性控制
0：正向输出
1：取反输出 相关函数 */
extern void COMP_CR1_POLAR_Set(uint32_t SetValue);
extern uint32_t COMP_CR1_POLAR_Get(void);

/* 比较器1输出数字滤波使能
0：禁止输出数字滤波
1：使能输出数字滤波 相关函数 */
extern void COMP_CR1_DFEN_Setable(FunState NewState);
extern FunState COMP_CR1_DFEN_Getable(void);

/* 比较器1输出（数字滤波前），软件只读 相关函数 */
extern FlagStatus COMP_CR1_CMP1OR_Chk(void);

/* 比较器1输出（数字滤波后），软件只读 相关函数 */
extern FlagStatus COMP_CR1_CMP1OF_Chk(void);

/* 比较器1负极输入选择
000：COMP1_INN1
001：COMP1_INN2
010：VREF
011：3/4 VREF
100：1/2 VREF
101：1/4 VREF
110：1/8 VREF
111：RFU 相关函数 */
extern void COMP_CR1_V1NSEL_Set(uint32_t SetValue);
extern uint32_t COMP_CR1_V1NSEL_Get(void);

/* 比较器1正极输入选择
00：COMP1_INP1
01：COMP1_INP2
10：COMP1_INP3
11：COMP1_INP4 相关函数 */
extern void COMP_CR1_V1PSEL_Set(uint32_t SetValue);
extern uint32_t COMP_CR1_V1PSEL_Get(void);

/* 比较器1使能位
0：关闭比较器1
1：使能比较器1 相关函数 */
extern void COMP_CR1_CMP1EN_Setable(FunState NewState);
extern FunState COMP_CR1_CMP1EN_Getable(void);

/* 比较器2输出数字滤波长度配置寄存器。滤波长度周期是DFLEN+1
00010：3次APBCLK采样
……
11111：32次APBCLK采样 相关函数 */
extern void COMP_CR2_DFLEN_Set(uint32_t SetValue);
extern uint32_t COMP_CR2_DFLEN_Get(void);

/* 比较器2窗口模式控制寄存器
0：禁止窗口模式
1：使能窗口模式 相关函数 */
extern void COMP_CR2_WINMODE_Setable(FunState NewState);
extern FunState COMP_CR2_WINMODE_Getable(void);

/* 比较器2输出极性控制
0：正向输出
1：取反输出 相关函数 */
extern void COMP_CR2_POLAR_Set(uint32_t SetValue);
extern uint32_t COMP_CR2_POLAR_Get(void);

/* 比较器2输出数字滤波使能
0：禁止输出数字滤波
1：使能输出数字滤波 相关函数 */
extern void COMP_CR2_DFEN_Setable(FunState NewState);
extern FunState COMP_CR2_DFEN_Getable(void);

/* 比较器2输出（数字滤波前），软件只读 相关函数 */
extern FlagStatus COMP_CR2_CMP2OR_Chk(void);

/* 比较器2输出（数字滤波后），软件只读 相关函数 */
extern FlagStatus COMP_CR2_CMP2OF_Chk(void);

/* 比较器2负极输入选择
000：COMP2_INN1
001：COMP2_INN2
010：VREF
011：3/4 VREF
100：1/2 VREF
101：1/4 VREF
110：1/8 VREF
111：RFU */
extern void COMP_CR2_V2NSEL_Set(uint32_t SetValue);
extern uint32_t COMP_CR2_V2NSEL_Get(void);

/* 比较器2正极输入选择
000：COMP2_INP1
001：COMP2_INP2
010：COMP1_INP1
011：COMP1_INP2
100：AVREF
101：VDD15
110, 111：RFU 相关函数 */
extern void COMP_CR2_V2PSEL_Set(uint32_t SetValue);
extern uint32_t COMP_CR2_V2PSEL_Get(void);

/* 比较器2使能位
0：关闭比较器2
1：使能比较器2 相关函数 */
extern void COMP_CR2_CMP2EN_Setable(FunState NewState);
extern FunState COMP_CR2_CMP2EN_Getable(void);

/* Out-Of-Window中断使能 相关函数 */
extern void COMP_ICR_OOW_IE_Setable(FunState NewState);
extern FunState COMP_ICR_OOW_IE_Getable(void);

/* Window中断使能 相关函数 */
extern void COMP_ICR_WIN_IE_Setable(FunState NewState);
extern FunState COMP_ICR_WIN_IE_Getable(void);

/* 比较器Buffer Bypass
0：不Bypass比较器Buffer
1：Bypass比较器Buffer 相关函数 */
extern void COMP_ICR_BUFBYP_Setable(FunState NewState);
extern FunState COMP_ICR_BUFBYP_Getable(void);

/* 比较器Buffer使能
0：使能比较器Buffer
1：禁止比较器Buffer 相关函数 */
extern void COMP_ICR_BUF_OFF_Setable(FunState NewState);
extern FunState COMP_ICR_BUF_OFF_Getable(void);

/* 比较器2中断源选择
00/11：比较器2输出上升或下降沿产生中断
01：比较器2输出上升沿产生中断
10：比较器2输出下降沿产生中断 相关函数 */
extern void COMP_ICR_CMP2SEL_Set(uint32_t SetValue);
extern uint32_t COMP_ICR_CMP2SEL_Get(void);

/* 比较器1中断源选择
00/11：比较器1输出上升或下降沿产生中断
01：比较器1输出上升沿产生中断
10：比较器1输出下降沿产生中断 相关函数 */
extern void COMP_ICR_CMP1SEL_Set(uint32_t SetValue);
extern uint32_t COMP_ICR_CMP1SEL_Get(void);

/* 比较器2中断使能 相关函数 */
extern void COMP_ICR_CMP2IE_Setable(FunState NewState);
extern FunState COMP_ICR_CMP2IE_Getable(void);

/* 比较器1中断使能 相关函数 */
extern void COMP_ICR_CMP1IE_Setable(FunState NewState);
extern FunState COMP_ICR_CMP1IE_Getable(void);

/* Out-of-Window中断标志，硬件置位，软件写1清零 相关函数 */
extern void COMP_ISR_OOW_IF_Clr(void);
extern FlagStatus COMP_ISR_OOW_IF_Chk(void);

/* Window中断标志，硬件置位，软件写1清零 相关函数 */
extern void COMP_ISR_WIN_IF_Clr(void);
extern FlagStatus COMP_ISR_WIN_IF_Chk(void);

/* 比较器2中断标志，硬件置位，软件写1清零 相关函数 */
extern void COMP_ISR_CMP2IF_Clr(void);
extern FlagStatus COMP_ISR_CMP2IF_Chk(void);

/* 比较器1中断标志，硬件置位，软件写1清零 相关函数 */
extern void COMP_ISR_CMP1IF_Clr(void);
extern FlagStatus COMP_ISR_CMP1IF_Chk(void);

/* 比较器BUFFER工作模式
0：低功耗模式
1：快速模式 相关函数 */
extern void COMP_PCR_HSCMPBUF_EN_Set(uint32_t SetValue);
extern uint32_t COMP_PCR_HSCMPBUF_EN_Get(void);

/* 比较器2工作模式
0：低功耗模式
1：快速模式 相关函数 */
extern void COMP_PCR_HSCMP2_EN_Set(uint32_t SetValue);
extern uint32_t COMP_PCR_HSCMP2_EN_Get(void);

/* 比较器1工作模式
0：低功耗模式
1：快速模式 相关函数 */
extern void COMP_PCR_HSCMP1_EN_Set(uint32_t SetValue);
extern uint32_t COMP_PCR_HSCMP1_EN_Get(void);


//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_COMP_H */
