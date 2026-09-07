/**
  ******************************************************************************
  * @file    fm33a0xxev_flash.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the FLASH firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_FLASH_H
#define __FM33A0XXEV_FLASH_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	FLS_RDCR_WAIT_Pos	0	/* Flash读等待周期配置 (Wait Cycles Config)
00/11：0 wait cycle
01：1 wait cycle
10：2 wait cycles
CPU主频小于等于24MHz时，不需要开启wait；主频大于24M小于48Mhz时使能1 wait，主频大于48Mhz时使能2 wait */
#define	FLS_RDCR_WAIT_Msk	(0x3U << FLS_RDCR_WAIT_Pos)
#define	FLS_RDCR_WAIT_0CYCLE	(0x0U << FLS_RDCR_WAIT_Pos)	/* 0 wait cycle */
#define	FLS_RDCR_WAIT_1CYCLE	(0x1U << FLS_RDCR_WAIT_Pos)	/* 1 wait cycle */
#define	FLS_RDCR_WAIT_2CYCLE	(0x2U << FLS_RDCR_WAIT_Pos)	/* 2 wait cycles */

#define	FLS_PFCR_PRFTEN_Pos	0	/* 指令预取指使能，在WAIT==00的情况下写1无效 (Prefetch Enable)
1：使能Prefetch
0：禁止Prefetch */
#define	FLS_PFCR_PRFTEN_Msk	(0x1U << FLS_PFCR_PRFTEN_Pos)
	/* 使能Prefetch */
	/* 禁止Prefetch */

#define	FLS_OPTBR_DBGCFGEN_Pos	31	
#define	FLS_OPTBR_DBGCFGEN_Msk	(0x1U << FLS_OPTBR_DBGCFGEN_Pos)

#define	FLS_OPTBR_IF2LOCK_Pos	18	/* Information2区锁定标志 (IF2 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 */
#define	FLS_OPTBR_IF2LOCK_Msk	(0x1U << FLS_OPTBR_IF2LOCK_Pos)

#define	FLS_OPTBR_IF1LOCK_Pos	17	/* Information1区锁定标志 (IF1 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 */
#define	FLS_OPTBR_IF1LOCK_Msk	(0x1U << FLS_OPTBR_IF1LOCK_Pos)

#define	FLS_OPTBR_DFLSEN_Pos	10	/* DataFlash使能 (DataFlash Enable)
0：无data flash
1：有data flash */
#define	FLS_OPTBR_DFLSEN_Msk	(0x1U << FLS_OPTBR_DFLSEN_Pos)
	/* 无data flash */
	/* 有data flash */

#define	FLS_OPTBR_BTSEN_Pos	8	/* BootSwap功能使能 (BootSwap Enable)
00/01/11：禁止BootSwap功能
10：允许BootSwap */
#define	FLS_OPTBR_BTSEN_Msk	(0x3U << FLS_OPTBR_BTSEN_Pos)
	/* 禁止BootSwap功能 */
	/* 禁止BootSwap功能 */
	/* 禁止BootSwap功能 */
	/* 允许BootSwap */

#define	FLS_OPTBR_ACLOCKEN_Pos	4	/* 应用代码权限锁定使能 (AppCode Lock Enable)
00/01/11：ACLOCK不使能
10：ACLOCK使能 */
#define	FLS_OPTBR_ACLOCKEN_Msk	(0x3U << FLS_OPTBR_ACLOCKEN_Pos)
	/* ACLOCK不使能 */
	/* ACLOCK不使能 */
	/* ACLOCK不使能 */
	/* ACLOCK使能 */

#define	FLS_OPTBR_DBRDPEN_Pos	0	/* Debug Port读取保护使能 (Debug Read Protection Enable)
00/01/11：DBRDP不使能
10：DBRDP使能 */
#define	FLS_OPTBR_DBRDPEN_Msk	(0xfU << FLS_OPTBR_DBRDPEN_Pos)
	/* DBRDP不使能 */
	/* DBRDP不使能 */
	/* DBRDP不使能 */
	/* DBRDP使能 */

