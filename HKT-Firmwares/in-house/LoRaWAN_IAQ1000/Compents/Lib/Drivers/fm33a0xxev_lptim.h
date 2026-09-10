/**
  ******************************************************************************
  * @file    fm33a0xxev_lptim.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the LPTIM firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_LPTIM_H
#define __FM33A0XXEV_LPTIM_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h"
   
#define	LPTIM_CFGR_ETR_AFEN_Pos	24	/* LPT_ETR输入模拟滤波使能(External Trigger input Analog Filter Enable)
0：关闭模拟滤波
1：使能模拟滤波，滤波宽度约100ns */
#define	LPTIM_CFGR_ETR_AFEN_Msk	(0x1U << LPTIM_CFGR_ETR_AFEN_Pos)
	/* 关闭模拟滤波 */
	/* 使能模拟滤波，滤波宽度约100ns */

#define	LPTIM_CFGR_PSCSEL_Pos	14	/* 时钟预分频输入选择(Prescaler input Select)
0：CLKSEL选择的时钟
1：LPTETRF */
#define	LPTIM_CFGR_PSCSEL_Msk	(0x1U << LPTIM_CFGR_PSCSEL_Pos)
#define	LPTIM_CFGR_PSCSEL_CLKSEL	(0x0U << LPTIM_CFGR_PSCSEL_Pos)	/* CLKSEL选择的时钟 */
#define	LPTIM_CFGR_PSCSEL_LPTETRF	(0x1U << LPTIM_CFGR_PSCSEL_Pos)	/* LPTETRF */

#define	LPTIM_CFGR_DIVSEL_Pos	10	/* 计数时钟分频选择(Counter Clock Divider Select)
000：1分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110：64分频
111：128分频 */
#define	LPTIM_CFGR_DIVSEL_Msk	(0x7U << LPTIM_CFGR_DIVSEL_Pos)
#define	LPTIM_CFGR_DIVSEL_DIV1	(0x0U << LPTIM_CFGR_DIVSEL_Pos)	/* 1分频 */
#define	LPTIM_CFGR_DIVSEL_DIV2	(0x1U << LPTIM_CFGR_DIVSEL_Pos)	/* 2分频 */
#define	LPTIM_CFGR_DIVSEL_DIV4	(0x2U << LPTIM_CFGR_DIVSEL_Pos)	/* 4分频 */
#define	LPTIM_CFGR_DIVSEL_DIV8	(0x3U << LPTIM_CFGR_DIVSEL_Pos)	/* 8分频 */
#define	LPTIM_CFGR_DIVSEL_DIV16	(0x4U << LPTIM_CFGR_DIVSEL_Pos)	/* 16分频 */
#define	LPTIM_CFGR_DIVSEL_DIV32	(0x5U << LPTIM_CFGR_DIVSEL_Pos)	/* 32分频 */
#define	LPTIM_CFGR_DIVSEL_DIV64	(0x6U << LPTIM_CFGR_DIVSEL_Pos)	/* 64分频 */
#define	LPTIM_CFGR_DIVSEL_DIV128	(0x7U << LPTIM_CFGR_DIVSEL_Pos)	/* 128分频 */

#define	LPTIM_CFGR_EDGESEL_Pos	7	/* ETR输入边沿选择(ETR Clock Edge Select)
0：LPT_ETR的上升沿计数
1：LPT_ETR的下降沿计数 */
#define	LPTIM_CFGR_EDGESEL_Msk	(0x1U << LPTIM_CFGR_EDGESEL_Pos)
#define	LPTIM_CFGR_EDGESEL_RISING	(0x0U << LPTIM_CFGR_EDGESEL_Pos)	/* LPT_ETR的上升沿计数 */
#define	LPTIM_CFGR_EDGESEL_FALLING	(0x1U << LPTIM_CFGR_EDGESEL_Pos)	/* LPT_ETR的下降沿计数 */

#define	LPTIM_CFGR_TRIGCFG_Pos	5	/* 外部触发边沿选择（需使用内部时钟同步采样LPT_ETR）(ETR trigger Configuration)
00: LPT_ETR输入信号上升沿触发
01: LPT_ETR输入信号下降沿触发
10/11: 外部输入信号上升下降沿触发 */
#define	LPTIM_CFGR_TRIGCFG_Msk	(0x3U << LPTIM_CFGR_TRIGCFG_Pos)
#define	LPTIM_CFGR_TRIGCFG_RISING	(0x0U << LPTIM_CFGR_TRIGCFG_Pos)
#define	LPTIM_CFGR_TRIGCFG_FALLING	(0x1U << LPTIM_CFGR_TRIGCFG_Pos)
#define	LPTIM_CFGR_TRIGCFG_EXTERNAL	(0x2U << LPTIM_CFGR_TRIGCFG_Pos)

