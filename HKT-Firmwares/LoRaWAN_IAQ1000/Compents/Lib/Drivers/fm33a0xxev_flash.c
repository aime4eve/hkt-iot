/**
  ******************************************************************************
  * @file    fm33a0xxev_flash.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_flash.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup FLASH 
  * @brief FLASH driver modules
  * @{
  */ 

/* Flash读等待周期配置 (Wait Cycles Config)
00/11：0 wait cycle
01：1 wait cycle
10：2 wait cycles
CPU主频小于等于24MHz时，不需要开启wait；主频大于24M小于48Mhz时使能1 wait，主频大于48Mhz时使能2 wait 相关函数 */
void FLS_RDCR_WAIT_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = FLS->RDCR;
	tmpreg &= ~(FLS_RDCR_WAIT_Msk);
	tmpreg |= (SetValue & FLS_RDCR_WAIT_Msk);
	FLS->RDCR = tmpreg;
}

uint32_t FLS_RDCR_WAIT_Get(void)
{
	return (FLS->RDCR & FLS_RDCR_WAIT_Msk);
}

/* 指令预取指使能，在WAIT==00的情况下写1无效 (Prefetch Enable)
1：使能Prefetch
0：禁止Prefetch 相关函数 */
void FLS_PFCR_PRFTEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->PFCR |= (FLS_PFCR_PRFTEN_Msk);
	}
	else
	{
		FLS->PFCR &= ~(FLS_PFCR_PRFTEN_Msk);
	}
}

FunState FLS_PFCR_PRFTEN_Getable(void)
{
	if (FLS->PFCR & (FLS_PFCR_PRFTEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* DBG控制寄存器使能
0：屏蔽
1：使能 */
FunState FLS_OPTBR_DBGCFGEN_Chk(void)
{
	if (FLS->OPTBR & FLS_OPTBR_DBGCFGEN_Msk)
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Information2区锁定标志 (IF2 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 相关函数 */
FlagStatus FLS_OPTBR_IF2LOCK_Chk(void)
{
	if (FLS->OPTBR & FLS_OPTBR_IF2LOCK_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Information1区锁定标志 (IF1 Lock Enable)
0：未锁定
1：已锁定，锁定后软件不可改写本扇区 相关函数 */
FlagStatus FLS_OPTBR_IF1LOCK_Chk(void)
{
	if (FLS->OPTBR & FLS_OPTBR_IF1LOCK_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* DataFlash使能 (DataFlash Enable)
0：无data flash
1：有data flash 相关函数 */
FunState FLS_OPTBR_DFLSEN_Getable(void)
{
	if (FLS->OPTBR & (FLS_OPTBR_DFLSEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* BootSwap功能使能 (BootSwap Enable)
00/01/11：禁止BootSwap功能
10：允许BootSwap 相关函数 */
FunState FLS_OPTBR_BTSEN_Getable(void)
{
	if ((FLS->OPTBR & (FLS_OPTBR_BTSEN_Msk))==(0x10<<8))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 应用代码权限锁定使能 (AppCode Lock Enable)
00/01/11：ACLOCK不使能
10：ACLOCK使能 相关函数 */
FunState FLS_OPTBR_ACLOCKEN_Getable(void)
{
	if ((FLS->OPTBR & (FLS_OPTBR_ACLOCKEN_Msk))==(0x10<<2))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug Port读取保护使能 (Debug Read Protection Enable)
00/01/11：DBRDP不使能
10：DBRDP使能 相关函数 */
uint32_t FLS_OPTBR_DBRDPEN_Getable(void)
{
	 return (FLS->OPTBR & (FLS_OPTBR_DBRDPEN_Msk));
	
}

uint32_t FLS_OPTBR_Read(void)
{
	return (FLS->OPTBR);
}



/* Flash擦除类型配置 (Erase Type)
00/11：Page Erase
01：Sector Erase
10：Chip Erase (SWD only) 相关函数 */
void FLS_EPCR_ERTYPE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = FLS->EPCR;
	tmpreg &= ~(FLS_EPCR_ERTYPE_Msk);
	tmpreg |= (SetValue & FLS_EPCR_ERTYPE_Msk);
	FLS->EPCR = tmpreg;
}

uint32_t FLS_EPCR_ERTYPE_Get(void)
{
	return (FLS->EPCR & FLS_EPCR_ERTYPE_Msk);
}

/* Program Request
软件置位，硬件完成编程后自动清零 相关函数 */
void FLS_EPCR_PREQ_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = FLS->EPCR;
	tmpreg &= ~(FLS_EPCR_PREQ_Msk);
	tmpreg |= (SetValue & FLS_EPCR_PREQ_Msk);
	FLS->EPCR = tmpreg;
}

uint32_t FLS_EPCR_PREQ_Get(void)
{
	return (FLS->EPCR & FLS_EPCR_PREQ_Msk);
}

/* Erase Request
软件置位，硬件完成擦除后自动清零 相关函数 */
void FLS_EPCR_EREQ_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = FLS->EPCR;
	tmpreg &= ~(FLS_EPCR_EREQ_Msk);
	tmpreg |= (SetValue & FLS_EPCR_EREQ_Msk);
	FLS->EPCR = tmpreg;
}

uint32_t FLS_EPCR_EREQ_Get(void)
{
	return (FLS->EPCR & FLS_EPCR_EREQ_Msk);
}

/* Flash擦写Key输入寄存器，软件或者SWD在启动擦写前必须正确地向此地址写入合法KEY序列。 (Flash Key) 相关函数 */
void FLS_KEY_Write(uint32_t SetValue)
{
	FLS->KEY = (SetValue);
}

/* OTP编程错误中断使能，1有效 (OTP program error Interrupt Enable) 相关函数 */
void FLS_IER_OTPIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_OTPIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_OTPIE_Msk);
	}
}

FunState FLS_IER_OTPIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_OTPIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Flash读写权限错误中断使能，1有效 (Flash Authentication Error Interrupt Enable) 相关函数 */
void FLS_IER_AUTHIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_AUTHIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_AUTHIE_Msk);
	}
}

FunState FLS_IER_AUTHIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_AUTHIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Flash KEY错误中断使能，1有效 (Flash Key Error Interrupt Enable) 相关函数 */
void FLS_IER_KEYIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_KEYIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_KEYIE_Msk);
	}
}

FunState FLS_IER_KEYIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_KEYIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 擦写定时时钟错误中断使能，1有效 (Erase/Program Clock Error Interrupt Enable) 相关函数 */
void FLS_IER_CKIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_CKIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_CKIE_Msk);
	}
}

