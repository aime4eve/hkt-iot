#ifndef	__DEFINEALL_H__
#define __DEFINEALL_H__

//定义常量, 常数

//系统时钟默认使用RCHF
#define RCHFCLKCFG	32	//8, 16, 24,32MHZ

//define_all.h中RCHFCLKCFG控制系统时钟
#if( RCHFCLKCFG == 8 )//8.0MHz
#define clkmode   1
#define SYSCLKdef CMU_RCHFCR_FSEL_8MHZ//RCHF中心频率8MHz 
#elif( RCHFCLKCFG == 16 )//16.0MHz
#define clkmode   2
#define SYSCLKdef CMU_RCHFCR_FSEL_16MHZ//RCHF中心频率16MHz
#elif( RCHFCLKCFG == 24 )//24.0MHz
#define clkmode   3
#define SYSCLKdef CMU_RCHFCR_FSEL_24MHZ//RCHF中心频率24MHz
#elif( RCHFCLKCFG == 32 )//32.0MHz
#define clkmode   4
#define SYSCLKdef CMU_RCHFCR_FSEL_32MHZ//RCHF中心频率32MHz
#elif( RCHFCLKCFG == 40 )//40.0MHz
#define clkmode   5
#define SYSCLKdef CMU_RCHFCR_FSEL_40MHZ//RCHF中心频率40MHz

#elif( RCHFCLKCFG == 48 )//48.0MHz
#define clkmode   6
#define SYSCLKdef CMU_RCHFCR_FSEL_48MHZ//RCHF中心频率48MHz
#endif
 
/*变量类型定义*/
typedef union
{
  unsigned char B08;
  struct
  {
    unsigned char bit0:1;
    unsigned char bit1:1;
    unsigned char bit2:1;
    unsigned char bit3:1;
    unsigned char bit4:1;
    unsigned char bit5:1;
    unsigned char bit6:1;
    unsigned char bit7:1;
  }Bit;
}B08_Bit;



/* GPIO配置函数参数宏定义 */
//IO输入口配置 
//type 0 = 普通 
//type 1 = 上拉
#define IN_NORMAL	0
#define IN_PULLUP	1

//IO输出口配置 
//type 0 = 普通 
//type 1 = OD
#define OUT_PUSHPULL	0
#define OUT_OPENDRAIN	1

//IO数字特殊功能口 
//type 0 = 普通 
//type 1 = OD (OD功能仅部分特殊功能支持)
//type 2 = 普通+上拉 
//type 3 = OD+上拉
#define ALTFUN_NORMAL			0
#define ALTFUN_OPENDRAIN		1
#define ALTFUN_PULLUP			2
#define ALTFUN_OPENDRAIN_PULLUP	3


/*include*/
#include "FM33A0XXEV.h"
#include "fm33a0xxev_include_all.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "stdlib.h"
#include <math.h>
#include <ctype.h>
#include "time.h"
#include "bintohex.h"
#include "user_init.h"

#endif



