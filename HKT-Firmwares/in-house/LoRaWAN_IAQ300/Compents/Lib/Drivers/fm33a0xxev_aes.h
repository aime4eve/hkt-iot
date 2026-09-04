/**
  ******************************************************************************
  * @file    fm33a0xxev_aes.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the AES firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_AES_H
#define __FM33A0XXEV_AES_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 

//------------------------------------------------------------------------------   
typedef struct
{
	uint32_t	KEYLEN;			//AES加密密钥长度，AESEN=1时不可修改
	uint32_t	CHMOD;			//AES数据流处理模式，AESEN=1时不可修改
	uint32_t	MODE;			//AES工作模式，AESEN=1时不可修改
	uint32_t	DATATYP;		//选择数据类型，AESEN=1时不可修改。具体交换规则可参考AES数据类型章节	
	FunState	DMAOEN;			//DMA数据自动读出使能
	FunState	DMAIEN;			//DMA数据自动写入使能
	FunState	WERRIE;			//错误标志（WRERR）中断使能
	FunState	RERRIE;			//错误标志（RDERR）中断使能
	FunState	CCFIE;			//CCF标志中断使能
	FunState	AESEN;			//AES使能 
	
}AES_InitTypeDef;
	 
#define	AES_CR_KEYLEN_Pos	13	/* AES加密密钥长度，AESEN=1时不可修改。
(Key Length)
00：128bit
01：192bit
10：256bit
11：保留 */
#define	AES_CR_KEYLEN_Msk	(0x3U << AES_CR_KEYLEN_Pos)
#define	AES_CR_KEYLEN_128BIT	(0x0U << AES_CR_KEYLEN_Pos)	/* 128bit */
#define	AES_CR_KEYLEN_192BIT	(0x1U << AES_CR_KEYLEN_Pos)	/* 192bit */
#define	AES_CR_KEYLEN_256BIT	(0x2U << AES_CR_KEYLEN_Pos)	/* 256bit */
#define	AES_CR_KEYLEN_	(0x3U << AES_CR_KEYLEN_Pos)	/* 保留 */

#define	AES_CR_DMAOEN_Pos	12	/* DMA数据自动读出使能
(DMA output enable)
0：不开启
1：开启
该位置位后在模式1，模式3和模式4下AES模块会自动产生AES->RAM的传输请求。模式2下不会产生。 */
#define	AES_CR_DMAOEN_Msk	(0x1U << AES_CR_DMAOEN_Pos)


#define	AES_CR_DMAIEN_Pos	11	/* 开启DMA数据自动写入使能
(DMA input enable)
0：不开启
1：开启
该位设置为1后在模式1，模式3和模式4以及MultH模式下AES模块会自动产生RAM->AES的传输请求。模式2下不会产生。 */
#define	AES_CR_DMAIEN_Msk	(0x1U << AES_CR_DMAIEN_Pos)


#define	AES_CR_CHMOD_Pos	5	/* AES数据流处理模式，AESEN=1时不可修改。
(Cipher Mode)
00：ECB
01：CBC
10：CTR
11：使用MultH模块 */
#define	AES_CR_CHMOD_Msk	(0x3U << AES_CR_CHMOD_Pos)
#define	AES_CR_CHMOD_ECB	(0x0U << AES_CR_CHMOD_Pos)	/* ECB */
#define	AES_CR_CHMOD_CBC	(0x1U << AES_CR_CHMOD_Pos)	/* CBC */
#define	AES_CR_CHMOD_CTR	(0x2U << AES_CR_CHMOD_Pos)	/* CTR */
#define	AES_CR_CHMOD_MULTH	(0x3U << AES_CR_CHMOD_Pos)	/* 使用MultH模块 */

#define	AES_CR_MODE_Pos	3	/* AES工作模式，AESEN=1时不可修改。
(operation MODE)
00：模式1：加密
01：模式2：密钥扩展
10：模式3：解密
11：模式4：密钥扩展+解密
CTR模式下配置成模式4将自动进入CTR的解密模式。即在CHMOD=2’b10时配置MODE=2’b11，AES将按照MODE=2’b10的情形执行。 */
#define	AES_CR_MODE_Msk	(0x3U << AES_CR_MODE_Pos)
#define	AES_CR_MODE_ENCRYPT	(0x0U << AES_CR_MODE_Pos)	/* 模式1 */
#define	AES_CR_MODE_KEYEXP	(0x1U << AES_CR_MODE_Pos)	/* 模式2 */
#define	AES_CR_MODE_DECRYPT	(0x2U << AES_CR_MODE_Pos)	/* 模式3 */
#define	AES_CR_MODE_KEYEXPDECRYPT	(0x3U << AES_CR_MODE_Pos)	/* 模式4 */

