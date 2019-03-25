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

/*
 * Include Files
 */
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
#include <hal/common/halctrl.h>
#include "drv_acl.h"
#include "ctc_clf.h"

#define MAX_TEMPLATE_NUM	4
#define MAX_ACL_FEILD_NUM	8
#define DHCP_SERVER_PORT	67
#define DHCP_CLIENT_PORT	68

static rtk_acl_template_t m_astAclTemplate[MAX_TEMPLATE_NUM] =
{
	{
		.index = 0,
		.fieldType[0] = ACL_FIELD_DMAC0,
		.fieldType[1] = ACL_FIELD_DMAC1,
		.fieldType[2] = ACL_FIELD_DMAC2,
		.fieldType[3] = ACL_FIELD_STAG,
		.fieldType[4] = ACL_FIELD_SMAC0,
		.fieldType[5] = ACL_FIELD_SMAC1,
		.fieldType[6] = ACL_FIELD_SMAC2,
		.fieldType[7] = ACL_FIELD_ETHERTYPE,
	},
	{
		.index = 1,
		.fieldType[0] = ACL_FIELD_CTAG,
		.fieldType[1] = ACL_FIELD_IPV4_SIP0,
		.fieldType[2] = ACL_FIELD_IPV4_SIP1,
		.fieldType[3] = ACL_FIELD_VID_RANGE,
		.fieldType[4] = ACL_FIELD_IP_RANGE,
		.fieldType[5] = ACL_FIELD_PORT_RANGE,
		.fieldType[6] = ACL_FIELD_IPV4_DIP0,
		.fieldType[7] = ACL_FIELD_IPV4_DIP1,
	},
	{
		.index = 2,
		.fieldType[0] = ACL_FIELD_USER_DEFINED07,
		.fieldType[1] = ACL_FIELD_USER_DEFINED08,
		.fieldType[2] = ACL_FIELD_USER_DEFINED09,
		.fieldType[3] = ACL_FIELD_USER_DEFINED10,
		.fieldType[4] = ACL_FIELD_USER_DEFINED02,
		.fieldType[5] = ACL_FIELD_USER_DEFINED03,
		.fieldType[6] = ACL_FIELD_USER_DEFINED04,
		/* siyuan 2016-11-25: support mpcp acl patch: use ethertype and ACL_FIELD_USER_DEFINED03 in same template */
		.fieldType[7] = ACL_FIELD_ETHERTYPE,
	},
	{
		.index = 3,
		.fieldType[0] = ACL_FIELD_DMAC0,
		.fieldType[1] = ACL_FIELD_DMAC1,
		.fieldType[2] = ACL_FIELD_DMAC2,
		.fieldType[3] = ACL_FIELD_CTAG,
		.fieldType[4] = ACL_FIELD_ETHERTYPE,
		.fieldType[5] = ACL_FIELD_USER_DEFINED07,/*ACL_FIELD_TCP_SPORT*//*ACL_FIELD_UDP_SPORT*/
		.fieldType[6] = ACL_FIELD_USER_DEFINED08,/*ACL_FIELD_TCP_DPORT*//*ACL_FIELD_UDP_DPORT*/
		.fieldType[7] = ACL_FIELD_STAG,
	}	
};

static rtk_acl_ingress_entry_t m_astAclCfg[ACL_RULE_NUM_MAX];

static port_clf_remark_cfg_t m_astUniClfMarkMode[MAX_PORT_NUM];

static int32 drv_cfg_port_clfrmk_mode_set(uint32 uiLPortId, uint32 uiClfRmkIndex, acl_trust_mode_t enMode);

static int32 drv_cfg_port_clfrmk_acl_ruleId_set(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 uiAclId);

static void drv_acl_init(void)
{
	uint32 lport;
	uint32 uiPrec;
	int32 index=0;

	memset(m_astAclCfg, 0, sizeof(m_astAclCfg));
	
	for(index=0;index<MAX_TEMPLATE_NUM;index++)
		rtk_acl_template_set(&m_astAclTemplate[index]);

	FOR_EACH_LAN_PORT(lport)
	{
		m_astUniClfMarkMode[lport].uiClfRmkRuleNum = 0;
		
		/*ctc classify remark cfg init*/
	    for (uiPrec = 0; uiPrec < CTC_CLF_REMARK_RULE_NUM_MAX; uiPrec++)
	    {
	        drv_cfg_port_clfrmk_mode_set(lport, uiPrec, ACL_TRUST_END);
	        drv_cfg_port_clfrmk_acl_ruleId_set(lport, uiPrec, ACL_RULE_ID_IVALLID);
	    }
	}
}

