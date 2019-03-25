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

#ifndef __EPON_OAM_IGMP_H__
#define __EPON_OAM_IGMP_H__

#include <common/rt_type.h>
#include "epon_oam_igmp_util.h"
#include <osal/lib.h>
#include <hal/common/halctrl.h>

/* if vid of the igmp packet doesn't belong to multicast vlan, we also check if the vid belongs to the unicast vlan setting of the port */
//#define IGMP_CHECK_UNICAST_VID

#define IPV4_ETHER_TYPE  0x0800
#define IPV6_ETHER_TYPE  0x86DD
#define PPPOE_ETHER_TYPE 0x8864

#define IPV6_HEADER_LENGTH 40

#define IGMP_PROTOCOL     2
#define ICMPV6_PROTOCOL    58
#define TCP_PROTOCOL 	  6
#define UDP_PROTOCOL 	 17

#define IGMP_QUERY_V2     2
#define IGMP_QUERY_V3     3

typedef enum igmp_type_e 
{
    IGMP_TYPE_MEMBERSHIP_QUERY     = 0x11,  /* Source Quench                */
    IGMPV1_TYPE_MEMBERSHIP_REPORT  = 0x12,  /* Host Membership Report       */
    IGMPV2_TYPE_MEMBERSHIP_REPORT  = 0x16,  /* Redirect (change route)      */
    IGMPV2_TYPE_MEMBERSHIP_LEAVE   = 0x17,  /*                              */
    IGMPV3_TYPE_MEMBERSHIP_REPORT  = 0x22   /*                              */
} igmp_type_t;

#define MLD_QUERY_V1                           1
#define MLD_QUERY_V2                           2

#define MLD_TYPE_MEMBERSHIP_QUERY      130
#define MLD_TYPE_MEMBERSHIP_REPORT     131  
#define MLD_TYPE_MEMBERSHIP_DONE       132
#define MLD_ROUTER_SOLICITATION        133
#define MLD_ROUTER_ADVERTISEMENT       134
#define MLD_NEIGHBOR_SOLICITATION      135
#define MLD_NEIGHBOR_ADVERTISEMENT     136
#define MLD_REDIRECT                   137
#define MLDV2_TYPE_MEMBERSHIP_REPORT   143 

#define HOP_BY_HOP_OPTIONS_HEADER 	 0
#define ROUTING_HEADER 				43
#define FRAGMENT_HEADER 			44
#define DESTINATION_OPTION_HEADER 	60
#define NO_NEXT_HEADER 				59

typedef struct packet_info_s
{
	uint8  *head;       /* pointer to the head of the packet data buffer */
    uint8  *data;       /* pointer to the base address of the packet */
    uint8  *tail;       /* pointer to the end address of the packet */
    uint8  *end;        /* pointer to the end of the packet data buffer */
	uint32 	length;      /* packet length when the packet is valid (not a empty data buffer) */

	struct
	{
		uint8   source_port:6;      /* Source port number, use physical port: start from 0 */
		uint8   svid_tagged:1;      /* Whether the SVID is tagged */
		uint8   cvid_tagged:1;      /* Whether the CVID is tagged (ethertype==0x8100) */
		uint16  outer_vid:12;       /* vid of outer tag */
		uint16  inner_vid:12;       /* vid of inner tag */
    } rx_tag;

	uint32  ipVersion;
	uint32 	sip[4];

	uint8  	groupMac[MAC_ADDR_LEN];
	uint32 	groupAddress[4];

	uint8 	*ipBuf;
	uint16  ipHdrLen;
	uint8   l3Protocol;
	uint8 	*l3PktBuf;
	uint16 	l3PktLen;
	uint16 	macFrameLen;
	uint8   forced;	/* bypass any firewall, just xmit it */
	uint8   reservedAddr;
	uint16  vid;
}packet_info_t;