#define	LPTIM_CFGR_ONST_Pos	2	/* 单次计数模式使能(One State Timer)
0：连续计数模式：计数器被触发后保持运行，直到被关闭为止。计数器达到目标值后回到0重新开始计数，并产生溢出中断。
1：单次计数模式：计数器被触发后计数到目标值后回到0，并自动停止，产生溢出中断。 */
#define	LPTIM_CFGR_ONST_Msk	(0x1U << LPTIM_CFGR_ONST_Pos)
#define	LPTIM_CFGR_ONST_CONTINUE	(0x0U << LPTIM_CFGR_ONST_Pos)	/* 连续计数模式 */
#define	LPTIM_CFGR_ONST_SINGLE	(0x1U << LPTIM_CFGR_ONST_Pos)	/* 单次计数模式 */

#define	LPTIM_CFGR_TMODE_Pos	0	/* 工作模式选择(Timer operation Mode)
00：普通定时器模式
01：Trigger脉冲触发计数模式
10：外部异步脉冲计数模式
11：Timeout模式 */
#define	LPTIM_CFGR_TMODE_Msk	(0x3U << LPTIM_CFGR_TMODE_Pos)
#define	LPTIM_CFGR_TMODE_COUNTER	(0x0U << LPTIM_CFGR_TMODE_Pos)
#define	LPTIM_CFGR_TMODE_PULSE	(0x1U << LPTIM_CFGR_TMODE_Pos)
#define	LPTIM_CFGR_TMODE_ASY	(0x2U << LPTIM_CFGR_TMODE_Pos)
#define	LPTIM_CFGR_TMODE_TIMEOUT	(0x3U << LPTIM_CFGR_TMODE_Pos)

#define	LPTIM_CNTR_CNT32_Pos	0	/* 32bit计数器当前计数值(Counter 32bits-wide) */
#define	LPTIM_CNTR_CNT32_Msk	(0xffffffffU << LPTIM_CNTR_CNT32_Pos)

#define	LPTIM_CCSR_CAP1SSEL_Pos	24	/* 通道1捕捉源选择(Capture channel 1 source select)，仅在CH1通道配置为输入捕捉时有效
00：LPT_CH1输入
01：XTLF
10：RCLP
11：RCMF */
#define	LPTIM_CCSR_CAP1SSEL_Msk	(0x3U << LPTIM_CCSR_CAP1SSEL_Pos)
#define	LPTIM_CCSR_CAP1SSEL_LPT_CH1	(0x0U << LPTIM_CCSR_CAP1SSEL_Pos)
#define	LPTIM_CCSR_CAP1SSEL_XTLF	(0x1U << LPTIM_CCSR_CAP1SSEL_Pos)
#define	LPTIM_CCSR_CAP1SSEL_RCLP	(0x2U << LPTIM_CCSR_CAP1SSEL_Pos)
#define	LPTIM_CCSR_CAP1SSEL_RCMF	(0x3U << LPTIM_CCSR_CAP1SSEL_Pos)

#define	LPTIM_CCSR_CAP4EDGE_Pos	23	/* 通道4当前被捕捉的边沿，在CC2IF置位时更新(Channel 4 Captured Edge)
0：上升沿
1：下降沿 */
#define	LPTIM_CCSR_CAP4EDGE_Msk	(0x1U << LPTIM_CCSR_CAP4EDGE_Pos)
#define	LPTIM_CCSR_CAP4EDGE_RISING	(0x0U << LPTIM_CCSR_CAP4EDGE_Pos)
#define	LPTIM_CCSR_CAP4EDGE_FALLING	(0x1U << LPTIM_CCSR_CAP4EDGE_Pos)

#define	LPTIM_CCSR_CAP3EDGE_Pos	22
#define	LPTIM_CCSR_CAP3EDGE_Msk	(0x1U << LPTIM_CCSR_CAP3EDGE_Pos)
#define	LPTIM_CCSR_CAP3EDGE_RISING	(0x0U << LPTIM_CCSR_CAP3EDGE_Pos)
#define	LPTIM_CCSR_CAP3EDGE_FALLING	(0x1U << LPTIM_CCSR_CAP3EDGE_Pos)

