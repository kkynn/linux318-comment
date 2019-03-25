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

#ifdef CONFIG_VLAN_USE_CLASSF
#include "drv_clf.h"
#include "ctc_mc.h"
#endif

static ctc_sfu_cfDb_t m_CfDB[CLF_RULE_NUM_MAX];
/* 
 * vlan translation will use classfication rule, so qos should not use 
 * classification rule anymore
 */
//static port_clf_pri_to_queue_cfg_t m_astUniClfPriToQueueMode[MAX_PORT_NUM];

static void drv_clf_db_init(void)
{
	uint32 lport, uiPrec;
	int i;

	memset(m_CfDB, 0, sizeof(m_CfDB));
	//for 9601B, pattern 0 only rule 0-31 with aclhitindex field
	for(i = CLF_ACLHIT_RULE_START; i <= CLF_ACLHIT_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_ACLHIT;
	}
	//for 9601B, pattern 0 rule 32-63 with ethertype field
	for(i = CLF_ETHTYPE_RULE_START; i <= CLF_ETHTYPE_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_ETHERTYPE;
	}
	//for 9601B, rule 64-95 for multicast vlan 
	for(i = CLF_MULTICAST_RULE_START; i <= CLF_MULTICAST_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_MULTICAST;
	}
	//for 9601B, pattern 1/2/3 has the same format
	for(i = CLF_GENERIC_RULE_START; i <= CLF_GENERIC_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_GENERIC;
	}
#ifdef CONFIG_VLAN_USE_CLASSF
	//for 9602c and 9607c
	for(i = CLF_VLAN_PERMIT_RULE_START; i <= CLF_VLAN_PERMIT_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_VLAN_PERMIT;
	}
#endif
	/* Reserved 2 entries for drop unmatch */
	for(i = CLF_RESERVED_RULE_START; i <= CLF_RESERVED_RULE_END; i++)
	{
		m_CfDB[i].type = CTC_SFU_CFTYPE_RESERVED;
	}

#if 0
	FOR_EACH_LAN_PORT(lport)
    {
    	/*ctc classify remark cfg init*/
        for (uiPrec = 0; uiPrec < CTC_CLF_REMARK_RULE_NUM_MAX; uiPrec++)
        {
            drv_cfg_port_clfpri2queue_ruleid_set(lport, uiPrec, CLF_RULE_ID_IVALLID);
        }
    }
   #endif
    return;
}

