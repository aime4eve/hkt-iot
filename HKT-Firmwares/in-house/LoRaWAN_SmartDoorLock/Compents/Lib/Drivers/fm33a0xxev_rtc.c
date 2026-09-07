/**
  ******************************************************************************
  * @file    fm33a0xxev_rtc.c
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file provides firmware functions to manage the following 
  *          functionalities of....:
  *
*/

/* Includes ------------------------------------------------------------------*/

#include "fm33a0xxev_rtc.h"

/** @addtogroup fm33a0xxev_StdPeriph_Driver
  * @{
  */

/** @defgroup RTC 
  * @brief RTC driver modules
  * @{
  */ 

/* RTC写使能寄存器，
当CPU向RTCWE写入0xACACACAC时，允许CPU向RTC的BCD时间寄存器写入初值，这时RTCWE置1；当CPU向RTCWE写入不为0xACACACAC的任意值时恢复写保护,这时RTCWE清0。 相关函数 */

void RTC_WER_Write(uint32_t SetValue)
{
	RTC->WER  = (SetValue );
}



void RTC_WER_RTCWE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->WER |= (RTC_WER_RTCWE_Msk);
	}
	else
	{
		RTC->WER &= ~(RTC_WER_RTCWE_Msk);
	}
}

FunState RTC_WER_RTCWE_Getable(void)
{
	if (RTC->WER & (RTC_WER_RTCWE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 调校周期中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_ADJ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_ADJ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_ADJ_IE_Msk);
	}
}

FunState RTC_IER_ADJ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_ADJ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 闹钟中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_ALARM_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_ALARM_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_ALARM_IE_Msk);
	}
}

FunState RTC_IER_ALARM_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_ALARM_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 1khz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_1KHZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_1KHZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_1KHZ_IE_Msk);
	}
}

FunState RTC_IER_1KHZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_1KHZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 256hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_256HZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_256HZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_256HZ_IE_Msk);
	}
}

FunState RTC_IER_256HZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_256HZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 16hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_16HZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_16HZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_16HZ_IE_Msk);
	}
}

FunState RTC_IER_16HZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_16HZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 8hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_8HZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_8HZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_8HZ_IE_Msk);
	}
}

FunState RTC_IER_8HZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_8HZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 4hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_4HZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_4HZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_4HZ_IE_Msk);
	}
}

FunState RTC_IER_4HZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_4HZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 2hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_2HZ_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_2HZ_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_2HZ_IE_Msk);
	}
}

FunState RTC_IER_2HZ_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_2HZ_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 秒中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_SEC_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_SEC_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_SEC_IE_Msk);
	}
}

FunState RTC_IER_SEC_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_SEC_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 分中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_MIN_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_MIN_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_MIN_IE_Msk);
	}
}

FunState RTC_IER_MIN_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_MIN_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 小时中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_HOUR_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_HOUR_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_HOUR_IE_Msk);
	}
}

FunState RTC_IER_HOUR_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_HOUR_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 天中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
void RTC_IER_DATE_IE_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->IER |= (RTC_IER_DATE_IE_Msk);
	}
	else
	{
		RTC->IER &= ~(RTC_IER_DATE_IE_Msk);
	}
}

FunState RTC_IER_DATE_IE_Getable(void)
{
	if (RTC->IER & (RTC_IER_DATE_IE_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 调校周期中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_ADJ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_ADJ_IF_Msk;
}

FlagStatus RTC_ISR_ADJ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_ADJ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 闹钟中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_ALARM_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_ALARM_IF_Msk;
}

FlagStatus RTC_ISR_ALARM_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_ALARM_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 1khz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_1KHZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_1KHZ_IF_Msk;
}

FlagStatus RTC_ISR_1KHZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_1KHZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 256hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_256HZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_256HZ_IF_Msk;
}

FlagStatus RTC_ISR_256HZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_256HZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 16hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_16HZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_16HZ_IF_Msk;
}

