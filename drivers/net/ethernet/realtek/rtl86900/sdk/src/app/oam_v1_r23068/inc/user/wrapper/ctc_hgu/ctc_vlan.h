#ifndef __CTC_ACL_FOR_VLAN_H__
#define __CTC_ACL_FOR_VLAN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <osal/print.h>

#include "ctc_wrapper.h"
#include "rtk_rg_struct.h"

#define ACL_RULE_NUM_MAX MAX_ACL_SW_ENTRY_SIZE

typedef struct ctc_wrapper_hguAclCfg_s {
    unsigned char valid:1;
    unsigned char port:7;
    unsigned short usage; /* Use by which function */

	rtk_rg_aclFilterAndQos_t aclCfg;
} ctc_wrapper_hguAclCfg_t;

#if 0
extern int32 ctc_hgu_acl_rule_for_vlanTransparent_create(unsigned int lport);
extern int32 ctc_hgu_acl_rule_for_vlanTag_create(unsigned int lport, 
											ctc_wrapper_vlan_t *pTagCfg);
extern int32 ctc_hgu_acl_rule_for_vlanTranslation_create(unsigned int lport, 
											ctc_wrapper_vlanTransCfg_t *pTransCfg);
extern int32 ctc_hgu_acl_rule_for_vlanAggregation_create(unsigned int lport, 
											ctc_wrapper_vlanAggreCfg_t *pAggrCfg);
extern int32 ctc_hgu_acl_rule_for_vlanTrunk_create(unsigned int lport, 
											ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg);
#endif
extern int32 ctc_hgu_clf_rule_for_mcastvlan_clear(unsigned int lport);

extern int32 ctc_hgu_acl_rule_for_McastTransparent_create(unsigned int lport, uint32 uiMcVid);

extern int32 ctc_hgu_acl_rule_for_McastStrip_create(unsigned int lport, uint32 mcVid);

extern int32 ctc_hgu_acl_rule_for_McastTranslation_create(unsigned int lport, 
											uint32 mcVid, uint32 userVid);
#endif
