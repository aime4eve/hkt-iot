
#ifndef __ACC_H__
#define __ACC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "lis3dh_types.h"
#include "stdbool.h"
#include <config.h>

// #define FIFO_MODE
#define FILTER_CNT 10
#define DYNAMIC_PRECISION (256 * 2) /*动态精度*/

#define MOST_ACTIVE_NULL 0 /*未找到最活跃轴*/
#define MOST_ACTIVE_X 1    /*最活跃轴X*/
#define MOST_ACTIVE_Y 2    /*最活跃轴Y*/
#define MOST_ACTIVE_Z 3    /*最活跃轴Z*/

#define ACTIVE_PRECISION (256 * 2) /*活跃轴最小变化值*/

#define DEG_PER_RAD (180.0 / 3.14159265358979)

#define FS_2g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 1.0f
#define FS_4g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 2.0f
#define FS_8g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 4.0f
#define FS_16g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 12.0f
#define LSB_TO_degC_HR(lsb) (float)((int16_t)lsb >> 6) / 4.0f + 25.0f

typedef struct
{
    short x;
    short y;
    short z;
} axis_info_t;

typedef struct
{
    float x;
    float y;
    float z;
} axis_float_t;

typedef struct filter_avg {
    axis_info_t info[FILTER_CNT];
    float roll;  // 翻滚脚
    float pitch; // 俯仰脚
    unsigned char count;
} filter_avg_t;

typedef struct reference_avg {
    axis_float_t offset;
    axis_float_t scale;
    float roll;  // 翻滚脚
    float pitch; // 俯仰脚
} reference_avg_t;

extern filter_avg_t lis3dh_t;
extern reference_avg_t acc_reference_t;
extern lis3dh_sensor_t *acc_sensor;

void swap(short *p, short *q);
void quick_sort(short *a, int low, int high);
short find_mode_number(short *arr, int len);
short getAccCentre(short *accVals, int cnt);
short getAccRaw(short *accVals, int cnt);
void Acc_Init(void);
void Acc_Handler(void);
void acc_power_down(void);
void Angle_SglConvert(void);

#ifdef __cplusplus
}
#endif

#endif