#define	FLS_OPTBR_LOCK1_Pos	0	/* ACLOCK配置寄存器低32bit，分别用于控制Block15~Block0的应用代码读写锁定。每个Block大小为8KB，每个Block使用2bit进行权限控制。 */
#define	FLS_OPTBR_LOCK1_Msk	(0xffffffffU << FLS_OPTBR_LOCK1_Pos)

#define	FLS_OPTBR_LOCK2_Pos	0	/* ACLOCK配置寄存器高32bit，分别用于控制Block31~Block16的应用代码读写锁定。每个Block大小为8KB，每个Block使用2bit进行权限控制 */
#define	FLS_OPTBR_LOCK2_Msk	(0xffffffffU << FLS_OPTBR_LOCK2_Pos)

#define	FLS_OPTBR_LOCK3_Pos	0	/* ACLOCK配置寄存器高32bit，分别用于控制Block31~Block16的应用代码读写锁定。每个Block大小为8KB，每个Block使用2bit进行权限控制 */
#define	FLS_OPTBR_LOCK3_Msk	(0xffffffffU << FLS_OPTBR_LOCK3_Pos)

#define	FLS_OPTBR_LOCK4_Pos	0	/* ACLOCK配置寄存器高32bit，分别用于控制Block31~Block16的应用代码读写锁定。每个Block大小为8KB，每个Block使用2bit进行权限控制 */
#define	FLS_OPTBR_LOCK4_Msk	(0xffffffffU << FLS_OPTBR_LOCK4_Pos)

#define	FLS_EPCR_ERTYPE_Pos	8	/* Flash擦除类型配置 (Erase Type)
00/11：Page Erase
01：Sector Erase
10：Chip Erase (SWD only) */
#define	FLS_EPCR_ERTYPE_Msk	(0x3U << FLS_EPCR_ERTYPE_Pos)
#define	FLS_EPCR_ERTYPE_PAGE	(0x0U << FLS_EPCR_ERTYPE_Pos)	/* Page Erase */
#define	FLS_EPCR_ERTYPE_SECTOR	(0x1U << FLS_EPCR_ERTYPE_Pos)	/* Sector Erase */
#define	FLS_EPCR_ERTYPE_CHIP_ERASE	(0x2U << FLS_EPCR_ERTYPE_Pos)	/* Chip Erase (SWD only) */

#define	FLS_EPCR_PREQ_Pos	1	/* Program Request
软件置位，硬件完成编程后自动清零 */
#define	FLS_EPCR_PREQ_Msk	(0x1U << FLS_EPCR_PREQ_Pos)

#define	FLS_EPCR_EREQ_Pos	0	/* Erase Request
软件置位，硬件完成擦除后自动清零 */
#define	FLS_EPCR_EREQ_Msk	(0x1U << FLS_EPCR_EREQ_Pos)

#define	FLS_KEY_KEY_Pos	0	/* Flash擦写Key输入寄存器，软件或者SWD在启动擦写前必须正确地向此地址写入合法KEY序列。 (Flash Key) */
#define	FLS_KEY_KEY_Msk	(0xffffffffU << FLS_KEY_KEY_Pos)

#define	FLS_IER_OTPIE_Pos	11	/* OTP编程错误中断使能，1有效 (OTP program error Interrupt Enable) */
#define	FLS_IER_OTPIE_Msk	(0x1U << FLS_IER_OTPIE_Pos)

#define	FLS_IER_AUTHIE_Pos	10	/* Flash读写权限错误中断使能，1有效 (Flash Authentication Error Interrupt Enable) */
#define	FLS_IER_AUTHIE_Msk	(0x1U << FLS_IER_AUTHIE_Pos)

#define	FLS_IER_KEYIE_Pos	9	/* Flash KEY错误中断使能，1有效 (Flash Key Error Interrupt Enable) */
#define	FLS_IER_KEYIE_Msk	(0x1U << FLS_IER_KEYIE_Pos)

