/**
  ******************************************************************************
  * @file    fm33a0xxev_dma.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_dma.h"
#include "fm33a0xxev_rmu.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup DMA 
  * @brief DMA driver modules
  * @{
  */ 

/* DMA错误地址中断使能
1：允许错误地址中断
0：禁止错误地址中断 相关函数 */
void DMA_GCR_DMA_ADDRERR_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DMA->GCR |= (DMA_GCR_DMA_ADDRERR_EN_Msk);
	}
	else
	{
		DMA->GCR &= ~(DMA_GCR_DMA_ADDRERR_EN_Msk);
	}
}

FunState DMA_GCR_DMA_ADDRERR_EN_Getable(void)
{
	if (DMA->GCR & (DMA_GCR_DMA_ADDRERR_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* DMA全局使能
1：DMA使能
0：DMA关闭 相关函数 */
void DMA_GCR_DMAEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DMA->GCR |= (DMA_GCR_DMAEN_Msk);
	}
	else
	{
		DMA->GCR &= ~(DMA_GCR_DMAEN_Msk);
	}
}

FunState DMA_GCR_DMAEN_Getable(void)
{
	if (DMA->GCR & (DMA_GCR_DMAEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channelx传输长度，1-8192次传输 相关函数 */
void DMA_CHxCR_CHxTSIZE_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;
	
	SetValue <<= DMA_CHxCR_CHxTSIZE_Pos;//手工添加
	
	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_CHxTSIZE_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_CHxTSIZE_Msk);
	*pREG = tmpreg;
}

uint32_t DMA_CHxCR_CHxTSIZE_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_CHxTSIZE_Msk);
}

/* Channelx优先级
00：Low
01：Medium
10：High
11：Very High 相关函数 */
void DMA_CHxCR_CHxPRI_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;

	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_CHxPRI_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_CHxPRI_Msk);
	*pREG = tmpreg;
}

uint32_t DMA_CHxCR_CHxPRI_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_CHxPRI_Msk);
}

/* RAM地址增减设置
1：RAM地址递增
0：RAM地址递减 相关函数 */
void DMA_CHxCR_CHxINC_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;

	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_CHxINC_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_CHxINC_Msk);
	*pREG = tmpreg;
}

uint32_t DMA_CHxCR_CHxINC_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_CHxINC_Msk);
}

/* Channelx外设请求映射
每个通道可以接受8个外设请求，外设请求的映射参见23.6.1DMA请求映射 相关函数 */
void DMA_CHxCR_CHxSSEL_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	
	
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;

	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_CHxSSEL_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_CHxSSEL_Msk);
	*pREG = tmpreg;
}

uint32_t DMA_CHxCR_CHxSSEL_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_CHxSSEL_Msk);
}

/* 循环缓冲模式下自动更新传输参数，本轮传输完成后将shadow寄存器内容复制到控制寄存器
0：禁止自动更新
1：使能自动更新
注意：仅通道0~6支持此功能，通道7~10无此寄存器 相关函数 */
void DMA_CHxCR_CIRC_UPD_Setable(DMA_CH_Type CHx, FunState NewState)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	if (NewState == ENABLE)
	{
		*pREG |= (DMA_CHxCR_CIRC_UPD_Msk);
	}
	else
	{
		*pREG &= ~(DMA_CHxCR_CIRC_UPD_Msk);
	}
}

FunState DMA_CHxCR_CIRC_UPD_Getable(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);;
	if (*pREG & (DMA_CHxCR_CIRC_UPD_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 通道传输方向
0：从外设读取数据写入RAM
1：从RAM读取数据写入外设 相关函数 */
void DMA_CHxCR_DIR_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;
	
	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_DIR_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_DIR_Msk);
	*pREG = tmpreg;

}

uint32_t DMA_CHxCR_DIR_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_DIR_Msk);
}

