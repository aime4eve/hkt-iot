/**
  ******************************************************************************
  * @file    fm33a0xxev_rtc.h
  * @author  FM33A0XXEV Application Team
  * @version V1.0.0
  * @date    16-April-2020
  * @brief   This file contains all the functions prototypes for the RTC firmware library.  
  ******************************************************************************
  */
  
/* Define to prevent recursive inclusion -------------------------------------*/

#ifndef __FM33A0XXEV_RTC_H
#define __FM33A0XXEV_RTC_H


#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "FM33A0XXEV.h" 
  
typedef struct
{       
	uint8_t			Year;		/*!<RTC 年*/
	uint8_t			Month;		/*!<RTC 月*/
	uint8_t			Date;		/*!<RTC 日*/	
	uint8_t			Hour;		/*!<RTC 时*/
	uint8_t			Minute;		/*!<RTC 分*/	
	uint8_t			Second;		/*!<RTC 秒*/			
	uint8_t			Week;		/*!<RTC 周*/	

}RTC_TimeDateTypeDef,RTC_StampTmieTypeDef;

typedef struct
{     
	uint8_t			Hour;		/*!<RTC闹钟 时*/		
	uint8_t			Minute;		/*!<RTC闹钟 分*/
	uint8_t			Second;		/*!<RTC闹钟 秒*/

}RTC_AlarmTmieTypeDef;	 



#define RTC_WRITE_ENABLE    ((uint32)0xACACACAC)
#define RTC_WRITE_DISABLE   ((uint32)0x55AA55AA)	 
	 
#define	RTC_WER_RTCWE_Pos	0	/* RTC写使能寄存器，
当CPU向RTCWE写入0xACACACAC时，允许CPU向RTC的BCD时间寄存器写入初值，这时RTCWE置1；当CPU向RTCWE写入不为0xACACACAC的任意值时恢复写保护,这时RTCWE清0。 */
#define	RTC_WER_RTCWE_Msk	(0x1U << RTC_WER_RTCWE_Pos)

#define	RTC_IER_ADJ_IE_Pos	12	/* 调校周期中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_ADJ_IE_Msk	(0x1U << RTC_IER_ADJ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_ALARM_IE_Pos	11	/* 闹钟中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_ALARM_IE_Msk	(0x1U << RTC_IER_ALARM_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_1KHZ_IE_Pos	10	/* 1khz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_1KHZ_IE_Msk	(0x1U << RTC_IER_1KHZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_256HZ_IE_Pos	9	/* 256hz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_256HZ_IE_Msk	(0x1U << RTC_IER_256HZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_16HZ_IE_Pos	7	/* 16hz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_16HZ_IE_Msk	(0x1U << RTC_IER_16HZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_8HZ_IE_Pos	6	/* 8hz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_8HZ_IE_Msk	(0x1U << RTC_IER_8HZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_4HZ_IE_Pos	5	/* 4hz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_4HZ_IE_Msk	(0x1U << RTC_IER_4HZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_2HZ_IE_Pos	4	/* 2hz中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_2HZ_IE_Msk	(0x1U << RTC_IER_2HZ_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_SEC_IE_Pos	3	/* 秒中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_SEC_IE_Msk	(0x1U << RTC_IER_SEC_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_MIN_IE_Pos	2	/* 分中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_MIN_IE_Msk	(0x1U << RTC_IER_MIN_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_HOUR_IE_Pos	1	/* 小时中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_HOUR_IE_Msk	(0x1U << RTC_IER_HOUR_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_IER_DATE_IE_Pos	0	/* 天中断使能。
1：中断使能打开
0：中断使能禁止 */
#define	RTC_IER_DATE_IE_Msk	(0x1U << RTC_IER_DATE_IE_Pos)
	/* 中断使能打开 */
	/* 中断使能禁止 */

#define	RTC_ISR_ADJ_IF_Pos	12	/* 调校周期中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_ADJ_IF_Msk	(0x1U << RTC_ISR_ADJ_IF_Pos)

#define	RTC_ISR_ALARM_IF_Pos	11	/* 闹钟中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_ALARM_IF_Msk	(0x1U << RTC_ISR_ALARM_IF_Pos)

#define	RTC_ISR_1KHZ_IF_Pos	10	/* 1khz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_1KHZ_IF_Msk	(0x1U << RTC_ISR_1KHZ_IF_Pos)

