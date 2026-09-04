#include "lis2dh12.h"
#include "lis2dh12_reg.h"
#include "spi.h"
#include "uart.h"
#include "control_center.h"
#include "gpio.h"
#include "systick.h"
#include "communicate.h"

#define LIS2DH12_FROM_FS_2g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 1.0f
#define LIS2DH12_FROM_FS_4g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 2.0f
#define LIS2DH12_FROM_FS_8g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 4.0f
#define LIS2DH12_FROM_FS_16g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 12.0f
#define LIS2DH12_FROM_LSB_TO_degC_HR(lsb) (float)((int16_t)lsb >> 6) / 4.0f + 25.0f

axis_info_t acc_sample;
filter_avg_t acc_data;
struct lis2dh12_offset lis2dh12_offset_t;

int32_t lis2dh12_iic_write_byte(uint8_t reg, uint8_t data)
{
	HAL_I2C_Mem_Write(&hi2c1, LIS2DH12_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, &data, 1, 1000);
	return 1;
}

int32_t lis2dh12_iic_read_byte(uint8_t reg, uint8_t *data)
{
	HAL_I2C_Mem_Read(&hi2c1, LIS2DH12_I2C_ADD_H, reg, I2C_MEMADD_SIZE_8BIT, data, 1, 1000);
	return 1;
}

int32_t lis2dh12_spi_write_byte(uint8_t reg, uint8_t data)
{
	u8 buf[2];
	buf[0] = reg & 0x7f;
	buf[1] = data;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(2);
	HAL_SPI_Transmit(&hspi1, buf, 2, 500);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	return 1;
}

int32_t lis2dh12_spi_read_byte(uint8_t reg, uint8_t *data)
{
	u8 cmd = reg | 0x80;
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
	// HAL_Delay(2);
	HAL_SPI_Transmit(&hspi1, &cmd, 1, 500);
	HAL_SPI_Receive(&hspi1, data, 1, 500);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	return 1;
}

