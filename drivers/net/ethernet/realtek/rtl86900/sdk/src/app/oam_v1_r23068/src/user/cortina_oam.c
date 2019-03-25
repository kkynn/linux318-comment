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
 * $Date: 2016-01-19 16:20:12 +0800 $
 *
 * Purpose : Define the Cortina related extended OAM
 *
 * Feature : Provide Cortina related extended OAM parsing and handling
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
#include "epon_oam_igmp_util.h"
#include "ctc_oam.h"
#include "cortina_oam.h"

#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <hal/common/halctrl.h>
#include <rtk/l2.h>

/*
 * Data Declaration
 */
/* meter is not used in epon mode, so just use the first meter entry */
#define STORM_CONTROL_METER_INDEX               	0

static cortina_wrapper_mac_t macDb[MAX_MAC_ENTRY_SIZE];
static int next_mac_index = 0;

static cortina_wrapper_storm_control_t stormDb[MAX_PORT_NUM];
/*
  * Utility Function Declaration
  */

#ifdef CONFIG_SFU_APP
int getMacEntryByPort(int port, unsigned char *num, unsigned char *hasMore)
{
	int32 ret;
	uint32 index;
	rtk_l2_addr_table_t l2table;
	uint32 addr;
	rtk_l2_ucastAddr_t   l2UcEntry;
	int macIdx = 0;
	cortina_wrapper_mac_t * mac;	
	int32 chipid, rev, dmy_type;
	
    rtk_switch_version_get(&chipid, &rev, &dmy_type);
	/* only support RTL9601B_CHIP_ID now*/
	if(RTL9601B_CHIP_ID != chipid)
		return -1;
		
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[ %s ]:\nIdx  Port          Mac      Vid     Status\n", __FUNCTION__);

	addr = next_mac_index;
	do
	{
		memset(&l2table,0x0,sizeof(rtk_l2_addr_table_t));
		l2table.method = LUT_READ_METHOD_NEXT_ADDRESS;
		
		index =	addr;
		if(RT_ERR_OK ==	(rtk_l2_nextValidEntry_get(&addr,&l2table)))
		{
			/* Wrap	around */
			if(addr	< index)
			{
				break;
			}
			addr++;
			
			if(l2table.entryType == RTK_LUT_L2UC)
			{
				l2UcEntry = l2table.entry.l2UcEntry;
				if(port == l2UcEntry.port)
				{
					mac = &macDb[macIdx++];
					mac->port = l2UcEntry.port;
					memcpy(mac->mac, l2UcEntry.mac.octet, ETHER_ADDR_LEN);
					mac->vid = l2UcEntry.ctag_vid;
					mac->state = (l2UcEntry.flags & RTK_L2_UCAST_FLAG_STATIC) ? 1:0 ;

					EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,"%-3d  %-4d "MAC_PRINT"  %-4d   %s\n", 
			   					l2UcEntry.index, mac->port, MAC_PRINT_ARG(mac->mac), 
			   					mac->vid, (mac->state == 1)?"Static":"Dynamic");
					
					if(macIdx >= MAX_MAC_ENTRY_SIZE)
						break;
				}
			}
		}
		else
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[ %s ]: get lut entry[%d] fail\n", __FUNCTION__, index);
			break;
		}
	} while(1);	

	if(macIdx >= MAX_MAC_ENTRY_SIZE)
	{
		next_mac_index = addr;
		*num = MAX_MAC_ENTRY_SIZE;
		*hasMore = 1;
	}
	else
	{
		next_mac_index = 0;
		*num = macIdx;
		*hasMore = 0;
	}
	
	return 0;
}

int stormControlset(unsigned int port, rtk_enable_t state, int type, unsigned int speed)
{
	int32 ret;
	rtk_rate_storm_group_ctrl_t  stormCtrl;
	rtk_rate_storm_group_t  stormType;

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
		"stromControl_set port[%d], state[%s] type[%d] speed[0x%x]\n", port, state?"enable":"disable", type, speed);
	
	rtk_rate_stormControlEnable_get(&stormCtrl);
	
	if(type == CORTINA_OAM_STORM_CONTROL_BROADCAST)
	{
		stormCtrl.broadcast_enable = state;
		stormType = STORM_GROUP_BROADCAST;
	}
    else if(type == CORTINA_OAM_STORM_CONTROL_MULTICAST)
    {
		stormCtrl.unknown_multicast_enable = state;
		stormType = STORM_GROUP_UNKNOWN_MULTICAST;
    }
    else if(type == CORTINA_OAM_STORM_CONTROL_UNICAST)
    {
		stormCtrl.unknown_unicast_enable = state;
		stormType = STORM_GROUP_UNKNOWN_UNICAST;
    }
    else
        return -1;

	if(state == ENABLED)
	{
		rtk_enable_t ifgInclude;
		uint32 rate;

		ret = rtk_rate_stormControlEnable_set(&stormCtrl);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_stormControlEnable_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}
		
		ret = rtk_rate_stormControlPortEnable_set(port, stormType, state);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_stormControlPortEnable_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}
		
		rtk_rate_shareMeter_get(STORM_CONTROL_METER_INDEX, &rate,&ifgInclude);

		ret = rtk_rate_shareMeter_set(STORM_CONTROL_METER_INDEX, speed, ifgInclude);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_shareMeter_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}
		
		ret = rtk_rate_stormControlMeterIdx_set(port, stormType, STORM_CONTROL_METER_INDEX);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_stormControlPortEnable_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}
	}
	else
	{
		/* For limitation of switch api, must disable port state first, then disable storm control type */
		ret = rtk_rate_stormControlPortEnable_set(port, stormType, state);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_stormControlPortEnable_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}

		ret = rtk_rate_stormControlEnable_set(&stormCtrl);
		if(ret != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[%s]: rtk_rate_stormControlEnable_set fail ret[%d]\n", __FUNCTION__, ret);
			return -1;
		}
	}
	
	return 0;
}
#else
/* TODO: use rg interface */
int getMacEntryByPort(int port, unsigned char *num, unsigned char *hasMore)
{
	
}

