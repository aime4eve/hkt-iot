/**
  ******************************************************************************
  * @file    fm33a0xxev_bt.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the BT firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_BT_H
#define __FM33A0XXEV_BT_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	BTx_CR1_CHEN_Pos	7	/* 高8位计数器（BT1H或BT2H）启动控制  */
#define	BTx_CR1_CHEN_Msk	(0x1U << BTx_CR1_CHEN_Pos)

#define	BTx_CR1_CLEN_Pos	6	/* 低8位计数器（BT1L或BT2L）启动控制
(Counter-Lowend enable)
1：启动低8bit计数器，在计数器模式下启动时将预置数值和加载值分别加载至计数器和比较寄存器；捕捉模式下启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数，捕捉功能只工作在16位模式；在16位的定时/计数和捕捉模式下CHEN作为计数器的启动控制，CLEN自动失效
0：停止低8bit计数器计数 */
#define	BTx_CR1_CLEN_Msk	(0x1U << BTx_CR1_CLEN_Pos)
	/* 启动低8bit计数器，在计数器模式下启动时将预置数值和加载值分别加载至计数器和比较寄存器；捕捉模式下启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数，捕捉功能只工作在16位模式；在16位的定时/计数和捕捉模式下CHEN作为计数器的启动控制，CLEN自动失效 */
	/* 停止低8bit计数器计数 */

#define	BTx_CR1_MODE_Pos	5	/* 工作模式选择 (work mode)
1：16位捕捉模式
0：8位定时/计数模式，若高位计数器计数源选择为低位计数器的溢出信号，则可实现16位定时/计数模式 */
#define	BTx_CR1_MODE_Msk	(0x1U << BTx_CR1_MODE_Pos)
#define	BTx_CR1_MODE_CAPTURE	(0x1U << BTx_CR1_MODE_Pos)	/* 16位捕捉模式 */
#define	BTx_CR1_MODE_COUNTER	(0x0U << BTx_CR1_MODE_Pos)	/* 8位定时/计数模式，若高位计数器计数源选择为低位计数器的溢出信号，则可实现16位定时/计数模式 */

#define	BTx_CR1_EDGESEL_Pos	4	/* 计数模式下的计数沿和周期捕捉时的捕捉沿选择位 (edge select)
1：计数模式采样计数源下降沿，周期捕捉模式时下沿捕捉
0：计数模式采样计数源上升沿，周期捕捉模式时上沿捕捉
注：不支持系统时钟的下降沿计数，捕捉源和计数源为系统时钟时选择下降沿将不会有效计数。 */
#define	BTx_CR1_EDGESEL_Msk	(0x1U << BTx_CR1_EDGESEL_Pos)
#define	BTx_CR1_EDGESEL_FALLING	(0x1U << BTx_CR1_EDGESEL_Pos)	/* 计数模式采样计数源下降沿，周期捕捉模式时下沿捕捉 */
#define	BTx_CR1_EDGESEL_RISING	(0x0U << BTx_CR1_EDGESEL_Pos)	/* 计数模式采样计数源上升沿，周期捕捉模式时上沿捕捉 */

#define	BTx_CR1_CAPMOD_Pos	3	/* 捕捉模式控制（只在捕捉模式下有效）(capture mode)
1：脉冲宽度捕捉
0：脉冲周期捕捉 */
#define	BTx_CR1_CAPMOD_Msk	(0x1U << BTx_CR1_CAPMOD_Pos)
#define	BTx_CR1_CAPMOD_PULSE	(0x1U << BTx_CR1_CAPMOD_Pos)	/* 脉冲宽度捕捉 */
#define	BTx_CR1_CAPMOD_CYCLE	(0x0U << BTx_CR1_CAPMOD_Pos)	/* 脉冲周期捕捉 */

#define	BTx_CR1_CAPCLR_Pos	2	/* 带清零捕捉模式控制 (capture clear)
1：不论在脉冲宽度还是周期捕捉情况下，捕捉到第一个沿后将计数器清零产生中断，捕捉到第二个沿后锁存（锁存到高低位预置数寄存器）计数值并同时清零计数器
0：捕捉不清零，计数器一直自由计数 */
#define	BTx_CR1_CAPCLR_Msk	(0x1U << BTx_CR1_CAPCLR_Pos)
	/* 不论在脉冲宽度还是周期捕捉情况下，捕捉到第一个沿后将计数器清零产生中断，捕捉到第二个沿后锁存（锁存到高低位预置数寄存器）计数值并同时清零计数器 */
	/* 捕捉不清零，计数器一直自由计数 */

#define	BTx_CR1_CAPONCE_Pos	1	/* 单次捕捉控制 (capture once)
1：单次捕捉有效，在捕捉到一次脉冲宽度或脉冲周期后计数器停止，若需要再次捕捉需重新启动
0：连续捕捉 */
#define	BTx_CR1_CAPONCE_Msk	(0x1U << BTx_CR1_CAPONCE_Pos)
#define	BTx_CR1_CAPONCE_SINGLE	(0x1U << BTx_CR1_CAPONCE_Pos)	/* 单次捕捉有效，在捕捉到一次脉冲宽度或脉冲周期后计数器停止，若需要再次捕捉需重新启动 */
#define	BTx_CR1_CAPONCE_CONTINUE	(0x0U << BTx_CR1_CAPONCE_Pos)	/* 连续捕捉 */

