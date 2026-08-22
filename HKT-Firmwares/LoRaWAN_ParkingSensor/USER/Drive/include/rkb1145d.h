
#ifndef __RKB1145B_H__
#define __RKB1145B_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define RKB_MIN_DISTANCE_THRESHOLD 2    // 频谱模式下雷达距离阈值
#define RKB_INDUCTION_THRESHOLD     600 // 近场感应阈值(is_radar_covered用)
#define RKB_UNUSUAL_THRESHOLD       600 // 异常信号阈值
#define RKB_INDUCTION_CAR_THRESHOLD 800 // 车辆检测阈值(融合/单模)
#define RKB_COVERED_MIN             600 // 模式2 覆盖下限(水反射600-1000)
#define RKB_COVERED_MAX             1500 // 模式2 覆盖上限(超过为低底盘车)
#define RKB_VEHICLE_THRESHOLD       800  // 模式2 车辆检测阈值(可独立调整)
#define RKB_RADAR_WAKE_INTERVAL     3   // 模式2 雷达唤醒间隔

struct rkb1145d {
    u8 is_calibration;
    u8 is_changed;         // 0 无车 1有车 FF异常
    u8 max_power_distance; // 最大频谱距离(频谱模式下)
    u16 power[10];
    u16 base_power[10];
    u16 adjust_power[10];
    u8 reserved[1]; // 预留，保证结构体能被8整除
} __attribute__((aligned(4)));

extern struct rkb1145d rkb1145d_t;

u8 rkb_cal_sum(u8 *buf, int len);
bool rd_exec_cmd(u8 *cmd, u8 len, u16 timeout);
void rkb_search_distance(void);
void rkb_set_min_distance(bool set, u16 distance);
void rkb_set_max_distance(bool set, u16 distance);
void rkb_set_buad_rate(bool set, u8 baud);
void rkb_set_data_mode(bool set, bool power);
void rkb_set_distance_mode(u8 cmd, bool set, u16 value);
void rkb_init(void);
bool rkb_search_distance_cal(void);
void rkb_period_event(void);
void rkb_calibration_event(void);
void rkb_adjust_offset(void);
void rkb_response_time_test(void);
void rkb_period_read_test(void);

#ifdef __cplusplus
}
#endif

#endif