#define	AES_CR_DATATYP_Pos	1	/* 选择数据类型，AESEN=1时不可修改。具体交换规则可参考AES数据类型章节。
(Data type)
00：32bit数据不交换
01：16bit数据半字交换
10：8bit数据字节交换
11：1bit数据比特交换 */
#define	AES_CR_DATATYP_Msk	(0x3U << AES_CR_DATATYP_Pos)
#define	AES_CR_DATATYP_32BITNOEX	(0x0U << AES_CR_DATATYP_Pos)	/* 32bit数据不交换 */
#define	AES_CR_DATATYP_16BITEX	(0x1U << AES_CR_DATATYP_Pos)	/* 16bit数据半字交换 */
#define	AES_CR_DATATYP_8BITEX	(0x2U << AES_CR_DATATYP_Pos)	/* 8bit数据字节交换 */
#define	AES_CR_DATATYP_1BITEX	(0x3U << AES_CR_DATATYP_Pos)	/* 1bit数据比特交换 */

#define	AES_CR_EN_Pos	0	/* AES使能 (AES enable)
0：不使能
1：使能
在任何时候清除AESEN位都能够复位AES模块
在模式2下该位会在一次计算完成后硬件自动清0 */
#define	AES_CR_EN_Msk	(0x1U << AES_CR_EN_Pos)
	/* 不使能 */
	/* 使能 */

#define	AES_IER_WRERR_IE_Pos	2	/* 写错误中断使能，1有效。(Write Error interrupt enable) */
#define	AES_IER_WRERR_IE_Msk	(0x1U << AES_IER_WRERR_IE_Pos)

#define	AES_IER_RDERR_IE_Pos	1	/* 读错误中断使能，1有效。(Read Error interrupt enable) */
#define	AES_IER_RDERR_IE_Msk	(0x1U << AES_IER_RDERR_IE_Pos)

#define	AES_IER_CCF_IE_Pos	0	/* AES计算完成中断使能，1有效。(Cipher Complete Interrupt enable) */
#define	AES_IER_CCF_IE_Msk	(0x1U << AES_IER_CCF_IE_Pos)

#define	AES_DIR_DIN_Pos	0	/* 数据输入寄存器，当AES需要输入加解密数据时，应该往该寄存器连续写4次。(AES Data Input)
模式1（加密）：把明文从MSB到LSB分4次写入。
模式2（密钥扩展）：无需使用数据输入寄存器
模式3和模式4（解密）：把密文从MSB到LSB分4次写入。
MultH模式：把乘数A或C从MSB到LSB分4次写入。 */
#define	AES_DIR_DIN_Msk	(0xffffffffU << AES_DIR_DIN_Pos)

#define	AES_DOR_DOUT_Pos	0	/* 数据输出寄存器，当AES计算完成后，可以分四次读出加解密的结果。(AES Data Output)
模式1（加密）：把密文从MSB到LSB分4次读出。
模式2（密钥扩展）：无需使用数据输输出寄存器
模式3和模式4（解密）：把明文从MSB到LSB分4次输出。
MultH模式：运算结果存储在IVR寄存器中，无需读取AES_DOUTR寄存器。 */
#define	AES_DOR_DOUT_Msk	(0xffffffffU << AES_DOR_DOUT_Pos)

#define	AES_KEY0_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY0_KEYx_Msk	(0xffffffffU << AES_KEY0_KEYx_Pos)

#define	AES_KEY1_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY1_KEYx_Msk	(0xffffffffU << AES_KEY1_KEYx_Pos)

#define	AES_KEY2_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY2_KEYx_Msk	(0xffffffffU << AES_KEY2_KEYx_Pos)

#define	AES_KEY3_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY3_KEYx_Msk	(0xffffffffU << AES_KEY3_KEYx_Pos)

#define	AES_KEY4_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY4_KEYx_Msk	(0xffffffffU << AES_KEY4_KEYx_Pos)

#define	AES_KEY5_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY5_KEYx_Msk	(0xffffffffU << AES_KEY5_KEYx_Pos)

#define	AES_KEY6_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY6_KEYx_Msk	(0xffffffffU << AES_KEY6_KEYx_Pos)

#define	AES_KEY7_KEYx_Pos	0	/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 */
#define	AES_KEY7_KEYx_Msk	(0xffffffffU << AES_KEY7_KEYx_Pos)

#define	AES_IVR0_IVRx_Pos	0	/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) */
#define	AES_IVR0_IVRx_Msk	(0xffffffffU << AES_IVR0_IVRx_Pos)

#define	AES_IVR1_IVRx_Pos	0	/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) */
#define	AES_IVR1_IVRx_Msk	(0xffffffffU << AES_IVR1_IVRx_Pos)

