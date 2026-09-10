#ifndef __LIS2DH12_H
#define __LIS2DH12_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "config.h"

#define DEGREE_CAL 180.0 / 3.1416
#define DEG_PER_RAD (180.0 / 3.14159265358979)
#define FILTER_CNT 4

  typedef enum acc_event
  {
    ACC_POWER_ON_EVENT = 0,
    ACC_OFFSET_ZERO_EVENT,
    ACC_OLD_ANGLE_EVENT,
    ACC_RUN_EVENT,
    ACC_STANDY_BY_EVENT,
    ACC_MOVE_CHECK_EVENT,
    ACC_NEW_ANGLE_EVENT,
  }_acc_event;

  typedef struct
  {
    short x;
    short y;
    short z;
    short new_angle_x;
    short new_angle_y;
    short new_angle_z;
    short old_angle_x;
    short old_angle_y;
    short old_angle_z;
  } axis_info_t;

  typedef struct filter_avg
  {
    axis_info_t info[FILTER_CNT];
    unsigned char count;
  } filter_avg_t;

  struct lis2dh12_offset
  {
    float x_mg[2];
    float y_mg[2];
    float z_mg[2];

    double move_x;
    double move_y;
    double move_z;

    double current_x_offset;
    double current_y_offset;
    double current_z_offset;

    float old_roll;
    float new_roll;

    float old_pitch;
    float new_pitch;

    int event;
  };

  extern axis_info_t acc_sample;
  extern filter_avg_t acc_data;
  extern struct lis2dh12_offset lis2dh12_offset_t;

  int32_t Lis2dh12_Init(void);
  void filter_calculate(filter_avg_t *filter, axis_info_t *sample);
  void old_angle_calculate(axis_info_t *sample);
  void new_angle_calculate(axis_info_t *sample);
  void lis2dh12_handler(void);

#endif
