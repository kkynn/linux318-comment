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
* $Revision: 1 $
* $Date: 2015-09-18 11:22:15 +0800 (Fri, 18 Sep 2015) $
*
* Purpose : 
*
* Feature : 
*
*/
#include <stdio.h>
#include <string.h>
#include <malloc.h> 
#include <pthread.h>
#include <sys/socket.h> 
#include <linux/netlink.h> 
#include <time.h>

#include <sys_portmask.h>

#include "epon_oam_igmp.h"
#include "epon_oam_igmp_db.h"
#include "epon_oam_igmp_util.h"
#include "epon_oam_igmp_hw.h"
#include "epon_oam_igmp_querier.h"
#include "epon_oam_err.h"
#include "pkt_redirect_user.h"
#include "ctc_oam_alarmtbl.h"

igmp_stats_t  igmp_stats;
int igmpRedirect_sock;

extern uint32            sys_ip;
rtk_enable_t  igmp_packet_stop = DISABLED;
rtk_enable_t  mcast_timer_stop = DISABLED;

#define MAX_PAYLOAD (1600)
#define MAC_FRAME_OFFSET 4 /* this 4 bytes is reserved to save ctag_vid and spa info */

#define PON_PORT_MASK (1 << PHYSICAL_PON_PORT)

#ifndef PR_USER_UID_IGMPMLD
#define PR_USER_UID_IGMPMLD 6
#endif

#ifdef SUPPORT_MLD_SNOOPING
const uint8 ipv6_dip_query1[] = {0xff, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01};
const uint8 ipv6_dip_query2[] = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x01};
const uint8 ipv6_dip_leave1[] = {0xff, 0x01, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02};
const uint8 ipv6_dip_leave2[] = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02};
const uint8 ipv6_dip_leave3[] = {0xff, 0x05, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x02};

const uint8 ipv6_dvmrp_routing[] =      {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x04};
const uint8 ipv6_ospf_all_routing[] =   {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x05};
const uint8 ipv6_ospf_designated_routing[] = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x06};
const uint8 ipv6_pimv2_routing[] =      {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0d};

const uint8 ipv6_mldv2_report[] = {0xff, 0x02, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x16};
const uint8 ipv6_proxy_query[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
#endif

uint16 mcast_checksum_get(uint16 *addr, int32 len, uint16* pChksum)
{
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register int32 sum = 0;
    uint16 *source = (uint16 *) addr;

    while (len > 1)  {
        /*  This is the inner loop */
        sum += *source++;
        len -= 2;
    }

    /*  Add left-over byte, if any */
    if (len > 0) {
        /* Make sure that the left-over byte is added correctly both
         * with little and big endian hosts */
        uint16 tmp = 0;
        *(uint8 *) (&tmp) = * (uint8 *) source;
        sum += tmp;
    }
    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    *pChksum = ~sum;
    
    return SYS_ERR_OK;
}

static uint32 mcast_process_igmp_query(packet_info_t * pktInfo, uint8* pktBuf, uint32 pktLen)
{
	uint32 	t, qqic;
	uint8  	robussVar = 0;
	uint32 	group;
	uint16 	srcNum = 0;
    uint8 	srcList[512];
    uint8 	*pSrcList = srcList;
    uint8 	isSpecQuery = TRUE;
    uint32	maxRespTime = 0; //get when it is group specific query
	uint8  	query_version;	
	int i;
	igmp_hdr_t *pIgmpHdr;
	uint16 vid;
	sys_logic_portmask_t txPmsk;
	
	vid = pktInfo->vid;
	pIgmpHdr = (igmp_hdr_t *)pktBuf;

#ifndef IGMP_TEST_MODE
	if(pktInfo->rx_tag.source_port != HAL_GET_PON_PORT())
	{
		return SYS_ERR_FAILED;
	}
#endif

	group = pktInfo->groupAddress[0];
	
	if(pktLen > 8) /*means igmpv3 query*/
	{
		query_version = IGMP_QUERY_V3;
	}
	else
	{
		query_version = IGMP_QUERY_V2;
	}

	if(query_version != IGMP_QUERY_V3)
    {
		t = pIgmpHdr->maxRespTime/10;
		if (group == 0)     /* general query */
        {
            igmp_stats.g_query_rcvd++;
            isSpecQuery = FALSE;
			
            SYS_DBG("A IGMP General QUERY received on Vid = %d, Port-%d from "IPADDR_PRINT". (Interval:%d s)\n", 
                vid, pktInfo->rx_tag.source_port, IPADDR_PRINT_ARG(pktInfo->sip[0]), t);
        }
        else                /* v2 group specific query */
        {
            igmp_stats.gs_query_rcvd++;
			if (t)
                maxRespTime = t; 
			
            SYS_DBG("A IGMP Group-Specific QUERY received on Vid = %d, Port-%d from "IPADDR_PRINT". (Group:"IPADDR_PRINT" Interval:%d s)\n",
                vid, pktInfo->rx_tag.source_port, IPADDR_PRINT_ARG(pktInfo->sip[0]), IPADDR_PRINT_ARG(group), t);
        }
    }
    else
    {
        srcNum = ntohl(pIgmpHdr->v3.numOfSrc);

        if (pIgmpHdr->maxRespTime & 0x80)
        {
            t = ((pIgmpHdr->maxRespTime & 0x0F) | 0x10) << (((pIgmpHdr->maxRespTime & 0x70) >> 4) +3);
            t = t/10;
        }
        else
        {
            t = pIgmpHdr->maxRespTime/10;
        }
        if (t)
          	maxRespTime = t;
        
        if (pIgmpHdr->v3.qqic & 0x80)
        {
            qqic = ((pIgmpHdr->v3.qqic & 0x0F) | 0x10) << (((pIgmpHdr->v3.qqic & 0x70) >> 4)+3); 
        }
        else
        {
            qqic = pIgmpHdr->v3.qqic;
        }
        
        robussVar =  pIgmpHdr->v3.rsq & 0x07;
        
        if(group == 0)
        {
            igmp_stats.v3.g_queryV3_rcvd++;
            isSpecQuery = FALSE;
			
            SYS_DBG("A IGMP General QUERY v3 received on Vid = %d, Port-%d from "IPADDR_PRINT". (Interval:%d sec)\n", 
                vid, pktInfo->rx_tag.source_port, IPADDR_PRINT_ARG(pktInfo->sip[0]), t); 
        }
        else if(srcNum == 0) /*v3 group specific query*/
        {
            igmp_stats.v3.gs_queryV3_rcvd++;
            SYS_DBG("A IGMP v3 Group-Specific QUERY received on Vid = %d, Port-%d from "IPADDR_PRINT". (Group:"IPADDR_PRINT" Interval:%d.%d)\n",
                vid, pktInfo->rx_tag.source_port, IPADDR_PRINT_ARG(pktInfo->sip[0]), IPADDR_PRINT_ARG(group), t/10, t%10);
        }
        else  /*v3 group and src specific query*/ 
        {
        	igmp_stats.v3.gss_queryV3_rcvd++;

			memset(srcList,0,sizeof(srcList));
            for(i = 0; i < srcNum; i++)
            {
                pSrcList += sprintf(pSrcList, IPADDR_PRINT, IPADDR_PRINT_ARG( (uint32)(*(&(pIgmpHdr->v3.srcList) + i))));
                pSrcList += sprintf(pSrcList, "  ");

                if (i == 20 )
                    break;
            }
            SYS_DBG("A IGMP Group-Specific QUERY received on Vid = %d, Port-%d from "IPADDR_PRINT". (Group:"IPADDR_PRINT" Source List : %s, Interval:%d.%d)\n",
                vid, pktInfo->rx_tag.source_port, IPADDR_PRINT_ARG(pktInfo->sip[0]), IPADDR_PRINT_ARG(group), srcList, t/10, t%10);
        }
    }
	
	if (isSpecQuery)
       mcast_group_portTime_update(pktInfo->ipVersion, vid, pktInfo->groupMac, pktInfo->groupAddress, maxRespTime);

    /* Handle Querier election */
    mcast_igmp_querier_check(vid, pktInfo->sip[0], query_version);

	//tx, set fwdPort to all lan ports
	LOGIC_PORTMASK_SET_ALL(txPmsk);
	mcast_snooping_tx_wrapper(pktInfo, vid, pktInfo->length, txPmsk);
    return SYS_ERR_OK;
}

static uint32 mcast_process_join(packet_info_t *pktInfo, uint8* pktBuf, uint32 pktLen)
{
	sys_logic_portmask_t txPmsk;
	uint8  port; 
	uint16 vid;

	LOGIC_PORTMASK_CLEAR_ALL(txPmsk);
	LOGIC_PORTMASK_SET_PORT(txPmsk, PHYSICAL_PON_PORT);
	
	if(IGMP_MODE_CTC == mcast_mcMode_get())
	{
		mcast_snooping_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length, txPmsk);
		return SYS_ERR_OK;
	}
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;
	
	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{			
		SYS_DBG("A IGMPv1/2 REPORT received on Vid = %d, Port-%d.\n", vid, port);			
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv1 REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif		
	
	mcast_ctc_add_group_wrapper(port, pktInfo->ipVersion, vid, pktInfo->groupMac, pktInfo->groupAddress);

	mcast_snooping_tx_wrapper(pktInfo, vid, pktInfo->length, txPmsk);
    return SYS_ERR_OK;
}

static uint32 mcast_process_leave(packet_info_t *pktInfo, uint8* pktBuf, uint32 pktLen)
{
	sys_logic_portmask_t txPmsk;
	uint8  port; 
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;
	
	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
    	SYS_DBG("A IGMPv2 Leave received on Vid = %d, Port-%d.\n", vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv1 Leave received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif
		
	mcast_ctc_del_group_wrapper(port, pktInfo->ipVersion, vid, pktInfo->groupMac, pktInfo->groupAddress);
		
	LOGIC_PORTMASK_CLEAR_ALL(txPmsk);
	LOGIC_PORTMASK_SET_PORT(txPmsk, PHYSICAL_PON_PORT);
	
	mcast_snooping_tx_wrapper(pktInfo, vid, pktInfo->length, txPmsk);
	
	return SYS_ERR_OK;
}

static uint32 mcast_process_isInclude(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 isInclude REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 isInclude REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, MODE_IS_INCLUDE, srcList, srcNum);

	return SYS_ERR_OK;
}

static uint32 mcast_process_isExclude(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 isExclude REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 isExclude REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, MODE_IS_EXCLUDE, srcList, srcNum);
	
	return SYS_ERR_OK;
}

