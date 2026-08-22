/**
  ******************************************************************************
  * @file    fm33a0xxev_cmu.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the CMU firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_CMU_H
#define __FM33A0XXEV_CMU_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
  

/* Defines------------------------------------------------------------------*/
#define HXVAR(object, addr)   (*((object *) (addr)))
#define const_rchf_Trim8 	HXVAR( uint32_t, 0x1FFFFB40 )	//RCHF 8MHz调校值
#define const_rchf_Trim16 	HXVAR( uint32_t, 0x1FFFFB3C )	//RCHF 16MHz调校值
#define const_rchf_Trim24 	HXVAR( uint32_t, 0x1FFFFB38 )	//RCHF 24MHz调校值
#define const_rchf_Trim32 	HXVAR( uint32_t, 0x1FFFFB34)   //RCHF 32MHz调校值
#define const_rchf_Trim40 	HXVAR( uint32_t, 0x1FFFFB48)   //RCHF 40MHz调校值
#define const_rchf_Trim48 	HXVAR( uint32_t, 0x1FFFFB4C)   //RCHF 48MHz调校值
	 
#define __XTHF_CLOCK                (8000000) 
/* Exported constants --------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
typedef struct
{
	uint32_t			FSEL;		/*!<RCHF频率  */
	FunState			RCHFEN;			/*!<RCHF使能控制  */
}CMU_RCHF_InitTypeDef;

typedef struct
{            
  uint32_t		PLLLDB;		/*!<PLL倍频比最大1023,pll倍频数 = PLLDB + 1  */
	FunState		PLLL_EN;		/*!<PLL使能控制  */
}CMU_PLL_L_InitTypeDef;


typedef struct
{            
  uint32_t		PLLHDB;		/*!<PLL倍频比最大1023,pll倍频数 = PLLDB + 1  */
	uint32_t 		REFPRSC;	/*!<PLLH参考时钟预分频  */
	uint32_t		PLLH_OSEL;	/*!<PLL输出选择，当需要输出超过1024倍时开启两倍输出模式  */
	uint32_t		PLLH_INSEL;	/*!<PLL输入源选择  */
	FunState		PLLH_EN;		/*!<PLL使能控制  */
}CMU_PLL_H_InitTypeDef;


typedef struct
{
  uint32_t			SYSCLKSEL;		/*!<系统时钟选择，从休眠状态唤醒后会自动恢复为RCHF8M  */	
	uint32_t			AHBPRES;		/*!<AHB时钟分频选择  */
	uint32_t			APBPRES;		/*!<APB时钟分频选择  */
	FunState			SLP_ENEXTI;		/*!<Sleep/DeepSleep模式下EXTI采样设置  */
	
}CMU_SYSCLK_InitTypeDef;

typedef struct
{
  uint32_t SYSCLK_Frequency;		/*!<  SYSCLK clock frequency expressed in Hz */
  uint32_t AHBCLK_Frequency;		/*!<  AHB clock frequency expressed in Hz  */
  uint32_t APBCLK_Frequency;		/*!<  APB clock frequency expressed in Hz  */
  uint32_t RCHF_Frequency;			/*!<  RCHF clock frequency expressed in Hz   */
  uint32_t PLL_H_Frequency;			/*!<  PLL H clock frequency expressed in Hz   */
  uint32_t XTHF_Frequency;			/*!<  XTHF clock frequency expressed in Hz   */
  uint32_t LSCLK_Frequency;			/*!<  LSCLK clock frequency expressed in Hz   */
}CMU_ClocksType;



#define	CMU_SYSCLKCR_SLP_ENEXTI_Pos	25
#define	CMU_SYSCLKCR_SLP_ENEXTI_Msk	(0x1U << CMU_SYSCLKCR_SLP_ENEXTI_Pos)

#define	CMU_SYSCLKCR_APBPRES_Pos	16	/* APB时钟分频选择 (APB1bus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 */
#define	CMU_SYSCLKCR_APBPRES_Msk	(0x7U << CMU_SYSCLKCR_APBPRES_Pos)
#define	CMU_SYSCLKCR_APBPRES_DIV1	(0x0U << CMU_SYSCLKCR_APBPRES_Pos)	/* 不分频 */
#define	CMU_SYSCLKCR_APBPRES_DIV2	(0x4U << CMU_SYSCLKCR_APBPRES_Pos)	/* 2分频 */
#define	CMU_SYSCLKCR_APBPRES_DIV4	(0x5U << CMU_SYSCLKCR_APBPRES_Pos)	/* 4分频 */
#define	CMU_SYSCLKCR_APBPRES_DIV8	(0x6U << CMU_SYSCLKCR_APBPRES_Pos)	/* 8分频 */
#define	CMU_SYSCLKCR_APBPRES_DIV16	(0x7U << CMU_SYSCLKCR_APBPRES_Pos)	/* 16分频 */

#define	CMU_SYSCLKCR_AHBPRES_Pos	8	/* AHB时钟分频选择 (AHB bus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 */
#define	CMU_SYSCLKCR_AHBPRES_Msk	(0x7U << CMU_SYSCLKCR_AHBPRES_Pos)
#define	CMU_SYSCLKCR_AHBPRES_DIV1	(0x0U << CMU_SYSCLKCR_AHBPRES_Pos)	/* 不分频 */
#define	CMU_SYSCLKCR_AHBPRES_DIV2	(0x4U << CMU_SYSCLKCR_AHBPRES_Pos)	/* 2分频 */
#define	CMU_SYSCLKCR_AHBPRES_DIV4	(0x5U << CMU_SYSCLKCR_AHBPRES_Pos)	/* 4分频 */
#define	CMU_SYSCLKCR_AHBPRES_DIV8	(0x6U << CMU_SYSCLKCR_AHBPRES_Pos)	/* 8分频 */
#define	CMU_SYSCLKCR_AHBPRES_DIV16	(0x7U << CMU_SYSCLKCR_AHBPRES_Pos)	/* 16分频 */

#define	CMU_SYSCLKCR_STCLKSEL_Pos	6	/* CPU内核systick工作时钟选择 (Systick clock select)
00：SCLK
01：LSCLK
10：RC4M
11：SYSCLK */
#define	CMU_SYSCLKCR_STCLKSEL_Msk	(0x3U << CMU_SYSCLKCR_STCLKSEL_Pos)
#define	CMU_SYSCLKCR_STCLKSEL_SCLK	(0x0U << CMU_SYSCLKCR_STCLKSEL_Pos)	/* SCLK */
#define	CMU_SYSCLKCR_STCLKSEL_LSCLK	(0x1U << CMU_SYSCLKCR_STCLKSEL_Pos)	/* LSCLK */
#define	CMU_SYSCLKCR_STCLKSEL_RC4M	(0x2U << CMU_SYSCLKCR_STCLKSEL_Pos)	/* RC4M */
#define	CMU_SYSCLKCR_STCLKSEL_SYSCLK	(0x3U << CMU_SYSCLKCR_STCLKSEL_Pos)	/* SYSCLK */

