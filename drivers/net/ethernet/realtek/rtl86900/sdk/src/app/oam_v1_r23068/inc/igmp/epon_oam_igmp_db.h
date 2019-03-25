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

#ifndef __EPON_OAM_IGMP_DB_H__
#define __EPON_OAM_IGMP_DB_H__

#include "epon_oam_igmp_util.h"
#include "ctc_wrapper.h"

#ifdef CONFIG_IPV6
#define SUPPORT_MLD_SNOOPING 1
#endif

#define PASS_SECONDS    1

typedef enum igmp_version_e{
	IGMP_VERSION_V1 = 1,
	IGMP_VERSION_V2,
	IGMP_VERSION_V3,
	MLD_VERSION_V1,
	MLD_VERSION_V2
}igmp_version_t;

typedef enum multicast_ipType_e
{
    MULTICAST_TYPE_IPV4,
    MULTICAST_TYPE_IPV6,
    MULTICAST_TYPE_END
} multicast_ipType_t;

#ifdef SUPPORT_MLD_SNOOPING
#define MCAST_MAX_GROUP_NUM		512
#define MCAST_GROUP_HASH_NUM 	256
#else
#define MCAST_MAX_GROUP_NUM		256
#define MCAST_GROUP_HASH_NUM 	256
#endif

#define MCAST_MAX_PORT_ENTRY_NUM   MCAST_MAX_GROUP_NUM
#define MCAST_MAX_SOURCE_ENTRY_NUM 300

typedef enum igmpv3_mode_e
{
	MODE_IS_INCLUDE	= 1,
	MODE_IS_EXCLUDE	= 2,
	CHANGE_TO_INCLUDE_MODE = 3,
	CHANGE_TO_EXCLUDE_MODE = 4,
	ALLOW_NEW_SOURCES = 5,
	BLOCK_OLD_SOURCES = 6,
	IGMPV3_MODE_END
} igmpv3_mode_t;

typedef struct igmp_source_entry_s
{
	struct igmp_source_entry_s *previous; 
	struct igmp_source_entry_s *next;      /*Pointer of next source entry*/
#ifdef SUPPORT_MLD_SNOOPING
	uint32 sourceAddr[4];                  /*D class IP multicast address*/
#else
	uint32 sourceAddr[1];                  /*D class IP multicast address*/
#endif
	uint32 fwdState;
	uint32 portTimer;
	struct igmp_source_entry_s *qpre;
	struct igmp_source_entry_s *qnxt;
}igmp_source_entry_t;

typedef struct igmp_port_entry_s
{
	struct igmp_port_entry_s *previous;
	struct igmp_port_entry_s *next;
	uint8  portNum;
	igmp_version_t igmpVersion;
	uint8  fmode;  							/*0: include,   1:exclude*/
	uint32 groupMbrTimer; 	/*reset by the join, report, or specific query packet */
	uint8  groupSpecQueryCnt;
	uint8  groupSrcSpecQueryCnt;
	igmp_source_entry_t *sourceList;	   /*this is the server source ip list*/
	igmp_source_entry_t *querySrcList;	/*only for group and source specified query*/
}igmp_port_entry_t;

typedef struct igmp_group_entry_s
{
	struct igmp_group_entry_s *previous;
	struct igmp_group_entry_s *next;      /*Pointer of next group entry*/
	uint32 ipVersion;
	uint16 vid;
	uint8  mac[MAC_ADDR_LEN];  	/* group mac get from group ip address */
#ifdef SUPPORT_MLD_SNOOPING
	uint32 groupAddr[4];
#else
	uint32 groupAddr[1];
#endif
	igmp_port_entry_t * portEntry[MAX_PORT_NUM]; /* port index should start from 0 */
	uint32 fwdPort; /* active port map */
}igmp_group_entry_t;

