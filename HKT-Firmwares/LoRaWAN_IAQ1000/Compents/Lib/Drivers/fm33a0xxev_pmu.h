/**
  ******************************************************************************
  * @file    fm33a0xxev_pmu.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the PMU firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_PMU_H
#define __FM33A0XXEV_PMU_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "FM33A0XXEV.h"
typedef struct
{
	uint32_t PMOD;				/*!<低功耗模式配置  */	
	uint32_t SLPDP;				/*!<DeepSleep控制寄存器  */
	FunState CVS;				/*!<内核电压降低使能控制  */
	uint32_t SCR;				/*!<M0系统控制寄存器，一般配置为0即可  */	
	uint32_t TIA;				/*!可编程额外唤醒延迟 */	
}PMU_SleepCfg_InitTypeDef;
   
#define	PMU_CR_LDO15EN_Pos	17	/* LDO15使能标志位
1：LDO15处于工作状态
0：LDO15被关闭 */
#define	PMU_CR_LDO15EN_Msk	(0x1U << PMU_CR_LDO15EN_Pos)

#define	PMU_CR_LDO15EN_B_Pos	16	/* LDO15使能标志反码校验位 */
#define	PMU_CR_LDO15EN_B_Msk	(0x1U << PMU_CR_LDO15EN_B_Pos)

#define	PMU_CR_WKFSEL_Pos	10	/* Sleep/DeepSleep唤醒后的系统频率
00：RCHF-8MHz
01：RCHF-16MHz
10：RCHF-24MHz
11：RCHF-32MHz */
#define	PMU_CR_WKFSEL_Msk	(0x3U << PMU_CR_WKFSEL_Pos)
#define	PMU_CR_WKFSEL_RCHF_8M	(0x0U << PMU_CR_WKFSEL_Pos)	/* RCHF-8MHz */
#define	PMU_CR_WKFSEL_RCHF_16M	(0x1U << PMU_CR_WKFSEL_Pos)	/* RCHF-16MHz */
#define	PMU_CR_WKFSEL_RCHF_24M	(0x2U << PMU_CR_WKFSEL_Pos)	/* RCHF-24MHz */
#define	PMU_CR_WKFSEL_RCHF_32M	(0x3U << PMU_CR_WKFSEL_Pos)	/* RCHF-32MHz */

#define	PMU_CR_SLPDP_Pos	9	/* DeepSleep控制寄存器
1：DeepSleep模式使能，下关闭基准电压源
0：常规Sleep模式
在Sleep下，如果置位了SLPDP位即为DeepSleep模式；
该位仅在Sleep下有效 */
#define	PMU_CR_SLPDP_Msk	(0x1U << PMU_CR_SLPDP_Pos)
#define	PMU_CR_SLPDP_DEEPSLEEP	(0x1U << PMU_CR_SLPDP_Pos)	/* DeepSleep模式使能，下关闭基准电压源 */
#define	PMU_CR_SLPDP_SLEEP	(0x0U << PMU_CR_SLPDP_Pos)	/* 常规Sleep模式 */

#define	PMU_CR_CVS_Pos	8	/* CoreVoltageScaling配置
0：低功耗模式下不使能内核电压调整
1：低功耗模式下降低内核电压
该位仅在Sleep/DeepSleep/RTCBKP模式下起作用 */
#define	PMU_CR_CVS_Msk	(0x1U << PMU_CR_CVS_Pos)
#define	PMU_CR_CVS_DISABLE	(0x0U << PMU_CR_CVS_Pos)	/* 低功耗模式下不使能内核电压调整 */
#define	PMU_CR_CVS_ENABLE	(0x1U << PMU_CR_CVS_Pos)	/* 低功耗模式下降低内核电压 */

