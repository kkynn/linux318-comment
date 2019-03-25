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
 * Purpose : CTC proprietary behavior HGU wrapper APIs
 *
 * Feature : Provide the wapper layer for CTC HGU application
 *
 */

#ifdef CONFIG_HGU_APP

#include <stdio.h>
#include<stdlib.h>
#include "ctc_wrapper.h"
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>
#include <dal/apollomp/dal_apollomp_switch.h>
/* RomerDriver related include */
#include "rtk_rg_struct.h"
#include "rtk_rg_define.h"

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
#include "ctc_hgu.h"
#include <sys_def.h>

#include "ctc_vlan.h"

#include <common/debug/rt_log.h>
#include <osal/lib.h>
#include <ioal/mem32.h>
#include <hal/chipdef/chip.h>
#include <hal/common/halctrl.h>
#include <rtk/oam.h>

static ctc_wrapper_chipInfo_t chipInfo;
/* Data is stored in the order of PON IF -> UNI PORT */
static ctc_hgu_pm_t pmStatistics[CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT];
static unsigned int ulQosTrustMode[MAX_PORT_NUM];

uint8 SYSTEM_ADDR[MAC_ADDR_LEN];

#if 0
int32 ctc_hgu_qos_trust_mode_cfg_get(uint32 lport, uint32 *pulQosTrustMode)
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

int32 ctc_hgu_qos_trust_mode_set(uint32 mode)
{
	rtk_rg_qos_priSelWeight_t stPriSelWeight;

	memset(&stPriSelWeight, 0, sizeof(rtk_rg_qos_priSelWeight_t));

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

	if(RT_ERR_OK != rtk_rg_qosInternalPriDecisionByWeight_set(stPriSelWeight))
	{
		return RT_ERR_FAILED;
	}

	return RT_ERR_OK;
}
#endif 

int32 ctc_sys_init(void)
{
	rtk_epon_regReq_t regReq;
	
	/*Get MAC Address of ONU*/
    rtk_rg_epon_registerReq_get(&regReq);
	//apollomp_raw_epon_regMac_get(&switch_mac);
	memcpy(SYSTEM_ADDR, regReq.mac.octet, ETHER_ADDR_LEN);
	 return RT_ERR_OK;
}

static void ctc_hgu_AclandVlanDB_init(void)
{
	ctc_sys_init();
	ctc_hgu_AclDB_init();
	ctc_hgu_VlanCfgDB_init();
    return;
}

