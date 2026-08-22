
#include <stdio.h>
#include <stdlib.h>
#include "math.h"

#include "systick.h"
#include "uart.h"
#include "adc.h"
#include "control_center.h"
#include "communicate.h"
#include "tim.h"
#include "time.h"
#include "multi_button.h"
#include "rtc.h"

#include <LoRaWAN_APPLY.h>

static u32 sys_pant_ms; //系统开启时间,单位ms
static u32 sys_OPTime_ss;
static u32 sys_OPTime_ms;

const static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
struct tm systime;
struct tm utctime;
time_t Timestamp;

extern void Error_Handler(void);

/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{
	/* USER CODE BEGIN MspInit 0 */

	/* USER CODE END MspInit 0 */

	__HAL_RCC_SYSCFG_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();

	/* System interrupt init*/

	/* USER CODE BEGIN MspInit 1 */

	/* USER CODE END MspInit 1 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

	/** Configure the main internal regulator output voltage
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
#if RTC_CLOCK_SOURCE_LSE
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
#else
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_LSI;
#endif
	RCC_OscInitStruct.HSIState = RCC_HSI_DIV4;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_USART2 | RCC_PERIPHCLK_LPUART1 | RCC_PERIPHCLK_I2C1 | RCC_PERIPHCLK_RTC;
	PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
	PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
	PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
	PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
#if RTC_CLOCK_SOURCE_LSE
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#else
	PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		Error_Handler();
	}
	HAL_PWREx_EnableUltraLowPower();
	HAL_PWREx_EnableFastWakeUp();
	__HAL_RCC_WAKEUPSTOP_CLK_CONFIG(RCC_STOP_WAKEUPCLOCK_HSI);
}

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

void stamp_to_date(time_t stampTime, struct tm *stm)
{
	struct tm *info;
	// char buffer[80];
	//	time(&stampTime);
	info = localtime(&stampTime);

	char nowtime[24];
	memset(nowtime, 0, sizeof(nowtime));
	strftime(nowtime, 24, "%Y-%m-%d %H:%M:%S", info);
#if HIGH_LEVEL_DEBUG_ENABLE
	INFO("\r\nnowtime = %s\n", nowtime);
#endif
	stm->tm_year = atoi(nowtime) - 1900;
	stm->tm_mon = atoi(nowtime + 5) - 1;
	stm->tm_mday = atoi(nowtime + 8);
	stm->tm_hour = atoi(nowtime + 11);
	stm->tm_min = atoi(nowtime + 14);
	stm->tm_sec = atoi(nowtime + 17);
}

/**
 * @brief	初始化系统时间
 * @param
 * @retval
 */
void systemTime_Init(void)
{
	systime.tm_year = 2022;
	systime.tm_mon = 0;
	systime.tm_mday = 1;
	systime.tm_hour = 0;
	systime.tm_min = 0;
	systime.tm_sec = 0;
	systime.tm_wday = whatday(systime.tm_year, systime.tm_mon, systime.tm_mday);
	getTimestamp();
}

/**
 * @brief	系统时钟，年月日分秒
 * @param
 * @retval
 */
static void SystemTimer(void)
{
	static char tem_month;
	systime.tm_sec++;
	Timestamp++;
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
					systime.tm_mday = 1;
					systime.tm_mon++;
					if (systime.tm_mon > 11)
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
 * @brief	uS级别延时，实际延时比理论稍长
 * @param	延时的时间
 * @retval
 */
void Delay_Us(u16 myUs)
{
	u16 i = 0;
	while (myUs--)
	{
		i = 100;
		while (i--)
			;
	}
}

/**
 * @brief	Ms级别延时，实际延时较准
 * @param	延时的时间
 * @retval
 */
void Delay_Ms(u16 myMs)
{
	u16 i = 0;
	while (myMs--)
	{
		i = 8000;
		while (i--)
			;
	}
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

///**
// * @brief This function handles System tick timer.
// */
// void SysTick_Handler(void)
//{
//	// Systick_1ms_Handler();
//  HAL_IncTick();
//}

/**
 * @brief	滴答mS中断事件
 * @param
 * @retval
 */
void Systick_1ms_Handler(void)
{
	static u32 SysTicker;
	static u8 button_tick_start;
	SysTicker++;
	sys_pant_ms++;
	sys_OPTime_ms++;

	if (uartInfo.recvTimeout)
		uartInfo.recvTimeout--;
	if (uartLoRa.recvTimeout)
		uartLoRa.recvTimeout--;
	if (uartLoRa.sendDelay)
		uartLoRa.sendDelay--;
	if (uartGps.recvTimeout)
		uartGps.recvTimeout--;
	if (uartGps.sendDelay)
		uartGps.sendDelay--;
	if (batteryVoltageConvertInterval)
		batteryVoltageConvertInterval--;
	if (sys_OPTime_ms >= SEC_DELAY)
	{
		sys_OPTime_ms = 0;
		sys_OPTime_ss++;
	}
	button_tick_start++;
	if (button_tick_start > 5)
	{
		button_tick_start = 0;
		button_ticks();
	}

	if (SysTicker >= SEC_DELAY)
	{
		SysTicker = 0;
		SystemTimer();
		if (!uartLoRa.sendDelay)
		{
			if (device_t.sleepDelay && !factoryMode)
				device_t.sleepDelay--;
		}
	}
	Led_Blink_Process();
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