/* 传输带宽设置
00：字节，8bit
01：半字，16bit
10：字，32bit
11：RFU 相关函数 */
void DMA_CHxCR_BDW_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);//DMA->CH0CTRL;
	
	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCR_BDW_Msk);
	tmpreg |= (SetValue&DMA_CHxCR_BDW_Msk);
	*pREG = tmpreg;
}

uint32_t DMA_CHxCR_BDW_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	return (*pREG & DMA_CHxCR_BDW_Msk);
}

/* 循环缓冲模式
0：关闭循环模式
1：使能循环模式 相关函数 */
void DMA_CHxCR_CIRC_Setable(DMA_CH_Type CHx, FunState NewState)
{
	
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	if (NewState == ENABLE)
	{
		*pREG |= (DMA_CHxCR_CIRC_Msk);
	}
	else
	{
		*pREG &= ~(DMA_CHxCR_CIRC_Msk);
	}
	
}

FunState DMA_CHxCR_CIRC_Getable(DMA_CH_Type CHx)
{
	
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);;
	if (*pREG & (DMA_CHxCR_CIRC_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channelx传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 相关函数 */
void DMA_CHxCR_CHxFTIE_Setable(DMA_CH_Type CHx, FunState NewState)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	if (NewState == ENABLE)
	{
		*pREG |= (DMA_CHxCR_CHxFTIE_Msk);
	}
	else
	{
		*pREG &= ~(DMA_CHxCR_CHxFTIE_Msk);
	}
}

FunState DMA_CHxCR_CHxFTIE_Getable(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);;
	if (*pREG & (DMA_CHxCR_CHxFTIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channelx半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 相关函数 */
void DMA_CHxCR_CHxHTIE_Setable(DMA_CH_Type CHx, FunState NewState)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	if (NewState == ENABLE)
	{
		*pREG |= (DMA_CHxCR_CHxHTIE_Msk);
	}
	else
	{
		*pREG &= ~(DMA_CHxCR_CHxHTIE_Msk);
	}
	
}

FunState DMA_CHxCR_CHxHTIE_Getable(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);;
	if (*pREG & (DMA_CHxCR_CHxHTIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channelx使能
1：启动通道0
0：关闭通道0 相关函数 */
void DMA_CHxCR_ChxEN_Setable(DMA_CH_Type CHx, FunState NewState)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);
	if (NewState == ENABLE)
	{
		*pREG |= (DMA_CHxCR_ChxEN_Msk);
		
	}
	else
	{
		*pREG &= ~(DMA_CHxCR_ChxEN_Msk);
	}

}

FunState DMA_CHxCR_ChxEN_Getable(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x04 + CHx*8);;
	if (*pREG & (DMA_CHxCR_ChxEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channelx存储器指针地址，DMA传输启动前软件向此寄存器写入存储器目标地址。
当指针指向空地址时，DMA访问将触发hardfault
当指针指向Flash时，禁止向Flash写入数据。
软件可以查询当前DMA传输的目标存储器地址。

注意：此指针禁止指向0x00080000~0x1FFFFFFF地址，这段地址为flash保留信息区，指向这段地址可能在DMA访问中导致不可预计的结果 相关函数 */
void DMA_CHxMAR_Write(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 8 + CHx*8);
	*pREG = SetValue;
}

uint32_t DMA_CHxMAR_Read(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 8 + CHx*8);
	return (*pREG);
}

/* Channel11传输长度，1-8192次传输，仅在Flash->RAM传输时有效，RAM->Flash传输为固定长度64次传输 相关函数 */
void DMA_CH11CR_CH11TSIZE_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
    
    SetValue <<= DMA_CH11CR_CH11TSIZE_Pos;
	
    tmpreg = DMA->CH11CR;
	tmpreg &= ~(DMA_CH11CR_CH11TSIZE_Msk);
	tmpreg |= (SetValue & DMA_CH11CR_CH11TSIZE_Msk);
	DMA->CH11CR = tmpreg;
}

uint32_t DMA_CH11CR_CH11TSIZE_Get(void)
{
	return (DMA->CH11CR & DMA_CH11CR_CH11TSIZE_Msk);
}

/* Channel11优先级
00：Low
01：Medium
10：High
11：Very High 相关函数 */
void DMA_CH11CR_CH11PRI_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = DMA->CH11CR;
	tmpreg &= ~(DMA_CH11CR_CH11PRI_Msk);
	tmpreg |= (SetValue & DMA_CH11CR_CH11PRI_Msk);
	DMA->CH11CR = tmpreg;
}

