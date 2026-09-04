
#ifndef __LORAWAN_ATCMD_H__
#define __LORAWAN_ATCMD_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

    typedef enum
    {
        AT_ERROR = 0,
        AT_OK,
        AT_TIMEOUT
    } AT_Rsp_Typedef;

    extern char *splitBuf[30];

    void cmd_string_to_hex(char *str, unsigned char *strhex);
    u16 cmd_get_hex2hexstr(u8 *hexstr, u16 hexstr_size, u8 *hex, u16 hexlen);
    u16 cmd_get_hexstr2hex(u8 *hex, u16 hex_size, const u8 *hexstr, u16 hexstr_len);
    void cmd_split(char *src, const char *separator, char **dest, int *num);
    AT_Rsp_Typedef AT_Exec_SendCMD(const char *cmd, u16 timeout, const char *keyword);
    u16 AT_Get_Keyword_Line(u8 *dst_buf, u16 bufSize, u8 *src_buf, u8 *keyword);
    u16 AT_Remove_Keyword_Line(u8 *buf, u8 *keyword);
    AT_Rsp_Typedef AT_Wait_Rsp_Keyword(u8 *buf, u8 *keyword, u16 timeout);

  /* AT 命令*/
  bool AT_LoRaWAN_checkAlive(void);
  bool AT_LoRaWAN_restFactory(void);
  bool AT_LoRaWAN_getVer(void);
  bool AT_LoRaWAN_setDebug(u8 value);
  bool AT_LoRaWAN_saveEEPROM(void);
  bool AT_LoRaWAN_systemReset(void);
  bool AT_LoRaWAN_setBaud(char *baud);
  bool AT_LoRaWAN_getBaud(void);
  bool AT_LoRaWAN_setBaudTimeout(int timeout);
  bool AT_LoRaWAN_getBaudTimeout(void);
  bool AT_LoRaWAN_setConfirm(int type, int sendCnt);
  bool AT_LoRaWAN_getConfirm(void);
  bool AT_LoRaWAN_setOTAA(void);
  bool AT_LoRaWAN_getOTAA(void);
  bool AT_LoRaWAN_setABP(void);
  bool AT_LoRaWAN_getDEVEUI(void);
  bool AT_LoRaWAN_setDEVEUI(void);
  bool AT_LoRaWAN_setAPPEUI(void);
  bool AT_LoRaWAN_getAPPEUI(void);
  bool AT_LoRaWAN_setAPPKEY(void);
  bool AT_LoRaWAN_getAPPKEY(void);
  bool AT_LoRaWAN_setDEVADDR(void);
  bool AT_LoRaWAN_getDEVADDR(void);
  bool AT_LoRaWAN_setAPPSKEY(void);
  bool AT_LoRaWAN_getAPPSKEY(void);
  bool AT_LoRaWAN_setNwkSKEY(void);
  bool AT_LoRaWAN_getNwkSKEY(void);
  bool AT_LoRaWAN_setPORT(int port);
  bool AT_LoRaWAN_getPORT(void);
  bool AT_LoRaWAN_setCSQ(int freq);
  bool AT_LoRaWAN_getCSQ(void);
  bool AT_LoRaWAN_setClass(int class);
  bool AT_LoRaWAN_getClass(void);
  bool AT_LoRaWAN_setADR(int adr);
  bool AT_LoRaWAN_getADR(void);
  bool AT_LoRaWAN_setPower(int power);
  bool AT_LoRaWAN_getPower(void);
  bool AT_LoRaWAN_setDataRate(int dataRate);
  bool AT_LoRaWAN_getDataRate(void);
  bool AT_LoRaWAN_setJOIN(void);
  bool AT_LoRaWAN_setCSMA(void);
  bool AT_LoRaWAN_getCSMA(void);
  bool AT_LoRaWAN_getStatus(void);
  bool AT_LoRaWAN_setCheckLink(int checkLink);
  bool AT_LoRaWAN_getCheckLink(void);
  bool AT_LoRaWAN_setRX2(int dataRate, int rx2);
  bool AT_LoRaWAN_getRX2(void);
  bool AT_LoRaWAN_setFREQ(int param, int channel_num, int freq);
  bool AT_LoRaWAN_setFREQBW(int param, int channel_num, int freq, int bw);
  bool AT_LoRaWAN_setFREQ_RU864(void);
  bool AT_LoRaWAN_setFREQ_IN865(void);
  bool AT_LoRaWAN_getFREQ(void);
  bool AT_LoRaWAN_setBAND(int band);
  bool AT_LoRaWAN_getBAND(void);
  bool AT_LoRaWAN_syncTime(void);
  bool AT_LoRaWAN_getRTC(void);
  bool AT_LoRaWAN_setCHMASK(int chmask);
  bool AT_LoRaWAN_getCHMASK(void);

#ifdef __cplusplus
}
#endif

#endif
