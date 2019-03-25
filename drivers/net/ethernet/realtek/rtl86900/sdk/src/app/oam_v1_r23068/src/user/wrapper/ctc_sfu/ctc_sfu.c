/*
 * Copyright (C) 2015 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * $Revision: $
 * $Date: $
 *
 * Purpose : CTC proprietary behavior SFU wrapper APIs
 *
 * Feature : Provide the wapper layer for CTC SFU application
 *
 */

#ifdef CONFIG_SFU_APP

#include <stdio.h>
#include <rtk/switch.h>
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
#include <sys_def.h>
#include <rtk/oam.h>
#include "epon_oam_igmp_util.h"
#include <sys/wait.h>

static ctc_wrapper_chipInfo_t chipInfo;
/* Data is stored in the order of PON IF -> UNI PORT */
static ctc_sfu_pm_t pmStatistics[CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT];
static ctc_wrapper_ingressBw_t ingressBwDb[CTC_WRAPPER_NUM_UNIPORT];
static ctc_wrapper_egressBw_t egressBwDb[CTC_WRAPPER_NUM_UNIPORT];
static unsigned int ulQosTrustMode[MAX_PORT_NUM];
static ctc_wrapper_mxuMngGlobal_t mxuMng;
static int mxuMngInit = 0;

int32 ctc_sfu_qos_trust_mode_cfg_get(uint32 lport, uint32 *pulQosTrustMode)
{
    *pulQosTrustMode = ulQosTrustMode[lport];

    return RT_ERR_OK;
}

static void port_cfg_init(void)
{
    uint32 lport;

    FOR_EACH_LAN_PORT(lport)
    {
        ulQosTrustMode[lport] = QOS_COS;
    }
}

int32 is9601B(void)
{
	if (RTL9601B_CHIP_ID == chipInfo.chipId)
	{
		return TRUE;
	}

	return FALSE;
}

int32 is9602C(void)
{
	if (RTL9602C_CHIP_ID == chipInfo.chipId)
	{
		return TRUE;
	}

	return FALSE;
} 

int32 is9607C(void)
{
#ifdef CONFIG_RTL9607C_SERIES
	if (RTL9607C_CHIP_ID == chipInfo.chipId)
	{
		return TRUE;
	}
#endif
	return FALSE;
}

int32 drv_qos_init(void)
{
	rtk_qos_priSelWeight_t stPriSelWeight;
	rtk_qos_pri2queue_t pri2que;
	rtk_qos_queue_weights_t queueWeight;
	rtk_ponmac_queueCfg_t   queueCfg;
	rtk_ponmac_queue_t queue;
	uint32 lport = 0;
	uint32 pport = 0;
	int i;

	/*******************************************************************/
	/// init driver
	rtk_qos_init();
	rtk_ponmac_init();

	/*******************************************************************/
	/// config pon port
	if(TRUE == is9601B())
	{
		/* set qos prio mode */
		memset(&queueWeight, 0, sizeof(rtk_qos_pri2queue_t));
		rtk_qos_schedulingQueue_set(HAL_GET_PON_PORT(), &queueWeight);

		/* select group 0 */
		rtk_qos_portPriMap_set(HAL_GET_PON_PORT(), 0);
	}
	else
	{
		/*9607 pon port select priority-to-queue table 3 */
		rtk_qos_portPriMap_set(HAL_GET_PON_PORT(), 3);
	}

	/* qos will be implemented in PONMAC, so pon port dont need to schedule 
	 * internal priority all mapping to queue 0.
	 */
	pri2que.pri2queue[0]=0;
	pri2que.pri2queue[1]=0;
	pri2que.pri2queue[2]=0;
	pri2que.pri2queue[3]=0;
	pri2que.pri2queue[4]=0;
	pri2que.pri2queue[5]=0;
	pri2que.pri2queue[6]=0;
	pri2que.pri2queue[7]=0;
	if(RT_ERR_OK != rtk_qos_priMap_set(0, &pri2que))
	{
		return RT_ERR_FAILED;
	}

	/* disable cos remarking. */
	FOR_EACH_LAN_PORT(lport)
	{
		pport = PortLogic2PhyID(lport);
		if (INVALID_PORT == pport)
			continue;
		rtk_qos_1pRemarkEnable_set(pport, DISABLED);
	}

	pport = PortLogic2PhyID(LOGIC_PON_PORT);
	if (INVALID_PORT != pport)
		rtk_qos_1pRemarkEnable_set(pport, DISABLED);

	/*******************************************************************/
	/// config ponmac
	//rtk_ponmac_mode_set(PONMAC_MODE_EPON);
	rtk_ponmac_schedulingType_set(RTK_QOS_WFQ);

	/* there is not API to set EPON_LLIDn_SIDMAP_CTRL, default group idx 
	 * for priority to sid mapping is 3
	 */
	pri2que.pri2queue[0]=0;
	pri2que.pri2queue[1]=1;
	pri2que.pri2queue[2]=2;
	pri2que.pri2queue[3]=3;
	pri2que.pri2queue[4]=4;
	pri2que.pri2queue[5]=5;
	pri2que.pri2queue[6]=6;
	pri2que.pri2queue[7]=7;
	if(RT_ERR_OK != rtk_qos_priMap_set(3, &pri2que))
	{
		return RT_ERR_FAILED;
	}

	/* set weight of each priority assignment */
	memset(&stPriSelWeight, 0, sizeof(rtk_qos_priSelWeight_t));

	/*set flow based as highest priority*/
	stPriSelWeight.weight_of_acl = 7;
	/*set 802.1p as higher priority*/
	stPriSelWeight.weight_of_vlanBased = 6;
	stPriSelWeight.weight_of_svlanBased = 5;
	stPriSelWeight.weight_of_dot1q = 4;
	/*set port based as lower priority*/
	stPriSelWeight.weight_of_portBased = 3;
	/*set DSCP as lowest priority*/
	stPriSelWeight.weight_of_dscp = 2;
	//stPriSelWeight.dmac_pri = 1;
	stPriSelWeight.weight_of_saBaed = 0;
	
	if(RT_ERR_OK != rtk_qos_priSelGroup_set(0, &stPriSelWeight))
	{
		return RT_ERR_FAILED;
	}
	
	/* reset all queue 9601b need init 0-8 queues*/
	for (i=0; i<=8; i++)
	{
		queue.schedulerId = 0;	//only llid 0 can work
		queue.queueId = i;

		rtk_ponmac_queue_del(&queue);
	}

	/* add priority queue and mapping with flowid */
	for (i=0; i<=8; i++)
	{
		queue.schedulerId = 0;	//llid 0
		queue.queueId = i;
		
		queueCfg.cir		= 0;
		queueCfg.pir		= HAL_PONMAC_PIR_CIR_RATE_MAX();
		queueCfg.type		= STRICT_PRIORITY;
		queueCfg.weight 	= 0;
		queueCfg.egrssDrop	= ENABLED;
		
		rtk_ponmac_queue_add(&queue, &queueCfg);

		rtk_ponmac_flow2Queue_set(i, &queue);
	}

	if(TRUE == is9601B())
	{
		/*init dying gaps stream id 31*/
		queue.schedulerId = 0;	//llid 0
		queue.queueId = 31;
		
		queueCfg.cir		= 0;
		queueCfg.pir		= HAL_PONMAC_PIR_CIR_RATE_MAX();
		queueCfg.type		= STRICT_PRIORITY;
		queueCfg.weight 	= 0;
		queueCfg.egrssDrop	= ENABLED;

		rtk_ponmac_flow2Queue_set(31, &queue);
	}
	return RT_ERR_OK;
}

