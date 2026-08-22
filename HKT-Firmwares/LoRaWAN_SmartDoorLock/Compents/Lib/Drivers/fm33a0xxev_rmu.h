/**
  ******************************************************************************
  * @file    fm33a0xxev_rmu.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the RMU firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_RMU_H
#define __FM33A0XXEV_RMU_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	RMU_PDRCR_PDRCFG_Pos	1	/* 下电复位电压配置00—1.5V01—1.25V（禁止使用）10—1.35V11—1.4V（默认） */
#define	RMU_PDRCR_PDRCFG_Msk	(0x3U << RMU_PDRCR_PDRCFG_Pos)
#define	RMU_PDRCR_PDRCFG_1P5V	(0x0U << RMU_PDRCR_PDRCFG_Pos)
#define	RMU_PDRCR_PDRCFG_1P25V	(0x1U << RMU_PDRCR_PDRCFG_Pos)
#define	RMU_PDRCR_PDRCFG_1P35V	(0x2U << RMU_PDRCR_PDRCFG_Pos)
#define	RMU_PDRCR_PDRCFG_1P4V	(0x3U << RMU_PDRCR_PDRCFG_Pos)

#define	RMU_PDRCR_PDREN_Pos	0	/* 下电复位使能0：关闭下电复位1：使能下电复位 */
#define	RMU_PDRCR_PDREN_Msk	(0x1U << RMU_PDRCR_PDREN_Pos)
	/* 关闭下电复位 */
	/* 使能下电复位 */

#define	RMU_BORCR_BOR_PDRCFG_Pos	1	/* 下电复位电压配置00—1.7V01—1.6V（默认）10—1.65V11—1.75V */
#define	RMU_BORCR_BOR_PDRCFG_Msk	(0x3U << RMU_BORCR_BOR_PDRCFG_Pos)
#define	RMU_BORCR_BOR_PDRCFG_1P7V	(0x0U << RMU_BORCR_BOR_PDRCFG_Pos)
#define	RMU_BORCR_BOR_PDRCFG_1P6V	(0x1U << RMU_BORCR_BOR_PDRCFG_Pos)
#define	RMU_BORCR_BOR_PDRCFG_1P65V	(0x2U << RMU_BORCR_BOR_PDRCFG_Pos)
#define	RMU_BORCR_BOR_PDRCFG_1P75V	(0x3U << RMU_BORCR_BOR_PDRCFG_Pos)

#define	RMU_BORCR_OFF_BOR_Pos	0	/* BOR使能控制寄存器0：使能BOR1：关闭BOR */
#define	RMU_BORCR_OFF_BOR_Msk	(0x1U << RMU_BORCR_OFF_BOR_Pos)
	/* 使能BOR */
	/* 关闭BOR */

#define	RMU_RSTCFGR_LKUPRST_EN_Pos	1	/* LOCKUP复位使能1：使能SC000 LOCKUP复位0：屏蔽SC000 LOCKUP复位 */
#define	RMU_RSTCFGR_LKUPRST_EN_Msk	(0x1U << RMU_RSTCFGR_LKUPRST_EN_Pos)
	/* 使能SC000 LOCKUP复位 */
	/* 屏蔽SC000 LOCKUP复位 */

#define	RMU_SOFTRST_SOFTRST_Pos	0	/* 软件写0x5C5C_AABB触发全局复位 */
#define	RMU_SOFTRST_SOFTRST_Msk	(0xffffffffU << RMU_SOFTRST_SOFTRST_Pos)

#define	RMU_RSR_NRSTN_FLAG_Pos	11	/* NRST引脚复位标志，高有效 */
#define	RMU_RSR_NRSTN_FLAG_Msk	(0x1U << RMU_RSR_NRSTN_FLAG_Pos)

#define	RMU_RSR_TESTN_FLAG_Pos	10	/* TESTN引脚复位标志，高有效 */
#define	RMU_RSR_TESTN_FLAG_Msk	(0x1U << RMU_RSR_TESTN_FLAG_Pos)

#define	RMU_RSR_PORN_FLAG_Pos	9	/* 上电复位标志，高有效 */
#define	RMU_RSR_PORN_FLAG_Msk	(0x1U << RMU_RSR_PORN_FLAG_Pos)

