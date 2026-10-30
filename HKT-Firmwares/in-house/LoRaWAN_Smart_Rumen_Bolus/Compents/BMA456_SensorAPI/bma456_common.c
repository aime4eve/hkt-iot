/**
 * Copyright (C) 2023 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bma456_common.h"
#include "bma4_defs.h"
#include "iic.h"
#include "systick.h"
#include "uart.h"

/******************************************************************************/
/*!                       Macro definitions                                   */

/*! Read write length varies based on user requirement */
#define BMA4_READ_WRITE_LEN UINT8_C(8)

/******************************************************************************/
/*!                Static variable definition                                 */

/*! Variable that holds the I2C device address or SPI chip selection */
static uint8_t dev_addr;

/******************************************************************************/
/*!                User interface functions                                   */

/*!
 * I2C read function map to COINES platform
 */
BMA4_INTF_RET_TYPE bma4_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_address = *(uint8_t *)intf_ptr;

    (void)intf_ptr;
    BMA4_INTF_RET_TYPE ret = I2C1_MasterRead(dev_address, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)reg_data, (uint16_t)len);

#if BMA456_IIC_DEBUG_ENBALE
    INFO("\n bma4_i2c_read:[%02X]",reg_addr);
    for (int i = 0; i < len; i++) {
        INFO(" %02x", reg_data[i]);
    }
    INFO("\n");
#endif

    return ret;
}

/*!
 * I2C write function map to COINES platform
 */
BMA4_INTF_RET_TYPE bma4_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_address = *(uint8_t *)intf_ptr;

    (void)intf_ptr;
    BMA4_INTF_RET_TYPE ret = I2C1_MasterWrite(dev_address, reg_addr, I2C_MEMADD_SIZE_8BIT, (uint8_t *)reg_data, len);

#if BMA456_IIC_DEBUG_ENBALE
    INFO("\n bma4_i2c_write:[%02X]",reg_addr);
    for (int i = 0; i < len; i++) {
        INFO(" %02x", reg_data[i]);
    }
    INFO("\n");
#endif

    return ret;
}

/*!
 * Delay function map to COINES platform
 */
void bma4_delay_us(uint32_t period, void *intf_ptr)
{
    (void)intf_ptr;
    Delay_Us(period);
}

/*!
 *  @brief Function to select the interface between SPI and I2C.
 *  Also to initialize coines platform
 */
int8_t bma4_interface_init(struct bma4_dev *bma, uint8_t intf, enum bma4_variant variant)
{
    int8_t rslt = BMA4_OK;

    if (bma != NULL) {
        /* Bus configuration : I2C */
        if (intf == BMA4_I2C_INTF) {
            INFO("\nI2C Interface \n");

            /* To initialize the user I2C function */
            dev_addr = BMA4_I2C_ADDR_PRIMARY;
            bma->intf = BMA4_I2C_INTF;
            bma->bus_read = bma4_i2c_read;
            bma->bus_write = bma4_i2c_write;
        }

        /* Assign variant information */
        bma->variant = variant;

        /* Assign device address to interface pointer */
        bma->intf_ptr = &dev_addr;

        /* Configure delay in microseconds */
        bma->delay_us = bma4_delay_us;

        /* Configure max read/write length (in bytes) ( Supported length depends on target machine) */
        bma->read_write_len = BMA4_READ_WRITE_LEN;

        /* Set Performance mode status */
        bma->perf_mode_status = BMA4_DISABLE;
    } else {
        rslt = BMA4_E_NULL_PTR;
    }

    return rslt;
}

/*!
 *  @brief Prints the execution status of the APIs.
 */
void bma4_error_codes_print_result(const char api_name[], int8_t rslt)
{
    if (rslt != BMA4_OK) {
        INFO("%s\t", api_name);
        if (rslt == BMA4_E_NULL_PTR) {
            INFO("Error [%d] : Null pointer\r\n", rslt);
        } else if (rslt == BMA4_E_COM_FAIL) {
            INFO("Error [%d] : Communication failure\r\n", rslt);
        } else if (rslt == BMA4_E_CONFIG_STREAM_ERROR) {
            INFO("Error [%d] : Invalid configuration stream\r\n", rslt);
        } else if (rslt == BMA4_E_SELF_TEST_FAIL) {
            INFO("Error [%d] : Self test failed\r\n", rslt);
        } else if (rslt == BMA4_E_INVALID_SENSOR) {
            INFO("Error [%d] : Device not found\r\n", rslt);
        } else if (rslt == BMA4_E_OUT_OF_RANGE) {
            INFO("Error [%d] : Out of Range\r\n", rslt);
        } else if (rslt == BMA4_E_AVG_MODE_INVALID_CONF) {
            INFO("Error [%d] : Invalid bandwidth and ODR combination in Accel Averaging mode\r\n", rslt);
        } else {
            /* For more error codes refer "*_defs.h" */
            INFO("Error [%d] : Unknown error code\r\n", rslt);
        }
    }
}
