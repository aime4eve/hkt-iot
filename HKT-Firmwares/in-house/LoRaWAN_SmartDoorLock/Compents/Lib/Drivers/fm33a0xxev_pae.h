/**
  ******************************************************************************
  * @file    fm33a0xxev_pae.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the PAE firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_PAE_H
#define __FM33A0XXEV_PAE_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
	 
//ECC计算模式说明
#define ECC_STD_DBL				0x00	 //雅克比坐标倍点
#define ECC_STD_ADD				0x01	 //混合坐标点加
#define ECC_ALG1_DBLADD			0x02	 //方案1公式，总是点加倍点
#define ECC_ALG2_ECDBL			0x03	 //方案2公式1，ECDBL
#define ECC_ALG2_ECADDDBL		0x04	 //方案2公式2，ECADDDBL
#define ECC_ALG2_YRECOVER		0x05	 //方案2公式3，恢复射影Y坐标
	 

//Macro_START
//2020-03-12-14-15-07
//FM33A0XX_Driver_Gen_V1.4

#define	PAE_CSR_DATA_TYPE_Pos	8	/* 输入数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48 */
#define	PAE_CSR_DATA_TYPE_Msk	(0x3U << PAE_CSR_DATA_TYPE_Pos)
#define	PAE_CSR_DATA_TYPE_NONE	(0x0U << PAE_CSR_DATA_TYPE_Pos)	/* 不交换 */
#define	PAE_CSR_DATA_TYPE_HALFWORD	(0x1U << PAE_CSR_DATA_TYPE_Pos)	/* half-word交换 */
#define	PAE_CSR_DATA_TYPE_BYTE	(0x2U << PAE_CSR_DATA_TYPE_Pos)	/* byte交换 */
#define	PAE_CSR_DATA_TYPE_BIT	(0x3U << PAE_CSR_DATA_TYPE_Pos)	/* bit交换 */

#define	PAE_CSR_START_Pos	7	/* 运算启动位，软件写1启动计算，计算结束后硬件自动清零 */
#define	PAE_CSR_START_Msk	(0x1U << PAE_CSR_START_Pos)

#define	PAE_CSR_RUN_MODE_Pos	4	/* 模块工作模式：
00：模式0，单次模运算，可配置4Block/16Block
01：模式1，单次点运算，固定16Block
10：模式2，32bit密钥连续RSA运算，固定4Block
11：模式3，32bit密钥连续ECC运算，固定16Block */
#define	PAE_CSR_RUN_MODE_Msk	(0x3U << PAE_CSR_RUN_MODE_Pos)
#define	PAE_CSR_RUN_MODE_MOD	(0x0U << PAE_CSR_RUN_MODE_Pos)
#define	PAE_CSR_RUN_MODE_DOT	(0x1U << PAE_CSR_RUN_MODE_Pos)	/* 模式0，单次模运算，可配置4Block/16Block */
#define	PAE_CSR_RUN_MODE_RSA	(0x2U << PAE_CSR_RUN_MODE_Pos)	/* 模式1，单次点运算，固定16Block */
#define	PAE_CSR_RUN_MODE_ECC	(0x3U << PAE_CSR_RUN_MODE_Pos)	/* 模式2，32bit密钥连续RSA运算，固定4Block */

#define	PAE_CSR_SOFT_RST_Pos	3	/* PAE软件复位，软件写1复位PAE内部主要寄存器（模长寄存器PAE_MLR和模参寄存器PAE_MPR不受影响），复位结束后硬件自动清零
注意：软件写SOFT_RST之后，需插入至少一条NOP指令后，再写入PAE其他寄存器；在SOFT_RST操作后立即写PAE寄存器可能写入失败。 */
#define	PAE_CSR_SOFT_RST_Msk	(0x1U << PAE_CSR_SOFT_RST_Pos)
	/* 软件写SOFT_RST之后，需插入至少一条NOP指令后，再写入PAE其他寄存器；在SOFT_RST操作后立即写PAE寄存器可能写入失败。 */

