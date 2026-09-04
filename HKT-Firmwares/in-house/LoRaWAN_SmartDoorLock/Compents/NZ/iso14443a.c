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
/*! \file
 *
 *
 *  \brief Implementation of ISO-14443A
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "errno.h"
#include "nz3801-ab_cfg.h"
#include "nz3801-ab_com.h"
#include "nz3801-ab.h"
#include "iso14443a.h"

#if ISO14443A_EN
/*
******************************************************************************
* LOCAL MACROS
******************************************************************************
*/
#define TYPEA_LOG_EN
#ifdef TYPEA_LOG_EN
#define ISO_14443A_DBG INFO
#define ISO_14443A_DBG_EXT PrintInfoDumpExt
#else
#define ISO_14443A_DBG(...)
#define ISO_14443A_DBG_EXT(...)
#endif

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/
static u8 iso14443ADoAntiCollisionLoop(u8 UIDLen, iso14443AProximityCard_t *card);

/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
u8 iso14443AInitialize(void)
{
    nz3801Init(CT_A);
    return ERR_NONE;
}

u8 iso14443ASelect(iso14443ACommand_t cmd, iso14443AProximityCard_t *card)
{
    u8 err = ERR_NONE;
    u8 buf[2];
    u8 atqalen;
    u8 UIDLen;

    if ((cmd != ISO14443A_CMD_REQA) && (cmd != ISO14443A_CMD_WUPA))
        return ERR_PARA;

    buf[0] = cmd;
    err = nz3801Transceive(TA_WUPA, buf, 1, 7, card->atqa, &atqalen, 0);
    if (err == ERR_TIMEOUT)
    {
        if (nzFlagOK())
        {
            return ERR_TIMEOUT;
        }
    }
    if (err != ERR_NONE)
    {
        return ERR_PARA;
    }
    if (atqalen != 2) // 16 bits ?
    {
        return ERR_DATA;
    }

    if (!nzFlagOK() || !nzCrcOK())
    {
        return ERR_DATA;
    }

    if (card->atqa[1] == 0xf0) // 2.5,20160714
    {
        return ERR_DATA;
    }

    UIDLen = ((card->atqa[0]) >> 6) & 0x3;
    err = iso14443ADoAntiCollisionLoop(UIDLen, card);

    return err;
}

u8 iso14443ASendHlta(void)
{
    u8 err;
    u8 buf[2];
    u8 rec[256];
    u8 len;

    /* send HLTA command */
    buf[0] = ISO14443A_CMD_HLTA;
    buf[1] = 0;
    err = nz3801Transceive(TA_HLTA, buf, 2, 0, rec, &len, 0);
    return err;
}

u8 iso14443AEnterProtocolMode(iso14443AProximityCard_t *card, u8 *answer, u8 *length)
{
    u8 err;
    u8 buf[2];
    u8 TA, TB, TC;
    u32 SFGT = 0;

    if (!answer || !length)
        return ERR_PARA;

    /* send RATS command */
    buf[0] = ISO14443A_CMD_RATS;
    buf[1] = (card->fsdi << 4) | 0x0; // = 0x80 : FSD=256,cid=0

    BlockNum = 0;
    FWI = 4;
    FSD = 256;
    FSC = 32;
    CID = 0;
    NAD = 0;
    err = nz3801Transceive(TA_RATS, buf, 2, 0, answer, length, 0);
    if (err == ERR_TIMEOUT) // 直接超时
        return ERR_TIMEOUT;
    if (err != ERR_NONE)
        return ERR_PARA;
    if (!nzFlagOK())
        return ERR_PARA;
    if (!nzCrcOK())
        return ERR_DATA;

    if (answer[0] == 0 || answer[0] != *length)
        return ERR_PROTOCOL;
    if (answer[0] == 1) // 无T0，无后续数据
        return ERR_NONE;

    TA = TB = TC = 0;
    if ((answer[1] & BIT4) != 0)
        TA = 1;
    if ((answer[1] & BIT5) != 0)
        TB = 1;
    if ((answer[1] & BIT6) != 0)
        TC = 1;
    if (answer[0] < (TA + TB + TC + 2))
        return ERR_PROTOCOL;

    // T0
    card->fsci = answer[1] & 0x0f;
    if (card->fsci >= 8)
        FSC = 256;
    else
        FSC = FSCTab[card->fsci];

    // TA中DR/DS可同时为0，所以该字节任何数据都是正确的
    if (TA)
    {
        card->TA = answer[2]; // 详细见-4协议
    }
    // TB值
    if (TB)
    {
        card->fwi = (answer[2 + TA] >> 4) & 0xf;
        card->sfgi = answer[2 + TA] & 0xf;
        SFGT = card->sfgi;
        if (SFGT == 15)
        {
            card->sfgi = 0;
            SFGT = 0;
        }
        if (card->fwi == 15)
        {
            card->fwi = 4;
        }
        FWI = card->fwi;
        if (card->fwi > 15 || SFGT > 15)
        {
            return ERR_PROTOCOL;
        }
    }
    // TC值没有限制，所有值合法
    if (TC)
    {
        NAD = answer[2 + TA + TB] & 0x01;
        CID = (answer[2 + TA + TB] >> 1) & 0x01;
    }

    if (SFGT) // 延时SFGT
    {
        nzClearFlag();
        nz3801SetTimer((256 * 16 + 384) * power(SFGT));
        nzSetBitMask(CONTROL, BFL_JBIT_TSTARTNOW); // 手动启动
        while ((nzReadReg(COMMIRQ) & BFL_JBIT_TIMERI) == 0)
            ;
    }

    return ERR_NONE;
}

