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
#include "ctc_vlan.h"

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

extern int32 drv_clf_translation_vlan_member_add(uint32 usrvlan,rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag);
extern int32 drv_clf_translation_vlan_member_remove(uint32 usrvlan, rtk_portmask_t stPhyMask);
extern int32 drv_mc_vlan_member_add(uint32 ulVlanId, rtk_portmask_t stPhyMask, rtk_portmask_t stPhyMaskUntag, int fidMode);
extern int32 drv_mc_vlan_member_remove(uint32 ulVlanId, rtk_portmask_t stPhyMask);
extern int32 drv_vlan_init(void);

#endif /* __RTK_DRV_SPECIAL_H__ */
