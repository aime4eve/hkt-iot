
#include "hp203b.h"
#include "iic.h"
#include "control_center.h"
#include "uart.h"
#include "systick.h"

#include "sensirion_i2c.h"
#include "sensirion_common.h"
#include "sensirion_config.h"

/****************************/
// for 24bit-adc HP20X senosr

u8 hp20x_cmm_soft_rst()
{
    u8 error;
    u8 buffer = HP20X_SOFT_RST;
    u16 offset = 1;
    error = sensirion_i2c_write_data(HP20X_I2C_DEV_ID, &buffer, offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(1000);
    return NO_ERROR;
}

u8 hp20x_get_data_ready_flag(u8 *status)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)(INT_SRC | HP20X_RD_REG_MODE);

    error = i2c_hard_write(HP20X_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);
    error = i2c_hard_read(HP20X_I2C_DEV_ID, &buffer[0], 1);
    if (error)
    {
        return error;
    }
    *status = buffer[0];
    return NO_ERROR;
}

u8 hp20x_get_data_pressure(u8 *buf)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)(HP20X_WR_CONVERT_CMD | HP20X_CONVERT_OSR1024);
    error = i2c_hard_write(HP20X_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);
    tx_thread_sleep(25); //等待转换完成

    offset = 0;
    buffer[offset++] = (uint8_t)HP20X_READ_P;
    error = i2c_hard_write(HP20X_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);

    error = i2c_hard_read(HP20X_I2C_DEV_ID, &buffer[0], 3);
    if (error)
    {
        return error;
    }
    memcpy(buf, buffer, 3);
    return NO_ERROR;
}

u8 hp20x_get_data_altitude(u8 *buf)
{
    u8 error;
    u8 buffer[6];
    u16 offset = 0;
    buffer[offset++] = (uint8_t)(HP20X_READ_A);
    error = i2c_hard_write(HP20X_I2C_DEV_ID, &buffer[0], offset);
    if (error)
    {
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000);

    error = i2c_hard_read(HP20X_I2C_DEV_ID, &buffer[0], 3);
    if (error)
    {
        return error;
    }
    memcpy(buf, buffer, 3);
    return NO_ERROR;
}

/**
 * @brief	获取大气压和海拔高度
 * @param
 * @retval
 */
void readAltitudeAndPressure()
{
    u8 data[6] = {0};
    u32 Pressure;
    int32_t Altitude;
    hp20x_cmm_soft_rst();
    tx_thread_sleep(25);

    u8 status = 0;
    hp20x_get_data_ready_flag(&status); //查询设备状态
    if (status & 0x40)                  //设备准备就绪 DEV_RDY = 1
    {
        IWDT_Clr();
        hp20x_get_data_pressure(data);
        IWDT_Clr();
        hp20x_get_data_altitude(data + 3);
        IWDT_Clr();

        Pressure = data[0] << 16 | data[1] << 8 | data[2];
        Altitude = data[3] << 16 | data[4] << 8 | data[5];
        if (Pressure & 0x800000)
        {
            Pressure |= 0xff000000;
        }
        if (Altitude & 0x800000)
        {
            Altitude |= 0xff000000;
        }
        device_t.sensor_t.pressure = Pressure / 100 + 0.5;
        device_t.sensor_t.altitude = Altitude / 100 + 0.5;
        DEBUG_TRACE(LOG_TAG, "%s Success, Altitude: %dm, Pressure: %dmbar", __func__, device_t.sensor_t.altitude, device_t.sensor_t.pressure);
    }
    else
    {
        DEBUG_TRACE(ERROR_TAG, "%s Fail", __func__);
    }
}

u8 HP203B_Check(void)
{
    u8 data[6] = {0};
    u32 Pressure;
    int32_t Altitude;
    tx_thread_sleep(2);
    hp20x_cmm_soft_rst();
    tx_thread_sleep(25);

    u8 status = 0;
    hp20x_get_data_ready_flag(&status); //查询设备状态
    if (status == 0x40)                 //设备准备就绪 DEV_RDY = 1
    {
        return 0;
    }
    else
    {
        return 1;
    }
}
