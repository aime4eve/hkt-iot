
/* Includes ------------------------------------------------------------------*/
#include "QMC5883L.h"
#include "communicate.h"
#include "control_center.h"
#include "flash.h"
#include "iic.h"
#include "math.h"
#include "rkb1145d.h"
#include "systick.h"
#include "uart.h"

struct qmc5883l qmc5883l_t;

const char _bearings[16][3] = {
    {' ', ' ', 'N'}, {'N', 'N', 'E'}, {' ', 'N', 'E'}, {'E', 'N', 'E'}, {' ', ' ', 'E'}, {'E', 'S', 'E'}, {' ', 'S', 'E'}, {'S', 'S', 'E'},
    {' ', ' ', 'S'}, {'S', 'S', 'W'}, {' ', 'S', 'W'}, {'W', 'S', 'W'}, {' ', ' ', 'W'}, {'W', 'N', 'W'}, {' ', 'N', 'W'}, {'N', 'N', 'W'},
};

/**
 * @brief  复位磁力计
 * @param
 * @retval
 */
void magnetometer_reset(void)
{
    u8 buffer[2];
    buffer[0] = QMC5883L_CONFIG2_RESET;
    // buffer[0] = QMC5883L_CONFIG2_RESET | 0X01; // 关闭转换中断
    mDelay(1);
    i2c_slave_write(QMC5883L_ADDR, QMC5883L_CONFIG2, buffer, 1);
    mDelay(2);
}

/**
 * @brief  磁力计进入待机模式
 * @param
 * @retval
 */
void magnetometer_standby(void)
{
    u8 buffer[2];
    buffer[0] = QMC5883L_CONFIG_STANDBY;
    i2c_slave_write(QMC5883L_ADDR, QMC5883L_CONFIG, buffer, 1);
}

/**
 * @brief  设置磁力计工作模式
 * @param
 * @retval
 */
void magnetometer_set_mode(u8 mode, u8 odr, u8 rng, u8 osr)
{
    u8 buffer[2];
    buffer[0] = QMC5883L_CONFIG_CONT;
    i2c_slave_write(QMC5883L_ADDR, QMC5883L_RESET, buffer, 1);

    buffer[0] = mode | odr | rng | osr;
    i2c_slave_write(QMC5883L_ADDR, QMC5883L_CONFIG, buffer, 1);
}

/**
 * @brief  读取磁力计转换状态
 * @param
 * @retval	true 数据已准备好  false 数据未准备好
 */
bool magnetometer_read_ready(void)
{
    u8 buffer[2] = {0};
    i2c_slave_read(QMC5883L_ADDR, QMC5883L_STATUS, buffer, 1);
    if ((buffer[0] & QMC5883L_STATUS_DRDY) != 1)
        return false;
    return true;
}

/**
 * @brief  读取磁力计三轴寄存器值
 * @param
 * @retval
 */
void magnetometer_read_xyz(short *x, short *y, short *z)
{
    u8 buffer[6] = {0};
    if (i2c_slave_read(QMC5883L_ADDR, 0, buffer, 6) == SUCCESS) {
        *x = (short)(buffer[0] | (buffer[1] << 8));
        *y = (short)(buffer[2] | (buffer[3] << 8));
        *z = (short)(buffer[4] | (buffer[5] << 8));
    } else {
        *x = 0;
        *y = 0;
        *z = 0;
    }
}

/**
 * @brief  校准磁力计
 * @param
 * @retval
 */