#define	AES_IVR2_IVRx_Pos	0	/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) */
#define	AES_IVR2_IVRx_Msk	(0xffffffffU << AES_IVR2_IVRx_Pos)

#define	AES_IVR3_IVRx_Pos	0	/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) */
#define	AES_IVR3_IVRx_Msk	(0xffffffffU << AES_IVR3_IVRx_Pos)

#define	AES_H0_Hx_Pos	0	/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] */
#define	AES_H0_Hx_Msk	(0xffffffffU << AES_H0_Hx_Pos)

#define	AES_H1_Hx_Pos	0	/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] */
#define	AES_H1_Hx_Msk	(0xffffffffU << AES_H1_Hx_Pos)

#define	AES_H2_Hx_Pos	0	/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] */
#define	AES_H2_Hx_Msk	(0xffffffffU << AES_H2_Hx_Pos)

#define	AES_H3_Hx_Pos	0	/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] */
#define	AES_H3_Hx_Msk	(0xffffffffU << AES_H3_Hx_Pos)

#define	AES_ISR_WRERR_Pos	2	/* 写错误标志：在计算或输出阶段发生写操作时置位，软件写1清零 */
#define	AES_ISR_WRERR_Msk	(0x1U << AES_ISR_WRERR_Pos)

#define	AES_ISR_RDERR_Pos	1	/* 读错误标志：在计算或输入阶段发生读操作时置位，软件写1清零 */
#define	AES_ISR_RDERR_Msk	(0x1U << AES_ISR_RDERR_Pos)

#define	AES_ISR_CCF_Pos	0	/* AES计算完成标志，软件写1清零
1：计算完成
0：计算没有完成 */
#define	AES_ISR_CCF_Msk	(0x1U << AES_ISR_CCF_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void AES_Deinit(void);

/* AES加密密钥长度，AESEN=1时不可修改。
(Key Length)
00：128bit
01：192bit
10：256bit
11：保留 相关函数 */
extern void AES_CR_KEYLEN_Set(uint32_t SetValue);
extern uint32_t AES_CR_KEYLEN_Get(void);

/* DMA数据自动读出使能
(DMA output enable)
0：不开启
1：开启
该位置位后在模式1，模式3和模式4下AES模块会自动产生AES->RAM的传输请求。模式2下不会产生。 相关函数 */
extern void AES_CR_DMAOEN_Setable(FunState NewState);
extern FunState AES_CR_DMAOEN_Getable(void);


/* 开启DMA数据自动写入使能
(DMA input enable)
0：不开启
1：开启
该位设置为1后在模式1，模式3和模式4以及MultH模式下AES模块会自动产生RAM->AES的传输请求。模式2下不会产生。 相关函数 */
extern void AES_CR_DMAIEN_Setable(FunState NewState);
extern FunState FunStateAES_CR_DMAIEN_Getable(void);

/* AES数据流处理模式，AESEN=1时不可修改。
(Cipher Mode)
00：ECB
01：CBC
10：CTR
11：使用MultH模块 相关函数 */
extern void AES_CR_CHMOD_Set(uint32_t SetValue);
extern uint32_t AES_CR_CHMOD_Get(void);

/* AES工作模式，AESEN=1时不可修改。
(operation MODE)
00：模式1：加密
01：模式2：密钥扩展
10：模式3：解密
11：模式4：密钥扩展+解密
CTR模式下配置成模式4将自动进入CTR的解密模式。即在CHMOD=2’b10时配置MODE=2’b11，AES将按照MODE=2’b10的情形执行。 相关函数 */
extern void AES_CR_MODE_Set(uint32_t SetValue);
extern uint32_t AES_CR_MODE_Get(void);

/* 选择数据类型，AESEN=1时不可修改。具体交换规则可参考AES数据类型章节。
(Data type)
00：32bit数据不交换
01：16bit数据半字交换
10：8bit数据字节交换
11：1bit数据比特交换 相关函数 */
extern void AES_CR_DATATYP_Set(uint32_t SetValue);
extern uint32_t AES_CR_DATATYP_Get(void);

/* AES使能 (AES enable)
0：不使能
1：使能
在任何时候清除AESEN位都能够复位AES模块
在模式2下该位会在一次计算完成后硬件自动清0 相关函数 */
extern void AES_CR_EN_Setable(FunState NewState);
extern FunState AES_CR_EN_Getable(void);

/* 写错误中断使能，1有效。(Write Error interrupt enable) 相关函数 */
extern void AES_IER_WRERR_IE_Setable(FunState NewState);
extern FunState AES_IER_WRERR_IE_Getable(void);

