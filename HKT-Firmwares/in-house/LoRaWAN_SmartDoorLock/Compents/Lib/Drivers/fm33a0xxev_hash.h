/**
  ******************************************************************************
  * @file    fm33a0xxev_hash.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the HASH firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_HASH_H
#define __FM33A0XXEV_HASH_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
   
#define	HASH_CSR_START_Pos	7	/* HASH运算启动寄存器，软件写1启动运算，运算结束后硬件自动清零 */
#define	HASH_CSR_START_Msk	(0x1U << HASH_CSR_START_Pos)

#define	HASH_CSR_BUSY_Pos	6	/* HASH运算状态，硬件置位，软件只读 */
#define	HASH_CSR_BUSY_Msk	(0x1U << HASH_CSR_BUSY_Pos)

#define	HASH_CSR_HASHSEL_Pos	1	/* SHA算法选择 */
#define	HASH_CSR_HASHSEL_Msk	(0x1U << HASH_CSR_HASHSEL_Pos)
#define	HASH_CSR_HASHSEL_SHA256	(0x0U << HASH_CSR_HASHSEL_Pos)	/* 0：SHA256 */
#define	HASH_CSR_HASHSEL_SHA1	(0x1U << HASH_CSR_HASHSEL_Pos)	/* 1：SHA1 */

#define	HASH_DTR_DATA_TYPE_Pos	0	/* 输入或输出数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48
 */
#define	HASH_DTR_DATA_TYPE_Msk	(0x3U << HASH_DTR_DATA_TYPE_Pos)
#define	HASH_DTR_DATA_TYPE_NONE	(0x0U << HASH_DTR_DATA_TYPE_Pos)	/* 不交换 */
#define	HASH_DTR_DATA_TYPE_HALFWORD	(0x1U << HASH_DTR_DATA_TYPE_Pos)	/* half-word交换 */
#define	HASH_DTR_DATA_TYPE_BYTE	(0x2U << HASH_DTR_DATA_TYPE_Pos)	/* byte交换 */
#define	HASH_DTR_DATA_TYPE_BIT	(0x3U << HASH_DTR_DATA_TYPE_Pos)	/* bit交换 */
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void HASH_Deinit(void);

/* HASH运算启动寄存器，软件写1启动运算，运算结束后硬件自动清零 相关函数 */
extern void HASH_CSR_START_Setable(FunState NewState);
extern FunState HASH_CSR_START_Getable(void);


/* SHA算法选择 相关函数 */
extern void HASH_CSR_HASHSEL_Set(uint32_t SetValue);
extern uint32_t HASH_CSR_HASHSEL_Get(void);

/* 输入或输出数据顺序交换控制
00：不交换
01：half-word交换
10：byte交换
11：bit交换
此寄存器用于对输入32bit数据的位序进行调整；比如输入为0x1234_5678，half-word交换后为0x5678_1234，byte交换后为0x7856_3412，bit交换后为0x1E6A_2C48
 相关函数 */
extern void HASH_DTR_DATA_TYPE_Set(uint32_t SetValue);
extern uint32_t HASH_DTR_DATA_TYPE_Get(void);
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_HASH_H */
