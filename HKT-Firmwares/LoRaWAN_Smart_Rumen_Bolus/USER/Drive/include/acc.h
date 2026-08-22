
#ifndef __ACC_H__
#define __ACC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdbool.h"
#include <config.h>

// #define FIFO_MODE
#define FILTER_CNT 5
#define SAMPLE_SIZE 2
#define DYNAMIC_PRECISION (256 * 2) /*动态精度*/

#define MOST_ACTIVE_NULL 0 /*未找到最活跃轴*/
#define MOST_ACTIVE_X 1    /*最活跃轴X*/
#define MOST_ACTIVE_Y 2    /*最活跃轴Y*/
#define MOST_ACTIVE_Z 3    /*最活跃轴Z*/

#define ACTIVE_PRECISION (256 * 2) /*活跃轴最小变化值*/

#define DEG_PER_RAD (180.0 / 3.14159265358979)
#define ACC_I2C_ADDRESS 0x27

#define NSA_REG_SPI_I2C 0x00
#define NSA_REG_WHO_AM_I 0x01
#define NSA_REG_ACC_X_LSB 0x02
#define NSA_REG_ACC_X_MSB 0x03
#define NSA_REG_ACC_Y_LSB 0x04
#define NSA_REG_ACC_Y_MSB 0x05
#define NSA_REG_ACC_Z_LSB 0x06
#define NSA_REG_ACC_Z_MSB 0x07
#define NSA_REG_MOTION_FLAG 0x09
#define NSA_REG_NEWDATA_FLAG 0x0A
#define NSA_REG_STEPS_MSB 0x0D
#define NSA_REG_STEPS_LSB 0x0E
#define NSA_REG_G_RANGE 0x0f
#define NSA_REG_ODR_AXIS_DISABLE 0x10
#define NSA_REG_POWERMODE_BW 0x11
#define NSA_REG_SWAP_POLARITY 0x12
#define NSA_REG_FIFO_CTRL 0x14
#define NSA_REG_INTERRUPT_SETTINGS0 0x15
#define NSA_REG_INTERRUPT_SETTINGS1 0x16
#define NSA_REG_INTERRUPT_SETTINGS2 0x17
#define NSA_REG_INTERRUPT_MAPPING1 0x19
#define NSA_REG_INTERRUPT_MAPPING2 0x1a
#define NSA_REG_INTERRUPT_MAPPING3 0x1b
#define NSA_REG_INT_PIN_CONFIG 0x20
#define NSA_REG_INT_LATCH 0x21
#define NSA_REG_ACTIVE_DURATION 0x27
#define NSA_REG_ACTIVE_THRESHOLD 0x28
#define NSA_REG_TAP_DURATION 0x2A
#define NSA_REG_TAP_THRESHOLD 0x2B
#define NSA_REG_RESET_STEP 0x2E
#define NSA_REG_STEP_CONGIF1 0x2F
#define NSA_REG_STEP_CONGIF2 0x30
#define NSA_REG_STEP_CONGIF3 0x31
#define NSA_REG_STEP_CONGIF4 0x32
#define NSA_REG_STEP_FILTER 0x33
#define NSA_REG_SM_THRESHOL 0x34
#define NSA_REG_CUSTOM_OFFSET_X 0x38
#define NSA_REG_CUSTOM_OFFSET_Y 0x39
#define NSA_REG_CUSTOM_OFFSET_Z 0x3a
#define NSA_REG_ENGINEERING_MODE 0x7f
#define NSA_REG_SENSITIVITY_TRIM_X 0x80
#define NSA_REG_SENSITIVITY_TRIM_Y 0x81
#define NSA_REG_SENSITIVITY_TRIM_Z 0x82
#define NSA_REG_COARSE_OFFSET_TRIM_X 0x83
#define NSA_REG_COARSE_OFFSET_TRIM_Y 0x84
#define NSA_REG_COARSE_OFFSET_TRIM_Z 0x85
#define NSA_REG_FINE_OFFSET_TRIM_X 0x86
#define NSA_REG_FINE_OFFSET_TRIM_Y 0x87
#define NSA_REG_FINE_OFFSET_TRIM_Z 0x88
#define NSA_REG_SENS_COMP 0x8c
#define NSA_REG_SENS_COARSE_TRIM 0xd1

#define ABS(a) (0 - (a)) > 0 ? (-(a)) : (a)
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define FS_2g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 1.0f
#define FS_4g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 2.0f
#define FS_8g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 4.0f
#define FS_16g_HR_TO_mg(lsb) (float)((int16_t)lsb >> 4) * 12.0f
#define LSB_TO_degC_HR(lsb) (float)((int16_t)lsb >> 6) / 4.0f + 25.0f

typedef struct {

    int16_t ax; // acceleration on x axis
    int16_t ay; // acceleration on y axis
    int16_t az; // acceleration on z axis

} raw_data_t;

void Acc_Init(void);
void Acc_Handler(void);
void convertAccData(void);

#ifdef __cplusplus
}
#endif

#endif
