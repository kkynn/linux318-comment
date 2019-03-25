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

#include <common/error.h>
#include <common/rt_error.h>
#include <common/rt_type.h>
#include <rtk/vlan.h>
#include <rtk/svlan.h>
#include <rtk/qos.h>
#include <rtk/switch.h>
#include <osal/print.h>
#include <common/util/rt_util.h>
#include <hal/common/halctrl.h>
#include <hal/mac/reg.h>

#include <sys_def.h>
#include <drv_vlan.h>
#include <drv_acl.h>
#include <ctc_wrapper.h>
#include "drv_clf.h"
#include "ctc_sfu.h"

/*For qos 802.1p priority remarking because priority of vlan egress keep-tag is higher than priority of acl 1P remarking,
   should add a clf rule to convert ctag to stag when vlan mode of the port is transparent. */
static int qos_transparentPortClf[MAX_PORT_NUM];

/* siyuan 20161109: for vlan aggregation mode we use rtk_svlan_dmacVidSelState_set function, 
   so downstream packet with default vlan will ignore vlan untag member port and directly use vid found from l2 table. 
   should add a clf rule to untag downstream packet with default vlan */
static int aggregationPortClf[MAX_PORT_NUM];

void ctc_port_transparent_mode_clf_init()
{
	uint32 lport;

    FOR_EACH_LAN_PORT(lport)
    {
    	qos_transparentPortClf[lport] = CLF_RULE_ID_IVALLID;
    }

	/* for transparent mode,  must not add a vlan tag to downstream untag packet */
	{
		int32 ret;
		uint32 uiClfRuleId;
		rtk_classify_cfg_t stClfCfg;
		uint32 value;
		
		/* downstream filter untag packet */
		ret = drv_clf_rule_emptyid_get(&uiClfRuleId, CTC_SFU_CFTYPE_GENERIC);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}
		
		/* Filter S tagged */
		value = 0;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		/* Filter c tagged */
		value = 0;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_DS;
		/* downstream action delete stag */
	    stClfCfg.act.dsAct.csAct = CLASSIFY_DS_CSACT_DEL_STAG;

		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, CTC_SFU_CF_ALLPORTS, CTC_SFU_CFUSAGE_VLAN);
	}
}

int32 ctc_port_transparent_mode_add_clf(uint32 uiLPort)
{
	int32 ret;
	uint32 uiClfRuleId;
	uint32 phyPort;
	rtk_classify_cfg_t stClfCfg;
	uint32 value;
	
	if ((!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }
	
	phyPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	uiClfRuleId = qos_transparentPortClf[uiLPort];
	if (CLF_RULE_ID_IVALLID == uiClfRuleId)
	{	
		/* Upstream filter incoming port */
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

		/* Filter c tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_CTAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}
		
		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_US;
	    /* Upstream action delete ctag */
	    stClfCfg.act.usAct.cAct = CLASSIFY_US_CACT_DEL_CTAG;
	    
		/* Upstream action stag value copy from ctag*/
		stClfCfg.act.usAct.csAct = CLASSIFY_US_CSACT_ADD_TAG_8100;
		stClfCfg.act.usAct.csVidAct = CLASSIFY_US_VID_ACT_FROM_1ST_TAG;
		stClfCfg.act.usAct.csPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, uiLPort, CTC_SFU_CFUSAGE_VLAN);
		
		qos_transparentPortClf[uiLPort] = uiClfRuleId;
	}

	return RT_ERR_OK;
}

int32 ctc_port_transparent_mode_del_clf(uint32 uiLPort)
{
	int32 ret;
	uint32 uiClfRuleId;

	if ((!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }
	
	uiClfRuleId = qos_transparentPortClf[uiLPort];
	if (CLF_RULE_ID_IVALLID != uiClfRuleId)
	{
		/*Delete clf rule of this port.*/
		ret = drv_clf_rule_unbind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{
			printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, uiClfRuleId);
			return RT_ERR_FAILED;
		}
		qos_transparentPortClf[uiLPort] = CLF_RULE_ID_IVALLID;
	}

	return RT_ERR_OK;
}

void ctc_port_aggregation_mode_clf_init()
{
	uint32 lport;

    FOR_EACH_LAN_PORT(lport)
    {
    	aggregationPortClf[lport] = CLF_RULE_ID_IVALLID;
    }
}

int32 ctc_port_aggregation_mode_add_clf(uint32 uiLPort, unsigned int vlan)
{
	int32 ret;
	uint32 uiClfRuleId;
	uint32 phyPort;
	rtk_classify_cfg_t stClfCfg;
	uint32 value;
	
	if ((!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }
	
	phyPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == phyPort)
		return RT_ERR_FAILED;
	
	uiClfRuleId = aggregationPortClf[uiLPort];
	if (CLF_RULE_ID_IVALLID == uiClfRuleId)
	{	
		/* Downstream filter incoming port */
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

		/* Filter c tagged */
		value = 1;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_IS_STAG, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}

		/* Filter tag VID */
		value = vlan;
		ret = drv_clf_rule_create(uiClfRuleId, CLASSIFY_FIELD_TAG_VID, (void *)&value);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED; 
		}
		
		drv_clf_cfg_get(uiClfRuleId, (void *)&stClfCfg);
		stClfCfg.direction = CLASSIFY_DIRECTION_DS;
	    /* Downstream action delete ctag */
	    stClfCfg.act.dsAct.cAct= CLASSIFY_DS_CACT_DEL_CTAG;
		
		drv_clf_cfg_set(uiClfRuleId, (void *)&stClfCfg);

		ret = drv_clf_rule_bind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{		
			return RT_ERR_FAILED;
		}

		drv_clf_resouce_set(uiClfRuleId, uiLPort, CTC_SFU_CFUSAGE_VLAN);
		
		aggregationPortClf[uiLPort] = uiClfRuleId;
	}

	return RT_ERR_OK;
}

int32 ctc_port_aggregation_mode_del_clf(uint32 uiLPort)
{
	int32 ret;
	uint32 uiClfRuleId;

	if ((!IsValidLgcPort(uiLPort)))
    {
        return RT_ERR_INPUT;
    }
	
	uiClfRuleId = aggregationPortClf[uiLPort];
	if (CLF_RULE_ID_IVALLID != uiClfRuleId)
	{
		/*Delete clf rule of this port.*/
		ret = drv_clf_rule_unbind(uiClfRuleId);
		if (RT_ERR_OK != ret)
		{
			printf("[%s %d] delete clf rule %d fail\n", __func__, __LINE__, uiClfRuleId);
			return RT_ERR_FAILED;
		}
		aggregationPortClf[uiLPort] = CLF_RULE_ID_IVALLID;
	}

	return RT_ERR_OK;
}

void ctc_port_vlan_clf_init()
{
	ctc_port_transparent_mode_clf_init();
	ctc_port_aggregation_mode_clf_init();
}

