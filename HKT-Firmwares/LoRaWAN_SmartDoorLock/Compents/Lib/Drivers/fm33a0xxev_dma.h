/**
  ******************************************************************************
  * @file    fm33a0xxev_dma.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the DMA firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_DMA_H
#define __FM33A0XXEV_DMA_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 

typedef enum {
    DMA_CH0 = 0, DMA_CH1 = 1, DMA_CH2 = 2, DMA_CH3 = 3, DMA_CH4 = 4, DMA_CH5 = 5, DMA_CH6 = 6, DMA_CH7 = 7,	
    DMA_CH8 = 8, DMA_CH9 = 9, DMA_CH10 = 10, DMA_CH11 = 11
}DMA_CH_Type;

typedef struct
{
	DMA_CH_Type CHx;			//DMA通道号
	uint32_t CHxTSIZE;          //通道传输长度
	uint32_t CHxPRI;            //通道优先级
	uint32_t CHxINC;            //CH0~CH10通道地址增长方向,CH11无意义
	uint32_t CHxSSEL;           //CH0~CH10外设通道选择,CH11传输方式选择
    uint32_t CHxDIR;            //CH0~CH10通道传输方向
    uint32_t CHxBDW;            //CH0~CH10通道传输带宽
    FunState CHxCICR;           //CH0~CH10循环缓冲模式
	FunState CHxFTIE;           //通道传输完成中断使能
	FunState CHxHTIE;           //通道传输半程中断使能
	FunState CHxEN;             //通道使能
	uint32_t CHxRAMAD;          //CH0~CH11通道RAM指针地址,注意CH11使用的是word地址，驱动里已处理过了，

    uint32_t CH11RI;            // RAM地址增长方向(仅Flash到RAM有效)
    uint32_t CH11FI;            // FLS地址增长方向(仅Flash到RAM有效)
	uint32_t CH11FLSAD;			//通道FLASH指针地址，仅针对通道11有意义	
}DMA_InitTypeDef;

#define	DMA_GCR_DMA_ADDRERR_EN_Pos	1	/* DMA错误地址中断使能
1：允许错误地址中断
0：禁止错误地址中断 */
#define	DMA_GCR_DMA_ADDRERR_EN_Msk	(0x1U << DMA_GCR_DMA_ADDRERR_EN_Pos)

#define	DMA_GCR_DMAEN_Pos	0	/* DMA全局使能
1：DMA使能
0：DMA关闭 */
#define	DMA_GCR_DMAEN_Msk	(0x1U << DMA_GCR_DMAEN_Pos)

#define	DMA_CHxCR_CHxTSIZE_Pos	16	/* Channelx传输长度，1-8192次传输 */
#define	DMA_CHxCR_CHxTSIZE_Msk	(0x1fffU << DMA_CHxCR_CHxTSIZE_Pos)

#define	DMA_CHxCR_CHxPRI_Pos	12	/* Channelx优先级
00：Low
01：Medium
10：High
11：Very High */
#define	DMA_CHxCR_CHxPRI_Msk	(0x3U << DMA_CHxCR_CHxPRI_Pos)
#define	DMA_CHxCR_CHxPRI_LOW	(0x0U << DMA_CHxCR_CHxPRI_Pos)	/* Low */
#define	DMA_CHxCR_CHxPRI_MEDIUM	(0x1U << DMA_CHxCR_CHxPRI_Pos)	/* Medium */
#define	DMA_CHxCR_CHxPRI_HIGH	(0x2U << DMA_CHxCR_CHxPRI_Pos)	/* High */
#define	DMA_CHxCR_CHxPRI_VERY_HIGH	(0x3U << DMA_CHxCR_CHxPRI_Pos)	/* Very High */

#define	DMA_CHxCR_CHxINC_Pos	11	/* RAM地址增减设置
1：RAM地址递增
0：RAM地址递减 */
#define	DMA_CHxCR_CHxINC_Msk	(0x1U << DMA_CHxCR_CHxINC_Pos)
#define	DMA_CHxCR_CHxINC_INCREASE	(0x1U << DMA_CHxCR_CHxINC_Pos)	/* RAM地址递增 */
#define	DMA_CHxCR_CHxINC_DECREASE	(0x0U << DMA_CHxCR_CHxINC_Pos)	/* RAM地址递减 */