#define	RMU_RSR_PDRN_FLAG_Pos	8	/* 下电复位标志，高有效 */
#define	RMU_RSR_PDRN_FLAG_Msk	(0x1U << RMU_RSR_PDRN_FLAG_Pos)

#define	RMU_RSR_SOFTN_FLAG_Pos	5	/* 软件复位标志，高有效 */
#define	RMU_RSR_SOFTN_FLAG_Msk	(0x1U << RMU_RSR_SOFTN_FLAG_Pos)

#define	RMU_RSR_IWDTN_FLAG_Pos	4	/* IWDT复位标志，高有效 */
#define	RMU_RSR_IWDTN_FLAG_Msk	(0x1U << RMU_RSR_IWDTN_FLAG_Pos)

#define	RMU_RSR_WWDTN_FLAG_Pos	2	/* WWDT复位标志，高有效 */
#define	RMU_RSR_WWDTN_FLAG_Msk	(0x1U << RMU_RSR_WWDTN_FLAG_Pos)

#define	RMU_RSR_LKUPN_FLAG_Pos	1	/* LOOKUP复位标志，高有效 */
#define	RMU_RSR_LKUPN_FLAG_Msk	(0x1U << RMU_RSR_LKUPN_FLAG_Pos)

#define	RMU_RSR_NVICN_FLAG_Pos	0	/* NVIC复位标志，高有效 */
#define	RMU_RSR_NVICN_FLAG_Msk	(0x1U << RMU_RSR_NVICN_FLAG_Pos)

#define	RMU_PRSTEN_PERHRSTEN_Pos	0	/* 外设模块复位使能，32bit虚寄存器，只写软件对此地址写0x1357_9BDF，使能外设复位功能，此后可以通过外设模块复位寄存器复位各个模块软件对此地址写任意其他数据，将关闭外设复位功能 */
#define	RMU_PRSTEN_PERHRSTEN_Msk	(0xffffffffU << RMU_PRSTEN_PERHRSTEN_Pos)

#define	RMU_AHBRST_HASHRST_Pos	2	/* HASH模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_AHBRST_HASHRST_Msk	(0x1U << RMU_AHBRST_HASHRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_AHBRST_PAERST_Pos	1	/* PAE模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_AHBRST_PAERST_Msk	(0x1U << RMU_AHBRST_PAERST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_AHBRST_DMARST_Pos	0	/* DMA模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_AHBRST_DMARST_Msk	(0x1U << RMU_AHBRST_DMARST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_UART5RST_Pos	31	/* UART5模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_UART5RST_Msk	(0x1U << RMU_APBRST1_UART5RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_UART4RST_Pos	30	/* UART4模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_UART4RST_Msk	(0x1U << RMU_APBRST1_UART4RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_UART3RST_Pos	29	/* UART3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_UART3RST_Msk	(0x1U << RMU_APBRST1_UART3RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_UART2RST_Pos	28	/* UART2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_UART2RST_Msk	(0x1U << RMU_APBRST1_UART2RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_TIMARST_Pos	23	/* Timer array复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_TIMARST_Msk	(0x1U << RMU_APBRST1_TIMARST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_LCDRST_Pos	16	/* LCD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_LCDRST_Msk	(0x1U << RMU_APBRST1_LCDRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_U7816RST_Pos	14	/* U7816模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_U7816RST_Msk	(0x1U << RMU_APBRST1_U7816RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_SPI4RST_Pos	12	/* SPI3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_SPI4RST_Msk	(0x1U << RMU_APBRST1_SPI4RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_SPI3RST_Pos	11	/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_SPI3RST_Msk	(0x1U << RMU_APBRST1_SPI3RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_SPI2RST_Pos	10	/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_SPI2RST_Msk	(0x1U << RMU_APBRST1_SPI2RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_LPUART0RST_Pos	6	/* EUART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_LPUART0RST_Msk	(0x1U << RMU_APBRST1_LPUART0RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_I2C1RST_Pos	3	/* I2C1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_I2C1RST_Msk	(0x1U << RMU_APBRST1_I2C1RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST1_LPTRST_Pos	0	/*  LPTIM 模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST1_LPTRST_Msk	(0x1U << RMU_APBRST1_LPTRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_UART1RST_Pos	31	/* UART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_UART1RST_Msk	(0x1U << RMU_APBRST2_UART1RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_UART0RST_Pos	30	/* UART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_UART0RST_Msk	(0x1U << RMU_APBRST2_UART0RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_UARTIRRST_Pos	29	/* UART红外模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_UARTIRRST_Msk	(0x1U << RMU_APBRST2_UARTIRRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_BSTRST_Pos	28	/* BSTIM模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_BSTRST_Msk	(0x1U << RMU_APBRST2_BSTRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_CICRST_Pos	24	/* CIC滤波模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_CICRST_Msk	(0x1U << RMU_APBRST2_CICRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_ADCRST_Pos	23	/* ADC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_ADCRST_Msk	(0x1U << RMU_APBRST2_ADCRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_AESRST_Pos	18	/* AES模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_AESRST_Msk	(0x1U << RMU_APBRST2_AESRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_CRCRST_Pos	17	/* CRC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_CRCRST_Msk	(0x1U << RMU_APBRST2_CRCRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_RNGRST_Pos	16	/* RNG模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_RNGRST_Msk	(0x1U << RMU_APBRST2_RNGRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_SPI1RST_Pos	9	/* SPI1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_SPI1RST_Msk	(0x1U << RMU_APBRST2_SPI1RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_SPI0RST_Pos	8	/* SPI0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_SPI0RST_Msk	(0x1U << RMU_APBRST2_SPI0RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_LPUART1RST_Pos	7	/* EUART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_LPUART1RST_Msk	(0x1U << RMU_APBRST2_LPUART1RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_I2C0RST_Pos	4	/* I2C0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_I2C0RST_Msk	(0x1U << RMU_APBRST2_I2C0RST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_SVDRST_Pos	1	/* SVD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_SVDRST_Msk	(0x1U << RMU_APBRST2_SVDRST_Pos)
	/* 不复位 */
	/* 复位 */