#define	BTx_CR1_PWM_Pos	0	/* PWM模式输出 (pulse width modulation)
1：PWM输出使能
0：PWM 输出不使能 */
#define	BTx_CR1_PWM_Msk	(0x1U << BTx_CR1_PWM_Pos)
	/* PWM输出使能 */
	/* PWM 输出不使能 */

#define	BTx_CR2_SIG2SEL_Pos	7	/* 计数器内部计数源信号选择 (signal group2 select)
1：内部计数源信号选择Group2
0：内部计数源信号选择Group1 */
#define	BTx_CR2_SIG2SEL_Msk	(0x1U << BTx_CR2_SIG2SEL_Pos)
#define	BTx_CR2_SIG2SEL_GROUP2	(0x1U << BTx_CR2_SIG2SEL_Pos)	/* 内部计数源信号选择Group2 */
#define	BTx_CR2_SIG2SEL_GROUP1	(0x0U << BTx_CR2_SIG2SEL_Pos)	/* 内部计数源信号选择Group1 */

#define	BTx_CR2_SIG1SEL_Pos	6	/* 计数器内部捕捉源信号选择 (signal group1 select)
1：内部捕捉源信号选择Group1
0：内部捕捉源信号选择Group2 */
#define	BTx_CR2_SIG1SEL_Msk	(0x1U << BTx_CR2_SIG1SEL_Pos)
#define	BTx_CR2_SIG1SEL_GROUP1	(0x1U << BTx_CR2_SIG1SEL_Pos)	/* 内部捕捉源信号选择Group1 */
#define	BTx_CR2_SIG1SEL_GROUP2	(0x0U << BTx_CR2_SIG1SEL_Pos)	/* 内部捕捉源信号选择Group2 */

#define	BTx_CR2_CNTHSEL_Pos	4	/* 高8位计数器计数源选择 (Counter Highend source select)
00/11：选择ET1的低位计数器的溢出信号，与低位计数器组成16位计数器
01：选择内部捕捉源信号
10：选择内部计数源信号或外部DIR输入组合信号 */
#define	BTx_CR2_CNTHSEL_Msk	(0x3U << BTx_CR2_CNTHSEL_Pos)
#define	BTx_CR2_CNTHSEL_COUNTER	(0x0U << BTx_CR2_CNTHSEL_Pos)	/* 选择ET1的低位计数器的溢出信号，与低位计数器组成16位计数器 */
#define	BTx_CR2_CNTHSEL_CAPTURE	(0x1U << BTx_CR2_CNTHSEL_Pos)	/* 选择内部捕捉源信号 */
#define	BTx_CR2_CNTHSEL_INNER	(0x2U << BTx_CR2_CNTHSEL_Pos)	/* 选择内部计数源信号或外部DIR输入组合信号 */

#define	BTx_CR2_DIREN_Pos	3	/* 外部输入DIR控制使能。通常电量脉冲输出时同时会输出一个由高低电平指示正反向的方向信号DIR。电路将通过DIR信号电平的高低，分别控制高位计数器和低位计数器计数使能，以实现针对正向、反向脉冲的各种计数功能 (direction bit enable)
1：外部输入的DIR信号有效，此时高低位计数器是否计数可由外部输入的DIR信号控制。
0：外部输入的DIR信号无效，此时高低位计数器是否计数将由内部控制信号控制。 */
#define	BTx_CR2_DIREN_Msk	(0x1U << BTx_CR2_DIREN_Pos)
	/* 外部输入的DIR信号有效，此时高低位计数器是否计数可由外部输入的DIR信号控制。 */
	/* 外部输入的DIR信号无效，此时高低位计数器是否计数将由内部控制信号控制。 */

#define	BTx_CR2_STDIR_Pos	2	/* 内部DIR控制信号，当DIREN为0，即外部输入DIR控制无效时，可由该信号代替DIR输入，直接控制内部计数器的计数。当需要外部DIR输入，即DIREN为1时，该位应设置为0 (set direction)
1：内部DIR信号为高电平，则高8位计数器计数 */
#define	BTx_CR2_STDIR_Msk	(0x1U << BTx_CR2_STDIR_Pos)
	/* 内部DIR信号为高电平，则高8位计数器计数 */

#define	BTx_CR2_SRCSEL_Pos	1	/* 低位计数器计数使能控制选择信号 (source select)
1低位计数器计数使能端选则常使能。此时低位计数器计数不受DIR控制，可将正反向所有脉冲一并计数
0：低位计数器计数使能端选则由寄存器STDIR或外部EX_SIG2输入控制。 */
#define	BTx_CR2_SRCSEL_Msk	(0x1U << BTx_CR2_SRCSEL_Pos)
	/* 低位计数器计数使能端选则由寄存器STDIR或外部EX_SIG2输入控制。 */

