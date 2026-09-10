
#ifndef __HP203B_H__
#define __HP203B_H__

#include "config.h"

//#define true 1
//#define false 0

// HP20X INCLUDED:HP203B,HP206C,HP209F
// HP20X REGISTER AND COMMAND
// #define HP20X_I2C_DEV_ID 0xEC  // CSB PIN is VDD level(address is 0x76)
// #define HP20X_I2C_DEV_ID2 0XEE // CSB PIN is GND level(address is 0x77)
#define HP20X_I2C_DEV_ID 0x76  // CSB PIN is VDD level(address is 0x76)
#define HP20X_I2C_DEV_ID2 0X77 // CSB PIN is GND level(address is 0x77)
#define HP20X_SOFT_RST 0x06
#define HP20X_WR_CONVERT_CMD 0x40
#define HP20X_CONVERT_OSR4096 (0 << 2)
#define HP20X_CONVERT_OSR2048 (1 << 2)
#define HP20X_CONVERT_OSR1024 (2 << 2)
#define HP20X_CONVERT_OSR512 (3 << 2)
#define HP20X_CONVERT_OSR256 (4 << 2)
#define HP20X_CONVERT_OSR128 (5 << 2)

#define HP20X_READ_P 0x30	// read_p command
#define HP20X_READ_A 0x31	// read_a command
#define HP20X_READ_T 0x32	// read_t command
#define HP20X_READ_PT 0x10	// read_pt command
#define HP20X_READ_AT 0x11	// read_at command
#define HP20X_READ_CAL 0X28 // RE-CAL ANALOG

#define HP20X_WR_REG_MODE 0xC0
#define HP20X_RD_REG_MODE 0x80

#define ERR_WR_DEVID_NACK 0x01
#define ERR_RD_DEVID_NACK 0x02
#define ERR_WR_REGADD_NACK 0x04
#define ERR_WR_REGCMD_NACK 0x08
#define ERR_WR_DATA_NACK 0x10
#define ERR_RD_DATA_MISMATCH 0x20

#define I2C_DID_WR_MASK 0xFE
#define I2C_DID_RD_MASK 0x01

#define T_WIN_EN 0X01
#define PA_WIN_EN 0X02
#define T_TRAV_EN 0X04
#define PA_TRAV_EN 0X08
#define PA_RDY_EN 0X20
#define T_RDY_EN 0X10

#define T_WIN_CFG 0X01
#define PA_WIN_CFG 0X02
#define PA_MODE_P 0X00
#define PA_MODE_A 0X40

#define T_TRAV_CFG 0X04

#define INT_SRC 0X0D

#ifdef __cplusplus
extern "C"
{
#endif

	void readAltitudeAndPressure(void);
	u8 HP203B_Check(void);

#ifdef __cplusplus
}
#endif

#endif