#define	LPTIM_CCSR_CAP2EDGE_Pos	21
#define	LPTIM_CCSR_CAP2EDGE_Msk	(0x1U << LPTIM_CCSR_CAP2EDGE_Pos)
#define	LPTIM_CCSR_CAP2EDGE_RISING	(0x0U << LPTIM_CCSR_CAP2EDGE_Pos)
#define	LPTIM_CCSR_CAP2EDGE_FALLING	(0x1U << LPTIM_CCSR_CAP2EDGE_Pos)

#define	LPTIM_CCSR_CAP1EDGE_Pos	20
#define	LPTIM_CCSR_CAP1EDGE_Msk	(0x1U << LPTIM_CCSR_CAP1EDGE_Pos)
#define	LPTIM_CCSR_CAP1EDGE_RISING	(0x0U << LPTIM_CCSR_CAP1EDGE_Pos)
#define	LPTIM_CCSR_CAP1EDGE_FALLING	(0x1U << LPTIM_CCSR_CAP1EDGE_Pos)

#define	LPTIM_CCSR_POLAR4_Pos	19	/* 通道4比较输出波形极性选择 (Channel 4 compare output Polarity)
0：正极性波形，起始为低，计数值==比较值时置高，计数值==ARR时恢复为低
1：负极性波形，正极性波形取反 */
#define	LPTIM_CCSR_POLAR4_Msk	(0x1U << LPTIM_CCSR_POLAR4_Pos)
#define	LPTIM_CCSR_POLAR4_POS	(0x0U << LPTIM_CCSR_POLAR4_Pos)
#define	LPTIM_CCSR_POLAR4_NEG	(0x1U << LPTIM_CCSR_POLAR4_Pos)

#define	LPTIM_CCSR_POLAR3_Pos	18
#define	LPTIM_CCSR_POLAR3_Msk	(0x1U << LPTIM_CCSR_POLAR3_Pos)
#define	LPTIM_CCSR_POLAR3_POS	(0x0U << LPTIM_CCSR_POLAR3_Pos)
#define	LPTIM_CCSR_POLAR3_NEG	(0x1U << LPTIM_CCSR_POLAR3_Pos)

#define	LPTIM_CCSR_POLAR2_Pos	17
#define	LPTIM_CCSR_POLAR2_Msk	(0x1U << LPTIM_CCSR_POLAR2_Pos)
#define	LPTIM_CCSR_POLAR2_POS	(0x0U << LPTIM_CCSR_POLAR2_Pos)
#define	LPTIM_CCSR_POLAR2_NEG	(0x1U << LPTIM_CCSR_POLAR2_Pos)

#define	LPTIM_CCSR_POLAR1_Pos	16
#define	LPTIM_CCSR_POLAR1_Msk	(0x1U << LPTIM_CCSR_POLAR1_Pos)
#define	LPTIM_CCSR_POLAR1_POS	(0x0U << LPTIM_CCSR_POLAR1_Pos)
#define	LPTIM_CCSR_POLAR1_NEG	(0x1U << LPTIM_CCSR_POLAR1_Pos)

#define	LPTIM_CCSR_CAPCFG4_Pos	14	/* 通道4捕捉边沿选择(Channel 4 Capture edge Config)
00：上升沿捕捉
01：下降沿捕捉
10：上升下降沿捕捉
11：RFU */
#define	LPTIM_CCSR_CAPCFG4_Msk	(0x3U << LPTIM_CCSR_CAPCFG4_Pos)
#define	LPTIM_CCSR_CAPCFG4_RISING	(0x0U << LPTIM_CCSR_CAPCFG4_Pos)
#define	LPTIM_CCSR_CAPCFG4_FALLING	(0x1U << LPTIM_CCSR_CAPCFG4_Pos)
#define	LPTIM_CCSR_CAPCFG4_BOTH	(0x2U << LPTIM_CCSR_CAPCFG4_Pos)

#define	LPTIM_CCSR_CAPCFG3_Pos	12
#define	LPTIM_CCSR_CAPCFG3_Msk	(0x3U << LPTIM_CCSR_CAPCFG3_Pos)
#define	LPTIM_CCSR_CAPCFG3_RISING	(0x0U << LPTIM_CCSR_CAPCFG3_Pos)
#define	LPTIM_CCSR_CAPCFG3_FALLING	(0x1U << LPTIM_CCSR_CAPCFG3_Pos)
#define	LPTIM_CCSR_CAPCFG3_BOTH	(0x2U << LPTIM_CCSR_CAPCFG3_Pos)

