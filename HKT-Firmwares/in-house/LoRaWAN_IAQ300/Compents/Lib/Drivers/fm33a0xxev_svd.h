/**
  ******************************************************************************
  * @file    fm33a0xxev_svd.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the SVD firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_SVD_H
#define __FM33A0XXEV_SVD_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
   
#include "FM33A0XXEV.h"

typedef struct
{
	uint32_t	SVDMOD;		/*!<SVD工作模式选择  */
	uint32_t	SVDITVL;	/*!<SVD间歇使能间隔  */
	uint32_t	SVDLVL;		/*!<SVD报警阈值设置  */
	FunState	DFEN;		/*!<SVD数字滤波（SVDMODE=1时必须置1）  */
	FunState	PFIE;		/*!<SVD电源跌落中断  */
	FunState	PRIE;		/*!<SVD电源恢复中断  */
	FunState	SVDEN;		/*!<SVD使能  */
	
}SVD_InitTypeDef;
 
#define	SVD_CFGR_LVL_Pos	4	/* SVD报警阈值档位设置，参见11.3.3检测阈值
(SVD threshold level) */
#define	SVD_CFGR_LVL_Msk	(0xfU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_1P800V	(0x0U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_2P014V	(0x1U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_2P229V	(0x2U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_2P443V	(0x3U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_2P657V	(0x4U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_2P871V	(0x5U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_3P086V	(0x6U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_3P300V	(0x7U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_3P514V	(0x8U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_3P729V	(0x9U << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_3P943V	(0xaU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_4P157V	(0xbU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_4P371V	(0xcU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_4P586V	(0xdU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_4P800V	(0xeU << SVD_CFGR_LVL_Pos)
#define	SVD_CFGR_LVL_SVS	(0xfU << SVD_CFGR_LVL_Pos)

#define	SVD_CFGR_DFEN_Pos	3	/* 数字滤波使能（MOD=1时必须置1）
(Digital Filter Enable)
1：启动SVD输出的数字滤波
0：关闭SVD输出的数字滤波 */
#define	SVD_CFGR_DFEN_Msk	(0x1U << SVD_CFGR_DFEN_Pos)
	/* 启动SVD输出的数字滤波 */
	/* 关闭SVD输出的数字滤波 */

#define	SVD_CFGR_MOD_Pos	2	/* SVD工作模式选择，配置模式后还要置位SVD_CR.EN才会启动SVD
(SVD Mode)
1：间歇使能模式
0：常使能模式
注意：间歇使能模式下必须开启数字滤波 */
#define	SVD_CFGR_MOD_Msk	(0x1U << SVD_CFGR_MOD_Pos)
#define	SVD_CFGR_MOD_INTERVAL	(0x1U << SVD_CFGR_MOD_Pos)	/* 间歇使能模式 */
#define	SVD_CFGR_MOD_ALWAYSON	(0x0U << SVD_CFGR_MOD_Pos)	/* 常使能模式 */

#define	SVD_CFGR_ITVL_Pos	0	/* SVD间歇使能间隔 (SVD interval enable period)
00：62.5ms
01：256ms
10：1s
11：4s */
#define	SVD_CFGR_ITVL_Msk	(0x3U << SVD_CFGR_ITVL_Pos)
#define	SVD_CFGR_ITVL_62P5MS	(0x0U << SVD_CFGR_ITVL_Pos)	/* 62.5ms */
#define	SVD_CFGR_ITVL_256MS	(0x1U << SVD_CFGR_ITVL_Pos)	/* 256ms */
#define	SVD_CFGR_ITVL_1S	(0x2U << SVD_CFGR_ITVL_Pos)	/* 1s */
#define	SVD_CFGR_ITVL_4S	(0x3U << SVD_CFGR_ITVL_Pos)	/* 4s */
	/* 62.5ms */
	/* 256ms */
	/* 1s */
	/* 4s */