int32 drv_clf_cfg_get(uint32 uiClfId, void *pstClfCfg)
{
    if ((CLF_RULE_NUM_MAX <= uiClfId) ||
        (NULL == pstClfCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(pstClfCfg, &m_CfDB[uiClfId].clfCfg, sizeof(rtk_classify_cfg_t));

    return RT_ERR_OK;
}

int32 drv_clf_cfg_set(uint32 uiClfId, void *pstClfCfg)
{
    if ((CLF_RULE_NUM_MAX <= uiClfId) ||
        (NULL == pstClfCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(&m_CfDB[uiClfId].clfCfg, pstClfCfg, sizeof(rtk_classify_cfg_t));

    return RT_ERR_OK;
}

uint32 drv_clf_resouce_set(uint32 uiClfId, uint32 port, uint32 usage)
{
	if (uiClfId >= CLF_RULE_NUM_MAX)
		return RT_ERR_FAILED;
	
	m_CfDB[uiClfId].port = port;
	m_CfDB[uiClfId].usage = usage;
	m_CfDB[uiClfId].valid = 1;
	
	return RT_ERR_OK;
}

uint32 drv_clf_resouce_get(uint32 uiClfId, uint32 *port, uint32 *usage)
{
	if ((NULL == port) ||
		(NULL == usage))
		return RT_ERR_INPUT;

	if (!m_CfDB[uiClfId].valid)
		return RT_ERR_FAILED;
	
	*port = m_CfDB[uiClfId].port;
	*usage = m_CfDB[uiClfId].usage;

	return RT_ERR_OK;
}

int32 drv_clf_cfg_rule_clear(uint32 uiClfRuleId)
{
    rtk_classify_field_t *pstClfRuleField = NULL;
    rtk_classify_field_t *pstClfRuleFieldNext = NULL;
    rtk_classify_cfg_t stClfCfg;

    if (CLF_RULE_NUM_MAX <= uiClfRuleId)
    {
        return RT_ERR_INPUT;
    }

    drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);

    pstClfRuleField = stClfCfg.field.pFieldHead;
    while (NULL != pstClfRuleField)
    {
        pstClfRuleFieldNext = pstClfRuleField->next;
        free(pstClfRuleField);
        pstClfRuleField = pstClfRuleFieldNext;
    }

    memset(&m_CfDB[uiClfRuleId].clfCfg, 0, sizeof(rtk_classify_cfg_t));
	m_CfDB[uiClfRuleId].valid = 0;

    return RT_ERR_OK;
}

int32 drv_clf_rule_emptyid_get(uint32 *puiClfRuleId, uint32 type)
{
    uint32 uiClfruleIndex = 0;

    if (NULL == puiClfRuleId)
    {
        return RT_ERR_INPUT;
    }
    
    for (uiClfruleIndex=0; uiClfruleIndex<CLF_RULE_NUM_MAX; uiClfruleIndex++)
    {
        if ((!m_CfDB[uiClfruleIndex].valid) && (type == m_CfDB[uiClfruleIndex].type))
        {
            *puiClfRuleId = uiClfruleIndex;
            break;
        }
    }

    if (CLF_RULE_NUM_MAX == uiClfruleIndex)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_clf_rule_field_fill(rtk_classify_field_type_t enTrustMode, 
											void *pValue, 
											void *pClfTypeAndValue) 
{
    rtk_classify_field_t *pstRtlClfTypeAndValue = NULL;

    if ((CLASSIFY_FIELD_END <= enTrustMode) ||
        (NULL == pValue)             ||
        (NULL == pClfTypeAndValue))
    {
        return RT_ERR_INPUT;
    }   

    pstRtlClfTypeAndValue = (rtk_classify_field_t *)pClfTypeAndValue;

    switch (enTrustMode)
    {        
        case CLASSIFY_FIELD_IS_STAG:
			pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_IS_STAG;
			pstRtlClfTypeAndValue->classify_pattern.isStag.value = *((uint32 *)pValue);
			pstRtlClfTypeAndValue->classify_pattern.isStag.mask = 0x1;
            break;
		case CLASSIFY_FIELD_IS_CTAG:
			pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_IS_CTAG;
			pstRtlClfTypeAndValue->classify_pattern.isCtag.value = *((uint32 *)pValue);
			pstRtlClfTypeAndValue->classify_pattern.isCtag.mask = 0x1;
			break;
		case CLASSIFY_FIELD_TAG_VID:
			pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_TAG_VID;
			pstRtlClfTypeAndValue->classify_pattern.tagVid.value = *((uint32 *)pValue);
			pstRtlClfTypeAndValue->classify_pattern.tagVid.mask = 0xfff;
			break;
		case CLASSIFY_FIELD_ACL_HIT:
			pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_ACL_HIT;
			pstRtlClfTypeAndValue->classify_pattern.aclHit.value = *((uint32 *)pValue);
			pstRtlClfTypeAndValue->classify_pattern.aclHit.mask = 0x7f;
			break;
		case CLASSIFY_FIELD_UNI:
			pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_UNI;
#ifdef CONFIG_RTL9607C_SERIES
			/* 9607c cpu port value may be 9 or 10 */
			pstRtlClfTypeAndValue->classify_pattern.uni.value = (*(uint32 *)pValue) & 0xf;
			pstRtlClfTypeAndValue->classify_pattern.uni.mask = 0xf; 
#else
			pstRtlClfTypeAndValue->classify_pattern.uni.value = (*(uint32 *)pValue) & 0x7;
			pstRtlClfTypeAndValue->classify_pattern.uni.mask = 0x7;
#endif
			break;
        default:
            pstRtlClfTypeAndValue->fieldType = CLASSIFY_FIELD_END;
            break;
    }

    return RT_ERR_OK;
}

int32 drv_clf_rule_create(uint32 uiClfId, 
								rtk_classify_field_type_t uiClfRuleType, 
								void *pClfRuleTypeValue)
{
    int32 ret = RT_ERR_OK;
    rtk_classify_field_t *pstClfRuleField = NULL;
	rtk_classify_cfg_t stClfCfg;
  
    if ((CLF_RULE_NUM_MAX <= uiClfId)    ||
        (CLASSIFY_FIELD_END <= uiClfRuleType) ||        
        (NULL == pClfRuleTypeValue))
    {
        return RT_ERR_INPUT;
    }

    pstClfRuleField = (rtk_classify_field_t *)malloc(sizeof(rtk_classify_field_t));
    if(NULL == pstClfRuleField)
    {
        return RT_ERR_FAILED;
    }
    
    memset(pstClfRuleField, 0, sizeof(rtk_classify_field_t));    

    ret = drv_clf_rule_field_fill(uiClfRuleType, pClfRuleTypeValue, (void *)pstClfRuleField);
    if (RT_ERR_OK != ret)
    {
        free(pstClfRuleField);
        return RT_ERR_FAILED;
    }

    if (CLASSIFY_FIELD_END <= pstClfRuleField->fieldType)
    {
        free(pstClfRuleField);
        return RT_ERR_FAILED;
    }

    pstClfRuleField->next = NULL;

    drv_clf_cfg_get(uiClfId,(void *)&stClfCfg);

    if (RT_ERR_OK != rtk_classify_field_add(&stClfCfg, pstClfRuleField))
    {
        free(pstClfRuleField);
        return RT_ERR_FAILED;
    }	
	
    drv_clf_cfg_set(uiClfId, (void *)&stClfCfg);

    return RT_ERR_OK;
}

int32 drv_clf_rule_bind(uint32 uiClfId)
{
	int32 ret = RT_ERR_OK;
    rtk_classify_cfg_t stClfCfg;

    if (CLF_RULE_NUM_MAX <= uiClfId)
    {
        return RT_ERR_INPUT;
    }

    drv_clf_cfg_get(uiClfId, (void *)&stClfCfg);

	stClfCfg.valid = ENABLED;
	stClfCfg.index = uiClfId;

    ret = rtk_classify_cfgEntry_add(&stClfCfg);
    if (RT_ERR_OK != ret)
    {
    	printf("[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
        return RT_ERR_FAILED;
    }

    drv_clf_cfg_set(uiClfId, (void *)&stClfCfg);
    
    return RT_ERR_OK;
}

int32 drv_clf_rule_unbind(uint32 uiClfId)
{
    int32 ret = RT_ERR_OK;

    if (CLF_RULE_NUM_MAX <= uiClfId)
    {
        return RT_ERR_INPUT;
    }
   
    drv_clf_cfg_rule_clear(uiClfId);
    
 	ret = rtk_classify_cfgEntry_del(uiClfId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

#if 0
int32 drv_cfg_port_clfpri2queue_num_Inc(uint32 uiLPortId)
{
    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }
    
    if ((CTC_CLF_REMARK_RULE_NUM_MAX) > m_astUniClfPriToQueueMode[uiLPortId].uiClfRmkRuleNum)
    {
        m_astUniClfPriToQueueMode[uiLPortId].uiClfRmkRuleNum++;
    }

    return RT_ERR_OK;
}

int32 drv_cfg_port_clfpri2queue_num_dec(uint32 uiLPortId)
{
    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }
    
    if (0 < m_astUniClfPriToQueueMode[uiLPortId].uiClfRmkRuleNum)
    {
        m_astUniClfPriToQueueMode[uiLPortId].uiClfRmkRuleNum--;
    }

    return RT_ERR_OK;
}

int32 drv_cfg_port_clfpri2queue_ruleid_set(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 uiClfId)
{   
    if ((!IsValidLgcPort(uiLPortId)) ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    m_astUniClfPriToQueueMode[uiLPortId].auiClfList[uiClfRmkIndex] = uiClfId;

    return RT_ERR_OK;
}

int32 drv_cfg_port_clfpri2queue_ruleid_get(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 *puiClfId)
{
	if ((!IsValidLgcPort(uiLPortId)) ||
        (NULL == puiClfId)           ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    *puiClfId = m_astUniClfPriToQueueMode[uiLPortId].auiClfList[uiClfRmkIndex];

    return RT_ERR_OK;
}
#endif

#ifdef CONFIG_VLAN_USE_CLASSF
/* siyuan 20170420: for 9602c and 9607c, create clf permit rules for each vlan set by vlan settings
   (in vlan translation, trunk, aggregation and tag mode).
   in order to not drop downstream flood packet whose vlan id belongs to vlan settings */

/* Here port is used as bitmap */
static uint32 drv_clf_resouce_set2(uint32 uiClfId, uint32 port, uint32 usage, uint16 vlan)
{
	if (uiClfId >= CLF_RULE_NUM_MAX)
		return RT_ERR_FAILED;
	
	m_CfDB[uiClfId].port = port;
	m_CfDB[uiClfId].usage = usage;
	m_CfDB[uiClfId].valid = 1;
	m_CfDB[uiClfId].vlan = vlan;
	return RT_ERR_OK;
}

static uint32 drv_clf_resouce_get2(uint32 uiClfId, uint32 *port, uint32 *usage, uint16 *vlan)
{
	if ((NULL == port) || (NULL == usage) || (NULL == vlan))
		return RT_ERR_INPUT;

	if (!m_CfDB[uiClfId].valid)
		return RT_ERR_FAILED;
	
	*port = m_CfDB[uiClfId].port;
	*usage = m_CfDB[uiClfId].usage;
	*vlan = m_CfDB[uiClfId].vlan;
	return RT_ERR_OK;
}

int32 drv_clf_rule_for_vlan_permit_create(uint32 lport, uint32 svlanId, uint32 isTag)
{
	int32 ret, i;
	rtk_portmask_t stPhyMask;
    rtk_portmask_t stPhyMaskUntag;
	uint32 uiPPort;
	uint32 lportTmp, usage;
	uint16 vlan;
	uint32 uiClfRuleId=0, value;
	rtk_classify_cfg_t stClfCfg;	

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] port[0x%x] svlan[%d] isTag[%d]\n",__func__,__LINE__,lport,svlanId,isTag);

	/* add svlan member port, create svlan if it not exist */
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);
	uiPPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	if(isTag == 0)
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);

    ret = drv_svlan_member_add(svlanId, 0, stPhyMask, stPhyMaskUntag, 0);
    if (RT_ERR_OK != ret)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
        return RT_ERR_FAILED;
    }

	/* check if the same vlan was added by other lan port */
	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
			
		if (RT_ERR_OK != drv_clf_resouce_get2(i, &lportTmp, &usage, &vlan))
			continue;
			
		if ((CTC_SFU_CFUSAGE_VLAN_PERMIT == usage) && (vlan == svlanId))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d] original[0x%x], add port[0x%x]\n",__func__,__LINE__,i,lportTmp,(1<<lport));
			lportTmp |= (1 << lport);			
			drv_clf_resouce_set(i, lportTmp, CTC_SFU_CFUSAGE_VLAN_PERMIT);
			return RT_ERR_OK;
		}
	}
	
	/* =========== Downstream entry =========== */
	/* permit downstream packet having the svlan */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_VLAN_PERMIT);
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
	value = svlanId;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED; 
	}

	/* Filter port */
	value = 0xf; /* uni value of flood packet is 0xf */
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&value);
	if (RT_ERR_OK != ret)
	{	
		return RT_ERR_FAILED; 
	}
		
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_NOP;

	/* use sp2c entry to implement translation function */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_ADD_CTAG_8100;
	stClfCfg.act.dsAct.cVidAct = CLASSIFY_DS_VID_ACT_TRANSLATION_SP2C;
