/**
  ******************************************************************************
  * @file    fm33a0xxev_lcd.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the LCD firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_LCD_H
#define __FM33A0XXEV_LCD_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
  
#define	LCD_CR_SCSEL_Pos	20/* 电容驱动模式，驱动频率
000：频率为帧频*COM数
001：频率为LSCLK/8
010：频率为LSCLK/16
011：频率为LSCLK/32
100：频率为LSCLK/64
101：频率为LSCLK/128
110：频率为LSCLK/256
111：频率为LSCLK/512相关函数 */
#define	LCD_CR_SCSEL_Msk	(0x7U << LCD_CR_SCSEL_Pos)
#define	LCD_CR_SCSEL_LSCLK_COM		(0x0U << LCD_CR_SCSEL_Pos)	/* 频率为帧频*COM数 */
#define	LCD_CR_SCSEL_LSCLK_8		(0x1U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/8*/
#define	LCD_CR_SCSEL_LSCLK_16		(0x2U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/16 */
#define	LCD_CR_SCSEL_LSCLK_32		(0x3U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/32 */
#define	LCD_CR_SCSEL_LSCLK_64		(0x4U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/64 */
#define	LCD_CR_SCSEL_LSCLK_128		(0x5U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/128 */
#define	LCD_CR_SCSEL_LSCLK_256		(0x6U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/256 */
#define	LCD_CR_SCSEL_LSCLK_512		(0x7U << LCD_CR_SCSEL_Pos)	/* 频率为LSCLK/512 */

#define	LCD_CR_SC_CTRL_Pos	18/* 电容驱动模式，驱动方式控制 
00：单次驱动
01：连续驱动2次
10：连续驱动4次
11：多次驱动 相关函数 */
#define	LCD_CR_SC_CTRL_Msk	(0x3U << LCD_CR_SC_CTRL_Pos)
#define	LCD_CR_SC_CTRL_S0	(0x0U << LCD_CR_SC_CTRL_Pos)	/* 单次驱动 */
#define	LCD_CR_SC_CTRL_S1	(0x1U << LCD_CR_SC_CTRL_Pos)	/* 连续驱动2次*/
#define	LCD_CR_SC_CTRL_S2	(0x2U << LCD_CR_SC_CTRL_Pos)	/* 连续驱动4次 */
#define	LCD_CR_SC_CTRL_S3	(0x3U << LCD_CR_SC_CTRL_Pos)	/* 多次驱动 */

	 
#define	LCD_CR_IC_CTRL_Pos	16	/* 偏置电路输入电流源大小控制 (Input bias Current Control)
00：电流最大
01：电流次大
10：电流次小
11：电流最小 */
#define	LCD_CR_IC_CTRL_Msk	(0x3U << LCD_CR_IC_CTRL_Pos)
#define	LCD_CR_IC_CTRL_L3	(0x0U << LCD_CR_IC_CTRL_Pos)	/* 电流最大 */
#define	LCD_CR_IC_CTRL_L2	(0x1U << LCD_CR_IC_CTRL_Pos)	/* 电流次大 */
#define	LCD_CR_IC_CTRL_L1	(0x2U << LCD_CR_IC_CTRL_Pos)	/* 电流次小 */
#define	LCD_CR_IC_CTRL_L0	(0x3U << LCD_CR_IC_CTRL_Pos)	/* 电流最小 */

#define	LCD_CR_ENMODE_Pos	15	/* 驱动模式选择 (LCD Enabling Mode)
0：RFU
1：片内电阻型驱动 */
#define	LCD_CR_ENMODE_Msk	(0x1U << LCD_CR_ENMODE_Pos)
#define	LCD_CR_ENMODE_EXTERNALCAP	(0x0U << LCD_CR_ENMODE_Pos)	/* 0 = 片外电容驱动 */
#define	LCD_CR_ENMODE_INNERRESISTER	(0x1U << LCD_CR_ENMODE_Pos)	/* 1 = 片内电阻型驱动 */

#define	LCD_CR_FLICK_Pos	14	/* 显示闪烁使能位 (LCD Flick Enable)
1：显示闪烁，闪烁频率由TON和TOFF寄存器设置
0：关闭闪烁 */
#define	LCD_CR_FLICK_Msk	(0x1U << LCD_CR_FLICK_Pos)
	/* 显示闪烁，闪烁频率由TON和TOFF寄存器设置 */
	/* 关闭闪烁 */