#define	RMU_APBRST2_COMPRST_Pos	0	/* 比较器模块复位，软件写1复位，写0撤销复位0：不复位1：复位 */
#define	RMU_APBRST2_COMPRST_Msk	(0x1U << RMU_APBRST2_COMPRST_Pos)
	/* 不复位 */
	/* 复位 */
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void RMU_Deinit(void);

/* 下电复位电压配置00—1.5V01—1.25V（禁止使用）10—1.35V11—1.4V（默认） 相关函数 */
extern void RMU_PDRCR_PDRCFG_Set(uint32_t SetValue);
extern uint32_t RMU_PDRCR_PDRCFG_Get(void);

/* 下电复位使能0：关闭下电复位1：使能下电复位 相关函数 */
extern void RMU_PDRCR_PDREN_Setable(FunState NewState);
extern FunState RMU_PDRCR_PDREN_Getable(void);

/* 下电复位电压配置00—1.7V01—1.6V（默认）10—1.65V11—1.75V 相关函数 */
extern void RMU_BORCR_BOR_PDRCFG_Set(uint32_t SetValue);
extern uint32_t RMU_BORCR_BOR_PDRCFG_Get(void);

/* BOR使能控制寄存器0：使能BOR1：关闭BOR 相关函数 */
extern void RMU_BORCR_OFF_BOR_Setable(FunState NewState);
extern FunState RMU_BORCR_OFF_BOR_Getable(void);

/* LOCKUP复位使能1：使能SC000 LOCKUP复位0：屏蔽SC000 LOCKUP复位 相关函数 */
extern void RMU_RSTCFGR_LKUPRST_EN_Setable(FunState NewState);
extern FunState RMU_RSTCFGR_LKUPRST_EN_Getable(void);

/* 软件写0x5C5C_AABB触发全局复位 相关函数 */
extern void RMU_SOFTRST_Write(uint32_t SetValue);

/* NRST引脚复位标志，高有效 相关函数 */
extern void RMU_RSR_NRSTN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_NRSTN_FLAG_Chk(void);

/* TESTN引脚复位标志，高有效 相关函数 */
extern void RMU_RSR_TESTN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_TESTN_FLAG_Chk(void);

/* 上电复位标志，高有效 相关函数 */
extern void RMU_RSR_PORN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_PORN_FLAG_Chk(void);

