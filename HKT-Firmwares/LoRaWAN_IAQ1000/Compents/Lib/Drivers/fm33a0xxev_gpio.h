/**
  ******************************************************************************
  * @file    fm33a0xxev_gpio.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the GPIO firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_GPIO_H
#define __FM33A0XXEV_GPIO_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 

//------------------------------------------------------------------------------
typedef enum
{ 
    GPIO_FCR_IN		= 0x00, /*!< GPIO 输入 */
    GPIO_FCR_OUT	= 0x01, /*!< GPIO 输出 */
    GPIO_FCR_DIG	= 0x02, /*!< GPIO 数字特殊功能 */
    GPIO_FCR_ANA	= 0x03  /*!< GPIO 模拟功能 */
}GPIO_FCR_TypeDef;

/** 
  * @brief  GPIO Output type enumeration 
  */  
typedef enum
{ 
    GPIO_OD_Dis = 0x00,
    GPIO_OD_En = 0x01
}GPIO_ODEN_TypeDef;

/** 
  * @brief  GPIO Configuration PullUp enumeration 
  */ 
typedef enum
{ 
    GPIO_PU_Dis = 0x00,
    GPIO_PU_En  = 0x01 
}GPIO_PUEN_TypeDef;

/** 
  * @brief  GPIO Configuration InEn enumeration 
  */ 
typedef enum
{ 
    GPIO_IN_Dis		= 0x00,
    GPIO_IN_En		= 0x01 
}GPIO_INEN_TypeDef;

/** 
  * @brief  GPIO Configuration HighDrive enumeration 
  */ 
typedef enum
{ 
    GPIO_HD_Dis		= 0x00,
    GPIO_HD_En		= 0x01 
}GPIO_HD_TypeDef;

/** 
  * @brief  GPIO Configuration AnalogChannel enumeration 
  */ 
typedef enum
{ 
    GPIO_ANEN_Dis		= 0x00,
    GPIO_ANEN_En		= 0x01 
}GPIO_ANEN_TypeDef;

/** 
  * @brief   GPIO Init structure definition  
  */ 
typedef struct
{
    uint32_t					Pin;     	//PIN选择
    GPIO_FCR_TypeDef			PxFCR;		//IO功能选择
    GPIO_ODEN_TypeDef			PxODEN;		//开漏输出使能控制
    GPIO_PUEN_TypeDef			PxPUEN;		//上拉电阻使能控制
    GPIO_INEN_TypeDef			PxINEN;		//输入使能控制
}GPIO_InitTypeDef;

/** 
  * @brief  GPIO Bit SET and Bit RESET enumeration 
  */ 
typedef enum
{ 
    Bit_RESET = 0,
    Bit_SET
}BitAction;
#define IS_GPIO_BIT_ACTION(ACTION) (((ACTION) == Bit_RESET) || ((ACTION) == Bit_SET))

typedef enum
{ 
    EXTI_RISING,
    EXTI_FALLING,
    EXTI_BOTH,
    EXTIDISABLE,
}GPIOExtiEdge;
#define IS_GPIO_INTERRUPT_TRIGER_EDGE(EDGE) (((EDGE) == GPIO_RISING) || ((EDGE) == GPIO_FALLING)|| ((EDGE) == GPIO_BOTH)|| ((EDGE) == GPIO_DISABLE))


/* Exported constants --------------------------------------------------------*/

/** @defgroup GPIO_Exported_Constants
  * @{
  */
	 
/**
* @}
*/ 

/** @defgroup GPIO_Exported_Types
  * @{
  */

#define IS_GPIO_ALL_PERIPH(PERIPH) (((PERIPH) == GPIOA) || \
                                    ((PERIPH) == GPIOB) || \
                                    ((PERIPH) == GPIOC) || \
                                    ((PERIPH) == GPIOD)	|| \
																		((PERIPH) == GPIOE)	|| \
																		((PERIPH) == GPIOF)	|| \
																		((PERIPH) == GPIOG))
    
