
/* Includes ------------------------------------------------------------------*/
#include "acc.h"
#include "control_center.h"
#include "flash.h"
#include "lis3dh.h"
#include "lsm6ds3_reg.h"
#include "math.h"
#include "spi.h"
#include "systick.h"
#include "uart.h"

filter_avg_t lis3dh_t;
reference_avg_t acc_reference_t;
lis3dh_sensor_t *acc_sensor;

#define ABS(a) (0 - (a)) > 0 ? (-(a)) : (a)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/*交换两个整数的值*/
void swap(short *p, short *q)
{
    short buf;
    buf = *p;
    *p = *q;
    *q = buf;
    return;
}

/*快速排序*/
void quick_sort(short *a, int low, int high)
{
    int i = low;
    int j = high;
    short key = a[low];
    if (low >= high) // 如果low >= high说明排序结束了
    {
        return;
    }
    while (low < high) // 该while循环结束一次表示比较了一轮
    {
        while (low < high && key <= a[high]) {
            --high; // 向前寻找
        }
        if (key > a[high]) {
            swap(&a[low], &a[high]);
            ++low;
        }
        while (low < high && key >= a[low]) {
            ++low; // 向后寻找
        }
        if (key < a[low]) {
            swap(&a[low], &a[high]);
            --high;
        }
    }
    quick_sort(a, i, low - 1); // 用同样的方式对分出来的左边的部分进行同上的做法
    quick_sort(a, low + 1, j); // 用同样的方式对分出来的右边的部分进行同上的做法
}

/*查找一个有序数组中的众数*/
short find_mode_number(short *arr, int len)
{
    short many = 1, less = 1;
    short value = 0;
    for (int i = 0; i < len; i++) {
        for (int j = i; j < len; j++) {
            if (arr[j] == arr[j + 1]) {
                less++;
            } else {
                if (many < less) {
                    swap(&many, &less);
                    value = arr[j];
                }
                less = 1;
                break;
            }
        }
    }
    return value;
}

short getAccCentre(short *accVals, int cnt)
{
    int value = 0;
    for (int i = 0; i < 12 - 6; i++) {
        value += accVals[i + 3];
    }
    value /= 12 - 6;
    return value;
}

// 软件滤波
short getAccRaw(short *accVals, int cnt)
{
    int value_collection = 0;
    short max = accVals[0];
    short min = accVals[0];

    for (int n = 0; n < cnt; n++) {
        max = (accVals[n] < max) ? max : accVals[n];
        min = (min < accVals[n]) ? min : accVals[n];
        value_collection += accVals[n];
    }
    value_collection = (value_collection - max - min) / (cnt - 2);
    if (value_collection == 0)
        value_collection = 1;
    // memset(&accVals, 0, sizeof(accVals));
    return value_collection;
}

// 辅助函数，用于计算两个角度之间的最小差值
double min_angle_diff(double angle1, double angle2)
{
    double diff = fabs(angle1 - angle2);
    return fmin(diff, fabs(360 - diff)); // 返回差值与环绕差值的较小者
}

// 辅助函数，用于规范化角度到[0, 360)范围
double normalize_angle(double angle)
{
    double ang = angle;
    while (ang > 360) {
        ang -= 360;
    }
    while (ang < 0) {
        ang += 360;
    }
    return angle;
}