uint32_t DMA_CH11CR_CH11PRI_Get(void)
{
	return (DMA->CH11CR & DMA_CH11CR_CH11PRI_Msk);
}

/* Channel11传输方向
1：Flash->RAM传输
0：RAM->Flash传输 相关函数 */
void DMA_CH11CR_CH11DIR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = DMA->CH11CR;
	tmpreg &= ~(DMA_CH11CR_CH11DIR_Msk);
	tmpreg |= (SetValue & DMA_CH11CR_CH11DIR_Msk);
	DMA->CH11CR = tmpreg;
}

uint32_t DMA_CH11CR_CH11DIR_Get(void)
{
	return (DMA->CH11CR & DMA_CH11CR_CH11DIR_Msk);
}

/* Channel11 RAM地址增减设置，仅在Flash->RAM传输中有效
1：RAM地址递增
0：RAM地址递减 相关函数 */
void DMA_CH11CR_CH11RI_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = DMA->CH11CR;
	tmpreg &= ~(DMA_CH11CR_CH11RI_Msk);
	tmpreg |= (SetValue & DMA_CH11CR_CH11RI_Msk);
	DMA->CH11CR = tmpreg;
}

uint32_t DMA_CH11CR_CH11RI_Get(void)
{
	return (DMA->CH11CR & DMA_CH11CR_CH11RI_Msk);
}

/* Channel11 Flash地址增减设置，仅在Flash->RAM传输中有效
1：Flash地址递增
0：Flash地址递减 相关函数 */
void DMA_CH11CR_CH11FI_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = DMA->CH11CR;
	tmpreg &= ~(DMA_CH11CR_CH11FI_Msk);
	tmpreg |= (SetValue & DMA_CH11CR_CH11FI_Msk);
	DMA->CH11CR = tmpreg;
}

uint32_t DMA_CH11CR_CH11FI_Get(void)
{
	return (DMA->CH11CR & DMA_CH11CR_CH11FI_Msk);
}

/* Channel11传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 相关函数 */
void DMA_CH11CR_CH11FTIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DMA->CH11CR |= (DMA_CH11CR_CH11FTIE_Msk);
	}
	else
	{
		DMA->CH11CR &= ~(DMA_CH11CR_CH11FTIE_Msk);
	}
}