#define	DMA_CHxCR_CHxSSEL_Pos	8	/* Channelx外设请求映射
每个通道可以接受8个外设请求，外设请求的映射参见23.6.1DMA请求映射 */
#define	DMA_CHxCR_CHxSSEL_Msk	(0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH0SSEL_ADC               (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_SPI3_RX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_LPUART0_RX        (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_LPUART1_RX        (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_UART2_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_UART4_RX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_AES_IN            (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH0SSEL_QSPI              (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH1SSEL_ADC               (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_SPI0_RX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_SPI2_RX           (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_SPI3_TX           (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_UART0_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_UART2_TX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_UART3_RX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH1SSEL_U7816_RX          (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH2SSEL_SPI0_TX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_SPI2_TX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_SPI4_RX           (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_UART0_TX          (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_UART3_TX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_UART5_RX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_U7816_TX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH2SSEL_AES_OUT           (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH3SSEL_SPI1_RX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_SPI4_TX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_LPUART0_RX        (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_UART1_RX          (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_UART4_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_UART5_TX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_I2C0_TX           (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH3SSEL_AES_IN            (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH4SSEL_SPI1_TX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_SPI3_RX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_LPUART0_TX        (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_UART1_TX          (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_UART2_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_UART4_TX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_I2C0_RX           (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH4SSEL_AES_OUT           (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH5SSEL_SPI0_RX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_SPI2_RX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_LPUART1_RX        (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_UART1_RX          (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_UART2_TX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_UART3_RX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_UART5_RX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH5SSEL_I2C1_TX           (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH6SSEL_SPI0_TX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_SPI2_TX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_LPUART1_TX        (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_UART1_TX          (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_UART3_TX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_UART5_TX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_I2C1_RX           (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH6SSEL_CRC               (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH7SSEL_ADC               (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_SPI1_RX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_SPI3_RX           (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_LPUART0_RX        (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_UART0_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_UART2_RX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_UART4_RX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH7SSEL_QSPI              (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH8SSEL_ADC               (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_SPI1_TX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_SPI2_RX           (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_SPI3_TX           (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_LPUART0_TX        (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_UART0_TX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_UART2_TX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH8SSEL_UART4_TX          (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH9SSEL_SPI0_RX           (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_SPI2_TX           (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_SPI4_RX           (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_LPUART1_RX        (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_UART1_RX          (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_UART3_RX          (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_UART5_RX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH9SSEL_I2C0_TX           (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define DMA_CHxCR_CH10SSEL_SPI0_TX          (0x0U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_SPI4_TX          (0x1U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_LPUART1_TX       (0x2U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_UART1_TX         (0x3U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_UART3_TX         (0x4U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_UART5_TX         (0x5U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_I2C0_RX          (0x6U << DMA_CHxCR_CHxSSEL_Pos)
#define DMA_CHxCR_CH10SSEL_CRC              (0x7U << DMA_CHxCR_CHxSSEL_Pos)

#define	DMA_CHxCR_CIRC_UPD_Pos	7	/* 循环缓冲模式下自动更新传输参数，本轮传输完成后将shadow寄存器内容复制到控制寄存器
0：禁止自动更新
1：使能自动更新
注意：仅通道0~6支持此功能，通道7~10无此寄存器 */
#define	DMA_CHxCR_CIRC_UPD_Msk	(0x1U << DMA_CHxCR_CIRC_UPD_Pos)
	/* 禁止自动更新 */
	/* 使能自动更新 */
	/* 仅通道0~6支持此功能，通道7~10无此寄存器 */

#define	DMA_CHxCR_DIR_Pos	6	/* 通道传输方向
0：从外设读取数据写入RAM
1：从RAM读取数据写入外设 */
#define	DMA_CHxCR_DIR_Msk	(0x1U << DMA_CHxCR_DIR_Pos)
#define	DMA_CHxCR_DIR_TO_RAM	(0x0U << DMA_CHxCR_DIR_Pos)	/* 从外设读取数据写入RAM */
#define	DMA_CHxCR_DIR_TO_PER	(0x1U << DMA_CHxCR_DIR_Pos)	/* 从RAM读取数据写入外设 */

#define	DMA_CHxCR_BDW_Pos	4	/* 传输带宽设置
00：字节，8bit
01：半字，16bit
10：字，32bit
11：RFU */
#define	DMA_CHxCR_BDW_Msk	(0x3U << DMA_CHxCR_BDW_Pos)
#define	DMA_CHxCR_BDW_8BITS	(0x0U << DMA_CHxCR_BDW_Pos)	/* 字节，8bit */
#define	DMA_CHxCR_BDW_16BITS	(0x1U << DMA_CHxCR_BDW_Pos)	/* 半字，16bit */
#define	DMA_CHxCR_BDW_32BITS	(0x2U << DMA_CHxCR_BDW_Pos)	/* 字，32bit */