#define IS_GPIO_HD_PERIPH(PERIPH) ( ((PERIPH) == GPIOD)	|| \
																		((PERIPH) == GPIOE)	|| \
																		((PERIPH) == GPIOF)	|| \
																		((PERIPH) == GPIOG))		
/** @defgroup GPIO_pins_define 
  * @{
  */ 
#define GPIO_Pin_0                 ((uint32_t)0x00000001)  /* Pin 0 selected */
#define GPIO_Pin_1                 ((uint32_t)0x00000002)  /* Pin 1 selected */
#define GPIO_Pin_2                 ((uint32_t)0x00000004)  /* Pin 2 selected */
#define GPIO_Pin_3                 ((uint32_t)0x00000008)  /* Pin 3 selected */
#define GPIO_Pin_4                 ((uint32_t)0x00000010)  /* Pin 4 selected */
#define GPIO_Pin_5                 ((uint32_t)0x00000020)  /* Pin 5 selected */
#define GPIO_Pin_6                 ((uint32_t)0x00000040)  /* Pin 6 selected */
#define GPIO_Pin_7                 ((uint32_t)0x00000080)  /* Pin 7 selected */
#define GPIO_Pin_8                 ((uint32_t)0x00000100)  /* Pin 8 selected */
#define GPIO_Pin_9                 ((uint32_t)0x00000200)  /* Pin 9 selected */
#define GPIO_Pin_10                ((uint32_t)0x00000400)  /* Pin 10 selected */
#define GPIO_Pin_11                ((uint32_t)0x00000800)  /* Pin 11 selected */
#define GPIO_Pin_12                ((uint32_t)0x00001000)  /* Pin 12 selected */
#define GPIO_Pin_13                ((uint32_t)0x00002000)  /* Pin 13 selected */
#define GPIO_Pin_14                ((uint32_t)0x00004000)  /* Pin 14 selected */
#define GPIO_Pin_15                ((uint32_t)0x00008000)  /* Pin 15 selected */
#define GPIO_Pin_All               ((uint16_t)0xFFFF)  /*!< All pins selected */

#define IS_GPIO_PIN(PIN) ((((PIN) & (uint16_t)0x00) == 0x00) && ((PIN) != (uint16_t)0x00))
#define IS_GET_GPIO_PIN(PIN) (((PIN) == GPIO_Pin_0) || \
                              ((PIN) == GPIO_Pin_1) || \
                              ((PIN) == GPIO_Pin_2) || \
                              ((PIN) == GPIO_Pin_3) || \
                              ((PIN) == GPIO_Pin_4) || \
                              ((PIN) == GPIO_Pin_5) || \
                              ((PIN) == GPIO_Pin_6) || \
                              ((PIN) == GPIO_Pin_7) || \
                              ((PIN) == GPIO_Pin_8) || \
                              ((PIN) == GPIO_Pin_9) || \
                              ((PIN) == GPIO_Pin_10) || \
                              ((PIN) == GPIO_Pin_11) || \
                              ((PIN) == GPIO_Pin_12) || \
                              ((PIN) == GPIO_Pin_13) || \
                              ((PIN) == GPIO_Pin_14) || \
                              ((PIN) == GPIO_Pin_15))



#define IS_GET_GPIOH_PIN(PIN) (((PIN) == GPIO_Pin_0) || \
                              ((PIN) == GPIO_Pin_1) || \
                              ((PIN) == GPIO_Pin_2) || \
                              ((PIN) == GPIO_Pin_3))
/* Exported macro ------------------------------------------------------------*/


/* WKUP引脚使能PIN宏定义 */
#define		PINWKEN_PD6		BIT7
#define		PINWKEN_PE9	  BIT6
#define		PINWKEN_PE2	  BIT5
#define		PINWKEN_PA13 	BIT4
#define		PINWKEN_PG7	  BIT3
#define		PINWKEN_PC13  BIT2
#define		PINWKEN_PB0	  BIT1
#define		PINWKEN_PF5	  BIT0

