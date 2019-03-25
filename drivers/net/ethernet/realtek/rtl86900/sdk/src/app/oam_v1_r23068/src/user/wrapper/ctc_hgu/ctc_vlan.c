/*
 * Martin ZHU: porting from ctc_clf.c
 */
#ifdef CONFIG_HGU_APP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <osal/print.h>
#include "ctc_wrapper.h"
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>

/* Chip specified include */
#include <dal/apollomp/dal_apollomp_switch.h>

/* EPON OAM include */
#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_db.h"
#include "epon_oam_dbg.h"
#include "epon_oam_rx.h"

/* User specific include */
#include "ctc_oam.h"
#include "ctc_oam_var.h"

/* Romedriver struct inclde */
#include "rtk_rg_struct.h"
#include "rtk_rg_define.h"

/* include CTC wrapper */
#include "ctc_hgu.h"
#include "ctc_vlan.h"
#include <sys_def.h>

static ctc_wrapper_vlanCfg_t m_VlanCfgDB[MAX_PORT_NUM];
static ctc_wrapper_hguAclCfg_t m_AclDB[ACL_RULE_NUM_MAX];

static uint32 upnpAclIdx = ACL_RULE_NUM_MAX;

void ctc_hgu_AclDB_init(void)
{
	memset(m_AclDB, 0, sizeof(m_AclDB));
}

void ctc_hgu_VlanCfgDB_init(void)
{
	memset(m_VlanCfgDB, 0, sizeof(m_VlanCfgDB));
}