/* 读错误中断使能，1有效。(Read Error interrupt enable) 相关函数 */
extern void AES_IER_RDERR_IE_Setable(FunState NewState);
extern FunState AES_IER_RDERR_IE_Getable(void);

/* AES计算完成中断使能，1有效。(Cipher Complete Interrupt enable) 相关函数 */
extern void AES_IER_CCF_IE_Setable(FunState NewState);
extern FunState AES_IER_CCF_IE_Getable(void);

/* 数据输入寄存器，当AES需要输入加解密数据时，应该往该寄存器连续写4次。(AES Data Input)
模式1（加密）：把明文从MSB到LSB分4次写入。
模式2（密钥扩展）：无需使用数据输入寄存器
模式3和模式4（解密）：把密文从MSB到LSB分4次写入。
MultH模式：把乘数A或C从MSB到LSB分4次写入。 相关函数 */
extern void AES_DIR_Write(uint32_t SetValue);
extern uint32_t AES_DIR_Read(void);

/* 数据输出寄存器，当AES计算完成后，可以分四次读出加解密的结果。(AES Data Output)
模式1（加密）：把密文从MSB到LSB分4次读出。
模式2（密钥扩展）：无需使用数据输输出寄存器
模式3和模式4（解密）：把明文从MSB到LSB分4次输出。
MultH模式：运算结果存储在IVR寄存器中，无需读取AES_DOUTR寄存器。 相关函数 */
extern uint32_t AES_DOR_Read(void);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY0_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY1_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY2_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY3_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY4_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY5_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY6_Write(uint32_t SetValue);

/* AES运算秘钥，最长256bit，AESKEY0存放秘钥最低32bit，AESLKEY7存放秘钥最高32bit。(AES Key)
注意：此寄存器软件只可写，不可读；此寄存器内容不受WWDT复位影响 相关函数 */
extern void AES_KEY7_Write(uint32_t SetValue);

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
extern void AES_IVR0_Write(uint32_t SetValue);
extern uint32_t AES_IVR0_Read(void);

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
extern void AES_IVR1_Write(uint32_t SetValue);
extern uint32_t AES_IVR1_Read(void);

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
extern void AES_IVR2_Write(uint32_t SetValue);
extern uint32_t AES_IVR2_Read(void);

/* AES运算128bit初始向量，在MultH模式下保存运算结果。
(AES Initial Vector Registers) 相关函数 */
extern void AES_IVR3_Write(uint32_t SetValue);
extern uint32_t AES_IVR3_Read(void);

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
extern void AES_H0_Write(uint32_t SetValue);
extern uint32_t AES_H0_Read(void);

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
extern void AES_H1_Write(uint32_t SetValue);
extern uint32_t AES_H1_Read(void);

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
extern void AES_H2_Write(uint32_t SetValue);
extern uint32_t AES_H2_Read(void);

/* MultH运算128bit输入H参数 (H Parameter)
H0保存H[31:0]，H3保存H[127:96] 相关函数 */
extern void AES_H3_Write(uint32_t SetValue);
extern uint32_t AES_H3_Read(void);

/* 写错误标志：在计算或输出阶段发生写操作时置位，软件写1清零 相关函数 */
extern void AES_ISR_WRERR_Clr(void);
extern FlagStatus AES_ISR_WRERR_Chk(void);

/* 读错误标志：在计算或输入阶段发生读操作时置位，软件写1清零 相关函数 */
extern void AES_ISR_RDERR_Clr(void);
extern FlagStatus AES_ISR_RDERR_Chk(void);

/* AES计算完成标志，软件写1清零
1：计算完成
0：计算没有完成 相关函数 */
extern void AES_ISR_CCF_Clr(void);
extern FlagStatus AES_ISR_CCF_Chk(void);
//Announce_End


extern void AES_KEY_WriteEx(uint8_t *KeyIn, uint8_t Len);
extern void AES_KEY_ReadEx(uint8_t *KeyOut, uint8_t Len);
extern void AES_IVR_WriteEx(uint8_t *IVRIn);
extern void AES_IVR_ReadEx(uint8_t *IVROut);
extern void AES_DIN_GroupWrite_128BIT(uint8_t *DataIn);
extern void AES_DOUT_GroupRead_128BIT(uint8_t *DataOut);
extern uint8_t AES_GroupWriteAndRead_128BIT(uint8_t *DataIn, uint8_t *DataOut);
extern uint8_t AES_GroupWriteAndRead_128BIT_IVR(uint8_t *DataIn, uint8_t *DataOut);
extern void AES_Init(AES_InitTypeDef* para);
extern void AES_Hx_WriteEx(uint8_t *HxIn, uint8_t Len);

#ifdef __cplusplus
}
#endif

#endif /*__FM33A0XXEV_AES_H */