FunState FLS_IER_CKIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_CKIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 编程完成标志中断使能，1有效 (Program Done Interrupt Enable) 相关函数 */
void FLS_IER_PRDIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_PRDIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_PRDIE_Msk);
	}
}

FunState FLS_IER_PRDIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_PRDIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 擦写完成标志中断使能，1有效 (Erase Done Interrupt Enable) 相关函数 */
void FLS_IER_ERDIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		FLS->IER |= (FLS_IER_ERDIE_Msk);
	}
	else
	{
		FLS->IER &= ~(FLS_IER_ERDIE_Msk);
	}
}

FunState FLS_IER_ERDIE_Getable(void)
{
	if (FLS->IER & (FLS_IER_ERDIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Flash擦写KEY输入状态 (Flash Key Status)
000：Flash写保护状态，未输入KEY
001：全擦解锁状态
010：扇区擦解锁状态
011：编程解锁状态
100：KEY错误锁定状态，需要复位才能解锁
101/110/111：RFU 相关函数 */
uint32_t FLS_ISR_KEYSTA_Get(void)
{
	return (FLS->ISR & FLS_ISR_KEYSTA_Msk);
}

/* BootSwap标志寄存器 (BootSwap)
0：启动程序区为Flash物理地址0000H~1FFFH
1：启动程序区为Flash物理地址2000H~3FFFH 相关函数 */
uint32_t FLS_ISR_BTSF_Get(void)
{
	return (FLS->ISR & FLS_ISR_BTSF_Msk);
}

/* OTP page编程权限错误，硬件置位，软件写1清零 (OTP program Error Flag. Write 1 to clear)
1：尝试对已编程的OTP字节进行编程
0：无OTP编程错误 相关函数 */
void FLS_ISR_OTPERR_Clr(void)
{
	FLS->ISR = FLS_ISR_OTPERR_Msk;
}

FlagStatus FLS_ISR_OTPERR_Chk(void)
{
	if (FLS->ISR & FLS_ISR_OTPERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Flash读写权限错误，读取LOCK块数据或对LOCK块擦写时置位，软件写1清零。(Flash Authentication Error Flag, write 1 to clear)
1：Flash访问权限错误
0：Flash访问没有发生权限错误 相关函数 */
void FLS_ISR_AUTHERR_Clr(void)
{
	FLS->ISR = FLS_ISR_AUTHERR_Msk;
}

FlagStatus FLS_ISR_AUTHERR_Chk(void)
{
	if (FLS->ISR & FLS_ISR_AUTHERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Flash KEY错误，硬件置位，软件写1清零 (Flash Key Error Flag, write 1 to clear) 相关函数 */
void FLS_ISR_KEYERR_Clr(void)
{
	FLS->ISR = FLS_ISR_KEYERR_Msk;
}

FlagStatus FLS_ISR_KEYERR_Chk(void)
{
	if (FLS->ISR & FLS_ISR_KEYERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 擦写定时时钟错误，NVMIF擦写Flash时如果RCHF未使能，则触发CKERR中断，软件写1清零。(Erase/Program Clock Error Flag, write 1 to clear) 相关函数 */
void FLS_ISR_CKERR_Clr(void)
{
	FLS->ISR = FLS_ISR_CKERR_Msk;
}

FlagStatus FLS_ISR_CKERR_Chk(void)
{
	if (FLS->ISR & FLS_ISR_CKERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Program Done，编程完成标志，硬件置位，软件写1清零(Program Done Flag,write 1 to clear) 相关函数 */
void FLS_ISR_PRD_Clr(void)
{
	FLS->ISR = FLS_ISR_PRD_Msk;
}

FlagStatus FLS_ISR_PRD_Chk(void)
{
	if (FLS->ISR & FLS_ISR_PRD_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Erase Done，擦写完成标志，硬件置位，软件写1清零(Erase Done Flag,write 1 to clear) 相关函数 */
void FLS_ISR_ERD_Clr(void)
{
	FLS->ISR = FLS_ISR_ERD_Msk;
}

FlagStatus FLS_ISR_ERD_Chk(void)
{
	if (FLS->ISR & FLS_ISR_ERD_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void FLS_Deinit(void)
{
	//FLS->RDCR = 0x00000000;
	//FLS->PFCR = 0x00000000;
	//FLS->OPTBR = 0x00000105;
	//FLS->EPCR = 0x00000000;
	//FLS->KEY = 0x00000000;
	//FLS->IER = 0x00000000;
	//FLS->ISR = 0x00000000;
}

/******END OF FILE****/
