/**
  ******************************************************************************
  * @file    fm33a0xxev_aes.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_aes.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup AES 
  * @brief AES driver modules
  * @{
  */ 
  
/* AES加密密钥长度，AESEN=1时不可修改。
(Key Length)
00：128bit
01：192bit
10：256bit
11：保留 相关函数 */
void AES_CR_KEYLEN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = AES->CR;
	tmpreg &= ~(AES_CR_KEYLEN_Msk);
	tmpreg |= (SetValue & AES_CR_KEYLEN_Msk);
	AES->CR = tmpreg;
}

uint32_t AES_CR_KEYLEN_Get(void)
{
	return (AES->CR & AES_CR_KEYLEN_Msk);
}

/* DMA数据自动读出使能
(DMA output enable)
0：不开启
1：开启
该位置位后在模式1，模式3和模式4下AES模块会自动产生AES->RAM的传输请求。模式2下不会产生。 相关函数 */
void AES_CR_DMAOEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->CR |= (AES_CR_DMAOEN_Msk);
	}
	else
	{
		AES->CR &= ~(AES_CR_DMAOEN_Msk);
	}

}

FunState AES_CR_DMAOEN_Getable(void)
{
	if (AES->CR & (AES_CR_DMAOEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 开启DMA数据自动写入使能
(DMA input enable)
0：不开启
1：开启
该位设置为1后在模式1，模式3和模式4以及MultH模式下AES模块会自动产生RAM->AES的传输请求。模式2下不会产生。 相关函数 */
/* AES使能 (AES enable)
0：不使能
1：使能
在任何时候清除AESEN位都能够复位AES模块
在模式2下该位会在一次计算完成后硬件自动清0 相关函数 */

void AES_CR_DMAIEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->CR |= (AES_CR_DMAIEN_Msk);
	}
	else
	{
		AES->CR &= ~(AES_CR_DMAIEN_Msk);
	}
}

FunState AES_CR_DMAIEN_Getable(void)
{
	if (AES->CR & (AES_CR_DMAIEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* AES数据流处理模式，AESEN=1时不可修改。
(Cipher Mode)
00：ECB
01：CBC
10：CTR
11：使用MultH模块 相关函数 */
void AES_CR_CHMOD_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = AES->CR;
	tmpreg &= ~(AES_CR_CHMOD_Msk);
	tmpreg |= (SetValue & AES_CR_CHMOD_Msk);
	AES->CR = tmpreg;
}

uint32_t AES_CR_CHMOD_Get(void)
{
	return (AES->CR & AES_CR_CHMOD_Msk);
}

/* AES工作模式，AESEN=1时不可修改。
(operation MODE)
00：模式1：加密
01：模式2：密钥扩展
10：模式3：解密
11：模式4：密钥扩展+解密
CTR模式下配置成模式4将自动进入CTR的解密模式。即在CHMOD=2’b10时配置MODE=2’b11，AES将按照MODE=2’b10的情形执行。 相关函数 */
void AES_CR_MODE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = AES->CR;
	tmpreg &= ~(AES_CR_MODE_Msk);
	tmpreg |= (SetValue & AES_CR_MODE_Msk);
	AES->CR = tmpreg;
}

uint32_t AES_CR_MODE_Get(void)
{
	return (AES->CR & AES_CR_MODE_Msk);
}

/* 选择数据类型，AESEN=1时不可修改。具体交换规则可参考AES数据类型章节。
(Data type)
00：32bit数据不交换
01：16bit数据半字交换
10：8bit数据字节交换
11：1bit数据比特交换 相关函数 */
void AES_CR_DATATYP_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = AES->CR;
	tmpreg &= ~(AES_CR_DATATYP_Msk);
	tmpreg |= (SetValue & AES_CR_DATATYP_Msk);
	AES->CR = tmpreg;
}

uint32_t AES_CR_DATATYP_Get(void)
{
	return (AES->CR & AES_CR_DATATYP_Msk);
}

/* AES使能 (AES enable)
0：不使能
1：使能
在任何时候清除AESEN位都能够复位AES模块
在模式2下该位会在一次计算完成后硬件自动清0 相关函数 */
void AES_CR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->CR |= (AES_CR_EN_Msk);
	}
	else
	{
		AES->CR &= ~(AES_CR_EN_Msk);
	}
}