static int32 drv_acl_cfg_get(uint32 uiAclId, void *pstAclCfg)
{
    if ((ACL_RULE_NUM_MAX <= uiAclId) ||
        (NULL == pstAclCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(pstAclCfg, &m_astAclCfg[uiAclId], sizeof(rtk_acl_ingress_entry_t));

    return RT_ERR_OK;
}

static int32 drv_acl_cfg_set(uint32 uiAclId, void *pstAclCfg)
{
    if ((ACL_RULE_NUM_MAX <= uiAclId) ||
        (NULL == pstAclCfg))
    {
        return RT_ERR_INPUT;
    }

    memcpy(&m_astAclCfg[uiAclId], pstAclCfg, sizeof(rtk_acl_ingress_entry_t));

    return RT_ERR_OK;
}

static int32 drv_acl_rule_field_fill(acl_trust_mode_t enTrustMode, void *pValue, void *pAclTypeAndValue) 
{
    rtk_acl_field_t *pstRtlAclTypeAndValue = NULL;

    if ((ACL_TRUST_END <= enTrustMode) ||
        (NULL == pValue)             ||
        (NULL == pAclTypeAndValue))
    {
        return RT_ERR_INPUT;
    }   

    pstRtlAclTypeAndValue = (rtk_acl_field_t *)pAclTypeAndValue;

    switch (enTrustMode)
    {
        case ACL_TRUST_SMAC:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_SMAC;
            memcpy(pstRtlAclTypeAndValue->fieldUnion.mac.value.octet,(uint8 *)pValue,ETHER_ADDR_LEN);
            memset(pstRtlAclTypeAndValue->fieldUnion.mac.mask.octet,0xFF,ETHER_ADDR_LEN);
            break;
        case ACL_TRUST_DMAC:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_DMAC;
            memcpy(pstRtlAclTypeAndValue->fieldUnion.mac.value.octet,(uint8 *)pValue,ETHER_ADDR_LEN);
            memset(pstRtlAclTypeAndValue->fieldUnion.mac.mask.octet,0xFF,ETHER_ADDR_LEN);
            break;
        case ACL_TRUST_CTAG_PRIO:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_CTAG;
            pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.value = *((uint32 *)pValue);
            pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.mask = RTK_MAX_NUM_OF_PRIORITY - 1;
            break;
        case ACL_TRUST_ETHTYPE:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_ETHERTYPE;
            pstRtlAclTypeAndValue->fieldUnion.data.value = *((uint32 *)pValue);
    		pstRtlAclTypeAndValue->fieldUnion.data.mask = 0xFFFF;
            break;
        case ACL_TRUST_CTAG_VID:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_CTAG;
            pstRtlAclTypeAndValue->fieldUnion.l2tag.vid.value = *((uint32 *)pValue);      
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.value = 0;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.cfi_dei.value = 0;
			pstRtlAclTypeAndValue->fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.mask = 0;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.cfi_dei.mask = 0;
            break;
        case ACL_TRUST_IPV4_SIP:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_IPV4_SIP;
            pstRtlAclTypeAndValue->fieldUnion.ip.value = *(uint32 *)pValue;
            pstRtlAclTypeAndValue->fieldUnion.ip.mask = 0xFFFFFFFF;
            break;
        case ACL_TRUST_IPV4_DIP:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_IPV4_DIP;
            pstRtlAclTypeAndValue->fieldUnion.ip.value = *(uint32 *)pValue;
            pstRtlAclTypeAndValue->fieldUnion.ip.mask = 0xFFFFFFFF;
            break;
        case ACL_TRUST_IPV4_PRENCEDENCE:
            pstRtlAclTypeAndValue->fieldType = ACL_FIELD_END;
            break;
        case ACL_TRUST_IPV4_TOS:
            pstRtlAclTypeAndValue->fieldType = ACL_FIELD_USER_DEFINED09;
            pstRtlAclTypeAndValue->fieldUnion.data.value = (*((uint32 *)pValue))*256;
            pstRtlAclTypeAndValue->fieldUnion.data.mask = 0xFF00;
			break;
        case ACL_TRUST_IPV4_PROTOCOL:
            pstRtlAclTypeAndValue->fieldType = ACL_FIELD_USER_DEFINED10;
            pstRtlAclTypeAndValue->fieldUnion.data.value = (*((uint32 *)pValue))*256;
            pstRtlAclTypeAndValue->fieldUnion.data.mask = 0xFF00;
            break;
        case ACL_TRUST_TCP_SPORT:
		case ACL_TRUST_UDP_SPORT:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_USER_DEFINED07;/*ACL_FIELD_TCP_SPORT*//*ACL_FIELD_UDP_SPORT*/
            pstRtlAclTypeAndValue->fieldUnion.data.value = *((uint32 *)pValue);
            pstRtlAclTypeAndValue->fieldUnion.data.mask = 0xFFFF;
            break;
        case ACL_TRUST_TCP_DPORT:
		case ACL_TRUST_UDP_DPORT:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_USER_DEFINED08;/*ACL_FIELD_TCP_DPORT*//*ACL_FIELD_UDP_DPORT*/
            pstRtlAclTypeAndValue->fieldUnion.data.value = *((uint32 *)pValue);
            pstRtlAclTypeAndValue->fieldUnion.data.mask = 0xFFFF;
            break;        
        case ACL_TRUST_STAG_VID:
			pstRtlAclTypeAndValue->fieldType = ACL_FIELD_STAG;
			pstRtlAclTypeAndValue->fieldUnion.l2tag.vid.value = *((uint32 *)pValue);      
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.value = 0;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.cfi_dei.value = 0;
			pstRtlAclTypeAndValue->fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.pri.mask = 0;
    		pstRtlAclTypeAndValue->fieldUnion.l2tag.cfi_dei.mask = 0;
            break;
		case ACL_TRUST_IP_VERSION:
        default:
            pstRtlAclTypeAndValue->fieldType = ACL_FIELD_END;
            break;
    }

    return RT_ERR_OK;
}

static int32 drv_acl_rule_field_add(uint32 uiAclId, 
									acl_trust_mode_t uiAclRuleType, 
									void *pAclRuleTypeValue)
{
    int32 ret = RT_ERR_OK;
    rtk_acl_field_t *pstAclRuleField = NULL;
	rtk_acl_ingress_entry_t stAclCfg;
  
    if ((ACL_RULE_NUM_MAX <= uiAclId)    ||
        (ACL_TRUST_END <= uiAclRuleType) ||
        (NULL == pAclRuleTypeValue))
    {
        return RT_ERR_INPUT;
    }

	drv_acl_cfg_get(uiAclId,(void *)&stAclCfg);

	if (ACL_TRUST_IP_VERSION == uiAclRuleType)
	{
		/* FIXME: I don't know what value means IPv4 or IPv6 */
		if (0 == *(unsigned int *)pAclRuleTypeValue) {
			stAclCfg.careTag.tags[ACL_CARE_TAG_IPV4].value = 1;
			stAclCfg.careTag.tags[ACL_CARE_TAG_IPV4].mask = 1;
		}
		else {
			stAclCfg.careTag.tags[ACL_CARE_TAG_IPV6].value = 1;
			stAclCfg.careTag.tags[ACL_CARE_TAG_IPV6].mask = 1;
		}
		stAclCfg.pFieldHead = NULL;
		memset(&stAclCfg.readField, 0, sizeof(rtk_acl_raw_field_t));

		drv_acl_cfg_set(uiAclId, (void *)&stAclCfg);

		return RT_ERR_OK;
	}

    pstAclRuleField = (rtk_acl_field_t *)malloc(sizeof(rtk_acl_field_t));
    if(NULL == pstAclRuleField)
    {
        return RT_ERR_FAILED;
    }
    
    memset(pstAclRuleField, 0, sizeof(rtk_acl_field_t));    

    ret = drv_acl_rule_field_fill(uiAclRuleType, pAclRuleTypeValue, (void *)pstAclRuleField);
    if (RT_ERR_OK != ret)
    {
        free(pstAclRuleField);
        return RT_ERR_FAILED;
    }

    if (ACL_FIELD_END <= pstAclRuleField->fieldType)
    {
        free(pstAclRuleField);
        return RT_ERR_FAILED;
    }
	pstAclRuleField->next = NULL;

    drv_acl_cfg_get(uiAclId,(void *)&stAclCfg);

    if (RT_ERR_OK != rtk_acl_igrRuleField_add(&stAclCfg, pstAclRuleField))
    {
        free(pstAclRuleField);
        return RT_ERR_FAILED;
    }

	drv_acl_cfg_set(uiAclId, (void *)&stAclCfg);
	return RT_ERR_OK;
}

static int32 drv_acl_rule_act_add(uint32 uiAclId, 
									acl_action_t enAclAct, 
									void *pAclRuleData)
{
    int32 ret = RT_ERR_OK;
	rtk_acl_ingress_entry_t stAclCfg;
  
    if ((ACL_RULE_NUM_MAX <= uiAclId)    ||
        (ACL_ACTION_END <= enAclAct)     ||
        (NULL == pAclRuleData))
    {
        return RT_ERR_INPUT;
    }

	drv_acl_cfg_get(uiAclId,(void *)&stAclCfg);

	switch (enAclAct)
    {
        case ACL_ACTION_PRIORITY_ASSIGN:
            stAclCfg.act.enableAct[ACL_IGR_PRI_ACT] = ENABLED;
            stAclCfg.act.priAct.act = ACL_IGR_PRI_ACL_PRI_ASSIGN_ACT;
    		stAclCfg.act.priAct.aclPri = *((uint32 *)pAclRuleData);
            break;
		case ACL_ACTION_CVLAN_1P_REMARK:
			stAclCfg.act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
			stAclCfg.act.cvlanAct.act = ACL_IGR_CVLAN_1P_REMARK_ACT;
			stAclCfg.act.cvlanAct.dot1p = *(uint32 *)pAclRuleData;
			break;
		case ACL_ACTION_REMATK_PRIORITY:
			stAclCfg.act.enableAct[ACL_IGR_PRI_ACT] = ENABLED;
            stAclCfg.act.priAct.act = ACL_IGR_PRI_1P_REMARK_ACT;
    		stAclCfg.act.priAct.dot1p= *((uint32 *)pAclRuleData);
			break;
        case ACL_ACTION_MIRROR:
			stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_IGR_MIRROR_ACT;
			stAclCfg.act.forwardAct.portMask.bits[0] = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_CVLAN_REMARK:
			stAclCfg.act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
			stAclCfg.act.cvlanAct.act = ACL_IGR_CVLAN_EGR_CVLAN_ACT;
            stAclCfg.act.cvlanAct.cvid = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_CVLAN_ASSIGN:
            stAclCfg.act.enableAct[ACL_IGR_CVLAN_ACT]= ENABLED;
    		stAclCfg.act.cvlanAct.act = ACL_IGR_CVLAN_IGR_CVLAN_ACT;
    		stAclCfg.act.cvlanAct.cvid= *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_SVLAN_REMARK:
            stAclCfg.act.enableAct[ACL_IGR_SVLAN_ACT] = ENABLED;
			stAclCfg.act.svlanAct.act = ACL_IGR_SVLAN_EGR_SVLAN_ACT;
            stAclCfg.act.svlanAct.svid = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_COPY_TO_PORTS:
			stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_COPY_ACT;
			stAclCfg.act.forwardAct.portMask.bits[0] = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_REDIRECT_TO_PORTS:
			stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_REDIRECT_ACT;
			stAclCfg.act.forwardAct.portMask.bits[0] = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_COPY_TO_CPU:
            stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_COPY_ACT;
			stAclCfg.act.forwardAct.portMask.bits[0] = 0x1 << HAL_GET_CPU_PORT();
            break;
        case ACL_ACTION_POLICING_0:
            stAclCfg.act.enableAct[ACL_IGR_LOG_ACT]= ENABLED;
    		stAclCfg.act.cvlanAct.act = ACL_IGR_LOG_POLICING_ACT;
    		stAclCfg.act.cvlanAct.meter = *((uint32 *)pAclRuleData);
            break;
        case ACL_ACTION_TRAP_TO_CPU:
            stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_TRAP_ACT;
            break;
        case ACL_ACTION_DROP:
            stAclCfg.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
			stAclCfg.act.forwardAct.act = ACL_IGR_FORWARD_REDIRECT_ACT;
			stAclCfg.act.forwardAct.portMask.bits[0] = 0;
            break;
        default:
            break;
    }
	
    drv_acl_cfg_set(uiAclId, (void *)&stAclCfg);

    return RT_ERR_OK;
}

static int32 drv_acl_rule_cfg_create(uint32 uiAclId, acl_trust_mode_t uiAclRuleType, acl_action_t enAclAct, void *pAclRuleTypeValue, void *pAclRuleData)
{
	if (RT_ERR_OK != drv_acl_rule_field_add(uiAclId, uiAclRuleType, pAclRuleTypeValue))
		return RT_ERR_FAILED;
	
	if (RT_ERR_OK != drv_acl_rule_act_add(uiAclId, enAclAct, pAclRuleData))
		return RT_ERR_FAILED;

	return RT_ERR_OK;
}

static int32 drv_acl_rule_cfg_clear(uint32 uiAclRuleId)
{
    rtk_acl_field_t *pstAclRuleField = NULL;
    rtk_acl_field_t *pstAclRuleFieldNext = NULL;
    rtk_acl_ingress_entry_t stAclCfg;

    if (ACL_RULE_NUM_MAX <= uiAclRuleId)
    {
        return RT_ERR_INPUT;
    }

    drv_acl_cfg_get(uiAclRuleId, (void *)&stAclCfg);

    pstAclRuleField = stAclCfg.pFieldHead;
    while (NULL != pstAclRuleField)
    {
        pstAclRuleFieldNext = pstAclRuleField->next;
        free(pstAclRuleField);
        pstAclRuleField = pstAclRuleFieldNext;
    }

    memset((m_astAclCfg + uiAclRuleId), 0, sizeof(rtk_acl_ingress_entry_t));

    return RT_ERR_OK;
}

static int32 drv_acl_rule_emptyid_get(uint32 *puiAclRuleId)
{
    uint32 uiAclruleIndex = 0;
    rtk_acl_ingress_entry_t stAclCfg;

    if (NULL == puiAclRuleId)
    {
        return RT_ERR_INPUT;
    }
    
    for (uiAclruleIndex = 0; uiAclruleIndex < ACL_RULE_NUM_MAX; uiAclruleIndex++)
    {
        drv_acl_cfg_get(uiAclruleIndex, (void *)&stAclCfg);

        if (!stAclCfg.valid)
        {
            *puiAclRuleId = uiAclruleIndex;
            break;
        }
    }

    if (ACL_RULE_NUM_MAX == uiAclruleIndex)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int32 drv_acl_rule_active_ports_get(uint32 uiAclRuleId, uint32 *puiPortMask)
{
    rtk_acl_ingress_entry_t stAclCfg;
    
    if ((ACL_RULE_NUM_MAX <= uiAclRuleId) ||
        (NULL == puiPortMask))
    {
        return RT_ERR_INPUT;
    }
    
    drv_acl_cfg_get(uiAclRuleId, (void *)&stAclCfg);

    *puiPortMask = stAclCfg.activePorts.bits[0];

    return RT_ERR_OK;
}

static int32 drv_acl_rule_cfg_ports_clear(uint32 uiAclRuleId)
{
    if (ACL_RULE_NUM_MAX <= uiAclRuleId)
    {
        return RT_ERR_INPUT;
    }
    
    m_astAclCfg[uiAclRuleId].activePorts.bits[0] = 0;

    return RT_ERR_OK;
}

static int32 drv_acl_template_find(rtk_acl_field_type_t * fieldType,int32 num)
{
	int32 tempindex=0,fieldindex=0,tempfeildindex=0;
	int32 matchnum=0;
	
	if((fieldType == NULL) || (num > MAX_ACL_FEILD_NUM) || (num <= 1))
	{
		return -1;
	}
	
	for(tempindex = 0; tempindex < MAX_TEMPLATE_NUM; tempindex++)
	{
		matchnum=0;
		for(fieldindex = 0; fieldindex < num; fieldindex++)
		{		
			for(tempfeildindex = 0; tempfeildindex < MAX_ACL_FEILD_NUM; tempfeildindex++)
			{
				if((fieldType[fieldindex]==m_astAclTemplate[tempindex].fieldType[tempfeildindex])||
					((fieldType[fieldindex]==ACL_FIELD_DMAC) && (m_astAclTemplate[tempindex].fieldType[tempfeildindex]==ACL_FIELD_DMAC0))
					|| ((fieldType[fieldindex]==ACL_FIELD_SMAC) && (m_astAclTemplate[tempindex].fieldType[tempfeildindex]==ACL_FIELD_SMAC0)))
				{
					matchnum++;
					break;
				}
			}
			if(matchnum==num)
			{
				return tempindex;
			}
			if(tempfeildindex==MAX_ACL_FEILD_NUM)
			{
				break;		
			}
		}
	}
	return tempindex;
}

static int32 drv_acl_template_bind(void *pstAclCfg)
{	
	unsigned char index = 0;
	int32 tempindex=0;
	unsigned char fieldindex;
	rtk_acl_field_type_t fieldType[RTK_MAX_NUM_OF_ACL_RULE_FIELD];
	rtk_acl_field_t *tmpPtr = NULL;
	rtk_acl_ingress_entry_t *pstRtlAclCfg = NULL;

	if (NULL == pstAclCfg)
    {
        return RT_ERR_INPUT;
    }
	
	pstRtlAclCfg = (rtk_acl_ingress_entry_t *)pstAclCfg;

	if (NULL == pstRtlAclCfg->pFieldHead) {
		pstRtlAclCfg->templateIdx = 0;
		return RT_ERR_OK;
	}
	
	fieldType[index++] = pstRtlAclCfg->pFieldHead->fieldType;
	tmpPtr = pstRtlAclCfg->pFieldHead->next;
	while(tmpPtr != NULL)
	{
		fieldType[index++] = tmpPtr->fieldType;
		tmpPtr = tmpPtr->next;
	}
	
	if((index > 1))
	{
		tempindex=drv_acl_template_find(fieldType,index);
		if(tempindex>=MAX_TEMPLATE_NUM)
		{
			return RT_ERR_FAILED;
		}
		pstRtlAclCfg->templateIdx=tempindex;
		return RT_ERR_OK;
	}
	else
	{
		if((ACL_FIELD_DMAC == pstRtlAclCfg->pFieldHead->fieldType) ||
			(ACL_FIELD_SMAC == pstRtlAclCfg->pFieldHead->fieldType))
		{
			pstRtlAclCfg->templateIdx = 0;
	    	return RT_ERR_OK;
		}
		else if((ACL_FIELD_IPV4_SIP == pstRtlAclCfg->pFieldHead->fieldType) ||
				(ACL_FIELD_IPV4_DIP == pstRtlAclCfg->pFieldHead->fieldType))
		{
			pstRtlAclCfg->templateIdx = 1;
	    	return RT_ERR_OK;
		}
		else
		{
			for(index = 0; index < MAX_TEMPLATE_NUM; index++)
			{
				for(fieldindex = 0; fieldindex < MAX_ACL_FEILD_NUM; fieldindex++)
				{						
					if(pstRtlAclCfg->pFieldHead->fieldType == m_astAclTemplate[index].fieldType[fieldindex])
					{
						pstRtlAclCfg->templateIdx = index;
		    			return RT_ERR_OK;
					}
				}
			}
		}
		
	}

	return RT_ERR_FAILED;
};

static int32 drv_acl_rule_bind(uint32 uiPortMask, uint32 uiAclId)
{
	int32 ret = RT_ERR_OK;
    rtk_acl_ingress_entry_t stAclCfg;

    if (ACL_RULE_NUM_MAX <= uiAclId)
    {
        return RT_ERR_INPUT;
    }

    drv_acl_cfg_get(uiAclId, (void *)&stAclCfg);

	stAclCfg.valid = ENABLED;
	stAclCfg.index = uiAclId;

	ret = drv_acl_template_bind((void *)&stAclCfg);
	if (RT_ERR_FAILED == ret)
	{
		return RT_ERR_FAILED;
	}

    stAclCfg.activePorts.bits[0] = uiPortMask;
    stAclCfg.invert = ACL_INVERT_DISABLE;

    ret = rtk_acl_igrRuleEntry_add(&stAclCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    drv_acl_cfg_set(uiAclId, (void *)&stAclCfg);
    
    return RT_ERR_OK;
}

static int32 drv_acl_rule_unbind(uint32 uiAclId)
{
    int32 ret = RT_ERR_OK;

    if (ACL_RULE_NUM_MAX <= uiAclId)
    {
        return RT_ERR_INPUT;
    }
   
    drv_acl_rule_cfg_clear(uiAclId);
    
 	ret = rtk_acl_igrRuleEntry_del(uiAclId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int32 drv_cfg_port_clfrmk_num_inc(uint32 uiLPortId)
{
    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }
    
    if ((CTC_CLF_REMARK_RULE_NUM_MAX) > m_astUniClfMarkMode[uiLPortId].uiClfRmkRuleNum)
    {
        m_astUniClfMarkMode[uiLPortId].uiClfRmkRuleNum++;
    }

    return RT_ERR_OK;
}

static int32 drv_cfg_port_clfrmk_num_dec(uint32 uiLPortId)
{
    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }
    
    if (0 < m_astUniClfMarkMode[uiLPortId].uiClfRmkRuleNum)
    {
        m_astUniClfMarkMode[uiLPortId].uiClfRmkRuleNum--;
    }

    return RT_ERR_OK;
}

static int32 drv_cfg_port_clfrmk_acl_ruleId_set(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 uiAclId)
{   
    if ((!IsValidLgcPort(uiLPortId)) ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    m_astUniClfMarkMode[uiLPortId].auiAclList[uiClfRmkIndex] = uiAclId;

    return RT_ERR_OK;
}

int32 drv_cfg_port_clfrmk_acl_ruleId_get(uint32 uiLPortId, uint32 uiClfRmkIndex, uint32 *puiAclId)
{
    if ((!IsValidLgcPort(uiLPortId)) ||
        (NULL == puiAclId)           ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    *puiAclId = m_astUniClfMarkMode[uiLPortId].auiAclList[uiClfRmkIndex];

    return RT_ERR_OK;
}

static int32 drv_cfg_port_clfrmk_mode_set(uint32 uiLPortId, uint32 uiClfRmkIndex, acl_trust_mode_t enMode)
{
    if ((!IsValidLgcPort(uiLPortId)) ||
        (ACL_TRUST_END < enMode)    ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    m_astUniClfMarkMode[uiLPortId].aenClfRemarkMode[uiClfRmkIndex] = enMode;

    return RT_ERR_OK;
}

static int32 drv_cfg_port_clfrmk_mode_get(uint32 uiLPortId, uint32 uiClfRmkIndex, acl_trust_mode_t *penMode)
{
    if ((!IsValidLgcPort(uiLPortId)) ||
        (NULL == penMode)    ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiClfRmkIndex))
    {
        return RT_ERR_INPUT;
    }
    
    *penMode = m_astUniClfMarkMode[uiLPortId].aenClfRemarkMode[uiClfRmkIndex];

    return RT_ERR_OK;
}

static int32 drv_cfg_acl_caretag(uint32 uiAclRuleId, rtk_acl_care_tag_index_t enCareTagType, int32 bEnable)
{
    if ((ACL_RULE_NUM_MAX <= uiAclRuleId) ||
        (ACL_CARE_TAG_END <= enCareTagType)   ||
        ((TRUE != bEnable) && (FALSE != bEnable)))
    {
        return RT_ERR_INPUT;
    }
    
    if (TRUE == bEnable)
    {
		m_astAclCfg[uiAclRuleId].careTag.tags[enCareTagType].value = TRUE;
        m_astAclCfg[uiAclRuleId].careTag.tags[enCareTagType].mask = TRUE;
    }
    else
    {
        m_astAclCfg[uiAclRuleId].careTag.tags[enCareTagType].value = FALSE;
        m_astAclCfg[uiAclRuleId].careTag.tags[enCareTagType].mask = FALSE;
    }

    return RT_ERR_OK;
}

int32 drv_acl_rule_for_copy2cpu_create(int32 bBroadEn, acl_trust_mode_t uiAclRuleType,void *pRuleValue)
{
    int32 ret;
    uint32 uiLPortId;
    uint32 uiPPort;
    uint32 uiPPortMask = 0;
    uint32 uiAclRuleId;
    uint8  aucMacAdd[ETHER_ADDR_LEN] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

    if ((NULL == pRuleValue)             ||
        (ACL_TRUST_END <= uiAclRuleType) ||
        ((TRUE != bBroadEn) && (FALSE != bBroadEn)))
    {
        return RT_ERR_INPUT;
    }
   
    ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    if (TRUE == bBroadEn)
    {
        ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_DMAC, ACL_ACTION_COPY_TO_CPU, aucMacAdd, aucMacAdd);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }
    
    ret = drv_acl_rule_cfg_create(uiAclRuleId, uiAclRuleType, ACL_ACTION_COPY_TO_CPU, pRuleValue, pRuleValue);
    if (RT_ERR_OK != ret) 
    {
        return RT_ERR_FAILED;
    }

    FOR_EACH_LAN_PORT(uiLPortId)
    {
		uiPPort = PortLogic2PhyID(uiLPortId);
		if (INVALID_PORT != uiPPort)
			uiPPortMask |= (1U << uiPPort);
    }
	uiPPort = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT != uiPPort)
		uiPPortMask |= (1U << uiPPort);
	
    ret = drv_acl_rule_bind(uiPPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 drv_acl_rule_for_ctc_clfrmk_create(uint32 uiLPortId, 
                                   uint32 uiRulePrecedence,
                                   acl_trust_mode_t uiAclRuleType,
                                   void *pRuleValue,
                                   void *pAclPri, 
                                   void *pRemarkPri)
{
    int32 ret;
    uint32 uiPPort;
    uint32 uiPortMask;
    uint32 uiAclRuleId;
	rtk_acl_ingress_entry_t stAclCfg;

    if ((!IsValidLgcPort(uiLPortId))     ||
        (NULL == pAclPri)                ||
        (NULL == pRemarkPri)             ||
        (NULL == pRuleValue)             ||
        (ACL_TRUST_END <= uiAclRuleType) ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiRulePrecedence))
    {
        return RT_ERR_INPUT;
    }

    drv_cfg_port_clfrmk_acl_ruleId_get(uiLPortId, uiRulePrecedence, &uiAclRuleId);
    if (ACL_RULE_ID_IVALLID != uiAclRuleId)
    {
        if (RT_ERR_OK != drv_acl_rule_active_ports_get(uiAclRuleId, &uiPortMask))
        {
            return RT_ERR_FAILED;
        }

        if (0 != uiPortMask)
        {
            ret = drv_acl_rule_unbind(uiAclRuleId);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }
    
            ret = drv_acl_rule_cfg_ports_clear(uiAclRuleId);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            drv_cfg_port_clfrmk_num_dec(uiLPortId);
            drv_cfg_port_clfrmk_mode_set(uiLPortId, uiRulePrecedence, ACL_TRUST_END);
            drv_cfg_port_clfrmk_acl_ruleId_set(uiLPortId, uiRulePrecedence, ACL_RULE_ID_IVALLID);
        }
    }
    
	uiPPort = PortLogic2PhyID(uiLPortId);
	if (INVALID_PORT != uiPPort)
		uiPortMask = (1U << uiPPort);
    
    ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
    ret = drv_acl_rule_cfg_create(uiAclRuleId, uiAclRuleType, ACL_ACTION_PRIORITY_ASSIGN, pRuleValue, pAclPri);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
#if 0
	drv_acl_cfg_get(uiAclRuleId, (void *)&stAclCfg);
	stAclCfg.act.enableAct[ACL_IGR_INTR_ACT]= ENABLED;
    stAclCfg.act.aclLatch = ENABLED;
	drv_acl_cfg_set(uiAclRuleId, (void *)&stAclCfg);
#else
	if (RT_ERR_OK != drv_acl_rule_act_add(uiAclRuleId, 
										ACL_ACTION_CVLAN_1P_REMARK, pRemarkPri))
		return RT_ERR_FAILED;
#endif
    ret = drv_acl_rule_bind(uiPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }

    drv_cfg_port_clfrmk_num_inc(uiLPortId);
    drv_cfg_port_clfrmk_mode_set(uiLPortId, uiRulePrecedence, uiAclRuleType);
    drv_cfg_port_clfrmk_acl_ruleId_set(uiLPortId, uiRulePrecedence, uiAclRuleId);

    return RT_ERR_OK;
}

int32 drv_acl_rule_for_ctc_clfrmk_delete(uint32 uiLPortId, uint32 uiRulePrecedence)
{
    int32 ret = RT_ERR_OK;
    uint32 uiAclRuleId;
    acl_trust_mode_t enAclMode;

    if ((!IsValidLgcPort(uiLPortId))     ||
        (CTC_CLF_REMARK_RULE_NUM_MAX <= uiRulePrecedence))
    {
        return RT_ERR_INPUT;
    }

    ret = drv_cfg_port_clfrmk_mode_get(uiLPortId, uiRulePrecedence, &enAclMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED; 
    }

    if (ACL_TRUST_END == enAclMode)
    {
        return RT_ERR_OK;
    }

    ret = drv_cfg_port_clfrmk_acl_ruleId_get(uiLPortId, uiRulePrecedence, &uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;  
    }

    if (ACL_RULE_NUM_MAX <= uiAclRuleId)
    {
        return RT_ERR_FAILED;
    }

    /*Delete acl rule of this port.*/
    if (ACL_RULE_ID_IVALLID != uiAclRuleId)
    {
        ret = drv_acl_rule_unbind(uiAclRuleId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    
        ret = drv_acl_rule_cfg_ports_clear(uiAclRuleId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        drv_cfg_port_clfrmk_num_dec(uiLPortId);
        drv_cfg_port_clfrmk_mode_set(uiLPortId, uiRulePrecedence, ACL_TRUST_END);
        drv_cfg_port_clfrmk_acl_ruleId_set(uiLPortId, uiRulePrecedence, ACL_RULE_ID_IVALLID);
    }

    return RT_ERR_OK;
}

int32 drv_acl_rule_for_ctc_clfrmk_clear(uint32 uiLPortId)
{
    int32 ret = RT_ERR_OK;
    uint32 uiAclRuleId;
    uint32 uiRulePrecedence;
    acl_trust_mode_t enAclMode;

    if (!IsValidLgcPort(uiLPortId))
    {
        return RT_ERR_INPUT;
    }

    for (uiRulePrecedence = 0; uiRulePrecedence < CTC_CLF_REMARK_RULE_NUM_MAX; uiRulePrecedence++)
    {
        ret = drv_cfg_port_clfrmk_mode_get(uiLPortId, uiRulePrecedence, &enAclMode);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED; 
        }

        if (ACL_TRUST_END <= enAclMode)
        {
            continue;
        }

        ret = drv_cfg_port_clfrmk_acl_ruleId_get(uiLPortId, uiRulePrecedence, &uiAclRuleId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;  
        }

        if (ACL_RULE_NUM_MAX <= uiAclRuleId)
        {
            return RT_ERR_FAILED;
        }
    
        /*Delete acl rule of this port.*/
        if (ACL_RULE_ID_IVALLID != uiAclRuleId)
        {
            ret = drv_acl_rule_unbind(uiAclRuleId);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }
        
            ret = drv_acl_rule_cfg_ports_clear(uiAclRuleId);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            drv_cfg_port_clfrmk_num_dec(uiLPortId);
            drv_cfg_port_clfrmk_mode_set(uiLPortId, uiRulePrecedence, ACL_TRUST_END);
            drv_cfg_port_clfrmk_acl_ruleId_set(uiLPortId, uiRulePrecedence, ACL_RULE_ID_IVALLID);
        }
    }

    return RT_ERR_OK;
}

static int32 drv_acl_rule_for_cpupacketpri_create(acl_trust_mode_t uiAclRuleType, void *pRuleValue, void *pRemarkPri)
{
	int32 ret;
	uint32 lport,pport;
    uint32 uiPortMask = 0;
    uint32 uiAclRuleId;	

	pport = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT != pport)
		uiPortMask |= (1U << pport);

	/* find am empty rule from db */
    ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
    ret = drv_acl_rule_cfg_create(uiAclRuleId, uiAclRuleType, ACL_ACTION_PRIORITY_ASSIGN, pRuleValue, pRemarkPri);
    if (RT_ERR_OK != ret) 
    {
        return RT_ERR_FAILED;
    }
    ret = drv_acl_rule_bind(uiPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }

 	return RT_ERR_OK;
}

int32 drv_acl_rule_for_drop_vid_create(uint32 uiLPortId, uint32 uiVid, uint32 *puiAclId)
{
	int32 ret = RT_ERR_OK;
    uint32 uiPPort;
    uint32 uiPPortMask = 0;
    uint32 uiAclRuleId;

    if ((!IsValidLgcPort(uiLPortId)) ||
        (!VALID_VLAN_ID(uiVid))      ||
        (NULL == puiAclId))
    {
        return RT_ERR_INPUT;
    }

    *puiAclId = ACL_RULE_NUM_MAX;
    
	uiPPort = PortLogic2PhyID(uiLPortId);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;

    ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED; 
    }
    
    ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_CTAG_VID, ACL_ACTION_DROP, (void *)&uiVid, (void *)&uiVid);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED; 
    }

    ret = drv_cfg_acl_caretag(uiAclRuleId, ACL_CARE_TAG_CTAG, TRUE);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    uiPPortMask = 1U << uiPPort;
    ret = drv_acl_rule_bind(uiPPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }

    *puiAclId = uiAclRuleId;

    return RT_ERR_OK;
}

