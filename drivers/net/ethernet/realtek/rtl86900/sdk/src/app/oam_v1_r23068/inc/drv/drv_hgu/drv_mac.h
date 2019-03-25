/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: 
 * $Date: 
 *
 * Purpose :            
 *
 * Feature :           
 *
 */

#ifndef __RTK_DRV_MAC_H__
#define __RTK_DRV_MAC_H__

#include <common/rt_type.h>

#include "drv_convert.h"
#include "rtk_rg_define.h"
#include "rtk_rg_struct.h"


#define MAX_MULTICAST_ENTRY MAX_LUT_HW_TABLE_SIZE

typedef struct vlan_fid_s
{
    int32 bValid;
    uint32 uiVid;
    uint32 uiFid;
    rtk_portmask_t stPhyMask;
    rtk_portmask_t stPhyMaskUntag;
}vlan_fid_t;

typedef unsigned char mac_address_t[MAC_ADDR_LEN];

typedef struct
{
    mac_address_t mac_addr;
    uint16 tdVid;
    rtk_portmask_t port_mask;
	rtk_fidMode_t fidMode;
}mac_mcast_t;

typedef struct
{
    rtk_rg_l2MulticastFlow_t l2McFlow;
	uint8 valid;    
}mac_mcast_db_t;

typedef enum mac_trap_mgmt_type_e
{
    FRAME_TRAP_TYPE_RIP = 0,
    FRAME_TRAP_TYPE_ICMP,
    FRAME_TRAP_TYPE_ICMPV6,
    FRAME_TRAP_TYPE_ARP,
    FRAME_TRAP_TYPE_MLD,
    FRAME_TRAP_TYPE_IGMP,
    FRAME_TRAP_TYPE_BGP,
    FRAME_TRAP_TYPE_OSPFV2,
    FRAME_TRAP_TYPE_OSPFV3,
    FRAME_TRAP_TYPE_SNMP,
    FRAME_TRAP_TYPE_SSH,
    FRAME_TRAP_TYPE_FTP,
    FRAME_TRAP_TYPE_TFTP,
    FRAME_TRAP_TYPE_TELNET,
    FRAME_TRAP_TYPE_HTTP,
    FRAME_TRAP_TYPE_HTTPS, 
    FRAME_TRAP_TYPE_DHCPV6,
    FRAME_TRAP_TYPE_DHCP,
    FRAME_TRAP_TYPE_DOT1X,
    FRAME_TRAP_TYPE_BPDU,
    FRAME_TRAP_TYPE_END
} mac_trap_mgmt_type_t;

typedef enum mac_trap_action_e
{
    FRAME_TRAP_ACTION_FORWARD = 0,
    FRAME_TRAP_ACTION_DROP,
    FRAME_TRAP_ACTION_TRAP2CPU,
    FRAME_TRAP_ACTION_COPY2CPU,
    FRAME_TRAP_ACTION_TO_GUESTVLAN,
    FRAME_TRAP_ACTION_FLOOD_IN_VLAN,
    FRAME_TRAP_ACTION_FLOOD_IN_ALL_PORT,
    FRAME_TRAP_ACTION_FLOOD_IN_ROUTER_PORTS,
    FRAME_TRAP_ACTION_END
} mac_trap_action_t;

typedef struct mac_trap_set_s
{
    mac_trap_mgmt_type_t frameType;
    mac_trap_action_t trapAction;
}mac_trap_set_t;

typedef enum rsv_mac_action_e
{
    PASS_TYPE_CPU_ONLY = 0,
    PASS_TYPE_ALLPORTS,
    PASS_TYPE_CPU_EXCLUDE,
    PASS_TYPE_DROP,
    PASS_TYPE_END
} rsv_mac_pass_action_t;

typedef struct
{
    mac_address_t mac;
    uint16 vid;
    uint32 port;
    uint8 ucMacType;
    uint8 ucStatic;
    uint8 ucIsAggr;
    uint32 aggr_group;
    rtk_portmask_t port_mask;
    uint32 ulL2Idx;
}mac_common_t;

typedef enum mcast_unknow_act_e
{
    MCAST_ACT_FORWARD = 0,
    MCAST_ACT_DROP,
    MCAST_ACT_TRAP2CPU,
    MCAST_ACT_ROUTER_PORT,
    MCAST_ACT_END
} mcast_unknow_act_t;

typedef enum mac_learn_enable_e
{
    MAC_LEARN_FWD_DISABLED = 0,
    MAC_LEARN_FWD_ENABLED,
    MAC_LEARN_END
} mac_learn_enable_t;

typedef struct
{
	uint32 	vid;
    uint32  dip;
	uint32  dipv6[4];
	uint8 	isIPv6;	
    uint32  sip;
    rtk_portmask_t port_mask;
}ip_mcast_t;

typedef struct
{
	rtk_rg_multicastFlow_t ipMcastEntry;
    uint8 valid;
}ip_multicast_db_t;

#define FID_INVALID_ID     0xFFFF

extern int drv_mac_init(void);

extern void drv_flush_mc_vlan_mac_by_vid(int32 vid);

extern int32 drv_fid_set_by_vid(uint32 uiVid, uint32 uiFid, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag);

extern int32 drv_mc_vlan_mem_get(uint32 uiVid, rtk_portmask_t *pstPhyMask, rtk_portmask_t *pstPhyMaskUntag);

extern int32 drv_valid_fid_get(uint32 uiVid, uint32 *puiFid);

extern int32 drv_mac_mcast_set_by_mac_and_fid(mac_mcast_t stMacMcast);

extern int32 drv_mc_mac_delete_by_mac_and_fid(mac_mcast_t stMacMcast);

extern int32 drv_mac_mcast_get_by_mac_and_vid(mac_address_t tdMac, uint16 tdVid, rtk_rg_l2MulticastFlow_t *pl2McFlow);

extern int32 drv_ipMcastMode_set(uint32 mode);

extern int32 drv_ipMcastEntry_add(ip_mcast_t stIpMcast);

extern int32 drv_ipMcastEntry_del(ip_mcast_t stIpMcast);

extern int32 drv_ipMcastEntry_get(ip_mcast_t *pStIpMcast);
#endif /* __RTK_DRV_MAC_H__ */