#define	FLS_IER_CKIE_Pos	8	/* 擦写定时时钟错误中断使能，1有效 (Erase/Program Clock Error Interrupt Enable) */
#define	FLS_IER_CKIE_Msk	(0x1U << FLS_IER_CKIE_Pos)

#define	FLS_IER_PRDIE_Pos	1	/* 编程完成标志中断使能，1有效 (Program Done Interrupt Enable) */
#define	FLS_IER_PRDIE_Msk	(0x1U << FLS_IER_PRDIE_Pos)

#define	FLS_IER_ERDIE_Pos	0	/* 擦写完成标志中断使能，1有效 (Erase Done Interrupt Enable) */
#define	FLS_IER_ERDIE_Msk	(0x1U << FLS_IER_ERDIE_Pos)

#define	FLS_ISR_KEYSTA_Pos	17	/* Flash擦写KEY输入状态 (Flash Key Status)
000：Flash写保护状态，未输入KEY
001：全擦解锁状态
010：扇区擦解锁状态
011：编程解锁状态
100：KEY错误锁定状态，需要复位才能解锁
101/110/111：RFU */
#define	FLS_ISR_KEYSTA_Msk	(0x7U << FLS_ISR_KEYSTA_Pos)
#define	FLS_ISR_KEYSTA_KEY_NONE_PROTECT	(0x0U << FLS_ISR_KEYSTA_Pos)	/* Flash写保护状态，未输入KEY */
#define	FLS_ISR_KEYSTA_CHIP_ERASE	(0x1U << FLS_ISR_KEYSTA_Pos)	/* 全擦解锁状态 */
#define	FLS_ISR_KEYSTA_SECTOR_ERASE	(0x2U << FLS_ISR_KEYSTA_Pos)	/* 扇区擦解锁状态 */
#define	FLS_ISR_KEYSTA_PROGRAM	(0x3U << FLS_ISR_KEYSTA_Pos)	/* 编程解锁状态 */
#define	FLS_ISR_KEYSTA_KEY_ERR_PROTECT	(0x4U << FLS_ISR_KEYSTA_Pos)	/* KEY错误锁定状态，需要复位才能解锁 */

#define	FLS_ISR_BTSF_Pos	16	/* BootSwap标志寄存器 (BootSwap)
0：启动程序区为Flash物理地址0000H~1FFFH
1：启动程序区为Flash物理地址2000H~3FFFH */
#define	FLS_ISR_BTSF_Msk	(0x1U << FLS_ISR_BTSF_Pos)
#define	FLS_ISR_BTSF_START_0000H	(0x0U << FLS_ISR_BTSF_Pos)	/* 启动程序区为Flash物理地址0000H~1FFFH */
#define	FLS_ISR_BTSF_START_2000H	(0x1U << FLS_ISR_BTSF_Pos)	/* 启动程序区为Flash物理地址2000H~3FFFH */

#define	FLS_ISR_OTPERR_Pos	11	/* OTP page编程权限错误，硬件置位，软件写1清零 (OTP program Error Flag. Write 1 to clear)
1：尝试对已编程的OTP字节进行编程
0：无OTP编程错误 */
#define	FLS_ISR_OTPERR_Msk	(0x1U << FLS_ISR_OTPERR_Pos)

#define	FLS_ISR_AUTHERR_Pos	10	/* Flash读写权限错误，读取LOCK块数据或对LOCK块擦写时置位，软件写1清零。(Flash Authentication Error Flag, write 1 to clear)
1：Flash访问权限错误
0：Flash访问没有发生权限错误 */
#define	FLS_ISR_AUTHERR_Msk	(0x1U << FLS_ISR_AUTHERR_Pos)

#define	FLS_ISR_KEYERR_Pos	9	/* Flash KEY错误，硬件置位，软件写1清零 (Flash Key Error Flag, write 1 to clear) */
#define	FLS_ISR_KEYERR_Msk	(0x1U << FLS_ISR_KEYERR_Pos)