#define	PAE_CSR_PAEIE_Pos	2	/* PAE中断使能
1：使能中断输出
0：禁止中断输出 */
#define	PAE_CSR_PAEIE_Msk	(0x1U << PAE_CSR_PAEIE_Pos)
#define	PAE_CSR_PAEIE_ENABLE	(0x1U << PAE_CSR_PAEIE_Pos)	/* 使能中断输出 */
#define	PAE_CSR_PAEIE_DISABLE	(0x0U << PAE_CSR_PAEIE_Pos)	/* 禁止中断输出 */

#define	PAE_CSR_PAEIF_Pos	1	/* PAE中断标志寄存器 */
#define	PAE_CSR_PAEIF_Msk	(0x1U << PAE_CSR_PAEIF_Pos)

#define	PAE_CSR_BUSY_Pos	0	/* PAE工作标志
1：PAE正在运算中
0：PAE空闲 */
#define	PAE_CSR_BUSY_Msk	(0x1U << PAE_CSR_BUSY_Pos)

#define	PAE_MLR_PAE_MLR_Pos	0	/* 在RAM配置为4 Block时，模长最大为2048bit：
模长=（MLR[5:0]+1）*32位。
在RAM配置为16 Block时，模长最大为512bit，MLR只有低4bit有效：
模长=（MLR[3:0]+1）*32位。 */
#define	PAE_MLR_PAE_MLR_Msk	(0x3fU << PAE_MLR_PAE_MLR_Pos)

#define	PAE_MPR_PAE_MPR_Pos	0	/* 32bit模参寄存器，只可写，不可读 */
#define	PAE_MPR_PAE_MPR_Msk	(0xffffffffU << PAE_MPR_PAE_MPR_Pos)

#define	PAE_M0CFG_BLKCFG_Pos	15	/* RAM Block配置位
1：算法RAM配置为16 Block
0：算法RAM配置位4 Block */
#define	PAE_M0CFG_BLKCFG_Msk	(0x1U << PAE_M0CFG_BLKCFG_Pos)
#define	PAE_M0CFG_BLKCFG_16BLOCK	(0x1U << PAE_M0CFG_BLKCFG_Pos)	/* 算法RAM配置为16 Block */
#define	PAE_M0CFG_BLKCFG_4BLOCK	(0x0U << PAE_M0CFG_BLKCFG_Pos)	/* 算法RAM配置位4 Block */

#define	PAE_M0CFG_INS_M0_Pos	12	/* 单次模运算指令：
00：蒙哥马利模乘
01：蒙哥马利模加
10：RFU
11：蒙哥马利模减 */
#define	PAE_M0CFG_INS_M0_Msk	(0x3U << PAE_M0CFG_INS_M0_Pos)
#define	PAE_M0CFG_INS_M0_MUL	(0x0U << PAE_M0CFG_INS_M0_Pos)
#define	PAE_M0CFG_INS_M0_ADD	(0x1U << PAE_M0CFG_INS_M0_Pos)	/* 蒙哥马利模乘 */
#define	PAE_M0CFG_INS_M0_SUB	(0x3U << PAE_M0CFG_INS_M0_Pos)	/* 蒙哥马利模加 */

#define	PAE_M0CFG_RES_BLK_Pos	8	/* 配置运算结果所在Block
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 */
#define	PAE_M0CFG_RES_BLK_Msk	(0xfU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK0	(0x0U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK1	(0x1U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK2	(0x2U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK3	(0x3U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK4	(0x4U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK5	(0x5U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK6	(0x6U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK7	(0x7U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK8	(0x8U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK9	(0x9U << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK10	(0xAU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK11	(0xBU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK12	(0xCU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK13	(0xDU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK14	(0xEU << PAE_M0CFG_RES_BLK_Pos)
#define	PAE_M0CFG_RES_BLK_BLOCK15	(0xFU << PAE_M0CFG_RES_BLK_Pos)






#define	PAE_M0CFG_OP1_BLK_Pos	4	/* 操作数1地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 */
#define	PAE_M0CFG_OP1_BLK_Msk	(0xfU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK0	(0x0U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK1	(0x1U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK2	(0x2U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK3	(0x3U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK4	(0x4U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK5	(0x5U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK6	(0x6U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK7	(0x7U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK8	(0x8U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK9	(0x9U << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK10	(0xAU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK11	(0xBU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK12	(0xCU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK13	(0xDU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK14	(0xEU << PAE_M0CFG_OP1_BLK_Pos)
#define	PAE_M0CFG_OP1_BLK_BLOCK15	(0xFU << PAE_M0CFG_OP1_BLK_Pos)




