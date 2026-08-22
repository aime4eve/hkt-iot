
/* Includes ------------------------------------------------------------------*/
#include "acc.h"
#include "uart.h"
#include "systick.h"
#include "control_center.h"
#include "lis3dh.h"
#include "lsm6ds3_reg.h"
#include "math.h"

#define ABS(a) (0 - (a)) > 0 ? (-(a)) : (a)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// static void convert_angle(peak_value_t *peak, axis_info_t *cur_sample);

float tilt_angle; // 倾斜角

peak_value_t acc_peak;
filter_avg_t acc_filter;
axis_info_t acc_current;
slid_reg_t acc_slid;

// 初始化最大值与最小值
static void peak_value_init(peak_value_t *peak)
{
	peak->newmax.x = 0;
	peak->newmax.y = 0;
	peak->newmax.z = 0;

	peak->newmin.x = 65535;
	peak->newmin.y = 65535;
	peak->newmin.z = 65535;

	peak->oldmax.x = 0;
	peak->oldmax.y = 0;
	peak->oldmax.z = 0;

	peak->oldmin.x = 65535;
	peak->oldmin.y = 65535;
	peak->oldmin.z = 65535;
}

// 读取xyz数据存入均值滤波器，存满进行计算，滤波后样本存入sample
static void filter_calculate(filter_avg_t *filter, axis_info_t *sample)
{
	unsigned int i;
	short x_sum = 0, y_sum = 0, z_sum = 0;
	for (i = 0; i < FILTER_CNT; i++)
	{
		x_sum += filter->info[i].x;
		y_sum += filter->info[i].y;
		z_sum += filter->info[i].z;
	}
	sample->x = x_sum / FILTER_CNT;
	sample->y = y_sum / FILTER_CNT;
	sample->z = z_sum / FILTER_CNT;
}

// 在动态阈值结构体初始化时，一定要将max的值都赋值为最小值，min赋值为最大值，这样才有利于动态更新。
static void peak_update(peak_value_t *peak, axis_info_t *cur_sample)
{
	static unsigned int sample_size = 0;
	sample_size++;
	// if (sample_size >= SAMPLE_SIZE)
	{
		/*采样达到50个，更新一次*/
		sample_size = 0;
		peak->oldmax = peak->newmax;
		peak->oldmin = peak->newmin;
		// 初始化
		//  peak_value_init(peak);
	}
	peak->newmax.x = MAX(peak->newmax.x, cur_sample->x);
	peak->newmax.y = MAX(peak->newmax.y, cur_sample->y);
	peak->newmax.z = MAX(peak->newmax.z, cur_sample->z);

	peak->newmin.x = MIN(peak->newmin.x, cur_sample->x);
	peak->newmin.y = MIN(peak->newmin.y, cur_sample->y);
	peak->newmin.z = MIN(peak->newmin.z, cur_sample->z);
}

/*过滤高频噪声*/
static char slid_update(slid_reg_t *slid, axis_info_t *cur_sample)
{
	char res = 0;
	if (ABS((cur_sample->x - slid->new_sample.x)) > DYNAMIC_PRECISION)
	{
		slid->old_sample.x = slid->new_sample.x;
		slid->new_sample.x = cur_sample->x;
		res = 1;
	}
	else
	{
		slid->old_sample.x = slid->new_sample.x;
	}
	if (ABS((cur_sample->y - slid->new_sample.y)) > DYNAMIC_PRECISION)
	{
		slid->old_sample.y = slid->new_sample.y;
		slid->new_sample.y = cur_sample->y;
		res = 1;
	}
	else
	{
		slid->old_sample.y = slid->new_sample.y;
	}

	if (ABS((cur_sample->z - slid->new_sample.z)) > DYNAMIC_PRECISION)
	{
		slid->old_sample.z = slid->new_sample.z;
		slid->new_sample.z = cur_sample->z;
		res = 1;
	}
	else
	{
		slid->old_sample.z = slid->new_sample.z;
	}
	return res;
}

