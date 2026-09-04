
#ifndef __SI12T_H__
#define __SI12T_H__

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*======================================================
寄存器定义
=======================================================*/
#define SI12TCH_IIC_ADDR (0xD0 >> 1)
#define SI12TCH_IIC_ADDR2 (0xF0 >> 1)

#define Sense1 0x02   // channel 2 and 1  灵敏度控制寄存器
#define Sense2 0x03   // channel 4 and 3
#define Sense3 0x04   // channel 6 and 5
#define Sense4 0x05   // channel 8 and 7
#define Sense5 0x06   // channel 10 and 9
#define Sense6 0x07   // channel 12 and 11
#define Sense7 0x22   // channel 14 and 13  新增
#define CTRL1 0x08    // 通用控制寄存器1
#define CTRL2 0x09    // 通用控制寄存器2
#define Ref_rst1 0x0A // 通道参考复位控制寄存器
#define Ref_rst2 0x0B
#define Ch_hold1 0x0C // 通道感应控制寄存器
#define Ch_hold2 0x0D
#define Cal_hold1 0x0E // 通道校准控制寄存器
#define Cal_hold2 0x0F
#define Output1 0x10 // channel 1 , 2 , 3 , 4
#define Output2 0x11 // channel 5 , 6 , 7 , 8
#define Output3 0x12 // channel 9 , 10 , 11 , 12
#define Output4 0x13 // 新增 channel 13 , 14

typedef struct s_si12t {
    u8 clear_signal;   // 下次按键无效信号
    u8 cnt;            // 触摸计数
    char password[21]; // 触摸内容
    u16 timeout;       // 超时内容清空时间
} s_si12t_t;

extern s_si12t_t s_touch;
extern const u8 voice_volume_enum[];

void si12tch_rst(void);
void si14t_init(void);
void si12t_sleep(void);
void si12t_wakeup(void);
void si12t_clear_read(void);
void thread_touch(ULONG thread_input);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