#define	BTx_CR2_DIRPO_Pos	0	/* 输入信号2极性选择 (direction polarity)
1：对外部输入DIR信号EX_SIG2反向
0：对外部输入DIR信号EX_SIG2不反向 */
#define	BTx_CR2_DIRPO_Msk	(0x1U << BTx_CR2_DIRPO_Pos)
#define	BTx_CR2_DIRPO_REVERSE	(0x1U << BTx_CR2_DIRPO_Pos)	/* 对外部输入DIR信号EX_SIG2反向 */
#define	BTx_CR2_DIRPO_NON	(0x0U << BTx_CR2_DIRPO_Pos)	/* 对外部输入DIR信号EX_SIG2不反向 */

#define	BTx_CFGR1_RTCSEL2_Pos	6	/* RTCOUT2信号选择控制2 (RTCOUT2 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU */
#define	BTx_CFGR1_RTCSEL2_Msk	(0x3U << BTx_CFGR1_RTCSEL2_Pos)
#define	BTx_CFGR1_RTCSEL2_32768HZ	(0x0U << BTx_CFGR1_RTCSEL2_Pos)
#define	BTx_CFGR1_RTCSEL2_RTC_SEC	(0x1U << BTx_CFGR1_RTCSEL2_Pos)
#define	BTx_CFGR1_RTCSEL2_RTC_MIN	(0x2U << BTx_CFGR1_RTCSEL2_Pos)

#define	BTx_CFGR1_RTCSEL1_Pos	4	/* RTCOUT1信号选择控制1 (RTCOUT1 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU */
#define	BTx_CFGR1_RTCSEL1_Msk	(0x3U << BTx_CFGR1_RTCSEL1_Pos)
#define	BTx_CFGR1_RTCSEL1_32768HZ	(0x0U << BTx_CFGR1_RTCSEL1_Pos)
#define	BTx_CFGR1_RTCSEL1_RTC_SEC	(0x1U << BTx_CFGR1_RTCSEL1_Pos)
#define	BTx_CFGR1_RTCSEL1_RTC_MIN	(0x2U << BTx_CFGR1_RTCSEL1_Pos)

#define	BTx_CFGR1_GRP2SEL_Pos	2	/* Group2信号选择控制 (Group2 source select)
00 = APBCLK
01 = RTCOUT2
10 = IN_SIG2，内部输入信号2
11 = EX_SIG2，外部输入信号2
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 */
#define	BTx_CFGR1_GRP2SEL_Msk	(0x3U << BTx_CFGR1_GRP2SEL_Pos)
#define	BTx_CFGR1_GRP2SEL_APBCLK	(0x0U << BTx_CFGR1_GRP2SEL_Pos)	/* 不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 */
#define	BTx_CFGR1_GRP2SEL_RTCOUT2	(0x1U << BTx_CFGR1_GRP2SEL_Pos)
#define	BTx_CFGR1_GRP2SEL_IN_SIG2	(0x2U << BTx_CFGR1_GRP2SEL_Pos)
#define	BTx_CFGR1_GRP2SEL_EX_SIG2	(0x3U << BTx_CFGR1_GRP2SEL_Pos)

#define	BTx_CFGR1_GRP1SEL_Pos	0	/* Group1信号选择控制（可作为捕捉模式下采样时钟选择，同时可作为信号捕捉源）(Group1 source select)
00 = APBCLK
01 = RTCOUT1
10 = IN_SIG1，内部输入信号1
11 = EX_SIG1，外部输入信号1
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 */
#define	BTx_CFGR1_GRP1SEL_Msk	(0x3U << BTx_CFGR1_GRP1SEL_Pos)
#define	BTx_CFGR1_GRP1SEL_APBCLK	(0x0U << BTx_CFGR1_GRP1SEL_Pos)	/* 不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 */
#define	BTx_CFGR1_GRP1SEL_RTCOUT1	(0x1U << BTx_CFGR1_GRP1SEL_Pos)
#define	BTx_CFGR1_GRP1SEL_IN_SIG1	(0x2U << BTx_CFGR1_GRP1SEL_Pos)
#define	BTx_CFGR1_GRP1SEL_EX_SIG1	(0x3U << BTx_CFGR1_GRP1SEL_Pos)

#define	BTx_CFGR2_EXSEL2_Pos	6	/* 外部输入信号选择控制2 (external source select2)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 */
#define	BTx_CFGR2_EXSEL2_Msk	(0x3U << BTx_CFGR2_EXSEL2_Pos)
#define	BTx_CFGR2_EXSEL2_IN0	(0x0U << BTx_CFGR2_EXSEL2_Pos)
#define	BTx_CFGR2_EXSEL2_IN1	(0x1U << BTx_CFGR2_EXSEL2_Pos)
#define	BTx_CFGR2_EXSEL2_IN2	(0x2U << BTx_CFGR2_EXSEL2_Pos)
#define	BTx_CFGR2_EXSEL2_IN3	(0x3U << BTx_CFGR2_EXSEL2_Pos)

