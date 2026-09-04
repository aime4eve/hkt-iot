
#ifndef __QMC5883L_H__
#define __QMC5883L_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

/* The default I2C address of this chip */
#define QMC5883L_ADDR 0x0D

/* Register numbers */
#define QMC5883L_X_LSB    0x00
#define QMC5883L_X_MSB    0x01
#define QMC5883L_Y_LSB    0x02
#define QMC5883L_Y_MSB    0x03
#define QMC5883L_Z_LSB    0x04
#define QMC5883L_Z_MSB    0x05
#define QMC5883L_STATUS   0x06
#define QMC5883L_TEMP_LSB 0x07
#define QMC5883L_TEMP_MSB 0x08
#define QMC5883L_CONFIG   0x09
#define QMC5883L_CONFIG2  0x0a
#define QMC5883L_RESET    0x0b
#define QMC5883L_RESERVED 0x0c
#define QMC5883L_CHIP_ID  0x0d

/* Bit values for the STATUS register */
#define QMC5883L_STATUS_DRDY 1
#define QMC5883L_STATUS_OVL  2
#define QMC5883L_STATUS_DOR  4

/* Oversampling values for the CONFIG register */
#define QMC5883L_CONFIG_OS512 0b00000000
#define QMC5883L_CONFIG_OS256 0b01000000
#define QMC5883L_CONFIG_OS128 0b10000000
#define QMC5883L_CONFIG_OS64  0b11000000

/* Range values for the CONFIG register */
#define QMC5883L_CONFIG_2GAUSS 0b00000000
#define QMC5883L_CONFIG_8GAUSS 0b00010000

/* Rate values for the CONFIG register */
#define QMC5883L_CONFIG_10HZ  0b00000000
#define QMC5883L_CONFIG_50HZ  0b00000100
#define QMC5883L_CONFIG_100HZ 0b00001000
#define QMC5883L_CONFIG_200HZ 0b00001100

/* Mode values for the CONFIG register */
#define QMC5883L_CONFIG_STANDBY 0b00000000
#define QMC5883L_CONFIG_CONT    0b00000001

/* Mode values for the CONFIG2 register */
#define QMC5883L_CONFIG2_RESET 0b10000000

#define DEG_PER_RAD (180.0 / 3.14159265358979)

#define MAX_MAGNETOMETER_CALCULATE_COUNT  12 // 12次
#define MAX_MAGNETOMETER_CALIBRATION_TIME 5000 // 5000ms
// 8gauss
// #define MAGNETOMETER_THRESHOLD_X 60
// #define MAGNETOMETER_THRESHOLD_Y 60
// #define MAGNETOMETER_THRESHOLD_Z 60

// 2gauss  静态diff[605, 542, 515]  荣耀手机dif[1990, 2022, 1432] oppo手机dif[2547, 2022, 2787]
// dif -1070,-1597,-617  1185,-1523,260  865,1577,525
#define MAGNETOMETER_THRESHOLD_X 500
#define MAGNETOMETER_THRESHOLD_Y 500
#define MAGNETOMETER_THRESHOLD_Z 500

/* 自适应阈值参数 */
#define MAG_BASE_THRESHOLD       200   // 阈值下限
#define MAG_ADAPTIVE_MULTIPLIER  4     // noise × 4 = 自适应阈值
#define MAG_THRESHOLD_CAP        2000  // 阈值上限
#define MAG_EMA_ALPHA_NUM        1
#define MAG_EMA_ALPHA_DEN        8     // EMA α = 1/8

enum {
    MAGNETOMETER_IN_IDLE,
    MAGNETOMETER_IN_CALIBRATION,
    MAGNETOMETER_IN_WORK,
};

typedef struct {
    short x;
    short y;
    short z;
} axis_info_t __attribute__((aligned(4)));

struct qmc5883l {
    u8 is_calibration;
    u8 is_changed;
    u8 state;
    float x_scale; // x轴校准比例
    float y_scale; // y轴校准比例
    float z_scale; // z轴校准比例
    float azimuth; // 方位角
    axis_info_t offset; // 零点数据
    axis_info_t threshold; // 决策阈值
    axis_info_t xyz; // EMA滤波后的实时数据
    axis_info_t last; // 上一次EMA值(用于噪声计算)
    u8 ema_initialized; // EMA是否已初始化
    u16 ema_noise; // 噪声水平(raw counts)
    u32 last_drift_ms; // 上次漂移更新时间戳
    u8 reserved[0]; // 无预留
} __attribute__((aligned(4)));



extern struct qmc5883l qmc5883l_t;

void magnetometer_standby(void);
bool magnetometer_is_actived(void);
void magnetometer_init(void);
void magnetometer_init_mode(void);
void magnetometer_work_process(void);
void magnetometer_drift_track(void);
void magnetometer_test(void);
void magnetometer_enter_cal(void);

#ifdef __cplusplus
}
#endif

#endif