#define	RTC_ISR_256HZ_IF_Pos	9	/* 256hz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_256HZ_IF_Msk	(0x1U << RTC_ISR_256HZ_IF_Pos)

#define	RTC_ISR_16HZ_IF_Pos	7	/* 16hz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_16HZ_IF_Msk	(0x1U << RTC_ISR_16HZ_IF_Pos)

#define	RTC_ISR_8HZ_IF_Pos	6	/* 8hz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_8HZ_IF_Msk	(0x1U << RTC_ISR_8HZ_IF_Pos)

#define	RTC_ISR_4HZ_IF_Pos	5	/* 4hz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_4HZ_IF_Msk	(0x1U << RTC_ISR_4HZ_IF_Pos)

#define	RTC_ISR_2HZ_IF_Pos	4	/* 2hz中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_2HZ_IF_Msk	(0x1U << RTC_ISR_2HZ_IF_Pos)

#define	RTC_ISR_SEC_IF_Pos	3	/* 秒中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_SEC_IF_Msk	(0x1U << RTC_ISR_SEC_IF_Pos)

#define	RTC_ISR_MIN_IF_Pos	2	/* 分中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_MIN_IF_Msk	(0x1U << RTC_ISR_MIN_IF_Pos)

#define	RTC_ISR_HOUR_IF_Pos	1	/* 小时中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_HOUR_IF_Msk	(0x1U << RTC_ISR_HOUR_IF_Pos)

#define	RTC_ISR_DATE_IF_Pos	0	/* 天中断标志。写1清零
1：中断置位
0：无中断产生 */
#define	RTC_ISR_DATE_IF_Msk	(0x1U << RTC_ISR_DATE_IF_Pos)

#define	RTC_BCDSEC_BCDSEC_Pos	0	/* 秒时间数值，BCD格式。 */
#define	RTC_BCDSEC_BCDSEC_Msk	(0x7fU << RTC_BCDSEC_BCDSEC_Pos)

#define	RTC_BCDMIN_BCDMIN_Pos	0	/* 分钟时间数值，BCD格式。 */
#define	RTC_BCDMIN_BCDMIN_Msk	(0x7fU << RTC_BCDMIN_BCDMIN_Pos)

#define	RTC_BCDHOUR_BCDHOUR_Pos	0	/* 小时数值，BCD格式。 */
#define	RTC_BCDHOUR_BCDHOUR_Msk	(0x3fU << RTC_BCDHOUR_BCDHOUR_Pos)

#define	RTC_BCDDATE_BCDDATE_Pos	0	/* 小时数值，BCD格式。 */
#define	RTC_BCDDATE_BCDDATE_Msk	(0x3fU << RTC_BCDDATE_BCDDATE_Pos)

#define	RTC_BCDWEEK_BCDWEEK_Pos	0	/* 周数值，BCD格式。 */
#define	RTC_BCDWEEK_BCDWEEK_Msk	(0x7U << RTC_BCDWEEK_BCDWEEK_Pos)

#define	RTC_BCDMONTH_BCDMONTH_Pos	0	/* 月数值，BCD格式。 */
#define	RTC_BCDMONTH_BCDMONTH_Msk	(0x1fU << RTC_BCDMONTH_BCDMONTH_Pos)

#define	RTC_BCDYEAR_BCDYEAR_Pos	0	/* 年数值，BCD格式。 */
#define	RTC_BCDYEAR_BCDYEAR_Msk	(0xffU << RTC_BCDYEAR_BCDYEAR_Pos)

#define	RTC_ALARM_ALMEN_Pos	31	/* 闹钟功能使能 (alarm enable)
1：使能Alarm功能
0：禁止Alarm功能 */
#define	RTC_ALARM_ALMEN_Msk	(0x1U << RTC_ALARM_ALMEN_Pos)
	/* 使能Alarm功能 */
	/* 禁止Alarm功能 */

#define	RTC_ALARM_ALARMHOUR_Pos	16	/* 闹钟的小时数值。 */
#define	RTC_ALARM_ALARMHOUR_Msk	(0x3fU << RTC_ALARM_ALARMHOUR_Pos)

#define	RTC_ALARM_ALARMMIN_Pos	8	/* 闹钟的分数值。 */
#define	RTC_ALARM_ALARMMIN_Msk	(0x7fU << RTC_ALARM_ALARMMIN_Pos)