#define	BTx_CFGR2_EXSEL1_Pos	4	/* 外部输入信号选择控制1 (external source select1)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 */
#define	BTx_CFGR2_EXSEL1_Msk	(0x3U << BTx_CFGR2_EXSEL1_Pos)
#define	BTx_CFGR2_EXSEL1_IN0	(0x0U << BTx_CFGR2_EXSEL1_Pos)
#define	BTx_CFGR2_EXSEL1_IN1	(0x1U << BTx_CFGR2_EXSEL1_Pos)
#define	BTx_CFGR2_EXSEL1_IN2	(0x2U << BTx_CFGR2_EXSEL1_Pos)
#define	BTx_CFGR2_EXSEL1_IN3	(0x3U << BTx_CFGR2_EXSEL1_Pos)

#define	BTx_CFGR2_INSEL2_Pos	2	/* 内部输入信号选择控制2 (internal source select 2)
00 = UART_RX3/UART_RX3
01 = UART_RX4/UART_RX4
10 = UART_RX5/UART_RX5
11 = RCLP/BT1_OUT */
#define	BTx_CFGR2_INSEL2_Msk	(0x3U << BTx_CFGR2_INSEL2_Pos)
#define	BTx_CFGR2_INSEL2_RX3	(0x0U << BTx_CFGR2_INSEL2_Pos)
#define	BTx_CFGR2_INSEL2_RX4	(0x1U << BTx_CFGR2_INSEL2_Pos)
#define	BTx_CFGR2_INSEL2_RX5	(0x2U << BTx_CFGR2_INSEL2_Pos)
#define	BTx_CFGR2_INSEL2_RCLP	(0x3U << BTx_CFGR2_INSEL2_Pos)

#define	BTx_CFGR2_INSEL1_Pos	0	/* 内部输入信号选择控制1 (internal source select 1)
00 = UART_RX0/UART_RX0
01 = UART_RX1/UART_RX1
10 = UART_RX2/UART_RX2
11 = RCLP/RCLP */
#define	BTx_CFGR2_INSEL1_Msk	(0x3U << BTx_CFGR2_INSEL1_Pos)
#define	BTx_CFGR2_INSEL1_RX0	(0x0U << BTx_CFGR2_INSEL1_Pos)
#define	BTx_CFGR2_INSEL1_RX1	(0x1U << BTx_CFGR2_INSEL1_Pos)
#define	BTx_CFGR2_INSEL1_RX2	(0x2U << BTx_CFGR2_INSEL1_Pos)
#define	BTx_CFGR2_INSEL1_RCLP	(0x3U << BTx_CFGR2_INSEL1_Pos)

#define	BTx_PRES_PRESCALE_Pos	0	/* 输入Group1的预分频寄存器 (Group1 input signal prescaler)
分频数=（X+1），即00表示1分频，FF表示256分频。预分频后的信号都为单周期脉冲的形式，占空比1:X */
#define	BTx_PRES_PRESCALE_Msk	(0xffU << BTx_PRES_PRESCALE_Pos)

#define	BTx_LOADCR_LHEN_Pos	4	/* 高位加载控制 (Counter highend load enable)
写1将预置数寄存器ET1PRESETH和加载寄存器ET1LOADH分别加载到计数值寄存器ET1CNTH和比较寄存器ET1CMPH，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 */
#define	BTx_LOADCR_LHEN_Msk	(0x1U << BTx_LOADCR_LHEN_Pos)

#define	BTx_LOADCR_LLEN_Pos	0	/* 低位加载控制 (Counter lowend load enable)
写1将预置数寄存器PRESETL和加载寄存器LOADL分别加载到计数值寄存器ET1CNTL和比较寄存器ET1CMPL，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 */
#define	BTx_LOADCR_LLEN_Msk	(0x1U << BTx_LOADCR_LLEN_Pos)

#define	BTx_CNTL_CNTL_Pos	0	/* 计数器低位计数值寄存器 (counter lowend)
LLEN有效时加载预置数到该寄存器。 */
#define	BTx_CNTL_CNTL_Msk	(0xffU << BTx_CNTL_CNTL_Pos)

#define	BTx_CNTH_CNTH_Pos	0	/* 计数器高位计数值寄存器 (counter highend)
LHEN有效时加载预置数到该寄存器。 */
#define	BTx_CNTH_CNTH_Msk	(0xffU << BTx_CNTH_CNTH_Pos)

#define	BTx_PRESET_PRESETH_Pos	8	/* 计数器高位预置数寄存器 (preset highend)
用于保存高位计数器初值，或保存捕捉结果高8bit */
#define	BTx_PRESET_PRESETH_Msk	(0xffU << BTx_PRESET_PRESETH_Pos)

#define	BTx_PRESET_PRESETL_Pos	0	/* 计数器低位预置数寄存器 (preset lowend)
用于保存低位计数器初值，或保存捕捉结果低8bit */
#define	BTx_PRESET_PRESETL_Msk	(0xffU << BTx_PRESET_PRESETL_Pos)

#define	BTx_LOADL_LOADL_Pos	0	/* 计数器低位加载寄存器 (load lowend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。 */
#define	BTx_LOADL_LOADL_Msk	(0xffU << BTx_LOADL_LOADL_Pos)

#define	BTx_LOADH_LOADH_Pos	0	/* 计数器高位加载寄存器 (load highend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。当工作在8位定时/计数器模式时，该高位加载寄存器不支持加载值为0x00的设置。 */
#define	BTx_LOADH_LOADH_Msk	(0xffU << BTx_LOADH_LOADH_Pos)