void magnetometer_calibration(void)
{
    static u8 started;
    static u32 over_time;
    static axis_info_t max, min;
    axis_info_t raw;

    if (qmc5883l_t.is_calibration == true)
        return;

    if (!started) {
        started = 1;
        over_time = get_syspant_ms();
        DEBUG_TRACE(LOG_TAG, "Start Magnetometer Calibration");

        max.x = raw.x;
        max.y = raw.y;
        max.z = raw.z;
        min.x = raw.x;
        min.y = raw.y;
        min.z = raw.z;
    }

    if (magnetometer_read_ready() == false)
        return;
    magnetometer_read_xyz(&raw.x, &raw.y, &raw.z);

    if (raw.x > max.x) {
        max.x = raw.x;
        over_time = get_syspant_ms();
    }
    if (raw.y > max.y) {
        max.y = raw.y;
        over_time = get_syspant_ms();
    }
    if (raw.z > max.z) {
        max.z = raw.z;
        over_time = get_syspant_ms();
    }
    if (raw.x < min.x) {
        min.x = raw.x;
        over_time = get_syspant_ms();
    }
    if (raw.y < min.y) {
        min.y = raw.y;
        over_time = get_syspant_ms();
    }
    if (raw.z < min.z) {
        min.z = raw.z;
        over_time = get_syspant_ms();
    }

    // if ((max.x - min.x) > (MAGNETOMETER_THRESHOLD_X * 2) ||
    //     (max.y - min.y) > (MAGNETOMETER_THRESHOLD_Y * 2) ||
    //     (max.z - min.z) > (MAGNETOMETER_THRESHOLD_Z * 2)) {
    if ((max.x - min.x) > (MAGNETOMETER_THRESHOLD_X) || (max.y - min.y) > (MAGNETOMETER_THRESHOLD_Y) ||
        (max.z - min.z) > (MAGNETOMETER_THRESHOLD_Z)) {
        max.x = raw.x;
        max.y = raw.y;
        max.z = raw.z;
        min.x = raw.x;
        min.y = raw.y;
        min.z = raw.z;
        DEBUG_TRACE(LOG_TAG, "Restart Magnetometer Calibration");
    }

#if HIGH_LEVEL_DEBUG_ENABLE
    INFO("\r\n%d, %d, %d current xyz, %d, %d, %d max xyz, %d, %d, %d min xyz", raw.x, raw.y, raw.z, max.x, max.y, max.z, min.x, min.y, min.z);
#endif

    if (device_t.sleep_delay < WAIT_SLEEP_TIME_DELAY) {
        device_t.sleep_delay = WAIT_SLEEP_TIME_DELAY;
    }

    if (!check_delay_expired(over_time, MAX_MAGNETOMETER_CALIBRATION_TIME))
        return;

    started = 0;

    axis_info_t sraw[22];
    int total_x = 0, total_y = 0, total_z = 0;
    u8 count = 0;
    while (1) {
        if (magnetometer_read_ready()) {
            magnetometer_read_xyz(&sraw[count].x, &sraw[count].y, &sraw[count].z);
            total_x += sraw[count].x;
            total_y += sraw[count].y;
            total_z += sraw[count].z;
            count++;
            if (count == 22) {
                count = 0;
                axis_info_t max, min;
                max.x = sraw[0].x;
                max.y = sraw[0].y;
                max.z = sraw[0].z;
                min.x = sraw[0].x;
                min.y = sraw[0].y;
                min.z = sraw[0].z;
                for (u8 i = 0; i < 22; i++) {
                    max.x = (sraw[i].x < max.x) ? max.x : sraw[i].x;
                    min.x = (min.x < sraw[i].x) ? min.x : sraw[i].x;

                    max.y = (sraw[i].y < max.y) ? max.y : sraw[i].y;
                    min.y = (min.y < sraw[i].y) ? min.y : sraw[i].y;

                    max.z = (sraw[i].z < max.z) ? max.z : sraw[i].z;
                    min.z = (min.z < sraw[i].z) ? min.z : sraw[i].z;
                }

                qmc5883l_t.offset.x = (total_x - max.x - min.x) / 20;
                qmc5883l_t.offset.y = (total_y - max.y - min.y) / 20;
                qmc5883l_t.offset.z = (total_z - max.z - min.z) / 20;
                break;
            }
        }
    }

    DEBUG_TRACE(LOG_TAG, "End Magnetometer Calibration");

    // qmc5883l_t.offset.x = (max.x + min.x) / 2;
    // qmc5883l_t.offset.y = (max.y + min.y) / 2;
    // qmc5883l_t.offset.z = (max.z + min.z) / 2;

    float x_avg_delta = (max.x - min.x) / 2;
    float y_avg_delta = (max.y - min.y) / 2;
    float z_avg_delta = (max.z - min.z) / 2;

    float avg_delta = (x_avg_delta + y_avg_delta + z_avg_delta) / 3;

    qmc5883l_t.x_scale = avg_delta / x_avg_delta;
    qmc5883l_t.y_scale = avg_delta / y_avg_delta;
    qmc5883l_t.z_scale = avg_delta / z_avg_delta;

    qmc5883l_t.is_calibration = true;
    qmc5883l_t.state = MAGNETOMETER_IN_WORK;

    /* 重置EMA状态，下次magnetometer_calculate()将从新采样重新种子 */
    qmc5883l_t.ema_initialized = 0;
    qmc5883l_t.ema_noise = 0;

    qmc5883l_t.threshold.x = MAGNETOMETER_THRESHOLD_X;
    qmc5883l_t.threshold.y = MAGNETOMETER_THRESHOLD_Y;
    qmc5883l_t.threshold.z = MAGNETOMETER_THRESHOLD_Z;
    FLASH_Write_Mag_Param();
    FLASH_Write_Mag_Param_Back(); // 写入备份区域

    INFO("\r\nmax xyz[%d, %d, %d], min xyz[%d, %d, %d]", max.x, max.y, max.z, min.x, min.y, min.z);
    INFO("\r\noffset xyz[%d, %d, %d], scale xyz[%f, %f, %f], avg delta[%d]", qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z,
         qmc5883l_t.x_scale, qmc5883l_t.y_scale, qmc5883l_t.z_scale, avg_delta);

    rkb_adjust_offset();
    FLASH_Write_Rkb_Param();

    if (device_t.ble_connected) {
        BLE_SendCMD("Calibration Done");
        // callback_BLEAck();
    }
    INFO("\r\nCalibration Done\r\n");
}

