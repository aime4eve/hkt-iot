
#ifndef __IIC_H__
#define __IIC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

#define I2CREAD 1  // I2C读操作
#define I2CWRITE 0 // I2C写操作

#define STARTBIT 0
#define RESTARTBIT 1
#define STOPBIT 2

#define I2C_SDA_PORT GPIOA
#define I2C_SDA_PIN GPIO_Pin_15
#define I2C_SDA_H GPIO_SetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_L GPIO_ResetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define READ_I2C_SDA_STATUS GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_OUT OutputIO(I2C_SDA_PORT, I2C_SDA_PIN, OUT_OPENDRAIN); // gpio 配置为输出
#define I2C_SDA_IN InputtIO(I2C_SDA_PORT, I2C_SDA_PIN, IN_NORMAL);      // gpio 配置为输入

#define I2C_SCL_PORT GPIOA
#define I2C_SCL_PIN GPIO_Pin_14
#define I2C_SCL_H GPIO_SetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SCL_L GPIO_ResetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define READ_I2C_SCL_STATUS GPIO_ReadInputDataBit(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SCL_OUT OutputIO(I2C_SCL_PORT, I2C_SCL_PIN, OUT_OPENDRAIN); // gpio 配置为输出
#define I2C_SCL_IN InputtIO(I2C_SCL_PORT, I2C_SCL_PIN, IN_NORMAL);      // gpio 配置为输入

  u8 I2C_Send_Bit(I2C_Type *I2Cx, u8 BIT_def);
  u8 I2C_Send_Byte(I2C_Type *I2Cx, u8 x_byte);
  u8 I2C_Receive_Byte(I2C_Type *I2Cx, u8 *x_byte);

  void I2C_Init(I2C_Type *I2Cx);
  u8 i2c_hard_write(uint8_t address, uint8_t *data, uint16_t count);
  u8 i2c_hard_read(uint8_t address, uint8_t *data, uint16_t count);
	u8 i2c_hard_write_read(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count);
  u8 i2c_hard_write_read_n(uint8_t address, uint8_t *send, uint8_t send_len, uint8_t *recv, uint8_t recv_len);
  u8 i2c_hard_write_wait_read_n(uint8_t address, uint8_t *send, uint8_t send_len, uint8_t *recv, uint8_t recv_len, uint16_t time);
  u8 I2C_Test(void);

#ifdef __cplusplus
}
#endif

#endif