#define	CMU_SYSCLKCR_SYSCLKSEL_Pos	0	/* 系统时钟源选择 */
#define	CMU_SYSCLKCR_SYSCLKSEL_Msk	(0x7U << CMU_SYSCLKCR_SYSCLKSEL_Pos)
#define	CMU_SYSCLKCR_SYSCLKSEL_RCHF	(0x0U << CMU_SYSCLKCR_SYSCLKSEL_Pos)	/* RCHF */
#define	CMU_SYSCLKCR_SYSCLKSEL_XTHF	(0x1U << CMU_SYSCLKCR_SYSCLKSEL_Pos)	/* XTHF */
#define	CMU_SYSCLKCR_SYSCLKSEL_PLL_H	(0x2U << CMU_SYSCLKCR_SYSCLKSEL_Pos)	/* PLL_H */
#define	CMU_SYSCLKCR_SYSCLKSEL_LSCLK	(0x3U << CMU_SYSCLKCR_SYSCLKSEL_Pos)	/* LSCLK */

#define	CMU_RCHFCR_FSEL_Pos	16	/* RCHF频率选择寄存器0000：8MHz
0001：16MHz
0010：24MHz
0011：32MHz
0111：40MHz
1111：48MHz
其他：RFU */
#define	CMU_RCHFCR_FSEL_Msk	(0xfU << CMU_RCHFCR_FSEL_Pos)
#define	CMU_RCHFCR_FSEL_8MHZ	(0x0U << CMU_RCHFCR_FSEL_Pos)	/* 8MHz */
#define	CMU_RCHFCR_FSEL_16MHZ	(0x1U << CMU_RCHFCR_FSEL_Pos)	/* 16MHz */
#define	CMU_RCHFCR_FSEL_24MHZ	(0x2U << CMU_RCHFCR_FSEL_Pos)	/* 24MHz */
#define	CMU_RCHFCR_FSEL_32MHZ	(0x3U << CMU_RCHFCR_FSEL_Pos)	/* 32MHZ */
#define	CMU_RCHFCR_FSEL_40MHZ	(0x7U << CMU_RCHFCR_FSEL_Pos)	/* 40MHZ */
#define	CMU_RCHFCR_FSEL_48MHZ	(0xfU << CMU_RCHFCR_FSEL_Pos)	/* 48MHZ */

#define	CMU_RCHFCR_RCHFEN_Pos	0	/* RCHF使能寄存器 (RCHF Enable)
1：使能RCHF
0：关闭RCHF */
#define	CMU_RCHFCR_RCHFEN_Msk	(0x1U << CMU_RCHFCR_RCHFEN_Pos)
	/* 使能RCHF */
	/* 关闭RCHF */

#define	CMU_RCHFTR_RCHFTRIM_Pos	0	/* RCHF频率调校寄存器，8’h00表示频率最低，8’hFF表示频率最高，调校范围为中心频率+/-30%，调校步长为中心频率0.25%
上电后芯片自动从LDT0读取8MHz调校值并写入此寄存器
软件使用其他频率时，可以自行从LDT0指定地址读取调校信息并写入此寄存器，从而确保输出频率准确。 */
#define	CMU_RCHFTR_RCHFTRIM_Msk	(0xffU << CMU_RCHFTR_RCHFTRIM_Pos)

#define	CMU_PLLLCR_PLLDB_Pos	16
#define	CMU_PLLLCR_PLLDB_Msk	(0x3ffU << CMU_PLLLCR_PLLDB_Pos)

#define	CMU_PLLLCR_LOCKED_Pos	7
#define	CMU_PLLLCR_LOCKED_Msk	(0x1U << CMU_PLLLCR_LOCKED_Pos)

#define	CMU_PLLLCR_PLLEN_Pos	0	/* PLL使能寄存器
1：使能PLL
0：关闭PLL */
#define	CMU_PLLLCR_PLLEN_Msk	(0x1U << CMU_PLLLCR_PLLEN_Pos)

#define	CMU_PLLHCR_PLLHDB_Pos	16	/* PLLH倍频比，参考时钟1Mhz
0000011111：输出32倍频
0101111：输出48倍频 */
#define	CMU_PLLHCR_PLLHDB_Msk	(0x3ffU << CMU_PLLHCR_PLLHDB_Pos)
#define	CMU_PLLHCR_PLLHDB_X32	(0x1fU << CMU_PLLHCR_PLLHDB_Pos)	/* 输出32倍频 */
#define	CMU_PLLHCR_PLLHDB_X48	(0x2fU << CMU_PLLHCR_PLLHDB_Pos)	/* 输出48倍频 */
#define	CMU_PLLHCR_PLLHDB_X64	(0x3fU << CMU_PLLHCR_PLLHDB_Pos)	/* 输出64倍频 */

#define	CMU_PLLHCR_LOCKED_Pos	7	/* PLLH锁定标志，软件通过查询此寄存器确认PLL已经处于锁定状态
1：PLL已锁定
0：PLL未锁定 */
#define	CMU_PLLHCR_LOCKED_Msk	(0x1U << CMU_PLLHCR_LOCKED_Pos)

#define	CMU_PLLHCR_REFPRSC_Pos	4	/* PLLH参考时钟预分频（目标是产生1MHz参考时钟给PLL）
000：不分频
001：2分频
010：4分频
011：8分频
100：12分频
101：16分频
110：24分频
111：32分频 */
#define	CMU_PLLHCR_REFPRSC_Msk	(0x7U << CMU_PLLHCR_REFPRSC_Pos)
#define	CMU_PLLHCR_REFPRSC_DIV1	(0x0U << CMU_PLLHCR_REFPRSC_Pos)	/* 不分频 */
#define	CMU_PLLHCR_REFPRSC_DIV2	(0x1U << CMU_PLLHCR_REFPRSC_Pos)	/* 2分频 */
#define	CMU_PLLHCR_REFPRSC_DIV4	(0x2U << CMU_PLLHCR_REFPRSC_Pos)	/* 4分频 */
#define	CMU_PLLHCR_REFPRSC_DIV8	(0x3U << CMU_PLLHCR_REFPRSC_Pos)	/* 8分频 */
#define	CMU_PLLHCR_REFPRSC_DIV12	(0x4U << CMU_PLLHCR_REFPRSC_Pos)	/* 12分频 */
#define	CMU_PLLHCR_REFPRSC_DIV16	(0x5U << CMU_PLLHCR_REFPRSC_Pos)	/* 16分频 */
#define	CMU_PLLHCR_REFPRSC_DIV24	(0x6U << CMU_PLLHCR_REFPRSC_Pos)	/* 24分频 */
#define	CMU_PLLHCR_REFPRSC_DIV32	(0x7U << CMU_PLLHCR_REFPRSC_Pos)	/* 32分频 */

#define	CMU_PLLHCR_OSEL_Pos	3	/* PLLH输出选择寄存器
0：选择PLLH一倍输出作为数字电路内的PLL时钟
1：选择PLLH两倍输出作为数字电路内的PLL时钟 */
#define	CMU_PLLHCR_OSEL_Msk	(0x1U << CMU_PLLHCR_OSEL_Pos)
#define	CMU_PLLHCR_OSEL_X1	(0x0U << CMU_PLLHCR_OSEL_Pos)
#define	CMU_PLLHCR_OSEL_X2	(0x1U << CMU_PLLHCR_OSEL_Pos)