// 1. PortX输入使能寄存器
#define	GPIOx_INEN_INEN_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_INEN_INEN_Msk	(0xffffU << GPIOx_INEN_INEN_Pos)
								//GPIO输入使能控制
								//0：关闭输入使能
								//1：打开输入使能

// 2. PortX上拉使能寄存器
#define	GPIOx_PUEN_PUEN_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_PUEN_PUEN_Msk	(0xffffU << GPIOx_PUEN_PUEN_Pos)								
								//GPIO上拉控制
								//0：关闭上拉
								//1：使能上拉	

// 3. PortX开漏使能寄存器								
#define	GPIOx_ODEN_ODEN_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_ODEN_ODEN_Msk	(0xffffU << GPIOx_ODEN_ODEN_Pos)
								//GPIO开漏输出使能
								//0：关闭开漏输出
								//1：使能开漏输出

// 4. PortX功能选择寄存器								
#define	GPIOx_FCR_Px15FCR_Pos	30	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px15FCR_Msk	(0x03U << GPIOx_FCR_Px15FCR_Pos)
								//Px[15]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function								
#define	GPIOx_FCR_Px14FCR_Pos	28	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px14FCR_Msk	(0x03U << GPIOx_FCR_Px14FCR_Pos)
								//Px[14]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px13FCR_Pos	26	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px13FCR_Msk	(0x03U << GPIOx_FCR_Px13FCR_Pos)
								//Px[13]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px12FCR_Pos	24	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px12FCR_Msk	(0x03U << GPIOx_FCR_Px12FCR_Pos)
								//Px[12]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px11FCR_Pos	22	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px11FCR_Msk	(0x03U << GPIOx_FCR_Px11FCR_Pos)
								//Px[11]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px10FCR_Pos	20	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px10FCR_Msk	(0x03U << GPIOx_FCR_Px10FCR_Pos)
								//Px[10]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//10：Analog function
#define	GPIOx_FCR_Px9FCR_Pos	18	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px9FCR_Msk	(0x03U << GPIOx_FCR_Px9FCR_Pos)
								//Px[9]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function	
#define	GPIOx_FCR_Px8FCR_Pos	16	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px8FCR_Msk	(0x03U << GPIOx_FCR_Px8FCR_Pos)
								//Px[8]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px7FCR_Pos	14	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px7FCR_Msk	(0x03U << GPIOx_FCR_Px7FCR_Pos)
								//Px[7]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px6FCR_Pos	12	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px6FCR_Msk	(0x03U << GPIOx_FCR_Px6FCR_Pos)
								//Px[6]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px5FCR_Pos	10	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px5FCR_Msk	(0x03U << GPIOx_FCR_Px5FCR_Pos)
								//Px[5]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px4FCR_Pos	8	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px4FCR_Msk	(0x03U << GPIOx_FCR_Px4FCR_Pos)
								//Px[4]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px3FCR_Pos	6	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px3FCR_Msk	(0x03U << GPIOx_FCR_Px3FCR_Pos)
								//Px[3]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px2FCR_Pos	4	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px2FCR_Msk	(0x03U << GPIOx_FCR_Px2FCR_Pos)
								//Px[2]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px1FCR_Pos	2	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px1FCR_Msk	(0x03U << GPIOx_FCR_Px1FCR_Pos)
								//Px[1]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function
#define	GPIOx_FCR_Px0FCR_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_FCR_Px0FCR_Msk	(0x03U << GPIOx_FCR_Px0FCR_Pos)
								//Px[0]引脚功能选择
								//00：GPIO输入
								//01：GPIO输出
								//10：Digital function
								//11：Analog function	

// 5. PortX输出数据寄存器																						
#define	GPIOx_DO_DO_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_DO_DO_Msk	(0xffffU << GPIOx_DO_DO_Pos)
								//GPIO output data register