/**
 * @brief  获取方位脚
 * @param
 * @retval
 */
float magnetometer_get_azimuth(short y, short x)
{
    float a = atan2((double)y, (double)x) * DEG_PER_RAD;
    return a < 0 ? 360 + a : a;
}

/**
 * @brief  将360度圆分成16等份 然后返回0-15的a值 基于方位角当前指向的位置
 * @param
 * @retval
 */
u8 magnetometer_get_bearing(int azimuth)
{
    unsigned long a = azimuth / 22.5;
    unsigned long r = a - (int)a;
    u8 sexdec = 0;
    sexdec = (r >= .5) ? ceil(a) : floor(a);
    return sexdec;
}

void magnetometer_enter_cal(void)
{
    qmc5883l_t.is_calibration = 0;
    qmc5883l_t.state = MAGNETOMETER_IN_CALIBRATION;
}

// 比较磁力计单轴数据差值与阈值关系，返回是否满足条件
bool compare_axis(u16 raw_value, u16 offset_value, u16 threshold, bool strict_compare)
{
    u16 diff = abs(offset_value - raw_value);
    if (diff >= threshold) {
        if (strict_compare && diff >= threshold * 3) { 
            return true;
        }
        return true;
    }
    return false;
}

/**
 * @brief  磁力计变化
 * @param
 * @retval  true 有变化  false 无变化
 */
/**
 * @brief  评估单次磁力计采样的轴活跃度
 * @param  raw: 原始采样值
 * @retval axis_act (0-7)
 */