#define	LCD_CR_BIAS_Pos	8	/* LCD偏置电平选择位，用于显示灰度控制 (LCD Bias Voltage Select) */
#define	LCD_CR_BIAS_Msk	(0xfU << LCD_CR_BIAS_Pos)

#define	LCD_CR_BIASMD_Pos	5	/* 偏置类型选择 (Bias Mode)
1：1/3 Bias
0：1/4 Bias */
#define	LCD_CR_BIASMD_Msk	(0x1U << LCD_CR_BIASMD_Pos)
#define	LCD_CR_BIASMD_3BIAS	(0x1U << LCD_CR_BIASMD_Pos)	/* 1/3 Bias */
#define	LCD_CR_BIASMD_4BIAS	(0x0U << LCD_CR_BIASMD_Pos)	/* 1/4 Bias */

#define	LCD_CR_ANTIPOLAR_Pos	4	/* 防极化使能 (Anti-Polarization)
1：COM和SEG在LCD关闭情况下接地
0：COM和SEG在LCD关闭情况下浮空 */
#define	LCD_CR_ANTIPOLAR_Msk	(0x1U << LCD_CR_ANTIPOLAR_Pos)
#define	LCD_CR_ANTIPOLAR_GND	(0x1U << LCD_CR_ANTIPOLAR_Pos)	/* COM和SEG在LCD关闭情况下接地 */
#define	LCD_CR_ANTIPOLAR_FLOAT	(0x0U << LCD_CR_ANTIPOLAR_Pos)	/* COM和SEG在LCD关闭情况下浮空 */

#define	LCD_CR_WFT_Pos	3	/* 驱动波形选择 (Waveform Format)
1：B类波形
0：A类波形 */
#define	LCD_CR_WFT_Msk	(0x1U << LCD_CR_WFT_Pos)
#define	LCD_CR_WFT_BTYPE	(0x1U << LCD_CR_WFT_Pos)	/* B类波形 */
#define	LCD_CR_WFT_ATYPE	(0x0U << LCD_CR_WFT_Pos)	/* A类波形 */

#define	LCD_CR_LMUX_Pos	1	/* COM数量选择 (Segment Line Mux)
00：4COM
01：6COM
10/11：8COM */
#define	LCD_CR_LMUX_Msk	(0x3U << LCD_CR_LMUX_Pos)
#define	LCD_CR_LMUX_4COM	(0x0U << LCD_CR_LMUX_Pos)	/* 4COM */
#define	LCD_CR_LMUX_6COM	(0x1U << LCD_CR_LMUX_Pos)	/* 6COM */
#define	LCD_CR_LMUX_8COM	(0x2U << LCD_CR_LMUX_Pos)	/* 8COM */


#define	LCD_CR_EN_Pos	0	/* LCD显示使能位 (LCD Enable)
1：启动LCD显示
0：关闭LCD显示 */
#define	LCD_CR_EN_Msk	(0x1U << LCD_CR_EN_Pos)
	/* 启动LCD显示 */
	/* 关闭LCD显示 */

#define	LCD_TEST_LCCTRL_Pos	7	/* LCD测试控制位，仅在测试模式下有效 (Line Constant Control)
COM、SEG输出电平由测试模式下的引脚输出数据寄存器决定。不同设置下SEG或COM输出的结果参见后文表格。 */
#define	LCD_TEST_LCCTRL_Msk	(0x1U << LCD_TEST_LCCTRL_Pos)
#define	LCD_TEST_LCCTRL_LOW	(0x0U << LCD_TEST_LCCTRL_Pos)
#define	LCD_TEST_LCCTRL_HIGH	(0x1U << LCD_TEST_LCCTRL_Pos)

#define	LCD_TEST_TESTEN_Pos	0	/* 测试模式使能位 (Test mode Enable)
1：LCD测试模式使能。在LCD测试模式下，LCD引脚静态输出模拟直流电平，所有与动态扫描时间以及扫描波形相关寄存器设置无效
0：正常工作模式，测试模式无效，相关测试寄存器控制无效 */
#define	LCD_TEST_TESTEN_Msk	(0x1U << LCD_TEST_TESTEN_Pos)
	/* LCD测试模式使能。在LCD测试模式下，LCD引脚静态输出模拟直流电平，所有与动态扫描时间以及扫描波形相关寄存器设置无效 */
	/* 正常工作模式，测试模式无效，相关测试寄存器控制无效 */