int stormControlset(unsigned int port, rtk_enable_t state, int type, unsigned int speed)
{
}
#endif

/*
  * Organization Specific Function Declaration
  */

/* 0xC7/0x0103 - ONU Mac table*/
int cortina_oam_varCb_mactable_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	int ret;
	unsigned char *pPtr;
    unsigned char number;
	unsigned char hasMore;
	cortina_wrapper_mac_t * mac;
	unsigned int port;
	int i;
	
    if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }
	
	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = 2;/* 2 + 16 * numOfMac */
	pVarContainer->pVarData = (unsigned char *) malloc(1400);
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}
	
    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
    {
        return EPON_OAM_ERR_PARAM;
    }

	port = pVarInstant->parse.uniPort.portNo;

	ret = getMacEntryByPort(port-1, &number, &hasMore);
	if(0 != ret)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_WARN, "[ %s ]: getMacEntryByPort return error!\n", __FUNCTION__);
		number = 0;
	}

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[ %s ]: port[%d] number[%d] hasMore[%d]\n", __FUNCTION__, port, number, hasMore);
	
	pPtr = pVarContainer->pVarData;
	pVarContainer->pVarData[0] = hasMore;
	pVarContainer->pVarData[1] = number;
	pPtr += 2;
	
	for(i = 0; i < number; i++)
	{
		mac = &macDb[i];
		/* ungly implement: because custom olt parse data with little endian format */
		/* Port id */
		LITTLE_ENDIAN_ENCODE32(pPtr, &port);
		pPtr += 4;
		/* Mac address */
		memcpy(pPtr, mac->mac, ETHER_ADDR_LEN);		
		pPtr += 6;
		/* Vlan id */
		LITTLE_ENDIAN_ENCODE16(pPtr, &mac->vid);
		pPtr += 2;
		/* Mac State */
		LITTLE_ENDIAN_ENCODE32(pPtr, &mac->state);
		pPtr += 4;
	}

	pVarContainer->varWidth = 2 + 16*number;
	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0013 - stromControl */
int cortina_oam_varCb_stromControl_get(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* handler should allocate resource, 
                                          * caller will release it
                                          */
{
	unsigned int speed;
	unsigned int port;
	
	if(NULL == pVarInstant)
    {
        return EPON_OAM_ERR_NOT_FOUND;
    }
	
	pVarContainer->varDesc = varDesc;
	pVarContainer->varWidth = CORTINA_OAM_STORM_CONTROL_LEN;
	pVarContainer->pVarData = (unsigned char *) malloc(pVarContainer->varWidth * sizeof(unsigned char));
	if(NULL == pVarContainer->pVarData)
	{
		return EPON_OAM_ERR_MEM;
	}
	
    if(CTC_INSTANT_IS_ALLPORTS(pVarInstant))
    {
        return EPON_OAM_ERR_PARAM;
    }
	
	port = pVarInstant->parse.uniPort.portNo - 1;

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
		"[ %s ]: port[%d] type[%d] speed[%d]\n", __FUNCTION__, port, stormDb[port].type, stormDb[port].speed);

	/* type = 0 means storm control is disabled */
	if(stormDb[port].type == 0)
	{
		pVarContainer->pVarData[0] = 0x0;
		pVarContainer->pVarData[1] = 0x0;
		pVarContainer->pVarData[2] = 0x0;
		pVarContainer->pVarData[3] = 0x0;
		pVarContainer->pVarData[4] = 0x0;
	}
	else
	{
		speed = stormDb[port].speed;
		
		pVarContainer->pVarData[0] = 0x40;
		pVarContainer->pVarData[1] = stormDb[port].type;
		pVarContainer->pVarData[2] = (speed >> 16) & 0xFF;
		pVarContainer->pVarData[3] = (speed >> 8) & 0xFF;
		pVarContainer->pVarData[4] = speed & 0xFF;
	}

	return EPON_OAM_ERR_OK;
}