// 校准参考系数
void acc_offset_scale(void)
{
    if (!device_t.acc_offset_mode)
        return;
    u8 changed;
    u32 offset_timeout, temp_time;
    double x_min = 16383.0;
    double x_max = -16383.0;

    double y_min = 16383.0;
    double y_max = -16383.0;

    double z_min = 16383.0;
    double z_max = -16383.0;
    lis3dh_raw_data_t raw;

    temp_time = get_syspant_ms();
    while (1) {
        if (lis3dh_new_data(acc_sensor) && lis3dh_get_raw_data(acc_sensor, &raw)) {
            if (raw.ax < x_min) {
                x_min = raw.ax;
                changed = 1;
            } else if (raw.ax > x_max) {
                x_max = raw.ax;
                changed = 1;
            }
            if (raw.ay < y_min) {
                y_min = raw.ay;
                changed = 1;
            } else if (raw.ay > y_max) {
                y_max = raw.ay;
                changed = 1;
            }
            if (raw.az < z_min) {
                z_min = raw.az;
                changed = 1;
            } else if (raw.az > z_max) {
                z_max = raw.az;
                changed = 1;
            }

            if (changed) {
                changed = 0;
                offset_timeout = get_syspant_ms();
            }
            temp_time = get_syspant_ms();
            if (abs(temp_time - offset_timeout) > 5000) {
                acc_reference_t.offset.x = (x_min + x_max) / 2.0;
                acc_reference_t.offset.y = (y_min + y_max) / 2.0;
                acc_reference_t.offset.z = (z_min + z_max) / 2.0;

                acc_reference_t.scale.x = 1.0 / (x_max - x_min);
                acc_reference_t.scale.y = 1.0 / (y_max - y_min);
                acc_reference_t.scale.z = 1.0 / (z_max - z_min);
                break;
            }
        }
        mDelay(20); // read xyz max 50hz
        LL_IWDG_ReloadCounter(IWDG);
    }
    device_t.acc_offset_mode = 0;
}

// 校准参考角度
void acc_reference(void)
{
    if (!device_t.angle_offset_mode)
        return;
    u32 start_timer, temp_time, offset_timeout;
    float roll, pitch;
    lis3dh_raw_data_t raw;
    u8 changed = 1;
    temp_time = get_syspant_ms();
    INFO("\nEnter Carliration Mode");
    while (1) {
        if (!check_delay_expired(start_timer, 50))
            continue;
        if (!lis3dh_new_data(acc_sensor) & !lis3dh_get_raw_data(acc_sensor, &raw))
            continue;
        start_timer = get_syspant_ms();
        LL_IWDG_ReloadCounter(IWDG);
        axis_info_t sample;
        sample.x = FS_2g_HR_TO_mg(raw.ax);
        sample.y = FS_2g_HR_TO_mg(raw.ay);
        sample.z = FS_2g_HR_TO_mg(raw.az);
        INFO("\nx= %d, y= %d, z=%d", sample.x, sample.y, sample.z);

        acc_reference_t.roll = (float)(atan2((float)(raw.ax), raw.az) * DEG_PER_RAD);      // 转换为度数
        acc_reference_t.pitch = (float)(atan2((float)(0 - raw.ay), raw.az) * DEG_PER_RAD); // 转换为度数

        // 规范化roll和pitch
        acc_reference_t.roll = normalize_angle(acc_reference_t.roll);
        acc_reference_t.pitch = normalize_angle(acc_reference_t.pitch);

        // 使用min_angle_diff函数来更新roll和pitch（如果需要的话）
        bool roll_changed = min_angle_diff(acc_reference_t.roll, roll) > 2;
        bool pitch_changed = min_angle_diff(acc_reference_t.pitch, pitch) > 3;

        if (roll_changed) {
            roll = acc_reference_t.roll;
        }
        if (pitch_changed) {
            pitch = acc_reference_t.pitch;
        }

        changed = roll_changed || pitch_changed;
        if (changed) {
            changed = 0;
            offset_timeout = get_syspant_ms();
            INFO("\nroll[%.2f][%.2f], pith[%.2f][%.2f]", acc_reference_t.roll, roll, acc_reference_t.pitch, pitch);
        }
        temp_time = get_syspant_ms();
        if (abs(temp_time - offset_timeout) > 3000) // 3秒
            break;
    }
    INFO("\nacc reference: roll[%.2f],  pith[%.2f]\n", acc_reference_t.roll, acc_reference_t.pitch);
    device_t.angle_offset_mode = 0;
    FLASH_Write_Acc_Reference();

    if (device_t.ble_connected) {
        BLE_SendCMD("Calibration Done");
        // callback_BLEAck();
    }
}

/**
 * Common function used to get sensor data.
 */
