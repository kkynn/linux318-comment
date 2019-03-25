/*
 * QL: porting from V2
 */
#ifdef CONFIG_SFU_APP
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
#include <rtk/epon.h>
#include "ctc_wrapper.h"
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>

/* Chip specified include */
#include <dal/apollomp/dal_apollomp_switch.h>
#include "rtk/classify.h"
#include "rtk/stat.h"
#include "rtk/rldp.h"

/* EPON OAM include */
#include "epon_oam_config.h"
#include "epon_oam_err.h"
#include "epon_oam_db.h"
#include "epon_oam_dbg.h"
#include "epon_oam_rx.h"

/* User specific include */
#include "ctc_oam.h"
#include "ctc_oam_var.h"

/* include CTC wrapper */
#include "ctc_sfu.h"
#include "ctc_clf.h"
#include "drv_acl.h"
#include <sys_def.h>

#ifdef CONFIG_VLAN_USE_CLASSF
#include "pkt_redirect_user.h"
#include "ctc_mc.h"
#endif

uint8 SYSTEM_ADDR[MAC_ADDR_LEN];


static ctc_wrapper_vlanCfg_t m_VlanCfgDB[MAX_PORT_NUM];

#ifdef CONFIG_VLAN_USE_CLASSF
static ctc_wrapper_mcastVlanCfg_t m_mcastVlanCfgDB[MAX_PORT_NUM];
#endif

int32 ctc_del_clfrmk_by_prec(uint32 lport, uint32 precedenceOfRule)
{
    if(!IsValidLgcPort(lport))
	{
        return RT_ERR_FAILED;
    }

    if(RT_ERR_OK != drv_acl_rule_for_ctc_clfrmk_delete(lport, precedenceOfRule))
	{
        return RT_ERR_FAILED;
    }

#if 0
	/*
	 * vlan translation will use classfication rule, so qos should not 
	 * use classification rule anymore
	 */
	if(RT_ERR_OK == ctc_clf_rule_for_clfpri2queue_delete(lport, precedenceOfRule))
	{
        return RT_ERR_FAILED;
    }
#endif
	return RT_ERR_OK;
}

int32 ctc_clear_clfrmk_by_port(uint32 lport)
{
    if(!IsValidLgcPort(lport)) 
	{
        return RT_ERR_FAILED;
    }

    if(RT_ERR_OK != drv_acl_rule_for_ctc_clfrmk_clear(lport))
	{
        return RT_ERR_FAILED;
    }

#if 0
	/*
	 * vlan translation will use classfication rule, so qos should not 
	 * use classification rule anymore
	 */
	if(RT_ERR_OK != ctc_clf_rule_for_clfpri2queue_clear(lport))
	{
        return RT_ERR_FAILED;
    }
#endif
	return RT_ERR_OK;
}

