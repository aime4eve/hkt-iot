/**
  ******************************************************************************
  * @file    fm33a0xxev_adc.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the ADC firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/
 
#ifndef __FM33A0XXEV_ADC_H
#define __FM33A0XXEV_ADC_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	ADC_CR_ADC_IE_Pos	7	/* ADC内部累加模式中断使能 */
#define	ADC_CR_ADC_IE_Msk	(0x1U << ADC_CR_ADC_IE_Pos)

#define	ADC_CR_ACC_IE_Pos	6	/* ADC外部累加模式中断使能 */
#define	ADC_CR_ACC_IE_Msk	(0x1U << ADC_CR_ACC_IE_Pos)

#define	ADC_CR_HPEN_Pos	3	/* ADC功耗模式
0：低功耗模式，工作时钟最高1MHz
1：高功耗模式，工作时钟最高2MHz */
#define	ADC_CR_HPEN_Msk	(0x1U << ADC_CR_HPEN_Pos)
#define	ADC_CR_HPEN_1MHZ	(0x0U << ADC_CR_HPEN_Pos)	/* 低功耗模式，工作时钟最高1MHz */
#define	ADC_CR_HPEN_2MHZ	(0x1U << ADC_CR_HPEN_Pos)	/* 高功耗模式，工作时钟最高2MHz */

#define	ADC_CR_MODE_Pos	2	/* ADC工作模式
0：内部累加器模式
1：外部累加器或CIC模式 */
#define	ADC_CR_MODE_Msk	(0x1U << ADC_CR_MODE_Pos)
#define	ADC_CR_MODE_INNER	(0x0U << ADC_CR_MODE_Pos)	/* 内部累加器模式 */
#define	ADC_CR_MODE_EXTERNAL	(0x1U << ADC_CR_MODE_Pos)	/* 外部累加器或CIC模式 */

#define	ADC_CR_RSTCTRL_EN_Pos	1	/* 积分器复位使能，在MODE=1并且使用外部累加器时，必须置位，其他条件下必须保持为0
0：禁止积分器外部复位
1：允许积分器外部复位 */
#define	ADC_CR_RSTCTRL_EN_Msk	(0x1U << ADC_CR_RSTCTRL_EN_Pos)
	/* 禁止积分器外部复位 */
	/* 允许积分器外部复位 */

#define	ADC_CR_EN_Pos	0	/* ADC使能信号
0：ADC不使能
1：ADC使能

注意：RTC执行自动温补时，硬件会周期性自动使能ADC */
#define	ADC_CR_EN_Msk	(0x1U << ADC_CR_EN_Pos)
	/* ADC不使能 */
	/* ADC使能 */
	/* RTC执行自动温补时，硬件会周期性自动使能ADC */

#define	ADC_TRIM_ADC_TRIM_Pos	0	/* ADC TRIM值，仅针对内部累加器模式输出
内部累加器累加周期：Period = TRIM*2*TADC_CLK
累加周期决定了输出数据位宽，当TRIM=0x7FF最大值时，实际累加周期为4095 cycle，即ADC输出数据最大有效位宽是12bit */
#define	ADC_TRIM_ADC_TRIM_Msk	(0x7ffU << ADC_TRIM_ADC_TRIM_Pos)

#define	ADC_DR_ADC_DATA_Pos	0	/* ADC输出数据，为码流累加结果，未经降采样滤波器处理
当MODE=0时，输出为ADC内部累加结果，最高12bit，位数由ADC_TRIM决定
当MODE=1时，输出位ADC外部累加结果，最高14bit，位数由ACC_PERIOD决定 */
#define	ADC_DR_ADC_DATA_Msk	(0xffffU << ADC_DR_ADC_DATA_Pos)

#define	ADC_ISR_INIT_RDY_Pos	9	/* ADC初始化完成标志，仅MODE=1时有效（不产生中断）
0：ADC还未完成初始化
1：ADC完成初始化

注：MODE=1，ADC_EN置位后，等待MODE_CTRL_DELAY时间之后，此标志置位，表示ADC内部积分器建立完成。MODE=0时，工作时序由ADC内部自行控制，此标志无效。 */
#define	ADC_ISR_INIT_RDY_Msk	(0x1U << ADC_ISR_INIT_RDY_Pos)

#define	ADC_ISR_ACC_IF_Pos	8	/* 累加器完成中断标志，硬件置位，软件写1清零，写0无效 */
#define	ADC_ISR_ACC_IF_Msk	(0x1U << ADC_ISR_ACC_IF_Pos)