#define	BTx_CMPL_CMPL_Pos	0	/* 计数器低位比较寄存器 (compare lowend )
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 */
#define	BTx_CMPL_CMPL_Msk	(0xffU << BTx_CMPL_CMPL_Pos)

#define	BTx_CMPH_CMPH_Pos	0	/* 计数器高位比较寄存器 (compare highend)
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 */
#define	BTx_CMPH_CMPH_Msk	(0xffU << BTx_CMPH_CMPH_Pos)

#define	BTx_OUTCNT_OUTCNT_Pos	0	/* 计数器输出脉冲宽度计数器 (output pulse width counter)
该寄存器用于调整输出脉冲宽度。计数时钟为32768Hz，对应的输出脉冲宽度范围为30.5uS~125mS。输出脉冲宽度=(OUTCNT+1)/32768秒 */
#define	BTx_OUTCNT_OUTCNT_Msk	(0xfffU << BTx_OUTCNT_OUTCNT_Pos)

#define	BTx_OCR_OUTCLR_Pos	5
#define	BTx_OCR_OUTCLR_Msk	(0x1U << BTx_OCR_OUTCLR_Pos)
#define	BTx_OCR_OUTCLR_INVALID	(0x0U << BTx_OCR_OUTCLR_Pos)
#define	BTx_OCR_OUTCLR_CLEAR	(0x1U << BTx_OCR_OUTCLR_Pos)

#define	BTx_OCR_OUTINV_Pos	4
#define	BTx_OCR_OUTINV_Msk	(0x1U << BTx_OCR_OUTINV_Pos)
#define	BTx_OCR_OUTINV_NORMAL	(0x0U << BTx_OCR_OUTINV_Pos)
#define	BTx_OCR_OUTINV_REVERSE	(0x1U << BTx_OCR_OUTINV_Pos)

#define	BTx_OCR_OUTMOD_Pos	3
#define	BTx_OCR_OUTMOD_Msk	(0x1U << BTx_OCR_OUTMOD_Pos)
#define	BTx_OCR_OUTMOD_PULSE_SHIFT	(0x0U << BTx_OCR_OUTMOD_Pos)
#define	BTx_OCR_OUTMOD_NEG	(0x1U << BTx_OCR_OUTMOD_Pos)

#define	BTx_OCR_OUTSEL_Pos	0
#define	BTx_OCR_OUTSEL_Msk	(0x7U << BTx_OCR_OUTSEL_Pos)
#define	BTx_OCR_OUTSEL_HIGH	(0x0U << BTx_OCR_OUTSEL_Pos)
#define	BTx_OCR_OUTSEL_LOW	(0x1U << BTx_OCR_OUTSEL_Pos)
#define	BTx_OCR_OUTSEL_GROUP1	(0x2U << BTx_OCR_OUTSEL_Pos)
#define	BTx_OCR_OUTSEL_GROUP2	(0x3U << BTx_OCR_OUTSEL_Pos)
#define	BTx_OCR_OUTSEL_PWM	(0x4U << BTx_OCR_OUTSEL_Pos)

#define	BTx_IER_CMPHIE_Pos	4	/* 扩展定时器高位比较发生信号 (compare highend interrupt enable)
1 = 中断使能
0 = 中断禁止 */
#define	BTx_IER_CMPHIE_Msk	(0x1U << BTx_IER_CMPHIE_Pos)

#define	BTx_IER_CMPLIE_Pos	3	/* 扩展定时器低位比较发生信号 (compare lowend interrupt enable)
1 = 中断使能
0 = 中断禁止 */
#define	BTx_IER_CMPLIE_Msk	(0x1U << BTx_IER_CMPLIE_Pos)

#define	BTx_IER_OVHIE_Pos	2	/* 定时器高位溢出信号 (highend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 */
#define	BTx_IER_OVHIE_Msk	(0x1U << BTx_IER_OVHIE_Pos)

#define	BTx_IER_OVLIE_Pos	1	/* 定时器低位溢出信号 (lowend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 */
#define	BTx_IER_OVLIE_Msk	(0x1U << BTx_IER_OVLIE_Pos)

#define	BTx_IER_CAPIE_Pos	0	/* 定时器捕捉产生信号 (capture interrupt enable)
1 = 中断使能
0 = 中断禁止 */
#define	BTx_IER_CAPIE_Msk	(0x1U << BTx_IER_CAPIE_Pos)

#define	BTx_ISR_CMPHIF_Pos	4	/* 定时器高位比较发生信号 (compare highend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 */
#define	BTx_ISR_CMPHIF_Msk	(0x1U << BTx_ISR_CMPHIF_Pos)

#define	BTx_ISR_CMPLIF_Pos	3	/* 定时器低位比较发生信号 (compare lowend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 */
#define	BTx_ISR_CMPLIF_Msk	(0x1U << BTx_ISR_CMPLIF_Pos)

#define	BTx_ISR_OVHIF_Pos	2	/* 定时器高位溢出信号 (highend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 */
#define	BTx_ISR_OVHIF_Msk	(0x1U << BTx_ISR_OVHIF_Pos)

