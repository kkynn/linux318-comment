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

#ifndef __RTK_DRV_SPECIAL_H__
#define __RTK_DRV_SPECIAL_H__

#include <common/rt_type.h>
#include <rtk/vlan.h>
#include <rtk/svlan.h>

#include "ctc_wrapper.h"

extern int32 ctc_transparent_vlan_set(uint32 lport);
extern int32 ctc_translation_vlan_set(uint32 lport, ctc_wrapper_vlanTransCfg_t *pTransCfg);
extern int32 ctc_tag_vlan_set(uint32 lport, ctc_wrapper_vlan_t *pTagCfg);
extern int32 ctc_aggregation_vlan_set(uint32 lport, ctc_wrapper_vlanAggreTableCfg_t *pAggreCfg);
extern int32 ctc_trunk_vlan_set(uint32 lport, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg);
extern void ctc_port_vlan_clf_init();


#endif /* __RTK_DRV_SPECIAL_H__ */