static u8 magnetometer_eval_axis_act(axis_info_t *raw)
{
    u8 axis_act = 0;
    u32 total_diff = 0;

    if (device_t.park_state == 0 || device_t.park_state == 0xFF) {
        total_diff += abs(qmc5883l_t.offset.x - raw->x);
        total_diff += abs(qmc5883l_t.offset.y - raw->y);
        total_diff += abs(qmc5883l_t.offset.z - raw->z);

        if (total_diff >= (qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z) * 0.85) {
            axis_act++;
        }

        if (abs(qmc5883l_t.offset.x - raw->x) >= qmc5883l_t.threshold.x) {
            axis_act++;
            if (abs(qmc5883l_t.offset.x - raw->x) >= qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z)
                axis_act++;
        }
        if (abs(qmc5883l_t.offset.y - raw->y) >= qmc5883l_t.threshold.y) {
            axis_act++;
            if (abs(qmc5883l_t.offset.y - raw->y) >= qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z)
                axis_act++;
        }
        if (abs(qmc5883l_t.offset.z - raw->z) >= qmc5883l_t.threshold.z) {
            axis_act++;
            if (abs(qmc5883l_t.offset.z - raw->z) >= qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z)
                axis_act++;
        }
    } else if (device_t.park_state == 1 || qmc5883l_t.is_changed == 1) {
        if (abs(qmc5883l_t.offset.x - raw->x) < qmc5883l_t.threshold.x)
            axis_act++;
        if (abs(qmc5883l_t.offset.y - raw->y) < qmc5883l_t.threshold.y)
            axis_act++;
        if (abs(qmc5883l_t.offset.z - raw->z) < qmc5883l_t.threshold.z)
            axis_act++;
    }
    return axis_act;
}

bool magnetometer_is_actived(void)
{
    static u8 last_axis_act = 0;
    axis_info_t raw = {0};
    u8 axis_act;

    /* OSR=128@200Hz有效输出率~60Hz，转换~17ms，超时35ms */
    u32 drdy_timeout = get_syspant_ms();
    while (magnetometer_read_ready() == false) {
        if (check_delay_expired(drdy_timeout, 35))
            break;
        mDelay(1);
    }
    magnetometer_read_xyz(&raw.x, &raw.y, &raw.z);


    if (qmc5883l_t.is_calibration != 1)
        return false;

    axis_act = magnetometer_eval_axis_act(&raw);

    INFO("\r\n[%d, %d, %d] [%d, %d, %d] [%d, %d, %d] [%d-%d] last xyz, base xyz, cur xyz, acct_cnt-park state",
         qmc5883l_t.last.x, qmc5883l_t.last.y, qmc5883l_t.last.z,
         qmc5883l_t.offset.x, qmc5883l_t.offset.y, qmc5883l_t.offset.z,
         raw.x, raw.y, raw.z, axis_act, device_t.park_state);

    /* 连续2次检测到变化才唤醒，同时过滤偶发噪声 */
    if (device_t.park_state == 0 || device_t.park_state == 0xFF) {
        if (axis_act >= 2 && last_axis_act >= 2) {
            last_axis_act = 0;
            return true;
        }
    } else {
        if (axis_act >= 3 && last_axis_act >= 3) {
            last_axis_act = 0;
            return true;
        }
    }

    last_axis_act = axis_act;
    return false;
}

#if 1
/**
 * @brief  磁力计计算 (EMA + 自适应阈值)
 *         用指数移动平均替代10样本截尾平均，响应更快且节省RAM
 *         根据实时噪声水平自适应调整检测阈值
 * @param
 * @retval
 */