int32 ctc_add_clfrmk_by_prec(uint32 lport,
	                         uint32  precedenceOfRule,
	                         oam_clasmark_rulebody_t *pstClsRule,
	                         oam_clasmark_fieldbody_t *pstClsField)
{
    uint32 uiAclRuleType;
    uint32 uiRemarkPri;
	uint32 uiQueueMapped;
	uint32 uiFieldNum;
    uint32 uiFieldId;
    acl_clf_rmk_val_en_t enRuleValue;

    /* 0180-C200-0000 ~ 0180-C200-003F */
    uint8  aucSavedMacAdd1[MAC_ADDR_LEN] = {0x01, 0x80, 0xC2, 0x0, 0x0, 0x3F};
    /* 0100-5Exx-xxxx */
    uint8  aucSavedMacAdd2[MAC_ADDR_LEN] = {0x01, 0x0, 0x5E, 0x0, 0x0, 0x0};

    if((!IsValidLgcPort(lport)) || (NULL == pstClsRule)  || (NULL == pstClsField))
    {
        return RT_ERR_FAILED;
    }

    uiFieldNum = (uint32)(pstClsRule->numOfField);
    uiRemarkPri = (uint32)(pstClsRule->ethPriMark);
	uiQueueMapped = (uint32)(pstClsRule->queueMapped);

    if (uiFieldNum > CLF_RMK_RULES_IN_1_PRECEDENCE_MAX)
    {
        return RT_ERR_FAILED;
    }

    for(uiFieldId = 0; uiFieldId < uiFieldNum; uiFieldId++)
    {
    	pstClsField += uiFieldId;

        switch (pstClsField->fieldSelect)
        {
            case OAM_CTC_CLS_FIELD_TYPE_DA_MAC :
            {
                uint8  aucZeroMacAdd[MAC_ADDR_LEN] = {0, 0, 0, 0, 0, 0};

                memcpy(enRuleValue.aucMacAddr, pstClsField->matchValue, MAC_ADDR_LEN);
				/* QL: I don't know why ignore below 2 max type, inherit from customer */
                if ( ((0 == memcmp(enRuleValue.aucMacAddr, aucSavedMacAdd1, (MAC_ADDR_LEN - 1))) &&
                      (enRuleValue.aucMacAddr[5] <= aucSavedMacAdd1[5])) ||
                     (0 == memcmp(enRuleValue.aucMacAddr, aucSavedMacAdd2, (MAC_ADDR_LEN - 3))) )
                {
                    return RT_ERR_FAILED;
                }

                uiAclRuleType = ACL_TRUST_DMAC;
                break;
            }
            case OAM_CTC_CLS_FIELD_TYPE_SA_MAC :
                uiAclRuleType = ACL_TRUST_SMAC;
                memcpy(enRuleValue.aucMacAddr, pstClsField->matchValue, MAC_ADDR_LEN); 
                if ( ((0 == memcmp(enRuleValue.aucMacAddr, aucSavedMacAdd1, (MAC_ADDR_LEN - 1))) &&
                      (enRuleValue.aucMacAddr[5] <= aucSavedMacAdd1[5])) ||
                     (0 == memcmp(enRuleValue.aucMacAddr, aucSavedMacAdd2, (MAC_ADDR_LEN - 3))) )
                {
                    return RT_ERR_FAILED;
                }
                break;
            case OAM_CTC_CLS_FIELD_TYPE_ETH_PRI :
                uiAclRuleType = ACL_TRUST_CTAG_PRIO;
                enRuleValue.ulValue = pstClsField->matchValue[5];
                break;
            case OAM_CTC_CLS_FIELD_TYPE_VLAN_ID :
                uiAclRuleType = ACL_TRUST_CTAG_VID;
                enRuleValue.ulValue = (pstClsField->matchValue[4]<<8) | pstClsField->matchValue[5];
                break;
            case OAM_CTC_CLS_FIELD_TYPE_ETHER_TYPE :
                uiAclRuleType = ACL_TRUST_ETHTYPE;
                enRuleValue.ulValue = (pstClsField->matchValue[4]<<8) | pstClsField->matchValue[5];
                break;
            case OAM_CTC_CLS_FIELD_TYPE_DST_IP :
                uiAclRuleType = ACL_TRUST_IPV4_DIP;
                memcpy(&enRuleValue.ulValue, &(pstClsField->matchValue[2]), sizeof(enRuleValue.ulValue));
                break;
            case OAM_CTC_CLS_FIELD_TYPE_SRC_IP :
                uiAclRuleType = ACL_TRUST_IPV4_SIP;
                memcpy(&enRuleValue.ulValue, &(pstClsField->matchValue[2]), sizeof(enRuleValue.ulValue));
                break;
            case OAM_CTC_CLS_FIELD_TYPE_IP_TYPE :
                uiAclRuleType = ACL_TRUST_IPV4_PROTOCOL;
                enRuleValue.ulValue = pstClsField->matchValue[5];
                break;
            /*IP PRECEDENCE(bit7-bit5)
			  IP_TOS(bit4-bit1)
			  RESERVED(bit0)*/
            case OAM_CTC_CLS_FIELD_TYPE_IP_DSCP :
                uiAclRuleType = ACL_TRUST_IPV4_TOS;
                enRuleValue.ulValue = (pstClsField->matchValue[5] << 2);
                break;
            case OAM_CTC_CLS_FIELD_TYPE_IP_PRECEDENCE :
                uiAclRuleType = ACL_TRUST_IPV4_TOS;
                enRuleValue.ulValue = pstClsField->matchValue[5];
                break;
            case OAM_CTC_CLS_FIELD_TYPE_L4_SRC_PORT :
                uiAclRuleType = ACL_TRUST_TCP_SPORT;/*ACL_TRUST_UDP_SPORT*/
                enRuleValue.ulValue = (pstClsField->matchValue[4]<<8) | pstClsField->matchValue[5]; 
                break;
            case OAM_CTC_CLS_FIELD_TYPE_L4_DST_PORT :
                uiAclRuleType = ACL_TRUST_TCP_DPORT;/*ACL_TRUST_UDP_DPORT*/
                enRuleValue.ulValue = (pstClsField->matchValue[4]<<8) | pstClsField->matchValue[5];
                break;
			case OAM_CTC_CLS_FIELD_TYPE_IP_VERSION:
				uiAclRuleType = ACL_TRUST_IP_VERSION;
				enRuleValue.ulValue = pstClsField->matchValue[15];
				break;
            default:
                return RT_ERR_FAILED;
        }

        if(RT_ERR_OK != drv_acl_rule_for_ctc_clfrmk_create(lport, precedenceOfRule, 
                                           uiAclRuleType, 
                                           (void *)&enRuleValue, 
                                           (void *)&uiQueueMapped, 
                                           (void *)&uiRemarkPri))
        {
        	printf("drv_acl_rule_for_ctc_clfrmk_create failed to add rule for port %d preced %d ruleType %d\n", 
					lport, precedenceOfRule, uiAclRuleType);
            return RT_ERR_FAILED;
        }
		#if 0
		/*
		 * vlan translation will use classfication rule, so qos should not 
		 * use classification rule anymore
		 */
		if(RT_ERR_OK != ctc_clf_rule_for_clfpri2queue_create(lport, precedenceOfRule, 
                                           (void *)&uiQueueMapped, (void *)uiRemarkPri))
        {
            return RT_ERR_FAILED;
        }
		#endif
    }

    return RT_ERR_OK;
}