#define	LCD_FCR_DF_Pos	0	/* 显示预分频寄存器 (Display Frequency) */
#define	LCD_FCR_DF_Msk	(0xffU << LCD_FCR_DF_Pos)

#define	LCD_FLKT_TOFF_Pos	8	/* 闪烁显示熄灭时间寄存器 (Display-Off Time)
TOFF最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际OFF时间为TOFF * Tstep */
#define	LCD_FLKT_TOFF_Msk	(0xffU << LCD_FLKT_TOFF_Pos)

#define	LCD_FLKT_TON_Pos	0	/* 闪烁显示点亮时间寄存器 (Display-On Time)
TON最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际ON时间为TON * Tstep */
#define	LCD_FLKT_TON_Msk	(0xffU << LCD_FLKT_TON_Pos)

#define	LCD_IER_DONIE_Pos	1	/* 显示点亮中断使能位 (Display-On Interrupt Enable)
1：显示点亮中断使能
0：显示点亮中断禁止 */
#define	LCD_IER_DONIE_Msk	(0x1U << LCD_IER_DONIE_Pos)
	/* 显示点亮中断使能 */
	/* 显示点亮中断禁止 */

#define	LCD_IER_DOFFIE_Pos	0	/* 显示熄灭中断使能位 (Display-OFF Interrupt Enable)
1：显示熄灭中断使能
0：显示熄灭中断禁止 */
#define	LCD_IER_DOFFIE_Msk	(0x1U << LCD_IER_DOFFIE_Pos)
	/* 显示熄灭中断使能 */
	/* 显示熄灭中断禁止 */

#define	LCD_ISR_DONIF_Pos	1	/* 显示点亮中断标志 (Display-On Interrupt Flag,write 1 to clear)
显示由灭变亮时硬件产生中断标志，硬件置位，软件清零 */
#define	LCD_ISR_DONIF_Msk	(0x1U << LCD_ISR_DONIF_Pos)

#define	LCD_ISR_DOFFIF_Pos	0	/* 显示熄灭中断标志 (Display-OFF Interrupt Flag,write 1 to clear)
显示由亮变灭时硬件产生中断标志，硬件置位，软件清零 */
#define	LCD_ISR_DOFFIF_Msk	(0x1U << LCD_ISR_DOFFIF_Pos)

#define	LCD_BSTCR_OSC_CFG_Pos	12
#define	LCD_BSTCR_OSC_CFG_Msk	(0xfU << LCD_BSTCR_OSC_CFG_Pos)

#define	LCD_BSTCR_BUF_OFF_Pos	9
#define	LCD_BSTCR_BUF_OFF_Msk	(0x1U << LCD_BSTCR_BUF_OFF_Pos)

#define	LCD_BSTCR_BUFBYP_Pos	8
#define	LCD_BSTCR_BUFBYP_Msk	(0x1U << LCD_BSTCR_BUFBYP_Pos)

#define	LCD_BSTCR_VLCDPD_Pos	7
#define	LCD_BSTCR_VLCDPD_Msk	(0x1U << LCD_BSTCR_VLCDPD_Pos)

#define	LCD_BSTCR_VLCDCFG_Pos	2
#define	LCD_BSTCR_VLCDCFG_Msk	(0x1fU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_2_4V	(0x0U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_2_6V	(0x1U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_2_8V	(0x2U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_3_0V	(0x3U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_3_2V	(0x4U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_3_4V	(0x5U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_3_6V	(0x6U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_3_8V	(0x7U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_4_0V	(0x8U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_4_2V	(0x9U << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_4_4V	(0xaU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_4_6V	(0xbU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_4_8V	(0xcU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_5_0V	(0xdU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_5_2V	(0xeU << LCD_BSTCR_VLCDCFG_Pos)
#define	LCD_BSTCR_VLCDCFG_5_4V	(0xfU << LCD_BSTCR_VLCDCFG_Pos)

#define	LCD_BSTCR_BOOST_TEN_Pos	1
#define	LCD_BSTCR_BOOST_TEN_Msk	(0x1U << LCD_BSTCR_BOOST_TEN_Pos)

