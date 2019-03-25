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

#include <linux/sched.h>
#include <pkt_redirect/pkt_redirect.h>
#include <DRV/platform/include/rtl9607_rtk/omci_pf_rtl9607.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
/*rtk api*/
#include <dal/apollomp/dal_apollomp_ponmac.h>
#include <rtk/pon_led.h>
#include <rtk/svlan.h>
#include <rtk/vlan.h>
#include <rtk/switch.h>
#include <rtk/l2.h>
#include "rtk/stat.h"
#include "rtk/acl.h"
#include "rtk/trap.h"
#include <rtk/rate.h>
#include <rtk/time.h>
#include <common/util/rt_util.h>
#include "hal/phy/phydef.h"
#include <module/intr_bcaster/intr_bcaster.h>
#include "module/ds_pkt_bcaster/ds_pkt_bcaster.h"
#include <dal/rtl9602c/dal_rtl9602c_switch.h>

#if defined(FPGA_DEFINED)
    #define __pf_rtl96xx_ActiveBdgConn pf_rtl96xx_ActiveBdgConn_fpga
    #define __pf_rtl96xx_DeactiveBdgConn pf_rtl96xx_DeactiveBdgConn_fpga
#else
    #define __pf_rtl96xx_ActiveBdgConn pf_rtl96xx_ActiveBdgConn
    #define __pf_rtl96xx_DeactiveBdgConn pf_rtl96xx_DeactiveBdgConn
#endif



#define BIT_NUM(val) ({         \
    unsigned int  i = 0;        \
    while(val > (1 << i)) i++;  \
    i;                          \
})


static pf_db_t gPlatformDb;

static rtk_classify_field_t isCtag,isStag,etherType,uni,tagVid,tagPri,tosGemId,dei,wanIf,interPri,InnerVlan,aclHit;
//static rtk_acl_field_t aclField_gemPort, aclField_cTag, aclField_sTag, aclField_etype;
static rtk_acl_field_t aclField_cTag;

static uint8    g_isDsDeiDpAclCfg = FALSE;
static int32    g_dsStagDei0DpAcl = -1;
static int32    g_dsStagDei1DpAcl = -1;
static int32    g_dsCtagDei0DpAcl = -1;
static int32    g_dsCtagDei1DpAcl = -1;

#define OMCI_ACL_DELETE -1
#define GROUP_MAC_PROTOCOL_MAX_RULE_NUM 4

#define OMCI_XLATEVID_NOT_USED 0x00000000
#define OMCI_XLATEVID_ONLY_ONE 0x00000001
#define OMCI_XLATEVID_TWO_MORE 0x00000002
#define OMCI_XLATEVID_UNTAG    0xFFFFFFFD

#define OMCI_XLATEVID_RULE_SP2C     1
#define OMCI_XLATEVID_RULE_LUT      2

typedef struct
{
    int aclIdx[GROUP_MAC_PROTOCOL_NUM][GROUP_MAC_PROTOCOL_MAX_RULE_NUM];
}groupMacAclIdx_t;

static groupMacAclIdx_t groupMacAclIdx;

static int omci_findAvailAcl(rtk_acl_ingress_entry_t *pAclRule)
{
    int16                       aclIdx;
    uint16                      maxAclIdx;
    BOOL                        findAvailAcl = FALSE;

    if (!pAclRule)
        return -1;

    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = gPlatformDb.aclStartIdx; aclIdx < maxAclIdx; aclIdx++)
    {
        pAclRule->index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(pAclRule))
        {
            if (!pAclRule->valid)
            {
                findAvailAcl = TRUE;
                break;
            }
        }
    }

    return ((findAvailAcl == FALSE) ? -1 : 0);
}
/* local function */
static dsAggregated_entry_t *omci_find_ds_aggregated_entry(
                                                           dsAggregated_group_t *pGroupEntry,
                                                           int srvId)
{
    dsAggregated_entry_t    *pEntryData     = NULL;
    struct list_head        *pEntry         = NULL, *pTmpEntry  = NULL;

    if (!pGroupEntry)
        return NULL;

    list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
    {
        pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

        if (srvId == pEntryData->id)
            return pEntryData;
    }
    return NULL;
}

static dsAggregated_group_t *omci_find_ds_aggregated_group_entry(
                                                                 PON_GEMPORT_DIRECTION dir,
                                                                 unsigned int dsFlowId)
{
    dsAggregated_group_t    *pGroupEntry    = NULL;
    struct list_head        *pEntry         = NULL, *pTmpEntry  = NULL;
    struct list_head        *pHead          = NULL;

    if (PON_GEMPORT_DIRECTION_BI != dir &&
        PON_GEMPORT_DIRECTION_DS != dir)
        return NULL;

    if (PON_GEMPORT_DIRECTION_BI == dir)
        pHead = &gPlatformDb.ucAggregatedGroupHead;

    if (PON_GEMPORT_DIRECTION_DS == dir)
        pHead = &gPlatformDb.mbAggregatedGroupHead;

    list_for_each_safe(pEntry, pTmpEntry, pHead)
    {
        pGroupEntry = list_entry(pEntry, dsAggregated_group_t, list);

        if (dsFlowId == pGroupEntry->dsFlowId)
            return pGroupEntry;
    }
    return NULL;
}

static unsigned char omci_check_vid_used_in_sp2c(rtk_vlan_t vid, rtk_portmask_t *pPortMask)
{
    struct list_head        *next   = NULL, *tmp = NULL;
    mbcast_service_t        *pMBcur = NULL;
    dsAggregated_entry_t    *pAggr  = NULL;
    unsigned int            outer_vid;

    list_for_each_safe(next, tmp, &gPlatformDb.mbcastHead)
    {
        pMBcur = list_entry(next, mbcast_service_t, list);

        if (VLAN_OPER_MODE_FORWARD_ALL == pMBcur->rule.filterMode)
            continue;

        if (!(pAggr = omci_find_ds_aggregated_entry(
                        (omci_find_ds_aggregated_group_entry(PON_GEMPORT_DIRECTION_DS, pMBcur->dsStreamId)),
                        (int)(pMBcur->index))))
            continue;

        outer_vid = pMBcur->rule.filterRule.filterCTag.vid;
        if ((VLAN_FILTER_VID & pMBcur->rule.filterRule.filterStagMode) ||
            (VLAN_FILTER_TCI & pMBcur->rule.filterRule.filterStagMode))
            outer_vid = pMBcur->rule.filterRule.filterSTag.vid;

        if (outer_vid == vid)
        {
            pPortMask->bits[0] |= pMBcur->uniMask;
            return TRUE;
        }
    }
    return FALSE;
}

static void omci_delMemberPortBySvlan(unsigned int cfIdx, unsigned int systemTpid)
{
    rtk_classify_cfg_t cfg;
    rtk_vlan_t vid = UINT_MAX, idx;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));

    memset(&cfg, 0, sizeof(rtk_classify_cfg_t));
    cfg.index = cfIdx;
    if(RT_ERR_OK != rtk_classify_cfgEntry_get(&cfg))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "No us classfy rule in Hardware ");
        return;
    }

    if(CLASSIFY_US_CSACT_ADD_TAG_VS_TPID == cfg.act.usAct.csAct)
    {
        switch (cfg.act.usAct.csVidAct)
        {
            case CLASSIFY_US_VID_ACT_ASSIGN:
                vid = cfg.act.usAct.sTagVid;
                break;
            case CLASSIFY_US_VID_ACT_FROM_1ST_TAG:
            case CLASSIFY_US_VID_ACT_FROM_2ND_TAG:
                vid = (cfg.field.readField.dataFieldRaw[1] & 0x7f) << 5 |
                    (cfg.field.readField.dataFieldRaw[0] >> 11 & 0x1f);
                break;
            default:
                break;
        }
    }
    else if (0x8100 == systemTpid && CLASSIFY_DIRECTION_DS == cfg.direction &&
        (0xfff == (((cfg.field.readField.careFieldRaw[1] & 0x7f) << 5) |
        ((cfg.field.readField.careFieldRaw[0] >> 11) & 0x1f))) &&
        ((cfg.field.readField.dataFieldRaw[0] >> 4) & 0x1) &&
        ((cfg.field.readField.careFieldRaw[0] >> 4) & 0x1) )
    {
        /* sbit data = 1, sbit mask = 1, tagVid mask = 0xFFF
            get tagVid to remove svlan member port for multicast streaming, such that
            igmp module does NOT handle svlan member
            TBD: bcast should be handled as follow this way.
        */
        idx = (((cfg.field.readField.dataFieldRaw[1] & 0x7f) << 5) |
                ((cfg.field.readField.dataFieldRaw[0] >> 11) & 0x1f));
        if(RT_ERR_OK == rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
        {
            /* vid already exit in vlan table */
            mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
            utgmask.bits[0] |= (1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
            if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
            {
                if (omci_check_vid_used_in_sp2c(idx, &mbrmsk))
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(idx, &mbrmsk, &utgmask));
                }
                else
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_destroy((rtk_vlan_t)idx));
                }
            }
            else
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(idx, &mbrmsk, &utgmask));
            }
        }
    }
    else
    {
        if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_DEL_SVLAN_MBR,
            systemTpid, cfg, &mbrmsk, &utgmask, gPlatformDb.ponPort, &allPortMask))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "no hooking customzied behavior");

            vid = 0;
        }
    }
    if(UINT_MAX != vid)
    {
        if(RT_ERR_OK == rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
        {
            /* vid already exit in vlan table */
            mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vid, &mbrmsk, &utgmask));
        }
    }
    return;
}

static void omci_SetMemberPortBySvlan(rtk_classify_cfg_t *pClassifyCfg, OMCI_BRIDGE_RULE_ts *pBridgeRule, unsigned int systemTpid)
{
    rtk_enable_t enableB;
    rtk_svlan_lookupType_t sType;
    rtk_vlan_t vid = UINT_MAX;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    unsigned int uniPortMask;

    uniPortMask = (((unsigned int)(pBridgeRule->uniMask)) & (~(1 << gPlatformDb.ponPort)));

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(gPlatformDb.ponPort, &enableB));
    if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_lookupType_get(&sType));
        if(SVLAN_LOOKUP_C4KVLAN != sType)
            return;
    }
    if(ENABLED ==enableB)
    {
        if(CLASSIFY_US_CSACT_ADD_TAG_VS_TPID == pClassifyCfg->act.usAct.csAct)
        {
            switch (pClassifyCfg->act.usAct.csVidAct)
            {
                case CLASSIFY_US_VID_ACT_ASSIGN:
                    vid = pClassifyCfg->act.usAct.sTagVid;
                    break;
                case CLASSIFY_US_VID_ACT_FROM_1ST_TAG:
                case CLASSIFY_US_VID_ACT_FROM_2ND_TAG:
                    vid = pBridgeRule->vlanRule.filterRule.filterCTag.vid;
                    break;
                default:
                    break;
            }
        }
        else if (0x8100 == systemTpid && PON_GEMPORT_DIRECTION_DS == pBridgeRule->dir &&
            VLAN_ACT_NON == pBridgeRule->vlanRule.sTagAct.vlanAct &&
            VID_ACT_ASSIGN == pBridgeRule->vlanRule.cTagAct.vidAct)
        {
            /*
                add svlan member port for multicast streaming, such that
                igmp module does NOT handle svlan member
            */
            if(RT_ERR_OK !=
                rtk_vlan_port_get(pBridgeRule->vlanRule.cTagAct.assignVlan.vid, &mbrmsk, &utgmask))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(pBridgeRule->vlanRule.cTagAct.assignVlan.vid));
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | uniPortMask;
                memcpy(&utgmask, &allPortMask, sizeof(rtk_portmask_t));
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= uniPortMask;
            }
            utgmask.bits[0] &= ~(uniPortMask);
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(pBridgeRule->vlanRule.cTagAct.assignVlan.vid, &mbrmsk, &utgmask));
        }
        else
        {
            if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_SET_SVLAN_MBR,
                systemTpid, pBridgeRule, &mbrmsk, &utgmask, gPlatformDb.ponPort, &allPortMask))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "no hooking customzied behavior");
                vid = 0;
            }
        }
        if(UINT_MAX != vid)
        {
            if(RT_ERR_OK != rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(vid));
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | uniPortMask;
                memcpy(&utgmask, &allPortMask, sizeof(rtk_portmask_t));
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= uniPortMask;
            }
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vid, &mbrmsk, &utgmask));
        }
    }
}

static void omci_delServicePort(unsigned int systemTpid, unsigned int usCfIdx)
{
    rtk_enable_t enableB = DISABLED;
    int res = FALSE, idx;
    rtk_classify_cfg_t cfg;
    rtk_port_t port = 0;
    rtk_portmask_t mbrmsk, utgmask;
    uint16 sbitData, sbitMask, cbitData, cbitMask, vidData, vidMask, priData, priMask;
    rtk_portmask_t allPortMask;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));

    memset(&cfg, 0, sizeof(rtk_classify_cfg_t));
    cfg.index = usCfIdx;
    if(RT_ERR_OK != rtk_classify_cfgEntry_get(&cfg))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "No us classfy rule in Hardware ");
        return;
    }

    sbitData = cfg.field.readField.dataFieldRaw[0] >> 4 & 0x1;
    sbitMask = cfg.field.readField.careFieldRaw[0] >> 4 & 0x1;

    cbitData = cfg.field.readField.dataFieldRaw[0] >> 3 & 0x1;
    cbitMask = cfg.field.readField.careFieldRaw[0] >> 3 & 0x1;

    vidData = ((cfg.field.readField.dataFieldRaw[1] & 0x7f) << 5) |
        (cfg.field.readField.dataFieldRaw[0] >> 11 & 0x1f);
    vidMask = ((cfg.field.readField.careFieldRaw[1] & 0x7f) << 5) |
        (cfg.field.readField.careFieldRaw[0] >> 11 & 0x1f);

    priData = cfg.field.readField.dataFieldRaw[0] >> 8 & 0x7;
    priMask = cfg.field.readField.careFieldRaw[0] >> 8 & 0x7;


    if ((0x88a8 == systemTpid || 0x9100 == systemTpid) && APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        if(0 == sbitData && 1 == sbitMask && 0 == cbitData && 1 == cbitMask)
        {
            res = TRUE;
            /* for 6.1.1 type only forward untag packet from UTP and packet with stag cannot be forward */
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] &= (~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7)));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_vlan_port_set(0, &mbrmsk, &utgmask));
        }
        else if(0 == sbitData && 0 == sbitMask && 0 == cbitData && 0 == cbitMask &&
            CLASSIFY_US_CSACT_TRANSPARENT == cfg.act.usAct.csAct &&
            CLASSIFY_US_CACT_TRANSPARENT == cfg.act.usAct.cAct && 0xfff == vidMask)
        {
            res = TRUE;
            /* for 6.1.5 type no filter stag and ctag and actions of stag and ctag are NO ACT */
            /* create outer vid to vlan table and set member port */

            if(RT_ERR_OK != rtk_vlan_port_get(vidData, &mbrmsk, &utgmask))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "VID(%u) is not in vlan table", vidData);
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                utgmask.bits[0] |= (1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));

                if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
                {
                    if (omci_check_vid_used_in_sp2c(vidData, &mbrmsk))
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set((rtk_vlan_t)vidData, &mbrmsk, &utgmask));
                    }
                    else
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_destroy((rtk_vlan_t)vidData));
                    }
                }
                else
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vidData, &mbrmsk, &utgmask));
                }
            }
            /* if ingress without stag */
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &mbrmsk, &utgmask));
        }
        else if((1 == sbitData && 1 == sbitMask && 0x7 == priMask && 0x0 == vidMask) ||
            (0 == sbitData && 0 == sbitMask && 0 == cbitData && 0 == cbitMask &&
            0 == vidData && 0 == priData && 0 == priMask &&  0 == vidMask &&
            CLASSIFY_US_CSACT_TRANSPARENT == cfg.act.usAct.csAct &&
            CLASSIFY_US_CACT_TRANSPARENT == cfg.act.usAct.cAct))
        {
            res = TRUE;
            /* for 6.1.24 type only filter s-pbit and send packet with stag from UTP port */
            for(idx = 0; idx <= 4095; idx++)
            {
                if(RT_ERR_OK == rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
                {
                    /* vid already exit in vlan table */
                    mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                    utgmask.bits[0] |= (1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                    if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
                    {
                        if (omci_check_vid_used_in_sp2c(idx, &mbrmsk))
                        {
                            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(idx, &mbrmsk, &utgmask));
                        }
                        else
                        {
                            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_destroy((rtk_vlan_t)idx));
                        }
                    }
                    else
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(idx, &mbrmsk, &utgmask));
                    }
                }
            }
        }
        else if(((1 == sbitData && 1 == sbitMask) || (0 == sbitData && 0 == sbitMask)) &&
            0xfff == vidMask)
        {
            res = TRUE;
            /* for 6.1.6 type filter stag and send packet with stag from UTP port */
            /* create outer vid to vlan table and set member port */
            if(RT_ERR_OK != rtk_vlan_port_get(vidData, &mbrmsk, &utgmask))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "VID(%u) is not in vlan table", vidData);
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                utgmask.bits[0] |= (1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
                {
                    if (omci_check_vid_used_in_sp2c(vidData, &mbrmsk))
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vidData, &mbrmsk, &utgmask));
                    }
                    else
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_destroy((rtk_vlan_t)vidData));
                    }
                }
                else
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vidData, &mbrmsk, &utgmask));
                }
            }
        }
        if (TRUE == res)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] &= (~(1 << gPlatformDb.cpuPort));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &mbrmsk, &utgmask));
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(port, &enableB));
                if(port == (cfg.field.readField.dataFieldRaw[0] & 0x7))
                    enableB = DISABLED;
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, enableB));
            }
        }
    }
    else if (0x8100 == systemTpid &&
        (RTL9601B_CHIP_ID == gPlatformDb.chipId || RTL9602C_CHIP_ID == gPlatformDb.chipId))
    {
        for (port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(port, &enableB))
            if (port == (cfg.field.readField.dataFieldRaw[0] & 0x7))
                enableB = ENABLED;
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, enableB));
        }
    }
}

static void omci_SetServicePort(unsigned int systemTpid, rtk_vlan_t vid, OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    rtk_enable_t enableB = DISABLED;
    int res = FALSE, idx;
    rtk_port_t port = 0;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    unsigned int uniPortMask;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));

    uniPortMask = (((unsigned int)(pBridgeRule->uniMask)) & (~(1 << gPlatformDb.ponPort)));

    if ((0x88a8 == systemTpid || 0x9100 == systemTpid) && APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {

        if(VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode &&
            VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterStagMode &&
            VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterCtagMode)
        {
            res = TRUE;
            /* for 6.1.1 type only forward untag packet from UTP and packet with stag cannot be forward */
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] = mbrmsk.bits[0] | uniPortMask;
            utgmask.bits[0] |= uniPortMask;
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &mbrmsk, &utgmask));
        }
        else if(VLAN_OPER_MODE_FILTER_SINGLETAG == pBridgeRule->vlanRule.filterMode &&
            (VLAN_ACT_NON == pBridgeRule->vlanRule.sTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.sTagAct.vlanAct) &&
            (VLAN_ACT_NON == pBridgeRule->vlanRule.cTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.cTagAct.vlanAct))
        {
            res = TRUE;
            /* for 6.1.5 type no filter stag and ctag and actions of stag and ctag are NO ACT */
            /* create outer vid to vlan table and set member port */
            if(RT_ERR_OK != rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(vid));
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | uniPortMask;
                memcpy(&utgmask, &allPortMask, sizeof(rtk_portmask_t));
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= uniPortMask;
            }
            utgmask.bits[0] &= ~(uniPortMask);
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(vid, &mbrmsk, &utgmask));
            /* if ingress without stag */
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] |= uniPortMask;
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &mbrmsk, &utgmask));

        }
        else if((VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode &&
            VLAN_FILTER_PRI == pBridgeRule->vlanRule.filterRule.filterStagMode) ||
            (VLAN_OPER_MODE_FORWARD_ALL == pBridgeRule->vlanRule.filterMode &&
            (VLAN_ACT_NON == pBridgeRule->vlanRule.sTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.sTagAct.vlanAct) &&
            (VLAN_ACT_NON == pBridgeRule->vlanRule.cTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.cTagAct.vlanAct)))
        {
            res = TRUE;
            /* for 6.1.24 type  only filter s-pbit and send packet with stag from UTP port */
            /* for forward all with transparent*/
            for(idx = 0; idx <= 4095; idx++)
            {
                if(RT_ERR_OK != rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(idx));
                    mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | uniPortMask;
                    memcpy(&utgmask, &allPortMask, sizeof(rtk_portmask_t));
                }
                else
                {
                    /* vid already exit in vlan table */
                    mbrmsk.bits[0] |= uniPortMask;
                }
                utgmask.bits[0] &= ~(uniPortMask);
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(idx, &mbrmsk, &utgmask));
            }
        }
        else if(VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode &&
            (!(VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterStagMode)))
        {
            res = TRUE;
            /* for 6.1.6 type filter stag and send packet with stag from UTP port */
            /* create outer vid to vlan table and set member port */
            if(RT_ERR_OK != rtk_vlan_port_get(pBridgeRule->vlanRule.filterRule.filterSTag.vid, &mbrmsk, &utgmask))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(pBridgeRule->vlanRule.filterRule.filterSTag.vid));
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | uniPortMask;
                memcpy(&utgmask, &allPortMask, sizeof(rtk_portmask_t));
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= uniPortMask;
            }
            utgmask.bits[0] &= ~(uniPortMask);
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(pBridgeRule->vlanRule.filterRule.filterSTag.vid, &mbrmsk, &utgmask));
        }
        if(TRUE == res)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_get(0, &mbrmsk, &utgmask));
            mbrmsk.bits[0] = mbrmsk.bits[0]  | (1 << gPlatformDb.cpuPort);
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &mbrmsk, &utgmask));
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(port, &enableB));
                if((1 << port) & uniPortMask)
                    enableB = ENABLED;
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, enableB));
            }
        }
    }
    else if ((RTL9601B_CHIP_ID == gPlatformDb.chipId ||
            RTL9602C_CHIP_ID == gPlatformDb.chipId) && 0x8100 == systemTpid)
    {
        if (pBridgeRule->vlanRule.outStyle.isDefaultRule != OMCI_EXTVLAN_REMOVE_TAG_DISCARD &&
            (pBridgeRule->vlanRule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG) &&
            (pBridgeRule->vlanRule.outStyle.outTagNum > 2))
        {

           res = TRUE;
        }
        if (TRUE == res)
        {
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(port, &enableB));
                if ((1 << port) & uniPortMask)
                    enableB = DISABLED;
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, enableB));
            }
        }
    }

    return;
}

static void portMask2Str(uint32 portMask, uint8 *pBuf)
{
    uint8 i, tmp[16];
    memset(pBuf, '\0', 30);

    for (i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
    {
        if(portMask & (1 << i))
        {
            if(strlen(pBuf) > 0)
                sprintf(tmp, ",%u", i);
            else
                sprintf(tmp, "%u", i);
            strcat(pBuf, tmp);
        }
    }
}

static int _classf_show_dsAction(rtk_classify_ds_act_t *pAct)
{
    //rtk_portmask_t portlist;
    uint8   buf[30];

    printk("Stag action: %s\n", diagStr_dsCStagAction[pAct->csAct]);
    if(CLASSIFY_DS_CSACT_ADD_TAG_VS_TPID == pAct->csAct ||
       CLASSIFY_DS_CSACT_ADD_TAG_8100 == pAct->csAct)
    {

        printk("Stag VID action: %s\n", diagStr_dsVidAction[pAct->csVidAct]);
        if(CLASSIFY_DS_VID_ACT_ASSIGN == pAct->csVidAct)
            printk("Stag VID: %d\n", pAct->sTagVid);

        printk("Stag PRI action: %s\n", diagStr_dsPriAction[pAct->csPriAct]);
        if(CLASSIFY_DS_PRI_ACT_ASSIGN == pAct->csPriAct)
            printk("Stag PRI: %d\n", pAct->sTagPri);

    }

    printk("Ctag action: %s\n", diagStr_dsCtagAction[pAct->cAct]);
    if(CLASSIFY_DS_CACT_ADD_CTAG_8100 == pAct->cAct)
    {
        printk("Ctag VID action: %s\n", diagStr_dsVidAction[pAct->cVidAct]);
        if(CLASSIFY_DS_VID_ACT_ASSIGN == pAct->cVidAct)
            printk("Ctag VID: %d\n", pAct->cTagVid);

        printk("Ctag PRI action: %s\n", diagStr_dsPriAction[pAct->cPriAct]);
        if(CLASSIFY_DS_PRI_ACT_ASSIGN == pAct->cPriAct)
            printk("Ctag PRI: %d\n", pAct->cTagPri);
    }

    printk("Classf PRI action: %s\n", diagStr_cfpriAction[pAct->interPriAct]);
    if(CLASSIFY_CF_PRI_ACT_ASSIGN == pAct->interPriAct)
        printk("CF PRI: %d\n", pAct->cfPri);

    printk("UNI action: %s\n", diagStr_dsUniAction[pAct->uniAct]);

    portMask2Str(pAct->uniMask.bits[0], buf);
    printk("UNI ports: %s\n", buf);

    printk("DSCP remarking action: %s\n", pAct->dscp ? "ENABLE":"DISABLE");

    return RT_ERR_OK;
}

static int _classf_show_usAction(rtk_classify_us_act_t *pAct)
{
    //rtk_portmask_t portlist;
    //uint8   buf[30];

    printk("Stag action: %s\n", diagStr_usCStagAction[pAct->csAct]);
    if(CLASSIFY_US_CSACT_ADD_TAG_VS_TPID == pAct->csAct ||
       CLASSIFY_US_CSACT_ADD_TAG_8100 == pAct->csAct)
    {
        printk("Stag VID action: %s\n", diagStr_usVidAction[pAct->csVidAct]);
       /* if(CLASSIFY_US_VID_ACT_ASSIGN == pAct->csVidAct)*/
            printk("Stag VID: %d\n", pAct->sTagVid);

        printk("Stag PRI action: %s\n", diagStr_usPriAction[pAct->csPriAct]);
        /*if(CLASSIFY_US_PRI_ACT_ASSIGN == pAct->csPriAct)*/
            printk("Stag PRI: %d\n", pAct->sTagPri);
    }

    printk("Ctag action: %s\n", diagStr_usCtagAction[pAct->cAct]);


    if(CLASSIFY_US_CACT_ADD_CTAG_8100 == pAct->cAct)
    {
        printk("Ctag VID action: %s\n", diagStr_usVidAction[pAct->cVidAct]);
       /* if(CLASSIFY_US_VID_ACT_ASSIGN == pAct->cVidAct)*/
            printk("Ctag VID: %d\n", pAct->cTagVid);

        printk("Ctag PRI action: %s\n", diagStr_usPriAction[pAct->cPriAct]);
        /*if(CLASSIFY_US_PRI_ACT_ASSIGN == pAct->csPriAct)*/
            printk("Ctag PRI: %d\n", pAct->cTagPri);
    }


    printk("SID action: %s\n", diagStr_usSidAction[pAct->sidQidAct]);
    printk("Assign ID: %d\n", pAct->sidQid);


    printk("Classf PRI action: %s\n", diagStr_cfpriAction[pAct->interPriAct]);
    if(CLASSIFY_CF_PRI_ACT_ASSIGN == pAct->interPriAct)
        printk("CF PRI: %d\n", pAct->cfPri);

    printk("DSCP remarking action: %s\n", pAct->dscp ? "ENABLE":"DISABLE");
    printk("Drop action: %s\n", pAct->drop ? "ENABLE":"DISABLE");
    printk("logging action: %s\n", pAct->log ? "ENABLE":"DISABLE");
    if(CLASSIFY_US_LOG_ACT_ENABLE == pAct->log)
        printk("logging index: %d\n", pAct->logCntIdx);

    return OMCI_ERR_OK;
}


static uint32 _classf_show_field_data(rtk_classify_field_t *pField)
{
    switch(pField->fieldType)
    {
        case CLASSIFY_FIELD_ETHERTYPE:
            printk("ether type data: 0x%04x\n", pField->classify_pattern.etherType.value);
            printk("           mask: 0x%x\n", pField->classify_pattern.etherType.mask);
        break;
        case CLASSIFY_FIELD_TOS_DSIDX:
            printk("tos/sid data: 0x%x\n", pField->classify_pattern.tosDsidx.value);
            printk("        mask: 0x%x\n", pField->classify_pattern.tosDsidx.mask);
        break;
        case CLASSIFY_FIELD_TAG_VID:
            printk("tag vid data: %d\n", pField->classify_pattern.tagVid.value);
            printk("        mask: 0x%x\n", pField->classify_pattern.tagVid.mask);
        break;
        case CLASSIFY_FIELD_TAG_PRI:
            printk("tag priority data: %d\n", pField->classify_pattern.tagPri.value);
            printk("             mask: 0x%x\n", pField->classify_pattern.tagPri.mask);
        break;
        case CLASSIFY_FIELD_INTER_PRI:
            printk("internal priority data: %d\n", pField->classify_pattern.interPri.value);
            printk("                  mask: 0x%x\n", pField->classify_pattern.interPri.mask);
        break;
        case CLASSIFY_FIELD_IS_CTAG:
            printk("c-bit data: %d\n", pField->classify_pattern.isCtag.value);
            printk("      mask: 0x%x\n", pField->classify_pattern.isCtag.mask);
        break;
        case CLASSIFY_FIELD_IS_STAG:
            printk("s-bit data: %d\n", pField->classify_pattern.isStag.value);
            printk("      mask: 0x%x\n", pField->classify_pattern.isStag.mask);
        break;
        case CLASSIFY_FIELD_UNI:
            printk("UNI data: %d\n", pField->classify_pattern.uni.value);
            printk("    mask: 0x%x\n", pField->classify_pattern.uni.mask);
        break;
        case CLASSIFY_FIELD_ACL_HIT:
            printk("ACL hit data: %d\n", pField->classify_pattern.aclHit.value);
            printk("        mask: 0x%x\n", pField->classify_pattern.aclHit.mask);
        break;
        case CLASSIFY_FIELD_DEI:
            printk("DEI  data: %d\n", pField->classify_pattern.dei.value);
            printk("     mask: 0x%x\n", pField->classify_pattern.dei.mask);
        break;
        default:
        break;
    }

    if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        switch(pField->fieldType)
        {
            case CLASSIFY_FIELD_PORT_RANGE:
                printk("L4-port range data: %d\n", pField->classify_pattern.portRange.value);
                printk("              mask: 0x%x\n", pField->classify_pattern.portRange.mask);
            break;
            case CLASSIFY_FIELD_IP_RANGE:
                printk("IP range data: %d\n", pField->classify_pattern.ipRange.value);
                printk("         mask: 0x%x\n", pField->classify_pattern.ipRange.mask);
            break;
            case CLASSIFY_FIELD_WAN_IF:
                printk("WAN interface data: %d\n", pField->classify_pattern.wanIf.value);
                printk("              mask: 0x%x\n", pField->classify_pattern.wanIf.mask);
            break;
            case CLASSIFY_FIELD_IP6_MC:
                printk("IPv6 multicast data: %d\n", pField->classify_pattern.ip6Mc.value);
                printk("               mask: 0x%x\n", pField->classify_pattern.ip6Mc.mask);
            break;
            case CLASSIFY_FIELD_IP4_MC:
                printk("IPv4 multicast data: %d\n", pField->classify_pattern.ip4Mc.value);
                printk("               mask: 0x%x\n", pField->classify_pattern.ip4Mc.mask);
            break;
            case CLASSIFY_FIELD_MLD:
                printk("MLD data: %d\n", pField->classify_pattern.mld.value);
                printk("    mask: 0x%x\n", pField->classify_pattern.mld.mask);
            break;
            case CLASSIFY_FIELD_IGMP:
                printk("IGMP data: %d\n", pField->classify_pattern.igmp.value);
                printk("     mask: 0x%x\n", pField->classify_pattern.igmp.mask);
            break;
            default:
            break;
        }
    }
    else if(RTL9601B_CHIP_ID == gPlatformDb.chipId)
    {
        switch(pField->fieldType)
        {
            case CLASSIFY_FIELD_INNER_VLAN:
                printk("INNER VLAN  data: 0x%x\n", pField->classify_pattern.innerVlan.value);
                printk("            mask: 0x%x\n", pField->classify_pattern.innerVlan.mask);
            break;
            default:
            break;
        }
    }
    else if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
    {
        switch(pField->fieldType)
        {
            case CLASSIFY_FIELD_WAN_IF:
                printk("WAN interface data: %d\n", pField->classify_pattern.wanIf.value);
                printk("              mask: 0x%x\n", pField->classify_pattern.wanIf.mask);
            break;
            case CLASSIFY_FIELD_IGMP_MLD:
                printk("IGMP/MLD data: %d\n", pField->classify_pattern.igmp_mld.value);
                printk("         mask: 0x%x\n", pField->classify_pattern.igmp_mld.mask);
            break;
            case CLASSIFY_FIELD_PPPOE:
                printk("PPPOE data: %d\n", pField->classify_pattern.pppoe.value);
                printk("      mask: 0x%x\n", pField->classify_pattern.pppoe.mask);
            break;
            case CLASSIFY_FIELD_IPV4:
                printk("IPv4 data: %d\n", pField->classify_pattern.ipv4.value);
                printk("     mask: 0x%x\n", pField->classify_pattern.ipv4.mask);
            break;
            case CLASSIFY_FIELD_IPV6:
                printk("IPv6 data: %d\n", pField->classify_pattern.ipv6.value);
                printk("     mask: 0x%x\n", pField->classify_pattern.ipv6.mask);
            break;
            case CLASSIFY_FIELD_IPMC:
                printk("IPMC/IP6MC data: %d\n", pField->classify_pattern.ipmc.value);
                printk("           mask: 0x%x\n", pField->classify_pattern.ipmc.mask);
            break;
            case CLASSIFY_FIELD_INTERNAL_VID_TAG_IF:
                printk("Int Vid Tag If  data: %d\n", pField->classify_pattern.intVidTagIf.value);
                printk("                mask: 0x%x\n", pField->classify_pattern.intVidTagIf.mask);
            break;
            case CLASSIFY_FIELD_CF_ACL_HIT:
                printk("CF ALC hit data: %d\n", pField->classify_pattern.cfAclHit.value);
                printk("           mask: 0x%x\n", pField->classify_pattern.cfAclHit.mask);
            break;
            case CLASSIFY_FIELD_STPID_SEL:
                printk("STPID Sel data: %d\n", pField->classify_pattern.stpidSel.value);
                printk("          mask: 0x%x\n", pField->classify_pattern.stpidSel.mask);
            break;
            case CLASSIFY_FIELD_INNER_VLAN:
                printk("INNER VLAN  data: 0x%x\n", pField->classify_pattern.innerVlan.value);
                printk("            mask: 0x%x\n", pField->classify_pattern.innerVlan.mask);
            break;
            default:
            break;
        }
    }

    return RT_ERR_OK;
}

/*
 * classf show for parsed field
 */
static void _classf_show_field(rtk_classify_cfg_t *pCfg)
{
    rtk_classify_field_t *cf_field;

    printk("==========================================\n");
    printk("Index: %u\n", pCfg->index);
    printk("==========================================\n");
    printk("Not: %s\n", pCfg->invert ? "enable" : "disable");
    printk("Direction: %s\n", pCfg->direction ? "Downstream" : "Upstream");

    cf_field = pCfg->field.pFieldHead;
    while(cf_field != NULL)
    {
        printk("Rule: \n");
        _classf_show_field_data(cf_field);
        cf_field = cf_field->next;
    }

    if(CLASSIFY_DIRECTION_DS == pCfg->direction)
    {
        printk("Downstream action: \n");
        _classf_show_dsAction(&pCfg->act.dsAct);
    }
    else
    {
        printk("Upstream action: \n");
        _classf_show_usAction(&pCfg->act.usAct);
    }
    return;
}

/*
 * classf show for raw_field
 */
static int _classf_show(rtk_classify_cfg_t *pCfg)
{

    printk("==========================================\n");
    printk("Index: %u\n", pCfg->index);
    printk("==========================================\n");
    printk("Not: %s\n", pCfg->invert ? "enable" : "disable");
    printk("Direction: %s\n", pCfg->direction ? "Downstream" : "Upstream");


    printk("%15s: (%d,0x%x)\n","TagVID",
        (pCfg->field.readField.dataFieldRaw[1] & 0x7f) << 5 | (pCfg->field.readField.dataFieldRaw[0] >> 11 & 0x1f),
        (pCfg->field.readField.careFieldRaw[1] & 0x7f) << 5 | (pCfg->field.readField.careFieldRaw[0] >> 11 & 0x1f));

    printk("%15s: (%d,0x%x)\n","TagPri",
        pCfg->field.readField.dataFieldRaw[0] >> 8 & 0x7,
        pCfg->field.readField.careFieldRaw[0] >> 8 & 0x7);

    printk("%15s: (%d,0x%x)\n","IntrPri",
        pCfg->field.readField.dataFieldRaw[0] >> 5 & 0x7,
        pCfg->field.readField.careFieldRaw[0] >> 5 & 0x7);

    printk("%15s: (%d,0x%x)\n","S Bit",
        pCfg->field.readField.dataFieldRaw[0] >> 4 & 0x1,
        pCfg->field.readField.careFieldRaw[0] >> 4 & 0x1);

    printk("%15s: (%d,0x%x)\n","C Bit",
        pCfg->field.readField.dataFieldRaw[0] >> 3 & 0x1,
        pCfg->field.readField.careFieldRaw[0] >> 3 & 0x1);

    printk("%15s: (%d,0x%x)\n","UNI",
        pCfg->field.readField.dataFieldRaw[0] & 0x7,
        pCfg->field.readField.careFieldRaw[0] & 0x7);

    if((RTL9601B_CHIP_ID == gPlatformDb.chipId) || (APOLLOMP_CHIP_ID == gPlatformDb.chipId))
    {
        if(pCfg->index > gPlatformDb.veipFastStop)
        {
            printk("%15s: (%d,0x%x)\n","TOS/TC/GEMIDX",
                (pCfg->field.readField.dataFieldRaw[1] >> 7) & 0xff,
                (pCfg->field.readField.careFieldRaw[1] >> 7) & 0xff);

            printk("%15s: (0x%x,0x%x)\n","EtherType",
                pCfg->field.readField.dataFieldRaw[2],
                pCfg->field.readField.careFieldRaw[2]);
        }
        else
        {
            printk("%15s: (%d,0x%x)\n","DEI",
                pCfg->field.readField.dataFieldRaw[1] >> 7 & 0x1,
                pCfg->field.readField.careFieldRaw[1] >> 7 & 0x1);

            if(RTL9601B_CHIP_ID == gPlatformDb.chipId)
            {
                printk("%15s: (%d,0x%x)\n","ACLHitLatchIndex",
                    pCfg->field.readField.dataFieldRaw[1] >> 8 & 0x1,
                    pCfg->field.readField.careFieldRaw[1] >> 8 & 0x1);

                printk("%15s: (%d,0x%x)\n", "Inner Tag PRI",
                    pCfg->field.readField.dataFieldRaw[2] >> 13 & 0x7,
                    pCfg->field.readField.careFieldRaw[2] >> 13 & 0x7);

                printk("%15s: (%d,0x%x)\n", "Inner Tag CFI",
                    pCfg->field.readField.dataFieldRaw[2] >> 12 & 0x1,
                    pCfg->field.readField.careFieldRaw[2] >> 12 & 0x1);

                printk("%15s: (%d,0x%x)\n", "Inner Tag VID",
                    pCfg->field.readField.dataFieldRaw[2] & 0xfff,
                    pCfg->field.readField.careFieldRaw[2] & 0xfff);
            }
            else
            {
                printk("%15s: (%d,0x%x)\n","IGMP",
                    pCfg->field.readField.dataFieldRaw[1] >> 8 & 0x1,
                    pCfg->field.readField.careFieldRaw[1] >> 8 & 0x1);

                printk("%15s: (%d,0x%x)\n","MLD",
                    pCfg->field.readField.dataFieldRaw[1] >> 9 & 0x1,
                    pCfg->field.readField.careFieldRaw[1] >> 9 & 0x1);

                printk("%15s: (%d,0x%x)\n","IP4 MC",
                    pCfg->field.readField.dataFieldRaw[1] >> 10 & 0x1,
                    pCfg->field.readField.careFieldRaw[1] >> 10 & 0x1);

                printk("%15s: (%d,0x%x)\n","IP6 MC",
                    pCfg->field.readField.dataFieldRaw[1] >> 11 & 0x1,
                    pCfg->field.readField.careFieldRaw[1] >> 11 & 0x1);

                printk("%15s: (%d,0x%x)\n","WAN IF",
                    pCfg->field.readField.dataFieldRaw[1] >> 12 & 0x7,
                    pCfg->field.readField.careFieldRaw[1] >> 12 & 0x7);

                printk("%15s: (%d,0x%x)\n","ACLHitLatchIndex",
                    pCfg->field.readField.dataFieldRaw[2] & 0xff,
                    pCfg->field.readField.careFieldRaw[2] & 0xff);

                printk("%15s: (%d,0x%x)\n","IP Range",
                    pCfg->field.readField.dataFieldRaw[2] >> 8 & 0xf,
                    pCfg->field.readField.careFieldRaw[2] >> 8 & 0xf);

                printk("%15s: (%d,0x%x)\n","PortRange",
                    pCfg->field.readField.dataFieldRaw[2] >> 12 & 0xf,
                    pCfg->field.readField.careFieldRaw[2] >> 12 & 0xf);
            }
        }
    }
    else if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
    {
        if(pCfg->index > gPlatformDb.veipFastStop)
        {
            printk("%15s: (0x%x,0x%x)\n", "STPID Sel",
            pCfg->field.readField.dataFieldRaw[0] >> 6 & 0x1, pCfg->field.readField.careFieldRaw[0] >> 6 & 0x1);
            printk("%15s: (0x%x,0x%x)\n", "DEI",
            pCfg->field.readField.dataFieldRaw[0] >> 7 & 0x1, pCfg->field.readField.careFieldRaw[0] >> 7 & 0x1);
            printk("%15s: (%d,0x%x)\n", "TOS/TC/GEMIDX",
           (pCfg->field.readField.dataFieldRaw[1] >> 7) & 0xff,(pCfg->field.readField.careFieldRaw[1] >> 7) & 0xff);
            printk("%15s: (0x%x,0x%x)\n", "EtherType/Ctag",
            pCfg->field.readField.dataFieldRaw[2], pCfg->field.readField.careFieldRaw[2]);
        }
        else
        {
            printk("%15s: (%d,0x%x)\n", "CFACLHitIndex",
            pCfg->field.readField.dataFieldRaw[1] >> 7 & 0x7f, pCfg->field.readField.careFieldRaw[1] >> 7 & 0x1);
            printk("%15s: (%d,0x%x)\n", "IntVidTagIf",
            pCfg->field.readField.dataFieldRaw[1] >> 14 & 0x1, pCfg->field.readField.careFieldRaw[1] >> 14 & 0x1);
            printk("%15s: (%d,0x%x)\n", "ACLHitLatchIndex",
            pCfg->field.readField.dataFieldRaw[2] & 0x7f, pCfg->field.readField.careFieldRaw[2] & 0x7f);
            printk("%15s: (%d,0x%x)\n", "IGMP/MLD",
            pCfg->field.readField.dataFieldRaw[2] >> 7 & 0x1, pCfg->field.readField.careFieldRaw[2] >> 7 & 0x1);
            printk("%15s: (%d,0x%x)\n", "IPMC",
            pCfg->field.readField.dataFieldRaw[2] >> 8 & 0x1, pCfg->field.readField.careFieldRaw[2] >> 8 & 0x1);
            printk("%15s: (%d,0x%x)\n", "IPv6",
            pCfg->field.readField.dataFieldRaw[2] >> 9 & 0x1, pCfg->field.readField.careFieldRaw[2] >> 9 & 0x1);
            printk("%15s: (%d,0x%x)\n", "IPv4",
            pCfg->field.readField.dataFieldRaw[2] >> 10 & 0x1, pCfg->field.readField.careFieldRaw[2] >> 10 & 0x1);
            printk("%15s: (%d,0x%x)\n", "PPPOE",
            pCfg->field.readField.dataFieldRaw[2] >> 11 & 0x1, pCfg->field.readField.careFieldRaw[2] >> 11 & 0x1);
            printk("%15s: (%d,0x%x)\n", "WAN IF",
            pCfg->field.readField.dataFieldRaw[2] >> 12 & 0xf, pCfg->field.readField.careFieldRaw[2] >> 12 & 0xf);
        }
    }


    if(CLASSIFY_DIRECTION_DS == pCfg->direction)
    {
        printk("Downstream action: \n");
        _classf_show_dsAction(&pCfg->act.dsAct);
    }
    else
    {
        printk("Upstream action: \n");
        _classf_show_usAction(&pCfg->act.usAct);
    }
    return OMCI_ERR_OK;
}    /* end of cparser_cmd_classf_show_rule */



/*
Tcont entry maintain
*/
static int _AssignNonUsedTcontId(unsigned int allocId, unsigned int *pTcontId)
{
    unsigned int i;
    for(i=0;i<gPlatformDb.maxTcont;i++)
    {
        if(gPlatformDb.tCont[i].allocId == allocId)
        {
            *pTcontId = i;
            return OMCI_ERR_OK;
        }
    }

    for(i=0;i<gPlatformDb.maxTcont;i++)
    {
        if(gPlatformDb.tCont[i].allocId ==0xff)
        {
            gPlatformDb.tCont[i].allocId = allocId;
            *pTcontId = i;
            return OMCI_ERR_OK;
        }
    }
    return OMCI_ERR_FAILED;
}




static int _RemoveUsedTcontId(unsigned int tcontId)
{
    // int i = tcontId/2;
    // adjust queue arrangement instead of reduce available t-cont
    unsigned int i = tcontId;

    if(i < gPlatformDb.maxTcont)
        gPlatformDb.tCont[i].allocId = 0xff;

    return OMCI_ERR_OK;
}

static int _InitUsedTcontId(void)
{
    unsigned int i;
    rtk_gpon_tcont_ind_t ind;

    for(i=0;i<gPlatformDb.maxTcont;i++)
    {
        if(gPlatformDb.tCont[i].allocId!=0xff)
        {
            ind.alloc_id = gPlatformDb.tCont[i].allocId;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"init tcont allocId %d",ind.alloc_id);
            rtk_gponapp_tcont_destroy_logical(&ind);
            gPlatformDb.tCont[i].allocId = 0xff;
        }
    }
    return 0;
}


static void _AllocateTcont(rtk_gpon_ploam_t *ploam)
{
    int ret;
    int alloc;
    rtk_gpon_tcont_ind_t ind;
    rtk_gpon_tcont_attr_t attr;

    alloc = (ploam->msg[0]<<4)|(ploam->msg[1]>>4);

    if(alloc==ploam->onuid)
        return;

    memset(&ind,0,sizeof(rtk_gpon_tcont_ind_t));
    memset(&attr,0,sizeof(rtk_gpon_tcont_attr_t));

    ind.alloc_id = alloc;
    ind.type  =RTK_GPON_TCONT_TYPE_1;

    ret = rtk_gponapp_tcont_get(&ind,&attr);

    /* de-allocate this alloc-id */
    if(ploam->msg[2]==0xFF && ret == RT_ERR_OK)
    {
       _RemoveUsedTcontId(attr.tcont_id);
       rtk_gponapp_tcont_destroy(&ind);
    }
    /* allocate this GEM alloc-id */
    else if(ploam->msg[2]==1 /*&& ret !=RT_ERR_OK*/)
    {
       ret = _AssignNonUsedTcontId(alloc, &attr.tcont_id);
       if(ret == OMCI_ERR_OK)
       {
            ret = rtk_gponapp_tcont_create(&ind,&attr);
       }
    }
    return;
}


static void _saveCfCfgToDb(unsigned int cfIdx, rtk_classify_cfg_t *pClassifyCfg)
{
    rtk_classify_field_t *pCurCfg_field, *pTmp_field;
    rtk_classify_field_t *pField;
    rtk_classify_cfg_t *pDbCfCfg;

    pDbCfCfg = (rtk_classify_cfg_t *)kmalloc(sizeof(rtk_classify_cfg_t), GFP_KERNEL);

    gPlatformDb.cfRule[cfIdx].classifyCfg = pDbCfCfg;
    memcpy(pDbCfCfg, pClassifyCfg, sizeof(rtk_classify_cfg_t));
    pDbCfCfg->field.pFieldHead = NULL;

    pCurCfg_field = pClassifyCfg->field.pFieldHead;
    while(pCurCfg_field != NULL)
    {
        pField = kmalloc(sizeof(rtk_classify_field_t), GFP_KERNEL);
        memcpy(pField, pCurCfg_field, sizeof(rtk_classify_field_t));

        if(NULL == pDbCfCfg->field.pFieldHead)
        {
            pDbCfCfg->field.pFieldHead = pField;
            pDbCfCfg->field.pFieldHead->next = NULL;
        }
        else
        {
            pTmp_field = pDbCfCfg->field.pFieldHead;
            while(pTmp_field->next != NULL)
                pTmp_field = pTmp_field->next;

            pTmp_field->next = pField;
            pField->next = NULL;
        }

        pCurCfg_field = pCurCfg_field->next;
    }
}


static void _delCfCfgFromDb(unsigned int cfIdx)
{
    rtk_classify_cfg_t *pDbCfCfg;
    rtk_classify_field_t *pFieldNext, *pFieldThis;

    pDbCfCfg = (rtk_classify_cfg_t *)gPlatformDb.cfRule[cfIdx].classifyCfg;

    if(pDbCfCfg == NULL)
        return;

    pFieldThis = pDbCfCfg->field.pFieldHead;

    while(pFieldThis != NULL)
    {
        pFieldNext = pFieldThis->next;
        kfree(pFieldThis);
        pFieldThis = pFieldNext;
    }
    pDbCfCfg->field.pFieldHead = NULL;

}


/*
CF entry maintain
*/
static int _AssignNonUsedCfIndex(unsigned int cfType, omci_rule_pri_t rule_pri, unsigned int *pIndex)
{

#if 1
    int i,j,start,stop;
    int cfRuleIdx = 0, found = 0, needMove = 0;

    switch(cfType){
    case PF_CF_TYPE_L2_COMM:
        start = gPlatformDb.l2CommStart;
        stop  = gPlatformDb.l2CommStop;
    break;
    case PF_CF_TYPE_L2_ETH_FILTER:
        start = gPlatformDb.ethTypeFilterStart;
        stop  = gPlatformDb.ethTypeFilterStop;
    break;
    case PF_CF_TYPE_VEIP_FAST:
        start = gPlatformDb.veipFastStart;
        stop  = gPlatformDb.veipFastStop;
    break;
    default:
        return OMCI_ERR_FAILED;
    break;
    }

    if (OMCI_VLAN_RULE_HIGH_PRI == rule_pri.rule_pri)
    {
        /* High priority rule */
        for(i=start;i<=stop;i++)
        {
            if(found)
                break;
            /*Find valid index and insert according to the order of rule_level*/
            if((gPlatformDb.cfRule[i].isCfg == 1) && (gPlatformDb.cfRule[i].rulePri.rule_level < rule_pri.rule_level))
            {
                found = 1;
                needMove = 1;
                cfRuleIdx = i;
                break;
            }
            else if(gPlatformDb.cfRule[i].isCfg == 0)
            {
                cfRuleIdx = i;
                for(j=i+1; j<=stop; j++)
                {
                    if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_pri < rule_pri.rule_pri))
                    {
                        found = 1;
                        break;
                    }
                    else if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_level <= rule_pri.rule_level))
                    {
                        found = 1;
                        break;
                    }
                    else if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_level > rule_pri.rule_level))
                    {
                        found = 0;
                        break;
                    }
                    else if(j==stop)
                    {
                        found = 1;
                    }
                }
            }

        }

        /*Move entries behind cfRuleIdx entry*/
        if(needMove)
        {
            pf_cf_rule_t tmpCfRule[gPlatformDb.cfNum];
            rtk_classify_cfg_t *dbCfCfg;
            unsigned int numNeedMove = 0;

            memcpy(&tmpCfRule, gPlatformDb.cfRule, sizeof(pf_cf_rule_t)*(gPlatformDb.cfNum));

            /*check how many entry need to move*/
            for(i = cfRuleIdx; i <= stop; i++)
            {
                if(1 == tmpCfRule[i].isCfg)
                {
                    numNeedMove++;
                } else {
                    break;
                }
            }
            /*Move cf entries and modify l2 mb serv DB from last entry, to avoid modify new cf index of DB*/
            for(i = (cfRuleIdx + (numNeedMove - 1)); i >= cfRuleIdx; i--)
            {
                memcpy(&gPlatformDb.cfRule[i+1], &tmpCfRule[i], sizeof(pf_cf_rule_t));
                dbCfCfg = (rtk_classify_cfg_t *)gPlatformDb.cfRule[i+1].classifyCfg;
                if(dbCfCfg!=NULL)
                {
                    dbCfCfg->index = i+1;
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_cfgEntry_add(dbCfCfg));
                    l2_mb_serv_entry_cfIdx_replace(i, i+1);
                }
            }
        }
        /*save rule pri to gPlatformDb*/
        gPlatformDb.cfRule[cfRuleIdx].isCfg = 1;
        gPlatformDb.cfRule[cfRuleIdx].rulePri.rule_pri = rule_pri.rule_pri;
        gPlatformDb.cfRule[cfRuleIdx].rulePri.rule_level = rule_pri.rule_level;
        *pIndex = cfRuleIdx;
        return OMCI_ERR_OK;

    }
    else
    {
        /* Low priority rule */
        for (i = stop; i >= start; i--)
        {
            if(found)
                break;
            /*Find valid index and insert according to the order of rule_level*/
            if((gPlatformDb.cfRule[i].isCfg == 1) && (gPlatformDb.cfRule[i].rulePri.rule_level > rule_pri.rule_level))
            {
                found = 1;
                needMove = 1;
                cfRuleIdx = i;
                break;
            }
            else if(gPlatformDb.cfRule[i].isCfg == 0)
            {
                cfRuleIdx = i;
                for(j=i-1; j>=start; j--)
                {
                    if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_pri > rule_pri.rule_pri))
                    {
                        found = 1;
                        break;
                    }
                    else if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_level >= rule_pri.rule_level))
                    {
                        found = 1;
                        break;
                    }
                    else if((gPlatformDb.cfRule[j].isCfg == 1) && (gPlatformDb.cfRule[j].rulePri.rule_level < rule_pri.rule_level))
                    {
                        found = 0;
                        break;
                    }
                    else if(j==start)
                    {
                        found = 1;
                    }
                }
            }

        }

        /*Move entries behind cfRuleIdx entry*/
        if(needMove)
        {
            pf_cf_rule_t tmpCfRule[gPlatformDb.cfNum];
            rtk_classify_cfg_t *dbCfCfg;
            unsigned int numNeedMove = 0;

            memcpy(&tmpCfRule, gPlatformDb.cfRule, sizeof(pf_cf_rule_t)*(gPlatformDb.cfNum));

            /*check how many entry need to move*/
            for(i = cfRuleIdx; i >= start; i--)
            {
                if(1 == tmpCfRule[i].isCfg)
                {
                    numNeedMove++;
                } else {
                    break;
                }
            }
            /*Move cf entries and modify l2 mb serv DB from last entry, to avoid modify new cf index of DB*/
            for(i = (cfRuleIdx - (numNeedMove - 1)); i <= cfRuleIdx; i++)
            {
                memcpy(&gPlatformDb.cfRule[i-1], &tmpCfRule[i], sizeof(pf_cf_rule_t));
                dbCfCfg = (rtk_classify_cfg_t *)gPlatformDb.cfRule[i-1].classifyCfg;
                if(dbCfCfg!=NULL)
                {
                    dbCfCfg->index = i-1;
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_cfgEntry_add(dbCfCfg));
                    l2_mb_serv_entry_cfIdx_replace(i, i-1);
                }
            }
        }
        /*save rule pri to gPlatformDb*/
        gPlatformDb.cfRule[cfRuleIdx].isCfg = 1;
        gPlatformDb.cfRule[cfRuleIdx].rulePri.rule_pri = rule_pri.rule_pri;
        gPlatformDb.cfRule[cfRuleIdx].rulePri.rule_level = rule_pri.rule_level;
        *pIndex = cfRuleIdx;
        return OMCI_ERR_OK;
    }
    return OMCI_ERR_FAILED;
#else /* for FPGA */
    int i,j;
    int start[]={0,64,128,192};
    int stop[]={(0+7),(64+7),(128+7),(192+7)};
    for(j=0;j<4;j++)
    {
        for(i=start[j];i<=stop[j];i++)
        {
            if(gPlatformDb.cfRule[i].isCfg==0)
            {
                gPlatformDb.cfRule[i].isCfg = 1;
                *pIndex = i;
                return OMCI_ERR_OK;
            }
        }
    }
    return OMCI_ERR_FAILED;
#endif
}

static void _RemoveUsedCfIndex(unsigned int index)
{
    if(gPlatformDb.cfRule[index].isCfg==1)
    {
        gPlatformDb.cfRule[index].isCfg = 0;

        _delCfCfgFromDb(index);
        memset(&gPlatformDb.cfRule[index].rulePri, 0x0, sizeof(omci_rule_pri_t));
    }
    return;
}



static void omci_send_to_user(rtk_gpon_omci_msg_t* omci)
{
    int ret;
    rtk_gpon_pkt_t  data;

    memset(&data,0,sizeof(rtk_gpon_pkt_t));
    data.type = RTK_GPON_MSG_OMCI;
    memcpy(&data.msg.omci,omci,sizeof(rtk_gpon_omci_msg_t));
    if((ret = pkt_redirect_kernelApp_sendPkt(PR_USER_UID_GPONOMCI,1, RTK_GPON_OMCI_MSG_LEN, (unsigned char *)&data))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s() ret:%d",__FUNCTION__,ret);
    }
    return;
}


static void omci_send_to_nic(unsigned short len,unsigned char *omci)
{
    rtk_ponmac_queue_t queue;
    rtk_ponmac_queueCfg_t queueCfg;
    /*queue assign*/
    queue.schedulerId = gPlatformDb.omccTcont;
    queue.queueId = gPlatformDb.omccQueue;
    /*queue configure assign*/
    memset(&queueCfg,0,sizeof(rtk_ponmac_queueCfg_t));
    queueCfg.cir = 0;
    queueCfg.pir = gPlatformDb.maxQueueRate;
    queueCfg.type = STRICT_PRIORITY;

    if (RT_ERR_OK != rtk_ponmac_queue_get(&queue,&queueCfg))
        return;

    if(rtk_gponapp_omci_tx((rtk_gpon_omci_msg_t *)&omci[0]) != RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s() Fail",__FUNCTION__);
    }

}


static int omcc_create(void)
{
    int ret;
    rtk_ponmac_queue_t queue;
    rtk_ponmac_queueCfg_t queueCfg;
    /*queue assign*/
    queue.schedulerId = gPlatformDb.omccTcont;
    queue.queueId = gPlatformDb.omccQueue;
    /*queue configure assign*/
    memset(&queueCfg,0,sizeof(rtk_ponmac_queueCfg_t));
    queueCfg.cir = 0;
    queueCfg.pir = gPlatformDb.maxQueueRate;
    queueCfg.type = STRICT_PRIORITY;
    /*add omcc tcont & queue*/
    if((ret = rtk_ponmac_queue_add(&queue,&queueCfg))!=RT_ERR_OK)
    {
        return ret;
    }
    /*assign strem id to tcont & queue*/
    if((ret = rtk_ponmac_flow2Queue_set(gPlatformDb.omccFlow, &queue))!=RT_ERR_OK)
    {
        return ret;
    }
    return RT_ERR_OK;
}


static int omcc_del(void)
{
    int ret;
    rtk_ponmac_queue_t queue;

    /*queue assign*/
    queue.schedulerId = gPlatformDb.omccTcont;
    queue.queueId = gPlatformDb.omccQueue;
    /*add omcc tcont & queue*/
    if((ret = rtk_ponmac_queue_del(&queue))!=RT_ERR_OK)
    {
        RT_ERR(ret,(MOD_GPON | MOD_DAL), "");
        return ret;
    }
    /*assign strem id to tcont & queue*/
    queue.queueId = 0;
    queue.schedulerId = 0;
    if((ret = rtk_ponmac_flow2Queue_set(gPlatformDb.omccFlow, &queue))!=RT_ERR_OK)
    {
        return ret;
    }
    return RT_ERR_OK;
}

static int omci_init(void)
{
    int32 ret = OMCI_ERR_OK;
    /*register omci callback function, register function for send packet to NIC to PON*/
    if((ret = pkt_redirect_kernelApp_reg(PR_KERNEL_UID_GPONOMCI,omci_send_to_nic))!=RT_ERR_OK)
    {
        RT_ERR(ret,(MOD_GPON | MOD_DAL), "");
        return ret;
    }
    /*register receive function from PON*/
    if((ret = rtk_gponapp_evtHdlOmci_reg(omci_send_to_user))!=RT_ERR_OK)
    {
        return ret;
    }

    /*create OMCI channel*/
    if((ret = omcc_create())!=RT_ERR_OK)
    {
        return ret;
    }
    return ret;
}


static int omci_exit(void)
{
    int32 ret = OMCI_ERR_OK;
    /*remove receive GPON OMCI callback for send packet to NIC*/
    if((ret = pkt_redirect_kernelApp_dereg(PR_KERNEL_UID_GPONOMCI))!=RT_ERR_OK)
    {
        return ret;
    }
    /*remove OMCI channel*/
    if((ret = omcc_del())!=RT_ERR_OK)
    {
        return ret;
    }
    return ret;
}

static void ploam_to_user(rtk_gpon_ploam_t *ploam)
{
    int ret;
    rtk_gpon_pkt_t  data;

    memset(&data,0,sizeof(rtk_gpon_pkt_t));
    data.type = RTK_GPON_MSG_PLOAM;
    memcpy(&data.msg.ploam, ploam, sizeof(rtk_gpon_ploam_t));
    if((ret = pkt_redirect_kernelApp_sendPkt(PR_USER_UID_GPONOMCI,1, RTK_GPON_OMCI_MSG_LEN, (unsigned char *)&data))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s() ret:%d",__FUNCTION__,ret);
    }
    return;

}

static int omci_ploam_callback(rtk_gpon_ploam_t *ploam)
{
    int8    onuId;

    if (RT_ERR_OK != rtk_gpon_onuId_get(&onuId))
        return GPON_PLOAM_CONTINUE;

    if (ploam->onuid != onuId)
        return GPON_PLOAM_CONTINUE;

    if (ploam->type == GPON_PLOAM_DS_ASSIGNEDALLOCID)
    {
        ploam_to_user(ploam);
        _AllocateTcont(ploam);
        return GPON_PLOAM_STOP_WITH_ACK;
    }

    if (GPON_PLOAM_DS_CONFIGPORT == ploam->type)
    {
        if (ploam->msg[0] & 0x1)
        {
            memset(gPlatformDb.gemEncrypt, 0, sizeof(unsigned char) * 512);
        }
    }

    if (GPON_PLOAM_DS_ENCRYPTPORT == ploam->type)
    {
        if (ploam->msg[0] & 0x2)
        {
            uint8   bEncrypted;
            uint16  portId;
            uint16  shiftByte;
            uint8   shiftBit;

            bEncrypted = (ploam->msg[0] & 0x1);
            portId = (ploam->msg[1]<<4) | (ploam->msg[2]>>4);
            shiftByte = portId / 8;
            shiftBit = portId % 8;

            if (bEncrypted)
                gPlatformDb.gemEncrypt[shiftByte] |= (1 << shiftBit);
            else
                gPlatformDb.gemEncrypt[shiftByte] &= ~(1 << shiftBit);
        }
    }

    return GPON_PLOAM_CONTINUE;
}



static int omci_GetSVlanRawAct(OMCI_VLAN_OPER_ts *pVlan, rtk_classify_us_csact_t *pCsAct)
{
    switch(pVlan->sTagAct.vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCsAct = CLASSIFY_US_CSACT_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCsAct = CLASSIFY_US_CSACT_DEL_STAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCsAct = CLASSIFY_US_CSACT_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        if (0x8100 == pVlan->sTagAct.assignVlan.tpid &&
            VLAN_OPER_MODE_VLANTAG_OPER == pVlan->filterMode)
        {

            if ((RTL9601B_CHIP_ID == gPlatformDb.chipId) ||
                (APOLLOMP_CHIP_ID == gPlatformDb.chipId))
            {
                *pCsAct = CLASSIFY_US_CSACT_ADD_TAG_8100;
            }
            else
            {
                *pCsAct = CLASSIFY_US_CSACT_ADD_TAG_VS_TPID2;
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_tpidEnable_set(1, ENABLED));
            }
        }
        else
        {
            //fit TR 247
            *pCsAct = CLASSIFY_US_CSACT_ADD_TAG_VS_TPID;
        }
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetCVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_classify_us_cact_t *pCAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCAct = CLASSIFY_US_CACT_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCAct = CLASSIFY_US_CACT_DEL_CTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCAct = CLASSIFY_US_CACT_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        *pCAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}



static int omci_GetVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_classify_us_vid_act_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = CLASSIFY_US_VID_ACT_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = CLASSIFY_US_VID_ACT_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        *pVidAct = CLASSIFY_US_VID_ACT_FROM_INTERNAL;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}


static int omci_GetPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_classify_us_pri_act_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = CLASSIFY_US_PRI_ACT_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = CLASSIFY_US_PRI_ACT_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}


static int omci_SetClassifyUsAct(OMCI_VLAN_OPER_ts *pVlanRule,unsigned int flowId,rtk_classify_us_act_t *pUsAct)
{
    if(pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        pUsAct->drop = ENABLED;
    }else
    {
        if(omci_GetSVlanRawAct(pVlanRule, &pUsAct->csAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetCVlanRawAct(pVlanRule->cTagAct.vlanAct,&pUsAct->cAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetVidRawAct(pVlanRule->sTagAct.vidAct,&pUsAct->csVidAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetPriRawAct(pVlanRule->sTagAct.priAct,&pUsAct->csPriAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetVidRawAct(pVlanRule->cTagAct.vidAct,&pUsAct->cVidAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetPriRawAct(pVlanRule->cTagAct.priAct,&pUsAct->cPriAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }

        pUsAct->sTagVid = pVlanRule->sTagAct.assignVlan.vid;
        pUsAct->sTagPri = pVlanRule->sTagAct.assignVlan.pri < 8 ? pVlanRule->sTagAct.assignVlan.pri:0;
        pUsAct->cTagVid = pVlanRule->cTagAct.assignVlan.vid;
        pUsAct->cTagPri = pVlanRule->cTagAct.assignVlan.pri < 8 ? pVlanRule->cTagAct.assignVlan.pri:0;
        pUsAct->sidQidAct = CLASSIFY_US_SQID_ACT_ASSIGN_SID;
        pUsAct->sidQid  = flowId;
    }
    return OMCI_ERR_OK;
}

static int omci_SetAclUsAct(OMCI_VLAN_OPER_ts *pVlanRule, unsigned int flowId, rtk_acl_ingress_entry_t *pUsAct)
{

    if (pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        // set drop action, it does not must due to nobody assgin sid. it would use default sid 126 and be dropped.
		//pUsAct->act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
		//pUsAct->act.forwardAct.act = ACL_IGR_FORWARD_DROP_ACT;
		//pUsAct->act.forwardAct.portMask.bits[0] = 0;
    }
    else
    {
        #if 0
        //
        // disable svlan action for non and transparent mode
        //
        if (VLAN_ACT_REMOVE == pVlanRule->sTagAct.vlanAct)
        {
            //delete stag: set vlan 1 entry's tag-member to none
            pUsAct->act.enableAct[ACL_IGR_SVLAN_ACT] = ENABLED;
            pUsAct->act.svlanAct.act = ACL_IGR_SVLAN_EGR_SVLAN_ACT;
            pUsAct->act.svlanAct.svid = 1;
        }
        // TBD: service port's pvid need set to vlan=1
        if (VLAN_ACT_ADD == pVlanRule->sTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->sTagAct.vlanAct)
        {
            //add/modify stag:
            pUsAct->act.enableAct[ACL_IGR_SVLAN_ACT] = ENABLED;
            pUsAct->act.svlanAct.act = ACL_IGR_SVLAN_IGR_SVLAN_ACT;
            pUsAct->act.svlanAct.svid = pVlanRule->sTagAct.assignVlan.vid;

            //
            // TBD: spri cannot support by ACL
            //
        }
        #endif
        //
        // disable cvlan action for non and transparent mode
        //
        if (VLAN_ACT_REMOVE == pVlanRule->cTagAct.vlanAct)
        {
            //delete ctag: set vlan 1 entry's tag-member to none
            pUsAct->act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
            pUsAct->act.cvlanAct.act = ACL_IGR_CVLAN_EGR_CVLAN_ACT;
            pUsAct->act.cvlanAct.cvid = 1;
        }

        if (VLAN_ACT_ADD == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->cTagAct.vlanAct)
        {
            //add/modify ctag:
            pUsAct->act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
            pUsAct->act.cvlanAct.act = ACL_IGR_CVLAN_IGR_CVLAN_ACT;
            pUsAct->act.cvlanAct.cvid = pVlanRule->cTagAct.assignVlan.vid;
            #if 0
            //
            // dscp to pbit and copy inner outer cpri doesn't support
            //
            if (PRI_ACT_ASSIGN == pVlanRule->cTagAct.priAct &&
                8 > pVlanRule->cTagAct.assignVlan.pri)
            {
                pUsAct->act.enableAct[ACL_IGR_PRI_ACT] = ENABLED;
                pUsAct->act.priAct.act = ACL_IGR_PRI_1P_REMARK_ACT;
                pUsAct->act.priAct.aclPri = pVlanRule->cTagAct.assignVlan.pri;
            }
            #endif
        }
        pUsAct->act.enableAct[ACL_IGR_INTR_ACT]= ENABLED;
		pUsAct->act.extendAct.act = ACL_IGR_EXTEND_SID_ACT;
		pUsAct->act.extendAct.index = flowId;
    }
    return OMCI_ERR_OK;
}


static int omci_GetDsSVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_classify_ds_csact_t *pCsAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCsAct = CLASSIFY_DS_CSACT_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCsAct = CLASSIFY_DS_CSACT_DEL_STAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCsAct = CLASSIFY_DS_CSACT_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        //fit TR247
        *pCsAct = CLASSIFY_DS_CSACT_ADD_TAG_VS_TPID;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsCVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_classify_ds_cact_t *pCAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCAct = CLASSIFY_DS_CACT_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCAct = CLASSIFY_DS_CACT_DEL_CTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCAct = CLASSIFY_DS_CACT_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        *pCAct = CLASSIFY_DS_CACT_ADD_CTAG_8100;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}



static int omci_GetDsVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_classify_ds_vid_act_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = CLASSIFY_DS_VID_ACT_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = CLASSIFY_DS_VID_ACT_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = CLASSIFY_DS_VID_ACT_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        *pVidAct = CLASSIFY_DS_VID_ACT_NOP;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}


static int omci_GetDsPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_classify_ds_pri_act_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = CLASSIFY_DS_PRI_ACT_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = CLASSIFY_DS_PRI_ACT_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = CLASSIFY_DS_PRI_ACT_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = CLASSIFY_DS_PRI_ACT_FROM_INTERNAL;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = CLASSIFY_DS_PRI_ACT_FROM_INTERNAL;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

int comparePriority(void *priv, struct list_head *a, struct list_head *b)
{
    unsigned int inuma, inumb;

    flow_to_ds_pq_sp_wrr_entry_t *pEntry1 = NULL, *pEntry2 = NULL;


    pEntry1 = list_entry(a, flow_to_ds_pq_sp_wrr_entry_t, list);
    pEntry2 = list_entry(b, flow_to_ds_pq_sp_wrr_entry_t, list);


    inuma = pEntry1->pri;
    inumb = pEntry2->pri;

    if (inuma == inumb) {
        return 0;
    } else if (inuma < inumb)
        return -1;

    return 1;
}


static void _debug_dump_flow_to_ds_pq_info(void)
{
    struct list_head            *pEntry       = NULL, *pEntry2 = NULL;
    struct list_head            *pTmpEntry  = NULL, *pTmpEntry2 = NULL;
    flow_to_ds_pq_info_t  *pInfoEntry = NULL;
    flow_to_ds_pq_sp_wrr_entry_t *pSpWrrEntry = NULL;
    uint32  qIdx;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqInfoHead)
    {
        pInfoEntry = list_entry(pEntry, flow_to_ds_pq_info_t, list);
        if (!list_empty(&(pInfoEntry->flow_to_ds_pq_sp_head)) && !list_empty(&(pInfoEntry->flow_to_ds_pq_wrr_head)))
        {
            printk("###########################################\n");
            printk("phyPortId=%u\n", pInfoEntry->phyPortId);
            printk("Priority(0:low;7:high):\t");
            for (qIdx = 0; qIdx < 8; qIdx++)
                printk("%u\t", pInfoEntry->queue[qIdx]);

            printk("\nRemapping CF Priority:\t0\t1\t2\t3\t4\t5\t6\t7\n");
            printk("WRR_list:\n");
            list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_wrr_head))
            {
                pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
                printk("pri=%u\n", pSpWrrEntry->pri);
            }
            printk("SP_list:\n");
            pSpWrrEntry = NULL;
            pEntry2 = NULL;
            pTmpEntry2 = NULL;
            list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_sp_head))
            {
                pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
                printk("pri=%u\n", pSpWrrEntry->pri);
            }
        }
    }
}

static void _debug_dump_tcont_info(void)
{
    unsigned int i;

    printk("\n\n");
    printk("============= used tcont ===============\n");
    printk("%8s\t%8s\t%8s\n", "TContID", "AllocID", "qIdFrom");

    for (i = 0; i < gPlatformDb.maxTcont; i++)
    {
        if (gPlatformDb.tCont[i].allocId!=0xff)
        {
            printk("%s\t%s\t%s\n", "--------", "--------", "--------");
            printk("%8u\t%8u\t%8u\n",
                    i, gPlatformDb.tCont[i].allocId, gPlatformDb.tCont[i].qIdFrom);
        }
    }
}

static flow_to_ds_pq_info_t *flow2DsPqInfo_entry_find(unsigned int portId)
{
    struct list_head            *pEntry       = NULL;
    struct list_head            *pTmpEntry  = NULL;
    flow_to_ds_pq_info_t  *pEntryData = NULL;


    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqInfoHead)
    {
       pEntryData = list_entry(pEntry, flow_to_ds_pq_info_t, list);
       if (pEntryData->phyPortId == portId)
           return pEntryData;
    }
    return NULL;
}

static void _remove_all_flow_to_ds_pq_Info(void)
{
    struct list_head            *pEntry       = NULL, *pEntry2 = NULL;
    struct list_head            *pTmpEntry  = NULL, *pTmpEntry2 = NULL;
    flow_to_ds_pq_info_t  *pInfoEntry = NULL;
    flow_to_ds_pq_sp_wrr_entry_t *pSpWrrEntry = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqInfoHead)
    {
        pInfoEntry = list_entry(pEntry, flow_to_ds_pq_info_t, list);
        list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_wrr_head))
        {
            pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
            list_del(&pSpWrrEntry->list);
            kfree(pSpWrrEntry);
        }
        INIT_LIST_HEAD(&(pInfoEntry->flow_to_ds_pq_wrr_head));

        pSpWrrEntry = NULL;
        list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_sp_head))
        {
            pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
            list_del(&pSpWrrEntry->list);
            kfree(pSpWrrEntry);
        }
        INIT_LIST_HEAD(&(pInfoEntry->flow_to_ds_pq_sp_head));

        list_del(&pInfoEntry->list);
        kfree(pInfoEntry);
    }
    INIT_LIST_HEAD(&gPlatformDb.flow2DsPqInfoHead);
}

static void flow_to_ds_pq_remap_with_sp_wrr_policy(void)
{
    struct list_head            *pEntry       = NULL, *pEntry2 = NULL;
    struct list_head            *pTmpEntry  = NULL, *pTmpEntry2 = NULL;
    struct list_head            *pHead        = NULL;
    flow2DsPq_entry_t       *pEntryData = NULL;
    flow_to_ds_pq_info_t  *pInfoEntry = NULL;
    flow_to_ds_pq_sp_wrr_entry_t *pSpWrrEntry = NULL;
    uint32 found, qIdx;

    //remote all flow2DsPqInfoHead
    _remove_all_flow_to_ds_pq_Info();

    //re-insert flow2DsPqInfoHead
    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);
        if ((pInfoEntry = flow2DsPqInfo_entry_find(pEntryData->dsQ.portId)))
        {
                if (PQ_POLICY_STRICT_PRIORITY == pEntryData->dsQ.policy)
                    pHead =  &(pInfoEntry->flow_to_ds_pq_sp_head);
                else
                    pHead = &(pInfoEntry->flow_to_ds_pq_wrr_head);

                found  = FALSE;
                list_for_each_safe(pEntry2, pTmpEntry2, pHead)
                {
                    pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
                    if ( pSpWrrEntry->pri !=  pEntryData->dsQ.priority)
                        continue;

                    found  = TRUE;

                }
                pSpWrrEntry = NULL;
                if (!found)
                {
                    //add sp wrr entry if it doesn't exist
                     pSpWrrEntry = kzalloc(sizeof(flow_to_ds_pq_sp_wrr_entry_t), GFP_KERNEL);
                    if (pSpWrrEntry)
                    {
                        pSpWrrEntry->pri = pEntryData->dsQ.priority;

                        list_add_tail(&pSpWrrEntry->list, pHead);
                    }
                }
        }
        else
        {
            //add
            pInfoEntry = kzalloc(sizeof(flow_to_ds_pq_info_t), GFP_KERNEL);
            if (pInfoEntry)
            {
                pInfoEntry->phyPortId = pEntryData->dsQ.portId;
                for (qIdx = 0; qIdx < 8; qIdx++)
                {
                    pInfoEntry->queue[qIdx] = 0xFF;
                }
                INIT_LIST_HEAD(&(pInfoEntry->flow_to_ds_pq_wrr_head));
                INIT_LIST_HEAD(&(pInfoEntry->flow_to_ds_pq_sp_head));
                list_add_tail(&pInfoEntry->list, &gPlatformDb.flow2DsPqInfoHead);

                pSpWrrEntry  = kzalloc(sizeof(flow_to_ds_pq_sp_wrr_entry_t), GFP_KERNEL);
                if (pSpWrrEntry)
                {
                    pSpWrrEntry->pri = pEntryData->dsQ.priority;

                    if (PQ_POLICY_STRICT_PRIORITY == pEntryData->dsQ.policy)
                        pHead =  &(pInfoEntry->flow_to_ds_pq_sp_head);
                    else
                        pHead =  &(pInfoEntry->flow_to_ds_pq_wrr_head);

                    list_add_tail(&pSpWrrEntry->list, pHead);
                }
            }
        }
    }

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqInfoHead)
    {
        pInfoEntry = list_entry(pEntry, flow_to_ds_pq_info_t, list);
        if (!list_empty(&(pInfoEntry->flow_to_ds_pq_sp_head)) && !list_empty(&(pInfoEntry->flow_to_ds_pq_wrr_head)))
        {
            //sorting wrr linked list
            _omci_list_sort(NULL, &(pInfoEntry->flow_to_ds_pq_wrr_head), &comparePriority);
            //sorting sp linked list
            _omci_list_sort(NULL, &(pInfoEntry->flow_to_ds_pq_sp_head), &comparePriority);

            //put wrr entry in Q0 to Q7 buckest first and put sp entry in Q0 to Q7
            list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_wrr_head))
            {
                pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
                for (qIdx = 0; qIdx < 8; qIdx++)
                {
                    if (0xFF == pInfoEntry->queue[qIdx])
                    {
                        pInfoEntry->queue[qIdx] = pSpWrrEntry->pri;
                        break;
                    }
                }
            }

            //put sp entry in Q0 to Q7 buckest first and put sp entry in Q0 to Q7
            list_for_each_safe(pEntry2, pTmpEntry2, &(pInfoEntry->flow_to_ds_pq_sp_head))
            {
                pSpWrrEntry = list_entry(pEntry2, flow_to_ds_pq_sp_wrr_entry_t, list);
                for (qIdx = 0; qIdx < 8; qIdx++)
                {
                    if (0xFF == pInfoEntry->queue[qIdx])
                    {
                        pInfoEntry->queue[qIdx] = pSpWrrEntry->pri;
                        break;
                    }
                }
            }
        }
    }
}

static uint32 flow_to_ds_pq_get_cf_pri_by_port_priority(unsigned int portId, unsigned int pri, uint8 *pCfPri)
{
    struct list_head            *pEntry       = NULL;
    struct list_head            *pTmpEntry  = NULL;
    flow_to_ds_pq_info_t  *pInfoEntry = NULL;
    uint8                          qIdx;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqInfoHead)
    {
        pInfoEntry = list_entry(pEntry, flow_to_ds_pq_info_t, list);
        if (portId != pInfoEntry->phyPortId)
            continue;

        for (qIdx = 0; qIdx < 8; qIdx++)
        {
            if (pri == pInfoEntry->queue[qIdx])
            {
                *pCfPri = qIdx;
                return OMCI_ERR_OK;
            }
        }
    }
    return OMCI_ERR_FAILED;
}

static int omci_SetClassifyDsCfPriAct(OMCI_BRIDGE_RULE_ts *pRule, rtk_classify_ds_act_t *pDsAct)
{
    flow2DsPq_entry_t   *pEntry = NULL;
    uint8 remap_cf_pri = 0xFF;

    if (!pRule || !pDsAct)
        return OMCI_ERR_FAILED;

    pEntry = flow2DsPq_entry_find(pRule->dsFlowId);

    if (!pEntry)
        return OMCI_ERR_OK;

    if (0xFFFF == pEntry->dsQ.dsPqOmciPri)
        return OMCI_ERR_OK;

    if (PQ_DROP_COLOUR_NO_MARKING != pEntry->dsQ.dpMarking || g_isDsDeiDpAclCfg)
        return OMCI_ERR_OK;

    if ((1 << pEntry->dsQ.portId) &  pRule->uniMask)
    {
        if (OMCI_ERR_OK == flow_to_ds_pq_get_cf_pri_by_port_priority( pEntry->dsQ.portId, pEntry->dsQ.priority, &remap_cf_pri))
        {
            pDsAct->interPriAct = CLASSIFY_CF_PRI_ACT_ASSIGN;
            pDsAct->cfPri = remap_cf_pri;
        }
    }

    return OMCI_ERR_OK;
}

static int omci_SetClassifyDsAct(OMCI_VLAN_OPER_ts *pVlanRule,unsigned int uniMask,rtk_classify_ds_act_t *pDsAct)
{
    if(pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        pDsAct->uniAct  = CLASSIFY_DS_UNI_ACT_FORCE_FORWARD;
        pDsAct->uniMask.bits[0] = 1 << 5; /*not useful*/
    }else
    {
        if(omci_GetDsSVlanRawAct(pVlanRule->sTagAct.vlanAct,&pDsAct->csAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsCVlanRawAct(pVlanRule->cTagAct.vlanAct,&pDsAct->cAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsVidRawAct(pVlanRule->sTagAct.vidAct,&pDsAct->csVidAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsPriRawAct(pVlanRule->sTagAct.priAct,&pDsAct->csPriAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsVidRawAct(pVlanRule->cTagAct.vidAct,&pDsAct->cVidAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsPriRawAct(pVlanRule->cTagAct.priAct,&pDsAct->cPriAct) ==-1)
        {
            return OMCI_ERR_FAILED;
        }

        pDsAct->sTagVid = pVlanRule->sTagAct.assignVlan.vid;
        pDsAct->sTagPri = pVlanRule->sTagAct.assignVlan.pri < 8 ? pVlanRule->sTagAct.assignVlan.pri : 0;;
        pDsAct->cTagVid = pVlanRule->cTagAct.assignVlan.vid;
        pDsAct->cTagPri = pVlanRule->cTagAct.assignVlan.pri < 8 ? pVlanRule->cTagAct.assignVlan.pri : 0;
        pDsAct->uniAct  = CLASSIFY_DS_UNI_ACT_MASK_BY_UNIMASK;
        pDsAct->uniMask.bits[0] = uniMask;

        if(OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000100_DS, pDsAct, &gPlatformDb))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d fail \n", __FUNCTION__, __LINE__);
    }
    return OMCI_ERR_OK;
}

static int omci_SetAclDsAct(OMCI_VLAN_OPER_ts *pVlanRule, unsigned int uniMask, rtk_acl_ingress_entry_t *pDsAct)
{
    // no care copy inner / outer  and spri cannot support by ACL
    if (pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        // set drop action
		pDsAct->act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
		pDsAct->act.forwardAct.act = ACL_IGR_FORWARD_DROP_ACT;
		pDsAct->act.forwardAct.portMask.bits[0] = 0;
    }
    else
    {
        //
        // disable svlan action for non and transparent mode
        //
        if (VLAN_ACT_REMOVE == pVlanRule->sTagAct.vlanAct)
        {
            //delete stag: set vlan 1 entry's tag-member to none
            pDsAct->act.enableAct[ACL_IGR_SVLAN_ACT] = ENABLED;
            pDsAct->act.svlanAct.act = ACL_IGR_SVLAN_EGR_SVLAN_ACT;
            pDsAct->act.svlanAct.svid = 1;
        }
        // TBD: service port's pvid need set to vlan=1
        if (VLAN_ACT_ADD == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->cTagAct.vlanAct)
        {
            //add/modify stag:
            pDsAct->act.enableAct[ACL_IGR_SVLAN_ACT] = ENABLED;
            pDsAct->act.svlanAct.act = ACL_IGR_SVLAN_IGR_SVLAN_ACT;
            pDsAct->act.svlanAct.svid = pVlanRule->sTagAct.assignVlan.vid;

            //
            // TBD: spri cannot support by ACL
            //
        }

        //
        // disable cvlan action for non and transparent mode
        //
        if (VLAN_ACT_REMOVE == pVlanRule->cTagAct.vlanAct)
        {
            //delete ctag: set vlan 1 entry's tag-member to none
            pDsAct->act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
            pDsAct->act.cvlanAct.act = ACL_IGR_CVLAN_EGR_CVLAN_ACT;
            pDsAct->act.cvlanAct.cvid = 1;
        }

        if (VLAN_ACT_ADD == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->cTagAct.vlanAct)
        {
            //add/modify ctag:
            pDsAct->act.enableAct[ACL_IGR_CVLAN_ACT] = ENABLED;
            pDsAct->act.svlanAct.act = ACL_IGR_CVLAN_IGR_CVLAN_ACT;
            pDsAct->act.svlanAct.svid = pVlanRule->cTagAct.assignVlan.vid;

            //
            // dscp to pbit and copy inner outer cpri doesn't support
            //
            if (PRI_ACT_ASSIGN == pVlanRule->cTagAct.priAct &&
                8 > pVlanRule->cTagAct.assignVlan.pri)
            {
                pDsAct->act.enableAct[ACL_IGR_PRI_ACT] = ENABLED;
                pDsAct->act.priAct.act = ACL_IGR_PRI_1P_REMARK_ACT;
                pDsAct->act.priAct.aclPri = pVlanRule->cTagAct.assignVlan.pri;
            }
        }
		pDsAct->act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
		pDsAct->act.forwardAct.act = ACL_IGR_FORWARD_EGRESSMASK_ACT;
		pDsAct->act.forwardAct.portMask.bits[0] = uniMask;

    }
    return OMCI_ERR_OK;
}


static int omci_SetExtValnClassifyRule(OMCI_VLAN_FILTER_ts *pVlanFilter,rtk_classify_cfg_t *pClassifyCfg, omci_rule_pri_t *pRulePri)
{
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d extVlan: sM=%u svid=%u, spri=%u, cM=%u, cvid=%u, cpri=%u, ether=%u\n",
        __FUNCTION__, __LINE__,
        pVlanFilter->filterStagMode,
        pVlanFilter->filterSTag.vid,
        pVlanFilter->filterSTag.pri,
        pVlanFilter->filterCtagMode,
        pVlanFilter->filterCTag.vid,
        pVlanFilter->filterCTag.pri,
        pVlanFilter->etherType);

    /*Start to handle ExtVlan filter rule, Stag filter*/
    if (pVlanFilter->filterStagMode & VLAN_FILTER_NO_TAG)
    {
        isStag.classify_pattern.isStag.value = 0;
        isStag.classify_pattern.isStag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isStag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }
    else if (pVlanFilter->filterStagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        isStag.classify_pattern.isStag.value = 0;
        isStag.classify_pattern.isStag.mask  = 0;
        rtk_classify_field_add(pClassifyCfg,&isStag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 0;
    }
    else
    {
        isStag.classify_pattern.isStag.value = 1;
        isStag.classify_pattern.isStag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isStag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_VID)
    {
        tagVid.classify_pattern.tagVid.value = pVlanFilter->filterSTag.vid;
        tagVid.classify_pattern.tagVid.mask = 0xfff;
        rtk_classify_field_add(pClassifyCfg,&tagVid);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_PRI)
    {
        tagPri.classify_pattern.tagPri.value = pVlanFilter->filterSTag.pri;
        tagPri.classify_pattern.tagPri.mask = 0x7;
        rtk_classify_field_add(pClassifyCfg,&tagPri);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_TCI)
    {
        tagVid.classify_pattern.tagVid.value = pVlanFilter->filterSTag.vid;
        tagVid.classify_pattern.tagVid.mask = 0xfff;
        rtk_classify_field_add(pClassifyCfg,&tagVid);
        tagPri.classify_pattern.tagPri.value = pVlanFilter->filterSTag.pri;
        tagPri.classify_pattern.tagPri.mask = 0x7;
        rtk_classify_field_add(pClassifyCfg,&tagPri);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 2;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
    {
        interPri.classify_pattern.interPri.value = pVlanFilter->filterSTag.pri;
        interPri.classify_pattern.interPri.mask = 0x7;
        rtk_classify_field_add(pClassifyCfg,&interPri);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    /*Ctag filter*/
    if(pVlanFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
    {
        isCtag.classify_pattern.isCtag.value = 0;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }
    else if (pVlanFilter->filterCtagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        isCtag.classify_pattern.isCtag.value = 0;
        isCtag.classify_pattern.isCtag.mask  = 0;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 0;
    }
    else
    {
        isCtag.classify_pattern.isCtag.value = 1;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_VID)
    {
        if((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanFilter->filterStagMode)
        {
            /*ApolloMP not support inner vlan filter*/
            if(APOLLOMP_CHIP_ID != gPlatformDb.chipId)
            {
                if (pVlanFilter->filterCtagMode & VLAN_FILTER_PRI)
                {
                    InnerVlan.classify_pattern.innerVlan.value = ((pVlanFilter->filterCTag.pri << 13) | pVlanFilter->filterCTag.vid);
                    InnerVlan.classify_pattern.innerVlan.mask = 0xffff;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;
                    else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 2;
                }
                else
                {
                    InnerVlan.classify_pattern.innerVlan.value = pVlanFilter->filterCTag.vid;
                    InnerVlan.classify_pattern.innerVlan.mask = 0x0fff;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;
                    else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 2;
                }
                rtk_classify_field_add(pClassifyCfg,&InnerVlan);
            }
        }
        else
        {
            tagVid.classify_pattern.tagVid.value = pVlanFilter->filterCTag.vid;
            tagVid.classify_pattern.tagVid.mask = 0xfff;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
            rtk_classify_field_add(pClassifyCfg,&tagVid);
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_PRI)
    {
        if((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanFilter->filterStagMode)
        {
            /*ApolloMP not support inner vlan filter*/
            if(APOLLOMP_CHIP_ID != gPlatformDb.chipId)
            {
                if(pVlanFilter->filterCtagMode & VLAN_FILTER_VID)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d only filter inner tag priority \n", __FUNCTION__, __LINE__);
                }
                else
                {
                    InnerVlan.classify_pattern.innerVlan.value = pVlanFilter->filterCTag.pri << 13;
                    InnerVlan.classify_pattern.innerVlan.mask = 0xf000;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;
                    else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 2;

                    rtk_classify_field_add(pClassifyCfg,&InnerVlan);
                }
            }
        }
        else
        {
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask = 0x7;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_TCI)
    {
        if((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanFilter->filterStagMode)
        {
            /*ApolloMP not support inner vlan filter*/
            if(APOLLOMP_CHIP_ID != gPlatformDb.chipId)
            {
                InnerVlan.classify_pattern.innerVlan.value = ((pVlanFilter->filterCTag.pri << 13) | pVlanFilter->filterCTag.vid);
                InnerVlan.classify_pattern.innerVlan.mask = 0xffff;
                pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                pRulePri->rule_level += 1;

                if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                    pClassifyCfg->templateIdx = 1;
                else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
                    pClassifyCfg->templateIdx = 2;

                rtk_classify_field_add(pClassifyCfg,&InnerVlan);
            }
        }
        else
        {
            tagVid.classify_pattern.tagVid.value = pVlanFilter->filterCTag.vid;
            tagVid.classify_pattern.tagVid.mask = 0xfff;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
            rtk_classify_field_add(pClassifyCfg,&tagVid);
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask = 0x7;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        if(ETHTYPE_FILTER_NO_CARE != pVlanFilter->etherType)
        {
            switch( pVlanFilter->etherType){
            case ETHTYPE_FILTER_IP:
            etherType.classify_pattern.etherType.value = 0x0800;
            break;
            case ETHTYPE_FILTER_PPPOE:
            etherType.classify_pattern.etherType.value = 0x8863;
            break;
            case ETHTYPE_FILTER_ARP:
            etherType.classify_pattern.etherType.value = 0x0806;
            break;
            case ETHTYPE_FILTER_PPPOE_S:
            etherType.classify_pattern.etherType.value = 0x8864;
            break;
            case ETHTYPE_FILTER_IPV6:
            etherType.classify_pattern.etherType.value = 0x86dd;
            break;
            default:
            break;
            }
            etherType.classify_pattern.etherType.mask = 0xffff;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
            if(pClassifyCfg->templateIdx == 1)
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"it should not set inner vlan and etherType filter at same time.\n");
            rtk_classify_field_add(pClassifyCfg,&etherType);
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI)
    {
        interPri.classify_pattern.interPri.value = pVlanFilter->filterCTag.pri;
        interPri.classify_pattern.interPri.mask = 0x7;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
        rtk_classify_field_add(pClassifyCfg,&interPri);
    }

    return OMCI_ERR_OK;
}

static int omci_SetExtValnAclRule(OMCI_VLAN_FILTER_ts *pVlanFilter, rtk_acl_ingress_entry_t *pAclRule)
{

    //memset(&aclField_sTag, 0, sizeof(aclField_sTag));
    memset(&aclField_cTag, 0, sizeof(aclField_cTag));
    //memset(&aclField_etype, 0, sizeof(aclField_etype));

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d extVlan: sM=%u svid=%u, spri=%u, cM=%u, cvid=%u, cpri=%u, ether=%u\n",
        __FUNCTION__, __LINE__,
        pVlanFilter->filterStagMode,
        pVlanFilter->filterSTag.vid,
        pVlanFilter->filterSTag.pri,
        pVlanFilter->filterCtagMode,
        pVlanFilter->filterCTag.vid,
        pVlanFilter->filterCTag.pri,
        pVlanFilter->etherType);
#if 0

    /*Start to handle ExtVlan filter rule, Stag filter*/
    if (pVlanFilter->filterStagMode & VLAN_FILTER_NO_TAG)
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
    }
    else if (pVlanFilter->filterStagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = DISABLED;
    }
    else
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = ENABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
    }


    if (pVlanFilter->filterStagMode & VLAN_FILTER_TCI)
    {
        aclField_sTag.fieldType = ACL_FIELD_STAG;
        aclField_sTag.fieldUnion.l2tag.vid.value = pVlanFilter->filterSTag.vid;
        aclField_sTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
        aclField_sTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterSTag.pri;
        aclField_sTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
        aclField_sTag.next = NULL;
        if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_sTag))
            return OMCI_ERR_FAILED;
    }
    else
    {
        if (pVlanFilter->filterStagMode & VLAN_FILTER_VID)
        {
            aclField_sTag.fieldType = ACL_FIELD_STAG;
            aclField_sTag.fieldUnion.l2tag.vid.value = pVlanFilter->filterSTag.vid;
            aclField_sTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
            if (pVlanFilter->filterStagMode & VLAN_FILTER_PRI)
            {
                aclField_sTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterSTag.pri;
                aclField_sTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
            }
            aclField_sTag.next = NULL;
            if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_sTag))
                return OMCI_ERR_FAILED;
        }
        else
        {
            if (pVlanFilter->filterStagMode & VLAN_FILTER_PRI)
            {
                aclField_sTag.fieldType = ACL_FIELD_STAG;
                aclField_sTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterSTag.pri;
                aclField_sTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
                aclField_sTag.next = NULL;
                if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_sTag))
                    return OMCI_ERR_FAILED;
            }
        }
    }

    if (pVlanFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
    {
        // TBD: filter internal priority
    }
#endif
    /*Ctag filter*/
    if(pVlanFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
    }
    else if (pVlanFilter->filterCtagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = DISABLED;
    }
    else
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = ENABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
    }

    if (pVlanFilter->filterCtagMode & VLAN_FILTER_TCI)
    {
        aclField_cTag.fieldType = ACL_FIELD_CTAG;
        aclField_cTag.fieldUnion.l2tag.vid.value = pVlanFilter->filterCTag.vid;
        aclField_cTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
        aclField_cTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterCTag.pri;
        aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
        aclField_cTag.next = NULL;
        if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
            return OMCI_ERR_FAILED;
    }
    else
    {
        if (pVlanFilter->filterCtagMode & VLAN_FILTER_VID)
        {
            aclField_cTag.fieldType = ACL_FIELD_CTAG;
            aclField_cTag.fieldUnion.l2tag.vid.value = pVlanFilter->filterCTag.vid;
            aclField_cTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
            if (pVlanFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                aclField_cTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterCTag.pri;
                aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
            }
            aclField_cTag.next = NULL;
            if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
                return OMCI_ERR_FAILED;

        }
        else
        {
            if (pVlanFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                aclField_cTag.fieldType = ACL_FIELD_CTAG;
                aclField_cTag.fieldUnion.l2tag.pri.value = pVlanFilter->filterCTag.pri;
                aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
                aclField_cTag.next = NULL;
                if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
                    return OMCI_ERR_FAILED;
            }
        }
    }
#if 0
    if (pVlanFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        aclField_etype.fieldType = ACL_FIELD_FRAME_TYPE_TAGS;
        if (ETHTYPE_FILTER_NO_CARE != pVlanFilter->etherType)
        {
            switch( pVlanFilter->etherType)
            {
                case ETHTYPE_FILTER_IP:
                    aclField_etype.fieldUnion.data.value = 0x20;
                    aclField_etype.fieldUnion.data.mask = 0x2020;
                    break;
                case ETHTYPE_FILTER_PPPOE:
                case ETHTYPE_FILTER_PPPOE_S:
                    aclField_etype.fieldUnion.data.value = 0x4;
                    aclField_etype.fieldUnion.data.mask = 0xffff;
                    break;
                case ETHTYPE_FILTER_ARP:
                    aclField_etype.fieldUnion.data.value = 0x1000;
                    aclField_etype.fieldUnion.data.mask = 0xffff;
                    break;
                case ETHTYPE_FILTER_IPV6:
                    aclField_etype.fieldUnion.data.value = 0x40;
                    aclField_etype.fieldUnion.data.mask = 0xC040;
                    break;
                default:
                    break;
            }

            aclField_etype.next = NULL;
            if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_etype))
                return OMCI_ERR_FAILED;
        }
    }

    if (pVlanFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI)
    {
        // TBD: filter internal priority
    }
#endif

    return OMCI_ERR_OK;
}

static int omci_SetClassifyUsRule(
    OMCI_VLAN_OPER_ts *pVlanRule, unsigned int srcUni, rtk_classify_cfg_t *pClassifyCfg, omci_rule_pri_t *pRulePri, unsigned int aclLatchIdx)
{

    OMCI_VLAN_FILTER_ts *pVlanFilter = &pVlanRule->filterRule;

    memset(&isCtag,0,sizeof(rtk_classify_field_t));
    memset(&isStag,0,sizeof(rtk_classify_field_t));
    memset(&etherType,0,sizeof(rtk_classify_field_t));
    memset(&uni,0,sizeof(rtk_classify_field_t));
    memset(&tagVid,0,sizeof(rtk_classify_field_t));
    memset(&tagPri,0,sizeof(rtk_classify_field_t));
    memset(&dei,0,sizeof(rtk_classify_field_t));
    memset(&tosGemId,0,sizeof(rtk_classify_field_t));
    memset(&interPri,0,sizeof(rtk_classify_field_t));
    memset(&InnerVlan, 0, sizeof(rtk_classify_field_t));
    memset(&aclHit, 0, sizeof(rtk_classify_field_t));

    isCtag.next = NULL;
    isStag.next = NULL;
    etherType.next = NULL;
    uni.next = NULL;
    tagVid.next = NULL;
    tagPri.next = NULL;
    dei.next = NULL;
    tosGemId.next = NULL;
    interPri.next = NULL;
    InnerVlan.next = NULL;
    aclHit.next = NULL;


    uni.fieldType       = CLASSIFY_FIELD_UNI;
    dei.fieldType       = CLASSIFY_FIELD_DEI;
    tosGemId.fieldType  = CLASSIFY_FIELD_TOS_DSIDX;
    isCtag.fieldType    = CLASSIFY_FIELD_IS_CTAG;
    isStag.fieldType    = CLASSIFY_FIELD_IS_STAG;
    tagVid.fieldType    = CLASSIFY_FIELD_TAG_VID;
    tagPri.fieldType    = CLASSIFY_FIELD_TAG_PRI;
    etherType.fieldType = CLASSIFY_FIELD_ETHERTYPE;
    interPri.fieldType  = CLASSIFY_FIELD_INTER_PRI;
    InnerVlan.fieldType = CLASSIFY_FIELD_INNER_VLAN;
    aclHit.fieldType    = CLASSIFY_FIELD_ACL_HIT;

    pClassifyCfg->field.pFieldHead = NULL;

    if(srcUni != gPlatformDb.uniPortMask)
    {
        /*assigned source uni port */
        uni.classify_pattern.uni.value = srcUni;
        uni.classify_pattern.uni.mask  = 0x7;
        rtk_classify_field_add(pClassifyCfg,&uni);
    }

    if (aclLatchIdx != OMCI_UNUSED_ACL)
    {
        if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
        {
            aclHit.classify_pattern.aclHit.value = (0x80 | aclLatchIdx);
            aclHit.classify_pattern.aclHit.mask  = 0xFF;
            rtk_classify_field_add(pClassifyCfg, &aclHit);
        }
        else if(RTL9601B_CHIP_ID == gPlatformDb.chipId)
        {
            aclHit.classify_pattern.aclHit.value = (0x40 | aclLatchIdx);
            aclHit.classify_pattern.aclHit.mask  = 0x7F;
            rtk_classify_field_add(pClassifyCfg, &aclHit);
        }
    }

    switch(pVlanRule->filterMode){
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        isCtag.classify_pattern.isCtag.value = 0;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        isStag.classify_pattern.isStag.value = 0;
        isStag.classify_pattern.isStag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isStag);
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 2;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        isCtag.classify_pattern.isCtag.value = 1;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 1;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
        {
            isCtag.classify_pattern.isCtag.value = 0;
            isCtag.classify_pattern.isCtag.mask  = 1;
            rtk_classify_field_add(pClassifyCfg,&isCtag);
            isStag.classify_pattern.isStag.value = 0;
            isStag.classify_pattern.isStag.mask  = 1;
            rtk_classify_field_add(pClassifyCfg,&isStag);
            interPri.classify_pattern.interPri.value = pVlanFilter->filterCTag.pri;
            interPri.classify_pattern.interPri.mask = 0x7;
            rtk_classify_field_add(pClassifyCfg,&interPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 3;
        }
        else
        {
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask  = 0x7;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    {
        tagVid.classify_pattern.tagVid.value = pVlanFilter->filterCTag.vid;
        tagVid.classify_pattern.tagVid.mask  = 0xfff;
        rtk_classify_field_add(pClassifyCfg,&tagVid);
        pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level = 1;

        // if filterCtagMode is VID, do not care pbit */
        if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask  = 0x7;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
        }
    }
    break;
    case VLAN_OPER_MODE_FORWARD_ALL:
        /*do nothing*/
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
    break;
    case VLAN_OPER_MODE_EXTVLAN:
        omci_SetExtValnClassifyRule(pVlanFilter, pClassifyCfg, pRulePri);
    break;
    case VLAN_OPER_MODE_VLANTAG_OPER:
        if (VLAN_FILTER_VID & pVlanFilter->filterCtagMode)
        {
            tagVid.classify_pattern.tagVid.value = pVlanFilter->filterCTag.vid;
            tagVid.classify_pattern.tagVid.mask  = 0xfff;
            rtk_classify_field_add(pClassifyCfg,&tagVid);
            pRulePri->rule_level = 1;

            // if filterCtagMode is VID, do not care pbit */
            if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode ||
                VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
            {
                tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
                tagPri.classify_pattern.tagPri.mask  = 0x7;
                rtk_classify_field_add(pClassifyCfg,&tagPri);
                pRulePri->rule_level += 1;
            }
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
        }

    break;
    }
    return OMCI_ERR_OK;
}

static int omci_SetAclUsRule(
    OMCI_VLAN_OPER_ts *pVlanRule, unsigned int uniMask, rtk_acl_ingress_entry_t *pAclRule)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = &pVlanRule->filterRule;

    pAclRule->activePorts.bits[0] = uniMask;

    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_UNTAG:
        {
            pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
        }
        break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
        {
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = ENABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
        }
        break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
        {
            if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
            {
                pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
                pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
                pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
                pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
                //TBD: cannot filter internal priority
                //
                //interPri.classify_pattern.interPri.value = pVlanFilter->filterCTag.pri;
                //interPri.classify_pattern.interPri.mask = 0x7;
                //rtk_classify_field_add(pClassifyCfg,&interPri);

            }
            else if (VLAN_FILTER_PRI & pVlanFilter->filterCtagMode)
            {
                aclField_cTag.fieldType = ACL_FIELD_CTAG;
                aclField_cTag.fieldUnion.l2tag.pri.value = pVlanRule->filterRule.filterCTag.pri;
                aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
                aclField_cTag.next = NULL;
                if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
                    return OMCI_ERR_FAILED;
            }
        }
        break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
        case VLAN_OPER_MODE_VLANTAG_OPER:
        {
            aclField_cTag.fieldType = ACL_FIELD_CTAG;
            if (VLAN_FILTER_VID & pVlanFilter->filterCtagMode ||
                VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
            {
                aclField_cTag.fieldUnion.l2tag.vid.value = pVlanRule->filterRule.filterCTag.vid;
                aclField_cTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
            }

            if (VLAN_FILTER_PRI & pVlanFilter->filterCtagMode ||
                VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
            {
                aclField_cTag.fieldUnion.l2tag.pri.value = pVlanRule->filterRule.filterCTag.pri;
                aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
            }

            aclField_cTag.next = NULL;
            if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
                return OMCI_ERR_FAILED;
        }
        break;
        case VLAN_OPER_MODE_FORWARD_ALL:
            /*do nothing*/
        break;
        case VLAN_OPER_MODE_EXTVLAN:
            omci_SetExtValnAclRule(pVlanFilter, pAclRule);
        break;
    }
    return OMCI_ERR_OK;
}


static int omci_SetClassifyDsRule(
    OMCI_VLAN_OPER_ts *pDsRule, OMCI_VLAN_OPER_ts *pVlanRule, unsigned int flowId,
    rtk_classify_cfg_t *pClassifyCfg, omci_rule_pri_t *pRulePri, int aclLatchIdx)
{

    OMCI_VLAN_FILTER_ts *pVlanFilter = &pDsRule->filterRule;

    memset(&isCtag,0,sizeof(rtk_classify_field_t));
    memset(&isStag,0,sizeof(rtk_classify_field_t));
    memset(&etherType,0,sizeof(rtk_classify_field_t));
    memset(&uni,0,sizeof(rtk_classify_field_t));
    memset(&tagVid,0,sizeof(rtk_classify_field_t));
    memset(&tagPri,0,sizeof(rtk_classify_field_t));
    memset(&dei,0,sizeof(rtk_classify_field_t));
    memset(&tosGemId,0,sizeof(rtk_classify_field_t));
    memset(&interPri,0,sizeof(rtk_classify_field_t));
    memset(&InnerVlan, 0, sizeof(rtk_classify_field_t));
    memset(&aclHit, 0, sizeof(rtk_classify_field_t));

    isCtag.next = NULL;
    isStag.next = NULL;
    etherType.next = NULL;
    uni.next = NULL;
    tagVid.next = NULL;
    tagPri.next = NULL;
    dei.next = NULL;
    tosGemId.next = NULL;
    interPri.next = NULL;
    InnerVlan.next = NULL;
    aclHit.next = NULL;

    uni.fieldType       = CLASSIFY_FIELD_UNI;
    dei.fieldType       = CLASSIFY_FIELD_DEI;
    tosGemId.fieldType  = CLASSIFY_FIELD_TOS_DSIDX;
    isCtag.fieldType    = CLASSIFY_FIELD_IS_CTAG;
    isStag.fieldType    = CLASSIFY_FIELD_IS_STAG;
    tagVid.fieldType    = CLASSIFY_FIELD_TAG_VID;
    tagPri.fieldType    = CLASSIFY_FIELD_TAG_PRI;
    etherType.fieldType = CLASSIFY_FIELD_ETHERTYPE;
    interPri.fieldType  = CLASSIFY_FIELD_INTER_PRI;
    InnerVlan.fieldType = CLASSIFY_FIELD_INNER_VLAN;
    aclHit.fieldType    = CLASSIFY_FIELD_ACL_HIT;

    pClassifyCfg->field.pFieldHead = NULL;
    /*assigned source uni port */
    tosGemId.classify_pattern.tosDsidx.value = flowId;
    tosGemId.classify_pattern.tosDsidx.mask  = gPlatformDb.sidMask;
    rtk_classify_field_add(pClassifyCfg,&tosGemId);

    if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000002, pVlanFilter))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD \n");
    }

    if (aclLatchIdx != OMCI_UNUSED_ACL)
    {
        if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
        {
            aclHit.classify_pattern.aclHit.value = (0x80 | aclLatchIdx);
            aclHit.classify_pattern.aclHit.mask  = 0xFF;
            rtk_classify_field_add(pClassifyCfg, &aclHit);
        }
        else if (RTL9601B_CHIP_ID == gPlatformDb.chipId)
        {
            aclHit.classify_pattern.aclHit.value = (0x40 | aclLatchIdx);
            aclHit.classify_pattern.aclHit.mask  = 0x7F;
            rtk_classify_field_add(pClassifyCfg, &aclHit);
        }
    }

    switch(pDsRule->filterMode){
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        isCtag.classify_pattern.isCtag.value = 0;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        isStag.classify_pattern.isStag.value = 0;
        isStag.classify_pattern.isStag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isStag);
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 2;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        isCtag.classify_pattern.isCtag.value = 1;
        isCtag.classify_pattern.isCtag.mask  = 1;
        rtk_classify_field_add(pClassifyCfg,&isCtag);
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 1;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
        {
            isCtag.classify_pattern.isCtag.value = 0;
            isCtag.classify_pattern.isCtag.mask  = 1;
            rtk_classify_field_add(pClassifyCfg,&isCtag);
            isStag.classify_pattern.isStag.value = 0;
            isStag.classify_pattern.isStag.mask  = 1;
            rtk_classify_field_add(pClassifyCfg,&isStag);
            interPri.classify_pattern.interPri.value = pVlanFilter->filterCTag.pri;
            interPri.classify_pattern.interPri.mask = 0x7;
            rtk_classify_field_add(pClassifyCfg,&interPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 3;
        }
        else if (VLAN_FILTER_PRI & pVlanFilter->filterCtagMode)
        {
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask  = 0x7;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    case VLAN_OPER_MODE_VLANTAG_OPER:
    {
        if(VLAN_FILTER_VID & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            tagVid.classify_pattern.tagVid.value = pVlanFilter->filterCTag.vid;
            tagVid.classify_pattern.tagVid.mask  = 0xfff;
            rtk_classify_field_add(pClassifyCfg,&tagVid);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }
        if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            tagPri.classify_pattern.tagPri.value = pVlanFilter->filterCTag.pri;
            tagPri.classify_pattern.tagPri.mask  = 0x7;
            rtk_classify_field_add(pClassifyCfg,&tagPri);
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
        }

        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "DS: sMode=%u, cMode=%u, US: sMode=%u, cMode=%u \n",
            pVlanFilter->filterStagMode, pVlanFilter->filterCtagMode,
            pVlanRule->filterRule.filterStagMode,
            pVlanRule->filterRule.filterCtagMode);
    }
    break;
    case VLAN_OPER_MODE_FORWARD_ALL:
        /*do nothing*/
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
    break;
    case VLAN_OPER_MODE_EXTVLAN:
        omci_SetExtValnClassifyRule(pVlanFilter,pClassifyCfg, pRulePri);
    break;
    }

    return OMCI_ERR_OK;
}

static int omci_SetAclDsRule(
    OMCI_VLAN_OPER_ts *pDsRule, OMCI_VLAN_OPER_ts *pVlanRule, unsigned int flowId,
    rtk_acl_ingress_entry_t *pAclRule)
{

    OMCI_VLAN_FILTER_ts *pVlanFilter = &pDsRule->filterRule;

    memset(&aclField_cTag, 0x00, sizeof(rtk_acl_field_t));
    #if 0
    memset(&aclField_gemPort, 0x00, sizeof(rtk_acl_field_t));

    aclField_gemPort.fieldType = ACL_FIELD_GEMPORT;
    aclField_gemPort.fieldUnion.data.value = flowId;
    aclField_gemPort.fieldUnion.data.mask = 0xFFFF;//gPlatformDb.sidMask;
    aclField_gemPort.next = NULL;

    if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_gemPort))
        return OMCI_ERR_FAILED;
    #endif
    switch(pDsRule->filterMode){
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = ENABLED;
        pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
        {
            pAclRule->careTag.tags[ACL_CARE_TAG_STAG].value = DISABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].value = DISABLED;
            pAclRule->careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
            //TBD: cannot filter internal priority
            //
            //interPri.classify_pattern.interPri.value = pVlanFilter->filterCTag.pri;
            //interPri.classify_pattern.interPri.mask = 0x7;
            //rtk_classify_field_add(pClassifyCfg,&interPri);

        }
        else if (VLAN_FILTER_PRI & pVlanFilter->filterCtagMode)
        {
            aclField_cTag.fieldType = ACL_FIELD_CTAG;
            aclField_cTag.fieldUnion.l2tag.pri.value = pVlanRule->filterRule.filterCTag.pri;
            aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
            aclField_cTag.next = NULL;
            if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
                return OMCI_ERR_FAILED;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    case VLAN_OPER_MODE_VLANTAG_OPER:
    {
        aclField_cTag.fieldType = ACL_FIELD_CTAG;
        if (VLAN_FILTER_VID & pVlanFilter->filterCtagMode ||
            VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            aclField_cTag.fieldUnion.l2tag.vid.value = pVlanRule->filterRule.filterCTag.vid;
            aclField_cTag.fieldUnion.l2tag.vid.mask = RTK_VLAN_ID_MAX;
        }

        if (VLAN_FILTER_PRI & pVlanFilter->filterCtagMode ||
            VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            aclField_cTag.fieldUnion.l2tag.pri.value = pVlanRule->filterRule.filterCTag.pri;
            aclField_cTag.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;
        }

        aclField_cTag.next = NULL;
        if (RT_ERR_OK != rtk_acl_igrRuleField_add(pAclRule, &aclField_cTag))
            return OMCI_ERR_FAILED;
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "DS: sMode=%u, cMode=%u, US: sMode=%u, cMode=%u \n",
            pVlanFilter->filterStagMode, pVlanFilter->filterCtagMode,
            pVlanRule->filterRule.filterStagMode,
            pVlanRule->filterRule.filterCtagMode);
    }
    break;
    case VLAN_OPER_MODE_FORWARD_ALL:
        /*do nothing*/
    break;
    case VLAN_OPER_MODE_EXTVLAN:
        omci_SetExtValnAclRule(pVlanFilter, pAclRule);
    break;
    }

    return OMCI_ERR_OK;
}


static void omci_SetDsTagActByUsFilter(OMCI_VLAN_ACT_ts *pVlanAct,OMCI_VLAN_FILTER_MODE_e filterMode,OMCI_VLAN_ts filterVlan,
    unsigned int tagNum, OMCI_VLAN_ACT_ts usAct)
{
    if(filterMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
    }

    if(filterMode & VLAN_FILTER_CARE_TAG)
    {
        pVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
    }

    if(filterMode & VLAN_FILTER_NO_TAG)
    {
        pVlanAct->vlanAct = VLAN_ACT_REMOVE;
    }

    if(filterMode & VLAN_FILTER_VID)
    {
        pVlanAct->vlanAct = VLAN_ACT_MODIFY;
        pVlanAct->vidAct =  VID_ACT_ASSIGN;
        pVlanAct->priAct =  PRI_ACT_TRANSPARENT;
        pVlanAct->assignVlan.vid = filterVlan.vid;
    }

    if(filterMode & VLAN_FILTER_PRI)
    {
        pVlanAct->vlanAct = VLAN_ACT_MODIFY;
        pVlanAct->vidAct =  VID_ACT_ASSIGN;
        pVlanAct->priAct =  PRI_ACT_ASSIGN;
        pVlanAct->assignVlan.pri = filterVlan.pri;
    }

    if(filterMode & VLAN_FILTER_TCI)
    {
        pVlanAct->vlanAct = VLAN_ACT_MODIFY;
        pVlanAct->vidAct =  VID_ACT_ASSIGN;
        pVlanAct->priAct =  PRI_ACT_ASSIGN;
        pVlanAct->assignVlan = filterVlan;
    }

    if(filterMode & VLAN_FILTER_DSCP_PRI)
    {
        pVlanAct->priAct =  PRI_ACT_ASSIGN;
        pVlanAct->assignVlan.pri = filterVlan.pri;
    }

    if (filterMode & VLAN_FILTER_VID && filterVlan.pri == OMCI_PRI_FILTER_IGNORE)
        pVlanAct->priAct = GET_ACT(PRI, tagNum, usAct.vlanAct, usAct.priAct);
    if (filterMode & VLAN_FILTER_PRI && filterVlan.vid == OMCI_VID_FILTER_IGNORE)
        pVlanAct->vidAct = GET_ACT(VID, tagNum, usAct.vlanAct, usAct.vidAct);
}

static void omci_GetDownStreamFilterRule(OMCI_VLAN_ACT_ts usTagAct, OMCI_VLAN_FILTER_MODE_e usFilterTagMode, OMCI_VLAN_ts usFilterTag, OMCI_VLAN_FILTER_MODE_e *pDsFilterTagMode, OMCI_VLAN_ts *pDsFilterTag)
{

    switch(usTagAct.vlanAct)
    {
        case VLAN_ACT_REMOVE:
        case VLAN_ACT_NON:
            *pDsFilterTagMode = VLAN_FILTER_NO_TAG;
            break;
        case VLAN_ACT_TRANSPARENT:
            *pDsFilterTagMode = usFilterTagMode;
            *pDsFilterTag = usFilterTag;
            break;
        case VLAN_ACT_ADD:
        case VLAN_ACT_MODIFY:
            if(VID_ACT_ASSIGN == usTagAct.vidAct &&
                PRI_ACT_ASSIGN == usTagAct.priAct)
            {
                *pDsFilterTagMode = VLAN_FILTER_TCI;
                *pDsFilterTag = usTagAct.assignVlan;
            }
            if(VID_ACT_ASSIGN == usTagAct.vidAct &&
                PRI_ACT_ASSIGN != usTagAct.priAct)
            {
                *pDsFilterTagMode = VLAN_FILTER_VID;
                pDsFilterTag->vid = usTagAct.assignVlan.vid;
            }
            if(VID_ACT_ASSIGN != usTagAct.vidAct &&
                PRI_ACT_ASSIGN == usTagAct.priAct)
            {
                *pDsFilterTagMode = VLAN_FILTER_PRI;
                pDsFilterTag->pri= usTagAct.assignVlan.pri;
            }
            if(VID_ACT_ASSIGN != usTagAct.vidAct &&
                PRI_ACT_ASSIGN != usTagAct.priAct)
            {
                *pDsFilterTagMode = VLAN_FILTER_CARE_TAG;
            }
            if((VID_ACT_COPY_INNER == usTagAct.vidAct &&
               PRI_ACT_COPY_INNER== usTagAct.priAct) ||
               (VID_ACT_COPY_OUTER == usTagAct.vidAct &&
               PRI_ACT_COPY_OUTER== usTagAct.priAct))
            {
                *pDsFilterTagMode = VLAN_FILTER_TCI;
                *pDsFilterTag = usTagAct.assignVlan;
            }
            break;
    }
}

static void omci_GetDownStreamFilterMode(OMCI_VLAN_OPER_MODE_t *pFilterMode, OMCI_VLAN_FILTER_MODE_e sMode,
    OMCI_VLAN_FILTER_MODE_e cMode, const OMCI_VLAN_OPER_ts *pUsRule)
{

    if (VLAN_FILTER_NO_TAG & sMode && VLAN_FILTER_NO_TAG & cMode)
    {
        *pFilterMode = ((VLAN_FILTER_DSCP_PRI & cMode) ?
                        VLAN_OPER_MODE_FILTER_INNER_PRI : VLAN_OPER_MODE_FORWARD_UNTAG);
    }
    else
    {
        *pFilterMode = pUsRule->filterMode;
        /* exist ctag but dont care which ctag */
    }

}

static void omci_SetUsRuleInvert2Ds(OMCI_VLAN_OPER_ts *pDsRule,const OMCI_VLAN_OPER_ts *pUsRule)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = (OMCI_VLAN_FILTER_ts *)&(pUsRule->filterRule);
    OMCI_VLAN_FILTER_MODE_e original;
    OMCI_VLAN_ts tmp;

    memset(pDsRule,0,sizeof(OMCI_VLAN_OPER_ts));
    //DS filter <-> action of US
    pDsRule->filterMode = VLAN_OPER_MODE_FORWARD_ALL;
    if(pUsRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        //pUsAct->drop = ENABLED;
    }
    else
    {
        omci_GetDownStreamFilterRule(pUsRule->sTagAct, pUsRule->filterRule.filterStagMode, pUsRule->filterRule.filterSTag, &(pDsRule->filterRule.filterStagMode), &(pDsRule->filterRule.filterSTag));
        omci_GetDownStreamFilterRule(pUsRule->cTagAct, pUsRule->filterRule.filterCtagMode, pUsRule->filterRule.filterCTag, &(pDsRule->filterRule.filterCtagMode), &(pDsRule->filterRule.filterCTag));

        if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_USRULE2DSRULE, pUsRule, pDsRule))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");

        omci_GetDownStreamFilterMode(&pDsRule->filterMode, pDsRule->filterRule.filterStagMode, pDsRule->filterRule.filterCtagMode, pUsRule);

        switch (pDsRule->filterMode)
        {
            case VLAN_OPER_MODE_FILTER_SINGLETAG:
            case VLAN_OPER_MODE_FILTER_INNER_PRI:
                pDsRule->filterRule.filterCTag = pUsRule->filterRule.filterCTag;
                break;
            case VLAN_OPER_MODE_EXTVLAN:
                if(VLAN_FILTER_CARE_TAG == pUsRule->filterRule.filterCtagMode &&
                    VLAN_FILTER_CARE_TAG == pUsRule->filterRule.filterStagMode)
                {
                    pDsRule->filterRule.filterCtagMode = VLAN_FILTER_CARE_TAG;
                    pDsRule->filterRule.filterStagMode = VLAN_FILTER_CARE_TAG;
                }
                if(VLAN_FILTER_CARE_TAG & pDsRule->filterRule.filterCtagMode &&
                    VLAN_FILTER_NO_TAG & pUsRule->filterRule.filterCtagMode)
                {
                    pDsRule->filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;
                }
                if(pUsRule->filterRule.etherType > ETHTYPE_FILTER_NO_CARE)
                {
                    pDsRule->filterRule.filterCtagMode  |= VLAN_FILTER_ETHTYPE;
                    pDsRule->filterRule.etherType = pUsRule->filterRule.etherType;
                }
                if(pDsRule->filterRule.filterStagMode != pUsRule->filterRule.filterStagMode &&
                    (PRI_ACT_COPY_INNER == pUsRule->sTagAct.priAct ||
                    PRI_ACT_COPY_OUTER == pUsRule->sTagAct.priAct) &&
                    (VLAN_FILTER_PRI & pUsRule->filterRule.filterStagMode))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;
                    if(VLAN_FILTER_PRI & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.pri = pUsRule->filterRule.filterSTag.pri;
                }
                else if(VLAN_FILTER_NO_TAG == pUsRule->filterRule.filterStagMode &&
                    pDsRule->filterRule.filterCtagMode != pUsRule->filterRule.filterCtagMode &&
                    (PRI_ACT_COPY_INNER == pUsRule->cTagAct.priAct ||
                    PRI_ACT_COPY_OUTER == pUsRule->cTagAct.priAct) &&
                    (VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                    if(VLAN_FILTER_PRI & (pDsRule->filterRule.filterCtagMode & (~original)))
                        pDsRule->filterRule.filterCTag.pri = pUsRule->filterRule.filterCTag.pri;
                }
                else if(VLAN_FILTER_DSCP_PRI & pUsRule->filterRule.filterStagMode &&
                        (PRI_ACT_FROM_DSCP != pUsRule->sTagAct.priAct &&
                         PRI_ACT_FROM_DSCP != pUsRule->cTagAct.priAct))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;
                    if(VLAN_FILTER_PRI & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.pri = pUsRule->filterRule.filterSTag.pri;
                }
                else if(VLAN_FILTER_DSCP_PRI & pUsRule->filterRule.filterCtagMode &&
                        (PRI_ACT_FROM_DSCP != pUsRule->cTagAct.priAct &&
                         PRI_ACT_FROM_DSCP != pUsRule->sTagAct.priAct))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                    if(VLAN_FILTER_PRI & (pDsRule->filterRule.filterCtagMode & (~original)))
                        pDsRule->filterRule.filterCTag.pri = pUsRule->filterRule.filterCTag.pri;
                }
                else if (PRI_ACT_FROM_DSCP == pUsRule->sTagAct.priAct)
                {
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_DSCP_PRI;
                    pDsRule->filterRule.filterSTag.pri = pUsRule->sTagAct.assignVlan.pri;

                    if (VLAN_FILTER_TCI & pUsRule->filterRule.filterStagMode ||
                        VLAN_FILTER_PRI & pUsRule->filterRule.filterStagMode)
                    {
                        /*Add filter Pbit*/
                        if(pDsRule->filterRule.filterStagMode & VLAN_FILTER_VID){
                            pDsRule->filterRule.filterStagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_TCI;

                        }else if((pDsRule->filterRule.filterStagMode & VLAN_FILTER_TCI)== 0x0){
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;

                        }
                        // TBD ds action prio value
                    }
                    else if (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode ||
                        VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode)
                    {
                        /*Filter Pbit*/
                        if(pDsRule->filterRule.filterStagMode & VLAN_FILTER_VID){
                            pDsRule->filterRule.filterStagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_TCI;

                        }else if((pDsRule->filterRule.filterStagMode & VLAN_FILTER_TCI)== 0x0){
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;

                        }
                    }
                }
                else if (PRI_ACT_FROM_DSCP == pUsRule->cTagAct.priAct)
                {
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_DSCP_PRI;
                    pDsRule->filterRule.filterCTag.pri = pUsRule->cTagAct.assignVlan.pri;

                    if (VLAN_FILTER_TCI & pUsRule->filterRule.filterStagMode ||
                        VLAN_FILTER_PRI & pUsRule->filterRule.filterStagMode)
                    {
                        /*Filter Pbit*/
                        if(pDsRule->filterRule.filterCtagMode & VLAN_FILTER_VID){
                            pDsRule->filterRule.filterCtagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_TCI;

                        }else if((pDsRule->filterRule.filterCtagMode & VLAN_FILTER_TCI)== 0x0){
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;

                        }
                        // TBD ds action prio value
                    }
                    else if (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode ||
                        VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode)
                    {
                        /*Filter Pbit*/
                        if(pDsRule->filterRule.filterCtagMode & VLAN_FILTER_VID){
                            pDsRule->filterRule.filterCtagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_TCI;

                        }else if((pDsRule->filterRule.filterCtagMode & VLAN_FILTER_TCI)== 0x0){
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;

                        }
                    }
                }
                /*For US VID copy action <-> DS filter*/
                if(pDsRule->filterRule.filterStagMode != pUsRule->filterRule.filterStagMode &&
                    (VID_ACT_COPY_INNER == pUsRule->sTagAct.vidAct ||
                    VID_ACT_COPY_OUTER == pUsRule->sTagAct.vidAct) &&
                    ((VLAN_FILTER_VID & pUsRule->filterRule.filterStagMode) ||
                     (VLAN_FILTER_TCI & pUsRule->filterRule.filterStagMode)))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_VID;
                    if(VLAN_FILTER_VID & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.vid = pUsRule->filterRule.filterSTag.vid;
                }
                else if(VLAN_FILTER_NO_TAG == pUsRule->filterRule.filterStagMode &&
                    pDsRule->filterRule.filterCtagMode != pUsRule->filterRule.filterCtagMode &&
                    (VID_ACT_COPY_INNER == pUsRule->cTagAct.vidAct ||
                    VID_ACT_COPY_OUTER == pUsRule->cTagAct.vidAct) &&
                    ((VLAN_FILTER_VID & pUsRule->filterRule.filterCtagMode) ||
                    (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode)))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_VID;
                    if(VLAN_FILTER_VID & (pDsRule->filterRule.filterCtagMode & (~original)))
                        pDsRule->filterRule.filterCTag.vid = pUsRule->filterRule.filterCTag.vid;
                }
                else if (VLAN_FILTER_TCI == pUsRule->filterRule.filterStagMode &&
                         VLAN_FILTER_TCI == pUsRule->filterRule.filterCtagMode &&
                         VID_ACT_COPY_INNER == pUsRule->sTagAct.vidAct &&
                         VID_ACT_COPY_OUTER == pUsRule->cTagAct.vidAct)
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterSTag = pUsRule->filterRule.filterCTag;
                    pDsRule->filterRule.filterCTag = pUsRule->filterRule.filterSTag;
                }
                //TODO: DS: S->C / US: C->S
                break;
            case VLAN_OPER_MODE_VLANTAG_OPER:
                if (0x8100 == pUsRule->outStyle.outVlan.tpid &&
                    2 == pUsRule->outStyle.outTagNum)
                {
                    // switch filter rule of stag and ctag
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode = original;

                    memcpy(&tmp, &(pDsRule->filterRule.filterSTag), sizeof(OMCI_VLAN_ts));
                    memcpy(&(pDsRule->filterRule.filterSTag), &(pDsRule->filterRule.filterCTag), sizeof(OMCI_VLAN_ts));
                    memcpy(&(pDsRule->filterRule.filterCTag), &tmp, sizeof(OMCI_VLAN_ts));
                }
                break;
            default:
                break;
        }
    }

    //DS action <-> filter of US
    switch(pUsRule->filterMode){
    case VLAN_OPER_MODE_VLANTAG_OPER:
    {
        pDsRule->cTagAct.vlanAct = pUsRule->outStyle.dsTagOperMode;
        pDsRule->sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_ALL:
    {
        pDsRule->cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
        pDsRule->sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        pDsRule->cTagAct.vlanAct = VLAN_ACT_REMOVE;
        pDsRule->sTagAct.vlanAct = VLAN_ACT_REMOVE;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        pDsRule->cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
        pDsRule->sTagAct.vlanAct = VLAN_ACT_REMOVE;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        pDsRule->sTagAct.vlanAct = VLAN_ACT_REMOVE;
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_NO_TAG | VLAN_FILTER_DSCP_PRI))
        {
            pDsRule->cTagAct.vlanAct = VLAN_ACT_REMOVE;
        }
        else
        {
            pDsRule->cTagAct.vlanAct = VLAN_ACT_MODIFY;
            if(pUsRule->outStyle.outTagNum > 1)
                pDsRule->cTagAct.vidAct = VID_ACT_COPY_INNER;
            else
                pDsRule->cTagAct.vidAct  = VID_ACT_COPY_OUTER;
            pDsRule->cTagAct.priAct  = PRI_ACT_ASSIGN;
            pDsRule->cTagAct.assignVlan.pri = pUsRule->filterRule.filterCTag.pri;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    {

        if ((VLAN_FILTER_NO_CARE_TAG & pDsRule->filterRule.filterStagMode &&
            VLAN_FILTER_NO_CARE_TAG & pDsRule->filterRule.filterCtagMode) ||
            (!memcmp(&pDsRule->filterRule.filterSTag, &pUsRule->filterRule.filterSTag, sizeof(OMCI_VLAN_ts)) &&
            !memcmp(&pDsRule->filterRule.filterCTag, &pUsRule->filterRule.filterCTag, sizeof(OMCI_VLAN_ts))))
        {
            pDsRule->cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
            pDsRule->sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
        }
        else
        {
            pDsRule->sTagAct.vlanAct = VLAN_ACT_REMOVE;
            pDsRule->cTagAct.vlanAct = VLAN_ACT_MODIFY;
            pDsRule->cTagAct.vidAct  = VID_ACT_ASSIGN;
            pDsRule->cTagAct.priAct  = PRI_ACT_ASSIGN;
            pDsRule->cTagAct.assignVlan = pUsRule->filterRule.filterCTag;
        }
    }
    break;
    case VLAN_OPER_MODE_EXTVLAN:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"s.pri=%u, c.pri=%u tagNum=%u cpAct=%u",
            pVlanFilter->filterSTag.pri, pVlanFilter->filterCTag.pri, pUsRule->outStyle.outTagNum, pUsRule->cTagAct.priAct);
        omci_SetDsTagActByUsFilter(&pDsRule->sTagAct,pVlanFilter->filterStagMode,pVlanFilter->filterSTag,
            pUsRule->outStyle.outTagNum, pUsRule->sTagAct);

        if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_USACT2DSSACT, pUsRule, pDsRule))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");

        omci_SetDsTagActByUsFilter(&pDsRule->cTagAct, pVlanFilter->filterCtagMode,pVlanFilter->filterCTag,
            pUsRule->outStyle.outTagNum, pUsRule->cTagAct);

        if (OMCIDRV_FEATURE_ERR_FAIL ==  omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_USACT2DSCACT, pDsRule))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");

        if (!memcmp(&pDsRule->filterRule.filterSTag, &pUsRule->filterRule.filterSTag, sizeof(OMCI_VLAN_ts)) &&
            !memcmp(&pDsRule->filterRule.filterCTag, &pUsRule->filterRule.filterCTag, sizeof(OMCI_VLAN_ts)))
        {
            pDsRule->cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
            pDsRule->sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
        }
    }

    break;
    }
}

static int omci_is_same_rule_by_ds_vlan_filter(
                                               PON_GEMPORT_DIRECTION dir,
                                               OMCI_VLAN_OPER_ts *pBR,
                                               OMCI_VLAN_OPER_ts *pL2)
{
    int ret = FALSE;
    OMCI_VLAN_OPER_ts old, cur;

    if (PON_GEMPORT_DIRECTION_BI != dir &&
        PON_GEMPORT_DIRECTION_DS != dir)
        return ret;

    if (PON_GEMPORT_DIRECTION_BI == dir)
    {
        memset(&old, 0, sizeof(OMCI_VLAN_OPER_ts));
        memset(&cur, 0, sizeof(OMCI_VLAN_OPER_ts));

        omci_SetUsRuleInvert2Ds(&old, pL2);
        omci_SetUsRuleInvert2Ds(&cur, pBR);
    }

    if (PON_GEMPORT_DIRECTION_DS == dir)
    {
        memcpy(&old, pL2, sizeof(OMCI_VLAN_OPER_ts));
        memcpy(&cur, pBR, sizeof(OMCI_VLAN_OPER_ts));
    }

    if (old.filterMode == cur.filterMode &&
        !(VLAN_FILTER_DSCP_PRI & cur.filterRule.filterStagMode) &&
        !(VLAN_FILTER_DSCP_PRI & cur.filterRule.filterCtagMode) &&
        0 == memcmp(&old.filterRule, &cur.filterRule, sizeof(OMCI_VLAN_FILTER_ts)))
        return TRUE;

    /*Both enable ethertype filter rule,but different ethertype value*/
    if(VLAN_FILTER_ETHTYPE & old.filterRule.filterCtagMode &&
       VLAN_FILTER_ETHTYPE & cur.filterRule.filterCtagMode &&
       old.filterRule.etherType != cur.filterRule.etherType){
      return FALSE;

    }
    /*Only one rule  enables filter ethertype */
    if(VLAN_FILTER_ETHTYPE &
        (old.filterRule.filterCtagMode ^cur.filterRule.filterCtagMode) ){
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d \n", __FUNCTION__, __LINE__);
      return FALSE;
    }


    if (old.filterMode == cur.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == cur.filterMode &&
        old.filterRule.filterStagMode == cur.filterRule.filterStagMode &&
        !(VLAN_FILTER_DSCP_PRI & cur.filterRule.filterStagMode) &&
        (0 == memcmp(&old.filterRule.filterSTag, &cur.filterRule.filterSTag, sizeof(OMCI_VLAN_ts))))
    {
        if (((VLAN_FILTER_VID | VLAN_FILTER_PRI) & old.filterRule.filterCtagMode) &&
            (VLAN_FILTER_TCI & cur.filterRule.filterCtagMode ||
             (VLAN_FILTER_VID | VLAN_FILTER_PRI) & cur.filterRule.filterCtagMode) &&
             old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid &&
             old.filterRule.filterCTag.pri == cur.filterRule.filterCTag.pri)
            return TRUE;

        if (((VLAN_FILTER_VID | VLAN_FILTER_PRI) & cur.filterRule.filterCtagMode) &&
            (VLAN_FILTER_TCI & old.filterRule.filterCtagMode ||
             (VLAN_FILTER_VID | VLAN_FILTER_PRI) & old.filterRule.filterCtagMode) &&
             old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid &&
             old.filterRule.filterCTag.pri == cur.filterRule.filterCTag.pri)
            return TRUE;

        if ((VLAN_FILTER_VID == old.filterRule.filterCtagMode) &&
            (VLAN_FILTER_VID == cur.filterRule.filterCtagMode) &&
             old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid)
            return TRUE;

        if ((VLAN_FILTER_NO_TAG != cur.filterRule.filterCtagMode) &&
            (VLAN_FILTER_CARE_TAG == old.filterRule.filterCtagMode) &&
            (VLAN_ACT_TRANSPARENT == old.sTagAct.vlanAct) &&
            (VLAN_ACT_TRANSPARENT == old.cTagAct.vlanAct))
            return TRUE;

        if ((VLAN_FILTER_NO_TAG != old.filterRule.filterCtagMode) &&
            (VLAN_FILTER_CARE_TAG == cur.filterRule.filterCtagMode) &&
            (VLAN_ACT_TRANSPARENT == cur.sTagAct.vlanAct) &&
            (VLAN_ACT_TRANSPARENT == cur.cTagAct.vlanAct))
            return TRUE;
    }

    if (VLAN_OPER_MODE_FILTER_SINGLETAG == old.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == cur.filterMode &&
        VLAN_FILTER_NO_CARE_TAG == old.filterRule.filterStagMode &&
        old.filterRule.filterStagMode != cur.filterRule.filterStagMode &&
        ((VLAN_FILTER_VID | VLAN_FILTER_PRI) & old.filterRule.filterCtagMode) &&
        (VLAN_FILTER_TCI & cur.filterRule.filterCtagMode ||
         (VLAN_FILTER_VID | VLAN_FILTER_PRI) & cur.filterRule.filterCtagMode) &&
         old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid &&
         old.filterRule.filterCTag.pri == cur.filterRule.filterCTag.pri)
        return TRUE;

    if (VLAN_OPER_MODE_FILTER_SINGLETAG == cur.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == old.filterMode &&
        VLAN_FILTER_NO_CARE_TAG == cur.filterRule.filterStagMode &&
        old.filterRule.filterStagMode != cur.filterRule.filterStagMode &&
        ((VLAN_FILTER_VID | VLAN_FILTER_PRI) & cur.filterRule.filterCtagMode) &&
        (VLAN_FILTER_TCI & old.filterRule.filterCtagMode ||
         (VLAN_FILTER_VID | VLAN_FILTER_PRI) & old.filterRule.filterCtagMode) &&
         old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid &&
         old.filterRule.filterCTag.pri == cur.filterRule.filterCTag.pri)
        return TRUE;

    if (VLAN_OPER_MODE_FILTER_SINGLETAG == old.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == cur.filterMode &&
        VLAN_FILTER_NO_CARE_TAG == old.filterRule.filterStagMode &&
        old.filterRule.filterStagMode != cur.filterRule.filterStagMode &&
        (VLAN_FILTER_VID & old.filterRule.filterCtagMode &&
         old.filterRule.filterCtagMode == cur.filterRule.filterCtagMode) &&
         old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid)
        return TRUE;

    if (VLAN_OPER_MODE_FILTER_SINGLETAG == cur.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == old.filterMode &&
        VLAN_FILTER_NO_CARE_TAG == cur.filterRule.filterStagMode &&
        old.filterRule.filterStagMode != cur.filterRule.filterStagMode &&
        (VLAN_FILTER_VID & cur.filterRule.filterCtagMode &&
         cur.filterRule.filterCtagMode == old.filterRule.filterCtagMode) &&
         old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid)
        return TRUE;

    if (VLAN_OPER_MODE_FILTER_SINGLETAG == old.filterMode &&
        VLAN_OPER_MODE_EXTVLAN == cur.filterMode &&
        VLAN_FILTER_NO_TAG == old.filterRule.filterStagMode &&
        VLAN_FILTER_NO_TAG == cur.filterRule.filterStagMode &&
        VLAN_FILTER_VID == old.filterRule.filterCtagMode &&
        VLAN_FILTER_VID == cur.filterRule.filterCtagMode &&
        old.filterRule.filterCTag.vid == cur.filterRule.filterCTag.vid)
        return TRUE;

    if (VLAN_OPER_MODE_FORWARD_ALL == old.filterMode)
        return TRUE;

    if (VLAN_OPER_MODE_FORWARD_ALL == cur.filterMode)
        return TRUE;

    return ret;
}


static int omci_dump_ds_aggregated_list(PON_GEMPORT_DIRECTION dir)
{
    dsAggregated_group_t    *pGroupEntry= NULL;
    dsAggregated_entry_t    *pEntryData = NULL;
    struct list_head        *pEntry     = NULL, *pTmpEntry  = NULL;
    struct list_head        *pCur       = NULL, *pNext      = NULL;
    struct list_head        *pHead      = NULL;
    unsigned int            i;

    if (PON_GEMPORT_DIRECTION_BI != dir &&
        PON_GEMPORT_DIRECTION_DS != dir)
        return OMCI_ERR_OK;

    if (PON_GEMPORT_DIRECTION_BI == dir)
        pHead = &gPlatformDb.ucAggregatedGroupHead;

    if (PON_GEMPORT_DIRECTION_DS == dir)
        pHead = &gPlatformDb.mbAggregatedGroupHead;

    list_for_each_safe(pCur, pNext, pHead)
    {
        pGroupEntry = list_entry(pCur, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            printk("\ndsFlowId=%u, id=%u, uniMask=%x, cfIdx=%d, ",
                   pGroupEntry->dsFlowId, pEntryData->id, pEntryData->aggrUniMask, pEntryData->cfIdx);

            if (pEntryData->outerVID == OMCI_XLATEVID_UNTAG)
                printk("outerVID=UNTAG\n");
            else
                printk("outerVID=%d\n", pEntryData->outerVID);


            printk("DS UNI port:   ");
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
                printk("%5d ", i);

            printk("\n               ");
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
                printk("----- ");

            printk("\nTranslate Mode: ");
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (pEntryData->pXlateVID[i].XlateMode == OMCI_XLATEVID_NOT_USED)
                    printk("  N/A ");
                else if (pEntryData->pXlateVID[i].XlateMode == OMCI_XLATEVID_TWO_MORE)
                    printk("  LUT ");
                else if (pEntryData->pXlateVID[i].XlateMode == OMCI_XLATEVID_ONLY_ONE)
                    printk(" SP2C ");
                else
                    printk("  ERR ");
            }

            printk("\nTranslate VID: ");
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (pEntryData->pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED)
                {
                    if (pEntryData->pXlateVID[i].vid == OMCI_XLATEVID_UNTAG)
                        printk("UNTAG ");
                    else
                        printk("%5d ", pEntryData->pXlateVID[i].vid);
                }
                else
                    printk("      ");
            }

            if (PON_GEMPORT_DIRECTION_BI == dir)
            {
                printk("\nLUT CF IDX   : ");
                for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
                {
                    if (pEntryData->pLutCfIdx[i] == OMCI_UNUSED_CF)
                        printk("      ");
                    else
                        printk("%5d ", pEntryData->pLutCfIdx[i]);
                }
            }
            printk("\n");
        }
    }
    return OMCI_ERR_OK;
}

static int omci_ds_aggregated_list_flush(void)
{
    dsAggregated_group_t    *pGroupEntry= NULL;
    dsAggregated_entry_t    *pEntryData = NULL;
    struct list_head        *pEntry     = NULL, *pTmpEntry  = NULL;
    struct list_head        *pCur       = NULL, *pNext      = NULL;

    list_for_each_safe(pCur, pNext, &gPlatformDb.ucAggregatedGroupHead)
    {
        if ((pGroupEntry = list_entry(pCur, dsAggregated_group_t, list)))
        {
            list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
            {
                if ((pEntryData = list_entry(pEntry, dsAggregated_entry_t, list)))
                {
                    list_del(&pEntryData->list);
                    kfree(pEntryData->pXlateVID);
                    kfree(pEntryData->pLutCfIdx);
                    kfree(pEntryData);
                }
            }
            INIT_LIST_HEAD(&(pGroupEntry->dsAggregatedHead));
            list_del(&pGroupEntry->list);
            kfree(pGroupEntry);
        }
    }
    INIT_LIST_HEAD(&gPlatformDb.ucAggregatedGroupHead);

    pGroupEntry= NULL;
    pEntryData = NULL;
    pEntry     = NULL, pTmpEntry  = NULL;
    pCur       = NULL, pNext      = NULL;

    list_for_each_safe(pCur, pNext, &gPlatformDb.mbAggregatedGroupHead)
    {
        if ((pGroupEntry = list_entry(pCur, dsAggregated_group_t, list)))
        {
            list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
            {
                if ((pEntryData = list_entry(pEntry, dsAggregated_entry_t, list)))
                {
                    list_del(&pEntryData->list);
                    kfree(pEntryData->pXlateVID);
                    kfree(pEntryData);
                }
            }
            INIT_LIST_HEAD(&(pGroupEntry->dsAggregatedHead));
            list_del(&pGroupEntry->list);
            kfree(pGroupEntry);
        }
    }
    INIT_LIST_HEAD(&gPlatformDb.mbAggregatedGroupHead);

    return OMCI_ERR_OK;
}

static int omci_set_ds_aggregated_list_by_srvId(unsigned char isUpdate,
                                                PON_GEMPORT_DIRECTION dir,
                                                unsigned int  dsFlowId,
                                                int srvId,
                                                unsigned int aggrUniMask,
                                                unsigned int cfIdx,
                                                unsigned int outerVID,
                                                vlanAggregatedXlateInfo_t *pXlateVID)
{
    dsAggregated_group_t    *pGroup     = NULL;
    dsAggregated_entry_t    *pEntryData = NULL;
    struct list_head        *pHead      = NULL;
    unsigned int            i;

    pGroup = omci_find_ds_aggregated_group_entry(dir, dsFlowId);

    pEntryData = omci_find_ds_aggregated_entry(pGroup, srvId);

    if (isUpdate)
    {
        // update
        if (pEntryData)
        {
            pEntryData->aggrUniMask = aggrUniMask;
            pEntryData->cfIdx = cfIdx;
            pEntryData->outerVID = outerVID;
            memcpy(pEntryData->pXlateVID, pXlateVID, sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1));
            return OMCI_ERR_OK;
        }


        if (pGroup)
        {
            // add dsAggregated_entry_t
            if (!(pEntryData = kzalloc(sizeof(dsAggregated_entry_t), GFP_KERNEL)))
            {
                return OMCI_ERR_FAILED;
            }

            if ((pEntryData->pXlateVID = (vlanAggregatedXlateInfo_t *) kzalloc(sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1), GFP_KERNEL)) == NULL)
            {
                kfree(pEntryData);
                return OMCI_ERR_FAILED;
            }

            if ((pEntryData->pLutCfIdx = (unsigned int *)kzalloc(sizeof(unsigned int) * (gPlatformDb.allPortMax + 1), GFP_KERNEL)) == NULL)
            {
                kfree(pEntryData->pXlateVID);
                kfree(pEntryData);
                return OMCI_ERR_FAILED;
            }

            pEntryData->id = srvId;
            pEntryData->aggrUniMask = aggrUniMask;
            pEntryData->cfIdx = cfIdx;
            pEntryData->outerVID = outerVID;
            memcpy(pEntryData->pXlateVID, pXlateVID, sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1));
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
                pEntryData->pLutCfIdx[i] = OMCI_UNUSED_CF;

            list_add_tail(&pEntryData->list, &(pGroup->dsAggregatedHead));
        }
        else
        {
            // alloc dsAggregated_group_t
            if (PON_GEMPORT_DIRECTION_BI != dir &&
                PON_GEMPORT_DIRECTION_DS != dir)
                return OMCI_ERR_FAILED;

            if (PON_GEMPORT_DIRECTION_BI == dir)
                pHead = &gPlatformDb.ucAggregatedGroupHead;

            if (PON_GEMPORT_DIRECTION_DS == dir)
                pHead = &gPlatformDb.mbAggregatedGroupHead;

            if (!(pGroup = kzalloc(sizeof(dsAggregated_group_t), GFP_KERNEL)))
                return OMCI_ERR_FAILED;

            pGroup->dsFlowId = dsFlowId;

            INIT_LIST_HEAD(&pGroup->dsAggregatedHead);

            if (!(pEntryData = kzalloc(sizeof(dsAggregated_entry_t), GFP_KERNEL)))
            {
                kfree(pGroup);
                return OMCI_ERR_FAILED;
            }

            if ((pEntryData->pXlateVID = (vlanAggregatedXlateInfo_t *) kzalloc(sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1), GFP_KERNEL)) == NULL)
            {
                kfree(pGroup);
                kfree(pEntryData);
                return OMCI_ERR_FAILED;
            }

            if ((pEntryData->pLutCfIdx = (unsigned int *)kzalloc(sizeof(unsigned int) * (gPlatformDb.allPortMax + 1), GFP_KERNEL)) == NULL)
            {
                kfree(pGroup);
                kfree(pEntryData->pXlateVID);
                kfree(pEntryData);
                return OMCI_ERR_FAILED;
            }

            pEntryData->id = srvId;
            pEntryData->aggrUniMask = aggrUniMask;
            pEntryData->cfIdx = cfIdx;
            pEntryData->outerVID = outerVID;
            memcpy(pEntryData->pXlateVID, pXlateVID, sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1));
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
                pEntryData->pLutCfIdx[i] = OMCI_UNUSED_CF;

            list_add_tail(&pEntryData->list, &(pGroup->dsAggregatedHead));
            list_add_tail(&pGroup->list, pHead);

        }

    }
    else
    {
        //delete
        if (!pEntryData || !pGroup)
            return OMCI_ERR_FAILED;

        list_del(&pEntryData->list);
        kfree(pEntryData->pXlateVID);
        kfree(pEntryData->pLutCfIdx);
        kfree(pEntryData);

        if (list_empty(&(pGroup->dsAggregatedHead)))
        {
            INIT_LIST_HEAD(&pGroup->dsAggregatedHead);
            list_del(&pGroup->list);
            kfree(pGroup);
        }

    }
    return OMCI_ERR_OK;
}

static int omci_set_ds_aggregated_list_lutcfidx_by_srvId(PON_GEMPORT_DIRECTION dir,
                                                         unsigned int  dsFlowId,
                                                         int srvId,
                                                         unsigned int *pLutCfIdx)
{
    dsAggregated_group_t    *pGroup     = NULL;
    dsAggregated_entry_t    *pEntryData = NULL;
    pGroup = omci_find_ds_aggregated_group_entry(dir, dsFlowId);

    pEntryData = omci_find_ds_aggregated_entry(pGroup, srvId);

    if ((pGroup == NULL) || (pEntryData == NULL))
    {
        return OMCI_ERR_FAILED;
    }

    memcpy(pEntryData->pLutCfIdx, pLutCfIdx, sizeof(unsigned int) * (gPlatformDb.allPortMax + 1));
    return OMCI_ERR_OK;
}

/*
veip maintain
*/
static veip_service_t*
omcidrv_getVeipEntry(int                servId,
                    OMCI_VLAN_OPER_ts   *pVlanRule,
                    int                 usFlowId,
                    int                 dsFlowId)
{
    veip_service_t  *pEntry;

    pEntry = veipServ_entry_find(servId);
    if (!pEntry)
    {
        if (OMCI_ERR_OK !=
                veipServ_entry_add(servId, pVlanRule, usFlowId, dsFlowId))
            return NULL;

        pEntry = veipServ_entry_find(servId);
    }

    return pEntry;
}

static int
omcidrv_setVeipUsWanCfAction(OMCI_VLAN_OPER_ts  *pVlanRule,
                            int                 vid,
                            int                 pri,
                            rtk_classify_cfg_t  *pCfEntry,
                            unsigned int        streamId)
{
    rtk_classify_us_act_t   *pUsAct;

    if (!pVlanRule || !pCfEntry)
        return OMCI_ERR_FAILED;

    pUsAct = &pCfEntry->act.usAct;

    // set cf action
    pUsAct->sidQidAct = CLASSIFY_US_SQID_ACT_ASSIGN_SID;
    pUsAct->sidQid = streamId;
    if (VLAN_ACT_NON == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pVlanRule->cTagAct.vlanAct)
    {
        // assign vid
        if (vid >= 0)
        {
            pUsAct->cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;
            pUsAct->cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
            pUsAct->cTagVid = vid;

            // assign pri
            if (pri >= 0)
            {
                pUsAct->cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
                pUsAct->cTagPri = pri;
            }
            else
            {
                // use internal priority when wan 1p disabled
                pUsAct->cPriAct = CLASSIFY_US_PRI_ACT_FROM_INTERNAL;
            }
        }
    }
    else if (VLAN_ACT_ADD == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->cTagAct.vlanAct)
    {
        pUsAct->cAct = CLASSIFY_US_CACT_ADD_CTAG_8100;

        // assign vid
        if (VID_ACT_ASSIGN == pVlanRule->cTagAct.vidAct)
        {
            pUsAct->cVidAct = CLASSIFY_US_VID_ACT_ASSIGN;
            pUsAct->cTagVid = pVlanRule->cTagAct.assignVlan.vid;
        }
        else
            pUsAct->cVidAct = CLASSIFY_US_VID_ACT_FROM_1ST_TAG;

        // assign pri
        if (PRI_ACT_ASSIGN == pVlanRule->cTagAct.priAct)
        {
            pUsAct->cPriAct = CLASSIFY_US_PRI_ACT_ASSIGN;
            pUsAct->cTagPri = pVlanRule->cTagAct.assignVlan.pri;
        }
        else
            pUsAct->cPriAct = CLASSIFY_US_PRI_ACT_FROM_1ST_TAG;
    }
    else
        return OMCI_ERR_FAILED;

    return OMCI_ERR_OK;
}

static int
omcidrv_setVeipUsWanVlanCf(OMCI_VLAN_OPER_ts    *pVlanRule,
                            int                 vid,
                            int                 pri,
                            unsigned int        cfIdx,
                            unsigned int        streamId)
{
    int                     ret = OMCI_ERR_FAILED;
    rtk_classify_cfg_t      cfEntry;
    rtk_classify_us_act_t   *pUsAct;

    if (!pVlanRule)
        return OMCI_ERR_FAILED;

    pUsAct = &cfEntry.act.usAct;

    memset(&cfEntry, 0, sizeof(rtk_classify_cfg_t));
    cfEntry.index = cfIdx;
    cfEntry.valid = ENABLED;
    cfEntry.direction = CLASSIFY_DIRECTION_US;

    // set cf rule
    memset(&uni, 0, sizeof(rtk_classify_field_t));
    uni.fieldType = CLASSIFY_FIELD_UNI;
    uni.classify_pattern.uni.value = gPlatformDb.cpuPort;
    uni.classify_pattern.uni.mask = 0x7;
    rtk_classify_field_add(&cfEntry, &uni);
    // filter ctag
    memset(&isCtag, 0, sizeof(rtk_classify_field_t));
    isCtag.fieldType = CLASSIFY_FIELD_IS_CTAG;
    isCtag.classify_pattern.isCtag.value = (vid >= 0);
    isCtag.classify_pattern.isCtag.mask = 1;
    rtk_classify_field_add(&cfEntry, &isCtag);
    // filter vid
    if (vid >= 0)
    {
        memset(&tagVid, 0, sizeof(rtk_classify_field_t));
        isCtag.fieldType = CLASSIFY_FIELD_TAG_VID;
        tagVid.classify_pattern.tagVid.value = vid;
        tagVid.classify_pattern.tagVid.mask = 0xFFF;
        rtk_classify_field_add(&cfEntry, &tagVid);

        // filter pri
        if (pri >= 0)
        {
            memset(&tagPri, 0, sizeof(rtk_classify_field_t));
            isCtag.fieldType = CLASSIFY_FIELD_TAG_PRI;
            tagPri.classify_pattern.tagPri.value = pri;
            tagPri.classify_pattern.tagPri.mask  = 0x7;
            rtk_classify_field_add(&cfEntry, &tagPri);
        }
    }

    // set cf action
    ret = omcidrv_setVeipUsWanCfAction(pVlanRule, vid, pri, &cfEntry, streamId);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set action for us wan vlan cf fail, ret = %d", ret);

        return OMCI_ERR_FAILED;
    }

    // invoke driver api
    if (RT_ERR_OK != (ret = rtk_classify_cfgEntry_add(&cfEntry)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "create rule for us wan vlan cf fail, ret = %d", ret);
    }
    _saveCfCfgToDb(cfEntry.index, &cfEntry);

    return ret;
}

static int
omcidrv_setVeipUsWanIntfCf(OMCI_VLAN_OPER_ts    *pVlanRule,
                            int                 vid,
                            int                 pri,
                            int                 netIfIdx,
                            unsigned int        cfIdx,
                            unsigned int        streamId)
{
    int                     ret = OMCI_ERR_FAILED;
    rtk_classify_cfg_t      cfEntry;
    rtk_classify_us_act_t   *pUsAct;

    if (!pVlanRule)
        return OMCI_ERR_FAILED;

    pUsAct = &cfEntry.act.usAct;

    memset(&cfEntry, 0, sizeof(rtk_classify_cfg_t));
    cfEntry.index = cfIdx;
    cfEntry.valid = ENABLED;
    cfEntry.direction = CLASSIFY_DIRECTION_US;

    // set cf rule
    memset(&wanIf, 0, sizeof(rtk_classify_field_t));
    wanIf.fieldType = CLASSIFY_FIELD_WAN_IF;
    wanIf.classify_pattern.wanIf.value = netIfIdx;
    wanIf.classify_pattern.wanIf.mask  = 0x7;
    rtk_classify_field_add(&cfEntry, &wanIf);

    // set cf action
    ret = omcidrv_setVeipUsWanCfAction(pVlanRule, vid, pri, &cfEntry, streamId);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set action for us wan intf cf fail, ret = %d", ret);

        return OMCI_ERR_FAILED;
    }

    // invoke driver api
    if (RT_ERR_OK != (ret = rtk_classify_cfgEntry_add(&cfEntry)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "create rule for us wan intf cf fail, ret = %d", ret);
    }
    _saveCfCfgToDb(cfEntry.index, &cfEntry);

    return ret;
}

static int
omcidrv_setVeipRule(veip_service_t  *pVeipEntry,
                    int             wanIdx,
                    int             vid,
                    int             pri,
                    int             type,
                    int             netIfIdx)
{
    int     ret = OMCI_ERR_FAILED;
    omci_rule_pri_t rule_pri;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    if (!pVeipEntry)
        return OMCI_ERR_FAILED;

    // check if cf already exists
    if (OMCI_UNUSED_CF != pVeipEntry->pHwPathCfIdx[wanIdx] ||
            OMCI_UNUSED_CF != pVeipEntry->pSwPathCfIdx[wanIdx])
        return OMCI_ERR_FAILED;

    // netifidx should be available
    if (netIfIdx < 0 || netIfIdx >= gPlatformDb.intfNum)
        return OMCI_ERR_FAILED;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set veip rule for wan %d", wanIdx);

    // allocate cf for hw path
    if (OMCI_ERR_OK !=
            _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &pVeipEntry->pHwPathCfIdx[wanIdx]))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "allocate cf for r-wan hw path fail!");

        return OMCI_ERR_FAILED;
    }

    // allocate cf for sw path
    if (OMCI_ERR_OK !=
            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pSwPathCfIdx[wanIdx]))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "allocate cf for r-wan sw path fail!");

        // rollback
        goto remove_hw_path_cf;
    }

    // create cf for hw path
    ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
            netIfIdx, pVeipEntry->pHwPathCfIdx[wanIdx], pVeipEntry->usStreamId);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "create cf rule for r-wan hw path fail, ret = %d", ret);

        goto remove_sw_path_cf;
    }

    // create cf for sw path
    ret = omcidrv_setVeipUsWanVlanCf(&pVeipEntry->rule, vid, pri,
            pVeipEntry->pSwPathCfIdx[wanIdx], pVeipEntry->usStreamId);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "create cf rule for r-wan sw path fail, ret = %d", ret);

        // rollback
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_cfgEntry_del(pVeipEntry->pHwPathCfIdx[wanIdx]));

        goto remove_sw_path_cf;
    }

    return OMCI_ERR_OK;

remove_sw_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
    pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

remove_hw_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->pHwPathCfIdx[wanIdx]);
    pVeipEntry->pHwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

    return ret;
}

static int
omcidrv_delVeipRule(unsigned int servId, int wanIdx)
{
    int             ret;
    veip_service_t  *pEntry;

    pEntry = veipServ_entry_find(servId);
    if (!pEntry)
        return OMCI_ERR_FAILED;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "del veip rule for wan %d", wanIdx);

    // deallocate cf for hw path
    if (OMCI_UNUSED_CF != pEntry->pHwPathCfIdx[wanIdx])
    {
        ret = rtk_classify_cfgEntry_del(pEntry->pHwPathCfIdx[wanIdx]);
        if (RT_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "delete veip cf rule %u fail, ret = %d", pEntry->pHwPathCfIdx[wanIdx], ret);

            return OMCI_ERR_FAILED;
        }
        else
        {
            _RemoveUsedCfIndex(pEntry->pHwPathCfIdx[wanIdx]);
            pEntry->pHwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;
        }
    }

    // deallocate cf for sw path
    if (OMCI_UNUSED_CF != pEntry->pSwPathCfIdx[wanIdx])
    {
        ret = rtk_classify_cfgEntry_del(pEntry->pSwPathCfIdx[wanIdx]);
        if (RT_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "delete veip cf rule %u fail, ret = %d", pEntry->pSwPathCfIdx[wanIdx], ret);

            return OMCI_ERR_FAILED;
        }
        else
        {
            _RemoveUsedCfIndex(pEntry->pSwPathCfIdx[wanIdx]);
            pEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;
        }
    }

    return OMCI_ERR_OK;
}

static int
omcidrv_matchVeipRule(OMCI_VLAN_OPER_ts     *pVlanRule,
                        int                 vid,
                        int                 pri,
                        int                 type)
{
    if (!pVlanRule)
        return OMCI_ERR_FAILED;

    // not support bridge wan in l34 lite
    if (OMCI_MODE_BRIDGE == type)
        return OMCI_ERR_FAILED;

    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_UNTAG:
            // vid should be < 0 for untag packets
            if (vid >= 0)
                return OMCI_ERR_FAILED;
            break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
            // vid should be >= 0 for tagged packets
            if (vid < 0)
                return OMCI_ERR_FAILED;
            break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            // vid should be >= 0 for tagged packets
            if (vid < 0)
                return OMCI_ERR_FAILED;

            // pri should be match if it's enabled
            if (pri >= 0 && pri != pVlanRule->filterRule.filterCTag.pri)
                return OMCI_ERR_FAILED;
            break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
            // vid should be match if it's enabled
            if (vid != pVlanRule->filterRule.filterCTag.vid)
                return OMCI_ERR_FAILED;

            // pri should be match if it's enabled
            if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI &&
                    pri >= 0 && pri != pVlanRule->filterRule.filterCTag.pri)
                return OMCI_ERR_FAILED;
            break;
        case VLAN_OPER_MODE_EXTVLAN:
            // ignore double-tagged entries by filter-out stag mode
            if (!(pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG))
                return OMCI_ERR_FAILED;

            if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_NO_TAG)
            {
                // vid should be < 0 for untag packets
                if (vid >= 0)
                    return OMCI_ERR_FAILED;
            }
            else
            {
                // vid should be >= 0 for tagged packets
                if (vid < 0)
                    return OMCI_ERR_FAILED;

                // vid should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_VID) &&
                        vid != pVlanRule->filterRule.filterCTag.vid)
                    return OMCI_ERR_FAILED;

                // pri should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI) &&
                        pri >= 0 && pri != pVlanRule->filterRule.filterCTag.pri)
                    return OMCI_ERR_FAILED;

                // ethertype should be match if it's enabled
                if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_ETHTYPE)
                {
                    // check if it's pppoe
                    if ((ETHTYPE_FILTER_PPPOE == pVlanRule->filterRule.etherType ||
                            ETHTYPE_FILTER_PPPOE_S == pVlanRule->filterRule.etherType) &&
                            (OMCI_MODE_PPPOE != type && OMCI_MODE_PPPOE_V4NAPT_V6 != type))
                        return OMCI_ERR_FAILED;

                    // check if it's ipoe 'cause
                    //  ethertype filter only applies to route wan
                    if (OMCI_MODE_IPOE != type && OMCI_MODE_IPOE_V4NAPT_V6 != type)
                        return OMCI_ERR_FAILED;
                }
            }
            break;
        default:
            break;
    }

    return OMCI_ERR_OK;
}

static int
pf_rtl96xx_UpdateVeipRule(int               wanIdx,
                            int             vid,
                            int             pri,
                            int             type,
                            int             service,
                            int             isBinding,
                            int             netIfIdx,
                            unsigned char   isRegister)
{
    struct list_head    *pEntry;
    struct list_head    *pTmpEntry;
    veip_service_t      *pData;

    if (wanIdx < 0 || wanIdx >= gPlatformDb.intfNum)
        return OMCI_ERR_FAILED;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        if (isRegister)
        {
            // check veip rule and see if it is matched to this wan interface
            if (OMCI_ERR_OK != omcidrv_matchVeipRule(&pData->rule, vid, pri, type))
                continue;

            // set veip rule
            if (OMCI_ERR_OK != omcidrv_setVeipRule(pData,
                    wanIdx, vid, pri, type, netIfIdx))
                return OMCI_ERR_FAILED;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "set veip rule %u for wan %d", pData->index, wanIdx);

            pData->wanIfBitmap |= (1 << wanIdx);

            return OMCI_ERR_OK;
        }
        else
        {
            if (!(pData->wanIfBitmap & (1 << wanIdx)))
                continue;

            // del veip rule
            if (OMCI_ERR_OK != omcidrv_delVeipRule(pData->index, wanIdx))
                return OMCI_ERR_FAILED;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "del veip rule %u for wan %d", pData->index, wanIdx);

            pData->wanIfBitmap &= ~(1 << wanIdx);

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_FAILED;
}

static int
pf_rtl96xx_SetVeipRule(OMCI_BRIDGE_RULE_ts  *pBridgeRule)
{
    OMCI_VLAN_OPER_ts   *pVlanRule;
    veip_service_t      *pVeipEntry;

    int                 wanIdx;
    int                 vid;
    int                 pri;
    int                 type;
    int                 service;
    int                 isBinding;
    int                 netIfIdx;
    unsigned char       isRuleCfg;

    if (!pBridgeRule)
        return OMCI_ERR_FAILED;

    pVlanRule = &pBridgeRule->vlanRule;

    // get/create veip entry
    pVeipEntry = omcidrv_getVeipEntry(pBridgeRule->servId,
            pVlanRule, pBridgeRule->usFlowId, pBridgeRule->dsFlowId);
    if (!pVeipEntry)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "get/create veip entry fail!");

        return OMCI_ERR_FAILED;
    }

    // check wan interfaces and see if it is matched to this rule
    for (wanIdx = 0; wanIdx < gPlatformDb.intfNum; wanIdx++)
    {
        if (OMCI_ERR_OK != omcidrv_getWanInfoByIfIdx(wanIdx,
                &vid, &pri, &type, &service, &isBinding, &netIfIdx, &isRuleCfg))
            continue;

        // ignore wan that is not exist or already matched
        if (type < 0 || isRuleCfg)
            continue;

        if (OMCI_ERR_OK != omcidrv_matchVeipRule(pVlanRule, vid, pri, type))
            continue;

        // set veip rule
        if (OMCI_ERR_OK != omcidrv_setVeipRule(pVeipEntry,
                wanIdx, vid, pri, type, netIfIdx))
            continue;

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set veip rule %u for wan %d", pVeipEntry->index, wanIdx);

        // modify wan status
        omcidrv_setWanStatusByIfIdx(wanIdx, TRUE);

        pVeipEntry->wanIfBitmap |= (1 << wanIdx);
    }

    return OMCI_ERR_OK;
}



static int pf_rtl96xx_CreateTcont(OMCI_TCONT_ts *pTcont)
{
    int ret = OMCI_ERR_OK;
    rtk_gpon_tcont_ind_t ind;
    rtk_gpon_tcont_attr_t attr;

    memset(&ind,0,sizeof(rtk_gpon_tcont_ind_t));
    memset(&attr,0,sizeof(rtk_gpon_tcont_attr_t));

    ind.alloc_id = pTcont->allocId;
    ind.type = RTK_GPON_TCONT_TYPE_1;

    if((ret = rtk_gponapp_tcont_get(&ind,&attr))!=RT_ERR_OK)
    {
        if (RT_ERR_OK == rtk_gponapp_tcont_get_physical(&ind, &attr))
        {
            gPlatformDb.tCont[attr.tcont_id].allocId = ind.alloc_id;
        }
        else
        {
            ret = _AssignNonUsedTcontId(ind.alloc_id, &attr.tcont_id);
            if(ret != OMCI_ERR_OK)
            {
                pTcont->tcontId = OMCI_DRV_INVALID_TCONT_ID;
                return OMCI_ERR_FAILED;
            }
        }
        ret = rtk_gponapp_tcont_create(&ind,&attr);
    }

    pTcont->tcontId = attr.tcont_id;

    return ret;
}


static int omci_reqTcontEmptyQ(unsigned int     schedulerId,
                                unsigned int    numOfReqQueue,
                                unsigned int    *pQidFrom)
{
    int                     ret = OMCI_ERR_FAILED;
    rtk_ponmac_queue_t      queue;
    rtk_ponmac_queueCfg_t   queueCfg;
    unsigned int            numOfEmptyQueue;
    unsigned int            tContId;
    unsigned int            queueId;

    if (schedulerId >= gPlatformDb.maxTcont ||
            0 == numOfReqQueue || !pQidFrom)
        return ret;

    numOfEmptyQueue = 0;
    for (queueId = 0; queueId < gPlatformDb.maxTcontQueue; queueId++)
    {
        queue.queueId = queueId;
        queue.schedulerId = schedulerId;

        // skip queues that are already configured and reset counter
        if (RT_ERR_OK == rtk_ponmac_queue_get(&queue, &queueCfg))
        {
            numOfEmptyQueue ++;
        }
        else
        {
            if (RTL9603D_CHIP_ID == gPlatformDb.chipId)
            {
                // each tcont with individual tcont queue
                numOfEmptyQueue++;

                // TBD: check hierarchical enable or not
            }
            else
            {
                // search if other tcont has this queue
                for (tContId = (schedulerId / 8 * 8);
                        tContId < (schedulerId / 8 * 8 + 8); tContId++)
                {
                    if (tContId == schedulerId)
                        continue;

                    queue.schedulerId = tContId;

                    if (RT_ERR_OK == rtk_ponmac_queue_get(&queue, &queueCfg))
                    {
                        numOfEmptyQueue = 0;

                        break;
                    }
                }

                if (tContId == (schedulerId / 8 * 8 + 8))
                    numOfEmptyQueue++;
            }
        }

        if (numOfEmptyQueue == numOfReqQueue)
        {
            *pQidFrom = queueId - numOfReqQueue + 1;

            ret = OMCI_ERR_OK;

            break;
        }
    }

    return ret;
}

static int pf_rtl96xx_UpdateTcont(OMCI_TCONT_ts *pTcont)
{
    int                     ret = OMCI_ERR_FAILED;
    rtk_gpon_tcont_ind_t    ind;
    rtk_gpon_tcont_attr_t   attr;
    unsigned int            numOfReqQueue;
    unsigned int            tContId;
    unsigned int            queueId;

    memset(&ind, 0, sizeof(rtk_gpon_tcont_ind_t));
    memset(&attr, 0, sizeof(rtk_gpon_tcont_attr_t));

    ind.alloc_id = pTcont->allocId;
    if (RT_ERR_OK != rtk_gponapp_tcont_get(&ind, &attr))
        return OMCI_ERR_FAILED;

    // use tcontId to pass numOfReqQueue
    numOfReqQueue = pTcont->tcontId;

    if (OMCI_ERR_OK == omci_reqTcontEmptyQ(attr.tcont_id, numOfReqQueue, &queueId))
    {
        gPlatformDb.tCont[attr.tcont_id].qIdFrom = queueId;

        pTcont->tcontId = attr.tcont_id;

        ret = OMCI_ERR_OK;
    }
    else
    {
        for (tContId = 0; tContId < gPlatformDb.maxTcont; tContId++)
        {
            if (RTL9603D_CHIP_ID == gPlatformDb.chipId)
            {
                // TBD: check hierarchical enable or not
            }
            else
            {
                if ((tContId / 8) == (attr.tcont_id / 8))
                    continue;
            }
            if (OMCI_ERR_OK != omci_reqTcontEmptyQ(tContId, numOfReqQueue, &queueId))
                continue;

            gPlatformDb.tCont[attr.tcont_id].allocId = 0xff;
            gPlatformDb.tCont[tContId].allocId = ind.alloc_id;
            gPlatformDb.tCont[tContId].qIdFrom = queueId;

            pTcont->tcontId = tContId;

            attr.tcont_id = tContId;
            ind.type = RTK_GPON_TCONT_TYPE_1;

            // destory old tcont and re-create with new id
            rtk_gponapp_tcont_destroy(&ind);
            ret = rtk_gponapp_tcont_create(&ind, &attr);

            break;
        }
    }

    return ret;
}

static unsigned int omci_getTcontQid(unsigned int tContId, unsigned int queueId)
{
    if (tContId >= gPlatformDb.maxTcont)
        return OMCI_DRV_INVALID_QUEUE_ID;

    return (gPlatformDb.tCont[tContId].qIdFrom + queueId);
}

static int
omci_set_stag_acl_by_dp_marking(
    unsigned int dpMarking, unsigned int uniMask, unsigned int *aclIdBitmap)
{
    int                         ret, rsv_cnt = 1, empty_cnt = 0;
    unsigned short              i;
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    int16                       aclIdx;
    uint16                      maxAclIdx;
    BOOL                        findAvailAcl = FALSE;

    switch (dpMarking)
    {
        case PQ_DROP_COLOUR_PCP_7P1D_MARKING:
            rsv_cnt += 1;
            break;
        case PQ_DROP_COLOUR_PCP_6P2D_MARKING:
            rsv_cnt += 2;
            break;
        case PQ_DROP_COLOUR_PCP_5P3D_MARKING:
            rsv_cnt += 3;
            break;
        default:
            return OMCI_ERR_OK;
    }

    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid)
            {
                empty_cnt++;
                if(rsv_cnt == empty_cnt){
                    findAvailAcl = TRUE;
                    break;
            }
            }
            else
                empty_cnt = 0;
        }
    }

    if (findAvailAcl == FALSE)
    {
        return OMCI_ERR_FAILED;
    }

    if (PQ_DROP_COLOUR_PCP_7P1D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_6P2D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 4;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // set priority action
        aclRule.act.enableAct[ACL_IGR_PRI_ACT]= ENABLED;
        aclRule.act.priAct.act = ACL_IGR_PRI_ACL_PRI_ASSIGN_ACT;
        aclRule.act.priAct.aclPri = 1;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx, ret);

            return OMCI_ERR_FAILED;
        }


        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        aclRule.careTag.tags[ACL_CARE_TAG_STAG].value = ENABLED;
        aclRule.careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 1;
        aclRule.templateIdx = 0x0;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 1, ret);

            return OMCI_ERR_FAILED;
        }
    }

    if (PQ_DROP_COLOUR_PCP_6P2D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 2;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 2;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 2, ret);

            return OMCI_ERR_FAILED;
        }
    }

    if (PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 0;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 3;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 3, ret);

            return OMCI_ERR_FAILED;
        }
    }

    memset(aclIdBitmap, 0x00, (gPlatformDb.aclNum / (sizeof(unsigned int) * 8)));

    for (i = 0; i < rsv_cnt; i++)
    {
        aclIdBitmap[(aclIdx + i) / (sizeof(unsigned int) * 8)] |= (1 << ((aclIdx + i) % (sizeof(unsigned int) * 8)));
    }

    return OMCI_ERR_OK;

}

static int dsDpGemStagAcl_add_entry(unsigned int gem_id, unsigned int dpMarking)
{
    dsDpGemStagAcl_entry_t   *pEntry = NULL;

    if (!(pEntry = kzalloc(sizeof(dsDpGemStagAcl_entry_t), GFP_KERNEL)))
        return OMCI_ERR_FAILED;

    pEntry->gemId = gem_id;

    if (!(pEntry->pAclIdBitmap = (unsigned int *)kzalloc(sizeof(unsigned int) * (gPlatformDb.aclNum / (sizeof(unsigned int) * 8)), GFP_KERNEL)))
    {
        kfree(pEntry);
        return OMCI_ERR_FAILED;
    }

    omci_set_stag_acl_by_dp_marking(dpMarking,
        (1 << gPlatformDb.ponPort), pEntry->pAclIdBitmap);

    list_add_tail(&pEntry->list, &gPlatformDb.dsDpGemStagAclHead);

    return OMCI_ERR_OK;
}

static dsDpGemStagAcl_entry_t* dsDpGemStagAcl_find_entry(unsigned int gem_id)
{
    dsDpGemStagAcl_entry_t  *pEntryData = NULL;
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.dsDpGemStagAclHead)
    {
        pEntryData = list_entry(pEntry, dsDpGemStagAcl_entry_t, list);

        if (gem_id == pEntryData->gemId)
            return pEntryData;
    }

    return NULL;
}

static void dsDpGemStagAcl_del_entry(unsigned int gem_id)
{
    dsDpGemStagAcl_entry_t  *pEntryData = NULL;
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.dsDpGemStagAclHead)
    {
        pEntryData = list_entry(pEntry, dsDpGemStagAcl_entry_t, list);

        if (gem_id == pEntryData->gemId)
        {
            kfree(pEntryData->pAclIdBitmap);
            list_del(&pEntryData->list);
            kfree(pEntryData);
            return;
        }
    }
   return;
}

static int
omci_replace_stag_acl_by_dp_marking(
    unsigned int dpMarking, unsigned int uniMask, unsigned int *aclIdBitmap)
{
    int16                       aclIdx;
    uint16                      maxAclIdx;
    int                         ret, rsv_cnt = 1, empty_cnt = 0;
    unsigned short              i;
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    BOOL                        findAvailAcl = FALSE;

    if (PQ_DROP_COLOUR_PCP_7P1D_MARKING != dpMarking &&
        PQ_DROP_COLOUR_PCP_6P2D_MARKING != dpMarking &&
        PQ_DROP_COLOUR_PCP_5P3D_MARKING != dpMarking)
    {
        for (i = 0; i < gPlatformDb.aclNum; i++)
        {
            if (aclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(i));
            }
        }
        return OMCI_ERR_OK;
    }

    switch (dpMarking)
    {
        case PQ_DROP_COLOUR_PCP_7P1D_MARKING:
            rsv_cnt += 1;
            break;
        case PQ_DROP_COLOUR_PCP_6P2D_MARKING:
            rsv_cnt += 2;
            break;
        case PQ_DROP_COLOUR_PCP_5P3D_MARKING:
            rsv_cnt += 3;
            break;
        default:
            return OMCI_ERR_OK;
    }

    maxAclIdx = gPlatformDb.aclActNum > gPlatformDb.aclNum ? gPlatformDb.aclActNum : gPlatformDb.aclNum;

    for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid)
            {
                empty_cnt++;
                if(rsv_cnt == empty_cnt){
                    findAvailAcl = TRUE;
                    break;
            }
            }
            else
                empty_cnt = 0;
        }
    }

    if (findAvailAcl == FALSE)
        return -1;

    if (PQ_DROP_COLOUR_PCP_7P1D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_6P2D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 4;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // set priority action
        aclRule.act.enableAct[ACL_IGR_PRI_ACT]= ENABLED;
        aclRule.act.priAct.act = ACL_IGR_PRI_ACL_PRI_ASSIGN_ACT;
        aclRule.act.priAct.aclPri = 1;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx, ret);

            return OMCI_ERR_FAILED;
        }

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        aclRule.careTag.tags[ACL_CARE_TAG_STAG].value = ENABLED;
        aclRule.careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 1;
        aclRule.templateIdx = 0x0;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 1, ret);

            return OMCI_ERR_FAILED;
        }
    }

    if (PQ_DROP_COLOUR_PCP_6P2D_MARKING == dpMarking ||
        PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 2;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 2;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 2, ret);

            return OMCI_ERR_FAILED;
        }
    }

    if (PQ_DROP_COLOUR_PCP_5P3D_MARKING == dpMarking)
    {

        memset(&aclField, 0, sizeof(rtk_acl_field_t));
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // add stag/ctag field
        aclField.fieldType = ACL_FIELD_STAG;
        aclField.fieldUnion.l2tag.pri.value = 0;
        aclField.fieldUnion.l2tag.pri.mask = RTK_DOT1P_PRIORITY_MAX;

        if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
            return OMCI_ERR_FAILED;

        // set other rules
        aclRule.valid = ENABLED;
        aclRule.index = aclIdx + 3;
        aclRule.templateIdx = 0x0;
        aclRule.invert = ACL_INVERT_ENABLE;
        aclRule.activePorts.bits[0] = uniMask;

        // add acl entry
        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add ds dei dp acl %u failed, return code %d",
                aclIdx + 3, ret);

            return OMCI_ERR_FAILED;
        }
    }

    for (i = 0; i < rsv_cnt; i++)
    {
        aclIdBitmap[(aclIdx + i) / (sizeof(unsigned int) * 8)] |= (1 << ((aclIdx + i) % (sizeof(unsigned int) * 8)));
    }

    return OMCI_ERR_OK;

}

static int omci_cfg_ds_dp_stag_acl(int op, unsigned int flow_id, unsigned int gem_id)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    flow2DsPq_entry_t       *pEntryData = NULL;
    dsDpGemStagAcl_entry_t  *pDsDpGem   = NULL;
    unsigned int            i;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        if (flow_id != pEntryData->flowId || gem_id != pEntryData->gemPortId)
            continue;

        if (op)
        {
            //create
            if ((pDsDpGem = dsDpGemStagAcl_find_entry(gem_id)))
            {
                omci_replace_stag_acl_by_dp_marking(pEntryData->dsQ.dpMarking,
                                                    (1 << gPlatformDb.ponPort),
                                                    pDsDpGem->pAclIdBitmap);

            }
            else
            {
                // new
                dsDpGemStagAcl_add_entry(gem_id, pEntryData->dsQ.dpMarking);
            }

        }
        else
        {
            //delete
            if ((pDsDpGem = dsDpGemStagAcl_find_entry(gem_id)) != NULL)
            {
                for (i = 0; i < gPlatformDb.aclNum; i++)
                {
                    if (pDsDpGem->pAclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
                    {
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(i));
                    }
                }

                dsDpGemStagAcl_del_entry(gem_id);
            }
        }
    }
    return OMCI_ERR_OK;
}

static int omci_set_flow_map_ds_queue(OMCI_GEM_FLOW_ts *f)
{
    flow2DsPq_entry_t   *pEntry = NULL;
    rtk_qos_queue_weights_t weights;
    uint32 index;
    int ret = RT_ERR_OK;

    memset(&weights, 0x00, sizeof(rtk_qos_queue_weights_t));

    if (!f)
        return OMCI_ERR_FAILED;

//    if (f->dir != PON_GEMPORT_DIRECTION_DS)
//        return OMCI_ERR_OK;

    if (RT_ERR_OK != (ret = rtk_qos_schedulingQueue_get(f->dsQInfo.portId, &weights)))
    {
        printk("%s: Failed to call rtk_qos_schedulingQueue_get\n", __FUNCTION__);
        return OMCI_ERR_FAILED;
    }

    weights.weights[f->dsQInfo.priority] = f->dsQInfo.weight;

    if (f->ena)
    {
        // insert
        pEntry = flow2DsPq_entry_find(f->flowId);
        if (pEntry)
        {
            memcpy(&(pEntry->dsQ), &(f->dsQInfo), sizeof(OMCI_DS_PQ_INFO));
        }
        else
        {
            if (OMCI_ERR_OK != flow2DsPq_entry_add(f->flowId, f->portId, &(f->dsQInfo)))
               return RT_ERR_FAILED;
        }
        if (f->dsQInfo.policy == PQ_POLICY_WEIGHTED_ROUND_ROBIN)
        {
            if (RT_ERR_OK != (ret = rtk_qos_schedulingQueue_set(f->dsQInfo.portId, &weights)))
                printk("%s %d ret=%d\n", __FUNCTION__, __LINE__, ret);

            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s %d portId=%u,qid=%u,weight=%u\n",
                __FUNCTION__, __LINE__,
                f->dsQInfo.portId, f->dsQInfo.priority, f->dsQInfo.weight);
        }
        omci_cfg_ds_dp_stag_acl(f->ena, f->flowId, f->portId);
    }
    else
    {
        omci_cfg_ds_dp_stag_acl(f->ena, f->flowId, f->portId);
        // delete
        for (index = 0; index < 8; index++)
        {
            weights.weights[index] = 0;
            ret = rtk_qos_schedulingQueue_set(f->dsQInfo.portId, &weights);
        }
        if (OMCI_ERR_OK != flow2DsPq_entry_del(f->flowId))
            return RT_ERR_FAILED;
    }
    flow_to_ds_pq_remap_with_sp_wrr_policy();
    return OMCI_ERR_OK;
}

static int pf_rtl96xx_CfgGemFlow(OMCI_GEM_FLOW_ts *gemFlow)
{
    int ret,flowId;
    rtk_gpon_dsFlow_attr_t dsFlow;
    rtk_gpon_usFlow_attr_t usFlow;
    rtk_ponmac_queue_t queue;
    uint16  shiftByte;
    uint8   shiftBit;

    if(!gemFlow)
        return OMCI_ERR_FAILED;


    flowId = gemFlow->flowId;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Create/Delete Flow for Gem Port 0x%x in Direction[%d]",gemFlow->portId, gemFlow->dir);

    if(gemFlow->dir == PON_GEMPORT_DIRECTION_US || gemFlow->dir == PON_GEMPORT_DIRECTION_BI)
    {

        if (gemFlow->ena == TRUE)
        {

            memset(&usFlow, 0, sizeof(rtk_gpon_usFlow_attr_t));
            usFlow.type = gemFlow->isOmcc ? RTK_GPON_FLOW_TYPE_OMCI : RTK_GPON_FLOW_TYPE_ETH;
            usFlow.gem_port_id = gemFlow->portId;

            usFlow.tcont_id = gemFlow->tcontId;

            /*create us gem flow*/
            ret = rtk_gponapp_usFlow_set(gemFlow->flowId, &usFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"could not create u/s gem flow [0x%x]", (int)ret);
                return OMCI_ERR_FAILED;
            }

            queue.queueId = omci_getTcontQid(gemFlow->tcontId,gemFlow->queueId);
            queue.schedulerId = gemFlow->tcontId;

            if(queue.queueId!=OMCI_DRV_INVALID_QUEUE_ID)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_ponmac_flow2Queue_set(flowId,&queue));
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"create a u/s gem flow [%d]", flowId);

        }
        else
        {
            /* delete a gem u/s flow */
            memset(&usFlow, 0, sizeof(rtk_gpon_usFlow_attr_t));
            usFlow.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;
            ret = rtk_gponapp_usFlow_set(flowId, &usFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"\n delete a u/s gem flow [%d] Fail, ret = 0x%X", flowId, ret);
                return OMCI_ERR_FAILED;
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete a u/s gem flow [%d]", flowId);
        }
    }
    if (gemFlow->dir == PON_GEMPORT_DIRECTION_DS || gemFlow->dir == PON_GEMPORT_DIRECTION_BI)
    {
        if (gemFlow->ena == TRUE)
        {
            shiftByte = gemFlow->portId / 8;
            shiftBit = gemFlow->portId % 8;

            dsFlow.gem_port_id = gemFlow->portId;
            dsFlow.type        = gemFlow->isOmcc == TRUE ? RTK_GPON_FLOW_TYPE_OMCI : RTK_GPON_FLOW_TYPE_ETH;
            dsFlow.multicast   = gemFlow->isFilterMcast == TRUE ? ENABLED : DISABLED;
            dsFlow.aes_en      = (gPlatformDb.gemEncrypt[shiftByte] & (1 << shiftBit)) ? ENABLED : DISABLED;

            ret = rtk_gponapp_dsFlow_set(flowId, &dsFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"could not create d/s gem flow [0x%x]", (int)ret);

                return OMCI_ERR_FAILED;
            }
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"create a d/s gem flow [%d]", flowId);

        }
        else
        {
            /* delete a gem d/s flow */
            dsFlow.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;
            dsFlow.type         = 0;
            dsFlow.multicast    = DISABLED;
            dsFlow.aes_en       = DISABLED;
            ret = rtk_gponapp_dsFlow_set(flowId, &dsFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"\n delete a d/s gem flow [%d] Fail, ret = 0x%X", flowId, ret);
                return OMCI_ERR_FAILED;
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete a d/s gem flow [%d]", flowId);
        }
        omci_set_flow_map_ds_queue(gemFlow);
    }
    return OMCI_ERR_OK;
}

static int omci_SetPriRemapByDpMarking(uint8 dpMarking)
{
    uint8   dot1qPri;

    // 1p remapping only for ctag. stag should be use acl rule with multiple rule.
    if (dpMarking >= PQ_DROP_COLOUR_PCP_8P0D_MARKING &&
            dpMarking <= PQ_DROP_COLOUR_PCP_5P3D_MARKING)
    {
        // PQ_DROP_COLOUR_PCP_8P0D_MARKING
        for (dot1qPri = 0; dot1qPri < RTK_MAX_NUM_OF_PRIORITY; dot1qPri++)
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, dot1qPri, 1, 0));
    }
    switch (dpMarking)
    {
        case PQ_DROP_COLOUR_PCP_7P1D_MARKING:
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, 4, 0, 0));
            break;

        case PQ_DROP_COLOUR_PCP_6P2D_MARKING:
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, 4, 0, 0));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, 2, 0, 0));
            break;

        case PQ_DROP_COLOUR_PCP_5P3D_MARKING:
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, 4, 0, 0));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG,rtk_qos_1pPriRemapGroup_set(0, 2, 0, 0));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, 0, 0, 0));
            break;

        default:
            break;
    }

    return OMCI_ERR_OK;
}

static int omci_CreateDsDeiDpAcl(uint8 isStag, uint8 dei)
{
    int                         ret;
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    int16                       aclIdx;
    uint16                      maxAclIdx;
    BOOL                        findAvailAcl = FALSE;

    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid)
            {
                findAvailAcl = TRUE;
                break;
        }
    }
    }
    if (findAvailAcl == FALSE)
        return -1;

    memset(&aclField, 0, sizeof(rtk_acl_field_t));
    memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

    // add stag/ctag field
    aclField.fieldType = isStag ? ACL_FIELD_STAG : ACL_FIELD_CTAG;
    aclField.fieldUnion.l2tag.cfi_dei.value = dei;
    aclField.fieldUnion.l2tag.cfi_dei.mask = RTK_DOT1P_DEI_MAX;

    if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
        return -1;

    // set care stag/ctag
    if (isStag)
    {
        aclRule.careTag.tags[ACL_CARE_TAG_STAG].value = ENABLED;
        aclRule.careTag.tags[ACL_CARE_TAG_STAG].mask = ENABLED;
    }
    else
    {
        aclRule.careTag.tags[ACL_CARE_TAG_CTAG].value = ENABLED;
        aclRule.careTag.tags[ACL_CARE_TAG_CTAG].mask = ENABLED;
    }

    // set other rules
    aclRule.valid = ENABLED;
    aclRule.index = aclIdx;
    aclRule.templateIdx = isStag ? 0x0 : 0x1;
    aclRule.activePorts.bits[0] = (1 << gPlatformDb.ponPort);

    // set priority action
    aclRule.act.enableAct[ACL_IGR_PRI_ACT]= ENABLED;
    aclRule.act.priAct.act = ACL_IGR_PRI_ACL_PRI_ASSIGN_ACT;
    aclRule.act.priAct.aclPri = !dei;

    // add acl entry
    if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add ds dei dp acl %u (isStag %hu, dei %hu) failed, return code %d",
            aclIdx, isStag, dei, ret);

        return -1;
    }

    return aclIdx;
}

static int omci_CreateMacFilterAcl(unsigned int portMask, OMCI_MACFILTER_ts *pMacFilter, int *pHwAclIdx)
{
    uint32                      port, rsv_cnt = 2, empty_cnt = 0;
    int                         ret;
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    int16                       aclIdx;
    uint16                      maxAclIdx;
    unsigned char               mac_mask[ETHER_ADDR_LEN];
    BOOL                        multi_match_flags = FALSE;
    BOOL                        findAvailAcl = FALSE;

    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx ; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (RTL9601B_CHIP_ID == gPlatformDb.chipId ||
                MAC_FILTER_ACT_FORWARD != pMacFilter->filter)
            {
                if (!aclRule.valid){
                    findAvailAcl = TRUE;
                    break;
            }
            }
            else
            {
                if (!aclRule.valid)
                {
                    empty_cnt++;
                    if (rsv_cnt == empty_cnt){
                        findAvailAcl = TRUE;
                        break;
                }
                }
                else
                    empty_cnt = 0;

            }
        }
    }

    if (findAvailAcl == FALSE)
        return -1;

    memset(mac_mask, 0xFF, ETHER_ADDR_LEN);
    memset(&aclField, 0, sizeof(rtk_acl_field_t));
    memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
    // set rule
    aclField.fieldType = ((MAC_FILTER_TYPE_SA == pMacFilter->macType) ? ACL_FIELD_SMAC : ACL_FIELD_DMAC);
    memcpy(aclField.fieldUnion.mac.value.octet, pMacFilter->mac, ETHER_ADDR_LEN);
    memcpy(aclField.fieldUnion.mac.mask.octet, mac_mask, ETHER_ADDR_LEN);

    if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
        return -1;

    // set rules work on port
    aclRule.valid = ENABLED;
    aclRule.index = aclIdx;

    if (RTL9601B_CHIP_ID == gPlatformDb.chipId)
    {
        aclRule.templateIdx = 0x0;
        if(portMask & (1 << gPlatformDb.ponPort))
            aclRule.activePorts.bits[0] = (1 << gPlatformDb.ponPort);
        else
            aclRule.activePorts.bits[0] = portMask;

        // set drop action
        aclRule.act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
        aclRule.act.forwardAct.act = ACL_IGR_FORWARD_DROP_ACT;
        aclRule.act.forwardAct.portMask.bits[0] = 0;
    }
    else
    {

        aclRule.templateIdx = 0x1;

        if (MAC_FILTER_ACT_FORWARD == pMacFilter->filter)
        {
            aclRule.invert = ACL_INVERT_ENABLE;
            // set flag for creating the other acl rules
            multi_match_flags = TRUE;
        }
        aclRule.activePorts.bits[0] = portMask;

        // set drop action
        aclRule.act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
        aclRule.act.forwardAct.act = (APOLLOMP_CHIP_ID == gPlatformDb.chipId ? ACL_IGR_FORWARD_REDIRECT_ACT : ACL_IGR_FORWARD_EGRESSMASK_ACT);
        aclRule.act.forwardAct.portMask.bits[0] = portMask;
    }

    for (port = gPlatformDb.etherPortMin; port <= gPlatformDb.etherPortMax; port++)
    {
        if((1 << port) & portMask)
        {
            if(RT_ERR_OK != rtk_acl_igrState_set(port, ENABLED))
                return -1;
        }
    }

    // add acl entry
    if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add acl rule %u failed, return code %d", aclIdx, ret);

        return -1;
    }
    *pHwAclIdx = aclIdx;

    if (multi_match_flags)
    {
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

        // set rules work on port
        aclRule.valid = ENABLED;
        aclRule.index = (aclIdx + 1);
        aclRule.templateIdx = 0x1;

        aclRule.activePorts.bits[0] = portMask;

        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add acl rule %u failed, return code %d", aclRule.index, ret);

            return -1;
        }
    }

    if (RTL9601B_CHIP_ID != gPlatformDb.chipId && multi_match_flags)
    {
        int *pTmp = NULL;
        pTmp = (pHwAclIdx + 1);
        *pTmp = (aclIdx + 1);
    }

    return OMCI_ERR_OK;
}

static int omci_copyArpToCpu(rtk_portmask_t *pPortMask)
{
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    uint32                      maxAclIdx;
    int32                       aclIdx;
    int32                       ret;

    // find an available acl
    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = maxAclIdx - 1; aclIdx >= 0; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid)
                break;
        }
    }
    if (aclIdx < 0)
        return -1;

    memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
    memset(&aclField, 0, sizeof(rtk_acl_field_t));
    // set acl rule
    aclField.fieldType = ACL_FIELD_ETHERTYPE;
    aclField.fieldUnion.data.value = 0x0806;
    aclField.fieldUnion.data.mask = 0xFFFF;
    ret = rtk_acl_igrRuleField_add(&aclRule, &aclField);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add gem port field for ds bc gem trap acl failed, ret %d", ret);

        return ret;
    }
    aclRule.valid = ENABLED;
    aclRule.index = aclIdx;
    aclRule.templateIdx = 0;
    memcpy(&aclRule.activePorts, pPortMask, sizeof(rtk_portmask_t));

    // set copy-to-cpu action
    aclRule.act.enableAct[ACL_IGR_FORWARD_ACT] = ENABLED;
    if(RTL9601B_CHIP_ID == gPlatformDb.chipId)
    {
        aclRule.act.forwardAct.act = ACL_IGR_FORWARD_COPY_ACT;
    } else {
        aclRule.act.forwardAct.act = ACL_IGR_FORWARD_IGR_MIRROR_ACT;
    }
    aclRule.act.forwardAct.portMask.bits[0] = (1 << gPlatformDb.cpuPort);

    // add acl entry
    ret = rtk_acl_igrRuleEntry_add(&aclRule);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add ds bc gem trap acl failed, ret %d", ret);

        return ret;
    }

    return RT_ERR_OK;
}


static int omci_SetIgmpMldInternalPri(void)
{
    uint32 port;
    int                         ret;
    rtk_acl_field_t             aclField;
    rtk_acl_ingress_entry_t     aclRule;
    int16                       aclIdx;
    uint16                      maxAclIdx;
    rtk_pri_t                   priId;
    rtk_qos_pri2queue_t         pri2queue;

    //Set priority re-mapping of CPU port
    for (priId = 0; priId < RTK_MAX_NUM_OF_PRIORITY; priId++)
    {
        if ((RT_ERR_OK != rtk_qos_fwd2CpuPriRemap_set(priId, priId)))
        {
            return -1;
        }
        pri2queue.pri2queue[priId] = priId;
    }

    //set table Id = 0 for cpu port
    rtk_qos_portPriMap_set(gPlatformDb.cpuPort, 3);
    //assign priority to queue mapping
    rtk_qos_priMap_set(3, &pri2queue);

    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;

    for (aclIdx = maxAclIdx - 1; aclIdx >= 0; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid)
                break;
        }
    }
    if (aclIdx < 0)
        return -1;

    memset(&aclField, 0, sizeof(rtk_acl_field_t));
    memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
    // set user defined rule for IGMP-MLD packet type
    aclField.fieldType = ACL_FIELD_USER_DEFINED15;
    aclField.fieldUnion.data.value = 0x40;
    aclField.fieldUnion.data.mask = 0x40;

    if (RT_ERR_OK != rtk_acl_igrRuleField_add(&aclRule, &aclField))
        return -1;

    // set rules work on port
    aclRule.valid = ENABLED;
    aclRule.index = aclIdx;
    aclRule.templateIdx = 0x3;

    for (port = gPlatformDb.etherPortMin; port <= gPlatformDb.etherPortMax; port++)
    {
        if (port == gPlatformDb.cpuPort || port == gPlatformDb.rgmiiPort)
            continue;

        aclRule.activePorts.bits[0] |= (1 << port);
    }

    // set acl pri action
    aclRule.act.enableAct[ACL_IGR_PRI_ACT] = ENABLED;
    aclRule.act.priAct.act = ACL_IGR_PRI_ACL_PRI_ASSIGN_ACT;
    aclRule.act.priAct.aclPri = 7;

    for(port = gPlatformDb.etherPortMin; port <= gPlatformDb.etherPortMax; port++)
    {
        if (port == gPlatformDb.cpuPort || port == gPlatformDb.rgmiiPort)
            continue;

        if(RT_ERR_OK != rtk_acl_igrState_set(port, ENABLED))
            return -1;
    }
    // add acl entry
    if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add acl rule %u failed, return code %d", aclIdx, ret);

        return -1;
    }
    return OMCI_ERR_OK;
}


/*Assign DS OMCI CF PRI to 7, to avoid priority queue issue*/
static int omci_setOmciInternalPri(void)
{
    int     ret;
    rtk_classify_cfg_t  classifyCfg;
    rtk_classify_field_t tosGemIdField;

    memset(&classifyCfg, 0x0, sizeof(rtk_classify_cfg_t));
    memset(&tosGemIdField, 0x0, sizeof(tosGemIdField));

    if(gPlatformDb.chipId == APOLLOMP_CHIP_ID)
    {
        classifyCfg.direction = CLASSIFY_DIRECTION_DS;
        classifyCfg.index = gPlatformDb.cfTotalNum - 2;
        tosGemIdField.fieldType = CLASSIFY_FIELD_TOS_DSIDX;
        tosGemIdField.classify_pattern.tosDsidx.value = 127;
        tosGemIdField.classify_pattern.tosDsidx.mask = 0x7f;
        rtk_classify_field_add(&classifyCfg, &tosGemIdField);

        classifyCfg.act.dsAct.interPriAct = CLASSIFY_CF_PRI_ACT_ASSIGN;
        classifyCfg.act.dsAct.cfPri = 7;

        if (RT_ERR_OK != (ret = rtk_classify_cfgEntry_add(&classifyCfg)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add downstream OMCI internal pri CF fail, return code %d", ret);
            return OMCI_ERR_FAILED;
        }
    }
    return OMCI_ERR_OK;

}

static void omci_UpdateDpMarkingByPriQId(OMCI_PRIQ_ts *pPriQ)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    flow2DsPq_entry_t       *pEntryData = NULL;
    dsDpGemStagAcl_entry_t  *pDsDpGem   = NULL;
    rtk_qos_queue_weights_t weights;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        if (pEntryData->dsQ.dsPqOmciPri == pPriQ->queueId)
        {
            pEntryData->dsQ.dpMarking = pPriQ->dpMarking;
            pEntryData->dsQ.policy = pPriQ->scheduleType;
            pEntryData->dsQ.weight = pPriQ->weight;
            memset(&weights, 0x00, sizeof(rtk_qos_queue_weights_t));
            if (RT_ERR_OK != rtk_qos_schedulingQueue_get(pEntryData->dsQ.portId, &weights))
            {
                printk("%s(%d): Failed to call rtk_qos_schedulingQueue_get\n", __FUNCTION__, __LINE__);
                continue;
            }

            if (pEntryData->dsQ.policy == PQ_POLICY_WEIGHTED_ROUND_ROBIN)
                weights.weights[pEntryData->dsQ.priority] = pEntryData->dsQ.weight;
            else
                weights.weights[pEntryData->dsQ.priority] = 0;

            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_schedulingQueue_set(pEntryData->dsQ.portId, &weights));

            if ((pDsDpGem = dsDpGemStagAcl_find_entry(pEntryData->gemPortId)))
            {
                omci_replace_stag_acl_by_dp_marking(pEntryData->dsQ.dpMarking,
                                                    (1 << gPlatformDb.ponPort),
                                                    pDsDpGem->pAclIdBitmap);

            }
            else
            {
                // new
                dsDpGemStagAcl_add_entry(pEntryData->gemPortId, pEntryData->dsQ.dpMarking);
            }
        }
    }
    return;
}

static int omci_SetDsPriQ(OMCI_PRIQ_ts *pPriQ)
{
    rtk_qos_pri2queue_t     pri2Qid;
    uint8                   priority;

    omci_UpdateDpMarkingByPriQId(pPriQ);

    // return if dp is supported natively
    if (gPlatformDb.perUNIQueueDp)
        return OMCI_ERR_OK;

    if (PQ_DROP_COLOUR_NO_MARKING != pPriQ->dpMarking &&
            PQ_DROP_COLOUR_DEI_MARKING != pPriQ->dpMarking &&
            PQ_DROP_COLOUR_PCP_8P0D_MARKING != pPriQ->dpMarking &&
            PQ_DROP_COLOUR_PCP_7P1D_MARKING != pPriQ->dpMarking &&
            PQ_DROP_COLOUR_PCP_6P2D_MARKING != pPriQ->dpMarking &&
            PQ_DROP_COLOUR_PCP_5P3D_MARKING != pPriQ->dpMarking)
        return OMCI_ERR_OK;

    if (PQ_DROP_COLOUR_NO_MARKING == pPriQ->dpMarking)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_portPriMap_set(pPriQ->tcontId, 0));

        if (g_isDsDeiDpAclCfg)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei0DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei1DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei0DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei1DpAcl));

            g_isDsDeiDpAclCfg = FALSE;
        }

        return OMCI_ERR_OK;
    }

    for (priority = 1; priority < RTK_MAX_NUM_OF_PRIORITY; priority++)
        pri2Qid.pri2queue[priority] = 1;
    pri2Qid.pri2queue[0] = 0;

    // invoke priority to queue setting
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priMap_set(1, &pri2Qid));
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_portPriMap_set(pPriQ->tcontId, 1));

    if (PQ_DROP_COLOUR_DEI_MARKING == pPriQ->dpMarking)
    {
        if (!g_isDsDeiDpAclCfg)
        {
            g_dsStagDei0DpAcl = omci_CreateDsDeiDpAcl(TRUE, 0);
            g_dsStagDei1DpAcl = omci_CreateDsDeiDpAcl(TRUE, 1);
            g_dsCtagDei0DpAcl = omci_CreateDsDeiDpAcl(FALSE, 0);
            g_dsCtagDei1DpAcl = omci_CreateDsDeiDpAcl(FALSE, 1);

            g_isDsDeiDpAclCfg = TRUE;
        }
    }
    else
    {
        if (g_isDsDeiDpAclCfg)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei0DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei1DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei0DpAcl));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei1DpAcl));

            g_isDsDeiDpAclCfg = FALSE;
        }

        // invoke priority remap by dp marking
        omci_SetPriRemapByDpMarking(pPriQ->dpMarking);
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetPriQueue(OMCI_PRIQ_ts *pPriQ)
{
    int ret;
    rtk_ponmac_queue_t  queue;
    rtk_ponmac_queueCfg_t queueCfg;

    if (PON_GEMPORT_DIRECTION_DS == pPriQ->dir)
        return omci_SetDsPriQ(pPriQ);

    memset(&queue,0,sizeof(rtk_ponmac_queue_t));
    memset(&queueCfg,0,sizeof(rtk_ponmac_queueCfg_t));

    queue.queueId = omci_getTcontQid(pPriQ->tcontId,pPriQ->queueId);
    queue.schedulerId = pPriQ->tcontId;
    /*Start to add queue*/
    queueCfg.type = (PQ_POLICY_STRICT_PRIORITY == pPriQ->scheduleType) ? STRICT_PRIORITY : WFQ_WRR_PRIORITY;
    queueCfg.cir =
        (pPriQ->cir > gPlatformDb.maxQueueRate) ? gPlatformDb.maxQueueRate : pPriQ->cir;
    queueCfg.pir =
        ((pPriQ->cir == 0 && pPriQ->pir == 0) ||
            pPriQ->pir > gPlatformDb.maxQueueRate) ? gPlatformDb.maxQueueRate : pPriQ->pir;
    queueCfg.egrssDrop = DISABLED;
    queueCfg.weight = pPriQ->weight;
    /*add new queue first*/
    if((ret = rtk_ponmac_queue_add(&queue,&queueCfg))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PriQ add failed, queueId %d, tcontId %d",pPriQ->queueId,pPriQ->tcontId);
        return OMCI_ERR_FAILED;
    }

    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    // invoke priority remap by dp marking
    omci_SetPriRemapByDpMarking(pPriQ->dpMarking);

    return OMCI_ERR_OK;

}


static int pf_rtl96xx_ClearPriQueue(OMCI_PRIQ_ts *pPriQ)
{
    int ret;
    rtk_ponmac_queue_t  queue;

    memset(&queue,0,sizeof(rtk_ponmac_queue_t));

    queue.queueId = omci_getTcontQid(pPriQ->tcontId,pPriQ->queueId);
    queue.schedulerId = pPriQ->tcontId;

    if((ret = rtk_ponmac_queue_del(&queue))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PriQ del failed, queueId %d, tcontId %d",pPriQ->queueId,pPriQ->tcontId);
        return OMCI_ERR_FAILED;
    }

    return OMCI_ERR_OK;

}

static int
omci_cfg_us_dp_stag_acl(unsigned int op, l2_service_t *pL2srv, unsigned int uniMask)
{
    dp_stag_acl_entry_t *pEntry = NULL;
    unsigned int        *pAclIdBitmap = NULL;
    unsigned int        i;

    if (!pL2srv)
        return OMCI_ERR_FAILED;

    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    if (!(pAclIdBitmap = (unsigned int *)kzalloc(sizeof(unsigned int) * (gPlatformDb.aclNum / (sizeof(unsigned int) * 8)), GFP_KERNEL)))
        return OMCI_ERR_FAILED;

    if (op)
    {
        if (!(pEntry = dpStagAcl_entry_find(pL2srv->usDpStreamId)))
        {
            // create
            // add asci acl rule
            if (OMCI_ERR_OK == omci_set_stag_acl_by_dp_marking(pL2srv->usDpMarking, uniMask, pAclIdBitmap))
            {
                if (OMCI_ERR_OK != dpStagAcl_entry_add(
                                        pL2srv->usDpStreamId, pL2srv->usDpMarking,
                                        uniMask, pAclIdBitmap))
                {
                    // del asci acl rule
                    for (i = 0; i < gPlatformDb.aclNum; i++)
                    {
                        if (pAclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
                            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG,rtk_acl_igrRuleEntry_del(i));
                    }

                    kfree(pAclIdBitmap);
                    return OMCI_ERR_FAILED;
                }

                if (!(pEntry = dpStagAcl_entry_find(pL2srv->usDpStreamId)))
                {
                    kfree(pAclIdBitmap);
                    return OMCI_ERR_FAILED;
                }
            }
        }
        else
        {
            for (i = 0; i < gPlatformDb.aclNum; i++)
            {
                if (pEntry->pAclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(i));
                }
            }
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, omci_set_stag_acl_by_dp_marking(pL2srv->usDpMarking, uniMask, pEntry->pAclIdBitmap));
        }
    }
    else
    {
        //delete
        if (!(pEntry = dpStagAcl_entry_find(pL2srv->usDpStreamId)))
        {
            kfree(pAclIdBitmap);
            return OMCI_ERR_FAILED;
        }

        memcpy(pAclIdBitmap, pEntry->pAclIdBitmap, ((gPlatformDb.aclNum / (sizeof(unsigned int) * 8)) * 4));

        for (i = 0; i < gPlatformDb.aclNum; i++)
        {
            if (pAclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(i));
            }
        }

        if (OMCI_ERR_FAILED == dpStagAcl_entry_del(pL2srv->usDpStreamId))
        {
            kfree(pAclIdBitmap);
            return OMCI_ERR_FAILED;
        }
    }

    kfree(pAclIdBitmap);
    return OMCI_ERR_OK;
}

static int
omci_DeleteUsDpCf(uint32 portId, l2_service_t *pL2srv)
{
    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    if (pL2srv->usDpStreamId != pL2srv->usStreamId)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_cfgEntry_del(pL2srv->pUsDpCfIndex[portId]));
        _RemoveUsedCfIndex(pL2srv->pUsDpCfIndex[portId]);
    }

    omci_cfg_us_dp_stag_acl(FALSE, pL2srv, pL2srv->uniMask);
    return OMCI_ERR_OK;
}

static int omci_MaintainVlanTable(void)
{
    int ret;
    l2_service_t            *pL2Entry = NULL;
    struct list_head        *next = NULL, *tmp=NULL;
    rtk_portmask_t          *pMember, *pUntag;
    rtk_portmask_t          allPortMask;
    rtk_vlan_t              vid;
    int                     i;
    struct list_head        *pGroup    = NULL;
    struct list_head        *pTmpGroup = NULL;
    struct list_head        *pEntry    = NULL;
    struct list_head        *pTmpEntry = NULL;
    dsAggregated_group_t    *pGroupEntry = NULL;
    dsAggregated_entry_t    *pEntryData  = NULL;

    pMember = kzalloc(sizeof(rtk_portmask_t) * 4096, GFP_KERNEL);
    pUntag  = kzalloc(sizeof(rtk_portmask_t) * 4096, GFP_KERNEL);

    if( (pMember == NULL) || (pUntag == NULL) )
    {
        if(pMember != NULL)
            kfree(pMember);
        if(pUntag != NULL)
            kfree(pUntag);

        return OMCI_ERR_FAILED;
    }

    /* VLAN default configuration */
    if((ret = rtk_vlan_init()) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] VLAN Init Fail, ret = %d", __LINE__, ret);

    if((ret = rtk_vlan_vlanFunctionEnable_set(DISABLED)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Disable VLAN filter Fail, ret = %d", __LINE__, ret);

    if((ret = rtk_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Configure Reserved VID Fail, ret = %d", __LINE__, ret);

    /* Set all UNI port PVID to 0 */
    if((ret = rtk_switch_allPortMask_set(&allPortMask)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] set all portmask fail, ret = %d", __LINE__, ret);

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(0));
    if((ret = rtk_vlan_port_set(0, &allPortMask, &allPortMask)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Set VID 0 Fail, ret = %d", __LINE__, ret);

    for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
    {
        if((ret = rtk_vlan_portPvid_set(i, 0))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Set PVID at port %d Fail", __LINE__, i);
            kfree(pMember);
            kfree(pUntag);
            return OMCI_ERR_FAILED;
        }
    }

    /* Scan all L2 entry */
    list_for_each_safe(next, tmp, &gPlatformDb.l2Head)
    {
        pL2Entry = list_entry(next,l2_service_t,list);

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if (i == gPlatformDb.ponPort)
                continue;

            if(pL2Entry->uniMask & (1 << i))
            {
                if(pL2Entry->rule.filterMode == VLAN_OPER_MODE_FORWARD_ALL)
                {
                    /* Transparent Mode: Set this UNI port into all 4K VLAN (0~4094) */
                    for(vid = 0; vid <= 4094; vid++)
                    {
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }
                }
                else if(pL2Entry->rule.filterMode == VLAN_OPER_MODE_EXTVLAN)
                {
                    /* Action */
                    if((pL2Entry->rule.filterRule.filterCtagMode == VLAN_FILTER_NO_TAG) &&
                       (pL2Entry->rule.cTagAct.vlanAct == VLAN_ACT_ADD) )
                    {
                        /* Add Ctag VID to US untagged packet: add this UNI port to default VLAN as an untagged port */
                        vid = pL2Entry->rule.cTagAct.assignVlan.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] |= (0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }
                    else if((pL2Entry->rule.filterRule.filterStagMode == VLAN_FILTER_NO_TAG) &&
                            (pL2Entry->rule.sTagAct.vlanAct == VLAN_ACT_ADD) )
                    {
                        /* Add Stag VID to US untagged packet: add this UNI port to default VLAN as an untagged port */
                        vid = pL2Entry->rule.sTagAct.assignVlan.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] |= (0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }
                    else if( (pL2Entry->rule.cTagAct.vlanAct == VLAN_ACT_MODIFY) && (pL2Entry->rule.cTagAct.vidAct == VID_ACT_ASSIGN) )
                    {
                        /* Translate to new Ctag VID for US tagged packet: add this UNI port to default VLAN as an tagged port */
                        vid = pL2Entry->rule.cTagAct.assignVlan.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }
                    else if( (pL2Entry->rule.sTagAct.vlanAct == VLAN_ACT_MODIFY) && (pL2Entry->rule.sTagAct.vidAct == VID_ACT_ASSIGN) )
                    {
                        /* Translate to new Stag VID for US tagged packet: add this UNI port to default VLAN as an tagged port */
                        vid = pL2Entry->rule.sTagAct.assignVlan.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }

                    /* Filter Stag */
                    if( (pL2Entry->rule.filterRule.filterStagMode & VLAN_FILTER_VID) || (pL2Entry->rule.filterRule.filterStagMode & VLAN_FILTER_TCI) )
                    {
                        /* Due to SVLAN filtering can't be disabled at some chip, Add UNI port to filtered SVLAN as an tagged port */
                        vid = pL2Entry->rule.filterRule.filterSTag.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }

                    /* Filter Ctag */
                    if( (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_VID) || (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_TCI) )
                    {
                        /* Add UNI port to filtered CVLAN as an tagged port */
                        vid = pL2Entry->rule.filterRule.filterCTag.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }

                    /* Filter only PRI */
                    if( (pL2Entry->rule.filterRule.filterStagMode & VLAN_FILTER_PRI) || (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_PRI) )
                    {
                        /* Care only Priority => Don't care VID => add UNI port to all 4K VLAN (0 ~ 4094) */
                        for(vid = 0; vid <= 4094; vid++)
                        {
                            pMember[vid].bits[0] |= (0x0001 << i);
                            pUntag[vid].bits[0] &= ~(0x0001 << i);
                            pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                        }
                    }
                }
                else if( (pL2Entry->rule.filterMode == VLAN_OPER_MODE_FILTER_SINGLETAG) &&
                    (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_VID) &&
                    (pL2Entry->rule.sTagAct.vlanAct == VLAN_ACT_TRANSPARENT) &&
                    (pL2Entry->rule.cTagAct.vlanAct == VLAN_ACT_TRANSPARENT) )
                {
                    /* Filter a VID tagged packet and transparent this tagged: Add this UNI port to filtered VLAN as an tagged port */
                    vid = pL2Entry->rule.filterRule.filterCTag.vid;
                    pMember[vid].bits[0] |= (0x0001 << i);
                    pUntag[vid].bits[0] &= ~(0x0001 << i);
                    pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                }
            }
        }
    }

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_untagAction_set(SVLAN_ACTION_SVLAN, 0));

    /* Set all VLAN configuration */
    for(vid = 0; vid <= 4094; vid++)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(vid));

        if((ret = rtk_vlan_port_set(vid, &pMember[vid], &pUntag[vid]))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Configure VLAN %d Fail", __LINE__, vid);
            kfree(pMember);
            kfree(pUntag);
            return OMCI_ERR_FAILED;
        }
    }

    kfree(pMember);
    kfree(pUntag);

    /* SP2C entry */
    /* uc Aggregated VLAN entry */
    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.ucAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if ((pEntryData->pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED) && (pEntryData->pXlateVID[i].vid != OMCI_XLATEVID_UNTAG))
                {
                    if ((ret = rtk_svlan_sp2c_add(pEntryData->outerVID, i, pEntryData->pXlateVID[i].vid)) != RT_ERR_OK)
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Add SP2C Entry Fail, SVID = %d, dst port = %d, CVID = %d, retVal = 0x%X", pEntryData->outerVID, i, pEntryData->pXlateVID[i].vid, ret);
                }
            }
        }
    }

    /* mb Aggregated VLAN entry */
    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.mbAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if ((pEntryData->pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED) && (pEntryData->pXlateVID[i].vid != OMCI_XLATEVID_UNTAG) )
                {
                    if ((ret = rtk_svlan_sp2c_add(pEntryData->outerVID, i, pEntryData->pXlateVID[i].vid)) != RT_ERR_OK)
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Add SP2C Entry Fail, SVID = %d, dst port = %d, CVID = %d, retVal = 0x%X", pEntryData->outerVID, i, pEntryData->pXlateVID[i].vid, ret);
                }
            }
        }
    }

    return OMCI_ERR_OK;
}

/* if ruleType == OMCI_XLATEVID_RULE_SP2C, uniPort is UNI mask and pAggr_cf_id is pointer of only one CF rule index */
/* if ruleType == OMCI_XLATEVID_RULE_LUT,  uniPort is UNI port and pAggr_cf_id is pointer of CF rules for all UNI   */
static int omci_set_vlan_aggregate_cf(unsigned int ruleType,
                                      unsigned int convertToDs,
                                      OMCI_VLAN_OPER_ts *pRule,
                                      unsigned int ds_flow_id,
                                      unsigned int uniPort,
                                      vlanAggregatedXlateInfo_t *pXlateVID,
                                      unsigned int highestRuleLevel,
                                      unsigned int *pAggr_cf_id,
                                      int DsAclIdx)
{
    rtk_classify_cfg_t  cfg;
    OMCI_VLAN_OPER_ts   dsRule;
    omci_rule_pri_t     rule_pri;
    unsigned int        systemTpid;
    int                 ret = OMCI_ERR_OK;
    unsigned int        targetUniMask = 0x0;
    unsigned int        i;
    unsigned int        cfIdx;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
    if ((ret = rtk_svlan_tpidEntry_get(0, &systemTpid)) != RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Get SVLAN TPID Fail, Line:%d", __LINE__);
        return OMCI_ERR_FAILED;
    }

    if (convertToDs)
        omci_SetUsRuleInvert2Ds(&dsRule, pRule);
    else
        memcpy(&dsRule, pRule, sizeof(OMCI_VLAN_OPER_ts));

    if (0x8100 == systemTpid)
    {
        if (dsRule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG)
        {
            /* Copy Ctag Filter to Stag and remove ctag filter */
            memcpy(&dsRule.filterRule.filterSTag, &dsRule.filterRule.filterCTag, sizeof(OMCI_VLAN_ts));
            dsRule.filterRule.filterStagMode = dsRule.filterRule.filterCtagMode;
            memset(&dsRule.filterRule.filterCTag, 0x0, sizeof(OMCI_VLAN_ts));
            dsRule.filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;

            if (dsRule.outStyle.outTagNum < 2)
            {
                if (VLAN_ACT_NON == dsRule.sTagAct.vlanAct ||
                    VLAN_ACT_TRANSPARENT == dsRule.sTagAct.vlanAct)
                {
                    memcpy(&dsRule.sTagAct, &dsRule.cTagAct, sizeof(OMCI_VLAN_ACT_ts));
                    memset(&dsRule.cTagAct, 0x0, sizeof(OMCI_VLAN_ACT_ts));
                }
            }
            else
            {
                if ((VLAN_ACT_ADD != dsRule.cTagAct.vlanAct && VLAN_ACT_MODIFY != dsRule.cTagAct.vlanAct) &&
                    (VLAN_ACT_ADD == dsRule.sTagAct.vlanAct || VLAN_ACT_MODIFY == dsRule.sTagAct.vlanAct) &&
                    !(dsRule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG))
                {
                    memcpy(&dsRule.cTagAct.assignVlan, &dsRule.filterRule.filterSTag, sizeof(OMCI_VLAN_ts));
                    dsRule.cTagAct.vlanAct = VLAN_ACT_MODIFY;
                    dsRule.cTagAct.vidAct = VID_ACT_ASSIGN;
                    if (dsRule.cTagAct.assignVlan.pri > 7)
                    {
                        dsRule.cTagAct.priAct = PRI_ACT_COPY_OUTER;
                    }
                }
            }
        }

        /* Handle filtering double tag without care vid and pbit, treat the number of tag is over than 2 */
        if ((dsRule.filterRule.filterStagMode & VLAN_FILTER_CARE_TAG) &&
            (dsRule.filterRule.filterCtagMode & VLAN_FILTER_CARE_TAG) &&
            (dsRule.outStyle.outTagNum > 2))
        {
            memset(&dsRule.filterRule.filterSTag, 0x0, sizeof(OMCI_VLAN_ts));
            dsRule.filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
        }
    }

    /* Setup CF rule */
    memset(&cfg, 0, sizeof(rtk_classify_cfg_t));
    cfg.invert = 0;
    cfg.valid = ENABLED;
    cfg.direction = CLASSIFY_DIRECTION_DS;
    if(RTL9607C_CHIP_ID == gPlatformDb.chipId)
                    cfg.templateIdx = 1;

    if(ruleType == OMCI_XLATEVID_RULE_SP2C)
    {
        /* Calculate SP2C UNI mask */
        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if (pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED)
                targetUniMask |= (0x0001 << i);
        }
    }
    else
        targetUniMask = (0x0001 << uniPort);

    /* Setup CF action */
    if ((ret = omci_SetClassifyDsAct(&dsRule, targetUniMask, &cfg.act.dsAct)) != OMCI_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Set ds cf action fail! Line:%d", __LINE__);
        return ret;
    }

    /* Setup CF rule */
    if ((ret = omci_SetClassifyDsRule(&dsRule, pRule, ds_flow_id, &cfg, &rule_pri, DsAclIdx)) != OMCI_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Set ds cf rule fail! Line:%d", __LINE__);
        return ret;
    }

    if(ruleType == OMCI_XLATEVID_RULE_LUT)
    {
        /* Don't need to do force forward because LUT CF rule already filter destination port */
        cfg.act.dsAct.uniAct = CLASSIFY_DS_UNI_ACT_NOP;
        cfg.act.dsAct.uniMask.bits[0] = 0x0;

        /* Filter destination UNI */
        uni.classify_pattern.uni.value = uniPort;
        uni.classify_pattern.uni.mask = 0x7;
        if( (ret = rtk_classify_field_add(&cfg, &uni)) != RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Set ds cf rule with UNI fail! Line:%d", __LINE__);
            return OMCI_ERR_FAILED;
        }
    }

    /* Make rule_pri of this VLAN aggregated CF is higher than original rule */
    if(ruleType == OMCI_XLATEVID_RULE_SP2C)
        rule_pri.rule_level = highestRuleLevel + 1;
    else
        rule_pri.rule_level = highestRuleLevel + 2;

    /* Get empty CF index */
    if(VLAN_FILTER_ETHTYPE & dsRule.filterRule.filterCtagMode)
    {
        if (OMCI_ERR_OK != _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &cfIdx))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_ETH_FILTER);
            return OMCI_ERR_FAILED;
        }
    }
    else if(((RTL9601B_CHIP_ID == gPlatformDb.chipId) || (APOLLOMP_CHIP_ID == gPlatformDb.chipId)) &&
            (VLAN_OPER_MODE_EXTVLAN == dsRule.filterMode) &&
            ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & dsRule.filterRule.filterStagMode) &&
            ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & dsRule.filterRule.filterCtagMode))
    {
        if (OMCI_ERR_OK != _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &cfIdx))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_VEIP_FAST);
            return OMCI_ERR_FAILED;
        }
    }
    else
    {
        if (OMCI_ERR_OK != _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &cfIdx))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_COMM);
            return OMCI_ERR_FAILED;
        }
    }

    /* Assign CF index */
    cfg.index = cfIdx;

    if(ruleType == OMCI_XLATEVID_RULE_SP2C)
    {
        /* for SP2C action, only RL6266 is configured at cAct, others should be configured at cVidAct */
        if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
        {
            cfg.act.dsAct.cAct      = CLASSIFY_DS_CACT_TRANSLATION_SP2C;
            cfg.act.dsAct.cTagVid   = 0;
        }
        else
        {
            cfg.act.dsAct.cAct      = CLASSIFY_DS_CACT_ADD_CTAG_8100;
            cfg.act.dsAct.cVidAct   = CLASSIFY_DS_VID_ACT_TRANSLATION_SP2C;
            cfg.act.dsAct.cTagVid   = 0;
        }

        *pAggr_cf_id = cfIdx;
    }
    else
    {
        /* CVID Action = LUT */
        cfg.act.dsAct.cAct      = CLASSIFY_DS_CACT_ADD_CTAG_8100;
        cfg.act.dsAct.cVidAct   = CLASSIFY_DS_VID_ACT_FROM_LUT;
        cfg.act.dsAct.cTagVid   = 0;

        pAggr_cf_id[uniPort] = cfIdx;
    }

    if (0x8100 == systemTpid && dsRule.outStyle.outTagNum < 2)
    {
        cfg.act.dsAct.csAct     = CLASSIFY_DS_CSACT_DEL_STAG;
    }
    // TBD double tag when global svlan tpid is 0x8100

    /* Add CF Rule */
    if ((ret = rtk_classify_cfgEntry_add(&cfg)) != RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add cf ds rule %d failed, ret=%d", cfg.index, ret);
        _classf_show_field(&cfg);
        return OMCI_ERR_FAILED;
    }

    /* Save CF rule to database */
    _saveCfCfgToDb(cfg.index, &cfg);

    return OMCI_ERR_OK;
}

static void omci_l2_vlan_aggregate_cf_delAll(void)
{
    struct list_head        *pGroup    = NULL;
    struct list_head        *pTmpGroup = NULL;
    struct list_head        *pEntry    = NULL;
    struct list_head        *pTmpEntry = NULL;
    dsAggregated_group_t    *pGroupEntry = NULL;
    dsAggregated_entry_t    *pEntryData  = NULL;
    unsigned int            i;
    int                     retVal;

    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.ucAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            /* Delete SP2C CF Rule */
            if (pEntryData->cfIdx != OMCI_UNUSED_CF)
            {
                if ((retVal = rtk_classify_cfgEntry_del(pEntryData->cfIdx)) != RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Delete original Aggregated VLAN CF rule (index:%d) Fail, retVal = 0x%X", pEntryData->cfIdx, retVal);
                }
                else
                {
                    /* Remove Used CF Index */
                    _RemoveUsedCfIndex(pEntryData->cfIdx);
                }
            }

            /* Delete LUT CF Rule */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (pEntryData->pXlateVID[i].XlateMode == OMCI_XLATEVID_TWO_MORE)
                {
                    if ((retVal = rtk_classify_cfgEntry_del(pEntryData->pLutCfIdx[i])) != RT_ERR_OK)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Delete original Aggregated VLAN CF rule (index:%d) Fail, retVal = 0x%X", pEntryData->pLutCfIdx[i], retVal);
                    }
                    else
                    {
                        /* Remove Used CF Index */
                        _RemoveUsedCfIndex(pEntryData->pLutCfIdx[i]);
                    }
                }
            }

            list_del(&pEntryData->list);
            kfree(pEntryData->pXlateVID);
            kfree(pEntryData->pLutCfIdx);
            kfree(pEntryData);
        }

        INIT_LIST_HEAD(&(pGroupEntry->dsAggregatedHead));
        list_del(&pGroupEntry->list);
        kfree(pGroupEntry);
    }
    INIT_LIST_HEAD(&gPlatformDb.ucAggregatedGroupHead);
}

static void omci_mb_vlan_aggregate_cf_delAll(void)
{
    struct list_head        *pGroup    = NULL;
    struct list_head        *pTmpGroup = NULL;
    struct list_head        *pEntry    = NULL;
    struct list_head        *pTmpEntry = NULL;
    dsAggregated_group_t    *pGroupEntry = NULL;
    dsAggregated_entry_t    *pEntryData  = NULL;
    int                     retVal;

    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.mbAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            /* Delete SP2C CF Rule */
            if ((retVal = rtk_classify_cfgEntry_del(pEntryData->cfIdx)) != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Delete original Aggregated VLAN CF rule (index:%d) Fail, retVal = 0x%X", pEntryData->cfIdx, retVal);
            }
            else
            {
                /* Remove Used CF Index */
                _RemoveUsedCfIndex(pEntryData->cfIdx);
            }

            list_del(&pEntryData->list);
            kfree(pEntryData->pXlateVID);
            kfree(pEntryData);
        }

        INIT_LIST_HEAD(&(pGroupEntry->dsAggregatedHead));
        list_del(&pGroupEntry->list);
        kfree(pGroupEntry);
    }
    INIT_LIST_HEAD(&gPlatformDb.mbAggregatedGroupHead);
}

static void omci_vlan_aggregate_fill_outerVID(unsigned int *pOuterVID, OMCI_VLAN_OPER_ts *pDsVlanRule)
{
    if ((pOuterVID== NULL) || (pDsVlanRule == NULL))
        return;

    /* filter Stag => outer VID is SVID, filter Ctag => outer VID is CVID,  */
    if ((VLAN_FILTER_VID & pDsVlanRule->filterRule.filterStagMode) ||
        (VLAN_FILTER_TCI & pDsVlanRule->filterRule.filterStagMode))
    {
        *pOuterVID = pDsVlanRule->filterRule.filterSTag.vid;
    }
    else if ((VLAN_FILTER_VID & pDsVlanRule->filterRule.filterCtagMode) ||
             (VLAN_FILTER_TCI & pDsVlanRule->filterRule.filterCtagMode))
    {
        *pOuterVID = pDsVlanRule->filterRule.filterCTag.vid;
    }
    else if ((VLAN_FILTER_NO_TAG & pDsVlanRule->filterRule.filterCtagMode) &&
             (VLAN_FILTER_NO_TAG & pDsVlanRule->filterRule.filterStagMode))
    {
        /* No S-tag & no C-tag => filter untag packet */
        *pOuterVID = OMCI_XLATEVID_UNTAG;
    }
    else
    {
        /* This service flow filters nothing, return */
        *pOuterVID = OMCI_XLATEVID_NOT_USED;
        return;
    }
}

static void omci_vlan_aggregate_fill_xlateVID(vlanAggregatedXlateInfo_t *pXlateVID,
                                              OMCI_VLAN_OPER_ts *pDsVlanRule,
                                              unsigned int outerVID,
                                              unsigned int uniMask)
{
    unsigned int xlate_cvid = OMCI_XLATEVID_UNTAG;
    unsigned int port;

    if ((pXlateVID == NULL) || (pDsVlanRule == NULL))
        return;

    if (VLAN_ACT_ADD == pDsVlanRule->cTagAct.vlanAct || VLAN_ACT_MODIFY == pDsVlanRule->cTagAct.vlanAct)
    {
        switch (pDsVlanRule->cTagAct.vidAct)
        {
            case VID_ACT_ASSIGN:
                xlate_cvid = pDsVlanRule->cTagAct.assignVlan.vid;
                break;
            case VID_ACT_COPY_INNER:
            case VID_ACT_COPY_OUTER:
            case VID_ACT_TRANSPARENT:
                xlate_cvid = outerVID;
                break;
            default:
                break;
        }
    }

    if (VLAN_ACT_NON == pDsVlanRule->cTagAct.vlanAct || VLAN_ACT_TRANSPARENT == pDsVlanRule->cTagAct.vlanAct)
    {
        xlate_cvid = outerVID;
    }

    if (VLAN_ACT_REMOVE == pDsVlanRule->cTagAct.vlanAct)
    {
        xlate_cvid = OMCI_XLATEVID_UNTAG;
    }

    for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
    {
        if (uniMask & (0x0001 << port))
        {
            /* One VID, Mode = SP2C, more than one VID, Mode = LUT */
            if (pXlateVID[port].XlateMode == OMCI_XLATEVID_NOT_USED)
                pXlateVID[port].XlateMode = OMCI_XLATEVID_ONLY_ONE;
            else
                pXlateVID[port].XlateMode = OMCI_XLATEVID_TWO_MORE;

            /* if more than one VID, we record untag first, than smallest VID */
            if ( (xlate_cvid == OMCI_XLATEVID_UNTAG) || (pXlateVID[port].vid > xlate_cvid) )
                pXlateVID[port].vid = xlate_cvid;
        }
    }
}

static void omci_vlan_aggregate_sp2c_delAll(void)
{
    struct list_head        *pGroup    = NULL;
    struct list_head        *pTmpGroup = NULL;
    struct list_head        *pEntry    = NULL;
    struct list_head        *pTmpEntry = NULL;
    dsAggregated_group_t    *pGroupEntry = NULL;
    dsAggregated_entry_t    *pEntryData  = NULL;
    int                     i;
    int                     retVal;

    /* uc Aggregated VLAN entry */
    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.ucAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if ((pEntryData->pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED) && (pEntryData->pXlateVID[i].vid != OMCI_XLATEVID_UNTAG))
                {
                    if ((retVal = rtk_svlan_sp2c_del(pEntryData->outerVID, i)) != RT_ERR_OK)
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Delete SP2C Entry Fail, SVID = %d, dst port = %d, retVal = 0x%X", pEntryData->outerVID, i, retVal);
                }
            }
        }
    }

    /* mb Aggregated VLAN entry */
    list_for_each_safe(pGroup, pTmpGroup, &(gPlatformDb.mbAggregatedGroupHead))
    {
        pGroupEntry = list_entry(pGroup, dsAggregated_group_t, list);

        list_for_each_safe(pEntry, pTmpEntry, &(pGroupEntry->dsAggregatedHead))
        {
            pEntryData = list_entry(pEntry, dsAggregated_entry_t, list);

            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if ((pEntryData->pXlateVID[i].XlateMode != OMCI_XLATEVID_NOT_USED) && (pEntryData->pXlateVID[i].vid != OMCI_XLATEVID_UNTAG))
                {
                    if ((retVal = rtk_svlan_sp2c_del(pEntryData->outerVID, i)) != RT_ERR_OK)
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Delete SP2C Entry Fail, SVID = %d, dst port = %d, retVal = 0x%X", pEntryData->outerVID, i, retVal);
                }
            }
        }
    }
}

static int omci_mb_vlan_aggregate_proc(void)
{
    struct list_head        *cur_next  = NULL;
    struct list_head        *cur_tmp   = NULL;
    mbcast_service_t        *cur       = NULL;
    struct list_head        *tar_next  = NULL;
    struct list_head        *tar_tmp   = NULL;
    mbcast_service_t        *tar       = NULL;
    unsigned int            aggrUniMask;
    int                     retVal;
    unsigned int            outerVID;
    vlanAggregatedXlateInfo_t *pXlateVID;
    unsigned int            setupAggVlanCf;
    unsigned int            i;
    unsigned int            cf_id;
    unsigned int            highestRuleLevel;

    if ((pXlateVID = (vlanAggregatedXlateInfo_t *) kzalloc(sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1), GFP_KERNEL)) == NULL)
        return OMCI_ERR_FAILED;

    /* Delete all exist SP2C entry */
    omci_vlan_aggregate_sp2c_delAll();

    /* Delete all exist L2 Aggregated VLAN CF rule & clear database*/
    omci_mb_vlan_aggregate_cf_delAll();

    /* Serach all service flow to find out any aggregated service */
    list_for_each_safe(cur_next, cur_tmp, &gPlatformDb.mbcastHead)
    {
        cur = list_entry(cur_next, mbcast_service_t, list);
        aggrUniMask = cur->uniMask;
        highestRuleLevel = gPlatformDb.cfRule[cur->dsCfIndex].rulePri.rule_level;
        setupAggVlanCf = FALSE;

        if (cur->rule.filterMode != VLAN_OPER_MODE_FORWARD_ALL)
        {
            /* fill Outer & Translation VID database by current flow */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                pXlateVID[i].XlateMode = OMCI_XLATEVID_NOT_USED;
                pXlateVID[i].vid = 0;
            }

            omci_vlan_aggregate_fill_outerVID(&outerVID, &(cur->rule));
            omci_vlan_aggregate_fill_xlateVID(pXlateVID, &(cur->rule), outerVID, cur->uniMask);

            list_for_each_safe(tar_next, tar_tmp, &gPlatformDb.mbcastHead)
            {
                tar = list_entry(tar_next, mbcast_service_t, list);

                /* Compare 2 flows only when they are in the same DS SID and current flow is not VLAN_OPER_MODE_FORWARD_ALL */
                if ((cur->dsStreamId == tar->dsStreamId) && (cur->index != tar->index))
                {
                    if (omci_is_same_rule_by_ds_vlan_filter(PON_GEMPORT_DIRECTION_DS, &(cur->rule), &(tar->rule)) == TRUE)
                    {
                        /* Same DS VLAN Filter */
                        /* Setup Aggregated VLAN rule only if           */
                        /* 1. Target rule is VLAN_OPER_MODE_FORWARD_ALL */
                        /* 2. Current rule index < Target rule index    */
                        if ( (tar->rule.filterMode == VLAN_OPER_MODE_FORWARD_ALL) || (cur->index < tar->index) )
                        {
                            aggrUniMask |= tar->uniMask;
                            setupAggVlanCf = TRUE;

                            /* Store highest rule level */
                            if (gPlatformDb.cfRule[tar->dsCfIndex].rulePri.rule_level > highestRuleLevel)
                                highestRuleLevel = gPlatformDb.cfRule[tar->dsCfIndex].rulePri.rule_level;

                            /* fill Translation VID database by target flow */
                            omci_vlan_aggregate_fill_xlateVID(pXlateVID, &(tar->rule), outerVID, tar->uniMask);
                        }
                        else if (cur->index > tar->index)
                        {
                            /* Always setup aggregated VLAN CF via lowest index service flow */
                            setupAggVlanCf = FALSE;
                            break;
                        }
                    }
                }
            }
        }

        if (setupAggVlanCf == TRUE)
        {
            /* Setup Aggregated VLAN CF */
            if ((retVal = omci_set_vlan_aggregate_cf(OMCI_XLATEVID_RULE_SP2C, FALSE, &(cur->rule), cur->dsStreamId, aggrUniMask, pXlateVID, highestRuleLevel, &cf_id, cur->dsAclIndex)) != OMCI_ERR_OK)
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_vlan_aggregate_cf Fail, retVal = 0x%X", retVal);

            /* Insert into database */
            if (retVal == OMCI_ERR_OK)
            {
                if ((retVal = omci_set_ds_aggregated_list_by_srvId(TRUE, PON_GEMPORT_DIRECTION_DS, cur->dsStreamId, cur->index, aggrUniMask, cf_id, outerVID, pXlateVID)) != OMCI_ERR_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_ds_aggregated_list_by_srvId Fail, retVal = 0x%X", retVal);
            }
        }
    }

    kfree(pXlateVID);
    return OMCI_ERR_OK;
}

static int omci_l2_vlan_aggregate_proc(void)
{
    struct list_head        *cur_next  = NULL;
    struct list_head        *cur_tmp   = NULL;
    l2_service_t            *cur       = NULL;
    struct list_head        *tar_next  = NULL;
    struct list_head        *tar_tmp   = NULL;
    l2_service_t            *tar       = NULL;
    unsigned int            aggrUniMask;
    unsigned int            setupAggVlanCf;
    unsigned int            cf_id;
    int                     retVal;
    OMCI_VLAN_OPER_ts       curDsVlanRule;
    OMCI_VLAN_OPER_ts       tarDsVlanRule;
    unsigned int            outerVID;
    vlanAggregatedXlateInfo_t *pXlateVID;
    unsigned int            *pLutCfIdx;
    unsigned int            i;
    unsigned int            highestRuleLevel;

    /* Allocate Translation VID & LUT CF Index memory space */
    pXlateVID = (vlanAggregatedXlateInfo_t *) kzalloc(sizeof(vlanAggregatedXlateInfo_t) * (gPlatformDb.allPortMax + 1), GFP_KERNEL);
    pLutCfIdx = (unsigned int *) kzalloc(sizeof(unsigned int) * (gPlatformDb.allPortMax + 1), GFP_KERNEL);

    if ( (pXlateVID == NULL) || (pLutCfIdx == NULL) )
    {
        if (pXlateVID != NULL)
            kfree(pXlateVID);
        if (pLutCfIdx != NULL)
            kfree(pLutCfIdx);

        return OMCI_ERR_FAILED;
    }

    /* Delete all exist SP2C entry */
    omci_vlan_aggregate_sp2c_delAll();

    /* Delete all exist L2 Aggregated VLAN CF rule & clear database*/
    omci_l2_vlan_aggregate_cf_delAll();

    /* Serach all service flow to find out any aggregated service */
    list_for_each_safe(cur_next, cur_tmp, &gPlatformDb.l2Head)
    {
        cur = list_entry(cur_next, l2_service_t, list);
        aggrUniMask = cur->uniMask;
        highestRuleLevel = gPlatformDb.cfRule[cur->dsCfIndex].rulePri.rule_level;
        setupAggVlanCf = FALSE;

        if (cur->rule.filterMode != VLAN_OPER_MODE_FORWARD_ALL)
        {
            /* Convert current flow to DS for further compare */
            omci_SetUsRuleInvert2Ds(&curDsVlanRule, &(cur->rule));

            /* fill Outer & Translation VID database by current flow */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                pXlateVID[i].XlateMode = OMCI_XLATEVID_NOT_USED;
                pXlateVID[i].vid = 0;
                pLutCfIdx[i] = OMCI_UNUSED_CF;
            }

            omci_vlan_aggregate_fill_outerVID(&outerVID, &curDsVlanRule);
            omci_vlan_aggregate_fill_xlateVID(pXlateVID, &curDsVlanRule, outerVID, cur->uniMask);

            list_for_each_safe(tar_next, tar_tmp, &gPlatformDb.l2Head)
            {
                tar = list_entry(tar_next, l2_service_t, list);
                omci_SetUsRuleInvert2Ds(&tarDsVlanRule, &(tar->rule));

                /* Compare 2 flows only when they are in the same DS SID and current flow is not VLAN_OPER_MODE_FORWARD_ALL */
                if ((cur->dsStreamId == tar->dsStreamId) && (cur->index != tar->index))
                {
                    if (omci_is_same_rule_by_ds_vlan_filter(PON_GEMPORT_DIRECTION_DS, &curDsVlanRule, &tarDsVlanRule) == TRUE)
                    {
                        /* Same DS VLAN Filter */
                        /* Setup Aggregated VLAN rule only if           */
                        /* 1. Target rule is VLAN_OPER_MODE_FORWARD_ALL */
                        /* 2. Current rule index < Target rule index    */
                        if ( (tar->rule.filterMode == VLAN_OPER_MODE_FORWARD_ALL) || (cur->index < tar->index) )
                        {
                            aggrUniMask |= tar->uniMask;
                            setupAggVlanCf = TRUE;

                            /* Store highest rule level */
                            if (gPlatformDb.cfRule[tar->dsCfIndex].rulePri.rule_level > highestRuleLevel)
                                highestRuleLevel = gPlatformDb.cfRule[tar->dsCfIndex].rulePri.rule_level;

                            /* fill Translation VID database by target flow */
                            omci_vlan_aggregate_fill_xlateVID(pXlateVID, &tarDsVlanRule, outerVID, tar->uniMask);
                        }
                        else if (cur->index > tar->index)
                        {
                            /* Always setup aggregated VLAN CF via lowest index service flow */
                            setupAggVlanCf = FALSE;
                            break;
                        }
                    }
                }
            }
        }

        if (setupAggVlanCf == TRUE)
        {
            if (RTL9601B_CHIP_ID != gPlatformDb.chipId)
            {
                /* Setup Aggregated VLAN CF */
                if ((retVal = omci_set_vlan_aggregate_cf(OMCI_XLATEVID_RULE_SP2C, TRUE, &(cur->rule), cur->dsStreamId, aggrUniMask, pXlateVID, highestRuleLevel, &cf_id, cur->dsAclIndex)) != OMCI_ERR_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_vlan_aggregate_cf Fail, retVal = 0x%X", retVal);
            }
            else if (RTL9601B_CHIP_ID == gPlatformDb.chipId)
            {
                /* RTL9601B is single UNI ONT, no SP2C cf_id is needed. */
                cf_id = OMCI_UNUSED_CF;
                retVal = OMCI_ERR_OK;
            }

            /* Insert into database */
            if (retVal == OMCI_ERR_OK)
            {
                if ((retVal = omci_set_ds_aggregated_list_by_srvId(TRUE, PON_GEMPORT_DIRECTION_BI, cur->dsStreamId, cur->index, aggrUniMask, cf_id, outerVID, pXlateVID)) != OMCI_ERR_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_ds_aggregated_list_by_srvId Fail, retVal = 0x%X", retVal);
            }
            /* Setup Aggregated VLAN LUT CF */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (pXlateVID[i].XlateMode == OMCI_XLATEVID_TWO_MORE)
                {
                    if ((retVal = omci_set_vlan_aggregate_cf(OMCI_XLATEVID_RULE_LUT, TRUE, &(cur->rule), cur->dsStreamId, i, pXlateVID, highestRuleLevel, pLutCfIdx, cur->dsAclIndex)) != OMCI_ERR_OK)
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_vlan_aggregate_cf Fail, retVal = 0x%X", retVal);

                    /* Update all LUT CF Index to database */
                    if (retVal == OMCI_ERR_OK)
                    {
                        if ((retVal = omci_set_ds_aggregated_list_lutcfidx_by_srvId(PON_GEMPORT_DIRECTION_BI, cur->dsStreamId, cur->index, pLutCfIdx)) != OMCI_ERR_OK)
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_set_ds_aggregated_list_lutcfidx_by_srvId Fail, retVal = 0x%X", retVal);
                    }
                }
            }
        }
    }

    kfree(pXlateVID);
    kfree(pLutCfIdx);
    return OMCI_ERR_OK;
}

static void omci_ext_filter_latch_acl_delAll(void)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    ext_filter_service_t    *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &(gPlatformDb.efHead))
    {
        pEntryData = list_entry(pEntry, ext_filter_service_t, list);

        if (OMCI_UNUSED_ACL == pEntryData->rule_id)
            continue;

        // remove hw rule
        if (RT_ERR_OK != rtk_acl_igrRuleEntry_del(pEntryData->rule_id))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del latch acl rule %x failed", pEntryData->rule_id);
        }
        else
        {
            // remove software rule
            list_del(&pEntryData->list);
            kfree(pEntryData);
        }
    }
}

static uint32 omci_is_last_latch_acl(int acl_index)
{
    ext_filter_service_t* ptr = efServ_entry_find_by_rule_index(acl_index);

    if (ptr && ptr->ref_cnt)
    {
        ptr->ref_cnt--;
        if (0 == ptr->ref_cnt)
        {
            list_del(&ptr->list);
            kfree(ptr);
            return TRUE;
        }
    }
    return FALSE;
}

static int pf_rtl96xx_DeactiveBdgConn(int servId)
{
    int ret;
    int i;
    unsigned int systemTpid;
    l2_service_t *pL2Entry;
    veip_service_t *pVeipEntry;
    mbcast_service_t *pMBEntry = NULL;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_svlan_tpidEntry_get(0, &systemTpid));

    pMBEntry = mbcastServ_entry_find(servId);

    if (pMBEntry)
    {
        /*omci_set_ds_aggregated_list_by_srvId(
                                             FALSE, PON_GEMPORT_DIRECTION_DS,
                                             pMBEntry->dsStreamId, (int)(pMBEntry->index), 0);*/
        // invoke ds pkt bcaster
        if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_APPLY))
        {
            if (OMCIDRV_FEATURE_ERR_FAIL ==
                (ret = omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_DELRULE, servId)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "add ds tag operation fail, ret = %d", ret);
            }
        }


        if((ret = rtk_classify_cfgEntry_del(pMBEntry->dsCfIndex))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del ds cf rule %x failed", pMBEntry->dsCfIndex);
            return OMCI_ERR_FAILED;
        }

        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);
        pMBEntry->dsCfIndex = OMCI_UNUSED_CF;

        if (pMBEntry->dsAclIndex != OMCI_UNUSED_ACL && omci_is_last_latch_acl(pMBEntry->dsAclIndex))
        {
            if((ret = rtk_acl_igrRuleEntry_del(pMBEntry->dsAclIndex))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del ds acl rule %x failed", pMBEntry->dsAclIndex);
            }
        }

        mbcastServ_entry_del(pMBEntry->index);

        if (RTL9601B_CHIP_ID != gPlatformDb.chipId)
        {
            if((ret = omci_mb_vlan_aggregate_proc()) != OMCI_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_mb_vlan_aggregate_proc Fail, ret = 0x%X", ret);
            }
        }
    }

    pL2Entry = l2Serv_entry_find(servId);

    if(pL2Entry)
    {

        /*for us rule*/
        if(PON_GEMPORT_DIRECTION_US == pL2Entry->dir ||
            PON_GEMPORT_DIRECTION_BI == pL2Entry->dir)
        {

            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++){
                if(pL2Entry->pUsCfIndex[i] != OMCI_UNUSED_CF)
                {
                    if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                    {
                        omci_delMemberPortBySvlan(pL2Entry->pUsCfIndex[i], systemTpid);
                    }
                    omci_delServicePort(systemTpid, pL2Entry->pUsCfIndex[i]);

                    omci_DeleteUsDpCf(i, pL2Entry);
                    if((ret = rtk_classify_cfgEntry_del(pL2Entry->pUsCfIndex[i]))!=RT_ERR_OK)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del us cf rule %x failed", pL2Entry->pUsCfIndex[i]);
                        return OMCI_ERR_FAILED;
                    }
                    _RemoveUsedCfIndex(pL2Entry->pUsCfIndex[i]);
                }
            }

            if (pL2Entry->usAclIndex != OMCI_UNUSED_ACL && omci_is_last_latch_acl(pL2Entry->usAclIndex))
            {
                if((ret = rtk_acl_igrRuleEntry_del(pL2Entry->usAclIndex))!=RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del us acl rule %x failed", pL2Entry->usAclIndex);
                }
            }
        }

        /*for ds rule*/
        if(PON_GEMPORT_DIRECTION_BI == pL2Entry->dir)
        {



            if((ret = rtk_classify_cfgEntry_del(pL2Entry->dsCfIndex))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del ds cf rule %x failed", pL2Entry->dsCfIndex);
                return OMCI_ERR_FAILED;
            }
            _RemoveUsedCfIndex(pL2Entry->dsCfIndex);

            if (pL2Entry->dsAclIndex != OMCI_UNUSED_ACL && omci_is_last_latch_acl(pL2Entry->dsAclIndex))
            {
                if((ret = rtk_acl_igrRuleEntry_del(pL2Entry->dsAclIndex))!=RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del ds acl rule %x failed", pL2Entry->dsAclIndex);
                }
            }
        }
        /*delete from list*/
        l2Serv_entry_del(servId);

        /* Process VLAN aggregated CF */
        if((ret = omci_l2_vlan_aggregate_proc()) != OMCI_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_l2_vlan_aggregate_proc Fail, ret = 0x%X", ret);
        }
    }

    pVeipEntry = veipServ_entry_find(servId);
    if (pVeipEntry)
    {
        int     wanIdx;

        for (wanIdx = 0; wanIdx < gPlatformDb.intfNum; wanIdx++)
        {
            if (!(pVeipEntry->wanIfBitmap & (1 << wanIdx)))
                continue;

            // del veip rule
            if (OMCI_ERR_OK != omcidrv_delVeipRule(pVeipEntry->index, wanIdx))
                continue;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "del veip rule %u for wan %d", pVeipEntry->index, wanIdx);

            // modify wan status
            omcidrv_setWanStatusByIfIdx(wanIdx, FALSE);

            pVeipEntry->wanIfBitmap &= ~(1 << wanIdx);
        }

        // delete veip entry
        veipServ_entry_del(servId);
    }
#if !defined(FPGA_DEFINED)
    /* Maintain VLAN table */
    omci_MaintainVlanTable();
#endif
    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DeactiveBdgConn_fpga(int servId)
{
    int             ret;
    unsigned int    systemTpid;
    l2_service_t    *pL2Entry = NULL;

    if (RTL9603D_CHIP_ID == gPlatformDb.chipId)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_svlan_tpidEntry_get(0, &systemTpid));

        pL2Entry = l2Serv_entry_find(servId);

        if (!pL2Entry)
            return OMCI_ERR_OK;

        if (PON_GEMPORT_DIRECTION_BI != pL2Entry->dir)
            return OMCI_ERR_OK;

        //omci_DeleteUsDpCf(i, pL2Entry);

        if (pL2Entry->usAclIndex != OMCI_UNUSED_ACL)
        {
            if((ret = rtk_acl_igrRuleEntry_del(pL2Entry->usAclIndex))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del us acl rule %x failed", pL2Entry->usAclIndex);
            }
        }

        if (pL2Entry->dsAclIndex != OMCI_UNUSED_ACL)
        {
            if((ret = rtk_acl_igrRuleEntry_del(pL2Entry->dsAclIndex))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del ds acl rule %x failed", pL2Entry->dsAclIndex);
            }
        }
        /*delete from list*/
        l2Serv_entry_del(servId);

    }
    else
        return pf_rtl96xx_DeactiveBdgConn(servId);

    return OMCI_ERR_OK;
}


static int
omci_CreateUsDpCf(uint32 portId,
        l2_service_t *pL2srv, rtk_classify_cfg_t *orgCfCfg, uint8 isEthTypeFilter, omci_rule_pri_t rule_pri, unsigned int uniMask)
{
    int                 ret;
    rtk_classify_cfg_t  newCfCfg;

    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    if (!pL2srv || !orgCfCfg)
        return OMCI_ERR_FAILED;

    // nothing has to be done for normal flow
    if (pL2srv->usDpStreamId == pL2srv->usStreamId)
        return OMCI_ERR_OK;

    // copy classify content
    memcpy(&newCfCfg, orgCfCfg, sizeof(rtk_classify_cfg_t));

    // allocate dp cf index
    if (PQ_DROP_COLOUR_DEI_MARKING == pL2srv->usDpMarking)
    {
        // for dp connection, set dei bit to 1
        if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
        {

            //dei need to use Pattern 1, Template 1
            newCfCfg.templateIdx = 1;

            dei.classify_pattern.dei.value = 1;
            dei.classify_pattern.dei.mask = 0x1;
            rtk_classify_field_add(&newCfCfg, &dei);


            if (OMCI_ERR_OK !=
                    _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2srv->pUsDpCfIndex[portId]))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "allocate us cf type %d for dp fail!", PF_CF_TYPE_VEIP_FAST);

                return OMCI_ERR_FAILED;
            }
        }
        else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
        {
            newCfCfg.templateIdx = 2;
            dei.classify_pattern.dei.value = 1;
            dei.classify_pattern.dei.mask = 0x1;
            rtk_classify_field_add(&newCfCfg, &dei);
            if (OMCI_ERR_OK !=
                    _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2srv->pUsDpCfIndex[portId]))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "allocate us cf type %d for dp fail!", PF_CF_TYPE_VEIP_FAST);

                return OMCI_ERR_FAILED;
            }
        }
        else
        {
            dei.classify_pattern.dei.value = 1;
            dei.classify_pattern.dei.mask = 0x1;
            rtk_classify_field_add(&newCfCfg, &dei);

            // dei classify needs to assign cf index to 0 ~ 63
            if (OMCI_ERR_OK !=
                    _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &pL2srv->pUsDpCfIndex[portId]))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "allocate us cf type %d for dp fail!", PF_CF_TYPE_VEIP_FAST);

                return OMCI_ERR_FAILED;
            }
        }
    }
    else
    {
        // for dp connection, set internal pri to 0
        interPri.classify_pattern.interPri.value = 0;
        interPri.classify_pattern.interPri.mask = 0x7;
        rtk_classify_field_add(&newCfCfg, &interPri);

        if (isEthTypeFilter)
        {
            if (OMCI_ERR_OK !=
                    _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pL2srv->pUsDpCfIndex[portId]))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "allocate us cf type %d for dp fail!", PF_CF_TYPE_L2_ETH_FILTER);

                return OMCI_ERR_FAILED;
            }
        }
        else
        {
            if (OMCI_ERR_OK !=
                    _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2srv->pUsDpCfIndex[portId]))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "allocate us cf type %d for dp fail!", PF_CF_TYPE_L2_COMM);

                return OMCI_ERR_FAILED;
            }
        }
    }
    newCfCfg.index = pL2srv->pUsDpCfIndex[portId];

    // assign dp flow id
    newCfCfg.act.usAct.sidQid = pL2srv->usDpStreamId;

    // configure dp cf
    if (RT_ERR_OK != (ret = rtk_classify_cfgEntry_add(&newCfCfg)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add us dp cf %u (sid %u, dpsid %d) failed, return code %d",
            newCfCfg.index, pL2srv->usStreamId, pL2srv->usDpStreamId, ret);
    }
    _saveCfCfgToDb(newCfCfg.index, &newCfCfg);

    if (PQ_DROP_COLOUR_DEI_MARKING == pL2srv->usDpMarking)
    {
        /*for normal connection, set dei bit to 0*/
        if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
        {
            orgCfCfg->templateIdx = 1;
            dei.classify_pattern.dei.value = 0;

        }
        else if (RTL9607C_CHIP_ID == gPlatformDb.chipId)
        {
            orgCfCfg->templateIdx = 2;
            dei.classify_pattern.dei.value = 0;
        }
        else
        {
            // for normal connection, set dei bit to 0
            //  but it's actually useless...
            //  dei classify needs to reassign cf index to 0 ~ 63
            dei.classify_pattern.dei.value = 0;
        }

    }
    else
    {
        // for normal connection, set internal pri to 1
        interPri.classify_pattern.interPri.value = 1;
    }
    omci_cfg_us_dp_stag_acl(TRUE, pL2srv, uniMask);
    return OMCI_ERR_OK;
}

static int
omci_CreateDsDefaultDropCf(void)
{
    int                 ret;
    rtk_classify_cfg_t  classifyCfg;

    memset(&classifyCfg, 0, sizeof(rtk_classify_cfg_t));
    classifyCfg.index = gPlatformDb.cfTotalNum - 1;
    classifyCfg.invert = 0;
    classifyCfg.valid = ENABLED;
    classifyCfg.direction = CLASSIFY_DIRECTION_DS;
    classifyCfg.act.dsAct.uniAct = CLASSIFY_DS_UNI_ACT_FORCE_FORWARD;

    if (RT_ERR_OK != (ret = rtk_classify_cfgEntry_add(&classifyCfg)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add downstream default drop cf fail, return code %d", ret);
        return OMCI_ERR_FAILED;
    }
    _saveCfCfgToDb(classifyCfg.index, &classifyCfg);
    return OMCI_ERR_OK;
}

static int
omci_changeVlanRuleByTpid8100(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int                 res = FALSE;
    rtk_enable_t        enableB = DISABLED;
    rtk_port_t          port = 0;
    OMCI_VLAN_OPER_ts   *pVlanRule = NULL;
    unsigned int        uniPortMask;

    if (!pBridgeRule)
        return OMCI_ERR_OK;

    pVlanRule = &pBridgeRule->vlanRule;

    uniPortMask = (((unsigned int)(pBridgeRule->uniMask)) & (~(1 << gPlatformDb.ponPort)));
    /* Due to SVLAN TPID change to 0x8100 */

    /* Handle filtering single tag Ctag need to change to Stag */
    if (pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG)
    {
        /* Copy Ctag Filter to Stag and remove ctag filter */
        memcpy(&pVlanRule->filterRule.filterSTag, &pVlanRule->filterRule.filterCTag, sizeof(OMCI_VLAN_ts));
        pVlanRule->filterRule.filterStagMode = pVlanRule->filterRule.filterCtagMode;
        memset(&pVlanRule->filterRule.filterCTag, 0x0, sizeof(OMCI_VLAN_ts));
        pVlanRule->filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;

        if (pVlanRule->outStyle.outTagNum < 2)
        {
            /*When Stag Act is no-act, Copy Ctag Act to Stag*/
            if (VLAN_ACT_NON == pVlanRule->sTagAct.vlanAct ||
                VLAN_ACT_TRANSPARENT == pVlanRule->sTagAct.vlanAct)
            {
                memcpy(&pVlanRule->sTagAct, &pVlanRule->cTagAct, sizeof(OMCI_VLAN_ACT_ts));
                memset(&pVlanRule->cTagAct, 0x0, sizeof(OMCI_VLAN_ACT_ts));
            }
        }
        else
        {
            if ((VLAN_ACT_ADD != pVlanRule->cTagAct.vlanAct &&
                VLAN_ACT_MODIFY != pVlanRule->cTagAct.vlanAct) &&
                (VLAN_ACT_ADD == pVlanRule->sTagAct.vlanAct ||
                VLAN_ACT_MODIFY == pVlanRule->sTagAct.vlanAct) &&
                !(pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG))
            {
                memcpy(&pVlanRule->cTagAct.assignVlan, &pVlanRule->filterRule.filterSTag, sizeof(OMCI_VLAN_ts));
                pVlanRule->cTagAct.vlanAct = VLAN_ACT_MODIFY;
                pVlanRule->cTagAct.vidAct = VID_ACT_ASSIGN;
                if (pVlanRule->cTagAct.assignVlan.pri > 7)
                {
                    pVlanRule->cTagAct.priAct = PRI_ACT_COPY_OUTER;
                }
            }
        }
        /*Keep Ctag ethernet filter if orignal ethernet filter is enable*/
        if(pVlanRule->filterRule.filterStagMode & VLAN_FILTER_ETHTYPE)
            pVlanRule->filterRule.filterCtagMode |= VLAN_FILTER_ETHTYPE;

        res = TRUE;
    }

    /* Handle filtering double tag without care vid and pbit, treat the number of tag is over than 2 */
    if ((pVlanRule->filterRule.filterStagMode & VLAN_FILTER_CARE_TAG) &&
        (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_CARE_TAG) &&
        (pVlanRule->outStyle.outTagNum > 2))
    {
        memset(&pVlanRule->filterRule.filterSTag, 0x0, sizeof(OMCI_VLAN_ts));
        pVlanRule->filterRule.filterStagMode = VLAN_FILTER_NO_TAG;
        res = TRUE;
    }

    //due to deactive bridge connection does NOT restore tpid to 0x88A8 if there is a rule withtpid 0x8100
    if (res && APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        for (port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_get(port, &enableB));
            if ((1 << port) & uniPortMask)
                enableB = ENABLED;
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, enableB));
        }
    }

    return OMCI_ERR_OK;
}


static uint32
omci_get_acl_field_id_by_template_index_field_type(uint32 idx, rtk_acl_field_type_t type)
{
    rtk_acl_template_t aclTemplate;
    uint32 i;

    aclTemplate.index = idx;
    if(RT_ERR_OK != rtk_acl_template_get(&aclTemplate))
        return RTK_MAX_NUM_OF_ACL_RULE_FIELD;

    for (i = 0; i < RTK_MAX_NUM_OF_ACL_RULE_FIELD; i++)
    {
        if (aclTemplate.fieldType[i] == type)
        {
            return i;
        }
    }
    return RTK_MAX_NUM_OF_ACL_RULE_FIELD;
}

/* RTL9601B:                                 */
/*    US: No ACL rule                        */
/*    DS: Filter GEM port                    */
/* APOLLOMP:                                 */
/*    US: Filter Inner tag                   */
/*    DS: Filter Inner tag + GEM port        */
/* Others:                                   */
/*    US: No ACL rule                        */
/*    DS: No ACL rule                        */
static int omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION dir, OMCI_VLAN_OPER_ts *pVlanRule, unsigned int uniMask, unsigned int flowId, unsigned int *pAclIndex)
{
    int ret;
    int aclIdx = OMCI_UNUSED_ACL;
    unsigned int maxAclIdx;
    rtk_acl_ingress_entry_t aclRule;
    rtk_acl_field_t aclField_cTag;
    rtk_acl_field_t aclField_gemPort;
    omci_ext_filter_entry_t filter_entry;
    ext_filter_service_t*   p_ext_filter_rule;
    uint32                  field_id;
    BOOL                    findAvailAcl = FALSE;

    memset(&filter_entry, 0, sizeof(omci_ext_filter_entry_t));
    filter_entry.templateIdx = 1;


    if((APOLLOMP_CHIP_ID != gPlatformDb.chipId) && (RTL9601B_CHIP_ID != gPlatformDb.chipId))
        return OMCI_ERR_OK;

    if((RTL9601B_CHIP_ID == gPlatformDb.chipId) && (dir == PON_GEMPORT_DIRECTION_US))
        return OMCI_ERR_OK;

    /* APOLLOMP and RTL9601B doesn't support to filter 2 tags + SID. */
    /* If this service flow needs to filter double tags + SID => create ACL rule to filter SID. (filter inner tag in APOLLOMP also). */
    /* Setup ACL rule before create CF rule */
    if((VLAN_OPER_MODE_EXTVLAN == pVlanRule->filterMode) &&
       ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanRule->filterRule.filterStagMode) &&
       ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanRule->filterRule.filterCtagMode))
    {
        /*Find un-used ACL index*/
        maxAclIdx = gPlatformDb.aclActNum > gPlatformDb.aclNum ? gPlatformDb.aclActNum : gPlatformDb.aclNum;
        for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx; aclIdx--)
        {
            aclRule.index = aclIdx;
            if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
            {
                if (!aclRule.valid){
                    findAvailAcl = TRUE;
                    break;
            }
        }
    }
    }

    if (findAvailAcl == FALSE)
        return OMCI_ERR_FAILED;

    /* Setup ACL rule */
    osal_memset(&aclRule, 0x00, sizeof(rtk_acl_ingress_entry_t));
    osal_memset(&aclField_cTag, 0x00, sizeof(rtk_acl_field_t));
    osal_memset(&aclField_gemPort, 0x00, sizeof(rtk_acl_field_t));

    if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        aclField_cTag.fieldType = ACL_FIELD_CTAG;
        field_id = omci_get_acl_field_id_by_template_index_field_type(filter_entry.templateIdx, ACL_FIELD_CTAG);

        if ( (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI) || (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_VID) )
        {
            aclField_cTag.fieldUnion.l2tag.vid.value = pVlanRule->filterRule.filterCTag.vid;
            aclField_cTag.fieldUnion.l2tag.vid.mask = 0xFFF;

            if (field_id < RTK_MAX_NUM_OF_ACL_RULE_FIELD)
            {
                filter_entry.filters.field[field_id].value = aclField_cTag.fieldUnion.l2tag.vid.value;
                filter_entry.filters.field[field_id].mask = aclField_cTag.fieldUnion.l2tag.vid.mask;
            }
        }

        if ( (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI) || (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI) )
        {
            aclField_cTag.fieldUnion.l2tag.pri.value = pVlanRule->filterRule.filterCTag.pri;
            aclField_cTag.fieldUnion.l2tag.pri.mask = 0x7;

            if (field_id < RTK_MAX_NUM_OF_ACL_RULE_FIELD)
            {
                filter_entry.filters.field[field_id].value |= aclField_cTag.fieldUnion.l2tag.pri.value;
                filter_entry.filters.field[field_id].mask |= 0x7000;
            }
        }
        aclField_cTag.next = NULL;
        rtk_acl_igrRuleField_add(&aclRule, &aclField_cTag);
    }

    if (dir == PON_GEMPORT_DIRECTION_DS)
    {
        aclField_gemPort.fieldType = ACL_FIELD_GEMPORT;
        aclField_gemPort.fieldUnion.data.value = flowId;
        aclField_gemPort.fieldUnion.data.mask = 0xFFFF;
        aclField_gemPort.next = NULL;
        rtk_acl_igrRuleField_add(&aclRule, &aclField_gemPort);

        field_id = omci_get_acl_field_id_by_template_index_field_type(filter_entry.templateIdx, ACL_FIELD_GEMPORT);

        if (field_id < RTK_MAX_NUM_OF_ACL_RULE_FIELD)
        {
            filter_entry.filters.field[field_id].value = aclField_gemPort.fieldUnion.data.value;
            filter_entry.filters.field[field_id].mask = aclField_gemPort.fieldUnion.data.mask;
        }
    }

    /* Add ACL rule */
    aclRule.index = aclIdx;
    aclRule.valid = ENABLED;
    aclRule.templateIdx = 0x1;
    aclRule.act.enableAct[ACL_IGR_INTR_ACT] = ENABLED;
    aclRule.act.aclLatch = ENABLED;

    if (dir == PON_GEMPORT_DIRECTION_US)
        aclRule.activePorts.bits[0] = uniMask;
    else
        aclRule.activePorts.bits[0] = (0x0001 << gPlatformDb.ponPort);

    // maintain software database for latch acl rule
    filter_entry.activePortMask = aclRule.activePorts.bits[0];

    p_ext_filter_rule = efServ_entry_find(&filter_entry);

    if (p_ext_filter_rule)
    {
        //update ref_cnt
        p_ext_filter_rule->ref_cnt += 1;
        *pAclIndex = p_ext_filter_rule->rule_id;
        return *pAclIndex;
    }
    else
    {
        //add ext filtering rule
        efServ_entry_add(&filter_entry, aclIdx);
    }

    if( RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add acl rule %u failed, return code %d", aclIdx, ret);
        return OMCI_ERR_FAILED;
    }

    *pAclIndex = (unsigned int)aclIdx;
    return OMCI_ERR_OK;
}
static int pf_rtl96xx_SetL2Rule(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int ret = OMCI_ERR_OK;
    rtk_classify_cfg_t  classifyCfg, oldCfg;
    OMCI_VLAN_OPER_ts   dsRule;
    l2_service_t *pL2Entry;
    int i;
    unsigned int minUsIndex, systemTpid;
    omci_rule_pri_t rule_pri;
    int usAclIndex = OMCI_UNUSED_ACL;
    int dsAclIndex = OMCI_UNUSED_ACL;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    pL2Entry = l2Serv_entry_find(pBridgeRule->servId);

    if(!pL2Entry)
    {
        if (OMCI_ERR_OK != (ret = l2Serv_entry_add(pBridgeRule)))
        {
            return OMCI_ERR_FAILED;
        }

        pL2Entry = l2Serv_entry_find(pBridgeRule->servId);
        if(!pL2Entry)
        {
            return OMCI_ERR_FAILED;
        }
    }

    pL2Entry->usStreamId = pBridgeRule->usFlowId;
    pL2Entry->usDpStreamId = pBridgeRule->usDpFlowId;
    pL2Entry->usDpMarking = pBridgeRule->usDpMarking;
    pL2Entry->dsStreamId = pBridgeRule->dsFlowId;

    /*Get SVLAN TPID*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_svlan_tpidEntry_get(0, &systemTpid));

    if (0x8100 == systemTpid)
    {
        omci_changeVlanRuleByTpid8100(pBridgeRule);
    }

    if(pBridgeRule->dir==PON_GEMPORT_DIRECTION_US || pBridgeRule->dir==PON_GEMPORT_DIRECTION_BI)
    {
        if ((pL2Entry->uniMask != pBridgeRule->uniMask) &&
            (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)) == gPlatformDb.uniPortMask &&
            pL2Entry->uniMask)
        {
            /* delete the same classify rule exlcue uni  */
            minUsIndex = OMCI_UNUSED_CF;
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (i == gPlatformDb.ponPort)
                    continue;

                if((pL2Entry->uniMask & (1 << i)) && (pL2Entry->isCfg == 1) && (OMCI_UNUSED_CF != pL2Entry->pUsCfIndex[i]))
                {
                    memset(&oldCfg,0,sizeof(rtk_classify_cfg_t));
                    oldCfg.index = pL2Entry->pUsCfIndex[i];
                    minUsIndex = (oldCfg.index <= minUsIndex ? oldCfg.index : minUsIndex);
                    if(RT_ERR_OK == rtk_classify_cfgEntry_get(&oldCfg))
                    {
                        omci_DeleteUsDpCf(i, pL2Entry);
                        if((ret = rtk_classify_cfgEntry_del(oldCfg.index))!=RT_ERR_OK)
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del us cf rule %d failed", oldCfg.index);
                            return OMCI_ERR_FAILED;
                        }
                        _RemoveUsedCfIndex(oldCfg.index);
                    }
                }
            }
            /* add new one classify rule without filter uni */
            gPlatformDb.cfRule[minUsIndex].isCfg = 1;
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                pL2Entry->pUsCfIndex[i] = minUsIndex;
            }
            memset(&classifyCfg, 0, sizeof(rtk_classify_cfg_t));
            classifyCfg.index = minUsIndex;
            classifyCfg.invert = 0;
            classifyCfg.valid = ENABLED;
            classifyCfg.direction = CLASSIFY_DIRECTION_US;

            if(RTL9607C_CHIP_ID == gPlatformDb.chipId)
                classifyCfg.templateIdx = 1;

            omci_SetClassifyUsAct(&pBridgeRule->vlanRule, pBridgeRule->usFlowId, &classifyCfg.act.usAct);

            memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
            omci_SetClassifyUsRule(&pBridgeRule->vlanRule, (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)), &classifyCfg, &rule_pri, pL2Entry->usAclIndex);

            omci_CreateUsDpCf(0, pL2Entry, &classifyCfg,
                VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
                (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));
            if((ret = rtk_classify_cfgEntry_add(&classifyCfg))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] add cf us rule %x failed, ret=%d, uniMask=%d, oldservId=%u, servId=%d",
                    __LINE__, minUsIndex, ret, pBridgeRule->uniMask, pL2Entry->index, pBridgeRule->servId);
                _classf_show_field(&classifyCfg);
                return OMCI_ERR_FAILED;
            }
            _saveCfCfgToDb(classifyCfg.index, &classifyCfg);
        }
        else
        {
            /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
            /* Setup ACL rule before create CF rule */
            omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_US, &(pBridgeRule->vlanRule), pBridgeRule->uniMask, 0, &usAclIndex);

            /* add classfy rule since different rule */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if (i == gPlatformDb.ponPort)
                    continue;

                if((pBridgeRule->uniMask & (1 << i))
                    && ((pL2Entry->isCfg != 1) || (OMCI_UNUSED_CF == pL2Entry->pUsCfIndex[i])))
                {
                    memset(&classifyCfg,0,sizeof(rtk_classify_cfg_t));
                    classifyCfg.invert = 0;
                    classifyCfg.valid = ENABLED;
                    classifyCfg.direction = CLASSIFY_DIRECTION_US;
                    if(RTL9607C_CHIP_ID == gPlatformDb.chipId)
                        classifyCfg.templateIdx = 1;

                    omci_SetClassifyUsAct(&pBridgeRule->vlanRule,pBridgeRule->usFlowId,&classifyCfg.act.usAct);

                    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
                    omci_SetClassifyUsRule(&pBridgeRule->vlanRule, i, &classifyCfg, &rule_pri, usAclIndex);

                    if(VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode)
                    {
                        if (OMCI_ERR_OK !=
                                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pL2Entry->pUsCfIndex[i]))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                                "allocate us cf type %d for l2 fail!", PF_CF_TYPE_L2_ETH_FILTER);

                            return OMCI_ERR_FAILED;
                        }
                    }
                    else if(((RTL9601B_CHIP_ID == gPlatformDb.chipId) || (APOLLOMP_CHIP_ID == gPlatformDb.chipId)) &&
                        (VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode) &&
                        ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pBridgeRule->vlanRule.filterRule.filterStagMode) &&
                        ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pBridgeRule->vlanRule.filterRule.filterCtagMode))
                    {
                        if (OMCI_ERR_OK !=
                                _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &pL2Entry->pUsCfIndex[i]))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                                "allocate us cf type %d for l2 fail!", PF_CF_TYPE_VEIP_FAST);

                            return OMCI_ERR_FAILED;
                        }
                    }
                    else
                    {
                        if (OMCI_ERR_OK !=
                                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2Entry->pUsCfIndex[i]))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                                "allocate us cf type %d for l2 fail!", PF_CF_TYPE_L2_COMM);

                            return OMCI_ERR_FAILED;
                        }
                    }

                    classifyCfg.index = pL2Entry->pUsCfIndex[i];

                    omci_CreateUsDpCf(i, pL2Entry, &classifyCfg,
                        VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
                        (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));

                    if((ret = rtk_classify_cfgEntry_add(&classifyCfg))!=RT_ERR_OK)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] add cf us rule %x failed, ret=%d",
                            __LINE__, pL2Entry->pUsCfIndex[i], ret);
                        _classf_show_field(&classifyCfg);
                        return OMCI_ERR_FAILED;
                    }
                    _saveCfCfgToDb(classifyCfg.index, &classifyCfg);
                }
            }

            /* Save Us ACL index into database */
            pL2Entry->usAclIndex = usAclIndex;

            if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_BDP_00000100_APPLY) &&
               (OMCIDRV_FEATURE_ERR_FAIL ==
                omcidrv_feature_api(FEATURE_KAPI_BDP_00000100_US, pL2Entry, pBridgeRule, &gPlatformDb,
                                    (OMCI_SET_CF_US_ACT_PTR)omci_SetClassifyUsAct,
                                    (OMCI_SET_CF_US_RULE_PTR)omci_SetClassifyUsRule,
                                    (ASSIGN_NONUSED_CF_IDX_PTR)_AssignNonUsedCfIndex,
                                    (OMCI_CREATE_US_DP_CF_PTR)omci_CreateUsDpCf,
                                    (SHOW_CF_FIELD)_classf_show_field,
                                    (REMOVE_USED_CF_IDX)_RemoveUsedCfIndex,
                                    (SAVE_CFCFG_TO_DB_PTR)_saveCfCfgToDb)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");
                return OMCI_ERR_FAILED;
            }

        }
    }


    //for TR-247 test cases verify stag or send packet with stag from UTP port

    omci_SetServicePort(systemTpid, pBridgeRule->vlanRule.outStyle.outVlan.vid, pBridgeRule);

    if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        //for downstream receive packet with stag but it cannot learn in l2-table since svlan drop reason.
        /* By default, uplink port aware svlan and downstream uni forward action is set to forwarding
         * member mask(flood), so svlan member table should be configured uplink port and specfic uni
         * port as member ports. Due to disable cvlan filtering, it doesn't care vlan member table of CVID  */
        omci_SetMemberPortBySvlan(&classifyCfg, pBridgeRule, systemTpid);
    }

    if(pBridgeRule->dir==PON_GEMPORT_DIRECTION_BI)
    {
        if(pL2Entry->uniMask != pBridgeRule->uniMask)
        {
            omci_SetUsRuleInvert2Ds(&dsRule,&pBridgeRule->vlanRule);

            /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
            /* Setup ACL rule before create CF rule */
            omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_DS, &dsRule, pBridgeRule->uniMask, pBridgeRule->dsFlowId, &dsAclIndex);

            memset(&classifyCfg,0,sizeof(rtk_classify_cfg_t));
            memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

            classifyCfg.invert = 0;
            classifyCfg.valid = ENABLED;
            classifyCfg.direction = CLASSIFY_DIRECTION_DS;
            if(RTL9607C_CHIP_ID == gPlatformDb.chipId)
                classifyCfg.templateIdx = 1;

            omci_SetClassifyDsAct(&dsRule, (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)), &classifyCfg.act.dsAct);
#if !defined(FPGA_DEFINED)
            omci_SetClassifyDsCfPriAct(pBridgeRule, &classifyCfg.act.dsAct);
#endif
            omci_SetClassifyDsRule(&dsRule, &pBridgeRule->vlanRule, pBridgeRule->dsFlowId, &classifyCfg, &rule_pri, dsAclIndex);

            if ((pL2Entry->isCfg != 1) || (pL2Entry->dsCfIndex == OMCI_UNUSED_CF))
            {
                if(VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode)
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pL2Entry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                            "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_ETH_FILTER);

                        return OMCI_ERR_FAILED;
                    }
                }
                else if(((RTL9601B_CHIP_ID == gPlatformDb.chipId) || (APOLLOMP_CHIP_ID == gPlatformDb.chipId)) &&
                        (VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode) &&
                        ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & dsRule.filterRule.filterStagMode) &&
                        ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & dsRule.filterRule.filterCtagMode))
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &pL2Entry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                            "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_VEIP_FAST);

                        return OMCI_ERR_FAILED;
                    }
                }
                else
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2Entry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                            "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_COMM);

                        return OMCI_ERR_FAILED;
                    }
                }
            }

            classifyCfg.index = pL2Entry->dsCfIndex;

            if((ret = rtk_classify_cfgEntry_add(&classifyCfg))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add cf ds rule %d failed, ret=%d", pL2Entry->dsCfIndex, ret);
                _classf_show_field(&classifyCfg);
                return OMCI_ERR_FAILED;
            }
            _saveCfCfgToDb(classifyCfg.index, &classifyCfg);

            /* Save Ds ACL index into database */
            pL2Entry->dsAclIndex = dsAclIndex;
        }
    }
    pL2Entry->uniMask = pBridgeRule->uniMask;
    pL2Entry->isCfg = 1;


    return ret;
}

static int pf_rtl96xx_SetL2Rule_fpga(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int                     ret = OMCI_ERR_OK;
    //OMCI_VLAN_OPER_ts       dsRule;
    l2_service_t            *pL2Entry = NULL;
    unsigned int            systemTpid;
    int                     usAclIndex = OMCI_UNUSED_ACL;
    int                     dsAclIndex = OMCI_UNUSED_ACL;
    rtk_acl_ingress_entry_t aclRule;

    /*Get SVLAN TPID*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_svlan_tpidEntry_get(0, &systemTpid));

    if (0x8100 == systemTpid)
    {
        omci_changeVlanRuleByTpid8100(pBridgeRule);
    }

    pL2Entry = l2Serv_entry_find(pBridgeRule->servId);

    if (!pL2Entry)
    {
        // generate acl rule
        //
        // upstream
        //
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
        if (OMCI_ERR_OK != omci_findAvailAcl(&aclRule))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "cannot find available acl entry for l2 fail!");
            return OMCI_ERR_FAILED;
        }

        usAclIndex = aclRule.index;
        memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));
        aclRule.index = usAclIndex;
        aclRule.valid = ENABLED;
        aclRule.templateIdx = 0x2;
        aclRule.activePorts.bits[0] = (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort));

        omci_SetAclUsAct(&pBridgeRule->vlanRule, pBridgeRule->usFlowId, &aclRule);

        omci_SetAclUsRule(&pBridgeRule->vlanRule, (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)), &aclRule);

        /*omci_CreateUsDpCf(i, pL2Entry, &classifyCfg,
            VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
            (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));*/

        if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add us acl rule %d failed, ret=%d", usAclIndex, ret);
            //_acl_show_field(&aclRule);
            return OMCI_ERR_FAILED;
        }

        // add l2 sw db
        //
        if (OMCI_ERR_OK != (ret = l2Serv_entry_add(pBridgeRule)))
            return OMCI_ERR_FAILED;

        if (!(pL2Entry = l2Serv_entry_find(pBridgeRule->servId)))
            return OMCI_ERR_FAILED;

        pL2Entry->usStreamId = pBridgeRule->usFlowId;
        pL2Entry->usDpStreamId = pBridgeRule->usDpFlowId;
        pL2Entry->usDpMarking = pBridgeRule->usDpMarking;
        pL2Entry->dsStreamId = pBridgeRule->dsFlowId;
        pL2Entry->usAclIndex = usAclIndex;
        pL2Entry->dsAclIndex = dsAclIndex;
        pL2Entry->uniMask = pBridgeRule->uniMask;
        pL2Entry->isCfg = 1;
    }
    else
    {
        // if isCfg=0, something wrong due to the prevous configuration is failed.

        // if isCfg=1, replace original acl rule if pBridgeRule->uniMask is different and update l2 sw db
        if (pL2Entry->uniMask != pBridgeRule->uniMask)
        {
            //
            // upstream
            //

            if (pL2Entry->usAclIndex != OMCI_UNUSED_ACL)
            {
                if((ret = rtk_acl_igrRuleEntry_del(pL2Entry->usAclIndex))!=RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"del us acl rule %x failed", pL2Entry->usAclIndex);
                }
            }

            memset(&aclRule, 0, sizeof(rtk_acl_ingress_entry_t));

            aclRule.index = pL2Entry->usAclIndex;
            aclRule.valid = ENABLED;
            aclRule.templateIdx = 0x2;
            aclRule.activePorts.bits[0] = (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort));

            omci_SetAclUsAct(&pBridgeRule->vlanRule, pBridgeRule->usFlowId, &aclRule);

            omci_SetAclUsRule(&pBridgeRule->vlanRule, (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)), &aclRule);

            /*omci_CreateUsDpCf(i, pL2Entry, &classifyCfg,
                VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
                (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));*/

            if (RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add us acl rule %d failed, ret=%d", usAclIndex, ret);
                //_acl_show_field(&aclRule);
                return OMCI_ERR_FAILED;
            }

            pL2Entry->usStreamId = pBridgeRule->usFlowId;
            pL2Entry->usDpStreamId = pBridgeRule->usDpFlowId;
            pL2Entry->usDpMarking = pBridgeRule->usDpMarking;
            pL2Entry->dsStreamId = pBridgeRule->dsFlowId;
            pL2Entry->uniMask = pBridgeRule->uniMask;
            pL2Entry->isCfg = 1;
        }
    }

    return ret;
}


static int omcidrv_set_ds_tag_operation_filter_by_evlan(
        OMCI_VLAN_FILTER_ts *pFilter, tagOperation_t *pTagOp)
{
    if (!pFilter || !pTagOp)
        return OMCI_ERR_FAILED;

    // set stag filter
    if (pFilter->filterStagMode & VLAN_FILTER_NO_TAG)
        pTagOp->tagFilter.sTagMode = TAG_FILTER_MODE_NO_TAG;
    else
        pTagOp->tagFilter.sTagMode = TAG_FILTER_MODE_TAG;

    if (pFilter->filterStagMode & VLAN_FILTER_VID)
    {
        pTagOp->tagFilter.sTagMask |= TAG_FILTER_FIELD_VID;
        pTagOp->tagFilter.sTagField.tagVid = pFilter->filterSTag.vid;
    }
    if (pFilter->filterStagMode & VLAN_FILTER_PRI)
    {
        pTagOp->tagFilter.sTagMask |= TAG_FILTER_FIELD_PRI;
        pTagOp->tagFilter.sTagField.tagPri = pFilter->filterSTag.pri;
    }
    if (pFilter->filterStagMode & VLAN_FILTER_TCI)
    {
        pTagOp->tagFilter.sTagMask |= TAG_FILTER_FIELD_VID;
        pTagOp->tagFilter.sTagField.tagVid = pFilter->filterSTag.vid;
        pTagOp->tagFilter.sTagMask |= TAG_FILTER_FIELD_PRI;
        pTagOp->tagFilter.sTagField.tagPri = pFilter->filterSTag.pri;
    }
    if (pFilter->filterStagMode & VLAN_FILTER_ETHTYPE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "filter ethertype is not supported\n");
    }
    if (pFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "filter pri from dscp is not supported\n");
    }

    // set ctag filter
    if (pFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
        pTagOp->tagFilter.cTagMode = TAG_FILTER_MODE_NO_TAG;
    else
        pTagOp->tagFilter.cTagMode = TAG_FILTER_MODE_TAG;

    if (pFilter->filterCtagMode & VLAN_FILTER_VID)
    {
        pTagOp->tagFilter.cTagMask |= TAG_FILTER_FIELD_VID;
        pTagOp->tagFilter.cTagField.tagVid = pFilter->filterCTag.vid;
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
    {
        pTagOp->tagFilter.cTagMask |= TAG_FILTER_FIELD_PRI;
        pTagOp->tagFilter.cTagField.tagPri = pFilter->filterCTag.pri;
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_TCI)
    {
        pTagOp->tagFilter.cTagMask |= TAG_FILTER_FIELD_VID;
        pTagOp->tagFilter.cTagField.tagVid = pFilter->filterCTag.vid;
        pTagOp->tagFilter.cTagMask |= TAG_FILTER_FIELD_PRI;
        pTagOp->tagFilter.cTagField.tagPri = pFilter->filterCTag.pri;
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "filter ethertype is not supported\n");
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "filter pri from dscp is not supported\n");
    }

    return OMCI_ERR_OK;
}

static void omcidrv_set_ds_tag_operation_treatment_tagmode(
        OMCI_VLAN_ACT_MODE_e vlanAct, tagTreatmentMode_t *pTagMode)
{
    if (!pTagMode)
        return;

    switch (vlanAct)
    {
        case VLAN_ACT_ADD:
            *pTagMode = TAG_TREATMENT_MODE_INSERT;
            break;
        case VLAN_ACT_REMOVE:
            *pTagMode = TAG_TREATMENT_MODE_REMOVE;
            break;
        case VLAN_ACT_MODIFY:
            *pTagMode = TAG_TREATMENT_MODE_MODIFY;
            break;
        default:
            *pTagMode = TAG_TREATMENT_MODE_NOP;
            break;
    }
}

static void omcidrv_set_ds_tag_operation_treatment_vidaction(
        OMCI_VID_ACT_MODE_e vidAct, tagTreatmentVidAction_t *pVidAction)
{
    if (!pVidAction)
        return;

    switch (vidAct)
    {
        case VID_ACT_ASSIGN:
            *pVidAction = TAG_TREATMENT_VID_ACTION_INSERT;
            break;
        case VID_ACT_COPY_INNER:
            *pVidAction = TAG_TREATMENT_VID_ACTION_COPY_FROM_INNER;
            break;
        case VID_ACT_COPY_OUTER:
            *pVidAction = TAG_TREATMENT_VID_ACTION_COPY_FROM_OUTER;
            break;
        default:
            *pVidAction = TAG_TREATMENT_VID_ACTION_NOP;
            break;
    }
}

static void omcidrv_set_ds_tag_operation_treatment_priaction(
        OMCI_PRI_ACT_MODE_e priAct, tagTreatmentPriAction_t *pPriAction)
{
    if (!pPriAction)
        return;

    switch (priAct)
    {
        case PRI_ACT_ASSIGN:
            *pPriAction = TAG_TREATMENT_PRI_ACTION_INSERT;
            break;
        case PRI_ACT_COPY_INNER:
            *pPriAction = TAG_TREATMENT_PRI_ACTION_COPY_FROM_INNER;
            break;
        case PRI_ACT_COPY_OUTER:
            *pPriAction = TAG_TREATMENT_PRI_ACTION_COPY_FROM_OUTER;
            break;
        case PRI_ACT_FROM_DSCP:
            *pPriAction = TAG_TREATMENT_PRI_ACTION_DERIVE_FROM_DSCP;
            break;
        default:
            *pPriAction = TAG_TREATMENT_PRI_ACTION_NOP;
            break;
    }
}

static int omcidrv_ds_tag_operation_rule_add(OMCI_BRIDGE_RULE_ts *pDsRule)
{
    OMCI_VLAN_OPER_ts       *pVlanRule;
    OMCI_VLAN_FILTER_ts     *pFilter;
    OMCI_VLAN_ACT_ts        *pSTagAct;
    OMCI_VLAN_ACT_ts        *pCTagAct;
    tagOperation_t          tagOp;
    int32                   ret;

    if (!pDsRule)
        return OMCI_ERR_FAILED;

    // do nothing
    if (OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_APPLY))
        return OMCI_ERR_OK;

    pVlanRule = &pDsRule->vlanRule;
    pFilter = &pVlanRule->filterRule;
    pSTagAct = &pVlanRule->sTagAct;
    pCTagAct = &pVlanRule->cTagAct;
    memset(&tagOp, 0, sizeof(tagOp));

    // skip entries that is destined to drop
    if (OMCI_EXTVLAN_REMOVE_TAG_DISCARD == pVlanRule->outStyle.isDefaultRule)
        return OMCI_ERR_FAILED;

    // set filter
    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_ALL:
            tagOp.tagFilter.sTagMode = TAG_FILTER_MODE_DONT_CARE;
            tagOp.tagFilter.cTagMode = TAG_FILTER_MODE_DONT_CARE;
            break;
        case VLAN_OPER_MODE_FORWARD_UNTAG:
            tagOp.tagFilter.sTagMode = TAG_FILTER_MODE_NO_TAG;
            tagOp.tagFilter.cTagMode = TAG_FILTER_MODE_NO_TAG;
            break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
            tagOp.tagFilter.sTagMode = TAG_FILTER_MODE_DONT_CARE;
            tagOp.tagFilter.cTagMode = TAG_FILTER_MODE_TAG;
            break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            tagOp.tagFilter.sTagMode = TAG_FILTER_MODE_DONT_CARE;
            tagOp.tagFilter.cTagMode = TAG_FILTER_MODE_TAG;
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                tagOp.tagFilter.cTagMask |= TAG_FILTER_FIELD_PRI;
                tagOp.tagFilter.cTagField.tagPri = pFilter->filterCTag.pri;
            }
            break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
        case VLAN_OPER_MODE_VLANTAG_OPER:
            tagOp.tagFilter.sTagMode = TAG_FILTER_MODE_DONT_CARE;
            tagOp.tagFilter.cTagMode = TAG_FILTER_MODE_TAG;
            if (pFilter->filterCtagMode & VLAN_FILTER_VID ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                tagOp.tagFilter.cTagMask |= TAG_FILTER_FIELD_VID;
                tagOp.tagFilter.cTagField.tagVid = pFilter->filterCTag.vid;
            }
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                tagOp.tagFilter.cTagMask |= TAG_FILTER_FIELD_PRI;
                tagOp.tagFilter.cTagField.tagPri = pFilter->filterCTag.pri;
            }
            break;
        case VLAN_OPER_MODE_EXTVLAN:
            ret = omcidrv_set_ds_tag_operation_filter_by_evlan(pFilter, &tagOp);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "set ds tag operation filter by evlan fail, ret = %d", ret);
            }
            break;
        default:
            break;
    }

    // set treatment of tag mode
    omcidrv_set_ds_tag_operation_treatment_tagmode(
        pSTagAct->vlanAct, &tagOp.tagTreatment.sTagMode);
    omcidrv_set_ds_tag_operation_treatment_tagmode(
        pCTagAct->vlanAct, &tagOp.tagTreatment.cTagMode);

    // set treatment of stag action
    omcidrv_set_ds_tag_operation_treatment_vidaction(
        pSTagAct->vidAct, &tagOp.tagTreatment.sTagAction.vidAction);
    omcidrv_set_ds_tag_operation_treatment_priaction(
        pSTagAct->priAct, &tagOp.tagTreatment.sTagAction.priAction);

    // set treatment of ctag action
    omcidrv_set_ds_tag_operation_treatment_vidaction(
        pCTagAct->vidAct, &tagOp.tagTreatment.cTagAction.vidAction);
    omcidrv_set_ds_tag_operation_treatment_priaction(
        pCTagAct->priAct, &tagOp.tagTreatment.cTagAction.priAction);

    // set treatment of tag field
    tagOp.tagTreatment.sTagField.tagTpid = pSTagAct->assignVlan.tpid;
    tagOp.tagTreatment.sTagField.tagVid = pSTagAct->assignVlan.vid;
    tagOp.tagTreatment.sTagField.tagPri = pSTagAct->assignVlan.pri;
    tagOp.tagTreatment.cTagField.tagTpid = pCTagAct->assignVlan.tpid;
    tagOp.tagTreatment.cTagField.tagVid = pCTagAct->assignVlan.vid;
    tagOp.tagTreatment.cTagField.tagPri = pCTagAct->assignVlan.pri;

    // set dst port mask
    if (pDsRule->uniMask & (1 << gPlatformDb.ponPort))
        tagOp.dstPortMask = gPlatformDb.uniPortMask;
    else
        tagOp.dstPortMask = pDsRule->uniMask;

    if (OMCIDRV_FEATURE_ERR_FAIL ==
        (ret = omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_ADDRULE, &tagOp, pDsRule->servId)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "add ds tag operation fail, ret = %d", ret);
    }

    return ret;
}

static int
omcidrv_updateMcastBcastRule(mbcast_service_t *pMBEntry,
                            OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    rtk_classify_cfg_t  classifyCfg;
    int                 ret = OMCI_ERR_OK;
    omci_rule_pri_t     rule_pri;
    unsigned int        systemTpid;
    int        dsAclIndex = OMCI_UNUSED_ACL;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
    /*Get SVLAN TPID*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_svlan_tpidEntry_get(0, &systemTpid));

    if (((VLAN_FILTER_PRI & pBridgeRule->vlanRule.filterRule.filterCtagMode) ||
        (VLAN_FILTER_TCI & pBridgeRule->vlanRule.filterRule.filterCtagMode)) &&
        pBridgeRule->vlanRule.filterRule.filterCTag.pri > 7)
    {
        pBridgeRule->vlanRule.filterRule.filterCtagMode &= ~VLAN_FILTER_PRI;

        if (VLAN_FILTER_TCI == pBridgeRule->vlanRule.filterRule.filterCtagMode)
            pBridgeRule->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID;

    }

    if (((VLAN_FILTER_PRI & pBridgeRule->vlanRule.filterRule.filterStagMode) ||
        (VLAN_FILTER_TCI & pBridgeRule->vlanRule.filterRule.filterStagMode)) &&
        pBridgeRule->vlanRule.filterRule.filterSTag.pri > 7)
    {
        pBridgeRule->vlanRule.filterRule.filterStagMode &= ~VLAN_FILTER_PRI;

        if (VLAN_FILTER_TCI == pBridgeRule->vlanRule.filterRule.filterStagMode)
            pBridgeRule->vlanRule.filterRule.filterStagMode = VLAN_FILTER_VID;
    }

    if (0x8100 == systemTpid)
    {
        omci_changeVlanRuleByTpid8100(pBridgeRule);
    }

    //add/update cf rule
    if(pMBEntry->uniMask != pBridgeRule->uniMask)
    {
        // invoke ds pkt bcaster
        omcidrv_ds_tag_operation_rule_add(pBridgeRule);

        /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
        /* Setup ACL rule before create CF rule */
        omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_DS, &(pBridgeRule->vlanRule), pBridgeRule->uniMask, pBridgeRule->dsFlowId, &dsAclIndex);

        memset(&classifyCfg,0,sizeof(rtk_classify_cfg_t));

        classifyCfg.invert = 0;
        classifyCfg.valid = ENABLED;
        classifyCfg.direction = CLASSIFY_DIRECTION_DS;
        if(RTL9607C_CHIP_ID == gPlatformDb.chipId)
            classifyCfg.templateIdx = 1;

        if(pBridgeRule->uniMask & (1 << gPlatformDb.ponPort))
            omci_SetClassifyDsAct(&pBridgeRule->vlanRule, gPlatformDb.etherPortMask, &classifyCfg.act.dsAct);
        else
            omci_SetClassifyDsAct(&pBridgeRule->vlanRule, pBridgeRule->uniMask, &classifyCfg.act.dsAct);

        omci_SetClassifyDsRule(&pBridgeRule->vlanRule, &pBridgeRule->vlanRule, pBridgeRule->dsFlowId, &classifyCfg, &rule_pri, dsAclIndex);

        if ((pMBEntry->isCfg != 1) || (pMBEntry->dsCfIndex == OMCI_UNUSED_CF))
        {
            if(VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode)
            {
                if (OMCI_ERR_OK !=
                        _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pMBEntry->dsCfIndex))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "allocate ds cf type %d for mcast/bcast fail!", PF_CF_TYPE_L2_ETH_FILTER);

                    return OMCI_ERR_FAILED;
                }
            }
            else if(((RTL9601B_CHIP_ID == gPlatformDb.chipId) || (APOLLOMP_CHIP_ID == gPlatformDb.chipId)) &&
                    (VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode) &&
                    ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pBridgeRule->vlanRule.filterRule.filterStagMode) &&
                    ((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pBridgeRule->vlanRule.filterRule.filterCtagMode))
            {
                if (OMCI_ERR_OK !=
                        _AssignNonUsedCfIndex(PF_CF_TYPE_VEIP_FAST, rule_pri, &pMBEntry->dsCfIndex))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "allocate ds cf type %d for mcast/bcast fail!", PF_CF_TYPE_VEIP_FAST);

                    return OMCI_ERR_FAILED;
                }
            }
            else
            {
                if (OMCI_ERR_OK !=
                        _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pMBEntry->dsCfIndex))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "allocate ds cf type %d for mcast/bcast fail!", PF_CF_TYPE_L2_COMM);

                    return OMCI_ERR_FAILED;
                }
            }
        }

        classifyCfg.index = pMBEntry->dsCfIndex;

        if((ret = rtk_classify_cfgEntry_add(&classifyCfg))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add cf ds rule %d failed, ret=%d", pMBEntry->dsCfIndex, ret);

            _classf_show_field(&classifyCfg);

            if (OMCI_UNUSED_CF != pMBEntry->dsCfIndex)
                _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

            return OMCI_ERR_FAILED;
        }
        _saveCfCfgToDb(classifyCfg.index, &classifyCfg);

        omci_SetServicePort(systemTpid, pBridgeRule->vlanRule.outStyle.outVlan.vid, pBridgeRule);

        if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
        {
            //for downstream receive packet with stag but it cannot learn in l2-table since svlan drop reason.
            /* By default, uplink port aware svlan and downstream uni forward action is set to forwarding
             * member mask(flood), so svlan member table should be configured uplink port and specfic uni
             * port as member ports. Due to disable cvlan filtering, it doesn't care vlan member table of CVID  */
            omci_SetMemberPortBySvlan(&classifyCfg, pBridgeRule, systemTpid);
        }
    }

    pMBEntry->dsAclIndex = dsAclIndex;
    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetMBcastRule(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int ret = OMCI_ERR_OK;
    mbcast_service_t *pMBEntry;

    pMBEntry = mbcastServ_entry_find(pBridgeRule->servId);

    if(!pMBEntry)
    {
        // add entry
        if((ret = mbcastServ_entry_add(pBridgeRule->servId,
            &pBridgeRule->vlanRule, pBridgeRule->dsFlowId))!= OMCI_ERR_OK)
        {
            return OMCI_ERR_FAILED;
        }
        // get entry
        pMBEntry = mbcastServ_entry_find(pBridgeRule->servId);
        if(!pMBEntry)
        {
            return OMCI_ERR_FAILED;
        }
    }

    // invoke update not matter it's new or already exists
    if (OMCI_ERR_FAILED == (ret = omcidrv_updateMcastBcastRule(pMBEntry, pBridgeRule)))
    {
        pMBEntry->isCfg = 0;
    }
    else
    {
        pMBEntry->isCfg = 1;
    }

    pMBEntry->uniMask = pBridgeRule->uniMask;

    return ret;
}

static int pf_rtl96xx_ActiveBdgConn(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int ret = OMCI_ERR_OK;

    if(pBridgeRule==NULL)
    {
        return OMCI_ERR_OK;
    }

    if(PON_GEMPORT_DIRECTION_DS == pBridgeRule->dir)
    {
        ret = pf_rtl96xx_SetMBcastRule(pBridgeRule);

        if (RTL9601B_CHIP_ID != gPlatformDb.chipId)
        {
            if((ret = omci_mb_vlan_aggregate_proc()) != OMCI_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_mb_vlan_aggregate_proc Fail, ret = 0x%X", ret);
            }
        }
    }
    else
    {
        if(((pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)) && gDrvCtrl.devMode != OMCI_DEV_MODE_ROUTER) ||
                OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_BDP_00000100_APPLY))
        {
            ret = pf_rtl96xx_SetL2Rule(pBridgeRule);

            /* Process VLAN aggregated CF */
            if((ret = omci_l2_vlan_aggregate_proc()) != OMCI_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_l2_vlan_aggregate_proc Fail, ret = 0x%X", ret);
            }
        }

        if((pBridgeRule->uniMask & (1 << gPlatformDb.ponPort)) &&  gDrvCtrl.devMode != OMCI_DEV_MODE_BRIDGE)
            ret = pf_rtl96xx_SetVeipRule(pBridgeRule);
    }
#if !defined(FPGA_DEFINED)
    /* Maintain VLAN table */
    omci_MaintainVlanTable();
#endif
    return ret;
}

static int pf_rtl96xx_ActiveBdgConn_fpga(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int ret = OMCI_ERR_OK;

    if (RTL9603D_CHIP_ID == gPlatformDb.chipId)
    {
        if (!pBridgeRule)
            return OMCI_ERR_OK;
        // No care one-direction rule and vlan aggregate due to the number ACL entries of limitation on FPGA
        if (PON_GEMPORT_DIRECTION_BI == pBridgeRule->dir &&
            (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)))
        {
            ret = pf_rtl96xx_SetL2Rule_fpga(pBridgeRule);
        }
    }
    else
        ret = pf_rtl96xx_ActiveBdgConn(pBridgeRule);

    return ret;
}


static int pf_rtl96xx_SetSerialNum(char *serial)
{
    int ret;
    rtk_gpon_serialNumber_t sn;
    sn.vendor[0] = serial[0];
    sn.vendor[1] = serial[1];
    sn.vendor[2] = serial[2];
    sn.vendor[3] = serial[3];
    sn.specific[0] = serial[4];
    sn.specific[1] = serial[5];
    sn.specific[2] = serial[6];
    sn.specific[3] = serial[7];

    ret = rtk_gponapp_serialNumber_set(&sn);
    return ret;
}

static int pf_rtl96xx_GetSerialNum(char *serial)
{
    int ret;
    rtk_gpon_serialNumber_t sn;

    ret = rtk_gponapp_serialNumber_get(&sn);

    serial[0] = sn.vendor[0];
    serial[1] = sn.vendor[1];
    serial[2] = sn.vendor[2];
    serial[3] = sn.vendor[3];
    serial[4] = sn.specific[0];
    serial[5] = sn.specific[1];
    serial[6] = sn.specific[2];
    serial[7] = sn.specific[3];

    return ret;
}


static int pf_rtl96xx_SetGponPasswd(char *gponPwd)
{
    int ret;
    rtk_gpon_password_t gpon_password;

    memset(&gpon_password,0,sizeof(gpon_password));
    memcpy(gpon_password.password, gponPwd, RTK_GPON_PASSWORD_LEN);

    ret = rtk_gponapp_password_set(&gpon_password);

    return ret;
}

static int pf_rtl96xx_ActivateGpon(int activate)
{
    int ret;

    if(activate)
        ret = rtk_gponapp_activate(RTK_GPONMAC_INIT_STATE_O1);
    else
        ret = rtk_gponapp_deActivate();

    return ret;
}

static int32    g_dsBcGemTrapAcl = -1;

static void omci_clear_us_dp_stag_acl_cfg(int flowId)
{
    dp_stag_acl_entry_t *pDp_stag_acl = NULL;
    unsigned int        acl_idx;

    // clear all us dp rule for stag
    if (NULL != (pDp_stag_acl = dpStagAcl_entry_find(flowId)))
    {
        for (acl_idx = 0; acl_idx < gPlatformDb.aclNum; acl_idx++)
        {
            if (pDp_stag_acl->pAclIdBitmap[acl_idx / (sizeof(unsigned int) * 8)] & (1 << (acl_idx % (sizeof(unsigned int) * 8))))
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(acl_idx));
        }
        dpStagAcl_entry_del(flowId);
    }
}

static void omci_clear_ds_dp_stag_acl_cfg(int flowId)
{
    // clear all ds dp rule for stag
    flow2DsPq_entry_t       *pDsFlowCfg = NULL;
    dsDpGemStagAcl_entry_t  *pDpGemCfg  = NULL;
    unsigned int            acl_idx;

    if (!(pDsFlowCfg = flow2DsPq_entry_find(flowId)))
        return;

    if (!(pDpGemCfg = dsDpGemStagAcl_find_entry(pDsFlowCfg->gemPortId)))
        return;

    for (acl_idx = 0; acl_idx < gPlatformDb.aclNum; acl_idx++)
    {
        if (pDpGemCfg->pAclIdBitmap[acl_idx / (sizeof(unsigned int) * 8)] & (1 << (acl_idx % (sizeof(unsigned int) * 8))))
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(acl_idx));
    }

    dsDpGemStagAcl_del_entry(pDsFlowCfg->gemPortId);
}

static int pf_rtl96xx_ResetMib(void)
{
    int i,j,ret;
    rtk_gpon_usFlow_attr_t usAttr;
    rtk_gpon_dsFlow_attr_t dsAttr;
    rtk_port_t port = 0;
    rtk_qos_priSelWeight_t weight;
    rtk_rate_storm_group_ctrl_t stormCtrl;
    rtk_qos_pri2queue_t         pri2Qid;
    rtk_portmask_t allPortMask;
    rtk_qos_queue_weights_t qWeight;

    memset(&qWeight, 0x00, sizeof(rtk_qos_queue_weights_t));

    _InitUsedTcontId();

    memset(&usAttr,0,sizeof(rtk_gpon_usFlow_attr_t));
    memset(&dsAttr,0,sizeof(rtk_gpon_dsFlow_attr_t));
    usAttr.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;
    dsAttr.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;

    for(i=0;i<gPlatformDb.maxFlow;i++)
    {
        if(i!=gPlatformDb.omccFlow)
        {
            ret = rtk_gponapp_usFlow_set(i,&usAttr);
            ret = rtk_gponapp_dsFlow_set(i,&dsAttr);
            omci_clear_us_dp_stag_acl_cfg(i);
            omci_clear_ds_dp_stag_acl_cfg(i);
        }
    }

    for(i = 0; i < GROUP_MAC_PROTOCOL_NUM; i++)
    {
        for(j = 0; j < GROUP_MAC_PROTOCOL_MAX_RULE_NUM; j++)
        {
            if(groupMacAclIdx.aclIdx[i][j] != -1)
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[i][j]));
        }
    }
    memset(&groupMacAclIdx, -1, sizeof(groupMacAclIdx));


    INIT_LIST_HEAD(&gPlatformDb.dpStagAclHead);
    INIT_LIST_HEAD(&gPlatformDb.dsDpGemStagAclHead);

    // clear all uni qos info
    uni_qos_entry_delAll();

    _remove_all_flow_to_ds_pq_Info();

    // clear all ds flow queue info
    flow2DsPq_entry_delAll();

    /*remove software database*/
    if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_APPLY))
    {
        int32 clear_aclId = -1;
        if (OMCIDRV_FEATURE_ERR_FAIL ==
            (ret = omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_SET_DSBC_GEMFLOW,
                                        &gPlatformDb, UINT_MAX, &clear_aclId)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "add ds tag operation fail");
        }
        if(OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_RESET))
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() [%d] fail",__FUNCTION__,__LINE__);
    }

    omci_vlan_aggregate_sp2c_delAll();
    omci_ds_aggregated_list_flush();
    mbcastServ_entry_delAll();
    l2Serv_entry_delAll();
    veipServ_entry_delAll();
    /*remove all hw used cf rules*/
    for(i = 0; i < gPlatformDb.cfNum; i++)
    {
        ret = rtk_classify_cfgEntry_del(i);
    }
    /* reset sw used cf rule */
    for (i = 0; i < gPlatformDb.cfNum; i++)
    {
        _RemoveUsedCfIndex(i);
    }

    // remove all hw latch acl rule
    omci_ext_filter_latch_acl_delAll();

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_init());
    ret = rtk_vlan_vlanFunctionEnable_set(DISABLED);

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(0));
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &allPortMask, &allPortMask));

    for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
    {
        if((ret = rtk_vlan_portPvid_set(port, 0))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Set PVID at port %d Fail", __LINE__, port);
        }
    }

    switch(gPlatformDb.chipId)
    {
        case APOLLOMP_CHIP_ID:
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, DISABLED));
            }
            ret = rtk_svlan_servicePort_set(gPlatformDb.ponPort, ENABLED);
            break;
        case RTL9601B_CHIP_ID:
        default:
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                ret = rtk_svlan_servicePort_set(port, ENABLED);
            }
            ret = rtk_svlan_svlanFunctionEnable_set(DISABLED);
            break;
    }

    ret = rtk_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG);

    // set default scheduler type to WRR
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_schedulingType_set(RTK_QOS_WRR));

    // rise the weight of dot1q for default priority-to-queue mapping
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priSelGroup_get(0, &weight));
    weight.weight_of_dot1q = weight.weight_of_portBased;
    weight.weight_of_dscp = 0;
    if (gPlatformDb.maxPriSelWeight > weight.weight_of_dot1q)
        weight.weight_of_dot1q += 1;
    // always make the weight of acl to the highest weight we supported
    weight.weight_of_acl = gPlatformDb.maxPriSelWeight;
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priSelGroup_set(0, &weight));

    // reset 1q to internal priority remapping
    for (i = 0; i < RTK_MAX_NUM_OF_PRIORITY; i++)
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pPriRemapGroup_set(0, i, i, 0));

    // reset port priority-to-queue mapping:
    //      - for lan ports use table index 3
    for (i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if(i == gPlatformDb.ponPort || i == gPlatformDb.rgmiiPort)
            continue;
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_portPriMap_set(i, 0));

        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_schedulingQueue_set(i, &qWeight));
    }
    //      - Only reset priority to queue mapping for table index 3
    for (i = 0; i < RTK_MAX_NUM_OF_PRIORITY; i++)
        pri2Qid.pri2queue[i] = i;
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priMap_set(0, &pri2Qid));

    // remove acl that is created for dp
    if (g_isDsDeiDpAclCfg)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei0DpAcl));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsStagDei1DpAcl));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei0DpAcl));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsCtagDei1DpAcl));

        g_isDsDeiDpAclCfg = FALSE;
    }
    // remove acl that is created for ds bc gem
    if (g_dsBcGemTrapAcl >= 0)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(g_dsBcGemTrapAcl));

        g_dsBcGemTrapAcl = -1;
    }
    // reset bandwidth
    for (i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if (i == gPlatformDb.ponPort ||
            i == gPlatformDb.rgmiiPort ||
            i == gPlatformDb.cpuPort)
            continue;
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rate_portEgrBandwidthCtrlRate_set(i, MAX_BW_RATE));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rate_portIgrBandwidthCtrlRate_set(i, MAX_BW_RATE));
    }

    // disable storm control
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rate_stormControlEnable_get(&stormCtrl));
    stormCtrl.unknown_unicast_enable = DISABLED;
    stormCtrl.broadcast_enable = DISABLED;
    stormCtrl.multicast_enable = DISABLED;
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rate_stormControlEnable_set(&stormCtrl));
    // disable all meters
    for (i = 0; i < gPlatformDb.meterNum; i++)
    {
        if (gPlatformDb.rsvMeterId == i)
            continue;
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG,rtk_rate_shareMeter_set(i, 0, DISABLED));
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DumpL2Serv(void)
{
    struct list_head *next,*tmp;
    l2_service_t *cur;
    rtk_classify_cfg_t cfg;
    int ret = OMCI_ERR_OK, i;

    printk("======== OMCI L2 Service Rule ============\n");

    list_for_each_safe(next,tmp,&gPlatformDb.l2Head)
    {
        cur = list_entry(next,l2_service_t,list);

        printk("Application ID: %d, uniMask: %u, dsCf=%u, dsACL=%d ", cur->index, cur->uniMask, cur->dsCfIndex, cur->dsAclIndex);

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            printk("usCf[%d]=%u ", i, cur->pUsCfIndex[i]);
        }
        printk("\n");

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if(cur->pUsCfIndex[i] != OMCI_UNUSED_CF)
            {
                memset(&cfg,0,sizeof(rtk_classify_cfg_t));
                cfg.index = cur->pUsCfIndex[i];
                if((ret = rtk_classify_cfgEntry_get(&cfg))!=RT_ERR_OK)
                {
                    continue;
                }
                _classf_show(&cfg);
            }
        }

        cfg.index = cur->dsCfIndex;
        if((ret = rtk_classify_cfgEntry_get(&cfg))!=RT_ERR_OK)
        {
            continue;
        }
        _classf_show(&cfg);
        printk("###################################\n");
    }

    printk("======== OMCI CF Rule Priority ============\n\n");
    for(i = 0; i < gPlatformDb.cfNum; i++)
    {
        if(gPlatformDb.cfRule[i].isCfg)
        {
            printk("cfRule[%d] rule_pri %d, level %d\n", i, gPlatformDb.cfRule[i].rulePri.rule_pri, gPlatformDb.cfRule[i].rulePri.rule_level);
        }
    }
    printk("=======================================End.\n");
    printk("======== OMCI L2 Aggregated Rule ============\n\n");
    omci_dump_ds_aggregated_list(PON_GEMPORT_DIRECTION_BI);
    return ret;
}


static int pf_rtl96xx_DumpVeipServ(void)
{
    struct list_head            *pEntry;
    struct list_head            *pTmpEntry;
    veip_service_t              *pData;
    int                         wanIdx;
    rtk_classify_cfg_t          cfEntry;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        if (0 == pData->wanIfBitmap)
        {
            printk(" %5s | %10s | %10s | %6s | %6s | %6s\n",
                "INDEX", "HW Path CF", "SW Path CF", "US SID", "DS SID", "WAN IF");
            printk(" %5u | %10s | %10s | %6u | %6u | %6s\n",
                pData->index,
                "N/A",
                "N/A",
                pData->usStreamId,
                pData->dsStreamId,
                "N/A");

            continue;
        }

        for (wanIdx = 0; wanIdx < gPlatformDb.intfNum; wanIdx++)
        {
            if (!(pData->wanIfBitmap & (1 << wanIdx)))
                continue;

            printk(" %5s | %10s | %10s | %6s | %6s | %6s\n",
                "INDEX", "HW Path CF", "SW Path CF", "US SID", "DS SID", "WAN IF");
            printk(" %5u | %10u | %10u | %6u | %6u | %6d\n",
                pData->index,
                pData->pHwPathCfIdx[wanIdx],
                pData->pSwPathCfIdx[wanIdx],
                pData->usStreamId,
                pData->dsStreamId,
                wanIdx);

            // display hw path cf
            if (OMCI_UNUSED_CF != pData->pHwPathCfIdx[wanIdx])
            {
                cfEntry.index = pData->pHwPathCfIdx[wanIdx];
                if (RT_ERR_OK ==
                        rtk_classify_cfgEntry_get(&cfEntry))
                    _classf_show(&cfEntry);
            }

            // display sw path cf
            if (OMCI_UNUSED_CF != pData->pSwPathCfIdx[wanIdx])
            {
                cfEntry.index = pData->pSwPathCfIdx[wanIdx];
                if (RT_ERR_OK ==
                        rtk_classify_cfgEntry_get(&cfEntry))
                    _classf_show(&cfEntry);
            }

            printk("##########################################\n");
        }

    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DumpMBServ(void)
{
    struct list_head *next,*tmp;
    mbcast_service_t *cur;
    rtk_classify_cfg_t cfg;
    int ret = OMCI_ERR_OK;

    printk("======== OMCI mcast / bcast Service Rule ============\n");
    list_for_each_safe(next, tmp, &gPlatformDb.mbcastHead){

        cur = list_entry(next, mbcast_service_t, list);
        printk("Application ID: %d, uniMask: %u, dsCf=%u, dsACL=%d\n", cur->index, cur->uniMask, cur->dsCfIndex, cur->dsAclIndex);

        if(cur->dsCfIndex != OMCI_UNUSED_CF)
        {
            cfg.index = cur->dsCfIndex;
            if((ret = rtk_classify_cfgEntry_get(&cfg))!=RT_ERR_OK)
            {
                continue;
            }
            _classf_show(&cfg);
            printk("###################################\n");
        }
    }

    printk("=======================================End.\n");
    printk("======== OMCI MB Aggregated Rule ============\n\n");
    omci_dump_ds_aggregated_list(PON_GEMPORT_DIRECTION_DS);
    return ret;
}

static int pf_rtl96xx_DumpCfMap(void)
{
    printk("VEIP Fast Path [%u]: %u ~ %u\n", PF_CF_TYPE_VEIP_FAST, gPlatformDb.veipFastStart, gPlatformDb.veipFastStop);
    printk("L2 EtherType   [%u]: %u ~ %u\n", PF_CF_TYPE_L2_ETH_FILTER, gPlatformDb.ethTypeFilterStart, gPlatformDb.ethTypeFilterStop);
    printk("L2 Common      [%u]: %u ~ %u\n", PF_CF_TYPE_L2_COMM, gPlatformDb.l2CommStart, gPlatformDb.l2CommStop);
    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetCfMap(unsigned int cfType, unsigned int start, unsigned int stop)
{
    if (start <= stop && start < gPlatformDb.cfNum && stop < gPlatformDb.cfNum)
    {
        switch(cfType)
        {
            case PF_CF_TYPE_L2_COMM:
                gPlatformDb.l2CommStart = start;
                gPlatformDb.l2CommStop = stop;
                break;
            case PF_CF_TYPE_L2_ETH_FILTER:
                gPlatformDb.ethTypeFilterStart = start;
                gPlatformDb.ethTypeFilterStop = stop;
                break;
            case PF_CF_TYPE_VEIP_FAST:
                gPlatformDb.veipFastStart = start;
                gPlatformDb.veipFastStop = stop;
                break;
            default:
                return OMCI_ERR_FAILED;
            break;
        }
    }
    return 0;
}

static int pf_rtl96xx_DumpMacFilter(void)
{
    struct list_head *next,*tmp;
    int ret = OMCI_ERR_OK;
    macFilter_entry_t *ptr = NULL;

    printk("======== OMCI mac filter entry Rule ============\n");

    list_for_each_safe(next, tmp, &gPlatformDb.macFilterHead)
    {
        ptr = list_entry(next, macFilter_entry_t, list);
        if (ptr)
            printk("hwAclIdx[0]=%d, hwAclIdx[1]=%d, key=%llu\n", ptr->hwAclIdx[0], ptr->hwAclIdx[1], ptr->key);
    }
    printk("=======================================End.\n");


    return ret;
}

static int pf_rtl96xx_DumpFlow2dsPq(void)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    flow2DsPq_entry_t       *pEntryData;

    printk("---------------------------------------------------------------------------------------------\n");

    printk(" %10s | %10s | %10s | %10s | %10s | %10s | %10s\n",
        "flowId", "dsPqOmciPri", "dpMarking", "lanPort", "policy", "priority", "weight");
    printk("---------------------------------------------------------------------------------------------\n");
    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.flow2DsPqHead)
    {
        pEntryData = list_entry(pEntry, flow2DsPq_entry_t, list);

        printk(" %10u | %10u | %10u | %10u | %10u | %10u | %10u\n",
            pEntryData->flowId,
            pEntryData->dsQ.dsPqOmciPri,
            pEntryData->dsQ.dpMarking,
            pEntryData->dsQ.portId,
            pEntryData->dsQ.policy,
            pEntryData->dsQ.priority,
            pEntryData->dsQ.weight);

        printk("---------------------------------------------------------------------------------------------\n");
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_InitPlatform(void)
{
    rtk_port_t port = 0;
    int ret, i;
    rtk_portmask_t portmask;
    rtk_enable_t state = ENABLED;
    rtk_switch_system_mode_t mode;
    rtk_portmask_t allPortMask;
    rtk_acl_template_t aclTemplate;

    RTK_PORTMASK_RESET(portmask);
    /* unkown uc does not flood in cpu port */
    for(i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if (i == gPlatformDb.cpuPort || i == gPlatformDb.rgmiiPort)
            continue;

        portmask.bits[0] |= (1 << i);
    }

    memset(&mode, 0x00, sizeof(rtk_switch_system_mode_t));
    mode.initDefault = ENABLED;

    /*initial chip*/
    if (RT_ERR_DRIVER_NOT_FOUND == rtk_switch_system_init(&mode))
    {
        rtk_l2_lookupMissFloodPortMask_set(DLF_TYPE_UCAST, &portmask);

        rtk_l2_lookupMissFloodPortMask_set(DLF_TYPE_BCAST, &portmask);

        /*Temporary code*/
        /*Set CPU egress port Q0 ~ Q6 rate limit*/
        if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
        {
            ret = rtk_rate_shareMeterMode_set(gPlatformDb.rsvMeterId, METER_MODE_PACKET_RATE);
            if(ret != RT_ERR_OK)
            {
                OMCI_LOG(
                    OMCI_LOG_LEVEL_DBG,
                    "[%d] Failed to set METER_MODE_PACKET_RATE of Meter\nn",
                    __LINE__);
            }
            ret = rtk_rate_shareMeter_set(gPlatformDb.rsvMeterId, 5000, ENABLED);
            if(ret != RT_ERR_OK)
            {
                OMCI_LOG(
                    OMCI_LOG_LEVEL_DBG,
                    "[%d] Failed to set Meter ID[%u]\nn",
                    __LINE__,
                    gPlatformDb.rsvMeterId);
            }
        }

        for (i = 0; i <= 6; i++)
        {
            rtk_rate_egrQueueBwCtrlMeterIdx_set(gPlatformDb.cpuPort, i, gPlatformDb.rsvMeterId);
            rtk_rate_egrQueueBwCtrlEnable_set(gPlatformDb.cpuPort, i, ENABLED);
        }
    }
    else{
        gPlatformDb.aclStartIdx = mode.sysUsedAclNum;
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s ACL start index:%d\n",
            __FUNCTION__, gPlatformDb.aclStartIdx);
    }
    memset(&mode, 0x00, sizeof(rtk_switch_system_mode_t));
    mode.initIgmpSnooping = ENABLED;
    if (RT_ERR_DRIVER_NOT_FOUND == rtk_switch_system_init(&mode))
    {
        /* set internal priority for igmp and mld */
        omci_SetIgmpMldInternalPri();
    }

    if(OMCI_ERR_OK != rtk_gponapp_initial(1))
        return OMCI_ERR_FAILED;

    /*remove all hw cf rules*/
    for(i = 0; i < gPlatformDb.cfTotalNum; i++)
    {
        ret = rtk_classify_cfgEntry_del(i);
        if(ret != RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Failed to delete CF[%u]\n", __LINE__, i);
        }
    }
    ret = rtk_classify_unmatchAction_set(CLASSIFY_UNMATCH_PERMIT_WITHOUT_PON);
    if(ret != RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Failed to set unmatchAction of CF\n", __LINE__);
    }

    /*Set DS default drop rule*/
    omci_CreateDsDefaultDropCf();

    /*Set CF pri 7 for DS OMCI*/
    omci_setOmciInternalPri();

    ret = rtk_vlan_vlanFunctionEnable_set(DISABLED);

    /*enable acl state of all ports*/
    for(i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if((ret = rtk_acl_igrState_set(i, ENABLED)) != RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Set ACL state at port %d Fail", __LINE__, i);
        }
    }
    ret = rtk_acl_igrState_set(gPlatformDb.ponPort, ENABLED);

    /* SVLAN setting */
    /* Beside ApolloMP, set all port to SVLAN aware port and disable SVLAN filter
     * ApolloMP not support disable SVLAN filter, so only set pon port to aware port
     */
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_allPortMask_set(&allPortMask));
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_create(0));
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_vlan_port_set(0, &allPortMask, &allPortMask));

    for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
    {
        if((ret = rtk_vlan_portPvid_set(port, 0))!=RT_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Set PVID at port %d Fail", __LINE__, port);
        }
    }

    switch(gPlatformDb.chipId)
    {
        case APOLLOMP_CHIP_ID:
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(gPlatformDb.ponPort, ENABLED));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_lookupType_set(SVLAN_LOOKUP_C4KVLAN));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_untagAction_set(SVLAN_ACTION_SVLAN, 0));
            break;
        default:
            for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_servicePort_set(port, ENABLED));
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_portSvid_set(port, 0));
            }
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_svlanFunctionEnable_set(DISABLED));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_untagAction_set(SVLAN_ACTION_PSVID, 0));
            break;
    }

    // acl template entry 2 is used to mac filtering function
    aclTemplate.index = 0x1;
    aclTemplate.fieldType[0] = ACL_FIELD_DMAC0;
    aclTemplate.fieldType[1] = ACL_FIELD_DMAC1;
    aclTemplate.fieldType[2] = ACL_FIELD_DMAC2;
    aclTemplate.fieldType[3] = ACL_FIELD_SMAC0;
    aclTemplate.fieldType[4] = ACL_FIELD_SMAC1;
    aclTemplate.fieldType[5] = ACL_FIELD_SMAC2;
    aclTemplate.fieldType[6] = ACL_FIELD_CTAG;      // it could be changed to STAG
    aclTemplate.fieldType[7] = ACL_FIELD_GEMPORT;
    if (RT_ERR_OK != (ret = rtk_acl_template_set(&aclTemplate)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set template for ds bc gem trap acl failed, ret %d", ret);

        return ret;
    }
#if defined(FPGA_DEFINED)
    memset(&aclTemplate, 0, sizeof(rtk_acl_template_t));
    // acl template entry 2 is used to mac filtering function
    aclTemplate.index = 0x2;
    aclTemplate.fieldType[0] = ACL_FIELD_CTAG;
    aclTemplate.fieldType[1] = ACL_FIELD_FRAME_TYPE_TAGS;
    aclTemplate.fieldType[2] = ACL_FIELD_ETHERTYPE;
    aclTemplate.fieldType[3] = ACL_FIELD_STAG;
    aclTemplate.fieldType[4] = ACL_FIELD_GEMPORT;
    aclTemplate.fieldType[5] = ACL_FIELD_DMAC0;
    aclTemplate.fieldType[6] = ACL_FIELD_DMAC1;      // it could be changed to STAG
    aclTemplate.fieldType[7] = ACL_FIELD_DMAC2;
    if (RT_ERR_OK != (ret = rtk_acl_template_set(&aclTemplate)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set template to replace CF failed, ret %d", ret);

        return ret;
    }
    /*if (RT_ERR_OK != (ret = rtk_acl_igrPermitState_set(gPlatformDb.ponPort, DISABLED)))
    {

        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set template for ds bc gem trap acl failed, ret %d", ret);
        return ret;
    }*/
#endif
    /* vid 0 and 4095 as tagging */
    ret = rtk_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG);


    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_l2_camState_set(state));

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, omci_init());
    rtk_gponapp_evtHdlPloam_reg(omci_ploam_callback);
    //rtk_gponapp_activate(RTK_GPONMAC_INIT_STATE_O1);

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_priorityRef_set(REF_CTAG_PRI));
    return 0;
}


static int pf_rtl96xx_ExitPlatform(void)
{
    rtk_gponapp_deActivate();
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, omci_exit());
    rtk_gponapp_evtHdlPloam_dreg();
    rtk_gponapp_evtHdlAlarm_dreg(RTK_GPON_ALARM_SF);
    rtk_gponapp_evtHdlAlarm_dreg(RTK_GPON_ALARM_SD);
    return 0;
}


static int pf_rtl96xx_SetDscpRemap(OMCI_DSCP2PBIT_ts *pDscp2PbitTable)
{
    int i;
    rtk_qos_priSelWeight_t   weight;

    /*Set DSCP remapping to Pbit table*/
    for(i = 0; i < OMCI_DSCP_NUM; i++)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_dscpPriRemapGroup_set(0, i, pDscp2PbitTable->pbit[i], 0));
    }

    /*Disable 1p remarking*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_1pRemarkEnable_set(gPlatformDb.ponPort, DISABLED));
    /*Disable dscp remark, keep original DSCP*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_dscpRemarkEnable_set(gPlatformDb.ponPort, DISABLED));

    /*Set DSCP weight higher than port-based/dot1q in group 0*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priSelGroup_get(0, &weight));
    if (weight.weight_of_dot1q >= weight.weight_of_portBased)
        weight.weight_of_dscp = weight.weight_of_dot1q;
    else
        weight.weight_of_dscp = weight.weight_of_portBased;
    if (gPlatformDb.maxPriSelWeight > weight.weight_of_dscp)
        weight.weight_of_dscp += 1;
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priSelGroup_set(0, &weight));

    /*Set priority-selector group of WAN port*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_portPriSelGroup_set(gPlatformDb.ponPort, 0));

    return 0;
}

static int pf_rtl96xx_SetMacLearnLimit(OMCI_MACLIMIT_ts *pMacLimit)
{
    rtk_switch_devInfo_t    tDevInfo;

    if (!pMacLimit)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_switch_deviceInfo_get(&tDevInfo))
        return RT_ERR_FAILED;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_l2_portLimitLearningCnt_set((unsigned int)pMacLimit->portIdx, (unsigned int)pMacLimit->macLimitNum));

    if (pMacLimit->macLimitNum == tDevInfo.capacityInfo.l2_learn_limit_cnt_max)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_l2_portLimitLearningCntAction_set((unsigned int)pMacLimit->portIdx, LIMIT_LEARN_CNT_ACTION_FORWARD));
    }
    else
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_l2_portLimitLearningCntAction_set((unsigned int)pMacLimit->portIdx, LIMIT_LEARN_CNT_ACTION_DROP));
    }

    return 0;
}
static int omci_mac_filter_hw_proc(OMCI_MACFILTER_ts *pMacFilter, macFilter_entry_t *pOldMacFilter, int *pHwAclIdx)
{
    rtk_l2_ucastAddr_t  l2Addr;
    unsigned int        idx, i;
    int                 *pTmp = NULL;

    memset(&l2Addr, 0, sizeof(rtk_l2_ucastAddr_t));
    memcpy(&l2Addr.mac.octet, pMacFilter->mac, ETHER_ADDR_LEN);
    l2Addr.flags |= RTK_L2_UCAST_FLAG_STATIC;

    if (!pMacFilter || !pHwAclIdx)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "%s()@%d input args are null", __FUNCTION__, __LINE__);
        return OMCI_ERR_FAILED;
    }

    if (RTL9601B_CHIP_ID == gPlatformDb.chipId)
    {
        if (MAC_FILTER_ENTRY_ACT_ADD == pMacFilter->entryAct)
        {
            if (MAC_FILTER_ACT_FORWARD == pMacFilter->filter)
            {
                if (MAC_FILTER_TYPE_SA == pMacFilter->macType)
                {
                    //unknown sa action per port
                    //set static mac address in l2-table
                    if(MAC_FILTER_DIR_DS == pMacFilter->dir)
                    {
                        if(RT_ERR_OK != rtk_l2_newMacOp_set(gPlatformDb.ponPort, HARDWARE_LEARNING, ACTION_DROP))
                            return OMCI_ERR_FAILED;
                        l2Addr.port = gPlatformDb.ponPort;
                        if(RT_ERR_OK != rtk_l2_addr_add(&l2Addr))
                            return OMCI_ERR_FAILED;
                    }
                    else
                    {
                        for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                        {
                            if(((pMacFilter->portMask) & (1 << idx)))
                            {
                                if(RT_ERR_OK != rtk_l2_newMacOp_set(idx, HARDWARE_LEARNING, ACTION_DROP))
                                    return OMCI_ERR_FAILED;
                                l2Addr.port = idx;
                                if(RT_ERR_OK != rtk_l2_addr_add(&l2Addr))
                                    return OMCI_ERR_FAILED;
                            }
                        }
                    }
                }
                else
                {   //lookup miss action per port
                    //set static mac address in l2-table
                    if(MAC_FILTER_DIR_DS == pMacFilter->dir)
                    {
                        if (RT_ERR_OK !=
                            rtk_l2_portLookupMissAction_set(gPlatformDb.ponPort, DLF_TYPE_UCAST, ACTION_DROP))
                            return OMCI_ERR_FAILED;
                        for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                        {
                            if(((pMacFilter->portMask) & (1 << idx)))
                            {
                                l2Addr.port = idx;
                                if(RT_ERR_OK != rtk_l2_addr_add(&l2Addr))
                                    return OMCI_ERR_FAILED;
                            }
                        }
                    }
                    else
                    {
                        for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                        {
                            if(((pMacFilter->portMask) & (1 << idx)))
                            {
                                if(RT_ERR_OK !=
                                    rtk_l2_portLookupMissAction_set(idx, DLF_TYPE_UCAST, ACTION_DROP))
                                    return OMCI_ERR_FAILED;
                            }
                        }
                        l2Addr.port = gPlatformDb.ponPort;
                        if(RT_ERR_OK != rtk_l2_addr_add(&l2Addr))
                            return OMCI_ERR_FAILED;
                    }
                }
            }
            else
            {
                /* MAC_FILTER_ACT_FILTER CASE */

                //DROP use ACL
                idx = (MAC_FILTER_DIR_DS == pMacFilter->dir ? (1 << gPlatformDb.ponPort) : pMacFilter->portMask);
                if (OMCI_ERR_OK != omci_CreateMacFilterAcl(idx, pMacFilter, pHwAclIdx))
                    *pHwAclIdx = OMCI_UNUSED_CF;
            }
        }
        else if (MAC_FILTER_ENTRY_ACT_REMOVE == pMacFilter->entryAct)
        {
            if (!pOldMacFilter)
                return OMCI_ERR_FAILED;

            if (OMCI_UNUSED_CF == pOldMacFilter->hwAclIdx[0])
            {
                if(RT_ERR_OK != rtk_l2_addr_del(&l2Addr))
                    return OMCI_ERR_FAILED;
                if(MAC_FILTER_TYPE_SA == pMacFilter->macType)
                {
                    if(MAC_FILTER_DIR_DS == pMacFilter->dir)
                    {
                        if (RT_ERR_OK !=
                            rtk_l2_newMacOp_set(gPlatformDb.ponPort, HARDWARE_LEARNING, ACTION_FORWARD))
                            return OMCI_ERR_FAILED;
                    }
                    else
                    {

                        for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                        {
                            if(((pMacFilter->portMask) & (1 << idx)))
                            {
                                if(RT_ERR_OK != rtk_l2_newMacOp_set(idx, HARDWARE_LEARNING, ACTION_FORWARD))
                                    return OMCI_ERR_FAILED;
                            }
                        }
                    }
                }
                else
                {
                    if(MAC_FILTER_DIR_DS == pMacFilter->dir)
                    {
                        if(RT_ERR_OK !=
                            rtk_l2_portLookupMissAction_set(gPlatformDb.ponPort, DLF_TYPE_UCAST, ACTION_FORWARD))
                            return OMCI_ERR_FAILED;
                    }
                    else
                    {
                        for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                        {
                            if(((pMacFilter->portMask) & (1 << idx)))
                            {
                                if(RT_ERR_OK !=
                                    rtk_l2_portLookupMissAction_set(idx, DLF_TYPE_UCAST, ACTION_FORWARD))
                                    return OMCI_ERR_FAILED;
                            }
                        }
                    }
                }
                *pHwAclIdx = OMCI_UNUSED_CF;
            }
            else
            {
                if(RT_ERR_OK != rtk_acl_igrRuleEntry_del(pOldMacFilter->hwAclIdx[0]))
                {
                    *pHwAclIdx = pOldMacFilter->hwAclIdx[0];
                    return OMCI_ERR_FAILED;
                }

                /* TBD check other module does not enble this port
                if(pMacFilter->portMask & (1 << gPlatformDb.ponPort))
                    rtk_acl_igrState_set(gPlatformDb.ponPort, DISABLED);
                */
                for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                {
                    if((1 << idx) & pMacFilter->portMask)
                    {
                        if(RT_ERR_OK != rtk_acl_igrState_set(idx, DISABLED))
                            return OMCI_ERR_FAILED;
                    }
                }
                *pHwAclIdx = OMCI_UNUSED_CF;
            }
        }
    }
    else
    {
        /* chip id is not 9601b */
        if (MAC_FILTER_DIR_US == pMacFilter->dir)
        {
            if (MAC_FILTER_ENTRY_ACT_ADD == pMacFilter->entryAct)
            {
                // add action
                if (OMCI_ERR_OK != omci_CreateMacFilterAcl(pMacFilter->portMask, pMacFilter, pHwAclIdx))
                {
                    pTmp = pHwAclIdx;
                    *pTmp = OMCI_UNUSED_CF;
                    pTmp++;
                    *pTmp = OMCI_UNUSED_CF;
                }
            }
            else
            {
                // del action
                if (!pOldMacFilter)
                    return OMCI_ERR_FAILED;

                for (i = 0; i < sizeof(pOldMacFilter->hwAclIdx) / sizeof(int); i++)
                {
                    pTmp = pHwAclIdx + i;
                    if (OMCI_UNUSED_CF != pOldMacFilter->hwAclIdx[i])
                    {
                        if (RT_ERR_OK != rtk_acl_igrRuleEntry_del(pOldMacFilter->hwAclIdx[i]))
                        {
                            *pTmp = pOldMacFilter->hwAclIdx[i];
                            return OMCI_ERR_FAILED;
                        }
                         /* TBD check other module does not enble this port
                        if(pMacFilter->portMask & (1 << gPlatformDb.ponPort))
                            rtk_acl_igrState_set(gPlatformDb.ponPort, DISABLED);
                        */
                        if (i == 0)
                        {
                            for(idx = gPlatformDb.etherPortMin; idx <= gPlatformDb.etherPortMax; idx++)
                            {
                                if((1 << idx) & pMacFilter->portMask)
                                {
                                    if(RT_ERR_OK != rtk_acl_igrState_set(idx, DISABLED))
                                        return OMCI_ERR_FAILED;
                                }
                            }
                        }
                        *pTmp = OMCI_UNUSED_CF;
                    }
                }
            }
        }
        else
        {
            // TBD
            //OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            printk(
                "%s()@%d not support ds mac filter for multiple port", __FUNCTION__, __LINE__);
        }

    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetMacFilter(OMCI_MACFILTER_ts *pMacFilter)
{
    int                 hwAclIdx[2]         = {OMCI_UNUSED_CF, OMCI_UNUSED_CF};
    unsigned long long  key                 = 0;
    macFilter_entry_t   *pMacFilterEntry    = NULL;

    key = ((unsigned long long)((pMacFilter->mac[0]) & 0xff));
    key = ((unsigned long long)((pMacFilter->mac[1]) & 0xff) << 8) | key;
    key = ((unsigned long long)((pMacFilter->mac[2]) & 0xff) << 16) | key;
    key = ((unsigned long long)((pMacFilter->mac[3]) & 0xff) << 24) | key;
    key = ((unsigned long long)((pMacFilter->mac[4]) & 0xff) << 32) | key;
    key = ((unsigned long long)((pMacFilter->mac[5]) & 0xff) << 40) | key;
    key = ((unsigned long long)((pMacFilter->id) & 0xffff) << 48) | key;

    pMacFilterEntry = macFilter_entry_find(key);

    // process hardware rule dependence on chip
    if (OMCI_ERR_OK != omci_mac_filter_hw_proc(pMacFilter, pMacFilterEntry, hwAclIdx))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "%s()@%d mac filter proc hw Fail", __FUNCTION__, __LINE__);
        return OMCI_ERR_FAILED;
    }

    // process software database if hw process sucessfully
    if (MAC_FILTER_ENTRY_ACT_ADD == pMacFilter->entryAct)
    {
        if(!pMacFilterEntry)
        {
            if(OMCI_ERR_OK != macFilter_entry_add(key))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "%s()@%d mac filter proc software Fail", __FUNCTION__, __LINE__);
                return OMCI_ERR_FAILED;
            }
            if (NULL != (pMacFilterEntry = macFilter_entry_find(key)))
            {
                memcpy(pMacFilterEntry->hwAclIdx, hwAclIdx, sizeof(hwAclIdx));
            }
        }
    }
    else if (MAC_FILTER_ENTRY_ACT_REMOVE == pMacFilter->entryAct)
    {
        if(OMCI_ERR_OK != macFilter_entry_del(key))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "%s()@%d mac filter proc software Fail", __FUNCTION__, __LINE__);
            return OMCI_ERR_FAILED;
        }
    }
    else
    {
        printk("%s():%d clear all not suuport \n", __FUNCTION__, __LINE__);
        return OMCI_ERR_FAILED;
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_GetDevCapabilities(omci_dev_capability_t *p)
{
    rtk_switch_devInfo_t    tDevInfo;
    uint8                   portId;
    uint32                  r;
    uint8                   totalUniPort = 0;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_switch_deviceInfo_get(&tDevInfo))
        return RT_ERR_FAILED;

    memset(p->ethPort, 0, sizeof(omci_eth_port_t) * RTK_MAX_NUM_OF_PORTS);
    p->fePortNum = 0;
    p->gePortNum = 0;
    p->cpuPort = -1;
    p->rgmiiPort = -1;
    p->ponPort = -1;
    p->potsPortNum = 0;
    p->totalTContNum = 0;
    p->totalGEMPortNum = 0;
    p->totalTContQueueNum = 0;
    p->perUNIQueueNum = 0;
    p->perTContQueueDp = FALSE;
    p->perUNIQueueDp = FALSE;
    p->meterNum = 0;
    p->totalL2Num = 0;

    // learn non-UNI ports info
    p->cpuPort = tDevInfo.cpuPort;
    p->rgmiiPort = tDevInfo.rgmiiPort;
    if (RT_ERR_OK != rtk_switch_phyPortId_get(RTK_PORT_PON, &p->ponPort))
        p->ponPort = -1;

    // learn UNI ports info
    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        p->ethPort[portId].portIdInType = 0xFF;
        if ((1 << portId) & tDevInfo.fe.portmask.bits[0])
        {
            if (portId != p->cpuPort &&
                    portId != p->rgmiiPort &&
                    portId != p->ponPort)
            {
                p->ethPort[portId].portType = RT_FE_PORT;
                p->ethPort[portId].portIdInType = p->fePortNum++;
                totalUniPort++;
            }
        }
        if ((1 << portId) & tDevInfo.ge.portmask.bits[0])
        {
            if (portId != p->cpuPort &&
                    portId != p->rgmiiPort &&
                    portId != p->ponPort)
            {
                p->ethPort[portId].portType = RT_GE_PORT;
                p->ethPort[portId].portIdInType = p->gePortNum++;
                totalUniPort++;
            }
        }
    }

    if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_ME_00020000, (int32)-1, (uint32)0, p, &totalUniPort))
    {
        r = (OMCIDRV_FEATURE_ERR_OK ==
                omcidrv_feature_api(FEATURE_KAPI_ME_00000200)) ? TRUE : FALSE;
        if (OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_ME_00000001, r, p))
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "FEATURE_KAPI_ME_00000001 is not enabled or exec failed...");
    }

    // learn other capabilities
    p->totalTContNum = tDevInfo.capacityInfo.gpon_tcont_max - 1;

#if defined(FPGA_DEFINED)
    p->totalTContNum = 8;
#else
    if( OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_ME_00100000, p))
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "FEATURE_KAPI_ME_00100000 is not enabled or exec failed...");
#endif
    p->totalGEMPortNum = tDevInfo.capacityInfo.classify_sid_max - 1;
    p->totalTContQueueNum = ((p->totalTContNum) * 8);//tDevInfo.capacityInfo.max_num_of_pon_queue - 1;
    p->perUNIQueueNum = tDevInfo.capacityInfo.max_num_of_queue;
    p->meterNum = tDevInfo.capacityInfo.max_num_of_metering;
    p->rsvMeterId = gPlatformDb.rsvMeterId;
    p->totalL2Num = tDevInfo.capacityInfo.l2_learn_limit_cnt_max;

    get_resouce_by_dev_feature(DEV_FEATURE_ALL, p);

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetDevIdVersion(omci_dev_id_version_t *p)
{
    uint32  chipId = 0;
    uint32  rev = 0;
    uint32  subType = 0;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_switch_version_get(&chipId, &rev, &subType))
        return RT_ERR_FAILED;

    memset(p->id, 0, OMCI_DRV_DEV_ID_LEN);
    memset(p->version, 0, OMCI_DRV_DEV_VERSION_LEN);
    p->chipId = chipId;

    // learn device id and its version
    switch (chipId)
    {
        case APOLLOMP_CHIP_ID:
        {
            switch (subType)
            {
                case 0x03:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9601");
                    break;
                case 0x05:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9602B");
                    break;
                case 0x07:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL86906");
                    break;
                case 0x09:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9606");
                    break;
                case 0x0b:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9607");
                    break;
                case 0x0d:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9602");
                    break;
                case 0x0f:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9603");
                    break;
                case 0x13:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL8696");
                    break;
                case 0x17:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL8198B");
                    break;
                default:
                    snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "Unknown");
                    break;
            }
            break;
        }
        case RTL9601B_CHIP_ID:
            snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9601B");
            break;
        case RTL9602C_CHIP_ID:
            snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9602C");
            break;
        default:
            snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "Unknown");
            break;
    }
    snprintf(p->version, OMCI_DRV_DEV_VERSION_LEN, "%u", rev);

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetUsDBRuStatus(unsigned int *p)
{
    rtk_gpon_us_dbr_para_t  gponPara;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_gponapp_parameter_get(RTK_GPON_PARA_TYPE_US_DBR, &gponPara))
        return RT_ERR_FAILED;

    *p = gponPara.us_dbru_en ? TRUE : FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetTransceiverStatus(omci_transceiver_status_t *p)
{
    rtk_transceiver_parameter_type_t    type;
    rtk_transceiver_data_t              data;

    if (!p)
        return RT_ERR_FAILED;

    switch (p->type)
    {
        case OMCI_TRANSCEIVER_STATUS_TYPE_VENDOR_NAME:
            type = RTK_TRANSCEIVER_PARA_TYPE_VENDOR_NAME;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_VENDOR_PART_NUM:
            type = RTK_TRANSCEIVER_PARA_TYPE_VENDOR_PART_NUM;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_TEMPERATURE:
            type = RTK_TRANSCEIVER_PARA_TYPE_TEMPERATURE;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_VOLTAGE:
            type = RTK_TRANSCEIVER_PARA_TYPE_VOLTAGE;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_BIAS_CURRENT:
            type = RTK_TRANSCEIVER_PARA_TYPE_BIAS_CURRENT;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER:
            type = RTK_TRANSCEIVER_PARA_TYPE_TX_POWER;
            break;
        case OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER:
            type = RTK_TRANSCEIVER_PARA_TYPE_RX_POWER;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    memset(&data, 0, sizeof(rtk_transceiver_data_t));

    if (RT_ERR_OK != rtk_ponmac_transceiver_get(type, &data))
        return RT_ERR_FAILED;

    if (sizeof(data.buf) < sizeof(p->data))
        memcpy(p->data, data.buf, sizeof(data.buf));
    else
        memcpy(p->data, data.buf, sizeof(p->data));

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortLinkStatus(omci_port_link_status_t *p)
{
    rtk_port_linkStatus_t   linkStatus;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_port_link_get(p->port, &linkStatus))
        return RT_ERR_FAILED;

    p->status = (PORT_LINKUP == linkStatus) ? TRUE : FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortSpeedDuplexStatus(omci_port_speed_duplex_status_t *p)
{
    rtk_port_speed_t    speed;
    rtk_port_duplex_t   duplex;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_port_speedDuplex_get(p->port, &speed, &duplex))
        return RT_ERR_FAILED;

    switch (speed)
    {
        case PORT_SPEED_10M:
            p->speed = OMCI_PORT_SPEED_10M;
            break;
        case PORT_SPEED_100M:
            p->speed = OMCI_PORT_SPEED_100M;
            break;
        case PORT_SPEED_1000M:
            p->speed = OMCI_PORT_SPEED_1000M;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    switch (duplex)
    {
        case PORT_HALF_DUPLEX:
            p->duplex = OMCI_PORT_HALF_DUPLEX;
            break;
        case PORT_FULL_DUPLEX:
            p->duplex = OMCI_PORT_FULL_DUPLEX;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortAutoNegoAbility(omci_port_auto_nego_ability_t *p)
{
    rtk_port_phy_ability_t  phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    phyAbility.Full_10 = p->full_10;
    phyAbility.Half_10 = p->half_10;
    phyAbility.Full_100 = p->full_100;
    phyAbility.Half_100 = p->half_100;
    phyAbility.Full_1000 = p->full_1000;
    phyAbility.Half_1000 = p->half_1000;

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_set(p->port, &phyAbility))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortAutoNegoAbility(omci_port_auto_nego_ability_t *p)
{
    rtk_port_phy_ability_t  phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    p->full_10 = phyAbility.Full_10;
    p->half_10 = phyAbility.Half_10;
    p->full_100 = phyAbility.Full_100;
    p->half_100 = phyAbility.Half_100;
    p->full_1000 = phyAbility.Full_1000;
    p->half_1000 = phyAbility.Half_1000;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortState(omci_port_state_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if((RTL9602C_CHIP_ID == gPlatformDb.chipId) &&
        ((RTL9602C_CHIP_SUB_TYPE_RTL9601C == gPlatformDb.chipSubtype) ||
         (RTL9602C_CHIP_SUB_TYPE_RTL9601C_VB == gPlatformDb.chipSubtype)))
    {
        rtk_port_macAbility_t macAbility;
        if (RT_ERR_OK != rtk_port_macForceAbility_get(p->port, &macAbility))
            return RT_ERR_FAILED;

        if(0 == p->state)
        {
            macAbility.linkStatus = PORT_LINKDOWN;
        }
        else
        {
            macAbility.linkStatus = PORT_LINKUP;
        }
        if (RT_ERR_OK != rtk_port_macForceAbility_set(p->port, macAbility))
            return RT_ERR_FAILED;

    } else {
        if (RT_ERR_OK != rtk_port_adminEnable_set(p->port, p->state))
            return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortState(omci_port_state_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if((RTL9602C_CHIP_ID == gPlatformDb.chipId) &&
        ((RTL9602C_CHIP_SUB_TYPE_RTL9601C == gPlatformDb.chipSubtype) ||
         (RTL9602C_CHIP_SUB_TYPE_RTL9601C_VB == gPlatformDb.chipSubtype)))
    {
        rtk_port_macAbility_t macAbility;

        if (RT_ERR_OK != rtk_port_macForceAbility_get(p->port, &macAbility))
            return RT_ERR_FAILED;

        if(PORT_LINKDOWN == macAbility.linkStatus)
        {
            p->state = 0;
        }
        else
        {
            p->state = 1;
        }

    } else {
        if (RT_ERR_OK != rtk_port_adminEnable_get(p->port, &p->state))
            return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (APOLLOMP_CHIP_ID == gPlatformDb.chipId ||
            RTL9602C_CHIP_ID == gPlatformDb.chipId)
    {
        // mtu is shared with cpu port
        // hence cpu tag and vlan tag has to be considered
        p->size += (8 + 8);
    }

    if (RT_ERR_OK != rtk_switch_maxPktLenByPort_set(p->port, p->size))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_switch_maxPktLenByPort_get(p->port, &p->size))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortPhyLoopback(omci_port_loopback_t *p)
{
    uint32  data;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if (RT_ERR_OK != rtk_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (p->loopback)
        data |= Loopback_MASK;
    else
        data &= ~Loopback_MASK;

    if (RT_ERR_OK != rtk_port_phyReg_set(p->port, PHY_PAGE_0, PHY_CONTROL_REG, data))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortPhyLoopback(omci_port_loopback_t *p)
{
    uint32  data;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (data & Loopback_MASK)
        p->loopback = TRUE;
    else
        p->loopback = FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    rtk_enable_t    state;
    rtk_port_t      port;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    port = p->port;

    state = (p->state) ? ENABLED : DISABLED;

    if (RT_ERR_OK != rtk_port_phyPowerDown_set(port, state))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    rtk_enable_t    state;
    rtk_port_t      port;

    if (!p)
        return RT_ERR_FAILED;

    port = p->port;

    if (RT_ERR_OK !=rtk_port_phyPowerDown_get(port, &state))
        return RT_ERR_FAILED;

    p->state = (ENABLED == state) ? TRUE : FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortStat(omci_port_stat_t *p)
{
    rtk_stat_port_cntr_t    portCntrs;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_stat_mibSyncMode_set(STAT_MIB_SYNC_MODE_STOP_SYNC))
    {
        printk("%s():%d rtk_stat_mibSyncMode_set Fail \n", __FUNCTION__, __LINE__);
        return RT_ERR_FAILED;
    }
    if (RT_ERR_OK != rtk_stat_port_getAll(p->port, &portCntrs))
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_stat_mibSyncMode_set(STAT_MIB_SYNC_MODE_FREE_SYNC));
        return RT_ERR_FAILED;
    }
    p->ifInOctets                       = portCntrs.ifInOctets;
    p->ifInUcastPkts                    = portCntrs.ifInUcastPkts;
    p->ifInMulticastPkts                = portCntrs.ifInMulticastPkts;
    p->ifInBroadcastPkts                = portCntrs.ifInBroadcastPkts;
    p->ifInDiscards                     = portCntrs.ifInDiscards;
    p->ifOutOctets                      = portCntrs.ifOutOctets;
    p->ifOutUcastPkts                   = portCntrs.ifOutUcastPkts;
    p->ifOutMulticastPkts               = portCntrs.ifOutMulticastPkts;
    p->ifOutBrocastPkts                 = portCntrs.ifOutBrocastPkts;
    p->ifOutDiscards                    = portCntrs.ifOutDiscards;
    p->dot1dTpPortInDiscards            = portCntrs.dot1dTpPortInDiscards;
    p->dot3StatsSingleCollisionFrames   = portCntrs.dot3StatsSingleCollisionFrames;
    p->dot3StatsMultipleCollisionFrames = portCntrs.dot3StatsMultipleCollisionFrames;
    p->dot3StatsDeferredTransmissions   = portCntrs.dot3StatsDeferredTransmissions;
    p->dot3StatsLateCollisions          = portCntrs.dot3StatsLateCollisions;
    p->dot3StatsExcessiveCollisions     = portCntrs.dot3StatsExcessiveCollisions;
    p->dot3InPauseFrames                = portCntrs.dot3InPauseFrames;
    p->dot3OutPauseFrames               = portCntrs.dot3OutPauseFrames;
    p->dot3StatsAligmentErrors          = portCntrs.dot3StatsAligmentErrors;
    p->dot3StatsFCSErrors               = portCntrs.dot3StatsFCSErrors;
    p->dot3StatsSymbolErrors            = portCntrs.dot3StatsSymbolErrors;
    p->dot3StatsFrameTooLongs           = portCntrs.dot3StatsFrameTooLongs;
    p->etherStatsDropEvents             = portCntrs.etherStatsDropEvents;
    p->etherStatsFragments              = portCntrs.etherStatsFragments;
    p->etherStatsJabbers                = portCntrs.etherStatsJabbers;
    p->etherStatsCRCAlignErrors         = portCntrs.etherStatsCRCAlignErrors;
    p->etherStatsTxUndersizePkts        = portCntrs.etherStatsTxUndersizePkts;
    p->etherStatsTxOversizePkts         = portCntrs.etherStatsTxOversizePkts;
    p->etherStatsTxPkts64Octets         = portCntrs.etherStatsTxPkts64Octets;
    p->etherStatsTxPkts65to127Octets    = portCntrs.etherStatsTxPkts65to127Octets;
    p->etherStatsTxPkts128to255Octets   = portCntrs.etherStatsTxPkts128to255Octets;
    p->etherStatsTxPkts256to511Octets   = portCntrs.etherStatsTxPkts256to511Octets;
    p->etherStatsTxPkts512to1023Octets  = portCntrs.etherStatsTxPkts512to1023Octets;
    p->etherStatsTxPkts1024to1518Octets = portCntrs.etherStatsTxPkts1024to1518Octets;
    p->etherStatsTxPkts1519toMaxOctets  = portCntrs.etherStatsTxPkts1519toMaxOctets;
    p->etherStatsTxCRCAlignErrors       = portCntrs.etherStatsTxCRCAlignErrors;
    p->etherStatsRxUndersizePkts        = portCntrs.etherStatsRxUndersizePkts;
    p->etherStatsRxOversizePkts         = portCntrs.etherStatsRxOversizePkts;
    p->etherStatsRxPkts64Octets         = portCntrs.etherStatsRxPkts64Octets;
    p->etherStatsRxPkts65to127Octets    = portCntrs.etherStatsRxPkts65to127Octets;
    p->etherStatsRxPkts128to255Octets   = portCntrs.etherStatsRxPkts128to255Octets;
    p->etherStatsRxPkts256to511Octets   = portCntrs.etherStatsRxPkts256to511Octets;
    p->etherStatsRxPkts512to1023Octets  = portCntrs.etherStatsRxPkts512to1023Octets;
    p->etherStatsRxPkts1024to1518Octets = portCntrs.etherStatsRxPkts1024to1518Octets;
    p->etherStatsRxPkts1519toMaxOctets  = portCntrs.etherStatsRxPkts1519toMaxOctets;


    if (RT_ERR_OK != rtk_stat_mibSyncMode_set(STAT_MIB_SYNC_MODE_FREE_SYNC))
    {
        printk("%s():%d rtk_stat_mibSyncMode_set Fail \n", __FUNCTION__, __LINE__);
        return RT_ERR_FAILED;
    }


    return RT_ERR_OK;
}

static int pf_rtl96xx_ResetPortStat(unsigned int port)
{
    if (RT_ERR_OK != rtk_stat_port_reset(port))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetUsFlowStat(omci_flow_stat_t *p)
{
    rtk_gpon_flow_counter_t     flowCntrs;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_gponapp_flowCounter_get(p->flow,
            RTK_GPON_PMTYPE_FLOW_US_GEM, &flowCntrs))
        return RT_ERR_FAILED;

    p->gemBlock = flowCntrs.usgem.gem_block;
    p->gemByte = flowCntrs.usgem.gem_byte;

    return RT_ERR_OK;
}

static int pf_rtl96xx_ResetUsFlowStat(unsigned int flow)
{
    return RT_ERR_OK;
}

static int pf_rtl96xx_GetDsFlowStat(omci_flow_stat_t *p)
{
    rtk_gpon_flow_counter_t     flowCntrs;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_gponapp_flowCounter_get(p->flow,
            RTK_GPON_PMTYPE_FLOW_DS_GEM, &flowCntrs))
        return RT_ERR_FAILED;

    p->gemBlock = flowCntrs.dsgem.gem_block;
    p->gemByte = flowCntrs.dsgem.gem_byte;

    return RT_ERR_OK;
}

static int pf_rtl96xx_ResetDsFlowStat(unsigned int flow)
{
    return RT_ERR_OK;
}

static int pf_rtl96xx_GetDsFecStat(omci_ds_fec_stat_t *p)
{
    rtk_gpon_global_counter_t   gponCntrs;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_gponapp_globalCounter_get(
            RTK_GPON_PMTYPE_DS_PHY, &gponCntrs))
        return RT_ERR_FAILED;

    p->corByte = gponCntrs.dsphy.rx_fec_correct_byte;
    p->corCodeword = gponCntrs.dsphy.rx_fec_correct_cw;
    p->uncorCodeword = gponCntrs.dsphy.rx_fec_uncor_cw;

    return RT_ERR_OK;
}

static int pf_rtl96xx_ResetDsFecStat(void)
{
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetSvlanTpid(omci_svlan_tpid_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_svlan_tpidEntry_set(p->index, p->tpid))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetSvlanTpid(omci_svlan_tpid_t *p)
{
    uint32  svlanTpid;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_svlan_tpidEntry_get(p->index, &svlanTpid))
        return RT_ERR_FAILED;

    p->tpid = svlanTpid;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetCvlanState(unsigned int *p)
{
    uint32  cvlan_state;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_vlan_vlanFunctionEnable_get(&cvlan_state))
        return RT_ERR_FAILED;

    *p = cvlan_state;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetGemBlkLen(unsigned short *pGemBlkLen)
{
    uint32 blockSize = 0;
    if (RT_ERR_OK != rtk_gponapp_dbruBlockSize_get(&blockSize))
        return RT_ERR_FAILED;

    *pGemBlkLen = (unsigned short)blockSize;
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetGemBlkLen(unsigned short gemBlkLen)
{
    if (RT_ERR_OK != rtk_gponapp_dbruBlockSize_set((uint32)gemBlkLen))
        return RT_ERR_FAILED;

    return RT_ERR_OK;

}

static int pf_rtl96xx_GetDrvVersion(char *drvVersion)
{
#ifdef CONFIG_DRV_RELEASE_VERSION
    snprintf(drvVersion, 64, "%s", CONFIG_DRV_RELEASE_VERSION);
#endif
    return RT_ERR_OK;
}

static int pf_rtl96xx_GetOnuState(PON_ONU_STATE *pOnuState)
{
    rtk_gpon_onuState_t onuState;

    if (RT_ERR_OK != rtk_gpon_onuState_get(&onuState))
        return RT_ERR_FAILED;

    *pOnuState = (PON_ONU_STATE)onuState;
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPonBwThreshold(omci_pon_bw_threshold_t *pPonBwThreshold)
{
    if (RT_ERR_OK != rtk_ponmac_bwThreshold_set(pPonBwThreshold->bwThreshold, pPonBwThreshold->reqBwThreshold))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetUniPortRate(omci_port_rate_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (OMCI_UNI_RATE_DIRECTION_EGRESS == p->dir)
    {
        if (RT_ERR_OK != rtk_rate_portEgrBandwidthCtrlRate_set(p->port, p->rate))
            return RT_ERR_FAILED;
    }
    else
    {
        if (RT_ERR_OK != rtk_rate_portIgrBandwidthCtrlRate_set(p->port, p->rate))
            return RT_ERR_FAILED;
    }
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPauseControl(omci_port_pause_ctrl_t *p)
{
    rtk_port_phy_ability_t  phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    /*Only support enable / disable flow control*/
    if(0 == p->pause_time)
    {
        phyAbility.FC = 0;
        phyAbility.AsyFC = 0;
    }
    else
    {
        phyAbility.FC = 1;
        phyAbility.AsyFC = 1;
    }

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_set(p->port, &phyAbility))
            return RT_ERR_FAILED;


    /*For 9601B SFP*/
    if(RTL9601B_CHIP_ID == gPlatformDb.chipId)
    {
        rtk_port_macAbility_t macAbility;

        if (RT_ERR_OK != rtk_port_macForceAbility_get(p->port, &macAbility))
            return RT_ERR_FAILED;

        /*Only support enable / disable flow control*/
        if(0 == p->pause_time)
        {
            macAbility.rxFc = 0;
            macAbility.txFc = 0;
        }
        else
        {
            macAbility.rxFc = 1;
            macAbility.txFc = 1;
        }
        if (RT_ERR_OK != rtk_port_macForceAbility_set(p->port, macAbility))
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPauseControl(omci_port_pause_ctrl_t *p)
{
    rtk_port_phy_ability_t  phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_port_phyAutoNegoAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    p->pause_time = phyAbility.FC;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetDsBcGemFlow(unsigned int *pFlowId)
{
    if (!pFlowId)
        return RT_ERR_INPUT;

    // do nothing
    if (OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_APPLY))
        return RT_ERR_OK;

    if (OMCIDRV_FEATURE_ERR_FAIL ==
        omcidrv_feature_api(FEATURE_KAPI_BDP_00000010_SET_DSBC_GEMFLOW,
                            &gPlatformDb, *pFlowId, &g_dsBcGemTrapAcl))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TBD");
        return OMCI_ERR_FAILED;
    }
    // do something ok
    return RT_ERR_OK;
}

static int omci_CreateGroupMacFilterAcl(rtk_portmask_t portmask, unsigned char *mac, unsigned char *mac_mask, unsigned int etherTypeValue)
{
    int aclIdx;
    unsigned int maxAclIdx;
    rtk_acl_ingress_entry_t aclRule;
    rtk_acl_field_t aclField_mac, aclField_ethType;
    int ret;
    BOOL findAvailAcl = FALSE;

    /*Find un-used ACL index*/
    maxAclIdx = gPlatformDb.aclNum;
    maxAclIdx = gPlatformDb.aclActNum > maxAclIdx ?
        gPlatformDb.aclActNum : maxAclIdx;
    for (aclIdx = maxAclIdx - 1; aclIdx >= gPlatformDb.aclStartIdx; aclIdx--)
    {
        aclRule.index = aclIdx;

        if (RT_ERR_OK == rtk_acl_igrRuleEntry_get(&aclRule))
        {
            if (!aclRule.valid){
                findAvailAcl = TRUE;
                break;
        }
    }
    }
    if (findAvailAcl == FALSE)
        return -1;

    /*Action is filter, add ACL rule*/
    osal_memset(&aclRule, 0, sizeof(aclRule));
    osal_memset(&aclField_mac, 0, sizeof(aclField_mac));

    aclField_mac.fieldType = ACL_FIELD_DMAC;
    osal_memcpy(&aclField_mac.fieldUnion.mac.value.octet, mac, ETHER_ADDR_LEN);
    osal_memcpy(&aclField_mac.fieldUnion.mac.mask.octet, mac_mask, ETHER_ADDR_LEN);
    aclField_mac.next = NULL;
    rtk_acl_igrRuleField_add(&aclRule, &aclField_mac);

    if(0 != etherTypeValue)
    {
        osal_memset(&aclField_ethType, 0, sizeof(aclField_ethType));
        aclField_ethType.fieldType = ACL_FIELD_ETHERTYPE;
        aclField_ethType.fieldUnion.data.value = etherTypeValue;
        aclField_ethType.fieldUnion.data.mask = 0xFFFF;
        aclField_ethType.next = NULL;
        rtk_acl_igrRuleField_add(&aclRule, &aclField_ethType);
    }

    aclRule.index = aclIdx;
    aclRule.valid = ENABLED;
    aclRule.templateIdx = 0;
    aclRule.act.enableAct[ACL_IGR_FORWARD_ACT]= ENABLED;
    aclRule.act.forwardAct.act = ACL_IGR_FORWARD_DROP_ACT;
    osal_memcpy(&aclRule.activePorts, &portmask, sizeof(rtk_portmask_t));
    if( RT_ERR_OK != (ret = rtk_acl_igrRuleEntry_add(&aclRule)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add acl rule %u failed, return code %d", aclIdx, ret);
        return -1;
    }

    return aclIdx;
}

static int pf_rtl96xx_SetGroupMacFilter(OMCI_GROUPMACFILTER_ts *pGroupMacFilter)
{
    unsigned int i, etherTypeValue;
    int ret;
    int aclIdx;
    rtk_portmask_t portmask;
    rtk_enable_t aclState;
    unsigned char mac[ETHER_ADDR_LEN];
    unsigned char mac_mask[ETHER_ADDR_LEN];

    RTK_PORTMASK_RESET(portmask);
    /*Get active portmask.*/
    if(pGroupMacFilter->portIdx == gPlatformDb.ponPort)
    {
        /*TBD: Per UNI port setting*/
        portmask.bits[0] = gPlatformDb.uniPortMask;

        /*Enable ACL state*/
        for(i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
        {
            if((portmask.bits[0] >> i) & 0x1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrState_get(i, &aclState));
                if(DISABLED == aclState)
                {
                    ret = rtk_acl_igrState_set(i, ENABLED);
                    if(ret != RT_ERR_OK)
                    {
                        OMCI_LOG (OMCI_LOG_LEVEL_DBG, "Failed to enable igrState of ACL\n");
                    }
                }
            }
        }
    }
    else
    {
        RTK_PORTMASK_PORT_SET(portmask, gPlatformDb.ponPort);

        /*Enable ACL state*/
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrState_get(gPlatformDb.ponPort, &aclState));
        if(DISABLED == aclState)
        {
            ret = rtk_acl_igrState_set(gPlatformDb.ponPort, ENABLED);
            if(ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Failed to enable igrState of ACL\n");
            }
        }
    }

    /*Set Group MAC filter rules*/

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_IPV4_MCAST])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_IPV4_MCAST] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_MCAST][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_MCAST][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_MCAST][0] = -1;
            }
            return RT_ERR_OK;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_MCAST][0] == -1)
        {
            /*Action is filter, add ACL rule*/
            etherTypeValue = 0;
            mac[0] = 0x01; mac[1] = 0x00; mac[2] = 0x5e; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0x10; mac_mask[4] = 0x00; mac_mask[5] = 0x00;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_MCAST][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_IPV6_MCAST])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_IPV6_MCAST] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV6_MCAST][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV6_MCAST][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV6_MCAST][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV6_MCAST][0] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x33; mac[1] = 0x33; mac[2] = 0x00; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0x00; mac_mask[3] = 0x00; mac_mask[4] = 0x00; mac_mask[5] = 0x00;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV6_MCAST][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_IPV4_BCAST])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_IPV4_BCAST] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_BCAST][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_BCAST][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_BCAST][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_BCAST][0] == -1)
        {
            etherTypeValue = 0x0800;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPV4_BCAST][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_RARP])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_RARP] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_RARP][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_RARP][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_RARP][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_RARP][0] == -1)
        {
            etherTypeValue = 0x8035;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0x00; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_RARP][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_IPX])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_IPX] == MAC_FILTER_ACT_FORWARD)
        {
            for(i = 0; i < 3; i++)
            {
                if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][i] != -1)
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][i]));
                    groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][i] = -1;
                }
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][0] == -1)
        {
            etherTypeValue = 0x8137;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][0] = aclIdx;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][1] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x09; mac[1] = 0x00; mac[2] = 0x1B; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][1] = aclIdx;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][2] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x09; mac[1] = 0x00; mac[2] = 0x4E; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x02;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_IPX][2] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_NET_BEUI])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_NET_BEUI] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_NET_BEUI][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_NET_BEUI][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_NET_BEUI][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_NET_BEUI][0] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x03; mac[1] = 0x00; mac[2] = 0x00; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x02;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            /*record the acl index*/
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_NET_BEUI][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_APPLE_TALK])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_APPLE_TALK] == MAC_FILTER_ACT_FORWARD)
        {
            for(i = 0; i < 4; i++)
            {
                if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][i] != -1)
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][i]));
                    groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][i] = -1;
                }
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][0] == -1)
        {
            etherTypeValue = 0x809B;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0x00; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][0] = aclIdx;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][1] == -1)
        {
            etherTypeValue = 0x80F3;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][1] = aclIdx;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][2] == -1)
        {
            etherTypeValue = 0;
            /*????*/
            mac[0] = 0x09; mac[1] = 0x00; mac[2] = 0x07; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0x00;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][2] = aclIdx;
        }

        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][3] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x09; mac[1] = 0x00; mac[2] = 0x07; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_APPLE_TALK][3] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_BPDU])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_BPDU] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_BPDU][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_BPDU][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_BPDU][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_BPDU][0] == -1)
        {
            etherTypeValue = 0;
            mac[0] = 0x01; mac[1] = 0x80; mac[2] = 0xC2; mac[3] = 0x00; mac[4] = 0x00; mac[5] = 0x00;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0xFF; mac_mask[5] = 0x00;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            /*record the acl index*/
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_BPDU][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_ARP])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_ARP] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_ARP][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_ARP][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_ARP][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_ARP][0] == -1)
        {
            etherTypeValue = 0x0806;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0x00; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            /*record the acl index*/
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_ARP][0] = aclIdx;
        }
    }

    if(pGroupMacFilter->protocol_mask[GROUP_MAC_PROTOCOL_PPPOE_BCAST])
    {
        /*Action is forward, delete ACL rule*/
        if(pGroupMacFilter->protocol_act[GROUP_MAC_PROTOCOL_PPPOE_BCAST] == MAC_FILTER_ACT_FORWARD)
        {
            if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_PPPOE_BCAST][0] != -1)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_acl_igrRuleEntry_del(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_PPPOE_BCAST][0]));
                groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_PPPOE_BCAST][0] = -1;
            }
            return RT_ERR_OK;
        }

        /*Action is filter, add ACL rule*/
        if(groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_PPPOE_BCAST][0] == -1)
        {
            etherTypeValue = 0x8863;
            mac[0] = 0xFF; mac[1] = 0xFF; mac[2] = 0xFF; mac[3] = 0xFF; mac[4] = 0xFF; mac[5] = 0xFF;
            mac_mask[0] = 0xFF; mac_mask[1] = 0xFF; mac_mask[2] = 0xFF; mac_mask[3] = 0xFF; mac_mask[4] = 0x00; mac_mask[5] = 0xFF;
            aclIdx = omci_CreateGroupMacFilterAcl(portmask, mac, mac_mask, etherTypeValue);
            /*record the acl index*/
            groupMacAclIdx.aclIdx[GROUP_MAC_PROTOCOL_PPPOE_BCAST][0] = aclIdx;
        }
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetSigParameter(OMCI_SIGNAL_PARA_ts *pSigPara)
{
    int ret = RT_ERR_OK;

    rtk_gpon_sig_para_t gpon_sig_para;

    memset(&gpon_sig_para,0,sizeof(gpon_sig_para));
    memcpy(&gpon_sig_para, pSigPara, sizeof(rtk_gpon_sig_para_t));

    ret = rtk_gponapp_signal_parameter_set(&gpon_sig_para);
    return ret;
}

static int pf_rtl96xx_SetDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    int32                           ret;
    rtk_rate_storm_group_t          stormType;
    rtk_rate_storm_group_ctrl_t     stormCtrl;
    unsigned int                    cirInKbps;
    unsigned int                    portId;

    if (!pDot1RateMeter)
        return RT_ERR_INPUT;

    if (pDot1RateMeter->dot1Rate.type >= OMCI_DOT1_RATE_TYPE_END)
        return RT_ERR_INPUT;

    ret = rtk_rate_shareMeterMode_set(pDot1RateMeter->meterId, METER_MODE_BIT_RATE);
    if (RT_ERR_OK != ret && RT_ERR_DRIVER_NOT_FOUND != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set shared meter mode failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    ret = rtk_rate_shareMeterBucket_set(pDot1RateMeter->meterId, pDot1RateMeter->dot1Rate.cbs);
    if (RT_ERR_OK != ret && RT_ERR_INPUT != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set shared meter bucket failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    cirInKbps = pDot1RateMeter->dot1Rate.cir * 8 / 1024;

    ret = rtk_rate_shareMeter_set(pDot1RateMeter->meterId, cirInKbps, DISABLED);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set shared meter rate failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    ret = rtk_rate_stormControlEnable_get(&stormCtrl);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "get system storm control state failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    switch (pDot1RateMeter->dot1Rate.type)
    {
        case OMCI_DOT1_RATE_UNICAST_FLOOD:
            stormType = STORM_GROUP_UNKNOWN_UNICAST;
            stormCtrl.unknown_unicast_enable = ENABLED;
            break;
        case OMCI_DOT1_RATE_BROADCAST:
            stormType = STORM_GROUP_BROADCAST;
            stormCtrl.broadcast_enable = ENABLED;
            break;
        case OMCI_DOT1_RATE_MULTICAST_PAYLOAD:
            stormType = STORM_GROUP_MULTICAST;
            stormCtrl.multicast_enable = ENABLED;
            break;
        default:
            stormType = STORM_GROUP_END;
            break;
    }

    ret = rtk_rate_stormControlEnable_set(&stormCtrl);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "set system storm control state failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (!((1 << portId) & pDot1RateMeter->dot1Rate.portMask))
            continue;

        ret = rtk_rate_stormControlMeterIdx_set(portId, stormType, pDot1RateMeter->meterId);
        if (RT_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "set port storm control meter failed, ret %d", ret);

            return RT_ERR_FAILED;
        }

        ret = rtk_rate_stormControlPortEnable_set(portId, stormType, ENABLED);
        if (RT_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "set port storm control enable failed, ret %d", ret);

            return RT_ERR_FAILED;
        }
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_DelDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    int32                   ret;
    rtk_rate_storm_group_t  stormType;
    unsigned int            portId;

    if (!pDot1RateMeter)
        return RT_ERR_INPUT;

    switch (pDot1RateMeter->dot1Rate.type)
    {
        case OMCI_DOT1_RATE_UNICAST_FLOOD:
            stormType = STORM_GROUP_UNKNOWN_UNICAST;
            break;
        case OMCI_DOT1_RATE_BROADCAST:
            stormType = STORM_GROUP_BROADCAST;
            break;
        case OMCI_DOT1_RATE_MULTICAST_PAYLOAD:
            stormType = STORM_GROUP_MULTICAST;
            break;
        default:
            stormType = STORM_GROUP_END;
            break;
    }

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (!((1 << portId) & pDot1RateMeter->dot1Rate.portMask))
            continue;

        ret = rtk_rate_stormControlPortEnable_set(portId, stormType, DISABLED);
        if (RT_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "set port storm control disable failed, ret %d", ret);

            return RT_ERR_FAILED;
        }
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetBridgeTblPerPort(omci_bridge_tbl_per_port_t *p)
{
    rtk_l2_addr_table_t     l2table;
    uint32                  index = 0, addr, ageTime = 0, ageTimeInSecPerUnit = 0;
    unsigned short          information = 0xFFFF;
    unsigned char           *pPos = NULL;


    if (!p)
        return RT_ERR_INPUT;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "%s() portId=%u\n", __FUNCTION__,
                        p->portId);

    memset(gPlatformDb.pMmap, 0, MMT_BUF_SIZE);
    pPos = gPlatformDb.pMmap;

    if (RT_ERR_OK != (rtk_l2_aging_get(&ageTime)))
    {
        ageTimeInSecPerUnit = DEFAULT_AGING_TIME_IN_SEC / DEFAULT_AGING_TIME_UNIT;
    }
    else
    {
        ageTimeInSecPerUnit = (ageTime / 10) / DEFAULT_AGING_TIME_UNIT;
    }


    addr = index;
    do
    {
        memset(&l2table, 0x0, sizeof(rtk_l2_addr_table_t));
        index = addr;
        l2table.method = LUT_READ_METHOD_ADDRESS;
        l2table.entryType = RTK_LUT_END;
        if(RT_ERR_OK == (rtk_l2_nextValidEntry_get(&addr,&l2table)))
        {

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                                "%s() %d\n",
                                __FUNCTION__, __LINE__);
            /* Wrap around */
            if(addr < index)
            {
                break;
            }
            if (p->portId == l2table.entry.l2UcEntry.port)
            {
                unsigned short age = l2table.entry.l2UcEntry.age * ageTimeInSecPerUnit;
                unsigned short is_dynamic = (l2table.entry.l2UcEntry.flags & RTK_L2_UCAST_FLAG_STATIC ? 0 : 1);
                unsigned short is_filter = (l2table.entry.l2UcEntry.flags & RTK_L2_UCAST_FLAG_DA_BLOCK ? 1 : 0);
                information = ((age & 0xFFF) << 4) | ((is_dynamic & 0x1) << 2) | (is_filter & 0x1);
                memcpy(pPos, &information, sizeof(unsigned short));
                pPos += sizeof(unsigned short);
                memcpy(pPos, l2table.entry.l2UcEntry.mac.octet, ETHER_ADDR_LEN);
                pPos += ETHER_ADDR_LEN;
                p->cnt++;
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "l2 entry learn count=%u, age=%u, is_dynamic=%u, is_filter=%u, max=%02x:%02x:%02x:%02x:%02x:%02x\n",
                        p->cnt, age, is_dynamic, is_filter,
                        l2table.entry.l2UcEntry.mac.octet[0],
                        l2table.entry.l2UcEntry.mac.octet[1],
                        l2table.entry.l2UcEntry.mac.octet[2],
                        l2table.entry.l2UcEntry.mac.octet[3],
                        l2table.entry.l2UcEntry.mac.octet[4],
                        l2table.entry.l2UcEntry.mac.octet[5]);
            }
            //addr++;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                                "%s() %d, addr=%x\n",
                                __FUNCTION__, __LINE__, addr);
            if (addr > gPlatformDb.maxLearnCnt)
                break;
        }
        addr++;
    } while(1);

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetMacAgeTime(unsigned int ageTime)
{
    return  rtk_l2_aging_set(ageTime * 10);
}


static int pf_rtl96xx_SetLoidAuthStatus(omci_event_msg_t *p)
{
    uint8 authStatus = p->status;

    if((authStatus != PON_ONU_LOID_INITIAL_STATE) && (authStatus != PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION))
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_pon_led_status_set(PON_LED_PON_MODE_GPON, PON_LED_STATE_AUTH_NG));
    }
    else if(authStatus == PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_pon_led_status_set(PON_LED_PON_MODE_GPON, PON_LED_STATE_AUTH_OK));

        queue_broadcast(MSG_TYPE_OMCI_EVENT, p->subType, p->bitMask, p->status);
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SendOmciEvent(omci_event_msg_t *p)
{
    queue_broadcast(MSG_TYPE_OMCI_EVENT,
        p->subType, p->bitMask, p->status);

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetForceEmergencyStop(unsigned int *pState)
{
    int32   ret;

    if (!pState)
        return RT_ERR_INPUT;

    ret = rtk_gponapp_forceEmergencyStop_set(*pState);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "force emergency stop failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}


static int pf_rtl96xx_SetPortBridging(unsigned int enable)
{
    rtk_port_t port;
    rtk_portmask_t portmask;
    rtk_portmask_t extPortmask;

    for (port = gPlatformDb.etherPortMin; port <= gPlatformDb.etherPortMax; port++)
    {
        if(!(gPlatformDb.etherPortMask & (1 << port)))
            continue;

        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_port_isolationEntry_get(0, port, &portmask, &extPortmask));

        if(enable)
        {
            /*All LAN port can bridge, so port mask is all*/
            portmask.bits[0] = gPlatformDb.uniPortMask | (1 << gPlatformDb.ponPort);
        } else {
            /*only can send to CPU and WAN*/
            portmask.bits[0] = (1 << gPlatformDb.ponPort) | (1 << gPlatformDb.cpuPort);
        }

        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_port_isolationEntry_set(0, port, &portmask, &extPortmask));
    }
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetFloodingPortMask(omci_flood_port_info *p)
{
    int32                   ret;
    rtk_l2_lookupMissType_t type;
    rtk_portmask_t          old_pm, new_pm;

    if (!p)
        return RT_ERR_INPUT;

    switch (p->type)
    {
        case OMCI_FLOOD_UNICAST:
            type = DLF_TYPE_UCAST;
            break;
        case OMCI_FLOOD_BROADCAST:
            type = DLF_TYPE_BCAST;
            break;
        case OMCI_FLOOD_MULTICAST:
            type = DLF_TYPE_MCAST;
            break;
        default:
            return RT_ERR_INPUT;
    }

    if (RT_ERR_OK != (ret = rtk_l2_lookupMissFloodPortMask_get(type, &old_pm)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "get lookup miss flood port mask failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    switch (p->act)
    {
        case OMCI_LOOKUP_MISS_ACT_DROP:
            new_pm.bits[0] = (old_pm.bits[0] & ~(p->portMask));
            break;
        case OMCI_LOOKUP_MISS_ACT_FLOOD:
            new_pm.bits[0] = (old_pm.bits[0] | (p->portMask));
            break;
        default:
            return RT_ERR_INPUT;
    }

    if (RT_ERR_OK != (ret = rtk_l2_lookupMissFloodPortMask_set(type, &new_pm)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "set lookup miss flood port mask failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "set flooding port mask= %x", new_pm.bits[0]);

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetTodInfo ( omci_tod_info_t* pTodInfo)
{
    int32 ret = RT_ERR_FAILED;
    rtk_enable_t enable = DISABLED;
    rtk_pon_tod_t ponTod;
    unsigned long long uTimeStampSecs;

    if (gPlatformDb.chipId == RTL9601B_CHIP_ID ||
            gPlatformDb.chipId == APOLLOMP_CHIP_ID) {
        return (RT_ERR_INPUT);
    }

    if (!pTodInfo) {
        return (RT_ERR_INPUT);
    }

    memset (&ponTod, 0x00, sizeof (rtk_pon_tod_t));

    /* Avoid compiler warning when printing */
    uTimeStampSecs = pTodInfo->uTimeStampSecs;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "%s: superframe 0x%x, sec %llu, ns %u\n",
        __FUNCTION__,
        pTodInfo->uSeqNumOfGemSuperframe,
        uTimeStampSecs,
        pTodInfo->uTimeStampNanosecs);

    ponTod.ponMode = TOD_PON_MODE_GPON;
    ponTod.startPoint.superFrame = pTodInfo->uSeqNumOfGemSuperframe;
    ponTod.timeStamp.sec         = pTodInfo->uTimeStampSecs;
    ponTod.timeStamp.nsec         = pTodInfo->uTimeStampNanosecs;
    enable = ENABLED;

    ret = rtk_time_ponTodTime_set (ponTod);
    if (ret != RT_ERR_OK) {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "Failed to set time of ToD, ret %d", ret);
        goto END_SETTODINFO;
    }

    ret = rtk_time_todEnable_set (enable);
    if (ret != RT_ERR_OK) {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "Failed to enable ToD, ret %d", ret);
    }

END_SETTODINFO:
    return (ret);
}

static void change_cf_priority(unsigned char enable)
{
    l2_service_t            *pL2Entry = NULL;
    struct list_head        *next = NULL, *tmp=NULL;
    rtk_classify_cfg_t      oldCfg, newCfg;
    OMCI_VLAN_OPER_ts       dsRule;

    omci_rule_pri_t rule_pri;

    int            dsAclIndex = OMCI_UNUSED_ACL;

    list_for_each_safe(next, tmp, &gPlatformDb.l2Head)
    {
        pL2Entry = list_entry(next, l2_service_t, list);

        if (PON_GEMPORT_DIRECTION_BI != pL2Entry->dir)
            continue;

        memset(&oldCfg, 0, sizeof(rtk_classify_cfg_t));
        oldCfg.index = pL2Entry->dsCfIndex;
        if (RT_ERR_OK != rtk_classify_cfgEntry_get(&oldCfg))
            continue;
        if (!oldCfg.valid)
            continue;
        if (CLASSIFY_CF_PRI_ACT_ASSIGN != oldCfg.act.dsAct.interPriAct)
            continue;

        memset(&newCfg, 0, sizeof(rtk_classify_cfg_t));
        memset(&dsRule, 0, sizeof(OMCI_VLAN_OPER_ts));

        omci_SetUsRuleInvert2Ds(&dsRule, &(pL2Entry->rule));

        /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
        /* Setup ACL rule before create CF rule */
        omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_DS, &dsRule, pL2Entry->uniMask, pL2Entry->dsStreamId, &dsAclIndex);

        memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

        newCfg.invert = 0;
        newCfg.valid = ENABLED;
        newCfg.direction = CLASSIFY_DIRECTION_DS;
        newCfg.index = pL2Entry->dsCfIndex;

        omci_SetClassifyDsAct(&dsRule, (pL2Entry->uniMask & ~(1 << gPlatformDb.ponPort)), &newCfg.act.dsAct);

        omci_SetClassifyDsRule(&dsRule, &pL2Entry->rule, pL2Entry->dsStreamId, &newCfg, &rule_pri, dsAclIndex);
        if (enable)
        {
            flow2DsPq_entry_t   *pFlow2DsPqEntry = NULL;

            pFlow2DsPqEntry = flow2DsPq_entry_find(pL2Entry->dsStreamId);

            if (!pFlow2DsPqEntry)
                continue;

            if (0xFFFF == pFlow2DsPqEntry->dsQ.dsPqOmciPri)
                continue;

            if (PQ_DROP_COLOUR_NO_MARKING != pFlow2DsPqEntry->dsQ.dpMarking || g_isDsDeiDpAclCfg)
                continue;

            if ((1 << pFlow2DsPqEntry->dsQ.portId) &  pL2Entry->uniMask)
            {
                newCfg.act.dsAct.interPriAct = CLASSIFY_CF_PRI_ACT_ASSIGN;
                newCfg.act.dsAct.cfPri = pFlow2DsPqEntry->dsQ.priority;
            }
        }
        else
        {
            newCfg.act.dsAct.interPriAct = CLASSIFY_CF_PRI_ACT_NOP;
        }

        if (RT_ERR_OK != rtk_classify_cfgEntry_add(&newCfg))
            continue;

    }

}

static int pf_rtl96xx_SetUniQosInfo(omci_uni_qos_info_t* p)
{
    int                     ret     = RT_ERR_OK, i;
    uniQos_entry_t          *pEntry = NULL;
    rtk_qos_queue_weights_t weights;
    rtk_qos_pri2queue_t     pri2Qid;
    struct list_head        *pFlow2DsPqEntry     = NULL;
    struct list_head        *pFlow2DsPqTmpEntry  = NULL;
    flow2DsPq_entry_t       *pFlow2DsPqEntryData = NULL;

    pEntry = uni_qos_entry_find(p->port);

    if (pEntry)
    {
        memcpy(&(pEntry->uniQos), p, sizeof(omci_uni_qos_info_t));
    }
    else
    {
        if (OMCI_ERR_OK != uni_qos_entry_add(p))
           return RT_ERR_FAILED;
    }

    memset(&weights, 0x00, sizeof(rtk_qos_queue_weights_t));

    if (p->valid)
    {
        // modify downstream classify rule: CF priority action should be changed to follow switch core
        change_cf_priority(FALSE);

        // set priority to queue of uni port
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_portPriMap_set(p->port, 0));
        memset(&pri2Qid, 0x00, sizeof(rtk_qos_pri2queue_t));
        memcpy(&pri2Qid, p->pri2queue, sizeof(rtk_qos_pri2queue_t));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priMap_set(0, &pri2Qid));

        // set weight per egress queue of uni port
        memcpy(&weights, p->weights, sizeof(rtk_qos_queue_weights_t));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG,  rtk_qos_schedulingQueue_set(p->port, &weights));
    }
    else
    {
        // restore downstream classify rule: CF priority action should be changed to assign CF priority
        change_cf_priority(TRUE);

        // restore priority to queue mapping for table index 0 which is used to uni port
        for (i = 0; i < RTK_MAX_NUM_OF_PRIORITY; i++)
            pri2Qid.pri2queue[i] = i;
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_qos_priMap_set(0, &pri2Qid));

        // follow standard weight per egress queue of uni port

        list_for_each_safe(pFlow2DsPqEntry, pFlow2DsPqTmpEntry, &gPlatformDb.flow2DsPqHead)
        {
            pFlow2DsPqEntryData = list_entry(pFlow2DsPqEntry, flow2DsPq_entry_t, list);

            if (pFlow2DsPqEntryData && pFlow2DsPqEntryData->dsQ.portId == p->port)
            {
                if (pFlow2DsPqEntryData->dsQ.policy == PQ_POLICY_WEIGHTED_ROUND_ROBIN)
                {

                    weights.weights[pFlow2DsPqEntryData->dsQ.priority] = pFlow2DsPqEntryData->dsQ.weight;
                }
            }
        }
        if (RT_ERR_OK != (ret = rtk_qos_schedulingQueue_set(p->port, &weights)))
            printk("%s %d ret=%d\n", __FUNCTION__, __LINE__, ret);

    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DumpUniQosInfo(void)
{
    struct list_head        *pEntry     = NULL;
    struct list_head        *pTmpEntry  = NULL;
    uniQos_entry_t          *pEntryData = NULL;


    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.uniQosHead)
    {
        pEntryData = list_entry(pEntry, uniQos_entry_t, list);

        printk("\n");
        printk("\nUNI port ID:%5u\n", pEntryData->uniQos.port);
        printk("Valid:%14s\n", (pEntryData->uniQos.valid ? "True" : "False"));
        switch (pEntryData->uniQos.policy)
        {
            case 0:
                printk("Schedule mode:\t%s\n", "sp");
                break;
            case 1:
                printk("Schedule mode:\t%s\n", "wrr");
                break;
            case 2:
                printk("Schedule mode:\t%s\n", "sp+wrr");
                break;
            default:
                printk("Schedule mode:\t%s\n", "default");

        }
        printk("queue map(prio->queue):%6s 0->%u%5s1->%u%5s2->%u%5s3->%u%5s4->%u%5s5->%u%5s6->%u%5s7->%u\n", " ",
                pEntryData->uniQos.pri2queue[0], " ",
                pEntryData->uniQos.pri2queue[1], " ",
                pEntryData->uniQos.pri2queue[2], " ",
                pEntryData->uniQos.pri2queue[3], " ",
                pEntryData->uniQos.pri2queue[4], " ",
                pEntryData->uniQos.pri2queue[5], " ",
                pEntryData->uniQos.pri2queue[6], " ",
                pEntryData->uniQos.pri2queue[7]);


        printk("queue weight(queue->weight):%s 0->%u%5s1->%u%5s2->%u%5s3->%u%5s4->%u%5s5->%u%5s6->%u%5s7->%u\n", " ",
                pEntryData->uniQos.weights[0], " ",
                pEntryData->uniQos.weights[1], " ",
                pEntryData->uniQos.weights[2], " ",
                pEntryData->uniQos.weights[3], " ",
                pEntryData->uniQos.weights[4], " ",
                pEntryData->uniQos.weights[5], " ",
                pEntryData->uniQos.weights[6], " ",
                pEntryData->uniQos.weights[7]);
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DumpDebugInfo(void)
{
    struct list_head *next,*tmp;
    uint32 i;
    ext_filter_service_t    *ptr = NULL;
    dp_stag_acl_entry_t     *pEntryData = NULL;

    printk("======== OMCI extend filtering entry Rule ============\n");

    list_for_each_safe(next, tmp, &gPlatformDb.efHead)
    {
        ptr = list_entry(next, ext_filter_service_t, list);
        if (ptr)
        {
            printk("rule_id=%u, ref_cnt=%u, activePortMask=%u, templateIdx=%u\n",
                ptr->rule_id, ptr->ref_cnt, ptr->rule.activePortMask,
                ptr->rule.templateIdx);

            for (i = 0; i  < 8; i++)
            {
                printk("value=%04x, mask=%04x\n", ptr->rule.filters.field[i].value, ptr->rule.filters.field[i].mask);
            }

         }
    }
    printk("=======================================End.\n\n");


    printk("======== OMCI drop precedence us stag entry Rule ============\n");

    list_for_each_safe(next, tmp, &gPlatformDb.dpStagAclHead)
    {
        pEntryData = list_entry(next, dp_stag_acl_entry_t, list);

        if (pEntryData)
        {
            for (i = 0; i < gPlatformDb.aclNum; i++)
            {
                if (pEntryData->pAclIdBitmap[i / (sizeof(unsigned int) * 8)] & (1 << (i % (sizeof(unsigned int) * 8))))
                {
                    printk("usFlowId=%d, dpMarking=%u, pAclIdBitmap=%p, acl_rule_index=%u, uniMask=%x\n",
                        pEntryData->usFlowId, pEntryData->dpMarking, pEntryData->pAclIdBitmap, i, pEntryData->uniMask);
                }
            }


        }
    }
    printk("=======================================End.\n");


    _debug_dump_flow_to_ds_pq_info();

    _debug_dump_tcont_info();

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetOmciMirror(unsigned int *p)
{
    rtk_gponapp_omci_mirror_set(p);

    return OMCI_ERR_OK;
}

int omci_ioctl_callback(rtk_gpon_extMsg_t *pExtMsg)
{
    return  omcidrv_ioctl((OMCI_IOCTL_t)pExtMsg->optId,(void*)pExtMsg->extValue);
}

static void gpon_state_change(intrBcasterMsg_t  *pMsgData)
{
    int ret = OMCI_ERR_OK;

    if (MSG_TYPE_ONU_STATE == pMsgData->intrType &&
        gPlatformDb.ponPort ==  pMsgData->intrBitMask)
    {
        ret = rtk_pon_led_status_set(PON_LED_PON_MODE_GPON, (uint32)pMsgData->intrSubType);

        /*When onu status change to O5, reset PON port theashold to default value (small),
          for Huawei OLT set small bwmap to OMCC when ONU unauthenticated,*/
        if(gPlatformDb.chipId == APOLLOMP_CHIP_ID)
        {
            if((pMsgData->intrSubType == GPON_STATE_O5))
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_ponmac_bwThreshold_set(21, 14));
            }
        }
    }

    return;
}

static void gpon_alarm_callback(rtk_gpon_alarm_type_t alarmType, int32 set)
{
    rtk_gpon_onuState_t onuState;

    if (RT_ERR_OK == rtk_gpon_onuState_get(&onuState) &&
        GPON_STATE_O5 == onuState)
    {
        rtk_gpon_pkt_t  data;

        memset(&data, 0, sizeof(rtk_gpon_pkt_t));

        data.type = RTK_GPON_MSG_ALARM;
        data.msg.alarm.msg[0] = (uint8)alarmType;
        data.msg.alarm.msg[1] = (uint8)set;

        if(RT_ERR_OK != pkt_redirect_kernelApp_sendPkt(PR_USER_UID_GPONOMCI,
            1, sizeof(rtk_gpon_alarm_t) + sizeof(int), (unsigned char *)&data))
        {
            return;
        }
    }
    return;
}


static pf_wrapper_t rtl96xx_mapper =
{
    .pf_ResetMib            =pf_rtl96xx_ResetMib,
    .pf_GetSerialNum        =pf_rtl96xx_GetSerialNum,
    .pf_SetSerialNum        =pf_rtl96xx_SetSerialNum,
    .pf_ClearPriQueue       =pf_rtl96xx_ClearPriQueue,
    .pf_SetPriQueue         =pf_rtl96xx_SetPriQueue,
    .pf_ActiveBdgConn       =__pf_rtl96xx_ActiveBdgConn,
    .pf_DeactiveBdgConn     =__pf_rtl96xx_DeactiveBdgConn,
    .pf_CfgGemFlow          =pf_rtl96xx_CfgGemFlow,
    .pf_CreateTcont         =pf_rtl96xx_CreateTcont,
    .pf_UpdateTcont         =pf_rtl96xx_UpdateTcont,
    .pf_DumpL2Serv          =pf_rtl96xx_DumpL2Serv,
    .pf_DumpVeipServ        =pf_rtl96xx_DumpVeipServ,
    .pf_DumpMBServ          =pf_rtl96xx_DumpMBServ,
    .pf_DumpCfMap           =pf_rtl96xx_DumpCfMap,
    .pf_SetCfMap            =pf_rtl96xx_SetCfMap,
    .pf_SetDscpRemap        =pf_rtl96xx_SetDscpRemap,
    .pf_SetGponPasswd       =pf_rtl96xx_SetGponPasswd,
    .pf_ActivateGpon        =pf_rtl96xx_ActivateGpon,
    .pf_UpdateVeipRule      =pf_rtl96xx_UpdateVeipRule,
    .pf_SetMacLearnLimit    =pf_rtl96xx_SetMacLearnLimit,
    .pf_SetMacFilter        =pf_rtl96xx_SetMacFilter,
    .pf_GetDevCapabilities          = pf_rtl96xx_GetDevCapabilities,
    .pf_GetDevIdVersion             = pf_rtl96xx_GetDevIdVersion,
    .pf_GetUsDBRuStatus             = pf_rtl96xx_GetUsDBRuStatus,
    .pf_GetTransceiverStatus        = pf_rtl96xx_GetTransceiverStatus,
    .pf_GetPortLinkStatus           = pf_rtl96xx_GetPortLinkStatus,
    .pf_GetPortSpeedDuplexStatus    = pf_rtl96xx_GetPortSpeedDuplexStatus,
    .pf_SetPortAutoNegoAbility      = pf_rtl96xx_SetPortAutoNegoAbility,
    .pf_GetPortAutoNegoAbility      = pf_rtl96xx_GetPortAutoNegoAbility,
    .pf_SetPortState                = pf_rtl96xx_SetPortState,
    .pf_GetPortState                = pf_rtl96xx_GetPortState,
    .pf_SetPortMaxFrameSize         = pf_rtl96xx_SetPortMaxFrameSize,
    .pf_GetPortMaxFrameSize         = pf_rtl96xx_GetPortMaxFrameSize,
    .pf_SetPortPhyLoopback          = pf_rtl96xx_SetPortPhyLoopback,
    .pf_GetPortPhyLoopback          = pf_rtl96xx_GetPortPhyLoopback,
    .pf_SetPortPhyPwrDown           = pf_rtl96xx_SetPortPhyPwrDown,
    .pf_GetPortPhyPwrDown           = pf_rtl96xx_GetPortPhyPwrDown,
    .pf_GetPortStat                 = pf_rtl96xx_GetPortStat,
    .pf_ResetPortStat               = pf_rtl96xx_ResetPortStat,
    .pf_GetUsFlowStat               = pf_rtl96xx_GetUsFlowStat,
    .pf_ResetUsFlowStat             = pf_rtl96xx_ResetUsFlowStat,
    .pf_GetDsFlowStat               = pf_rtl96xx_GetDsFlowStat,
    .pf_ResetDsFlowStat             = pf_rtl96xx_ResetDsFlowStat,
    .pf_GetDsFecStat                = pf_rtl96xx_GetDsFecStat,
    .pf_ResetDsFecStat              = pf_rtl96xx_ResetDsFecStat,
    .pf_SetSvlanTpid                = pf_rtl96xx_SetSvlanTpid,
    .pf_GetSvlanTpid                = pf_rtl96xx_GetSvlanTpid,
    .pf_GetCvlanState               = pf_rtl96xx_GetCvlanState,
    .pf_GetGemBlkLen                = pf_rtl96xx_GetGemBlkLen,
    .pf_SetGemBlkLen                = pf_rtl96xx_SetGemBlkLen,
    .pf_DumpMacFilter               = pf_rtl96xx_DumpMacFilter,
    .pf_DumpFlow2dsPq               = pf_rtl96xx_DumpFlow2dsPq,
    .pf_SetGroupMacFilter           = pf_rtl96xx_SetGroupMacFilter,
    .pf_GetDrvVersion               = pf_rtl96xx_GetDrvVersion,
    .pf_GetOnuState                 = pf_rtl96xx_GetOnuState,
    .pf_SetSigParameter             = pf_rtl96xx_SetSigParameter,
    .pf_SetPonBwThreshold           = pf_rtl96xx_SetPonBwThreshold,
    .pf_DumpVeipGemFlow             = NULL,
    .pf_SetVeipGemFlow              = NULL,
    .pf_DelVeipGemFlow              = NULL,
    .pf_SetUniPortRate              = pf_rtl96xx_SetUniPortRate,
    .pf_SetPauseControl             = pf_rtl96xx_SetPauseControl,
    .pf_GetPauseControl             = pf_rtl96xx_GetPauseControl,
    .pf_SetDsBcGemFlow              = pf_rtl96xx_SetDsBcGemFlow,
    .pf_SetDot1RateLimiter          = pf_rtl96xx_SetDot1RateLimiter,
    .pf_DelDot1RateLimiter          = pf_rtl96xx_DelDot1RateLimiter,
    .pf_GetBgTblPerPort             = pf_rtl96xx_GetBridgeTblPerPort,
    .pf_SetMacAgeTime               = pf_rtl96xx_SetMacAgeTime,
    .pf_SetLoidAuthStatus           = pf_rtl96xx_SetLoidAuthStatus,
    .pf_SendOmciEvent               = pf_rtl96xx_SendOmciEvent,
    .pf_SetForceEmergencyStop       = pf_rtl96xx_SetForceEmergencyStop,
    .pf_SetPortBridging             = pf_rtl96xx_SetPortBridging,
    .pf_SetFloodingPortMask         = pf_rtl96xx_SetFloodingPortMask,
    .pf_SetTodInfo                  = pf_rtl96xx_SetTodInfo,
    .pf_SetUniQosInfo               = pf_rtl96xx_SetUniQosInfo,
    .pf_DumpUniQos                  = pf_rtl96xx_DumpUniQosInfo,
    .pf_DumpDebugInfo               = pf_rtl96xx_DumpDebugInfo,
    .pf_ClearPPPoEDb                = NULL,
    .pf_SetOmciMirror               = pf_rtl96xx_SetOmciMirror,
};

static intrBcasterNotifier_t gponStateChangeNotifier = {
    .notifyType = MSG_TYPE_ONU_STATE,
    .notifierCb = gpon_state_change,
};

static int platform_db_init(void)
{
    rtk_switch_devInfo_t        devInfo;
    rt_register_capacity_t      *pCapacityInfo;
    rtk_gpon_schedule_info_t    scheInfo;
    int                         i;
    unsigned int                cid, rid, stype, utpPortNum = 0;

    memset(&gPlatformDb, 0, sizeof(pf_db_t));
    INIT_LIST_HEAD(&gPlatformDb.l2Head);
    INIT_LIST_HEAD(&gPlatformDb.veipHead);
    INIT_LIST_HEAD(&gPlatformDb.veipGemFlowHead);
    INIT_LIST_HEAD(&gPlatformDb.mbcastHead);
    INIT_LIST_HEAD(&gPlatformDb.macFilterHead);
    INIT_LIST_HEAD(&gPlatformDb.flow2DsPqHead);
    INIT_LIST_HEAD(&gPlatformDb.dpStagAclHead);
    INIT_LIST_HEAD(&gPlatformDb.dsDpGemStagAclHead);
    INIT_LIST_HEAD(&gPlatformDb.ucAggregatedGroupHead);
    INIT_LIST_HEAD(&gPlatformDb.mbAggregatedGroupHead);
    INIT_LIST_HEAD(&gPlatformDb.uniQosHead);
    INIT_LIST_HEAD(&gPlatformDb.efHead);
    INIT_LIST_HEAD(&gPlatformDb.flow2DsPqInfoHead);

    memset(&groupMacAclIdx, -1, sizeof(groupMacAclIdx));

    if (RT_ERR_OK != rtk_gpon_scheInfo_get(&scheInfo))
        return OMCI_ERR_FAILED;

    if (RT_ERR_OK != rtk_switch_deviceInfo_get(&devInfo))
        return OMCI_ERR_FAILED;
    pCapacityInfo = &devInfo.capacityInfo;

    // chip info
    gPlatformDb.chipId = devInfo.chipId;

    if (RT_ERR_OK != rtk_switch_version_get(&cid, &rid, &stype))
        return OMCI_ERR_FAILED;
    // chip revision
    gPlatformDb.chipRev = rid;
    gPlatformDb.chipSubtype = stype;

    // pon info
    gPlatformDb.omccFlow = scheInfo.omcc_flow;
    gPlatformDb.omccQueue = scheInfo.omcc_queue;
    gPlatformDb.omccTcont = scheInfo.omcc_tcont;
    gPlatformDb.maxTcont = pCapacityInfo->gpon_tcont_max;
    gPlatformDb.maxTcontQueue = pCapacityInfo->ponmac_tcont_queue_max;
    gPlatformDb.maxPonQueue = pCapacityInfo->max_num_of_pon_queue;
    gPlatformDb.maxQueueRate = pCapacityInfo->ponmac_pir_cir_rate_max;
    gPlatformDb.maxFlow = pCapacityInfo->classify_sid_max;
    gPlatformDb.sidMask = (1 << BIT_NUM(pCapacityInfo->classify_sid_max)) - 1;
    gPlatformDb.tCont = kmalloc(sizeof(pf_tcont_t) * gPlatformDb.maxTcont, GFP_KERNEL);
    if (!gPlatformDb.tCont)
        return OMCI_ERR_FAILED;
    for (i = 0; i < gPlatformDb.maxTcont; i++)
        gPlatformDb.tCont[i].allocId = 0xff;
    gPlatformDb.gemEncrypt = kmalloc(sizeof(unsigned char) * 512, GFP_KERNEL);

    // port info
    gPlatformDb.cpuPort = devInfo.cpuPort;
    gPlatformDb.rgmiiPort = devInfo.rgmiiPort;
    if (RT_ERR_OK != rtk_gpon_port_get((rtk_port_t *)&gPlatformDb.ponPort))
        gPlatformDb.ponPort = -1;
    gPlatformDb.etherPortMin = devInfo.ether.min;
    gPlatformDb.etherPortMax = devInfo.ether.max;
    for (i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if (i == gPlatformDb.cpuPort ||
                i == gPlatformDb.rgmiiPort ||
                i == gPlatformDb.ponPort)
            continue;

        gPlatformDb.etherPortMask |= (1 << i);
        utpPortNum++;
    }

    if(0 == utpPortNum)/*Check UTP number*/
        return OMCI_ERR_FAILED;

    gPlatformDb.allPortMin = devInfo.all.min;
    gPlatformDb.allPortMax = devInfo.all.max;
    for (i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
    {
        if (!((1 << i) & gPlatformDb.etherPortMask))
            continue;

        gPlatformDb.uniPortMask |= (1 << i);
    }
    gPlatformDb.uniPortMask |= (1 << gPlatformDb.cpuPort);
    gPlatformDb.perUniQueue = pCapacityInfo->max_num_of_queue;

    // qos
    gPlatformDb.maxPriSelWeight = pCapacityInfo->pri_sel_weight_max;
    gPlatformDb.perTContQueueDp = FALSE;
    gPlatformDb.perUNIQueueDp = FALSE;

    gPlatformDb.rsvMeterId = ((gPlatformDb.cpuPort % utpPortNum) * 8 + 7);
    gPlatformDb.meterNum = pCapacityInfo->max_num_of_metering;


    // cf
    gPlatformDb.cfTotalNum = pCapacityInfo->classify_entry_max;
    gPlatformDb.cfRule = kzalloc(sizeof(pf_cf_rule_t) * gPlatformDb.cfTotalNum, GFP_KERNEL);
    if (!gPlatformDb.cfRule)
        return OMCI_ERR_FAILED;
    switch (gPlatformDb.chipId)
    {
        case RTL9601B_CHIP_ID:
            /* reserve last entry for default drop */
            gPlatformDb.cfNum               = gPlatformDb.cfTotalNum - 1;

            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 31;
            gPlatformDb.ethTypeFilterStart  = 32;
            gPlatformDb.ethTypeFilterStop   = 63;
            gPlatformDb.l2CommStart         = 64;
            /* reserve last entry for default drop */
            gPlatformDb.l2CommStop          = gPlatformDb.cfNum - 1;

            break;
        case RTL9602C_CHIP_ID:
        {
            /* TBD: no need to reserve last entry for default drop, use ds-umatch-drop*/
            /* reserve last entry for default drop for test chip*/
            gPlatformDb.cfNum               = gPlatformDb.cfTotalNum - 1;

            /*Set pattern 1 number*/
            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 31;
            gPlatformDb.ethTypeFilterStart  = 32;
            gPlatformDb.ethTypeFilterStop   = 63;
            gPlatformDb.l2CommStart         = 64;
            gPlatformDb.l2CommStop          = gPlatformDb.cfNum - 1;

            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_entryNumPattern1_set(224));
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_classify_unmatchAction_ds_set(CLASSIFY_UNMATCH_DS_PERMIT));
            break;
        }
        case RTL9607C_CHIP_ID:
            gPlatformDb.rsvMeterId = 31;
            gPlatformDb.cfNum               = gPlatformDb.cfTotalNum - 2;

            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 63;
            gPlatformDb.ethTypeFilterStart  = 64;
            gPlatformDb.ethTypeFilterStop   = 127;
            gPlatformDb.l2CommStart         = 128;
            gPlatformDb.l2CommStop          = gPlatformDb.cfNum - 1;
            break;
        case APOLLOMP_CHIP_ID:
        default:
        {
            /* reserve last 2 entry for default drop and OMCI internal pri rule*/
            gPlatformDb.cfNum               = gPlatformDb.cfTotalNum - 2;

            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 63;
            gPlatformDb.ethTypeFilterStart  = 64;
            gPlatformDb.ethTypeFilterStop   = 127;
            gPlatformDb.l2CommStart         = 128;
            gPlatformDb.l2CommStop          = gPlatformDb.cfNum - 1;
            break;
        }
    }

    // acl
    gPlatformDb.aclNum = pCapacityInfo->max_num_of_acl_rule_entry;
    gPlatformDb.aclActNum = pCapacityInfo->max_num_of_acl_action;

    // l34
    gPlatformDb.intfNum = pCapacityInfo->l34_netif_table_max;
    gPlatformDb.bindingTblMax = pCapacityInfo->l34_binding_table_max;

    // l2
    gPlatformDb.maxLearnCnt = pCapacityInfo->l2_learn_limit_cnt_max;

    // callback
    gPlatformDb.pMap = &rtl96xx_mapper;

    return OMCI_ERR_OK;
}

#define DEV_LAN_MAC_ADDR_FILE "/sys/class/net/br0/address"
#define DEV_WAN_MAC_ADDR_FILE "/sys/class/net/pon0/address"
static int get_dev_mac_addr(char *pFileName, unsigned char *p)
{
    mm_segment_t    oldfs;
    struct file     *fp = NULL;
    char            buf[READ_BUF_LEN];
    char            *strptr = NULL, *tokptr = NULL;
    unsigned char   index = 0, tmp_byte;
    unsigned char   *p_cur = NULL;
    char            *endCP;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    memset(buf, 0, READ_BUF_LEN);

    fp = filp_open(pFileName, O_RDONLY, 0);

    if (IS_ERR(fp))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, ">> file open failed << \n\n");
        goto get_dev_mac_addr_finish;
    }
    if (!(fp->f_op))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, ">> file op failed << \n\n");
        goto get_dev_mac_addr_close_fp;
    }
    if (!(fp->f_op->read))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, ">> read failed << \n\n");
        goto get_dev_mac_addr_close_fp;
    }
    if (fp->f_op->read(fp, buf, READ_BUF_LEN, &fp->f_pos) <= 0)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, ">> cannot read << \n\n");
        goto get_dev_mac_addr_close_fp;
    }

    strptr = buf;
    p_cur = p;

    for (index = 0; index < ETHER_ADDR_LEN ; index++)
    {
        tokptr = strsep(&strptr, ":");
        if(tokptr != NULL) {
            tmp_byte = (unsigned char)(simple_strtoul(tokptr, &endCP, 16));
            memcpy((p_cur + index), &tmp_byte, sizeof(unsigned char));
        }
        else {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "It should not get here, please check DEV MAC\n");
        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "get_dev_mac_addr =%02X:%02X:%02X:%02X:%02X:%02X\n\n",
                p[0], p[1], p[2], p[3], p[4], p[5]);
get_dev_mac_addr_close_fp:
    filp_close(fp, NULL);

get_dev_mac_addr_finish:
    set_fs(oldfs);

    return OMCI_ERR_OK;
}

static int omci_device_event(struct notifier_block *nb, unsigned long event, void *ptr)
{
    unsigned int        i, isHit = FALSE;
    struct net_device   *pDev = (struct net_device *)ptr;
    unsigned char       *pMac = NULL;
    rtk_l2_ucastAddr_t  l2Addr;

    if (!pDev)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "net device is NULL");
        goto end_device_event;
    }

    if (event != NETDEV_UP && event != NETDEV_UNREGISTER && event != NETDEV_CHANGEADDR)
        goto end_device_event;

    for (i = OMCI_IF_TYPE_LAN; i < OMCI_IF_TYPE_END; i++)
    {
        if (0 == strncmp(pDev->name, omci_if_name_list[i], strlen(pDev->name)))
        {
            isHit = TRUE;
            break;
        }
    }

    if (!isHit)
        goto end_device_event;

    if (i == OMCI_IF_TYPE_LAN)
    {
        get_dev_mac_addr(DEV_LAN_MAC_ADDR_FILE, gPlatformDb.devLanMac);
        pMac = gPlatformDb.devLanMac;
    }
    else if (i == OMCI_IF_TYPE_WAN)
    {
        get_dev_mac_addr(DEV_WAN_MAC_ADDR_FILE, gPlatformDb.devWanMac);
        pMac = gPlatformDb.devWanMac;
    }
    else
    {
        goto end_device_event;
    }

    memset(&l2Addr, 0, sizeof(rtk_l2_ucastAddr_t));
    l2Addr.flags |= RTK_L2_UCAST_FLAG_STATIC;

    if (event == NETDEV_UNREGISTER)
    {
        memcpy(l2Addr.mac.octet, pMac, ETHER_ADDR_LEN);
        if (RT_ERR_OK != rtk_l2_addr_del(&l2Addr))
            printk("del l2 entry failed for device");
        goto end_device_event;
    }
    /* set device mac address in L2 table */
    memcpy(l2Addr.mac.octet, pMac, ETH_ALEN);
    l2Addr.port = gPlatformDb.cpuPort;

    if (RT_ERR_OK != rtk_l2_addr_add(&l2Addr))
        printk("add l2 entry failed for device");

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "pMac addr=%02X:%02X:%02X:%02X:%02X:%02X\n\n\n\n\n",
                pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
end_device_event:
    return NOTIFY_DONE;
}

static int omci_ipv4_event(struct notifier_block *nb, unsigned long event, void *ptr)
{
    unsigned int                i, j, isHit = FALSE;
    rtk_portmask_t              portmask;
    rtk_switch_system_mode_t    mode;
    struct net_device           *pDev = NULL;
    struct in_ifaddr            *if_info = NULL;

    if (event != NETDEV_UP && event != NETDEV_CHANGEADDR)
        goto end_ipv4_event;

    if (!ptr)
        goto end_ipv4_event;

    if_info = (struct in_ifaddr *)ptr;

    if (!(if_info->ifa_dev))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "in_dev is NULL");
        goto end_ipv4_event;
    }
    pDev = if_info->ifa_dev->dev;
    if (!pDev)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "net_dev is NULL");
        goto end_ipv4_event;
    }

    for (i = OMCI_IF_TYPE_LAN; i < OMCI_IF_TYPE_END; i++)
    {
        if (0 == strncmp(pDev->name, omci_if_name_list[i], strlen(omci_if_name_list[i])))
        {
            isHit = TRUE;
            break;
        }
    }

    if (!isHit)
        goto end_ipv4_event;

    RTK_PORTMASK_RESET(portmask);

    /* unkown uc does not flood in cpu port */
    for (j = gPlatformDb.etherPortMin; j <= gPlatformDb.etherPortMax; j++)
    {
        if (j == gPlatformDb.cpuPort || j == gPlatformDb.rgmiiPort)
            continue;

        portmask.bits[0] |= (1 << j);
    }

    if (i == OMCI_IF_TYPE_LAN)
    {
        memset(&mode, 0x00, sizeof(rtk_switch_system_mode_t));
        mode.initLan = ENABLED;
        memcpy(mode.macLan.octet, gPlatformDb.devLanMac, ETHER_ADDR_LEN);
        memcpy(&(mode.ipLan), &(if_info->ifa_local), sizeof(rtk_ip_addr_t));
        if (RT_ERR_DRIVER_NOT_FOUND == rtk_switch_system_init(&mode))
        {
            omci_copyArpToCpu(&portmask);
        }
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set acl rule for lan interface");
    }

    if (i == OMCI_IF_TYPE_WAN)
    {
        memset(&mode, 0x00, sizeof(rtk_switch_system_mode_t));
        mode.initWan = ENABLED;
        memcpy(mode.macWan.octet, gPlatformDb.devWanMac, ETHER_ADDR_LEN);
        memcpy(&(mode.ipWan), &(if_info->ifa_local), sizeof(rtk_ip_addr_t));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_switch_system_init(&mode));
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set acl rule for wan interface");
     }

end_ipv4_event:
    return NOTIFY_DONE;
}


static struct notifier_block omci_netdev_notifier_block __read_mostly =
{
    .notifier_call = omci_device_event,
};
static struct notifier_block omci_ipv4_notifier_block __read_mostly =
{
    .notifier_call = omci_ipv4_event,
};

int __init rtk_platform_init(void)
{
    unsigned char isRegistered = FALSE;

    register_netdevice_notifier(&omci_netdev_notifier_block);
    register_inetaddr_notifier(&omci_ipv4_notifier_block);

    platform_db_init();

    omcidrv_platform_register(&gPlatformDb);
    printk("omci platform attached!\n");

    if(OMCI_ERR_OK != pf_rtl96xx_InitPlatform())
        return OMCI_ERR_OK;
    else
        isRegistered = TRUE;

    if(OMCI_ERR_OK != pkt_redirect_kernelApp_sendPkt(PR_KERNEL_UID_BCASTER,
        0, sizeof(unsigned char), &isRegistered))
    {
        printk("%s() %d: send registered command failed !! \n", __FUNCTION__, __LINE__);
    }

    rtk_gponapp_callbackExtMsgGetHandle_reg(omci_ioctl_callback);

    if(OMCI_ERR_OK != intr_bcaster_notifier_cb_register(&gponStateChangeNotifier))
    {
        printk(" OMCI module register intr_bcaster module Error !! \n");
    }

    rtk_gponapp_evtHdlAlarm_reg(RTK_GPON_ALARM_SF, gpon_alarm_callback);
    rtk_gponapp_evtHdlAlarm_reg(RTK_GPON_ALARM_SD, gpon_alarm_callback);

    return OMCI_ERR_OK;
}

void __exit rtk_platform_exit(void)
{
    unregister_netdevice_notifier(&omci_netdev_notifier_block);
    unregister_inetaddr_notifier(&omci_ipv4_notifier_block);
    if(OMCI_ERR_OK != intr_bcaster_notifier_cb_unregister(&gponStateChangeNotifier))
    {
        printk("OMCI unregister bcaster notifier chain Error !! \n");
    }

    pf_rtl96xx_ExitPlatform();
}

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("RealTek OMCI kernel module");
MODULE_AUTHOR("RealTek");

module_init(rtk_platform_init);
module_exit(rtk_platform_exit);

