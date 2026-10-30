
/* Includes ------------------------------------------------------------------*/
#include "adc.h"
#include "uart.h"
#include "systick.h"
#include "control_center.h"

u8 batteryLevel;
u32 batteryInitialCapacity; //电池上报次数

/**
 * @brief  转换电池电量，计算电量信息（以前期预估上报次数来计算剩余容量）
 * @param  None
 * @retval None
 */
void ADC_BatterySglConvert(void)
{
	static u8 tempBattery = 100;
	batteryInitialCapacity++; //按发包次数减少电量信息
	if (batteryInitialCapacity <= BATTERY_MAX_PUSH_CNT)
	{
		batteryLevel = (double)((BATTERY_MAX_PUSH_CNT - batteryInitialCapacity) * 100) / BATTERY_MAX_PUSH_CNT + 0.5;
		if (batteryLevel != tempBattery)
		{
			tempBattery = batteryLevel; //电量信息更新
			device_t.updateBattery = 1;
		}
	}
	else
	{
		device_t.updateBattery = 1;
		batteryLevel = 0;
	}
}