int32 ctc_sfu_qos_trust_mode_set(uint32 mode)
{
	rtk_qos_priSelWeight_t stPriSelWeight;

	memset(&stPriSelWeight, 0, sizeof(rtk_qos_priSelWeight_t));

	if(QOS_COS == mode)
	{
		/*set flow based as highest priority*/
		stPriSelWeight.weight_of_acl = 7;
		/*set 802.1p as higher priority*/
		stPriSelWeight.weight_of_vlanBased = 6;
		stPriSelWeight.weight_of_svlanBased = 5;        
		stPriSelWeight.weight_of_dot1q = 4;        
		/*set port based as lower priority*/
		stPriSelWeight.weight_of_portBased = 3;        
		/*set DSCP as lowest priority*/
		stPriSelWeight.weight_of_dscp = 2;              
		//stPriSelWeight.dmac_pri = 1; 
		stPriSelWeight.weight_of_saBaed = 0;
	}
	else if(QOS_DSCP == mode)
	{
		/*set flow based as highest priority*/
		stPriSelWeight.weight_of_acl = 7;
		/*set 802.1p as higher priority*/
		stPriSelWeight.weight_of_vlanBased = 5;
		stPriSelWeight.weight_of_svlanBased = 4;        
		stPriSelWeight.weight_of_dot1q = 3;        
		/*set port based as lower priority*/
		stPriSelWeight.weight_of_portBased = 2;        
		/*set DSCP as lowest priority*/
		stPriSelWeight.weight_of_dscp = 6;              
		//stPriSelWeight.dmac_pri = 1; 
		stPriSelWeight.weight_of_saBaed = 0;
	}
	else if(QOS_PORT == mode)
	{
		/*set flow based as highest priority*/
		stPriSelWeight.weight_of_acl = 7;
		/*set 802.1p as higher priority*/
		stPriSelWeight.weight_of_vlanBased = 5;
		stPriSelWeight.weight_of_svlanBased = 4;        
		stPriSelWeight.weight_of_dot1q = 3;        
		/*set port based as lower priority*/
		stPriSelWeight.weight_of_portBased = 6;        
		/*set DSCP as lowest priority*/
		stPriSelWeight.weight_of_dscp = 2;              
		//stPriSelWeight.dmac_pri = 1; 
		stPriSelWeight.weight_of_saBaed = 0;
	}
	else if(QOS_SVLAN == mode)
	{
		/*set flow based as highest priority*/
		stPriSelWeight.weight_of_acl = 7;
		/*set 802.1p as higher priority*/
		stPriSelWeight.weight_of_vlanBased = 5;
		stPriSelWeight.weight_of_svlanBased = 6;        
		stPriSelWeight.weight_of_dot1q = 4;        
		/*set port based as lower priority*/
		stPriSelWeight.weight_of_portBased = 3;        
		/*set DSCP as lowest priority*/
		stPriSelWeight.weight_of_dscp = 2;              
		//stPriSelWeight.dmac_pri = 1; 
		stPriSelWeight.weight_of_saBaed = 0;
	}
	else
	{
		return RT_ERR_INPUT;
	}

	if(RT_ERR_OK != rtk_qos_priSelGroup_set(0, &stPriSelWeight))
	{
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}

/* According to CTC Spec, the transparent mode is defined as:
 * Upstream: tag in, tag out, untag in, untag out. No change to tag.
 * Downstream: tag in, tag out, untag in, untag out. No change to tag.
 */
static int ctc_sfu_vlanTransparent_set(unsigned int port)
{
	if ((FALSE == is9601B()) && (FALSE == is9607C()) && (FALSE == is9602C()))
	{
		if (RT_ERR_OK == ctc_transparent_vlan_set(port))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
	else
	{
		if (RT_ERR_OK == ctc_clf_rule_for_vlanTransparent_create(port))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
}

/* According to CTC Spec, the tag mode is defined as:
 * Upstream: untag in, tag out, tag in drop.
 * Downstream: tag in, untag out, untag in, drop.
 */
static int ctc_sfu_vlanTag_set(unsigned int port, ctc_wrapper_vlan_t *pTagCfg)
{
	if ((FALSE == is9601B()) && (FALSE == is9607C()) && (FALSE == is9602C()))
	{
		if (RT_ERR_OK == ctc_tag_vlan_set(port, pTagCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
	else
	{
		if (RT_ERR_OK == ctc_clf_rule_for_vlanTag_create(port, pTagCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
}

/* According to CTC Spec, the translation mode is defined as:
 * Upstream: untag in, default tag out, tag within translation table in, translated tag out, others drop.
 * Downstream: default tag in, untag out, translated tag in, tag out, others drop
 */
static int ctc_sfu_vlanTranslation_set(unsigned int port, ctc_wrapper_vlanTransCfg_t *pTransCfg)
{
	if ((FALSE == is9601B()) && (FALSE == is9607C()) && (FALSE == is9602C()))
	{
		if (RT_ERR_OK == ctc_translation_vlan_set(port, pTransCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
	else
	{
		if (RT_ERR_OK == ctc_clf_rule_for_vlanTranslation_create(port, pTransCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
}

/* According to CTC Spec, the aggregation mode is defined as:
 * Upstream: untag in, default tag out, tag within aggregated table in, aggregated tag out, others drop.
 * Downstream: default tag in, untag out, aggregated tag in, tag out (MAC 2 VID), others drop
 */
static int ctc_sfu_vlanAggregation_set(unsigned int port, ctc_wrapper_vlanAggreCfg_t *pAggrCfg)
{
	if ((FALSE == is9601B()) && (FALSE == is9607C()) && (FALSE == is9602C()))
	{
		if (RT_ERR_OK == ctc_aggregation_vlan_set(port, pAggrCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
	else
	{
		if (RT_ERR_OK == ctc_clf_rule_for_vlanAggregation_create(port, pAggrCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
}

/* According to CTC Spec, the trunk mode is defined as:
 * Upstream: untag in, default tag out, tag within accept table in, tag out, others drop.
 * Downstream: default tag in, untag out, accept tag in, tag out, others drop
 */
static int ctc_sfu_vlanTrunk_set(unsigned int port, ctc_wrapper_vlanTrunkCfg_t *pTrunkCfg)
{
	if ((FALSE == is9601B()) && (FALSE == is9607C()) && (FALSE == is9602C()))
	{
		if (RT_ERR_OK == ctc_trunk_vlan_set(port, pTrunkCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
	else
	{
		if (RT_ERR_OK == ctc_clf_rule_for_vlanTrunk_create(port, pTrunkCfg))
			return EPON_OAM_ERR_OK;

		return EPON_OAM_ERR_WRAPPER;
	}
}

static void ctc_sfu_vlan_init(void)
{
	rtk_portmask_t stPhyMask;
	rtk_portmask_t stPhyMaskUntag;
    int i, ret;
	uint32 uiPPort;

	/*
	 * 9601B will use classification rule to implement vlan attribute.
	 * it need below operation
	 * 1. disable lan port ingress/egress filter function
	 * 2. drop unmatch classfication
	 * 3. add unmatch svlan 0 for ds stream.
	 * 4. permit unmatch acl, done in drv_acl_rule_init()
	 * 5. 
	 */
	if (TRUE == is9601B())
	{
		RTK_PORTMASK_RESET(stPhyMask);
		RTK_PORTMASK_RESET(stPhyMaskUntag);
		
		/* disable ingress/egress cvlan filter for lan port */
		rtk_vlan_vlanFunctionEnable_set(DISABLED);

		/* disable ingress/egress svlan filter for pon port */
		rtk_svlan_svlanFunctionEnable_set(DISABLED);
		
		for(i = 0 ; i < chipInfo.uniPortCnt ; i++)
		{
			/* disable ingress filter */
			//drv_vlan_port_ingressfilter_set(i, FALSE);
			
			uiPPort = PortLogic2PhyID(i);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
		}

		/* add all port to svlan 0 */
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		/* pon port based svid is svlan 0 and svlan untag action is to assign port based svid.
		   downstream untag packet may flood to cpu port, so not add cpu port to svlan 0 to fix the problem */
		//RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_CPU_PORT());
		//RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_CPU_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());

		drv_svlan_member_add(0, 0, stPhyMask, stPhyMaskUntag, 0);
		
		/* set upstream classification unmatch action to drop */
		rtk_classify_unmatchAction_set(CLASSIFY_UNMATCH_DROP);
		
		/* downstream classification default drop */
		drv_clf_rule_for_ctc_def_drop_create();
	}
	else if((TRUE == is9607C()) || (TRUE == is9602C()))
	{
		/* 9602c and 9607c not support keeptag and c2s function, so must use classification rule to implement vlan attribute */
		RTK_PORTMASK_RESET(stPhyMask);
		RTK_PORTMASK_RESET(stPhyMaskUntag);
		
		/* disable ingress/egress cvlan filter for lan port */
		rtk_vlan_vlanFunctionEnable_set(DISABLED);

		/* Enable ingress/egress svlan filter for pon port */
		rtk_svlan_svlanFunctionEnable_set(ENABLED);
		
		for(i = 0 ; i < chipInfo.uniPortCnt ; i++)
		{	
			uiPPort = PortLogic2PhyID(i);
			if (INVALID_PORT != uiPPort)
				RTK_PORTMASK_PORT_SET(stPhyMask, uiPPort);
		}

		/* add all port to svlan 0 */
		RTK_PORTMASK_PORT_SET(stPhyMask, HAL_GET_PON_PORT());
		RTK_PORTMASK_PORT_SET(stPhyMaskUntag, HAL_GET_PON_PORT());

		drv_svlan_member_add(0, 0, stPhyMask, stPhyMaskUntag, 0);
		
		/* set upstream classification unmatch action to permit_without_pon: hardware not support drop action */
		rtk_classify_unmatchAction_set(CLASSIFY_UNMATCH_PERMIT_WITHOUT_PON);
		
		/* downstream classification default drop */
		drv_clf_rule_for_ctc_def_drop_create();

		if(TRUE == is9602C())
		{	/* must use classf rule pattern 1 (support vlan action), set range (64 ~ 255) */
			rtk_classify_entryNumPattern1_set(192); /* 256 - 64 */
		}
	}
	else
	{
		/*9607 need to set vlan transparent state enable to make vlan transparent mode work*/
		rtk_vlan_transparentEnable_set(ENABLED);
		/* set cvlan ingress-filer of pon port disable for downstream double-tag packets */
		drv_vlan_port_ingressfilter_set(LOGIC_PON_PORT, FALSE);

		/* unnessary for 9601B and cause untag broadcast packet flooding to lan port although lan port is tag mode */
		ctc_port_vlan_clf_init();
	}

    for(i = 0 ; i < chipInfo.uniPortCnt ; i++)
    {
        ret = ctc_sfu_vlanTransparent_set(i);
        if(ret)
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, "[OAM:%s:%d] default vlan config failed\n", __FILE__, __LINE__);
        }
    }

    return;
}

/* siyuan 2016-11-25: For chip version <= CHIP_REV_ID_F, add a acl to process mpcp packet: 
   only create a interrupt, not send this mpcp packet to cpu port */
static int ctc_sfu_add_mpcp_interrupt_acl()
{
	int ret;
	
	if(APOLLOMP_CHIP_ID == chipInfo.chipId)
	{
		if(chipInfo.rev <= CHIP_REV_ID_F)
		{		
			rtk_acl_field_entry_t aclField;
			rtk_acl_template_t aclTemplete;
			rtk_acl_ingress_entry_t aclRule;
			rtk_acl_field_t fieldHead1,fieldHead2;
						
			/*drop all mpcp gate message*/
			/*field selector*/
			aclField.index = 3;
			aclField.format = ACL_FORMAT_RAW;
			aclField.offset = 14;
			if ((ret = rtk_acl_fieldSelect_set(&aclField)) != RT_ERR_OK)
			{
				printf("rtk_acl_fieldSelect_set fail ret[%d]\n",ret);
				return ret;
			}

			/*add acl rule drop all gate message and trigger interrupt*/
			osal_memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
			aclRule.index = 63;
			aclRule.templateIdx = 2;
			/*enable ACL on PON port*/
			RTK_PORTMASK_PORT_SET(aclRule.activePorts, HAL_GET_PON_PORT());
			aclRule.valid = ENABLED;
			aclRule.act.enableAct[ACL_IGR_INTR_ACT] = ENABLED;
			aclRule.act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
			aclRule.act.aclInterrupt = ENABLED;
			aclRule.act.forwardAct.act = ACL_IGR_FORWARD_DROP_ACT;
				
			fieldHead1.fieldType = ACL_FIELD_ETHERTYPE;
			fieldHead1.fieldUnion.data.value = 0x8808;
			fieldHead1.fieldUnion.data.mask = 0xFFFF;
			fieldHead1.next = NULL;
			if ((ret = rtk_acl_igrRuleField_add(&aclRule,&fieldHead1)) != RT_ERR_OK)
			{
				printf("rtk_acl_igrRuleField_add fail ret[%d]\n",ret);
				return ret;
			}
			
			fieldHead2.fieldType = ACL_FIELD_USER_DEFINED03;
			fieldHead2.fieldUnion.data.value = 0x0002;
			fieldHead2.fieldUnion.data.mask = 0xFFFF;					   
			fieldHead2.next = NULL;
			if ((ret = rtk_acl_igrRuleField_add(&aclRule,&fieldHead2)) != RT_ERR_OK)
			{
				printf("rtk_acl_igrRuleField_add fail ret[%d]\n",ret);
				return ret;
			}
				
			if ((ret = rtk_acl_igrRuleEntry_add(&aclRule)) != RT_ERR_OK)
			{
				printf("rtk_acl_igrRuleEntry_add fail ret[%d]\n",ret);
				return ret;
			}
			
			if ((ret = rtk_acl_igrState_set(HAL_GET_PON_PORT(), ENABLED)) != RT_ERR_OK)
			{
				printf("rtk_acl_igrState_set fail ret[%d]\n",ret);
				return ret;
			}
		}
	}
}

static void ctc_sfu_cf_init(void)
{
	ctc_sys_init();
	
	rtk_acl_init();
    drv_acl_rule_init();

	ctc_sfu_add_mpcp_interrupt_acl();

#ifdef CONFIG_KERNEL_RTK_ONUCOMM
	drv_acl_rule_special_packet_trp2cpu();
#endif

	rtk_classify_init();
	drv_clf_init();

	ctc_clf_init();
	
    return;
}

static void ctc_sfu_rldp_init(void)
{
    rtk_rldp_config_t config;

    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
    case RTL9602C_CHIP_ID:
        rtk_rldp_init();
        rtk_rldp_config_get(&config);
        config.rldp_enable = ENABLED;
        rtk_rldp_config_set(&config);
        break;
    case RTL9601B_CHIP_ID:
    default:
        break;
    }
}

static void ctc_sfu_pmUpdatFromMib(ctc_pm_statistics_t *pmStat, rtk_stat_port_cntr_t mibCnt, int isLan)
{
	if(isLan)
	{
		/* Lan port: ifInOctets and rx are upstream, ifOutOctets and tx are downstream */
		pmStat->dsDropEvents = 0;
	    pmStat->usDropEvents = mibCnt.etherStatsDropEvents;

	    pmStat->dsOctets = mibCnt.ifOutOctets;
	    pmStat->usOctets = mibCnt.ifInOctets;

	    pmStat->dsFrames = mibCnt.ifOutUcastPkts + mibCnt.ifOutMulticastPkts + mibCnt.ifOutBrocastPkts;
	    pmStat->usFrames = mibCnt.ifInUcastPkts + mibCnt.ifInMulticastPkts + mibCnt.ifInBroadcastPkts;

	    pmStat->dsBroadcastFrames = mibCnt.ifOutBrocastPkts;
	    pmStat->usBroadcastFrames = mibCnt.ifInBroadcastPkts;

	    pmStat->dsMulticastFrames = mibCnt.ifOutMulticastPkts;
	    pmStat->usMulticastFrames = mibCnt.ifInMulticastPkts;

	    pmStat->dsCrcErrorFrames = 0;
		pmStat->usCrcErrorFrames = mibCnt.etherStatsCRCAlignErrors;

	    pmStat->dsUndersizeFrames = mibCnt.etherStatsTxUndersizePkts;
	    pmStat->usUndersizeFrames = mibCnt.etherStatsRxUndersizePkts;

	    pmStat->dsOversizeFrames = mibCnt.etherStatsTxOversizePkts;
	    pmStat->usOversizeFrames = mibCnt.etherStatsRxOversizePkts;

	    pmStat->dsFragmentFrames = 0;
		pmStat->usFragmentFrames = mibCnt.etherStatsFragments;

	    pmStat->dsJabberFrames = 0;
		pmStat->usJabberFrames = mibCnt.etherStatsJabbers;

	    pmStat->dsFrames64Octets = mibCnt.etherStatsTxPkts64Octets;
	    pmStat->dsFrames65to127Octets = mibCnt.etherStatsTxPkts65to127Octets;
	    pmStat->dsFrames128to255Octets = mibCnt.etherStatsTxPkts128to255Octets;
	    pmStat->dsFrames256to511Octets = mibCnt.etherStatsTxPkts256to511Octets;
	    pmStat->dsFrames512to1023Octets = mibCnt.etherStatsTxPkts512to1023Octets;
	    pmStat->dsFrames1024to1518Octets = mibCnt.etherStatsTxPkts1024to1518Octets;
		
	    pmStat->usFrames64Octets = mibCnt.etherStatsRxPkts64Octets;
	    pmStat->usFrames65to127Octets = mibCnt.etherStatsRxPkts65to127Octets;
	    pmStat->usFrames128to255Octets = mibCnt.etherStatsRxPkts128to255Octets;
	    pmStat->usFrames256to511Octets = mibCnt.etherStatsRxPkts256to511Octets;
	    pmStat->usFrames512to1023Octets = mibCnt.etherStatsRxPkts512to1023Octets;
	    pmStat->usFrames1024to1518Octets = mibCnt.etherStatsRxPkts1024to1518Octets;

	    pmStat->dsDiscardFrames = mibCnt.ifOutDiscards;
	    pmStat->usDiscardFrames = mibCnt.ifInDiscards;

	    pmStat->dsErrorFrames = 0;
		pmStat->usErrorFrames = mibCnt.etherStatsCRCAlignErrors;
	}
	else
	{
		/* Pon port: ifInOctets and rx are downstream, ifOutOctets and tx are upstream */
		pmStat->dsDropEvents = mibCnt.etherStatsDropEvents;
	    pmStat->usDropEvents = 0;

	    pmStat->dsOctets = mibCnt.ifInOctets;
	    pmStat->usOctets = mibCnt.ifOutOctets;

	    pmStat->dsFrames = mibCnt.ifInUcastPkts + mibCnt.ifInMulticastPkts + mibCnt.ifInBroadcastPkts;
	    pmStat->usFrames = mibCnt.ifOutUcastPkts + mibCnt.ifOutMulticastPkts + mibCnt.ifOutBrocastPkts;

	    pmStat->dsBroadcastFrames = mibCnt.ifInBroadcastPkts;
	    pmStat->usBroadcastFrames = mibCnt.ifOutBrocastPkts;

	    pmStat->dsMulticastFrames = mibCnt.ifInMulticastPkts;
	    pmStat->usMulticastFrames = mibCnt.ifOutMulticastPkts;

	    pmStat->dsCrcErrorFrames = mibCnt.etherStatsCRCAlignErrors;
		pmStat->usCrcErrorFrames = 0;
		
	    pmStat->dsUndersizeFrames = mibCnt.etherStatsRxUndersizePkts;
	    pmStat->usUndersizeFrames = mibCnt.etherStatsTxUndersizePkts;

	    pmStat->dsOversizeFrames = mibCnt.etherStatsRxOversizePkts;
	    pmStat->usOversizeFrames = mibCnt.etherStatsTxOversizePkts;

	    pmStat->dsFragmentFrames = mibCnt.etherStatsFragments;
		pmStat->usFragmentFrames = 0;
		
	    pmStat->dsJabberFrames = mibCnt.etherStatsJabbers;
		pmStat->usJabberFrames = 0;
		
	    pmStat->dsFrames64Octets = mibCnt.etherStatsRxPkts64Octets;
	    pmStat->dsFrames65to127Octets = mibCnt.etherStatsRxPkts65to127Octets;
	    pmStat->dsFrames128to255Octets = mibCnt.etherStatsRxPkts128to255Octets;
	    pmStat->dsFrames256to511Octets = mibCnt.etherStatsRxPkts256to511Octets;
	    pmStat->dsFrames512to1023Octets = mibCnt.etherStatsRxPkts512to1023Octets;
	    pmStat->dsFrames1024to1518Octets = mibCnt.etherStatsRxPkts1024to1518Octets;
	    pmStat->usFrames64Octets = mibCnt.etherStatsTxPkts64Octets;
	    pmStat->usFrames65to127Octets = mibCnt.etherStatsTxPkts65to127Octets;
	    pmStat->usFrames128to255Octets = mibCnt.etherStatsTxPkts128to255Octets;
	    pmStat->usFrames256to511Octets = mibCnt.etherStatsTxPkts256to511Octets;
	    pmStat->usFrames512to1023Octets = mibCnt.etherStatsTxPkts512to1023Octets;
	    pmStat->usFrames1024to1518Octets = mibCnt.etherStatsTxPkts1024to1518Octets;

	    pmStat->dsDiscardFrames = mibCnt.ifInDiscards;
	    pmStat->usDiscardFrames = mibCnt.ifOutDiscards;

	    pmStat->dsErrorFrames = mibCnt.etherStatsCRCAlignErrors;
		pmStat->usErrorFrames = 0;
	}
    pmStat->statusChangeTimes = 0;
}

int ctc_sfu_portState_set(unsigned int port, rtk_enable_t state)
{
    int ret;
    int phyPort;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_adminEnable_set(phyPort, state);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portState_get(unsigned int port, rtk_enable_t *pState)
{
    int ret;
    int phyPort;
    rtk_enable_t enable;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_adminEnable_get(phyPort, &enable);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portAutoNegoRestart_set(unsigned int port)
{
    int ret;
    int phyPort;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;

	/* port set phy-reg port 0 page 0 register 0 data 0x1200 
	  * data bit 9 (1:restart auto nego)
	  * data bit 12 (1:auto nego enable, 0:auto nego disable)
	  */
    ret = rtk_port_phyReg_set(phyPort, 0, 0, 0x1200);
    if(ret != RT_ERR_OK)
    {
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_ERROR, 
			"[OAM:%s:%d] rtk_port_phyReg_set failed ret[%d]\n", __FILE__, __LINE__, ret);
		return EPON_OAM_ERR_WRAPPER;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portAutoNego_set(unsigned int port, rtk_enable_t state)
{
    int ret;
    int phyPort;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;

    ret = rtk_port_phyAutoNegoEnable_set(phyPort, state);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portAutoNego_get(unsigned int port, rtk_enable_t *pState)
{
    int ret;
    int phyPort;
    rtk_enable_t enable;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_phyAutoNegoEnable_get(phyPort, &enable);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portCap_get(unsigned int port, unsigned int *pCapability)
{
    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pCapability), EPON_OAM_ERR_PARAM);

    *pCapability = chipInfo.uniPortCap[port];
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portLinkStatus_get (unsigned int port, rtk_enable_t *pState)
{
    int ret;
    int phyPort;
    rtk_port_linkStatus_t status;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_link_get(phyPort, &status);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    if(status == PORT_LINKDOWN)
    {
        *pState = DISABLED;
    }
    else
    {
        *pState = ENABLED;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portFlowControl_set(unsigned int port, rtk_enable_t state)
{
    int ret;
    int phyPort;
    rtk_port_phy_ability_t ability;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_phyAutoNegoAbility_get(phyPort, &ability);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }

	/* don't set flowcontrol state if it's not changed to avoid lan port down problem */
	if( (ability.FC && (state == DISABLED)) || ((ability.FC == 0) && (state == ENABLED)))
	{
	    ability.FC = state;
	    ability.AsyFC = state;

	    ret = rtk_port_phyAutoNegoAbility_set(phyPort, &ability);
	    if(ret != RT_ERR_OK)
	    {
	        return EPON_OAM_ERR_WRAPPER;
	    }
	}
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portFlowControl_get(unsigned int port, rtk_enable_t *pState)
{
    int ret;
    int phyPort;
    rtk_port_phy_ability_t ability;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_port_phyAutoNegoAbility_get(phyPort, &ability);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    if(ability.FC)
    {
        *pState = ENABLED;
    }
    else
    {
        *pState = DISABLED;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portIngressBw_set(unsigned int port, rtk_enable_t enable, unsigned int cir, unsigned int cbs, unsigned int ebs)
{
    int ret;
    int phyPort;
    unsigned int confRate;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    confRate = (enable == ENABLED) ? cir : 1048568;
    ret = rtk_rate_portIgrBandwidthCtrlRate_set(phyPort, confRate);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }

    ingressBwDb[port].enable = enable;
    if(ENABLED == enable)
    {
        ingressBwDb[port].cir = cir;
        ingressBwDb[port].cbs = cbs;
        ingressBwDb[port].ebs = ebs;
    }
    else
    {
        ingressBwDb[port].cir = 0;
        ingressBwDb[port].cbs = 0;
        ingressBwDb[port].ebs = 0;
    }

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portIngressBw_get(unsigned int port, rtk_enable_t *pEnable, unsigned int *pCir, unsigned int *pCbs, unsigned int *pEbs)
{
    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pEnable), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pCir), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pCbs), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pEbs), EPON_OAM_ERR_PARAM);

    /* Retrieve the config from stored database */
    *pEnable = ingressBwDb[port].enable;
    *pCir = ingressBwDb[port].cir;
    *pCbs = ingressBwDb[port].cbs;
    *pEbs = ingressBwDb[port].ebs;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portEgressBw_set(unsigned int port, rtk_enable_t enable, unsigned int cir, unsigned int pir)
{
    int ret;
    int phyPort;
    unsigned int confRate;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    confRate = (enable == ENABLED) ? ((pir > cir) ? pir : cir) : 1048568;
    ret = rtk_rate_portEgrBandwidthCtrlRate_set(phyPort, confRate);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }

    egressBwDb[port].enable = enable;
    if(ENABLED == enable)
    {
        egressBwDb[port].cir = cir;
        egressBwDb[port].pir = pir;
    }
    else
    {
        egressBwDb[port].cir = 0;
        egressBwDb[port].pir = 0;
    }

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portEgressBw_get(unsigned int port, rtk_enable_t *pEnable, unsigned int *pCir, unsigned int *pPir)
{
    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pEnable), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pCir), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pPir), EPON_OAM_ERR_PARAM);

    /* Retrieve the config from stored database */
    *pEnable = egressBwDb[port].enable;
    *pCir = egressBwDb[port].cir;
    *pPir = egressBwDb[port].pir;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portLoopDetect_set(unsigned int port, rtk_enable_t enable)
{
    int ret;
    int phyPort;
    rtk_rldp_portConfig_t portConfig;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

	if(set_port_loopdetect(port, enable)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portLoopDetect_get(unsigned int port, rtk_enable_t *pEnable)
{
    int ret;
    int phyPort;
    rtk_rldp_portConfig_t portConfig;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pEnable), EPON_OAM_ERR_PARAM);
   
	if (get_port_loopdetect(port, pEnable)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portDisableLooped_set(unsigned int port, rtk_enable_t enable)
{
    int ret;
    rtk_rldp_portConfig_t portConfig;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

    if(set_port_loopdetect_autodown(port, enable)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portDisableLooped_get(unsigned int port, rtk_enable_t *pEnable)
{
    int ret;
    rtk_rldp_portConfig_t portConfig;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pEnable), EPON_OAM_ERR_PARAM);

   	if(get_port_loopdetect_autodown(port, pEnable)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;		

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portLoopParameterConfig_set(unsigned int port, unsigned short detectFrequency, 
								unsigned short recoveryInterval, short * svlan, short * cvlan)
{	
	int ret;
	CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
	CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
	
	if(set_port_loopdetect_parameterConfig(port, detectFrequency, recoveryInterval, svlan, cvlan)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;
		
	return EPON_OAM_ERR_OK;
}

int ctc_sfu_portMacAging_set(unsigned int port, rtk_enable_t enable, unsigned int agingTime)
{
    int ret;
    int phyPort;
	int i;

	/* siyuan 20170719: operation target of Mac Aging command was changed from UNI port to ONU, ignore parameter port */
	for(i = 0 ; i < chipInfo.uniPortCnt ; i++)
	{	
		phyPort = PortLogic2PhyID(i);
		if (INVALID_PORT != phyPort)
		{
			ret = rtk_l2_portAgingEnable_set(phyPort, enable);
		    if(ret != RT_ERR_OK)
		    {
		        return EPON_OAM_ERR_WRAPPER;
		    }
		}
	}

    if(ENABLED == enable)
    {
        switch(chipInfo.chipId)
        {
        case APOLLOMP_CHIP_ID:
        case RTL9601B_CHIP_ID:
        case RTL9602C_CHIP_ID:
		case RTL9607C_CHIP_ID:
            /* These chip didn't support per-port aging time config, only per-system */
            ret = rtk_l2_aging_set(agingTime * 10); /* In unit of 0.1 sec */
            if(ret != RT_ERR_OK)
            {
                return EPON_OAM_ERR_WRAPPER;
            }
            break;
        default:
            break;
        }
    }

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portMacAging_get(unsigned int port, rtk_enable_t *pEnable, unsigned int *pAgingTime)
{
    int ret;
    int phyPort;
    unsigned int agingTime;
    rtk_enable_t enable;

    CTC_WRAPPER_PARAM_CHECK((NULL == pEnable), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pAgingTime), EPON_OAM_ERR_PARAM);

	/* siyuan 20170719: operation target of Mac Aging command was changed from UNI port to ONU, ignore parameter port */
	phyPort = PortLogic2PhyID(0); /* get lan port 0 by default */
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
    ret = rtk_l2_portAgingEnable_get(phyPort, &enable);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }

    if(ENABLED == enable)
    {
        switch(chipInfo.chipId)
        {
        case APOLLOMP_CHIP_ID:
        case RTL9601B_CHIP_ID:
        case RTL9602C_CHIP_ID:
		case RTL9607C_CHIP_ID:
            /* These chip didn't support per-port aging time config, only per-system */
            ret = rtk_l2_aging_get(&agingTime); 
            if(ret != RT_ERR_OK)
            {
                return EPON_OAM_ERR_WRAPPER;
            }
            break;
        default:
            break;
        }
    }

    *pEnable = enable;
    *pAgingTime = agingTime / 10; /* In unit of 0.1 sec */

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_boardCap_get(ctc_boardCap_t *pBoardCap)
{
    int i;

    CTC_WRAPPER_PARAM_CHECK((NULL == pBoardCap), EPON_OAM_ERR_PARAM);

    pBoardCap->uniPortCnt = chipInfo.uniPortCnt;
    pBoardCap->uniGePortCnt = chipInfo.uniGePortCnt;
    pBoardCap->uniFePortCnt = chipInfo.uniFePortCnt;
    pBoardCap->potsPortCnt = chipInfo.voipPortCnt;
	pBoardCap->usbPortCnt = chipInfo.usbPortCnt;
	pBoardCap->wlanPortCnt = chipInfo.wlanPortCnt;
    for(i = 0 ; i < pBoardCap->uniPortCnt ; i ++)
    {
        pBoardCap->uniPortCap[i] = chipInfo.uniPortCap[i];
    }
   
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_transceiverStatus_get(unsigned int statusType, unsigned short *pStatusData)
{
    int ret;
    rtk_transceiver_parameter_type_t type;
    rtk_transceiver_data_t data;

    CTC_WRAPPER_PARAM_CHECK((NULL == pStatusData), EPON_OAM_ERR_PARAM);

    switch(statusType)
    {
    case CTC_WRAPPER_TRANSCIVERSTATUS_TEMPERATURE:
        type = RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE;
        break;
    case CTC_WRAPPER_TRANSCIVERSTATUS_SUPPLYVOLTAGE:
        type = RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE;
        break;
    case CTC_WRAPPER_TRANSCIVERSTATUS_BIASCURRENT:
        type = RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT;
        break;
    case CTC_WRAPPER_TRANSCIVERSTATUS_TXPOWER:
         type = RTK_TRANSCEIVER_PARA_TYPE_TX_POWER;
       break;
    case CTC_WRAPPER_TRANSCIVERSTATUS_RXPOWER:
        type = RTK_TRANSCEIVER_PARA_TYPE_RX_POWER;
        break;
    default:
        return EPON_OAM_ERR_PARAM;
    }
    ret = rtk_ponmac_transceiver_get(type, &data);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pStatusData = data.buf[0] << 8 | data.buf[1];

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_portAutoNegoAdvertise_get(unsigned int port, unsigned int *pAdvertise)
{
    int ret;
	uint32 phyPort, data;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pAdvertise), EPON_OAM_ERR_PARAM);

    /* MODIFY ME - assume the all capability are advertised as default */
    //*pAdvertise = chipInfo.uniPortCap[port];
    /* QL: get advertisement ability from reg4 */
#define PORT_ADV_CAP	((1<<11) | /*Asymmetric Pause*/ \
						 (1<<10) | /*pause*/ \
						 (1<<8) | /*100Base-TX-FD*/ \
						 (1<<7) | /*100Base-TX*/ \
						 (1<<6) | /*10Base-T-FD*/ \
						 (1<<5) /*10Base-T*/ \
						 )
	phyPort = PortLogic2PhyID(port);
	if (INVALID_PORT == phyPort)
		return EPON_OAM_ERR_WRAPPER;
	
	rtk_port_phyReg_get(phyPort, 0, 4, &data);
	*pAdvertise = data & (CTC_WRAPPER_PORTCAP_10BASE_T |
						  CTC_WRAPPER_PORTCAP_10BASE_T_FD|
						  CTC_WRAPPER_PORTCAP_100BASE_T |
						  CTC_WRAPPER_PORTCAP_100BASE_T_FD |
						  CTC_WRAPPER_PORTCAP_PAUSE |
						  CTC_WRAPPER_PORTCAP_APAUSE);

	/* giga ability derive from reg9 */
	rtk_port_phyReg_get(phyPort, 0, 9, &data);
	*pAdvertise |= data & CTC_WRAPPER_PORTCAP_1000BASE_T_FD;
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_fecState_set(rtk_enable_t state)
{
    int ret;

    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
        if((ret = rtk_epon_fecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        if((ret = rtk_epon_usFecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    default:
        if((ret = rtk_epon_usFecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_fecState_get(rtk_enable_t *pState)
{
    int ret;
    rtk_enable_t enable;

    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
        if((ret = rtk_epon_fecState_get(&enable)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    default:
        if((ret = rtk_epon_usFecState_get(&enable)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_sfu_chipInfo_get(ctc_wrapper_chipInfo_t *pChipInfo)
{
    CTC_WRAPPER_PARAM_CHECK((NULL == pChipInfo), EPON_OAM_ERR_PARAM);

    *pChipInfo = chipInfo;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_vlan_set(unsigned int port, ctc_wrapper_vlanCfg_t *pVlanCfg)
{
    int ret;

	CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

    /* All these VLAN mode perform below steps
     * 1. Resource check
     * 2. Clear old settings
     * 3. Apply new settings
     * 4. Save settings
     */

    switch(pVlanCfg->vlanMode)
    {
    case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
        return ctc_sfu_vlanTransparent_set(port);
    case CTC_OAM_VAR_VLAN_MODE_TAG:
        return ctc_sfu_vlanTag_set(port, &(pVlanCfg->cfg.tagCfg));
    case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
        return ctc_sfu_vlanTranslation_set(port, &(pVlanCfg->cfg.transCfg));
    case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
        return ctc_sfu_vlanAggregation_set(port, &(pVlanCfg->cfg.aggreCfg));
    case CTC_OAM_VAR_VLAN_MODE_TRUNK:
        return ctc_sfu_vlanTrunk_set(port, &(pVlanCfg->cfg.trunkCfg));
    }

    return EPON_OAM_ERR_PARAM;
}

int ctc_sfu_vlan_get(unsigned int port,
    ctc_wrapper_vlanCfg_t *pVlanCfg /* get function will allcate the memory needed, caller should free it */)
{
	ctc_wrapper_vlanCfg_t *pVlanCfgDatabase;
    int i, j;

    CTC_WRAPPER_PARAM_CHECK((port >= CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pVlanCfg), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

	if (ctc_sfu_vlanCfg_get(port, &pVlanCfgDatabase) != RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;
	
    pVlanCfg->vlanMode = pVlanCfgDatabase->vlanMode;
    switch(pVlanCfg->vlanMode)
    {
    case CTC_OAM_VAR_VLAN_MODE_TRANSPARENT:
        /* No additional settings for transparent mode */
        break;
    case CTC_OAM_VAR_VLAN_MODE_TAG:
        pVlanCfg->cfg.tagCfg = pVlanCfgDatabase->cfg.tagCfg;
        break;
    case CTC_OAM_VAR_VLAN_MODE_TRANSLATION:
        pVlanCfg->cfg.transCfg.defVlan = pVlanCfgDatabase->cfg.transCfg.defVlan;
        pVlanCfg->cfg.transCfg.num = pVlanCfgDatabase->cfg.transCfg.num;
        pVlanCfg->cfg.transCfg.transVlanPair = (ctc_wrapper_vlanTransPair_t *) malloc(sizeof(ctc_wrapper_vlanTransPair_t) * pVlanCfg->cfg.transCfg.num);
        if(NULL == pVlanCfg->cfg.transCfg.transVlanPair)
        {
            return EPON_OAM_ERR_MEM;
        }
        memcpy(pVlanCfg->cfg.transCfg.transVlanPair, pVlanCfgDatabase->cfg.transCfg.transVlanPair, sizeof(ctc_wrapper_vlanTransPair_t) * pVlanCfg->cfg.transCfg.num);
        break;
    case CTC_OAM_VAR_VLAN_MODE_AGGREGATION:
        pVlanCfg->cfg.aggreCfg.defVlan = pVlanCfgDatabase->cfg.aggreCfg.defVlan;
        pVlanCfg->cfg.aggreCfg.tableNum = pVlanCfgDatabase->cfg.aggreCfg.tableNum;
        pVlanCfg->cfg.aggreCfg.aggrTbl = (ctc_wrapper_vlanAggreTableCfg_t *) malloc(sizeof(ctc_wrapper_vlanAggreTableCfg_t) * pVlanCfg->cfg.aggreCfg.tableNum);
        if(NULL == pVlanCfg->cfg.aggreCfg.aggrTbl)
        {
            return EPON_OAM_ERR_MEM;
        }
        for(i = 0 ; i < pVlanCfg->cfg.aggreCfg.tableNum ; i++)
        {
            pVlanCfg->cfg.aggreCfg.aggrTbl[i].entryNum = pVlanCfgDatabase->cfg.aggreCfg.aggrTbl[i].entryNum;
            pVlanCfg->cfg.aggreCfg.aggrTbl[i].aggreToVlan = pVlanCfgDatabase->cfg.aggreCfg.aggrTbl[i].aggreToVlan;
            pVlanCfg->cfg.aggreCfg.aggrTbl[i].aggreFromVlan = (ctc_wrapper_vlan_t *) malloc(sizeof(ctc_wrapper_vlan_t) * pVlanCfg->cfg.aggreCfg.aggrTbl[i].entryNum);
            if(NULL == pVlanCfg->cfg.aggreCfg.aggrTbl[i].aggreFromVlan)
            {
                for(j = 0 ; j < i ; j++)
                {
                    free(pVlanCfg->cfg.aggreCfg.aggrTbl[j].aggreFromVlan);
                }
                free(pVlanCfg->cfg.aggreCfg.aggrTbl);
                return EPON_OAM_ERR_MEM;
            }
            memcpy(pVlanCfg->cfg.aggreCfg.aggrTbl[i].aggreFromVlan, 
					pVlanCfgDatabase->cfg.aggreCfg.aggrTbl[i].aggreFromVlan, 
					sizeof(ctc_wrapper_vlan_t) * pVlanCfg->cfg.aggreCfg.aggrTbl[i].entryNum);
        }
        break;
    case CTC_OAM_VAR_VLAN_MODE_TRUNK:
        pVlanCfg->cfg.trunkCfg.defVlan = pVlanCfgDatabase->cfg.trunkCfg.defVlan;
        pVlanCfg->cfg.trunkCfg.num = pVlanCfgDatabase->cfg.trunkCfg.num;
        pVlanCfg->cfg.trunkCfg.acceptVlan = (ctc_wrapper_vlan_t *) malloc(sizeof(ctc_wrapper_vlan_t) * pVlanCfg->cfg.trunkCfg.num);
        if(NULL == pVlanCfg->cfg.trunkCfg.acceptVlan)
        {
            return EPON_OAM_ERR_MEM;
        }
        memcpy(pVlanCfg->cfg.trunkCfg.acceptVlan, pVlanCfgDatabase->cfg.trunkCfg.acceptVlan, sizeof(ctc_wrapper_vlan_t) * pVlanCfg->cfg.trunkCfg.num);
        break;
    }

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_pmStatus_set(unsigned int intf, rtk_enable_t state, unsigned int period)
{
    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

    if(state == DISABLED)
    {
        pmStatistics[intf].pmPeriodCur = 0;
    }

    pmStatistics[intf].pmState = state;
    pmStatistics[intf].pmPeriod = period;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_pmStatus_get(unsigned int intf, rtk_enable_t *pState, unsigned int *pPeriod)
{
    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pPeriod), EPON_OAM_ERR_PARAM);
    
    *pState = pmStatistics[intf].pmState;
    *pPeriod = pmStatistics[intf].pmPeriod;

    return EPON_OAM_ERR_OK;
}

void ctc_sfu_pmStat_update(unsigned int time)
{
    int i;
    int phyPort;
    rtk_stat_port_cntr_t portCnt;
    ctc_pm_statistics_t *tmp;

    for(i = 0 ; i < chipInfo.uniPortCnt + 1 /* UNI + PON port */ ; i++)
    {
        if(pmStatistics[i].pmState == ENABLED)
        {
            pmStatistics[i].pmPeriodCur += time;
            if(pmStatistics[i].pmPeriodCur >= pmStatistics[i].pmPeriod)
            {
                /* Statistics period endded */
                if(0 == i)
                {
                    /* PON interface */
                    rtk_stat_port_getAll(HAL_GET_PON_PORT(), &portCnt);
                    rtk_stat_port_reset(HAL_GET_PON_PORT());
					ctc_sfu_pmUpdatFromMib(pmStatistics[i].pmDataCur, portCnt, 0);
                }
                else
                {
                    /* UNI ports */
					phyPort = PortLogic2PhyID(i-1);
					if (INVALID_PORT == phyPort)
						continue;
					
                    rtk_stat_port_getAll(phyPort, &portCnt);
                    rtk_stat_port_reset(phyPort);
					ctc_sfu_pmUpdatFromMib(pmStatistics[i].pmDataCur, portCnt, 1);
                }
                
                /* MODIFY ME - Update link change counter */

                /* Clear History data and exchange buffer */
                memset(pmStatistics[i].pmDataHis, 0, sizeof(ctc_pm_statistics_t));
                tmp = pmStatistics[i].pmDataCur;
                pmStatistics[i].pmDataCur = pmStatistics[i].pmDataHis;
                pmStatistics[i].pmDataHis = tmp;

                /* Reset period counter */
                pmStatistics[i].pmPeriodCur = 0;
            }
        }
    }
}

int ctc_sfu_pmStat_set(unsigned int intf)
{
    int phyPort;
    rtk_stat_port_cntr_t portCnt;
    ctc_pm_statistics_t *tmp;

    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);

    /* Force statistics period endded */
    if(0 == intf)
    {
        /* PON interface */
        rtk_stat_port_getAll(HAL_GET_PON_PORT(), &portCnt);
		ctc_sfu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 0);
    }
    else
    {
        /* UNI ports */
		phyPort = PortLogic2PhyID(intf - 1);
		if (INVALID_PORT == phyPort)
			return EPON_OAM_ERR_WRAPPER;
		
        rtk_stat_port_getAll(phyPort, &portCnt);
		ctc_sfu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 1);
    }
    
    /* MODIFY ME - Update link change counter */

    /* Clear History data and exchange buffer */
    memset(pmStatistics[intf].pmDataHis, 0, sizeof(ctc_pm_statistics_t));
    tmp = pmStatistics[intf].pmDataCur;
    pmStatistics[intf].pmDataCur = pmStatistics[intf].pmDataHis;
    pmStatistics[intf].pmDataHis = tmp;

    /* Reset period counter */
    pmStatistics[intf].pmPeriodCur = 0;

    return EPON_OAM_ERR_OK;
}

int ctc_sfu_pmStat_get(unsigned int intf, ctc_pm_statistics_t *pmStat, ctc_pm_statisticsType_t pmType)
{
    int phyPort;
    rtk_stat_port_cntr_t portCnt;
    ctc_pm_statistics_t *tmp;

    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pmStat), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((pmType >= CTC_WRAPPER_PMTYPE_END), EPON_OAM_ERR_PARAM);

    switch(pmType)
    {
    case CTC_WRAPPER_PMTYPE_CURRENT:
        /* Update current immediately */
        if(0 == intf)
        {
            /* PON interface */
            rtk_stat_port_getAll(HAL_GET_PON_PORT(), &portCnt);
			ctc_sfu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 0);
        }
        else
        {
            /* UNI ports */
			phyPort = PortLogic2PhyID(intf-1);
			if (INVALID_PORT == phyPort)
				return EPON_OAM_ERR_NOT_FOUND;
			
            rtk_stat_port_getAll(phyPort, &portCnt);
			ctc_sfu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 1);
        }
        
        /* MODIFY ME - Update link change counter */
        *pmStat = *(pmStatistics[intf].pmDataCur);
        break;
    case CTC_WRAPPER_PMTYPE_HISTORY:
        *pmStat = *(pmStatistics[intf].pmDataHis);
        break;
    default:
        return EPON_OAM_ERR_NOT_FOUND;
    }

    return EPON_OAM_ERR_NOT_FOUND;
}

int ctc_sfu_loopback_set(rtk_enable_t state)
{
	int ponPort;

	ponPort = PortLogic2PhyID(LOGIC_PON_PORT);
	
	if (ENABLED == state)//Enable
		rtk_oam_parserAction_set(ponPort, OAM_PARSER_ACTION_LOOPBACK);
	else if (DISABLED == state)//Disable
		rtk_oam_parserAction_set(ponPort, OAM_PARSER_ACTION_FORWARD);

	return EPON_OAM_ERR_OK;
}

int is_vid_used_by_mxuMng(uint32 uiVid)
{
    if ((mxuMng.cvlan == uiVid) || (mxuMng.svlan == uiVid))
        return 1;
    
    return 0;
}

static int ctc_mxuMngGlobal_vlan_set(int act, ctc_wrapper_mxuMngGlobal_t * mxu)
{
	int32 ret;
    rtk_portmask_t stPortmask, stUntagPortmask;
    
    if (act)//create
    {
        if((mxu->cvlan == 0) && (mxu->svlan == 0))
        {
            /* create svlan 0*/
        	RTK_PORTMASK_RESET(stPortmask);
        	RTK_PORTMASK_RESET(stUntagPortmask);

        	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_PON_PORT());
            RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_CPU_PORT());
        	RTK_PORTMASK_PORT_SET(stUntagPortmask, HAL_GET_PON_PORT());
            RTK_PORTMASK_PORT_SET(stUntagPortmask, HAL_GET_CPU_PORT());

        	ret = drv_svlan_member_add(0, 0, stPortmask, stUntagPortmask, 0);
        	if (RT_ERR_OK != ret)
        	{
        		printf("%s set svlan 0 member fail\n", __func__);
        	}

        	/*set default svlan id*/
        	ret = drv_svlan_port_svid_set(LOGIC_CPU_PORT, 0);
        	if (RT_ERR_OK != ret)
        	{
        		printf("%s drv_svlan_port_svid_set cpu port fail\n", __func__);
        	}
            ret = drv_svlan_port_svid_set(LOGIC_PON_PORT, 0);
        	if (RT_ERR_OK != ret)
        	{
        		printf("%s drv_svlan_port_svid_set pon port fail\n", __func__);
        	}
        }
        else if ((mxu->cvlan > 0) && (mxu->svlan == 0))
        {
            /**************** upstream ****************/
            /* 1. cvlan entry create */
            ret = drv_vlan_entry_create(mxu->cvlan, VLAN_FID_SVL);
        	if ((RT_ERR_VLAN_EXIST == ret) || (RT_ERR_OK == ret))
            {
                RTK_PORTMASK_RESET(stPortmask);
                RTK_PORTMASK_RESET(stUntagPortmask);

            	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_PON_PORT());
            	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_CPU_PORT());
                ret = drv_vlan_member_add(mxu->cvlan, stPortmask, stUntagPortmask);
                if (RT_ERR_OK != ret)
                {
                    printf("%s set cvlan %d member fail\n", __func__, mxu->cvlan);
                }
            }
            else
                printf("%s create cvlan %d fail\n", __func__, mxu->cvlan);

            /* 2. cpu port svid is 0, pon port must be the member of svlan 0*/
        	RTK_PORTMASK_RESET(stPortmask);
        	RTK_PORTMASK_RESET(stUntagPortmask);

        	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_PON_PORT());	
        	RTK_PORTMASK_PORT_SET(stUntagPortmask, HAL_GET_PON_PORT());

        	ret = drv_svlan_member_add(0, 0, stPortmask, stUntagPortmask, 0);
        	if (RT_ERR_OK != ret)
        	{
        		printf("%s set svlan 0 member fail\n", __func__);
        	}

        	/*set default svlan id*/
        	ret = drv_svlan_port_svid_set(LOGIC_CPU_PORT, 0);
        	if (RT_ERR_OK != ret)
        	{
        		printf("%s drv_svlan_port_svid_set 0 fail\n", __func__);
        	}

            /**************** downstream ****************/
            /*set default svlan entry*/
            RTK_PORTMASK_RESET(stPortmask);
            RTK_PORTMASK_RESET(stUntagPortmask);

        	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_PON_PORT());
        	RTK_PORTMASK_PORT_SET(stPortmask, HAL_GET_CPU_PORT());

            ret = drv_svlan_member_add(mxu->cvlan, mxu->priority, stPortmask, stUntagPortmask, 0);
            if (RT_ERR_OK != ret)
            {
                printf("%s set svlan %d member fail\n", __func__, mxu->cvlan);
            }
        }
    }
    else//delete
    {
        if ((mxu->cvlan > 0) && (mxu->svlan == 0))
        {
            /* 1. svlan entry delete */
            RTK_PORTMASK_RESET(stPortmask);
            RTK_PORTMASK_RESET(stUntagPortmask);
            
            ret = rtk_svlan_memberPort_get(mxu->cvlan, &stPortmask, &stUntagPortmask);
        	if (RT_ERR_OK == ret)
            {
                if (FALSE == PhyMaskNotNull(stPortmask))//only used by mxuMng
                {
                    ret = rtk_svlan_destroy(mxu->cvlan);
            		if(RT_ERR_OK != ret)
                    {
                    	printf("%s delete svlan %d fail\n", __func__, mxu->cvlan);
                    }
                }
            }
            else
                printf("%s svlan %s not exist\n", __func__, mxu->cvlan);

            /* 2. cvlan entry delete*/
            RTK_PORTMASK_RESET(stPortmask);
            RTK_PORTMASK_RESET(stUntagPortmask);
        	ret = rtk_vlan_port_get(mxu->cvlan, &stPortmask, &stUntagPortmask);
        	if (RT_ERR_OK == ret)
            {
                if (FALSE == PhyMaskNotNull(stPortmask))//only used by mxuMng
                {
                    ret = rtk_vlan_destroy(mxu->cvlan);
            		if(RT_ERR_OK != ret)
                    {
                    	printf("%s delete cvlan %d fail\n", __func__, mxu->cvlan);
                    }
                }
            }
        }
    }

    return EPON_OAM_ERR_OK;
}

int ctc_set_ipv6_connectivity(int act, ctc_wrapper_mxuMngGlobal_t * mxu)
{
	//TODO: set ipv6 connectivity
	return EPON_OAM_ERR_OK;
}

int ctc_set_ipv4_connectivity(int act, ctc_wrapper_mxuMngGlobal_t * mxu)
{
	char cmd[128];
    char buf[20];
	const char PON_IF[] = "pon0";
	int ret;

	if(act)
	{
		/* Set */
        ctc_oam_flash_var_get("ELAN_MAC_ADDR", buf, sizeof(buf));
		memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "ifconfig %s hw ether %s", PON_IF, buf);
        system(cmd);
        memset(cmd, 0, sizeof(cmd));
	    snprintf(cmd, sizeof(cmd), "ifconfig %s up", PON_IF);
	    system(cmd);
		
		if((mxu->cvlan == 0) && (mxu->svlan == 0))
		{
			memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "ifconfig %s "IPADDR_PRINT" netmask "IPADDR_PRINT" up",
		             PON_IF, IPADDR_PRINT_ARG(mxu->ip[0]), IPADDR_PRINT_ARG(mxu->mask));
		    system(cmd);

		    memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "iptables -I INPUT -i %s -j ACCEPT", PON_IF);
		    system(cmd);
		}
		else if((mxu->cvlan > 0) && (mxu->svlan == 0))
		{		
			memset(cmd, 0, sizeof(cmd));
	        snprintf(cmd, sizeof(cmd), "vconfig add %s %u", PON_IF, mxu->cvlan);
	        ret = system(cmd);
	        if (-1 == ret || FALSE == WIFEXITED(ret) || 0 != WEXITSTATUS(ret))
	        {
	            return EPON_OAM_ERR_PARAM;
	        }
			
			memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "ifconfig %s.%u "IPADDR_PRINT" netmask "IPADDR_PRINT" up",
		             PON_IF, mxu->cvlan, IPADDR_PRINT_ARG(mxu->ip[0]), IPADDR_PRINT_ARG(mxu->mask));
		    system(cmd);

		    memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "iptables -I INPUT -i %s.%u -j ACCEPT", PON_IF, mxu->cvlan);
		    system(cmd);
		}
		else
		{
			//TODO: svlan and cvlan both set
		}
	}
	else
	{
		/* Remove */
		if((mxu->cvlan == 0) && (mxu->svlan == 0))
		{
		    memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "iptables -D INPUT -i %s -j ACCEPT", PON_IF);
		    system(cmd);

			memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "ifconfig %s down", PON_IF);
		    system(cmd);
		}
		else if((mxu->cvlan > 0) && (mxu->svlan == 0))
		{
			memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "iptables -D INPUT -i %s.%u -j ACCEPT", PON_IF, mxu->cvlan);
		    system(cmd);

			memset(cmd, 0, sizeof(cmd));
		    snprintf(cmd, sizeof(cmd), "ifconfig %s.%u down", PON_IF);
		    system(cmd);
			
			memset(cmd, 0, sizeof(cmd));
	        snprintf(cmd, sizeof(cmd), "vconfig rem %s.%u", PON_IF, mxu->cvlan);
	        system(cmd);
		}
		else
		{
			//TODO: svlan and cvlan both set
		}
	}

	return EPON_OAM_ERR_OK;
}

int ctc_sfu_mxuMngGlobal_set(ctc_wrapper_mxuMngGlobal_t * mxu)
{
	char cmd[128];
	const char PON_IF[] = "pon0";	
	const char NIC_PROC_PATH[] = "/proc/rtl8686gmac/dev_port_mapping";
	int ret = EPON_OAM_ERR_OK;
	
	if(mxuMngInit == 0)
	{
		memset(cmd, 0, sizeof(cmd));
    	snprintf(cmd, sizeof(cmd), "echo '%d %s' > %s", HAL_GET_PON_PORT(), PON_IF, NIC_PROC_PATH);
    	system(cmd);
		
		mxuMngInit = 1;
	}
	
	if(mxu->isIpv4)
	{
		if(mxu->ip[0] == 0)
		{
			/* Remove mxuMngGlobal config*/
			ret = ctc_set_ipv4_connectivity(0, &mxuMng);
            ctc_mxuMngGlobal_vlan_set(0, &mxuMng);
			memset(&mxuMng, 0, sizeof(ctc_wrapper_mxuMngGlobal_t));
		}
		else
		{
			/* Set mxuMngGlobal config*/
			memcpy(&mxuMng, mxu, sizeof(ctc_wrapper_mxuMngGlobal_t));
			ret = ctc_set_ipv4_connectivity(1, &mxuMng);
            ctc_mxuMngGlobal_vlan_set(1, &mxuMng);
		}
	}
	else
	{
		if((mxu->ip[0] == 0) && (mxu->ip[1] == 0) && (mxu->ip[2] == 0) && (mxu->ip[3] == 0))
		{
			/* Remove mxuMngGlobal config*/
			ret = ctc_set_ipv6_connectivity(0, &mxuMng);
            ctc_mxuMngGlobal_vlan_set(0, &mxuMng);
			memset(&mxuMng, 0, sizeof(ctc_wrapper_mxuMngGlobal_t));
		}
		else
		{
			/* Set mxuMngGlobal config*/
			memcpy(&mxuMng, mxu, sizeof(ctc_wrapper_mxuMngGlobal_t));
			ret = ctc_set_ipv6_connectivity(1, &mxuMng);
            ctc_mxuMngGlobal_vlan_set(1, &mxuMng);
		}
	}
	
	return ret;
}

int ctc_sfu_mxuMngGlobal_get(ctc_wrapper_mxuMngGlobal_t * mxu)
{
	if(mxuMngInit == 0)
		return EPON_OAM_ERR_NOT_FOUND;
	
	memcpy(mxu, &mxuMng, sizeof(ctc_wrapper_mxuMngGlobal_t));

	return EPON_OAM_ERR_OK;
}

int32 ctc_is_uniport(uint32 lport)
{
	if (lport < chipInfo.uniPortCnt)
		return 1;

	return 0;
}

static void ctc_onu_capability_init()
{
	FILE *fp = NULL;
	int wlan2g = 0;
	int wlan5g = 0;
	int i;
	
	fp=fopen("/proc/realtek/sys_capability","r");
	if (fp)
	{
		fscanf(fp,"%d;%d;%d;%d;%d;%d",  &chipInfo.uniFePortCnt, &chipInfo.uniGePortCnt,
			    &chipInfo.voipPortCnt, &chipInfo.usbPortCnt, &wlan2g, &wlan5g);

		chipInfo.uniPortCnt = chipInfo.uniFePortCnt + chipInfo.uniGePortCnt;
		chipInfo.wlanPortCnt = wlan2g + wlan5g;

		for(i = 0 ; i < chipInfo.uniGePortCnt ; i++)
	    {
	        chipInfo.uniPortCap[i] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
	    }

		for(i = chipInfo.uniGePortCnt; i < chipInfo.uniPortCnt; i++)
		{
			chipInfo.uniPortCap[i] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
		}
		
		fclose(fp);

		//printf("%s: FE[%d] GE[%d] voip[%d] usb[%d] wlan[%d]\n", __func__, chipInfo.uniFePortCnt,chipInfo.uniGePortCnt, chipInfo.voipPortCnt, chipInfo.usbPortCnt,chipInfo.wlanPortCnt);
	}
	else
	{
		/* for old code not support sys_capability info */
		for(i = 0 ; i < chipInfo.uniPortCnt ; i ++)
	    {
	        if(chipInfo.uniPortCap[i] & CTC_WRAPPER_PORTCAP_1000M)
	        {
	            chipInfo.uniGePortCnt++; 
	        }
	        else if(chipInfo.uniPortCap[i] & CTC_WRAPPER_PORTCAP_100M)
	        {
	            chipInfo.uniFePortCnt ++; 
	        }
	    }
		chipInfo.voipPortCnt = 0;
		chipInfo.usbPortCnt = 0;
		chipInfo.wlanPortCnt = 0;
	}
}

void ctc_wrapper_sfu_init(void)
{
    int i;

    /* Get current platform infomation */
    rtk_switch_version_get(&chipInfo.chipId, &chipInfo.rev, &chipInfo.subType);
    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
        switch(chipInfo.subType)
        {
        case APPOLOMP_CHIP_SUB_TYPE_RTL9602:
        case APPOLOMP_CHIP_SUB_TYPE_RTL9603:
            chipInfo.uniPortCnt = 4;
            chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            chipInfo.uniPortCap[2] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            chipInfo.uniPortCap[3] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            chipInfo.voipPortCnt = 2;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9601:
            chipInfo.uniPortCnt = 1;
            chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.voipPortCnt = 0;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9602B:
            chipInfo.uniPortCnt = 2;
            chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.voipPortCnt = 1;
            break;
        case APPOLOMP_CHIP_SUB_TYPE_RTL9606:
        case APPOLOMP_CHIP_SUB_TYPE_RTL9607:
        case APPOLOMP_CHIP_SUB_TYPE_RTL9607P:
        default:
            chipInfo.uniPortCnt = 4;
            chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.uniPortCap[2] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.uniPortCap[3] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            chipInfo.voipPortCnt = 2;
            break;
        }
        break;
    case RTL9601B_CHIP_ID:
        chipInfo.uniPortCnt = 1;
        chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.voipPortCnt = 0;
        break;
    case RTL9602C_CHIP_ID:
        chipInfo.uniPortCnt = 2;
        chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
        chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.voipPortCnt = 1;
        break;
    default:
        chipInfo.uniPortCnt = 4;
        chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.uniPortCap[2] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.uniPortCap[3] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        chipInfo.voipPortCnt = 2;
        break;
    }
    chipInfo.ponPortNo = HAL_GET_PON_PORT();
    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
        chipInfo.cfEntryCnt = 512;
        break;
    default:
        chipInfo.cfEntryCnt = 0;
    }

	/*update global lan port num value*/
	extern int LAN_PORT_NUM;
	LAN_PORT_NUM = chipInfo.uniPortCnt;

	ctc_onu_capability_init();
	
    /* Init wrapper function pointers */
    wrapper_func.portStateSet = ctc_sfu_portState_set;
    wrapper_func.portStateGet = ctc_sfu_portState_get;
    wrapper_func.portAutoNegoSet = ctc_sfu_portAutoNego_set;
    wrapper_func.portAutoNegoGet = ctc_sfu_portAutoNego_get;
    wrapper_func.portCapGet = ctc_sfu_portCap_get;
    wrapper_func.portLinkStatusGet = ctc_sfu_portLinkStatus_get;
    wrapper_func.portFlowControlSet = ctc_sfu_portFlowControl_set;
    wrapper_func.portFlowControlGet = ctc_sfu_portFlowControl_get;
    wrapper_func.portIngressBwSet = ctc_sfu_portIngressBw_set;
    wrapper_func.portIngressBwGet = ctc_sfu_portIngressBw_get;
    wrapper_func.portEgressBwSet = ctc_sfu_portEgressBw_set;
    wrapper_func.portEgressBwGet = ctc_sfu_portEgressBw_get;
    wrapper_func.portLoopDetectSet = ctc_sfu_portLoopDetect_set;
    wrapper_func.portLoopDetectGet = ctc_sfu_portLoopDetect_get;
    wrapper_func.portDisableLoopedSet = ctc_sfu_portDisableLooped_set;
    wrapper_func.portDisableLoopedGet = ctc_sfu_portDisableLooped_get;
	wrapper_func.portLoopParameterConfigSet = ctc_sfu_portLoopParameterConfig_set;
    wrapper_func.portMacAgingSet = ctc_sfu_portMacAging_set;
    wrapper_func.portMacAgingGet = ctc_sfu_portMacAging_get;
    wrapper_func.boardCapGet = ctc_sfu_boardCap_get;
    wrapper_func.transceiverStatusGet = ctc_sfu_transceiverStatus_get;
    wrapper_func.portAutoNegoAdvertiseGet = ctc_sfu_portAutoNegoAdvertise_get;
    wrapper_func.fecStateSet = ctc_sfu_fecState_set;
    wrapper_func.fecStateGet = ctc_sfu_fecState_get;
    wrapper_func.chipInfoGet = ctc_sfu_chipInfo_get;
    wrapper_func.vlanSet = ctc_sfu_vlan_set;
    wrapper_func.vlanGet = ctc_sfu_vlan_get;
    wrapper_func.pmStatusSet = ctc_sfu_pmStatus_set;
    wrapper_func.pmStatusGet = ctc_sfu_pmStatus_get;
    wrapper_func.pmStatUpdate = ctc_sfu_pmStat_update;
    wrapper_func.pmStatSet = ctc_sfu_pmStat_set;
    wrapper_func.pmStatGet = ctc_sfu_pmStat_get;
	wrapper_func.ponLoopbackSet = ctc_sfu_loopback_set;
	wrapper_func.mxuMngGlobalSet = ctc_sfu_mxuMngGlobal_set;
	wrapper_func.mxuMngGlobalGet = ctc_sfu_mxuMngGlobal_get;
	wrapper_func.portAutoNegoRestart = ctc_sfu_portAutoNegoRestart_set;

    /* Init per-port configurations */
    for(i = 0 ; i < CTC_WRAPPER_NUM_UNIPORT ; i++)
    {
        ingressBwDb[i].enable = DISABLED;
        egressBwDb[i].enable = DISABLED;
    }

    /* Init PM state & period for all ports */
    for(i = 0 ; i < CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT ; i++)
    {
        pmStatistics[i].pmState = DISABLED;
        pmStatistics[i].pmPeriod = 0x384; /* Default 15 minutes */
        pmStatistics[i].pmPeriodCur = 0;
        memset(&pmStatistics[i].pmData[0], 0, sizeof(ctc_pm_statistics_t) * 2);
        pmStatistics[i].pmDataCur = &pmStatistics[i].pmData[0];
        pmStatistics[i].pmDataHis = &pmStatistics[i].pmData[1];
    }

#ifndef CONFIG_EPON_OAM_DUMMY_MODE
    /* port remapping must be done before any port based operation */
    ctc_oam_port_remapping_init();

	port_cfg_init();

	drv_mac_init();

	drv_vlan_init();
	
	drv_qos_init();

    /* Configure default classification entry */
    ctc_sfu_cf_init();
	
    /* Configure default vlan mode */
    ctc_sfu_vlan_init();

    /* Init RLDP module */
    ctc_sfu_rldp_init();
#endif
}
#endif

