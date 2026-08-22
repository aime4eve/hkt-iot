/**
  ******************************************************************************
  * @file    fm33a0xxev_et.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the ET firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_ET_H
#define __FM33A0XXEV_ET_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	ETx_CR_OPOL_Pos	10	/* 输出极性选择 (Output Polarity)
0：输出不取反
1：输出取反 */
#define	ETx_CR_OPOL_Msk	(0x1U << ETx_CR_OPOL_Pos)
#define	ETx_CR_OPOL_NORMAL	(0x0U << ETx_CR_OPOL_Pos)	/* 输出不取反 */
#define	ETx_CR_OPOL_REVERSE	(0x1U << ETx_CR_OPOL_Pos)	/* 输出取反 */

#define	ETx_CR_EXFLT_Pos	9	/* 外部引脚输入数字滤波使能
1 = 打开引脚输入信号数字滤波
0 = 关闭引脚输入信号数字滤波 */
#define	ETx_CR_EXFLT_Msk	(0x1U << ETx_CR_EXFLT_Pos)
#define	ETx_CR_EXFLT_OPEN	(0x1U << ETx_CR_EXFLT_Pos)
#define	ETx_CR_EXFLT_CLOSE	(0x0U << ETx_CR_EXFLT_Pos)

#define	ETx_CR_PWM_Pos	8	/* PWM输出控制
1 = PWM输出使能
0 = PWM输出禁止 */
#define	ETx_CR_PWM_Msk	(0x1U << ETx_CR_PWM_Pos)

#define	ETx_CR_CEN_Pos	7	/* 启动控制
1 = 启动定时器，在计数器模式下启动时将计数初值加载至计数器和工作寄存器；在捕捉模式下，启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数
0 = 停止计数器计数 */
#define	ETx_CR_CEN_Msk	(0x1U << ETx_CR_CEN_Pos)

#define	ETx_CR_MOD_Pos	6	/* 工作模式选择
1 = 捕捉模式
0 = 定时/计数模式 */
#define	ETx_CR_MOD_Msk	(0x1U << ETx_CR_MOD_Pos)
#define	ETx_CR_MOD_CAPTURE	(0x1U << ETx_CR_MOD_Pos)
#define	ETx_CR_MOD_COUNTER	(0x0U << ETx_CR_MOD_Pos)

#define	ETx_CR_CASEN_Pos	5	/* Cascade Enable，扩展定时器级联使能
1 = ET1（ET3）和ET2（ET4）级联成32bit定时器
0 = 16bit定时器独立工作 */
#define	ETx_CR_CASEN_Msk	(0x1U << ETx_CR_CASEN_Pos)
#define	ETx_CR_CASEN_32BITS	(0x1U << ETx_CR_CASEN_Pos)
#define	ETx_CR_CASEN_16BITS	(0x0U << ETx_CR_CASEN_Pos)

#define	ETx_CR_EDGESEL_Pos	4	/* 计数模式采沿方式选择
（计数时钟选择mcu_clk时该位无效，总是采用mcu_clk时钟上升沿计数）
1 = 计数模式采下降沿
0 = 计数模式采上升沿 */
#define	ETx_CR_EDGESEL_Msk	(0x1U << ETx_CR_EDGESEL_Pos)
#define	ETx_CR_EDGESEL_FALLING	(0x1U << ETx_CR_EDGESEL_Pos)
#define	ETx_CR_EDGESEL_RISING	(0x0U << ETx_CR_EDGESEL_Pos)

#define	ETx_CR_CAPMOD_Pos	3	/* 捕捉模式控制
1 = 脉宽捕捉
0 = 脉冲周期捕捉 */
#define	ETx_CR_CAPMOD_Msk	(0x1U << ETx_CR_CAPMOD_Pos)
#define	ETx_CR_CAPMOD_PULSE	(0x1U << ETx_CR_CAPMOD_Pos)
#define	ETx_CR_CAPMOD_CYCLE	(0x0U << ETx_CR_CAPMOD_Pos)

