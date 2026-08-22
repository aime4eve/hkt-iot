
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
uint8_t I2C1_MasterWrite(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint32_t length);
uint8_t I2C1_MasterRead(uint8_t device_7BitAddr, uint16_t mem_address, uint16_t mem_size, uint8_t *pdata, uint32_t length);

void IIC_Config(void);
void IIC_Send_Byte(u8 txd);
u8 IIC_Read_Byte(unsigned char ack);

#ifdef __cplusplus
}
#endif

#endif