/* 下电复位标志，高有效 相关函数 */
extern void RMU_RSR_PDRN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_PDRN_FLAG_Chk(void);

/* 软件复位标志，高有效 相关函数 */
extern void RMU_RSR_SOFTN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_SOFTN_FLAG_Chk(void);

/* IWDT复位标志，高有效 相关函数 */
extern void RMU_RSR_IWDTN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_IWDTN_FLAG_Chk(void);

/* WWDT复位标志，高有效 相关函数 */
extern void RMU_RSR_WWDTN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_WWDTN_FLAG_Chk(void);

/* LOOKUP复位标志，高有效 相关函数 */
extern void RMU_RSR_LKUPN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_LKUPN_FLAG_Chk(void);

/* NVIC复位标志，高有效 相关函数 */
extern void RMU_RSR_NVICN_FLAG_Clr(void);
extern FlagStatus RMU_RSR_NVICN_FLAG_Chk(void);

/* 外设模块复位使能，32bit虚寄存器，只写软件对此地址写0x1357_9BDF，使能外设复位功能，此后可以通过外设模块复位寄存器复位各个模块软件对此地址写任意其他数据，将关闭外设复位功能 相关函数 */
extern void RMU_PRSTEN_Write(uint32_t SetValue);

/* HASH模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_AHBRST_HASHRST_Setable(FunState NewState);
extern FunState RMU_AHBRST_HASHRST_Getable(void);

/* PAE模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_AHBRST_PAERST_Setable(FunState NewState);
extern FunState RMU_AHBRST_PAERST_Getable(void);

/* DMA模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_AHBRST_DMARST_Setable(FunState NewState);
extern FunState RMU_AHBRST_DMARST_Getable(void);

/* UART5模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_UART5RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_UART5RST_Getable(void);

/* UART4模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_UART4RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_UART4RST_Getable(void);

/* UART3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_UART3RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_UART3RST_Getable(void);

/* UART2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_UART2RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_UART2RST_Getable(void);

/* Timer array复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_TIMARST_Setable(FunState NewState);
extern FunState RMU_APBRST1_TIMARST_Getable(void);

/* LCD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_LCDRST_Setable(FunState NewState);
extern FunState RMU_APBRST1_LCDRST_Getable(void);

/* U7816模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_U7816RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_U7816RST_Getable(void);

/* SPI3模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_SPI4RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_SPI4RST_Getable(void);

/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_SPI3RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_SPI3RST_Getable(void);

/* SPI2模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_SPI2RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_SPI2RST_Getable(void);

/* EUART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_LPUART0RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_LPUART0RST_Getable(void);

/* I2C1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_I2C1RST_Setable(FunState NewState);
extern FunState RMU_APBRST1_I2C1RST_Getable(void);

/* LPTIM 模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST1_LPTRST_Setable(FunState NewState);
extern FunState RMU_APBRST1_LPTRST_Getable(void);

/* UART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_UART1RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_UART1RST_Getable(void);

/* UART0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_UART0RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_UART0RST_Getable(void);

/* UART红外模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_UARTIRRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_UARTIRRST_Getable(void);

/* BSTIM模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_BSTRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_BSTRST_Getable(void);

/* CIC滤波模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_CICRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_CICRST_Getable(void);

/* ADC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_ADCRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_ADCRST_Getable(void);

/* AES模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_AESRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_AESRST_Getable(void);

/* CRC模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_CRCRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_CRCRST_Getable(void);

/* RNG模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_RNGRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_RNGRST_Getable(void);

/* SPI1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_SPI1RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_SPI1RST_Getable(void);

/* SPI0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_SPI0RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_SPI0RST_Getable(void);

/* EUART1模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_LPUART1RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_LPUART1RST_Getable(void);

/* I2C0模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_I2C0RST_Setable(FunState NewState);
extern FunState RMU_APBRST2_I2C0RST_Getable(void);

/* SVD模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_SVDRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_SVDRST_Getable(void);

/* 比较器模块复位，软件写1复位，写0撤销复位0：不复位1：复位 相关函数 */
extern void RMU_APBRST2_COMPRST_Setable(FunState NewState);
extern FunState RMU_APBRST2_COMPRST_Getable(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_RMU_H */