FlagStatus RTC_ISR_16HZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_16HZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 8hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_8HZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_8HZ_IF_Msk;
}

FlagStatus RTC_ISR_8HZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_8HZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 4hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_4HZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_4HZ_IF_Msk;
}

FlagStatus RTC_ISR_4HZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_4HZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 2hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_2HZ_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_2HZ_IF_Msk;
}

FlagStatus RTC_ISR_2HZ_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_2HZ_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 秒中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_SEC_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_SEC_IF_Msk;
}

FlagStatus RTC_ISR_SEC_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_SEC_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 分中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_MIN_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_MIN_IF_Msk;
}

FlagStatus RTC_ISR_MIN_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_MIN_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 小时中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_HOUR_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_HOUR_IF_Msk;
}

FlagStatus RTC_ISR_HOUR_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_HOUR_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 天中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
void RTC_ISR_DATE_IF_Clr(void)
{
	RTC->ISR = RTC_ISR_DATE_IF_Msk;
}

FlagStatus RTC_ISR_DATE_IF_Chk(void)
{
	if (RTC->ISR & RTC_ISR_DATE_IF_Msk)
	{
		return SET;
	}
	else
	{
		return RESET;
	}
}

/* 秒时间数值，BCD格式。 相关函数 */
void RTC_BCDSEC_Write(uint32_t SetValue)
{
	RTC->BCDSEC = (SetValue & RTC_BCDSEC_BCDSEC_Msk);
}

uint32_t RTC_BCDSEC_Read(void)
{
	return (RTC->BCDSEC & RTC_BCDSEC_BCDSEC_Msk);
}

/* 分钟时间数值，BCD格式。 相关函数 */
void RTC_BCDMIN_Write(uint32_t SetValue)
{
	RTC->BCDMIN = (SetValue & RTC_BCDMIN_BCDMIN_Msk);
}

uint32_t RTC_BCDMIN_Read(void)
{
	return (RTC->BCDMIN & RTC_BCDMIN_BCDMIN_Msk);
}

/* 小时数值，BCD格式。 相关函数 */
void RTC_BCDHOUR_Write(uint32_t SetValue)
{
	RTC->BCDHOUR = (SetValue & RTC_BCDHOUR_BCDHOUR_Msk);
}

uint32_t RTC_BCDHOUR_Read(void)
{
	return (RTC->BCDHOUR & RTC_BCDHOUR_BCDHOUR_Msk);
}

/* 小时数值，BCD格式。 相关函数 */
void RTC_BCDDATE_Write(uint32_t SetValue)
{
	RTC->BCDDATE = (SetValue & RTC_BCDDATE_BCDDATE_Msk);
}

uint32_t RTC_BCDDATE_Read(void)
{
	return (RTC->BCDDATE & RTC_BCDDATE_BCDDATE_Msk);
}


/* 周数值，BCD格式。 相关函数 */
void RTC_BCDWEEK_Write(uint32_t SetValue)
{
	RTC->BCDWEEK = (SetValue & RTC_BCDWEEK_BCDWEEK_Msk);
}

uint32_t RTC_BCDWEEK_Read(void)
{
	return (RTC->BCDWEEK & RTC_BCDWEEK_BCDWEEK_Msk);
}

/* 月数值，BCD格式。 相关函数 */
void RTC_BCDMONTH_Write(uint32_t SetValue)
{
	RTC->BCDMONTH = (SetValue & RTC_BCDMONTH_BCDMONTH_Msk);
}

uint32_t RTC_BCDMONTH_Read(void)
{
	return (RTC->BCDMONTH & RTC_BCDMONTH_BCDMONTH_Msk);
}

/* 年数值，BCD格式。 相关函数 */
void RTC_BCDYEAR_Write(uint32_t SetValue)
{
	RTC->BCDYEAR = (SetValue & RTC_BCDYEAR_BCDYEAR_Msk);
}

