/**
  ******************************************************************************
  * @file    fm33a0xxev_vrtc.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the VRTC firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_VRTC_H
#define __FM33A0XXEV_VRTC_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 

#define HXVAR(object, addr)   (*((object *) (addr)))
#define const_rcmf_Trim 	HXVAR( uint32_t, 0x1FFFFB44 )	//RCMF 2MHz调校值
   
#define	VRTC_PDRCR_CFG_Pos	0	/*  VRTC电源域下PDR配置*/
#define	VRTC_PDRCR_CFG_Msk	(0x1U << VRTC_PDRCR_CFG_Pos) 
      
/*0：关闭RCMF
1：使能RCMF
【注】当使能了RTC自动温补后，RCMF受硬件控制自行启动和关闭 */
#define	VRTC_RCMFCR_EN_Msk	(0x1U << VRTC_RCMFCR_EN_Pos)
	/* 关闭RCMF */
	/* 使能RCMF */
   
#define	VRTC_RCMFCR_EN_Pos	7	/* RCMF使能寄存器
0：关闭RCMF
1：使能RCMF
【注】当使能了RTC自动温补后，RCMF受硬件控制自行启动和关闭 */
#define	VRTC_RCMFCR_EN_Msk	(0x1U << VRTC_RCMFCR_EN_Pos)
	/* 关闭RCMF */
	/* 使能RCMF */

#define	VRTC_RCMFCR_TRIM_Pos	0	/* RCMF频率调校寄存器 */
#define	VRTC_RCMFCR_TRIM_Msk	(0x3fU << VRTC_RCMFCR_TRIM_Pos)

#define	VRTC_RCLPCR_CHOPEN_Pos	1	/* Chopper使能 */
#define	VRTC_RCLPCR_CHOPEN_Msk	(0x1U << VRTC_RCLPCR_CHOPEN_Pos)

#define	VRTC_RCLPCR_RCLP_OFF_Pos	0	/* RCLP使能信号
0：使能RCLP 
1：关闭RCLP
【注】XTLF异常停振时，自动使能RCLP */
#define	VRTC_RCLPCR_RCLP_OFF_Msk	(0x1U << VRTC_RCLPCR_RCLP_OFF_Pos)
	/* 使能RCLP  */
	/* 关闭RCLP */

#define	VRTC_RCLPTR_RCLP_TRIM_Pos	0	/* RCLP调校值寄存器
0000 0000：频率最低
1111 1111：频率最高 */
#define	VRTC_RCLPTR_RCLP_TRIM_Msk	(0xffU << VRTC_RCLPTR_RCLP_TRIM_Pos)

#define	VRTC_XTLFCR_XTLFIPW_Pos	0
#define	VRTC_XTLFCR_XTLFIPW_Msk	(0xfU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_850NA	(0x0U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_800NA	(0x1U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_750NA	(0x2U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_700NA	(0x3U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_650NA	(0x4U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_600NA	(0x5U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_550NA	(0x6U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_500NA	(0x7U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_450NA	(0x8U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_400NA	(0x9U << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_350NA	(0xaU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_300NA	(0xbU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_250NA	(0xcU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_200NA	(0xdU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_150NA	(0xeU << VRTC_XTLFCR_XTLFIPW_Pos)
#define	VRTC_XTLFCR_XTLFIPW_100NA	(0xfU << VRTC_XTLFCR_XTLFIPW_Pos)

#define	VRTC_ADCCR_CKS_Pos	1	/* ADC工作时钟选择寄存器
000：RCMF
001：RCMF/2
010：RCMF/4
011：RCMF/8
100：RCMF/16
101：RCMF/32
110：RCLP
111：XTLF */
#define	VRTC_ADCCR_CKS_Msk	(0x7U << VRTC_ADCCR_CKS_Pos)
#define	VRTC_ADCCR_CKS_RCMF	(0x0U << VRTC_ADCCR_CKS_Pos)	/* RCMF */
#define	VRTC_ADCCR_CKS_RCMF_2	(0x1U << VRTC_ADCCR_CKS_Pos)	/* RCMF/2 */
#define	VRTC_ADCCR_CKS_RCMF_4	(0x2U << VRTC_ADCCR_CKS_Pos)	/* RCMF/4 */
#define	VRTC_ADCCR_CKS_RCMF_8	(0x3U << VRTC_ADCCR_CKS_Pos)	/* RCMF/8 */
#define	VRTC_ADCCR_CKS_RCMF_16	(0x4U << VRTC_ADCCR_CKS_Pos)	/* RCMF/16 */
#define	VRTC_ADCCR_CKS_RCMF_32	(0x5U << VRTC_ADCCR_CKS_Pos)	/* RCMF/32 */
#define	VRTC_ADCCR_CKS_RCLP	(0x6U << VRTC_ADCCR_CKS_Pos)	/* RCLP */
#define	VRTC_ADCCR_CKS_XTLF	(0x7U << VRTC_ADCCR_CKS_Pos)	/* XTLF */

