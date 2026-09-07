#include "real_time.h"
#include "iic.h"
#include "rtc.h"
#include "tim.h"

#define REAL_TIME_ADDR (0xA2 >> 1)

/**
 * @brief  初始化实时时钟芯片
 * @param
 * @retval
 */
void real_time_init(void)
{
    u8 buffer[15] = {0};
#if FUNC_RCC_BYPASS_ENABLE
    buffer[0] = 0x80;   // 输出32.768kHz
#else
    buffer[0] = 0;
#endif
    i2c_slave_write(REAL_TIME_ADDR, 0x0D, buffer, 1);
}

/**
 * @brief  读取实时时钟时间
 * @param
 * @retval
 */
void read_real_time(void)
{
    u8 buffer[15] = {0};
    i2c_slave_read(REAL_TIME_ADDR, 0, buffer, 15);
    systime.tm_sec  = __LL_RTC_CONVERT_BCD2BIN(buffer[2] & 0x7F);
    systime.tm_min  = __LL_RTC_CONVERT_BCD2BIN(buffer[3] & 0x7F);
    systime.tm_hour = __LL_RTC_CONVERT_BCD2BIN(buffer[4] & 0x3F);
    systime.tm_mday = __LL_RTC_CONVERT_BCD2BIN(buffer[5] & 0x3F);
    systime.tm_wday = __LL_RTC_CONVERT_BCD2BIN(buffer[6] & 0x07);
    systime.tm_mon  = __LL_RTC_CONVERT_BCD2BIN(buffer[7] & 0x3F) - 1;
    systime.tm_year = __LL_RTC_CONVERT_BCD2BIN(buffer[8]) + 2000;

    getTimestamp();
    // RTC_Init();
}

/**
 * @brief  设置实时时钟时间
 * @param
 * @retval
 */
void set_real_time(void)
{
    u8 buffer[15] = {0};

    systime.tm_wday = whatday(systime.tm_year, systime.tm_mon + 1, systime.tm_mday);
    buffer[8]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_year - 2000);
    buffer[7]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_mon + 1);
    buffer[6]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_wday);
    buffer[5]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_mday);
    buffer[4]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_hour);
    buffer[3]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_min);
    buffer[2]       = __LL_RTC_CONVERT_BIN2BCD(systime.tm_sec);

    i2c_slave_write(REAL_TIME_ADDR, 0, buffer, 15);
}