uint32_t RTC_BCDYEAR_Read(void)
{
	return (RTC->BCDYEAR & RTC_BCDYEAR_BCDYEAR_Msk);
}

/* 闹钟功能使能 (alarm enable)
1：使能Alarm功能
0：禁止Alarm功能 相关函数 */
void RTC_ALARM_ALMEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->ALARM |= (RTC_ALARM_ALMEN_Msk);
	}
	else
	{
		RTC->ALARM &= ~(RTC_ALARM_ALMEN_Msk);
	}
}

FunState RTC_ALARM_ALMEN_Getable(void)
{
	if (RTC->ALARM & (RTC_ALARM_ALMEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 闹钟的小时数值。 相关函数 */
void RTC_ALARM_ALARMHOUR_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->ALARM;
	tmpreg &= ~(RTC_ALARM_ALARMHOUR_Msk);
	tmpreg |= (SetValue & RTC_ALARM_ALARMHOUR_Msk);
	RTC->ALARM = tmpreg;
}

uint32_t RTC_ALARM_ALARMHOUR_Get(void)
{
	return (RTC->ALARM & RTC_ALARM_ALARMHOUR_Msk);
}

/* 闹钟的分数值。 相关函数 */
void RTC_ALARM_ALARMMIN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->ALARM;
	tmpreg &= ~(RTC_ALARM_ALARMMIN_Msk);
	tmpreg |= (SetValue & RTC_ALARM_ALARMMIN_Msk);
	RTC->ALARM = tmpreg;
}

uint32_t RTC_ALARM_ALARMMIN_Get(void)
{
	return (RTC->ALARM & RTC_ALARM_ALARMMIN_Msk);
}

/* 闹钟的秒数值。 相关函数 */
void RTC_ALARM_ALARMSEC_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->ALARM;
	tmpreg &= ~(RTC_ALARM_ALARMSEC_Msk);
	tmpreg |= (SetValue & RTC_ALARM_ALARMSEC_Msk);
	RTC->ALARM = tmpreg;
}

uint32_t RTC_ALARM_ALARMSEC_Get(void)
{
	return (RTC->ALARM & RTC_ALARM_ALARMSEC_Msk);
}

/* 频率输出选择信号：
0000：输出PLL分频得到的精确1秒方波 
0001：输出PLL分频的可变占空比的精确秒时标 
0010：输出秒计数器进位信号，高电平宽度1s
0011：输出分计数器进位信号，高电平宽度1s
0100：输出小时计数器进位信号，高电平宽度1s
0101：输出天计数器进位信号，高电平宽度1s
0110：输出闹钟匹配信号
0111：输出256秒方波信号
1000：反向输出PLL分频的可变占空比的精确秒时标
1001：反向输出秒计数器进位信号
1010：反向输出分计数器进位信号
1011：反向输出小时计数器进位信号
1100：反向输出天计数器进位信号
1101：反向输出闹钟匹配信号
1110：反向输出PLL分频的精确1s方波信号
1111：输出RTC内部秒时标方波 相关函数 */
void RTC_TMSEL_Write(uint32_t SetValue)
{
	RTC->TMSEL = (SetValue & RTC_TMSEL_FSEL_Msk);
}

uint32_t RTC_TMSEL_Read(void)
{
	return (RTC->TMSEL & RTC_TMSEL_FSEL_Msk);
}

/* LTBC补偿调整数值（原码格式） 相关函数 */
void RTC_ADJUST_Write(uint32_t SetValue)
{
	RTC->ADJUST = (SetValue & RTC_ADJUST_ADJUST_Msk);
}

uint32_t RTC_ADJUST_Read(void)
{
	return (RTC->ADJUST & RTC_ADJUST_ADJUST_Msk);
}

/* LTBC补偿方向
0：表示增加计数初值
1：表示减少计数初值 相关函数 */
void RTC_ADSIGN_ADSIGN_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->ADSIGN;
	tmpreg &= ~(RTC_ADSIGN_ADSIGN_Msk);
	tmpreg |= (SetValue & RTC_ADSIGN_ADSIGN_Msk);
	RTC->ADSIGN = tmpreg;
}