#define	FLS_ISR_CKERR_Pos	8	/* 擦写定时时钟错误，NVMIF擦写Flash时如果RCHF未使能，则触发CKERR中断，软件写1清零。(Erase/Program Clock Error Flag, write 1 to clear) */
#define	FLS_ISR_CKERR_Msk	(0x1U << FLS_ISR_CKERR_Pos)

#define	FLS_ISR_PRD_Pos	1	/* Program Done，编程完成标志，硬件置位，软件写1清零(Program Done Flag,write 1 to clear) */
#define	FLS_ISR_PRD_Msk	(0x1U << FLS_ISR_PRD_Pos)

#define	FLS_ISR_ERD_Pos	0	/* Erase Done，擦写完成标志，硬件置位，软件写1清零(Erase Done Flag,write 1 to clear) */
#define	FLS_ISR_ERD_Msk	(0x1U << FLS_ISR_ERD_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void FLS_Deinit(void);

/* Flash读等待周期配置 (Wait Cycles Config)
00/11：0 wait cycle
01：1 wait cycle
10：2 wait cycles
CPU主频小于等于24MHz时，不需要开启wait；主频大于24M小于48Mhz时使能1 wait，主频大于48Mhz时使能2 wait 相关函数 */
extern void FLS_RDCR_WAIT_Set(uint32_t SetValue);
extern uint32_t FLS_RDCR_WAIT_Get(void);

/* 指令预取指使能，在WAIT==00的情况下写1无效 (Prefetch Enable)
1：使能Prefetch
0：禁止Prefetch 相关函数 */
extern void FLS_PFCR_PRFTEN_Setable(FunState NewState);
extern FunState FLS_PFCR_PRFTEN_Getable(void);

/* IWDT在休眠模式下是否允许应用暂停计数 (IWDT Sleep Enable)
1：允许应用在休眠模式下暂停IWDT计数
0：禁止应用在休眠模式下暂停IWDT计数 相关函数 */
extern FunState FLS_OPTBR_IWDTSLP_Getable(void);

extern FunState FLS_OPTBR_DBGCFGEN_Chk(void);

/* Information2区锁定标志 (IF2 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 相关函数 */
extern FlagStatus FLS_OPTBR_IF2LOCK_Chk(void);

/* Information1区锁定标志 (IF1 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 相关函数 */
extern FlagStatus FLS_OPTBR_IF1LOCK_Chk(void);

/* DataFlash使能 (DataFlash Enable)
0：无data flash
1：有data flash 相关函数 */
extern FunState FLS_OPTBR_DFLSEN_Getable(void);

/* BootSwap功能使能 (BootSwap Enable)
00/01/11：禁止BootSwap功能
10：允许BootSwap 相关函数 */
extern FunState FLS_OPTBR_BTSEN_Getable(void);

/* 应用代码权限锁定使能 (AppCode Lock Enable)
00/01/11：ACLOCK不使能
10：ACLOCK使能 相关函数 */
extern FunState FLS_OPTBR_ACLOCKEN_Getable(void);

/* Debug Port读取保护使能 (Debug Read Protection Enable)
00/01/11：DBRDP不使能
10：DBRDP使能 相关函数 */
extern uint32_t FLS_OPTBR_DBRDPEN_Getable(void);

/* Flash擦除类型配置 (Erase Type)
00/11：Page Erase
01：Sector Erase
10：Chip Erase (SWD only) 相关函数 */
extern void FLS_EPCR_ERTYPE_Set(uint32_t SetValue);
extern uint32_t FLS_EPCR_ERTYPE_Get(void);

/* Program Request
软件置位，硬件完成编程后自动清零 相关函数 */
extern void FLS_EPCR_PREQ_Set(uint32_t SetValue);
extern uint32_t FLS_EPCR_PREQ_Get(void);

/* Erase Request
软件置位，硬件完成擦除后自动清零 相关函数 */
extern void FLS_EPCR_EREQ_Set(uint32_t SetValue);
extern uint32_t FLS_EPCR_EREQ_Get(void);

/* Flash擦写Key输入寄存器，软件或者SWD在启动擦写前必须正确地向此地址写入合法KEY序列。 (Flash Key) 相关函数 */
extern void FLS_KEY_Write(uint32_t SetValue);