typedef struct igmpv3_record_s
{
	uint8	type;				/* Record Type */
	uint8	auxLen;			    /* auxiliary data length, in uints of 32 bit words*/
	uint16	numOfSrc;			/* number of sources */
	uint32	groupAddr;			/* group address being reported */
	uint32	srcList[0];			/* first entry of source list */	
} igmpv3_record_t;

typedef struct igmpv3_report_s
{
	uint8	type;				/* Report Type */
	uint8  reserved1;             
	uint16 checkSum;			/*IGMP checksum*/
	uint16 reserved2;
	uint16	numOfRecords;		/* number of Group records */
	igmpv3_record_t recordList[0];  /*first entry of group record list*/
} igmpv3_report_t;

typedef struct mldv1_hdr_s
{
	uint8 type;                                   
	uint8 code;						/*initialize by sender, ignored by receiver*/
	uint16 checkSum;
	uint16 maxResDelay;                       /*maximum response delay,unit:0.001 second*/ 
	uint16 reserved;
	uint32 mCastAddr[4];                      /*ipv6 multicast address*/  
} mldv1_hdr_t;

typedef struct mldv2_qryhdr_s
{
    uint8  type;                    /* type*/
    uint8  code;            
    uint16 checksum;
    uint16 responseDelay;
    uint16 reserved;
    uint32 mCastAddr[4];            /*ipv6 multicast address*/  

    uint8 rsq;                      /* 4bit: reserved, 1bit: suppress router-side processing, 3bit: querier's robustness variable*/
    uint8 qqic;                     /* querier's query interval code */
    uint16 numOfSrc;                /* number of sources */
    uint32 srcList[0];              /* first entry of source list */    
} mldv2_qryhdr_t;

typedef struct mldv2_record_s
{
	uint8	type;				/* Record Type */
	uint8	auxLen;			    /* auxiliary data length, in uints of 32 bit words*/
	uint16	numOfSrc;			/* number of sources */
	uint32	mCastAddr[4];		/* group address being reported */
	uint32	srcList[0];			/* first entry of source list */
} mldv2_record_t;

typedef struct mldv2_report_s
{
	uint8 type;
	uint8  reserved1;                                  
	uint16 checkSum;                           
	uint16 reserved2;                    
	uint16 numOfRecords;		/* number of multicast address records */
	mldv2_record_t recordList[0];
} mldv2_report_t;

//statistic start
#define VALID_OTHER_PACKET                      0
#define SUPPORTED_IGMP_CONTROL_PACKET           1
#define UNSUPPORTED_IGMP_CONTROL_PACKET         2
#define ERRONEOUS_PACKET                        3
#define MULTICAST_DATA_PACKET                   5
#define ROUTER_ROUTING_PACKET                   6

typedef struct igmp_stats_s
{   
    uint32  total_pkt_rcvd;
    uint32  valid_pkt_rcvd;
    uint32  invalid_pkt_rcvd;

    uint32  g_query_rcvd;
    uint32  gs_query_rcvd;
    uint32  leave_rcvd;
    uint32  report_rcvd;
    uint32  other_rcvd;

    uint32  g_query_xmit;
    uint32  gs_query_xmit;
    uint32  leave_xmit;
    uint32  report_xmit;

    struct
    {
        uint32  g_queryV3_rcvd;
        uint32  gs_queryV3_rcvd;
        uint32  gss_queryV3_rcvd;
        uint32  isIn_rcvd;
        uint32  isEx_rcvd;
        uint32  toIn_rcvd;
        uint32  toEx_rcvd;
        uint32  allow_rcvd;
        uint32  block_rcvd;
        uint32  g_queryV3_xmit;      /*querier tx packet*/
        uint32  gs_queryV3_xmit;
        uint32  gss_queryV3_xmit;         /*igmpv3 specific group and source  tx packet*/
    }v3;   
} igmp_stats_t;
//statistic end

extern igmp_stats_t  igmp_stats;
int mcast_vlan_querier_add(int vid);
int mcast_vlan_querier_del(int vid);

int32 epon_igmp_init();
#endif