uint32_t RTC_ADSIGN_ADSIGN_Get(void)
{
	return (RTC->ADSIGN & RTC_ADSIGN_ADSIGN_Msk);
}

/* 虚拟调校使能信号
0：表示禁止虚拟调校功能
1：表示使能虚拟调校功能 相关函数 */
void RTC_VCAL_PR1SEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->VCAL |= (RTC_VCAL_PR1SEN_Msk);
	}
	else
	{
		RTC->VCAL &= ~(RTC_VCAL_PR1SEN_Msk);
	}
}

FunState RTC_VCAL_PR1SEN_Getable(void)
{
	if (RTC->VCAL & (RTC_VCAL_PR1SEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 毫秒计数器
CALSTEP=00/11：有效位8bit，精度3.9ms
CALSTEP=01：有效值7bit，精度7.8ms
CALSTEP=10：有效值6bit，精度15.625ms 相关函数 */
void RTC_MSCNT_Write(uint32_t SetValue)
{
	RTC->MSCNT = (SetValue & RTC_MSCNT_MSCNT_Msk);
}

uint32_t RTC_MSCNT_Read(void)
{
	return (RTC->MSCNT & RTC_MSCNT_MSCNT_Msk);
}

/* LTBC最小调校步长选择
10：0.06ppm（精度+/-0.03ppm）
01：0.119ppm（精度+/-0.06ppm）
00/11：0.238ppm（精度+/-0.119ppm） 相关函数 */
void RTC_CALSTEP_CALSTEP_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->CALSTEP;
	tmpreg &= ~(RTC_CALSTEP_CALSTEP_Msk);
	tmpreg |= (SetValue & RTC_CALSTEP_CALSTEP_Msk);
	RTC->CALSTEP = tmpreg;
}

uint32_t RTC_CALSTEP_CALSTEP_Get(void)
{
	return (RTC->CALSTEP & RTC_CALSTEP_CALSTEP_Msk);
}

uint32_t RTC_ADCNT_Read(void)
{
	return (RTC->ADCNT & RTC_ADCNT_ADJCNT_Msk);
}



/* 亚秒计数器 (sub-second counter register)
计数步长15.625ms 相关函数 */
uint32_t RTC_SSR_Read(void)
{
	return (RTC->SSR & RTC_SSR_SSR_Msk);
}

/* 亚秒闹钟使能 (sub-second alarm enable)
1：使能亚秒闹钟功能
0：禁止亚秒闹钟功能 相关函数 */
void RTC_SSA_SSAEN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->SSA |= (RTC_SSA_SSAEN_Msk);
	}
	else
	{
		RTC->SSA &= ~(RTC_SSA_SSAEN_Msk);
	}
}