#define	SVD_CR_TE_Pos	8	/* SVD测试使能，避免写1 (SVD test enable) */
#define	SVD_CR_TE_Msk	(0x1U << SVD_CR_TE_Pos)

#define	SVD_CR_SVSEN_Pos	1	/* SVS外部电源检测通道控制信号 
(SVS external monitor channel enable)
0：SVS通道关闭
1：SVS通道使能
当EN=1时，根据SVDLVL寄存器可以设置SVS输入后是否经过内部电阻分压；如果LVL=1111，则SVS输入不做分压，如果LVL != 1111，则SVS输入经过内部电阻分压。 */
#define	SVD_CR_SVSEN_Msk	(0x1U << SVD_CR_SVSEN_Pos)
	/* SVS通道关闭 */
	/* SVS通道使能 */

#define	SVD_CR_SVDEN_Pos	0
#define	SVD_CR_SVDEN_Msk	(0x1U << SVD_CR_SVDEN_Pos)

#define	SVD_IER_PFIE_Pos	1	/* 电源跌落中断使能寄存器，1有效 (Power Fall interrupt enable) */
#define	SVD_IER_PFIE_Msk	(0x1U << SVD_IER_PFIE_Pos)

#define	SVD_IER_PRIE_Pos	0	/* 电源恢复中断使能寄存器，1有效 (Power Rise interrupt enable) */
#define	SVD_IER_PRIE_Msk	(0x1U << SVD_IER_PRIE_Pos)

#define	SVD_ISR_SVDO_Pos	8	/* SVD电源检测输出 (SVD output)
1：电源电压高于SVD当前阈值
0：电源电压低于SVD当前阈值 */
#define	SVD_ISR_SVDO_Msk	(0x1U << SVD_ISR_SVDO_Pos)

#define	SVD_ISR_SVDR_Pos	7	/* SVD输出锁存信号，数字电路锁存的SVD状态
(SVD registered output) */
#define	SVD_ISR_SVDR_Msk	(0x1U << SVD_ISR_SVDR_Pos)

#define	SVD_ISR_PFF_Pos	1	/* 电源跌落中断标志寄存器，电源电压跌落到SVD阈值之下时置位，软件写1清零 (Power fall flag) */
#define	SVD_ISR_PFF_Msk	(0x1U << SVD_ISR_PFF_Pos)

#define	SVD_ISR_PRF_Pos	0	/* 电源恢复中断标志寄存器，电源电压上升到SVD阈值之上时置位，软件写1清零 (Power rise flag) */
#define	SVD_ISR_PRF_Msk	(0x1U << SVD_ISR_PRF_Pos)

#define	SVD_VSR_V1P0EN_Pos	2	/* 1.0V基准输入使能信号 (1.0V reference enable)
1：使能1.0V基准输入
0：关闭1.0V基准输入 */
#define	SVD_VSR_V1P0EN_Msk	(0x1U << SVD_VSR_V1P0EN_Pos)

#define	SVD_VSR_V0P95EN_Pos	1	/* 0.95V基准输入使能信号 (0.95V reference enable)
1：使能0.95V基准输入
0：关闭0.95V基准输入 */
#define	SVD_VSR_V0P95EN_Msk	(0x1U << SVD_VSR_V0P95EN_Pos)

#define	SVD_VSR_V0P9EN_Pos	0	/* 0.9V基准输入使能信号 (0.9V reference enable)
1：使能0.9V基准输入
0：关闭0.9V基准输入 */
#define	SVD_VSR_V0P9EN_Msk	(0x1U << SVD_VSR_V0P9EN_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void SVD_Deinit(void);

/* SVD报警阈值档位设置，参见11.3.3检测阈值
(SVD threshold level) 相关函数 */
extern void SVD_CFGR_LVL_Set(uint32_t SetValue);
extern uint32_t SVD_CFGR_LVL_Get(void);