#define	CMU_PLLHCR_INSEL_Pos	1	/* PLLH输入选择寄存器
0：RCHF
1：XTHF */
#define	CMU_PLLHCR_INSEL_Msk	(0x1U << CMU_PLLHCR_INSEL_Pos)
#define	CMU_PLLHCR_INSEL_RCHF	(0x0U << CMU_PLLHCR_INSEL_Pos)
#define	CMU_PLLHCR_INSEL_XTHF	(0x1U << CMU_PLLHCR_INSEL_Pos)

#define	CMU_PLLHCR_EN_Pos	0	/* PLLH使能寄存器
1：使能PLLH
0：关闭PLLH */
#define	CMU_PLLHCR_EN_Msk	(0x1U << CMU_PLLHCR_EN_Pos)

#define	CMU_XTHFCR_XTHF_CFG_Pos	8	/* XTHF振荡强度配置
000：最弱
111：最强 */
#define	CMU_XTHFCR_XTHF_CFG_Msk	(0x7U << CMU_XTHFCR_XTHF_CFG_Pos)
#define	CMU_XTHFCR_XTHF_CFG_MIN	(0x0U << CMU_XTHFCR_XTHF_CFG_Pos)	/* 最弱 */
#define	CMU_XTHFCR_XTHF_CFG_MAX	(0x7U << CMU_XTHFCR_XTHF_CFG_Pos)	/* 最强 */

#define	CMU_XTHFCR_XTHFEN_Pos	0	/* XTHF使能寄存器
0：关闭XTHF
1：使能XTHF */
#define	CMU_XTHFCR_XTHFEN_Msk	(0x1U << CMU_XTHFCR_XTHFEN_Pos)
	/* 关闭XTHF */
	/* 使能XTHF */

#define	CMU_IER_SYSCSE_IE_Pos	1
#define	CMU_IER_SYSCSE_IE_Msk	(0x1U << CMU_IER_SYSCSE_IE_Pos)

#define	CMU_IER_HFDET_IE_Pos	0
#define	CMU_IER_HFDET_IE_Msk	(0x1U << CMU_IER_HFDET_IE_Pos)


#define	CMU_ISR_HFDETO_Pos	8	/* 高频晶体停振检测模块输出
1：XTHF未停振
0：XTHF停振 */
#define	CMU_ISR_HFDETO_Msk	(0x1U << CMU_ISR_HFDETO_Pos)

#define	CMU_ISR_SYSCSE_IF_Pos	1/* SYSCLK时钟选择错误中断标志寄存器；当被选中的时钟源没有使能时，此中断标志置位，软件写1清零。 */
#define	CMU_ISR_SYSCSE_IF_Msk	(0x1U << CMU_ISR_SYSCSE_IF_Pos)

#define	CMU_ISR_HFDET_IF_Pos	0	/* 高频停振检测中断标志寄存器，XTHF停振时硬件异步置位，软件写1清零；只有在FFDETO不为0的情况下才能够清除此寄存器 */
#define	CMU_ISR_HFDET_IF_Msk	(0x1U << CMU_ISR_HFDET_IF_Pos)

#define	CMU_PCLKCR1_COMP_PCE_Pos	9	/* COMP总线时钟使能，高使能 */
#define	CMU_PCLKCR1_COMP_PCE_Msk	(0x1U << CMU_PCLKCR1_COMP_PCE_Pos)

#define	CMU_PCLKCR1_SVD_PCE_Pos	8	/* SVD总线时钟使能，高使能 */
#define	CMU_PCLKCR1_SVD_PCE_Msk	(0x1U << CMU_PCLKCR1_SVD_PCE_Pos)

#define	CMU_PCLKCR1_PAD_PCE_Pos	7	/* PADCFG总线时钟使能，高使能 */
#define	CMU_PCLKCR1_PAD_PCE_Msk	(0x1U << CMU_PCLKCR1_PAD_PCE_Pos)

#define	CMU_PCLKCR1_ANAC_PCE_Pos	6	/* ANAC总线时钟使能，高使能
此寄存器用于控制SVD、比较器的总线时钟 */
#define	CMU_PCLKCR1_ANAC_PCE_Msk	(0x1U << CMU_PCLKCR1_ANAC_PCE_Pos)

#define	CMU_PCLKCR1_IWDT_PCE_Pos	5	/* IWDT总线时钟使能，高使能 */
#define	CMU_PCLKCR1_IWDT_PCE_Msk	(0x1U << CMU_PCLKCR1_IWDT_PCE_Pos)

#define	CMU_PCLKCR1_SCU_PCE_Pos	4	/* SCU总线时钟使能，高使能 */
#define	CMU_PCLKCR1_SCU_PCE_Msk	(0x1U << CMU_PCLKCR1_SCU_PCE_Pos)

#define	CMU_PCLKCR1_PMU_PCE_Pos	3	/* PMU总线时钟使能，高使能 */
#define	CMU_PCLKCR1_PMU_PCE_Msk	(0x1U << CMU_PCLKCR1_PMU_PCE_Pos)

#define	CMU_PCLKCR1_RTC_PCE_Pos	2	/* RTC总线时钟使能，高使能 */
#define	CMU_PCLKCR1_RTC_PCE_Msk	(0x1U << CMU_PCLKCR1_RTC_PCE_Pos)

#define	CMU_PCLKCR1_LPT_PCE_Pos	0	/*  _32总线时钟使能，高使能 */
#define	CMU_PCLKCR1_LPT_PCE_Msk	(0x1U << CMU_PCLKCR1_LPT_PCE_Pos)

#define	CMU_PCLKCR2_PAE_PCE_Pos	17
#define	CMU_PCLKCR2_PAE_PCE_Msk	(0x1U << CMU_PCLKCR2_PAE_PCE_Pos)

#define	CMU_PCLKCR2_SHA_PCE_Pos	16
#define	CMU_PCLKCR2_SHA_PCE_Msk	(0x1U << CMU_PCLKCR2_SHA_PCE_Pos)

#define	CMU_PCLKCR2_CIC_PCE_Pos	8	/* ADC总线时钟使能，高使能 */
#define	CMU_PCLKCR2_CIC_PCE_Msk	(0x1U << CMU_PCLKCR2_CIC_PCE_Pos)

#define	CMU_PCLKCR2_WWDT_PCE_Pos	7	/* WWDT总线时钟使能，高使能 */
#define	CMU_PCLKCR2_WWDT_PCE_Msk	(0x1U << CMU_PCLKCR2_WWDT_PCE_Pos)

#define	CMU_PCLKCR2_RAMBIST_PCE_Pos	6	/* RAMBIST总线时钟使能，高使能 */
#define	CMU_PCLKCR2_RAMBIST_PCE_Msk	(0x1U << CMU_PCLKCR2_RAMBIST_PCE_Pos)

#define	CMU_PCLKCR2_NVM_PCE_Pos	5	/* NVMIF（Flash擦写控制器）总线时钟使能，高使能 */
#define	CMU_PCLKCR2_NVM_PCE_Msk	(0x1U << CMU_PCLKCR2_NVM_PCE_Pos)

#define	CMU_PCLKCR2_DMA_PCE_Pos	4	/* DMA总线时钟使能，高使能 */
#define	CMU_PCLKCR2_DMA_PCE_Msk	(0x1U << CMU_PCLKCR2_DMA_PCE_Pos)