static int32 ctc_vlan_check_vid_used(uint32 uiLPort, uint32 uiVid, int32 *pFlag)
{
	int32 ret;
	uint32 i, j;
	uint32 uiLPortTmp;
	uint32 uiOldVlanId;
	uint32 uiNewVlanId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;

	if ((!IsValidLgcPort(uiLPort)) || 
		(NULL == pFlag) || 
		((0 != uiVid) && (!VALID_VLAN_ID(uiVid))))
	{
		return RT_ERR_INPUT;
	}

	*pFlag = FALSE;

	/*check if the vid is used by other port*/
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		if (CTC_VLAN_MODE_TAG == pstCtcVlanMode->vlanMode)
		{
			if (uiVid == pstCtcVlanMode->cfg.tagCfg.vid)
			{
				*pFlag = TRUE;
				break;
			}
		}
		else if (CTC_VLAN_MODE_TRANSLATION == pstCtcVlanMode->vlanMode)
		{
			if (uiVid == pstCtcVlanMode->cfg.transCfg.defVlan.vid)
			{
				*pFlag = TRUE;
				break;
			}

			for (i = 0; i < pstCtcVlanMode->cfg.transCfg.num; i++)
			{
				uiOldVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].oriVlan.vid;
				uiNewVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid;

				if ((uiVid == uiOldVlanId) || (uiVid == uiNewVlanId))
				{
					*pFlag = TRUE;
					break;
				}
			}

			if (TRUE == *pFlag)
			{
				break;
			}
		}
		else if (CTC_VLAN_MODE_AGGREGATION == pstCtcVlanMode->vlanMode)
		{
			if (uiVid == pstCtcVlanMode->cfg.aggreCfg.defVlan.vid)
			{
				*pFlag = TRUE;
				break;
			}
			
			for(i = 0 ; i < pstCtcVlanMode->cfg.aggreCfg.tableNum ; i++)
			{
				if (uiVid == pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid)
				{
					*pFlag = TRUE;
					break;
				}
				
				for (j = 0; j < pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].entryNum; j++)
				{
					if (uiVid == pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid)
					{
						*pFlag = TRUE;
						break;
					}
				}
			}

			if (TRUE == *pFlag)
				break;
		}
		else if (CTC_VLAN_MODE_TRUNK == pstCtcVlanMode->vlanMode)
		{
			if (uiVid == pstCtcVlanMode->cfg.trunkCfg.defVlan.vid)
			{
				*pFlag = TRUE;
				break;
			}

			for (i = 0; i< pstCtcVlanMode->cfg.trunkCfg.num; i++)
			{
				if (uiVid == pstCtcVlanMode->cfg.trunkCfg.acceptVlan[i].vid)
				{
					*pFlag = TRUE;
					break;
				}
			}
			
			if (TRUE == *pFlag)
				break;
		}
	}

    /*check if the vid is used by management IP */
    if (is_vid_used_by_mxuMng(uiVid))
        *pFlag = TRUE;

	return RT_ERR_OK;
}

