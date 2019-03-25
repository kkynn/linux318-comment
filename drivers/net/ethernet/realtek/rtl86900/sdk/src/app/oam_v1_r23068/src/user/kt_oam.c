/*
 * Copyright (C) 2015 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 46475 $
 * $Date: 2014-02-14 11:03:12 +0800 (?±ä?, 14 äºŒæ? 2014) $
 *
 * Purpose : Define the KT related extended OAM
 *
 * Feature : Provide KT related extended OAM parsing and handling
 *
 */

/*
 * Include Files
 */
/* Standard include */
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/wait.h>
/* EPON OAM include */
#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_db.h"
#include "epon_oam_dbg.h"
#include "epon_oam_rx.h"

#include "kt_oam.h"

/*
 * Symbol Definition
 */

/*
 * Data Declaration
 */

/*
 * Macro Definition
 */

/*
 * Function Declaration
 */
static unsigned char kt_getDhcpOption254Status_req[] = {
    0x01, 0xa7, 0x00, 0x18, 0x00
};
static unsigned char kt_getDhcpOption254Status_resp[] = {
    0x02, 0xa7, 0x00, 0x18, 0x01, 0x01, 0x00
};

static unsigned char kt_disableDhcpOption254_req[] = {
    0x03, 0xa7, 0x00, 0x18, 0x01, 0x00
};
static unsigned char kt_disableDhcpOption254_resp[] = {
    0x04, 0xa7, 0x00, 0x18, 0x80, 0x00
};


static unsigned char *reqList[] = {
    kt_getDhcpOption254Status_req,  /* Get ONU DHCP option254 status */
    kt_disableDhcpOption254_req,    /* Disable ONU DHCP option 254 */
    NULL
};
static unsigned char reqLenList[] = {
    sizeof(kt_getDhcpOption254Status_req),  /* Get ONU DHCP option254 status */
    sizeof(kt_disableDhcpOption254_req),    /* Disable ONU DHCP option 254 */
};

static unsigned char *respList[] = {
    kt_getDhcpOption254Status_resp, /* Get ONU DHCP option254 status */
    kt_disableDhcpOption254_resp,   /* Disable ONU DHCP option 254 */
    NULL
};
static unsigned char respLenList[] = {
    sizeof(kt_getDhcpOption254Status_resp), /* Get ONU DHCP option254 status */
    sizeof(kt_disableDhcpOption254_resp),   /* Disable ONU DHCP option 254 */
};


static int kt_oam_extInfo_parser(
    unsigned char llidIdx,      /* LLID index from HW table */
    unsigned char *pFrame,      /* Frame payload current pointer */
    unsigned short length,      /* Frame payload length */
    unsigned short *pExtractLen,/* Parser extract length */
    void **ppOrgSpecData)       /* Orgnization specific data */
{

    return EPON_OAM_ERR_OK;
}

static int kt_oam_extInfo_handler(
    oam_oamPdu_t *pOamPdu,      /* OAMPDU data */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short bufLen,      /* Frame buffer size */
    unsigned short *pReplyLen,  /* Reply size used by this handler*/
    void **ppOrgSpecData)       /* Orgnization specific data
                                 * Direct comes from parser
                                 */
{

    return EPON_OAM_ERR_OK;
}

static int
kt_oam_orgSpec_processor(
    oam_oamPdu_t *pOamPdu,      /* OAMPDU data */
    unsigned char *pFrame,      /* Frame payload current pointer */
    unsigned short length,      /* Frame payload length */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short bufLen,      /* Frame buffer size */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    unsigned int i, j;
    unsigned char oui[3];
    unsigned char *pReq = NULL;
    unsigned char *pResp = NULL;
    unsigned char *pPtr;

    /* Parse the extended OAM header */
    oui[0] = pFrame[0];
    oui[1] = pFrame[1];
    oui[2] = pFrame[2];

    /* Search the request list and response it, otherwise ignore */
    *pReplyLen = 0;
    for(i = 0 ; reqList[i] != NULL ; i ++)
    {
        if((length - sizeof(oui)) < reqLenList[i])
        {
            /* Not enough length for compare */
            continue;
        }

        pReq = reqList[i];
        pPtr = pFrame + sizeof(oui);
        if(!memcmp(pPtr, pReq, reqLenList[i]))
        {
            /* Match request found, add reply */
            if(bufLen >= respLenList[i] + sizeof(oui))
            {
                pPtr = pReplyBuf;
                memcpy(pPtr, oui, sizeof(oui));
                pPtr += sizeof(oui);
                
                pResp = respList[i];
                memcpy(pPtr, pResp, respLenList[i]);
                pPtr += respLenList[i];

                *pReplyLen = pPtr - pReplyBuf;
                break;
            }
        }
    }

    return EPON_OAM_ERR_OK;
}

int kt_oam_init(void)
{
    int i, ret;
    unsigned char oui[] = KT_OAM_OUI;
    oam_infoOrgSpecCb_t infoCb;
    oam_orgSpecCb_t orgSpecCb;

    /* Register info organization specific callback */
    infoCb.parser = kt_oam_extInfo_parser;
    infoCb.handler = kt_oam_extInfo_handler;
    ret = epon_oam_orgSpecCb_reg(
        EPON_OAM_CBTYPE_INFO_ORGSPEC,
        oui,
        (void *) &infoCb);
    if(EPON_OAM_ERR_OK != ret)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "Failed to register KT info callback functions\n");
        return EPON_OAM_ERR_UNKNOWN;
    }

    /* Register organization specific callback */
    orgSpecCb.processor = kt_oam_orgSpec_processor;
    ret = epon_oam_orgSpecCb_reg(
        EPON_OAM_CBTYPE_ORGSPEC,
        oui,
        (void *) &orgSpecCb);
    if(EPON_OAM_ERR_OK != ret)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "Failed to register KT orgSpec callback functions\n");
        return EPON_OAM_ERR_UNKNOWN;
    }

    return EPON_OAM_ERR_OK;
}

int kt_oam_db_init(
    unsigned char llidIdx)
{

    return EPON_OAM_ERR_OK;
}