/* 数字滤波使能（MOD=1时必须置1）
(Digital Filter Enable)
1：启动SVD输出的数字滤波
0：关闭SVD输出的数字滤波 相关函数 */
extern void SVD_CFGR_DFEN_Setable(FunState NewState);
extern FunState SVD_CFGR_DFEN_Getable(void);

/* SVD工作模式选择，配置模式后还要置位SVD_CR.EN才会启动SVD
(SVD Mode)
1：间歇使能模式
0：常使能模式
注意：间歇使能模式下必须开启数字滤波 相关函数 */
extern void SVD_CFGR_MOD_Set(uint32_t SetValue);
extern uint32_t SVD_CFGR_MOD_Get(void);

/* SVD间歇使能间隔 (SVD interval enable period)
00：62.5ms
01：256ms
10：1s
11：4s 相关函数 */
extern void SVD_CFGR_ITVL_Set(uint32_t SetValue);
extern uint32_t SVD_CFGR_ITVL_Get(void);

/* SVS外部电源检测通道控制信号 
(SVS external monitor channel enable)
0：SVS通道关闭
1：SVS通道使能
当EN=1时，根据SVDLVL寄存器可以设置SVS输入后是否经过内部电阻分压；如果LVL=1111，则SVS输入不做分压，如果LVL != 1111，则SVS输入经过内部电阻分压。 相关函数 */
extern void SVD_CR_SVSEN_Setable(FunState NewState);
extern FunState SVD_CR_SVSEN_Getable(void);
extern void SVD_CR_SVDEN_Setable(FunState NewState);
extern FunState SVD_CR_SVDEN_Getable(void);

/* 电源跌落中断使能寄存器，1有效 (Power Fall interrupt enable) 相关函数 */
extern void SVD_IER_PFIE_Setable(FunState NewState);
extern FunState SVD_IER_PFIE_Getable(void);

/* 电源恢复中断使能寄存器，1有效 (Power Rise interrupt enable) 相关函数 */
extern void SVD_IER_PRIE_Setable(FunState NewState);
extern FunState SVD_IER_PRIE_Getable(void);

/* SVD电源检测输出 (SVD output)
1：电源电压高于SVD当前阈值
0：电源电压低于SVD当前阈值 相关函数 */
extern FlagStatus SVD_ISR_SVDO_Chk(void);

/* SVD输出锁存信号，数字电路锁存的SVD状态
(SVD registered output) 相关函数 */
extern FlagStatus SVD_ISR_SVDR_Chk(void);

/* 电源跌落中断标志寄存器，电源电压跌落到SVD阈值之下时置位，软件写1清零 (Power fall flag) 相关函数 */
extern void SVD_ISR_PFF_Clr(void);
extern FlagStatus SVD_ISR_PFF_Chk(void);

/* 电源恢复中断标志寄存器，电源电压上升到SVD阈值之上时置位，软件写1清零 (Power rise flag) 相关函数 */
extern void SVD_ISR_PRF_Clr(void);
extern FlagStatus SVD_ISR_PRF_Chk(void);

/* 1.0V基准输入使能信号 (1.0V reference enable)
1：使能1.0V基准输入
0：关闭1.0V基准输入 相关函数 */
extern void SVD_VSR_V1P0EN_Setable(FunState NewState);
extern FunState SVD_VSR_V1P0EN_Getable(void);

/* 0.95V基准输入使能信号 (0.95V reference enable)
1：使能0.95V基准输入
0：关闭0.95V基准输入 相关函数 */
extern void SVD_VSR_V0P95EN_Setable(FunState NewState);
extern FunState SVD_VSR_V0P95EN_Getable(void);

/* 0.9V基准输入使能信号 (0.9V reference enable)
1：使能0.9V基准输入
0：关闭0.9V基准输入 相关函数 */
extern void SVD_VSR_V0P9EN_Setable(FunState NewState);
extern FunState SVD_VSR_V0P9EN_Getable(void);

extern void SVD_Init(SVD_InitTypeDef* para);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_SVD_H */