/*判断当前最活跃轴*/
static char is_most_active(peak_value_t *peak)
{
	char res = MOST_ACTIVE_NULL;
	short x_change = ABS((peak->newmax.x - peak->newmin.x));
	short y_change = ABS((peak->newmax.y - peak->newmin.y));
	short z_change = ABS((peak->newmax.z - peak->newmin.z));

	if (x_change > y_change && x_change > z_change && x_change >= ACTIVE_PRECISION)
	{
		res = MOST_ACTIVE_X;
	}
	else if (y_change > x_change && y_change > z_change && y_change >= ACTIVE_PRECISION)
	{
		res = MOST_ACTIVE_Y;
	}
	else if (z_change > x_change && z_change > y_change && z_change >= ACTIVE_PRECISION)
	{
		res = MOST_ACTIVE_Z;
	}
	return res;
}

/*判断是否走步*/
static void detect_step(peak_value_t *peak, slid_reg_t *slid)
{
	char res = is_most_active(peak);
	// DEBUG_TRACE(LOG_TAG, "action axis res: %d", res);
	switch (res)
	{
	case MOST_ACTIVE_NULL:
	{
		// fix
		break;
	}
	case MOST_ACTIVE_X:
	{
		short threshold_x = (peak->oldmax.x + peak->oldmin.x) / 2;
		if (slid->old_sample.x > threshold_x && slid->new_sample.x < threshold_x)
		{
			device_t.sensor_t.step++;
			DEBUG_TRACE(LOG_TAG, "action x axis total step: %d", device_t.sensor_t.step);
		}
		break;
	}
	case MOST_ACTIVE_Y:
	{
		short threshold_y = (peak->oldmax.y + peak->oldmin.y) / 2;
		if (slid->old_sample.y > threshold_y && slid->new_sample.y < threshold_y)
		{
			device_t.sensor_t.step++;
			DEBUG_TRACE(LOG_TAG, "action y axis total step: %d", device_t.sensor_t.step);
		}
		break;
	}
	case MOST_ACTIVE_Z:
	{
		short threshold_z = (peak->oldmax.z + peak->oldmin.z) / 2;
		if (slid->old_sample.z > threshold_z && slid->new_sample.z < threshold_z)
		{
			device_t.sensor_t.step++;
			DEBUG_TRACE(LOG_TAG, "action z axis total step: %d", device_t.sensor_t.step);
		}
		break;
	}
	default:
		break;
	}
}

/*计算倾斜角*/
static void convert_angle(peak_value_t *peak, axis_info_t *cur_sample)
{
	peak->newmax.tilt_angle = (float)atan2(peak->newmax.y, peak->newmax.z) * DEG_PER_RAD;
	peak->newmin.tilt_angle = (float)atan2(peak->newmin.y, peak->newmin.z) * DEG_PER_RAD;
	peak->oldmax.tilt_angle = (float)atan2(peak->oldmax.y, peak->oldmax.z) * DEG_PER_RAD;
	peak->oldmin.tilt_angle = (float)atan2(peak->oldmin.y, peak->oldmin.z) * DEG_PER_RAD;
	cur_sample->tilt_angle = (float)atan2(cur_sample->y, cur_sample->z) * DEG_PER_RAD;
}

lis3dh_sensor_t *acc_sensor;
stmdev_ctx_t lsm6ds3_t;

static int32_t lsm6ds3_iic_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);
static int32_t lsm6ds3_iic_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len);

/**
 * Common function used to get sensor data.
 */