#define	RTC_ALARM_ALARMSEC_Pos	0	/* 闹钟的秒数值。 */
#define	RTC_ALARM_ALARMSEC_Msk	(0x7fU << RTC_ALARM_ALARMSEC_Pos)

#define	RTC_TMSEL_FSEL_Pos	0	/* 频率输出选择信号：
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
1111：输出RTC内部秒时标方波 */
#define	RTC_TMSEL_FSEL_Msk	        (0xfU << RTC_TMSEL_FSEL_Pos)
#define	RTC_TMSEL_FSEL_PLL1HZ    	(0x0U << RTC_TMSEL_FSEL_Pos)// 输出PLL分频得到的精确1秒方波 
#define	RTC_TMSEL_FSEL_PLLD1HZ   	(0x1U << RTC_TMSEL_FSEL_Pos)//输出PLL分频的可变占空比的精确秒时标 
#define	RTC_TMSEL_FSEL_SECOND    	(0x2U << RTC_TMSEL_FSEL_Pos)	/* 4’b0010：输出秒计数器进位信号，高电平宽度1s */
#define	RTC_TMSEL_FSEL_MINUTE    	(0x3U << RTC_TMSEL_FSEL_Pos)	/* 4’b0011：输出分计数器进位信号，高电平宽度1s */
#define	RTC_TMSEL_FSEL_HOUR	      (0x4U << RTC_TMSEL_FSEL_Pos)	/* 4’b0100：输出小时计数器进位信号，高电平宽度1s */
#define	RTC_TMSEL_FSEL_DAY	        (0x5U << RTC_TMSEL_FSEL_Pos)	/* 4’b0101：输出天计数器进位信号，高电平宽度1s */
#define	RTC_TMSEL_FSEL_ALARM	      (0x6U << RTC_TMSEL_FSEL_Pos)	/* 4’b0110：输出闹钟匹配信号 */
#define	RTC_TMSEL_FSEL_256S	      (0x7U << RTC_TMSEL_FSEL_Pos)	/* 4’b0111：输出128秒方波信号 */
#define	RTC_TMSEL_FSEL_PLLD1HZREV	(0x8U << RTC_TMSEL_FSEL_Pos)	/* 4’b1000：反向输出PLL分频的高电平宽度80ms的秒时标 */
#define	RTC_TMSEL_FSEL_SECONDREV	  (0x9U << RTC_TMSEL_FSEL_Pos)	/* 4’b1001：反向输出秒计数器进位信号 */
#define	RTC_TMSEL_FSEL_MINUTEREV	  (0xaU << RTC_TMSEL_FSEL_Pos)	/* 4’b1010：反向输出分计数器进位信号 */
#define	RTC_TMSEL_FSEL_HOURREV	    (0xbU << RTC_TMSEL_FSEL_Pos)	/* 4’b1011：反向输出小时计数器进位信号 */
#define	RTC_TMSEL_FSEL_DAYREV	    (0xcU << RTC_TMSEL_FSEL_Pos)	/* 4’b1100：反向输出天计数器进位信号 */
#define	RTC_TMSEL_FSEL_ALARMREV	  (0xdU << RTC_TMSEL_FSEL_Pos)	/* 4’b1101：反向输出闹钟匹配信号 */
#define	RTC_TMSEL_FSEL_PLL1HZREV	  (0xeU << RTC_TMSEL_FSEL_Pos)	/* 4’b1110：反向输出PLL分频的精确1s方波信号 */
#define	RTC_TMSEL_FSEL_SECONDRUN	  (0xfU << RTC_TMSEL_FSEL_Pos)	/* 4’b1111：输出RTC内部秒时标方波 */





#define	RTC_ADJUST_ADJUST_Pos	0	/* LTBC补偿调整数值（原码格式） */
#define	RTC_ADJUST_ADJUST_Msk	(0x1fffU << RTC_ADJUST_ADJUST_Pos)

#define	RTC_ADSIGN_ADSIGN_Pos	0	/* LTBC补偿方向
0：表示增加计数初值
1：表示减少计数初值 */
#define	RTC_ADSIGN_ADSIGN_Msk	(0x1U << RTC_ADSIGN_ADSIGN_Pos)
#define	RTC_ADSIGN_ADSIGN_INCREASE	(0x0U << RTC_ADSIGN_ADSIGN_Pos)	/* 表示增加计数初值 */
#define	RTC_ADSIGN_ADSIGN_DECREASE	(0x1U << RTC_ADSIGN_ADSIGN_Pos)	/* 表示减少计数初值 */