/* 0xC7/0x0013 - stromControl */
int cortina_oam_varCb_stromControl_set(
    unsigned char llidIdx, /* LLID index of the incoming operation */
    unsigned char op, /* Operation to be taken */
    ctc_varInstant_t *pVarInstant, /* It might be NULL if no instant specified */
    ctc_varDescriptor_t varDesc,
    ctc_varContainer_t *pVarContainer)   /* caller prepare all the resource needed, 
                                          * handler won't need to allocate anything
                                          */
{
	unsigned char control;
	unsigned char type;
	unsigned int speed;
	unsigned int port;
	rtk_enable_t state;
	
	if(CORTINA_OAM_STORM_CONTROL_LEN != pVarContainer->varWidth)
    {
        return EPON_OAM_ERR_UNKNOWN;
    }

	control = pVarContainer->pVarData[0];
	type = pVarContainer->pVarData[1];
	speed = pVarContainer->pVarData[4] + (pVarContainer->pVarData[3] << 8) + (pVarContainer->pVarData[2] << 16);

	port = pVarInstant->parse.uniPort.portNo - 1;
	if(control & 0x40)
		state = ENABLED;
	else
		state = DISABLED;
	
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, 
		"stromControl_set port[%d], admin[%s] type[%d] speed[0x%x]\n", port, state?"enable":"disable", type, speed);

	stormDb[port].speed = speed;

	if(state)
	{
		stormDb[port].type = type;
		
		if(type & CORTINA_OAM_STORM_CONTROL_BROADCAST)	
			stormControlset(port, ENABLED, CORTINA_OAM_STORM_CONTROL_BROADCAST, speed);
		else
			stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_BROADCAST, speed);
		
		if(type & CORTINA_OAM_STORM_CONTROL_MULTICAST)
			stormControlset(port, ENABLED, CORTINA_OAM_STORM_CONTROL_MULTICAST, speed);
		else
			stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_MULTICAST, speed);
		
		if(type & CORTINA_OAM_STORM_CONTROL_UNICAST)
			stormControlset(port, ENABLED, CORTINA_OAM_STORM_CONTROL_UNICAST, speed);
		else
			stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_UNICAST, speed);
	}
	else
	{
		stormDb[port].type = 0x0;
		stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_BROADCAST, speed);
		stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_MULTICAST, speed);
		stormControlset(port, DISABLED, CORTINA_OAM_STORM_CONTROL_UNICAST, speed);
	}
	
	pVarContainer->varWidth = CORTINA_OAM_VAR_RESP_SETOK;
	return EPON_OAM_ERR_OK;
}

/*
  * Process Cortina Organization Specific oam packet 
  * packet format is same with ctc oam, reuse ctc process function
  */
static void cortina_oam_orgSpecHdr_gen(
    unsigned char oui[],        /* OUI to be generate */
    unsigned char extOpcode,    /* Extended Opcode to be generate */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    /* Buffer size check should be done before calling gen function */

    pReplyBuf[0] = oui[0];
    pReplyBuf[1] = oui[1];
    pReplyBuf[2] = oui[2];
    pReplyBuf[3] = extOpcode;

    *pReplyLen = CTC_ORGSPEC_HDR_LEN;
}

static void cortina_oam_orgSpecVarContainer_gen(
    ctc_varContainer_t *pContainer, /* Instant data to be generate */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    /* Buffer size check should be done before calling gen function */
    unsigned char *pPtr;
    unsigned short remainLen;

    pPtr = pReplyBuf;
    *pPtr = pContainer->varDesc.varBranch;
    pPtr += 1; /* Branch */
    pPtr[0] = ((unsigned char *)(&pContainer->varDesc.varLeaf))[0];
    pPtr[1] = ((unsigned char *)(&pContainer->varDesc.varLeaf))[1];
    pPtr += 2; /* Leaf */
    if(CTC_VAR_CONTAINER_INDICATOR & pContainer->varWidth)
    {
        *pPtr = pContainer->varWidth & 0x00ff;
        pPtr += 1; /* Width */
        /* No data field */
    }
    else
    {
        remainLen = pContainer->varWidth;
        while(0 != remainLen)
        {
            /* ungly implement: custom olt doesn't have 0x80 data size limit */
            {
				if(remainLen >= 0x80)
					*pPtr = 0x80;
				else
					*pPtr = remainLen;
                pPtr += 1; /* Width */
                memcpy(pPtr, pContainer->pVarData + pContainer->varWidth - remainLen, sizeof(unsigned char) * remainLen);
                pPtr += sizeof(unsigned char) * remainLen; /* Data */
                remainLen = 0;
            }
        }
    }

    *pReplyLen = pPtr - pReplyBuf;
}