static uint32 mcast_process_toInclude(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 toInclude REPORT srcNum[%d] received on Vid = %d, Port-%d.\n", srcNum, vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 toInclude REPORT srcNum[%d] received on Vid = %d, Port-%d.\n", srcNum, vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, CHANGE_TO_INCLUDE_MODE, srcList, srcNum);
	
	return SYS_ERR_OK;
}

static uint32 mcast_process_toExclude(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 toExclude REPORT srcNum[%d] received on Vid = %d, Port-%d.\n",srcNum, vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 toExclude REPORT srcNum[%d] received on Vid = %d, Port-%d.\n",srcNum, vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, CHANGE_TO_EXCLUDE_MODE, srcList, srcNum);

	return SYS_ERR_OK;
}

static uint32 mcast_process_allow(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 allow REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 allow REPORT received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, ALLOW_NEW_SOURCES, srcList, srcNum);

	return SYS_ERR_OK;
}

static uint32 mcast_process_block(packet_info_t *pktInfo, uint8* pktBuf,
											uint8* groupMac, uint32 *groupAddr,
											uint32 * srcList, uint16 srcNum)
{
	uint8  port;
	uint16 vid;
	
	vid = pktInfo->vid;
    port = pktInfo->rx_tag.source_port;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		SYS_DBG("A IGMPv3 allow block received on Vid = %d, Port-%d.\n", vid, port);
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		SYS_DBG("A MLDv2 allow block received on Vid = %d, Port-%d.\n", vid, port);
	}
#endif

	mcast_ctc_update_igmpv3_group_wrapper(port, pktInfo->ipVersion, vid, groupMac, 
										groupAddr, BLOCK_OLD_SOURCES, srcList, srcNum);
	
	return SYS_ERR_OK;
}

//implement basic igmpv3 and mldv2 fucntion: not care soure address list now
static uint32 mcast_process_igmpv3_mldv2_report(packet_info_t *pktInfo, uint8* pktBuf, uint32 pktLen)
{
	uint32 i;
	uint16 numOfRecords = 0;
	uint8  recordType;
	uint8  *groupRecords = NULL;
	uint32 groupAddress[4] = {0, 0, 0, 0};
	uint16 srcNum = 0;
	uint32 *srcList = NULL;
	uint8  groupMac[MAC_ADDR_LEN];
	sys_logic_portmask_t txPmsk;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		numOfRecords = ntohs(((igmpv3_report_t *)pktBuf)->numOfRecords);
		if(numOfRecords!=0)
		{
			groupRecords = (uint8 *)(((igmpv3_report_t *)pktBuf)->recordList);
		}
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		numOfRecords = ntohs(((mldv2_report_t *)pktBuf)->numOfRecords);
		if(numOfRecords!=0)
		{
			groupRecords = (uint8 *)(((mldv2_report_t *)pktBuf)->recordList);
		}
	}