#define	RTC_VCAL_PR1SEN_Pos	0	/* 虚拟调校使能信号
0：表示禁止虚拟调校功能
1：表示使能虚拟调校功能 */
#define	RTC_VCAL_PR1SEN_Msk	(0x1U << RTC_VCAL_PR1SEN_Pos)
	/* 表示禁止虚拟调校功能 */
	/* 表示使能虚拟调校功能 */

#define	RTC_MSCNT_MSCNT_Pos	0	/* 毫秒计数器
CALSTEP=00/11：有效位8bit，精度3.9ms
CALSTEP=01：有效值7bit，精度7.8ms
CALSTEP=10：有效值6bit，精度15.625ms */
#define	RTC_MSCNT_MSCNT_Msk	(0xffU << RTC_MSCNT_MSCNT_Pos)

#define	RTC_CALSTEP_CALSTEP_Pos	0	/* LTBC最小调校步长选择
10：0.06ppm（精度+/-0.03ppm）
01：0.119ppm（精度+/-0.06ppm）
00/11：0.238ppm（精度+/-0.119ppm） */
#define	RTC_CALSTEP_CALSTEP_Msk	(0x1U << RTC_CALSTEP_CALSTEP_Pos)
#define	RTC_CALSTEP_CALSTEP_006PPM	(0x2U << RTC_CALSTEP_CALSTEP_Pos)	/* 0.06ppm（精度+/-0.03ppm） */
#define	RTC_CALSTEP_CALSTEP_0119PPM	(0x1U << RTC_CALSTEP_CALSTEP_Pos)	/* 0.119ppm（精度+/-0.06ppm） */
#define	RTC_CALSTEP_CALSTEP_0238PPM	(0x0U << RTC_CALSTEP_CALSTEP_Pos)	/* 0.238ppm（精度+/-0.119ppm） */

#define	RTC_ADCNT_ADJCNT_Pos	0
#define	RTC_ADCNT_ADJCNT_Msk	(0x1ffU << RTC_ADCNT_ADJCNT_Pos)

#define	RTC_SSR_SSR_Pos	0	/* 亚秒计数器 (sub-second counter register)
计数步长15.625ms */
#define	RTC_SSR_SSR_Msk	(0x3fU << RTC_SSR_SSR_Pos)

#define	RTC_SSA_SSAEN_Pos	7	/* 亚秒闹钟使能 (sub-second alarm enable)
1：使能亚秒闹钟功能
0：禁止亚秒闹钟功能 */
#define	RTC_SSA_SSAEN_Msk	(0x1U << RTC_SSA_SSAEN_Pos)
	/* 使能亚秒闹钟功能 */
	/* 禁止亚秒闹钟功能 */

#define	RTC_SSA_SS_ALARM_Pos	0	/* 亚秒闹钟 (sub-second alarm)
当SSAEN=1，并且SSR计数值等于SS_ALARM时，中断标志置位。 */
#define	RTC_SSA_SS_ALARM_Msk	(0x3fU << RTC_SSA_SS_ALARM_Pos)

#define	RTC_DTR_DUTY_Pos	0	/* RTC_A虚拟秒时标输出占空比，定义1秒周期内高电平长度，复位默认值是80ms高电平 */
#define	RTC_DTR_DUTY_Msk	(0x1ffffffU << RTC_DTR_DUTY_Pos)

#define	RTC_CR_RTC_EN_Pos	0	/* RTC_A时钟和日历使能 (RTC_A Enable)
1：使能RTC走时
0：停止RTC走时

此寄存器用于门控LSCLK输入，如果清零，RTC将立即停止走时；但是虚拟调校功能不受此寄存器影响。 */
#define	RTC_CR_RTC_EN_Msk	(0x1U << RTC_CR_RTC_EN_Pos)
	/* 使能RTC走时 */
	/* 停止RTC走时 */
//Macro_End

/* Exported functions --------------------------------------------------------*/ 
extern void RTC_Deinit(void);

/* RTC写使能寄存器，
当CPU向RTCWE写入0xACACACAC时，允许CPU向RTC的BCD时间寄存器写入初值，这时RTCWE置1；当CPU向RTCWE写入不为0xACACACAC的任意值时恢复写保护,这时RTCWE清0。 相关函数 */