/* OTP编程错误中断使能，1有效 (OTP program error Interrupt Enable) 相关函数 */
extern void FLS_IER_OTPIE_Setable(FunState NewState);
extern FunState FLS_IER_OTPIE_Getable(void);

/* Flash读写权限错误中断使能，1有效 (Flash Authentication Error Interrupt Enable) 相关函数 */
extern void FLS_IER_AUTHIE_Setable(FunState NewState);
extern FunState FLS_IER_AUTHIE_Getable(void);

/* Flash KEY错误中断使能，1有效 (Flash Key Error Interrupt Enable) 相关函数 */
extern void FLS_IER_KEYIE_Setable(FunState NewState);
extern FunState FLS_IER_KEYIE_Getable(void);

/* 擦写定时时钟错误中断使能，1有效 (Erase/Program Clock Error Interrupt Enable) 相关函数 */
extern void FLS_IER_CKIE_Setable(FunState NewState);
extern FunState FLS_IER_CKIE_Getable(void);

/* 编程完成标志中断使能，1有效 (Program Done Interrupt Enable) 相关函数 */
extern void FLS_IER_PRDIE_Setable(FunState NewState);
extern FunState FLS_IER_PRDIE_Getable(void);

/* 擦写完成标志中断使能，1有效 (Erase Done Interrupt Enable) 相关函数 */
extern void FLS_IER_ERDIE_Setable(FunState NewState);
extern FunState FLS_IER_ERDIE_Getable(void);

/* Flash擦写KEY输入状态 (Flash Key Status)
000：Flash写保护状态，未输入KEY
001：全擦解锁状态
010：扇区擦解锁状态
011：编程解锁状态
100：KEY错误锁定状态，需要复位才能解锁
101/110/111：RFU 相关函数 */
extern uint32_t FLS_ISR_KEYSTA_Get(void);

/* BootSwap标志寄存器 (BootSwap)
0：启动程序区为Flash物理地址0000H~1FFFH
1：启动程序区为Flash物理地址2000H~3FFFH 相关函数 */
extern uint32_t FLS_ISR_BTSF_Get(void);

/* OTP page编程权限错误，硬件置位，软件写1清零 (OTP program Error Flag. Write 1 to clear)
1：尝试对已编程的OTP字节进行编程
0：无OTP编程错误 相关函数 */
extern void FLS_ISR_OTPERR_Clr(void);
extern FlagStatus FLS_ISR_OTPERR_Chk(void);

/* Flash读写权限错误，读取LOCK块数据或对LOCK块擦写时置位，软件写1清零。(Flash Authentication Error Flag, write 1 to clear)
1：Flash访问权限错误
0：Flash访问没有发生权限错误 相关函数 */
extern void FLS_ISR_AUTHERR_Clr(void);
extern FlagStatus FLS_ISR_AUTHERR_Chk(void);

/* Flash KEY错误，硬件置位，软件写1清零 (Flash Key Error Flag, write 1 to clear) 相关函数 */
extern void FLS_ISR_KEYERR_Clr(void);
extern FlagStatus FLS_ISR_KEYERR_Chk(void);

/* 擦写定时时钟错误，NVMIF擦写Flash时如果RCHF未使能，则触发CKERR中断，软件写1清零。(Erase/Program Clock Error Flag, write 1 to clear) 相关函数 */
extern void FLS_ISR_CKERR_Clr(void);
extern FlagStatus FLS_ISR_CKERR_Chk(void);

/* Program Done，编程完成标志，硬件置位，软件写1清零(Program Done Flag,write 1 to clear) 相关函数 */
extern void FLS_ISR_PRD_Clr(void);
extern FlagStatus FLS_ISR_PRD_Chk(void);

/* Erase Done，擦写完成标志，硬件置位，软件写1清零(Erase Done Flag,write 1 to clear) 相关函数 */
extern void FLS_ISR_ERD_Clr(void);
extern FlagStatus FLS_ISR_ERD_Chk(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_FLASH_H */