#define	LPTIM_CCSR_CAPCFG2_Pos	10
#define	LPTIM_CCSR_CAPCFG2_Msk	(0x3U << LPTIM_CCSR_CAPCFG2_Pos)
#define	LPTIM_CCSR_CAPCFG2_RISING	(0x0U << LPTIM_CCSR_CAPCFG2_Pos)
#define	LPTIM_CCSR_CAPCFG2_FALLING	(0x1U << LPTIM_CCSR_CAPCFG2_Pos)
#define	LPTIM_CCSR_CAPCFG2_BOTH	(0x2U << LPTIM_CCSR_CAPCFG2_Pos)

#define	LPTIM_CCSR_CAPCFG1_Pos	8
#define	LPTIM_CCSR_CAPCFG1_Msk	(0x3U << LPTIM_CCSR_CAPCFG1_Pos)
#define	LPTIM_CCSR_CAPCFG1_RISING	(0x0U << LPTIM_CCSR_CAPCFG1_Pos)
#define	LPTIM_CCSR_CAPCFG1_FALLING	(0x1U << LPTIM_CCSR_CAPCFG1_Pos)
#define	LPTIM_CCSR_CAPCFG1_BOTH	(0x2U << LPTIM_CCSR_CAPCFG1_Pos)

#define	LPTIM_CCSR_CC4S_Pos	6	/* 通道4捕捉/比较功能使能(Channel 4 Capture/Compare Select)
00,11：禁止通道4捕捉/比较功能
01：使能通道4捕捉功能（LPT_CH4为输入）
10：使能通道4比较功能（LPT_CH4为输出） */
#define	LPTIM_CCSR_CC4S_Msk	(0x3U << LPTIM_CCSR_CC4S_Pos)
#define	LPTIM_CCSR_CC4S_NON	(0x0U << LPTIM_CCSR_CC4S_Pos)
#define	LPTIM_CCSR_CC4S_LPT_CH4_IN	(0x1U << LPTIM_CCSR_CC4S_Pos)
#define	LPTIM_CCSR_CC4S_LPT_CH4_OUT	(0x2U << LPTIM_CCSR_CC4S_Pos)

#define	LPTIM_CCSR_CC3S_Pos	4
#define	LPTIM_CCSR_CC3S_Msk	(0x3U << LPTIM_CCSR_CC3S_Pos)
#define	LPTIM_CCSR_CC3S_NON	(0x0U << LPTIM_CCSR_CC3S_Pos)
#define	LPTIM_CCSR_CC3S_LPT_CH3_IN	(0x1U << LPTIM_CCSR_CC3S_Pos)
#define	LPTIM_CCSR_CC3S_LPT_CH3_OUT	(0x2U << LPTIM_CCSR_CC3S_Pos)

#define	LPTIM_CCSR_CC2S_Pos	2
#define	LPTIM_CCSR_CC2S_Msk	(0x3U << LPTIM_CCSR_CC2S_Pos)
#define	LPTIM_CCSR_CC2S_NON	(0x0U << LPTIM_CCSR_CC2S_Pos)
#define	LPTIM_CCSR_CC2S_LPT_CH2_IN	(0x1U << LPTIM_CCSR_CC2S_Pos)
#define	LPTIM_CCSR_CC2S_LPT_CH2_OUT	(0x2U << LPTIM_CCSR_CC2S_Pos)

#define	LPTIM_CCSR_CC1S_Pos	0
#define	LPTIM_CCSR_CC1S_Msk	(0x3U << LPTIM_CCSR_CC1S_Pos)
#define	LPTIM_CCSR_CC1S_NON	(0x0U << LPTIM_CCSR_CC1S_Pos)
#define	LPTIM_CCSR_CC1S_LPT_CH1_IN	(0x1U << LPTIM_CCSR_CC1S_Pos)
#define	LPTIM_CCSR_CC1S_LPT_CH1_OUT	(0x2U << LPTIM_CCSR_CC1S_Pos)

#define	LPTIM_ARR_ARR_Pos	0	/* 自动重载目标寄存器(Auto-Reload Register)
当计数器计数值等于ARR时，计数器回到初值重新开始向上计数 */
#define	LPTIM_ARR_ARR_Msk	(0xffffffffU << LPTIM_ARR_ARR_Pos)