#endif

	for(i=0; i<numOfRecords; i++)
	{
		if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
		{
			igmpv3_record_t * igmp_record = (igmpv3_record_t *)groupRecords;
			groupAddress[0] = ntohl(igmp_record->groupAddr);
			srcNum = ntohs(igmp_record->numOfSrc);
			srcList = igmp_record->srcList;
			recordType = igmp_record->type;

			groupMac[0] = 0x01;
			groupMac[1] = 0x00;
			groupMac[2] = 0x5e;
			groupMac[3] = (uint8)((groupAddress[0] >> 16) & 0x7f);
			groupMac[4] = (uint8)((groupAddress[0] >> 8) & 0xff);
			groupMac[5] = (uint8)(groupAddress[0] & 0xff);
		}
#ifdef SUPPORT_MLD_SNOOPING		
		else
		{
			mldv2_record_t * mld_record = (mldv2_record_t *)groupRecords;
			groupAddress[0] = ntohl(mld_record->mCastAddr[0]);
			groupAddress[1] = ntohl(mld_record->mCastAddr[1]);
			groupAddress[2] = ntohl(mld_record->mCastAddr[2]);
			groupAddress[3] = ntohl(mld_record->mCastAddr[3]);		
			srcNum = ntohs(mld_record->numOfSrc);
			srcList = mld_record->srcList;
			recordType = mld_record->type;

			groupMac[0] = 0x33;
       		groupMac[1] = 0x33;
        	groupMac[2] = (uint8)((groupAddress[3] >> 24) & 0xff);
        	groupMac[3] = (uint8)((groupAddress[3] >> 16) & 0xff);
        	groupMac[4] = (uint8)((groupAddress[3] >> 8) & 0xff);
        	groupMac[5] = (uint8)(groupAddress[3] & 0xff);
		}
#endif		
	
		switch(recordType)
		{
		case MODE_IS_INCLUDE:
			mcast_process_isInclude(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		case MODE_IS_EXCLUDE:
			mcast_process_isExclude(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		case CHANGE_TO_INCLUDE_MODE:
			mcast_process_toInclude(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		case CHANGE_TO_EXCLUDE_MODE:
			mcast_process_toExclude(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		case ALLOW_NEW_SOURCES:
			mcast_process_allow(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		case BLOCK_OLD_SOURCES:
			mcast_process_block(pktInfo, groupRecords, groupMac, groupAddress, srcList, srcNum);
			break;
			
		default:
			break;
			
		}

		if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
		{
			/*shift pointer to another group record*/
			groupRecords = groupRecords+8+ srcNum*4 +(((igmpv3_record_t *)(groupRecords))->auxLen)*4;
		}
#ifdef SUPPORT_MLD_SNOOPING		
		else
		{
			/*shift pointer to another group record*/
			groupRecords = groupRecords+20+ srcNum*16 +(((mldv2_record_t *)(groupRecords))->auxLen)*4;
		}
#endif			
	}

	LOGIC_PORTMASK_CLEAR_ALL(txPmsk);
	LOGIC_PORTMASK_SET_PORT(txPmsk, PHYSICAL_PON_PORT);
	
	mcast_snooping_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length, txPmsk);
	
	return TRUE;
}

static uint32 mcast_igmp_process(packet_info_t *pktInfo)
{
	uint8* pktBuf = pktInfo->l3PktBuf;
	uint32 pktLen = pktInfo->l3PktLen;
	igmp_hdr_t *igmpHdr;
	uint32 groupAddress = 0;
	
	igmpHdr = (igmp_hdr_t*)(pktInfo->l3PktBuf);

	/* siyuan 2016-08-23: not process igmp report packet from pon port */
	if(igmpHdr->type != IGMP_TYPE_MEMBERSHIP_QUERY)
	{
		if(FALSE == IsValidLgcLanPort(pktInfo->rx_tag.source_port))
		{
			SYS_DBG("A IGMP REPORT received on Port-%d, not process\n", pktInfo->rx_tag.source_port);
			return SYS_ERR_FAILED;
		}
	}
	
	//get igmp group address adn group mac	
	if(igmpHdr->type != IGMPV3_TYPE_MEMBERSHIP_REPORT)
	{
		pktInfo->groupAddress[0] = ntohl(igmpHdr->groupAddr);
		
		groupAddress = pktInfo->groupAddress[0];
		pktInfo->groupMac[0] = 0x01;
		pktInfo->groupMac[1] = 0x00;
		pktInfo->groupMac[2] = 0x5e;
		pktInfo->groupMac[3] = (uint8)((groupAddress >> 16) & 0x7f);
		pktInfo->groupMac[4] = (uint8)((groupAddress >> 8) & 0xff);
		pktInfo->groupMac[5] = (uint8)(groupAddress & 0xff);
	}
				
	switch(igmpHdr->type)
	{
		case IGMP_TYPE_MEMBERSHIP_QUERY:		
			mcast_process_igmp_query(pktInfo, pktBuf, pktLen);
			break;
			
		case IGMPV1_TYPE_MEMBERSHIP_REPORT:
		case IGMPV2_TYPE_MEMBERSHIP_REPORT:
			igmp_stats.report_rcvd++;
			mcast_process_join(pktInfo, pktBuf, pktLen);
			break;
			
		case IGMPV2_TYPE_MEMBERSHIP_LEAVE:
			igmp_stats.leave_rcvd++;
			mcast_process_leave(pktInfo, pktBuf, pktLen);
			break;

		case IGMPV3_TYPE_MEMBERSHIP_REPORT:
			mcast_process_igmpv3_mldv2_report(pktInfo, pktBuf, pktLen);
			break;

		default:			
			break;
	}						
	
	return SYS_ERR_OK;		
}

#ifdef SUPPORT_MLD_SNOOPING
static uint32 mcast_process_mld_query(packet_info_t * pktInfo, uint8* pktBuf, uint32 pktLen)
{	
    uint32                  t, qqic = 0;
    uint8                   robussVar = 0;
    uint32                  group;
    sys_ipv6_addr_t         groupIpv6;
    uint8                   query_version = MLD_QUERY_V1;
	sys_logic_portmask_t    txPmsk;
    uint16                  i;
    uint16                  srcNum = 0;
    uint8                   srcList[512];
    uint8                   *pSrcList ;
    uint8                   isSpecQuery = TRUE;
    uint32                  lastIntvl = 0;
	uint32                  maxRespTime;
	igmp_config_t           *igmpconfig;
	uint8                   *pSIP = (unsigned char *)pktInfo->sip;

    sys_logic_port_t port;

    mldv1_hdr_t *pMldHdr = (mldv1_hdr_t *)pktBuf;
    mldv2_qryhdr_t *pMldv2Hdr = (mldv2_qryhdr_t *)pktBuf;

	if(pktInfo->rx_tag.source_port != HAL_GET_PON_PORT())
	{
		return SYS_ERR_FAILED;
	}

	igmpconfig = getIgmpConfig();
        
    /*mlv2  query packet length is larger than sizeof(mld_hdr_t)*/
    if (pktLen > sizeof(mldv1_hdr_t))
    {
        query_version = MLD_QUERY_V2;
        osal_memcpy(groupIpv6.ipv6_addr, pMldv2Hdr->mCastAddr, IPV6_ADDR_LEN);
    }
    else
    {
        osal_memcpy(groupIpv6.ipv6_addr, pMldHdr->mCastAddr, IPV6_ADDR_LEN);
    }

    if(query_version != MLD_QUERY_V2)
    {
        if (osal_memcmp(groupIpv6.ipv6_addr, ipv6_proxy_query, IPV6_ADDR_LEN)== 0)     /* general query */
        {
            t = pMldHdr->maxResDelay/1000 ? pMldHdr->maxResDelay/1000 : igmpconfig->responseTime;
            igmp_stats.g_query_rcvd++;
            isSpecQuery = FALSE;

            SYS_DBG("A MLD General QUERY received on Vid = %d, Port-%d from "IPADDRV6_PRINT". (Interval:%d s)\n", 
                pktInfo->vid, pktInfo->rx_tag.source_port, IPADDRV6_PRINT_ARG(pSIP), t);
        }
        else                /* v2 group specific query */
        {
            t = pMldHdr->maxResDelay/1000 ? pMldHdr->maxResDelay/1000 : igmpconfig->responseTime;
            if (pMldHdr->maxResDelay/1000)
                lastIntvl = pMldHdr->maxResDelay/1000;
            igmp_stats.gs_query_rcvd++;
            SYS_DBG("A MLD Group-Specific QUERY received on Vid = %d, Port-%d from "IPADDRV6_PRINT". (Group:"IPADDRV6_PRINT" Interval:%d s)\n",
                pktInfo->vid, pktInfo->rx_tag.source_port, IPADDRV6_PRINT_ARG(pSIP), IPADDRV6_PRINT_ARG(groupIpv6.ipv6_addr), t);
        }
    }
    else
    {
        i = i;
        pSrcList = srcList;
        
        srcNum = ntohl(pMldv2Hdr->numOfSrc);
        t = pMldv2Hdr->responseDelay/1000;
        if (t)
          lastIntvl = pMldv2Hdr->responseDelay/1000;
        
        if (pMldv2Hdr->qqic & 0x80)
        {
            qqic = ((pMldv2Hdr->qqic & 0x0F) | 0x10) << (((pMldv2Hdr->qqic & 0x70) >> 4)+3); 
        }
        else
        {
            qqic = pMldv2Hdr->qqic;
        }
        
        robussVar =  pMldv2Hdr->rsq & 0x07;
        
        if (osal_memcmp(groupIpv6.ipv6_addr, ipv6_proxy_query, IPV6_ADDR_LEN)== 0)     /* general query */
        {
            t = t  ?  t : igmpconfig->responseTime;
            igmp_stats.v3.g_queryV3_rcvd++;
            isSpecQuery = FALSE;
            SYS_DBG("A MLD General QUERY received on Vid = %d, Port-%d from "IPADDRV6_PRINT". (Interval:%d s)\n", 
                pktInfo->vid, pktInfo->rx_tag.source_port, IPADDRV6_PRINT_ARG(pSIP), t);
        }
        else if(srcNum == 0) /*mldv2  group specific query*/
        {
            t = t ? t : igmpconfig->queryIntv;
            igmp_stats.v3.gs_queryV3_rcvd++;
            SYS_DBG("A MLD General QUERY received on Vid = %d, Port-%d from "IPADDRV6_PRINT". (Interval:%d s)\n", 
                pktInfo->vid, pktInfo->rx_tag.source_port, IPADDRV6_PRINT_ARG(pSIP), t);
        }
        else  /*mld v2 group and src specific query*/ 
        {
            t = t ? t : igmpconfig->queryIntv;
            igmp_stats.v3.gss_queryV3_rcvd++;
            SYS_DBG("A MLD General QUERY received on Vid = %d, Port-%d from "IPADDRV6_PRINT". (Interval:%d s)\n", 
                pktInfo->vid, pktInfo->rx_tag.source_port, IPADDRV6_PRINT_ARG(pSIP), t);
        }
    }
	
	if (t)
		maxRespTime = t;
	
	if (isSpecQuery)
       mcast_group_portTime_update(pktInfo->ipVersion, pktInfo->vid, pktInfo->groupMac, pktInfo->groupAddress, maxRespTime);

    /* Handle Querier election */
    mcast_mld_querier_check(pktInfo->vid, pktInfo->sip, query_version);

	//tx, set fwdPort to all lan ports
	LOGIC_PORTMASK_SET_ALL(txPmsk);
	mcast_snooping_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length, txPmsk);
	
    return SYS_ERR_OK;
}

static uint32 mcast_mld_process(packet_info_t *pktInfo)
{	
	uint8* pktBuf = pktInfo->l3PktBuf;
	uint32 pktLen = pktInfo->l3PktLen;
	mldv1_hdr_t *mldHdr;
	uint32 groupAddress = 0;

	SYS_DBG("[IGMP %s]:%d\n", __func__, __LINE__);

	/* siyuan 2016-08-23: not process igmp report packet from pon port */
	if(pktBuf[0] != MLD_TYPE_MEMBERSHIP_QUERY)
	{
		if(FALSE == IsValidLgcLanPort(pktInfo->rx_tag.source_port))
		{
			SYS_DBG("A MLD REPORT received on Port-%d, not process\n", pktInfo->rx_tag.source_port);
			return SYS_ERR_FAILED;
		}
	}
	
	if(pktBuf[0] != MLDV2_TYPE_MEMBERSHIP_REPORT)
	{
		mldHdr = (mldv1_hdr_t *)(pktInfo->l3PktBuf);
		pktInfo->groupAddress[0] = ntohl(mldHdr->mCastAddr[0]);
		pktInfo->groupAddress[1] = ntohl(mldHdr->mCastAddr[1]);
		pktInfo->groupAddress[2] = ntohl(mldHdr->mCastAddr[2]);
		pktInfo->groupAddress[3] = ntohl(mldHdr->mCastAddr[3]);

		groupAddress = pktInfo->groupAddress[3];
		pktInfo->groupMac[0] = 0x33;
        pktInfo->groupMac[1] = 0x33;
        pktInfo->groupMac[2] = (uint8)((groupAddress >> 24) & 0xff);
        pktInfo->groupMac[3] = (uint8)((groupAddress >> 16) & 0xff);
        pktInfo->groupMac[4] = (uint8)((groupAddress >> 8) & 0xff);
        pktInfo->groupMac[5] = (uint8)(groupAddress & 0xff);
	}
	
	switch(pktBuf[0])
	{
		case MLD_TYPE_MEMBERSHIP_QUERY:
			mcast_process_mld_query(pktInfo, pktBuf, pktLen);
			break;
			
		case MLD_TYPE_MEMBERSHIP_REPORT:
			mcast_process_join(pktInfo, pktBuf, pktLen);
			break;
			
		case MLD_TYPE_MEMBERSHIP_DONE:
			mcast_process_leave(pktInfo, pktBuf, pktLen);
			break;
			
		case MLDV2_TYPE_MEMBERSHIP_REPORT:
			mcast_process_igmpv3_mldv2_report(pktInfo, pktBuf, pktLen);
			break;

		default:			
			break;
	}
}
#endif

static int32 checkMCastAddrMapping(uint32 ipVersion, uint32 *ipAddr, uint8* macAddr)
{
	if(ipVersion == MULTICAST_TYPE_IPV4)
	{
		if (macAddr[0]!=0x01)
		{
			return FALSE;
		}
		
		if((macAddr[3]&0x7f)!=(uint8)((ipAddr[0]&0x007f0000)>>16))
		{
			return FALSE;
		}
		
		if(macAddr[4]!=(uint8)((ipAddr[0]&0x0000ff00)>>8))
		{
			return FALSE;
		}

		if(macAddr[5]!=(uint8)(ipAddr[0]&0x000000ff))
		{
			return FALSE;
		}

		return TRUE;
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		if(macAddr[0]!=0x33)
		{
			return FALSE;
		}

		if(macAddr[1]!=0x33)
		{
			return FALSE;
		}

		if(macAddr[2]!=(uint8)((ipAddr[3]&0xff000000)>>24))
		{
			return FALSE;
		}
		
		if(macAddr[3]!=(uint8)((ipAddr[3]&0x00ff0000)>>16))
		{
			return FALSE;
		}

		if(macAddr[4]!=(uint8)((ipAddr[3]&0x0000ff00)>>8))
		{
			return FALSE;
		}
		
		if(macAddr[5]!=(uint8)(ipAddr[3]&0x000000ff))
		{
			return FALSE;
		}
		
		return TRUE;
	}
#endif
	return FALSE;
}

/*Frame Content:
 *	DA(6 bytes)+SA(6 bytes)+ CPU tag(4 bytes) + VlAN tag(Optional, 4 bytes)
 *    Type(IPv4:0x0800, IPV6:0x86DD, PPPOE:0x8864, 2 bytes )
 *    Data(46~1500 bytes)+CRC(4 bytes)
 */
static int32 mcast_parse_packet(packet_info_t *pktInfo) 
{
	uint8 *ptr, *macFrame;
	uint32 ipAddr[4] = {0,0,0,0};
	ipv4hdr_t *pIpHdr;
	uint8* pktBuf;
	int isPPPoe = 0;
	
	macFrame = pktInfo->data;
	ptr = macFrame;
	ptr += 12;

	pktInfo->rx_tag.svid_tagged = FALSE;
	pktInfo->rx_tag.cvid_tagged = FALSE;

	/* FIXME by QL
	 * 1. if service port disabled on rx port, stag will never be parsed, that is to say
	 *    type 0x88A8 is unknown, and this packet will be treated as a unknown packet.
	 *    if this packet has two cvlan tag, only outer tag should be processed
	 * 2. if service port enabled on rx port, stag TPID should be get from VS_TPID register,
	 *    it maybe is not fixed to 0x88A8
	 */
	if ((ptr[0] == 0x88) && (ptr[1] == 0xA8))
	{
		pktInfo->rx_tag.svid_tagged = TRUE;
		pktInfo->rx_tag.outer_vid = (*(uint16 *)(&ptr[2])) & 0xfff;

		ptr += 4;
	}
	if ((ptr[0] == 0x81) && (ptr[1] == 0x00))
	{
		pktInfo->rx_tag.cvid_tagged = TRUE;
		pktInfo->rx_tag.inner_vid = (*(uint16 *)(&ptr[2])) & 0xfff;

		ptr += 4;
	}

	//use inner vid to set packet vid 
	pktInfo->vid = pktInfo->rx_tag.inner_vid;

	/*process packet with PPPOE header*/	
	if (*(int16 *)(ptr)==(int16)htons(PPPOE_ETHER_TYPE))
	{
		SYS_DBG("%s pppoe igmp packet\n",__func__);
		isPPPoe = 1;
		ptr += 2 + 6; /* PPPoE header: 6 bytes */
		if((ptr[0] == 0x00) && (ptr[1] == 0x21)) /* ipv4 */
		{
			ptr += 2;
			pktInfo->ipBuf = ptr;
			pktInfo->ipVersion = MULTICAST_TYPE_IPV4;
		}
		else if(ptr[0] == 0x21) /* compressed ipv4 */
		{
			ptr += 1;
			pktInfo->ipBuf = ptr;
			pktInfo->ipVersion = MULTICAST_TYPE_IPV4;
		}
		else if((ptr[0] == 0x00) && (ptr[1] == 0x57)) /* ipv6 */
		{
			ptr += 2;
			pktInfo->ipBuf = ptr;
			pktInfo->ipVersion = MULTICAST_TYPE_IPV6;	
		}
		else if(ptr[0] == 0x57) /* compressed ipv6 */
		{
			ptr += 1;
			pktInfo->ipBuf = ptr;
			pktInfo->ipVersion = MULTICAST_TYPE_IPV6;	
		}
		else
			return FALSE;	
	}	
	/*check the presence of ipv4 type*/
	else if (*(int16 *)(ptr)==(int16)htons(IPV4_ETHER_TYPE))
	{
		ptr += 2;
		pktInfo->ipBuf = ptr;
		pktInfo->ipVersion = MULTICAST_TYPE_IPV4;
	}
	else if (*(int16 *)(ptr)==(int16)htons(IPV6_ETHER_TYPE))
	{
		ptr += 2;
		pktInfo->ipBuf = ptr;
		pktInfo->ipVersion = MULTICAST_TYPE_IPV6;	
	}
	else
	{
		return FALSE;
	}
	
	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		/*distinguish different IGMP packet:
		                                        ip_header_length   destination_ip      igmp_packet_length   igmp_type   group_address         	
		 IGMPv1_general_query:                  20                   224.0.0.1               8                 0x11          0
		 IGMPv2_general_query:                  24                   224.0.0.1               8                 0x11          0                     
		 IGMPv2_group_specific_query:			24                   224.0.0.1               8                 0x11          !=0  
		 IGMPv3 _query:                         24                   224.0.0.1               >=12              0x11        according_to_different_situation 

		 IGMPv1_join:                           20          actual_multicast_address         8                 0x12          actual_multicast_address
		 IGMPv2_join:                           24          actual_multicast_address         8                 0x16          actual_multicast_address
		 IGMPv2_leave:                          24          actual_multicast_address         8                 0x17          actual_multicast_address
		 IGMPv3_report:                         24          actual_multicast_address       >=12                0x22          actual_multicast_address
		*/
		pIpHdr = (ipv4hdr_t *)(pktInfo->ipBuf);
		pktInfo->ipHdrLen = (uint16)(pIpHdr->ihl << 2);
		pktInfo->l3PktLen = ntohs(pIpHdr->length)- pktInfo->ipHdrLen;
		
		ptr = ptr + pktInfo->ipHdrLen;
		pktInfo->l3PktBuf = ptr;
		pktInfo->macFrameLen=(uint16)((ptr-macFrame)+pktInfo->l3PktLen);
		pktInfo->sip[0] = ntohl(pIpHdr->sip);
		pktInfo->l3Protocol = pIpHdr->protocol;

		if ((ntohl(pIpHdr->dip) == 0xEFFFFFFA) ||
			((ntohl(pIpHdr->dip)&0xFFFFFF00) == 0xE0000000))
		{
			pktInfo->reservedAddr = 1;
		}
		
		/* parse IGMP type and version*/	
		if(pIpHdr->protocol == IGMP_PROTOCOL)
		{
			ipAddr[0] = ntohl(pIpHdr->dip);
			if((isPPPoe == 0) && (checkMCastAddrMapping(MULTICAST_TYPE_IPV4, ipAddr, macFrame) == FALSE))
			{
				return FALSE;
			}
			return TRUE;
		}
		else if ((TCP_PROTOCOL == pIpHdr->protocol) || (UDP_PROTOCOL == pIpHdr->protocol))
		{
			return TRUE;
		}
		
		//not igmp packet, may be multicast data packet
		return FALSE;
	}

#ifdef SUPPORT_MLD_SNOOPING
	if(MULTICAST_TYPE_IPV6 == pktInfo->ipVersion)
	{
		uint8  nextHeader=0;
		uint16 extensionHdrLen = 0;
		ipv6hdr_t * ipv6hdr;
		
		ipv6hdr = (ipv6hdr_t *)(pktInfo->ipBuf);
		pktInfo->sip[0] = ntohl(ipv6hdr->sip[0]);
		pktInfo->sip[1] = ntohl(ipv6hdr->sip[1]);
		pktInfo->sip[2] = ntohl(ipv6hdr->sip[2]);
		pktInfo->sip[3] = ntohl(ipv6hdr->sip[3]);
		
		pktInfo->macFrameLen=(uint16)(ptr - macFrame + IPV6_HEADER_LENGTH + ntohs(ipv6hdr->payloadLenth));
		pktInfo->ipHdrLen = IPV6_HEADER_LENGTH;
		
		nextHeader = ipv6hdr->nextHeader;
		ptr = ptr + IPV6_HEADER_LENGTH;
		while((ptr - pktInfo->ipBuf)<(ntohs(ipv6hdr->payloadLenth)+IPV6_HEADER_LENGTH)
			|| (NO_NEXT_HEADER != nextHeader))
		{
			switch(nextHeader) 
			{
			case HOP_BY_HOP_OPTIONS_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;
				ptr=ptr+extensionHdrLen;
				break;
						
			case ROUTING_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;
				ptr=ptr+extensionHdrLen;
				break;
						
			case FRAGMENT_HEADER:
				nextHeader=ptr[0];
				ptr=ptr+8;
				break;
				
			case DESTINATION_OPTION_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;
				ptr=ptr+extensionHdrLen;
				break;

			case TCP_PROTOCOL:
			case UDP_PROTOCOL:
			case ICMPV6_PROTOCOL:
				pktInfo->l3PktLen = ntohs(ipv6hdr->payloadLenth)-(uint16)(ptr - pktInfo->ipBuf-IPV6_HEADER_LENGTH);
				pktInfo->l3PktBuf=ptr;
				
				ipAddr[0] = ntohl(ipv6hdr->dip[0]);
				ipAddr[1] = ntohl(ipv6hdr->dip[1]);
				ipAddr[2] = ntohl(ipv6hdr->dip[2]);
				ipAddr[3] = ntohl(ipv6hdr->dip[3]);
				
				if (ICMPV6_PROTOCOL == nextHeader)
				{
					if((isPPPoe == 1) || (checkMCastAddrMapping(MULTICAST_TYPE_IPV6, ipAddr, macFrame)==TRUE))
					{
						pktInfo->l3Protocol = ICMPV6_PROTOCOL;

						return TRUE;
					}
					else {
						if ((macFrame[0] == 0x33) && (macFrame[1] == 0x33) && (macFrame[2] == 0xFF))
						{
							/* this is a solicitation node multicast address */
							pktInfo->reservedAddr = 1;
						}
						return FALSE;
					}
				}
				else
					return TRUE;

				nextHeader=NO_NEXT_HEADER;
				break;

			default:
				nextHeader=NO_NEXT_HEADER;
				break;
			}			
		}

		return FALSE;
	}
#endif
}

static int32 mcast_igmp_check_header(packet_info_t *pktInfo)
{
    ipv4hdr_t *pIpHdr;
	igmp_hdr_t *pIgmpHdr;
	igmpv3_report_t *report;
    igmpv3_record_t *grec, *ptr;
	uint32 group;
	uint32 size1, size2;
	uint16 csum = 0;
	int i;
	
	pIpHdr = (ipv4hdr_t *)(pktInfo->ipBuf);
   
    if (pIpHdr->ihl < 5 || pIpHdr->version != 4)
    {
        SYS_DBG("IP Header Error("IPADDR_PRINT"): Version or Length Incorrect pIpHdr->ihl %02x pIpHdr->version %02x!\n",IPADDR_PRINT_ARG(pIpHdr->dip),pIpHdr->ihl,pIpHdr->version);
        return ERRONEOUS_PACKET;
    }
    
    if (!IGMP_IN_MULTICAST(ntohl(pIpHdr->dip)))
    {
        SYS_DBG("IP Header Error: Dst IP("IPADDR_PRINT") is not in "
            "Multicast range(224.0.0.0 ~ 239.255.255.255)\n", IPADDR_PRINT_ARG(pIpHdr->dip));
        return ERRONEOUS_PACKET;
    }

    if (pIpHdr->protocol != IGMP_PROTOCOL)
    {
        if ((pIpHdr->protocol == 255/* IPPROTO_RAW */) ||
			(pIpHdr->protocol == TCP_PROTOCOL) ||
			(pIpHdr->protocol == UDP_PROTOCOL))
        {
            return MULTICAST_DATA_PACKET;
        }

        return VALID_OTHER_PACKET;
    }

	pIgmpHdr = (igmp_hdr_t*)((char*)pIpHdr + (pIpHdr->ihl<<2));
	
    if (pIgmpHdr->type != IGMP_TYPE_MEMBERSHIP_QUERY &&
        pIgmpHdr->type != IGMPV1_TYPE_MEMBERSHIP_REPORT &&
        pIgmpHdr->type != IGMPV2_TYPE_MEMBERSHIP_REPORT &&
        pIgmpHdr->type != IGMPV2_TYPE_MEMBERSHIP_LEAVE &&
        pIgmpHdr->type != IGMPV3_TYPE_MEMBERSHIP_REPORT)
    {
        SYS_DBG("IGMP Header Error: Unsupported IGMP version, type = 0x%X!\n", pIgmpHdr->type);
        return UNSUPPORTED_IGMP_CONTROL_PACKET;
    }
	
	mcast_checksum_get((uint16 *)pIgmpHdr, pktInfo->l3PktLen, &csum);
	if (csum != 0)
	{
		SYS_DBG("IGMPv3 Header Error: Inorrect Checksum!\n");
		return ERRONEOUS_PACKET;
	}
	
	if (pIgmpHdr->type == IGMPV3_TYPE_MEMBERSHIP_REPORT)
    {
        if (ntohl(0xE0000016L) != ntohl(pIpHdr->dip))
        {
            SYS_DBG("Dst IP("IPADDR_PRINT") in IGMPv3 Report is not 224.0.0.22\n",
                IPADDR_PRINT_ARG(pIpHdr->dip));
            return ERRONEOUS_PACKET;
        }

        report = (igmpv3_report_t*)pIgmpHdr;

        if (report->numOfRecords == 0)
        {
            return SUPPORTED_IGMP_CONTROL_PACKET;
        }

        grec = report->recordList;
		size1 = sizeof(igmpv3_record_t);

        for (i = 0; i < report->numOfRecords; i++)
        {
            group = ntohl(grec->groupAddr);
            size2 = grec->numOfSrc * 4;
            ptr = (igmpv3_record_t*)((char*) grec + size1 + size2);
            grec = ptr;

            if (!IGMP_IN_MULTICAST(ntohl(group)))
            {
                SYS_DBG("IGMPv3 Header Error: Group Address("IPADDR_PRINT") is not in "
                    "Multicast range(224.0.0.0~239.255.255.255)\n", IPADDR_PRINT_ARG(group));
                return ERRONEOUS_PACKET;
            }

            if ((ntohl(group) >= ntohl(0xE0000000L)) && (ntohl(group) <= ntohl(0xE00000FFL)))
            {
                SYS_DBG("IGMPv3 Header Error: Group Address("IPADDR_PRINT") belongs to "
                    "Reserved Multicast range(224.0.0.0~224.0.0.255)!\n", IPADDR_PRINT_ARG(group));
                return ERRONEOUS_PACKET;
            }
        }

        return SUPPORTED_IGMP_CONTROL_PACKET;
    }
	
    if (pIgmpHdr->type == IGMP_TYPE_MEMBERSHIP_QUERY)
    {
        if ((0 == pIgmpHdr->groupAddr) && (ntohl(0xE0000001L) != ntohl(pIpHdr->dip)))
        {
            /* General Query */
            SYS_DBG("IP Header Error: Dst IP("IPADDR_PRINT") of IGMP GENERAL-QUERY "
                "packet is incorrect!\n", IPADDR_PRINT_ARG(pIpHdr->dip));
            return ERRONEOUS_PACKET;
        }

        if ((0 != pIgmpHdr->groupAddr) && (ntohl(0xE0000001L) == ntohl(pIpHdr->dip)))
        {
            /*Error General Query */
            SYS_DBG("Igmp Header Error: group IP("IPADDR_PRINT") of IGMP GENERAL-QUERY "
                "packet is incorrect!\n", IPADDR_PRINT_ARG(pIgmpHdr->groupAddr));
            return ERRONEOUS_PACKET;
        }

        if ((0 != pIgmpHdr->groupAddr) && (pIgmpHdr->groupAddr != pIpHdr->dip))
        {
            /* GS-Specific Query */
            SYS_DBG("IGMP Header Error: Group Address("IPADDR_PRINT") is not same "
                "with the Dst IP("IPADDR_PRINT") of IGMP GS-Specific QUERY packet!\n",
                IPADDR_PRINT_ARG(pIgmpHdr->groupAddr), IPADDR_PRINT_ARG(pIpHdr->dip));
            return ERRONEOUS_PACKET;
        }
    }

    if (((pIgmpHdr->type == IGMPV1_TYPE_MEMBERSHIP_REPORT) ||
        (pIgmpHdr->type == IGMPV2_TYPE_MEMBERSHIP_REPORT)) &&
        (pIgmpHdr->groupAddr != pIpHdr->dip))
    {
        SYS_DBG("IGMP Header Error: Group Address("IPADDR_PRINT") is not same "
            "with the Dst IP("IPADDR_PRINT") of IGMP REPORT packet!\n",
            IPADDR_PRINT_ARG(pIgmpHdr->groupAddr), IPADDR_PRINT_ARG(pIpHdr->dip));
        return ERRONEOUS_PACKET;
    }

    if ((pIgmpHdr->type == IGMPV2_TYPE_MEMBERSHIP_LEAVE) && (ntohl(0xE0000002L) != ntohl(pIpHdr->dip)))
    {
        SYS_DBG("IP Header Error: Dst IP("IPADDR_PRINT") of IGMP LEAVE packet is incorrect!\n", IPADDR_PRINT_ARG(pIpHdr->dip));
        return ERRONEOUS_PACKET;
    }
	
    return SUPPORTED_IGMP_CONTROL_PACKET;
}

#ifdef SUPPORT_MLD_SNOOPING
static uint8  mcast_mld_dmac_check(uint8 *pMac, uint32 ipAddr)
{
	uint32 dip;

	if (NULL == pMac)
		return FALSE;

	dip = *((uint32 *)(pMac + 2));

	if (dip != ipAddr)
		return FALSE;

	return TRUE;
}

static uint8 is_ipv6Addr_equel(const uint8 *pSrc1Addr, const uint8 *pSrc2Addr)
{
    if (pSrc1Addr == NULL || pSrc2Addr == NULL)
        return FALSE;
    
    if (osal_memcmp(pSrc1Addr, pSrc2Addr, IPV6_ADDR_LEN) == 0)
        return TRUE;

    return FALSE;
}

static mcast_mld_check_header(packet_info_t *pktInfo)
{
	uint16			i, size1, size2, mldLen;

	mldv1_hdr_t		*pMldHdr;
	mldv2_report_t	*report;
	mldv2_record_t   *grec, *ptr;

	uint8  *pGroupIpv6 = NULL;
	uint8  *pDip;

	uint32	dip;

	ipv6hdr_t *pIpv6Hdr = (ipv6hdr_t *)(pktInfo->ipBuf);
	uint8 *pMac = pktInfo->data;

	if (((pIpv6Hdr->vtf>>28)&0xF) != 6)
	{
		SYS_DBG("IP Header Error: Version	Incorrect!\n");
		return ERRONEOUS_PACKET;
	}

	dip = pIpv6Hdr->dip[3];
	if (!mcast_mld_dmac_check(pMac, dip))
	{
		pDip = (uint8 *)pIpv6Hdr->dip;
		SYS_DBG("IPv6 Header Error: Dst IPv6("IPADDRV6_PRINT") is not match "
			"The Mac address "MAC_PRINT" \n", IPADDRV6_PRINT_ARG(pDip), MAC_PRINT_ARG(pMac));
		return ERRONEOUS_PACKET;	
	}

	if (is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dvmrp_routing))
	{
		return ROUTER_ROUTING_PACKET;
	}

	if ((is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_ospf_all_routing)) ||
		(is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_ospf_designated_routing)))
	{
		return ROUTER_ROUTING_PACKET;
	}

	if (is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_pimv2_routing))
	{
		return ROUTER_ROUTING_PACKET;
	}


	if (pIpv6Hdr->nextHeader == NO_NEXT_HEADER  /* Ipv6 data */)
	{
		return MULTICAST_DATA_PACKET;
	}

	pMldHdr = (mldv1_hdr_t *)pktInfo->l3PktBuf;

	/* Routing protocol packet */ 
	if(pMldHdr->type >= MLD_ROUTER_SOLICITATION && pMldHdr->type <= MLD_REDIRECT)
		return ROUTER_ROUTING_PACKET;

	if (pMldHdr->type == MLD_TYPE_MEMBERSHIP_QUERY ||
		pMldHdr->type == MLD_TYPE_MEMBERSHIP_REPORT ||
		pMldHdr->type == MLD_TYPE_MEMBERSHIP_DONE ||
		pMldHdr->type == MLDV2_TYPE_MEMBERSHIP_REPORT)
	{
		SYS_DBG("MLD type = 0x%X!\n", pMldHdr->type);
	}
	else
	{
		SYS_DBG("MLD Header Error: Unsupported mld type, type = 0x%X!\n", pMldHdr->type);
		return VALID_OTHER_PACKET;
	}

	/* MLDv2  header length */
	mldLen = pktInfo->l3PktLen;

	if (pMldHdr->type == MLDV2_TYPE_MEMBERSHIP_REPORT)
	{
		pDip = (uint8 *)pIpv6Hdr->dip;
		
		if (!is_ipv6Addr_equel(pDip, ipv6_mldv2_report))
		{
			SYS_DBG("Dst IPv6("IPADDRV6_PRINT") in MLDv2 Report is not  FF02::16\n",
						IPADDRV6_PRINT_ARG(pDip));
			return ERRONEOUS_PACKET;
		}

		report = (mldv2_report_t *)((char*)pMldHdr);

		if (0 == report->numOfRecords )
		{
			return SUPPORTED_IGMP_CONTROL_PACKET;
		}

		grec = report->recordList;

		size1 = sizeof(mldv2_record_t);

		for (i = 0; i < report->numOfRecords; i++)
		{
			pGroupIpv6 = (uint8 *)grec->mCastAddr;
			size2 = grec->numOfSrc * IPV6_ADDR_LEN;
			ptr = (mldv2_record_t *)((char*) grec + size1 + size2);
			grec = ptr;

			if (pGroupIpv6[0] != 0xff)
			{
				SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is not	"
						"Multicast	address \n", IPADDRV6_PRINT_ARG(pGroupIpv6));
				return ERRONEOUS_PACKET;
			}

			/*	rfc4541,
				MLD messages are also not sent regarding groups with addresses in the
				range FF00::/15 (which encompasses both the reserved FF00::/16 and
				node-local FF01::/16 IPv6 address spaces).	These addresses should
				never appear in packets on the link.*/
			if ((pGroupIpv6[0] == 0xff) && ( (pGroupIpv6[1]  & 0xf0) == 0x0) &&
				  (osal_memcmp(&pGroupIpv6[2], &ipv6_proxy_query[2] ,IPV6_ADDR_LEN-2) == 0)  )
			{
				SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is "
					"invalid Multicast group address !\n", IPADDRV6_PRINT_ARG(pGroupIpv6));

				return ERRONEOUS_PACKET;
			}
		}

		return SUPPORTED_IGMP_CONTROL_PACKET;
	}

	if ((pMldHdr->type == MLD_TYPE_MEMBERSHIP_QUERY) && 
		(is_ipv6Addr_equel((uint8 *)pMldHdr->mCastAddr, ipv6_proxy_query)))
	{
		;
	}
	else
	{
		pGroupIpv6 = (uint8 *)pMldHdr->mCastAddr;
		if (pGroupIpv6[0] != 0xff)
		{
			SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is not	"
				"Multicast	address \n", IPADDRV6_PRINT_ARG(pGroupIpv6));
				return ERRONEOUS_PACKET;
		}
		/*	rfc4541,
			MLD messages are also not sent regarding groups with addresses in the
			range FF00::/15 (which encompasses both the reserved FF00::/16 and
			node-local FF01::/16 IPv6 address spaces).	These addresses should
			never appear in packets on the link.*/

		if ((pGroupIpv6[0] == 0xff) && ((pGroupIpv6[1] & 0xf0) == 0x0) &&
			  (osal_memcmp(&pGroupIpv6[2], &ipv6_proxy_query[2], IPV6_ADDR_LEN -2) == 0)  )
		{
			SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is "
					"invalid Multicast group address !\n", IPADDRV6_PRINT_ARG(pGroupIpv6));
			return ERRONEOUS_PACKET;
		}
	}

	if (pMldHdr->type == MLD_TYPE_MEMBERSHIP_QUERY)
	{
		if (is_ipv6Addr_equel((uint8 *)pMldHdr->mCastAddr, ipv6_proxy_query)) 
		{
			 /* General Query */
			if ( (!is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_query1)) &&
				  (!is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_query2)))
			{
				pDip = (uint8 *)pIpv6Hdr->dip;
				SYS_DBG("IP Header Error: Dst IP("IPADDRV6_PRINT") of MLD GENERAL-QUERY "
						"packet is incorrect!\n", IPADDRV6_PRINT_ARG(pDip));
				return ERRONEOUS_PACKET;
			}
		}
		else
		{
			 /*Error General Query */
			if ( (is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_query1)) || 
				 (is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_query2)))
			{
				pGroupIpv6 = (uint8 *)pMldHdr->mCastAddr;
				SYS_DBG("Mld Header Error: group IP("IPADDRV6_PRINT") of MLD GENERAL-QUERY "
						"packet is incorrect!\n", IPADDRV6_PRINT_ARG(pGroupIpv6));
				return ERRONEOUS_PACKET;	
			}

			/* GS-Specific Query */
			if (!is_ipv6Addr_equel((uint8 *)pMldHdr->mCastAddr, (uint8 *)pIpv6Hdr->dip))
			{
				pGroupIpv6 = (uint8 *)pMldHdr->mCastAddr;
				pDip = (uint8 *)pIpv6Hdr->dip;
				SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is not same "
						"with the Dst IPV6("IPADDRV6_PRINT") of MLD GS-Specific QUERY packet!\n",
					IPADDRV6_PRINT_ARG(pGroupIpv6), IPADDRV6_PRINT_ARG(pDip));
				return ERRONEOUS_PACKET;	 
			}  
		}
	}

	if (pMldHdr->type == MLD_TYPE_MEMBERSHIP_REPORT)
	{
		if (!is_ipv6Addr_equel((uint8 *)pMldHdr->mCastAddr, (uint8 *)pIpv6Hdr->dip))
		{
			pGroupIpv6 = (uint8 *)pMldHdr->mCastAddr;
			pDip = (uint8 *)pIpv6Hdr->dip;
			SYS_DBG("MLD Header Error: Group Address("IPADDRV6_PRINT") is not same "
					"with the Dst IPV6("IPADDRV6_PRINT") of MLD REPORT packet!\n",
					IPADDRV6_PRINT_ARG(pGroupIpv6), IPADDRV6_PRINT_ARG(pDip));
			return ERRONEOUS_PACKET;
		}
	}

	if (pMldHdr->type == MLD_TYPE_MEMBERSHIP_DONE)
	{
		if (!(is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_leave1) ||
			  is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_leave2) ||
			  is_ipv6Addr_equel((uint8 *)pIpv6Hdr->dip, ipv6_dip_leave3)))
		{
			pDip = (uint8 *)pIpv6Hdr->dip;
			SYS_DBG("IP Header Error: Dst IP("IPADDRV6_PRINT") of MLD LEAVE packet is incorrect!\n", IPADDRV6_PRINT_ARG(pDip));
			return ERRONEOUS_PACKET;
		}
	}

	return SUPPORTED_IGMP_CONTROL_PACKET;
}
#endif

