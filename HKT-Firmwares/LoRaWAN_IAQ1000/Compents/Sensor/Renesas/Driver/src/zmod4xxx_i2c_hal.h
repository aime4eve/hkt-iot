
#ifndef ZMOD4XXX_I2C_HAL_H
#define ZMOD4XXX_I2C_HAL_H

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief Sleep for some time. Depending on target and application this can \n
 *        be used to go into power down or to do task switching.
 * @param [in] ms will sleep for at least this number of milliseconds
 */
void zmod4xxx_sleep(uint32_t ms);

/* I2C communication */
/**
 * @brief Read a register over I2C
 * @param [in] i2c_addr 7-bit I2C slave address of the ZMOD45xx
 * @param [in] reg_addr address of internal register to read
 * @param [out] buf destination buffer; must have at least a size of len*uint8_t
 * @param [in] len number of bytes to read
 * @return error code
 */
int8_t zmod4xxx_i2c_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf,
                      uint8_t len);

/**
 * @brief Write a register over I2C using protocol described in Renesas App Note \n
 *        ZMOD4xxx functional description.
 * @param [in] i2c_addr 7-bit I2C slave address of the ZMOD4xxx
 * @param [in] reg_addr address of internal register to write
 * @param [in] buf source buffer; must have at least a size of len*uint8_t
 * @param [in] len number of bytes to write
 * @return error code
 */
int8_t zmod4xxx_i2c_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf,
                       uint8_t len);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SENSIRION_I2C_HAL_H */