#define	LPTIM_IER_OVR4IE_Pos	11	/* 通道4捕捉溢出中断使能(Channel 4 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 */
#define	LPTIM_IER_OVR4IE_Msk	(0x1U << LPTIM_IER_OVR4IE_Pos)
	/* 允许中断 */
	/* 禁止中断 */

#define	LPTIM_IER_OVR3IE_Pos	10	/* 通道3捕捉溢出中断使能(Channel 3 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 */
#define	LPTIM_IER_OVR3IE_Msk	(0x1U << LPTIM_IER_OVR3IE_Pos)
	/* 允许中断 */
	/* 禁止中断 */

#define	LPTIM_IER_OVR2IE_Pos	9	/* 通道2捕捉溢出中断使能(Channel 2 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 */
#define	LPTIM_IER_OVR2IE_Msk	(0x1U << LPTIM_IER_OVR2IE_Pos)
	/* 允许中断 */
	/* 禁止中断 */

#define	LPTIM_IER_OVR1IE_Pos	8	/* 通道1捕捉溢出中断使能(Channel 1 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 */
#define	LPTIM_IER_OVR1IE_Msk	(0x1U << LPTIM_IER_OVR1IE_Pos)
	/* 允许中断 */
	/* 禁止中断 */

#define	LPTIM_IER_TRIGIE_Pos	7	/* 外部触发到来中断使能位(External Trigger Interrupt Enable)
1：外部触发到来中断使能
0：外部触发到来中断禁止 */
#define	LPTIM_IER_TRIGIE_Msk	(0x1U << LPTIM_IER_TRIGIE_Pos)
	/* 外部触发到来中断使能 */
	/* 外部触发到来中断禁止 */

#define	LPTIM_IER_OVIE_Pos	6	/* 计数器溢出中断使能位(Counter Over-Flow Interrupt Enable)
1：计数器溢出中断使能
0：计数器溢出中断禁止 */
#define	LPTIM_IER_OVIE_Msk	(0x1U << LPTIM_IER_OVIE_Pos)
	/* 计数器溢出中断使能 */
	/* 计数器溢出中断禁止 */

#define	LPTIM_IER_CC4IE_Pos	3	/* 捕捉/比较通道4中断使能位(Capture/Compare channel 4 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 */
#define	LPTIM_IER_CC4IE_Msk	(0x1U << LPTIM_IER_CC4IE_Pos)
	/* 捕捉/比较通道2中断使能 */
	/* 捕捉/比较通道2中断禁止 */

#define	LPTIM_IER_CC3IE_Pos	2	/* 捕捉/比较通道3中断使能位(Capture/Compare channel 3 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 */
#define	LPTIM_IER_CC3IE_Msk	(0x1U << LPTIM_IER_CC3IE_Pos)
	/* 捕捉/比较通道1中断使能 */
	/* 捕捉/比较通道1中断禁止 */

#define	LPTIM_IER_CC2IE_Pos	1	/* 捕捉/比较通道2中断使能位(Capture/Compare channel 2 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 */
#define	LPTIM_IER_CC2IE_Msk	(0x1U << LPTIM_IER_CC2IE_Pos)
	/* 捕捉/比较通道2中断使能 */
	/* 捕捉/比较通道2中断禁止 */

#define	LPTIM_IER_CC1IE_Pos	0	/* 捕捉/比较通道1中断使能位(Capture/Compare channel 1 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 */
#define	LPTIM_IER_CC1IE_Msk	(0x1U << LPTIM_IER_CC1IE_Pos)
	/* 捕捉/比较通道1中断使能 */
	/* 捕捉/比较通道1中断禁止 */

#define	LPTIM_ISR_CAP4OVR_Pos	11
#define	LPTIM_ISR_CAP4OVR_Msk	(0x1U << LPTIM_ISR_CAP4OVR_Pos)

#define	LPTIM_ISR_CAP3OVR_Pos	10
#define	LPTIM_ISR_CAP3OVR_Msk	(0x1U << LPTIM_ISR_CAP3OVR_Pos)

#define	LPTIM_ISR_CAP2OVR_Pos	9
#define	LPTIM_ISR_CAP2OVR_Msk	(0x1U << LPTIM_ISR_CAP2OVR_Pos)

#define	LPTIM_ISR_CAP1OVR_Pos	8
#define	LPTIM_ISR_CAP1OVR_Msk	(0x1U << LPTIM_ISR_CAP1OVR_Pos)