#define	LCD_BSTCR_BOOST_EN_Pos	0
#define	LCD_BSTCR_BOOST_EN_Msk	(0x1U << LCD_BSTCR_BOOST_EN_Pos)

#define	LCD_COM_EN_COMEN_Pos	0	/* LCD COM0~3输出使能控制 */
#define	LCD_COM_EN_COMEN_Msk	(0xfU << LCD_COM_EN_COMEN_Pos)

#define	LCD_SEG_EN0_SEGENx_Pos	0	/* LCD SEG0~31输出使能控制 */
#define	LCD_SEG_EN0_SEGENx_Msk	(0xffffffffU << LCD_SEG_EN0_SEGENx_Pos)

#define	LCD_SEG_EN1_SEGENx_Pos	0	/* LCD SEG32~43,COM4~7输出使能控制 */
#define	LCD_SEG_EN1_SEGENx_Msk	(0xfffU << LCD_SEG_EN1_SEGENx_Pos)

#define	LCD_COM_EN_COMEN3_Pos	3	/* COM3输出控制@4\6\8COM模式 */
#define	LCD_COM_EN_COMEN3_Msk	(0x1U << LCD_COM_EN_COMEN3_Pos)

#define	LCD_COM_EN_COMEN2_Pos	2	/* COM2输出控制@4\6\8COM模式 */
#define	LCD_COM_EN_COMEN2_Msk	(0x1U << LCD_COM_EN_COMEN2_Pos)

#define	LCD_COM_EN_COMEN1_Pos	1	/* COM1输出控制@4\6\8COM模式 */
#define	LCD_COM_EN_COMEN1_Msk	(0x1U << LCD_COM_EN_COMEN1_Pos)

#define	LCD_COM_EN_COMEN0_Pos	0	/* COM0输出控制@4\6\8COM模式 */
#define	LCD_COM_EN_COMEN0_Msk	(0x1U << LCD_COM_EN_COMEN0_Pos)

#define	LCD_SEG_EN0_SEGEN31_Pos	31	/* SEG31输出控制 */
#define	LCD_SEG_EN0_SEGEN31_Msk	(0x1U << LCD_SEG_EN0_SEGEN31_Pos)

#define	LCD_SEG_EN0_SEGEN30_Pos	30	/* SEG30输出控制 */
#define	LCD_SEG_EN0_SEGEN30_Msk	(0x1U << LCD_SEG_EN0_SEGEN30_Pos)

#define	LCD_SEG_EN0_SEGEN29_Pos	29	/* SEG29输出控制 */
#define	LCD_SEG_EN0_SEGEN29_Msk	(0x1U << LCD_SEG_EN0_SEGEN29_Pos)

#define	LCD_SEG_EN0_SEGEN28_Pos	28	/* SEG28输出控制 */
#define	LCD_SEG_EN0_SEGEN28_Msk	(0x1U << LCD_SEG_EN0_SEGEN28_Pos)

#define	LCD_SEG_EN0_SEGEN27_Pos	27	/* SEG27输出控制 */
#define	LCD_SEG_EN0_SEGEN27_Msk	(0x1U << LCD_SEG_EN0_SEGEN27_Pos)

#define	LCD_SEG_EN0_SEGEN26_Pos	26	/* SEG26输出控制 */
#define	LCD_SEG_EN0_SEGEN26_Msk	(0x1U << LCD_SEG_EN0_SEGEN26_Pos)

#define	LCD_SEG_EN0_SEGEN25_Pos	25	/* SEG25输出控制 */
#define	LCD_SEG_EN0_SEGEN25_Msk	(0x1U << LCD_SEG_EN0_SEGEN25_Pos)

#define	LCD_SEG_EN0_SEGEN24_Pos	24	/* SEG24输出控制 */
#define	LCD_SEG_EN0_SEGEN24_Msk	(0x1U << LCD_SEG_EN0_SEGEN24_Pos)

#define	LCD_SEG_EN0_SEGEN23_Pos	23	/* SEG23输出控制 */
#define	LCD_SEG_EN0_SEGEN23_Msk	(0x1U << LCD_SEG_EN0_SEGEN23_Pos)

#define	LCD_SEG_EN0_SEGEN22_Pos	22	/* SEG22输出控制 */
#define	LCD_SEG_EN0_SEGEN22_Msk	(0x1U << LCD_SEG_EN0_SEGEN22_Pos)