static void cortina_oam_orgSpecVarInstant_gen(
    ctc_varInstant_t *pInstant, /* Instant data to be generate */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    /* Buffer size check should be done before calling gen function */
    unsigned char *pPtr;

    pPtr = pReplyBuf;
    *pPtr = pInstant->varDesc.varBranch;
    pPtr += 1; /* Branch */
    pPtr[0] = ((unsigned char *)(&pInstant->varDesc.varLeaf))[0];
    pPtr[1] = ((unsigned char *)(&pInstant->varDesc.varLeaf))[1];
    pPtr += 2; /* Leaf */
    *pPtr = pInstant->varWidth;
    pPtr += 1; /* Width */

    if(pInstant->varWidth == CTC_ORGSPEC_VARINSTANT_WIDTH20)
    {
        /*
         * CTC2.0 Index TLV:
         * ----------------
         * 1 	Branch
         * ----------------
         * 2 	Leaf
         * ----------------
         * 1 	Width=0x01
         * ----------------
         * 1	Value
         * ----------------			 
         */
        memcpy(pPtr, pInstant->varData, sizeof(unsigned char) * pInstant->varWidth);
    }
    else if(pInstant->varWidth == CTC_ORGSPEC_VARINSTANT_WIDTH21)
    {
        /*
		 * CTC2.1 Index TLV:
		 * ----------------
		 * 1 	Branch
		 * ----------------
		 * 2 	Leaf
		 * ----------------
		 * 1 	Width=0x04
		 * ----------------
		 * 4	Value	-------\
		 * ----------------     \
         *                      ----------
         *                      1 portType
         *                      ----------
         *                      1 chassisNo
         *                      ----------
         *                      1 slotNo
         *                      ----------
         *                      1 portNo
         *                      ----------
         */
        memcpy(pPtr, pInstant->varData, sizeof(unsigned char) * pInstant->varWidth);
    }

    pPtr += sizeof(unsigned char) * pInstant->varWidth; /* Data */

    *pReplyLen = pPtr - pReplyBuf;
}

static int cortina_oam_orgSpecValidInstantLeaf_decode20(
    ctc_varInstant_t *pVarInstant)
{
    unsigned int value;

    switch(pVarInstant->varDesc.varLeaf)
    {
        case CTC_VAR_REQBRANCH_INSTANT_PORT:
            value = *pVarInstant->varData;
            /* Chassis & slot number are not defined in CTC 2.0- instant, always zero */
            pVarInstant->parse.uniPort.chassisNo = 0;
            pVarInstant->parse.uniPort.slotNo = 0;
            if(0 == value)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_PON;
                pVarInstant->parse.uniPort.portNo = 0;
            }
            else if(value <= 0x4F)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_ETHERNET;
                pVarInstant->parse.uniPort.portNo = value;
            }
            else if(value <= 0x8F)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_VOIP;
                pVarInstant->parse.uniPort.portNo = value - 0x4F;
            }
            else if(value <= 0x9F)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_E1;
                pVarInstant->parse.uniPort.portNo = value - 0x90;
            }
            else if(value < 0xFF)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_OTHERS;
                pVarInstant->parse.uniPort.portNo = value - 0x9F;
            }
            else if(value == 0xFF)
            {
                pVarInstant->parse.uniPort.portType = CTC_VAR_INSTANT_PORTTYPE_ETHERNET;
                pVarInstant->parse.uniPort.portNo = CTC_VAR_INSTANT_PORTALL;
            }
            else
            {
                return 0;
            }
            /* Valid instant leaf value for CTC 2.0 */
            return 1;
        default:
            return 0;
    }
}

static int cortina_oam_orgSpecValidInstantLeaf_decode21(
    ctc_varInstant_t *pVarInstant)
{
    unsigned int value;

    switch(pVarInstant->varDesc.varLeaf)
    {
        case CTC_VAR_REQBRANCH_INSTANT_PORT:
            CTC_BUF_PARSE32(pVarInstant->varData, &value);
            pVarInstant->parse.uniPort.portType = (value >> 24) & 0xffUL;
            pVarInstant->parse.uniPort.chassisNo = (value >> 22) & 0x03UL;
            pVarInstant->parse.uniPort.slotNo = (value >> 16) & 0x3fUL;
            pVarInstant->parse.uniPort.portNo = value & 0xffffUL;
            /* Valid instant leaf value for CTC 2.1 */
            return 1;
        case CTC_VAR_REQBRANCH_INSTANT_LLID:
            CTC_BUF_PARSE32(pVarInstant->varData, &value);
			pVarInstant->parse.llid = value & 0xffffUL;
            /* Valid instant leaf value for CTC 2.1 */
            return 1;
        case CTC_VAR_REQBRANCH_INSTANT_PONIF:
            CTC_BUF_PARSE32(pVarInstant->varData, &value);
			pVarInstant->parse.ponIf = value & 0x1UL;
            /* Valid instant leaf value for CTC 2.1 */
            return 1;
        case CTC_VAR_REQBRANCH_INSTANT_ONU:
        default:
            return 0;
    }
}

