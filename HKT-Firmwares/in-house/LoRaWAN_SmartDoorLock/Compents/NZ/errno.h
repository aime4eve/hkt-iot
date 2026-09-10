//-----------------------------------------------------------------------------
// errno.h
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


#ifndef __ERRNO_H_
#define __ERRNO_H_

/*!
 * Error codes to be used within the application.
 * They are represented by an u8
 */
#define ERR_NONE           0 /*!< no error occured */
#define ERR_INIT           1       
#define ERR_CMD_TIMEOUT    2   
#define ERR_KEY            3
#define ERR_ACCESS         4
#define ERR_FIFO_OFL       5
#define ERR_CRC            6
#define ERR_FRAMING        7
#define ERR_PARITY         8
#define ERR_COLL           9
#define ERR_UID            10
#define ERR_PARA           11
#define ERR_DATA           12
#define ERR_FDT            13
#define ERR_TIMEOUT        14 
#define ERR_TRANSMIT       15 
#define ERR_OVERLOAD       16
#define ERR_PROTOCOL       17
#define ERR_ONOFFFIELD     18
#define ERR_EOT_IND        19
#define ERR_EOT_IND_30MS   20
#define ERR_NOTFOUND       21
#define ERR_GENERAL        99


#endif /* ERRNO_H */