/* ============================================================
func name   :  Lis2dh12_Init
discription :  配置检测阈值与中断
param       :  void
return      :
Revision    :
=============================================================== */
int32_t Lis2dh12_Init(void)
{
	/* Initialization of sensor */
	lis2dh12_spi_write_byte(0x20, 0x37); /* CTRL_REG1(20h): 关闭sensor，设置进入掉电模式 ODR 25HZ */
										 //	lis2dh12_iic_write_byte(0x20, 0x57);	/* CTRL_REG1(20h): 关闭sensor，设置进入低功耗模式 ODR 100HZ */
	lis2dh12_spi_write_byte(0x21, 0x03); /* CTRL_REG2(21h): IA1、IA2 开启高通滤波 bc */
	lis2dh12_spi_write_byte(0x22, 0xc0); /* CTRL_REG3(22h): 0x80 使能单击中断到INT_1 INT_2 */
	lis2dh12_spi_write_byte(0x23, 0x88); /* CTRL_REG4(23h): 使能快，数据更新，全量程+/-2G，非常精度模式 */
	lis2dh12_spi_write_byte(0x24, 0x04); /* CTRL_REG5(24h): 使能4D检测 INT_1 */
	// lis2dh12_iic_write_byte(0x25, 0x00);  /* CTRL_REG6(25h): 高电平(上升沿)触发中断 */

	/* INT1 翻转检测，中断*/			 // 0x6a
	lis2dh12_spi_write_byte(0x30, 0x7f); /* INT1_CFG(30h): 使能，6D X/Y/Z任一超过阈值中断 */
	//	lis2dh12_iic_write_byte(0x30, 0x4f);  /* INT1_CFG(30h): 使能，6D X/Y任一超过阈值中断 */
	//  lis2dh12_iic_write_byte(0x31, 0x20);  /* INT1_SRC(31h): 设置中断源 */

	lis2dh12_spi_write_byte(0x32, 0x03); /* INT1_THS(32h): 设置中断阀值 0x10: 16*2(FS)  0x20: 32*16(FS) */
	//	lis2dh12_spi_write_byte(0x32, 0x20);

	//	lis2dh12_iic_write_byte(0x33, 0x02);  	/* INT1_DURATION(33h): 1LSB=1/ODR  如果ODR=25HZ  那么1LSB=40ms 设置延时 1s,对应25->0x19 */
	//	lis2dh12_iic_write_byte(0x33, 0x03);  /* INT1_DURATION(33h): 1LSB=1/ODR  如果ODR=50HZ   那么1LSB=20ms 设置延时 1s,对应50->0x32 */
	lis2dh12_spi_write_byte(0x33, 0x03); /* INT1_DURATION(33h): 1LSB=1/ODR  如果ODR=100HZ  那么1LSB=10ms 设置延时 1s,对应100->0x64 */

	//	/* INT2 单击中断 */
	//	lis2dh12_iic_write_byte(0x24, 0x01);	/* CTRL_REG5(24h):  */
	//	lis2dh12_iic_write_byte(0x25, 0xa0);  /* CTRL_REG6(25h): Click interrupt on INT2 pin */
	//
	//	lis2dh12_iic_write_byte(0x38, 0x15);	/* CLICK_CFG (38h): 单击识别中断使能 */
	//	lis2dh12_iic_write_byte(0x39, 0x10);
	//	lis2dh12_iic_write_byte(0x3a, 0x7f);  /* CLICK_THS (3Ah): 单击阀值 */
	//	lis2dh12_iic_write_byte(0x3b, 0xff);  /* TIME_LIMIT (3Bh): 时间限制窗口6 ODR 1LSB=1/ODR 1LSB=1/100HZ,10ms,设置延时1s,对应100—>0x64*/
	//	lis2dh12_iic_write_byte(0x3c, 0xff);  /* TIME_LATENCY (3Ch): 中断电平持续时间1 ODR=10ms */
	//	lis2dh12_iic_write_byte(0x3d, 0x01);  /* TIME_WINDOW (3Dh):  单击时间窗口 */

	/* Start sensor */
	// lis2dh12_iic_write_byte(0x20, 0x37);
	// lis2dh12_iic_write_byte(0x20, 0x5f); /* CTRL_REG1(20h): Start sensor at ODR 100Hz Low-power mode */
	if (!device_t.powerState)
		lis2dh12_spi_write_byte(0x20, 0x0F); // POWER DOWN mode
	else
		lis2dh12_spi_write_byte(0x20, 0x4F); /* CTRL_REG1(20h): Start sensor at ODR 50Hz Low-power mode */

	return 0;
}

/* ============================================================
func name   :  get_acc_value
discription :  获取加速度值
param       :  axis_info_t *sample
return      :  void
Revision    :
=============================================================== */
void get_acc_value(axis_info_t *sample)
{
	uint8_t i = 0;
	uint8_t data[6];
	for (i = 0; i < 6; i++)
	{
		lis2dh12_spi_read_byte(0x28 + i, data + i); //获取X、y、z轴的数据
	}
	//  sample->x = abs((int)(LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t*)data)));
	//	sample->y = abs((int)(LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t*)(data+2))));
	//	sample->z = abs((int)(LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t*)(data+4))));
	sample->x = LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t *)data);
	sample->y = LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t *)(data + 2));
	sample->z = LIS2DH12_FROM_FS_2g_HR_TO_mg(*(int16_t *)(data + 4));
}

/* ============================================================
func name   :  filter_calculate
discription :  均值滤波器---滤波
			   读取xyz数据存入均值滤波器，存满进行计算，滤波后样本存入sample
param       :  filter_avg_t *filter, axis_info_t *sample
return      :  void
Revision    :
=============================================================== */
void filter_calculate(filter_avg_t *filter, axis_info_t *sample)
{
	uint8_t i = 0;
	short x_sum = 0, y_sum = 0, z_sum = 0;

	for (i = 0; i < FILTER_CNT; i++)
	{
		get_acc_value(sample);

		filter->info[i].x = sample->x;
		filter->info[i].y = sample->y;
		filter->info[i].z = sample->z;

		x_sum += filter->info[i].x;
		y_sum += filter->info[i].y;
		z_sum += filter->info[i].z;

		INFO("acc_x:%d,  acc_y:%d,  acc_z:%d \n", filter->info[i].x, filter->info[i].y, filter->info[i].z);
	}
	sample->x = x_sum / FILTER_CNT;
	sample->y = y_sum / FILTER_CNT;
	sample->z = z_sum / FILTER_CNT;
	INFO("\r\nacc_info.acc_x:%d, acc_info.acc_y:%d, acc_info.acc_z:%d \r\n", sample->x, sample->y, sample->z);
}