void read_data()
{
#ifdef FIFO_MODE
    lis3dh_float_data_fifo_t fifo;
    if (lis3dh_new_data(acc_sensor)) {
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
    static u32 covert_time;
    static u16 slant_cnt;
    static float old_angle;

    axis_info_t sample;
    lis3dh_raw_data_t raw;
    lis3dh_raw_data_t raw_cent;
    short x_value[FILTER_CNT], y_value[FILTER_CNT], z_value[FILTER_CNT];

    if (device_t.power_on) {
        acc_offset_scale();
        acc_reference();
    }

    if (!lis3dh_new_data(acc_sensor) & !lis3dh_get_raw_data(acc_sensor, &raw))
        return;

    if (!check_delay_expired(covert_time, 50))
        return;
    covert_time = get_syspant_ms();

    lis3dh_t.info[lis3dh_t.count].x = raw.ax;
    lis3dh_t.info[lis3dh_t.count].y = raw.ay;
    lis3dh_t.info[lis3dh_t.count].z = raw.az;
    lis3dh_t.count++;

    if (lis3dh_t.count >= FILTER_CNT - 1) {
        for (u8 i = 0; i < FILTER_CNT; i++) {
            x_value[i] = lis3dh_t.info[i].x;
            y_value[i] = lis3dh_t.info[i].y;
            z_value[i] = lis3dh_t.info[i].z;
        }
        lis3dh_t.count = 0;
        quick_sort(x_value, 0, sizeof(x_value) / sizeof(short) - 1);
        quick_sort(y_value, 0, sizeof(y_value) / sizeof(short) - 1);
        quick_sort(z_value, 0, sizeof(z_value) / sizeof(short) - 1);
        raw_cent.ax = getAccCentre(x_value, FILTER_CNT);
        raw_cent.ay = getAccCentre(y_value, FILTER_CNT);
        raw_cent.az = getAccCentre(z_value, FILTER_CNT);

        // INFO("\nx= %d, y= %d, z=%d", raw.ax, raw.ay, raw.az);
        // INFO("%d,%d,%d\n", raw.ax, raw.ay, raw.az);

        sample.x = FS_2g_HR_TO_mg(raw_cent.ax);
        sample.y = FS_2g_HR_TO_mg(raw_cent.ay);
        sample.z = FS_2g_HR_TO_mg(raw_cent.az);
        // INFO("\nx= %d, y= %d, z=%d", sample.x, sample.y, sample.z);

        lis3dh_t.roll = (float)(atan2((float)(sample.x), sample.z) * DEG_PER_RAD);  // 转换为度数
        lis3dh_t.pitch = (float)(atan2((float)(sample.y), sample.z) * DEG_PER_RAD); // 转换为度数
        if (lis3dh_t.roll < 0)
            lis3dh_t.roll += 360;
        if (lis3dh_t.pitch < 0)
            lis3dh_t.pitch += 360;
        // device_t.angle = lis3dh_t.pitch;
        device_t.angle = lis3dh_t.roll;

        if (acc_reference_t.roll < 0) {
            acc_reference_t.roll += 360;
        }
        if (acc_reference_t.pitch < 0) {
            acc_reference_t.pitch += 360;
        }

        // 角度变化20度认为盖未关或设备倾斜
        // if (fabs(device_t.angle - acc_reference_t.pitch) > 20) {
        if (fabs(device_t.angle - acc_reference_t.roll) > 20 && fabs(device_t.angle - acc_reference_t.roll) < 340) {
            slant_cnt++;
            if (fabs(device_t.angle - old_angle) > 5) { // 一直在变化
                // device_t.ud_det_cnt = 5 * UD_CONVERT_CNT;
                if (slant_cnt >= 6 && !device_t.is_slant) { // 3秒未有变化上报倾斜角度与状态
                    device_t.is_slant = 1;
                    if (device_t.power_on)
                        device_t.sync_state = 1;
                    old_angle = device_t.angle;
                    uartLoRa.sendDelay = 3 * SEC_DELAY;
                }
            }
        } else {
            if (slant_cnt) {
                slant_cnt--;
                // device_t.ud_det_cnt = 5 * UD_CONVERT_CNT; // 每次从倾斜状态切换为正常状态时都检测一次
                if (!slant_cnt && device_t.is_slant) {
                    device_t.is_slant = 0;
                    if (device_t.power_on)
                        device_t.sync_state = 1;
                    old_angle = device_t.angle;
                    uartLoRa.sendDelay = 3 * SEC_DELAY;
                }
            }
        }
        if (slant_cnt >= 6) {
            slant_cnt = 6;
        }
        // DEBUG_TRACE(LOG_TAG, "x= %d, y= %d, z=%d, roll= %.2f, pitch= %.2f, angle= %.2f", sample.x, sample.y, sample.z, lis3dh_t.roll, lis3dh_t.pitch, device_t.angle);
        DEBUG_TRACE(LOG_TAG, "convert angle %.2f, slant[%d], count[%d]", device_t.angle, device_t.is_slant, slant_cnt);
    }
}

void Acc_Init(void)
{
    if (!acc_sensor) {
        acc_sensor = lis3dh_init_sensor(LIS3DH_I2C_ADDRESS_2, 0);
    }
    if (acc_sensor) {
        DEBUG_TRACE(LOG_TAG, "Acc Sensor Init Success");

        // enable data interrupts on INT1
        lis3dh_int_event_config_t event_config;

        // event_config.mode = lis3dh_wake_up;
        // event_config.mode = lis3dh_free_fall;
        event_config.mode = lis3dh_6d_movement;
        // event_config.mode = lis3dh_6d_position;
        // event_config.mode = lis3dh_4d_movement;
        // event_config.mode = lis3dh_4d_position;
        event_config.threshold = 10;
        event_config.x_low_enabled = true;
        event_config.x_high_enabled = true;
        event_config.y_low_enabled = true;
        event_config.y_high_enabled = true;
        event_config.z_low_enabled = true;
        event_config.z_high_enabled = true;
        event_config.duration = 0;
        event_config.latch = false;
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
        lis3dh_config_hpf(acc_sensor, lis3dh_hpf_normal, 0, false, false, false, false);
        lis3dh_get_hpf_ref(acc_sensor);

        // enable ADC inputs and temperature sensor for ADC input 3
        lis3dh_enable_adc(acc_sensor, false, false);

        // LAST STEP: Finally set scale and mode to start measurements
        lis3dh_set_scale(acc_sensor, lis3dh_scale_2_g);
        lis3dh_set_mode(acc_sensor, lis3dh_odr_50, lis3dh_low_power, true, true, true);
        // lis3dh_set_int_event_config(acc_sensor, &event_config, lis3dh_int_event1_gen);
        // lis3dh_enable_int(acc_sensor, lis3dh_int_event1, lis3dh_int1_signal, true);
        lis3dh_enable_int(acc_sensor, lis3dh_int_event1, lis3dh_int1_signal, false);

        lis3dh_int_data_source_t data_src = {0};
        lis3dh_int_event_source_t event_src = {0};
        // lis3dh_int_click_source_t click_src = {0};
        // get the source of the interrupt and reset *INTx* signals
        lis3dh_get_int_data_source(acc_sensor, &data_src);
        lis3dh_get_int_event_source(acc_sensor, &event_src, lis3dh_int_event1_gen);
        // lis3dh_get_int_click_source (acc_sensor, &click_src);
#endif
        if (!device_t.power_on) // 切换待机状态
            // lis3dh_set_mode(acc_sensor, lis3dh_power_down, lis3dh_low_power, true, true, true);
            lis3dh_set_mode(acc_sensor, lis3dh_odr_1, lis3dh_low_power, true, true, true);
    }
}

void acc_power_down(void)
{
    // lis3dh_set_mode(acc_sensor, lis3dh_power_down, lis3dh_low_power, true, true, true);
    lis3dh_set_mode(acc_sensor, lis3dh_odr_1, lis3dh_low_power, true, true, true);
}

void Angle_SglConvert(void)
{
    static float old_angle;
    axis_info_t sample;
    lis3dh_raw_data_t raw;

    if (!lis3dh_new_data(acc_sensor) & !lis3dh_get_raw_data(acc_sensor, &raw))
        return;

    sample.x = FS_2g_HR_TO_mg(raw.ax);
    sample.y = FS_2g_HR_TO_mg(raw.ay);
    sample.z = FS_2g_HR_TO_mg(raw.az);

    float roll = (float)(atan2((float)(sample.x), sample.z) * DEG_PER_RAD);  // 转换为度数
    float pitch = (float)(atan2((float)(sample.y), sample.z) * DEG_PER_RAD); // 转换为度数
    if (roll < 0)
        roll += 360;
    if (pitch < 0)
        pitch += 360;
    // float angle = pitch;
    float angle = roll; // 翻滚角

    if (fabs(angle - old_angle) > 5) {
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }
    old_angle = angle;
    DEBUG_TRACE(LOG_TAG, "Result: %.2f, Func: %s", angle, __func__);
}