static int32 ctc_port_transparent_mode_set(uint32 uiLPort)
{
	int32 ret;
	uint32 i;
	uint32 uiNewVlanId;
	uint32 uiPPort;
	uint32 uiLPortTmp;
	ctc_wrapper_vlanCfg_t *pstVlanMode;
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stPhyMaskUntag;

	if (!IsValidLgcPort(uiLPort))
	{
		return RT_ERR_INPUT;
	}

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;

	/*set svlan 0 entry*/ 
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());	
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());

	ret = drv_svlan_member_add(0, 0, stPhyMask, stPhyMaskUntag, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*set default svlan id*/
	ret = drv_svlan_port_svid_set(uiLPort, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*set default cvlan entry*/ 
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
	
	ret = drv_vlan_member_add(0, stPhyMask, stPhyMaskUntag);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*must set extPort(0-5) for port-based vlan */
	RTK_PORTMASK_RESET(stPhyMask);
	for(i = 0; i < 6; i++)
	{
		RTK_PORTMASK_PORT_SET(stPhyMask, i);
	}
	ret = rtk_vlan_extPort_set(0, &stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*set default cvlan id*/
	ret = drv_vlan_port_pvid_set(uiLPort, 0, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*Keep transparent Port upstream packet format, packet from transparent Port 
	  will not add S-tag in uplink port. */
	ret = drv_vlan_egress_keeptag_ingress_enable_set(LOGIC_PON_PORT, uiLPort);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*Keep uplink Port downstream to transparent Port packet format*/	
	ret = drv_vlan_egress_keeptag_ingress_enable_set(uiLPort, LOGIC_PON_PORT);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*disable ingress filter for upstream packet only*/
	ret = drv_vlan_port_ingressfilter_set(uiLPort, FALSE);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*renew non-transparent port vlan member*/
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstVlanMode);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
		{
			/*add svlan entry member for tag port's pvid*/
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_RESET(stPhyMaskUntag);
			
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			
			ret = drv_svlan_member_add(pstVlanMode->cfg.tagCfg.vid, pstVlanMode->cfg.tagCfg.pri, stPhyMask, stPhyMaskUntag, 0);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add cvlan entry member for tag port's pvid*/
			ret = drv_vlan_member_add(pstVlanMode->cfg.tagCfg.vid, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}
		}
		else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
		{
			/*add svlan entry member for port's pvid*/
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_RESET(stPhyMaskUntag);
			
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			
			ret = drv_svlan_member_add(pstVlanMode->cfg.transCfg.defVlan.vid, pstVlanMode->cfg.transCfg.defVlan.pri, stPhyMask, stPhyMaskUntag, 0);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add cvlan entry member for port's pvid*/			
			ret = drv_vlan_member_add(pstVlanMode->cfg.transCfg.defVlan.vid, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add cvlan entry member for tag port's pvid*/
			ret = drv_vlan_member_add(pstVlanMode->cfg.transCfg.defVlan.vid, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}
			
			for (i = 0; i < pstVlanMode->cfg.transCfg.num; i++)
			{
				uiNewVlanId = pstVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid;

				/*add svlan entry member for tag port's new vid*/
				ret = drv_svlan_member_add(uiNewVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}

				/*add cvlan entry member for tag port's pvid*/
				ret = drv_vlan_member_add(uiNewVlanId, stPhyMask, stPhyMaskUntag);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
			}
		}
		else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
		{
			/*add svlan entry member for port's pvid*/
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_RESET(stPhyMaskUntag);
			
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			
			ret = drv_svlan_member_add(pstVlanMode->cfg.aggreCfg.defVlan.vid, pstVlanMode->cfg.aggreCfg.defVlan.pri, stPhyMask, stPhyMaskUntag, 0);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add cvlan entry member for port's pvid*/			
			ret = drv_vlan_member_add(pstVlanMode->cfg.aggreCfg.defVlan.vid, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			for (i =0; i< pstVlanMode->cfg.aggreCfg.tableNum; i++)
			{
				uiNewVlanId = pstVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid;

				/*add svlan entry member for tag port's new vid*/
				ret = drv_svlan_member_add(uiNewVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
				/* siyuan 20161109: aggregation  aggreToVlan don't have cvlan 
				   FIXME: aggreFromVlan not add transparent port now */
			#if 0		
				/*add cvlan entry member for tag port's pvid*/
				ret = drv_vlan_member_add(uiNewVlanId, stPhyMask, stPhyMaskUntag);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
			#endif
			}
		}
		else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
		{
			/*add svlan entry member for port's pvid*/
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_RESET(stPhyMaskUntag);
			
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
			
			ret = drv_svlan_member_add(pstVlanMode->cfg.trunkCfg.defVlan.vid, pstVlanMode->cfg.trunkCfg.defVlan.pri, stPhyMask, stPhyMaskUntag, 0);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add cvlan entry member for port's pvid*/			
			ret = drv_vlan_member_add(pstVlanMode->cfg.trunkCfg.defVlan.vid, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			for (i =0; i< pstVlanMode->cfg.trunkCfg.num; i++)
			{
				uiNewVlanId = pstVlanMode->cfg.trunkCfg.acceptVlan[i].vid;

				/*add svlan entry member for tag port's new vid*/
				ret = drv_svlan_member_add(uiNewVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
				
				/*add cvlan entry member for tag port's pvid*/
				ret = drv_vlan_member_add(uiNewVlanId, stPhyMask, stPhyMaskUntag);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
			}
		}
		else
		{
			/*Keep packet format egress from this port*/	
			ret = drv_vlan_egress_keeptag_ingress_enable_set(uiLPort, uiLPortTmp);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*Keep packet format ingress into this port and egress from other transparent port*/	
			ret = drv_vlan_egress_keeptag_ingress_enable_set(uiLPortTmp, uiLPort);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}
		}
	}

	(void)ctc_sfu_vlanCfg_set(uiLPort, CTC_VLAN_MODE_TRANSPARENT, NULL);

	/*Add clf rule of this port.*/
	ctc_port_transparent_mode_add_clf(uiLPort);

	return RT_ERR_OK;
	}


static int32 ctc_port_transparent_mode_reset(uint32 uiLPort)
{
	int32 ret;
	uint32 i, j;
    uint32 uiOldVlanId;
    uint32 uiNewVlanId;
	uint32 uiPPort;
    uint32 uiLPortTmp;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
    rtk_portmask_t stPhyMask;

    if (!IsValidLgcPort(uiLPort))
    {
        return RT_ERR_INPUT;
    }

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    
    RTK_PORTMASK_RESET(stPhyMask);
	
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
    /*resset svlan 0 entry*/ 
    ret = drv_svlan_member_remove(0, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*retset default cvlan entry*/ 
    ret = drv_vlan_member_remove(0, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*undo upstream packet transparent. */
    ret = drv_vlan_egress_keeptag_ingress_disable_set(LOGIC_PON_PORT, uiLPort);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*undo downstream packet transparent*/	
    ret = drv_vlan_egress_keeptag_ingress_disable_set(uiLPort, LOGIC_PON_PORT);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*enable ingress filter*/
    ret = drv_vlan_port_ingressfilter_set(uiLPort, TRUE);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
    
    /*renew non-transparent port vlan member*/
    FOR_EACH_LAN_PORT(uiLPortTmp)
    {
        if (uiLPortTmp == uiLPort)
        {
            continue;
        }

        ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        if (CTC_VLAN_MODE_TAG == pstCtcVlanMode->vlanMode)
        {
            /*remove svlan entry member for tag port's pvid*/
            ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.tagCfg.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            /*remove cvlan entry member for tag port's pvid*/
            ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.tagCfg.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }
        }
        else if (CTC_VLAN_MODE_TRANSLATION == pstCtcVlanMode->vlanMode)
        {
            /*remove svlan entry member for tag port's pvid*/
            ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.transCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            /*remove cvlan entry member for tag port's pvid*/
            ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.transCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }
            
            for (i = 0; i < pstCtcVlanMode->cfg.transCfg.num; i++)
            {
                uiOldVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].oriVlan.vid;
                uiNewVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid;

                /*remove cvlan entry member for tag port's old vid*/
                ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
                if (RT_ERR_OK != ret)
                {
                    return RT_ERR_FAILED;
                }

                /*remove svlan entry member for tag port's new vid*/
                ret = drv_svlan_member_remove(uiNewVlanId, stPhyMask);
                if (RT_ERR_OK != ret)
                {
                    return RT_ERR_FAILED;
                }
            }
        }
		else if (CTC_VLAN_MODE_AGGREGATION == pstCtcVlanMode->vlanMode)
		{
            /*remove svlan entry member for tag port's pvid*/
            ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.aggreCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            /*remove cvlan entry member for tag port's pvid*/
            ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.aggreCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

			for (i = 0; i < pstCtcVlanMode->cfg.aggreCfg.tableNum; i++)
			{
                uiNewVlanId = pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid;

                /*remove svlan entry member for tag port's new vid*/
                ret = drv_svlan_member_remove(uiNewVlanId, stPhyMask);
                if (RT_ERR_OK != ret)
                {
                    return RT_ERR_FAILED;
                }

				for (j = 0; j < pstCtcVlanMode->cfg.aggreCfg.tableNum; j++)
				{
					uiOldVlanId = pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid;
					
					/*remove cvlan entry member for tag port's old vid*/
					ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
					if (RT_ERR_OK != ret)
					{
						return RT_ERR_FAILED;
					}
				}
			}
		}
		else if (CTC_VLAN_MODE_TRUNK == pstCtcVlanMode->vlanMode)
		{
            /*remove svlan entry member for tag port's pvid*/
            ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.trunkCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

            /*remove cvlan entry member for tag port's pvid*/
            ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.aggreCfg.defVlan.vid, stPhyMask);
            if (RT_ERR_OK != ret)
            {
                return RT_ERR_FAILED;
            }

			for (i = 0; i < pstCtcVlanMode->cfg.trunkCfg.num; i++)
			{
				uiOldVlanId = pstCtcVlanMode->cfg.trunkCfg.acceptVlan[i].vid;

				/*remove cvlan entry member for tag port's old vid*/
				ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
				
				/*remove svlan entry member for tag port's new vid*/
				ret = drv_svlan_member_remove(uiOldVlanId, stPhyMask);
				if (RT_ERR_OK != ret)
				{
					return RT_ERR_FAILED;
				}
			}
		}

        /*undo packet transparent egress from this port*/	
        ret = drv_vlan_egress_keeptag_ingress_disable_set(uiLPort, uiLPortTmp);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*undo packet transparent ingress into this port*/	
        ret = drv_vlan_egress_keeptag_ingress_disable_set(uiLPortTmp, uiLPort);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

	/*Delete clf rule of this port.*/
	ctc_port_transparent_mode_del_clf(uiLPort);
	
    return RT_ERR_OK;
}

static int32 ctc_port_translation_mode_set(uint32 uiLPort, ctc_wrapper_vlanTransCfg_t *pTransCfg)
{
	int32 ret;
    uint32 i;
    uint32 uiOldVlanId;
    uint32 uiNewVlanId;
    uint32 uiPPort;
    uint32 uiLPortTmp;
    uint32 uiAclId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
    rtk_portmask_t stPhyMask;
    rtk_portmask_t stPhyMaskUntag;
    rtk_portmask_t stTranspPhyMask;

    if ((!IsValidLgcPort(uiLPort)) || (NULL == pTransCfg))
    {
        return RT_ERR_INPUT;
    }

    /* get transparent port mask */
    RTK_PORTMASK_RESET(stTranspPhyMask);
    FOR_EACH_LAN_PORT(uiLPortTmp)
    {
        if (uiLPortTmp == uiLPort)
        {
            continue;
        }

        (void)ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		
        if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
        {
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);            
        }
    }

    /*set default svlan entry*/
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());	
    RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	
    ret = drv_svlan_member_add(pTransCfg->defVlan.vid, pTransCfg->defVlan.pri, stPhyMask, stPhyMaskUntag, 0);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*set default svlan id*/
    ret = drv_svlan_port_svid_set(uiLPort, pTransCfg->defVlan.vid);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*create default cvlan*/
	ret = drv_vlan_entry_create(pTransCfg->defVlan.vid, VLAN_FID_SVL);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
    {
        return RT_ERR_FAILED;
    }

    /*set default cvlan entry*/ 
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));    
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	/* siyuan 2016-10-11: add cpu port to cvlan for lanside icmp packet trap to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
    RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
	
    ret = drv_vlan_member_add(pTransCfg->defVlan.vid, stPhyMask, stPhyMaskUntag);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*must set extPort(0-5) for port-based vlan */
	RTK_PORTMASK_RESET(stPhyMask);
	for(i = 0; i < 6; i++)
	{
		RTK_PORTMASK_PORT_SET(stPhyMask, i);
	}
	ret = rtk_vlan_extPort_set(pTransCfg->defVlan.vid, &stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
    /*set default cvlan id*/
    ret = drv_vlan_port_pvid_set(uiLPort, pTransCfg->defVlan.vid, pTransCfg->defVlan.pri);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*renew transparent port vlan member*/
    for (i = 0; i < pTransCfg->num; i++)
    {
        uiOldVlanId = pTransCfg->transVlanPair[i].oriVlan.vid;
        uiNewVlanId = pTransCfg->transVlanPair[i].newVlan.vid;
		
        /* defvlan has been processed before */
        if ((uiOldVlanId == uiNewVlanId) &&
            (pTransCfg->defVlan.vid == uiNewVlanId))
        {
            continue;
        }

		/* create cvlan for translation pair */
		ret = drv_vlan_entry_create(uiOldVlanId, VLAN_FID_SVL);
		if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
	    {
	        return RT_ERR_FAILED;
	    }
        
        /*add cvlan entry member for port's old vid*/
        RTK_PORTMASK_RESET(stPhyMask);
    	RTK_PORTMASK_RESET(stPhyMaskUntag);

        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));    
    	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);        
        RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
		
        ret = drv_vlan_member_add(uiOldVlanId, stPhyMask, stPhyMaskUntag);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*add svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);
    	RTK_PORTMASK_RESET(stPhyMaskUntag);

        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);

        ret = drv_svlan_member_add(uiNewVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*add c2s entry*/
        ret = drv_port_c2s_entry_add(uiLPort, uiOldVlanId, uiNewVlanId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*add sp2c entry*/
        ret = drv_port_sp2c_entry_add(uiLPort, uiNewVlanId, uiOldVlanId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

    (void)ctc_sfu_vlanCfg_set(uiLPort, CTC_VLAN_MODE_TRANSLATION, pTransCfg);

    return RT_ERR_OK;
}

static int32 ctc_port_translation_mode_reset(uint32 uiLPort)
{
	int32 ret;
    int32 bFind = FALSE;
    uint32 i;
    uint32 uiPvid;
    uint32 uiOldVlanId;
    uint32 uiNewVlanId;
    uint32 uiPPort;
    uint32 uiLPortTmp;
    ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
    rtk_portmask_t stPhyMask;
    rtk_portmask_t stTranspPhyMask;

    if (!IsValidLgcPort(uiLPort))
    {
        return RT_ERR_INPUT;
    }

    /* get transparent port mask */
    RTK_PORTMASK_RESET(stTranspPhyMask);
    FOR_EACH_LAN_PORT(uiLPortTmp)
    {
        if (uiLPortTmp == uiLPort)
        {
            continue;
        }

        ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
        {
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
        }
    }

    ret = ctc_sfu_vlanCfg_get(uiLPort, &pstCtcVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    uiPvid = pstCtcVlanMode->cfg.transCfg.defVlan.vid;
    /*check if the pvid is used by other port*/
    ret = ctc_vlan_check_vid_used(uiLPort, uiPvid, &bFind);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
    
    /*renew default svlan entry*/
    RTK_PORTMASK_RESET(stPhyMask);

    /*pvid svlan entry will be deleted if it is not used by other port.*/
    if (FALSE == bFind)
    {
        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
    }
    
	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
    ret = drv_svlan_member_remove(uiPvid, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    ret = drv_vlan_member_remove(uiPvid, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    } 

	/*delete default cvlan*/
	if (FALSE == bFind)
    {
    	ret = drv_vlan_check_member_empty(pstCtcVlanMode->cfg.transCfg.defVlan.vid, &bFind);
		if ((RT_ERR_OK == ret) && (FALSE == bFind))
		{
	        ret = drv_vlan_entry_delete(pstCtcVlanMode->cfg.transCfg.defVlan.vid);
			if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
	        {
	            return RT_ERR_FAILED;
	        }
		}
    }
    
    /*renew translation port vlan member*/
    for (i = 0; i < pstCtcVlanMode->cfg.transCfg.num; i++)
    {
        uiOldVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].oriVlan.vid;/*cvlan*/
        uiNewVlanId = pstCtcVlanMode->cfg.transCfg.transVlanPair[i].newVlan.vid;/*svlan*/

        /*for access mode*/
        if ((uiOldVlanId == uiNewVlanId) &&
            (pstCtcVlanMode->cfg.transCfg.defVlan.vid == uiNewVlanId))
        {
            continue;
        }

        /*delete port from c2s and s2c entry*//*modify by shipeng 2013-04-28*/
		/* siyuan 2016-10-17: some olt will set different user vlan map to the same svlan,
		   which cause delete c2s or sp2c entry error, just ignore the error */
        ret = drv_port_c2s_entry_delete(uiLPort, uiOldVlanId, uiNewVlanId);
        //if (RT_ERR_OK != ret)
        //{
        //    return RT_ERR_FAILED;
        //}

        ret = drv_port_sp2c_entry_delete(uiLPort, uiNewVlanId);
        //if (RT_ERR_OK != ret)
        //{
        //    return RT_ERR_FAILED;
        //}

        /*renew cvlan entry member for port's old vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the old vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiOldVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the cvlan entry if it's no not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        }
        
        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

		/*delete cvlan uiOldVlanId*/
		if (FALSE == bFind)
	    {
	    	ret = drv_vlan_check_member_empty(uiOldVlanId, &bFind);
			if ((RT_ERR_OK == ret) && (FALSE == bFind))
			{
		        ret = drv_vlan_entry_delete(uiOldVlanId);
				if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
		        {
		            return RT_ERR_FAILED;
		        }
			}
	    }

        /*renew svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the new vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiNewVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the new svlan entry if it is not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        }

        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_svlan_member_remove(uiNewVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

    return RT_ERR_OK;
}

static int32 ctc_port_tag_mode_set(uint32 uiLPort, ctc_wrapper_vlan_t *pTagCfg)
{
	int32 ret;
	uint32 uiPPort;
    uint32 uiLPortTmp;
	uint32 uiAclId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
    rtk_portmask_t stPhyMask;
    rtk_portmask_t stPhyMaskUntag;
    rtk_portmask_t stTranspPhyMask;
	uint32 i;
	
    if ((!IsValidLgcPort(uiLPort)) || (NULL == pTagCfg))
    {
        return RT_ERR_INPUT;
    }

    /*get transparent port mask*/
    RTK_PORTMASK_RESET(stTranspPhyMask);
    FOR_EACH_LAN_PORT(uiLPortTmp)
    {
        if (uiLPortTmp == uiLPort)
        {
            continue;
        }

        ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
        {
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
        }
    }

    /*set default svlan entry*/
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
    memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
    RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	/* 2016-0523 siyuan: add cpu port to svlan member port in order to
	   allow downstream arp reply packet having this vlan tag to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
    RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);

    ret = drv_svlan_member_add(pTagCfg->vid, pTagCfg->pri, stPhyMask, stPhyMaskUntag, 0);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /*set default svlan id*/
    ret = drv_svlan_port_svid_set(uiLPort, pTagCfg->vid);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*create default cvlan*/
	ret = drv_vlan_entry_create(pTagCfg->vid, VLAN_FID_SVL);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
    {
        return RT_ERR_FAILED;
    }

    /*set default cvlan entry*/ 
    RTK_PORTMASK_RESET(stPhyMask);
    RTK_PORTMASK_RESET(stPhyMaskUntag);

    memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	/* siyuan 2016-10-11: add cpu port to cvlan for lanside icmp packet trap to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
    
    ret = drv_vlan_member_add(pTagCfg->vid, stPhyMask, stPhyMaskUntag);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

	/*must set extPort(0-5) for port-based vlan */
	RTK_PORTMASK_RESET(stPhyMask);
	for(i = 0; i < 6; i++)
	{
		RTK_PORTMASK_PORT_SET(stPhyMask, i);
	}
	ret = rtk_vlan_extPort_set(pTagCfg->vid, &stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
    /*set default cvlan id*/
    ret = drv_vlan_port_pvid_set(uiLPort, pTagCfg->vid, pTagCfg->pri);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    /* accept untag-only */
    rtk_vlan_portAcceptFrameType_set(uiPPort, ACCEPT_FRAME_TYPE_UNTAG_ONLY);

    (void)ctc_sfu_vlanCfg_set(uiLPort, CTC_VLAN_MODE_TAG, pTagCfg);

    return RT_ERR_OK;
}

static int32 ctc_port_tag_mode_reset(uint32 uiLPort)
{
	int32 ret;
	int32 bFind;
	uint32 uiPPort;
	uint32 uiLPortTmp;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stTranspPhyMask;
	
	if (!IsValidLgcPort(uiLPort))
    {
        return RT_ERR_INPUT;
    }

	/*get transparent port mask*/
	RTK_PORTMASK_RESET(stTranspPhyMask);
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
		{
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
		}
	}

	ret = ctc_sfu_vlanCfg_get(uiLPort, &pstCtcVlanMode);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*check if the pvid is used by other port*/
	ret = ctc_vlan_check_vid_used(uiLPort, pstCtcVlanMode->cfg.tagCfg.vid, &bFind);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*renew default svlan entry*/
	RTK_PORTMASK_RESET(stPhyMask);

	if (FALSE == bFind)
	{
		memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	}
	
	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
	ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.tagCfg.vid, stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*renew default cvlan entry*/ 
	ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.tagCfg.vid, stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*delete default cvlan*/
	if (FALSE == bFind)
	{
		ret = drv_vlan_check_member_empty(pstCtcVlanMode->cfg.tagCfg.vid, &bFind);
		if ((RT_ERR_OK == ret) && (FALSE == bFind))
		{
			ret = drv_vlan_entry_delete(pstCtcVlanMode->cfg.tagCfg.vid);
			if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
			{
				return RT_ERR_FAILED;
			}
		}
	}

    /* accept all */
    rtk_vlan_portAcceptFrameType_set(uiPPort, ACCEPT_FRAME_TYPE_ALL);


	return RT_ERR_OK;
}

static int32 ctc_port_aggregation_mode_set(uint32 uiLPort, ctc_wrapper_vlanAggreCfg_t *pAggreCfg)
{
	int32 ret;
	uint32 i, j;
	uint32 uiOldVlanId;
	uint32 uiNewVlanId;
	uint32 uiPPort;
	uint32 uiLPortTmp;
	uint32 uiAclId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stPhyMaskUntag;
	rtk_portmask_t stTranspPhyMask;

	if ((!IsValidLgcPort(uiLPort)) || (NULL == pAggreCfg))
	{
		return RT_ERR_INPUT;
	}

	/* get transparent port mask */
	RTK_PORTMASK_RESET(stTranspPhyMask);
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		(void)ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		
		if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
		{
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);			
		}
	}

	/*set default svlan entry*/
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());	
	/* 2016-0523 siyuan: add cpu port to svlan member port in order to
	   allow downstream arp reply packet having this vlan tag to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	
	ret = drv_svlan_member_add(pAggreCfg->defVlan.vid, 
							pAggreCfg->defVlan.pri, 
							stPhyMask, stPhyMaskUntag, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*set default svlan id*/
	ret = drv_svlan_port_svid_set(uiLPort, pAggreCfg->defVlan.vid);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*create default cvlan*/
	ret = drv_vlan_entry_create(pAggreCfg->defVlan.vid, VLAN_FID_SVL);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
	{
		return RT_ERR_FAILED;
	}

	/*set default cvlan entry*/ 
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));	 
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	/* siyuan 2016-10-11: add cpu port to cvlan for lanside icmp packet trap to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
	
	ret = drv_vlan_member_add(pAggreCfg->defVlan.vid, stPhyMask, stPhyMaskUntag);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*must set extPort(0-5) for port-based vlan */
	RTK_PORTMASK_RESET(stPhyMask);
	for(i = 0; i < 6; i++)
	{
		RTK_PORTMASK_PORT_SET(stPhyMask, i);
	}
	ret = rtk_vlan_extPort_set(pAggreCfg->defVlan.vid, &stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*set default cvlan id*/
	ret = drv_vlan_port_pvid_set(uiLPort, pAggreCfg->defVlan.vid, pAggreCfg->defVlan.pri);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*renew transparent port vlan member*/
	for (i = 0; i < pAggreCfg->tableNum; i++)
	{
		uiNewVlanId = pAggreCfg->aggrTbl[i].aggreToVlan.vid;

		/*add svlan entry member for port's new vid*/
		RTK_PORTMASK_RESET(stPhyMask);
		RTK_PORTMASK_RESET(stPhyMaskUntag);

		memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
		RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		/* 2016-0523 siyuan: add cpu port to svlan member port in order to
	   	   allow downstream arp reply packet having this vlan tag to cpu port */
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);

		ret = drv_svlan_member_add(uiNewVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}
		
		for (j = 0; j < pAggreCfg->aggrTbl[i].entryNum; j++)
		{
			uiOldVlanId = pAggreCfg->aggrTbl[i].aggreFromVlan[j].vid;
			
			/* defvlan has been processed before */
			if ((uiOldVlanId == uiNewVlanId) &&
				(pAggreCfg->defVlan.vid == uiNewVlanId))
			{
				continue;
			}

			/* create cvlan for translation pair */
			ret = drv_vlan_entry_create(uiOldVlanId, VLAN_FID_SVL);
			if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
			{
				return RT_ERR_FAILED;
			}
			
			/*add cvlan entry member for port's old vid*/
			RTK_PORTMASK_RESET(stPhyMask);
			RTK_PORTMASK_RESET(stPhyMaskUntag);

			memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));	 
			RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);		  
			RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
			RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
			RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
			
			ret = drv_vlan_member_add(uiOldVlanId, stPhyMask, stPhyMaskUntag);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}

			/*add c2s entry*/
			ret = drv_port_c2s_entry_add(uiLPort, uiOldVlanId, uiNewVlanId);
			if (RT_ERR_OK != ret)
			{
				return RT_ERR_FAILED;
			}
		}

	}
	
	rtk_svlan_dmacVidSelState_set(uiPPort, ENABLED);

	(void)ctc_sfu_vlanCfg_set(uiLPort, CTC_VLAN_MODE_AGGREGATION, pAggreCfg);

	/* siyuan 20161109: add clf rule to untag downstream packet with default vlan */
	ctc_port_aggregation_mode_add_clf(uiLPort, pAggreCfg->defVlan.vid);
	
	return RT_ERR_OK;
}