// 6. PortX输出数据置位寄存器								
#define	GPIOx_DSET_DSET_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_DSET_DSET_Msk	(0xffffU << GPIOx_DSET_DSET_Pos)
								//GPIO output data set register
								//举例：向PADSET写0x0000_8000，则PADOR[15]置位，其余位保持不变。
								//PASET/PBSET为16位；PCSET/PDSET为13位

// 7. PortX输出数据复位寄存器								
#define	GPIOx_DRST_DRESET_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_DRST_DRESET_Msk	(0xffffU << GPIOx_DRST_DRESET_Pos)
								//GPIO output data reset register
								//举例：向PADRST写0x0000_8000，则PADOR[15]清零，其余位保持不变
								//PASET/PBSET为16位；PCSET/PDSET为13位

// 8. PortX输入数据寄存器																																															
#define	GPIOx_DIN_DIN_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_DIN_DIN_Msk	(0xffffU << GPIOx_DIN_DIN_Pos)
								//Portx input data register
								//此寄存器仅占用地址空间，无物理实现。
								//软件读此寄存器直接返回引脚输入信号，芯片并不对引脚输入进行锁存

// 9. PortX额外数字功能寄存器								
#define	GPIOx_DFS_DFS_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_DFS_DFS_Msk	(0xffffU << GPIOx_DFS_DFS_Pos)
								//Portx Digital Function Select
								//对于具有多个数字外设功能的引脚，通过PxDFS寄存器可以选择使用哪个外设功能。
								//注意，对于不同的IO分组，有效的寄存器位置是不一样的

// 11. PortX模拟开关使能寄存器								
#define	GPIOx_ANEN_ANEN_Pos	0	//R/W: rw	ResetValue: 0x0
#define	GPIOx_ANEN_ANEN_Msk	(0xffffU << GPIOx_ANEN_ANEN_Pos)
								//PortX 模拟开关使能
								//1：使能IO模拟开关
								//0：关闭IO模拟开关
								//注：支持模拟开关的IO有
								//PA6/PA7/PA13/PA14/PA15
								//PB0/PB1/PB13/PB14/PB15
								//PC0/PC1/PC6/PC7/PC8/PC9/PC10
								//PD0/PD1/PD2/PD11/PD12
								//对应以上IO的PxANEN寄存器有效；其余寄存器无意义。


#define	GPIO_FOUTSEL_FOUT1SEL_Pos	8	/* FOUT输出频率选择信号 */
#define	GPIO_FOUTSEL_FOUT1SEL_Msk	(0x1fU << GPIO_FOUTSEL_FOUT1SEL_Pos)
#define	GPIO_FOUTSEL_FOUT1SEL_XTLF	(0x0U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00000: XTLF */
#define	GPIO_FOUTSEL_FOUT1SEL_RCLP	(0x1U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00001: RCLP */
#define	GPIO_FOUTSEL_FOUT1SEL_RCHFD64	(0x2U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00010: RCHF/64 */
#define	GPIO_FOUTSEL_FOUT1SEL_LSCLK	(0x3U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00011: LSCLK */
#define	GPIO_FOUTSEL_FOUT1SEL_AHBCLKD64	(0x4U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00100: AHBCLK/64 */
#define	GPIO_FOUTSEL_FOUT1SEL_RTC_TM	(0x5U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00101: RTC_TM */
#define	GPIO_FOUTSEL_FOUT1SEL_PLLHD64	(0x6U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00110: PLL_H/64 */
#define	GPIO_FOUTSEL_FOUT1SEL_RTCCLK64Hz	(0x7U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 00111: RTCCLK64Hz */
#define	GPIO_FOUTSEL_FOUT1SEL_APBCLKD64	(0x8U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01000: APBCLK/64 */
#define	GPIO_FOUTSEL_FOUT1SEL_PLL_L	(0x9U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01001: PLL_L */
#define	GPIO_FOUTSEL_FOUT1SEL_RCMF	(0xAU << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01010: RCMF */
#define	GPIO_FOUTSEL_FOUT1SEL_RCHF	(0xBU << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01011: RCHF */
#define	GPIO_FOUTSEL_FOUT1SEL_XTHFD64	(0xCU << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01100: XTHF/64 */
#define	GPIO_FOUTSEL_FOUT1SEL_ADCCLK	(0xDU << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01101: ADCCLK */
#define	GPIO_FOUTSEL_FOUT1SEL_PLL_H	(0xEU << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 01110: PLL_H */
#define	GPIO_FOUTSEL_FOUT1SEL_BOOST_FLAG	(0x10U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 10000: Boost Flag */
#define	GPIO_FOUTSEL_FOUT1SEL_AUTOTRIM_OUTPUT	(0x11U << GPIO_FOUTSEL_FOUT1SEL_Pos)	/* 10001: Autotrim output */