static int32 drv_acl_rule_for_cpumac_create(void)
{
	int32 ret = RT_ERR_OK;
	uint32 uiAclRuleId=0;
	uint32 uiLPortId=0;
	uint32 uiPPort=0;
	uint32 uiPPortMask;	
	uint32 uiCpuPacketPri= 6;
	
	uiPPortMask = 0;
	FOR_EACH_LAN_PORT(uiLPortId)
    {
		uiPPort = PortLogic2PhyID(uiLPortId);
		if (INVALID_PORT != uiPPort)
			uiPPortMask |= (1U << uiPPort);
    }
	uiPPort = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT != uiPPort)
		uiPPortMask |= (1U << uiPPort);

	/*dst mac==cpu mac*/
	ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
	   
	ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_DMAC, ACL_ACTION_TRAP_TO_CPU, SYSTEM_ADDR, SYSTEM_ADDR);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_DMAC, ACL_ACTION_PRIORITY_ASSIGN, SYSTEM_ADDR, &uiCpuPacketPri);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    ret = drv_acl_rule_bind(uiPPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }
	
	return RT_ERR_OK;
}

int32 drv_acl_rule_delete_by_aclid(uint32 uiAclRuleId)
{
    int32 ret = RT_ERR_OK;
    
    if (ACL_RULE_NUM_MAX <= uiAclRuleId)
    {
        return RT_ERR_FAILED;
    }

    ret = drv_acl_rule_unbind(uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;  
    }

    ret = drv_acl_rule_cfg_ports_clear(uiAclRuleId);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;  
    }

    return RT_ERR_OK;
}