static int32 ctc_port_aggregation_mode_reset(uint32 uiLPort)
{
	int32 ret;
    int32 bFind = FALSE;
    uint32 i, j;
    uint32 uiPvid;
    uint32 uiOldVlanId;
    uint32 uiNewVlanId;
    uint32 uiPPort;
    uint32 uiLPortTmp;
    ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
    rtk_portmask_t stPhyMask;
    rtk_portmask_t stTranspPhyMask;

    if (!IsValidLgcPort(uiLPort))
    {
        return RT_ERR_INPUT;
    }

    /*get transparent port mask*/
    RTK_PORTMASK_RESET(stTranspPhyMask);
    FOR_EACH_LAN_PORT(uiLPortTmp)
    {
        if (uiLPortTmp == uiLPort)
        {
            continue;
        }

        ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
        {
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
        }
    }

    ret = ctc_sfu_vlanCfg_get(uiLPort, &pstCtcVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
	
    uiPvid = pstCtcVlanMode->cfg.aggreCfg.defVlan.vid;
    /*check if the pvid is used by other port*/
    ret = ctc_vlan_check_vid_used(uiLPort, uiPvid, &bFind);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
    
    /*renew default svlan entry*/
    RTK_PORTMASK_RESET(stPhyMask);

    /*pvid svlan entry will be deleted if it is not used by other port.*/
    if (FALSE == bFind)
    {
        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
    }
    
	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
    ret = drv_svlan_member_remove(uiPvid, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    ret = drv_vlan_member_remove(uiPvid, stPhyMask);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    } 

	/*delete default cvlan*/
	if (FALSE == bFind)
    {
    	ret = drv_vlan_check_member_empty(pstCtcVlanMode->cfg.aggreCfg.defVlan.vid, &bFind);
		if ((RT_ERR_OK == ret) && (FALSE == bFind))
		{
	        ret = drv_vlan_entry_delete(pstCtcVlanMode->cfg.aggreCfg.defVlan.vid);
			if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
	        {
	            return RT_ERR_FAILED;
	        }
		}
    }

	rtk_svlan_dmacVidSelState_set(uiPPort, DISABLED);
	
    /*renew translation port vlan member*/
    for (i = 0; i < pstCtcVlanMode->cfg.aggreCfg.tableNum; i++)
    {
        uiNewVlanId = pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreToVlan.vid;/*svlan*/

		for (j = 0; j < pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].entryNum; j++)
		{
			uiOldVlanId = pstCtcVlanMode->cfg.aggreCfg.aggrTbl[i].aggreFromVlan[j].vid;/*cvlan*/
			
	        /*for access mode, defvlan has been processed before*/
	        if ((uiOldVlanId == uiNewVlanId) &&
	            (pstCtcVlanMode->cfg.aggreCfg.defVlan.vid == uiNewVlanId))
	        {
	            continue;
	        }

	        /*delete port from c2s and s2c entry*//*modify by shipeng 2013-04-28*/
	        ret = drv_port_c2s_entry_delete(uiLPort, uiOldVlanId, uiNewVlanId);
	        if (RT_ERR_OK != ret)
	        {
	            return RT_ERR_FAILED;
	        }

	        /*renew cvlan entry member for port's old vid*/
	        RTK_PORTMASK_RESET(stPhyMask);

	        /*check if the old vid is used by other port*/
	        ret = ctc_vlan_check_vid_used(uiLPort, uiOldVlanId, &bFind);
	        if (RT_ERR_OK != ret)
	        {
	            return RT_ERR_FAILED;
	        }

	        /*delete the cvlan entry if it's no not used by any other port*/
	        if (FALSE == bFind)
	        {
	            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
	        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	        }
	        
	        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	        ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
	        if (RT_ERR_OK != ret)
	        {
	            return RT_ERR_FAILED;
	        }

			/*delete cvlan uiOldVlanId*/
			if (FALSE == bFind)
		    {
		    	ret = drv_vlan_check_member_empty(uiOldVlanId, &bFind);
				if ((RT_ERR_OK == ret) && (FALSE == bFind))
				{
			        ret = drv_vlan_entry_delete(uiOldVlanId);
					if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
			        {
			            return RT_ERR_FAILED;
			        }
				}
		    }
		}

        /*renew svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the new vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiNewVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the new svlan entry if it is not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
			RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
        }

        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_svlan_member_remove(uiNewVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

	ctc_port_aggregation_mode_del_clf(uiLPort);
		
    return RT_ERR_OK;
}

static int32 ctc_port_trunk_mode_set(uint32 uiLPort, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg)
{
	int32 ret;
	uint32 i;
	uint32 uiPPort;
	uint32 uiLPortTmp;
	uint32 uiAclId;
	uint32 uiOldVlanId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stPhyMaskUntag;
	rtk_portmask_t stTranspPhyMask;

	if ((!IsValidLgcPort(uiLPort)) || (NULL == pTrunkCfg))
	{
		return RT_ERR_INPUT;
	}

	/*get transparent port mask*/
	RTK_PORTMASK_RESET(stTranspPhyMask);
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
		{
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
		}
	}

	/*set default svlan entry*/
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());	
	/* 2016-0523 siyuan: add cpu port to svlan member port in order to
	   allow downstream arp reply packet having this vlan tag to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);

	ret = drv_svlan_member_add(pTrunkCfg->defVlan.vid, 
								pTrunkCfg->defVlan.pri, 
								stPhyMask, stPhyMaskUntag, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*set default svlan id*/
	ret = drv_svlan_port_svid_set(uiLPort, pTrunkCfg->defVlan.vid);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* siyuan 20160930:should remove trunk lan port from svlan 0 member port,
	   in order to drop downstream tag packet which not belong to trunk vlan group 
	   and svlan unmatch action assigns svlan 0 to this packet*/
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
	ret = drv_svlan_member_remove(0, stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*create default cvlan*/
	ret = drv_vlan_entry_create(pTrunkCfg->defVlan.vid, VLAN_FID_SVL);
	if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
	{
		return RT_ERR_FAILED;
	}

	/*set default cvlan entry*/ 
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);

	memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
	/* siyuan 2016-10-11: add cpu port to cvlan for lanside icmp packet trap to cpu port */
	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, uiPPort);
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
	RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
	
	ret = drv_vlan_member_add(pTrunkCfg->defVlan.vid, stPhyMask, stPhyMaskUntag);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*must set extPort(0-5) for port-based vlan */
	RTK_PORTMASK_RESET(stPhyMask);
	for(i = 0; i < 6; i++)
	{
		RTK_PORTMASK_PORT_SET(stPhyMask, i);
	}
	ret = rtk_vlan_extPort_set(pTrunkCfg->defVlan.vid, &stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*set default cvlan id*/
	ret = drv_vlan_port_pvid_set(uiLPort, pTrunkCfg->defVlan.vid, pTrunkCfg->defVlan.pri);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

    /*renew transparent port vlan member*/
    for (i = 0; i < pTrunkCfg->num; i++)
    {
        uiOldVlanId = pTrunkCfg->acceptVlan[i].vid;
		
        /* defvlan has been processed before */
        if (pTrunkCfg->defVlan.vid == uiOldVlanId)
        {
            continue;
        }

		/* create cvlan for translation pair */
		ret = drv_vlan_entry_create(uiOldVlanId, VLAN_FID_SVL);
		if ((RT_ERR_VLAN_EXIST != ret) && (RT_ERR_OK != ret))
	    {
	        return RT_ERR_FAILED;
	    }
        
        /*add cvlan entry member for port's old vid*/
        RTK_PORTMASK_RESET(stPhyMask);
    	RTK_PORTMASK_RESET(stPhyMaskUntag);

        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));    
    	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);        
        RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
		
        ret = drv_vlan_member_add(uiOldVlanId, stPhyMask, stPhyMaskUntag);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*add svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);
    	RTK_PORTMASK_RESET(stPhyMaskUntag);

        memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		/* 2016-0523 siyuan: add cpu port to svlan member port in order to
	       allow downstream arp reply packet having this vlan tag to cpu port */
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
		
        ret = drv_svlan_member_add(uiOldVlanId, 0, stPhyMask, stPhyMaskUntag, 0);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
		
        /*add c2s entry*/
        ret = drv_port_c2s_entry_add(uiLPort, uiOldVlanId, uiOldVlanId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

	(void)ctc_sfu_vlanCfg_set(uiLPort, CTC_VLAN_MODE_TRUNK, pTrunkCfg);

	return RT_ERR_OK;
}