#define	GPIO_FOUTSEL_FOUT0SEL_Pos	0	/* FOUT输出频率选择信号 */
#define	GPIO_FOUTSEL_FOUT0SEL_Msk	(0x1fU << GPIO_FOUTSEL_FOUT0SEL_Pos)
#define	GPIO_FOUTSEL_FOUT0SEL_XTLF	(0x0U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00000: XTLF */
#define	GPIO_FOUTSEL_FOUT0SEL_RCLP	(0x1U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00001: RCLP */
#define	GPIO_FOUTSEL_FOUT0SEL_RCHFD64	(0x2U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00010: RCHF/64 */
#define	GPIO_FOUTSEL_FOUT0SEL_LSCLK	(0x3U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00011: LSCLK */
#define	GPIO_FOUTSEL_FOUT0SEL_AHBCLKD64	(0x4U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00100: AHBCLK/64 */
#define	GPIO_FOUTSEL_FOUT0SEL_RTC_TM	(0x5U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00101: RTC_TM */
#define	GPIO_FOUTSEL_FOUT0SEL_PLLHD64	(0x6U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00110: PLL_H/64 */
#define	GPIO_FOUTSEL_FOUT0SEL_RTCCLK64Hz	(0x7U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 00111: RTCCLK64Hz */
#define	GPIO_FOUTSEL_FOUT0SEL_APBCLKD64	(0x8U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01000: APBCLK/64 */
#define	GPIO_FOUTSEL_FOUT0SEL_PLL_L	(0x9U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01001: PLL_L */
#define	GPIO_FOUTSEL_FOUT0SEL_RCMF	(0xAU << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01010: RCMF */
#define	GPIO_FOUTSEL_FOUT0SEL_RCHF	(0xBU << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01011: RCHF */
#define	GPIO_FOUTSEL_FOUT0SEL_XTHFD64	(0xCU << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01100: XTHF/64 */
#define	GPIO_FOUTSEL_FOUT0SEL_ADCCLK	(0xDU << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01101: ADCCLK */
#define	GPIO_FOUTSEL_FOUT0SEL_PLL_H	(0xEU << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 01110: PLL_H */
#define	GPIO_FOUTSEL_FOUT0SEL_BOOST_FLAG	(0x10U << GPIO_FOUTSEL_FOUT0SEL_Pos)	/* 10000: Boost Flag */

#define	GPIO_IOMCR_IOMUXSEL_Pos 1
#define	GPIO_IOMCR_IOMUXSEL_Msk	(0x3U << GPIO_IOMCR_IOMUXSEL_Pos)
#define	GPIO_IOMCR_IOMUXSEL_PD6	(0x0U << GPIO_IOMCR_IOMUXSEL_Pos)
#define	GPIO_IOMCR_IOMUXSEL_PD7	(0x1U << GPIO_IOMCR_IOMUXSEL_Pos)
#define	GPIO_IOMCR_IOMUXSEL_PG2	(0x2U << GPIO_IOMCR_IOMUXSEL_Pos)
#define	GPIO_IOMCR_IOMUXSEL_PG3	(0x3U << GPIO_IOMCR_IOMUXSEL_Pos)