#define	LCD_SEG_EN0_SEGEN21_Pos	21	/* SEG21输出控制 */
#define	LCD_SEG_EN0_SEGEN21_Msk	(0x1U << LCD_SEG_EN0_SEGEN21_Pos)

#define	LCD_SEG_EN0_SEGEN20_Pos	20	/* SEG20输出控制 */
#define	LCD_SEG_EN0_SEGEN20_Msk	(0x1U << LCD_SEG_EN0_SEGEN20_Pos)

#define	LCD_SEG_EN0_SEGEN19_Pos	19	/* SEG19输出控制 */
#define	LCD_SEG_EN0_SEGEN19_Msk	(0x1U << LCD_SEG_EN0_SEGEN19_Pos)

#define	LCD_SEG_EN0_SEGEN18_Pos	18	/* SEG18输出控制 */
#define	LCD_SEG_EN0_SEGEN18_Msk	(0x1U << LCD_SEG_EN0_SEGEN18_Pos)

#define	LCD_SEG_EN0_SEGEN17_Pos	17	/* SEG17输出控制 */
#define	LCD_SEG_EN0_SEGEN17_Msk	(0x1U << LCD_SEG_EN0_SEGEN17_Pos)

#define	LCD_SEG_EN0_SEGEN16_Pos	16	/* SEG16输出控制 */
#define	LCD_SEG_EN0_SEGEN16_Msk	(0x1U << LCD_SEG_EN0_SEGEN16_Pos)

#define	LCD_SEG_EN0_SEGEN15_Pos	15	/* SEG15输出控制 */
#define	LCD_SEG_EN0_SEGEN15_Msk	(0x1U << LCD_SEG_EN0_SEGEN15_Pos)

#define	LCD_SEG_EN0_SEGEN14_Pos	14	/* SEG14输出控制 */
#define	LCD_SEG_EN0_SEGEN14_Msk	(0x1U << LCD_SEG_EN0_SEGEN14_Pos)

#define	LCD_SEG_EN0_SEGEN13_Pos	13	/* SEG13输出控制 */
#define	LCD_SEG_EN0_SEGEN13_Msk	(0x1U << LCD_SEG_EN0_SEGEN13_Pos)

#define	LCD_SEG_EN0_SEGEN12_Pos	12	/* SEG12输出控制 */
#define	LCD_SEG_EN0_SEGEN12_Msk	(0x1U << LCD_SEG_EN0_SEGEN12_Pos)

#define	LCD_SEG_EN0_SEGEN11_Pos	11	/* SEG11输出控制 */
#define	LCD_SEG_EN0_SEGEN11_Msk	(0x1U << LCD_SEG_EN0_SEGEN11_Pos)

#define	LCD_SEG_EN0_SEGEN10_Pos	10	/* SEG10输出控制 */
#define	LCD_SEG_EN0_SEGEN10_Msk	(0x1U << LCD_SEG_EN0_SEGEN10_Pos)

#define	LCD_SEG_EN0_SEGEN9_Pos	9	/* SEG9输出控制 */
#define	LCD_SEG_EN0_SEGEN9_Msk	(0x1U << LCD_SEG_EN0_SEGEN9_Pos)

#define	LCD_SEG_EN0_SEGEN8_Pos	8	/* SEG8输出控制 */
#define	LCD_SEG_EN0_SEGEN8_Msk	(0x1U << LCD_SEG_EN0_SEGEN8_Pos)

#define	LCD_SEG_EN0_SEGEN7_Pos	7	/* SEG7输出控制 */
#define	LCD_SEG_EN0_SEGEN7_Msk	(0x1U << LCD_SEG_EN0_SEGEN7_Pos)

#define	LCD_SEG_EN0_SEGEN6_Pos	6	/* SEG6输出控制 */
#define	LCD_SEG_EN0_SEGEN6_Msk	(0x1U << LCD_SEG_EN0_SEGEN6_Pos)

#define	LCD_SEG_EN0_SEGEN5_Pos	5	/* SEG5输出控制 */
#define	LCD_SEG_EN0_SEGEN5_Msk	(0x1U << LCD_SEG_EN0_SEGEN5_Pos)

#define	LCD_SEG_EN0_SEGEN4_Pos	4	/* SEG4输出控制 */
#define	LCD_SEG_EN0_SEGEN4_Msk	(0x1U << LCD_SEG_EN0_SEGEN4_Pos)