static int cortina_oam_orgSpecVar_req(
    unsigned char llidIdx,      /* LLID index of the incoming operation */
    unsigned char *pFrame,      /* Frame payload current pointer */
    unsigned short length,      /* Frame payload length */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short bufLen,      /* Frame buffer size */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    int ret;
    unsigned char *pProcPtr, *pReplyPtr;
    unsigned short genLen;
    ctc_varCb_t varCb;
    ctc_varDescriptor_t varDesc;
    ctc_varInstant_t *pVarInstant = NULL, varInstant, varInstCnt;
    ctc_varContainer_t varContainer;
    unsigned short remainLen;

    /* 1. Parse branch/leaf and search database for process function
     * 2. Call process function to preocess the branch/leaf
     */
    pProcPtr = pFrame;
    pReplyPtr = pReplyBuf;
    remainLen = length;
    *pReplyLen = 0;

    while(remainLen >= CTC_ORGSPEC_VARDESC_LEN)
    {
        varDesc.varBranch = *pProcPtr;
        memcpy(&varDesc.varLeaf, &pProcPtr[1], sizeof(unsigned short));
        CTC_BUF_ADD(pProcPtr, remainLen, CTC_ORGSPEC_VARDESC_LEN); /* Variable Descriptor */

        switch(varDesc.varBranch)
        {
        case CTC_VAR_REQBRANCH_INSTANT20:
            /* Parse for variable instant */
            if((remainLen < CTC_ORGSPEC_VARINSTANT_LEN20) &&
               (bufLen > (CTC_ORGSPEC_VARDESC_LEN + CTC_ORGSPEC_VARINSTANT_LEN20)))
            {
                /* Ignore the following variables */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }
            varInstant.varDesc = varDesc;
            varInstant.varWidth = CTC_ORGSPEC_VARINSTANT_WIDTH20;
            CTC_BUF_ADD(pProcPtr, remainLen, 1); /* Width */
            memcpy(varInstant.varData, pProcPtr, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH20);
            CTC_BUF_ADD(pProcPtr, remainLen, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH20); /* Data */
            if(cortina_oam_orgSpecValidInstantLeaf_decode20(&varInstant))
            {
                /* Set instant pointer for all remain variable request */
                pVarInstant = &varInstant;
                if(!CTC_INSTANT_IS_ALLPORTS(pVarInstant))
                {
                    /* Encode the variable instant into reply packet */
                    cortina_oam_orgSpecVarInstant_gen(pVarInstant, pReplyPtr, &genLen);
                    CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
                }
                else
                {
                    /* For get request with ALL port instatnt, all port should response with its own data and instant 
		                     * The response callback should encode the instant for each port
		                     */
                }
            }
            else
            {
                /* Ignore the invalid variable instant */
                pVarInstant = NULL;
            }
            break;
        case CTC_VAR_REQBRANCH_INSTANT21:
            /* Parse for variable instant */
            if((remainLen < CTC_ORGSPEC_VARINSTANT_LEN21) &&
               (bufLen > (CTC_ORGSPEC_VARDESC_LEN + CTC_ORGSPEC_VARINSTANT_LEN21)))
            {
                /* Ignore the following variables */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }
            varInstant.varDesc = varDesc;
            varInstant.varWidth = CTC_ORGSPEC_VARINSTANT_WIDTH21;
            CTC_BUF_ADD(pProcPtr, remainLen, 1); /* Width */
            memcpy(varInstant.varData, pProcPtr, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH21);
            CTC_BUF_ADD(pProcPtr, remainLen, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH21); /* Data */
            if(cortina_oam_orgSpecValidInstantLeaf_decode21(&varInstant))
            {
                /* Set instant pointer for all remain variable request */
                pVarInstant = &varInstant;
                if(!CTC_INSTANT_IS_ALLPORTS(pVarInstant))
                {
                    /* Encode the variable instant into reply packet */
                    cortina_oam_orgSpecVarInstant_gen(pVarInstant, pReplyPtr, &genLen);
                    CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
                }
                else
                {
                    /* For get request with ALL port instatnt, all port should response with its own data and instant 
		                     * The response should encode the instant for each port
		                     */
                }
            }
            else
            {
                /* Ignore the invalid variable instant */
                pVarInstant = NULL;
            }
            break;
        case CTC_VAR_REQBRANCH_STDATTR:
        case CTC_VAR_REQBRANCH_EXTATTR:
        case CTC_VAR_REQBRANCH_STDACT:
        case CTC_VAR_REQBRANCH_EXTACT:               
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARGET,
                    	"[Cortina OAM:%s:%d] var get 0x%02x/0x%04x\n", __FILE__, __LINE__, varDesc.varBranch, varDesc.varLeaf);

            if((NULL != pVarInstant) && (CTC_INSTANT_IS_ALLPORTS(pVarInstant)))
            {
                /* If the target instant is all ports, loop each port one by one */
            }
            else
            {
                /* Use branch/leaf specific function to process */
                varContainer.pVarData = NULL;
				if((varDesc.varBranch ==  CTC_VAR_REQBRANCH_EXTATTR) && 
					(varDesc.varLeaf == CORTINA_VAR_LEAF_MACTABLE))
				{
                	ret = cortina_oam_varCb_mactable_get(llidIdx, CTC_VAR_OP_GET, pVarInstant, varDesc, &varContainer);
				}
				else if((varDesc.varBranch ==  CTC_VAR_REQBRANCH_EXTATTR) && 
					(varDesc.varLeaf == CORTINA_VAR_LEAF_STORMCONTROL))
				{
					ret = cortina_oam_varCb_stromControl_get(llidIdx, CTC_VAR_OP_GET, pVarInstant, varDesc, &varContainer);
				}
				else
				{
					/* Ignore unsupported branch/leaf  */
					break;
				}
				
                if(EPON_OAM_ERR_OK == ret)
                {
                    if(bufLen >= (CTC_ORGSPEC_VARDESC_LEN + varContainer.varWidth + 1 /* Width */))
                    {
                        genLen = 0;
                        cortina_oam_orgSpecVarContainer_gen(
                            &varContainer,
                            pReplyPtr,
                            &genLen);
                        CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
                    }
                }
                if(NULL != varContainer.pVarData)
                {
                    free(varContainer.pVarData);
                }
            }

            break;
        case CTC_VAR_REQBRANCH_END:
            *pReplyLen = (pReplyPtr - pReplyBuf);
            return EPON_OAM_ERR_OK;
        default:
            break;
        }
    }
    *pReplyLen = (pReplyPtr - pReplyBuf);

    return EPON_OAM_ERR_OK;
}