#define	PAE_M0CFG_OP2_BLK_Pos	0	/* 操作数2地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 */
#define	PAE_M0CFG_OP2_BLK_Msk	(0xfU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK0	(0x0U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK1	(0x1U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK2	(0x2U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK3	(0x3U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK4	(0x4U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK5	(0x5U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK6	(0x6U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK7	(0x7U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK8	(0x8U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK9	(0x9U << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK10	(0xAU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK11	(0xBU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK12	(0xCU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK13	(0xDU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK14	(0xEU << PAE_M0CFG_OP2_BLK_Pos)
#define	PAE_M0CFG_OP2_BLK_BLOCK15	(0xFU << PAE_M0CFG_OP2_BLK_Pos)


#define	PAE_M1CFG_INS_M1_Pos	4	/* 单次点运算指令：
000：Jacobin坐标倍点运算
001：混合坐标点加运算
010：RFU
011：蒙哥马利点乘倍点运算（ECDBL）
100：蒙哥马利点乘倍点点加运算（ECADDDBL）
101：蒙哥马利点乘Y坐标恢复运算（ECYRecover）
110：RFU
111：RFU */
#define	PAE_M1CFG_INS_M1_Msk	(0x7U << PAE_M1CFG_INS_M1_Pos)
#define	PAE_M1CFG_INS_M1_JACOBIN	(0x0U << PAE_M1CFG_INS_M1_Pos)	/* Jacobin坐标倍点运算 */
#define	PAE_M1CFG_INS_M1_MIXADD	(0x1U << PAE_M1CFG_INS_M1_Pos)	/* 混合坐标点加运算 */
#define	PAE_M1CFG_INS_M1_ECDBL	(0x3U << PAE_M1CFG_INS_M1_Pos)	/* 蒙哥马利点乘倍点运算（ECDBL） */
#define	PAE_M1CFG_INS_M1_ECADDDBL	(0x4U << PAE_M1CFG_INS_M1_Pos)	/* 蒙哥马利点乘倍点点加运算（ECADDDBL） */
#define	PAE_M1CFG_INS_M1_ECYRECOVER	(0x5U << PAE_M1CFG_INS_M1_Pos)	/* 蒙哥马利点乘Y坐标恢复运算（ECYRecover） */

#define	PAE_M1CFG_AEN3_M1_Pos	1	/* 参数A指示信号
1：A等于-3
0：A不等于-3 */
#define	PAE_M1CFG_AEN3_M1_Msk	(0x1U << PAE_M1CFG_AEN3_M1_Pos)
#define	PAE_M1CFG_AEN3_M1_NE3	(0x1U << PAE_M1CFG_AEN3_M1_Pos)	/* A等于-3 */
#define	PAE_M1CFG_AEN3_M1_EQ3	(0x0U << PAE_M1CFG_AEN3_M1_Pos)	/* A不等于-3 */

#define	PAE_M1CFG_BIT_VALUE_M1_Pos	0	/* 当前计算密钥位指示标志
1：当前位为1
0：当前位为0 */
#define	PAE_M1CFG_BIT_VALUE_M1_Msk	(0x1U << PAE_M1CFG_BIT_VALUE_M1_Pos)
#define	PAE_M1CFG_BIT_VALUE_M1_VALUE1	(0x1U << PAE_M1CFG_BIT_VALUE_M1_Pos)	/* 当前位为1 */
#define	PAE_M1CFG_BIT_VALUE_M1_VALUE0	(0x0U << PAE_M1CFG_BIT_VALUE_M1_Pos)	/* 当前位为0 */

#define	PAE_M2CFG_ALWAYS_MULT_Pos	4	/* 虚模乘控制位
1：当前密钥位为0时同样进行模乘操作
0：当前密钥位为0时不进行模乘操作 */
#define	PAE_M2CFG_ALWAYS_MULT_Msk	(0x1U << PAE_M2CFG_ALWAYS_MULT_Pos)
#define	PAE_M2CFG_ALWAYS_MULT_ALWAYS	(0x1U << PAE_M2CFG_ALWAYS_MULT_Pos)	/* 当前密钥位为0时同样进行模乘操作 */
#define	PAE_M2CFG_ALWAYS_MULT_NONE	(0x0U << PAE_M2CFG_ALWAYS_MULT_Pos)	/* 当前密钥位为0时不进行模乘操作 */

