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

#ifndef __ISO_14443_A_H_
#define __ISO_14443_A_H_

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
#define ISO14443A_MAX_UID_LENGTH 	 10
#define ISO14443A_MAX_CASCADE_LEVELS 3
#define ISO14443A_CASCADE_LENGTH 	 7
#define ISO14443A_RESPONSE_CT  		 0x88

/*
******************************************************************************
* GLOBAL DATATYPES
******************************************************************************
*/
/*!< 
 * struct representing an ISO14443A PICC as returned by
 * #iso14443ASelect.
 */
typedef struct  //_AProximityCard_s
{
    u8 uid[ISO14443A_MAX_UID_LENGTH]; /*<! UID of the PICC */
    u8 actlength;     /*!< actual UID length */
    u8 atqa[2];       /*!< content of answer to request byte */
    u8 sak[ISO14443A_MAX_CASCADE_LEVELS]; /*!< SAK bytes */
    u8 cascadeLevels; /*!< number of cascading levels */
    bool collision;   /*!< TRUE, if there was a collision which has been resolved,
                        otherwise no collision occured */
	u8 fsdi;          /*!< Frame size integer of the PCD. */

	u8 fwi;           /*!< Frame wait integer of the PICC. */
    u8 fsci;          /*!< Frame size integer of the PICC. */
    u8 sfgi;          /*!< Special frame guard time of the PICC. */
    u8 TA;            /*!< Data rate bits PICC->PCD. Data rate bits PCD->PICC.*/
}iso14443AProximityCard_t;

/*! 
 * PCD command set.
 */
typedef enum
{
    ISO14443A_CMD_REQA = 0x26,  /*!< command REQA */
    ISO14443A_CMD_WUPA = 0x52, /*!< command WUPA */
    ISO14443A_CMD_SELECT_CL1 = 0x93, /*!< command SELECT cascade level 1 */
    ISO14443A_CMD_SELECT_CL2 = 0x95, /*!< command SELECT cascade level 2 */
    ISO14443A_CMD_SELECT_CL3 = 0x97, /*!< command SELECT cascade level 3 */
    ISO14443A_CMD_HLTA = 0x50, /*!< command HLTA */
    ISO14443A_CMD_PPSS = 0xd0, /*!< command PPSS */
    ISO14443A_CMD_RATS = 0xe0, /*!< command RATS */
}iso14443ACommand_t;

/*
******************************************************************************
* GLOBAL FUNCTION PROTOTYPES
******************************************************************************
*/
extern u8 iso14443AInitialize(void);
extern u8 iso14443ASelect(iso14443ACommand_t cmd, iso14443AProximityCard_t* card);
extern u8 iso14443ASendHlta(void);
extern u8 iso14443AEnterProtocolMode(iso14443AProximityCard_t* card, u8* answer, u8* length);
extern u8 iso14443ASendProtocolAndParameterSelection(u8 pps1);

extern u8 ApduTransceive(u8 *inf, u16 infLength, u8 *response, u16 *responseLength);

extern u8 Iso14443ADemonstrate(void);

#endif /* __ISO_14443_A_H_ */