/*
 * TODO: maybe some case should be redo, such as MULTICAST_DATA_PACKET, 
 *       it should be flood or trap to protocol stack for further process.
 */
static int32 mcast_check_packet(packet_info_t *pktInfo)
{
	int32  result;	
	uint16 vid = pktInfo->vid;

	if(pktInfo->ipVersion == MULTICAST_TYPE_IPV4)
	{
		igmp_stats.total_pkt_rcvd++;
    	result = mcast_igmp_check_header(pktInfo);
	    switch (result)
	    {
	        case SUPPORTED_IGMP_CONTROL_PACKET:
	            break;

	        case UNSUPPORTED_IGMP_CONTROL_PACKET:
	            igmp_stats.valid_pkt_rcvd++;
	            igmp_stats.other_rcvd++;
	           
	            SYS_DBG("Unsupported IGMP Pkt: Port = %d, Vid = %d\n", pktInfo->rx_tag.source_port, vid);
	            return FALSE;

	        case ERRONEOUS_PACKET:
	            SYS_DBG( "ERRONEOUS_PACKET drop.\n");
	            igmp_stats.invalid_pkt_rcvd++;
	            return FALSE;

	        case VALID_OTHER_PACKET:
	        case MULTICAST_DATA_PACKET:
				SYS_DBG( "OTHER_PACKET drop.\n");
				igmp_stats.invalid_pkt_rcvd++;
				return FALSE;
				
	        case ROUTER_ROUTING_PACKET:
	            SYS_DBG( "A Routing Pkt: Port = %d, Vid = %d\n", pktInfo->rx_tag.source_port, vid);
				return FALSE;          

	        default:
	            SYS_DBG("An unrecognized packet received.\n");
	            return FALSE;
	    }

	    igmp_stats.valid_pkt_rcvd++;
	}
#ifdef SUPPORT_MLD_SNOOPING
	else
	{
		result = mcast_mld_check_header(pktInfo);
		switch (result)
		{
	        case SUPPORTED_IGMP_CONTROL_PACKET:
	            break;

	        case UNSUPPORTED_IGMP_CONTROL_PACKET:
	            SYS_DBG("Unsupported IGMP Pkt: Port = %d, Vid = %d\n", pktInfo->rx_tag.source_port, vid);
	            return FALSE;

	        case ERRONEOUS_PACKET:
	            SYS_DBG( "ERRONEOUS_PACKET drop.\n");
	            return FALSE;

	        case VALID_OTHER_PACKET:
	        case MULTICAST_DATA_PACKET:
				SYS_DBG( "OTHER_PACKET drop.\n");
				return FALSE;
				
	        case ROUTER_ROUTING_PACKET:
	            SYS_DBG( "A Routing Pkt: Port = %d, Vid = %d\n", pktInfo->rx_tag.source_port, vid);
				return FALSE;          

	        default:
	            SYS_DBG("An unrecognized packet received.\n");
	            return FALSE;
		}
	}
#endif
	
	return TRUE;
}