void magnetometer_calculate(void)
{
    static u32 covert_time;
    u8 bearing;
    float azimuth;
    axis_info_t raw;
    u16 new_threshold;

    if (qmc5883l_t.is_calibration == 0)
        return;
    if (!check_delay_expired(covert_time, 20))
        return;
    if (magnetometer_read_ready() == false)
        return;
    covert_time = get_syspant_ms();
    magnetometer_read_xyz(&raw.x, &raw.y, &raw.z);

    if (!qmc5883l_t.ema_initialized) {
        /* 首次采样：用原始值种子EMA */
        qmc5883l_t.xyz.x = raw.x;
        qmc5883l_t.xyz.y = raw.y;
        qmc5883l_t.xyz.z = raw.z;
        qmc5883l_t.ema_initialized = 1;
        qmc5883l_t.ema_noise = 0;
        device_t.mag_valid = 1;
        return;
    }

    /* 保存上一轮EMA值用于噪声计算 */
    qmc5883l_t.last.x = qmc5883l_t.xyz.x;
    qmc5883l_t.last.y = qmc5883l_t.xyz.y;
    qmc5883l_t.last.z = qmc5883l_t.xyz.z;

    /* EMA滤波: ema = ema + (raw - ema) / 8 */
    qmc5883l_t.xyz.x = qmc5883l_t.xyz.x + (raw.x - qmc5883l_t.xyz.x) * MAG_EMA_ALPHA_NUM / MAG_EMA_ALPHA_DEN;
    qmc5883l_t.xyz.y = qmc5883l_t.xyz.y + (raw.y - qmc5883l_t.xyz.y) * MAG_EMA_ALPHA_NUM / MAG_EMA_ALPHA_DEN;
    qmc5883l_t.xyz.z = qmc5883l_t.xyz.z + (raw.z - qmc5883l_t.xyz.z) * MAG_EMA_ALPHA_NUM / MAG_EMA_ALPHA_DEN;

    /* 噪声追踪: noise = noise*0.95 + |raw-ema|*0.05 */
    {
        u16 instant_noise = (abs(raw.x - qmc5883l_t.xyz.x)
                           + abs(raw.y - qmc5883l_t.xyz.y)
                           + abs(raw.z - qmc5883l_t.xyz.z)) / 3;
        qmc5883l_t.ema_noise = (qmc5883l_t.ema_noise * 95 + instant_noise * 5) / 100;
    }

    /* 自适应阈值: max(200, min(noise*4, 2000))，平滑过渡 */
    new_threshold = qmc5883l_t.ema_noise * MAG_ADAPTIVE_MULTIPLIER;
    if (new_threshold < MAG_BASE_THRESHOLD)
        new_threshold = MAG_BASE_THRESHOLD;
    if (new_threshold > MAG_THRESHOLD_CAP)
        new_threshold = MAG_THRESHOLD_CAP;

    qmc5883l_t.threshold.x = (qmc5883l_t.threshold.x * 7 + new_threshold) / 8;
    qmc5883l_t.threshold.y = qmc5883l_t.threshold.x;
    qmc5883l_t.threshold.z = qmc5883l_t.threshold.x;

    qmc5883l_t.azimuth = magnetometer_get_azimuth(qmc5883l_t.xyz.y, qmc5883l_t.xyz.x);
    bearing = magnetometer_get_bearing(qmc5883l_t.azimuth);

    char dir[4] = {0};
    dir[0] = _bearings[bearing][0];
    dir[1] = _bearings[bearing][1];
    dir[2] = _bearings[bearing][2];
    device_t.mag_valid = 1;

    /* 每10次(~400ms)打印一次，避免串口被频繁输出淹没 */
    {
        static u8 info_cnt = 0;
        info_cnt++;
        if (info_cnt >= 10) {
            info_cnt = 0;
#if HIGH_LEVEL_DEBUG_ENABLE
            DEBUG_TRACE(LOG_TAG, "Read Magnetometer Info");
            INFO("\r\n[%.02f] [%s] [%d] Azimuth, Bearing, Calibration\r\n", qmc5883l_t.azimuth, dir, qmc5883l_t.is_calibration);
#endif
            INFO("\r\n[%d %d %d] [%d %d %d] cur xyz, base xyz", qmc5883l_t.xyz.x, qmc5883l_t.xyz.y, qmc5883l_t.xyz.z, qmc5883l_t.offset.x,
                 qmc5883l_t.offset.y, qmc5883l_t.offset.z);
        }
    }
}