#define	ETx_CR_CAPCLR_Pos	2	/* 带清零捕捉模式控制
1 = 事件触发捕捉：使能后计数器保持0，捕捉到第一个有效沿之后timer才开始计数
0 = 捕捉不清零，计数器一直自由计数 */
#define	ETx_CR_CAPCLR_Msk	(0x1U << ETx_CR_CAPCLR_Pos)
	/* 使能后计数器保持0，捕捉到第一个有效沿之后timer才开始计数 */

#define	ETx_CR_CAPONCE_Pos	1	/* 单次捕捉控制
1 = 单次捕捉有效，在捕捉到一次脉冲周期后计数器停止，若需要再次捕捉需重新启动
0 = 连续捕捉 */
#define	ETx_CR_CAPONCE_Msk	(0x1U << ETx_CR_CAPONCE_Pos)
#define	ETx_CR_CAPONCE_SINGLE	(0x1U << ETx_CR_CAPONCE_Pos)
#define	ETx_CR_CAPONCE_CONTINUE	(0x0U << ETx_CR_CAPONCE_Pos)

#define	ETx_CR_CAPEDGE_Pos	0	/* 捕捉沿选择
1 = 周期捕捉模式时下沿捕捉
0 = 周期捕捉模式时上沿捕捉 */
#define	ETx_CR_CAPEDGE_Msk	(0x1U << ETx_CR_CAPEDGE_Pos)
#define	ETx_CR_CAPEDGE_FALLING	(0x1U << ETx_CR_CAPEDGE_Pos)
#define	ETx_CR_CAPEDGE_RISING	(0x0U << ETx_CR_CAPEDGE_Pos)

#define	ETx_INSR_SIG2SEL_Pos	7	/* 内部信号2源选择(捕捉源)
1 = 扩展定时器3的内部信号2选择Group1
0 = 扩展定时器3的内部信号2选择Group2 */
#define	ETx_INSR_SIG2SEL_Msk	(0x1U << ETx_INSR_SIG2SEL_Pos)
#define	ETx_INSR_SIG2SEL_GROUP1	(0x1U << ETx_INSR_SIG2SEL_Pos)
#define	ETx_INSR_SIG2SEL_GROUP2	(0x0U << ETx_INSR_SIG2SEL_Pos)

#define	ETx_INSR_SIG1SEL_Pos	6	/* 内部信号1源选择（在计数模式下计数源仅由此选择，捕捉模式下计数源）
1 = 扩展定时器3的内部信号1选择Group2
0 = 扩展定时器3的内部信号1选择Group1 */
#define	ETx_INSR_SIG1SEL_Msk	(0x1U << ETx_INSR_SIG1SEL_Pos)
#define	ETx_INSR_SIG1SEL_GROUP2	(0x1U << ETx_INSR_SIG1SEL_Pos)
#define	ETx_INSR_SIG1SEL_GROUP1	(0x0U << ETx_INSR_SIG1SEL_Pos)

#define	ETx_INSR_GRP2SEL_Pos	2	/* GROUP2 信号选择控制
ET1
000 = UART0_RX
001 = UART1_RX
010 = XTLF
011 = ET1_IN1
100 = ET1_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET2
000 = UART2_RX
001 = UART3_RX
010 = XTLF
011 = ET2_IN1
100 = ET2_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET3
000 = ET3_IN1
001 = XTLF 
010 = UART4_RX
011 = UART5_RX
100 = RTCSEC
101~111 = RFU
ET4
000 = ET4_IN1
001 = XTLF
010 = UART_RX2
011 = UART_RX0
100 = CMP1O（比较器1输出）
101 = CMP2O（比较器2输出）
110= RTCSEC
111= RCMF? */
#define	ETx_INSR_GRP2SEL_Msk	(0x7U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_UART0_RX	(0x0U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_UART1_RX	(0x1U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_XTLF	(0x2U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_IN1	(0x3U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_IN2	(0x4U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_CMP1O	(0x5U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_CMP2O	(0x6U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET1_RCMF	(0x7U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_UART2_RX	(0x0U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_UART3_RX	(0x1U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_XTLF	(0x2U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_IN1	(0x3U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_IN2	(0x4U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_CMP1O	(0x5U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_CMP2O	(0x6U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET2_RCMF	(0x7U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET3_IN1	(0x0U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET3_XTLF 	(0x1U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET3_UART4_RX	(0x2U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET3_UART5_RX	(0x3U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET3_RTCSEC	(0x4U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_IN	(0x0U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_XTLF	(0x1U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_UART_RX2	(0x2U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_UART_RX0	(0x3U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_CMP1O	(0x4U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_CMP2O	(0x5U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_RTCSEC	(0x6U << ETx_INSR_GRP2SEL_Pos)
#define	ETx_INSR_GRP2SEL_ET4_RCMF	(0x7U << ETx_INSR_GRP2SEL_Pos)