/* ============================================================
func name   :  new_angle_calculate
discription :  计算新角度
param       :  axis_info_t *sample
return      :  void
Revision    :
=============================================================== */
void new_angle_calculate(axis_info_t *sample)
{
	sample->new_angle_x = atan((short)sample->x / (short)sqrt(pow(sample->y, 2) + pow(sample->z, 2))) * DEGREE_CAL;
	sample->new_angle_y = atan((short)sample->y / (short)sqrt(pow(sample->x, 2) + pow(sample->z, 2))) * DEGREE_CAL;
	sample->new_angle_z = atan((short)sample->z / (short)sqrt(pow(sample->x, 2) + pow(sample->y, 2))) * DEGREE_CAL;
	if (sample->new_angle_z < 0)
	{
		sample->new_angle_x = 180 - sample->new_angle_x;
		sample->new_angle_y = 180 - sample->new_angle_y;
	}
	INFO("sample->new_angle_x:%d, sample->new_angle_y:%d, sample->new_angle_z:%d \r\n", sample->new_angle_x, sample->new_angle_y, sample->new_angle_z);
}

/* ============================================================
func name   :  old_angle_calculate
discription :  计算旧角度
param       :  axis_info_t *sample
return      :  void
Revision    :
=============================================================== */
void old_angle_calculate(axis_info_t *sample)
{
	sample->old_angle_x = atan((short)sample->x / (short)sqrt(pow(sample->y, 2) + pow(sample->z, 2))) * DEGREE_CAL;
	sample->old_angle_y = atan((short)sample->y / (short)sqrt(pow(sample->x, 2) + pow(sample->z, 2))) * DEGREE_CAL;
	sample->old_angle_z = atan((short)sample->z / (short)sqrt(pow(sample->x, 2) + pow(sample->y, 2))) * DEGREE_CAL;
	if (sample->old_angle_z < 0)
	{
		sample->old_angle_x = 180 - sample->old_angle_x;
		sample->old_angle_y = 180 - sample->old_angle_y;
	}
	INFO("sample->old_angle_x:%d, sample->old_angle_y:%d, sample->old_angle_z:%d \r\n", sample->old_angle_x, sample->old_angle_y, sample->old_angle_z);
}