#define	ADC_ISR_ADC_DONE_Pos	1	/* ADC转换完成输出，软件只读（不产生中断）
转换完成后此信号保持为高电平，只有关闭ADC才会清0 */
#define	ADC_ISR_ADC_DONE_Msk	(0x1U << ADC_ISR_ADC_DONE_Pos)

#define	ADC_ISR_ADC_IF_Pos	0	/* ADC转换完成中断标志，硬件置位，软件写1清零，写0无效 */
#define	ADC_ISR_ADC_IF_Msk	(0x1U << ADC_ISR_ADC_IF_Pos)

#define	ADC_CFGR_ACC_PERIOD_Pos	16	/* 外部累加器累加周期配置，单位ADC_CLK
000：1023（对应结果10bit）
001：2047（对应结果11bit）
010：4095（对应结果12bit）
011：8191（对应结果13bit）
100：16383（对应结果14bit）
Others：4095 */
#define	ADC_CFGR_ACC_PERIOD_Msk	(0x7U << ADC_CFGR_ACC_PERIOD_Pos)
#define	ADC_CFGR_ACC_PERIOD_10BITS	(0x0U << ADC_CFGR_ACC_PERIOD_Pos)	/* 1023（对应结果10bit） */
#define	ADC_CFGR_ACC_PERIOD_11BITS	(0x1U << ADC_CFGR_ACC_PERIOD_Pos)	/* 2047（对应结果11bit） */
#define	ADC_CFGR_ACC_PERIOD_12BITS	(0x2U << ADC_CFGR_ACC_PERIOD_Pos)	/* 4095（对应结果12bit） */
#define	ADC_CFGR_ACC_PERIOD_13BITS	(0x3U << ADC_CFGR_ACC_PERIOD_Pos)	/* 8191（对应结果13bit） */
#define	ADC_CFGR_ACC_PERIOD_14BITS	(0x4U << ADC_CFGR_ACC_PERIOD_Pos)	/* 16383（对应结果14bit） */

#define	ADC_CFGR_RST_CTRL_DELAY_Pos	8	/* SDADC使能后mode_ctrl延迟长度配置，单位是ADC_CLK周期
0x00：不延迟
0xFF：延迟255个ADC_CLK

注意：复位值为16个时钟周期，不得修改为小于16的数值 */
#define	ADC_CFGR_RST_CTRL_DELAY_Msk	(0xffU << ADC_CFGR_RST_CTRL_DELAY_Pos)

#define	ADC_CFGR_BUFEN_Pos	5	/* ADC输入通道Buffer使能 */
#define	ADC_CFGR_BUFEN_Msk	(0x1U << ADC_CFGR_BUFEN_Pos)

#define	ADC_CFGR_BUFSEL_Pos	0	/* ADC输入通道选择
0000：ADC_IN0
0001：ADC_IN1
0010：ADC_IN2
0011：ADC_IN3
0100：ADC_IN4
0101：ADC_IN5
0110：ADC_IN6
0111：ADC_IN7
1000：ADC_IN8
1001：ADC_IN9
1010：ADC_IN10
1011：ADC_IN11
1100：ADC_IN12
1101：ADC_IN13
1110：VBAT
1111：TS */
#define	ADC_CFGR_BUFSEL_Msk	(0xfU << ADC_CFGR_BUFSEL_Pos)
#define	ADC_CFGR_BUFSEL_ADC_IN1	(0x1U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN1 */
#define	ADC_CFGR_BUFSEL_ADC_IN2	(0x2U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN2 */
#define	ADC_CFGR_BUFSEL_ADC_IN3	(0x3U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN3 */
#define	ADC_CFGR_BUFSEL_ADC_IN4	(0x4U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN4 */
#define	ADC_CFGR_BUFSEL_ADC_IN5	(0x5U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN5 */
#define	ADC_CFGR_BUFSEL_ADC_IN6	(0x6U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN6 */
#define	ADC_CFGR_BUFSEL_ADC_IN7	(0x7U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN7 */
#define	ADC_CFGR_BUFSEL_ADC_IN8	(0x8U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN8 */
#define	ADC_CFGR_BUFSEL_ADC_IN9	(0x9U << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN9 */
#define	ADC_CFGR_BUFSEL_ADC_IN10	(0xaU << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN10 */
#define	ADC_CFGR_BUFSEL_ADC_IN11	(0xbU << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN11 */
#define	ADC_CFGR_BUFSEL_ADC_IN12	(0xcU << ADC_CFGR_BUFSEL_Pos)	/* ADC_IN12 */
#define	ADC_CFGR_BUFSEL_VBAT	(0xeU << ADC_CFGR_BUFSEL_Pos)	/* VBAT */
#define	ADC_CFGR_BUFSEL_TS	(0xfU << ADC_CFGR_BUFSEL_Pos)	/* TS */