#define	LPTIM_ISR_TRIGIF_Pos	7
#define	LPTIM_ISR_TRIGIF_Msk	(0x1U << LPTIM_ISR_TRIGIF_Pos)

#define	LPTIM_ISR_OVIF_Pos	6
#define	LPTIM_ISR_OVIF_Msk	(0x1U << LPTIM_ISR_OVIF_Pos)

#define	LPTIM_ISR_CC4IF_Pos	3
#define	LPTIM_ISR_CC4IF_Msk	(0x1U << LPTIM_ISR_CC4IF_Pos)

#define	LPTIM_ISR_CC3IF_Pos	2
#define	LPTIM_ISR_CC3IF_Msk	(0x1U << LPTIM_ISR_CC3IF_Pos)

#define	LPTIM_ISR_CC2IF_Pos	1
#define	LPTIM_ISR_CC2IF_Msk	(0x1U << LPTIM_ISR_CC2IF_Pos)

#define	LPTIM_ISR_CC1IF_Pos	0
#define	LPTIM_ISR_CC1IF_Msk	(0x1U << LPTIM_ISR_CC1IF_Pos)

#define	LPTIM_CR_EN_Pos	0	/* LPTIM使能位(LPTIM Enable)
1：使能计数器计数
0：禁止计数器计数 */
#define	LPTIM_CR_EN_Msk	(0x1U << LPTIM_CR_EN_Pos)
	/* 使能计数器计数 */
	/* 禁止计数器计数 */

#define	LPTIM_CCR1_CCR1_Pos	0	/* 捕捉/比较值寄存器1 (Channel1 Capture/Compare Register) */
#define	LPTIM_CCR1_CCR1_Msk	(0xffffffffU << LPTIM_CCR1_CCR1_Pos)

#define	LPTIM_CCR2_CCR2_Pos	0	/* 捕捉/比较值寄存器2 (Channel2 Capture/Compare Register) */
#define	LPTIM_CCR2_CCR2_Msk	(0xffffffffU << LPTIM_CCR2_CCR2_Pos)

#define	LPTIM_CCR3_CCR3_Pos	0	/* 捕捉/比较值寄存器3 (Channel3 Capture/Compare Register) */
#define	LPTIM_CCR3_CCR3_Msk	(0xffffffffU << LPTIM_CCR3_CCR3_Pos)

#define	LPTIM_CCR4_CCR4_Pos	0	/* 捕捉/比较值寄存器4 (Channel4 Capture/Compare Register) */
#define	LPTIM_CCR4_CCR4_Msk	(0xffffffffU << LPTIM_CCR4_CCR4_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void LPTIM_Deinit(void);

/* LPT_ETR输入模拟滤波使能(External Trigger input Analog Filter Enable)
0：关闭模拟滤波
1：使能模拟滤波，滤波宽度约100ns 相关函数 */
extern void LPTIM_CFGR_ETR_AFEN_Setable(FunState NewState);
extern FunState LPTIM_CFGR_ETR_AFEN_Getable(void);

/* 时钟预分频输入选择(Prescaler input Select)
0：CLKSEL选择的时钟
1：LPTETRF 相关函数 */
extern void LPTIM_CFGR_PSCSEL_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_PSCSEL_Get(void);

/* 计数时钟分频选择(Counter Clock Divider Select)
000：1分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110：64分频
111：128分频 相关函数 */
extern void LPTIM_CFGR_DIVSEL_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_DIVSEL_Get(void);

/* ETR输入边沿选择(ETR Clock Edge Select)
0：LPT_ETR的上升沿计数
1：LPT_ETR的下降沿计数 相关函数 */
extern void LPTIM_CFGR_EDGESEL_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_EDGESEL_Get(void);

/* 外部触发边沿选择（需使用内部时钟同步采样LPT_ETR）(ETR trigger Configuration)
00: LPT_ETR输入信号上升沿触发
01: LPT_ETR输入信号下降沿触发
10/11: 外部输入信号上升下降沿触发 相关函数 */
extern void LPTIM_CFGR_TRIGCFG_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_TRIGCFG_Get(void);

/* 单次计数模式使能(One State Timer)
0：连续计数模式：计数器被触发后保持运行，直到被关闭为止。计数器达到目标值后回到0重新开始计数，并产生溢出中断。
1：单次计数模式：计数器被触发后计数到目标值后回到0，并自动停止，产生溢出中断。 相关函数 */
extern void LPTIM_CFGR_ONST_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_ONST_Get(void);