#define	DMA_CHxCR_CIRC_Pos	3	/* 循环缓冲模式
0：关闭循环模式
1：使能循环模式 */
#define	DMA_CHxCR_CIRC_Msk	(0x1U << DMA_CHxCR_CIRC_Pos)
	/* 关闭循环模式 */
	/* 使能循环模式 */

#define	DMA_CHxCR_CHxFTIE_Pos	2	/* Channelx传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 */
#define	DMA_CHxCR_CHxFTIE_Msk	(0x1U << DMA_CHxCR_CHxFTIE_Pos)
	/* 使能传输完成中断 */
	/* 关闭传输完成中断 */

#define	DMA_CHxCR_CHxHTIE_Pos	1	/* Channelx半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 */
#define	DMA_CHxCR_CHxHTIE_Msk	(0x1U << DMA_CHxCR_CHxHTIE_Pos)
	/* 使能半程中断 */
	/* 关闭半程中断 */

#define	DMA_CHxCR_ChxEN_Pos	0	/* Channelx使能
1：启动通道0
0：关闭通道0 */
#define	DMA_CHxCR_ChxEN_Msk	(0x1U << DMA_CHxCR_ChxEN_Pos)
	/* 启动通道0 */
	/* 关闭通道0 */

#define	DMA_CHxMAR_CHxMEMAD_Pos	0	/* Channelx存储器指针地址，DMA传输启动前软件向此寄存器写入存储器目标地址。
当指针指向空地址时，DMA访问将触发hardfault
当指针指向Flash时，禁止向Flash写入数据。
软件可以查询当前DMA传输的目标存储器地址。

注意：此指针禁止指向0x00080000~0x1FFFFFFF地址，这段地址为flash保留信息区，指向这段地址可能在DMA访问中导致不可预计的结果 */
#define	DMA_CHxMAR_CHxMEMAD_Msk	(0xffffffffU << DMA_CHxMAR_CHxMEMAD_Pos)

#define	DMA_CH11CR_CH11TSIZE_Pos	16	/* Channel11传输长度，1-8192次传输，仅在Flash->RAM传输时有效，RAM->Flash传输为固定长度64次传输 */
#define	DMA_CH11CR_CH11TSIZE_Msk	(0x1fffU << DMA_CH11CR_CH11TSIZE_Pos)

#define	DMA_CH11CR_CH11PRI_Pos	12	/* Channel11优先级
00：Low
01：Medium
10：High
11：Very High */
#define	DMA_CH11CR_CH11PRI_Msk	(0x3U << DMA_CH11CR_CH11PRI_Pos)
#define	DMA_CH11CR_CH11PRI_LOW	(0x0U << DMA_CH11CR_CH11PRI_Pos)	/* Low */
#define	DMA_CH11CR_CH11PRI_MEDIUM	(0x1U << DMA_CH11CR_CH11PRI_Pos)	/* Medium */
#define	DMA_CH11CR_CH11PRI_HIGH	(0x2U << DMA_CH11CR_CH11PRI_Pos)	/* High */
#define	DMA_CH11CR_CH11PRI_VERY_HIGH	(0x3U << DMA_CH11CR_CH11PRI_Pos)	/* Very High */

#define	DMA_CH11CR_CH11DIR_Pos	10	/* Channel11传输方向
1：Flash->RAM传输
0：RAM->Flash传输 */
#define	DMA_CH11CR_CH11DIR_Msk	(0x1U << DMA_CH11CR_CH11DIR_Pos)
#define	DMA_CH11CR_CH11DIR_TO_RAM	(0x1U << DMA_CH11CR_CH11DIR_Pos)	/* Flash->RAM传输 */
#define	DMA_CH11CR_CH11DIR_TO_FLASH	(0x0U << DMA_CH11CR_CH11DIR_Pos)	/* RAM->Flash传输 */

#define	DMA_CH11CR_CH11RI_Pos	9	/* Channel11 RAM地址增减设置，仅在Flash->RAM传输中有效
1：RAM地址递增
0：RAM地址递减 */
#define	DMA_CH11CR_CH11RI_Msk	(0x1U << DMA_CH11CR_CH11RI_Pos)
#define	DMA_CH11CR_CH11RI_INCREASE	(0x1U << DMA_CH11CR_CH11RI_Pos)	/* RAM地址递增 */
#define	DMA_CH11CR_CH11RI_DECREASE	(0x0U << DMA_CH11CR_CH11RI_Pos)	/* RAM地址递减 */

