
#include <stdio.h>
#include <stdlib.h>
#include "math.h"

#include "systick.h"
#include "uart.h"
#include "tim.h"
#include "gpio.h"

u32 sys_pant_ms; //系统开启时间,单位ms
u32 sys_OPTime_ss;
u32 sys_OPTime_ms;
u32 tm_tick_ms;

const static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct tm systime;
time_t Timestamp;

// 根据日期推算星期
int whatday(int year, int mon, int day)
{
	int m = mon;
	int d = day;
	// 根据月份对年份和月份进行调整
	if (m <= 2)
	{
		year -= 1;
		m += 12;
	}
	int c = year / 100; // 取得年份前两位
	int y = year % 100; // 取得年份后两位

	// 根据泰勒公式计算星期
	int w = (int)(c / 4) - 2 * c + y + (int)(y / 4) + (int)(13 * (m + 1) / 5) + d - 1;

	return w % 7; // 返回星期
}

/**
 * @brief	初始化系统时间
 * @param
 * @retval
 */
void systemTime_Init(void)
{
	systime.tm_year = 2022;
	systime.tm_mon = 1;
	systime.tm_mday = 1;
	systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
	getTimestamp();
}

/**
 * @brief	系统时钟，年月日分秒
 * @param
 * @retval
 */
void SystemTimer(void)
{
	static char tem_month;
	systime.tm_sec++;
	if (systime.tm_sec > 59)
	{
		systime.tm_sec = 0;
		systime.tm_min++;

		if (systime.tm_min > 59)
		{
			systime.tm_min = 0;
			systime.tm_hour++;
			if (systime.tm_hour > 23)
			{
				systime.tm_hour = 0;
				systime.tm_mday++;
				systime.tm_wday++;
				if (systime.tm_wday > 6)
					systime.tm_wday = 0;
				tem_month = month[systime.tm_mon];
				if ((systime.tm_year % 4 == 0 && systime.tm_year % 100 != 0) || (systime.tm_year % 400 == 0))
				{
					tem_month += 1;
				}
				if (systime.tm_mday > tem_month)
				{
					systime.tm_mday = 0;
					systime.tm_mon++;
					if (systime.tm_mon > 12)
					{
						systime.tm_mon = 0;
						systime.tm_year++;
					}
				}
			}
		}
	}
}

/**
 * @brief	mS级别延时，使用系统中断定时器，延时较准
 * @param	延时的时间
 * @retval
 */
void mDelay(u16 ms)
{
	u16 delay_ms = ms;
	tm_tick_ms = 0;
	while (tm_tick_ms < delay_ms)
		;
}

/**
 * @brief	获取系统运行mS
 * @param
 * @retval	返回系统运行mS
 */
u32 get_syspant_ms(void)
{
	return sys_pant_ms;
}

/**
 * @brief This function handles System tick timer.
 */
// void SysTick_Handler(void)
//{
//	// Systick_1ms_Handler();
// }

/**
 * @brief	滴答mS中断事件
 * @param
 * @retval
 */
void Systick_1ms_Handler(void)
{
	static u16 SysTicker;
	SysTicker++;
	tm_tick_ms++;
	sys_pant_ms++;
	if (uartInfo.recvTimeout)
		uartInfo.recvTimeout--;
	if (SysTicker >= SEC_DELAY)
	{
		SysTicker = 0;
		Timestamp++;
		SystemTimer();
	}
}

/**
 * @brief   随机函数发生，seed:sys_pant_ms
 * @param   参数设定随机数范围1~n
 * @retval  返回生成的随机数value;
 **/
u16 random_syspant(u16 n)
{
	u16 value;
	srand(sys_pant_ms);
	value = rand() % n + 1; //获取一个随机数(1~n)
	return value;
}

/**
 * @brief   系统运行时间计时器
 * @param
 * @retval  返回系统运行时间mS
 **/
u32 get_sysopentime(void)
{
	u32 sys_ms;
	sys_ms = sys_OPTime_ss * 1000 + sys_OPTime_ms;
	return sys_ms;
}