void read_data()
{
#ifdef FIFO_MODE
	lis3dh_float_data_fifo_t fifo;
	if (lis3dh_new_data(acc_sensor))
	{
		uint8_t num = lis3dh_get_float_data_fifo(acc_sensor, fifo);
		INFO("LIS3DH num=%d\n", num);
		for (int i = 0; i < num; i++)
			// max. full scale is +-16 g and best resolution is 1 mg, i.e. 5 digits
			INFO("LIS3DH (xyz)[g] ax=%+7.3f ay=%+7.3f az=%+7.3f\n",
				 fifo[i].ax, fifo[i].ay, fifo[i].az);
	}
#else
	lis3dh_float_data_t data;
	if (lis3dh_new_data(acc_sensor) && lis3dh_get_float_data(acc_sensor, &data))
		// max. full scale is +-16 g and best resolution is 1 mg, i.e. 5 digits
		INFO("LIS3DH (xyz)[g] ax=%+7.3f ay=%+7.3f az=%+7.3f\n",
			 data.ax, data.ay, data.az);
	INFO("LIS3DH (xyz)[g] ax=%+7.3f ay=%+7.3f az=%+7.3f\n", data.ax, data.ay, data.az);
#endif // FIFO_MODE
}

void Acc_Handler(void)
{
	static u16 total, filter_count;
#ifdef FIFO_MODE // FIFO 设置为50hz 10条数据溢出 200ms
	if (!device_t.lis3d_int || !device_t.powerState)
		return;
	device_t.lis3d_int = 0;
	INFO("lis3dh int event\n");

	lis3dh_int_data_source_t data_src = {0};
	lis3dh_float_data_fifo_t fifo;

	// get the source of the interrupt and reset *INTx* signals
	lis3dh_get_int_data_source(acc_sensor, &data_src);

	if (lis3dh_new_data(acc_sensor))
	{
		u8 num = lis3dh_get_float_data_fifo(acc_sensor, fifo);
		INFO("LIS3DH num=%d\n", num);
		for (int i = 0; i < num; i++)
			// max. full scale is +-16 g and best resolution is 1 mg, i.e. 5 digits
			INFO("LIS3DH (xyz)[g] ax=%+7.3f ay=%+7.3f az=%+7.3f\n",
				 fifo[i].ax, fifo[i].ay, fifo[i].az);

		// acc_filter.info[filter_count].x = raw_buf[1] << 8 | raw_buf[0];
		// acc_filter.info[filter_count].y = raw_buf[3] << 8 | raw_buf[2];
		// acc_filter.info[filter_count].z = raw_buf[5] << 8 | raw_buf[4];
		// if (++filter_count >= 5)
		// {
		// 	filter_count = 0;
		// 	filter_calculate(&acc_filter, &acc_current);
		// 	slid_update(&acc_slid, &acc_current);
		// 	peak_update(&acc_peak, &acc_current);
		// 	total++;
		// 	if (total >= SAMPLE_SIZE)
		// 	{
		// 		total = 0;
		// 		detect_step(&acc_peak, &acc_slid);
		// 		peak_value_init(&acc_peak);
		// 		memset(&acc_slid, 0, sizeof(acc_slid));
		// 	}
		// }
	}
#else
	static u32 covert_time;
	lis3dh_raw_data_t raw;

	if (covert_time >= get_syspant_ms() || !device_t.powerState)
		return;
	if (!lis3dh_new_data(acc_sensor))
		return;
	covert_time = get_syspant_ms() + 20;
	if (covert_time >= 86400000)
		covert_time = 0;

	// read raw data sample
	if (lis3dh_get_raw_data(acc_sensor, &raw) == false)
		return;
	// INFO("x= %d, y= %d, z=%d\n", raw.ax, raw.ay, raw.az);
	// INFO("%d,%d,%d\n", raw.ax, raw.ay, raw.az);

	// 消除异常抖动带来的影响
	if (raw.ax > 5736)
		raw.ax = ACTIVE_PRECISION;
	if (raw.ax < -5736)
		raw.ax = -ACTIVE_PRECISION;
	if (raw.ay > 5736)
		raw.ay = ACTIVE_PRECISION;
	if (raw.ay < -5736)
		raw.ay = -ACTIVE_PRECISION;
	if (raw.az > 5736)
		raw.az = ACTIVE_PRECISION;
	if (raw.az < -5736)
		raw.az = -ACTIVE_PRECISION;

	// 产生有效动作
	if (abs(raw.ax) >= ACTIVE_PRECISION || abs(raw.ay) >= ACTIVE_PRECISION || abs(raw.az) >= ACTIVE_PRECISION)
	{
		acc_filter.info[filter_count].x = raw.ax;
		acc_filter.info[filter_count].y = raw.ay;
		acc_filter.info[filter_count].z = raw.az;

		if (++filter_count >= FILTER_CNT)
		{
			filter_count = 0;
			filter_calculate(&acc_filter, &acc_current);
			slid_update(&acc_slid, &acc_current);
			peak_update(&acc_peak, &acc_current);
			total++;
			if (total >= SAMPLE_SIZE)
			{
				total = 0;
				detect_step(&acc_peak, &acc_slid); 
				peak_value_init(&acc_peak);
				memset(&acc_slid, 0, sizeof(acc_slid));
			}
		}
	}
	else
	{
		filter_count = 0;
		total = 0;
	}
#endif
}