#define	LCD_SEG_EN0_SEGEN3_Pos	3	/* SEG3输出控制 */
#define	LCD_SEG_EN0_SEGEN3_Msk	(0x1U << LCD_SEG_EN0_SEGEN3_Pos)

#define	LCD_SEG_EN0_SEGEN2_Pos	2	/* SEG2输出控制 */
#define	LCD_SEG_EN0_SEGEN2_Msk	(0x1U << LCD_SEG_EN0_SEGEN2_Pos)

#define	LCD_SEG_EN0_SEGEN1_Pos	1	/* SEG1输出控制 */
#define	LCD_SEG_EN0_SEGEN1_Msk	(0x1U << LCD_SEG_EN0_SEGEN1_Pos)

#define	LCD_SEG_EN0_SEGEN0_Pos	0	/* SEG0输出控制 */
#define	LCD_SEG_EN0_SEGEN0_Msk	(0x1U << LCD_SEG_EN0_SEGEN0_Pos)

#define	LCD_SEG_EN1_SEGEN43_Pos	11	/* SEG43输出控制 */
#define	LCD_SEG_EN1_SEGEN43_Msk	(0x1U << LCD_SEG_EN1_SEGEN43_Pos)

#define	LCD_SEG_EN1_SEGEN42_Pos	10	/* SEG42输出控制 */
#define	LCD_SEG_EN1_SEGEN42_Msk	(0x1U << LCD_SEG_EN1_SEGEN42_Pos)

#define	LCD_SEG_EN1_SEGEN41_Pos	9	/* SEG41输出控制 */
#define	LCD_SEG_EN1_SEGEN41_Msk	(0x1U << LCD_SEG_EN1_SEGEN41_Pos)

#define	LCD_SEG_EN1_SEGEN40_Pos	8	/* SEG40输出控制 */
#define	LCD_SEG_EN1_SEGEN40_Msk	(0x1U << LCD_SEG_EN1_SEGEN40_Pos)

#define	LCD_SEG_EN1_SEGEN39_Pos	7	/* SEG39输出控制 */
#define	LCD_SEG_EN1_SEGEN39_Msk	(0x1U << LCD_SEG_EN1_SEGEN39_Pos)

#define	LCD_SEG_EN1_SEGEN38_Pos	6	/* SEG38输出控制 */
#define	LCD_SEG_EN1_SEGEN38_Msk	(0x1U << LCD_SEG_EN1_SEGEN38_Pos)

#define	LCD_SEG_EN1_SEGEN37_Pos	5	/* SEG37输出控制 */
#define	LCD_SEG_EN1_SEGEN37_Msk	(0x1U << LCD_SEG_EN1_SEGEN37_Pos)

#define	LCD_SEG_EN1_SEGEN36_Pos	4	/* SEG36输出控制 */
#define	LCD_SEG_EN1_SEGEN36_Msk	(0x1U << LCD_SEG_EN1_SEGEN36_Pos)

#define	LCD_SEG_EN1_SEGEN35_Pos	3	/* SEG35输出控制 */
#define	LCD_SEG_EN1_SEGEN35_Msk	(0x1U << LCD_SEG_EN1_SEGEN35_Pos)

#define	LCD_SEG_EN1_SEGEN34_Pos	2	/* SEG34输出控制 */
#define	LCD_SEG_EN1_SEGEN34_Msk	(0x1U << LCD_SEG_EN1_SEGEN34_Pos)

#define	LCD_SEG_EN1_SEGEN33_Pos	1	/* SEG33输出控制 */
#define	LCD_SEG_EN1_SEGEN33_Msk	(0x1U << LCD_SEG_EN1_SEGEN33_Pos)

#define	LCD_SEG_EN1_SEGEN32_Pos	0	/* SEG32输出控制 */
#define	LCD_SEG_EN1_SEGEN32_Msk	(0x1U << LCD_SEG_EN1_SEGEN32_Pos)

#define	LCD_SEG_EN1_COMEN7_Pos	11	/* COM7复用SEGEN43 */
#define	LCD_SEG_EN1_COMEN7_Msk	(0x1U << LCD_SEG_EN1_COMEN7_Pos)

#define	LCD_SEG_EN1_COMEN6_Pos	10	/* COM6复用SEGEN42 */
#define	LCD_SEG_EN1_COMEN6_Msk	(0x1U << LCD_SEG_EN1_COMEN6_Pos)