extern void RTC_TimeDate_SetEx(RTC_TimeDateTypeDef* para);
extern void RTC_TimeDate_GetEx(RTC_TimeDateTypeDef* para);

extern void RTC_WER_Write(uint32_t SetValue);
extern void RTC_WER_RTCWE_Setable(FunState NewState);
extern FunState RTC_WER_RTCWE_Getable(void);

/* 调校周期中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_ADJ_IE_Setable(FunState NewState);
extern FunState RTC_IER_ADJ_IE_Getable(void);

/* 闹钟中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_ALARM_IE_Setable(FunState NewState);
extern FunState RTC_IER_ALARM_IE_Getable(void);

/* 1khz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_1KHZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_1KHZ_IE_Getable(void);

/* 256hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_256HZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_256HZ_IE_Getable(void);

/* 16hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_16HZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_16HZ_IE_Getable(void);

/* 8hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_8HZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_8HZ_IE_Getable(void);

/* 4hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_4HZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_4HZ_IE_Getable(void);

/* 2hz中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_2HZ_IE_Setable(FunState NewState);
extern FunState RTC_IER_2HZ_IE_Getable(void);

/* 秒中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_SEC_IE_Setable(FunState NewState);
extern FunState RTC_IER_SEC_IE_Getable(void);

/* 分中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_MIN_IE_Setable(FunState NewState);
extern FunState RTC_IER_MIN_IE_Getable(void);

/* 小时中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_HOUR_IE_Setable(FunState NewState);
extern FunState RTC_IER_HOUR_IE_Getable(void);

/* 天中断使能。
1：中断使能打开
0：中断使能禁止 相关函数 */
extern void RTC_IER_DATE_IE_Setable(FunState NewState);
extern FunState RTC_IER_DATE_IE_Getable(void);

/* 调校周期中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_ADJ_IF_Clr(void);
extern FlagStatus RTC_ISR_ADJ_IF_Chk(void);

/* 闹钟中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_ALARM_IF_Clr(void);
extern FlagStatus RTC_ISR_ALARM_IF_Chk(void);

/* 1khz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_1KHZ_IF_Clr(void);
extern FlagStatus RTC_ISR_1KHZ_IF_Chk(void);

/* 256hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_256HZ_IF_Clr(void);
extern FlagStatus RTC_ISR_256HZ_IF_Chk(void);

/* 16hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_16HZ_IF_Clr(void);
extern FlagStatus RTC_ISR_16HZ_IF_Chk(void);

/* 8hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_8HZ_IF_Clr(void);
extern FlagStatus RTC_ISR_8HZ_IF_Chk(void);

/* 4hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_4HZ_IF_Clr(void);
extern FlagStatus RTC_ISR_4HZ_IF_Chk(void);

/* 2hz中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_2HZ_IF_Clr(void);
extern FlagStatus RTC_ISR_2HZ_IF_Chk(void);

/* 秒中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_SEC_IF_Clr(void);
extern FlagStatus RTC_ISR_SEC_IF_Chk(void);

/* 分中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_MIN_IF_Clr(void);
extern FlagStatus RTC_ISR_MIN_IF_Chk(void);

/* 小时中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_HOUR_IF_Clr(void);
extern FlagStatus RTC_ISR_HOUR_IF_Chk(void);

/* 天中断标志。写1清零
1：中断置位
0：无中断产生 相关函数 */
extern void RTC_ISR_DATE_IF_Clr(void);
extern FlagStatus RTC_ISR_DATE_IF_Chk(void);

/* 秒时间数值，BCD格式。 相关函数 */
extern void RTC_BCDSEC_Write(uint32_t SetValue);
extern uint32_t RTC_BCDSEC_Read(void);

/* 分钟时间数值，BCD格式。 相关函数 */
extern void RTC_BCDMIN_Write(uint32_t SetValue);
extern uint32_t RTC_BCDMIN_Read(void);

/* 小时数值，BCD格式。 相关函数 */
extern void RTC_BCDHOUR_Write(uint32_t SetValue);
extern uint32_t RTC_BCDHOUR_Read(void);

/* 周数值，BCD格式。 相关函数 */
extern void RTC_BCDWEEK_Write(uint32_t SetValue);
extern uint32_t RTC_BCDWEEK_Read(void);

/* 月数值，BCD格式。 相关函数 */
extern void RTC_BCDMONTH_Write(uint32_t SetValue);
extern uint32_t RTC_BCDMONTH_Read(void);