FunState AES_CR_EN_Getable(void)
{
	if (AES->CR & (AES_CR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 写错误中断使能，1有效。(Write Error interrupt enable) 相关函数 */
void AES_IER_WRERR_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->IER |= (AES_IER_WRERR_IE_Msk);
	}
	else
	{
		AES->IER &= ~(AES_IER_WRERR_IE_Msk);
	}
}

FunState AES_IER_WRERR_IE_Getable(void)
{
	if (AES->IER & (AES_IER_WRERR_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 读错误中断使能，1有效。(Read Error interrupt enable) 相关函数 */
void AES_IER_RDERR_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->IER |= (AES_IER_RDERR_IE_Msk);
	}
	else
	{
		AES->IER &= ~(AES_IER_RDERR_IE_Msk);
	}
}

FunState AES_IER_RDERR_IE_Getable(void)
{
	if (AES->IER & (AES_IER_RDERR_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* AES计算完成中断使能，1有效。(Cipher Complete Interrupt enable) 相关函数 */
void AES_IER_CCF_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		AES->IER |= (AES_IER_CCF_IE_Msk);
	}
	else
	{
		AES->IER &= ~(AES_IER_CCF_IE_Msk);
	}
}

FunState AES_IER_CCF_IE_Getable(void)
{
	if (AES->IER & (AES_IER_CCF_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 数据输入寄存器，当AES需要输入加解密数据时，应该往该寄存器连续写4次。(AES Data Input)
模式1（加密）：把明文从MSB到LSB分4次写入。
模式2（密钥扩展）：无需使用数据输入寄存器
模式3和模式4（解密）：把密文从MSB到LSB分4次写入。
MultH模式：把乘数A或C从MSB到LSB分4次写入。 相关函数 */
void AES_DIR_Write(uint32_t SetValue)
{
	AES->DIR = (SetValue);
}

uint32_t AES_DIR_Read(void)
{
	return (AES->DIR);
}

/* 数据输出寄存器，当AES计算完成后，可以分四次读出加解密的结果。(AES Data Output)
模式1（加密）：把密文从MSB到LSB分4次读出。
模式2（密钥扩展）：无需使用数据输输出寄存器
模式3和模式4（解密）：把明文从MSB到LSB分4次输出。
MultH模式：运算结果存储在IVR寄存器中，无需读取AES_DOUTR寄存器。 相关函数 */
uint32_t AES_DOR_Read(void)
{
	return (AES->DOR);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY0_Write(uint32_t SetValue)
{
	AES->KEY0 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY1_Write(uint32_t SetValue)
{
	AES->KEY1 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY2_Write(uint32_t SetValue)
{
	AES->KEY2 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY3_Write(uint32_t SetValue)
{
	AES->KEY3 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY4_Write(uint32_t SetValue)
{
	AES->KEY4 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY5_Write(uint32_t SetValue)
{
	AES->KEY5 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY6_Write(uint32_t SetValue)
{
	AES->KEY6 = (SetValue);
}

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
void AES_KEY7_Write(uint32_t SetValue)
{
	AES->KEY7 = (SetValue);
}

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
void AES_IVR0_Write(uint32_t SetValue)
{
	AES->IVR0 = (SetValue);
}

uint32_t AES_IVR0_Read(void)
{
	return (AES->IVR0);
}

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
void AES_IVR1_Write(uint32_t SetValue)
{
	AES->IVR1 = (SetValue);
}

uint32_t AES_IVR1_Read(void)
{
	return (AES->IVR1);
}

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
void AES_IVR2_Write(uint32_t SetValue)
{
	AES->IVR2 = (SetValue);
}

uint32_t AES_IVR2_Read(void)
{
	return (AES->IVR2);
}

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
void AES_IVR3_Write(uint32_t SetValue)
{
	AES->IVR3 = (SetValue);
}

uint32_t AES_IVR3_Read(void)
{
	return (AES->IVR3);
}

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
void AES_H0_Write(uint32_t SetValue)
{
	AES->H0 = (SetValue);
}

uint32_t AES_H0_Read(void)
{
	return (AES->H0);
}

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
void AES_H1_Write(uint32_t SetValue)
{
	AES->H1 = (SetValue);
}

uint32_t AES_H1_Read(void)
{
	return (AES->H1);
}

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
void AES_H2_Write(uint32_t SetValue)
{
	AES->H2 = (SetValue);
}

uint32_t AES_H2_Read(void)
{
	return (AES->H2);
}

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
void AES_H3_Write(uint32_t SetValue)
{
	AES->H3 = (SetValue);
}

uint32_t AES_H3_Read(void)
{
	return (AES->H3);
}


/* 写错误标志：在计算或输出阶段发生写操作时置位，软件写1清零 相关函数 */
void AES_ISR_WRERR_Clr(void)
{
	AES->ISR = AES_ISR_WRERR_Msk;
}

FlagStatus AES_ISR_WRERR_Chk(void)
{
	if (AES->ISR & AES_ISR_WRERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 读错误标志：在计算或输入阶段发生读操作时置位，软件写1清零 相关函数 */
void AES_ISR_RDERR_Clr(void)
{
	AES->ISR = AES_ISR_RDERR_Msk;
}

FlagStatus AES_ISR_RDERR_Chk(void)
{
	if (AES->ISR & AES_ISR_RDERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* AES计算完成标志，软件写1清零
1：计算完成
0：计算没有完成 相关函数 */
void AES_ISR_CCF_Clr(void)
{
	AES->ISR = AES_ISR_CCF_Msk;
}

FlagStatus AES_ISR_CCF_Chk(void)
{
	if (AES->ISR & AES_ISR_CCF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

void AES_Deinit(void)
{
	//AES->CR = 0x00000000;
	//AES->IER = 0x00000000;
	//AES->DIR = 0x00000000;
	//AES->DOR = 0x00000000;
	//AES->KEY0 = 0x00000000;
	//AES->KEY1 = 0x00000000;
	//AES->KEY2 = 0x00000000;
	//AES->KEY3 = 0x00000000;
	//AES->KEY4 = 0x00000000;
	//AES->KEY5 = 0x00000000;
	//AES->KEY6 = 0x00000000;
	//AES->KEY7 = 0x00000000;
	//AES->IVR0 = 0x00000000;
	//AES->IVR1 = 0x00000000;
	//AES->IVR2 = 0x00000000;
	//AES->IVR3 = 0x00000000;
	//AES->H0 = 0x00000000;
	//AES->H1 = 0x00000000;
	//AES->H2 = 0x00000000;
	//AES->H3 = 0x00000000;
}
//Code_End

/********************************
AES 功能控制函数
功能: 加解密密钥输入(key0存放密钥最低32bit)
输入：密钥	16/24/32字节长度的密钥
			长度	16/24/32
输出: 无
********************************/
void AES_KEY_WriteEx(uint8_t *KeyIn, uint8_t Len)
{
	uint8_t i;
	__IO uint32_t *PointKeyReg;
	__IO uint32_t Temp32;
	
	if( (Len != 16) && (Len != 24) && (Len != 32) ) return;
	
	PointKeyReg = (&AES->KEY0);
	for(i=0; i<Len; i=i+4)
	{
		Temp32 = (KeyIn[0+i]<<24)|(KeyIn[1+i]<<16)|(KeyIn[2+i]<<8)|(KeyIn[3+i]<<0);
		PointKeyReg[(Len-i)/4-1] = Temp32;
	}
}

/*加解密密钥输出，第一个字存放密钥最低32bit  */
void AES_KEY_ReadEx(uint8_t *KeyOut, uint8_t Len)
{
	uint8_t i;
	__IO uint32_t *PointKeyReg;
	__IO uint32_t Temp32;
	
	if( (Len != 16) && (Len != 24) && (Len != 32) ) return;
	
	PointKeyReg = (&AES->KEY0);
	for(i=0; i<Len; i=i+4)
	{
		Temp32 = PointKeyReg[(Len-i)/4-1];
		KeyOut[0+i] = Temp32>>24;
		KeyOut[1+i] = Temp32>>16;
		KeyOut[2+i] = Temp32>>8;
		KeyOut[3+i] = Temp32>>0;
	}
}

/*初始向量输入 */
void AES_IVR_WriteEx(uint8_t *IVRIn)
{
	uint8_t i;
	__IO uint32_t *PointIvrReg;
	__IO uint32_t Temp32;
	
	PointIvrReg = (&AES->IVR0);
	for(i=0; i<4*4; i=i+4)
	{
		Temp32 = (IVRIn[0+i]<<24)|(IVRIn[1+i]<<16)|(IVRIn[2+i]<<8)|(IVRIn[3+i]<<0);
		PointIvrReg[(16-i)/4-1] = Temp32;
	}
}

/*初始向量读取 */
void AES_IVR_ReadEx(uint8_t *IVROut)
{
	uint8_t i;
	__IO uint32_t *PointIvrReg;
	__IO uint32_t Temp32;
	
	PointIvrReg = (&AES->IVR0);
	for(i=0; i<4*4; i=i+4)
	{
		Temp32 = PointIvrReg[(4*4-i)/4-1];
		IVROut[0+i] = Temp32>>24;
		IVROut[1+i] = Temp32>>16;
		IVROut[2+i] = Temp32>>8;
		IVROut[3+i] = Temp32>>0;	
	}
}


/*加解密数据输入，16字节（128bit）分组数据输入  */
void AES_DIN_GroupWrite_128BIT(uint8_t *DataIn)
{
	uint8_t i;
	__IO uint32_t Temp32;

	for(i=0; i<16; i=i+4)
	{
		Temp32 = (DataIn[0+i]<<24)|(DataIn[1+i]<<16)|(DataIn[2+i]<<8)|(DataIn[3+i]<<0);
		AES->DIR = Temp32;
	}
}

/*加解密数据输出，16字节（128bit）分组数据输出  */
void AES_DOUT_GroupRead_128BIT(uint8_t *DataOut)
{
	uint8_t i;
	__IO uint32_t Temp32;

	for(i=0; i<16; i=i+4)
	{
		Temp32 = AES->DOR;
		DataOut[0+i] = Temp32>>24;
		DataOut[1+i] = Temp32>>16;
		DataOut[2+i] = Temp32>>8;
		DataOut[3+i] = Temp32>>0;
	}
}


uint8_t AES_GroupWriteAndRead_128BIT(uint8_t *DataIn, uint8_t *DataOut)
{	
	uint16_t i;
	
	//输入加解密数据
	AES_DIN_GroupWrite_128BIT(DataIn);
	
	//等待CCF标志置起,完成计算大约需要100个时钟周期
	for(i=0;i<500;i++)
	{
		if(SET == AES_ISR_CCF_Chk() ) break;
	}
	//清CCF标志
	//读取前必须清除
	AES_ISR_CCF_Clr();
	
	//读取结果
	AES_DOUT_GroupRead_128BIT(DataOut);
	
	if(i==500) return 1;
	else return 0;
}


uint8_t AES_GroupWriteAndRead_128BIT_IVR(uint8_t *DataIn, uint8_t *DataOut)
{	
	uint16_t i;
	
	//输入加解密数据
	AES_DIN_GroupWrite_128BIT(DataIn);
	
	//等待CCF标志置起,完成计算大约需要100个时钟周期
	for(i=0;i<500;i++)
	{
		if(SET == AES_ISR_CCF_Chk() ) break;
	}
	//清CCF标志
	//读取前必须清除
	AES_ISR_CCF_Clr();
	
	//读取结果
	AES_IVR_ReadEx(DataOut);
	
	if(i==500) return 1;
	else return 0;
}

/********************************
AES 功能控制函数
功能: MultH参数寄存器输入(h0存放密钥最低32bit)
输入：MultH运算128bit输入H参数	
输出: 无
********************************/
void AES_Hx_WriteEx(uint8_t *HxIn, uint8_t Len)
{
	uint8_t i;
	__IO uint32_t *PointHxReg;
	__IO uint32_t Temp32;
	
	if(Len != 16) return;
	
	PointHxReg = (&AES->H0);
	for(i=0; i<Len; i=i+4)
	{
		Temp32 = (HxIn[0+i]<<24)|(HxIn[1+i]<<16)|(HxIn[2+i]<<8)|(HxIn[3+i]<<0);
		PointHxReg[(Len-i)/4-1] = Temp32;
	}
}

/* AES模块初始化函数 */
void AES_Init(AES_InitTypeDef* para)
{
	AES_CR_KEYLEN_Set(para->KEYLEN);			//AES加密密钥长度，AESEN=1时不可修改
	AES_CR_CHMOD_Set(para->CHMOD);			//AES数据流处理模式，AESEN=1时不可修改
	AES_CR_MODE_Set(para->MODE);				//AES工作模式，AESEN=1时不可修改
	AES_CR_DATATYP_Set(para->DATATYP);		//选择数据类型，AESEN=1时不可修改。具体交换规则可参考AES数据类型章节	
	AES_CR_DMAOEN_Setable(para->DMAOEN);		//DMA数据自动读出使能
	AES_CR_DMAIEN_Setable(para->DMAIEN);		//DMA数据自动写入使能
	AES_IER_WRERR_IE_Setable(para->WERRIE);		//错误标志（WRERR）中断使能
	AES_IER_RDERR_IE_Setable(para->RERRIE);	//错误标志（RDERR）中断使能
	AES_IER_CCF_IE_Setable(para->CCFIE);			//CCF标志中断使能
	AES_CR_EN_Setable(para->AESEN);			//AES使能 
}
 
/******END OF FILE****/