#define	CMU_PCLKCR2_LCD_PCE_Pos	3	/* LCD总线时钟使能，高使能 */
#define	CMU_PCLKCR2_LCD_PCE_Msk	(0x1U << CMU_PCLKCR2_LCD_PCE_Pos)

#define	CMU_PCLKCR2_AES_PCE_Pos	2	/* AES总线时钟使能，高使能 */
#define	CMU_PCLKCR2_AES_PCE_Msk	(0x1U << CMU_PCLKCR2_AES_PCE_Pos)

#define	CMU_PCLKCR2_TRNG_PCE_Pos	1	/* RNG总线时钟使能，高使能 */
#define	CMU_PCLKCR2_TRNG_PCE_Msk	(0x1U << CMU_PCLKCR2_TRNG_PCE_Pos)

#define	CMU_PCLKCR2_CRC_PCE_Pos	0	/* CRC总线时钟使能，高使能 */
#define	CMU_PCLKCR2_CRC_PCE_Msk	(0x1U << CMU_PCLKCR2_CRC_PCE_Pos)

#define	CMU_PCLKCR3_I2C1_PCE_Pos	25	/* I2C1总线时钟使能，高有效 */
#define	CMU_PCLKCR3_I2C1_PCE_Msk	(0x1U << CMU_PCLKCR3_I2C1_PCE_Pos)

#define	CMU_PCLKCR3_I2C0_PCE_Pos	24	/* I2C0总线时钟使能，高有效 */
#define	CMU_PCLKCR3_I2C0_PCE_Msk	(0x1U << CMU_PCLKCR3_I2C0_PCE_Pos)

#define	CMU_PCLKCR3_LPUART1_PCE_Pos	18	/* LPUART1总线时钟使能，高有效 */
#define	CMU_PCLKCR3_LPUART1_PCE_Msk	(0x1U << CMU_PCLKCR3_LPUART1_PCE_Pos)

#define	CMU_PCLKCR3_U7816_PCE_Pos	16	/* 7816总线时钟使能，高有效 */
#define	CMU_PCLKCR3_U7816_PCE_Msk	(0x1U << CMU_PCLKCR3_U7816_PCE_Pos)

#define	CMU_PCLKCR3_LPUART0_PCE_Pos	15	/* LPUART0总线时钟使能，高有效 */
#define	CMU_PCLKCR3_LPUART0_PCE_Msk	(0x1U << CMU_PCLKCR3_LPUART0_PCE_Pos)

#define	CMU_PCLKCR3_UICR_PCE_Pos	14	/* UART红外调制总线使能，高有效 */
#define	CMU_PCLKCR3_UICR_PCE_Msk	(0x1U << CMU_PCLKCR3_UICR_PCE_Pos)

#define	CMU_PCLKCR3_UART5_PCE_Pos	13	/* UART5总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART5_PCE_Msk	(0x1U << CMU_PCLKCR3_UART5_PCE_Pos)

#define	CMU_PCLKCR3_UART4_PCE_Pos	12	/* UART4总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART4_PCE_Msk	(0x1U << CMU_PCLKCR3_UART4_PCE_Pos)

#define	CMU_PCLKCR3_UART3_PCE_Pos	11	/* UART3总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART3_PCE_Msk	(0x1U << CMU_PCLKCR3_UART3_PCE_Pos)

#define	CMU_PCLKCR3_UART2_PCE_Pos	10	/* UART2总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART2_PCE_Msk	(0x1U << CMU_PCLKCR3_UART2_PCE_Pos)

#define	CMU_PCLKCR3_UART1_PCE_Pos	9	/* UART1总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART1_PCE_Msk	(0x1U << CMU_PCLKCR3_UART1_PCE_Pos)

#define	CMU_PCLKCR3_UART0_PCE_Pos	8	/* UART0总线时钟使能，高有效 */
#define	CMU_PCLKCR3_UART0_PCE_Msk	(0x1U << CMU_PCLKCR3_UART0_PCE_Pos)

#define	CMU_PCLKCR3_QSPI_PCE_Pos	7	/* QuadSPI总线时钟使能，高有效 */
#define	CMU_PCLKCR3_QSPI_PCE_Msk	(0x1U << CMU_PCLKCR3_QSPI_PCE_Pos)

#define	CMU_PCLKCR3_SPI4_PCE_Pos	4	/* SPI4总线时钟使能，高有效 */
#define	CMU_PCLKCR3_SPI4_PCE_Msk	(0x1U << CMU_PCLKCR3_SPI4_PCE_Pos)

#define	CMU_PCLKCR3_SPI3_PCE_Pos	3	/* SPI3总线时钟使能，高有效 */
#define	CMU_PCLKCR3_SPI3_PCE_Msk	(0x1U << CMU_PCLKCR3_SPI3_PCE_Pos)

#define	CMU_PCLKCR3_SPI2_PCE_Pos	2	/* SPI2总线时钟使能，高有效 */
#define	CMU_PCLKCR3_SPI2_PCE_Msk	(0x1U << CMU_PCLKCR3_SPI2_PCE_Pos)

#define	CMU_PCLKCR3_SPI1_PCE_Pos	1	/* SPI1总线时钟使能，高有效 */
#define	CMU_PCLKCR3_SPI1_PCE_Msk	(0x1U << CMU_PCLKCR3_SPI1_PCE_Pos)

#define	CMU_PCLKCR3_SPI0_PCE_Pos	0	/* SPI0总线时钟使能，高有效 */
#define	CMU_PCLKCR3_SPI0_PCE_Msk	(0x1U << CMU_PCLKCR3_SPI0_PCE_Pos)

#define	CMU_PCLKCR4_ET4_PCE_Pos	6	/* Etimer4时钟使能，高使能 */
#define	CMU_PCLKCR4_ET4_PCE_Msk	(0x1U << CMU_PCLKCR4_ET4_PCE_Pos)

#define	CMU_PCLKCR4_ET3_PCE_Pos	5	/* Etimer3时钟使能，高使能 */
#define	CMU_PCLKCR4_ET3_PCE_Msk	(0x1U << CMU_PCLKCR4_ET3_PCE_Pos)

#define	CMU_PCLKCR4_ET2_PCE_Pos	4	/* Etimer2时钟使能，高使能 */
#define	CMU_PCLKCR4_ET2_PCE_Msk	(0x1U << CMU_PCLKCR4_ET2_PCE_Pos)

#define	CMU_PCLKCR4_ET1_PCE_Pos	3	/* Etimer1时钟使能，高使能 */
#define	CMU_PCLKCR4_ET1_PCE_Msk	(0x1U << CMU_PCLKCR4_ET1_PCE_Pos)

#define	CMU_PCLKCR4_BT2_PCE_Pos	2	/* BaseTimer2时钟使能，高使能 */
#define	CMU_PCLKCR4_BT2_PCE_Msk	(0x1U << CMU_PCLKCR4_BT2_PCE_Pos)

#define	CMU_PCLKCR4_BT1_PCE_Pos	1	/* BaseTimer1时钟使能，高使能 */
#define	CMU_PCLKCR4_BT1_PCE_Msk	(0x1U << CMU_PCLKCR4_BT1_PCE_Pos)

#define	CMU_PCLKCR4_BSTIM_PCE_Pos	0	/* 基本定时器BSTIM总线时钟使能，高使能 */
#define	CMU_PCLKCR4_BSTIM_PCE_Msk	(0x1U << CMU_PCLKCR4_BSTIM_PCE_Pos)

