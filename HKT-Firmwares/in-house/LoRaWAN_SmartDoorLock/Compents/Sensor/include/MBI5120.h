
#ifndef __MBI5120_H__
#define __MBI5120_H__

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    // 1 -> bit15
    // 2 -> bit14
    // 3 -> bit13
    // 4 -> bit12
    // 5 -> bit11
    // 6 -> bit10
    // 7 -> bit9
    // 8 -> bit8
    // 9 -> bit7
    // * -> bit6
    // 0 -> bit5
    // # -> bit4
    union u_mbi5120
    {
        struct s_mbi5120
        {
            u16 led_1 : 1;
            u16 led_2 : 1;
            u16 led_3 : 1;
            u16 led_4 : 1;
            u16 led_5 : 1;
            u16 led_6 : 1;
            u16 led_7 : 1;
            u16 led_8 : 1;
            u16 led_9 : 1;
            u16 led_x : 1;
            u16 led_0 : 1;
            u16 led_j : 1;
        }s_mbi5120_t;
        u16 dat;
    };

    extern union u_mbi5120 u_mbi5120_t;

    void MBI5120_Init(void);
    void MBI5120_Display(u16 num);
    u16 getLedShowNum(u16 inputNum);
    u16 getLedShowNum_1(u16 inputNum);
    u16 Mbi_ShowNum(u16 inputNum);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWRB_HDR_H */