void ctc_clf_init(void)
{
	memset(m_VlanCfgDB, 0, sizeof(m_VlanCfgDB));
}

int32 ctc_sys_init(void)
{
	rtk_epon_regReq_t regReq;
	
	/*Get MAC Address of ONU*/
    rtk_epon_registerReq_get(&regReq);
	//apollomp_raw_epon_regMac_get(&switch_mac);
	memcpy(SYSTEM_ADDR, regReq.mac.octet, ETHER_ADDR_LEN);
}

uint32 ctc_sfu_vlanCfg_get(uint32 lport, ctc_wrapper_vlanCfg_t **pVlanCfg)
{
	if (FALSE == IsValidLgcLanPort(lport)) return RT_ERR_INPUT;

	*pVlanCfg = &m_VlanCfgDB[lport];

	return RT_ERR_OK;
}

uint32 ctc_sfu_vlanCfg_set(uint32 lport, int vlanMode, void *pCfg)
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

#if 0
/*
 * vlan translation will use classfication rule, so qos should not 
 * use classification rule anymore
 */
int32 ctc_clf_rule_for_clfpri2queue_create(uint32 uiLPortId, 
                                   uint32 uiRulePrecedence,
                                   void *pCfPri, 
                                   void *pRemarkPri)
{
    int32 ret;
    uint32 uiClfRuleId;
	uint32 uiAclRuleId;
	rtk_classify_cfg_t stClfCfg;

    if ((!IsValidLgcPort(uiLPortId))     ||
        (NULL == pCfPri)             ||
        (NULL == pRemarkPri)		||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiRulePrecedence))
    {
        return RT_ERR_INPUT;
    }

    drv_cfg_port_clfpri2queue_ruleid_get(uiLPortId, uiRulePrecedence, &uiClfRuleId);
    if (CLF_RULE_ID_IVALLID != uiClfRuleId)
    {        
        ret = drv_clf_rule_unbind(uiClfRuleId);
        if (RT_ERR_OK != ret)
        {
        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, uiClfRuleId);
            return RT_ERR_FAILED;
        }

        drv_cfg_port_clfpri2queue_num_dec(uiLPortId);
        drv_cfg_port_clfpri2queue_ruleid_set(uiLPortId, uiRulePrecedence, CLF_RULE_ID_IVALLID);
    }
    
    ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_ACLHIT);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
	drv_cfg_port_clfrmk_acl_ruleId_get(uiLPortId, uiRulePrecedence, &uiAclRuleId);
	
    ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_ACL_HIT, (void *)&uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
        return RT_ERR_FAILED;
    }

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);	
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
	stClfCfg.act.usAct.interPriAct = CLASSIFY_CF_PRI_ACT_ASSIGN;
	stClfCfg.act.usAct.cfPri = *(uint32 *)pCfPri;

	if (0xFF != *(uint32 *)pRemarkPri)
	{
		stClfCfg.act.usAct.csPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
		stClfCfg.act.usAct.sTagPri = *(uint32 *)pRemarkPri;

		stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
		stClfCfg.act.usAct.cTagPri = *(uint32 *)pRemarkPri;
	}
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);
	
    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret) 
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, uiLPortId, CTC_SFU_CFUSAGE_CLASSIFICATION);
    drv_cfg_port_clfpri2queue_num_Inc(uiLPortId);
    drv_cfg_port_clfpri2queue_ruleid_set(uiLPortId, uiRulePrecedence, uiClfRuleId);

    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_clfpri2queue_delete(uint32 uiLPortId, uint32 uiRulePrecedence)
{
    int32 ret = RT_ERR_OK;
    uint32 uiClfRuleId;

    if ((!IsValidLgcPort(uiLPortId))     ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiRulePrecedence))
    {
        return RT_ERR_INPUT;
    }

    ret = drv_cfg_port_clfpri2queue_ruleid_get(uiLPortId, uiRulePrecedence, &uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;  
    }

    if ((CLF_RULE_NUM_MAX <= uiClfRuleId) && (CLF_RULE_ID_IVALLID != uiClfRuleId))
    {
        return RT_ERR_FAILED;
    }

    /*Delete clf rule of this port.*/
    if (CLF_RULE_ID_IVALLID != uiClfRuleId)
    {
        ret = drv_clf_rule_unbind(uiClfRuleId);
        if (RT_ERR_OK != ret)
        {
        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, uiClfRuleId);
            return RT_ERR_FAILED;
        }

        drv_cfg_port_clfpri2queue_num_dec(uiLPortId);
        drv_cfg_port_clfpri2queue_ruleid_set(uiLPortId, uiRulePrecedence, CLF_RULE_ID_IVALLID);
    }

    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_clfpri2queue_clear(uint32 uiLPortId)
{
    int32 ret = RT_ERR_OK;
    uint32 uiRulePrecedence;

    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }

    for (uiRulePrecedence = 0; uiRulePrecedence < CTC_CLF_REMARK_RULE_NUM_MAX; uiRulePrecedence++)
    {	
		ret = ctc_clf_rule_for_clfpri2queue_delete(uiLPortId, uiRulePrecedence);
		if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;  
        }
    }

    return RT_ERR_OK;
}
#endif