static int32 ctc_port_trunk_mode_reset(uint32 uiLPort)
{
	int32 ret;
	int32 bFind;
	uint32 i;
	uint32 uiPPort;
	uint32 uiLPortTmp;
	uint32 uiOldVlanId;
	ctc_wrapper_vlanCfg_t *pstCtcVlanMode;
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stTranspPhyMask;
	rtk_portmask_t stPhyMaskUntag;
	
	if (!IsValidLgcPort(uiLPort))
	{
		return RT_ERR_INPUT;
	}

	/*get transparent port mask*/
	RTK_PORTMASK_RESET(stTranspPhyMask);
	FOR_EACH_LAN_PORT(uiLPortTmp)
	{
		if (uiLPortTmp == uiLPort)
		{
			continue;
		}

		ret = ctc_sfu_vlanCfg_get(uiLPortTmp, &pstCtcVlanMode);
		if (RT_ERR_OK != ret)
		{
			return RT_ERR_FAILED;
		}

		if (CTC_VLAN_MODE_TRANSPARENT == pstCtcVlanMode->vlanMode)
		{
			uiPPort = PortLogic2PhyID(uiLPortTmp);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stTranspPhyMask, uiPPort);
		}
	}

	ret = ctc_sfu_vlanCfg_get(uiLPort, &pstCtcVlanMode);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*check if the pvid is used by other port*/
	ret = ctc_vlan_check_vid_used(uiLPort, pstCtcVlanMode->cfg.trunkCfg.defVlan.vid, &bFind);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*renew default svlan entry*/
	RTK_PORTMASK_RESET(stPhyMask);

	if (FALSE == bFind)
	{
		memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
	}
	
	uiPPort = PortLogic2PhyID(uiLPort);
	if (INVALID_PORT == uiPPort)
		return RT_ERR_FAILED;
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
	ret = drv_svlan_member_remove(pstCtcVlanMode->cfg.trunkCfg.defVlan.vid, stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/* siyuan 20160930:should remove trunk lan port from svlan 0 member port,
	   in order to drop downstream tag packet which not belong to trunk vlan group 
	   and svlan unmatch action assigns svlan 0 to this packet*/
	RTK_PORTMASK_RESET(stPhyMask);
	RTK_PORTMASK_RESET(stPhyMaskUntag);
	RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
	
	ret = drv_svlan_member_add(0, 0, stPhyMask, stPhyMaskUntag, 0);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}
	
	/*renew default cvlan entry*/ 
	ret = drv_vlan_member_remove(pstCtcVlanMode->cfg.trunkCfg.defVlan.vid, stPhyMask);
	if (RT_ERR_OK != ret)
	{
		return RT_ERR_FAILED;
	}

	/*delete default cvlan*/
	if (FALSE == bFind)
	{
		ret = drv_vlan_check_member_empty(pstCtcVlanMode->cfg.trunkCfg.defVlan.vid, &bFind);
		if ((RT_ERR_OK == ret) && (FALSE == bFind))
		{
			ret = drv_vlan_entry_delete(pstCtcVlanMode->cfg.trunkCfg.defVlan.vid);
			if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
			{
				return RT_ERR_FAILED;
			}
		}
	}
	
    /*renew translation port vlan member*/
    for (i = 0; i < pstCtcVlanMode->cfg.trunkCfg.num; i++)
    {
        uiOldVlanId = pstCtcVlanMode->cfg.trunkCfg.acceptVlan[i].vid;/*cvlan*/

        /*for access mode*/
        if (pstCtcVlanMode->cfg.trunkCfg.defVlan.vid == uiOldVlanId)
        {
            continue;
        }

        /*delete port from c2s and s2c entry*/
        ret = drv_port_c2s_entry_delete(uiLPort, uiOldVlanId, uiOldVlanId);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

		/*renew svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the new vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiOldVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the new svlan entry if it is not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
			RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
        }

        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_svlan_member_remove(uiOldVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
		
        /*renew cvlan entry member for port's old vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the old vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiOldVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the cvlan entry if it's no not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        }
        
        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_vlan_member_remove(uiOldVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

		/*delete cvlan uiOldVlanId*/
		if (FALSE == bFind)
	    {
	    	ret = drv_vlan_check_member_empty(uiOldVlanId, &bFind);
			if ((RT_ERR_OK == ret) && (FALSE == bFind))
			{
		        ret = drv_vlan_entry_delete(uiOldVlanId);
				if ((RT_ERR_VLAN_ENTRY_NOT_FOUND != ret) && (RT_ERR_OK != ret))
		        {
		            return RT_ERR_FAILED;
		        }
			}
	    }

        /*renew svlan entry member for port's new vid*/
        RTK_PORTMASK_RESET(stPhyMask);

        /*check if the new vid is used by other port*/
        ret = ctc_vlan_check_vid_used(uiLPort, uiOldVlanId, &bFind);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }

        /*delete the new svlan entry if it is not used by any other port*/
        if (FALSE == bFind)
        {
            memcpy(&stPhyMask, &stTranspPhyMask, sizeof(rtk_portmask_t));
        	RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
        }

        RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
        ret = drv_svlan_member_remove(uiOldVlanId, stPhyMask);
        if (RT_ERR_OK != ret)
        {
            return RT_ERR_FAILED;
        }
    }

	return RT_ERR_OK;
}