#define	PMU_CR_PMOD_Pos	0	/* 低功耗模式配置寄存器
00：Active mode / LP Active mode
01：LPRUN mode
10：Sleep mode / DeepSleep mode
11：RTCBKP mode */
#define	PMU_CR_PMOD_Msk	(0x3U << PMU_CR_PMOD_Pos)
#define	PMU_CR_PMOD_ACTIVE	(0x0U << PMU_CR_PMOD_Pos)	/* Active mode / LP Active mode */
#define	PMU_CR_PMOD_LPRUN 	(0x1U << PMU_CR_PMOD_Pos)	/* LPRUN mode */
#define	PMU_CR_PMOD_SLEEP	(0x2U << PMU_CR_PMOD_Pos)	/* Sleep mode / DeepSleep mode */

#define	PMU_WKTR_STPCLR_Pos	2	/* Flash Stop唤醒控制
0：Stop信号等待时钟建立后同步清零
1：Stop信号异步清零 */
#define	PMU_WKTR_STPCLR_Msk	(0x1U << PMU_WKTR_STPCLR_Pos)
#define	PMU_WKTR_STPCLR_SYN	(0x0U << PMU_WKTR_STPCLR_Pos)	/* Stop信号等待时钟建立后同步清零 */
#define	PMU_WKTR_STPCLR_ASY	(0x1U << PMU_WKTR_STPCLR_Pos)	/* Stop信号异步清零 */

#define	PMU_WKTR_T1A_Pos	0	/* 可编程额外唤醒延迟
在DeepSleep模式下，RCHF时钟到来后，根据此寄存器配置等待额外延迟时间后，再读取Flash校验字
00：0us
01：2us
10：4us
11：8us */
#define	PMU_WKTR_T1A_Msk	(0x3U << PMU_WKTR_T1A_Pos)
#define	PMU_WKTR_T1A_0US	(0x0U << PMU_WKTR_T1A_Pos)	/* 0us */
#define	PMU_WKTR_T1A_2US	(0x1U << PMU_WKTR_T1A_Pos)	/* 2us */
#define	PMU_WKTR_T1A_4US	(0x2U << PMU_WKTR_T1A_Pos)	/* 4us */
#define	PMU_WKTR_T1A_8US	(0x3U << PMU_WKTR_T1A_Pos)	/* 8us */

#define	PMU_WKFR_ADCWKF_Pos	31	/* ADC中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_ADCWKF_Msk	(0x1U << PMU_WKFR_ADCWKF_Pos)

#define	PMU_WKFR_RTCWKF_Pos	28	/* RTC中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_RTCWKF_Msk	(0x1U << PMU_WKFR_RTCWKF_Pos)

#define	PMU_WKFR_SVDWKF_Pos	27	/* SVD中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_SVDWKF_Msk	(0x1U << PMU_WKFR_SVDWKF_Pos)

#define	PMU_WKFR_LFDETWKF_Pos	26	/* 32768Hz晶体停振中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_LFDETWKF_Msk	(0x1U << PMU_WKFR_LFDETWKF_Pos)

#define	PMU_WKFR_IOWKF_Pos	24	/* IO中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_IOWKF_Msk	(0x1U << PMU_WKFR_IOWKF_Pos)

#define	PMU_WKFR_LPU1WKF_Pos	21	/* LPUART1中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_LPU1WKF_Msk	(0x1U << PMU_WKFR_LPU1WKF_Pos)

#define	PMU_WKFR_LPU0WKF_Pos	20	/* LPUART0中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_LPU0WKF_Msk	(0x1U << PMU_WKFR_LPU0WKF_Pos)

#define	PMU_WKFR_COMP_OOWF_Pos	19	/* 比较器out-of-window唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_COMP_OOWF_Msk	(0x1U << PMU_WKFR_COMP_OOWF_Pos)

#define	PMU_WKFR_COMP_WINF_Pos	18	/* 比较器window唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_COMP_WINF_Msk	(0x1U << PMU_WKFR_COMP_WINF_Pos)

#define	PMU_WKFR_COMP2WKF_Pos	17	/* 比较器2中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_COMP2WKF_Msk	(0x1U << PMU_WKFR_COMP2WKF_Pos)

#define	PMU_WKFR_COMP1WKF_Pos	16	/* 比较器1中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_COMP1WKF_Msk	(0x1U << PMU_WKFR_COMP1WKF_Pos)

