#ifndef __RTK_DRV_CLF_H__
#define __RTK_DRV_CLF_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <osal/print.h>
#include <rtk/acl.h>
#include <rtk/vlan.h>
#include <rtk/rate.h>
#include <rtk/switch.h>
#include <rtk/classify.h>
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>

#include <sys_def.h>
#include "ctc_wrapper.h"
#include "drv_acl.h"
#include "ctc_sfu.h"

#ifndef int32
#define int32 signed int
#endif
#ifndef int16
#define int16 signed short
#endif
#ifndef uint32
#define uint32 unsigned int
#endif
#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef ERROR
#define ERROR -1
#endif

#ifndef NO_ERROR
#define NO_ERROR 0
#endif


#define CLF_RULE_NUM_MAX 256

#define CLF_RULE_ID_IVALLID 0xFFFF

#define CLF_RMK_RULES_IN_1_PRECEDENCE_MAX			1

#define CLF_ACLHIT_RULE_START	0
#define CLF_ACLHIT_RULE_END		31
#define CLF_ETHTYPE_RULE_START	32
#define CLF_ETHTYPE_RULE_END	63
/*for 9601b 32 clf rules for multicast vlan 
  when unicast vlan mode of lan port 0 is transparent, this transparent clf rule will be matched
  by downstream multicast packet if member port of this multicast l2 entry is port 0.
  Set multicast vlan clf rules higher priority to avoid this problem */
#define CLF_MULTICAST_RULE_START 64
#define CLF_MULTICAST_RULE_END	 95
#ifdef CONFIG_VLAN_USE_CLASSF
/* siyuan 20170420:for 9607c, should add clf rules for vlans in unicast vlan setting in order 
   to permit downstream flood packets. FIXME:32 rules enough? */
#define CLF_GENERIC_RULE_START	96
#define CLF_GENERIC_RULE_END	220
#define CLF_VLAN_PERMIT_RULE_START	221
#define CLF_VLAN_PERMIT_RULE_END	252
#else
#define CLF_GENERIC_RULE_START	96
#define CLF_GENERIC_RULE_END	252
#endif
#define CLF_RESERVED_RULE_START	253
#define CLF_RESERVED_RULE_END	255

typedef union acl_clf_rmk_val_en_u
{
    unsigned char aucMacAddr[MAC_ADDR_LEN];
    unsigned long ulValue;
	unsigned char in6Addr[16];
} acl_clf_rmk_val_en_t;

typedef struct port_clf_pri_to_queue_cfg_s
{
    uint32           uiClfRmkRuleNum;
    uint32           auiClfList[CTC_CLF_REMARK_RULE_NUM_MAX];
} port_clf_pri_to_queue_cfg_t;

extern int32 drv_clf_cfg_get(uint32 uiClfId, void *pstClfCfg);
extern int32 drv_clf_cfg_set(uint32 uiClfId, void *pstClfCfg);
extern uint32 drv_clf_resouce_set(uint32 uiClfId, uint32 port, uint32 usage);
extern uint32 drv_clf_resouce_get(uint32 uiClfId, uint32 *port, uint32 *usage);
extern int32 drv_clf_cfg_rule_clear(uint32 uiClfRuleId);
extern int32 drv_clf_rule_emptyid_get(uint32 *puiClfRuleId, uint32 type);
extern int32 drv_clf_rule_field_fill(rtk_classify_field_type_t enTrustMode, 
											void *pValue, 
											void *pClfTypeAndValue);
extern int32 drv_clf_rule_create(uint32 uiClfId, 
								rtk_classify_field_type_t uiClfRuleType, 
								void *pClfRuleTypeValue);
extern int32 drv_clf_rule_bind(uint32 uiClfId);
extern int32 drv_clf_rule_unbind(uint32 uiAclId);
#if 0
/*
 * vlan translation will use classfication rule, so qos should not 
 * use classification rule anymore
 */
extern int32 drv_cfg_port_clfpri2queue_num_Inc(uint32 uiLPortId);
extern int32 drv_cfg_port_clfpri2queue_num_dec(uint32 uiLPortId);
extern int32 drv_cfg_port_clfpri2queue_ruleid_get(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 *puiClfId);
extern int32 drv_cfg_port_clfpri2queue_ruleid_set(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 uiClfId);
#endif

#ifdef CONFIG_VLAN_USE_CLASSF
extern int32 drv_clf_rule_for_vlan_permit_create(uint32 port, uint32 svlanId, uint32 isTag);
extern int32 drv_clf_rule_for_vlan_permit_reset(uint32 port);
extern int32 drv_clf_rule_for_mcast_vlan_create(uint32 lport, uint32 svlanId, uint32 isTag);
extern int32 drv_clf_rule_for_mcast_vlan_reset(uint32 lport, unsigned char vlanMode);
extern int32 drv_clf_rule_for_mcastvlan_add_port(unsigned int lport, unsigned char vlanMode);
#endif

extern int32 drv_clf_rule_for_ctc_def_drop_create(void);

extern int32 drv_clf_init(void);
#endif