#define	BTx_ISR_OVLIF_Pos	1	/* 定时器低位溢出信号 (lowend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 */
#define	BTx_ISR_OVLIF_Msk	(0x1U << BTx_ISR_OVLIF_Pos)

#define	BTx_ISR_CAPIF_Pos	0	/* 定时器捕捉产生信号 (capture interrupt flag)
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 */
#define	BTx_ISR_CAPIF_Msk	(0x1U << BTx_ISR_CAPIF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void BTx_Deinit(BT_Type* BTx);

/* 高8位计数器（BT1H或BT2H）启动控制  相关函数 */
extern void BTx_CR1_CHEN_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR1_CHEN_Getable(BT_Type* BTx);

/* 低8位计数器（BT1L或BT2L）启动控制
(Counter-Lowend enable)
1：启动低8bit计数器，在计数器模式下启动时将预置数值和加载值分别加载至计数器和比较寄存器；捕捉模式下启动时计数器由零开始自由计数，计数到0xFFFF后产生溢出信号然后由零开始重新计数，捕捉功能只工作在16位模式；在16位的定时/计数和捕捉模式下CHEN作为计数器的启动控制，CLEN自动失效
0：停止低8bit计数器计数 相关函数 */
extern void BTx_CR1_CLEN_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR1_CLEN_Getable(BT_Type* BTx);

/* 工作模式选择 (work mode)
1：16位捕捉模式
0：8位定时/计数模式，若高位计数器计数源选择为低位计数器的溢出信号，则可实现16位定时/计数模式 相关函数 */
extern void BTx_CR1_MODE_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR1_MODE_Get(BT_Type* BTx);

/* 计数模式下的计数沿和周期捕捉时的捕捉沿选择位 (edge select)
1：计数模式采样计数源下降沿，周期捕捉模式时下沿捕捉
0：计数模式采样计数源上升沿，周期捕捉模式时上沿捕捉
注：不支持系统时钟的下降沿计数，捕捉源和计数源为系统时钟时选择下降沿将不会有效计数。 相关函数 */
extern void BTx_CR1_EDGESEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR1_EDGESEL_Get(BT_Type* BTx);

/* 捕捉模式控制（只在捕捉模式下有效）(capture mode)
1：脉冲宽度捕捉
0：脉冲周期捕捉 相关函数 */
extern void BTx_CR1_CAPMOD_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR1_CAPMOD_Get(BT_Type* BTx);

/* 带清零捕捉模式控制 (capture clear)
1：不论在脉冲宽度还是周期捕捉情况下，捕捉到第一个沿后将计数器清零产生中断，捕捉到第二个沿后锁存（锁存到高低位预置数寄存器）计数值并同时清零计数器
0：捕捉不清零，计数器一直自由计数 相关函数 */
extern void BTx_CR1_CAPCLR_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR1_CAPCLR_Getable(BT_Type* BTx);

/* 单次捕捉控制 (capture once)
1：单次捕捉有效，在捕捉到一次脉冲宽度或脉冲周期后计数器停止，若需要再次捕捉需重新启动
0：连续捕捉 相关函数 */
extern void BTx_CR1_CAPONCE_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR1_CAPONCE_Get(BT_Type* BTx);

/* PWM模式输出 (pulse width modulation)
1：PWM输出使能
0：PWM 输出不使能 相关函数 */
extern void BTx_CR1_PWM_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR1_PWM_Getable(BT_Type* BTx);

/* 计数器内部计数源信号选择 (signal group2 select)
1：内部计数源信号选择Group2
0：内部计数源信号选择Group1 相关函数 */
extern void BTx_CR2_SIG2SEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR2_SIG2SEL_Get(BT_Type* BTx);

/* 计数器内部捕捉源信号选择 (signal group1 select)
1：内部捕捉源信号选择Group1
0：内部捕捉源信号选择Group2 相关函数 */
extern void BTx_CR2_SIG1SEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR2_SIG1SEL_Get(BT_Type* BTx);

/* 高8位计数器计数源选择 (Counter Highend source select)
00/11：选择ET1的低位计数器的溢出信号，与低位计数器组成16位计数器
01：选择内部捕捉源信号
10：选择内部计数源信号或外部DIR输入组合信号 相关函数 */
extern void BTx_CR2_CNTHSEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR2_CNTHSEL_Get(BT_Type* BTx);

/* 外部输入DIR控制使能。通常电量脉冲输出时同时会输出一个由高低电平指示正反向的方向信号DIR。电路将通过DIR信号电平的高低，分别控制高位计数器和低位计数器计数使能，以实现针对正向、反向脉冲的各种计数功能 (direction bit enable)
1：外部输入的DIR信号有效，此时高低位计数器是否计数可由外部输入的DIR信号控制。
0：外部输入的DIR信号无效，此时高低位计数器是否计数将由内部控制信号控制。 相关函数 */
extern void BTx_CR2_DIREN_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR2_DIREN_Getable(BT_Type* BTx);