#define	PMU_WKFR_LPTWKF_Pos	10	/*  _32中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_LPTWKF_Msk	(0x1U << PMU_WKFR_LPTWKF_Pos)

#define	PMU_WKFR_BSTWKF_Pos	9	/* BSTIM32中断唤醒标志，中断撤销时硬件自动清零 */
#define	PMU_WKFR_BSTWKF_Msk	(0x1U << PMU_WKFR_BSTWKF_Pos)

#define	PMU_WKFR_DBGWKF_Pos	8	/* CPU Debugger唤醒标志，软件写1清零 */
#define	PMU_WKFR_DBGWKF_Msk	(0x1U << PMU_WKFR_DBGWKF_Pos)


#define	PMU_IER_SLPEIE_Pos	1	/* SLEEP错误中断使能 (Sleep mode Error Interrupt Enable)
1：使能SLEEP错误中断
0：禁止SLEEP错误中断 */
#define	PMU_IER_SLPEIE_Msk	(0x1U << PMU_IER_SLPEIE_Pos)
	/* 使能SLEEP错误中断 */
	/* 禁止SLEEP错误中断 */

#define	PMU_IER_RTCEIE_Pos	0	/* RTCBKP错误中断使能(RTCBKP mode Error Interrupt Enable)
1：使能RTCBKP错误中断
0：禁止RTCBKP错误中断 */
#define	PMU_IER_RTCEIE_Msk	(0x1U << PMU_IER_RTCEIE_Pos)
	/* 使能RTCBKP错误中断 */
	/* 禁止RTCBKP错误中断 */

#define	PMU_ISR_SLPEIF_Pos	1	/* SLEEP错误中断标志，硬件置位，软件写1清零 (Sleep Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h2后，CPU执行WFI/WFE指令前置位了SLEEPDEEP寄存器时置位
0：在PMOD=2’h2后，CPU正确进入SLEEP */
#define	PMU_ISR_SLPEIF_Msk	(0x1U << PMU_ISR_SLPEIF_Pos)

#define	PMU_ISR_RTCEIF_Pos	0	/* RTCBKP错误中断标志，硬件置位，软件写1清零(RTC Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h3后，未改写CPU内部寄存器SLEEPDEEP=1，然后执行WFI/WFE指令；或者系统时钟来自USB PHY，试图进入RTCBKP模式
0：在PMOD=2’h3后，CPU自身正确进入DEEP SLEEP */
#define	PMU_ISR_RTCEIF_Msk	(0x1U << PMU_ISR_RTCEIF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void PMU_Deinit(void);

/* LDO15使能标志位
1：LDO15处于工作状态
0：LDO15被关闭 相关函数 */
extern FlagStatus PMU_CR_LDO15EN_Chk(void);

/* LDO15使能标志反码校验位 相关函数 */
extern FlagStatus PMU_CR_LDO15EN_B_Chk(void);

/* Sleep/DeepSleep唤醒后的系统频率
00：RCHF-8MHz
01：RCHF-16MHz
10：RCHF-24MHz
11：RCHF-32MHz 相关函数 */
extern void PMU_CR_WKFSEL_Set(uint32_t SetValue);
extern uint32_t PMU_CR_WKFSEL_Get(void);

/* DeepSleep控制寄存器
1：DeepSleep模式使能，下关闭基准电压源
0：常规Sleep模式
在Sleep下，如果置位了SLPDP位即为DeepSleep模式；
该位仅在Sleep下有效 相关函数 */
extern void PMU_CR_SLPDP_Set(uint32_t SetValue);
extern uint32_t PMU_CR_SLPDP_Get(void);

/* CoreVoltageScaling配置
0：低功耗模式下不使能内核电压调整
1：低功耗模式下降低内核电压
该位仅在Sleep/DeepSleep/RTCBKP模式下起作用 相关函数 */
extern void PMU_CR_CVS_Set(uint32_t SetValue);
extern uint32_t PMU_CR_CVS_Get(void);

