#include "crc.h"

void Init_CRC_CRC_CCITT(void)
{
	CRC_InitTypeDef init_para;

	CMU_PERCLK_SetableEx(CRCCLK, ENABLE);

	init_para.CRCSEL = CRC_CR_SEL_CRC16;   /*!<CRC校验多项式选择*/
	init_para.RFLTIN = CRC_CR_RFLTIN_BYTE; /*!<CRC输入反转控制*/
	init_para.RFLTO = CRC_CR_RFLTO_BYTE;	   /*!<CRC输出反转控制*/
	init_para.XOR = DISABLE;			   /*!<输出异或使能*/
	init_para.PARA = CRC_CR_PARA_SERIAL;   /*!<CRC快速计算使能*/
	init_para.OPWD = CRC_CR_OPWD_BYTE;	   /*!<WORD操作使能*/
	init_para.CRCPOLY = 0x1021;			   /*!<CRC多项式寄存器*/
	init_para.CRC_XOR = 0x0000;			   /*!<运算结果异或寄存器*/
	init_para.LFSR = 0x0000;			   /*!<初始值*/
	CRC_Init(&init_para);
}

// CRC-16/CCITT x16+x12+x5+1 0x1021
// Init = 0x0000
u16 CalCRC16_CCITT(u16 Init, u8 *DataIn, u16 Len)
{
	u16 i, CRC16;

	CRC_LFSR_Write(Init); //初值寄存器

	for (i = 0; i < Len; i++)
	{
		CRC_DR_Write(DataIn[i]); //输入输出寄存器
		Do_DelayStart();
		{
			if (RESET == CRC_CR_BUSY_Chk())
			{
				break;
			}
		}
		While_DelayUsEnd(200);
	}

	CRC16 = CRC_DR_Read() & 0xffff;
	return CRC16;
}