int32 drv_hgu_acl_cfg_get(uint32 aclIdx, void *pstAclCfg)
{
    if ((ACL_RULE_NUM_MAX <= aclIdx) ||
        (NULL == pstAclCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(pstAclCfg, &m_AclDB[aclIdx].aclCfg, sizeof(rtk_rg_aclFilterAndQos_t));

    return RT_ERR_OK;
}

int32 drv_hgu_acl_cfg_set(uint32 aclIdx, void *pstAclCfg)
{
    if ((ACL_RULE_NUM_MAX <= aclIdx) ||
        (NULL == pstAclCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(&m_AclDB[aclIdx].aclCfg, pstAclCfg, sizeof(rtk_rg_aclFilterAndQos_t));

    return RT_ERR_OK;
}

int32 drv_hgu_acl_cfg_reset(uint32 aclIdx)
{
	memset(&m_AclDB[aclIdx], 0, sizeof(ctc_wrapper_hguAclCfg_t));
	return RT_ERR_OK;
}

uint32 drv_hgu_acl_resouce_get(uint32 aclIdx, uint32 *port, uint32 *usage)
{
	if (aclIdx >= ACL_RULE_NUM_MAX)
		return RT_ERR_INPUT;

	if ((NULL == port) ||
		(NULL == usage))
		return RT_ERR_INPUT;

	if (!m_AclDB[aclIdx].valid)
		return RT_ERR_FAILED;
	
	*port = m_AclDB[aclIdx].port;
	*usage = m_AclDB[aclIdx].usage;
	
	return RT_ERR_OK;
}

uint32 drv_hgu_acl_resouce_set(uint32 aclIdx, uint32 port, uint32 usage)
{
	if (aclIdx >= ACL_RULE_NUM_MAX)
		return RT_ERR_FAILED;
	
	m_AclDB[aclIdx].port = port;
	m_AclDB[aclIdx].usage = usage;
	m_AclDB[aclIdx].valid = 1;

	return RT_ERR_OK;
}

uint32 ctc_hgu_vlanCfg_get(uint32 lport, ctc_wrapper_vlanCfg_t **pVlanCfg)
{
	if (lport >= MAX_PORT_NUM) return RT_ERR_INPUT;

	*pVlanCfg = &m_VlanCfgDB[lport];

	return RT_ERR_OK;
}

uint32 ctc_hgu_vlanCfg_set(uint32 lport, int vlanMode, void *pCfg)
{
	ctc_wrapper_vlanTransCfg_t *pTransCfg;
	ctc_wrapper_vlanAggreCfg_t *pAggrCfg;
	ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg;
	ctc_wrapper_vlan_t *pTagCfg;
	int i, j;
	
	m_VlanCfgDB[lport].vlanMode = vlanMode;
	
	switch(vlanMode)
	{
	case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
		pTransCfg = (ctc_wrapper_vlanTransCfg_t *)pCfg;
		if (NULL == pTransCfg)
			return RT_ERR_FAILED;
		m_VlanCfgDB[lport].cfg.transCfg.defVlan = pTransCfg->defVlan;
		m_VlanCfgDB[lport].cfg.transCfg.num = pTransCfg->num;
		m_VlanCfgDB[lport].cfg.transCfg.transVlanPair = (ctc_wrapper_vlanTransPair_t *) malloc(sizeof(ctc_wrapper_vlanTransPair_t) * pTransCfg->num);
		if(NULL == m_VlanCfgDB[lport].cfg.transCfg.transVlanPair)
		{
			return RT_ERR_FAILED;
		}
		memcpy(m_VlanCfgDB[lport].cfg.transCfg.transVlanPair, pTransCfg->transVlanPair, sizeof(ctc_wrapper_vlanTransPair_t) * pTransCfg->num);
		break;
	case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
		pAggrCfg = (ctc_wrapper_vlanAggreCfg_t *)pCfg;
		if (NULL == pAggrCfg)
			return RT_ERR_FAILED;
		m_VlanCfgDB[lport].cfg.aggreCfg.defVlan = pAggrCfg->defVlan;
		m_VlanCfgDB[lport].cfg.aggreCfg.tableNum = pAggrCfg->tableNum;
		m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl = (ctc_wrapper_vlanAggreTableCfg_t *) malloc(sizeof(ctc_wrapper_vlanAggreTableCfg_t) * pAggrCfg->tableNum);
		if(NULL == m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl)
		{
			return RT_ERR_FAILED;
		}
		memcpy(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl, pAggrCfg->aggrTbl, sizeof(ctc_wrapper_vlanAggreTableCfg_t) * pAggrCfg->tableNum);
		for(i = 0 ; i < pAggrCfg->tableNum ; i++)
		{
			m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].entryNum = pAggrCfg->aggrTbl[i].entryNum;
			m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreToVlan = pAggrCfg->aggrTbl[i].aggreToVlan;
			m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreFromVlan = (ctc_wrapper_vlan_t *) malloc(sizeof(ctc_wrapper_vlan_t) * pAggrCfg->aggrTbl[i].entryNum);
			if(NULL == m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreFromVlan)
			{
				for(j = 0 ; j < i; j++)
				{
					free(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[j].aggreFromVlan);
				}
				free(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl);
				return RT_ERR_FAILED;
			}
			memcpy(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreFromVlan, pAggrCfg->aggrTbl[i].aggreFromVlan, sizeof(ctc_wrapper_vlan_t) * pAggrCfg->aggrTbl[i].entryNum);
		}
		break;
	case CTC_OAM_VAR_VLAN_MODE_TRUNK:
		pTrunkCfg = (ctc_wrapper_vlanTrunkCfg_t *)pCfg;
		if (NULL == pTrunkCfg)
			return RT_ERR_FAILED;
		m_VlanCfgDB[lport].cfg.trunkCfg.defVlan = pTrunkCfg->defVlan;
		m_VlanCfgDB[lport].cfg.trunkCfg.num = pTrunkCfg->num;
		m_VlanCfgDB[lport].cfg.trunkCfg.acceptVlan = (ctc_wrapper_vlan_t *) malloc(sizeof(ctc_wrapper_vlan_t) * pTrunkCfg->num);
		if(NULL == m_VlanCfgDB[lport].cfg.trunkCfg.acceptVlan)
		{
			return RT_ERR_FAILED;
		}
		memcpy(m_VlanCfgDB[lport].cfg.trunkCfg.acceptVlan, pTrunkCfg->acceptVlan, sizeof(ctc_wrapper_vlan_t) * pTrunkCfg->num);
		break;
	case CTC_OAM_VAR_VLAN_MODE_TAG:
		pTagCfg = (ctc_wrapper_vlan_t *)pCfg;
		if (NULL == pTagCfg)
			return RT_ERR_FAILED;
		m_VlanCfgDB[lport].cfg.tagCfg = *pTagCfg;
		break;
	case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
	default:
		break;
	}

	return RT_ERR_OK;
}

uint32 ctc_hgu_vlanCfg_reset(uint32 lport)
{
	int i;
	
	switch(m_VlanCfgDB[lport].vlanMode)
	{
	case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
		if(NULL != m_VlanCfgDB[lport].cfg.transCfg.transVlanPair)
		{
			free(m_VlanCfgDB[lport].cfg.transCfg.transVlanPair);
		}
		break;
	case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
		if(NULL != m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl)
		{
			for(i = 0 ; i < m_VlanCfgDB[lport].cfg.aggreCfg.tableNum ; i ++)
			{
				if(NULL != m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreFromVlan)
				{
					free(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl[i].aggreFromVlan);
				}
			}
			free(m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl);
		}
		break;
	case CTC_OAM_VAR_VLAN_MODE_TRUNK:
		if(NULL != m_VlanCfgDB[lport].cfg.trunkCfg.acceptVlan)
		{
			free(m_VlanCfgDB[lport].cfg.trunkCfg.acceptVlan);
		}
		break;
	case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
	case CTC_OAM_VAR_VLAN_MODE_TAG:
	default:
		break;
	}
	memset(&m_VlanCfgDB[lport], 0, sizeof(ctc_wrapper_vlanCfg_t));

	return RT_ERR_OK;
}

#if 0
int32 ctc_hgu_acl_rule_for_vlan_reset(unsigned int lport)
{
	uint32 ret;
	uint32 aclIdx=0;
	uint32 lportTmp, usage;
	int32  i;

	for (i=0; i<ACL_RULE_NUM_MAX; i++)
	{
		usage = CTC_HGU_CFUSAGE_RESERVED;
		
		drv_hgu_acl_resouce_get(i, &lportTmp, &usage);
		if ((lportTmp != lport) || (CTC_HGU_CFUSAGE_VLAN != usage))
			continue;

		aclIdx = i;
        ret = rtk_rg_aclFilterAndQos_del(aclIdx);
        if (RT_ERR_RG_OK != ret)
        {
            return RT_ERR_FAILED;
        }
		drv_hgu_acl_cfg_reset(aclIdx);
	}
	
	ctc_hgu_vlanCfg_reset(lport);
	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_vlanTransparent_create(unsigned int lport)
{
	int32 ret=0;
	uint32 aclIdx=0;
	uint32 phyPort=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* Martin ZHU note:delete port related VLAN seting ACL rules first */
	ctc_hgu_acl_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port */
	
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TRANSPARENT;

	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.ingress_port_idx = phyPort;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}
	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
	aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_TRANSPARENT;

	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TRANSPARENT;

	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.egress_port_idx = phyPort;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	ctc_hgu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRANSPARENT, NULL);
	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_vlanTag_create(unsigned int lport, ctc_wrapper_vlan_t *pTagCfg)
{
	int32 ret=0;
	uint32 aclIdx=0;
	uint32 phyPort=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* Martin ZHU note:delete port related VLAN seting ACL rules first */
	ctc_hgu_acl_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port */
	
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

	aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
	aclRule.action_acl_cvlan.assignedCvid=pTagCfg->vid;
	aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_ASSIGN;
	aclRule.action_acl_cvlan.assignedCpri=pTagCfg->pri;

	/* ingress port = phyPort */
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.ingress_port_idx = phyPort;

	/* ingress no ctag */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 0;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
	aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_UNTAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.egress_port_idx = phyPort;

	/* ingress c-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_STAGIF_BIT;
	aclRule.ingress_stagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = pTagCfg->vid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	ctc_hgu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TAG, NULL);
	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_vlanTranslation_create(unsigned int lport, ctc_wrapper_vlanTransCfg_t *pTransCfg)
{
	int32 ret=0, i;
	uint32 aclIdx=0;
	uint32 phyPort=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* Martin ZHU note:delete port related VLAN seting ACL rules first */
	ctc_hgu_acl_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port */
	
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

	aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
	aclRule.action_acl_cvlan.assignedCvid=pTransCfg->defVlan.vid;
	aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_ASSIGN;
	aclRule.action_acl_cvlan.assignedCpri=pTransCfg->defVlan.pri;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.ingress_port_idx = phyPort;

	/* ingress no ctag */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 0;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	/* Add translation entries one-by-one */
	 for (i = 0; i < pTransCfg->num; i++)
	 {		   
		memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
		aclRule.action_type = ACL_ACTION_TYPE_QOS;
		aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
		aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

		aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
		aclRule.action_acl_cvlan.assignedCvid=pTransCfg->transVlanPair[i].newVlan.vid;
		aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
	
		aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
		aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
		aclRule.ingress_port_idx = phyPort;

		/* ingress c-tag VID = pTransCfg->transVlanPair[i].oriVlan.vid  */
		aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
		aclRule.ingress_ctagIf = 1;
		/* fill in cf VID field */
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = pTransCfg->transVlanPair[i].oriVlan.vid;
	
		ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
		if (RT_ERR_RG_OK!= ret)
		{
			return RT_ERR_FAILED;
		}

		drv_hgu_acl_cfg_set(aclIdx, &aclRule);
		drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
	}

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
	aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_UNTAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.egress_port_idx = phyPort;

	/* ingress s-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_STAGIF_BIT;
	aclRule.ingress_stagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = pTransCfg->defVlan.vid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	for (i = 0; i < pTransCfg->num; i++)
	{
		/* Downstream filter out-going port */
		memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
		aclRule.action_type = ACL_ACTION_TYPE_QOS;
		aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
		aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_TAGGING_WITH_8100;

		aclRule.action_acl_svlan.svlanSvidDecision=ACL_SVLAN_SVID_ASSIGN;
		aclRule.action_acl_svlan.assignedSvid=pTransCfg->transVlanPair[i].oriVlan.vid;
		aclRule.action_acl_svlan.svlanSpriDecision=ACL_SVLAN_SPRI_COPY_FROM_1ST_TAG;
	
		aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
		aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
		aclRule.egress_port_idx = phyPort;

		/* ingress c-tag VID= pTagCfg->vid */
		aclRule.filter_fields |= INGRESS_STAGIF_BIT;
		aclRule.ingress_stagIf = 1;
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = pTransCfg->transVlanPair[i].newVlan.vid;
		
		ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
		if (RT_ERR_RG_OK!= ret)
		{
			return RT_ERR_FAILED;
		}

		drv_hgu_acl_cfg_set(aclIdx, &aclRule);
		drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
	}
	
	ctc_hgu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRANSLATION, NULL);
	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_vlanAggregation_create(unsigned int lport, ctc_wrapper_vlanAggreCfg_t *pAggrCfg)
{
	int32 ret=0, i, j;
	uint32 aclIdx=0;
	uint32 phyPort=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* Martin ZHU note:delete port related VLAN seting ACL rules first */
	ctc_hgu_acl_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	
	/* =========== Upstream entry =========== */

    /* Upstream action C tagging and assign VID + PRI */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

	aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
	aclRule.action_acl_cvlan.assignedCvid=pAggrCfg->defVlan.vid;
	aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_ASSIGN;
	aclRule.action_acl_cvlan.assignedCpri=pAggrCfg->defVlan.pri;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.ingress_port_idx = phyPort;

	/* ingress no ctag */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 0;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	/* Add aggregation entries one-by-one */
	for (i = 0; i < pAggrCfg->tableNum; i++)
	{
		for(j = 0; j < pAggrCfg->aggrTbl[i].entryNum; j++)
		{	   
			memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
			aclRule.action_type = ACL_ACTION_TYPE_QOS;
			aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
			aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

			aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
			aclRule.action_acl_cvlan.assignedCvid=pAggrCfg->aggrTbl[i].aggreToVlan.vid;
			aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
	
			aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
			aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
			aclRule.ingress_port_idx = phyPort;

			/* ingress c-tag VID = pTransCfg->transVlanPair[i].oriVlan.vid  */
			aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
			aclRule.ingress_ctagIf = 1;
			aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
			aclRule.egress_ctag_vid = pAggrCfg->aggrTbl[i].aggreFromVlan[j].vid;
	
			ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
			if (RT_ERR_RG_OK!= ret)
			{
				return RT_ERR_FAILED;
			}

			drv_hgu_acl_cfg_set(aclIdx, &aclRule);
			drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
		}
	}	

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_UNTAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.egress_port_idx = phyPort;

	/* ingress s-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_STAGIF_BIT;
	aclRule.ingress_stagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = pAggrCfg->defVlan.vid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	for (i = 0; i < pAggrCfg->tableNum; i++)
	{
		/* Downstream filter out-going port */
		memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
		aclRule.action_type = ACL_ACTION_TYPE_QOS;
		aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
		aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_UNTAG;

		
		aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
		aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;
		aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_CPOY_FROM_DMAC2CVID;
	
		aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
		aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
		aclRule.egress_port_idx = phyPort;

		/* ingress s-tag VID= pAggrCfg->aggrTbl[i].aggreToVlan.vid */
		aclRule.filter_fields |= INGRESS_STAGIF_BIT;
		aclRule.ingress_stagIf = 1;
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = pAggrCfg->aggrTbl[i].aggreToVlan.vid;
		
		ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
		if (RT_ERR_RG_OK!= ret)
		{
			return RT_ERR_FAILED;
		}

		drv_hgu_acl_cfg_set(aclIdx, &aclRule);
		drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
	}
	
	ctc_hgu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_AGGREGATION, NULL);
	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_vlanTrunk_create(unsigned int lport, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg)
{
	int32 ret=0, i;
	uint32 aclIdx=0;
	uint32 phyPort=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* Martin ZHU note:delete port related VLAN seting ACL rules first */
	ctc_hgu_acl_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	
	/* =========== Upstream entry =========== */

    /* Upstream action C tagging and assign VID + PRI */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

	aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
	aclRule.action_acl_cvlan.assignedCvid=pTrunkCfg->defVlan.vid;
	aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_ASSIGN;
	aclRule.action_acl_cvlan.assignedCpri=pTrunkCfg->defVlan.pri;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.ingress_port_idx = phyPort;

	/* ingress no ctag */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 0;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	 /* Add trunk entries one-by-one */
	for (i = 0; i < pTrunkCfg->num; i++)
	{
		memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
		aclRule.action_type = ACL_ACTION_TYPE_QOS;
		aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
		aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_NOP;
		
		aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;	
		aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
		aclRule.ingress_port_idx = phyPort;

		/* ingress c-tag VID = pTransCfg->transVlanPair[i].oriVlan.vid  */
		aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
		aclRule.ingress_ctagIf = 1;
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = pTrunkCfg->acceptVlan[i].vid;
	
		ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
		if (RT_ERR_RG_OK!= ret)
		{
			return RT_ERR_FAILED;
		}

		drv_hgu_acl_cfg_set(aclIdx, &aclRule);
		drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
	}	

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_UNTAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
	aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
	aclRule.egress_port_idx = phyPort;

	/* ingress c-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_STAGIF_BIT;
	aclRule.ingress_stagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = pTrunkCfg->defVlan.vid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);

	for (i = 0; i < pTrunkCfg->num; i++)
	{
		/* Downstream filter out-going port */
		memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
		aclRule.action_type = ACL_ACTION_TYPE_QOS;
		aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
		aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_NOP;
	
		aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	
		aclRule.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
		aclRule.egress_port_idx = phyPort;

		/* ingress c-tag VID= pAggrCfg->aggrTbl[i].aggreToVlan.vid */
		aclRule.filter_fields |= INGRESS_STAGIF_BIT;
		aclRule.ingress_stagIf = 1;
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = pTrunkCfg->acceptVlan[i].vid;
		
		ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
		if (RT_ERR_RG_OK!= ret)
		{
			return RT_ERR_FAILED;
		}

		drv_hgu_acl_cfg_set(aclIdx, &aclRule);
		drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_VLAN);
	}
	
	ctc_hgu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRUNK, NULL);
	
    return RT_ERR_OK;
}
#endif