/* 低功耗模式配置寄存器
00：Active mode / LP Active mode
01：LPRUN mode
10：Sleep mode / DeepSleep mode
11：RTCBKP mode 相关函数 */
extern void PMU_CR_PMOD_Set(uint32_t SetValue);
extern uint32_t PMU_CR_PMOD_Get(void);

/* Flash Stop唤醒控制
0：Stop信号等待时钟建立后同步清零
1：Stop信号异步清零 相关函数 */
extern void PMU_WKTR_STPCLR_Set(uint32_t SetValue);
extern uint32_t PMU_WKTR_STPCLR_Get(void);

/* 可编程额外唤醒延迟
在DeepSleep模式下，RCHF时钟到来后，根据此寄存器配置等待额外延迟时间后，再读取Flash校验字
00：0us
01：2us
10：4us
11：8us 相关函数 */
extern void PMU_WKTR_T1A_Set(uint32_t SetValue);
extern uint32_t PMU_WKTR_T1A_Get(void);

/* ADC中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_ADCWKF_Chk(void);

/* RTC中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_RTCWKF_Chk(void);

/* SVD中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_SVDWKF_Chk(void);

/* 32768Hz晶体停振中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_LFDETWKF_Chk(void);

/* IO中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_IOWKF_Chk(void);

/* LPUART1中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_LPU1WKF_Chk(void);

/* LPUART0中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_LPU0WKF_Chk(void);

/* 比较器out-of-window唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_COMP_OOWF_Chk(void);

/* 比较器window唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_COMP_WINF_Chk(void);

/* 比较器2中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_COMP2WKF_Chk(void);

/* 比较器1中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_COMP1WKF_Chk(void);

/*  _32中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_LPTWKF_Chk(void);

/* BSTIM32中断唤醒标志，中断撤销时硬件自动清零 相关函数 */
extern FlagStatus PMU_WKFR_BSTWKF_Chk(void);

/* CPU Debugger唤醒标志，软件写1清零 相关函数 */
extern void PMU_WKFR_DBGWKF_Clr(void);
extern FlagStatus PMU_WKFR_DBGWKF_Chk(void);


/* SLEEP错误中断使能 (Sleep mode Error Interrupt Enable)
1：使能SLEEP错误中断
0：禁止SLEEP错误中断 相关函数 */
extern void PMU_IER_SLPEIE_Setable(FunState NewState);
extern FunState PMU_IER_SLPEIE_Getable(void);

/* RTCBKP错误中断使能(RTCBKP mode Error Interrupt Enable)
1：使能RTCBKP错误中断
0：禁止RTCBKP错误中断 相关函数 */
extern void PMU_IER_RTCEIE_Setable(FunState NewState);
extern FunState PMU_IER_RTCEIE_Getable(void);

/* SLEEP错误中断标志，硬件置位，软件写1清零 (Sleep Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h2后，CPU执行WFI/WFE指令前置位了SLEEPDEEP寄存器时置位
0：在PMOD=2’h2后，CPU正确进入SLEEP 相关函数 */
extern void PMU_ISR_SLPEIF_Clr(void);
extern FlagStatus PMU_ISR_SLPEIF_Chk(void);

/* RTCBKP错误中断标志，硬件置位，软件写1清零(RTC Error Interrupt Flag, write 1 to clear)
1：在PMOD=2’h3后，未改写CPU内部寄存器SLEEPDEEP=1，然后执行WFI/WFE指令；或者系统时钟来自USB PHY，试图进入RTCBKP模式
0：在PMOD=2’h3后，CPU自身正确进入DEEP SLEEP 相关函数 */
extern void PMU_ISR_RTCEIF_Clr(void);
extern FlagStatus PMU_ISR_RTCEIF_Chk(void);

extern void PMU_WKFR_WKPxF_Clr(uint32_t NWKPinDef);
extern FlagStatus PMU_WKFR_WKPxF_Chk(uint32_t NWKPinDef);
extern void PMU_SleepCfg_Init(PMU_SleepCfg_InitTypeDef* SleepCfg_InitStruct);

//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_PMU_H */
