/**
  ******************************************************************************
  * @file    fm33a0xxev_pae.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_pae.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup PAE 
  * @brief PAE driver modules
  * @{
  */ 

/* 输入数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48 相关函数 */
void PAE_CSR_DATA_TYPE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->CSR;
	tmpreg &= ~(PAE_CSR_DATA_TYPE_Msk);
	tmpreg |= (SetValue & PAE_CSR_DATA_TYPE_Msk);
	PAE->CSR = tmpreg;
}

uint32_t PAE_CSR_DATA_TYPE_Get(void)
{
	return (PAE->CSR & PAE_CSR_DATA_TYPE_Msk);
}

/* 运算启动位，软件写1启动计算，计算结束后硬件自动清零 相关函数 */
void PAE_CSR_START_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		PAE->CSR |= (PAE_CSR_START_Msk);
	}
	else
	{
		PAE->CSR &= ~(PAE_CSR_START_Msk);
	}
}

/* 模块工作模式：
00：模式0，单次模运算，可配置4Block/16Block
01：模式1，单次点运算，固定16Block
10：模式2，32bit密钥连续RSA运算，固定4Block
11：模式3，32bit密钥连续ECC运算，固定16Block 相关函数 */
void PAE_CSR_RUN_MODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->CSR;
	tmpreg &= ~(PAE_CSR_RUN_MODE_Msk);
	tmpreg |= (SetValue & PAE_CSR_RUN_MODE_Msk);
	PAE->CSR = tmpreg;
}

uint32_t PAE_CSR_RUN_MODE_Get(void)
{
	return (PAE->CSR & PAE_CSR_RUN_MODE_Msk);
}

/* PAE软件复位，软件写1复位PAE内部主要寄存器（模长寄存器PAE_MLR和模参寄存器PAE_MPR不受影响），复位结束后硬件自动清零
注意：软件写SOFT_RST之后，需插入至少一条NOP指令后，再写入PAE其他寄存器；在SOFT_RST操作后立即写PAE寄存器可能写入失败。 相关函数 */
void PAE_CSR_SOFT_RST_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		PAE->CSR |= (PAE_CSR_SOFT_RST_Msk);
	}
	else
	{
		PAE->CSR &= ~(PAE_CSR_SOFT_RST_Msk);
	}
}

/* PAE中断使能
1：使能中断输出
0：禁止中断输出 相关函数 */
void PAE_CSR_PAEIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		PAE->CSR |= (PAE_CSR_PAEIE_Msk);
	}
	else
	{
		PAE->CSR &= ~(PAE_CSR_PAEIE_Msk);
	}
}