#ifdef CONFIG_RTL9602C_SERIES
	/* 9602c bug: sp2c only work when set cvlan priority action */
	stClfCfg.act.dsAct.cPriAct = CLASSIFY_DS_PRI_ACT_TRANSLATION_SP2C;
#endif

	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set2(uiClfRuleId, (1 << lport), CTC_SFU_CFUSAGE_VLAN_PERMIT, svlanId);
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d]\n",__func__,__LINE__,uiClfRuleId);
    return RT_ERR_OK;
}

int32 drv_clf_rule_for_vlan_permit_reset(uint32 lport)
{
	int32 ret;
	uint32 lportTmp, usage;
	int32  i;
	uint16 vlan;
	uint32 uiPPort;
	rtk_portmask_t stPhyMask;
	
	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
		
		if (RT_ERR_OK != drv_clf_resouce_get2(i, &lportTmp, &usage, &vlan))
			continue;
		
		if ((lportTmp & (1 << lport)) && (CTC_SFU_CFUSAGE_VLAN_PERMIT == usage))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d] original[0x%x], remove port[0x%x]\n",__func__,__LINE__,i,lportTmp,(1 << lport));

			/* remove slvan member port */
			RTK_PORTMASK_RESET(stPhyMask);
			uiPPort = PortLogic2PhyID(lport);
			if (INVALID_PORT == uiPPort)
				return RT_ERR_FAILED;
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			
		    ret = drv_svlan_member_remove(vlan, stPhyMask);
		    if (RT_ERR_OK != ret)
		    {
		        return RT_ERR_FAILED;
		    }
			
			/* remove port from clf resource port bitmap */
			lportTmp &= ~(1 << lport);
			if(lportTmp)
			{
				drv_clf_resouce_set(i, lportTmp, CTC_SFU_CFUSAGE_VLAN_PERMIT);
			}
			else
			{
		        ret = drv_clf_rule_unbind(i);
		        if (RT_ERR_OK != ret)
		        {
		        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, i);
		            return RT_ERR_FAILED;
		        }
			}
		}
	}
}