#define	PAE_M2CFG_FBO_M2_Pos	0	/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit */
#define	PAE_M2CFG_FBO_M2_Msk	(0x1U << PAE_M2CFG_FBO_M2_Pos)
#define	PAE_M2CFG_FBO_M2_FB1	(0x1U << PAE_M2CFG_FBO_M2_Pos)	/* 已找到为1的bit */
#define	PAE_M2CFG_FBO_M2_NFB1	(0x0U << PAE_M2CFG_FBO_M2_Pos)	/* 未找到为1的bit */

#define	PAE_M3CFG_AEN3_M3_Pos	4	/* 参数A指示信号
1：A等于-3
0：A不等于-3 */
#define	PAE_M3CFG_AEN3_M3_Msk	(0x1U << PAE_M3CFG_AEN3_M3_Pos)
#define	PAE_M3CFG_AEN3_M3_EQ3	(0x1U << PAE_M3CFG_AEN3_M3_Pos)	/* A等于-3 */
#define	PAE_M3CFG_AEN3_M3_NE3	(0x0U << PAE_M3CFG_AEN3_M3_Pos)	/* A不等于-3 */

#define	PAE_M3CFG_FBO_M3_Pos	0	/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit */
#define	PAE_M3CFG_FBO_M3_Msk	(0x1U << PAE_M3CFG_FBO_M3_Pos)
#define	PAE_M3CFG_FBO_M3_FB1	(0x1U << PAE_M3CFG_FBO_M3_Pos)	/* 已找到为1的bit */
#define	PAE_M3CFG_FBO_M3_NFB1	(0x0U << PAE_M3CFG_FBO_M3_Pos)	/* 未找到为1的bit */

#define	PAE_WORD_PAE_WORD_Pos	0	/* 32bit数据寄存器，只可写，不可读；仅在连续RSA运算和连续ECC运算模式下有效。 */
#define	PAE_WORD_PAE_WORD_Msk	(0xffffffffU << PAE_WORD_PAE_WORD_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void PAE_Deinit(void);

/* 输入数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48 相关函数 */
extern void PAE_CSR_DATA_TYPE_Set(uint32_t SetValue);
extern uint32_t PAE_CSR_DATA_TYPE_Get(void);

/* 运算启动位，软件写1启动计算，计算结束后硬件自动清零 相关函数 */
extern void PAE_CSR_START_Setable(FunState NewState);

/* 模块工作模式：
00：模式0，单次模运算，可配置4Block/16Block
01：模式1，单次点运算，固定16Block
10：模式2，32bit密钥连续RSA运算，固定4Block
11：模式3，32bit密钥连续ECC运算，固定16Block 相关函数 */
extern void PAE_CSR_RUN_MODE_Set(uint32_t SetValue);
extern uint32_t PAE_CSR_RUN_MODE_Get(void);

/* PAE软件复位，软件写1复位PAE内部主要寄存器（模长寄存器PAE_MLR和模参寄存器PAE_MPR不受影响），复位结束后硬件自动清零
注意：软件写SOFT_RST之后，需插入至少一条NOP指令后，再写入PAE其他寄存器；在SOFT_RST操作后立即写PAE寄存器可能写入失败。 相关函数 */
extern void PAE_CSR_SOFT_RST_Setable(FunState NewState);

/* PAE中断使能
1：使能中断输出
0：禁止中断输出 相关函数 */
extern void PAE_CSR_PAEIE_Setable(FunState NewState);
extern FunState PAE_CSR_PAEIE_Getable(void);

/* PAE中断标志寄存器 相关函数 */
extern void PAE_CSR_PAEIF_Clr(void);
extern FlagStatus PAE_CSR_PAEIF_Chk(void);

/* PAE工作标志
1：PAE正在运算中
0：PAE空闲 相关函数 */
extern FlagStatus PAE_CSR_BUSY_Chk(void);

