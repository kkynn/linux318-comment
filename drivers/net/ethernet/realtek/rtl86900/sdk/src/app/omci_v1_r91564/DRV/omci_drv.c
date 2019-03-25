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
 * $Revision: 39101 $
 * $Date: 2013-06-24 04:35:27 -0500 (Fri, 03 May 2013) $
 *
 * Purpose : OMCI driver layer module defination
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI (G.984.4)
 *
 */

/*
 * Include Files
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
/* needed for misc device */
#include <linux/miscdevice.h>
/* needed for remap_page_range */
#include <linux/mm.h>
/* needed for virt_to_phys() */
#include <asm/io.h>
#include <linux/vmalloc.h>

/*since ioctl, will modify later*/
#include <DRV/omci_drv.h>

#include <linux/unistd.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm/uaccess.h>
#include <linux/netfilter.h>

omci_drv_control_t gDrvCtrl;

pf_wrapper_t *pWrapper = NULL;
pf_db_t *pPlatformDb = NULL;
char            buf[READ_BUF_LEN];
char            tmp[READ_BUF_LEN];


int omcidrv_wrapper_createTcont(OMCI_TCONT_ts* pTcont)
{
    int ret;

    if(pWrapper && pWrapper->pf_CreateTcont)
    {
        ret = pWrapper->pf_CreateTcont(pTcont);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_updateTcont(OMCI_TCONT_ts* pTcont)
{
    int ret;

    if(pWrapper && pWrapper->pf_UpdateTcont)
    {
        ret = pWrapper->pf_UpdateTcont(pTcont);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_cfgGemFlow(OMCI_GEM_FLOW_ts* pGemFlow)
{
    int ret;

    if(pWrapper && pWrapper->pf_CfgGemFlow)
    {
        ret = pWrapper->pf_CfgGemFlow(pGemFlow);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPriQueue(OMCI_PRIQ_ts* pPriQ)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetPriQueue)
    {
        ret = pWrapper->pf_SetPriQueue(pPriQ);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_deactiveBdgConn(int srvId)
{
    int ret;

    if(pWrapper && pWrapper->pf_DeactiveBdgConn)
    {
        ret = pWrapper->pf_DeactiveBdgConn(srvId);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_activeBdgConn(OMCI_BRIDGE_RULE_ts* pBridgeRule)
{
    int ret;

    if(pWrapper && pWrapper->pf_ActiveBdgConn)
    {
        ret = pWrapper->pf_ActiveBdgConn(pBridgeRule);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_setSerialNum(char* pSerialNum)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetSerialNum)
    {
        ret = pWrapper->pf_SetSerialNum(pSerialNum);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_getSerialNum(char* pSerialNum)
{
    int ret;

    if(pWrapper && pWrapper->pf_GetSerialNum)
    {
        ret = pWrapper->pf_GetSerialNum(pSerialNum);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_setGponPasswd(char* pGponPwd)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetGponPasswd)
    {
        ret = pWrapper->pf_SetGponPasswd(pGponPwd);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_activateGpon(int activate)
{
    int ret;

    if(pWrapper && pWrapper->pf_ActivateGpon)
    {
        ret = pWrapper->pf_ActivateGpon(activate);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_resetMib(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_ResetMib)
    {
        ret = pWrapper->pf_ResetMib();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_clearPriQueue(OMCI_PRIQ_ts* pPriQ)
{
    int ret;

    if(pWrapper && pWrapper->pf_ClearPriQueue)
    {
        ret = pWrapper->pf_ClearPriQueue(pPriQ);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}


int omcidrv_wrapper_dumpL2Serv(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_DumpL2Serv)
    {
        ret = pWrapper->pf_DumpL2Serv();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}


int omcidrv_wrapper_dumpVeipServ(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_DumpVeipServ)
    {
        ret = pWrapper->pf_DumpVeipServ();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpMBServ(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_DumpMBServ)
    {
        ret = pWrapper->pf_DumpMBServ();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpCfMap(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_DumpCfMap)
    {
        ret = pWrapper->pf_DumpCfMap();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setCfMap(unsigned int cfType, unsigned int start, unsigned int stop)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetCfMap)
    {
        ret = pWrapper->pf_SetCfMap(cfType, start, stop);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setLog(int loglevel)
{
    gDrvCtrl.logLevel  = loglevel;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Set Driver Log Level %d",gDrvCtrl.logLevel);
    return OMCI_ERR_OK;
}

int omcidrv_wrapper_setDevMode(int dMode)
{
    gDrvCtrl.devMode = dMode;
    return OMCI_ERR_OK;
}

int omcidrv_wrapper_setDscpRemap(OMCI_DSCP2PBIT_ts *pDscp2PbitTable)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetDscpRemap)
    {
        ret = pWrapper->pf_SetDscpRemap(pDscp2PbitTable);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_updateVeipRule(int wanIdx, int vid, int pri, int type, int service, int isBinding, int netIfIdx, unsigned char isRegister)
{
    int ret;

    if(pWrapper && pWrapper->pf_UpdateVeipRule)
    {
        ret = pWrapper->pf_UpdateVeipRule(wanIdx, vid, pri, type, service, isBinding, netIfIdx, isRegister);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_setMacLearnLimit(OMCI_MACLIMIT_ts *pMacLimit)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetMacLearnLimit)
    {
        ret = pWrapper->pf_SetMacLearnLimit(pMacLimit);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setMacFilter(OMCI_MACFILTER_ts *pMacFilter)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetMacFilter)
    {
        ret = pWrapper->pf_SetMacFilter(pMacFilter);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getDrvVersion(char* pDrvVersion)
{
    int ret;

    if(pWrapper && pWrapper->pf_GetDrvVersion)
    {
        ret = pWrapper->pf_GetDrvVersion(pDrvVersion);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_getOnuState(PON_ONU_STATE *pOnuState)
{
    int ret;

    if(pWrapper && pWrapper->pf_GetOnuState)
    {
        ret = pWrapper->pf_GetOnuState(pOnuState);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_setSignalParameter(OMCI_SIGNAL_PARA_ts *pSigPara)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetSigParameter)
    {
        ret = pWrapper->pf_SetSigParameter(pSigPara);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

int omcidrv_wrapper_getDevCapabilities(omci_dev_capability_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetDevCapabilities)
    {
        ret = pWrapper->pf_GetDevCapabilities(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getDevIdVersion(omci_dev_id_version_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetDevIdVersion)
    {
        ret = pWrapper->pf_GetDevIdVersion(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setDualMgmtMode(int state)
{
    gDrvCtrl.dmMode = state;

    return OMCI_ERR_OK;
}

int omcidrv_wrapper_getUsDBRuStatus(unsigned int *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetUsDBRuStatus)
    {
        ret = pWrapper->pf_GetUsDBRuStatus(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getTransceiverStatus(omci_transceiver_status_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetTransceiverStatus)
    {
        ret = pWrapper->pf_GetTransceiverStatus(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortLinkStatus(omci_port_link_status_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortLinkStatus)
    {
        ret = pWrapper->pf_GetPortLinkStatus(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortSpeedDuplexStatus(omci_port_speed_duplex_status_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortSpeedDuplexStatus)
    {
        ret = pWrapper->pf_GetPortSpeedDuplexStatus(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortAutoNegoAbility(omci_port_auto_nego_ability_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortAutoNegoAbility)
    {
        ret = pWrapper->pf_SetPortAutoNegoAbility(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortAutoNegoAbility(omci_port_auto_nego_ability_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortAutoNegoAbility)
    {
        ret = pWrapper->pf_GetPortAutoNegoAbility(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortState(omci_port_state_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortState)
    {
        ret = pWrapper->pf_SetPortState(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortState(omci_port_state_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortState)
    {
        ret = pWrapper->pf_GetPortState(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortMaxFrameSize)
    {
        ret = pWrapper->pf_SetPortMaxFrameSize(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortMaxFrameSize)
    {
        ret = pWrapper->pf_GetPortMaxFrameSize(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortPhyLoopback(omci_port_loopback_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortPhyLoopback)
    {
        ret = pWrapper->pf_SetPortPhyLoopback(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortPhyLoopback(omci_port_loopback_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortPhyLoopback)
    {
        ret = pWrapper->pf_GetPortPhyLoopback(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortPhyPwrDown)
    {
        ret = pWrapper->pf_SetPortPhyPwrDown(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortPhyPwrDown)
    {
        ret = pWrapper->pf_GetPortPhyPwrDown(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPortStat(omci_port_stat_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPortStat)
    {
        ret = pWrapper->pf_GetPortStat(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_resetPortStat(unsigned int port)
{
    int ret;

    if (pWrapper && pWrapper->pf_ResetPortStat)
    {
        ret = pWrapper->pf_ResetPortStat(port);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getUsFlowStat(omci_flow_stat_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetUsFlowStat)
    {
        ret = pWrapper->pf_GetUsFlowStat(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_resetUsFlowStat(unsigned int flow)
{
    int ret;

    if (pWrapper && pWrapper->pf_ResetUsFlowStat)
    {
        ret = pWrapper->pf_ResetUsFlowStat(flow);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getDsFlowStat(omci_flow_stat_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetDsFlowStat)
    {
        ret = pWrapper->pf_GetDsFlowStat(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_resetDsFlowStat(unsigned int flow)
{
    int ret;

    if (pWrapper && pWrapper->pf_ResetDsFlowStat)
    {
        ret = pWrapper->pf_ResetDsFlowStat(flow);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getDsFecStat(omci_ds_fec_stat_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetDsFecStat)
    {
        ret = pWrapper->pf_GetDsFecStat(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_resetDsFecStat(void)
{
    int ret;

    if (pWrapper && pWrapper->pf_ResetDsFecStat)
    {
        ret = pWrapper->pf_ResetDsFecStat();
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setSvlanTpid(omci_svlan_tpid_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetSvlanTpid)
    {
        ret = pWrapper->pf_SetSvlanTpid(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getSvlanTpid(omci_svlan_tpid_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetSvlanTpid)
    {
        ret = pWrapper->pf_GetSvlanTpid(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getCvlanState(unsigned int *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetCvlanState)
    {
        ret = pWrapper->pf_GetCvlanState(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getGemBlkLen(unsigned short *pGemBlkLen)
{
    int ret;

    if(pWrapper && pWrapper->pf_GetGemBlkLen)
    {
        ret = pWrapper->pf_GetGemBlkLen(pGemBlkLen);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setGemBlkLen(unsigned short gemBlkLen)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetGemBlkLen)
    {
        ret = pWrapper->pf_SetGemBlkLen(gemBlkLen);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpMacFilter(void)
{
    int ret;

    if(pWrapper && pWrapper->pf_DumpMacFilter)
    {
        ret = pWrapper->pf_DumpMacFilter();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setGroupMacFilter(OMCI_GROUPMACFILTER_ts *pGroupMacFilter)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetGroupMacFilter)
    {
        ret = pWrapper->pf_SetGroupMacFilter(pGroupMacFilter);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPonBwThreshold(omci_pon_bw_threshold_t *pPonBwThreshold)
{
    int ret;

    if(pWrapper && pWrapper->pf_SetPonBwThreshold)
    {
        ret = pWrapper->pf_SetPonBwThreshold(pPonBwThreshold);
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpFlow2dsPq(void)
{
    int ret;

    if (pWrapper && pWrapper->pf_DumpFlow2dsPq)
    {
        ret = pWrapper->pf_DumpFlow2dsPq();
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpUsVeipGemFlow(void)
{
    int ret;

    if (pWrapper && pWrapper->pf_DumpVeipGemFlow)
    {
        ret = pWrapper->pf_DumpVeipGemFlow();
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setUsVeipGemFlow(veipGemFlow_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetVeipGemFlow)
    {
        ret = pWrapper->pf_SetVeipGemFlow(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_delUsVeipGemFlow(veipGemFlow_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_DelVeipGemFlow)
    {
        ret = pWrapper->pf_DelVeipGemFlow(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setUniPortRate(omci_port_rate_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetUniPortRate)
    {
        ret = pWrapper->pf_SetUniPortRate(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPauseControl(omci_port_pause_ctrl_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPauseControl)
    {
        ret = pWrapper->pf_SetPauseControl(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_getPauseControl(omci_port_pause_ctrl_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetPauseControl)
    {
        ret = pWrapper->pf_GetPauseControl(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setDsBcGemFlow(unsigned int *pFlowId)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetDsBcGemFlow)
    {
        ret = pWrapper->pf_SetDsBcGemFlow(pFlowId);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetDot1RateLimiter)
    {
        ret = pWrapper->pf_SetDot1RateLimiter(pDot1RateMeter);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_delDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    int ret;

    if (pWrapper && pWrapper->pf_DelDot1RateLimiter)
    {
        ret = pWrapper->pf_DelDot1RateLimiter(pDot1RateMeter);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}


int omcidrv_wrapper_getBgTblPerPort(omci_bridge_tbl_per_port_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_GetBgTblPerPort)
    {
        ret = pWrapper->pf_GetBgTblPerPort(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setMacAgeingTime(unsigned int ageTime)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetMacAgeTime)
    {
        ret = pWrapper->pf_SetMacAgeTime(ageTime);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setLoidAuthStatus(omci_event_msg_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetLoidAuthStatus)
    {
        ret = pWrapper->pf_SetLoidAuthStatus(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_sendOmciEvent(omci_event_msg_t *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SendOmciEvent)
    {
        ret = pWrapper->pf_SendOmciEvent(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setForceEmergencyStop(unsigned int *pState)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetForceEmergencyStop)
    {
        ret = pWrapper->pf_SetForceEmergencyStop(pState);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setPortBridging(unsigned int enable)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetPortBridging)
    {
        ret = pWrapper->pf_SetPortBridging(enable);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_setFloodingPortMask(omci_flood_port_info *p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetFloodingPortMask)
    {
        ret = pWrapper->pf_SetFloodingPortMask(p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }
    return ret;
}

int omcidrv_wrapper_setWanQueueNum(int num)
{
    gDrvCtrl.wanQueueNum = num;

    return OMCI_ERR_OK;
}

int omcidrv_wrapper_setPortRemap(int *p)
{
    memcpy(gDrvCtrl.portRemap, p, (sizeof(int) * OMCI_PORT_REMAP_MAX_INDEX));

    return OMCI_ERR_OK;
}


int omcidrv_wrapper_setTodInfo ( omci_tod_info_t* p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetTodInfo ) {
        ret = pWrapper->pf_SetTodInfo (p);
    } else {
        ret = OMCI_ERR_FAILED;
    }

    return (ret);
}

int omcidrv_wrapper_setUniQos(omci_uni_qos_info_t* p)
{
    int ret;

    if (pWrapper && pWrapper->pf_SetUniQosInfo )
    {
        ret = pWrapper->pf_SetUniQosInfo (p);
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return (ret);
}

int omcidrv_wrapper_dumpUniQos(void)
{
    int ret;

    if (pWrapper && pWrapper->pf_DumpUniQos)
    {
        ret = pWrapper->pf_DumpUniQos();
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}

int omcidrv_wrapper_dumpDebugInfo(void)
{
    int ret;

    if (pWrapper && pWrapper->pf_DumpDebugInfo)
    {
        ret = pWrapper->pf_DumpDebugInfo();
    }
    else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;
}


int omcidrv_ioctl(OMCI_IOCTL_t cmd,void *pArg)
{
    int ret = OMCI_ERR_OK;
    switch(cmd){
    case OMCI_IOCTL_TCONT_GET:
        ret = omcidrv_wrapper_createTcont((OMCI_TCONT_ts*)pArg);
        break;
    case OMCI_IOCTL_TCONT_UPDATE:
        ret = omcidrv_wrapper_updateTcont((OMCI_TCONT_ts*)pArg);
        break;
    case OMCI_IOCTL_GEMPORT_SET:
        ret = omcidrv_wrapper_cfgGemFlow((OMCI_GEM_FLOW_ts*)pArg);
        break;
    case OMCI_IOCTL_PRIQ_SET:
        ret = omcidrv_wrapper_setPriQueue((OMCI_PRIQ_ts*)pArg);
        break;
    case OMCI_IOCTL_CF_DEL:
        ret = omcidrv_wrapper_deactiveBdgConn(*((int*)pArg));
        break;
    case OMCI_IOCTL_CF_ADD:
        ret = omcidrv_wrapper_activeBdgConn((OMCI_BRIDGE_RULE_ts*)pArg);
        break;
    case OMCI_IOCTL_SN_SET:
        ret = omcidrv_wrapper_setSerialNum((char*)pArg);
        break;
    case OMCI_IOCTL_SN_GET:
        ret = omcidrv_wrapper_getSerialNum((char*)pArg);
        break;
    case OMCI_IOCTL_MIB_RESET:
        ret = omcidrv_wrapper_resetMib();
        break;
    case OMCI_IOCTL_LOG_SET:
        ret = omcidrv_wrapper_setLog(*((int*)pArg));
        break;
    case OMCI_IOCTL_PRIQ_DEL:
        ret = omcidrv_wrapper_clearPriQueue((OMCI_PRIQ_ts*)pArg);
        break;
    case OMCI_IOCTL_DEVMODE_SET:
        ret = omcidrv_wrapper_setDevMode(*((int*)pArg));
        break;
    case OMCI_IOCTL_DSCPREMAP_SET:
        ret = omcidrv_wrapper_setDscpRemap((OMCI_DSCP2PBIT_ts *)pArg);
        break;
    case OMCI_IOCTL_GPONPWD_SET:
        ret = omcidrv_wrapper_setGponPasswd((char*)pArg);
        break;
    case OMCI_IOCTL_GPON_ACTIVATE:
        ret = omcidrv_wrapper_activateGpon(*((int*)pArg));
        break;
    case OMCI_IOCTL_MACLIMIT_SET:
        ret = omcidrv_wrapper_setMacLearnLimit((OMCI_MACLIMIT_ts *)pArg);
        break;
    case OMCI_IOCTL_MACFILTER_SET:
        ret = omcidrv_wrapper_setMacFilter((OMCI_MACFILTER_ts *)pArg);
        break;
    case OMCI_IOCTL_DEV_CAPABILITIES_GET:
        ret = omcidrv_wrapper_getDevCapabilities((omci_dev_capability_t *)pArg);
        break;
    case OMCI_IOCTL_DEV_ID_VERSION_GET:
        ret = omcidrv_wrapper_getDevIdVersion((omci_dev_id_version_t *)pArg);
        break;
    case OMCI_IOCTL_DUAL_MGMT_MODE_SET:
        ret = omcidrv_wrapper_setDualMgmtMode(*((int *)pArg));
        break;
    case OMCI_IOCTL_US_DBRU_STATUS_GET:
        ret = omcidrv_wrapper_getUsDBRuStatus((unsigned int *)pArg);
        break;
    case OMCI_IOCTL_TRANSCEIVER_STATUS_GET:
        ret = omcidrv_wrapper_getTransceiverStatus((omci_transceiver_status_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_LINK_STATUS_GET:
        ret = omcidrv_wrapper_getPortLinkStatus((omci_port_link_status_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_SPEED_DUPLEX_STATUS_GET:
        ret = omcidrv_wrapper_getPortSpeedDuplexStatus((omci_port_speed_duplex_status_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_AUTO_NEGO_ABILITY_SET:
        ret = omcidrv_wrapper_setPortAutoNegoAbility((omci_port_auto_nego_ability_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_AUTO_NEGO_ABILITY_GET:
        ret = omcidrv_wrapper_getPortAutoNegoAbility((omci_port_auto_nego_ability_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_STATE_SET:
        ret = omcidrv_wrapper_setPortState((omci_port_state_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_STATE_GET:
        ret = omcidrv_wrapper_getPortState((omci_port_state_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_MAX_FRAME_SIZE_SET:
        ret = omcidrv_wrapper_setPortMaxFrameSize((omci_port_max_frame_size_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_MAX_FRAME_SIZE_GET:
        ret = omcidrv_wrapper_getPortMaxFrameSize((omci_port_max_frame_size_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PHY_LOOPBACK_SET:
        ret = omcidrv_wrapper_setPortPhyLoopback((omci_port_loopback_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PHY_LOOPBACK_GET:
        ret = omcidrv_wrapper_getPortPhyLoopback((omci_port_loopback_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PHY_PWR_DOWN_SET:
        ret = omcidrv_wrapper_setPortPhyPwrDown((omci_port_pwr_down_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PHY_PWR_DOWN_GET:
        ret = omcidrv_wrapper_getPortPhyPwrDown((omci_port_pwr_down_t *)pArg);
        break;
    case OMCI_IOCTL_CNTR_PORT_GET:
        ret = omcidrv_wrapper_getPortStat((omci_port_stat_t *)pArg);
        break;
    case OMCI_IOCTL_CNTR_PORT_CLEAR:
        ret = omcidrv_wrapper_resetPortStat(*((unsigned int *)pArg));
        break;
    case OMCI_IOCTL_CNTR_US_FLOW_GET:
        ret = omcidrv_wrapper_getUsFlowStat((omci_flow_stat_t *)pArg);
        break;
    case OMCI_IOCTL_CNTR_US_FLOW_CLEAR:
        ret = omcidrv_wrapper_resetUsFlowStat(*((unsigned int *)pArg));
        break;
    case OMCI_IOCTL_CNTR_DS_FLOW_GET:
        ret = omcidrv_wrapper_getDsFlowStat((omci_flow_stat_t *)pArg);
        break;
    case OMCI_IOCTL_CNTR_DS_FLOW_CLEAR:
        ret = omcidrv_wrapper_resetDsFlowStat(*((unsigned int *)pArg));
        break;
    case OMCI_IOCTL_CNTR_DS_FEC_GET:
        ret = omcidrv_wrapper_getDsFecStat((omci_ds_fec_stat_t *)pArg);
        break;
    case OMCI_IOCTL_CNTR_DS_FEC_CLEAR:
        ret = omcidrv_wrapper_resetDsFecStat();
        break;
    case OMCI_IOCTL_SVLAN_TPID_SET:
        ret = omcidrv_wrapper_setSvlanTpid((omci_svlan_tpid_t *)pArg);
        break;
    case OMCI_IOCTL_SVLAN_TPID_GET:
        ret = omcidrv_wrapper_getSvlanTpid((omci_svlan_tpid_t *)pArg);
        break;
    case OMCI_IOCTL_CVLAN_STATE_GET:
        ret = omcidrv_wrapper_getCvlanState((unsigned int *)pArg);
        break;
    case OMCI_IOCTL_GEMBLKLEN_GET:
        ret = omcidrv_wrapper_getGemBlkLen((unsigned short *)pArg);
        break;
    case OMCI_IOCTL_GEMBLKLEN_SET:
        ret = omcidrv_wrapper_setGemBlkLen(*((unsigned short *)pArg));
        break;
    case OMCI_IOCTL_GROUPMACFILTER_SET:
        ret = omcidrv_wrapper_setGroupMacFilter((OMCI_GROUPMACFILTER_ts *)pArg);
        break;
    case OMCI_IOCTL_DRV_VERSION_GET:
        ret = omcidrv_wrapper_getDrvVersion((char*)pArg);
        break;
    case OMCI_IOCTL_ONUSTATE_GET:
        ret = omcidrv_wrapper_getOnuState((PON_ONU_STATE *)pArg);
        break;
    case OMCI_IOCTL_SIGPARAMETER_SET:
        ret = omcidrv_wrapper_setSignalParameter((OMCI_SIGNAL_PARA_ts *)pArg);
        break;
    case OMCI_IOCTL_PON_BWTHRESHOLD_SET:
        ret = omcidrv_wrapper_setPonBwThreshold((omci_pon_bw_threshold_t *)pArg);
        break;
    case OMCI_IOCTL_VEIP_GEM_FLOW_SET:
        ret = omcidrv_wrapper_setUsVeipGemFlow((veipGemFlow_t *)pArg);
        break;
    case OMCI_IOCTL_VEIP_GEM_FLOW_DEL:
        ret = omcidrv_wrapper_delUsVeipGemFlow((veipGemFlow_t *)pArg);
        break;
    case OMCI_IOCTL_UNI_PORT_RATE:
        ret = omcidrv_wrapper_setUniPortRate((omci_port_rate_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PAUSE_CONTROL_SET:
        ret = omcidrv_wrapper_setPauseControl((omci_port_pause_ctrl_t *)pArg);
        break;
    case OMCI_IOCTL_PORT_PAUSE_CONTROL_GET:
        ret = omcidrv_wrapper_getPauseControl((omci_port_pause_ctrl_t *)pArg);
        break;
    case OMCI_IOCTL_DS_BC_GEM_FLOW_SET:
        ret = omcidrv_wrapper_setDsBcGemFlow((unsigned int *)pArg);
        break;
    case OMCI_IOCTL_DOT1_RATE_LIMITER_SET:
        ret = omcidrv_wrapper_setDot1RateLimiter((omci_dot1_rate_meter_t *)pArg);
        break;
    case OMCI_IOCTL_DOT1_RATE_LIMITER_DEL:
        ret = omcidrv_wrapper_delDot1RateLimiter((omci_dot1_rate_meter_t *)pArg);
        break;
    case OMCI_IOCTL_BG_TBL_PER_PORT_GET:
        ret = omcidrv_wrapper_getBgTblPerPort((omci_bridge_tbl_per_port_t *)pArg);
        break;
    case OMCI_IOCTL_MAC_AGEING_TIME_SET:
        ret = omcidrv_wrapper_setMacAgeingTime(*((unsigned int *)pArg));
        break;
    case OMCI_IOCTL_LOID_AUTH_STATUS_SET:
        ret = omcidrv_wrapper_setLoidAuthStatus((omci_event_msg_t *)pArg);
        break;
    case OMCI_IOCTL_SEND_OMCI_EVENT:
        ret = omcidrv_wrapper_sendOmciEvent((omci_event_msg_t *)pArg);
        break;
    case OMCI_IOCTL_FORCE_EMERGENCY_STOP_SET:
        ret = omcidrv_wrapper_setForceEmergencyStop((unsigned int *)pArg);
        break;
    case OMCI_IOCTL_PORT_BRIDGING_SET:
         ret = omcidrv_wrapper_setPortBridging(*((unsigned int *)pArg));
        break;
    case OMCI_IOCTL_FLOOD_PORT_MASK_SET:
        ret = omcidrv_wrapper_setFloodingPortMask((omci_flood_port_info *)pArg);
        break;
    case OMCI_IOCTL_WAN_QUEUE_NUM_SET:
        ret = omcidrv_wrapper_setWanQueueNum(*((int*)pArg));
        break;
    case OMCI_IOCTL_TOD_INFO_SET:
        ret = omcidrv_wrapper_setTodInfo((omci_tod_info_t*)pArg);
        break;
    case OMCI_IOCTL_UNI_QOS_SET:
        ret = omcidrv_wrapper_setUniQos((omci_uni_qos_info_t*)pArg);
        break;
    case OMCI_IOCTL_PORT_REMAP_SET:
        ret = omcidrv_wrapper_setPortRemap((int *)pArg);
        break;
    default:
        break;
    }
    return ret;
}


/*
 ef Service Data Path maintain
*/
int efServ_entry_add(
    omci_ext_filter_entry_t     *pRule,
    int                         rule_id)
{
    ext_filter_service_t *p;

    p = (ext_filter_service_t*)kmalloc(sizeof(ext_filter_service_t),GFP_KERNEL);
    p->rule_id = rule_id;
    p->ref_cnt = 1;

    memcpy(&p->rule, pRule, sizeof(omci_ext_filter_entry_t));

    list_add_tail(&p->list,&pPlatformDb->efHead);
    return OMCI_ERR_OK;
}

ext_filter_service_t* efServ_entry_find(omci_ext_filter_entry_t *p)
{
    ext_filter_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next,tmp,&pPlatformDb->efHead){

        cur = list_entry(next,ext_filter_service_t,list);

        if(0 == (memcmp(p, &(cur->rule), sizeof(omci_ext_filter_entry_t))))
        {
            return cur;
        }
    }
    return NULL;
}

ext_filter_service_t* efServ_entry_find_by_rule_index(int idx)
{
    ext_filter_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next,tmp,&pPlatformDb->efHead){

        cur = list_entry(next,ext_filter_service_t,list);

        if (cur->rule_id == idx)
        {
            return cur;
        }
    }
    return NULL;
}

int omcidrv_wrapper_clearPPPoEDb()
{
    int ret;

    if(pWrapper && pWrapper->pf_ClearPPPoEDb)
    {
        ret = pWrapper->pf_ClearPPPoEDb();
    }else
    {
        ret = OMCI_ERR_FAILED;
    }

    return ret;

}

/*
 L2 Service Data Path maintain
*/
int l2Serv_entry_add(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    l2_service_t *pL2Serv;
    int i;

    pL2Serv = (l2_service_t*)kmalloc(sizeof(l2_service_t),GFP_KERNEL);
    pL2Serv->pUsCfIndex = (unsigned int *)kmalloc(sizeof(unsigned int) * (pPlatformDb->allPortMax + 1), GFP_KERNEL);
    pL2Serv->pUsDpCfIndex = (unsigned int *)kmalloc(sizeof(unsigned int) * (pPlatformDb->allPortMax + 1), GFP_KERNEL);
    pL2Serv->index = pBridgeRule->servId;
    pL2Serv->uniMask = 0;
    for(i = pPlatformDb->allPortMin; i <= pPlatformDb->allPortMax; i++)
    {
        pL2Serv->pUsCfIndex[i] = OMCI_UNUSED_CF;
        pL2Serv->pUsDpCfIndex[i] = OMCI_UNUSED_CF;
    }
    pL2Serv->dsCfIndex = OMCI_UNUSED_CF;
    pL2Serv->usAclIndex = OMCI_UNUSED_ACL;
    pL2Serv->dsAclIndex = OMCI_UNUSED_ACL;

    pL2Serv->dir = pBridgeRule->dir;

    memcpy(&pL2Serv->rule, &(pBridgeRule->vlanRule), sizeof(OMCI_VLAN_OPER_ts));

    list_add_tail(&pL2Serv->list,&pPlatformDb->l2Head);
    return OMCI_ERR_OK;
}

int l2Serv_entry_del(unsigned int index)
{
    l2_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next,tmp,&pPlatformDb->l2Head){

        cur = list_entry(next,l2_service_t,list);

        if(index == cur->index)
        {
            kfree(cur->pUsCfIndex);
            kfree(cur->pUsDpCfIndex);
            list_del(&cur->list);
            kfree(cur);
            return OMCI_ERR_OK;
        }
    }
    return OMCI_ERR_OK;
}


int l2Serv_entry_delAll(void)
{
    l2_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next,tmp,&pPlatformDb->l2Head){

        cur = list_entry(next,l2_service_t,list);

        kfree(cur->pUsCfIndex);
        kfree(cur->pUsDpCfIndex);
        list_del(&cur->list);
        kfree(cur);
    }

    INIT_LIST_HEAD(&pPlatformDb->l2Head);
    return OMCI_ERR_OK;
}


l2_service_t*
l2Serv_entry_find(unsigned int index)
{
    l2_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next,tmp,&pPlatformDb->l2Head){

        cur = list_entry(next,l2_service_t,list);

        if(index == cur->index)
        {
            return cur;
        }
    }
    return NULL;
}


/*Find "all" cf index of pPlatformDb same as parameter index, and replace to newIndex*/
void
l2_mb_serv_entry_cfIdx_replace(unsigned int index, unsigned int newIndex)
{
    l2_service_t *cur = NULL;
    mbcast_service_t *cur_mb = NULL;
    struct list_head *next = NULL, *tmp=NULL;
    struct list_head *pEntry = NULL, *pTmpEntry = NULL;
    dsAggregated_group_t    *pGroupEntry= NULL;
    dsAggregated_entry_t    *pEntryData = NULL;
    int i;

    list_for_each_safe(next,tmp,&pPlatformDb->l2Head){

        cur = list_entry(next,l2_service_t,list);

        for(i = pPlatformDb->allPortMin; i <= pPlatformDb->allPortMax; i++)
        {
            if(index == cur->pUsCfIndex[i])
            {
                cur->pUsCfIndex[i] = newIndex;
            }
            else if(index == cur->pUsDpCfIndex[i])
            {
                cur->pUsDpCfIndex[i] = newIndex;
            }
        }
        if(index == cur->dsCfIndex)
        {
            cur->dsCfIndex = newIndex;
        }
    }

    list_for_each_safe(next, tmp, &pPlatformDb->mbcastHead)
    {
        cur_mb = list_entry(next, mbcast_service_t, list);

        if(index == cur_mb->dsCfIndex)
        {
            cur_mb->dsCfIndex = newIndex;
        }
    }

    list_for_each_safe(next, tmp, &(pPlatformDb->ucAggregatedGroupHead))
    {
        pGroupEntry = list_entry(next, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            if (index == pEntryData->cfIdx)
            {
                pEntryData->cfIdx = newIndex;
            }

            for(i = pPlatformDb->allPortMin; i <= pPlatformDb->allPortMax; i++)
            {
                if (index == pEntryData->pLutCfIdx[i])
                {
                    pEntryData->pLutCfIdx[i] = newIndex;
                }
            }
        }
    }

    list_for_each_safe(next, tmp, &(pPlatformDb->mbAggregatedGroupHead))
    {
        pGroupEntry = list_entry(next, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            if (index == pEntryData->cfIdx)
            {
                pEntryData->cfIdx = newIndex;
            }
        }
    }

    return;
}

/* mbcast maintain */
int mbcastServ_entry_add(unsigned int index,
    OMCI_VLAN_OPER_ts *pRule, unsigned int dsStreamId)
{
    mbcast_service_t *pMBServ;

    pMBServ = (mbcast_service_t*)kzalloc(sizeof(mbcast_service_t), GFP_KERNEL);
    if (!pMBServ)
        return OMCI_ERR_FAILED;

    pMBServ->index = index;
    memcpy(&pMBServ->rule, pRule, sizeof(OMCI_VLAN_OPER_ts));
    pMBServ->dsStreamId = dsStreamId;
    pMBServ->dsCfIndex = OMCI_UNUSED_CF;
    pMBServ->referAclIdx = OMCI_UNUSED_ACL;
    pMBServ->dsAclIndex = OMCI_UNUSED_ACL;
    //TBD uniMask
    pMBServ->uniMask = 0;

    list_add_tail(&pMBServ->list, &pPlatformDb->mbcastHead);

    return OMCI_ERR_OK;
}

int mbcastServ_entry_del(unsigned int index)
{
    mbcast_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next, tmp, &pPlatformDb->mbcastHead){

        cur = list_entry(next, mbcast_service_t, list);

        if(index == cur->index)
        {
            list_del(&cur->list);
            kfree(cur);
            return OMCI_ERR_OK;
        }
    }
    return OMCI_ERR_OK;
}

int mbcastServ_entry_delAll(void)
{
    mbcast_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next, tmp, &pPlatformDb->mbcastHead){

        cur = list_entry(next, mbcast_service_t, list);

        list_del(&cur->list);
        kfree(cur);
    }

    INIT_LIST_HEAD(&pPlatformDb->mbcastHead);
    return OMCI_ERR_OK;
}

mbcast_service_t*
mbcastServ_entry_find(unsigned int index)
{
    mbcast_service_t *cur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next, tmp, &pPlatformDb->mbcastHead){

        cur = list_entry(next, mbcast_service_t, list);

        if(index == cur->index)
        {
            return cur;
        }
    }
    return NULL;
}

/*
veip maintain
*/
veip_service_t* veipServ_entry_find(unsigned int index)
{
    struct list_head    *pEntry;
    struct list_head    *pTmpEntry;
    veip_service_t      *pData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        if (pData->index == index)
            return pData;
    }

    return NULL;
}

int veipServ_entry_add(unsigned int index,
        OMCI_VLAN_OPER_ts *pRule, unsigned int usStreamId, unsigned int dsStreamId)
{
    veip_service_t  *pEntry;
    unsigned int    i;

    if (0 == pPlatformDb->intfNum)
        return OMCI_ERR_FAILED;

    pEntry = kzalloc(sizeof(veip_service_t), GFP_KERNEL);
    if (!pEntry)
        return OMCI_ERR_FAILED;

    pEntry->pHwPathCfIdx = kmalloc(sizeof(unsigned int) * pPlatformDb->intfNum, GFP_KERNEL);
    if (!pEntry->pHwPathCfIdx)
    {
        kfree(pEntry);
        pEntry = NULL;
        return OMCI_ERR_FAILED;
    }

    pEntry->pSwPathCfIdx = kmalloc(sizeof(unsigned int) * pPlatformDb->intfNum, GFP_KERNEL);
    if (!pEntry->pSwPathCfIdx)
    {
        kfree(pEntry->pHwPathCfIdx);
        pEntry->pHwPathCfIdx = NULL;
        kfree(pEntry);
        pEntry = NULL;

        return OMCI_ERR_FAILED;
    }

    pEntry->pUsSwPathCfIdx = kmalloc(sizeof(unsigned int) * pPlatformDb->intfNum, GFP_KERNEL);
    if (!pEntry->pUsSwPathCfIdx)
    {
        kfree(pEntry->pHwPathCfIdx);
        pEntry->pHwPathCfIdx = NULL;
        kfree(pEntry->pSwPathCfIdx);
        pEntry->pSwPathCfIdx = NULL;
        kfree(pEntry);
        pEntry = NULL;

        return OMCI_ERR_FAILED;
    }

    pEntry->index = index;
    memcpy(&pEntry->rule, pRule, sizeof(OMCI_VLAN_OPER_ts));
    pEntry->usStreamId = usStreamId;
    pEntry->dsStreamId = dsStreamId;
    for (i = 0; i < pPlatformDb->intfNum; i++)
    {
        pEntry->pHwPathCfIdx[i] = OMCI_UNUSED_CF;
        pEntry->pSwPathCfIdx[i] = OMCI_UNUSED_CF;
        pEntry->pUsSwPathCfIdx[i] = OMCI_UNUSED_CF;
    }
    pEntry->defaultPathCfIdx = OMCI_UNUSED_CF;

    list_add_tail(&pEntry->list,&pPlatformDb->veipHead);

    return OMCI_ERR_OK;
}

int veipServ_entry_del(unsigned int index)
{
    struct list_head    *pEntry;
    struct list_head    *pTmpEntry;
    veip_service_t      *pData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        if (pData->index == index)
    {
            list_del(&pData->list);

            kfree(pData->pHwPathCfIdx);
            pData->pHwPathCfIdx = NULL;
            kfree(pData->pSwPathCfIdx);
            pData->pSwPathCfIdx = NULL;
            kfree(pData->pUsSwPathCfIdx);
            pData->pUsSwPathCfIdx = NULL;
            kfree(pData);
            pData = NULL;

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}

int veipServ_entry_delAll(void)
{
    struct list_head    *pEntry;
    struct list_head    *pTmpEntry;
    veip_service_t      *pData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        list_del(&pData->list);

        kfree(pData->pHwPathCfIdx);
        pData->pHwPathCfIdx = NULL;
        kfree(pData->pSwPathCfIdx);
        pData->pSwPathCfIdx = NULL;
        kfree(pData->pUsSwPathCfIdx);
        pData->pUsSwPathCfIdx = NULL;
        kfree(pData);
        pData = NULL;
    }

    INIT_LIST_HEAD(&pPlatformDb->veipHead);

    return OMCI_ERR_OK;
}

veipGemFlow_entry_t* veipGemFlow_entry_find(unsigned int gemPortId)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veipGemFlow_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipGemFlowHead)
    {
        pEntryData = list_entry(pEntry, veipGemFlow_entry_t, list);

        if (pEntryData->data.gemPortId == gemPortId)
            return pEntryData;
    }

    return NULL;
}

int veipGemFlow_entry_add(veipGemFlow_t *pData)
{
    veipGemFlow_entry_t     *pEntryData;

    if (!pData)
        return OMCI_ERR_FAILED;

    pEntryData = kzalloc(sizeof(veipGemFlow_entry_t), GFP_KERNEL);
    if (!pEntryData)
        return OMCI_ERR_FAILED;

    memcpy(&pEntryData->data, pData, sizeof(veipGemFlow_t));

    list_add_tail(&pEntryData->list, &pPlatformDb->veipGemFlowHead);

    return OMCI_ERR_OK;
}

int veipGemFlow_entry_del(unsigned int gemPortId)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veipGemFlow_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipGemFlowHead)
    {
        pEntryData = list_entry(pEntry, veipGemFlow_entry_t, list);

        if (pEntryData->data.gemPortId == gemPortId)
        {
            list_del(&pEntryData->list);

            kfree(pEntryData);

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}

int veipGemFlow_entry_delAll(void)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veipGemFlow_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->veipGemFlowHead)
    {
        pEntryData = list_entry(pEntry, veipGemFlow_entry_t, list);

        list_del(&pEntryData->list);

        kfree(pEntryData);
    }

    INIT_LIST_HEAD(&pPlatformDb->veipGemFlowHead);

    return OMCI_ERR_OK;
}


macFilter_entry_t *macFilter_entry_find(unsigned long long key)
{
    macFilter_entry_t *pCur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next, tmp, &pPlatformDb->macFilterHead){

        pCur = list_entry(next, macFilter_entry_t, list);

        if(key == pCur->key)
        {
            return pCur;
        }
    }
    return NULL;
}

int macFilter_entry_add(unsigned long long key)
{
    macFilter_entry_t *pNew = NULL;

    pNew = (macFilter_entry_t *)kmalloc(sizeof(macFilter_entry_t), GFP_KERNEL);
    if(!pNew) return OMCI_ERR_FAILED;
    pNew->key = key;
    pNew->hwAclIdx[0] = OMCI_UNUSED_CF;
    pNew->hwAclIdx[1] = OMCI_UNUSED_CF;
    list_add_tail(&pNew->list, &pPlatformDb->macFilterHead);
    return OMCI_ERR_OK;
}

int macFilter_entry_del(unsigned long long key)
{
    macFilter_entry_t *pCur = NULL;
    struct list_head *next = NULL, *tmp=NULL;

    list_for_each_safe(next, tmp, &pPlatformDb->macFilterHead){

        pCur = list_entry(next, macFilter_entry_t, list);

        if(key == pCur->key)
        {
            list_del(&pCur->list);
            kfree(pCur);
            return OMCI_ERR_OK;
        }
    }
    return OMCI_ERR_OK;
}

int flow2DsPq_entry_add(unsigned int key, unsigned int gemPort, OMCI_DS_PQ_INFO *p)
{
    flow2DsPq_entry_t   *pEntry = NULL;

    if (!p)
        return OMCI_ERR_FAILED;

    pEntry = kzalloc(sizeof(flow2DsPq_entry_t), GFP_KERNEL);
    if (!pEntry)
        return OMCI_ERR_FAILED;

    pEntry->flowId      = key;
    pEntry->gemPortId   = gemPort;
    memcpy(&pEntry->dsQ, p, sizeof(OMCI_DS_PQ_INFO));

    list_add_tail(&pEntry->list, &pPlatformDb->flow2DsPqHead);

    return OMCI_ERR_OK;
}

int flow2DsPq_entry_del(unsigned int key)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    flow2DsPq_entry_t       *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        if (pEntryData->flowId == key)
        {
            list_del(&pEntryData->list);

            kfree(pEntryData);

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}

flow2DsPq_entry_t* flow2DsPq_entry_find(unsigned int key)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    flow2DsPq_entry_t       *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        if (pEntryData->flowId == key)
            return pEntryData;
    }

    return NULL;
}

int flow2DsPq_entry_delAll(void)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    flow2DsPq_entry_t       *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        list_del(&pEntryData->list);

        kfree(pEntryData);
    }

    INIT_LIST_HEAD(&pPlatformDb->flow2DsPqHead);

    return OMCI_ERR_OK;
}

int dpStagAcl_entry_add(unsigned int usFlowId, unsigned int dpMarking,
                        unsigned int uniMask, unsigned int *pAclIdBitmap)
{
    dp_stag_acl_entry_t   *pEntry = NULL;

    if (!(pEntry = kzalloc(sizeof(dp_stag_acl_entry_t), GFP_KERNEL)))
        return OMCI_ERR_FAILED;

    if (!(pEntry->pAclIdBitmap = (unsigned int *)kzalloc(sizeof(unsigned int) * (pPlatformDb->aclNum / (sizeof(unsigned int) * 8)), GFP_KERNEL)))
    {
        kfree(pEntry);
        return OMCI_ERR_FAILED;
    }
    pEntry->usFlowId = usFlowId;

    pEntry->uniMask = uniMask;

    pEntry->dpMarking = dpMarking;

    memcpy(pEntry->pAclIdBitmap, pAclIdBitmap, ((pPlatformDb->aclNum / (sizeof(unsigned int) * 8)) * 4));

    list_add_tail(&pEntry->list, &pPlatformDb->dpStagAclHead);

    return OMCI_ERR_OK;
}

int dpStagAcl_entry_del(unsigned int usFlowId)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    dp_stag_acl_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->dpStagAclHead)
    {
        pEntryData = list_entry(pEntry, dp_stag_acl_entry_t, list);

        if (usFlowId == pEntryData->usFlowId)
        {
            kfree(pEntryData->pAclIdBitmap);
            list_del(&pEntryData->list);
            kfree(pEntryData);
            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}

dp_stag_acl_entry_t* dpStagAcl_entry_find(unsigned int usFlowId)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    dp_stag_acl_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->dpStagAclHead)
    {
        pEntryData = list_entry(pEntry, dp_stag_acl_entry_t, list);

        if (usFlowId == pEntryData->usFlowId)
            return pEntryData;
    }

    return NULL;
}

int dpStagAcl_entry_delAll(void)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    dp_stag_acl_entry_t     *pEntryData;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->dpStagAclHead)
    {
        pEntryData = list_entry(pEntry, dp_stag_acl_entry_t, list);

        kfree(pEntryData->pAclIdBitmap);
        list_del(&pEntryData->list);
        kfree(pEntryData);
    }

    INIT_LIST_HEAD(&pPlatformDb->dpStagAclHead);

    return OMCI_ERR_OK;
}

int uni_qos_entry_add(omci_uni_qos_info_t *p)
{
    uniQos_entry_t *pEntry = NULL;

    if (!p)
        return OMCI_ERR_FAILED;

    pEntry = kzalloc(sizeof(uniQos_entry_t), GFP_KERNEL);
    if (!pEntry)
        return OMCI_ERR_FAILED;

    memcpy(&(pEntry->uniQos), p, sizeof(omci_uni_qos_info_t));

    list_add_tail(&pEntry->list, &pPlatformDb->uniQosHead);

    return OMCI_ERR_OK;
}

int uni_qos_entry_del(unsigned int port)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    uniQos_entry_t          *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->uniQosHead)
    {
        pEntryData = list_entry(pEntry, uniQos_entry_t, list);

        if (pEntryData && pEntryData->uniQos.port == port)
        {
            list_del(&pEntryData->list);

            kfree(pEntryData);

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}

uniQos_entry_t* uni_qos_entry_find(unsigned int port)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    uniQos_entry_t          *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->uniQosHead)
    {
        pEntryData = list_entry(pEntry, uniQos_entry_t, list);

        if (pEntryData && pEntryData->uniQos.port == port)
            return pEntryData;
    }

    return NULL;
}

int uni_qos_entry_delAll(void)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    uniQos_entry_t          *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &pPlatformDb->uniQosHead)
    {
        pEntryData = list_entry(pEntry, uniQos_entry_t, list);

        if (pEntryData)
        {
            list_del(&pEntryData->list);
            kfree(pEntryData);
        }
    }

    INIT_LIST_HEAD(&pPlatformDb->uniQosHead);

    return OMCI_ERR_OK;
}

static int parse_buf(char *pBuf, omci_dev_capability_t *p)
{
    char *strptr = NULL, *tokptr = NULL;
    unsigned char fmt_index = 0;

    if (!pBuf || !p)
        return OMCI_ERR_FAILED;

    strptr = pBuf;

    tokptr = strsep(&strptr, RES_FILE_FMT_DELIM);

    while (tokptr)
    {
        switch (fmt_index)
        {
            case RES_FILE_FMT_FE:
                p->fePortNum = simple_strtol(tokptr, NULL, 0);
                break;
            case RES_FILE_FMT_GE:
                p->gePortNum = simple_strtol(tokptr, NULL, 0);
                break;
            case RES_FILE_FMT_VOIP:
                p->potsPortNum = simple_strtol(tokptr, NULL, 0);
                break;
            default:
                // TBD other service
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");
        }
        tokptr = strsep(&strptr, RES_FILE_FMT_DELIM);
        fmt_index++;
    }

    return OMCI_ERR_OK;
}

static int
proc_uni_capability_parse (
    char* pBuf,
    omci_dev_capability_t* p
)
{
    UINT32 idx = 0, idx1 = 0, uni = 0, start = 0, end = 0;
    char* strPtr = NULL, *tokPtr = NULL;
    mm_segment_t    oldfs;
    struct file* fp = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    memset(buf, 0, READ_BUF_LEN);
    memset(tmp, 0, READ_BUF_LEN);

    fp = filp_open(ETH_RES_INFO_FILE, O_RDONLY, 0);

    if (IS_ERR(fp))
    {
        printk ("Failed to open "ETH_RES_INFO_FILE"\n");
        goto FINISH;
    }

    if (!(fp->f_op))
    {
        goto CLOSE_FP;
    }

    if (!(fp->f_op->read))
    {
        goto CLOSE_FP;
    }

    if (fp->f_op->read(fp, buf, READ_BUF_LEN, &fp->f_pos) <= 0)
    {
        printk ("Failed to read "ETH_RES_INFO_FILE"\n");
        goto CLOSE_FP;
    }

    for (idx = 0; idx < READ_BUF_LEN; idx++) {
        //
        // Igonre the comment lines
        //
        if (buf[idx] == '#') {
            for (; idx < READ_BUF_LEN; idx++) {
                if (buf[idx] == '\n') {
                    break;
                }
            }
            continue;
        }
        //
        // Read one line from proc.
        //
        start = idx;
        end = 0;
        for (; idx < READ_BUF_LEN; idx++)
        {
          if (buf[idx] == '\n') {
            end = idx;
            break;
          }
        }

        if (end <= start)
        {
            continue;
        }

        memcpy(tmp, buf + start, sizeof(char) * (end - start));
        tmp[end - start] = '\n';
        if (idx1 == 0)
        {
            //
            // Get the portType (FE/GE/None)
            //
            strPtr = tmp;
            tokPtr = strsep(&strPtr, RES_FILE_FMT_DELIM);
            uni = 0;
            while (tokPtr) {
              signed procPType = simple_strtol(tokPtr, NULL, 10);

              if (procPType < p->ethPort[uni].portType) {
                  p->ethPort[uni].portType = procPType;
              }
              tokPtr = strsep(&strPtr, RES_FILE_FMT_DELIM);
              uni++;
            }
        }
        idx1 ++;
        memset (tmp, 0x00, sizeof(char) * (end - start));
    }


CLOSE_FP:
    filp_close(fp, NULL);

FINISH:
    set_fs(oldfs);


    return OMCI_ERR_OK;
}


int get_resouce_by_dev_feature(dev_feature_type_t type, omci_dev_capability_t *p)
{
    mm_segment_t    oldfs;
    struct file     *fp = NULL;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    memset(buf, 0, READ_BUF_LEN);

    switch (type)
    {
        case DEV_FEATURE_VOIP:
            fp = filp_open(VOIP_RES_INFO_FILE, O_RDONLY, 0);

            if (IS_ERR(fp))
                goto finish;

            if (!(fp->f_op))
                goto close_fp;

            if (!(fp->f_op->read))
                goto close_fp;

            if (fp->f_op->read(fp, buf, READ_BUF_LEN, &fp->f_pos) <= 0)
                goto close_fp;

            p->potsPortNum = simple_strtol(buf, NULL, 0);
            break;
        case DEV_FEATURE_ETH:
            proc_uni_capability_parse(buf, p);
            goto finish;

        case DEV_FEATURE_ALL:
            fp = filp_open(ALL_RES_INFO_FILE, O_RDONLY, 0);

            if (IS_ERR(fp))
                goto finish;

            if (!(fp->f_op))
                goto close_fp;

            if (!(fp->f_op->read))
                goto close_fp;

            if (fp->f_op->read(fp, buf, READ_BUF_LEN, &fp->f_pos) <= 0)
                goto close_fp;

            parse_buf(buf, p);

            break;

        default:
            //TBD
            return OMCI_ERR_OK;
    }

close_fp:
    filp_close(fp, NULL);

finish:
    set_fs(oldfs);

    return OMCI_ERR_OK;
}

#define MAX_LIST_LENGTH_BITS 20

static struct list_head *_omci_list_merge(void *priv,
                int (*cmp)(void *priv, struct list_head *a,
                    struct list_head *b),
                struct list_head *a, struct list_head *b)
{
    struct list_head head, *tail = &head;

    while (a && b) {
        /* if equal, take 'a' -- important for sort stability */
        if ((*cmp)(priv, a, b) <= 0) {
            tail->next = a;
            a = a->next;
        } else {
            tail->next = b;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = a?:b;
    return head.next;
}

/*
 * Combine final list merge with restoration of standard doubly-linked
 * list structure.  This approach duplicates code from _omci_list_merge(), but
 * runs faster than the tidier alternatives of either a separate final
 * prev-link restoration pass, or maintaining the prev links
 * throughout.
 */
static void _omci_list_merge_and_restore_back_links(void *priv,
                int (*cmp)(void *priv, struct list_head *a,
                    struct list_head *b),
                struct list_head *head,
                struct list_head *a, struct list_head *b)
{
    struct list_head *tail = head;
    u8 count = 0;

    while (a && b) {
        /* if equal, take 'a' -- important for sort stability */
        if ((*cmp)(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            a = a->next;
        } else {
            tail->next = b;
            b->prev = tail;
            b = b->next;
        }
        tail = tail->next;
    }
    tail->next = a ? : b;

    do {
        /*
         * In worst cases this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        if (unlikely(!(++count)))
            (*cmp)(priv, tail->next, tail->next);

        tail->next->prev = tail;
        tail = tail->next;
    } while (tail->next);

    tail->next = head;
    head->prev = tail;
}

/**
 * _omci_list_sort - sort a list
 * @priv: private data, opaque to _omci_list_sort(), passed to @cmp
 * @head: the list to sort
 * @cmp: the elements comparison function
 *
 * This function implements "merge sort", which has O(nlog(n))
 * complexity.
 *
 * The comparison function @cmp must return a negative value if @a
 * should sort before @b, and a positive value if @a should sort after
 * @b. If @a and @b are equivalent, and their original relative
 * ordering is to be preserved, @cmp must return 0.
 */
void _omci_list_sort(void *priv, struct list_head *head,
        int (*cmp)(void *priv, struct list_head *a,
            struct list_head *b))
{
    struct list_head *part[MAX_LIST_LENGTH_BITS+1]; /* sorted partial lists
                        -- last slot is a sentinel */
    int lev;  /* index into part[] */
    int max_lev = 0;
    struct list_head *list;

    if (list_empty(head))
        return;

    memset(part, 0, sizeof(part));

    head->prev->next = NULL;
    list = head->next;

    while (list) {
        struct list_head *cur = list;
        list = list->next;
        cur->next = NULL;

        for (lev = 0; part[lev]; lev++) {
            cur = _omci_list_merge(priv, cmp, part[lev], cur);
            part[lev] = NULL;
        }
        if (lev > max_lev) {
            if (unlikely(lev >= ARRAY_SIZE(part)-1)) {
                printk(KERN_DEBUG "list too long for efficiency\n");
                lev--;
            }
            max_lev = lev;
        }
        part[lev] = cur;
    }

    for (lev = 0; lev < max_lev; lev++)
        if (part[lev])
            list = _omci_list_merge(priv, cmp, part[lev], list);

    _omci_list_merge_and_restore_back_links(priv, cmp, head, part[max_lev], list);
}

EXPORT_SYMBOL(omcidrv_wrapper_createTcont);
EXPORT_SYMBOL(omcidrv_wrapper_updateTcont);
EXPORT_SYMBOL(omcidrv_wrapper_cfgGemFlow);
EXPORT_SYMBOL(omcidrv_wrapper_setPriQueue);
EXPORT_SYMBOL(omcidrv_wrapper_deactiveBdgConn);
EXPORT_SYMBOL(omcidrv_wrapper_activeBdgConn);
EXPORT_SYMBOL(omcidrv_wrapper_setSerialNum);
EXPORT_SYMBOL(omcidrv_wrapper_getSerialNum);
EXPORT_SYMBOL(omcidrv_wrapper_setGponPasswd);
EXPORT_SYMBOL(omcidrv_wrapper_activateGpon);
EXPORT_SYMBOL(omcidrv_wrapper_resetMib);
EXPORT_SYMBOL(omcidrv_wrapper_clearPriQueue);
EXPORT_SYMBOL(omcidrv_wrapper_dumpL2Serv);
EXPORT_SYMBOL(omcidrv_wrapper_dumpVeipServ);
EXPORT_SYMBOL(omcidrv_wrapper_dumpMBServ);
EXPORT_SYMBOL(omcidrv_wrapper_dumpCfMap);
EXPORT_SYMBOL(omcidrv_wrapper_setCfMap);
EXPORT_SYMBOL(omcidrv_wrapper_setLog);
EXPORT_SYMBOL(omcidrv_wrapper_setDevMode);
EXPORT_SYMBOL(omcidrv_wrapper_setDscpRemap);
EXPORT_SYMBOL(omcidrv_wrapper_updateVeipRule);
EXPORT_SYMBOL(omcidrv_wrapper_setMacLearnLimit);
EXPORT_SYMBOL(omcidrv_wrapper_setMacFilter);
EXPORT_SYMBOL(omcidrv_wrapper_dumpMacFilter);
EXPORT_SYMBOL(omcidrv_wrapper_getDevCapabilities);
EXPORT_SYMBOL(omcidrv_wrapper_getDevIdVersion);
EXPORT_SYMBOL(omcidrv_wrapper_setDualMgmtMode);
EXPORT_SYMBOL(omcidrv_wrapper_getUsDBRuStatus);
EXPORT_SYMBOL(omcidrv_wrapper_getTransceiverStatus);
EXPORT_SYMBOL(omcidrv_wrapper_getPortLinkStatus);
EXPORT_SYMBOL(omcidrv_wrapper_getPortSpeedDuplexStatus);
EXPORT_SYMBOL(omcidrv_wrapper_setPortAutoNegoAbility);
EXPORT_SYMBOL(omcidrv_wrapper_getPortAutoNegoAbility);
EXPORT_SYMBOL(omcidrv_wrapper_setPortState);
EXPORT_SYMBOL(omcidrv_wrapper_getPortState);
EXPORT_SYMBOL(omcidrv_wrapper_setPortMaxFrameSize);
EXPORT_SYMBOL(omcidrv_wrapper_getPortMaxFrameSize);
EXPORT_SYMBOL(omcidrv_wrapper_setPortPhyLoopback);
EXPORT_SYMBOL(omcidrv_wrapper_getPortPhyLoopback);
EXPORT_SYMBOL(omcidrv_wrapper_setPortPhyPwrDown);
EXPORT_SYMBOL(omcidrv_wrapper_getPortPhyPwrDown);
EXPORT_SYMBOL(omcidrv_wrapper_getPortStat);
EXPORT_SYMBOL(omcidrv_wrapper_resetPortStat);
EXPORT_SYMBOL(omcidrv_wrapper_getUsFlowStat);
EXPORT_SYMBOL(omcidrv_wrapper_resetUsFlowStat);
EXPORT_SYMBOL(omcidrv_wrapper_getDsFlowStat);
EXPORT_SYMBOL(omcidrv_wrapper_resetDsFlowStat);
EXPORT_SYMBOL(omcidrv_wrapper_getDsFecStat);
EXPORT_SYMBOL(omcidrv_wrapper_resetDsFecStat);
EXPORT_SYMBOL(omcidrv_wrapper_setSvlanTpid);
EXPORT_SYMBOL(omcidrv_wrapper_getSvlanTpid);
EXPORT_SYMBOL(omcidrv_wrapper_getGemBlkLen);
EXPORT_SYMBOL(omcidrv_wrapper_setGemBlkLen);
EXPORT_SYMBOL(omcidrv_wrapper_setGroupMacFilter);
EXPORT_SYMBOL(omcidrv_wrapper_dumpUsVeipGemFlow);
EXPORT_SYMBOL(omcidrv_wrapper_setUsVeipGemFlow);
EXPORT_SYMBOL(omcidrv_wrapper_delUsVeipGemFlow);
EXPORT_SYMBOL(omcidrv_wrapper_getDrvVersion);
EXPORT_SYMBOL(omcidrv_wrapper_getOnuState);
EXPORT_SYMBOL(omcidrv_wrapper_setSignalParameter);
EXPORT_SYMBOL(omcidrv_wrapper_setPonBwThreshold);
EXPORT_SYMBOL(omcidrv_wrapper_getCvlanState);
EXPORT_SYMBOL(omcidrv_wrapper_setUniPortRate);
EXPORT_SYMBOL(omcidrv_wrapper_setPauseControl);
EXPORT_SYMBOL(omcidrv_wrapper_getPauseControl);
EXPORT_SYMBOL(omcidrv_wrapper_setDsBcGemFlow);
EXPORT_SYMBOL(omcidrv_wrapper_setDot1RateLimiter);
EXPORT_SYMBOL(omcidrv_wrapper_delDot1RateLimiter);
EXPORT_SYMBOL(omcidrv_wrapper_getBgTblPerPort);
EXPORT_SYMBOL(omcidrv_wrapper_setMacAgeingTime);
EXPORT_SYMBOL(omcidrv_wrapper_setPortBridging);
EXPORT_SYMBOL(omcidrv_wrapper_setLoidAuthStatus);
EXPORT_SYMBOL(omcidrv_wrapper_sendOmciEvent);
EXPORT_SYMBOL(omcidrv_wrapper_setForceEmergencyStop);
EXPORT_SYMBOL(omcidrv_wrapper_dumpFlow2dsPq);
EXPORT_SYMBOL(omcidrv_wrapper_setFloodingPortMask);
EXPORT_SYMBOL(omcidrv_wrapper_setWanQueueNum);
EXPORT_SYMBOL(omcidrv_wrapper_setTodInfo);
EXPORT_SYMBOL(omcidrv_wrapper_clearPPPoEDb);

EXPORT_SYMBOL(omcidrv_ioctl);

EXPORT_SYMBOL(efServ_entry_add);
EXPORT_SYMBOL(efServ_entry_find);
EXPORT_SYMBOL(efServ_entry_find_by_rule_index);
EXPORT_SYMBOL(l2Serv_entry_add);
EXPORT_SYMBOL(l2Serv_entry_del);
EXPORT_SYMBOL(l2Serv_entry_delAll);
EXPORT_SYMBOL(l2Serv_entry_find);
EXPORT_SYMBOL(l2_mb_serv_entry_cfIdx_replace);
EXPORT_SYMBOL(mbcastServ_entry_add);
EXPORT_SYMBOL(mbcastServ_entry_del);
EXPORT_SYMBOL(mbcastServ_entry_delAll);
EXPORT_SYMBOL(mbcastServ_entry_find);
EXPORT_SYMBOL(veipServ_entry_add);
EXPORT_SYMBOL(veipServ_entry_find);
EXPORT_SYMBOL(veipServ_entry_del);
EXPORT_SYMBOL(veipServ_entry_delAll);
EXPORT_SYMBOL(veipGemFlow_entry_find);
EXPORT_SYMBOL(veipGemFlow_entry_add);
EXPORT_SYMBOL(veipGemFlow_entry_del);
EXPORT_SYMBOL(veipGemFlow_entry_delAll);
EXPORT_SYMBOL(macFilter_entry_find);
EXPORT_SYMBOL(macFilter_entry_add);
EXPORT_SYMBOL(macFilter_entry_del);
EXPORT_SYMBOL(flow2DsPq_entry_add);
EXPORT_SYMBOL(flow2DsPq_entry_del);
EXPORT_SYMBOL(flow2DsPq_entry_find);
EXPORT_SYMBOL(flow2DsPq_entry_delAll);
EXPORT_SYMBOL(dpStagAcl_entry_add);
EXPORT_SYMBOL(dpStagAcl_entry_del);
EXPORT_SYMBOL(dpStagAcl_entry_find);
EXPORT_SYMBOL(dpStagAcl_entry_delAll);
EXPORT_SYMBOL(get_resouce_by_dev_feature);
EXPORT_SYMBOL(uni_qos_entry_add);
EXPORT_SYMBOL(uni_qos_entry_del);
EXPORT_SYMBOL(uni_qos_entry_delAll);
EXPORT_SYMBOL(uni_qos_entry_find);
EXPORT_SYMBOL(_omci_list_sort);
/*
 *  PROC configure
*/
static int veip_read_proc(struct seq_file *seq, void *v)
{
    printk("=======TR142 Rule for VEIP=======\n");

    omcidrv_wrapper_dumpVeipServ();

    return 0;
}


static ssize_t veip_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t * off)
{
    char flag;

    if (count < 2)
        return -EFAULT;


    if (buffer && !copy_from_user(&flag, buffer, 1))
    {
        return count;
    }
    else
        return -EFAULT;

}

static int vgf_read_proc(struct seq_file *seq, void *v)
{
    printk("============ us veip gem flow ============\n");

    omcidrv_wrapper_dumpUsVeipGemFlow();

    return 0;
}

static int f2dq_read_proc(struct seq_file *seq, void *v)
{
    omcidrv_wrapper_dumpFlow2dsPq();
    return 0;
}

static int dmMode_read_proc(struct seq_file *seq, void *v)
{
    printk("dmMode = %u\n", gDrvCtrl.dmMode);

    return 0;
}


static ssize_t dmMode_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t * off)
{
    char flag;

    if (count < 2)
        return -EFAULT;

    if (buffer && !copy_from_user(&flag, buffer, sizeof(flag)))
    {
        if (flag == '0')
            gDrvCtrl.dmMode = 0;
        else if (flag == '1')
            gDrvCtrl.dmMode = 1;
        else if (flag == '2')
            gDrvCtrl.dmMode = 2;
        else
            gDrvCtrl.dmMode = 3;

        printk("dmMode = %u\n", gDrvCtrl.dmMode);

        return count;
    }
    else
        return -EFAULT;
}

static int l2Serv_read_proc(struct seq_file *seq, void *v)
{
    printk("=======L2 Rule for UNI===========\n");

    omcidrv_wrapper_dumpL2Serv();

    return 0;
}


static ssize_t l2Serv_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t * off)
{
    char flag;

    if (count < 2)
        return -EFAULT;


    if (buffer && !copy_from_user(&flag, buffer, 1))
    {

        return count;
    }
    else
        return -EFAULT;

}


static int MBServ_read_proc(struct seq_file *seq, void *v)
{
    printk("=======Mcast / Bcast Rule for UNI===========\n");

    omcidrv_wrapper_dumpMBServ();

    return 0;
}


static ssize_t MBServ_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t * off)
{
    char flag;

    if (count < 2)
        return -EFAULT;


    if (buffer && !copy_from_user(&flag, buffer, 1))
    {

        return count;
    }
    else
        return -EFAULT;

}

static ssize_t omciMirror_write_proc (struct file* file, const char* buffer,
                size_t count, loff_t* off)
{
    unsigned int mirroring_port = 0xFFFFFFFF;
    unsigned char mirror_state = 0;
    char pattern[10] = "\0", tmpbuf[128] ="\0", pattern_val[128] ="\0";
    char *pStr = NULL, *pTok = NULL;
    if(!buffer || (0 != copy_from_user(tmpbuf, buffer, count)))
    {
        return -EFAULT;
    }
    pStr = tmpbuf;
    pTok = strsep(&pStr, " ");

    if (!pTok || strlen(pTok) > 10)
    {
        goto mirror_help;
    }
    strncpy(pattern, pTok, strlen(pTok));

    if (0 == (strncmp(pattern, "MIR_PORT", strlen("MIR_PORT"))))
    {
        pTok = strsep(&pStr, "\n");
        if (!pTok)
            goto mirror_help;

        printk("MIR_PORT is %s\n", pTok);
        strncpy(pattern_val, pTok, strlen(pTok));
        gDrvCtrl.omciMirroringPort = simple_strtol(pattern_val, NULL, 10);
    }
    else if (0 == (strncmp(pattern, "MIR_STAT", strlen("MIR_STAT"))))
    {
        pTok = strsep(&pStr, "\n");
        if (!pTok)
            goto mirror_help;

        printk("MIR_STAT is %s\n", pTok);
        strncpy(pattern_val, pTok, strlen(pTok));
        mirror_state = simple_strtol(pattern_val, NULL, 10);

        if (0 != mirror_state && 1 != mirror_state)
            goto mirror_help;

        gDrvCtrl.omciMirrorState = mirror_state;
    }
    else
    {
        goto mirror_help;
    }

    if (1 == gDrvCtrl.omciMirrorState)
        mirroring_port = gDrvCtrl.omciMirroringPort;
    else
        mirroring_port = 0xFFFFFFFF;

    if (pWrapper && pWrapper->pf_SetOmciMirror)
    {
        if (OMCI_ERR_FAILED == (pWrapper->pf_SetOmciMirror(&mirroring_port)))
        {
            printk("pf_SetOmciMirror failed \n");
        }
    }
    return count;
mirror_help:
    printk("echo \"MIR_PORT [PHY_PORT_ID]\" > /proc/omci/omciMirror\n");
    printk("echo \"MIR_STAT [DIS(0) or EN(1)]\" > /proc/omci/omciMirror\n");
    printk("Example of enable OMCI mirror to phy port 0\n");
    printk("echo \"MIR_PORT 0\" > /proc/omci/omciMirror\n");
    printk("echo \"MIR_STAT 1\" > /proc/omci/omciMirror\n");
    return count;
}


static int usage_read_proc(struct seq_file *seq, void *v)
{
    int i, len = 0;
    for(i=0;i<pPlatformDb->cfNum ;i++){
        if (pPlatformDb->cfRule[i].isCfg)
        printk("CFID: %d, IS_CFG: %u\n",i,pPlatformDb->cfRule[i].isCfg);
    }
    return len;
}


static int range_read_proc(struct seq_file *seq, void *v)
{
    int len = 0;
    omcidrv_wrapper_dumpCfMap();
    return len;
}

static ssize_t range_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t *off)
{
    char tmpbuf[32] = "\0";
    char *strptr = NULL, *tokptr = NULL;
    unsigned int cfType = UINT_MAX, start = 0, stop = 0;

    if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
        strptr = tmpbuf;

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        cfType = simple_strtol(tokptr, NULL, 0);
        printk("cfType %u ", cfType);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        start = simple_strtol(tokptr, NULL, 0);
        printk("start %u ", start);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        stop = simple_strtol(tokptr, NULL, 0);
        printk("stop %u \n", stop);

        omcidrv_wrapper_setCfMap(cfType, start, stop);
    }
    return count;
}


static int devMode_read_proc(struct seq_file *seq, void *v)
{
    int len=0;

    switch(gDrvCtrl.devMode){
    case OMCI_DEV_MODE_BRIDGE:
        printk("devMode: bridge\n");
    break;
    case OMCI_DEV_MODE_ROUTER:
        printk("devMode: router\n");
    break;
    case OMCI_DEV_MODE_HYBRID:
        printk("devMode: hybrid\n");
    break;
    default:
    break;
    }


    return len;
}


static ssize_t devMode_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t *off)
{
    char flag[32]="\0";

    if (count < 2)
        return -EFAULT;


    if (buffer && !copy_from_user(&flag, buffer, sizeof(flag)))
    {

        if(flag[0]=='b')
        {
            gDrvCtrl.devMode = OMCI_DEV_MODE_BRIDGE;
        }else
        if(flag[0]=='r')
        {
            gDrvCtrl.devMode = OMCI_DEV_MODE_ROUTER;
        }else
        if(flag[0]=='h')
        {
            gDrvCtrl.devMode = OMCI_DEV_MODE_HYBRID;
        }
        sprintf(flag,"%s",flag);
        printk("write dev mode to %s\n",flag);
        return count;
    }
    else
        return -EFAULT;

}


static int wanInfo_read_proc(struct seq_file *seq, void *v)
{
    int len = 0;
    omcidrv_dumpWanInfo();
    return len;
}

static ssize_t wanInfo_write_proc(struct file *file, const char *buffer,
                size_t count, loff_t *off)
{
    char tmpbuf[32] = "\0";
    char *strptr = NULL, *tokptr = NULL;
    int netIfIdx, vid, pri, type, service, isBinding, bAdd;

    if (buffer && !copy_from_user(tmpbuf, buffer, count)) {
        strptr = tmpbuf;

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        netIfIdx = simple_strtol(tokptr, NULL, 0);
        printk("netIfIdx %d ", netIfIdx);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        vid = simple_strtol(tokptr, NULL, 0);
        printk("vid %d ", vid);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        pri = simple_strtol(tokptr, NULL, 0);
        printk("pri %d ", pri);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        type = simple_strtol(tokptr, NULL, 0);
        printk("type %d ", type);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        service = simple_strtol(tokptr, NULL, 0);
        printk("service %d ", service);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        isBinding = simple_strtol(tokptr, NULL, 0);
        printk("isBinding %d ", isBinding);

        tokptr = strsep(&strptr, " ");
        if (!tokptr) return -EFAULT;
        bAdd = simple_strtol(tokptr, NULL, 0);
        printk("bAdd %d \n", bAdd);

        omcidrv_updateWanInfoByProcEntry(netIfIdx, vid, pri, type, service, isBinding, bAdd);
    }
    return count;
}


static int macFilter_read_proc(struct seq_file *seq, void *v)
{
    printk("======= MAC filter Rule for US/DS ===========\n");

    omcidrv_wrapper_dumpMacFilter();

    return 0;
}

static int uniQos_read_proc(struct seq_file *seq, void *v)
{
    omcidrv_wrapper_dumpUniQos();
    return 0;
}

static int debugInfo_read_proc(struct seq_file *seq, void *v)
{
    omcidrv_wrapper_dumpDebugInfo();
    return 0;
}

static int omciMirror_read_proc ( struct seq_file* seq, void* v )
{
    printk("echo \"MIR_PORT [PHY_PORT_ID]\" > /proc/omci/omciMirror\n");
    printk("echo \"MIR_STAT [DIS(0) or EN(1)]\" > /proc/omci/omciMirror\n");
    printk("Example of enable OMCI mirror to phy port 0\n");
    printk("echo \"MIR_PORT 0\" > /proc/omci/omciMirror\n");
    printk("echo \"MIR_STAT 1\" > /proc/omci/omciMirror\n");
    printk("===============================================================\n\n");

    printk("Mirror  Status: %s\n", (gDrvCtrl.omciMirrorState ? "On" : "Off"));
    printk("Mirroring Port: %u\n", gDrvCtrl.omciMirroringPort);
    return 0;
}

static int f2dq_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, f2dq_read_proc, NULL);
}

static int veip_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, veip_read_proc, NULL);
}

static int vgf_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, vgf_read_proc, NULL);
}

static int dmMode_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, dmMode_read_proc, NULL);
}

static int wanInfo_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, wanInfo_read_proc, NULL);
}

static int l2Serv_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, l2Serv_read_proc, NULL);
}

static int devMode_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, devMode_read_proc, NULL);
}

static int usage_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, usage_read_proc, NULL);
}

static int range_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, range_read_proc, NULL);
}

static int macFilter_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, macFilter_read_proc, NULL);
}

static int MBServ_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, MBServ_read_proc, NULL);
}

static int uniQos_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, uniQos_read_proc, NULL);
}

static int debugInfo_open_proc(struct inode *inode, struct file *file)
{
    return single_open(file, debugInfo_read_proc, NULL);
}

static int omciMirror_open_proc ( struct inode* inode, struct file* file )
{
    return single_open(file, omciMirror_read_proc, NULL);
}

static int omci_mmap_init(void)
{
    struct page *page;
    pPlatformDb->pMmap = NULL;
    pPlatformDb->pMmap = (unsigned char *)kmalloc(MMT_BUF_SIZE, GFP_KERNEL);

    if (!(pPlatformDb->pMmap))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "[%d] failed kmalloc\n", __LINE__);
        return 0;
    }

    for(page = virt_to_page(pPlatformDb->pMmap); page < virt_to_page(pPlatformDb->pMmap + MMT_BUF_SIZE); page++)
        SetPageReserved(page);

    return 0;
}

static void omci_mmap_clean(void)
{
    struct page *page;
    if (pPlatformDb->pMmap)
    {
        for(page = virt_to_page(pPlatformDb->pMmap); page < virt_to_page(pPlatformDb->pMmap + MMT_BUF_SIZE); page++)
           ClearPageReserved(page);

        kfree(pPlatformDb->pMmap);
    }

    return;
}

static int omcidev_open_proc(struct inode *inode, struct file *filp)
{
    omci_mmap_init();
    return 0;
}

static int omcidev_release_proc(struct inode *inode, struct file *filp)
{
    omci_mmap_clean();
    return 0;
}

static int omcidev_mmap_proc(struct file *filp, struct vm_area_struct *vma)
{

    unsigned long page;
    unsigned long start = (unsigned long)vma->vm_start;
    unsigned long size = (unsigned long)(vma->vm_end-vma->vm_start);
    void *ptr = (void *)(pPlatformDb->pMmap);

    if (size > MMT_BUF_SIZE)
        return -EINVAL;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "map=%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
                        pPlatformDb->pMmap[0],
                        pPlatformDb->pMmap[1],
                        pPlatformDb->pMmap[2],
                        pPlatformDb->pMmap[3],
                        pPlatformDb->pMmap[4],
                        pPlatformDb->pMmap[5],
                        pPlatformDb->pMmap[6],
                        pPlatformDb->pMmap[7]);

    page = (virt_to_phys(ptr)) >> PAGE_SHIFT;

    if (remap_pfn_range(vma, start, page, size, vma->vm_page_prot) < 0)
        return -EAGAIN;


    return 0;
}


struct file_operations veip_fop = {
    .open = veip_open_proc,
    .write = veip_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations vgf_fop = {
    .open = vgf_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations dmMode_fop = {
    .open = dmMode_open_proc,
    .write = dmMode_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations wanInfo_fop = {
    .open = wanInfo_open_proc,
    .write = wanInfo_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations l2_fop = {
    .open = l2Serv_open_proc,
    .write = l2Serv_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations mode_fop = {
    .open = devMode_open_proc,
    .write = devMode_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations usage_fop = {
    .open = usage_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations cfAlloc_fop = {
    .open = range_open_proc,
    .write = range_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations pMacFilter_fop = {
    .open = macFilter_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
struct file_operations mb_fop = {
    .open = MBServ_open_proc,
    .write = MBServ_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

struct file_operations f2dq_fop = {
    .open = f2dq_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

struct file_operations uniQos_fop = {
    .open = uniQos_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

struct file_operations debugInfo_fop = {
    .open = debugInfo_open_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

struct file_operations omciMirror_fop = {
    .open  = omciMirror_open_proc,
    .write = omciMirror_write_proc,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};


struct file_operations dev_fops = {
    .open = omcidev_open_proc,
    .mmap = omcidev_mmap_proc,
    .release = omcidev_release_proc,
};
static struct miscdevice misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "omcidrv",
    .fops = &dev_fops,
};

struct proc_dir_entry *dir = NULL, *l2 = NULL, *mode = NULL;
struct proc_dir_entry *usage = NULL, *cfAlloc = NULL, *pMacFilter = NULL;
struct proc_dir_entry *wanInfo = NULL, *veip = NULL, *vgf = NULL, *dmMode = NULL;
struct proc_dir_entry *mb = NULL, *f2dq = NULL, *uniQos = NULL, *debugInfo = NULL;
struct proc_dir_entry *omciMirror = NULL;

static void omci_proc_init(void)
{

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"omci proc init");

    dir = proc_mkdir("omci",NULL);

    veip = proc_create("veip", 0, dir, &veip_fop);
    if (!veip) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"veip, create proc failed!");
    }

    vgf = proc_create("vgf", 0, dir, &vgf_fop);
    if (!vgf) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"vgf, create proc failed!");
    }

    dmMode = proc_create("dmMode", 0, dir, &dmMode_fop);
    if (!dmMode) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"dmMode, create proc failed!");
    }

    l2 = proc_create("l2", 0, dir, &l2_fop);
    if (!l2) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"l2, create proc failed!");
    }

    mode = proc_create("devMode", 0, dir, &mode_fop);
    if (!mode) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"devmode, create proc failed!");
    }

    usage = proc_create("usage", 0, dir, &usage_fop);
    if (!usage) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"usage, create proc failed!");
    }

    cfAlloc = proc_create("cf_map",0,dir, &cfAlloc_fop);
    if(!cfAlloc) {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"cf_map, create proc failed!");
    }

    wanInfo = proc_create("wanInfo", 0, dir, &wanInfo_fop);
    if(!wanInfo) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "wanInfo, create proc failed!");
    }

    pMacFilter = proc_create("macFilter", 0, dir, &pMacFilter_fop);
    if(!pMacFilter) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "macFilter, create proc failed!");
    }

    mb = proc_create("mb", 0, dir, &mb_fop);
    if(!mb) {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mcast /bcast, create proc failed!");
    }

    f2dq = proc_create("f2dq", 0, dir, &f2dq_fop);
    if (!f2dq) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"f2dq, create proc failed!");
    }

    uniQos = proc_create("uniQos", 0, dir, &uniQos_fop);
    if (!uniQos) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"uniQos, create proc failed!");
    }

    debugInfo = proc_create("debugInfo", 0, dir, &debugInfo_fop);
    if (!debugInfo)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"debugInfo, create proc failed!");
    }

    omciMirror = proc_create("omciMirror", 0, dir, &omciMirror_fop);
    if (!omciMirror)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "omciMirror, create proc failed!");
    }
}

static void omci_proc_exit(void)
{
    if (omciMirror)
    {
        remove_proc_entry("omciMirror", dir);
        omciMirror = NULL;
    }

    if (debugInfo)
    {
        remove_proc_entry("debugInfo", dir);
        debugInfo = NULL;
    }

    if(uniQos)
    {
        remove_proc_entry("uniQos", dir);
        uniQos=NULL;
    }

    if(f2dq)
    {
        remove_proc_entry("f2dq", dir);
        f2dq=NULL;
    }

    if(mb)
    {
        remove_proc_entry("mb", dir);
        mb=NULL;
    }

    if(pMacFilter)
    {
        remove_proc_entry("macFilter", dir);
        pMacFilter=NULL;
    }

    if(wanInfo)
    {
        remove_proc_entry("wanInfo", dir);
        wanInfo=NULL;
    }

    if(cfAlloc)
    {
        remove_proc_entry("cf_map", dir);
        cfAlloc=NULL;
    }

    if(usage)
    {
        remove_proc_entry("usage", dir);
        usage=NULL;
    }

    if(mode)
    {
        remove_proc_entry("devMode", dir);
        mode=NULL;
    }

    if(l2)
    {
        remove_proc_entry("l2", dir);
        l2=NULL;
    }

    if(dmMode)
    {
        remove_proc_entry("dmMode", dir);
        dmMode=NULL;
    }

    if(vgf)
    {
        remove_proc_entry("vgf", dir);
        vgf=NULL;
    }

    if(veip)
    {
        remove_proc_entry("veip", dir);
        veip=NULL;
    }

    if(dir)
    {
        remove_proc_entry("omci", NULL);
        dir = NULL;
    }
}


static void omci_drvCtrl_init(void)
{
    memset(&gDrvCtrl,0,sizeof(gDrvCtrl));
    gDrvCtrl.logLevel = OMCI_LOG_LEVEL_ERR;
    gDrvCtrl.devMode = OMCI_DEV_MODE_HYBRID;
    gDrvCtrl.omciMirroringPort = 0xFFFFFFFF;
}

int omcidrv_platform_register(pf_db_t *pDb)
{
    pWrapper = pDb->pMap;
    pPlatformDb = pDb;

    omcidrv_alloc_resource(pPlatformDb->intfNum);

    return 0;
}

int omcidrv_feature_register(feature_kapi_t *p)
{
    if (FEATURE_KAPI_END <= p->regApiId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Feature API ID out of range:%d", p->regApiId);
        return OMCIDRV_FEATURE_ERR_FAIL;
    }
    if (pPlatformDb->kApiDb[p->regApiId].regCB)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Feature API has already been registered");
        return OMCIDRV_FEATURE_ERR_FAIL;
    }
    else
    {
        pPlatformDb->kApiDb[p->regApiId].regApiId = p->regApiId;
        pPlatformDb->kApiDb[p->regApiId].regCB = p->regCB;
        strncpy(pPlatformDb->kApiDb[p->regApiId].regModuleName, p->regModuleName, strlen(p->regModuleName) + 1);
        return OMCIDRV_FEATURE_ERR_OK;
    }
}

int omcidrv_feature_unregister(feature_kapi_t *p)
{
    if (FEATURE_KAPI_END <= p->regApiId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Feature API ID out of range:%d", p->regApiId);
        return OMCIDRV_FEATURE_ERR_FAIL;
    }

    if (pPlatformDb->kApiDb[p->regApiId].regCB)
    {

        pPlatformDb->kApiDb[p->regApiId].regApiId      = 0;
        pPlatformDb->kApiDb[p->regApiId].regCB         = NULL;
        memset(pPlatformDb->kApiDb[p->regApiId].regModuleName, 0, FEATURE_KMODULE_MAME_LENGTH);
        return OMCIDRV_FEATURE_ERR_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Feature API has already been unregistered");
        return OMCIDRV_FEATURE_ERR_FAIL;
    }
}

int omcidrv_feature_api(unsigned int apiID, ...)
{
    va_list argptr;
    va_list argToAPI;
    int ret;

    if (FEATURE_KAPI_END <= apiID)
        return OMCIDRV_FEATURE_ERR_FAIL;

    if (!pPlatformDb->kApiDb[apiID].regCB)
        return OMCIDRV_FEATURE_ERR_FAIL;

    va_start(argptr, apiID );
    va_copy(argToAPI, argptr);
    va_end (argptr);

    ret = pPlatformDb->kApiDb[apiID].regCB(argToAPI);
    va_end (argToAPI);
    return ret;
}

EXPORT_SYMBOL(omcidrv_platform_register);
EXPORT_SYMBOL(gDrvCtrl);
EXPORT_SYMBOL(omcidrv_feature_register);
EXPORT_SYMBOL(omcidrv_feature_unregister);
EXPORT_SYMBOL(omcidrv_feature_api);

#ifdef OMCI_X86
int do_omcidrv_get_ctl(struct sock *sk, int cmd, void *user, int *len)
{
    omci_ioctl_cmd_t gponExt_cfg;
    switch(cmd)
    {
        case OMCIDRV_GPON_EXTMSG_GET:
            copy_from_user(&gponExt_cfg, user, sizeof(omci_ioctl_cmd_t));
            omcidrv_ioctl((OMCI_IOCTL_t)gponExt_cfg.optId,(void*)gponExt_cfg.extValue);
            copy_to_user(user, &gponExt_cfg, sizeof(omci_ioctl_cmd_t));
            break;
        default:
            break;
    }
    return 0;
}

static struct nf_sockopt_ops omcidrv_sockopts = {
    .pf = PF_INET,
    .set_optmin = 0,
    .set_optmax = 0,
    .set = NULL,
    .get_optmin = OMCIDRV_BASE_CTL,
    .get_optmax = OMCIDRV_GET_MAX+1,
    .get = do_omcidrv_get_ctl,
};
#endif

int __init omci_drv_init(void)
{
#ifdef OMCI_X86
    nf_register_sockopt(&omcidrv_sockopts);
#endif
    omci_drvCtrl_init();
    omci_proc_init();
    misc_register(&misc);
    // remove net_device event notifier
    // omcidrv_initDrvEvent();
    return 0;
}


void __exit omci_drv_exit(void)
{
#ifdef OMCI_X86
    nf_unregister_sockopt(&omcidrv_sockopts);
#endif
    // remove net_device event notifier
    // omcidrv_exitDrvEvent();
    omci_proc_exit();
    omcidrv_dealloc_resource();
    misc_deregister(&misc);
}


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek OMCI kernel module");
MODULE_AUTHOR("RealTek");


module_init(omci_drv_init);
module_exit(omci_drv_exit);


