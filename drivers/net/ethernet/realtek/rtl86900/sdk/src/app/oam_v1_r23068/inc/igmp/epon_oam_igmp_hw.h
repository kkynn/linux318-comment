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

#ifndef __EPON_OAM_IGMP_HW_H__
#define __EPON_OAM_IGMP_HW_H__

#include <common/rt_type.h>
#include <rtk/rtusr/rtusr_pkt.h>

typedef struct hw_igmp_group_entry_s
{
	uint32  ipVersion;
	uint16  vid;
    uint8   mac[MAC_ADDR_LEN];
	uint32  groupAddr[4];
    uint64  sortKey;
    uint32 	fwdPortMask;
	struct  hw_igmp_group_entry_s *next_free;
} hw_igmp_group_entry_t;

typedef struct igmp_group_head_s
{
    uint32        freeListNumber;
    hw_igmp_group_entry_t  *item;
} igmp_group_head_t;

typedef struct igmp_aggregate_entry_s
{
    uint64                  sortKey;    /* vid + mac*/
    uint8                   port_ref_cnt[MAX_PORT_NUM];
    uint16                  group_ref_cnt;
} igmp_aggregate_entry_t;

typedef struct igmp_aggregate_db_s
{
    igmp_aggregate_entry_t  aggregate_entry[SYS_MCAST_MAX_DB_NUM];
    uint16                  entry_num;
} igmp_aggregate_db_t;

void mcast_ctc_db_init();
igmp_mode_t mcast_mcMode_get();
int mcast_mcFastLeave_get();
mc_control_e mcast_mcCtrl_type_get(uint32 ipVersion);

int32 mcast_snooping_tx_wrapper(packet_info_t * pkt, uint16 vid, uint32 length, sys_logic_portmask_t fwdPortMask);

void mcast_igmp_setting_show();
igmp_mc_vlan_entry_t* mcast_mcVlan_entry_get(int vlanId);

int32 mcast_hw_groupMbrPort_add_wrapper(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr);
int32 mcast_hw_groupMbrPort_del_wrapper(uint32 ipVersion, uint32 port, uint16  vid, uint32 * groupAddr);
int32 mcast_del_port_from_vlan(uint16 vid, uint32 port);


/* below functions used by oam ctc to update igmp settings*/
int mcast_db_add(uint32 portId, uint32 vlanId);
int mcast_db_del(uint32 portId, uint32 vlanId);
int mcast_mcVlan_clear(uint32 portId);

int mcast_tag_oper_set(uint32 portId, mcvlan_tag_oper_mode_t tagOper);
int mcast_translation_add(uint32 portId, uint32 mcVid, uint32 userVid);
int mcast_translation_clear(uint32 portId);

int mcast_mcSwitch_set(uint32 mode);
int mcast_mcFastLeave_set(uint32 leaveMode);

int mcast_mcCtrl_group_entry_clear(void);
int mcast_mcCtrl_type_set(mc_control_e type);
int mcast_mcCtrl_group_entry_add(uint32 portId, uint32 vlanId, uint8 *mac);
int mcast_group_entry_exist(uint32 portId, uint32 vlanId);
int mcast_mcCtrl_group_entry_delete(uint32 portId, uint32 vlanId, uint8 *mac);

int mcast_mcGrp_maxNum_set(uint32 portId, uint32 num);
int mcast_mcGrp_maxNum_get(uint32 portId);

int32 mcast_ctc_add_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr);

int32 mcast_ctc_del_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, 
										uint8* mac, uint32 *groupAddr);

int32 mcast_ctc_update_igmpv3_group_wrapper(uint32 port, uint32 ipVersion, uint16  vid, uint8* mac, 
													uint32 *groupAddr, igmpv3_mode_t igmpv3_mode, 
													uint32 * srcList, uint16 srcNum);

#endif