#ifdef CONFIG_VLAN_USE_CLASSF
/* to forward downstream flood packets, must add transparent lan port to all vlan table member port */
static int32 ctc_clf_transparent_vlan_maintain_cmd_send(uint8 action, uint32 lport)
{
	unsigned char pReplyPtr[10];
	unsigned char * pCurr;
	uint32 uiPPort;

	uiPPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	
    pCurr = pReplyPtr;
    *pCurr++ = 0x42; /* magic number */
	*pCurr++ = action;
	*pCurr++ = uiPPort;
	
    ptk_redirect_userApp_sendPkt(
        pktRedirect_sock,
        PR_KERNEL_UID_GMAC,
        0,
        pCurr - pReplyPtr,
        pReplyPtr);

    return RT_ERR_OK;
}

/* changing vlan setting from transparent mode to other mode will remove lan port from svlan member port 
   so need to add the mcast svlan member port again */
int32 ctc_clf_rule_for_mcastvlan_add_port(unsigned int lport)
{
	drv_clf_rule_for_mcastvlan_add_port(lport, m_mcastVlanCfgDB[lport].vlanMode);
	return RT_ERR_OK;
}
#endif

uint32 ctc_sfu_vlanCfg_reset(uint32 lport)
{
	int i;
	uint32 uiPPort;

	uiPPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;

	switch(m_VlanCfgDB[lport].vlanMode)
	{
	case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
		if(NULL != m_VlanCfgDB[lport].cfg.transCfg.transVlanPair)
		{
#ifdef CONFIG_VLAN_USE_CLASSF
			for (i = 0; i < m_VlanCfgDB[lport].cfg.transCfg.num; i++)
			{
				drv_port_sp2c_entry_delete(lport, m_VlanCfgDB[lport].cfg.transCfg.transVlanPair[i].newVlan.vid);
			}
#endif
			free(m_VlanCfgDB[lport].cfg.transCfg.transVlanPair);
		}
		break;
	case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
		if(NULL != m_VlanCfgDB[lport].cfg.aggreCfg.aggrTbl)
		{
#ifdef CONFIG_VLAN_USE_CLASSF
			rtk_svlan_dmacVidSelState_set(uiPPort, DISABLED);
#endif
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
#ifdef CONFIG_VLAN_USE_CLASSF
#ifdef CONFIG_RTL9602C_SERIES
		rtk_vlan_tagMode_set(uiPPort, VLAN_TAG_MODE_ORIGINAL);
#endif
		ctc_clf_transparent_vlan_maintain_cmd_send(0, lport);
		{
			/* remove port from svlan 0 table */
			rtk_portmask_t stPhyMask;
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			drv_svlan_member_remove(0, stPhyMask);
		}
		ctc_clf_rule_for_mcastvlan_add_port(lport);
		break;
#endif
	case CTC_OAM_VAR_VLAN_MODE_TAG:
		/* reset qos port internal priority of the port to default value 0 */
		rtk_qos_portPri_set(uiPPort, 0);
		break;
	default:
		break;
	}
	memset(&m_VlanCfgDB[lport], 0, sizeof(ctc_wrapper_vlanCfg_t));

	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlan_reset(unsigned int lport)
{
	uint32 ret;
	uint32 lportTmp, usage;
	int32  i;

	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
		
		if (RT_ERR_OK != drv_clf_resouce_get(i, &lportTmp, &usage))
			continue;
		
		if ((lportTmp != lport) || (CTC_SFU_CFUSAGE_VLAN != usage))
			continue;

        ret = drv_clf_rule_unbind(i);
        if (RT_ERR_OK != ret)
        {
        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, i);
            return RT_ERR_FAILED;
        }
	}
#ifdef CONFIG_VLAN_USE_CLASSF
	drv_clf_rule_for_vlan_permit_reset(lport);
#endif
	ctc_sfu_vlanCfg_reset(lport);
	
    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlanTransparent_create(unsigned int lport)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0;
	rtk_classify_cfg_t stClfCfg;
#ifdef CONFIG_VLAN_USE_CLASSF
	rtk_portmask_t stPhyMask, stPhyMaskUntag;
#endif

	ctc_clf_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port */
    ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/* Filter port */
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&phyPort);
    if (RT_ERR_OK != ret)
    {    	
        return RT_ERR_FAILED; 
    }

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
	/* Upstream action transparent */
	/* lan port will not enable svlan, so all tagged ingress packets will be 
	 *  parsed with ctag, so cAct should be assigned.
	 */
	if(TRUE == is9601B())
	{
		stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_TRANSPARENT;
	}
	else
	{
	/* fix classificationMarking not working: 
	   cvlan-act transparent action of classf rule is prior to vlan priority change action of acl rule,
	   so change to set cvlan-act tag action:copy vid and do nothing to priority */
    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
	stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_FROM_1ST_TAG;
	stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_NOP;
	}
	
	stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {    	
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

#ifdef CONFIG_VLAN_USE_CLASSF
	/* 9607c use 4k svlan table to implement transparent mode, clf rule is not necessary. */
	ctc_clf_transparent_vlan_maintain_cmd_send(1, lport);

	/* untag downstream packet will be assignned the pon port pvid(default pvid is 0), so need to add port to svlan 0 table and
	   set port to untag memberport in order to keep transparent */
	RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);

	RTK_PORTMASK_PORT_SET(stPhyMask, phyPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, phyPort);
	drv_svlan_member_add(0, 0, stPhyMask, stPhyMaskUntag, 0);
	