typedef struct ipv4hdr_s 
{
#if defined(_LITTLE_ENDIAN)
    uint8   ihl:4,
            version:4;
#else
    uint8   version:4,
            ihl:4;
#endif            
	uint8	typeOfService;			
	uint16	length;			/* total length */
	uint16	identification;	/* identification */
	uint16	offset;			
	uint8	ttl;			/* time to live */
	uint8	protocol;			
	uint16	checksum;			
	uint32 	sip;
	uint32 	dip;
}ipv4hdr_t;

typedef struct ipv6hdr_s
{
	uint32  vtf;            /*version(4bits),  traffic class(8bits), and flow label(20bits)*/
	uint16	payloadLenth;	/* payload length */
	uint8	nextHeader;		/* next header */
	uint8	hopLimit;		/* hop limit*/
	uint32  sip[4];	    	/*source address*/
	uint32  dip[4];			/* destination address */
} ipv6hdr_t;

typedef struct igmp_hdr_s
{
    uint8  type;            /* type*/
    uint8  maxRespTime;     /*maximum response time,unit:0.1 seconds*/
    uint16 checksum;
    uint32 groupAddr;
    
    struct
    {
        uint8 rsq;          /* 4bit: reserved, 1bit: suppress router-side processing, 3bit: querier's robustness variable*/
        uint8 qqic;         /* querier's query interval code */
        uint16 numOfSrc;    /* number of sources */
        uint32 srcList[0];  /* first entry of source list */    
    }v3;
} igmp_hdr_t;

typedef struct igmp_config_e
{	
	uint8 	enableSnooping;
	uint8  	robustness; 			//Default: 2
	uint16  queryIntv;				//Default: 125 seconds
	uint16  queryRespIntv;			//Default: 10 seconds
	uint16  lastMemberQueryIntv; 	//Default: 1 second
	uint8  	lastMemberQueryCnt;  	//Default: the Robustness Variable
	uint16  groupMemberIntv;		//value must be (Robustness * queryIntv + queryRespIntv)
	uint32  otherQuerierPresentInterval;//queryIntv*robustness + queryRespIntv/2
	uint32  responseTime;			//Default: 10 seconds

	uint32 unknownMCastFloodMap;
} igmp_config_t;

typedef struct mcast_data_info_s
{
	uint32 ipVersion;
	uint16 vid;
	uint32 groupAddr[4];
	uint32 sip[4];
} mcast_data_info_t;

typedef struct mcast_fwd_info_s
{
	uint8 unknownMCast;
	uint8 reservedMCast;
	uint16 cpuFlag;
	uint32 fwdPortMask;	
} mcast_fwd_info_t;


int32 mcast_add_group_wrapper(uint32 port,
								uint32 ipVersion, uint16  vid, 
								uint8* mac, uint32 *groupAddr);

int32 mcast_del_group_wrapper(uint32 port,
								uint32 ipVersion, uint16  vid, 
								uint8* mac, uint32 *groupAddr);

int32 mcast_update_igmpv3_group_wrapper(uint32 port,
					uint32 ipVersion, uint16  vid, uint8* mac, uint32 *groupAddr,
					igmpv3_mode_t igmpv3_mode, uint32 * srcList, uint16 srcNum);

int32 mcast_group_portTime_update(uint32 ipVersion, uint16  vid, 
							uint8* mac, uint32 *groupAddr,uint32 maxRespTime);

int32 mcast_maintain_group_timer();
void mcast_fastpath_read();

uint8 isEnableIgmpSnooping();

int igmpMcCtrlGrpEntryGet(oam_mcast_control_entry_t *ctl_entry_list, int *num);
igmp_group_entry_t * mcast_search_group_entry(uint32 ipVersion, 
												uint16  vid, 
												uint32 *groupAddr);

igmp_config_t *getIgmpConfig(void);

void epon_igmp_group_entry_clear(uint32 ipVersion);
void epon_igmp_groupMbrPort_delFromVlan(uint32 ipVersion, uint16 vid, uint32 port);

void epon_igmp_db_init();
#endif
