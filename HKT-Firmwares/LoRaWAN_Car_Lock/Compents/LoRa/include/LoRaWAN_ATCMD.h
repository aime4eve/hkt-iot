
#ifndef __LORAWAN_ATCMD_H__
#define __LORAWAN_ATCMD_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <config.h>

typedef enum {
    AT_ERROR = 0,
    AT_OK,
    AT_TIMEOUT
} AT_Rsp_Typedef;

//#define __LL_RTC_CONVERT_BIN2BCD(__VALUE__) (uint8_t)((((__VALUE__) / 10U) << 4U) | ((__VALUE__) % 10U))
//#define __LL_RTC_CONVERT_BCD2BIN(__VALUE__) (uint8_t)((((uint8_t)((__VALUE__) & (uint8_t)0xF0U) >> (uint8_t)0x4U) * 10U) + ((__VALUE__) & (uint8_t)0x0FU))

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
bool AT_LoRaWAN_restFactory(void);
bool AT_LoRaWAN_reboot(void);
bool AT_LoRaWAN_getVer(void);
bool AT_LoRaWAN_getMode(void);
bool AT_LoRaWAN_setLoRaWANMode(void);
bool AT_LoRaWAN_setJOIN(int isABP);
bool AT_LoRaWAN_setJOIN_Close(int isABP);
bool AT_LoRaWAN_getJOIN(void);
bool AT_LoRaWAN_getJOIN_1(void);
bool AT_LoRaWAN_setClass(int class);
bool AT_LoRaWAN_getClass(void);
bool AT_LoRaWAN_setDataRate(int dataRate);
bool AT_LoRaWAN_getDataRate(void);
bool AT_LoRaWAN_setADR(int adr);
bool AT_LoRaWAN_getADR(void);
bool AT_LoRaWAN_setRX2(int dataRate, int rx2);
bool AT_LoRaWAN_getRX2(void);
bool AT_LoRaWAN_setCHMASK(uint32_t freq);
bool AT_LoRaWAN_getCHMASK(void);
bool AT_LoRaWAN_getDEVEUI(void);
bool AT_LoRaWAN_setDEVEUI(void);
bool AT_LoRaWAN_setAPPEUI(void);
bool AT_LoRaWAN_getAPPEUI(void);
bool AT_LoRaWAN_setAPPKEY(void);
bool AT_LoRaWAN_getAPPKEY(void);
bool AT_LoRaWAN_setDEVADDR(void);
bool AT_LoRaWAN_getDEVADDR(void);
bool AT_LoRaWAN_setAPPSKEY(void);
bool AT_LoRaWAN_setNwkSKEY(void);
bool AT_LoRaWAN_setPower(int power);
bool AT_LoRaWAN_getPower(void);
bool AT_LoRaWAN_setPORT(int port);
bool AT_LoRaWAN_getPORT(void);
bool AT_LoRaWAN_setFccd(int freq);
bool AT_LoRaWAN_getFccd(void);
bool AT_LoRaWAN_setRxPrLen(void);
bool AT_LoRaWAN_getRxPrLen(void);
bool AT_LoRaWAN_setSleep(void);
bool AT_LoRaWAN_getSleep(void);
bool AT_LoRaWAN_setIrq(void);
bool AT_LoRaWAN_getIrq(void);
bool AT_LoRaWAN_setATExit(void);
bool AT_LoRaWAN_setATEnter(void);
bool AT_LoRaWAN_setBuad(void);
bool AT_LoRaWAN_setComMode(void);
bool AT_LoRaWAN_getParam(void);
bool AT_LoRaWAN_getBand(void);
bool AT_LoRaWAN_setBand(char *band);
bool AT_LoRaWAN_getLpcpin(void);
bool AT_LoRaWAN_getRx1(void);
bool AT_LoRaWAN_getMacMode(void);
bool AT_LoRaWAN_getRxChext(void);
bool AT_LoRaWAN_setJoinAdrOFF(void);
#endif

#ifdef __cplusplus
}
#endif