static int cortina_oam_orgSpecSet_req(
    unsigned char llidIdx,      /* LLID index of the incoming operation */
    unsigned char *pFrame,      /* Frame payload current pointer */
    unsigned short length,      /* Frame payload length */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short bufLen,      /* Frame buffer size */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    int ret;
    unsigned int extFlag;
    unsigned char *pProcPtr, *pReplyPtr;
    unsigned short genLen;
    ctc_varCb_t varCb;
    ctc_varInstant_t *pVarInstant = NULL, varInstant;
    ctc_varContainer_t varContainer;
    unsigned short remainLen, cascadeLen, concateLen;

    /* 1. Parse branch/leaf and search database for process function
     * 2. Call process function to preocess the branch/leaf
     */
    pProcPtr = pFrame;
    pReplyPtr = pReplyBuf;
    remainLen = length;
    *pReplyLen = 0;

    while(remainLen >= CTC_ORGSPEC_VARDESC_LEN)
    {
        varContainer.varDesc.varBranch = *pProcPtr;
        memcpy(&varContainer.varDesc.varLeaf, &pProcPtr[1], sizeof(unsigned short));

		/* To be simple: not consider set request whose width is 0 */
        {
            if(remainLen < (CTC_ORGSPEC_VARCONTAINER_MIN - CTC_ORGSPEC_VARDESC_LEN))
            {
                /* Insufficient length for parsing
                 		  * Skip all remain frame
                 		  */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }
            varContainer.varWidth = pProcPtr[3];
            /* Mark the next line, when olt reboot onu, the varContainer have no the member or the varWidth = 0 */
            varContainer.varWidth = (0 == varContainer.varWidth) ? 0x80 : varContainer.varWidth;
            CTC_BUF_ADD(pProcPtr, remainLen, CTC_ORGSPEC_VARCONTAINER_MIN); /* Variable Container */
        }

        switch(varContainer.varDesc.varBranch)
        {
        case CTC_VAR_REQBRANCH_INSTANT20:
            /* Parse for variable instant */
            if((remainLen < varContainer.varWidth) &&
               (bufLen > (CTC_ORGSPEC_VARDESC_LEN + CTC_ORGSPEC_VARINSTANT_LEN20)))
            {
                /* Ignore the following variables */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }
            varInstant.varDesc = varContainer.varDesc;
            varInstant.varWidth = CTC_ORGSPEC_VARINSTANT_WIDTH20;
            memcpy(varInstant.varData, pProcPtr, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH20);
            CTC_BUF_ADD(pProcPtr, remainLen, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH20); /* Data */
            if(cortina_oam_orgSpecValidInstantLeaf_decode20(&varInstant))
            {
                /* Set instant pointer for all remain variable request */
                pVarInstant = &varInstant;
                /* Encode the variable instant into reply packet */
                cortina_oam_orgSpecVarInstant_gen(pVarInstant, pReplyPtr, &genLen);
                CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
            }
            else
            {
                /* Ignore the invalid variable instant */
                pVarInstant = NULL;
            }
            break;
        case CTC_VAR_REQBRANCH_INSTANT21:
            /* Parse for variable instant */
            if((remainLen < varContainer.varWidth) &&
               (bufLen > (CTC_ORGSPEC_VARDESC_LEN + CTC_ORGSPEC_VARINSTANT_LEN21)))
            {
                /* Ignore the following variables */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }
            varInstant.varDesc = varContainer.varDesc;
            varInstant.varWidth = CTC_ORGSPEC_VARINSTANT_WIDTH21;
            memcpy(varInstant.varData, pProcPtr, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH21);
            CTC_BUF_ADD(pProcPtr, remainLen, sizeof(unsigned char) * CTC_ORGSPEC_VARINSTANT_WIDTH21); /* Data */
            if(cortina_oam_orgSpecValidInstantLeaf_decode21(&varInstant))
            {
                /* Set instant pointer for all remain variable request */
                pVarInstant = &varInstant;
                /* Encode the variable instant into reply packet */
                cortina_oam_orgSpecVarInstant_gen(pVarInstant, pReplyPtr, &genLen);
                CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
            }
            else
            {
                /* Ignore the invalid variable instant */
                pVarInstant = NULL;
            }
            break;
        case CTC_VAR_REQBRANCH_STDATTR:
        case CTC_VAR_REQBRANCH_EXTATTR:
        case CTC_VAR_REQBRANCH_STDACT:
        case CTC_VAR_REQBRANCH_EXTACT:
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_VARSET, "[Cortina OAM:%s:%d] var set 0x%02x/0x%04x\n", 
					__FILE__, __LINE__, varContainer.varDesc.varBranch, varContainer.varDesc.varLeaf);
			
            if(remainLen < varContainer.varWidth)
            {
                /* Insufficient length for parsing
                 		  * Skip all remain frame
                 		  */
                *pReplyLen = (pReplyPtr - pReplyBuf);
                return EPON_OAM_ERR_OK;
            }

			if((varContainer.varDesc.varBranch ==  CTC_VAR_REQBRANCH_EXTATTR) && 
				(varContainer.varDesc.varLeaf == CORTINA_VAR_LEAF_STORMCONTROL))
			{
				varCb.handler_set = cortina_oam_varCb_stromControl_set;
			}
            else
            {
                /* Ignore unsupport branch/leaf 
                 		  * Skip the data field of the frame
                 		  */
                CTC_BUF_ADD(pProcPtr, remainLen, varContainer.varWidth); /* Data */
                break;
            }

            /* Extract data field from frame */
            if(0 != varContainer.varWidth)
            {
            	/* To be simple: not consider width >= 0x80 */
                {
                    /* No cascade variable container is possible */
                    varContainer.pVarData = (unsigned char *) malloc(varContainer.varWidth);
                    if(NULL == varContainer.pVarData)
                    {
                        return EPON_OAM_ERR_MEM;
                    }
                    memcpy(varContainer.pVarData, pProcPtr, varContainer.varWidth);
                    CTC_BUF_ADD(pProcPtr, remainLen, varContainer.varWidth); /* Data */
                }
            }
            else
            {
                varContainer.pVarData = NULL;
            }        
            
            /* Use branch/leaf specific callback function to process */
            ret = varCb.handler_set(llidIdx, CTC_VAR_OP_SET, pVarInstant, varContainer.varDesc, &varContainer);
            
            if(EPON_OAM_ERR_OK == ret)
            {
                if(bufLen >= CTC_ORGSPEC_VARCONTAINER_MIN)
                {
                    /* For set operation, only set result should be encoded */
                    varContainer.varWidth |= CTC_VAR_CONTAINER_INDICATOR;

                    genLen = 0;
                    cortina_oam_orgSpecVarContainer_gen(
                        &varContainer,
                        pReplyPtr,
                        &genLen);
                    CTC_BUF_ADD(pReplyPtr, bufLen, genLen);
                }
            }

            if(NULL != varContainer.pVarData)
            {
                free(varContainer.pVarData);
            }
            break;
        case CTC_VAR_REQBRANCH_END:
            *pReplyLen = (pReplyPtr - pReplyBuf);
            return EPON_OAM_ERR_OK;
        default:
            break;
        }
    }
    *pReplyLen = (pReplyPtr - pReplyBuf);

    return EPON_OAM_ERR_OK;
}