int32 drv_clf_rule_for_mcast_vlan_create(uint32 lport, uint32 svlanId, uint32 isTag)
{
	int32 ret, i;
	uint32 lportTmp, usage;
	uint16 vlan;
	uint32 uiClfRuleId=0, value;
	rtk_classify_cfg_t stClfCfg;	

	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] port[0x%x] svlan[%d] isTag[%d]\n",__func__,__LINE__,lport,svlanId,isTag);

	/* check if the same vlan was added by other lan port */
	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
			
		if (RT_ERR_OK != drv_clf_resouce_get2(i, &lportTmp, &usage, &vlan))
			continue;
			
		if ((CTC_SFU_CFUSAGE_MC == usage) && (vlan == svlanId))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d] original[0x%x], add port[0x%x]\n",__func__,__LINE__,i,lportTmp,(1<<lport));
			lportTmp |= (1 << lport);			
			drv_clf_resouce_set(i, lportTmp, CTC_SFU_CFUSAGE_MC);
			return RT_ERR_OK;
		}
	}
	
	/* =========== Downstream entry =========== */
	/* permit downstream packet having the svlan */
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
	value = svlanId;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED; 
	}
		
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_NOP;

	/* use sp2c entry to implement translation function */
	stClfCfg.act.dsAct.cAct = CLASSIFY_DS_CACT_ADD_CTAG_8100;
	stClfCfg.act.dsAct.cVidAct = CLASSIFY_DS_VID_ACT_TRANSLATION_SP2C;