#define	CMU_OPCCR1_EXTICKE_Pos	31	/* EXTI工作时钟使能，高有效 */
#define	CMU_OPCCR1_EXTICKE_Msk	(0x1U << CMU_OPCCR1_EXTICKE_Pos)

#define	CMU_OPCCR1_EXTICKSEL_Pos	30	/* EXTI中断采样时钟选择 */
#define	CMU_OPCCR1_EXTICKSEL_Msk	(0x1U << CMU_OPCCR1_EXTICKSEL_Pos)
#define	CMU_OPCCR1_EXTICKSEL_LSCLK	(0x1U << CMU_OPCCR1_EXTICKSEL_Pos) /* 1：外部引脚中断使用LSCLK采样 */
#define	CMU_OPCCR1_EXTICKSEL_HCLK	  (0x0U << CMU_OPCCR1_EXTICKSEL_Pos) /* 0：外部引脚中断使用HCLK采样 */

#define	CMU_OPCCR1_LPUART1CKE_Pos	29	/* LPUART1工作时钟使能，高有效 */
#define	CMU_OPCCR1_LPUART1CKE_Msk	(0x1U << CMU_OPCCR1_LPUART1CKE_Pos)

#define	CMU_OPCCR1_LPUART0CKE_Pos	28	/* LPUART0工作时钟使能，高有效 */
#define	CMU_OPCCR1_LPUART0CKE_Msk	(0x1U << CMU_OPCCR1_LPUART0CKE_Pos)

#define	CMU_OPCCR1_LPUART1CKS_Pos	26	/* LPUART1工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU */
#define	CMU_OPCCR1_LPUART1CKS_Msk	(0x3U << CMU_OPCCR1_LPUART1CKS_Pos)
#define	CMU_OPCCR1_LPUART1CKS_LSCLK	(0x0U << CMU_OPCCR1_LPUART1CKS_Pos)	/* LSCLK */
#define	CMU_OPCCR1_LPUART1CKS_RCHF	(0x1U << CMU_OPCCR1_LPUART1CKS_Pos)	/* RCHF分频（根据RCHF档位自动分频到32768Hz附近） */

#define	CMU_OPCCR1_LPUART0CKS_Pos	24	/* LPUART0工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU */
#define	CMU_OPCCR1_LPUART0CKS_Msk	(0x3U << CMU_OPCCR1_LPUART0CKS_Pos)
#define	CMU_OPCCR1_LPUART0CKS_LSCLK	(0x0U << CMU_OPCCR1_LPUART0CKS_Pos)	/* LSCLK */
#define	CMU_OPCCR1_LPUART0CKS_RCHF	(0x1U << CMU_OPCCR1_LPUART0CKS_Pos)	/* RCHF分频（根据RCHF档位自动分频到32768Hz附近） */

#define	CMU_OPCCR1_I2C1CKE_Pos	21	/* I2C1工作时钟使能 */
#define	CMU_OPCCR1_I2C1CKE_Msk	(0x1U << CMU_OPCCR1_I2C1CKE_Pos)

#define	CMU_OPCCR1_I2C0CKE_Pos	20	/* I2C0工作时钟使能 */
#define	CMU_OPCCR1_I2C0CKE_Msk	(0x1U << CMU_OPCCR1_I2C0CKE_Pos)

#define	CMU_OPCCR1_I2C1CKS_Pos	18	/* I2C1主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK */
#define	CMU_OPCCR1_I2C1CKS_Msk	(0x3U << CMU_OPCCR1_I2C1CKS_Pos)
#define	CMU_OPCCR1_I2C1CKS_APBCLK	(0x0U << CMU_OPCCR1_I2C1CKS_Pos)	/* APBCLK */
#define	CMU_OPCCR1_I2C1CKS_RCHF	(0x1U << CMU_OPCCR1_I2C1CKS_Pos)	/* RCHF */
#define	CMU_OPCCR1_I2C1CKS_SYSCLK	(0x2U << CMU_OPCCR1_I2C1CKS_Pos)	/* SYSCLK */

#define	CMU_OPCCR1_I2C0CKS_Pos	16	/* I2C0主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK */
#define	CMU_OPCCR1_I2C0CKS_Msk	(0x3U << CMU_OPCCR1_I2C0CKS_Pos)
#define	CMU_OPCCR1_I2C0CKS_APBCLK	(0x0U << CMU_OPCCR1_I2C0CKS_Pos)	/* APBCLK */
#define	CMU_OPCCR1_I2C0CKS_RCHF	(0x1U << CMU_OPCCR1_I2C0CKS_Pos)	/* RCHF */
#define	CMU_OPCCR1_I2C0CKS_SYSCLK	(0x2U << CMU_OPCCR1_I2C0CKS_Pos)	/* SYSCLK */

#define	CMU_OPCCR1_UART1CKE_Pos	9	/* UART1工作时钟使能，高有效 */
#define	CMU_OPCCR1_UART1CKE_Msk	(0x1U << CMU_OPCCR1_UART1CKE_Pos)

#define	CMU_OPCCR1_UART0CKE_Pos	8	/* UART0工作时钟使能，高有效 */
#define	CMU_OPCCR1_UART0CKE_Msk	(0x1U << CMU_OPCCR1_UART0CKE_Pos)

#define	CMU_OPCCR1_UART1CKS_Pos	2	/* UART1工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU */
#define	CMU_OPCCR1_UART1CKS_Msk	(0x3U << CMU_OPCCR1_UART1CKS_Pos)
#define	CMU_OPCCR1_UART1CKS_APBCLK	(0x0U << CMU_OPCCR1_UART1CKS_Pos)	/* APBCLK */
#define	CMU_OPCCR1_UART1CKS_RCHF	(0x1U << CMU_OPCCR1_UART1CKS_Pos)	/* RCHF */
#define	CMU_OPCCR1_UART1CKS_SYSCLK	(0x2U << CMU_OPCCR1_UART1CKS_Pos)	/* SYSCLK */

#define	CMU_OPCCR1_UART0CKS_Pos	0	/* UART0工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU */
#define	CMU_OPCCR1_UART0CKS_Msk	(0x3U << CMU_OPCCR1_UART0CKS_Pos)
#define	CMU_OPCCR1_UART0CKS_APBCLK	(0x0U << CMU_OPCCR1_UART0CKS_Pos)	/* APBCLK */
#define	CMU_OPCCR1_UART0CKS_RCHF	(0x1U << CMU_OPCCR1_UART0CKS_Pos)	/* RCHF */
#define	CMU_OPCCR1_UART0CKS_SYSCLK	(0x2U << CMU_OPCCR1_UART0CKS_Pos)	/* SYSCLK */