static int cortina_oam_orgSpec_processor(
    oam_oamPdu_t *pOamPdu,      /* OAMPDU data */
    unsigned char *pFrame,      /* Frame payload current pointer */
    unsigned short length,      /* Frame payload length */
    unsigned char *pReplyBuf,   /* Frame buffer for reply OAM */
    unsigned short bufLen,      /* Frame buffer size */
    unsigned short *pReplyLen)  /* Reply size used by this handler*/
{
    int ret = EPON_OAM_ERR_OK;
    unsigned char oui[3];
    unsigned char extOpcode;
	unsigned short genLen;

    /* Parse the extended OAM header */
    oui[0] = pFrame[0];
    oui[1] = pFrame[1];
    oui[2] = pFrame[2];
	extOpcode = pFrame[3];
	CTC_BUF_ADD(pFrame, length, CTC_ORGSPEC_HDR_LEN);
	
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[Cortina OAM: %s]: extOpcode[%d]\n", __FUNCTION__, extOpcode);
	
	switch(extOpcode)
    {
    case CTC_EXTOAM_OPCODE_VARREQ:
        cortina_oam_orgSpecHdr_gen(oui, CTC_EXTOAM_OPCODE_VARRESP, pReplyBuf, &genLen);
        CTC_BUF_ADD(pReplyBuf, bufLen, genLen);
        *pReplyLen += genLen;

		ret = cortina_oam_orgSpecVar_req(
               pOamPdu->llidIdx, pFrame, length, pReplyBuf, bufLen, &genLen);

        if(0 != genLen)
        {
            *pReplyLen += genLen;
        }
        else
        {
            /* Nothing to be replied, remove encoded header */
            *pReplyLen = 0;
        }
        break;
	case CTC_EXTOAM_OPCODE_SETREQ:
        cortina_oam_orgSpecHdr_gen(oui, CTC_EXTOAM_OPCODE_SETRESP, pReplyBuf, &genLen);
        CTC_BUF_ADD(pReplyBuf, bufLen, genLen);
        *pReplyLen += genLen;
        genLen= 0;

        ret = cortina_oam_orgSpecSet_req(
            	pOamPdu->llidIdx, pFrame, length, pReplyBuf, bufLen, &genLen);

        if(0 != genLen)
        {
            *pReplyLen += genLen;
        }
        else
        {
            /* Nothing to be replied, remove encoded header */
            *pReplyLen = 0;
        }
        break;
	default:
	 	break;
    }

    if(EPON_OAM_ERR_OK != ret)
    {
        /* Something wrong, remove all reply */
        *pReplyLen = 0;
    }

    return EPON_OAM_ERR_OK;
}