/* 工作模式选择(Timer operation Mode)
00：普通定时器模式
01：Trigger脉冲触发计数模式
10：外部异步脉冲计数模式
11：Timeout模式 相关函数 */
extern void LPTIM_CFGR_TMODE_Set(uint32_t SetValue);
extern uint32_t LPTIM_CFGR_TMODE_Get(void);

/* 32bit计数器当前计数值(Counter 32bits-wide) 相关函数 */
extern uint32_t LPTIM_CNTR_Read(void);

/* 通道1捕捉源选择(Capture channel 1 source select)，仅在CH1通道配置为输入捕捉时有效
00：LPT_CH1输入
01：XTLF
10：RCLP
11：RCMF 相关函数 */
extern void LPTIM_CCSR_CAP1SSEL_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAP1SSEL_Get(void);

/* 通道4当前被捕捉的边沿，在CC2IF置位时更新(Channel 4 Captured Edge)
0：上升沿
1：下降沿 相关函数 */
extern void LPTIM_CCSR_CAP4EDGE_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAP4EDGE_Get(void);
extern void LPTIM_CCSR_CAP3EDGE_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAP3EDGE_Get(void);
extern void LPTIM_CCSR_CAP2EDGE_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAP2EDGE_Get(void);
extern void LPTIM_CCSR_CAP1EDGE_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAP1EDGE_Get(void);

/* 通道4比较输出波形极性选择 (Channel 4 compare output Polarity)
0：正极性波形，起始为低，计数值==比较值时置高，计数值==ARR时恢复为低
1：负极性波形，正极性波形取反 相关函数 */
extern void LPTIM_CCSR_POLAR4_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_POLAR4_Get(void);
extern void LPTIM_CCSR_POLAR3_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_POLAR3_Get(void);
extern void LPTIM_CCSR_POLAR2_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_POLAR2_Get(void);
extern void LPTIM_CCSR_POLAR1_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_POLAR1_Get(void);

/* 通道4捕捉边沿选择(Channel 4 Capture edge Config)
00：上升沿捕捉
01：下降沿捕捉
10：上升下降沿捕捉
11：RFU 相关函数 */
extern void LPTIM_CCSR_CAPCFG4_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAPCFG4_Get(void);
extern void LPTIM_CCSR_CAPCFG3_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAPCFG3_Get(void);
extern void LPTIM_CCSR_CAPCFG2_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAPCFG2_Get(void);
extern void LPTIM_CCSR_CAPCFG1_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CAPCFG1_Get(void);

/* 通道4捕捉/比较功能使能(Channel 4 Capture/Compare Select)
00,11：禁止通道4捕捉/比较功能
01：使能通道4捕捉功能（LPT_CH4为输入）
10：使能通道4比较功能（LPT_CH4为输出） 相关函数 */
extern void LPTIM_CCSR_CC4S_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CC4S_Get(void);
extern void LPTIM_CCSR_CC3S_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CC3S_Get(void);
extern void LPTIM_CCSR_CC2S_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CC2S_Get(void);
extern void LPTIM_CCSR_CC1S_Set(uint32_t SetValue);
extern uint32_t LPTIM_CCSR_CC1S_Get(void);

/* 自动重载目标寄存器(Auto-Reload Register)
当计数器计数值等于ARR时，计数器回到初值重新开始向上计数 相关函数 */
extern void LPTIM_ARR_Write(uint32_t SetValue);
extern uint32_t LPTIM_ARR_Read(void);

/* 通道4捕捉溢出中断使能(Channel 4 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
extern void LPTIM_IER_OVR4IE_Setable(FunState NewState);
extern FunState LPTIM_IER_OVR4IE_Getable(void);

/* 通道3捕捉溢出中断使能(Channel 3 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
extern void LPTIM_IER_OVR3IE_Setable(FunState NewState);
extern FunState LPTIM_IER_OVR3IE_Getable(void);

/* 通道2捕捉溢出中断使能(Channel 2 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
extern void LPTIM_IER_OVR2IE_Setable(FunState NewState);
extern FunState LPTIM_IER_OVR2IE_Getable(void);

/* 通道1捕捉溢出中断使能(Channel 1 Over-Capture Interrupt Enable)
1：允许中断
0：禁止中断 相关函数 */
extern void LPTIM_IER_OVR1IE_Setable(FunState NewState);
extern FunState LPTIM_IER_OVR1IE_Getable(void);