void Acc_Init(void)
{
#if 1
	acc_sensor = lis3dh_init_sensor(LIS3DH_I2C_ADDRESS_2, 0);
	if (acc_sensor)
	{
		DEBUG_TRACE(LOG_TAG, "Acc Sensor Init Success");

		// enable data interrupts on INT1
		lis3dh_int_event_config_t event_config;

		// event_config.mode = lis3dh_wake_up;
		// event_config.mode = lis3dh_free_fall;
		// event_config.mode = lis3dh_6d_movement;
		// event_config.mode = lis3dh_6d_position;
		// event_config.mode = lis3dh_4d_movement;
		// event_config.mode = lis3dh_4d_position;
		// event_config.threshold = 10;
		// event_config.x_low_enabled = false;
		// event_config.x_high_enabled = true;
		// event_config.y_low_enabled = false;
		// event_config.y_high_enabled = true;
		// event_config.z_low_enabled = false;
		// event_config.z_high_enabled = true;
		// event_config.duration = 0;
		// event_config.latch = true;
#ifdef FIFO_MODE
		// lis3dh_set_int_event_config(acc_sensor, &event_config, lis3dh_int_event1_gen);
		lis3dh_enable_int(acc_sensor, lis3dh_int_fifo_watermark, lis3dh_int1_signal, 10); // fifo mode 超过10次触发中断

		// clear FIFO and activate FIFO mode if needed
		lis3dh_set_fifo_mode(acc_sensor, lis3dh_fifo, 0, lis3dh_int1_signal);
		// lis3dh_set_fifo_mode (acc_sensor, lis3dh_stream, 10, lis3dh_int1_signal);

		// configure HPF and reset the reference by dummy read
		// lis3dh_config_hpf(acc_sensor, lis3dh_hpf_normal, 0, true, true, true, true);
		lis3dh_config_hpf(acc_sensor, lis3dh_hpf_normal, 1, true, true, true, false);
		lis3dh_get_hpf_ref(acc_sensor);

		// enable ADC inputs and temperature sensor for ADC input 3
		// lis3dh_enable_adc(acc_sensor, true, true);
		lis3dh_enable_adc(acc_sensor, false, false);

		// LAST STEP: Finally set scale and mode to start measurements
		lis3dh_set_scale(acc_sensor, lis3dh_scale_2_g);
		lis3dh_set_mode(acc_sensor, lis3dh_odr_50, lis3dh_low_power, true, true, true);
		// lis3dh_set_mode(acc_sensor, lis3dh_odr_100, lis3dh_high_res, true, true, true);

		lis3dh_int_data_source_t data_src = {0};
		// lis3dh_int_event_source_t event_src = {0};
		// lis3dh_int_click_source_t click_src = {0};

		// get the source of the interrupt and reset *INTx* signals
		lis3dh_get_int_data_source(acc_sensor, &data_src);
		// lis3dh_get_int_event_source (acc_sensor, &event_src, lis3dh_int_event1_gen);
		// lis3dh_get_int_click_source (acc_sensor, &click_src);
#else
		// configure HPF and reset the reference by dummy read
		lis3dh_config_hpf(acc_sensor, lis3dh_hpf_normal, 0, true, true, true, true);
		lis3dh_get_hpf_ref(acc_sensor);

		// enable ADC inputs and temperature sensor for ADC input 3
		lis3dh_enable_adc(acc_sensor, true, true);

		// LAST STEP: Finally set scale and mode to start measurements
		lis3dh_set_scale(acc_sensor, lis3dh_scale_2_g);
		lis3dh_set_mode(acc_sensor, lis3dh_odr_50, lis3dh_low_power, true, true, true);
#endif
		peak_value_init(&acc_peak);

		// while (1)
		//{
		//  in case of DRDY interrupt or inertial event interrupt read one data sample
		//  if (data_src.data_ready)
		//  	read_data();
		//  // in case of FIFO interrupts read the whole FIFO
		//  else  if (data_src.fifo_watermark || data_src.fifo_overrun)
		//      read_data ();

		// // in case of event interrupt
		// else if (event_src.active)
		// {
		//     INFO("%.3f LIS3DH ", (double)sdk_system_get_time()*1e-3);
		//     if (event_src.x_low)  INFO("x is lower than threshold\n");
		//     if (event_src.y_low)  INFO("y is lower than threshold\n");
		//     if (event_src.z_low)  INFO("z is lower than threshold\n");
		//     if (event_src.x_high) INFO("x is higher than threshold\n");
		//     if (event_src.y_high) INFO("y is higher than threshold\n");
		//     if (event_src.z_high) INFO("z is higher than threshold\n");
		// }

		// // in case of click detection interrupt
		// else if (click_src.active)
		//    INFO("%.3f LIS3DH %s\n", (double)sdk_system_get_time()*1e-3,
		//           click_src.s_click ? "single click" : "double click");
		//	read_data();
		//	Acc_Handler();
		//}

		if (!device_t.powerState) // 切换待机状态
			lis3dh_set_mode(acc_sensor, lis3dh_power_down, lis3dh_low_power, true, true, true);
	}
#else

	lsm6ds3_t.write_reg = lsm6ds3_iic_write;
	lsm6ds3_t.read_reg = lsm6ds3_iic_read;
	uint32_t i = 0;
	uint8_t whoamI;

	mDelay(50);
	lsm6ds3_device_id_get(&lsm6ds3_t, &whoamI);
	if (whoamI != LSM6DS3_ID)
	{
		DEBUG_TRACE(ERROR_TAG, "Acc Sensor Init Fail");
	}
	/* Restore default configuration */
	lsm6ds3_reset_set(&lsm6ds3_t, PROPERTY_ENABLE);
	platform_delay(1);

	lsm6ds3_reset_set(&lsm6ds3_t, PROPERTY_ENABLE);
	platform_delay(1);

	lsm6ds3_reset_get(&lsm6ds3_t, &rst);

	/*  Enable Block Data Update */
	lsm6ds3_block_data_update_set(&lsm6ds3_t, PROPERTY_ENABLE);
	/* Set full scale */
	lsm6ds3_xl_full_scale_set(&lsm6ds3_t, LSM6DS3_2g);
	lsm6ds3_gy_full_scale_set(&lsm6ds3_t, LSM6DS3_2000dps);
	/* Set Output Data Rate for Acc and Gyro */
	lsm6ds3_xl_data_rate_set(&lsm6ds3_t, LSM6DS3_XL_ODR_12Hz5);
	lsm6ds3_gy_data_rate_set(&lsm6ds3_t, LSM6DS3_GY_ODR_12Hz5);

	/* Read samples in polling mode (no int) */
	while (1)
	{
		uint8_t reg;
		/* Read output only if new value is available */
		lsm6ds3_xl_flag_data_ready_get(&lsm6ds3_t, &reg);

		if (reg)
		{
			/* Read acceleration field data */
			memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
			lsm6ds3_acceleration_raw_get(&lsm6ds3_t, data_raw_acceleration);
			acceleration_mg[0] =
				lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[0]);
			acceleration_mg[1] =
				lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[1]);
			acceleration_mg[2] =
				lsm6ds3_from_fs2g_to_mg(data_raw_acceleration[2]);
			sprintf((char *)tx_buffer,
					"Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n",
					acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
			tx_com(tx_buffer, strlen((char const *)tx_buffer));
		}

		lsm6ds3_gy_flag_data_ready_get(&lsm6ds3_t, &reg);

		if (reg)
		{
			/* Read angular rate field data */
			memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
			lsm6ds3_angular_rate_raw_get(&lsm6ds3_t, data_raw_angular_rate);
			angular_rate_mdps[0] =
				lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[0]);
			angular_rate_mdps[1] =
				lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[1]);
			angular_rate_mdps[2] =
				lsm6ds3_from_fs2000dps_to_mdps(data_raw_angular_rate[2]);
			sprintf((char *)tx_buffer,
					"Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n",
					angular_rate_mdps[0],
					angular_rate_mdps[1],
					angular_rate_mdps[2]);
			//   INFO(tx_buffer, strlen((char const *)tx_buffer));
		}

		lsm6ds3_temp_flag_data_ready_get(&lsm6ds3_t, &reg);

		if (reg)
		{
			/* Read temperature data */
			memset(&data_raw_temperature, 0x00, sizeof(int16_t));
			lsm6ds3_temperature_raw_get(&lsm6ds3_t, &data_raw_temperature);
			temperature_degC =
				lsm6ds3_from_lsb_to_celsius(data_raw_temperature);
			sprintf((char *)tx_buffer,
					"Temperature [degC]:%6.2f\r\n",
					temperature_degC);
			//   INFO(tx_buffer, strlen((char const *)tx_buffer));
		}
	}