/*
  * Common Function Declaration
  */
static int cortina_oam_extInfo_parser(
	unsigned char llidIdx, 	 /* LLID index from HW table */
	unsigned char *pFrame, 	 /* Frame payload current pointer */
	unsigned short length, 	 /* Frame payload length */
	unsigned short *pExtractLen,/* Parser extract length */
	void **ppOrgSpecData)		 /* Orgnization specific data */
{
 
	return EPON_OAM_ERR_OK;
}
 
static int cortina_oam_extInfo_handler(
	oam_oamPdu_t *pOamPdu, 	 /* OAMPDU data */
	unsigned char *pReplyBuf,	 /* Frame buffer for reply OAM */
	unsigned short bufLen, 	 /* Frame buffer size */
	unsigned short *pReplyLen,  /* Reply size used by this handler*/
	void **ppOrgSpecData)		 /* Orgnization specific data
								  * Direct comes from parser
								  */
{
 
	return EPON_OAM_ERR_OK;
}

int cortina_oam_init(void)
{
	int i, ret;
	unsigned char oui[] = CORTINA_OAM_OUI;
	oam_infoOrgSpecCb_t infoCb;
	oam_orgSpecCb_t orgSpecCb;
 
	/* Register info organization specific callback */
	infoCb.parser = cortina_oam_extInfo_parser;
	infoCb.handler = cortina_oam_extInfo_handler;
	ret = epon_oam_orgSpecCb_reg(
		  EPON_OAM_CBTYPE_INFO_ORGSPEC,
		  oui,
		  (void *) &infoCb);
	if(EPON_OAM_ERR_OK != ret)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
			 "Failed to register TK info callback functions\n");
		return EPON_OAM_ERR_UNKNOWN;
	}
 
	/* Register organization specific callback */
	orgSpecCb.processor = cortina_oam_orgSpec_processor;
	ret = epon_oam_orgSpecCb_reg(
		  EPON_OAM_CBTYPE_ORGSPEC,
		  oui,
		  (void *) &orgSpecCb);
	if(EPON_OAM_ERR_OK != ret)
	{
		 EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
			 "Failed to register TK orgSpec callback functions\n");
		 return EPON_OAM_ERR_UNKNOWN;
	}
 
	return EPON_OAM_ERR_OK;
}
 
int cortina_oam_db_init(unsigned char llidIdx)
{
	memset(stormDb, 0, sizeof(stormDb));
	return EPON_OAM_ERR_OK;
}