FunState DMA_CH11CR_CH11FTIE_Getable(void)
{
	if (DMA->CH11CR & (DMA_CH11CR_CH11FTIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channel11半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 相关函数 */
void DMA_CH11CR_CH11HTIE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DMA->CH11CR |= (DMA_CH11CR_CH11HTIE_Msk);
	}
	else
	{
		DMA->CH11CR &= ~(DMA_CH11CR_CH11HTIE_Msk);
	}
}

FunState DMA_CH11CR_CH11HTIE_Getable(void)
{
	if (DMA->CH11CR & (DMA_CH11CR_CH11HTIE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channel11使能
1：启动通道0
0：关闭通道0 相关函数 */
void DMA_CH11CR_CH11EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		DMA->CH11CR |= (DMA_CH11CR_CH11EN_Msk);
	}
	else
	{
		DMA->CH11CR &= ~(DMA_CH11CR_CH11EN_Msk);
	}
}

FunState DMA_CH11CR_CH11EN_Getable(void)
{
	if (DMA->CH11CR & (DMA_CH11CR_CH11EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* Channel11 Flash指针地址，DMA传输启动前软件向此寄存器写入Flash目标地址，DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标Flash地址
此寄存器低位（bit5-0）仅在Flash->RAM传输中有效，RAM->Flash传输中默认对齐Flash的half-sector起始地址 相关函数 */
void DMA_CH11FAR_Write(uint32_t SetValue)
{
	DMA->CH11FAR = ((SetValue >> 2) & DMA_CH11FAR_CH11FLSAD_Msk);
}

uint32_t DMA_CH11FAR_Read(void)
{
	return (DMA->CH11FAR & DMA_CH11FAR_CH11FLSAD_Msk);
}

/* Channel7 RAM字指针地址，DMA传输启动前软件向此寄存器写入RAM目标地址（word地址），DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标RAM地址 相关函数 */
void DMA_CH11RAR_Write(uint32_t SetValue)
{
	DMA->CH11RAR = ((SetValue >> 2) & DMA_CH11RAR_CH7RAMAD_Msk);
}

uint32_t DMA_CH11RAR_Read(void)
{
	return (DMA->CH11RAR & DMA_CH11RAR_CH7RAMAD_Msk);
}

/* DMA传输地址错误标志，当存储器指针超过RAM和Flash合法地址范围时置位 相关函数 */
void DMA_ISR_DMA_ADDRERR_Clr(void)
{
	DMA->ISR = DMA_ISR_DMA_ADDRERR_Msk;
}

FlagStatus DMA_ISR_DMA_ADDRERR_Chk(void)
{
	if (DMA->ISR & DMA_ISR_DMA_ADDRERR_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* DMA通道x传输完成标志，硬件置位，软件写1清零
1：对应通道传输完成
0：对应通道传输未完成 相关函数 */
void DMA_ISR_DMACHFT_Clr(DMA_CH_Type CHx)
{
	DMA->ISR = ((1 << DMA_ISR_DMACHFT_Pos) << CHx);
}

FlagStatus DMA_ISR_DMACHFT_Chk(DMA_CH_Type CHx)
{
	if (DMA->ISR & ((1 << DMA_ISR_DMACHFT_Pos) << CHx))
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* DMA通道x传输半程标志，硬件置位，软件写1清零 相关函数 */
void DMA_ISR_DMACHHT_Clr(DMA_CH_Type CHx)
{
	DMA->ISR = (1 << DMA_ISR_DMACHHT_Pos) << CHx;
}

FlagStatus DMA_ISR_DMACHHT_Chk(DMA_CH_Type CHx)
{
	if (DMA->ISR & ((1 << DMA_ISR_DMACHHT_Pos) << CHx))
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* Channelx传输长度shadow寄存器；循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 相关函数 */
void DMA_CHxCSR_CHxTSIZE_SDW_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x100 + CHx*8);//DMA->CH0CTRL;
	SetValue <<= DMA_CHxCSR_CHxTSIZE_SDW_Pos;//手工添加	
	
	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCSR_CHxTSIZE_SDW_Msk);
	tmpreg |= (SetValue&DMA_CHxCSR_CHxTSIZE_SDW_Msk);
	*pREG = tmpreg;

}

uint32_t DMA_CHxCSR_CHxTSIZE_SDW_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x100 + CHx*8);
	return (*pREG & DMA_CHxCSR_CHxTSIZE_SDW_Msk);
}

/* RAM地址增减设置shadow寄存器，循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 相关函数 */
void DMA_CHxCSR_CHxINC_SDW_Set(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	uint32_t tmpreg;    
	pREG = (uint32_t *)(DMA_BASE + 0x100 + CHx*8);//DMA->CH0CTRL;
	
	tmpreg = *pREG;
	tmpreg &= ~(DMA_CHxCSR_CHxINC_SDW_Msk);
	tmpreg |= (SetValue&DMA_CHxCSR_CHxINC_SDW_Msk);
	*pREG = tmpreg;

}

uint32_t DMA_CHxCSR_CHxINC_SDW_Get(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x100 + CHx*8);
	return (*pREG & DMA_CHxCSR_CHxINC_SDW_Msk);
}

/* Channelx存储器指针地址影子寄存器。
循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxMEMAD中。 相关函数 */
void DMA_CHxMASR_Write(DMA_CH_Type CHx, uint32_t SetValue)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x104 + CHx*8);
	*pREG = SetValue;
}

uint32_t DMA_CHxMASR_Read(DMA_CH_Type CHx)
{
	uint32_t *pREG;
	pREG = (uint32_t *)(DMA_BASE + 0x104 + CHx*8);
	return (*pREG);
}


void DMA_DeInit(void)
{
    RMU_PRSTEN_Write(0x13579BDF);
    RMU_AHBRST_DMARST_Setable(ENABLE);
    RMU_AHBRST_DMARST_Setable(DISABLE);
    RMU_PRSTEN_Write(0x00000000);
}

void DMA_Init(DMA_InitTypeDef *para)
{
    if (DMA_CH11 == para->CHx)
    {
		DMA_CHxCR_ChxEN_Setable(para->CHx, DISABLE);    	// 通道关闭使能
        DMA_CH11CR_CH11TSIZE_Set(para->CHxTSIZE);   		// 通道传输长度
        DMA_CH11CR_CH11PRI_Set(para->CHxPRI);       		// 通道优先级
        DMA_CH11CR_CH11DIR_Set(para->CHxDIR);       		// 数据传输方向
        DMA_CH11CR_CH11RI_Set(para->CH11RI);        		// RAM增长方向
        DMA_CH11CR_CH11FI_Set(para->CH11FI);        		// FLS增长方向
        DMA_CH11CR_CH11FTIE_Setable(para->CHxFTIE);    		// 传输完成中断
        DMA_CH11CR_CH11HTIE_Setable(para->CHxHTIE);    		// 传输半满中断
        
        DMA_CHxMAR_Write(para->CHx, para->CHxRAMAD);        // DMA传输RAM地址
        DMA_CH11FAR_Write(para->CH11FLSAD);                 // DMA传输Flash地址
		
		DMA_CH11CR_CH11EN_Setable(para->CHxEN);        		// 通道使能
    }
    else
    {
		DMA_CHxCR_ChxEN_Setable(para->CHx, DISABLE);    	// 通道关闭使能
        DMA_CHxCR_CHxTSIZE_Set(para->CHx, para->CHxTSIZE);  // 通道传输长度
        DMA_CHxCR_CHxPRI_Set(para->CHx, para->CHxPRI);      // 通道优先级
        DMA_CHxCR_CHxINC_Set(para->CHx, para->CHxINC);      // 通道地址增长方向
        DMA_CHxCR_CHxSSEL_Set(para->CHx, para->CHxSSEL);    // 通道外设选择
        DMA_CHxCR_DIR_Set(para->CHx, para->CHxDIR);         // 通道传输方向
        DMA_CHxCR_BDW_Set(para->CHx, para->CHxBDW);         // 通道传输带宽
        DMA_CHxCR_CIRC_Setable(para->CHx, para->CHxCICR);   // 循环缓冲模式
        DMA_CHxCR_CHxFTIE_Setable(para->CHx, para->CHxFTIE);    // 传输完成中断
        DMA_CHxCR_CHxHTIE_Setable(para->CHx, para->CHxHTIE);    // 传输半满中断
        
        DMA_CHxMAR_Write(para->CHx, para->CHxRAMAD);        // DMA传输RAM地址
		
		DMA_CHxCR_ChxEN_Setable(para->CHx, para->CHxEN);    // 通道使能
    }
}


/******END OF FILE****/