/* 内部DIR控制信号，当DIREN为0，即外部输入DIR控制无效时，可由该信号代替DIR输入，直接控制内部计数器的计数。当需要外部DIR输入，即DIREN为1时，该位应设置为0 (set direction)
1：内部DIR信号为高电平，则高8位计数器计数 相关函数 */
extern void BTx_CR2_STDIR_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR2_STDIR_Getable(BT_Type* BTx);

/* 低位计数器计数使能控制选择信号 (source select)
1低位计数器计数使能端选则常使能。此时低位计数器计数不受DIR控制，可将正反向所有脉冲一并计数
0：低位计数器计数使能端选则由寄存器STDIR或外部EX_SIG2输入控制。 相关函数 */
extern void BTx_CR2_SRCSEL_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_CR2_SRCSEL_Getable(BT_Type* BTx);

/* 输入信号2极性选择 (direction polarity)
1：对外部输入DIR信号EX_SIG2反向
0：对外部输入DIR信号EX_SIG2不反向 相关函数 */
extern void BTx_CR2_DIRPO_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CR2_DIRPO_Get(BT_Type* BTx);

/* RTCOUT2信号选择控制2 (RTCOUT2 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU 相关函数 */
extern void BTx_CFGR1_RTCSEL2_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR1_RTCSEL2_Get(BT_Type* BTx);

/* RTCOUT1信号选择控制1 (RTCOUT1 source select)
00 = 32768Hz，XTLF时钟输出
01 = RTCSec，由RTC模块输出的秒信号
10 = RTCMin，由RTC模块输出的分钟信号
11 = RFU 相关函数 */
extern void BTx_CFGR1_RTCSEL1_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR1_RTCSEL1_Get(BT_Type* BTx);

/* Group2信号选择控制 (Group2 source select)
00 = APBCLK
01 = RTCOUT2
10 = IN_SIG2，内部输入信号2
11 = EX_SIG2，外部输入信号2
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 相关函数 */
extern void BTx_CFGR1_GRP2SEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR1_GRP2SEL_Get(BT_Type* BTx);

/* Group1信号选择控制（可作为捕捉模式下采样时钟选择，同时可作为信号捕捉源）(Group1 source select)
00 = APBCLK
01 = RTCOUT1
10 = IN_SIG1，内部输入信号1
11 = EX_SIG1，外部输入信号1
注：不支持APBCLK的下降沿捕捉和计数，捕捉源和计数源为APBCLK时选择下降沿将不会有效捕捉和计数。 相关函数 */
extern void BTx_CFGR1_GRP1SEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR1_GRP1SEL_Get(BT_Type* BTx);

/* 外部输入信号选择控制2 (external source select2)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 相关函数 */
extern void BTx_CFGR2_EXSEL2_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR2_EXSEL2_Get(BT_Type* BTx);

/* 外部输入信号选择控制1 (external source select1)
00 = BT1_IN0/BT2_IN0
01 = BT1_IN1/BT2_IN1
10 = BT1_IN2/BT2_IN2
11 = BT1_IN3/BT2_IN3 相关函数 */
extern void BTx_CFGR2_EXSEL1_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR2_EXSEL1_Get(BT_Type* BTx);

/* 内部输入信号选择控制2 (internal source select 2)
00 = UART_RX3/UART_RX3
01 = UART_RX4/UART_RX4
10 = UART_RX5/UART_RX5
11 = RCLP/BT1_OUT 相关函数 */
extern void BTx_CFGR2_INSEL2_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR2_INSEL2_Get(BT_Type* BTx);

/* 内部输入信号选择控制1 (internal source select 1)
00 = UART_RX0/UART_RX0
01 = UART_RX1/UART_RX1
10 = UART_RX2/UART_RX2
11 = RCLP/RCLP 相关函数 */
extern void BTx_CFGR2_INSEL1_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CFGR2_INSEL1_Get(BT_Type* BTx);

/* 输入Group1的预分频寄存器 (Group1 input signal prescaler)
分频数=（X+1），即00表示1分频，FF表示256分频。预分频后的信号都为单周期脉冲的形式，占空比1:X 相关函数 */
extern void BTx_PRES_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_PRES_Read(BT_Type* BTx);

/* 高位加载控制 (Counter highend load enable)
写1将预置数寄存器ET1PRESETH和加载寄存器ET1LOADH分别加载到计数值寄存器ET1CNTH和比较寄存器ET1CMPH，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 相关函数 */
extern void BTx_LOADCR_LHEN_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_LOADCR_LHEN_Getable(BT_Type* BTx);

/* 低位加载控制 (Counter lowend load enable)
写1将预置数寄存器PRESETL和加载寄存器LOADL分别加载到计数值寄存器ET1CNTL和比较寄存器ET1CMPL，写0无效，该位硬件自动清0。在16位的定时/计数下LHEN作为计数器的加载控制，LLEN自动失效 相关函数 */
extern void BTx_LOADCR_LLEN_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_LOADCR_LLEN_Getable(BT_Type* BTx);

/* 计数器低位计数值寄存器 (counter lowend)
LLEN有效时加载预置数到该寄存器。 相关函数 */
extern uint32_t BTx_CNTL_Read(BT_Type* BTx);