#define	CMU_OPCCR2_RNGPRSC_Pos	28	/* 随机数发生器工作时钟分频
000：不分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110, 111：RFU */
#define	CMU_OPCCR2_RNGPRSC_Msk	(0x7U << CMU_OPCCR2_RNGPRSC_Pos)
#define	CMU_OPCCR2_RNGPRSC_DIV1	(0x0U << CMU_OPCCR2_RNGPRSC_Pos)	/* 不分频 */
#define	CMU_OPCCR2_RNGPRSC_DIV2	(0x1U << CMU_OPCCR2_RNGPRSC_Pos)	/* 2分频 */
#define	CMU_OPCCR2_RNGPRSC_DIV4	(0x2U << CMU_OPCCR2_RNGPRSC_Pos)	/* 4分频 */
#define	CMU_OPCCR2_RNGPRSC_DIV8	(0x3U << CMU_OPCCR2_RNGPRSC_Pos)	/* 8分频 */
#define	CMU_OPCCR2_RNGPRSC_DIV16	(0x4U << CMU_OPCCR2_RNGPRSC_Pos)	/* 16分频 */
#define	CMU_OPCCR2_RNGPRSC_DIV32	(0x5U << CMU_OPCCR2_RNGPRSC_Pos)	/* 32分频 */

#define	CMU_OPCCR2_NVMCKE_Pos	22	/* Flash擦写时钟使能，高有效 */
#define	CMU_OPCCR2_NVMCKE_Msk	(0x1U << CMU_OPCCR2_NVMCKE_Pos)

#define	CMU_OPCCR2_RNGCKE_Pos	21	/* 随机数发生器工作时钟使能，高有效 */
#define	CMU_OPCCR2_RNGCKE_Msk	(0x1U << CMU_OPCCR2_RNGCKE_Pos)

#define	CMU_OPCCR2_LPTCKE_Pos	12	/*  _工作时钟使能，高有效 */
#define	CMU_OPCCR2_LPTCKE_Msk	(0x1U << CMU_OPCCR2_LPTCKE_Pos)

#define	CMU_OPCCR2_LPTCKS_Pos	8	/*  _工作时钟选择
00：APBCLK
01：LSCLK
10：RCLP
11：PLL_L */
#define	CMU_OPCCR2_LPTCKS_Msk	(0x3U << CMU_OPCCR2_LPTCKS_Pos)
#define	CMU_OPCCR2_LPTCKS_APBCLK	(0x0U << CMU_OPCCR2_LPTCKS_Pos)	/* APBCLK */
#define	CMU_OPCCR2_LPTCKS_LSCLK	(0x1U << CMU_OPCCR2_LPTCKS_Pos)	/* LSCLK */
#define	CMU_OPCCR2_LPTCKS_RCLP	(0x2U << CMU_OPCCR2_LPTCKS_Pos)	/* RCLP */
#define	CMU_OPCCR2_LPTCKS_PLL_L	(0x3U << CMU_OPCCR2_LPTCKS_Pos)	/* PLL_L */

#define	CMU_OPCCR2_BSTCKE_Pos	4	/* BSTIM工作时钟使能，高有效 */
#define	CMU_OPCCR2_BSTCKE_Msk	(0x1U << CMU_OPCCR2_BSTCKE_Pos)

#define	CMU_OPCCR2_BSTCKS_Pos	0	/* BSTIM工作时钟源选择
00：APBCLK
01：LSCLK
10：RCLP
11：SYSCLK */
#define	CMU_OPCCR2_BSTCKS_Msk	(0x3U << CMU_OPCCR2_BSTCKS_Pos)
#define	CMU_OPCCR2_BSTCKS_APBCLK	(0x0U << CMU_OPCCR2_BSTCKS_Pos)	/* APBCLK */
#define	CMU_OPCCR2_BSTCKS_LSCLK	(0x1U << CMU_OPCCR2_BSTCKS_Pos)	/* LSCLK */
#define	CMU_OPCCR2_BSTCKS_RCLP	(0x2U << CMU_OPCCR2_BSTCKS_Pos)	/* RCLP */
#define	CMU_OPCCR2_BSTCKS_SYSCLK	(0x3U << CMU_OPCCR2_BSTCKS_Pos)	/* SYSCLK */
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void CMU_Deinit(void);
extern void CMU_SYSCLKCR_SLP_ENEXTI_Setable(FunState NewState);
extern FunState CMU_SYSCLKCR_SLP_ENEXTI_Getable(void);

/* APB时钟分频选择 (APBbus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 相关函数 */
extern void CMU_SYSCLKCR_APBPRES_Set(uint32_t SetValue);
extern uint32_t CMU_SYSCLKCR_APBPRES_Get(void);

/* AHB时钟分频选择 (AHB bus clock Prescaler)
000/001/010/011：不分频
100：2分频
101：4分频
110：8分频
111：16分频 相关函数 */
extern void CMU_SYSCLKCR_AHBPRES_Set(uint32_t SetValue);
extern uint32_t CMU_SYSCLKCR_AHBPRES_Get(void);

/* CPU内核systick工作时钟选择 (Systick clock select)
00：SCLK
01：LSCLK
10：RC4M
11：SYSCLK 相关函数 */
extern void CMU_SYSCLKCR_STCLKSEL_Set(uint32_t SetValue);
extern uint32_t CMU_SYSCLKCR_STCLKSEL_Get(void);

/* 系统时钟源选择 相关函数 */
extern void CMU_SYSCLKCR_SYSCLKSEL_Set(uint32_t SetValue);
extern uint32_t CMU_SYSCLKCR_SYSCLKSEL_Get(void);

/* RCHF频率选择寄存器0000：8MHz
0001：16MHz
0010：24MHz
0011：32MHz
0111：40MHz
1111：48MHz
其他：RFU 相关函数 */
extern void CMU_RCHFCR_FSEL_Set(uint32_t SetValue);
extern uint32_t CMU_RCHFCR_FSEL_Get(void);

/* RCHF使能寄存器 (RCHF Enable)
1：使能RCHF
0：关闭RCHF 相关函数 */
extern void CMU_RCHFCR_RCHFEN_Setable(FunState NewState);
extern FunState CMU_RCHFCR_RCHFEN_Getable(void);

/* RCHF频率调校寄存器，8’h00表示频率最低，8’hFF表示频率最高，调校范围为中心频率+/-30%，调校步长为中心频率0.25%
上电后芯片自动从LDT0读取8MHz调校值并写入此寄存器
软件使用其他频率时，可以自行从LDT0指定地址读取调校信息并写入此寄存器，从而确保输出频率准确。 相关函数 */
extern void CMU_RCHFTR_RCHFTRIM_Set(uint32_t SetValue);
extern uint32_t CMU_RCHFTR_RCHFTRIM_Get(void);
extern FlagStatus CMU_PLLLCR_LOCKED_Chk(void);
extern void CMU_PLLLCR_PLLDB_Set(uint32_t SetValue);
extern uint32_t CMU_PLLLCR_PLLDB_Get(void);

/* PLL使能寄存器
1：使能PLL
0：关闭PLL 相关函数 */
extern void CMU_PLLLCR_PLLEN_Setable(FunState NewState);
extern FunState CMU_PLLLCR_PLLEN_Getable(void);

/* PLLH倍频比，参考时钟1Mhz
0000011111：输出32倍频
0101111：输出48倍频 相关函数 */
extern void CMU_PLLHCR_PLLHDB_Set(uint32_t SetValue);
extern uint32_t CMU_PLLHCR_PLLHDB_Get(void);

/* PLLH锁定标志，软件通过查询此寄存器确认PLL已经处于锁定状态
1：PLL已锁定
0：PLL未锁定 相关函数 */
extern FlagStatus CMU_PLLHCR_LOCKED_Chk(void);