#define	ETx_INSR_GRP1SEL_Pos	0	/* GROUP1 信号选择控制
ET1
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET1_IN0
ET2
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET2_IN0
ET3
00 = APBCLK
01 = ET3_IN0
10 = RTCSEC
11 = RCLP
ET4
00 = APBCLK
01 = ET4_IN0
10 = RTC64HZ
11 = RCMF */
#define	ETx_INSR_GRP1SEL_Msk	(0x3U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET1_APBCLK	(0x0U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET1_XTLF	(0x1U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET1_RCLP	(0x2U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET1_IN0	(0x3U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET2_APBCLK	(0x0U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET2_XTLF	(0x1U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET2_RCLP	(0x2U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET2_IN0	(0x3U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET3_APBCLK	(0x0U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET3_IN0	(0x1U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET3_RTCSEC	(0x2U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET3_RCLP	(0x3U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET4_APBCLK	(0x0U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET4_IN0	(0x1U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET4_RTC64HZ	(0x2U << ETx_INSR_GRP1SEL_Pos)
#define	ETx_INSR_GRP1SEL_ET4_RCMF	(0x3U << ETx_INSR_GRP1SEL_Pos)

#define	ETx_PSCR1_PRESCALE1_Pos	0	/* 输入Signal1（计数源）的预分频寄存器
00表示1分频，FF表示256分频 */
#define	ETx_PSCR1_PRESCALE1_Msk	(0xffU << ETx_PSCR1_PRESCALE1_Pos)

#define	ETx_PSCR2_PRESCALE2_Pos	0	/* 输入Signal2（捕捉源）的预分频寄存器
00表示1分频，FF表示256分频。 */
#define	ETx_PSCR2_PRESCALE2_Msk	(0xffU << ETx_PSCR2_PRESCALE2_Pos)

#define	ETx_IVR_INITVALUE_Pos	0	/* 扩展定时器初值寄存器 */
#define	ETx_IVR_INITVALUE_Msk	(0xffffU << ETx_IVR_INITVALUE_Pos)

#define	ETx_CMPR_CMP_Pos	0	/* 扩展定时器比较寄存器
该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 */
#define	ETx_CMPR_CMP_Msk	(0xffffU << ETx_CMPR_CMP_Pos)

#define	ETx_IER_CMPIE_Pos	2	/* 扩展定时器比较中断使能
1 = 使能
0 = 禁止 */
#define	ETx_IER_CMPIE_Msk	(0x1U << ETx_IER_CMPIE_Pos)

#define	ETx_IER_CAPIE_Pos	1	/* 扩展定时器捕捉中断使能
1 = 使能
0 = 禁止 */
#define	ETx_IER_CAPIE_Msk	(0x1U << ETx_IER_CAPIE_Pos)

#define	ETx_IER_OVIE_Pos	0	/* 扩展定时器3溢出中断使能
1 = 使能
0 = 禁止 */
#define	ETx_IER_OVIE_Msk	(0x1U << ETx_IER_OVIE_Pos)

#define	ETx_ISR_CMPIF_Pos	3	/* 比较状态，写1清0
1 =当前计数器的值大于等于比较寄存器的值
0 =当前计数器的值小于比较寄存器的值 */
#define	ETx_ISR_CMPIF_Msk	(0x1U << ETx_ISR_CMPIF_Pos)