#define	LCD_SEG_EN1_COMEN5_Pos	9	/* COM5复用SEGEN41 */
#define	LCD_SEG_EN1_COMEN5_Msk	(0x1U << LCD_SEG_EN1_COMEN5_Pos)

#define	LCD_SEG_EN1_COMEN4_Pos	8	/* COM4复用SEGEN40 */
#define	LCD_SEG_EN1_COMEN4_Msk	(0x1U << LCD_SEG_EN1_COMEN4_Pos)

//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void LCD_Deinit(void);

/* 电容驱动模式，驱动频率
000：频率为帧频*COM数
001：频率为LSCLK/8
010：频率为LSCLK/16
011：频率为LSCLK/32
100：频率为LSCLK/64
101：频率为LSCLK/128
110：频率为LSCLK/256
111：频率为LSCLK/512相关函数 */
extern void LCD_CR_SCSEL_Set(uint32_t SetValue);
extern uint32_t LCD_CR_SCSEL_Get(void);

/* 电容驱动模式，驱动方式控制 
00：单次驱动
01：连续驱动2次
10：连续驱动4次
11：多次驱动 相关函数 */
extern void LCD_CR_SC_CTRL_Set(uint32_t SetValue);
extern uint32_t LCD_CR_SC_CTRL_Get(void);

/* 偏置电路输入电流源大小控制 (Input bias Current Control)
00：电流最大
01：电流次大
10：电流次小
11：电流最小 相关函数 */
extern void LCD_CR_IC_CTRL_Set(uint32_t SetValue);
extern uint32_t LCD_CR_IC_CTRL_Get(void);

/* 驱动模式选择 (LCD Enabling Mode)
0：RFU
1：片内电阻型驱动 相关函数 */
extern void LCD_CR_ENMODE_Set(uint32_t SetValue);
extern uint32_t LCD_CR_ENMODE_Get(void);

/* 显示闪烁使能位 (LCD Flick Enable)
1：显示闪烁，闪烁频率由TON和TOFF寄存器设置
0：关闭闪烁 相关函数 */
extern void LCD_CR_FLICK_Setable(FunState NewState);
extern FunState LCD_CR_FLICK_Getable(void);

/* LCD偏置电平选择位，用于显示灰度控制 (LCD Bias Voltage Select) 相关函数 */
extern void LCD_CR_BIAS_Set(uint32_t SetValue);
extern uint32_t LCD_CR_BIAS_Get(void);

/* 偏置类型选择 (Bias Mode)
1：1/3 Bias
0：1/4 Bias 相关函数 */
extern void LCD_CR_BIASMD_Set(uint32_t SetValue);
extern uint32_t LCD_CR_BIASMD_Get(void);

/* 防极化使能 (Anti-Polarization)
1：COM和SEG在LCD关闭情况下接地
0：COM和SEG在LCD关闭情况下浮空 相关函数 */
extern void LCD_CR_ANTIPOLAR_Set(uint32_t SetValue);
extern uint32_t LCD_CR_ANTIPOLAR_Get(void);

/* 驱动波形选择 (Waveform Format)
1：B类波形
0：A类波形 相关函数 */
extern void LCD_CR_WFT_Set(uint32_t SetValue);
extern uint32_t LCD_CR_WFT_Get(void);

/* COM数量选择 (Segment Line Mux)
00：4COM
01：6COM
10/11：8COM 相关函数 */
extern void LCD_CR_LMUX_Set(uint32_t SetValue);
extern uint32_t LCD_CR_LMUX_Get(void);

/* LCD显示使能位 (LCD Enable)
1：启动LCD显示
0：关闭LCD显示 相关函数 */
extern void LCD_CR_EN_Setable(FunState NewState);
extern FunState LCD_CR_EN_Getable(void);

/* LCD测试控制位，仅在测试模式下有效 (Line Constant Control)
COM、SEG输出电平由测试模式下的引脚输出数据寄存器决定。不同设置下SEG或COM输出的结果参见后文表格。 相关函数 */
extern void LCD_TEST_LCCTRL_Set(uint32_t SetValue);
extern uint32_t LCD_TEST_LCCTRL_Get(void);

