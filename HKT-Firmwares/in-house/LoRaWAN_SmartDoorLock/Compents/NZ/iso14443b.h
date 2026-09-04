//-----------------------------------------------------------------------------
// nz3801-ab.h
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

#ifndef __ISO_14443_B_H_
#define __ISO_14443_B_H_


/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "nz3801-ab_cfg.h"


/*
******************************************************************************
* GLOBAL DEFINES
******************************************************************************
*/
#define ISO14443B_PUPI_LENGTH        4
#define ISO14443B_APPDATA_LENGTH     4
#define ISO14443B_PROTINFO_LENGTH    3

#define ISO14443B_PARAM_APF          0x5
#define ISO14443B_MAX_AC_LOOP_COUNT  5

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*! 
 * PCD command set.
 */
typedef enum
{
    ISO14443B_CMD_REQB   = 0x00, /*!< command REQB */
    ISO14443B_CMD_WUPB   = 0x08, /*!< command WUPB */
    ISO14443B_CMD_HLTB   = 0x50, /*!< command HLTB */
    ISO14443B_CMD_ATTRIB = 0x1D, /*!< command ATTRIB */
}iso14443BCommand_t;

/*! 
 * slot count (N parameter) used for iso14443b anti collision
 */
typedef enum
{
    ISO14443B_SLOT_COUNT_1  = 0,
    ISO14443B_SLOT_COUNT_2  = 1,
    ISO14443B_SLOT_COUNT_4  = 2,
    ISO14443B_SLOT_COUNT_8  = 3,
    ISO14443B_SLOT_COUNT_16 = 4,
}iso14443BSlotCount_t;

/*!  
 * struct representing the content of ATQB
 * See ISO14443b spec. for more information.
 */
typedef struct
{
    u8 atqb; /*<! content of answer to request byte */
    u8 pupi[ISO14443B_PUPI_LENGTH]; /*!< UID of the PICC */
    u8 applicationData[ISO14443B_APPDATA_LENGTH]; /*!< application specific data */
    u8 protocolInfo[ISO14443B_PROTINFO_LENGTH]; /*!< protocol info */
    bool collision; /*!< TRUE, if there was a collision which has been resolved,
                        otherwise no collision occured */
}iso14443BProximityCard_t;

/*! 
 * struct holding parameter needed for ATTRIB command
 * The parameters are called param1, param2, param3 and
 * param4 and are described in ISO14443-3 spec.
 */
typedef struct
{
    u8 param1;
    u8 param2;
    u8 param3;
    u8 param4;
}iso14443BAttribParameter_t;

/*! 
 * struct holding the answer to ATTRIB command
 */
typedef struct
{
    u8 mbli; /*!< maximum buffer length */
    u8 cid;  /*!< logical card identifier */
}iso14443BAttribAnswer_t;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/
extern u8 iso14443BInitialize(void);

#if 0
extern u8 iso14443BSelect(iso14443BCommand_t cmd,
                        iso14443BProximityCard_t* card,
                        u8 afi,
                        iso14443BSlotCount_t slotCount);
						
#else
extern u8 iso14443BSelect(iso14443BCommand_t cmd,
                iso14443BProximityCard_t* card,
                u8 afi);
#endif

extern u8 iso14443BSendHltb(iso14443BProximityCard_t* card);

extern u8 iso14443BEnterProtocolMode(iso14443BProximityCard_t* card,
                                iso14443BAttribParameter_t* param,
                                iso14443BAttribAnswer_t* answer);

extern u8 Iso14443BDemonstrate(void);

#endif /* __ISO_14443_B_H_ */