#define	ETx_ISR_CAPIF_Pos	1	/* 扩展定时器捕捉产生信号
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 */
#define	ETx_ISR_CAPIF_Msk	(0x1U << ETx_ISR_CAPIF_Pos)

#define	ETx_ISR_OVIF_Pos	0	/* 扩展定时器3溢出信号，当计数器的值由0xFFFF再增加时将置位
1 = 产生计数溢出
0 = 未产生溢出 */
#define	ETx_ISR_OVIF_Msk	(0x1U << ETx_ISR_OVIF_Pos)

#define	ETCNTx_CNT_Pos	0	
#define	ETCNTx_CNT_Msk	(0xffffU << ETCNTx_CNT_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void ETx_Deinit(ET_Type* ETx);

/* 输出极性选择 (Output Polarity)
0：输出不取反
1：输出取反 相关函数 */
extern void ETx_CR_OPOL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_OPOL_Get(ET_Type* ETx);

/* 外部引脚输入数字滤波使能
1 = 打开引脚输入信号数字滤波
0 = 关闭引脚输入信号数字滤波 相关函数 */
extern void ETx_CR_EXFLT_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_EXFLT_Get(ET_Type* ETx);

/* PWM输出控制
1 = PWM输出使能
0 = PWM输出禁止 相关函数 */
extern void ETx_CR_PWM_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_CR_PWM_Getable(ET_Type* ETx);

/* 启动控制
1 = 启动定时器，在计数器模式下启动时将计数初值加载至计数器和工作寄存器；在捕捉模式下，启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数
0 = 停止计数器计数 相关函数 */
extern void ETx_CR_CEN_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_CR_CEN_Getable(ET_Type* ETx);

/* 工作模式选择
1 = 捕捉模式
0 = 定时/计数模式 相关函数 */
extern void ETx_CR_MOD_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_MOD_Get(ET_Type* ETx);

/* Cascade Enable，扩展定时器级联使能
1 = ET1（ET3）和ET2（ET4）级联成32bit定时器
0 = 16bit定时器独立工作 相关函数 */
extern void ETx_CR_CASEN_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_CASEN_Get(ET_Type* ETx);

/* 计数模式采沿方式选择
（计数时钟选择mcu_clk时该位无效，总是采用mcu_clk时钟上升沿计数）
1 = 计数模式采下降沿
0 = 计数模式采上升沿 相关函数 */
extern void ETx_CR_EDGESEL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_EDGESEL_Get(ET_Type* ETx);

/* 捕捉模式控制
1 = 脉宽捕捉
0 = 脉冲周期捕捉 相关函数 */
extern void ETx_CR_CAPMOD_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_CAPMOD_Get(ET_Type* ETx);

/* 带清零捕捉模式控制
1 = 事件触发捕捉：使能后计数器保持0，捕捉到第一个有效沿之后timer才开始计数
0 = 捕捉不清零，计数器一直自由计数 相关函数 */
extern void ETx_CR_CAPCLR_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_CR_CAPCLR_Getable(ET_Type* ETx);

/* 单次捕捉控制
1 = 单次捕捉有效，在捕捉到一次脉冲周期后计数器停止，若需要再次捕捉需重新启动
0 = 连续捕捉 相关函数 */
extern void ETx_CR_CAPONCE_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_CAPONCE_Get(ET_Type* ETx);

/* 捕捉沿选择
1 = 周期捕捉模式时下沿捕捉
0 = 周期捕捉模式时上沿捕捉 相关函数 */
extern void ETx_CR_CAPEDGE_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CR_CAPEDGE_Get(ET_Type* ETx);

/* 内部信号2源选择(捕捉源)
1 = 扩展定时器3的内部信号2选择Group1
0 = 扩展定时器3的内部信号2选择Group2 相关函数 */
extern void ETx_INSR_SIG2SEL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_INSR_SIG2SEL_Get(ET_Type* ETx);

/* 内部信号1源选择（在计数模式下计数源仅由此选择，捕捉模式下计数源）
1 = 扩展定时器3的内部信号1选择Group2
0 = 扩展定时器3的内部信号1选择Group1 相关函数 */
extern void ETx_INSR_SIG1SEL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_INSR_SIG1SEL_Get(ET_Type* ETx);

