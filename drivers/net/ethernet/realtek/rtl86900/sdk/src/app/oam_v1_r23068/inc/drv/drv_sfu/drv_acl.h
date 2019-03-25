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

#ifndef __RTK_DRV_ACL_H__
#define __RTK_DRV_ACL_H__

#include <common/rt_type.h>

#define ACL_RULE_NUM_MAX 64

#define ACL_RULE_ID_IVALLID 0xFFFF

#define CTC_CLF_REMARK_RULE_NUM_MAX 8

typedef enum acl_trust_mode_e
{
    ACL_TRUST_PORT = 1,
    ACL_TRUST_SMAC,
    ACL_TRUST_DMAC,
    ACL_TRUST_CTAG_PRIO,
    ACL_TRUST_ETHTYPE,
    ACL_TRUST_CTAG_VID,
    ACL_TRUST_IPV4_SIP,
    ACL_TRUST_IPV4_DIP,
    ACL_TRUST_IPV4_PRENCEDENCE,
    ACL_TRUST_IPV4_TOS,
    ACL_TRUST_IPV4_PROTOCOL,
    ACL_TRUST_TCP_SPORT,
    ACL_TRUST_TCP_DPORT,
    ACL_TRUST_UDP_SPORT,
    ACL_TRUST_UDP_DPORT,
    ACL_TRUST_STAG_VID,
    ACL_TRUST_IP_VERSION,
    ACL_TRUST_END
}acl_trust_mode_t;

typedef enum acl_action_e
{
    ACL_ACTION_REMATK_PRIORITY = 0,
	ACL_ACTION_PRIORITY_ASSIGN,
    ACL_ACTION_MIRROR,
    ACL_ACTION_CVLAN_REMARK,
    ACL_ACTION_CVLAN_ASSIGN,
    ACL_ACTION_CVLAN_1P_REMARK,
    ACL_ACTION_SVLAN_REMARK,
    ACL_ACTION_COPY_TO_PORTS,
    ACL_ACTION_REDIRECT_TO_PORTS,
    ACL_ACTION_COPY_TO_CPU,
    ACL_ACTION_TRAP_TO_CPU,
    ACL_ACTION_POLICING_0,
    ACL_ACTION_DROP,
    ACL_ACTION_END
}acl_action_t;

typedef struct port_clf_remark_cfg_s
{
    uint32           uiClfRmkRuleNum;
    uint32           auiAclList[CTC_CLF_REMARK_RULE_NUM_MAX];
    acl_trust_mode_t aenClfRemarkMode[CTC_CLF_REMARK_RULE_NUM_MAX];
} port_clf_remark_cfg_t;

extern int32 drv_cfg_port_clfrmk_acl_ruleId_get(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 *puiAclId);
extern int32 drv_acl_rule_for_ctc_clfrmk_create(uint32 uiLPortId, 
                                   uint32 uiRulePrecedence,
                                   acl_trust_mode_t uiAclRuleType,
                                   void *pRuleValue,
                                   void *pAclPri, 
                                   void *pRemarkPri);
extern int32 drv_acl_rule_for_ctc_clfrmk_delete(uint32 uiLPortId, uint32 uiRulePrecedence);
extern int32 drv_acl_rule_for_ctc_clfrmk_clear(uint32 uiLPortId);
extern int32 drv_acl_rule_for_drop_vid_create(uint32 uiLPortId, uint32 uiVid, uint32 *puiAclId);
extern int32 drv_acl_rule_delete_by_aclid(uint32 uiAclRuleId);
extern int32 drv_acl_rule_init(void);
#ifdef CONFIG_KERNEL_RTK_ONUCOMM
extern int32 drv_acl_rule_special_packet_trp2cpu();
#endif

#endif /* __RTK_DRV_CONVERT_H__ */