/**
 * @brief  基线慢速漂移追踪
 *         在确认无车时缓慢跟踪环境磁场变化，防止温漂导致漏检
 *         调整速率极慢(α=1/64)，单次最大调整±50 counts
 * @param
 * @retval
 */
void magnetometer_drift_track(void)
{
    /* 仅在无车且无变化时追踪 */
    if (device_t.park_state != 0 || qmc5883l_t.is_changed != 0)
        return;

    /* 至少间隔5分钟 */
    if (!check_delay_expired(qmc5883l_t.last_drift_ms, 300000))
        return;

    if (qmc5883l_t.last_drift_ms == 0) {
        qmc5883l_t.last_drift_ms = get_syspant_ms();
        return;
    }

    /* 计算当前EMA与offset的误差 */
    int16_t err_x = qmc5883l_t.xyz.x - qmc5883l_t.offset.x;
    int16_t err_y = qmc5883l_t.xyz.y - qmc5883l_t.offset.y;
    int16_t err_z = qmc5883l_t.xyz.z - qmc5883l_t.offset.z;

    /* 仅在误差较小时追踪（< 2倍阈值），避免在有车时误调整 */
    u16 total_err = abs(err_x) + abs(err_y) + abs(err_z);
    u16 threshold_sum = qmc5883l_t.threshold.x + qmc5883l_t.threshold.y + qmc5883l_t.threshold.z;
    if (total_err > threshold_sum * 2)
        return;

    /* 极慢调整: α = 1/64, 单次最大±50 counts */
    int16_t adj_x = err_x / 64;
    int16_t adj_y = err_y / 64;
    int16_t adj_z = err_z / 64;

    if (adj_x > 50) adj_x = 50;
    if (adj_x < -50) adj_x = -50;
    if (adj_y > 50) adj_y = 50;
    if (adj_y < -50) adj_y = -50;
    if (adj_z > 50) adj_z = 50;
    if (adj_z < -50) adj_z = -50;

    qmc5883l_t.offset.x += adj_x;
    qmc5883l_t.offset.y += adj_y;
    qmc5883l_t.offset.z += adj_z;

    qmc5883l_t.last_drift_ms = get_syspant_ms();
}

void magnetometer_test(void)
{
    axis_info_t raw;
    axis_info_t dif;
    static axis_info_t max, min;
    static u32 covert_time;

    if (!check_delay_expired(covert_time, 500))
        return;
    if (magnetometer_read_ready() == false)
        return false;
    magnetometer_read_xyz(&raw.x, &raw.y, &raw.z);
    if (max.x == 0 && max.y == 0 && max.z == 0) {
        max.x = raw.x;
        max.y = raw.y;
        max.z = raw.z;
    }

    max.x = (raw.x < max.x) ? max.x : raw.x;
    min.x = (min.x < raw.x) ? min.x : raw.x;

    max.y = (raw.y < max.y) ? max.y : raw.y;
    min.y = (min.y < raw.y) ? min.y : raw.y;

    max.z = (raw.z < max.z) ? max.z : raw.z;
    min.z = (min.z < raw.z) ? min.z : raw.z;

    dif.x = max.x - min.x;
    dif.y = max.y - min.y;
    dif.z = max.z - min.z;

    INFO("cur[%d, %d, %d], max[%d, %d, %d], min[%d, %d, %d], dif[%d, %d, %d]", raw.x, raw.y, raw.z, max.x, max.y, max.z, min.x, min.y, min.z, dif.x,
         dif.y, dif.z);
}

#else

/**
 * @brief  磁力计计算
 * @param
 * @retval
 */