static int32 mcast_igmp_mld_process(packet_info_t * pktInfo)
{	
	sys_logic_portmask_t txPmsk;
	
	SYS_DBG("Received IGMP packet  DA:"MAC_PRINT"  SA:"MAC_PRINT"\n", MAC_PRINT_ARG(pktInfo->data), MAC_PRINT_ARG(pktInfo->data + 6));

	if (0 == isEnableIgmpSnooping())
	{
		SYS_DBG("Flood IGMP packet becasue IGMP is disabled\n");
		goto flood_pkt;
	}
	
	if (TRUE != mcast_check_packet(pktInfo))
	{
		SYS_DBG("IGMP packet check error\n");
		
		return SYS_ERR_FAILED;
	}

	switch(pktInfo->l3Protocol)
	{
		case IGMP_PROTOCOL:		
			mcast_igmp_process(pktInfo);
			break;
#ifdef SUPPORT_MLD_SNOOPING
		case ICMPV6_PROTOCOL:			
			mcast_mld_process(pktInfo);
			break;
#endif
	}
	
	return SYS_ERR_OK;

flood_pkt:
	//TODO forward igmp control packet
	if(pktInfo->rx_tag.source_port == HAL_GET_PON_PORT())
	{
		LOGIC_PORTMASK_SET_ALL(txPmsk);
	}
	else
	{
		LOGIC_PORTMASK_CLEAR_ALL(txPmsk);
		LOGIC_PORTMASK_SET_PORT(txPmsk, PHYSICAL_PON_PORT);
	}
	mcast_snooping_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length, txPmsk);
	
	return SYS_ERR_OK;
}