#ifdef CONFIG_RTL9602C_SERIES
	/* 9602c must set tag-mode to keep-format, otherwise switch follow ingress pvid setting 
		when port vlan tag-mode is original */
	ret = rtk_vlan_tagMode_set(phyPort, VLAN_TAG_MODE_KEEP_FORMAT);
	if (RT_ERR_OK != ret)
    {  
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
        return RT_ERR_FAILED;
    }
#endif
#else
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
    ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
    if (RT_ERR_OK != ret)
	{
        return RT_ERR_FAILED;
    }

    /* For downstream l2 table lookup miss packet, it will be flooded.
         * If classification downstream rule filters lan port, the rule doesn't match the flooded packet.
         * As a result the last classification rule will drop the flooded packet.
         * To resolve this problem, we can don't filter lan port since it has only one lan port for 9601b . */

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	/* Downstream action transparent */
    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_TRANSPARENT;
	/* down stream packets maybe has ctag or stag, we should set both cAct and csAct
	 * to transparent mode.
	 */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

	ctc_sfu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRANSPARENT, NULL);
#endif

    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlanTag_create(unsigned int lport, ctc_wrapper_vlan_t *pTagCfg)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;

	ctc_clf_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port and no C tagged */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter port */
	value = phyPort;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	/* Filter no C tagged */
	value = 0;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
	/* Upstream action C tagging and assign VID + PRI */
    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
    stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagVid = pTagCfg->vid;
	if(TRUE == is9601B())
	{
		stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
		stClfCfg.act.usAct.cTagPri = pTagCfg->pri;
	}
	else
	{
	/* fix classificationMarking not working: 
	   cvlan-act assign vlan action of classf rule is prior to vlan priority change action of acl rule,
	   so change cvlan-priority-act action to nop. */
	stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_NOP;
	/* use qos port internal priority to set priority of the tag mode */
	rtk_qos_portPri_set(phyPort, pTagCfg->pri);
	}
	
	/* Upstream action S untag */
	stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = pTagCfg->vid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	if(TRUE == is9607C() || TRUE == is9602C())
	{
		/* Filter port */
		value = phyPort;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
		if (RT_ERR_OK != ret)
		{	
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
			return RT_ERR_FAILED; 
		}
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
    /* Downstream action un-tagging */
	//stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_DEL_CTAG;
	/* ingress packet has stag, so csAct should be set here */
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	/* 9602c and 9601b bug: must set cvlan action, otherwise switch follow ingress pvid setting 
		when vlan tag-mode is original (RTK.0> vlan get tag-mode port 0) */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_DEL_CTAG;

	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

	ctc_sfu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TAG, pTagCfg);

#ifdef CONFIG_VLAN_USE_CLASSF
	drv_clf_rule_for_vlan_permit_create(lport, pTagCfg->vid, 0);
