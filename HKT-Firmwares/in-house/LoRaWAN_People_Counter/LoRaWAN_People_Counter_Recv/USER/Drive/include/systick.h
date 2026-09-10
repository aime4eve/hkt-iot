
#ifndef __SYSTICK_H__
#define __SYSTICK_H__

#include "config.h"
#include "time.h"

struct tim
{
    int tm_sec;          /* 秒–取值区间为[0,59] */
    int tm_min;          /* 分 - 取值区间为[0,59] */
    int tm_hour;         /* 时 - 取值区间为[0,23] */
    int tm_mday;         /* 一个月中的日期 - 取值区间为[1,31] */
    int tm_mon;          /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
    int tm_year;         /* 年份，其值从1900开始 */
    int tm_wday;         /* 星期–取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */
    int tm_yday;         /* 从每年的1月1日开始的天数–取值区间为[0,365]，其中0代表1月1日，1代表1月2日，以此类推 */
    int tm_isdst;        /* 夏令时标识符，实行夏令时的时候，tm_isdst为正。不实行夏令时的进候，tm_isdst为0；不了解情况时，tm_isdst()为负。*/
    long int tm_gmtoff;  /*指定了日期变更线东面时区中UTC东部时区正秒数或UTC西部时区的负秒数*/
    const char *tm_zone; /*当前时区的名字(与环境变量TZ有关)*/
};

extern struct tim systime;
extern time_t Timestamp;
extern u32 runHours; //设备运行小时计数
extern u32 runMins;
extern LL_RTC_DateTypeDef RtcDate;//服务器下发的同步日期
extern LL_RTC_TimeTypeDef RtcTime;//服务器下发的同步时间
extern LL_RTC_AlarmTypeDef RTC_AlarmStruct;//定时闹钟

/* Exported functions ------------------------------------------------------- */
void SystemClock_Config(void);
void Systick_Init(void);
void Systick_1ms_Handler(void);
int whatday(int year, int mon, int day);
void stamp_to_date(time_t stampTime, struct tm *stm);
void getTimestamp(void);
void systemTime_Init(void);
void Delay_Us(__IO u16 myUs);
void Delay_Ms(__IO u16 myMs);
void mDelay(u16 ms);
u16 random_syspant(u16 n);
u32 get_syspant_ms(void);
u32 get_sysopentime(void);
void RTC_Init(void);
void getBcdTime2BinTime(LL_RTC_DateTypeDef *RtcDate,LL_RTC_TimeTypeDef *RtcTime);
void setRtcTime(LL_RTC_DateTypeDef *RtcDate,LL_RTC_TimeTypeDef *RtcTime);
void setAlmaTime(u8 hour,u8 minute);
void offAlma(void);
void syncRtcTime(u8 minutes);

void IWDG_Configuration(void);
void IWDG_Feed(void);

#endif /* __SYSTICK_H */