int32 mcast_fast_forward(packet_info_t * pktInfo)
{
	mcast_data_info_t data_info;
	mcast_fwd_info_t fwd_info;
	int ret = FAIL;

	if(MULTICAST_TYPE_IPV4 == pktInfo->ipVersion)
	{
		ipv4hdr_t *pIpHdr;
		
		pIpHdr = (ipv4hdr_t *)(pktInfo->ipBuf);
		data_info.ipVersion = MULTICAST_TYPE_IPV4;
		data_info.vid = pktInfo->vid;
		data_info.sip[0] = (uint32)(pIpHdr->sip);
		data_info.groupAddr[0] = (uint32)(pIpHdr->dip);
	}
	else
	{
		ipv6hdr_t * ipv6hdr;
		
		ipv6hdr = (ipv6hdr_t *)(pktInfo->ipBuf);
		data_info.ipVersion = MULTICAST_TYPE_IPV6;
		data_info.vid = pktInfo->vid;
		
		data_info.sip[0] = ntohl(ipv6hdr->sip[0]);
		data_info.sip[1] = ntohl(ipv6hdr->sip[1]);
		data_info.sip[2] = ntohl(ipv6hdr->sip[2]);
		data_info.sip[3] = ntohl(ipv6hdr->sip[3]);
	
		data_info.groupAddr[0] = ntohl(ipv6hdr->dip[0]);
		data_info.groupAddr[1] = ntohl(ipv6hdr->dip[1]);
		data_info.groupAddr[2] = ntohl(ipv6hdr->dip[2]);
		data_info.groupAddr[3] = ntohl(ipv6hdr->dip[3]);			
	}

	ret = mcast_get_data_fwd_info(&data_info, &fwd_info);
	SYS_DBG("%s %d "IPADDR_PRINT" fwdPort[%x]\n", __FUNCTION__, __LINE__, 
			IPADDR_PRINT_ARG(data_info.groupAddr[0]), fwd_info.fwdPortMask);

	if(ret == SUCCESS)
	{
		sys_logic_portmask_t txPmsk;
		
		txPmsk.bits[0] = fwd_info.fwdPortMask;
		LOGIC_PORTMASK_CLEAR_PORT(txPmsk, pktInfo->rx_tag.source_port);

		mcast_snooping_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length, txPmsk);
	}
	else
	{
		//flood
		mcast_flood_tx_wrapper(pktInfo, pktInfo->vid, pktInfo->length);
	}
}

