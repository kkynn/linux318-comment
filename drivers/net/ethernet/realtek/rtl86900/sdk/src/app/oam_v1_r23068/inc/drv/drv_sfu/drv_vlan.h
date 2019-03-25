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

#ifndef __RTK_DRV_VLAN_H__
#define __RTK_DRV_VLAN_H__

#include <common/rt_type.h>
#include <rtk/vlan.h>
#include <rtk/svlan.h>

#include "drv_acl.h"

#define VLAN_TRANSLATION_ENTRY (16*MAX_PORT_NUM)
#define MAX_VLAN_NUMBER 4096
#define FID_MAX_VLAN_NUM (MAX_VLAN_NUMBER - 1)

typedef enum ctc_vlan_mode_e
{
	CTC_VLAN_MODE_TRANSPARENT       = 0x00,
	CTC_VLAN_MODE_TAG               = 0x01,
	CTC_VLAN_MODE_TRANSLATION       = 0x02,
	CTC_VLAN_MODE_AGGREGATION       = 0x03,
	CTC_VLAN_MODE_TRUNK             = 0x04,
	CTC_VLAN_MODE_END
} ctc_vlan_mode_t;

typedef struct ctc_vlan_cfg_s
{
    uint32          number_of_acl_rules;
    uint32          acl_list[ACL_RULE_NUM_MAX];
} ctc_vlan_cfg_t;

extern int32 drv_vlan_set_fidMode(uint32 ulVlanId, int fidMode);
extern int32 drv_vlan_entry_create(uint32 ulVlanId, int fidMode);
extern int32 drv_vlan_entry_delete(uint32 ulVlanId);
extern int32 drv_vlan_check_member_empty(uint32 ulVlanId, int32 *isNotEmpty);
extern int32 drv_vlan_member_add(uint32 ulVlanId, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag);
extern int32 drv_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask);
extern int32 drv_svlan_member_add(uint32 ulSvlanId, uint32 ulSvlanPri, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, uint32 ulSvlanFid);
extern int32 drv_svlan_member_remove(uint32 ulSvlanId, rtk_portmask_t stPhyMask);
extern int32 drv_svlan_port_svid_set(uint32 uiLPort, uint32 ulSvlanId);
extern int32 drv_mc_translation_vlan_member_add(uint32 usrvlan, uint32 mvlan, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, 
														rtk_portmask_t stPhyMasksvlanUntag, int fidMode);
extern int32 drv_mc_translation_vlan_member_remove(uint32 usrvlan,uint32 mvlan, rtk_portmask_t stPhyMask);
extern int32 drv_clf_translation_vlan_member_add(uint32 usrvlan,rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag);
extern int32 drv_clf_translation_vlan_member_remove(uint32 usrvlan, rtk_portmask_t stPhyMask);
extern int32 drv_mc_vlan_member_add(uint32 ulVlanId, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, int fidMode);
extern int32 drv_mc_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask);
extern int32 drv_vlan_port_pvid_set(uint32 ulLgcPortNumber, uint32 ulPvid, uint32 priority);
extern int32 drv_vlan_port_ingressfilter_set(uint32 uiLPort, int32 bEnable);
extern int32 drv_vlan_egress_keeptag_ingress_enable_set(uint32 uiEgLPort, uint32 uiIngLPort);
extern int32 drv_vlan_egress_keeptag_ingress_disable_set(uint32 uiEgLPort, uint32 uiIngLPort);
extern int32 drv_port_c2s_entry_add(uint32 uiLPort, uint32 ulCvid, uint32 ulSvid);
extern int32 drv_port_c2s_entry_delete(uint32 uiLPort, uint32 ulCvid, uint32 ulSvid);
extern int32 drv_port_sp2c_entry_add(uint32 uiLPort, uint32 ulSvid, uint32 ulCvid);
extern int32 drv_port_sp2c_entry_delete(uint32 uiLPort, uint32 ulSvid);
extern int32 drv_cfg_port_vlan_ruleid_set(uint32 uiLPortId, ctc_vlan_cfg_t *pstVlanCfg);
extern int32 drv_cfg_port_vlan_ruleid_get(uint32 uiLPortId, ctc_vlan_cfg_t *pstVlanCfg);
extern int32 drv_vlan_init(void);


#endif /* __RTK_DRV_SPECIAL_H__ */