/* PLLH参考时钟预分频（目标是产生1MHz参考时钟给PLL）
000：不分频
001：2分频
010：4分频
011：8分频
100：12分频
101：16分频
110：24分频
111：32分频 相关函数 */
extern void CMU_PLLHCR_REFPRSC_Set(uint32_t SetValue);
extern uint32_t CMU_PLLHCR_REFPRSC_Get(void);

/* PLLH输出选择寄存器
0：选择PLLH一倍输出作为数字电路内的PLL时钟
1：选择PLLH两倍输出作为数字电路内的PLL时钟 相关函数 */
extern void CMU_PLLHCR_OSEL_Set(uint32_t SetValue);
extern uint32_t CMU_PLLHCR_OSEL_Get(void);

/* PLLH输入选择寄存器
00/11：RCHF
01：PLL_L
10：XTHF 相关函数 */
extern void CMU_PLLHCR_INSEL_Set(uint32_t SetValue);
extern uint32_t CMU_PLLHCR_INSEL_Get(void);

/* PLLH使能寄存器
1：使能PLLH
0：关闭PLLH 相关函数 */
extern void CMU_PLLHCR_EN_Setable(FunState NewState);
extern FunState CMU_PLLHCR_EN_Getable(void);

/* XTHF振荡强度配置
000：最弱
111：最强 相关函数 */
extern void CMU_XTHFCR_XTHF_CFG_Set(uint32_t SetValue);
extern uint32_t CMU_XTHFCR_XTHF_CFG_Get(void);

/* XTHF使能寄存器
0：关闭XTHF
1：使能XTHF 相关函数 */
extern void CMU_XTHFCR_XTHFEN_Setable(FunState NewState);
extern FunState CMU_XTHFCR_XTHFEN_Getable(void);
extern void CMU_IER_SYSCSE_IE_Setable(FunState NewState);
extern FunState CMU_IER_SYSCSE_IE_Getable(void);
extern void CMU_IER_HFDET_IE_Setable(FunState NewState);
extern FunState CMU_IER_HFDET_IE_Getable(void);


/* 高频晶体停振检测模块输出
1：XTHF未停振
0：XTHF停振 相关函数 */
extern FlagStatus CMU_ISR_HFDETO_Chk(void);

/* 低频晶体停振检测模块输出
1：XTLF未停振
0：XTLF停振 相关函数 */
extern FlagStatus CMU_ISR_LFDETO_Chk(void);

/* SYSCLK时钟选择错误中断标志寄存器；当被选中的时钟源没有使能时，此中断标志置位，软件写1清零。 相关函数 */
extern void CMU_ISR_SYSCSE_IF_Clr(void);
extern FlagStatus CMU_ISR_SYSCSE_IF_Chk(void);

/* 高频停振检测中断标志寄存器，XTHF停振时硬件异步置位，软件写1清零；只有在FFDETO不为0的情况下才能够清除此寄存器 相关函数 */
extern void CMU_ISR_HFDET_IF_Clr(void);
extern FlagStatus CMU_ISR_HFDET_IF_Chk(void);

/* EXTI工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR1_EXTICKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_EXTICKE_Getable(void);

/* EXTI中断采样时钟选择 */
extern void CMU_OPCCR1_EXTICKSEL_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_EXTICKSEL_Get(void);

/* LPUART1工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR1_LPUART1CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_LPUART1CKE_Getable(void);

/* LPUART0工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR1_LPUART0CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_LPUART0CKE_Getable(void);

/* LPUART1工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU 相关函数 */
extern void CMU_OPCCR1_LPUART1CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_LPUART1CKS_Get(void);

/* LPUART0工作时钟选择
00：LSCLK
01：RCHF分频（根据RCHF档位自动分频到32768Hz附近）
10：RFU
11：RFU 相关函数 */
extern void CMU_OPCCR1_LPUART0CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_LPUART0CKS_Get(void);

/* I2C1工作时钟使能 相关函数 */
extern void CMU_OPCCR1_I2C1CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_I2C1CKE_Getable(void);

/* I2C0工作时钟使能 相关函数 */
extern void CMU_OPCCR1_I2C0CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_I2C0CKE_Getable(void);

/* I2C1主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK 相关函数 */
extern void CMU_OPCCR1_I2C1CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_I2C1CKS_Get(void);


/* I2C0主机工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：APBCLK 相关函数 */
extern void CMU_OPCCR1_I2C0CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_I2C0CKS_Get(void);

/* UART1工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR1_UART1CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_UART1CKE_Getable(void);

/* UART0工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR1_UART0CKE_Setable(FunState NewState);
extern FunState CMU_OPCCR1_UART0CKE_Getable(void);

/* UART1工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU 相关函数 */
extern void CMU_OPCCR1_UART1CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_UART1CKS_Get(void);

/* UART0工作时钟选择
00：APBCLK
01：RCHF
10：SYSCLK
11：RFU 相关函数 */
extern void CMU_OPCCR1_UART0CKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR1_UART0CKS_Get(void);

/* 随机数发生器工作时钟分频
000：不分频
001：2分频
010：4分频
011：8分频
100：16分频
101：32分频
110, 111：RFU 相关函数 */
extern void CMU_OPCCR2_RNGPRSC_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR2_RNGPRSC_Get(void);

/* Flash擦写时钟使能，高有效 相关函数 */
extern void CMU_OPCCR2_NVMCKE_Setable(FunState NewState);
extern FunState CMU_OPCCR2_NVMCKE_Getable(void);

/* 随机数发生器工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR2_RNGCKE_Setable(FunState NewState);
extern FunState CMU_OPCCR2_RNGCKE_Getable(void);

/*  _工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR2_LPTCKE_Setable(FunState NewState);
extern FunState CMU_OPCCR2_LPTCKE_Getable(void);

/*  _工作时钟选择
00：APBCLK
01：LSCLK
10：RCLP
11：PLL_L 相关函数 */
extern void CMU_OPCCR2_LPTCKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR2_LPTCKS_Get(void);

/* BSTIM工作时钟使能，高有效 相关函数 */
extern void CMU_OPCCR2_BSTCKE_Setable(FunState NewState);
extern FunState CMU_OPCCR2_BSTCKE_Getable(void);

/* BSTIM工作时钟源选择
00：APBCLK
01：LSCLK
10：RCLP
11：SYSCLK 相关函数 */
extern void CMU_OPCCR2_BSTCKS_Set(uint32_t SetValue);
extern uint32_t CMU_OPCCR2_BSTCKS_Get(void);
//Announce_End

/* 获取芯片已设置的不同时钟频率函数 */
extern void CMU_GetClocksFreq(CMU_ClocksType* para);

/* RCHF初始化函数 */
extern void CMU_RCHF_Init(CMU_RCHF_InitTypeDef* para);

/* PLL H初始化函数 */
void CMU_PLL_H_Init(CMU_PLL_H_InitTypeDef* para);

/* rchf常温校准值载入函数 */
extern void CMU_Init_RCHF_Trim( uint8_t ClkMode );

/* 系统时钟初始化 */
extern void CMU_SysClk_Init(CMU_SYSCLK_InitTypeDef* para);