static void drv_acl_fieldselect_init(void)
{
	rtk_acl_field_entry_t fieldSel;
	
	fieldSel.index = 7;
	fieldSel.format = ACL_FORMAT_IPPAYLOAD;
	fieldSel.offset = 0;
	rtk_acl_fieldSelect_set(&fieldSel);/*ACL_FIELD_TCP_SPORT*//*ACL_FIELD_UDP_SPORT*/

	fieldSel.index = 8;
	fieldSel.format = ACL_FORMAT_IPPAYLOAD;
	fieldSel.offset = 2;
	rtk_acl_fieldSelect_set(&fieldSel);/*ACL_FIELD_TCP_DPORT*//*ACL_FIELD_UDP_DPORT*/

	fieldSel.index = 9;
	fieldSel.format = ACL_FORMAT_IPV4;
	fieldSel.offset = 1;
	rtk_acl_fieldSelect_set(&fieldSel);/*ACL_TRUST_IPV4_TOS*/

	fieldSel.index = 10;
	fieldSel.format = ACL_FORMAT_IPV4;
	fieldSel.offset = 9;
	rtk_acl_fieldSelect_set(&fieldSel);/*ACL_TRUST_IPV4_PROTOCOL*/
}

int32 drv_acl_rule_init(void)
{
	uint32 uiLPortId;
    uint32 uiPPort;
    uint32 uiPPortMask;
    uint32 uiEthTypeArp = 0x0806;	
    uint32 uiUdpPort;
	uint32 uiEthTypeOAM = 0x8809;	
	uint32 uiEthTypeOAMPri= 7;    

	/* init local db */
    drv_acl_init();

	FOR_EACH_LAN_PORT(uiLPortId)
    {
		uiPPort = PortLogic2PhyID(uiLPortId);
		if (INVALID_PORT == uiPPort)
			continue;
		
		/*enable all the uiport*/
		rtk_acl_igrState_set(uiPPort, ENABLED);
		/* set acl unmatch action to permit */
		rtk_acl_igrUnmatchAction_set(uiPPort, FILTER_UNMATCH_PERMIT);
	}

	uiPPort = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT != uiPPort)
	{
		/* enable acl function */
		rtk_acl_igrState_set(uiPPort, ENABLED);
		/* set acl unmatch action to permit */
		rtk_acl_igrUnmatchAction_set(uiPPort, FILTER_UNMATCH_PERMIT);
	}
	
	drv_acl_fieldselect_init();

	/*OAM packet pri */
	drv_acl_rule_for_cpupacketpri_create(ACL_TRUST_ETHTYPE,&uiEthTypeOAM,&uiEthTypeOAMPri);

	/* trap dst mac == cpu mac frame to cpu. */
	drv_acl_rule_for_cpumac_create();
	
    /* Copy broadcast arp frame to cpu. */
    drv_acl_rule_for_copy2cpu_create(TRUE, ACL_TRUST_ETHTYPE, &uiEthTypeArp);

    /* Copy broadcast dhcp req frame to cpu. */
    uiUdpPort = DHCP_CLIENT_PORT;
    drv_acl_rule_for_copy2cpu_create(TRUE, ACL_TRUST_UDP_SPORT, &uiUdpPort);
    uiUdpPort = DHCP_SERVER_PORT; 
    drv_acl_rule_for_copy2cpu_create(TRUE, ACL_TRUST_UDP_DPORT, &uiUdpPort);

    /* Copy broadcast dhcp ack frame to cpu. */
    uiUdpPort = DHCP_SERVER_PORT;
    drv_acl_rule_for_copy2cpu_create(TRUE, ACL_TRUST_UDP_SPORT, &uiUdpPort);
    uiUdpPort = DHCP_CLIENT_PORT; 
    drv_acl_rule_for_copy2cpu_create(TRUE, ACL_TRUST_UDP_DPORT, &uiUdpPort);
	
    return RT_ERR_OK;
}