/* 外部触发到来中断使能位(External Trigger Interrupt Enable)
1：外部触发到来中断使能
0：外部触发到来中断禁止 相关函数 */
extern void LPTIM_IER_TRIGIE_Setable(FunState NewState);
extern FunState LPTIM_IER_TRIGIE_Getable(void);

/* 计数器溢出中断使能位(Counter Over-Flow Interrupt Enable)
1：计数器溢出中断使能
0：计数器溢出中断禁止 相关函数 */
extern void LPTIM_IER_OVIE_Setable(FunState NewState);
extern FunState LPTIM_IER_OVIE_Getable(void);

/* 捕捉/比较通道4中断使能位(Capture/Compare channel 4 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 相关函数 */
extern void LPTIM_IER_CC4IE_Setable(FunState NewState);
extern FunState LPTIM_IER_CC4IE_Getable(void);

/* 捕捉/比较通道3中断使能位(Capture/Compare channel 3 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 相关函数 */
extern void LPTIM_IER_CC3IE_Setable(FunState NewState);
extern FunState LPTIM_IER_CC3IE_Getable(void);

/* 捕捉/比较通道2中断使能位(Capture/Compare channel 2 Interrupt Enable)
1：捕捉/比较通道2中断使能
0：捕捉/比较通道2中断禁止 相关函数 */
extern void LPTIM_IER_CC2IE_Setable(FunState NewState);
extern FunState LPTIM_IER_CC2IE_Getable(void);

/* 捕捉/比较通道1中断使能位(Capture/Compare channel 1 Interrupt Enable)
1：捕捉/比较通道1中断使能
0：捕捉/比较通道1中断禁止 相关函数 */
extern void LPTIM_IER_CC1IE_Setable(FunState NewState);
extern FunState LPTIM_IER_CC1IE_Getable(void);
extern void LPTIM_ISR_CAP4OVR_Clr(void);
extern FlagStatus LPTIM_ISR_CAP4OVR_Chk(void);
extern void LPTIM_ISR_CAP3OVR_Clr(void);
extern FlagStatus LPTIM_ISR_CAP3OVR_Chk(void);
extern void LPTIM_ISR_CAP2OVR_Clr(void);
extern FlagStatus LPTIM_ISR_CAP2OVR_Chk(void);
extern void LPTIM_ISR_CAP1OVR_Clr(void);
extern FlagStatus LPTIM_ISR_CAP1OVR_Chk(void);
extern void LPTIM_ISR_TRIGIF_Clr(void);
extern FlagStatus LPTIM_ISR_TRIGIF_Chk(void);
extern void LPTIM_ISR_OVIF_Clr(void);
extern FlagStatus LPTIM_ISR_OVIF_Chk(void);
extern void LPTIM_ISR_CC4IF_Clr(void);
extern FlagStatus LPTIM_ISR_CC4IF_Chk(void);
extern void LPTIM_ISR_CC3IF_Clr(void);
extern FlagStatus LPTIM_ISR_CC3IF_Chk(void);
extern void LPTIM_ISR_CC2IF_Clr(void);
extern FlagStatus LPTIM_ISR_CC2IF_Chk(void);
extern void LPTIM_ISR_CC1IF_Clr(void);
extern FlagStatus LPTIM_ISR_CC1IF_Chk(void);




/* LPTIM使能位(LPTIM Enable)
1：使能计数器计数
0：禁止计数器计数 相关函数 */
extern void LPTIM_CR_EN_Setable(FunState NewState);
extern FunState LPTIM_CR_EN_Getable(void);

/* 捕捉/比较值寄存器1 (Channel1 Capture/Compare Register) 相关函数 */
extern void LPTIM_CCR1_Write(uint32_t SetValue);
extern uint32_t LPTIM_CCR1_Read(void);
extern void LPTIM_Deinit(void);

/* 捕捉/比较值寄存器2 (Channel2 Capture/Compare Register) 相关函数 */
extern void LPTIM_CCR2_Write(uint32_t SetValue);
extern uint32_t LPTIM_CCR2_Read(void);
extern void LPTIM_Deinit(void);

/* 捕捉/比较值寄存器3 (Channel3 Capture/Compare Register) 相关函数 */
extern void LPTIM_CCR3_Write(uint32_t SetValue);
extern uint32_t LPTIM_CCR3_Read(void);

/* 捕捉/比较值寄存器4 (Channel4 Capture/Compare Register) 相关函数 */
extern void LPTIM_CCR4_Write(uint32_t SetValue);
extern uint32_t LPTIM_CCR4_Read(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_LPTIM_H */
