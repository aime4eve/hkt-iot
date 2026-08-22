
#ifndef __ACC_H__
#define __ACC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>
#include "stdbool.h"

#define FILTER_CNT 4
#define SAMPLE_SIZE 25
#define DYNAMIC_PRECISION 256 /*动态精度*/

#define MOST_ACTIVE_NULL 0 /*未找到最活跃轴*/
#define MOST_ACTIVE_X 1    /*最活跃轴X*/
#define MOST_ACTIVE_Y 2    /*最活跃轴Y*/
#define MOST_ACTIVE_Z 3    /*最活跃轴Z*/

#define ACTIVE_PRECISION 2048 /*活跃轴最小变化值*/

#define DEG_PER_RAD (180.0 / 3.14159265358979)



  typedef struct
  {
    short x;
    short y;
    short z;
    float tilt_angle;
  } axis_info_t;

  typedef struct filter_avg
  {
    axis_info_t info[FILTER_CNT];
    unsigned char count;
  } filter_avg_t;

  typedef struct
  {
    axis_info_t newmax;
    axis_info_t newmin;
    axis_info_t oldmax;
    axis_info_t oldmin;
  } peak_value_t;

  /*一个线性移位寄存器，用于过滤高频噪声*/
  typedef struct slid_reg
  {
    axis_info_t new_sample;
    axis_info_t old_sample;
  } slid_reg_t;
	
	void acc_init(void);
	void acc_handler(void);

#ifdef __cplusplus
}
#endif

#endif