void magnetometer_calculate(void)
{
    static u32 covert_time;

    u8 i;
    u8 bearing;
    float azimuth;

    if (covert_time >= get_syspant_ms())
        return;
    if (magnetometer_read_ready() == false)
        return;
    covert_time = get_syspant_ms() + DATA_COLLECT_INTERVAL;
    magnetometer_read_xyz(&qmc5883l_t.xyz.x, &qmc5883l_t.xyz.y, &qmc5883l_t.xyz.z);

    qmc5883l_t.last.x = qmc5883l_t.xyz.x;
    qmc5883l_t.last.y = qmc5883l_t.xyz.y;
    qmc5883l_t.last.z = qmc5883l_t.xyz.z;

    qmc5883l_t.azimuth = magnetometer_get_azimuth(qmc5883l_t.xyz.y, qmc5883l_t.xyz.x);
    bearing = magnetometer_get_bearing(azimuth);

    char dir[4] = {0};
    dir[0] = _bearings[bearing][0];
    dir[1] = _bearings[bearing][1];
    dir[2] = _bearings[bearing][2];
    device_t.mag_valid = 1;
#if HIGH_LEVEL_DEBUG_ENABLE
    DEBUG_TRACE(LOG_TAG, "Read Magnetometer Info");
    INFO("\r\n[%.02f] [%s] [%d] Azimuth, Bearing, Calibration\r\n", qmc5883l_t.azimuth, dir, qmc5883l_t.is_calibration);
#endif
    INFO("\r\n[%d %d %d] [%d %d %d] cur xyz, base xyz", qmc5883l_t.xyz.x, qmc5883l_t.xyz.y, qmc5883l_t.xyz.z, qmc5883l_t.offset.x,
         qmc5883l_t.offset.y, qmc5883l_t.offset.z);
    // INFO("%d,%d,%d\r\n", qmc5883l_t.xyz.x, qmc5883l_t.xyz.y, qmc5883l_t.xyz.z);
}

#endif

/**
 * @brief  磁力计初始化
 * @param
 * @retval
 */
void magnetometer_init(void)
{
    magnetometer_reset();
    magnetometer_init_mode();
}

/**
 * @brief  磁力计模式配置
 * @param
 * @retval
 */
void magnetometer_init_mode(void)
{
    // magnetometer_set_mode(QMC5883L_CONFIG_CONT, QMC5883L_CONFIG_10HZ, QMC5883L_CONFIG_2GAUSS, QMC5883L_CONFIG_OS64);
    magnetometer_set_mode(QMC5883L_CONFIG_CONT, QMC5883L_CONFIG_200HZ, QMC5883L_CONFIG_2GAUSS, QMC5883L_CONFIG_OS128);
    // magnetometer_set_mode(QMC5883L_CONFIG_CONT, QMC5883L_CONFIG_50HZ, QMC5883L_CONFIG_2GAUSS, QMC5883L_CONFIG_OS512);
    // magnetometer_set_mode(QMC5883L_CONFIG_CONT, QMC5883L_CONFIG_50HZ, QMC5883L_CONFIG_8GAUSS, QMC5883L_CONFIG_OS512);
}

/**
 * @brief  磁力计异步工作管理
 * @param
 * @retval
 */
void magnetometer_work_process(void)
{
    if (!device_t.power_on)
        return;

    switch (qmc5883l_t.state) {
    case MAGNETOMETER_IN_IDLE:
        if (magnetometer_read_ready() == true) {
            magnetometer_read_xyz(&qmc5883l_t.xyz.x, &qmc5883l_t.xyz.y, &qmc5883l_t.xyz.z);
            // INFO("\r\n[%d %d %d] cur xyz", qmc5883l_t.xyz.x, qmc5883l_t.xyz.y, qmc5883l_t.xyz.z);
        }
        break;
    case MAGNETOMETER_IN_CALIBRATION: {
        magnetometer_calibration();
    } break;
    case MAGNETOMETER_IN_WORK: {
        magnetometer_calculate();
        magnetometer_drift_track();
    } break;
    default:
        break;
    }
}