/* 测试模式使能位 (Test mode Enable)
1：LCD测试模式使能。在LCD测试模式下，LCD引脚静态输出模拟直流电平，所有与动态扫描时间以及扫描波形相关寄存器设置无效
0：正常工作模式，测试模式无效，相关测试寄存器控制无效 相关函数 */
extern void LCD_TEST_TESTEN_Setable(FunState NewState);
extern FunState LCD_TEST_TESTEN_Getable(void);

/* 显示预分频寄存器 (Display Frequency) 相关函数 */
extern void LCD_FCR_Write(uint32_t SetValue);
extern uint32_t LCD_FCR_Read(void);

/* 闪烁显示熄灭时间寄存器 (Display-Off Time)
TOFF最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际OFF时间为TOFF * Tstep 相关函数 */
extern void LCD_FLKT_TOFF_Set(uint32_t SetValue);
extern uint32_t LCD_FLKT_TOFF_Get(void);

/* 闪烁显示点亮时间寄存器 (Display-On Time)
TON最小步长为Tstep = COM*DF[7:0]*2*16/32768Hz，实际ON时间为TON * Tstep 相关函数 */
extern void LCD_FLKT_TON_Set(uint32_t SetValue);
extern uint32_t LCD_FLKT_TON_Get(void);

/* 显示点亮中断使能位 (Display-On Interrupt Enable)
1：显示点亮中断使能
0：显示点亮中断禁止 相关函数 */
extern void LCD_IER_DONIE_Setable(FunState NewState);
extern FunState LCD_IER_DONIE_Getable(void);

/* 显示熄灭中断使能位 (Display-OFF Interrupt Enable)
1：显示熄灭中断使能
0：显示熄灭中断禁止 相关函数 */
extern void LCD_IER_DOFFIE_Setable(FunState NewState);
extern FunState LCD_IER_DOFFIE_Getable(void);

/* 显示点亮中断标志 (Display-On Interrupt Flag,write 1 to clear)
显示由灭变亮时硬件产生中断标志，硬件置位，软件清零 相关函数 */
extern void LCD_ISR_DONIF_Clr(void);
extern FlagStatus LCD_ISR_DONIF_Chk(void);

/* 显示熄灭中断标志 (Display-OFF Interrupt Flag,write 1 to clear)
显示由亮变灭时硬件产生中断标志，硬件置位，软件清零 相关函数 */
extern void LCD_ISR_DOFFIF_Clr(void);
extern FlagStatus LCD_ISR_DOFFIF_Chk(void);
extern void LCD_BSTCR_OSC_CFG_Set(uint32_t SetValue);
extern uint32_t LCD_BSTCR_OSC_CFG_Get(void);
extern void LCD_BSTCR_BUF_OFF_Setable(FunState NewState);
extern FunState LCD_BSTCR_BUF_OFF_Getable(void);
extern void LCD_BSTCR_BUFBYP_Setable(FunState NewState);
extern FunState LCD_BSTCR_BUFBYP_Getable(void);
extern void LCD_BSTCR_VLCDPD_Setable(FunState NewState);
extern FunState LCD_BSTCR_VLCDPD_Getable(void);
extern void LCD_BSTCR_VLCDCFG_Set(uint32_t SetValue);
extern uint32_t LCD_BSTCR_VLCDCFG_Get(void);
extern void LCD_BSTCR_BOOST_TEN_Setable(FunState NewState);
extern FunState LCD_BSTCR_BOOST_TEN_Getable(void);
extern void LCD_BSTCR_BOOST_EN_Setable(FunState NewState);
extern FunState LCD_BSTCR_BOOST_EN_Getable(void);

/* LCD COM0~3输出使能控制 相关函数 */
extern void LCD_COM_EN_Write(uint32_t SetValue);
extern uint32_t LCD_COM_EN_Read(void);

/* LCD SEG0~31输出使能控制 相关函数 */
extern void LCD_SEG_EN0_Write(uint32_t SetValue);
extern uint32_t LCD_SEG_EN0_Read(void);

/* LCD SEG32~43,COM4~7输出使能控制 相关函数 */
extern void LCD_SEG_EN1_Write(uint32_t SetValue);
extern uint32_t LCD_SEG_EN1_Read(void);
//Announce_End

/*DISPDATAx显示数据寄存器刷新*/
extern void LCD_DISPDATAx_Refresh(uint32_t* DispBuf);

#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_LCD_H */