#define	DMA_CH11CR_CH11FI_Pos	8	/* Channel11 Flash地址增减设置，仅在Flash->RAM传输中有效
1：Flash地址递增
0：Flash地址递减 */
#define	DMA_CH11CR_CH11FI_Msk	(0x1U << DMA_CH11CR_CH11FI_Pos)
#define	DMA_CH11CR_CH11FI_INCREASE	(0x1U << DMA_CH11CR_CH11FI_Pos)	/* Flash地址递增 */
#define	DMA_CH11CR_CH11FI_DECREASE	(0x0U << DMA_CH11CR_CH11FI_Pos)	/* Flash地址递减 */

#define	DMA_CH11CR_CH11FTIE_Pos	2	/* Channel11传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 */
#define	DMA_CH11CR_CH11FTIE_Msk	(0x1U << DMA_CH11CR_CH11FTIE_Pos)
	/* 使能传输完成中断 */
	/* 关闭传输完成中断 */

#define	DMA_CH11CR_CH11HTIE_Pos	1	/* Channel11半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 */
#define	DMA_CH11CR_CH11HTIE_Msk	(0x1U << DMA_CH11CR_CH11HTIE_Pos)
	/* 使能半程中断 */
	/* 关闭半程中断 */

#define	DMA_CH11CR_CH11EN_Pos	0	/* Channel11使能
1：启动通道0
0：关闭通道0 */
#define	DMA_CH11CR_CH11EN_Msk	(0x1U << DMA_CH11CR_CH11EN_Pos)
	/* 启动通道0 */
	/* 关闭通道0 */

#define	DMA_CH11FAR_CH11FLSAD_Pos	0	/* Channel11 Flash指针地址，DMA传输启动前软件向此寄存器写入Flash目标地址，DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标Flash地址
此寄存器低位（bit5-0）仅在Flash->RAM传输中有效，RAM->Flash传输中默认对齐Flash的half-sector起始地址 */
#define	DMA_CH11FAR_CH11FLSAD_Msk	(0x1ffffU << DMA_CH11FAR_CH11FLSAD_Pos)

#define	DMA_CH11RAR_CH7RAMAD_Pos	0	/* Channel7 RAM字指针地址，DMA传输启动前软件向此寄存器写入RAM目标地址（word地址），DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标RAM地址 */
#define	DMA_CH11RAR_CH7RAMAD_Msk	(0x3fffU << DMA_CH11RAR_CH7RAMAD_Pos)

#define	DMA_ISR_DMA_ADDRERR_Pos	28	/* DMA传输地址错误标志，当存储器指针超过RAM和Flash合法地址范围时置位 */
#define	DMA_ISR_DMA_ADDRERR_Msk	(0x1U << DMA_ISR_DMA_ADDRERR_Pos)

#define	DMA_ISR_DMACHFT_Pos	16	/* DMA通道x传输完成标志，硬件置位，软件写1清零
1：对应通道传输完成
0：对应通道传输未完成 */
#define	DMA_ISR_DMACHFT_Msk	(0xfffU << DMA_ISR_DMACHFT_Pos)

#define	DMA_ISR_DMACHHT_Pos	0	/* DMA通道x传输半程标志，硬件置位，软件写1清零 */
#define	DMA_ISR_DMACHHT_Msk	(0xfffU << DMA_ISR_DMACHHT_Pos)

#define	DMA_CHxCSR_CHxTSIZE_SDW_Pos	16	/* Channelx传输长度shadow寄存器；循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 */
#define	DMA_CHxCSR_CHxTSIZE_SDW_Msk	(0x1fffU << DMA_CHxCSR_CHxTSIZE_SDW_Pos)

#define	DMA_CHxCSR_CHxINC_SDW_Pos	11	/* RAM地址增减设置shadow寄存器，循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 */
#define	DMA_CHxCSR_CHxINC_SDW_Msk	(0x1U << DMA_CHxCSR_CHxINC_SDW_Pos)

#define	DMA_CHxMASR_CHxMAD_SDW_Pos	0	/* Channelx存储器指针地址影子寄存器。
循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxMEMAD中。 */
#define	DMA_CHxMASR_CHxMAD_SDW_Msk	(0xffffffffU << DMA_CHxMASR_CHxMAD_SDW_Pos)
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void DMA_Deinit(void);