//      实测各个轴在各状态下最大差值
// 	    靠近车	    车前	    车中	    车后        无车
// X	72	        173.5	    -78.5	    -90         25
// 	    79	        184	        -73	        -88
// Y	743	        -57.5	    25.5	    -709.5      100
// 	    740.5	    -93	        10	        -703
// Z	214.5	    402	        -116.5	    11          45
//	    222	        423	        -110	    11

/*

地磁车位检测算法（Magnetic Sensor-based Parking Space Detection Algorithm）是一种基于地磁传感器设计的用于识别停车位状态的技术。
通过在每个停车位安装一个地磁传感器，该算法可以通过收集和分析传感器数据来检测车位是否被占用。

以下是一些可能实用的地磁车位检测算法：

    常数阈值算法：该算法根据已知静态环境下，地磁场强度会在特定范围内波动的事实，设置一个合适的常数阈值，如果探测到的地磁场强度大于或小于该阈值，则认为车位被占用或空闲；
    动态阈值算法：与常数阈值算法不同，动态阈值算法利用移动平均方法根据当前环境的动态变化来更新阈值，使其能够更好地适应环境中的噪声和干扰；
    机器学习算法：利用机器学习算法（如决策树、支持向量机等）对采集到的数据进行建模，并从中学习出占用状态的特征，进而预测停车位的状态，从而实现自动化检测。

以上算法都有其优势和限制，因此在选择适当的算法时，需要考虑性能指标、硬件成本、可用性和精度等方面的因素。

*/

// #include <stdio.h>
// #include <math.h>

// #define PI 3.14159265358979323846

// // 磁力计校准参数
// float mag_offset_x = 0.0;
// float mag_offset_y = 0.0;
// float mag_offset_z = 0.0;
// float mag_scale_x = 1.0;
// float mag_scale_y = 1.0;
// float mag_scale_z = 1.0;

// // 磁力计数据
// float mag_x = 0.0;
// float mag_y = 0.0;
// float mag_z = 0.0;

// // 地球磁场参考值
// float mag_ref_x = 22.0;
// float mag_ref_y = 0.0;
// float mag_ref_z = 45.0;

// // 地球磁场倾角和偏角
// float inclination = 0.0;
// float declination = 0.0;

// // 地磁场X、Y、Z分量
// float mag_field_x = 0.0;
// float mag_field_y = 0.0;
// float mag_field_z = 0.0;

// // 计算地球磁场倾角和偏角
// void calculate_inclination_and_declination(float lat, float lon) {
//     float lat_rad = lat * PI / 180.0;
//     float lon_rad = lon * PI / 180.0;
//     float cos_lat = cos(lat_rad);
//     float sin_lat = sin(lat_rad);
//     float cos_lon = cos(lon_rad);
//     float sin_lon = sin(lon_rad);
//     float bx = cos_lat * cos_lon;
//     float by = cos_lat * sin_lon;
//     float bz = sin_lat;
//     float bh = sqrt(bx * bx + by * by);
//     inclination = atan2(bz, bh);
//     declination = atan2(by, bx);
// }

// // 计算地磁场X、Y、Z分量
// void calculate_mag_field() {
//     mag_field_x = (mag_x - mag_offset_x) * mag_scale_x - mag_ref_x;
//     mag_field_y = (mag_y - mag_offset_y) * mag_scale_y - mag_ref_y;
//     mag_field_z = (mag_z - mag_offset_z) * mag_scale_z - mag_ref_z;
//     float cos_inc = cos(inclination);
//     float sin_inc = sin(inclination);
//     float cos_dec = cos(declination);
//     float sin_dec = sin(declination);
//     float bx = mag_field_x * cos_dec + mag_field_y * sin_dec;
//     float by = mag_field_x * sin_inc * sin_dec + mag_field_y * cos_inc - mag_field_z * sin_inc * cos_dec;
//     float bz = -mag_field_x * cos_inc * sin_dec + mag_field_y * sin_inc * cos_dec + mag_field_z * cos_inc * cos_dec;
//     mag_field_x = bx;
//     mag_field_y = by;
//     mag_field_z = bz;
// }