#ifdef CONFIG_RTL9602C_SERIES
	/* 9602c bug: sp2c only work when set cvlan priority action */
	stClfCfg.act.dsAct.cPriAct = CLASSIFY_DS_PRI_ACT_TRANSLATION_SP2C;
#endif

	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set2(uiClfRuleId, (1 << lport), CTC_SFU_CFUSAGE_MC, svlanId);
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d]\n",__func__,__LINE__,uiClfRuleId);
    return RT_ERR_OK;
}

int32 drv_clf_rule_for_mcast_vlan_reset(uint32 lport, unsigned char vlanMode)
{
	int32 ret;
	uint32 lportTmp, usage;
	int32  i;
	uint16 vlan;
	
	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
		
		if (RT_ERR_OK != drv_clf_resouce_get2(i, &lportTmp, &usage, &vlan))
			continue;
		
		if ((lportTmp & (1 << lport)) && (CTC_SFU_CFUSAGE_MC == usage))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d] original[0x%x], remove port[0x%x]\n",__func__,__LINE__,i,lportTmp,(1 << lport));

			if(vlanMode == TAG_OPER_MODE_TRANSLATION)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] sp2c_entry_delete port[%d] svlan[%d]\n",__func__,__LINE__,lport,vlan);
				drv_port_sp2c_entry_delete(lport, vlan);
			}
			
			/* remove port from clf resource port bitmap */
			lportTmp &= ~(1 << lport);
			if(lportTmp)
			{
				drv_clf_resouce_set(i, lportTmp, CTC_SFU_CFUSAGE_MC);
			}
			else
			{
		        ret = drv_clf_rule_unbind(i);
		        if (RT_ERR_OK != ret)
		        {
		        	printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, i);
		            return RT_ERR_FAILED;
		        }
			}
		}
	}
}

