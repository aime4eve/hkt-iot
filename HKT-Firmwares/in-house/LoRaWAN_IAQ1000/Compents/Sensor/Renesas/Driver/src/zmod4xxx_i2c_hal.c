/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "zmod4xxx_i2c_hal.h"

#include "sensirion_common.h"
#include "sensirion_config.h"
#include "sensirion_i2c.h"
#include "iic.h"

int8_t zmod4xxx_i2c_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    int8_t ret;
    ret = i2c_hard_write_read(i2c_addr, reg_addr, buf, len);
    if (ret)
    {
        return ret;
    }
    return NO_ERROR;
}

int8_t zmod4xxx_i2c_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *buf, uint8_t len)
{
    u8 data[20] = {0};
    data[0] = reg_addr;
    memcpy(data + 1, buf, len);
    return i2c_hard_write(i2c_addr, data, len + 1);
}

void zmod4xxx_sleep(uint32_t ms)
{
    tx_thread_sleep(ms);
}