static void ctc_hgu_rldp_init(void)
{
    rtk_rldp_config_t config;

    switch(chipInfo.chipId)
    {
    case APOLLOMP_CHIP_ID:
    case RTL9602C_CHIP_ID:
       // rtk_rldp_init();
        rtk_rg_rldp_config_get(&config);
        config.rldp_enable = ENABLED;
        rtk_rg_rldp_config_set(&config);
        break;
    case RTL9601B_CHIP_ID:
    default:
        break;
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

int32 is9603(void)
{
	if (APOLLOMP_CHIP_ID == chipInfo.chipId)
	{
		if(APPOLOMP_CHIP_SUB_TYPE_RTL9603 == chipInfo.subType)
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

static void ctc_hgu_pmUpdatFromMib(ctc_pm_statistics_t *pmStat, rtk_stat_port_cntr_t mibCnt, int isLan)
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

int ctc_hgu_boardCap_get(ctc_boardCap_t *pBoardCap)
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

int ctc_hgu_transceiverStatus_get(unsigned int statusType, unsigned short *pStatusData)
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
    ret = rtk_rg_ponmac_transceiver_get(type, &data);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pStatusData = data.buf[0] << 8 | data.buf[1];

    return EPON_OAM_ERR_OK;
}

int ctc_hgu_fecState_set(rtk_enable_t state)
{
    int ret;

    CTC_WRAPPER_PARAM_CHECK((state >= RTK_ENABLE_END), EPON_OAM_ERR_PARAM);

    switch(chipInfo.chipId)
    {
#ifdef CONFIG_SDK_APOLLOMP
    case APOLLOMP_CHIP_ID:
        if((ret = rtk_rg_epon_fecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        if((ret = rtk_rg_epon_usFecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
#endif
    default:
        if((ret = rtk_rg_epon_usFecState_set(state)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    }
    
    return EPON_OAM_ERR_OK;
}

int ctc_hgu_fecState_get(rtk_enable_t *pState)
{
    int ret;
    rtk_enable_t enable;

    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);

    switch(chipInfo.chipId)
    {
#ifdef CONFIG_SDK_APOLLOMP
    case APOLLOMP_CHIP_ID:
        if((ret = rtk_rg_epon_fecState_get(&enable)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
#endif
    default:
        if((ret = rtk_rg_epon_usFecState_get(&enable)) != RT_ERR_OK)
        {
            return EPON_OAM_ERR_WRAPPER;
        }
        break;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_hgu_chipInfo_get(ctc_wrapper_chipInfo_t *pChipInfo)
{
    CTC_WRAPPER_PARAM_CHECK((NULL == pChipInfo), EPON_OAM_ERR_PARAM);

    *pChipInfo = chipInfo;

    return EPON_OAM_ERR_OK;
}

int ctc_hgu_pmStatus_set(unsigned int intf, rtk_enable_t state, unsigned int period)
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

int ctc_hgu_pmStatus_get(unsigned int intf, rtk_enable_t *pState, unsigned int *pPeriod)
{
    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pState), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((NULL == pPeriod), EPON_OAM_ERR_PARAM);
    
    *pState = pmStatistics[intf].pmState;
    *pPeriod = pmStatistics[intf].pmPeriod;

    return EPON_OAM_ERR_OK;
}

void ctc_hgu_pmStat_update(unsigned int time)
{
    int i;
    int phyPort;
    rtk_stat_port_cntr_t portCnt;
    ctc_pm_statistics_t *tmp;

    for(i = 0 ; i < chipInfo.uniPortCnt + 1 ; i++)
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
                    rtk_rg_stat_port_getAll(HAL_GET_PON_PORT(), &portCnt);
					rtk_rg_stat_port_reset(HAL_GET_PON_PORT());
					ctc_hgu_pmUpdatFromMib(pmStatistics[i].pmDataCur, portCnt, 0);
                }
                else
                {
                    /* UNI ports */
					phyPort = PortLogic2PhyID(i-1);
					if (INVALID_PORT == phyPort)
						continue;
                    rtk_rg_stat_port_getAll(phyPort, &portCnt);
					rtk_rg_stat_port_reset(phyPort);
					ctc_hgu_pmUpdatFromMib(pmStatistics[i].pmDataCur, portCnt, 1);
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

int ctc_hgu_pmStat_set(unsigned int intf)
{
    int phyPort;
    rtk_stat_port_cntr_t portCnt;
    ctc_pm_statistics_t *tmp;

    CTC_WRAPPER_PARAM_CHECK((intf >= CTC_WRAPPER_NUM_PONIF + CTC_WRAPPER_NUM_UNIPORT), EPON_OAM_ERR_PARAM);

    /* Force statistics period endded */
    if(0 == intf)
    {
        /* PON interface */
        rtk_rg_stat_port_getAll(HAL_GET_PON_PORT(), &portCnt);
		ctc_hgu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 0);
    }
    else
    {
        /* UNI ports */
		phyPort = PortLogic2PhyID(intf - 1);
		if (INVALID_PORT == phyPort)
			return EPON_OAM_ERR_WRAPPER;
        rtk_rg_stat_port_getAll(phyPort, &portCnt);
		ctc_hgu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 1);
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

int ctc_hgu_pmStat_get(unsigned int intf, ctc_pm_statistics_t *pmStat, ctc_pm_statisticsType_t pmType)
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
			ctc_hgu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 0);
        }
        else
        {
            /* UNI ports */
			phyPort = PortLogic2PhyID(intf-1);
			if (INVALID_PORT == phyPort)
				return EPON_OAM_ERR_NOT_FOUND;
			
            rtk_stat_port_getAll(phyPort, &portCnt);
			ctc_hgu_pmUpdatFromMib(pmStatistics[intf].pmDataCur, portCnt, 1);
        }
        
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

int ctc_hgu_loopback_set(rtk_enable_t state)
{
	int ponPort;

	ponPort = PortLogic2PhyID(LOGIC_PON_PORT);
	
	if (ENABLED == state)//Enable
		rtk_oam_parserAction_set(ponPort, OAM_PARSER_ACTION_LOOPBACK);
	else if (DISABLED == state)//Disable
		rtk_oam_parserAction_set(ponPort, OAM_PARSER_ACTION_FORWARD);

	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portLoopDetect_set(unsigned int port, rtk_enable_t enable)
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

int ctc_hgu_portDisableLooped_set(unsigned int port, rtk_enable_t enable)
{
    int ret;
    rtk_rldp_portConfig_t portConfig;

    CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
    CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);

    if(set_port_loopdetect_autodown(port, enable)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;

    return EPON_OAM_ERR_OK;
}

int ctc_hgu_portLoopParameterConfig_set(unsigned int port, unsigned short detectFrequency, 
								unsigned short recoveryInterval, short * svlan, short * cvlan)
{	
	int ret;
	CTC_WRAPPER_PARAM_CHECK((port >= chipInfo.uniPortCnt), EPON_OAM_ERR_PARAM);
	CTC_WRAPPER_PARAM_CHECK((port == chipInfo.ponPortNo), EPON_OAM_ERR_PARAM);
	
	if(set_port_loopdetect_parameterConfig(port, detectFrequency, recoveryInterval, svlan, cvlan)!=RT_ERR_OK)
		return EPON_OAM_ERR_PARAM;
		
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portLinkStatus_get (unsigned int port, rtk_enable_t *pState)
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
	
    ret = rtk_rg_port_link_get(phyPort, &status);
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

int ctc_hgu_portState_get(unsigned int port, rtk_enable_t *pState)
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
	
    ret = rtk_rg_port_adminEnable_get(phyPort, &enable);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_hgu_portAutoNego_get(unsigned int port, rtk_enable_t *pState)
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
	
    ret = rtk_rg_port_phyAutoNegoEnable_get(phyPort, &enable);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }
    *pState = enable;
    
    return EPON_OAM_ERR_OK;
}

int ctc_hgu_portFlowControl_get(unsigned int port, rtk_enable_t *pState)
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
	
    ret = rtk_rg_port_phyAutoNegoAbility_get(phyPort, &ability);
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

int ctc_hgu_potsStatus_get(unsigned int port, unsigned int * portState, unsigned int * serviceState, unsigned int * codecMode)
{
	/* FIXEME: voip only have one port now, not check port */

	voip_oam_state_init_variables();
	
	*portState = ctc_hgu_voip_IADPortStauts(port);
	*serviceState = ctc_hgu_voip_iadPortServiceState(port);
	*codecMode = ctc_hgu_voip_iadPortCodecMode_get(port);

	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portState_set(unsigned int port, rtk_enable_t state)
{
	/* avoid oam register failure, so do nothing and just return EPON_OAM_ERR_OK */
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portAutoNego_set(unsigned int port, rtk_enable_t state)
{
	/* avoid oam register failure, so do nothing and just return EPON_OAM_ERR_OK */
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portFlowControl_set(unsigned int port, rtk_enable_t state)
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
	
    ret = rtk_rg_port_phyAutoNegoAbility_get(phyPort, &ability);
    if(ret != RT_ERR_OK)
    {
        return EPON_OAM_ERR_WRAPPER;
    }

#ifndef YUEME_CUSTOMIZED_CHANGE /* ignore the flowcontrol set */
	/* don't set flowcontrol state if it's not changed to avoid lan port down problem */
	if( (ability.FC && (state == DISABLED)) || ((ability.FC == 0) && (state == ENABLED)))
	{
	    ability.FC = state;
	    ability.AsyFC = state;

	    ret = rtk_rg_port_phyAutoNegoAbility_set(phyPort, &ability);
	    if(ret != RT_ERR_OK)
	    {
	        return EPON_OAM_ERR_WRAPPER;
	    }
	}
#endif
    
    return EPON_OAM_ERR_OK;
}

int ctc_hgu_portIngressBw_set(unsigned int port, rtk_enable_t enable, unsigned int cir, unsigned int cbs, unsigned int ebs)
{
	/* avoid oam register failure, so do nothing and just return EPON_OAM_ERR_OK */
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portEgressBw_set(unsigned int port, rtk_enable_t enable, unsigned int cir, unsigned int pir)
{
	/* avoid oam register failure, so do nothing and just return EPON_OAM_ERR_OK */
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_portMacAging_set(unsigned int port, rtk_enable_t enable, unsigned int agingTime)
{
	/* avoid oam register failure, so do nothing and just return EPON_OAM_ERR_OK */
	return EPON_OAM_ERR_OK;
}

int ctc_hgu_fastLeaveAbility_get(int * fastLeaveEnable)
{
#ifdef YUEME_CUSTOMIZED_CHANGE
	rtk_rg_igmpMldSnoopingControl_t config;
	int ret;

	ret = rtk_rg_igmpMldSnoopingControl_get(&config);
	if(ret == RT_ERR_RG_OK)
	{
		*fastLeaveEnable = config.enableFastLeave;
	}
	else
		*fastLeaveEnable = 1; /* default enable fastleave */
#else
	*fastLeaveEnable = 1;	/* not call rg function to avoid compile error when rg code is old */
#endif

	return EPON_OAM_ERR_OK;
}

int32 ctc_is_uniport(uint32 lport)
{
	if (lport < chipInfo.uniPortCnt)
		return 1;

	return 0;
}

/* capability value format: FE;GE;VOIP;USB;2.4G;5G , e.g. 1;1;1;0;0;0 */
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

		//printf("%s old code: FE[%d] GE[%d]\n", __func__, chipInfo.uniFePortCnt,chipInfo.uniGePortCnt);
	}
}

void ctc_wrapper_hgu_init(void)
{
    int i;
	/* Get current platform infomation */
    rtk_rg_switch_version_get(&chipInfo.chipId, &chipInfo.rev, &chipInfo.subType);
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
            	break;
        	case APPOLOMP_CHIP_SUB_TYPE_RTL9601:
            	chipInfo.uniPortCnt = 1;
           		chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
            	break;
        	case APPOLOMP_CHIP_SUB_TYPE_RTL9602B:
            	chipInfo.uniPortCnt = 2;
            	chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
            	chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
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
            	break;
        }
        break;
    	case RTL9601B_CHIP_ID:
        	chipInfo.uniPortCnt = 1;
        	chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	break;
    	case RTL9602C_CHIP_ID:
        	chipInfo.uniPortCnt = 2;
        	chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M;
        	chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	break;
    	default:
        	chipInfo.uniPortCnt = 4;
        	chipInfo.uniPortCap[0] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	chipInfo.uniPortCap[1] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	chipInfo.uniPortCap[2] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	chipInfo.uniPortCap[3] = CTC_WRAPPER_PORTCAP_10M | CTC_WRAPPER_PORTCAP_100M | CTC_WRAPPER_PORTCAP_1000M;
        	break;
    }

	ctc_onu_capability_init();

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
	
	/* Init wrapper function pointers */
    wrapper_func.fecStateSet = ctc_hgu_fecState_set;
    wrapper_func.fecStateGet = ctc_hgu_fecState_get;
    wrapper_func.chipInfoGet = ctc_hgu_chipInfo_get;
    wrapper_func.pmStatusSet = ctc_hgu_pmStatus_set;
    wrapper_func.pmStatusGet = ctc_hgu_pmStatus_get;
    wrapper_func.pmStatUpdate = ctc_hgu_pmStat_update;
    wrapper_func.pmStatSet = ctc_hgu_pmStat_set;
    wrapper_func.pmStatGet = ctc_hgu_pmStat_get;
    wrapper_func.boardCapGet = ctc_hgu_boardCap_get;
    wrapper_func.transceiverStatusGet = ctc_hgu_transceiverStatus_get;
	wrapper_func.ponLoopbackSet = ctc_hgu_loopback_set;
	wrapper_func.portLoopDetectSet = ctc_hgu_portLoopDetect_set;
	wrapper_func.portDisableLoopedSet = ctc_hgu_portDisableLooped_set;
	wrapper_func.portLoopParameterConfigSet = ctc_hgu_portLoopParameterConfig_set;
	wrapper_func.portLinkStatusGet = ctc_hgu_portLinkStatus_get;
	wrapper_func.portStateGet = ctc_hgu_portState_get;
	wrapper_func.portAutoNegoGet = ctc_hgu_portAutoNego_get;
    wrapper_func.portFlowControlGet = ctc_hgu_portFlowControl_get;
	wrapper_func.potsStatusGet = ctc_hgu_potsStatus_get;

	wrapper_func.portStateSet = ctc_hgu_portState_set;
	wrapper_func.portAutoNegoSet = ctc_hgu_portAutoNego_set;
	wrapper_func.portFlowControlSet = ctc_hgu_portFlowControl_set;
	wrapper_func.portIngressBwSet = ctc_hgu_portIngressBw_set;
	wrapper_func.portEgressBwSet = ctc_hgu_portEgressBw_set;
	wrapper_func.portMacAgingSet = ctc_hgu_portMacAging_set;

	wrapper_func.fastLeaveAbilityGet = ctc_hgu_fastLeaveAbility_get;
	
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
    
	/* Martin ZHU note:add init func */
	drv_mac_init();
	
	drv_vlan_init();//set PON port as service port
	 /* init Acl-VLAN DB */
    ctc_hgu_AclandVlanDB_init();
	
    /* Init RLDP module */
    ctc_hgu_rldp_init();
#endif
}

#endif