int32 ctc_transparent_vlan_set(uint32 lport)
{
    int32 ret = RT_ERR_OK;
    ctc_wrapper_vlanCfg_t *pstVlanMode;

    if (!IsValidLgcPort(lport))
    {
        return RT_ERR_FAILED;
    }

    ret = ctc_sfu_vlanCfg_get(lport, &pstVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
 
    if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
    {
        ret = ctc_port_tag_mode_reset(lport);/*reset tag*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
    else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
    {
        ret = ctc_port_translation_mode_reset(lport);/*reset translation*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{
		ret = ctc_port_aggregation_mode_reset(lport);/*reset aggregation*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		ret = ctc_port_trunk_mode_reset(lport);
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
    else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
    {
    	ret = ctc_port_transparent_mode_reset(lport);
        if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
    }

    ret = ctc_port_transparent_mode_set(lport);/*set transparent*/
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 ctc_translation_vlan_set(uint32 lport, ctc_wrapper_vlanTransCfg_t *pTransCfg)
{
    int32 ret = RT_ERR_OK;
    ctc_wrapper_vlanCfg_t *pstVlanMode;

    if (!IsValidLgcPort(lport) || (NULL == pTransCfg))
    {
        return RT_ERR_FAILED;
    }

    ret = ctc_sfu_vlanCfg_get(lport, &pstVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
 
    if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
    {
        ret = ctc_port_tag_mode_reset(lport);/*reset tag*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
    else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
    {
        ret = ctc_port_translation_mode_reset(lport);/*reset translation*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{
		ret = ctc_port_aggregation_mode_reset(lport);/*reset aggregation*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		ret = ctc_port_trunk_mode_reset(lport);/*reset trunk*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
    else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
    {
        ret = ctc_port_transparent_mode_reset(lport);/*reset transparent*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
    }

    ret = ctc_port_translation_mode_set(lport, pTransCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 ctc_tag_vlan_set(uint32 lport, ctc_wrapper_vlan_t *pTagCfg)
{
    int32 ret = RT_ERR_OK;
    ctc_wrapper_vlanCfg_t *pstVlanMode;

    if (!IsValidLgcPort(lport) || (NULL == pTagCfg))
    {
        return RT_ERR_FAILED;
    }

    ret = ctc_sfu_vlanCfg_get(lport, &pstVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
 
    if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
    {
        ret = ctc_port_tag_mode_reset(lport);/*reset tag*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
    else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
    {
        ret = ctc_port_translation_mode_reset(lport);/*reset translation*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{
		ret = ctc_port_aggregation_mode_reset(lport);/*reset aggregation*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		ret = ctc_port_trunk_mode_reset(lport);/*reset trunk*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
    else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
    {
        ret = ctc_port_transparent_mode_reset(lport);/*reset transparent*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
    }

    ret = ctc_port_tag_mode_set(lport, pTagCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 ctc_aggregation_vlan_set(uint32 lport, ctc_wrapper_vlanAggreCfg_t *pAggreCfg)
{
    int32 ret = RT_ERR_OK;
    ctc_wrapper_vlanCfg_t *pstVlanMode;

    if (!IsValidLgcPort(lport) || (NULL == pAggreCfg))
    {
        return RT_ERR_FAILED;
    }

    ret = ctc_sfu_vlanCfg_get(lport, &pstVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
 
    if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
    {
        ret = ctc_port_tag_mode_reset(lport);/*reset tag*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
    else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
    {
        ret = ctc_port_translation_mode_reset(lport);/*reset translation*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{
		ret = ctc_port_aggregation_mode_reset(lport);/*reset aggregation*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		ret = ctc_port_trunk_mode_reset(lport);/*reset trunk*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
    else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
    {
        ret = ctc_port_transparent_mode_reset(lport);/*reset transparent*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
    }

    ret = ctc_port_aggregation_mode_set(lport, pAggreCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

int32 ctc_trunk_vlan_set(uint32 lport, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg)
{
    int32 ret = RT_ERR_OK;
    ctc_wrapper_vlanCfg_t *pstVlanMode;

    if (!IsValidLgcPort(lport) || (NULL == pTrunkCfg))
    {
        return RT_ERR_FAILED;
    }

    ret = ctc_sfu_vlanCfg_get(lport, &pstVlanMode);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }
 
    if (CTC_VLAN_MODE_TAG == pstVlanMode->vlanMode)
    {
        ret = ctc_port_tag_mode_reset(lport);/*reset tag*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
    else if (CTC_VLAN_MODE_TRANSLATION == pstVlanMode->vlanMode)
    {
        ret = ctc_port_translation_mode_reset(lport);/*reset translation*/
        if (RT_ERR_OK != ret)
            return RT_ERR_FAILED;
    }
	else if (CTC_VLAN_MODE_AGGREGATION == pstVlanMode->vlanMode)
	{
		ret = ctc_port_aggregation_mode_reset(lport);/*reset aggregation*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
	else if (CTC_VLAN_MODE_TRUNK == pstVlanMode->vlanMode)
	{
		ret = ctc_port_trunk_mode_reset(lport);/*reset trunk*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
	}
    else if (CTC_VLAN_MODE_TRANSPARENT == pstVlanMode->vlanMode)
    {
        ret = ctc_port_transparent_mode_reset(lport);/*reset transparent*/
		if (RT_ERR_OK != ret)
			return RT_ERR_FAILED;
    }

    ret = ctc_port_trunk_mode_set(lport, pTrunkCfg);
    if (RT_ERR_OK != ret)
    {
        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