#define	CIC_DR_ADC_CIC_DATA_Pos	0	/* CIC滤波器输出数据，更新频率由ADC工作时钟和OSR共同决定；注意这个数据是有符号数，格式为二进制补码。 */
#define	CIC_DR_ADC_CIC_DATA_Msk	(0xffffffU << CIC_DR_ADC_CIC_DATA_Pos)

#define	CIC_OS_ADC_CIC_OS_Pos	0	/* CIC滤波器输出数据offset调整寄存器，软件写入offset值，硬件将ADC_CIC_DR加上OS后可以得到无符号结果 */
#define	CIC_OS_ADC_CIC_OS_Msk	(0xffffffU << CIC_OS_ADC_CIC_OS_Pos)

#define	CIC_USDR_ADC_CIC_USDR_Pos	0	/* ADC_CIC_DR + ADC_CIC_OS得到的无符号数结果 */
#define	CIC_USDR_ADC_CIC_USDR_Msk	(0xffffffU << CIC_USDR_ADC_CIC_USDR_Pos)

#define	CIC_CR_CIC_EN_Pos	31	/* CIC滤波器使能 (CIC enable)
0：关闭CIC
1：使能CIC */
#define	CIC_CR_CIC_EN_Msk	(0x1U << CIC_CR_CIC_EN_Pos)
	/* 关闭CIC */
	/* 使能CIC */

#define	CIC_CR_NS_DISC_Pos	8	/* CIC使能后丢弃的输出点数 (Number of Samples to be Discarded)
可以丢弃ADC使能后开头输出的0~255个采样点
默认值0x4，即丢弃前4个转换结果，从第5个结果开始产生CIC_IF */
#define	CIC_CR_NS_DISC_Msk	(0xffU << CIC_CR_NS_DISC_Pos)

#define	CIC_CR_OVR_IE_Pos	7	/* CIC Overrun Interrupt enable
0：禁止CIC溢出中断
1：允许CIC溢出中断 */
#define	CIC_CR_OVR_IE_Msk	(0x1U << CIC_CR_OVR_IE_Pos)
	/* 禁止CIC溢出中断 */
	/* 允许CIC溢出中断 */

#define	CIC_CR_CIC_IE_Pos	6	/* CIC中断使能 (CIC interrupt enable)
0：禁止CIC中断
1：允许CIC中断，当CIC_IF置位时产生中断事件 */
#define	CIC_CR_CIC_IE_Msk	(0x1U << CIC_CR_CIC_IE_Pos)
	/* 禁止CIC中断 */
	/* 允许CIC中断，当CIC_IF置位时产生中断事件 */

#define	CIC_CR_TRUNC_Pos	3	/* 输出数据截取控制 (Truncate)，配置最终结果中丢弃最低位的位数；
比如TRUNC=0x4，最终结果丢弃最低4bit后存入ADC_CIC_DATA寄存器 */
#define	CIC_CR_TRUNC_Msk	(0x7U << CIC_CR_TRUNC_Pos)

#define	CIC_CR_OSR_Pos	0	/* 过采样率配置 (Over Sampling Rate)
000：x8
001：x16
010：x32
011：x64
100：x128
101：x256
110：x512
111：x1024 */
#define CIC_CR_OSR_Msk	(0x7U << CIC_CR_OSR_Pos)
#define	CIC_CR_OSR_X8	(0x0U << CIC_CR_OSR_Pos)	/* x8 */
#define	CIC_CR_OSR_X16	(0x1U << CIC_CR_OSR_Pos)	/* x16 */
#define	CIC_CR_OSR_X32	(0x2U << CIC_CR_OSR_Pos)	/* x32 */
#define	CIC_CR_OSR_X64	(0x3U << CIC_CR_OSR_Pos)	/* x64 */
#define	CIC_CR_OSR_X128	(0x4U << CIC_CR_OSR_Pos)	/* x128 */
#define	CIC_CR_OSR_X256	(0x5U << CIC_CR_OSR_Pos)	/* x256 */
#define	CIC_CR_OSR_X512	(0x6U << CIC_CR_OSR_Pos)	/* x512 */
#define	CIC_CR_OSR_X1024	(0x7U << CIC_CR_OSR_Pos)	/* x1024 */

