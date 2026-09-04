/**
  ******************************************************************************
  * @file    fm33a0xxev_dbg.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_dbg.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup DBG 
  * @brief DBG driver modules
  * @{
  */ 
  
/* Debug状态下LPTIM使能控制位
1：Debug时关闭LPTIM
0：Debug时保持LPTIM原来状态 相关函数 */
void DBG_CR_DBG_LPT_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_LPT_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_LPT_STOP_Msk);
	}
}

FunState DBG_CR_DBG_LPT_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_LPT_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下BSTIM使能控制位
1：Debug时关闭BSTIM
0：Debug时保持BSTIM原来状态 相关函数 */
void DBG_CR_DBG_BST_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_BST_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_BST_STOP_Msk);
	}
}

FunState DBG_CR_DBG_BST_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_BST_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下ET4使能控制位
1：Debug时关闭ET4
0：Debug时保持ET4原来状态 相关函数 */
void DBG_CR_DBG_ET4_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_ET4_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_ET4_STOP_Msk);
	}
}

FunState DBG_CR_DBG_ET4_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_ET4_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下ET3使能控制位
1：Debug时关闭ET3
0：Debug时保持ET3原来状态 相关函数 */
void DBG_CR_DBG_ET3_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_ET3_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_ET3_STOP_Msk);
	}
}

FunState DBG_CR_DBG_ET3_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_ET3_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下ET2使能控制位
1：Debug时关闭ET2
0：Debug时保持ET2原来状态 相关函数 */
void DBG_CR_DBG_ET2_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_ET2_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_ET2_STOP_Msk);
	}
}

FunState DBG_CR_DBG_ET2_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_ET2_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下ET1使能控制位
1：Debug时关闭ET1
0：Debug时保持ET1原来状态 相关函数 */
void DBG_CR_DBG_ET1_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_ET1_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_ET1_STOP_Msk);
	}
}

FunState DBG_CR_DBG_ET1_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_ET1_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下BT2使能控制位
1：Debug时关闭BT2
0：Debug时保持BT2原来状态 相关函数 */
void DBG_CR_DBG_BT2_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_BT2_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_BT2_STOP_Msk);
	}
}

FunState DBG_CR_DBG_BT2_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_BT2_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下BT1使能控制位
1：Debug时关闭BT1
0：Debug时保持BT1原来状态 相关函数 */
void DBG_CR_DBG_BT1_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_BT1_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_BT1_STOP_Msk);
	}
}

FunState DBG_CR_DBG_BT1_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_BT1_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下WWDT使能控制位
1：Debug时关闭WWDT
0：Debug时保持WWDT原来状态 相关函数 */
void DBG_CR_DBG_WWDT_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_WWDT_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_WWDT_STOP_Msk);
	}
}

FunState DBG_CR_DBG_WWDT_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_WWDT_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Debug状态下IWDT使能控制位
1：Debug时关闭IWDT
0：Debug时保持IWDT开启 相关函数 */
void DBG_CR_DBG_IWDT_STOP_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DBG->CR |= (DBG_CR_DBG_IWDT_STOP_Msk);
	}
	else
	{
		DBG->CR &= ~(DBG_CR_DBG_IWDT_STOP_Msk);
	}
}

FunState DBG_CR_DBG_IWDT_STOP_Getable(void)
{
	if (DBG->CR & (DBG_CR_DBG_IWDT_STOP_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 地址非对齐访问错误标志，写1清零
1：地址非对齐访问错误
0：未进行地址非对齐访问 相关函数 */
void DBG_HDFR_DABORT_ADDR_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_DABORT_ADDR_FLAG_Msk;
}

FlagStatus DBG_HDFR_DABORT_ADDR_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_DABORT_ADDR_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 非法地址访问错误标志，写1清零
1：总线传输中访问了非法地址导致HRESP为高产生错误
0：未访问非法地址 相关函数 */
void DBG_HDFR_DABORT_RESP_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_DABORT_RESP_FLAG_Msk;
}

FlagStatus DBG_HDFR_DABORT_RESP_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_DABORT_RESP_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* SVC instructions未定义标志，写1清零
if the SVCall priority is lower than the currently activelevel, 
or if HardFault or NMI is active, 
or PRIMASK is set,
the core should treat SVC instructions as though theywere UNDEFINED。 相关函数 */
void DBG_HDFR_SVCUNDEF_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_SVCUNDEF_FLAG_Msk;
}

FlagStatus DBG_HDFR_SVCUNDEF_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_SVCUNDEF_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 执行BKPT指令标志，写1清零
1：执行了BKPT指令
0：未执行BKPT指令 相关函数 */
void DBG_HDFR_BKPT_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_BKPT_FLAG_Msk;
}

FlagStatus DBG_HDFR_BKPT_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_BKPT_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Thumb-State标志，写1清零
1：切换到ARM状态
0：处于Thumb-State 相关函数 */
void DBG_HDFR_TBIT_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_TBIT_FLAG_Msk;
}

FlagStatus DBG_HDFR_TBIT_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_TBIT_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 特殊指令标志，写1清零
1：执行了特殊指令代码，如试图在XN区域内取指
0：无特殊指令代码被执行 相关函数 */
void DBG_HDFR_SPECIAL_OP_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_SPECIAL_OP_FLAG_Msk;
}

FlagStatus DBG_HDFR_SPECIAL_OP_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_SPECIAL_OP_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* hardfault标志位，任何类型的hardfault都会导致该位置位，写1清零
1：hardfault 请求
0：无hardfault请求 相关函数 */
void DBG_HDFR_HDF_REQUEST_FLAG_Clr(void)
{
	DBG->HDFR = DBG_HDFR_HDF_REQUEST_FLAG_Msk;
}

FlagStatus DBG_HDFR_HDF_REQUEST_FLAG_Chk(void)
{
	if (DBG->HDFR & DBG_HDFR_HDF_REQUEST_FLAG_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}


void DBG_Deinit(void)
{
	//DBG->CR = 0x00003d03;
	//DBG->HDFR = 0x00000000;
}

/******END OF FILE****/
