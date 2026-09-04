/*********************************************************************************************************//**
 * @file    ht32f50020_30_libcfg.h
 * @version $Rev:: 5993         $
 * @date    $Date:: 2022-06-24 #$
 * @brief   The library configuration file.
 *************************************************************************************************************
* @attention
*
* Firmware Disclaimer Information
*
* 1. The customer hereby acknowledges and agrees that the program technical documentation, including the
*    code, which is supplied by Holtek Semiconductor Inc., (hereinafter referred to as "HOLTEK") is the
*    proprietary and confidential intellectual property of HOLTEK, and is protected by copyright law and
*    other intellectual property laws.
*
* 2. The customer hereby acknowledges and agrees that the program technical documentation, including the
*    code, is confidential information belonging to HOLTEK, and must not be disclosed to any third parties
*    other than HOLTEK and the customer.
*
* 3. The program technical documentation, including the code, is provided "as is" and for customer reference
*    only. After delivery by HOLTEK, the customer shall use the program technical documentation, including
*    the code, at their own risk. HOLTEK disclaims any expressed, implied or statutory warranties, including
*    the warranties of merchantability, satisfactory quality and fitness for a particular purpose.
*
* <h2><center>Copyright (C) Holtek Semiconductor Inc. All rights reserved</center></h2>
 ************************************************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------------------------------------*/
#ifndef __ht32f50020_30_LIBCFG_H
#define __ht32f50020_30_LIBCFG_H

/* Settings ------------------------------------------------------------------------------------------------*/

#if !defined(USE_MEM_HT32F50020) && !defined(USE_MEM_HT32F50030)
#define USE_MEM_HT32F50030
#endif

#define LIBCFG_MAX_SPEED                  (16000000)

#define LIBCFG_FLASH_PAGESIZE             (1024)

#ifdef USE_MEM_HT32F50020
  #define LIBCFG_FLASH_SIZE               (LIBCFG_FLASH_PAGESIZE * 16)
  #define LIBCFG_RAM_SIZE                 (1024 * 2)
  #define LIBCFG_CHIPNAME                 (0x50020)
#endif

#ifdef USE_MEM_HT32F50030
  #define LIBCFG_FLASH_SIZE               (LIBCFG_FLASH_PAGESIZE * 31)
  #define LIBCFG_RAM_SIZE                 (1024 * 2)
  #define LIBCFG_CHIPNAME                 (0x50030)
#endif

#define LIBCFG_EXTI_4_7_GROUP             (1)
#define LIBCFG_SCTM0                      (1)
#define LIBCFG_SCTM1                      (1)
#define LIBCFG_SCTM2                      (1)
#define LIBCFG_GPIOC                      (1)
#define LIBCFG_ADC_CH8_11                 (1)
#define LIBCFG_NO_USART0                  (1)

#endif
