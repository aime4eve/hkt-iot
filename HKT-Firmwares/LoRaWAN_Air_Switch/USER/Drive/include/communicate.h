
#ifndef __COMMUNICATE_H__
#define __COMMUNICATE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

  extern u8 lwrb_write_count;
  extern lwrb_t lwrbBuff_t;
  extern u8 sendNum;
  extern u8 sendBuffer[100];

  /* LoRaWAN 下行数据解析结构体 (+radiorx / +macrx) */
  typedef struct {
      int rssi;                  // 信号强度
      int snr;                   // 信噪比
      int port;                  // LoRaWAN 端口号 (仅 +macrx 有效)
      u8  raw_data[256];         // 解析后的原始数据
      int raw_len;               // 原始数据长度
      int parse_ok;              // 解析成功标志: 1=成功, 0=失败
  } RadioRxData;

  void sendLoRaWANData(void);
  void parse_radiorx(const char *recv, RadioRxData *rx_data);
  void fromLoRaWANDataHandle(u8 *data, u16 len);
  void fromInfoDataHandle(u8 *data, u16 len);
  void fromBTDataHandle(u8 *data, u16 len);
  void fromUartDataHandle(void);
  void DeviceStateProcess(void);
  void my_buff_evt_fn(lwrb_t *buff, lwrb_evt_type_t type, size_t len);
  void lwrbDataInit(void);

#ifdef __cplusplus
}
#endif

#endif