void lis2dh12_handler(void)
{
	static u8 cnt, sec, chk_cnt;
	static int timer;
	axis_info_t acc_info = {0};
	float roll, pitch;

	if (!device_t.powerState)
		lis2dh12_offset_t.event = ACC_POWER_ON_EVENT;
	switch (lis2dh12_offset_t.event)
	{
	case ACC_POWER_ON_EVENT:
		cnt = 0;
		timer = 0;
		memset(&acc_sample, 0, sizeof(acc_sample));
		if (device_t.powerState) //开机状态
			lis2dh12_offset_t.event = ACC_OFFSET_ZERO_EVENT;
		break;
	case ACC_OFFSET_ZERO_EVENT:	  //获取初始位移
		if (device_t.accIntState) //触发中断清除历史采集数据
		{
			device_t.accIntState = 0;
			cnt = 0;
			sec = 0;
			lis2dh12_offset_t.move_x = 0;
			lis2dh12_offset_t.move_y = 0;
			lis2dh12_offset_t.move_z = 0;
		}

		if (timer <= get_syspant_ms()) // 20ms 获取一次加速计数据
		{
			timer = get_syspant_ms() + 20;

			get_acc_value(&acc_info);
			lis2dh12_offset_t.x_mg[0] = acc_info.x;
			lis2dh12_offset_t.y_mg[0] = acc_info.y;
			lis2dh12_offset_t.z_mg[0] = acc_info.z;

			float x_mg = fabs(lis2dh12_offset_t.x_mg[0] - lis2dh12_offset_t.x_mg[1]);
			float y_mg = fabs(lis2dh12_offset_t.y_mg[0] - lis2dh12_offset_t.y_mg[1]);
			float z_mg = fabs(lis2dh12_offset_t.z_mg[0] - lis2dh12_offset_t.z_mg[1]);
			// INFO("\r\nmove x mg:%.2f, move y mg:%.2f, move z mg:%.2f\n", x_mg, y_mg, z_mg);

			//判断是否出现高加速度
			if (x_mg > 16 * 3 || y_mg > 16 * 3 || z_mg > 16 * 3)
			{
				lis2dh12_offset_t.move_x = 0;
				lis2dh12_offset_t.move_y = 0;
				lis2dh12_offset_t.move_z = 0;
				cnt = 0;
				sec = 0;
				INFO("\r\nDevice move too larger, wait stable\n");
			}
			else
			{
				lis2dh12_offset_t.move_x += lis2dh12_offset_t.x_mg[0] / 25;
				lis2dh12_offset_t.move_y += lis2dh12_offset_t.y_mg[0] / 25;
				lis2dh12_offset_t.move_z += lis2dh12_offset_t.z_mg[0] / 25;
			}

			//无条件移位
			lis2dh12_offset_t.x_mg[1] = lis2dh12_offset_t.x_mg[0];
			lis2dh12_offset_t.y_mg[1] = lis2dh12_offset_t.y_mg[0];
			lis2dh12_offset_t.z_mg[1] = lis2dh12_offset_t.z_mg[0];
			if (++cnt >= 25)
			{
				cnt = 0;
				if (++sec >= 10) //连续5秒未检测到大加速度
				{
					sec = 0;
					lis2dh12_offset_t.current_x_offset = lis2dh12_offset_t.move_x / 10;
					lis2dh12_offset_t.current_y_offset = lis2dh12_offset_t.move_y / 10;
					lis2dh12_offset_t.current_z_offset = lis2dh12_offset_t.move_z / 10;
					lis2dh12_offset_t.event = ACC_OLD_ANGLE_EVENT;

					lis2dh12_offset_t.move_x = 0;
					lis2dh12_offset_t.move_y = 0;
					lis2dh12_offset_t.move_z = 0;
					INFO("\r\ncurrent x offset:%.2f, current y offset:%.2f, current z offset:%.2f\n", lis2dh12_offset_t.current_x_offset, lis2dh12_offset_t.current_y_offset, lis2dh12_offset_t.current_z_offset);
				}
			}
			if (device_t.sleepDelay < 3)
				device_t.sleepDelay = 3;
		}
		break;
	case ACC_OLD_ANGLE_EVENT: //获取睡眠前角度
		get_acc_value(&acc_info);
		roll = atan2f(acc_info.x, acc_info.z) * DEG_PER_RAD;
		pitch = atan2f(-acc_info.y, acc_info.z) * DEG_PER_RAD;

		if (roll < 0)
			roll += 180;
		if (pitch < 0)
			pitch += 180;
		INFO("\r\nold roll:%.2f, old pitch:%.2f\n", roll, pitch);
		lis2dh12_offset_t.old_roll = roll;
		lis2dh12_offset_t.old_pitch = pitch;
		lis2dh12_offset_t.event = ACC_RUN_EVENT; //切换到正常运行模式
		// lis2dh12_offset_t.event = ACC_STANDY_BY_EVENT;
		break;
	case ACC_MOVE_CHECK_EVENT:		   //移动检测
		if (timer <= get_syspant_ms()) // 20ms 获取一次加速计数据
		{
			timer = get_syspant_ms() + 20;
			get_acc_value(&acc_info);

			lis2dh12_offset_t.move_x += (float)acc_info.x / 25;
			lis2dh12_offset_t.move_y += (float)acc_info.y / 25;
			lis2dh12_offset_t.move_z += (float)acc_info.z / 25;

			if (++cnt >= 25) // 500ms判断一次加速计位移
			{
				cnt = 0;

				float x1 = fabs(lis2dh12_offset_t.move_x - lis2dh12_offset_t.current_x_offset);
				float y1 = fabs(lis2dh12_offset_t.move_y - lis2dh12_offset_t.current_y_offset);
				float z1 = fabs(lis2dh12_offset_t.move_z - lis2dh12_offset_t.current_z_offset);
				INFO("\r\nmove x offset:%.2f, move y offset:%.2f, move z offset:%.2f\n", x1, y1, z1);

				if (x1 <= 50 && y1 <= 50 && z1 <= 80) //加速度过小
				// if (x1 <= 50 && y1 <= 50) //加速度过小
				{
					lis2dh12_offset_t.move_x = 0;
					lis2dh12_offset_t.move_y = 0;
					lis2dh12_offset_t.move_z = 0;
					sec = 0;

					INFO("\r\nDevice move too small %d\n", chk_cnt++);
					if (chk_cnt >= 20) // 10秒未检测到位移
					{
						chk_cnt = 0;
						lis2dh12_offset_t.event = ACC_NEW_ANGLE_EVENT; //持续移动时间不满足，直接判断角度
						device_t.sensor_t.move_state = 0;
						return;
					}
				}
				else
				{
					// INFO("\r\nmove x offset:%.2f, move y offset:%.2f, move z offset:%.2f\n", x1, y1, z1);
					lis2dh12_offset_t.move_x = 0;
					lis2dh12_offset_t.move_y = 0;
					lis2dh12_offset_t.move_z = 0;
					chk_cnt = 0;

					if (++sec >= 7) //连续3.5秒内产生过大加速度
					{
						sec = 0;
						lis2dh12_offset_t.event = ACC_NEW_ANGLE_EVENT;
						device_t.sensor_t.move_state = 1;
						INFO("\r\nDevice Has Moved!!!\n");
					}
				}
			}
			if (device_t.sleepDelay < 3)
				device_t.sleepDelay = 3;
		}
		break;
	case ACC_NEW_ANGLE_EVENT:
		get_acc_value(&acc_info);
		roll = atan2f(acc_info.x, acc_info.z) * DEG_PER_RAD;
		pitch = atan2f(-acc_info.y, acc_info.z) * DEG_PER_RAD;

		if (roll < 0)
			roll += 180;
		if (pitch < 0)
			pitch += 180;
		INFO("\r\nnew roll:%.2f, new pitch:%.2f\n", roll, pitch);
		lis2dh12_offset_t.new_roll = roll;
		lis2dh12_offset_t.new_pitch = pitch;
		lis2dh12_offset_t.event = ACC_OFFSET_ZERO_EVENT; //重新获取对比数据
		lis2dh12_offset_t.move_x = 0;
		lis2dh12_offset_t.move_y = 0;
		lis2dh12_offset_t.move_z = 0;

		if ((fabs(lis2dh12_offset_t.new_pitch - lis2dh12_offset_t.old_pitch) > 25 && fabs(lis2dh12_offset_t.new_pitch - lis2dh12_offset_t.old_pitch) < 155) ||
			(fabs(lis2dh12_offset_t.new_roll - lis2dh12_offset_t.old_roll) > 25 && fabs(lis2dh12_offset_t.new_roll - lis2dh12_offset_t.old_roll) < 155) ||
			device_t.sensor_t.move_state)
		{
			DEBUG_TRACE(LOG_TAG, "Wake up from lis2dh12, old roll: %.2f, new roll: %.2f, old pitch: %.2f, new pitch: %.2f, move state: %d",
						lis2dh12_offset_t.old_roll, lis2dh12_offset_t.new_roll, lis2dh12_offset_t.old_pitch, lis2dh12_offset_t.new_pitch, device_t.sensor_t.move_state);
			device_t.getGpsInfo = 1;
			sensor_stamp = Timestamp;
		}
		else //未出现位移或角度变化，直接退出
		{
			lis2dh12_offset_t.event = ACC_STANDY_BY_EVENT;
		}
		break;
	case ACC_RUN_EVENT:
		if (device_t.accIntState) //触发中断
		{
			lis2dh12_offset_t.event = ACC_OFFSET_ZERO_EVENT;
		}
		break;
	case ACC_STANDY_BY_EVENT:	  //睡眠待机模式
		if (device_t.accIntState) //触发中断
		{
			device_t.accIntState = 0;
			lis2dh12_offset_t.event = ACC_MOVE_CHECK_EVENT;
			lis2dh12_offset_t.move_x = 0;
			lis2dh12_offset_t.move_y = 0;
			lis2dh12_offset_t.move_z = 0;
			cnt = 0;
			sec = 0;
			INFO("\r\n Acc standby to acc move check event");
		}
		break;
	default:
		break;
	}
}