/* 在RAM配置为4 Block时，模长最大为2048bit：
模长=（MLR[5:0]+1）*32位。
在RAM配置为16 Block时，模长最大为512bit，MLR只有低4bit有效：
模长=（MLR[3:0]+1）*32位。 相关函数 */
extern void PAE_MLR_Write(uint32_t SetValue);
extern uint32_t PAE_MLR_Read(void);

/* 32bit模参寄存器，只可写，不可读 相关函数 */
extern void PAE_MPR_Write(uint32_t SetValue);

/* RAM Block配置位
1：算法RAM配置为16 Block
0：算法RAM配置位4 Block 相关函数 */
extern void PAE_M0CFG_BLKCFG_Set(uint32_t SetValue);
extern uint32_t PAE_M0CFG_BLKCFG_Get(void);

/* 单次模运算指令：
00：蒙哥马利模乘
01：蒙哥马利模加
10：RFU
11：蒙哥马利模减 相关函数 */
extern void PAE_M0CFG_INS_M0_Set(uint32_t SetValue);
extern uint32_t PAE_M0CFG_INS_M0_Get(void);

/* 配置运算结果所在Block
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
extern void PAE_M0CFG_RES_BLK_Set(uint32_t SetValue);
extern uint32_t PAE_M0CFG_RES_BLK_Get(void);

/* 操作数1地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
extern void PAE_M0CFG_OP1_BLK_Set(uint32_t SetValue);
extern uint32_t PAE_M0CFG_OP1_BLK_Get(void);

/* 操作数2地址：
当BLK_CFG==1，4bit有效，表示Block0~Block15
当BLK_CFG==0，低2bit有效，表示Block0~Block3 相关函数 */
extern void PAE_M0CFG_OP2_BLK_Set(uint32_t SetValue);
extern uint32_t PAE_M0CFG_OP2_BLK_Get(void);

/* 单次点运算指令：
000：Jacobin坐标倍点运算
001：混合坐标点加运算
010：RFU
011：蒙哥马利点乘倍点运算（ECDBL）
100：蒙哥马利点乘倍点点加运算（ECADDDBL）
101：蒙哥马利点乘Y坐标恢复运算（ECYRecover）
110：RFU
111：RFU 相关函数 */
extern void PAE_M1CFG_INS_M1_Set(uint32_t SetValue);
extern uint32_t PAE_M1CFG_INS_M1_Get(void);

/* 参数A指示信号
1：A等于-3
0：A不等于-3 相关函数 */
extern void PAE_M1CFG_AEN3_M1_Set(uint32_t SetValue);
extern uint32_t PAE_M1CFG_AEN3_M1_Get(void);

/* 当前计算密钥位指示标志
1：当前位为1
0：当前位为0 相关函数 */
extern void PAE_M1CFG_BIT_VALUE_M1_Set(uint32_t SetValue);
extern uint32_t PAE_M1CFG_BIT_VALUE_M1_Get(uint32_t SetValue);
/* 虚模乘控制位
1：当前密钥位为0时同样进行模乘操作
0：当前密钥位为0时不进行模乘操作 相关函数 */
extern void PAE_M2CFG_ALWAYS_MULT_Set(uint32_t SetValue);
extern uint32_t PAE_M2CFG_ALWAYS_MULT_Get(void);

/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit 相关函数 */
extern void PAE_M2CFG_FBO_M2_Set(uint32_t SetValue);
extern uint32_t PAE_M2CFG_FBO_M2_Get(void);

/* 参数A指示信号
1：A等于-3
0：A不等于-3 相关函数 */
extern void PAE_M3CFG_AEN3_M3_Set(uint32_t SetValue);
extern uint32_t PAE_M3CFG_AEN3_M3_Get(void);

/* 密钥位指示信号，硬件自动置位，软件清零
1：已找到为1的bit
0：未找到为1的bit 相关函数 */
extern void PAE_M3CFG_FBO_M3_Set(uint32_t SetValue);
extern uint32_t PAE_M3CFG_FBO_M3_Get(void);

/* 32bit数据寄存器，只可写，不可读；仅在连续RSA运算和连续ECC运算模式下有效。 相关函数 */
extern void PAE_WORD_Write(uint32_t SetValue);
//Announce_End


#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_PAE_H */