/* 计数器高位计数值寄存器 (counter highend)
LHEN有效时加载预置数到该寄存器。 相关函数 */
extern uint32_t BTx_CNTH_Read(BT_Type* BTx);

/* 计数器高位预置数寄存器 (preset highend)
用于保存高位计数器初值，或保存捕捉结果高8bit 相关函数 */
extern void BTx_PRESET_PRESETH_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_PRESET_PRESETH_Get(BT_Type* BTx);

/* 计数器低位预置数寄存器 (preset lowend)
用于保存低位计数器初值，或保存捕捉结果低8bit 相关函数 */
extern void BTx_PRESET_PRESETL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_PRESET_PRESETL_Get(BT_Type* BTx);

/* 计数器低位加载寄存器 (load lowend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。 相关函数 */
extern void BTx_LOADL_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_LOADL_Read(BT_Type* BTx);

/* 计数器高位加载寄存器 (load highend counter)
在计数匹配或执行加载命令时将加载寄存器的值加载至比较工作寄存器。当工作在8位定时/计数器模式时，该高位加载寄存器不支持加载值为0x00的设置。 相关函数 */
extern void BTx_LOADH_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_LOADH_Read(BT_Type* BTx);

/* 计数器低位比较寄存器 (compare lowend )
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
extern void BTx_CMPL_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CMPL_Read(BT_Type* BTx);

/* 计数器高位比较寄存器 (compare highend)
加载寄存器的值加载后将写入该寄存器，该寄存器与计数器比较，若计数值大于等于该寄存器的值，则产生计数匹配信号至输出控制模块，并产生相应中断。 相关函数 */
extern void BTx_CMPH_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_CMPH_Read(BT_Type* BTx);

/* 计数器输出脉冲宽度计数器 (output pulse width counter)
该寄存器用于调整输出脉冲宽度。计数时钟为32768Hz，对应的输出脉冲宽度范围为30.5uS~125mS。输出脉冲宽度=(OUTCNT+1)/32768秒 相关函数 */
extern void BTx_OUTCNT_Write(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_OUTCNT_Read(BT_Type* BTx);
extern void BTx_OCR_OUTCLR_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_OCR_OUTCLR_Get(BT_Type* BTx);
extern void BTx_OCR_OUTINV_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_OCR_OUTINV_Get(BT_Type* BTx);
extern void BTx_OCR_OUTMOD_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_OCR_OUTMOD_Get(BT_Type* BTx);
extern void BTx_OCR_OUTSEL_Set(BT_Type* BTx, uint32_t SetValue);
extern uint32_t BTx_OCR_OUTSEL_Get(BT_Type* BTx);

/* 扩展定时器高位比较发生信号 (compare highend interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
extern void BTx_IER_CMPHIE_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_IER_CMPHIE_Getable(BT_Type* BTx);

/* 扩展定时器低位比较发生信号 (compare lowend interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
extern void BTx_IER_CMPLIE_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_IER_CMPLIE_Getable(BT_Type* BTx);

/* 定时器高位溢出信号 (highend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
extern void BTx_IER_OVHIE_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_IER_OVHIE_Getable(BT_Type* BTx);

/* 定时器低位溢出信号 (lowend overflow interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
extern void BTx_IER_OVLIE_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_IER_OVLIE_Getable(BT_Type* BTx);

/* 定时器捕捉产生信号 (capture interrupt enable)
1 = 中断使能
0 = 中断禁止 相关函数 */
extern void BTx_IER_CAPIE_Setable(BT_Type* BTx, FunState NewState);
extern FunState BTx_IER_CAPIE_Getable(BT_Type* BTx);

/* 定时器高位比较发生信号 (compare highend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 相关函数 */
extern void BTx_ISR_CMPHIF_Clr(BT_Type* BTx);
extern FlagStatus BTx_ISR_CMPHIF_Chk(BT_Type* BTx);

/* 定时器低位比较发生信号 (compare lowend interrupt flag)
1 = 当前计数器的值大于等于比较寄存器的值，该信号将重新加载新的加载寄存器的值到工作寄存器。
0 = 当前计数器的值小于比较寄存器的值 相关函数 */
extern void BTx_ISR_CMPLIF_Clr(BT_Type* BTx);
extern FlagStatus BTx_ISR_CMPLIF_Chk(BT_Type* BTx);

/* 定时器高位溢出信号 (highend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
extern void BTx_ISR_OVHIF_Clr(BT_Type* BTx);
extern FlagStatus BTx_ISR_OVHIF_Chk(BT_Type* BTx);

/* 定时器低位溢出信号 (lowend overflow interrupt flag)
1 = 产生计数溢出
0 = 未产生溢出 相关函数 */
extern void BTx_ISR_OVLIF_Clr(BT_Type* BTx);
extern FlagStatus BTx_ISR_OVLIF_Chk(BT_Type* BTx);

/* 定时器捕捉产生信号 (capture interrupt flag)
1 = 捕捉到指定的沿
0 = 未捕捉到指定的沿 相关函数 */
extern void BTx_ISR_CAPIF_Clr(BT_Type* BTx);
extern FlagStatus BTx_ISR_CAPIF_Chk(BT_Type* BTx);
//Announce_End

#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_BT_H */