FunState RTC_SSA_SSAEN_Getable(void)
{
	if (RTC->SSA & (RTC_SSA_SSAEN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}

/* 亚秒闹钟 (sub-second alarm)
当SSAEN=1，并且SSR计数值等于SS_ALARM时，中断标志置位。 相关函数 */
void RTC_SSA_SS_ALARM_Set(uint32_t SetValue)
{
	uint32_t tmpreg;
	tmpreg = RTC->SSA;
	tmpreg &= ~(RTC_SSA_SS_ALARM_Msk);
	tmpreg |= (SetValue & RTC_SSA_SS_ALARM_Msk);
	RTC->SSA = tmpreg;
}

uint32_t RTC_SSA_SS_ALARM_Get(void)
{
	return (RTC->SSA & RTC_SSA_SS_ALARM_Msk);
}

/* RTC虚拟秒时标输出占空比，定义1秒周期内高电平长度，复位默认值是80ms高电平 相关函数 */
void RTC_DTR_Write(uint32_t SetValue)
{
	RTC->DTR = (SetValue & RTC_DTR_DUTY_Msk);
}

uint32_t RTC_DTR_Read(void)
{
	return (RTC->DTR & RTC_DTR_DUTY_Msk);
}

/* RTC时钟和日历使能 (RTC_A Enable)
1：使能RTC走时
0：停止RTC走时

此寄存器用于门控LSCLK输入，如果清零，RTC将立即停止走时；但是虚拟调校功能不受此寄存器影响。 相关函数 */
void RTC_CR_RTC_EN_Setable(FunState NewState)
{
	if (NewState == ENABLE)
	{
		RTC->CR |= (RTC_CR_RTC_EN_Msk);
	}
	else
	{
		RTC->CR &= ~(RTC_CR_RTC_EN_Msk);
	}
}

FunState RTC_CR_RTC_EN_Getable(void)
{
	if (RTC->CR & (RTC_CR_RTC_EN_Msk))
	{
		return ENABLE;
	}
	else
	{
		return DISABLE;
	}
}


void RTC_AlarmTime_SetEx(RTC_AlarmTmieTypeDef* para)
{
	RTC_ALARM_ALARMSEC_Set(((uint32_t)para->Second<<RTC_ALARM_ALARMSEC_Pos));
	RTC_ALARM_ALARMMIN_Set(((uint32_t)para->Minute<<RTC_ALARM_ALARMMIN_Pos));
	RTC_ALARM_ALARMHOUR_Set(((uint32_t)para->Hour<<RTC_ALARM_ALARMHOUR_Pos));
}


/********************************
 RTC设置时间函数
功能:设置时间，秒，分，时，天，月，年，周，BCD格式
输入：要求设置的时间数组
		
********************************/
void RTC_TimeDate_SetEx(RTC_TimeDateTypeDef* para)
{
	RTC_BCDSEC_Write((uint32_t)para->Second);
	RTC_BCDMIN_Write((uint32_t)para->Minute);
	RTC_BCDHOUR_Write((uint32_t)para->Hour);
	RTC_BCDDATE_Write((uint32_t)para->Date);
	RTC_BCDMONTH_Write((uint32_t)para->Month);
	RTC_BCDYEAR_Write((uint32_t)para->Year);
	RTC_BCDWEEK_Write((uint32_t)para->Week);	
}

/********************************
 RTC读取时间函数
功能:读取时间，秒，分，时，天，月，年，周，BCD格式
输出：读取当前时间，保存为数组
		
********************************/
void RTC_TimeDate_GetEx(RTC_TimeDateTypeDef* para)
{
	para->Second = RTC_BCDSEC_Read();
	para->Minute = RTC_BCDMIN_Read();
	para->Hour = RTC_BCDHOUR_Read();
	para->Date = RTC_BCDDATE_Read();
	para->Month = RTC_BCDMONTH_Read();
	para->Year = RTC_BCDYEAR_Read();
	para->Week = RTC_BCDWEEK_Read();	
}






void RTC_Deinit(void)
{
	//RTC->WER = 0x00000000;
	//RTC->IER = 0x00000000;
	//RTC->ISR = 0x00000000;
	//RTC->BCDSEC = 0x00000000;
	//RTC->BCDMIN = 0x00000000;
	//RTC->BCDHOUR = 0x00000000;
	//RTC->BCDWEEK = 0x00000000;
	//RTC->BCDMONTH = 0x00000000;
	//RTC->BCDYEAR = 0x00000000;
	//RTC->ALARM = 0x00000000;
	//RTC->TMSEL = 0x00000000;
	//RTC->ADJUST = 0x00000000;
	//RTC->ADSIGN = 0x00000000;
	//RTC->VCAL = 0x00000000;
	//RTC->MSCNT = 0x00000000;
	//RTC->CALSTEP = 0x00000000;
	//RTC->ADCNT = ;
	//RTC->SSR = 0x00000000;
	//RTC->SSA = 0x00000000;
	//RTC->DTR = 0x00000001;
	//RTC->CR = 0x00000000;
}

/******END OF FILE****/