/* 外设时钟控制参数定义 */
#define	COMPCLK		  (0x01000000 + CMU_PCLKCR1_COMP_PCE_Pos)	//9	/* COMP总线时钟使能，高使能 */
#define	SVDCLK		  (0x01000000 + CMU_PCLKCR1_SVD_PCE_Pos)	//8	/* SVD总线时钟使能，高使能 */
#define	PADCLK			(0x01000000 + CMU_PCLKCR1_PAD_PCE_Pos)	//7	/* PADCFG总线时钟使能，高使能 */
#define	ANACCLK			(0x01000000 + CMU_PCLKCR1_ANAC_PCE_Pos)	//6	/* ANAC总线时钟使能，高使能此寄存器用于控制SVD、比较器的总线时钟 */
#define	IWDTCLK			(0x01000000 + CMU_PCLKCR1_IWDT_PCE_Pos)	//5	/* IWDT总线时钟使能，高使能 */
#define	SCUCLK			(0x01000000 + CMU_PCLKCR1_SCU_PCE_Pos)	//4	/* SCU总线时钟使能，高使能 */
#define	PMUCLK			(0x01000000 + CMU_PCLKCR1_PMU_PCE_Pos)	//3	/* PMU总线时钟使能，高使能 */
#define	RTCCLK			(0x01000000 + CMU_PCLKCR1_RTC_PCE_Pos)	//2	/* RTC总线时钟使能，高使能 */
#define	LPTCLK			(0x01000000 + CMU_PCLKCR1_LPT_PCE_Pos)	//0	/* LPTIM总线时钟使能，高使能 */
        

#define	PAECLK			(0x02000000 + CMU_PCLKCR2_PAE_PCE_Pos)	//17	/* PAE总线时钟使能，高使能 */
#define	SHACLK			(0x02000000 + CMU_PCLKCR2_SHA_PCE_Pos)	//16	/* SHA总线时钟使能，高使能 */
#define	CICCLK			(0x02000000 + CMU_PCLKCR2_CIC_PCE_Pos)	//8	  /* CIC总线时钟使能，高使能 */
#define	WWDTCLK			(0x02000000 + CMU_PCLKCR2_WWDT_PCE_Pos)	//7	/* WWDT时钟使能，高使能 */
#define	RAMBISTCLK	(0x02000000 + CMU_PCLKCR2_RAMBIST_PCE_Pos)	//6	/* RAMBIST时钟使能，高使能 */
#define	FLASHCLK		(0x02000000 + CMU_PCLKCR2_NVM_PCE_Pos)	//5	/* FLSC（Flash擦写控制器）时钟使能，高使能 */
#define	DMACLK			(0x02000000 + CMU_PCLKCR2_DMA_PCE_Pos)	//4	/* DMA时钟使能，高使能 */
#define	LCDCLK			(0x02000000 + CMU_PCLKCR2_LCD_PCE_Pos)	//3	/* LCD时钟使能，高使能 */
#define	AESCLK			(0x02000000 + CMU_PCLKCR2_AES_PCE_Pos)	//2	/* AES时钟使能，高使能 */
#define	TRNGCLK			(0x02000000 + CMU_PCLKCR2_TRNG_PCE_Pos)	//1	/* RNG时钟使能，高使能 */
#define	CRCCLK			(0x02000000 + CMU_PCLKCR2_CRC_PCE_Pos)	//0	/* CRC时钟使能，高使能 */

#define	I2C1CLK			(0x03000000 + CMU_PCLKCR3_I2C1_PCE_Pos)	//25	/* I2C1时钟使能 */
#define	I2C0CLK			(0x03000000 + CMU_PCLKCR3_I2C0_PCE_Pos)	//24	/* I2C0时钟使能 */
#define	LPUART1CLK	(0x03000000 + CMU_PCLKCR3_LPUART1_PCE_Pos)	//18	/* LPUART1功能时钟使能 */
#define	U7816CLK		(0x03000000 + CMU_PCLKCR3_U7816_PCE_Pos)	//16	/* U7816-0时钟使能 */
#define	LPUART0CLK	(0x03000000 + CMU_PCLKCR3_LPUART0_PCE_Pos)	//15	/* LPUART0寄存器总线时钟使能 */
#define	UARTIRCLK	   (0x03000000 + CMU_PCLKCR3_UICR_PCE_Pos)	//14	/* UART红外总线使能，高使能*/
#define	UART5CLK		(0x03000000 + CMU_PCLKCR3_UART5_PCE_Pos)	//13	/* UART5时钟使能 */
#define	UART4CLK		(0x03000000 + CMU_PCLKCR3_UART4_PCE_Pos)	//12	/* UART4时钟使能 */
#define	UART3CLK		(0x03000000 + CMU_PCLKCR3_UART3_PCE_Pos)	//11	/* UART3时钟使能 */
#define	UART2CLK		(0x03000000 + CMU_PCLKCR3_UART2_PCE_Pos)	//10	/* UART2时钟使能 */
#define	UART1CLK		(0x03000000 + CMU_PCLKCR3_UART1_PCE_Pos)	//9	/* UART1时钟使能 */
#define	UART0CLK		(0x03000000 + CMU_PCLKCR3_UART0_PCE_Pos)	//8	/* UART0时钟使能 */
#define	QSPICLK			(0x03000000 + CMU_PCLKCR3_QSPI_PCE_Pos)	//7	/* QSPI总线时钟使能 */
#define	SPI4CLK			(0x03000000 + CMU_PCLKCR3_SPI4_PCE_Pos)	//4	/* SPI4时钟使能 */
#define	SPI3CLK			(0x03000000 + CMU_PCLKCR3_SPI3_PCE_Pos)	//3	/* SPI3时钟使能 */
#define	SPI2CLK			(0x03000000 + CMU_PCLKCR3_SPI2_PCE_Pos)	//2	/* SPI2时钟使能 */
#define	SPI1CLK			(0x03000000 + CMU_PCLKCR3_SPI1_PCE_Pos)	//1	/* SPI1时钟使能 */
#define	SPI0CLK			(0x03000000 + CMU_PCLKCR3_SPI0_PCE_Pos)	//0	/* SPI0时钟使能 */


#define	ET4CLK			(0x04000000 + CMU_PCLKCR4_ET4_PCE_Pos)	//6	/* Etimer4时钟使能，高使能 */
#define	ET3CLK			(0x04000000 + CMU_PCLKCR4_ET3_PCE_Pos)	//5	/* Etimer3时钟使能，高使能 */
#define	ET2CLK			(0x04000000 + CMU_PCLKCR4_ET2_PCE_Pos)	//4	/* Etimer2时钟使能，高使能 */
#define	ET1CLK			(0x04000000 + CMU_PCLKCR4_ET1_PCE_Pos)	//3	/* Etimer1时钟使能，高使能 */
#define	BT2CLK			(0x04000000 + CMU_PCLKCR4_BT2_PCE_Pos)	//2	/* BaseTimer2时钟使能，高使能 */
#define	BT1CLK			(0x04000000 + CMU_PCLKCR4_BT1_PCE_Pos)	//1	/* BaseTimer1时钟使能，高使能 */
#define	BSTIMCLK	  (0x04000000 + CMU_PCLKCR4_BSTIM_PCE_Pos)	//0	/* 基本定时器BSTIM总线时钟使能，高使能 */

/* 外设时钟使能函数 */
extern void CMU_PERCLK_SetableEx(uint32_t periph_def, FunState NewState);
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_CMU_H */