FunState PAE_CSR_PAEIE_Getable(void)
{
	if (PAE->CSR & (PAE_CSR_PAEIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}
/* PAE中断标志寄存器 相关函数 */
void PAE_CSR_PAEIF_Clr(void)
{
	PAE->CSR = PAE_CSR_PAEIF_Msk;
}

FlagStatus PAE_CSR_PAEIF_Chk(void)
{
	if (PAE->CSR & PAE_CSR_PAEIF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* PAE工作标志
1：PAE正在运算中
0：PAE空闲 相关函数 */
FlagStatus PAE_CSR_BUSY_Chk(void)
{
	if (PAE->CSR & PAE_CSR_BUSY_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 在RAM配置为4 Block时，模长最大为2048bit：
模长=（MLR[5:0]+1）*32位。
在RAM配置为16 Block时，模长最大为512bit，MLR只有低4bit有效：
模长=（MLR[3:0]+1）*32位。 相关函数 */
void PAE_MLR_Write(uint32_t SetValue)
{
	PAE->MLR = (SetValue & PAE_MLR_PAE_MLR_Msk);
}

uint32_t PAE_MLR_Read(void)
{
	return (PAE->MLR & PAE_MLR_PAE_MLR_Msk);
}

/* 32bit模参寄存器，只可写，不可读 相关函数 */
void PAE_MPR_Write(uint32_t SetValue)
{
	PAE->MPR = (SetValue);
}

/* RAM Block配置位
1：算法RAM配置为16 Block
0：算法RAM配置位4 Block 相关函数 */
void PAE_M0CFG_BLKCFG_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M0CFG;
	tmpreg &= ~(PAE_M0CFG_BLKCFG_Msk);
	tmpreg |= (SetValue & PAE_M0CFG_BLKCFG_Msk);
	PAE->M0CFG = tmpreg;
}

uint32_t PAE_M0CFG_BLKCFG_Get(void)
{
	return (PAE->M0CFG & PAE_M0CFG_BLKCFG_Msk);
}

/* 单次模运算指令：
00：蒙哥马利模乘
01：蒙哥马利模加
10：RFU
11：蒙哥马利模减 相关函数 */
void PAE_M0CFG_INS_M0_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M0CFG;
	tmpreg &= ~(PAE_M0CFG_INS_M0_Msk);
	tmpreg |= (SetValue & PAE_M0CFG_INS_M0_Msk);
	PAE->M0CFG = tmpreg;
}

uint32_t PAE_M0CFG_INS_M0_Get(void)
{
	return (PAE->M0CFG & PAE_M0CFG_INS_M0_Msk);
}

/* 配置运算结果所在Block
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
void PAE_M0CFG_RES_BLK_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M0CFG;
	tmpreg &= ~(PAE_M0CFG_RES_BLK_Msk);
	tmpreg |= (SetValue & PAE_M0CFG_RES_BLK_Msk);
	PAE->M0CFG = tmpreg;
}

uint32_t PAE_M0CFG_RES_BLK_Get(void)
{
	return (PAE->M0CFG & PAE_M0CFG_RES_BLK_Msk);
}

/* 操作数1地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
void PAE_M0CFG_OP1_BLK_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M0CFG;
	tmpreg &= ~(PAE_M0CFG_OP1_BLK_Msk);
	tmpreg |= (SetValue & PAE_M0CFG_OP1_BLK_Msk);
	PAE->M0CFG = tmpreg;
}

uint32_t PAE_M0CFG_OP1_BLK_Get(void)
{
	return (PAE->M0CFG & PAE_M0CFG_OP1_BLK_Msk);
}

/* 操作数2地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
void PAE_M0CFG_OP2_BLK_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M0CFG;
	tmpreg &= ~(PAE_M0CFG_OP2_BLK_Msk);
	tmpreg |= (SetValue & PAE_M0CFG_OP2_BLK_Msk);
	PAE->M0CFG = tmpreg;
}

uint32_t PAE_M0CFG_OP2_BLK_Get(void)
{
	return (PAE->M0CFG & PAE_M0CFG_OP2_BLK_Msk);
}

/* 单次点运算指令：
000：Jacobin坐标倍点运算
001：混合坐标点加运算
010：RFU
011：蒙哥马利点乘倍点运算（ECDBL）
100：蒙哥马利点乘倍点点加运算（ECADDDBL）
101：蒙哥马利点乘Y坐标恢复运算（ECYRecover）
110：RFU
111：RFU 相关函数 */
void PAE_M1CFG_INS_M1_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M1CFG;
	tmpreg &= ~(PAE_M1CFG_INS_M1_Msk);
	tmpreg |= (SetValue & PAE_M1CFG_INS_M1_Msk);
	PAE->M1CFG = tmpreg;
}

uint32_t PAE_M1CFG_INS_M1_Get(void)
{
	return (PAE->M1CFG & PAE_M1CFG_INS_M1_Msk);
}

/* 参数A指示信号
1：A等于-3
0：A不等于-3 相关函数 */
void PAE_M1CFG_AEN3_M1_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M1CFG;
	tmpreg &= ~(PAE_M1CFG_AEN3_M1_Msk);
	tmpreg |= (SetValue & PAE_M1CFG_AEN3_M1_Msk);
	PAE->M1CFG = tmpreg;
}

uint32_t PAE_M1CFG_AEN3_M1_Get(void)
{
	return (PAE->M1CFG & PAE_M1CFG_AEN3_M1_Msk);
}

/* 当前计算密钥位指示标志
1：当前位为1
0：当前位为0 相关函数 */
void PAE_M1CFG_BIT_VALUE_M1_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M1CFG;
	tmpreg &= ~(PAE_M1CFG_BIT_VALUE_M1_Msk);
	tmpreg |= (SetValue & PAE_M1CFG_BIT_VALUE_M1_Msk);
	PAE->M1CFG = tmpreg;
}

uint32_t PAE_M1CFG_BIT_VALUE_M1_Get(uint32_t SetValue)
{
	return (PAE->M1CFG & PAE_M1CFG_BIT_VALUE_M1_Msk);
}

/* 虚模乘控制位
1：当前密钥位为0时同样进行模乘操作
0：当前密钥位为0时不进行模乘操作 相关函数 */
void PAE_M2CFG_ALWAYS_MULT_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M2CFG;
	tmpreg &= ~(PAE_M2CFG_ALWAYS_MULT_Msk);
	tmpreg |= (SetValue & PAE_M2CFG_ALWAYS_MULT_Msk);
	PAE->M2CFG = tmpreg;
}