int32 mcast_igmp_querier_set(igmp_querier_entry_t *pQuerier)
{
    igmp_querier_entry_t *pEntry;
	igmp_config_t *igmpconfig;

    SYS_PARAM_CHK(NULL == pQuerier, SYS_ERR_NULL_POINTER);
    
    sys_ipAddr_get("br0", &sys_ip);
	igmpconfig = getIgmpConfig();
	
    mcast_querier_db_get(pQuerier->vid, pQuerier->ipType, &pEntry);
    if (pEntry)
    {
        pEntry->enabled = pQuerier->enabled;
        pEntry->version = pQuerier->version;
        
        if (pEntry->enabled)
        {
            pEntry->status = IGMP_QUERIER;
            pEntry->timer = igmpconfig->queryIntv/4;
			pEntry->ip[0] = sys_ip;
        }
        else
        {
            pEntry->status = IGMP_NON_QUERIER;
            pEntry->ip[0] = 0;
        }
    }
    else
    {
        SYS_DBG("Warring! querier entry for VLAN-%d doesn't exist!\n", pQuerier->vid);
    }    

    return SYS_ERR_OK;
}

/* delete querier entry */
int mcast_vlan_querier_del(int vid)
{
	mcast_querier_db_del(vid, MULTICAST_TYPE_IPV4);
	mcast_querier_db_del(vid, MULTICAST_TYPE_IPV6);

	return SYS_ERR_OK;
}