#endif	
	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlanTranslation_create(unsigned int lport, ctc_wrapper_vlanTransCfg_t *pTransCfg)
{
	int32 ret=0, i;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;

	ctc_clf_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port and no C tagged */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter port */
	value = phyPort;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	/* Filter no C tagged */
	value = 0;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
    /* Upstream action C tagging and assign VID + PRI */
    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
    stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagVid = pTransCfg->defVlan.vid;
    stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagPri = pTransCfg->defVlan.pri;
	/* Upstream action S untag */
	stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

    /* Add translation entries one-by-one */
	for (i = 0; i < pTransCfg->num; i++)
	{
		/* Upstream filter incoming port and C tagged */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		/* Filter port */
		value = phyPort;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		/* Filter C tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

        /* Filter tag VIDi */
		value = pTransCfg->transVlanPair[i].oriVlan.vid;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_US;
        /* Upstream action C tagging and assign VID + PRI */
        stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
        stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
        stClfCfg.act.usAct.cTagVid = pTransCfg->transVlanPair[i].newVlan.vid;
        stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
		/* Upstream action S untag */
		stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);
	}
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = pTransCfg->defVlan.vid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
    /* Downstream action un-tagging */
    //stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_DEL_CTAG;
	/* ingress packet has stag, csAct should be set here */
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);
#ifdef CONFIG_VLAN_USE_CLASSF
	drv_clf_rule_for_vlan_permit_create(lport, pTransCfg->defVlan.vid, 0);
#endif

	for (i = 0; i < pTransCfg->num; i++)
	{
		/* Downstream filter out-going port */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		/* Filter S tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

        /* Filter tag VID */
		value = pTransCfg->transVlanPair[i].newVlan.vid;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED; 
		}

		if(TRUE == is9607C() || TRUE == is9602C())
		{
			/* Filter port */
			value = phyPort;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
			if (RT_ERR_OK != ret)
			{	
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
				return RT_ERR_FAILED; 
			}
		}
	
		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_DS;
        /* Downstream action C tagging and assign VID + PRI */
		if(TRUE == is9607C() || TRUE == is9602C())
		{	/* 9602c and 9607c not have CLASSIFY_DS_CSACT_ADD_TAG_8100, so use CLASSIFY_DS_CSACT_ADD_TAG_VS_TPID2 */
        	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_ADD_TAG_VS_TPID2;
		}
		else
			stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_ADD_TAG_8100;
		
        stClfCfg.act.dsAct.csVidAct = CLASSIFY_DS_VID_ACT_ASSIGN;
        stClfCfg.act.dsAct.sTagVid = pTransCfg->transVlanPair[i].oriVlan.vid;
        stClfCfg.act.dsAct.csPriAct = CLASSIFY_DS_PRI_ACT_FROM_1ST_TAG;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

#ifdef CONFIG_VLAN_USE_CLASSF
		/* 9607c add sp2c entry to do tranlation for downstream flood packet */
		/*add sp2c entry*/
        ret = drv_port_sp2c_entry_add(lport, pTransCfg->transVlanPair[i].newVlan.vid, pTransCfg->transVlanPair[i].oriVlan.vid);
        if (RT_ERR_OK != ret)
        {
        	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
            return RT_ERR_FAILED;
        }
		drv_clf_rule_for_vlan_permit_create(lport, pTransCfg->transVlanPair[i].newVlan.vid, 0);
#endif
	}

	ctc_sfu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRANSLATION, pTransCfg);
	
	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlanAggregation_create(unsigned int lport, ctc_wrapper_vlanAggreCfg_t *pAggrCfg)
{
	int32 ret=0, i, j;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;

	ctc_clf_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port and no C tagged */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter port */
	value = phyPort;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	/* Filter no C tagged */
	value = 0;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
    /* Upstream action C tagging and assign VID + PRI */
    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
    stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagVid = pAggrCfg->defVlan.vid;
    stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagPri = pAggrCfg->defVlan.pri;
	/* Upstream action S untag */
	stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

	/* Add aggregation entries one-by-one */
	for (i = 0; i < pAggrCfg->tableNum; i++)
	{
		for(j = 0; j < pAggrCfg->aggrTbl[i].entryNum; j++)
		{
			/* Upstream filter incoming port and C tagged */
			ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/* Filter port */
			value = phyPort;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
			if (RT_ERR_OK != ret)
			{		
				return RT_ERR_FAILED; 
			}

			/* Filter C tagged */
			value = 1;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
			if (RT_ERR_OK != ret)
			{		
				return RT_ERR_FAILED; 
			}

	        /* Filter tag VIDi */
			value = pAggrCfg->aggrTbl[i].aggreFromVlan[j].vid;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
			if (RT_ERR_OK != ret)
			{		
				return RT_ERR_FAILED; 
			}

			drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
			stClfCfg.direction = CLASSIFY_DIRECTION_US;
	        /* Upstream action C tagging and assign VID + PRI */
	        stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
	        stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
	        stClfCfg.act.usAct.cTagVid = pAggrCfg->aggrTbl[i].aggreToVlan.vid;
	        stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
			/* Upstream action S untag */
			stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
			
			drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

			ret = drv_clf_rule_bind(uiClfRuleId);
			if (RT_ERR_OK != ret)
			{		
				return RT_ERR_FAILED;
			}

			drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);
		}
	}
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = pAggrCfg->defVlan.vid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
    /* Downstream action un-tagging */
	/* ingress packet has stag, we should must set dsAct here */
    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);