int32 drv_clf_rule_for_mcastvlan_add_port(unsigned int lport, unsigned char vlanMode)
{
	int32 ret, i;
	rtk_portmask_t stPhyMask;
    rtk_portmask_t stPhyMaskUntag;
	uint32 uiPPort;
	uint32 lportTmp, usage;
	uint16 vlan;

	/* set svlan entry member port */
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);
	uiPPort = PortLogic2PhyID(lport);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	if((vlanMode == TAG_OPER_MODE_STRIP) || (vlanMode == TAG_OPER_MODE_TRANSLATION))
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	
	for (i=0; i<CLF_RULE_NUM_MAX; i++)
	{
		usage = CTC_SFU_CFUSAGE_RESERVED;
		
		if (RT_ERR_OK != drv_clf_resouce_get2(i, &lportTmp, &usage, &vlan))
			continue;
		
		if ((lportTmp & (1 << lport)) && (CTC_SFU_CFUSAGE_MC == usage))
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] uiClfRuleId[%d] vlan[%d], readd port[0x%x]\n",__func__,__LINE__,i,vlan,(1 << lport));

			ret = drv_svlan_member_add(vlan, 0, stPhyMask, stPhyMaskUntag, 0);
		    if (RT_ERR_OK != ret)
		    {
		    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] Fail ret[%d]\n", __func__, __LINE__,ret);
		        return RT_ERR_FAILED;
		    }
		}
	}
	return RT_ERR_OK;
}
#endif

static int32 drv_clf_rule_for_delstag0_create(void)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 value=0;
	rtk_classify_cfg_t stClfCfg;

	/* =========== Downstream entry =========== */
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
	value = 0;
	ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED; 
	}

	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);	
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	drv_clf_resouce_set(uiClfRuleId, CTC_SFU_CF_ALLPORTS, CTC_SFU_CFUSAGE_RESERVED);

    return RT_ERR_OK;
}

int32 drv_clf_rule_for_ctc_def_drop_create(void)
{
	int32 ret=0;
	uint32 uiClfRuleId=0;
	uint32 phyPort;
	rtk_classify_cfg_t stClfCfg;

	/* =========== Upstream entry =========== */
	/* cpu output should permit */
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_RESERVED);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED; 
    }

	/* Filter cpu port */
	phyPort = PortLogic2PhyID(LOGIC_CPU_PORT);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
    ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&phyPort);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/* Upstream filter nothing */
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_US;
	
	/* Upstream action drop */
    stClfCfg.act.usAct.drop = CLASSIFY_DROP_ACT_NONE;
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, CTC_SFU_CF_ALLPORTS, CTC_SFU_CFUSAGE_RESERVED);

	/* =========== Downstream entry =========== */
	/*********** permit local packet ***********/
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_RESERVED);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED; 
    }

	/* Filter port */
	phyPort = PortLogic2PhyID(LOGIC_CPU_PORT);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
    ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_UNI, (void *)&phyPort);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/* Downstream action force forward to none */
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;

#ifdef CONFIG_RTL9607C_SERIES
	/* 9607c setting uniMast to cpu port 9 will cause Error (0xc):Input parameter is out of range */
	stClfCfg.act.dsAct.uniAct = CLASSIFY_DS_UNI_ACT_TRAP;
#else
    stClfCfg.act.dsAct.uniAct = CLASSIFY_DS_UNI_ACT_FORCE_FORWARD;
	RTK_PORTMASK_PORT_SET(stClfCfg.act.dsAct.uniMask, phyPort);
#endif
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, CTC_SFU_CF_ALLPORTS, CTC_SFU_CFUSAGE_RESERVED);

/* 9602c and 9607c use 4k svlan table to filter invalid vlan */
#ifndef CONFIG_VLAN_USE_CLASSF
	/*********** default drop all forwarding packet ***********/
	ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_RESERVED);
    if (RT_ERR_OK != ret)
    {    	
        return RT_ERR_FAILED; 
    }

    /* Downstream filter nothing */
	drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
	stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	
	/* Downstream action force forward to none */
    stClfCfg.act.dsAct.uniAct = CLASSIFY_DS_UNI_ACT_FORCE_FORWARD;
    memset(&stClfCfg.act.dsAct.uniMask, 0, sizeof(rtk_portmask_t));
	
	drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

    ret = drv_clf_rule_bind(uiClfRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	drv_clf_resouce_set(uiClfRuleId, CTC_SFU_CF_ALLPORTS, CTC_SFU_CFUSAGE_RESERVED);
#endif
    return RT_ERR_OK;
}

int32 drv_clf_init(void)
{
	int pport;
	
	drv_clf_db_init();

	pport = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT == pport)
		return RT_ERR_FAILED;
	rtk_classify_cfSel_set(pport, CLASSIFY_CF_SEL_ENABLE);

	drv_clf_rule_for_delstag0_create();

	return RT_ERR_OK;
}
#endif//end of CONFIG_SFU_APP