#ifdef CONFIG_KERNEL_RTK_ONUCOMM
/* For the packets with the specified dmac received from lan port, 
   add a acl to assign it priority 6 and trap it to cpu 
   specified dmac value is fixed,  for 9601b, it is 00e04c030405 */
int32 drv_acl_rule_special_packet_trp2cpu()
{
	int32 ret = RT_ERR_OK;
	uint32 uiAclRuleId=0;
	uint32 uiLPortId=0;
	uint32 uiPPort=0;
	uint32 uiPPortMask;	
	uint32 uiCpuPacketPri= 6;
	uint8  dstMacAdd[ETHER_ADDR_LEN] = {0x00,0xe0,0x4c,0x03,0x04,0x05};
	
	uiPPortMask = 0;
	FOR_EACH_LAN_PORT(uiLPortId)
    {
		uiPPort = PortLogic2PhyID(uiLPortId);
		if (INVALID_PORT != uiPPort)
			uiPPortMask |= (1U << uiPPort);
    }

	/* match dst mac */
	ret = drv_acl_rule_emptyid_get(&uiAclRuleId);
	   
	ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_DMAC, ACL_ACTION_TRAP_TO_CPU, dstMacAdd, dstMacAdd);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	ret = drv_acl_rule_cfg_create(uiAclRuleId, ACL_TRUST_DMAC, ACL_ACTION_PRIORITY_ASSIGN, dstMacAdd, &uiCpuPacketPri);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    ret = drv_acl_rule_bind(uiPPortMask, uiAclRuleId);
    if (RT_ERR_OK != ret) 
    {
    	drv_acl_rule_cfg_clear(uiAclRuleId);
        return RT_ERR_FAILED;
    }
	
	return RT_ERR_OK;
}
#endif