#ifdef CONFIG_VLAN_USE_CLASSF
	drv_clf_rule_for_vlan_permit_create(lport, pAggrCfg->defVlan.vid, 0);
#endif

	for (i = 0; i < pAggrCfg->tableNum; i++)
	{
		/* Downstream filter out-going port */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		/* Filter S tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

        /* Filter tag VID */
		value = pAggrCfg->aggrTbl[i].aggreToVlan.vid;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		if(TRUE == is9607C() || TRUE == is9602C())
		{
			/* Filter port */
			value = phyPort;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
			if (RT_ERR_OK != ret)
			{	
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
				return RT_ERR_FAILED; 
			}
		}

		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_DS;
        /* Downstream action tagging + DMAC2CVID */
        stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;

		stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_ADD_CTAG_8100;
        stClfCfg.act.dsAct.cVidAct = CLASSIFY_DS_VID_ACT_FROM_LUT;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

#ifdef CONFIG_VLAN_USE_CLASSF
		if(TRUE == is9602C())
		{	
		/* 9602c add dmacVidSelState to do aggregation for downstream flood packet
		   9607c hardware not support rtk_svlan_dmacVidSelState_set function */
			rtk_svlan_dmacVidSelState_set(phyPort, ENABLED);
		}
		drv_clf_rule_for_vlan_permit_create(lport, pAggrCfg->aggrTbl[i].aggreToVlan.vid, 0);
#endif

	}

	ctc_sfu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_AGGREGATION, pAggrCfg);
	
	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_vlanTrunk_create(unsigned int lport, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg)
{
	int32 ret=0, i;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;

	ctc_clf_rule_for_vlan_reset(lport);
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* Upstream filter incoming port and no C tagged */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter port */
	value = phyPort;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED; 
	}

	/* Filter no C tagged */
	value = 0;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED; 
	}

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
    /* Upstream action C tagging and assign VID + PRI */
    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
    stClfCfg.act.usAct.cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagVid = pTrunkCfg->defVlan.vid;
    stClfCfg.act.usAct.cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
    stClfCfg.act.usAct.cTagPri = pTrunkCfg->defVlan.pri;
	/* Upstream action S untag */
	stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

    /* Add trunk entries one-by-one */
	for (i = 0; i < pTrunkCfg->num; i++)
	{
		/* Upstream filter incoming port and C tagged */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		/* Filter port */
		value = phyPort;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		/* Filter C tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

        /* Filter tag VID */
		value = pTrunkCfg->acceptVlan[i].vid;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_US;
        /* Upstream action nop */
        stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_NOP;
		/* Upstream action S untag */
		stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_TRANSPARENT;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);
	}
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = pTrunkCfg->defVlan.vid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
    /* Downstream action un-tagging */
	/* delete stag */
    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

	for (i = 0; i < pTrunkCfg->num; i++)
	{
		/* Downstream filter out-going port */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		/* Filter S tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

        /* Filter tag VID */
		value = pTrunkCfg->acceptVlan[i].vid;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		if(TRUE == is9607C() || TRUE == is9602C())
		{
			/* Filter port */
			value = phyPort;
			ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
			if (RT_ERR_OK != ret)
			{	
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
				return RT_ERR_FAILED; 
			}
		}
		
		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_DS;
        /* Downstream action nop */
		stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_NOP;
#ifdef CONFIG_RTL9602C_SERIES
		/* 9602c bug: must set cvlan action, otherwise switch follow ingress pvid setting 
			when vlan tag-mode is original (RTK.0> vlan get tag-mode port 0) */
		stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_TRANSPARENT;
#endif

		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_VLAN);

#ifdef CONFIG_VLAN_USE_CLASSF
		drv_clf_rule_for_vlan_permit_create(lport, pTrunkCfg->acceptVlan[i].vid, 1);
#endif
	}

	ctc_sfu_vlanCfg_set(lport, CTC_OAM_VAR_VLAN_MODE_TRUNK, pTrunkCfg);
	
	return RT_ERR_OK;
}