int mcast_vlan_querier_add(int vid)
{
	igmp_querier_entry_t Querier;
	
	mcast_querier_db_add(vid, MULTICAST_TYPE_IPV4);
	mcast_querier_db_add(vid, MULTICAST_TYPE_IPV6);

	memset(&Querier, 0, sizeof(Querier));
	Querier.vid = vid;
	Querier.ipType = MULTICAST_TYPE_IPV4;
	Querier.enabled = ENABLED;
	Querier.version = IGMP_QUERY_V3;
	mcast_igmp_querier_set(&Querier);

	memset(&Querier, 0, sizeof(Querier));
	Querier.vid = vid;
	Querier.ipType = MULTICAST_TYPE_IPV6;
	Querier.enabled = ENABLED;
	Querier.version = MLD_QUERY_V2;
	mcast_igmp_querier_set(&Querier);

	return SYS_ERR_OK;
}

static int32 get_ipv6_transport_protocol(ipv6hdr_t * ipv6h)
{
	unsigned char *ptr=NULL;
	unsigned char *startPtr=NULL;
	unsigned char *lastPtr=NULL;
	unsigned char nextHeader=0;
	unsigned short extensionHdrLen=0;
	unsigned char  optionDataLen=0;
	unsigned char  optionType=0;
	unsigned int ipv6RAO=0;

	if(ipv6h == NULL)
	{
		return -1;
	}
	
	startPtr= (unsigned char *)ipv6h;
	lastPtr=startPtr+sizeof(ipv6hdr_t)+(ipv6h->payloadLenth);
	nextHeader= ipv6h->nextHeader;
	ptr=startPtr+sizeof(ipv6hdr_t);
	while(ptr<lastPtr)
	{
		switch(nextHeader) 
		{
			case HOP_BY_HOP_OPTIONS_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;			
                ptr=ptr+extensionHdrLen;
				break;
			case ROUTING_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;			
                ptr=ptr+extensionHdrLen;
				break;
			case FRAGMENT_HEADER:
				nextHeader=ptr[0];
				ptr=ptr+8;
				break;
			case DESTINATION_OPTION_HEADER:
				nextHeader=ptr[0];
				extensionHdrLen=((uint16)(ptr[1])+1)*8;
				ptr=ptr+extensionHdrLen;
				break;
			case ICMPV6_PROTOCOL:
				nextHeader=NO_NEXT_HEADER;
				//if((ptr[0]==MLD_QUERY) ||(ptr[0]==MLDV1_REPORT) ||(ptr[0]==MLDV1_DONE) ||(ptr[0]==MLDV2_REPORT))
				{
					return ICMPV6_PROTOCOL;
				}
				break;		
			case TCP_PROTOCOL:
				nextHeader=NO_NEXT_HEADER;
				return TCP_PROTOCOL;
				break;
			case UDP_PROTOCOL:
				nextHeader=NO_NEXT_HEADER;
				return UDP_PROTOCOL;
				break;	
			default:		
				return -1;
				break;
		}
	}
	return -1;
}

int mcast_recv(unsigned char *frame, unsigned int frame_len)
{
	int len=0;
	unsigned short sll_port;
	unsigned short sll_vlanid;
	int ret = EPON_OAM_ERR_OK;
    packet_info_t pktInfo;
	int max_lan_port_num = MAX_PORT_NUM;

	if (ENABLED == igmp_packet_stop)
    {
        SYS_DBG("Free IGMP packet becasue packet stop sign is enabled\n");
        return ret;
    }
 
	memset((void*)&pktInfo, 0, sizeof(packet_info_t));
	if (frame_len > 45) //should frame length > 60 ? forged vlan tagged igmp packet is < 60
	{
		/* abstract info from driver */
    	len = frame_len - MAC_FRAME_OFFSET;
	
		sll_port = frame[0]; /* set by pRxInfo->opts3.bit.src_port_num + 1 */
		sll_vlanid = (frame[1] << 8) | frame[2];

        pktInfo.length = (uint32)len;
        pktInfo.data = frame + MAC_FRAME_OFFSET; /* make sure ether frame 4 bytes align */
        pktInfo.tail = frame + len;
		/* siyuan 2016-11-30: for 9602B, LAN_PORT_NUM is 2 and it has port remapping, so source_port maybe 2 or 3 which >= LAN_PORT_NUM 
		   use MAX_PORT_NUM which is fixed to 4 and pon port is always 4 except 9601b */
		if(TRUE == is9601B() || TRUE == is9602C())
			max_lan_port_num = LAN_PORT_NUM;
		if((sll_port - 1) >= max_lan_port_num)
			pktInfo.rx_tag.source_port = sll_port - 1; /* igmp query from pon port */
		else
        	pktInfo.rx_tag.source_port =  get_virtual_lan_port(sll_port - 1); //equal to pRxInfo->opts3.bit.src_port_num
        pktInfo.rx_tag.inner_vid = sll_vlanid;
		
		/* parse packet here */
		if (FALSE == mcast_parse_packet(&pktInfo))
			goto END;
		
        if (MULTICAST_TYPE_IPV4 == pktInfo.ipVersion)
		{
			if (IGMP_PROTOCOL == pktInfo.l3Protocol)
			{
				return mcast_igmp_mld_process(&pktInfo);			
			}
			else if ((TCP_PROTOCOL == pktInfo.l3Protocol) || (UDP_PROTOCOL == pktInfo.l3Protocol))
			{
				//igmp snooping multicast IP data packet
				return mcast_fast_forward(&pktInfo);
			}
        }
	#ifdef SUPPORT_MLD_SNOOPING
        else if (MULTICAST_TYPE_IPV6 == pktInfo.ipVersion)
		{                    
			if (ICMPV6_PROTOCOL == pktInfo.l3Protocol) 
			{	
				return mcast_igmp_mld_process(&pktInfo);
			}
			else if ((TCP_PROTOCOL == pktInfo.l3Protocol) || (UDP_PROTOCOL == pktInfo.l3Protocol))
			{
				return mcast_fast_forward(&pktInfo);
			}
        }         
  	#endif	

END:
		if (pktInfo.reservedAddr)
		{//trap to protocol stack
			
		}
		else
		{//flood
			mcast_flood_tx_wrapper(&pktInfo, pktInfo.vid, pktInfo.length);
		}
    }
	return ret;
}

void * epon_igmp_rxThread(void *argu)
{
    int ret;
    pr_identifier_t *prId;
    unsigned short dataLen;
    unsigned char *payload;
    unsigned char llidIdx;

    igmpRedirect_sock = socket(PF_NETLINK, SOCK_RAW,NETLINK_USERSOCK);
    ptk_redirect_userApp_reg(igmpRedirect_sock, PR_USER_UID_IGMPMLD, MAX_PAYLOAD);

    payload = (unsigned char *)malloc(MAX_PAYLOAD * sizeof(char));
    /* Read message from kernel */ 
    if(NULL == payload)
    {
        SYS_DBG("[OAM IGMP:%s:%d] No packet buffer for Rx!\n", __FILE__, __LINE__);
        return NULL;
    }
	SYS_DBG("[OAM IGMP:%s:%d] rxThread!\n", __FILE__, __LINE__);
    while(1)
    {
        ret = ptk_redirect_userApp_recvPkt(igmpRedirect_sock, MAX_PAYLOAD, &dataLen, payload);
        if(ret > 60)
        {
        	/* need to process pppoe packet */
            //if((payload[MAC_FRAME_OFFSET] & 0x1) == 1)
            {
                /*payload[0] = src port num; payload[1]&payload[2] = vlanid*/
                mcast_recv(payload, dataLen);
            }
        }
    }
    free(payload);
    close(igmpRedirect_sock);

	return NULL;
}

void * epon_igmp_timerThread(void *argu)
{
	struct timeval ltv;
	uint32 startTime, endTime;
    int32  interval;

	SYS_DBG("[OAM IGMP:%s:%d] timerThread!\n", __FILE__, __LINE__);
    /* forever loop */
    for (;;)
    {
        startTime = 0;
        endTime = 0;
        interval = 0;

        gettimeofday(&ltv, NULL);
        startTime = ltv.tv_sec * 1000 * 1000 + ltv.tv_usec;

		mcast_igmp_querier_timer();
		
		/* OLT will remove group entry if it's timeout in ctc mode */
		if(IGMP_MODE_CTC != mcast_mcMode_get())
		{
			mcast_maintain_group_timer();
		}

        /*send a event notification packet to OLT when some bad conditions occur or clear*/
        ctc_oam_alarm_check();
        
        gettimeofday(&ltv, NULL);
        endTime = ltv.tv_sec * 1000 * 1000 + ltv.tv_usec;
		
        interval = endTime - startTime;        
        interval = PASS_SECONDS * 1000 * 1000 - interval;

        if (interval > 0)
            usleep(interval);  /* PASS_SECONDS Sec */
    }
}

extern int igmpmcinit(void);

int32 epon_igmp_init()
{
	pthread_t th1,th2;

	epon_igmp_db_init();
	mcast_ctc_db_init();
	mcast_querier_db_init();
	igmpmcinit();

	pthread_create(&th1, NULL, &epon_igmp_rxThread, NULL);
	pthread_create(&th2, NULL, &epon_igmp_timerThread, NULL);
	SYS_DBG("[OAM IGMP:%s:%d] init!\n", __FILE__, __LINE__);
	
}
