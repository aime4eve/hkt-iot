
#ifndef __IIC_H__
#define __IIC_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"
#include <config.h>
// IIC操作的宏定义

// IO操作函数
#define IIC_SCL_L LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_8)
#define IIC_SCL_H LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8)
#define IIC_SDA_L LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_9)
#define IIC_SDA_H LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9)
#define READ_SDA LL_GPIO_ReadInputPin(GPIOB, LL_GPIO_PIN_9) // 读数据

#define I2C_SDA_PORT GPIOB
#define I2C_SDA_PIN LL_GPIO_PIN_9
#define I2C_SDA_H IIC_SDA_H
#define I2C_SDA_L IIC_SDA_L
#define READ_I2C_SDA_STATUS LL_GPIO_ReadInputPin(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_OUT SDA_OUT(); // gpio 配置为输出
#define I2C_SDA_IN SDA_IN();   // gpio 配置为输入

#define I2C_SCL_PORT GPIOB
#define I2C_SCL_PIN LL_GPIO_PIN_8
#define I2C_SCL_H IIC_SCL_H
#define I2C_SCL_L IIC_SCL_L
#define READ_I2C_SCL_STATUS LL_GPIO_ReadInputPin(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SCL_OUT SCL_OUT(); // gpio 配置为输出
#define I2C_SCL_IN SCL_IN();   // gpio 配置为输入

void I2C1_Init(void);
void I2C1_DeInit(void);
u8 i2c_slave_read(u8 addr, u8 reg, u8 *data, int len);
u8 i2c_slave_write(u8 addr, u8 reg, u8 *data, int len);
int8_t i2c_hal_read(uint8_t address, uint8_t *data, uint16_t count);
int8_t i2c_hal_write(uint8_t address, const uint8_t *data, uint16_t count);
int8_t i2c_hal_write_read(uint8_t address, uint8_t *tdata, uint16_t tcount, uint8_t *rdata, uint16_t rcount);
uint16_t i2c_add_command_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command);

void IIC_Config(void);
void IIC_Send_Byte(u8 txd);
u8 IIC_Read_Byte(unsigned char ack);

#ifdef __cplusplus
}
#endif

#endif