/* DMA错误地址中断使能
1：允许错误地址中断
0：禁止错误地址中断 相关函数 */
extern void DMA_GCR_DMA_ADDRERR_EN_Setable(FunState NewState);
extern FunState DMA_GCR_DMA_ADDRERR_EN_Getable(void);

/* DMA全局使能
1：DMA使能
0：DMA关闭 相关函数 */
extern void DMA_GCR_DMAEN_Setable(FunState NewState);
extern FunState DMA_GCR_DMAEN_Getable(void);

/* Channelx传输长度，1-8192次传输 相关函数 */
extern void DMA_CHxCR_CHxTSIZE_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_CHxTSIZE_Get(DMA_CH_Type CHx);

/* Channelx优先级
00：Low
01：Medium
10：High
11：Very High 相关函数 */
extern void DMA_CHxCR_CHxPRI_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_CHxPRI_Get(DMA_CH_Type CHx);

/* RAM地址增减设置
1：RAM地址递增
0：RAM地址递减 相关函数 */
extern void DMA_CHxCR_CHxINC_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_CHxINC_Get(DMA_CH_Type CHx);

/* Channelx外设请求映射
每个通道可以接受8个外设请求，外设请求的映射参见23.6.1DMA请求映射 相关函数 */
extern void DMA_CHxCR_CHxSSEL_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_CHxSSEL_Get(DMA_CH_Type CHx);

/* 循环缓冲模式下自动更新传输参数，本轮传输完成后将shadow寄存器内容复制到控制寄存器
0：禁止自动更新
1：使能自动更新
注意：仅通道0~6支持此功能，通道7~10无此寄存器 相关函数 */
extern void DMA_CHxCR_CIRC_UPD_Setable(DMA_CH_Type CHx, FunState NewState);
extern FunState DMA_CHxCR_CIRC_UPD_Getable(DMA_CH_Type CHx);

/* 通道传输方向
0：从外设读取数据写入RAM
1：从RAM读取数据写入外设 相关函数 */
extern void DMA_CHxCR_DIR_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_DIR_Get(DMA_CH_Type CHx);

/* 传输带宽设置
00：字节，8bit
01：半字，16bit
10：字，32bit
11：RFU 相关函数 */
extern void DMA_CHxCR_BDW_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCR_BDW_Get(DMA_CH_Type CHx);

/* 循环缓冲模式
0：关闭循环模式
1：使能循环模式 相关函数 */
extern void DMA_CHxCR_CIRC_Setable(DMA_CH_Type CHx, FunState NewState);
extern FunState DMA_CHxCR_CIRC_Getable(DMA_CH_Type CHx);

/* Channelx传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 相关函数 */
extern void DMA_CHxCR_CHxFTIE_Setable(DMA_CH_Type CHx, FunState NewState);
extern FunState DMA_CHxCR_CHxFTIE_Getable(DMA_CH_Type CHx);

/* Channelx半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 相关函数 */
extern void DMA_CHxCR_CHxHTIE_Setable(DMA_CH_Type CHx, FunState NewState);
extern FunState DMA_CHxCR_CHxHTIE_Getable(DMA_CH_Type CHx);

/* Channelx使能
1：启动通道0
0：关闭通道0 相关函数 */
extern void DMA_CHxCR_ChxEN_Setable(DMA_CH_Type CHx, FunState NewState);
extern FunState DMA_CHxCR_ChxEN_Getable(DMA_CH_Type CHx);