#define	CIC_ISR_CIC_OVR_Pos	1
#define	CIC_ISR_CIC_OVR_Msk	(0x1U << CIC_ISR_CIC_OVR_Pos)

#define	CIC_ISR_CIC_IF_Pos	0
#define	CIC_ISR_CIC_IF_Msk	(0x1U << CIC_ISR_CIC_IF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void ADC_Deinit(void);

/* ADC内部累加模式中断使能 相关函数 */
extern void ADC_CR_ADC_IE_Setable(FunState NewState);
extern FunState ADC_CR_ADC_IE_Getable(void);

/* ADC外部累加模式中断使能 相关函数 */
extern void ADC_CR_ACC_IE_Setable(FunState NewState);
extern FunState ADC_CR_ACC_IE_Getable(void);

/* ADC功耗模式
0：低功耗模式，工作时钟最高1MHz
1：高功耗模式，工作时钟最高2MHz 相关函数 */
extern void ADC_CR_HPEN_Set(uint32_t SetValue);
extern uint32_t ADC_CR_HPEN_Get(void);

/* ADC工作模式
0：内部累加器模式
1：外部累加器或CIC模式 相关函数 */
extern void ADC_CR_MODE_Set(uint32_t SetValue);
extern uint32_t ADC_CR_MODE_Get(void);

/* 积分器复位使能，在MODE=1并且使用外部累加器时，必须置位，其他条件下必须保持为0
0：禁止积分器外部复位
1：允许积分器外部复位 相关函数 */
extern void ADC_CR_RSTCTRL_EN_Setable(FunState NewState);
extern FunState ADC_CR_RSTCTRL_EN_Getable(void);

/* ADC使能信号
0：ADC不使能
1：ADC使能

注意：RTC执行自动温补时，硬件会周期性自动使能ADC 相关函数 */
extern void ADC_CR_EN_Setable(FunState NewState);
extern FunState ADC_CR_EN_Getable(void);

/* ADC TRIM值，仅针对内部累加器模式输出
内部累加器累加周期：Period = TRIM*2*TADC_CLK
累加周期决定了输出数据位宽，当TRIM=0x7FF最大值时，实际累加周期为4095 cycle，即ADC输出数据最大有效位宽是12bit 相关函数 */
extern void ADC_TRIM_Write(uint32_t SetValue);
extern uint32_t ADC_TRIM_Read(void);

/* ADC输出数据，为码流累加结果，未经降采样滤波器处理
当MODE=0时，输出为ADC内部累加结果，最高12bit，位数由ADC_TRIM决定
当MODE=1时，输出位ADC外部累加结果，最高14bit，位数由ACC_PERIOD决定 相关函数 */
extern uint32_t ADC_DR_Read(void);

/* ADC初始化完成标志，仅MODE=1时有效（不产生中断）
0：ADC还未完成初始化
1：ADC完成初始化

注：MODE=1，ADC_EN置位后，等待MODE_CTRL_DELAY时间之后，此标志置位，表示ADC内部积分器建立完成。MODE=0时，工作时序由ADC内部自行控制，此标志无效。 相关函数 */
extern FlagStatus ADC_ISR_INIT_RDY_Chk(void);

/* 累加器完成中断标志，硬件置位，软件写1清零，写0无效 相关函数 */
extern void ADC_ISR_ACC_IF_Clr(void);
extern FlagStatus ADC_ISR_ACC_IF_Chk(void);

/* ADC转换完成输出，软件只读（不产生中断）
转换完成后此信号保持为高电平，只有关闭ADC才会清0 相关函数 */
extern FlagStatus ADC_ISR_ADC_DONE_Chk(void);

/* ADC转换完成中断标志，硬件置位，软件写1清零，写0无效 相关函数 */
extern void ADC_ISR_ADC_IF_Clr(void);
extern FlagStatus ADC_ISR_ADC_IF_Chk(void);