u8 iso14443ASendProtocolAndParameterSelection(u8 pps1)
{
    u8 err;
    u8 buf[3], rec[256];
    u8 len;

    /* send PPS command */
    buf[0] = ISO14443A_CMD_PPSS | 0x0;
    buf[1] = 0x11;
    buf[2] = pps1;
    err = nz3801Transceive(TA_PPS, buf, 3, 0, rec, &len, 0);
    if (err == ERR_TIMEOUT) // 直接超时
        return ERR_TIMEOUT;
    if (err != ERR_NONE)
        return ERR_PARA;
    if (!nzFlagOK())
        return ERR_PARA;
    if (!nzCrcOK())
        return ERR_DATA;

    if (rec[0] != buf[0] || len != 1)
        return ERR_PROTOCOL;

    return ERR_NONE;
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
#if 0
static u8 iso14443ADoAntiCollisionLoop(u8 UIDLen,iso14443AProximityCard_t *card)
{
    u8 cmd[20], rec[256];
    u8 i, len;
    u8 err;
    eCmd cmdtype;
    u8 cl;
    u8 timer = 0;

	card->actlength = 0;
	mem_copy(cmd, (u8*)"\x93\x20", 2);
	// 0 - 5:  0,2,4发送ant;  1,3,5发送select
	for(i=0; i<(UIDLen+1)*2; i++)
	{
		cl = i/2;
		cmd[0] = 0x93+(i/2)*2;	// 0x93, 0x95, 0x97
		if((i&1)==0)	// ant
		{
			len = 2;
			cmd[1] = 0x20;
			cmdtype = TA_ANT;
		}
		else 			// select
		{
			len = 2+4+1;
			cmd[1] = 0x70;
			cmdtype = TA_SELECT;
		}
        
        //ISO_14443A_DBG_EXT(cmd,len,"TX:");
		err = nz3801Transceive(cmdtype, cmd, len, 0, rec, &len, 0);
        
        //ISO_14443A_DBG_EXT(rec,len,"RX:");  
        
		if(err==ERR_TIMEOUT)
		{
			if(nzFlagOK())
			{
				timer++;
				i--;		// 可能是0xff
				if(timer >= 3)
					return ERR_TIMEOUT;
				else
					continue;
			}
		}
		timer = 0;
		if(err!=ERR_NONE)
        {
			return ERR_PARA;
		}

		if(!nzFlagOK())
		{
		    return ERR_DATA;
		}
		if((i&1)==0)	// ant，对数据内容没有限制
		{
			u8 j, bcc = 0;

			mem_copy(card->uid+card->actlength, rec, 4);
			card->actlength += 4;
			if(len!=5) 
            {
                return ERR_DATA;
			}
            for(j=0; j<5; j++)	
            {
                bcc ^= rec[j];
			}
            if(bcc!=0)	
            {
                return ERR_DATA;
			}
            cl = i/2;
			if(rec[0]==0x88)// 已经是最后一级时报错
			{
				if(cl==UIDLen)
                {
					return ERR_DATA;
				}
			}
			else// 不是最后一级报错
			{
				if(cl!=UIDLen)
                {
					//return ERR_DATA;
				    UIDLen = cl;// 2.5,20160714,TA102中存在多重UID没发的情况
				}
			}
			mem_copy(cmd+2, rec, 5);
		}
		else// select
		{
			card->sak[0] = rec[0];
			if(!nzCrcOK())
            {
				return ERR_DATA;
			}
            if(len!=1)
			{
			    return ERR_DATA;
			}
            if(cl==UIDLen && (rec[0]&BIT2)!=0)// 最后一级仍然显示UID不完整
			{
			    return ERR_DATA;
			}
            if((rec[0]&BIT2)==0)
			{
			    return ERR_NONE;
            }
		}

	}

	return ERR_DATA;// UID指示的级数与实际SAK不一致
}
#endif

static u8 iso14443AVerifyBcc(const u8 *uid, u8 length, u8 bcc)
{
    u8 actbcc = 0;

    do
    {
        length--;
        actbcc ^= uid[length];
    } while (length);

    if (actbcc != bcc)
    {
        return ERR_CRC;
    }

    return ERR_NONE;
}

static u8 iso14443ADoAntiCollisionLoop(u8 UIDLen, iso14443AProximityCard_t *card)
{
    u8 cscs[ISO14443A_MAX_CASCADE_LEVELS][ISO14443A_CASCADE_LENGTH];
    u8 cl = ISO14443A_CMD_SELECT_CL1;
    u8 regcoll;
    u8 regerr;
    u8 bytes = 0;
    u8 bits = 0;
    u8 txlength = 2;
    u8 rxlength;
    u8 saklength;
    u8 savecoll = 0;
    u8 err;
    u8 i;
    u8 *buf;

    nzClearBitMask(STATUS2, 0x08);
    nzWriteReg(BITFRAMING, 0x00);
    nzClearBitMask(COLL, 0x80);

    card->cascadeLevels = 0;
    card->collision = false;
    card->actlength = 0;
    buf = cscs[card->cascadeLevels];
    /* start anticollosion loop by sending SELECT command and NVB 0x20 */
    buf[1] = 0x20;

    do
    {
        buf[0] = cl;
        // ISO_14443A_DBG_EXT(buf, txlength, "T");
        err = nz3801Transceive(TA_ANT, buf, txlength, bits, buf + 2, &rxlength, bits);
        if (err != ERR_NONE)
            goto out;
        // ISO_14443A_DBG("Received %d bytes\r\n", rxlength);
        // ISO_14443A_DBG_EXT(buf+2, rxlength, "R");

        /* now check for collision */
        regcoll = nzReadReg(COLL); /* read out collision register */
        regerr = nzReadReg(REGERROR);
        if ((!(regcoll & 0x20)) || (regerr & BFL_JBIT_COLLERR))
        {
            // ISO_14443A_DBG("Has Collision!\r\n");
            // ISO_14443A_DBG("regcoll=%d\r\n", (regcoll&0x1F));
            card->collision = true;
            /* save the colision byte */
            savecoll = buf[2 + (regcoll & 0x1F) / 8];

            bits = (regcoll & 0x1F) % 8;
            bytes = (regcoll & 0x1F) / 8;
            // bits++;
            // if (bits==8)
            //{
            //     bits = 0;
            //     bytes++;
            // }

            /* FIXME handle c_pb collision in parity bit */
            /* update NVB. Add 2 bytes for SELECT and NVB itself */
            buf[1] = 0x20 + (bytes << 4) + (bits & 0xF);
            if (bits != 0)
            {
                txlength = bytes + 2 + 1;
            }
            else
            {
                txlength = bytes + 2;
            }
            // ISO_14443A_DBG("Bytes=%d,Bits=%d,txlength=%d,savecoll=0x%02x\r\n",bytes,bits,txlength,savecoll);
        }
        else
        {
            // ISO_14443A_DBG("Got a frame\r\n");
            /* got a frame w/o collision - store the uid and check for CT */

            /* answer with complete uid and check for SAK. */
            buf[1] = 0x70;
            buf[0] = cl;
            buf[2 + bytes] |= savecoll;
            txlength = rxlength + 2;

            // ISO_14443A_DBG("Request SAK\r\n");
            // ISO_14443A_DBG_EXT(buf, txlength, "T");
            err = nz3801Transceive(TA_SELECT,
                                   buf, txlength, 0, &card->sak[card->cascadeLevels], &saklength, 0);
            if (err != ERR_NONE)
                goto out;
            // ISO_14443A_DBG("Got SAK = 0x%02x\r\n",card->sak[card->cascadeLevels]);
            if (!nzCrcOK())
            {
                err = ERR_DATA;
                goto out;
            }
            if (saklength != 1)
            {
                err = ERR_DATA;
                goto out;
            }

            if (card->sak[card->cascadeLevels] & 0x4)
            {
                // ISO_14443A_DBG("Next cascading level\r\n");
                /* reset variables for next cascading level */
                txlength = 2;
                bytes = 0;
                bits = 0;

                if (ISO14443A_CMD_SELECT_CL1 == cl)
                {
                    cl = ISO14443A_CMD_SELECT_CL2;
                }
                else if (ISO14443A_CMD_SELECT_CL2 == cl)
                {
                    cl = ISO14443A_CMD_SELECT_CL3;
                }
                else
                {
                    /* more than 3 cascading levels are not possible ! */
                    err = ERR_COLL;
                    goto out;
                }
            }
            else
            {
                // ISO_14443A_DBG("UID done\r\n");
                card->cascadeLevels++;
                break;
            }
            card->cascadeLevels++;
            buf = cscs[card->cascadeLevels];
            buf[0] = cl;
            buf[1] = 0x20;
        } /* no collision detected */

    } while (card->cascadeLevels <= ISO14443A_MAX_CASCADE_LEVELS);

    /* do final checks... */
    for (i = 0; i < card->cascadeLevels; i++)
    {
        err = iso14443AVerifyBcc(&cscs[i][2], 4, cscs[i][6]);
        if (err != ERR_NONE)
            goto out;
    }

    // ISO_14443A_DBG("cascadeLevels=0x%x\r\n", card->cascadeLevels);

    /* extract pure uid */
    switch (card->cascadeLevels)
    {
    case 3:
        memmove(card->uid + 6, cscs[2] + 2, 4);
        memmove(card->uid + 3, cscs[1] + 3, 3);
        memmove(card->uid + 0, cscs[0] + 3, 3);
        card->actlength = 10;
        break;
    case 2:
        memmove(card->uid + 3, cscs[1] + 2, 4);
        memmove(card->uid + 0, cscs[0] + 3, 3);
        card->actlength = 7;
        break;
    case 1:
        memmove(card->uid + 0, cscs[0] + 2, 4);
        card->actlength = 4;
        break;
    default:
        err = ERR_COLL;
        goto out;
    }

out:
    nzSetBitMask(COLL, 0x80);
    return err;
}

u8 ApduTransceive(u8 *inf, u16 infLength, u8 *response, u16 *responseLength)
{
    if (!inf || !response || !responseLength)
        return ERR_DATA;
    return nz3801IBLOCK(inf, infLength, response, responseLength);
}

//-----------------------------------------------------------------------------
u8 Iso14443ADemonstrate(void)
{
    const u8 iAPDU_GetRandom[5] = {0x00, 0x84, 0x00, 0x00, 0x08};
    const u8 iAPDU_SelectMainFile[7] = {0x00, 0xA4, 0x00, 0x00, 0x02, 0x3f, 0x00};
    u8 err;
    u8 buf[260];
    u8 clength;
    u16 ilength;
    iso14443AProximityCard_t gvCardA;

    ISO_14443A_DBG("\r\n\r\n******Type A Card Present***********************\r\n");
    iso14443AInitialize();
    err = nz3801ActivateField(false);
    if (err != ERR_NONE)
    {
        SuccessRate.numFieldOffFail++;
        ISO_14443A_DBG(">Field OFF\r\n FAIL.\r\n");
        return err;
    }
    SleepMS(5);
    err = nz3801ActivateField(true);
    if (err != ERR_NONE)
    {
        SuccessRate.numFieldOnFail++;
        ISO_14443A_DBG(">Field ON\r\n FAIL.\r\n");
        return err;
    }
    SleepMS(15);

    SuccessRate.Totality++;
    // REQAorWUPA & ANTICOLLION
    err = iso14443ASelect(ISO14443A_CMD_WUPA, &gvCardA);
    if (err == ERR_NONE)
    {
        ISO_14443A_DBG(">REQA\r\n OK.\r\n");
        ISO_14443A_DBG(" YES Card.\r\n");
        ISO_14443A_DBG_EXT(gvCardA.uid, gvCardA.actlength, " UID:");
        SuccessRate.numL3OK++;
    }
    else
    {
        ISO_14443A_DBG(">REQA\r\n FAIL.[%d]\r\n", err);
        ISO_14443A_DBG(" NO Card!!!\r\n");
    }

    // RATS
    if (err == ERR_NONE)
    {
        gvCardA.fsdi = 8; // fsdi=8 -> FSD=256
        err = iso14443AEnterProtocolMode(&gvCardA, buf, &clength);
        if (err == ERR_NONE)
        {
            ISO_14443A_DBG(">RATS\r\n OK.\r\n");
            ISO_14443A_DBG_EXT(buf, clength, " ATS:");
            // ISO_14443A_DBG(" FSCI=%d\r\n FWI=%d\r\n TA=%02x\r\n",gvCardA.fsci,gvCardA.fwi,gvCardA.TA);
        }
        else
        {
            ISO_14443A_DBG(">RATS\r\n FAIL.[%d]\r\n", err);
        }
    }

#if 0    
    // PPS 
    if((err==ERR_NONE) && (PPSEn))
    {
        u8 pps1 = 0x0; // 106K
        if(TxRxSpeed==212) pps1 = 0x5;
        else if(TxRxSpeed==424) pps1 = 0xA;
        else if(TxRxSpeed==848) pps1 = 0xF;
        else /*if(TxRxSpeed==106)*/ pps1 = 0x0;
        
        err = iso14443ASendProtocolAndParameterSelection(pps1);
        if(err==ERR_NONE)
        {
            ISO_14443A_DBG(">PPS\r\n OK.\r\n");
            // here set tx&rxbit speed
            if(pps1==0x5)
            {
                nzWriteReg(MODWIDTH, reg24h_MODWIDTH);
                nzSetBitMask(TXMODE, BFL_JBIT_212KBPS);
                nzSetBitMask(RXMODE, BFL_JBIT_212KBPS);
                ISO_14443A_DBG(" SPEED=212kBd.\r\n");
            }
            else if(pps1==0xA)
            {
                nzWriteReg(MODWIDTH, reg24h_MODWIDTH);
                nzSetBitMask(TXMODE, BFL_JBIT_424KBPS);
                nzSetBitMask(RXMODE, BFL_JBIT_424KBPS);
                ISO_14443A_DBG(" SPEED=424kBd.\r\n");
            }
            else if(pps1==0xF)
            {
                nzWriteReg(MODWIDTH, reg24h_MODWIDTH);
                nzSetBitMask(TXMODE, BFL_JBIT_848KBPS);
                nzSetBitMask(RXMODE, BFL_JBIT_848KBPS);
                ISO_14443A_DBG(" SPEED=848kBd.\r\n");
            }  
            else
            {
                ISO_14443A_DBG(" SPEED=106kBd.\r\n");
            }
        }
        else
        {               
            ISO_14443A_DBG(">PPS\r\n FAIL.[%d]\r\n",err);
            ISO_14443A_DBG(" SPEED=106kBd.\r\n");
        }
        err = ERR_NONE; 
    }
#endif

    // APDU
    if (err == ERR_NONE)
    {
        err = ApduTransceive((u8 *)iAPDU_GetRandom, sizeof(iAPDU_GetRandom), buf, &ilength);
        if (err == ERR_NONE)
        {
            ISO_14443A_DBG(">APDU-GetRandom\r\n OK.\r\n");
            ISO_14443A_DBG_EXT((u8 *)iAPDU_GetRandom, sizeof(iAPDU_GetRandom), " TX:");
            ISO_14443A_DBG_EXT(buf, ilength, " RX:");
            SuccessRate.numL4OK++;
        }
        else
        {
            ISO_14443A_DBG(">APDU-GetRandom\r\n FAIL.[%d]\r\n", err);
        }
    }

#if 0
    if(err==ERR_NONE)
    {
        err = ApduTransceive((u8*)iAPDU_SelectMainFile,sizeof(iAPDU_SelectMainFile),buf,&ilength);
        if(err==ERR_NONE)
        {
            ISO_14443A_DBG(">APDU-SelectMainFile\r\n OK.\r\n");
            ISO_14443A_DBG_EXT((u8*)iAPDU_SelectMainFile,sizeof(iAPDU_SelectMainFile)," TX:");
            ISO_14443A_DBG_EXT(buf,ilength," RX:");
			SuccessRate.numL4OK++;
        }
        else
        {
            ISO_14443A_DBG(">APDU-SelectMainFile\r\n FAIL.[%d]\r\n",err);
        }
    }
#endif

    //    ISO_14443A_DBG("<<RFOnFail=%d,RFOffFail=%d,L3Pass=%d,L4Pass=%d,Totality=%d\r\n",
    //        SuccessRate.numFieldOnFail,SuccessRate.numFieldOffFail,
    //        SuccessRate.numL3OK,SuccessRate.numL4OK,SuccessRate.Totality);

    ISO_14443A_DBG("<<RFOnFail=%d,RFOffFail=%d,ApduPass=%d,Totality=%d\r\n",
                   SuccessRate.numFieldOnFail, SuccessRate.numFieldOffFail,
                   SuccessRate.numL4OK, SuccessRate.Totality);

    return err;
}
#endif
