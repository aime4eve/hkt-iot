
#ifndef __RTC_H__
#define __RTC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

typedef struct
{
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
} rtc_time_t;

extern u32 wake_time;

void RTC_Configuration(void);

#ifdef __cplusplus
}
#endif

#endif
