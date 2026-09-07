#ifndef __CRC_H__
#define __CRC_H__

#include "config.h"

void Init_CRC_CRC_CCITT(void);
u16 CalCRC16_CCITT(u16 Init, u8 *DataIn, u16 Len);

#endif
