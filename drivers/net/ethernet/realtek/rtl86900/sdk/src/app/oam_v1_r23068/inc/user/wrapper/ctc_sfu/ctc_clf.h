#ifndef __CTC_CLF_H__
#define __CTC_CLF_H__

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

#include "ctc_wrapper.h"
#include "drv_acl.h"
#include "drv_clf.h"
#include "ctc_sfu.h"


typedef enum oam_ctc_clf_field_type_e
{
    OAM_CTC_CLS_FIELD_TYPE_DA_MAC=0x00,
    OAM_CTC_CLS_FIELD_TYPE_SA_MAC=0x01,
    OAM_CTC_CLS_FIELD_TYPE_ETH_PRI=0x02,
    OAM_CTC_CLS_FIELD_TYPE_VLAN_ID=0x03,
    OAM_CTC_CLS_FIELD_TYPE_ETHER_TYPE=0x04,
    OAM_CTC_CLS_FIELD_TYPE_DST_IP=0x05,
    OAM_CTC_CLS_FIELD_TYPE_SRC_IP=0x06,
    OAM_CTC_CLS_FIELD_TYPE_IP_TYPE=0x07,
    OAM_CTC_CLS_FIELD_TYPE_IP_DSCP=0x08,
    OAM_CTC_CLS_FIELD_TYPE_IP_PRECEDENCE=0x09,
    OAM_CTC_CLS_FIELD_TYPE_L4_SRC_PORT=0x0A,
    OAM_CTC_CLS_FIELD_TYPE_L4_DST_PORT=0x0B,
    /* below function is not supported, because template is too less to support so many types */
    OAM_CTC_CLS_FIELD_TYPE_IP_VERSION=0x0C,
    OAM_CTC_CLS_FIELD_TYPE_IPV6_FLOW_LABEL=0x0D,
    OAM_CTC_CLS_FIELD_TYPE_DST_IPV6=0x0E,
    OAM_CTC_CLS_FIELD_TYPE_SRC_IPV6=0x0F,
    OAM_CTC_CLS_FIELD_TYPE_DST_PREFIX=0x10,
    OAM_CTC_CLS_FIELD_TYPE_SRC_PREFIX=0x11,
    OAM_CTC_CLS_FIELD_TYPE_NXTHDR=0x12
}oam_ctc_clf_field_type_t;

typedef struct oam_clasmark_rulebody {
    unsigned char    precedenceOfRule;
    unsigned char    lenOfRule;
    unsigned char    queueMapped;
    unsigned char    ethPriMark;
    unsigned char    numOfField;
}__attribute__((packed)) oam_clasmark_rulebody_t;

typedef struct oam_clasmark_fieldbody {
	unsigned char	fieldSelect;
	unsigned char	matchValue[16];
	unsigned char	operator;
}__attribute__((packed)) oam_clasmark_fieldbody_t;

typedef struct oam_clasmark_local {
    oam_clasmark_rulebody_t ClsRule[8];
	oam_clasmark_fieldbody_t ClsField[8][CLF_RMK_RULES_IN_1_PRECEDENCE_MAX];
}__attribute__((packed)) oam_clasmark_local_t;

typedef struct ctc_sfu_cfDb_s {
    unsigned char valid:1;
    unsigned char port:7;
    unsigned char type;
    unsigned short usage; /* Use by which function */
#ifdef CONFIG_VLAN_USE_CLASSF
	unsigned short vlan;  /* use to check whether a vlan permit rule is added or not */
#endif
	rtk_classify_cfg_t clfCfg;
} ctc_sfu_cfDb_t;

extern uint8 SYSTEM_ADDR[6];

extern void ctc_clf_init(void);
extern int32 ctc_del_clfrmk_by_prec(uint32 lport, uint32 precedenceOfRule);
extern int32 ctc_clear_clfrmk_by_port(uint32 lport);
extern int32 ctc_add_clfrmk_by_prec(uint32 lport,
	                         uint32  precedenceOfRule,
	                         oam_clasmark_rulebody_t *pstClsRule,
	                         oam_clasmark_fieldbody_t *pstClsField);
extern int32 ctc_clf_rule_for_vlanTransparent_create(unsigned int lport);
extern int32 ctc_clf_rule_for_vlanTag_create(unsigned int lport, 
											ctc_wrapper_vlan_t *pTagCfg);
extern int32 ctc_clf_rule_for_vlanTranslation_create(unsigned int lport, 
											ctc_wrapper_vlanTransCfg_t *pTransCfg);
extern int32 ctc_clf_rule_for_vlanAggregation_create(unsigned int lport, 
											ctc_wrapper_vlanAggreCfg_t *pAggrCfg);
extern int32 ctc_clf_rule_for_vlanTrunk_create(unsigned int lport, 
											ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg);
extern int32 ctc_clf_rule_for_McastTranslation_create(unsigned int lport, 
											uint32 mcVid, uint32 userVid);

#endif