/* 年数值，BCD格式。 相关函数 */
extern void RTC_BCDYEAR_Write(uint32_t SetValue);
extern uint32_t RTC_BCDYEAR_Read(void);

/* 闹钟功能使能 (alarm enable)
1：使能Alarm功能
0：禁止Alarm功能 相关函数 */
extern void RTC_ALARM_ALMEN_Setable(FunState NewState);
extern FunState RTC_ALARM_ALMEN_Getable(void);

/* 闹钟的小时数值。 相关函数 */
extern void RTC_ALARM_ALARMHOUR_Set(uint32_t SetValue);
extern uint32_t RTC_ALARM_ALARMHOUR_Get(void);

/* 闹钟的分数值。 相关函数 */
extern void RTC_ALARM_ALARMMIN_Set(uint32_t SetValue);
extern uint32_t RTC_ALARM_ALARMMIN_Get(void);

/* 闹钟的秒数值。 相关函数 */
extern void RTC_ALARM_ALARMSEC_Set(uint32_t SetValue);
extern uint32_t RTC_ALARM_ALARMSEC_Get(void);

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
extern void RTC_TMSEL_Write(uint32_t SetValue);
extern uint32_t RTC_TMSEL_Read(void);

/* LTBC补偿调整数值（原码格式） 相关函数 */
extern void RTC_ADJUST_Write(uint32_t SetValue);
extern uint32_t RTC_ADJUST_Read(void);

/* LTBC补偿方向
0：表示增加计数初值
1：表示减少计数初值 相关函数 */
extern void RTC_ADSIGN_ADSIGN_Set(uint32_t SetValue);
extern uint32_t RTC_ADSIGN_ADSIGN_Get(void);

/* 虚拟调校使能信号
0：表示禁止虚拟调校功能
1：表示使能虚拟调校功能 相关函数 */
extern void RTC_VCAL_PR1SEN_Setable(FunState NewState);
extern FunState RTC_VCAL_PR1SEN_Getable(void);

/* 毫秒计数器
CALSTEP=00/11：有效位8bit，精度3.9ms
CALSTEP=01：有效值7bit，精度7.8ms
CALSTEP=10：有效值6bit，精度15.625ms 相关函数 */
extern void RTC_MSCNT_Write(uint32_t SetValue);
extern uint32_t RTC_MSCNT_Read(void);

/* LTBC最小调校步长选择
10：0.06ppm（精度+/-0.03ppm）
01：0.119ppm（精度+/-0.06ppm）
00/11：0.238ppm（精度+/-0.119ppm） 相关函数 */
extern void RTC_CALSTEP_CALSTEP_Set(uint32_t SetValue);
extern uint32_t RTC_CALSTEP_CALSTEP_Get(void);
extern uint32_t RTC_ADCNT_Read(void);

/* 亚秒计数器 (sub-second counter register)
计数步长15.625ms 相关函数 */
extern uint32_t RTC_SSR_Read(void);

/* 亚秒闹钟使能 (sub-second alarm enable)
1：使能亚秒闹钟功能
0：禁止亚秒闹钟功能 相关函数 */
extern void RTC_SSA_SSAEN_Setable(FunState NewState);
extern FunState RTC_SSA_SSAEN_Getable(void);

/* 亚秒闹钟 (sub-second alarm)
当SSAEN=1，并且SSR计数值等于SS_ALARM时，中断标志置位。 相关函数 */
extern void RTC_SSA_SS_ALARM_Set(uint32_t SetValue);
extern uint32_t RTC_SSA_SS_ALARM_Get(void);

/* RTC虚拟秒时标输出占空比，定义1秒周期内高电平长度，复位默认值是80ms高电平 相关函数 */
extern void RTC_DTR_Write(uint32_t SetValue);
extern uint32_t RTC_DTR_Read(void);

/* RTC时钟和日历使能 (RTC_A Enable)
1：使能RTC走时
0：停止RTC走时

此寄存器用于门控LSCLK输入，如果清零，RTC将立即停止走时；但是虚拟调校功能不受此寄存器影响。 相关函数 */
extern void RTC_CR_RTC_EN_Setable(FunState NewState);
extern FunState RTC_CR_RTC_EN_Getable(void);

extern void RTC_AlarmTime_SetEx(RTC_AlarmTmieTypeDef* para);


//Announce_End
#ifdef __cplusplus
}
#endif

#endif /* __FM33A0XXEV_RTC_H */