uint32_t PAE_M2CFG_ALWAYS_MULT_Get(void)
{
	return (PAE->M2CFG & PAE_M2CFG_ALWAYS_MULT_Msk);
}

/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit 相关函数 */
void PAE_M2CFG_FBO_M2_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M2CFG;
	tmpreg &= ~(PAE_M2CFG_FBO_M2_Msk);
	tmpreg |= (SetValue & PAE_M2CFG_FBO_M2_Msk);
	PAE->M2CFG = tmpreg;
}

uint32_t PAE_M2CFG_FBO_M2_Get(void)
{
	return (PAE->M2CFG & PAE_M2CFG_FBO_M2_Msk);
}

/* 参数A指示信号
1：A等于-3
0：A不等于-3 相关函数 */
void PAE_M3CFG_AEN3_M3_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M3CFG;
	tmpreg &= ~(PAE_M3CFG_AEN3_M3_Msk);
	tmpreg |= (SetValue & PAE_M3CFG_AEN3_M3_Msk);
	PAE->M3CFG = tmpreg;
}

uint32_t PAE_M3CFG_AEN3_M3_Get(void)
{
	return (PAE->M3CFG & PAE_M3CFG_AEN3_M3_Msk);
}

/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit 相关函数 */
void PAE_M3CFG_FBO_M3_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = PAE->M3CFG;
	tmpreg &= ~(PAE_M3CFG_FBO_M3_Msk);
	tmpreg |= (SetValue & PAE_M3CFG_FBO_M3_Msk);
	PAE->M3CFG = tmpreg;
}

uint32_t PAE_M3CFG_FBO_M3_Get(void)
{
	return (PAE->M3CFG & PAE_M3CFG_FBO_M3_Msk);
}

/* 32bit数据寄存器，只可写，不可读；仅在连续RSA运算和连续ECC运算模式下有效。 相关函数 */
void PAE_WORD_Write(uint32_t SetValue)
{
	PAE->WORD = (SetValue);
}


void PAE_Deinit(void)
{
	//PAE->CSR = 0x00000000;
	//PAE->MLR = 0x00000000;
	//PAE->MPR = 0x00000000;
	//PAE->M0CFG = 0x00000000;
	//PAE->M1CFG = 0x00000000;
	//PAE->M2CFG = 0x00000000;
	//PAE->M3CFG = 0x00000000;
	//PAE->WORD = 0x00000000;
}

/******END OF FILE****/