/* Channelx存储器指针地址，DMA传输启动前软件向此寄存器写入存储器目标地址。
当指针指向空地址时，DMA访问将触发hardfault
当指针指向Flash时，禁止向Flash写入数据。
软件可以查询当前DMA传输的目标存储器地址。

注意：此指针禁止指向0x00080000~0x1FFFFFFF地址，这段地址为flash保留信息区，指向这段地址可能在DMA访问中导致不可预计的结果 相关函数 */
extern void DMA_CHxMAR_Write(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxMAR_Read(DMA_CH_Type CHx);

/* Channel11传输长度，1-8192次传输，仅在Flash->RAM传输时有效，RAM->Flash传输为固定长度64次传输 相关函数 */
extern void DMA_CH11CR_CH11TSIZE_Set(uint32_t SetValue);
extern uint32_t DMA_CH11CR_CH11TSIZE_Get(void);

/* Channel11优先级
00：Low
01：Medium
10：High
11：Very High 相关函数 */
extern void DMA_CH11CR_CH11PRI_Set(uint32_t SetValue);
extern uint32_t DMA_CH11CR_CH11PRI_Get(void);

/* Channel11传输方向
1：Flash->RAM传输
0：RAM->Flash传输 相关函数 */
extern void DMA_CH11CR_CH11DIR_Set(uint32_t SetValue);
extern uint32_t DMA_CH11CR_CH11DIR_Get(void);

/* Channel11 RAM地址增减设置，仅在Flash->RAM传输中有效
1：RAM地址递增
0：RAM地址递减 相关函数 */
extern void DMA_CH11CR_CH11RI_Set(uint32_t SetValue);
extern uint32_t DMA_CH11CR_CH11RI_Get(void);

/* Channel11 Flash地址增减设置，仅在Flash->RAM传输中有效
1：Flash地址递增
0：Flash地址递减 相关函数 */
extern void DMA_CH11CR_CH11FI_Set(uint32_t SetValue);
extern uint32_t DMA_CH11CR_CH11FI_Get(void);

/* Channel11传输完成中断使能
1：使能传输完成中断
0：关闭传输完成中断 相关函数 */
extern void DMA_CH11CR_CH11FTIE_Setable(FunState NewState);
extern FunState DMA_CH11CR_CH11FTIE_Getable(void);

/* Channel11半程传输完成中断使能
1：使能半程中断
0：关闭半程中断 相关函数 */
extern void DMA_CH11CR_CH11HTIE_Setable(FunState NewState);
extern FunState DMA_CH11CR_CH11HTIE_Getable(void);

/* Channel11使能
1：启动通道0
0：关闭通道0 相关函数 */
extern void DMA_CH11CR_CH11EN_Setable(FunState NewState);
extern FunState DMA_CH11CR_CH11EN_Getable(void);

/* Channel11 Flash指针地址，DMA传输启动前软件向此寄存器写入Flash目标地址，DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标Flash地址
此寄存器低位（bit5-0）仅在Flash->RAM传输中有效，RAM->Flash传输中默认对齐Flash的half-sector起始地址 相关函数 */
extern void DMA_CH11FAR_Write(uint32_t SetValue);
extern uint32_t DMA_CH11FAR_Read(void);

/* Channel7 RAM字指针地址，DMA传输启动前软件向此寄存器写入RAM目标地址（word地址），DMA启动后此寄存器随DMA传输自增或自减
软件可以查询当前DMA传输的目标RAM地址 相关函数 */
extern void DMA_CH11RAR_Write(uint32_t SetValue);
extern uint32_t DMA_CH11RAR_Read(void);

/* DMA传输地址错误标志，当存储器指针超过RAM和Flash合法地址范围时置位 相关函数 */
extern void DMA_ISR_DMA_ADDRERR_Clr(void);
extern FlagStatus DMA_ISR_DMA_ADDRERR_Chk(void);

/* DMA通道x传输完成标志，硬件置位，软件写1清零
1：对应通道传输完成
0：对应通道传输未完成 相关函数 */
extern void DMA_ISR_DMACHFT_Clr(DMA_CH_Type CHx);
extern FlagStatus DMA_ISR_DMACHFT_Chk(DMA_CH_Type CHx);

/* DMA通道x传输半程标志，硬件置位，软件写1清零 相关函数 */
extern void DMA_ISR_DMACHHT_Clr(DMA_CH_Type CHx);
extern FlagStatus DMA_ISR_DMACHHT_Chk(DMA_CH_Type CHx);

/* Channelx传输长度shadow寄存器；循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 相关函数 */
extern void DMA_CHxCSR_CHxTSIZE_SDW_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCSR_CHxTSIZE_SDW_Get(DMA_CH_Type CHx);

/* RAM地址增减设置shadow寄存器，循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxTSIZE中。 相关函数 */
extern void DMA_CHxCSR_CHxINC_SDW_Set(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxCSR_CHxINC_SDW_Get(DMA_CH_Type CHx);

/* Channelx存储器指针地址影子寄存器。
循环模式下如果使能了CIRC_UPD寄存器，则在本轮传输完成后将shadow寄存器值复制到CHxMEMAD中。 相关函数 */
extern void DMA_CHxMASR_Write(DMA_CH_Type CHx, uint32_t SetValue);
extern uint32_t DMA_CHxMASR_Read(DMA_CH_Type CHx);
//Announce_End

void DMA_DeInit(void);
void DMA_Init(DMA_InitTypeDef *para);
   
//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_DMA_H */