/* 外部累加器累加周期配置，单位ADC_CLK
000：1023（对应结果10bit）
001：2047（对应结果11bit）
010：4095（对应结果12bit）
011：8191（对应结果13bit）
100：16383（对应结果14bit）
Others：4095 相关函数 */
extern void ADC_CFGR_ACC_PERIOD_Set(uint32_t SetValue);
extern uint32_t ADC_CFGR_ACC_PERIOD_Get(void);

/* SDADC使能后mode_ctrl延迟长度配置，单位是ADC_CLK周期
0x00：不延迟
0xFF：延迟255个ADC_CLK

注意：复位值为16个时钟周期，不得修改为小于16的数值 相关函数 */
extern void ADC_CFGR_RST_CTRL_DELAY_Set(uint32_t SetValue);
extern uint32_t ADC_CFGR_RST_CTRL_DELAY_Get(void);

/* ADC输入通道Buffer使能 相关函数 */
extern void ADC_CFGR_BUFEN_Setable(FunState NewState);
extern FunState ADC_CFGR_BUFEN_Getable(void);

/* ADC输入通道选择
0000：ADC_IN0
0001：ADC_IN1
0010：ADC_IN2
0011：ADC_IN3
0100：ADC_IN4
0101：ADC_IN5
0110：ADC_IN6
0111：ADC_IN7
1000：ADC_IN8
1001：ADC_IN9
1010：ADC_IN10
1011：ADC_IN11
1100：ADC_IN12
1101：ADC_IN13
1110：VBAT
1111：TS 相关函数 */
extern void ADC_CFGR_BUFSEL_Set(uint32_t SetValue);
extern uint32_t ADC_CFGR_BUFSEL_Get(void);

/* CIC滤波器输出数据，更新频率由ADC工作时钟和OSR共同决定；注意这个数据是有符号数，格式为二进制补码。 相关函数 */
extern uint32_t CIC_DR_Read(void);

/* CIC滤波器输出数据offset调整寄存器，软件写入offset值，硬件将ADC_CIC_DR加上OS后可以得到无符号结果 相关函数 */
extern void ACIC_OS_Write(uint32_t SetValue);
extern uint32_t CIC_OS_Read(void);

/* ADC_CIC_DR + ADC_CIC_OS得到的无符号数结果 相关函数 */
extern uint32_t CIC_USDR_Read(void);

/* CIC滤波器使能 (CIC enable)
0：关闭CIC
1：使能CIC 相关函数 */
extern void CIC_CR_CIC_EN_Setable(FunState NewState);
extern FunState CIC_CR_CIC_EN_Getable(void);

/* CIC使能后丢弃的输出点数 (Number of Samples to be Discarded)
可以丢弃ADC使能后开头输出的0~255个采样点
默认值0x4，即丢弃前4个转换结果，从第5个结果开始产生CIC_IF 相关函数 */
extern void ACIC_CR_NS_DISC_Set(uint32_t SetValue);
extern uint32_t CIC_CR_NS_DISC_Get(void);

/* CIC Overrun Interrupt enable
0：禁止CIC溢出中断
1：允许CIC溢出中断 相关函数 */
extern void CIC_CR_OVR_IE_Setable(FunState NewState);
extern FunState CIC_CR_OVR_IE_Getable(void);

/* CIC中断使能 (CIC interrupt enable)
0：禁止CIC中断
1：允许CIC中断，当CIC_IF置位时产生中断事件 相关函数 */
extern void CIC_CR_CIC_IE_Setable(FunState NewState);
extern FunState IC_CR_CIC_IE_Getable(void);

/* 输出数据截取控制 (Truncate)，配置最终结果中丢弃最低位的位数；
比如TRUNC=0x4，最终结果丢弃最低4bit后存入ADC_CIC_DATA寄存器 相关函数 */
extern void CIC_CR_TRUNC_Set(uint32_t SetValue);
extern uint32_t CIC_CR_TRUNC_Get(void);

/* 过采样率配置 (Over Sampling Rate)
000：x8
001：x16
010：x32
011：x64
100：x128
101：x256
110：x512
111：x1024 相关函数 */
extern void CIC_CR_OSR_Set(uint32_t SetValue);
extern uint32_t CIC_CR_OSR_Get(void);
extern void CIC_ISR_CIC_OVR_Clr(void);
extern FlagStatus CIC_ISR_CIC_OVR_Chk(void);
extern void CIC_ISR_CIC_IF_Clr(void);
extern FlagStatus CIC_ISR_CIC_IF_Chk(void);

#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_ADC_H */