#define	GPIO_IOMCR_IOMUXEN_Pos	0
#define	GPIO_IOMCR_IOMCUEN_Msk	(0x1U << GPIO_IOMCR_IOMUXEN_Pos)

#define	GPIO_PINWKEN_WKISEL_Pos	31	/* WKUP中断入口选择寄存器 */
#define	GPIO_PINWKEN_WKISEL_Msk	(0x1U << GPIO_PINWKEN_WKISEL_Pos)
#define	GPIO_PINWKEN_WKISEL_NMI	(0x0U << GPIO_PINWKEN_WKISEL_Pos)   /* NMI中断 */
#define	GPIO_PINWKEN_WKISEL_46	(0x1U << GPIO_PINWKEN_WKISEL_Pos)   /* #46中断 */

#define	GPIO_PINWKEN_PINWKSEL_Pos	8	/* WKUP边沿选择寄存器*/
#define	GPIO_PINWKEN_PINWKSEL_Msk	(0xffU << GPIO_PINWKEN_PINWKSEL_Pos)

#define	GPIO_PINWKEN_PINWKEN_Pos	0	/* WKUP引脚使能寄存器 */
#define	GPIO_PINWKEN_PINWKEN_Msk	(0xffU << GPIO_PINWKEN_PINWKEN_Pos)


//PortH输出数据寄存器																						
#define	GPIOH_DO_PHDO_Pos	0	
#define	GPIOH_DO_PHDO_Msk	(0xfU << GPIOH_DO_PHDO_Pos)

//PortH输入数据寄存器																						
#define	GPIOH_DIN_PHDIN_Pos	0	
#define	GPIOH_DIN_PHDIN_Msk	(0xfU << GPIOH_DIN_PHDIN_Pos)
								//GPIO output data register
/* Exported functions --------------------------------------------------------*/ 

/* PortX输出数据寄存器 相关函数 */
extern void GPIOx_DO_Write(GPIO_Type* GPIOx, uint32_t SetValue);
extern uint32_t GPIOx_DO_Read(GPIO_Type* GPIOx);

/* PortX输出数据置位寄存器 相关函数 */
extern void GPIOx_DSET_Write(GPIO_Type* GPIOx, uint32_t SetValue);

/* PortX输出数据复位寄存器 相关函数 */
extern void GPIOx_DRST_Write(GPIO_Type* GPIOx, uint32_t SetValue);

/* PortX输入数据寄存器 相关函数 */
extern uint32_t GPIOx_DIN_Read(GPIO_Type* GPIOx);