/* GROUP2 信号选择控制
ET1
000 = UART0_RX
001 = UART1_RX
010 = XTLF
011 = ET1_IN1
100 = ET1_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET2
000 = UART2_RX
001 = UART3_RX
010 = XTLF
011 = ET2_IN1
100 = ET2_IN2
101 = CMP1O（比较器1输出）
110 = CMP2O（比较器2输出）
111 = RCMF
ET3
000 = ET3_IN1
001 = XTLF 
010 = UART4_RX
011 = UART5_RX
100 = RTCSEC
101~111 = RFU
ET4
000 = ET4_IN1
001 = XTLF
010 = UART_RX2
011 = UART_RX0
100 = CMP1O（比较器1输出）
101 = CMP2O（比较器2输出）
110= RTCSEC
111= RCMF? 相关函数 */
extern void ETx_INSR_GRP2SEL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_INSR_GRP2SEL_Get(ET_Type* ETx);

/* GROUP1 信号选择控制
ET1
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET1_IN0
ET2
00 = APBCLK
01 = XTLF
10 = RCLP
11 = ET2_IN0
ET3
00 = APBCLK
01 = ET3_IN0
10 = RTCSEC
11 = RCLP
ET4
00 = APBCLK
01 = ET4_IN0
10 = RTC64HZ
11 = RCMF 相关函数 */
extern void ETx_INSR_GRP1SEL_Set(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_INSR_GRP1SEL_Get(ET_Type* ETx);

/* 输入Signal1（计数源）的预分频寄存器
00表示1分频，FF表示256分频 相关函数 */
extern void ETx_PSCR1_Write(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_PSCR1_Read(ET_Type* ETx);

/* 输入Signal2（捕捉源）的预分频寄存器
00表示1分频，FF表示256分频。 相关函数 */
extern void ETx_PSCR2_Write(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_PSCR2_Read(ET_Type* ETx);

/* 扩展定时器初值寄存器 相关函数 */
extern void ETx_IVR_Write(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_IVR_Read(ET_Type* ETx);

/* 扩展定时器比较寄存器
该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
extern void ETx_CMPR_Write(ET_Type* ETx, uint32_t SetValue);
extern uint32_t ETx_CMPR_Read(ET_Type* ETx);

/* 扩展定时器比较中断使能
1 = 使能
0 = 禁止 相关函数 */
extern void ETx_IER_CMPIE_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_IER_CMPIE_Getable(ET_Type* ETx);

/* 扩展定时器捕捉中断使能
1 = 使能
0 = 禁止 相关函数 */
extern void ETx_IER_CAPIE_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_IER_CAPIE_Getable(ET_Type* ETx);

/* 扩展定时器3溢出中断使能
1 = 使能
0 = 禁止 相关函数 */
extern void ETx_IER_OVIE_Setable(ET_Type* ETx, FunState NewState);
extern FunState ETx_IER_OVIE_Getable(ET_Type* ETx);

/* 比较状态，写1清0
1 =当前计数器的值大于等于比较寄存器的值
0 =当前计数器的值小于比较寄存器的值 相关函数 */
extern void ETx_ISR_CMPIF_Clr(ET_Type* ETx);
extern FlagStatus ETx_ISR_CMPIF_Chk(ET_Type* ETx);

/* 扩展定时器捕捉产生信号
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 相关函数 */
extern void ETx_ISR_CAPIF_Clr(ET_Type* ETx);
extern FlagStatus ETx_ISR_CAPIF_Chk(ET_Type* ETx);

/* 扩展定时器3溢出信号，当计数器的值由0xFFFF再增加时将置位
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
extern void ETx_ISR_OVIF_Clr(ET_Type* ETx);
extern FlagStatus ETx_ISR_OVIF_Chk(ET_Type* ETx);

/*获取ET 计数器数值*/
extern uint32_t ETxCNT_CNTx_Read(ETCNT_Type* ETIMx);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_ET_H */