#endif
}

static int32_t lsm6ds3_iic_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	// 等待iic空闲
	while (LL_I2C_IsActiveFlag_BUSY(I2C1))
		;
	// 确认iic是否使能
	if (!LL_I2C_IsEnabled(I2C1))
	{
		LL_I2C_Enable(I2C1);
	}
	// 配置
	LL_I2C_HandleTransfer(I2C1, (LSM6DS3_I2C_ADDRESS << 1) | 1, LL_I2C_ADDRSLAVE_7BIT, len, I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);

	// 读取数据
	for (int i = 0; i < len; i++)
	{
		// 等待接收非空
		while (!LL_I2C_IsActiveFlag_RXNE(I2C1))
			;
		*(bufp + i) = LL_I2C_ReceiveData8(I2C1);
	}

	// 等待停止完成
	while (!LL_I2C_IsActiveFlag_STOP(I2C1))
		;
	LL_I2C_ClearFlag_STOP(I2C1);
	return 0;
}

static int32_t lsm6ds3_iic_write(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
	// 等待iic空闲
	while (LL_I2C_IsActiveFlag_BUSY(I2C1))
		;
	// 确认iic是否使能
	if (!LL_I2C_IsEnabled(I2C1))
	{
		LL_I2C_Enable(I2C1);
	}
	// 配置为写
	LL_I2C_HandleTransfer(I2C1, LSM6DS3_I2C_ADDRESS << 1, LL_I2C_ADDRSLAVE_7BIT, len, I2C_AUTOEND_MODE, I2C_GENERATE_START_WRITE);
	while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
		;
	// 写数据
	for (int i = 0; i < len; i++)
	{
		// 等待发送完成
		while (!LL_I2C_IsActiveFlag_TXIS(I2C1))
			;
		LL_I2C_TransmitData8(I2C1, *(bufp + i));
	}
	// 等待停止完成
	while (!LL_I2C_IsActiveFlag_STOP(I2C1))
		;
	LL_I2C_ClearFlag_STOP(I2C1);
	return 0;
}