#ifdef CONFIG_VLAN_USE_CLASSF
/* siyuan 20170425: the same mcast vlan can use only one clf rule to implement all three mcast vlan settings.
   clf rule example:
   		classf set rule direction downstream 
		classf set rule svlan-bit data 1 mask 1
		classf set rule tag-vid data 2101 mask 0xfff
		classf set downstream-action cvlan-act c-tag 
		classf set downstream-action cvlan-id-act sp2c

   mcast transparent mode: set the lan port of svlan table to tag 
   mcast trip mode: set the lan port of svlan table to untag 
   mcast translation mode : set the lan port of svlan table to tag and add sp2c entry 
*/
int32 ctc_clf_rule_for_mcastvlan_clear(unsigned int lport)
{
	drv_clf_rule_for_mcast_vlan_reset(lport, m_mcastVlanCfgDB[lport].vlanMode);
	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastTransparent_create(unsigned int lport, uint32 mcVid)
{
	/* =========== Upstream entry =========== */
	/* don't care */

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	drv_clf_rule_for_mcast_vlan_create(lport, mcVid, 1);
	
	m_mcastVlanCfgDB[lport].vlanMode = TAG_OPER_MODE_TRANSPARENT;
	
	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastStrip_create(unsigned int lport, uint32 mcVid)
{
	/* =========== Upstream entry =========== */
	/* don't care */
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	drv_clf_rule_for_mcast_vlan_create(lport, mcVid, 0);
	
	m_mcastVlanCfgDB[lport].vlanMode = TAG_OPER_MODE_STRIP;

	return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastTranslation_create(unsigned int lport, uint32 mcVid, uint32 userVid)
{
	int32 ret=0;

	/* =========== Upstream entry =========== */
	/* don't care */
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	drv_clf_rule_for_mcast_vlan_create(lport, mcVid, 0);

	/*add sp2c entry*/        
	ret = drv_port_sp2c_entry_add(lport, mcVid, userVid);        
	if (RT_ERR_OK != ret)
	{        	
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
		return RT_ERR_FAILED;
	}

	m_mcastVlanCfgDB[lport].vlanMode = TAG_OPER_MODE_TRANSLATION;
	
	return RT_ERR_OK;
}
#else
int32 ctc_clf_rule_for_mcastvlan_clear(unsigned int lport)
{
	uint32 ret;
	uint32 uiClfRuleId=0;
	uint32 lportTmp, usage;
	int32  i;

	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
		
		drv_clf_resouce_get(i, &lportTmp, &usage);
		if ((lportTmp != lport) || (CTC_SFU_CFUSAGE_MC != usage))
			continue;

		uiClfRuleId = i;
        ret = drv_clf_rule_unbind(uiClfRuleId);
        if (RT_ERR_OK != ret)
        {
        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, uiClfRuleId);
            return RT_ERR_FAILED;
        }
	}
	
    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastTransparent_create(unsigned int lport, uint32 mcVid)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* don't care */

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
    ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_MULTICAST);
    if (RT_ERR_OK != ret)
	{
        return RT_ERR_FAILED;
    }

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = mcVid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	/* Downstream action transparent */
    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_TRANSPARENT;
	/* down stream packets maybe has ctag or stag, we should set both cAct and csAct
	 * to transparent mode.
	 */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_TRANSPARENT;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_MC);
	
    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastStrip_create(unsigned int lport, uint32 mcVid)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;
	
	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* don't care */

	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
    ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_MULTICAST);
    if (RT_ERR_OK != ret)
	{
        return RT_ERR_FAILED;
    }

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = mcVid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	/* Downstream action transparent */
    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	/* down stream packets maybe has ctag or stag, we should set both cAct and csAct
	 * to transparent mode.
	 */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_DEL_CTAG;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_MC);
	
    return RT_ERR_OK;
}

int32 ctc_clf_rule_for_McastTranslation_create(unsigned int lport, uint32 mcVid, uint32 userVid)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort=0, value;
	rtk_classify_cfg_t stClfCfg;

	phyPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	/* =========== Upstream entry =========== */
	/* don't care */
	
	/* =========== Downstream entry =========== */
	/* Downstream filter out-going port */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_MULTICAST);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* Filter S tagged */
	value = 1;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}

    /* Filter tag VID */
	value = mcVid;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{		
		return RT_ERR_FAILED; 
	}
	
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	/* Downstream action S tagging and assign VID + PRI */
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_ADD_TAG_8100;
	stClfCfg.act.dsAct.csVidAct = CLASSIFY_DS_VID_ACT_ASSIGN;
	stClfCfg.act.dsAct.sTagVid = userVid;
	stClfCfg.act.dsAct.csPriAct = CLASSIFY_DS_PRI_ACT_FROM_1ST_TAG;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

	ret = drv_clf_rule_bind(uiClfRuleId);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	drv_clf_resouce_set(uiClfRuleId, lport, CTC_SFU_CFUSAGE_MC);

	return RT_ERR_OK;
}
#endif
#endif//end of CONFIG_SFU_APP