/* PortX额外数字功能寄存器 相关函数 */
extern void GPIOx_DFS_Setable(GPIO_Type* GPIOx, uint32_t GPIO_Pin, FunState NewState);
extern FunState GPIOx_DFS_Getable(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* PortX模拟开关使能寄存器 相关函数 */
extern void GPIOx_ANEN_Setable(GPIO_Type* GPIOx, uint32_t GPIO_Pin, FunState NewState);
extern FunState GPIOx_ANEN_Getable(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* GPIO 初始化配置 */
extern void GPIO_Init(GPIO_Type* GPIOx, GPIO_InitTypeDef* para);

/* 获取一个IO口的配置参数结构体 
	注意一次只能读取一个IO的配置*/
extern void GPIO_Get_InitPara(GPIO_Type* GPIOx, uint32_t GPIO_Pin, GPIO_InitTypeDef* para);

/* 读取GPIOx输入数据寄存器 */
extern uint32_t GPIO_ReadInputData(GPIO_Type* GPIOx);

/* 读取GPIOx输入数据寄存器bit */
extern uint8_t GPIO_ReadInputDataBit(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* GPIOx输出数据置1 */
extern void GPIO_SetBits(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* GPIOx输出数据置0 */
extern void GPIO_ResetBits(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* GPIOx输出数据置翻转 */
extern void GPIO_ToggleBits(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* 读取GPIOx输出数据寄存器 */
extern uint32_t GPIO_ReadOutputData(GPIO_Type* GPIOx);

/* 读取GPIOx输出数据寄存器bit */
extern uint8_t GPIO_ReadOutputDataBit(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* 外部引脚选择 */
extern void GPIO_EXTI_Select_Pin(GPIO_Type* GPIOx,uint32_t GPIO_Pin);

/* 外部中断边沿选择 */
extern void GPIO_EXTI_Select_Edge(GPIO_Type* GPIOx,uint32_t GPIO_Pin,GPIOExtiEdge edge_select);


/* EXTI数字滤波控制寄存器 相关函数 */
extern void GPIO_EXTI_EXTIDF_Setable(GPIO_Type* GPIOx, uint32_t GPIO_Pin, FunState NewState);

/* 读取EXTI输入信号寄存器 */
extern FunState GPIO_EXTI_EXTIDI_Getable(GPIO_Type* GPIOx, uint32_t GPIO_Pin);

/* 清除中断标志 */
extern void GPIO_EXTI_EXTIISR_ClrEx(GPIO_Type* GPIOx,uint32_t GPIO_Pin);

/* 读取中断标志状态 */
extern FlagStatus GPIO_EXTI_EXTIISR_ChkEx(GPIO_Type* GPIOx,uint32_t GPIO_Pin);

/* FOUT1输出配置函数 */
extern void GPIO_FOUTSEL_FOUT1SEL_Set(uint32_t SetValue);

/* 读FOUT1输出状态函数 */
extern uint32_t GPIO_FOUTSEL_FOUT1SEL_Get(void);

/* FOUT0输出配置函数 */
extern void GPIO_FOUTSEL_FOUT0SEL_Set(uint32_t SetValue);

/* 读FOUT0输出状态函数 */
extern uint32_t GPIO_FOUTSEL_FOUT0SEL_Get(void);

/* 功能说明：GPIO_EXTI_Init 外部中断初始化 */
extern void GPIO_EXTI_Init(GPIO_Type* GPIOx, uint32_t GPIO_Pin, GPIOExtiEdge edge_select, FunState DFState);

/* GPIO_EXTI_Close外部中断关闭 */
extern void GPIO_EXTI_Close(GPIO_Type* GPIOx,uint32_t GPIO_Pin);

/* GPIOH初始化配置函数 */
extern void GPIOH_Init(GPIO_InitTypeDef* para);

/* 获取一个IO配置参数结构体函数 */
extern void GPIOH_Get_InitPara(uint32_t GPIO_Pin, GPIO_InitTypeDef* para);

/* GPIOH输出数据寄存器配置函数 */
extern void GPIOH_DO_Write(uint32_t SetValue);

/* 读取GPIOH输出寄存器 */
extern uint32_t GPIOH_ReadOutputData(void);

/* 按管脚Pin读取输出寄存器 */
extern uint8_t GPIOH_ReadOutputDataBit(uint32_t GPIO_Pin);

/* 读取GPIOH输入寄存器 */
extern uint32_t GPIOH_ReadInputData(void);

/* 按管脚Pin读取输入寄存器 */
extern uint8_t GPIOH_ReadInputDataBit(uint32_t GPIO_Pin);

/*配置IO四选一输出*/
extern void GPIO_IOMCR_IOMUXSEL_Set(uint32_t SetValue);
	
/*读IOMCR四选一配置状态*/
extern uint32_t GPIO_IOMCR_IOMUXSEL_Get(void);

/* GPIO 四选一使能 函数 */
extern void GPIO_IOMCR_Setable(FunState NewState);

/* GPIO 获取GPIO 四选一使能 函数 */
extern FunState GPIO_IOMCR_Getable(void);

extern void GPIO_PINWKEN_SetableEx(uint32_t NWKPinDef, FunState NewState);
extern void GPIO_PINWKSEL_SetableEx(uint32_t NWKPinDef, FunState NewState);
extern void GPIO_PINWKEN_WKISEL_Set(uint32_t SetValue);
extern uint32_t GPIO_PINWKEN_WKISEL_Get(void);
            
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_GPIO_H */