/* for multicast */
int32 ctc_hgu_acl_rule_for_mcastvlan_clear(unsigned int lport)
{
	uint32 ret;
	uint32 aclIdx=0;
	uint32 lportTmp, usage, VlanID;
	int32  i;
	
	for (i=0; i<ACL_RULE_NUM_MAX; i++)
	{
		usage = CTC_HGU_CFUSAGE_RESERVED;
		
		drv_hgu_acl_resouce_get(i, &lportTmp, &usage);
		if ((lportTmp != lport) || (CTC_HGU_CFUSAGE_MC != usage))
			continue;

		aclIdx = i;
        ret = rtk_rg_aclFilterAndQos_del(aclIdx);
        if (RT_ERR_RG_OK != ret)
		{
            return RT_ERR_FAILED;
        }
		
		/* reset ACL m_AclDB[i] */
		drv_hgu_acl_cfg_reset(i);
	}
	
    return RT_ERR_OK;
}


int32 ctc_hgu_acl_rule_for_McastTransparent_create(unsigned int lport, uint32 uiMcVid)
{
#if 0
	int32 ret=0;
	uint32 aclIdx=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	
	/* =========== Upstream entry =========== */
	/* Upstream don't care */
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	
	aclRule.qos_actions |= ACL_ACTION_ACL_SVLANTAG_BIT;
	aclRule.action_acl_svlan.svlanTagIfDecision=ACL_SVLAN_TAGIF_TRANSPARENT;

	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TRANSPARENT;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	

	/* ingress c-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = uiMcVid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK!= ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_MC);
#endif	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_McastStrip_create(unsigned int lport, uint32 uiMcVid)
{
#if 0
	int32 ret=0;
	uint32 aclIdx=0;
	rtk_rg_aclFilterAndQos_t aclRule;
		
	/* =========== Upstream entry =========== */
	/* Upstream don't care */

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_UNTAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;

	/* ingress C-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 1;
	aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
	aclRule.egress_ctag_vid = uiMcVid;
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_MC);
#endif	
    return RT_ERR_OK;
}

int32 ctc_hgu_acl_rule_for_McastTranslation_create(unsigned int lport, uint32 mcVid, uint32 userVid)
{
#if 0
	int32 ret=0;
	uint32 aclIdx=0;
	rtk_rg_aclFilterAndQos_t aclRule;
	 
	/* =========== Upstream entry =========== */
	/* don't care */
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	memset(&aclRule, 0, sizeof(rtk_rg_aclFilterAndQos_t));
	aclRule.action_type = ACL_ACTION_TYPE_QOS;
	aclRule.qos_actions |= ACL_ACTION_ACL_CVLANTAG_BIT;
	aclRule.action_acl_cvlan.cvlanTagIfDecision=ACL_CVLAN_TAGIF_TAGGING;

	aclRule.action_acl_cvlan.cvlanCvidDecision=ACL_CVLAN_CVID_ASSIGN;
	aclRule.action_acl_cvlan.assignedCvid=userVid;
	aclRule.action_acl_cvlan.cvlanCpriDecision=ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
	
	aclRule.fwding_type_and_direction = ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;	

	/* ingress s-tag VID= pTagCfg->vid */
	aclRule.filter_fields |= INGRESS_CTAGIF_BIT;
	aclRule.ingress_ctagIf = 1;

	if(is9602C())
	{
		aclRule.filter_fields |= INGRESS_PORT_BIT;
		aclRule.ingress_port_mask.portmask = 1 << HAL_GET_PON_PORT();
		
		aclRule.filter_fields |= INGRESS_CTAG_VID_BIT;
		aclRule.ingress_ctag_vid = mcVid;
	}
	else
	{
		/* Martin ZHu note:when set EGRESS_CTAG_VID_BIT,rg will fill in CLASSIFY_FIELD_TAG_VID field */
		aclRule.filter_fields |= EGRESS_CTAG_VID_BIT;
		aclRule.egress_ctag_vid = mcVid;
	}
	
	ret = rtk_rg_aclFilterAndQos_add(&aclRule, &aclIdx);
	if (RT_ERR_RG_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_hgu_acl_cfg_set(aclIdx, &aclRule);
	drv_hgu_acl_resouce_set(aclIdx, lport, CTC_HGU_CFUSAGE_MC);
#endif
	return RT_ERR_OK;
}

#endif//end of CONFIG_HGU_APP