#define	VRTC_ADCCR_CKE_Pos	0	/* ADC工作时钟使能
0：关闭ADC工作时钟
1：使能ADC工作时钟 */
#define	VRTC_ADCCR_CKE_Msk	(0x1U << VRTC_ADCCR_CKE_Pos)
	/* 关闭ADC工作时钟 */
	/* 使能ADC工作时钟 */

#define	VRTC_LFDIER_LFDET_IE_Pos	0	/* XTLF低频检测报警中断使能，1有效 */
#define	VRTC_LFDIER_LFDET_IE_Msk	(0x1U << VRTC_LFDIER_LFDET_IE_Pos)

#define	VRTC_LFDISR_LFDETO_Pos	1	/* 低频停振检测状态输出
1：XTLF正常
0：XTLF停振 */
#define	VRTC_LFDISR_LFDETO_Msk	(0x1U << VRTC_LFDISR_LFDETO_Pos)

#define	VRTC_LFDISR_LFDET_IF_Pos	0	/* 低频停振检测中断标志寄存器，XTLF停振时硬件异步置位，软件写1清零；只有在LFDETO不为0的情况下才能够清除此寄存器 */
#define	VRTC_LFDISR_LFDET_IF_Msk	(0x1U << VRTC_LFDISR_LFDET_IF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void VRTC_Deinit(void);

/* RCMF使能寄存器
0：关闭RCMF
1：使能RCMF
【注】当使能了RTC自动温补后，RCMF受硬件控制自行启动和关闭 相关函数 */
extern void VRTC_RCMFCR_EN_Setable(FunState NewState);
extern FunState VRTC_RCMFCR_EN_Getable(void);

/* RCMF频率调校寄存器 相关函数 */
extern void VRTC_RCMFCR_TRIM_Set(uint32_t SetValue);
extern uint32_t VRTC_RCMFCR_TRIM_Get(void);

/* Chopper使能 相关函数 */
extern void VRTC_RCLPCR_CHOPEN_Setable(FunState NewState);
extern FunState VRTC_RCLPCR_CHOPEN_Getable(void);

/* RCLP使能信号
0：使能RCLP 
1：关闭RCLP
【注】XTLF异常停振时，自动使能RCLP 相关函数 */
extern void VRTC_RCLPCR_RCLP_OFF_Setable(FunState NewState);
extern FunState VRTC_RCLPCR_RCLP_OFF_Getable(void);

/* RCLP调校值寄存器
0000 0000：频率最低
1111 1111：频率最高 相关函数 */
extern void VRTC_RCLPTR_Write(uint32_t SetValue);
extern uint32_t VRTC_RCLPTR_Read(void);
extern void VRTC_XTLFCR_XTLFIPW_Set(uint32_t SetValue);
extern uint32_t VRTC_XTLFCR_XTLFIPW_Get(void);

/* ADC工作时钟选择寄存器
000：RCMF
001：RCMF/2
010：RCMF/4
011：RCMF/8
100：RCMF/16
101：RCMF/32
110：RCLP
111：XTLF 相关函数 */
extern void VRTC_ADCCR_CKS_Set(uint32_t SetValue);
extern uint32_t VRTC_ADCCR_CKS_Get(void);

/* ADC工作时钟使能
0：关闭ADC工作时钟
1：使能ADC工作时钟 相关函数 */
extern void VRTC_ADCCR_CKE_Setable(FunState NewState);
extern FunState VRTC_ADCCR_CKE_Getable(void);

/* XTLF低频检测报警中断使能，1有效 相关函数 */
extern void VRTC_LFDIER_LFDET_IE_Setable(FunState NewState);
extern FunState VRTC_LFDIER_LFDET_IE_Getable(void);

/* 低频停振检测状态输出
1：XTLF正常
0：XTLF停振 相关函数 */
extern FlagStatus VRTC_LFDISR_LFDETO_Chk(void);

/* 低频停振检测中断标志寄存器，XTLF停振时硬件异步置位，软件写1清零；只有在LFDETO不为0的情况下才能够清除此寄存器 相关函数 */
extern void VRTC_LFDISR_LFDET_IF_Clr(void);
extern FlagStatus VRTC_LFDISR_LFDET_IF_Chk(void);

extern void VRTC_Init_RCMF_Trim(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_VRTC_H */
