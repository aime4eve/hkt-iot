//-----------------------------------------------------------------------------
// nz3801-ab_cfg.h
//-----------------------------------------------------------------------------
// Copyright 2017 nationz Ltd, Inc.
// http://www.nationz.com
//
// Program Description:
//
// driver definitions for the nfc reader.
//
//
//      PROJECT:   NZ3801-AB firmware
//      $Revision: $
//      LANGUAGE:  ANSI C
//
//
// Release 1.0
//    -Initial Revision (NZ)
//    -14 Feb 2017
//    -Latest release before new firmware coding standard
//


#ifndef __NZ3801_AB_CFG_H_
#define __NZ3801_AB_CFG_H_

#include "config.h"
#include "gpio.h"


/*
******************************************************************************
* HW RST 
******************************************************************************
*/
#define RF_RST_Enable        CARD_RST_H
#define RF_RST_Disable       CARD_RST_L

/*
******************************************************************************
* DELAY 
******************************************************************************
*/
#define SleepMS              tx_thread_sleep         
#define SleepUs              TicksDelayUs        

/*
******************************************************************************
* FIELD RETRY 
******************************************************************************
*/
#define FIELD_ONOFF_RETRY_EN  0                //开关场重试: 1使能，0禁止

/*
******************************************************************************
* DEMO 
******************************************************************************
*/
#define ISO14443B_EN          1                //使能TYPEB 功能
#define ISO14443A_EN          1                //使能TYPEA 功能

#endif /* __SYS_CFG_H_ */



