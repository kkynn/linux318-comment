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
#include <linux/slab.h>
#include <pkt_redirect/pkt_redirect.h>
#include <DRV/platform/include/rtl9607_rg/omci_pf_rtl9607.h>

/*rtk api*/
#include <dal/apollomp/dal_apollomp_ponmac.h>
#include <rtk/pon_led.h>
#include <rtk/svlan.h>
#include <rtk/vlan.h>
#include <rtk/switch.h>
#include <rtk/rate.h>
#include <rtk/time.h>
#include "rtk_rg_liteRomeDriver.h"

#include "omci_dm_cb.h"
#include "hal/phy/phydef.h"
#include <module/intr_bcaster/intr_bcaster.h>
#include <module/gpon_pppoe_adv/gpon_pppoe_adv.h>


#define BIT_NUM(val) ({         \
    unsigned int  i = 0;        \
    while(val > (1 << i)) i++;  \
    i;                          \
})

// TBD: let TR142 can get rule entry as input arguments of RG API
typedef struct
{
    unsigned int                    valid;
    omci_cf_rule_rg_opt_t           ruleOpt;
    union
    {
        rtk_rg_aclFilterAndQos_t    aclEntry;
        rtk_rg_classifyEntry_t      cfEntry;
    } rule;                                                         // for 9602C test chip issue: dual stack or others
} omci_dm_patch_pf_rule_t;

typedef struct {
    unsigned int                wanIdx;
    rtk_rg_aclFilterAndQos_t    rgAclEntry;                 // rg us acl entry
    rtk_rg_aclFilterAndQos_t    rgUsSwAclEntry;             // for bridge wan only
    omci_dm_patch_pf_rule_t     rgPatchEntry[PATCH_END];    // for patch rule
    struct list_head            list;
} omci_dmm_pon_wan_pf_info_t;


static pf_db_t gPlatformDb;
static omci_dmm_cb_t *pDmmCb = NULL;
static omci_pon_wan_rule_t     gPonWanEntry[OMCI_PON_WAN_RULE_POS_END];
static omci_dmm_pon_wan_pf_info_t  gPon_wan_pf_info;
struct list_head    ponWanPfInfoHead;

int omci_dmm_rule_available_check(int id)
{
    if (id < 0 || id > gPlatformDb.cfNum)
        return FALSE;

    if (1 == gPlatformDb.cfRule[id].isCfg)
        return FALSE;
    else
        return TRUE;

}
EXPORT_SYMBOL(omci_dmm_rule_available_check);


omci_dmm_pon_wan_pf_info_t* omci_dmm_pon_wan_pf_info_entry_find(unsigned int wanIdx)
{
    struct list_head            *pEntry     = NULL;
    struct list_head            *pTmpEntry  = NULL;
    omci_dmm_pon_wan_pf_info_t  *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &ponWanPfInfoHead)
    {
        pEntryData = list_entry(pEntry, omci_dmm_pon_wan_pf_info_t, list);

        if (pEntryData->wanIdx == wanIdx)
            return pEntryData;
    }

    return NULL;
}

int omci_dmm_pon_wan_pf_info_entry_add(unsigned int wanIdx, omci_dmm_pon_wan_pf_info_t *pArg)
{
    omci_dmm_pon_wan_pf_info_t      *pEntryData = NULL;

    if (!(pEntryData = omci_dmm_pon_wan_pf_info_entry_find(wanIdx)))
    {
        // new item
        pEntryData = kzalloc(sizeof(omci_dmm_pon_wan_pf_info_t), GFP_KERNEL);
        if (!pEntryData)
            return OMCI_ERR_FAILED;

        pEntryData->wanIdx = wanIdx;

        memcpy(pEntryData, pArg, sizeof(omci_dmm_pon_wan_pf_info_t));

        list_add_tail(&pEntryData->list, &ponWanPfInfoHead);

    }
    else
    {
        memcpy(pEntryData, pArg, sizeof(omci_dmm_pon_wan_pf_info_t));
    }
    return OMCI_ERR_OK;
}

int omci_dmm_pon_wan_pf_info_entry_del(unsigned int wanIdx)
{
    struct list_head            *pEntry     = NULL;
    struct list_head            *pTmpEntry  = NULL;
    omci_dmm_pon_wan_pf_info_t  *pEntryData = NULL;

    list_for_each_safe(pEntry, pTmpEntry, &ponWanPfInfoHead)
    {
        pEntryData = list_entry(pEntry, omci_dmm_pon_wan_pf_info_t, list);

        if (pEntryData->wanIdx == wanIdx)
        {
            list_del(&pEntryData->list);

            kfree(pEntryData);

            return OMCI_ERR_OK;
        }
    }

    return OMCI_ERR_OK;
}



int omci_dmm_pon_wan_rule_xlate(omci_dm_pon_wan_info_t *pRule, omci_pon_wan_rule_position_t pos, void *p)
{
    omci_dmm_pon_wan_pf_info_t *ptr = NULL;

    ptr = omci_dmm_pon_wan_pf_info_entry_find(pRule->wanIdx);

    if (!ptr)
    {
        printk("%s()%d not found entry\n", __func__, __LINE__);
        return OMCI_ERR_FAILED;
    }

    switch (pos)
    {
        case OMCI_PON_WAN_RULE_POS_RG_ACL:
            memcpy(p, &(ptr->rgAclEntry), sizeof(rtk_rg_aclFilterAndQos_t));
            break;
        case OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL:
            memcpy(p, &(ptr->rgUsSwAclEntry), sizeof(rtk_rg_aclFilterAndQos_t));
            break;
        case OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A:
            if (pRule->rgPatchEntry[PATCH_DS_9602C_A].valid)
            {
                if (OMCI_CF_RULE_RG_OPT_ACL == pRule->rgPatchEntry[PATCH_DS_9602C_A].ruleOpt)
                {
                    memcpy(p, &(ptr->rgPatchEntry[PATCH_DS_9602C_A].rule.aclEntry), sizeof(rtk_rg_aclFilterAndQos_t));
                }
                else if (OMCI_CF_RULE_RG_OPT_CF == pRule->rgPatchEntry[PATCH_DS_9602C_A].ruleOpt)
                {
                    memcpy(p, &(ptr->rgPatchEntry[PATCH_DS_9602C_A].rule.cfEntry), sizeof(rtk_rg_classifyEntry_t));
                }
                else
                {
                    return OMCI_ERR_FAILED;
                }
            }
            break;
        case OMCI_PON_WAN_RULE_POS_PATCH_DPI:
            if (pRule->rgPatchEntry[PATCH_DPI].valid)
            {
                if (OMCI_CF_RULE_RG_OPT_ACL == pRule->rgPatchEntry[PATCH_DPI].ruleOpt)
                {
                    memcpy(p, &(ptr->rgPatchEntry[PATCH_DPI].rule.aclEntry), sizeof(rtk_rg_aclFilterAndQos_t));
                }
                else if (OMCI_CF_RULE_RG_OPT_CF == pRule->rgPatchEntry[PATCH_DPI].ruleOpt)
                {
                    memcpy(p, &(ptr->rgPatchEntry[PATCH_DPI].rule.cfEntry), sizeof(rtk_rg_classifyEntry_t));
                }
                else
                {
                    return OMCI_ERR_FAILED;
                }
            }
            break;
        default:
            return OMCI_ERR_FAILED;
    }
    return OMCI_ERR_OK;
}
EXPORT_SYMBOL(omci_dmm_pon_wan_rule_xlate);

void omci_dmm_cb_register(omci_dmm_cb_t *p)
{
    pDmmCb = p;
}
EXPORT_SYMBOL(omci_dmm_cb_register);

void omci_dmm_cb_unregister(void)
{
    pDmmCb = NULL;
}
EXPORT_SYMBOL(omci_dmm_cb_unregister);


/* local function */
// TBD, RG not support SVLAN filtering
#if 0
static int omci_cfg_vlan_table(rtk_vlan_t vid)
{
    rtk_portmask_t mbrmsk;
    rtk_portmask_t allPortMask;
    rtk_switch_allPortMask_set(&allPortMask);
    rtk_vlan_create(vid);
    mbrmsk.bits[0] = (1 << gPlatformDb.ponPort);
    rtk_vlan_port_set(vid, &mbrmsk, &allPortMask);

    return RT_ERR_OK;
}

static void omci_delMemberPortBySvlan(unsigned int usCfIdx, unsigned int systemTpid)
{
    rtk_classify_cfg_t cfg;
    rtk_vlan_t vid = UINT_MAX, idx;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    rtk_switch_allPortMask_set(&allPortMask);
    unsigned char customizeB = IS_CUSTOMIZE(gDrvCtrl.customize, customize_features_list[0].customize, sizeof(gDrvCtrl.customize));

    memset(&cfg, 0, sizeof(rtk_classify_cfg_t));
    cfg.index = usCfIdx;
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
    else if(0x8100 == systemTpid && TRUE == customizeB &&
            (cfg.field.readField.dataFieldRaw[0] >> 3 & 0x1) &&
            (cfg.field.readField.careFieldRaw[0] >> 3 & 0x1) &&
            (CLASSIFY_US_CSACT_TRANSPARENT == cfg.act.usAct.csAct) &&
            (CLASSIFY_US_CSACT_TRANSPARENT == cfg.act.usAct.cAct) &&
            (!(((cfg.field.readField.dataFieldRaw[1] & 0x7f) << 5) |
            (cfg.field.readField.dataFieldRaw[0] >> 11 & 0x1f))) &&
            (!(((cfg.field.readField.careFieldRaw[1] & 0x7f) << 5) |
            (cfg.field.readField.careFieldRaw[0] >> 11 & 0x1f))) &&
            (!(cfg.field.readField.dataFieldRaw[0] >> 8 & 0x7)) &&
            (!(cfg.field.readField.careFieldRaw[0] >> 8 & 0x7)))
    {
        for(idx = 0; idx <= 4095; idx++)
        {
            if(RT_ERR_OK == rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
                if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
                {
                    rtk_vlan_destroy((rtk_vlan_t)idx);
                }
                else
                {
                    rtk_vlan_port_set(idx, &mbrmsk, &allPortMask);
                }
            }
        }
    }
    else
    {
        vid = 0;
    }
    if(UINT_MAX != vid)
    {
        if(RT_ERR_OK == rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
        {
            /* vid already exit in vlan table */
            mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
            rtk_vlan_port_set(vid, &mbrmsk, &allPortMask);
        }
    }
    return;
}

static void omci_SetMemberPortBySvlan(rtk_classify_cfg_t *pClassifyCfg, OMCI_BRIDGE_RULE_ts *pBridgeRule, unsigned int systemTpid)
{
    rtk_enable_t enableB;
    rtk_svlan_lookupType_t sType;
    rtk_vlan_t vid = UINT_MAX, idx;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    rtk_switch_allPortMask_set(&allPortMask);
    unsigned char customizeB = IS_CUSTOMIZE(gDrvCtrl.customize, customize_features_list[0].customize, sizeof(gDrvCtrl.customize));

    rtk_svlan_servicePort_get(gPlatformDb.ponPort, &enableB);
    if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        rtk_svlan_lookupType_get(&sType);
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
        else if(0x8100 == systemTpid && TRUE == customizeB &&
            PON_GEMPORT_DIRECTION_BI == pBridgeRule->dir &&
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.sTagAct.vlanAct &&
            VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.cTagAct.vlanAct &&
            (VLAN_FILTER_NO_CARE & pBridgeRule->vlanRule.filterRule.filterCtagMode) &&
            ((VLAN_FILTER_NO_CARE & pBridgeRule->vlanRule.filterRule.filterStagMode) ||
            (VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterStagMode)))
        {
            for(idx = 0; idx <= 4095; idx++)
            {
                if(RT_ERR_OK != rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
                {
                    rtk_vlan_create(idx);
                    mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | pBridgeRule->uniMask;
                }
                else
                {
                    /* vid already exit in vlan table */
                    mbrmsk.bits[0] |= pBridgeRule->uniMask;
                }
                rtk_vlan_port_set(idx, &mbrmsk, &allPortMask);
            }
        }
        else
        {
            vid = 0;
        }
        if(UINT_MAX != vid)
        {
            if(RT_ERR_OK != rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
            {
                rtk_vlan_create(vid);
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | pBridgeRule->uniMask;
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= pBridgeRule->uniMask;
            }
            rtk_vlan_port_set(vid, &mbrmsk, &allPortMask);
        }
    }
}

static void omci_delServicePort(unsigned int usCfIdx)
{
    rtk_enable_t enableB = DISABLED;
    int res = FALSE, idx;
    rtk_classify_cfg_t cfg;
    rtk_port_t port = 0;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    rtk_switch_allPortMask_set(&allPortMask);

    uint16 sbitData, sbitMask, cbitData, cbitMask, vidData, vidMask, priData, priMask;

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

    if(0 == sbitData && 1 == sbitMask && 0 == cbitData && 1 == cbitMask)
    {
        res = TRUE;
        /* for 6.1.1 type only forward untag packet from UTP and packet with stag cannot be forward */
        rtk_vlan_port_get(0, &mbrmsk, &utgmask);
        mbrmsk.bits[0] &= (~((1 << (cfg.field.readField.dataFieldRaw[0] & 0x7)) | (1 << gPlatformDb.cpuPort)));
        rtk_vlan_port_set(0, &mbrmsk, &utgmask);
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

            if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
            {
                rtk_vlan_destroy((rtk_vlan_t)vidData);
            }
            else
            {
                rtk_vlan_port_set(vidData, &mbrmsk, &allPortMask);
            }
        }
        /* if ingress without stag */
        rtk_vlan_port_get(0, &mbrmsk, &utgmask);
        mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));
        rtk_vlan_port_set(0, &mbrmsk, &utgmask);
    }
    else if(1 == sbitData && 1 == sbitMask && 0x7 == priMask && 0x0 == vidMask)
    {
        res = TRUE;
        /* for 6.1.24 type only filter s-pbit and send packet with stag from UTP port */
        for(idx = 0; idx <= 4095; idx++)
        {
            if(RT_ERR_OK == rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] &= ~(1 << (cfg.field.readField.dataFieldRaw[0] & 0x7));

                if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
                {
                    rtk_vlan_destroy((rtk_vlan_t)idx);
                }
                else
                {
                    rtk_vlan_port_set(idx, &mbrmsk, &allPortMask);
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

            if(0 == (mbrmsk.bits[0] & (~(1 << gPlatformDb.ponPort))))
            {
                rtk_vlan_destroy((rtk_vlan_t)vidData);
            }
            else
            {
                rtk_vlan_port_set(vidData, &mbrmsk, &allPortMask);
            }
        }
    }
    if(TRUE == res)
    {
        for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
        {
            rtk_svlan_servicePort_get(port, &enableB);
            if(port == (cfg.field.readField.dataFieldRaw[0] & 0x7))
                enableB = DISABLED;
            rtk_svlan_servicePort_set(port, enableB);
        }
    }
}

static void omci_SetServicePort(rtk_vlan_t vid, OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    rtk_enable_t enableB = DISABLED;
    int res = FALSE, idx;
    rtk_port_t port = 0;
    rtk_portmask_t mbrmsk, utgmask;
    rtk_portmask_t allPortMask;
    rtk_switch_allPortMask_set(&allPortMask);

    if(VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode &&
        VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterStagMode &&
        VLAN_FILTER_NO_TAG & pBridgeRule->vlanRule.filterRule.filterCtagMode)
    {
        res = TRUE;
        /* for 6.1.1 type only forward untag packet from UTP and packet with stag cannot be forward */
        rtk_vlan_port_get(0, &mbrmsk, &utgmask);
        mbrmsk.bits[0] = mbrmsk.bits[0] | pBridgeRule->uniMask  | (1 << gPlatformDb.cpuPort);
        rtk_vlan_port_set(0, &mbrmsk, &utgmask);
    }
    else if(VLAN_OPER_MODE_FILTER_SINGLETAG == pBridgeRule->vlanRule.filterMode &&
        (VLAN_ACT_NON == pBridgeRule->vlanRule.sTagAct.vlanAct ||
        VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.sTagAct.vlanAct) &&
        (VLAN_ACT_NON == pBridgeRule->vlanRule.cTagAct.vlanAct ||
        VLAN_ACT_TRANSPARENT == pBridgeRule->vlanRule.cTagAct.vlanAct))
    {
        /*TODO: Current: if filterMode is filter single tag, filterStagMode & filterCtagMode is 0 */
        res = TRUE;
        /* for 6.1.5 type no filter stag and ctag and actions of stag and ctag are NO ACT */
        /* create outer vid to vlan table and set member port */
        if(RT_ERR_OK != rtk_vlan_port_get(vid, &mbrmsk, &utgmask))
        {
            rtk_vlan_create(vid);
            mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | pBridgeRule->uniMask;
        }
        else
        {
            /* vid already exit in vlan table */
            mbrmsk.bits[0] |= pBridgeRule->uniMask;
        }
        rtk_vlan_port_set(vid, &mbrmsk, &allPortMask);
        /* if ingress without stag */
        rtk_vlan_port_get(0, &mbrmsk, &utgmask);
        mbrmsk.bits[0] |= pBridgeRule->uniMask;
        rtk_vlan_port_set(0, &mbrmsk, &utgmask);

    }
    else if(VLAN_OPER_MODE_EXTVLAN == pBridgeRule->vlanRule.filterMode &&
        VLAN_FILTER_PRI == pBridgeRule->vlanRule.filterRule.filterStagMode)
    {
        res = TRUE;
        /* for 6.1.24 type only filter s-pbit and send packet with stag from UTP port */
        for(idx = 0; idx < 4095; idx++)
        {
            if(RT_ERR_OK != rtk_vlan_port_get(idx, &mbrmsk, &utgmask))
            {
                rtk_vlan_create(idx);
                mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | pBridgeRule->uniMask;
            }
            else
            {
                /* vid already exit in vlan table */
                mbrmsk.bits[0] |= pBridgeRule->uniMask;
            }
            rtk_vlan_port_set(idx, &mbrmsk, &allPortMask);
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
            rtk_vlan_create(pBridgeRule->vlanRule.filterRule.filterSTag.vid);
            mbrmsk.bits[0] = (1 << gPlatformDb.ponPort) | pBridgeRule->uniMask;
        }
        else
        {
            /* vid already exit in vlan table */
            mbrmsk.bits[0] |= pBridgeRule->uniMask;
        }
        rtk_vlan_port_set(pBridgeRule->vlanRule.filterRule.filterSTag.vid, &mbrmsk, &allPortMask);
    }
    if(TRUE == res)
    {
        for(port = gPlatformDb.allPortMin; port <= gPlatformDb.allPortMax; port++)
        {
            rtk_svlan_servicePort_get(port, &enableB);
            if((1 << port) & pBridgeRule->uniMask)
                enableB = ENABLED;
            rtk_svlan_servicePort_set(port, enableB);
        }
    }
    return;
}
#endif

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

static int rg_cf_show_dsAction(rtk_rg_classifyEntry_t *pAct)
{
    //rtk_portmask_t portlist;
    uint8   buf[30];

    if (pAct->ds_action_field & CF_DS_ACTION_STAG_BIT)
    {
        printk("Stag action: %s\n", diagStr_dsCStagAction[pAct->action_svlan.svlanTagIfDecision]);
        if(ACL_SVLAN_TAGIF_TAGGING_WITH_VSTPID == pAct->action_svlan.svlanTagIfDecision ||
           ACL_SVLAN_TAGIF_TAGGING_WITH_8100 == pAct->action_svlan.svlanTagIfDecision)
        {

            printk("Stag VID action: %s\n", diagStr_dsSvidAction[pAct->action_svlan.svlanSvidDecision]);
            if(ACL_SVLAN_SVID_ASSIGN == pAct->action_svlan.svlanSvidDecision)
                printk("Stag VID: %d\n", pAct->action_svlan.assignedSvid);

            printk("Stag PRI action: %s\n", diagStr_dsSpriAction[pAct->action_svlan.svlanSpriDecision]);
            if(ACL_SVLAN_SPRI_ASSIGN == pAct->action_svlan.svlanSpriDecision)
                printk("Stag PRI: %d\n", pAct->action_svlan.assignedSpri);

        }
    }
    else
        printk("Stag action: %s\n", DIAG_STR_NOP);

    if (pAct->ds_action_field & CF_DS_ACTION_CTAG_BIT)
    {
        printk("Ctag action: %s\n", diagStr_dsCtagAction[pAct->action_cvlan.cvlanTagIfDecision]);
        if(ACL_CVLAN_TAGIF_TAGGING == pAct->action_cvlan.cvlanTagIfDecision)
        {
            printk("Ctag VID action: %s\n", diagStr_dsCvidAction[pAct->action_cvlan.cvlanCvidDecision]);
            if(ACL_CVLAN_CVID_ASSIGN == pAct->action_cvlan.cvlanCvidDecision)
                printk("Ctag VID: %d\n", pAct->action_cvlan.assignedCvid);

            printk("Ctag PRI action: %s\n", diagStr_dsCpriAction[pAct->action_cvlan.cvlanCpriDecision]);
            if(ACL_CVLAN_CPRI_ASSIGN == pAct->action_cvlan.cvlanCpriDecision)
                printk("Ctag PRI: %d\n", pAct->action_cvlan.assignedCpri);
        }
    }
    else
        printk("Ctag action: %s\n", DIAG_STR_NOP);

    printk("Classf PRI action: %s\n", diagStr_cfpriAction[pAct->ds_action_field & CF_DS_ACTION_CFPRI_BIT ? 1 : 0]);
    printk("CF PRI: %d\n", pAct->action_cfpri.assignedCfPri);

    if (pAct->ds_action_field & CF_DS_ACTION_UNI_MASK_BIT)
    {
        printk("UNI action: %s\n", diagStr_dsUniAction[pAct->action_uni.uniActionDecision]);

        portMask2Str(pAct->action_uni.assignedUniPortMask, buf);
        printk("UNI ports: %s\n", buf);
    }
    else
        printk("UNI action: %s\n", DIAG_STR_NOP);

    printk("DSCP remarking action: %s\n",
        pAct->ds_action_field & CF_DS_ACTION_DSCP_BIT ? "ENABLE":"DISABLE");

    return RT_ERR_OK;
}

static int rg_cf_show_usAction(rtk_rg_classifyEntry_t *pAct)
{
    //rtk_portmask_t portlist;
    //uint8   buf[30];

    if (pAct->us_action_field & CF_US_ACTION_STAG_BIT)
    {
        printk("Stag action: %s\n", diagStr_usCStagAction[pAct->action_svlan.svlanTagIfDecision]);
        if(ACL_SVLAN_TAGIF_TAGGING_WITH_VSTPID == pAct->action_svlan.svlanTagIfDecision ||
           ACL_SVLAN_TAGIF_TAGGING_WITH_8100 == pAct->action_svlan.svlanTagIfDecision)
        {
            printk("Stag VID action: %s\n", diagStr_usSvidAction[pAct->action_svlan.svlanSvidDecision]);
           /* if(ACL_SVLAN_SVID_ASSIGN == pAct->action_svlan.svlanSvidDecision)*/
                printk("Stag VID: %d\n", pAct->action_svlan.assignedSvid);

            printk("Stag PRI action: %s\n", diagStr_usSpriAction[pAct->action_svlan.svlanSpriDecision]);
            /*if(ACL_SVLAN_SPRI_ASSIGN == pAct->action_svlan.svlanSpriDecision)*/
                printk("Stag PRI: %d\n", pAct->action_svlan.assignedSpri);
        }
    }
    else
        printk("Stag action: %s\n", DIAG_STR_NOP);

    if (pAct->us_action_field & CF_US_ACTION_CTAG_BIT)
    {
        printk("Ctag action: %s\n", diagStr_usCtagAction[pAct->action_cvlan.cvlanTagIfDecision]);

        if(ACL_CVLAN_TAGIF_TAGGING == pAct->action_cvlan.cvlanTagIfDecision)
        {
            printk("Ctag VID action: %s\n", diagStr_usCvidAction[pAct->action_cvlan.cvlanCvidDecision]);
           /* if(ACL_CVLAN_CVID_ASSIGN == pAct->action_cvlan.cvlanCvidDecision)*/
                printk("Ctag VID: %d\n", pAct->action_cvlan.assignedCvid);

            printk("Ctag PRI action: %s\n", diagStr_usCpriAction[pAct->action_cvlan.cvlanCpriDecision]);
            /*if(ACL_CVLAN_CPRI_ASSIGN == pAct->action_cvlan.cvlanCpriDecision)*/
                printk("Ctag PRI: %d\n", pAct->action_cvlan.assignedCpri);
        }
    }
    else
        printk("Ctag action: %s\n", DIAG_STR_NOP);

    printk("SID action: %s\n", diagStr_usSidAction[pAct->us_action_field & CF_US_ACTION_SID_BIT ? 1 : 0]);
    printk("Assign ID: %d\n", pAct->action_sid_or_llid.assignedSid_or_llid);

    printk("Classf PRI action: %s\n", diagStr_cfpriAction[pAct->us_action_field & CF_US_ACTION_CFPRI_BIT ? 1 : 0]);
    printk("CF PRI: %d\n", pAct->action_cfpri.assignedCfPri);

    printk("DSCP remarking action: %s\n",
        pAct->us_action_field & CF_US_ACTION_DSCP_BIT ? "ENABLE":"DISABLE");
    printk("Drop action: %s\n",
        pAct->us_action_field & CF_US_ACTION_DROP_BIT ? "ENABLE":"DISABLE");
    printk("logging action: %s\n",
        pAct->us_action_field & CF_US_ACTION_LOG_BIT ? "ENABLE":"DISABLE");
    if(pAct->us_action_field & CF_US_ACTION_LOG_BIT)
        printk("logging index: %d\n", pAct->action_log.assignedCounterIdx);

    return OMCI_ERR_OK;
}

/*
 * classf show
 */
static int rg_cf_show(rtk_rg_classifyEntry_t *pCfg)
{
    printk("==========================================\n");
    printk("Index: %d\n",pCfg->index);
    printk("==========================================\n");
    printk("%15s: %s\n","direction",pCfg->direction ? "Downstream":"Upstream");

    if (pCfg->filter_fields & EGRESS_TAGVID_BIT)
        printk("%15s: %d\n","TagVID", pCfg->outterTagVid);

    if (pCfg->filter_fields & EGRESS_TAGPRI_BIT)
        printk("%15s: %d\n","TagPri", pCfg->outterTagPri);

    if (pCfg->filter_fields & EGRESS_INTERNALPRI_BIT)
        printk("%15s: %d\n","IntrPri", pCfg->internalPri);

    if (pCfg->filter_fields & EGRESS_STAGIF_BIT)
        printk("%15s: %d\n","S Bit", pCfg->stagIf);

    if (pCfg->filter_fields & EGRESS_CTAGIF_BIT)
        printk("%15s: %d\n","C Bit", pCfg->ctagIf);

    if (pCfg->filter_fields & EGRESS_UNI_BIT)
        printk("%15s: %d\n","UNI", pCfg->uni & pCfg->uni_mask);

    if (pCfg->filter_fields & EGRESS_GEMIDX_BIT)
        printk("%15s: %d\n","TOS/TC/GEMIDX", pCfg->gemidx & pCfg->gemidx_mask);

    if (pCfg->filter_fields & EGRESS_ETHERTYPR_BIT)
        printk("%15s: 0x%x\n","EtherType", pCfg->etherType & pCfg->etherType_mask);

    if(RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM == pCfg->direction)
    {
        printk("Downstream action: \n");
        rg_cf_show_dsAction(pCfg);
    }
    else
    {
        printk("Upstream action: \n");
        rg_cf_show_usAction(pCfg);
    }

    return OMCI_ERR_OK;
}

static int rg_aclqos_show(unsigned int index, rtk_rg_aclFilterAndQos_t *pCfg)
{
    printk("==========================================\n");
    printk("Index: %d\n",index);
    printk("==========================================\n");
    printk("%15s: %s\n","direction",
        ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN == pCfg->fwding_type_and_direction ?
        "Downstream":"Upstream");

    if (pCfg->filter_fields & EGRESS_INTF_BIT)
        printk("%15s: %d\n","WAN IF", pCfg->egress_intf_idx);

    if (pCfg->qos_actions & ACL_ACTION_ACL_SVLANTAG_BIT)
    {
        printk("Stag action: %s\n", diagStr_usCStagAction[pCfg->action_acl_svlan.svlanTagIfDecision]);
        if(ACL_SVLAN_TAGIF_TAGGING_WITH_VSTPID == pCfg->action_acl_svlan.svlanTagIfDecision ||
           ACL_SVLAN_TAGIF_TAGGING_WITH_8100 == pCfg->action_acl_svlan.svlanTagIfDecision)
        {
            printk("Stag VID action: %s\n", diagStr_usSvidAction[pCfg->action_acl_svlan.svlanSvidDecision]);
           /* if(ACL_SVLAN_SVID_ASSIGN == pCfg->action_acl_svlan.svlanSvidDecision)*/
                printk("Stag VID: %d\n", pCfg->action_acl_svlan.assignedSvid);

            printk("Stag PRI action: %s\n", diagStr_usSpriAction[pCfg->action_acl_svlan.svlanSpriDecision]);
            /*if(ACL_SVLAN_SPRI_ASSIGN == pCfg->action_acl_svlan.svlanSpriDecision)*/
                printk("Stag PRI: %d\n", pCfg->action_acl_svlan.assignedSpri);
        }
    }
    else
        printk("Stag action: %s\n", DIAG_STR_NOP);

    if (pCfg->qos_actions & ACL_ACTION_ACL_CVLANTAG_BIT)
    {
        printk("Ctag action: %s\n", diagStr_usCtagAction[pCfg->action_acl_cvlan.cvlanTagIfDecision]);

        if(ACL_CVLAN_TAGIF_TAGGING == pCfg->action_acl_cvlan.cvlanTagIfDecision)
        {
            printk("Ctag VID action: %s\n", diagStr_usCvidAction[pCfg->action_acl_cvlan.cvlanCvidDecision]);
           /* if(ACL_CVLAN_CVID_ASSIGN == pCfg->action_acl_cvlan.cvlanCvidDecision)*/
                printk("Ctag VID: %d\n", pCfg->action_acl_cvlan.assignedCvid);

            printk("Ctag PRI action: %s\n", diagStr_usCpriAction[pCfg->action_acl_cvlan.cvlanCpriDecision]);
            /*if(ACL_CVLAN_CPRI_ASSIGN == pCfg->action_acl_cvlan.cvlanCpriDecision)*/
                printk("Ctag PRI: %d\n", pCfg->action_acl_cvlan.assignedCpri);
        }
    }
    else
        printk("Ctag action: %s\n", DIAG_STR_NOP);

    printk("SID action: %s\n", diagStr_usSidAction[pCfg->qos_actions & ACL_ACTION_STREAM_ID_OR_LLID_BIT ? 1 : 0]);
    printk("Assign ID: %d\n", pCfg->action_stream_id_or_llid);

    return OMCI_ERR_OK;
}


static int rg_cvlan_del(int vlanID)
{
    int ret;
    rtk_rg_cvlan_info_t cvlan_info;


    memset(&cvlan_info, 0, sizeof(rtk_rg_cvlan_info_t));
    cvlan_info.vlanId = vlanID;
    /*Check vlan valid before delete*/
    if(rtk_rg_cvlan_get(&cvlan_info) == RT_ERR_RG_OK)
    {
        ret = rtk_rg_cvlan_del(vlanID);
        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "delete vlan %u fail, ret = %d", vlanID, ret);
        }
    }
    return OMCI_ERR_OK;
}



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
        if(gPlatformDb.tCont[i].allocId == 0xff)
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
            rtk_rg_gpon_tcont_destroy_logical(&ind);
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

    ret = rtk_rg_gpon_tcont_get(&ind,&attr);

    /* de-allocate this alloc-id */
    if(ploam->msg[2]==0xFF && ret == RT_ERR_OK)
    {
       _RemoveUsedTcontId(attr.tcont_id);
       rtk_rg_gpon_tcont_destroy(&ind);
    }
    /* allocate this GEM alloc-id */
    else if(ploam->msg[2]==1 /*&& ret !=RT_ERR_OK*/)
    {
       ret = _AssignNonUsedTcontId(alloc, &attr.tcont_id);
       if(ret == OMCI_ERR_OK)
       {
            ret = rtk_rg_gpon_tcont_create(&ind,&attr);
       }
    }
    return;
}

static void _saveCfCfgToDb(int cfIdx, rtk_rg_classifyEntry_t *pRgCfEntry)
{
    rtk_rg_classifyEntry_t *pDbCfCfg = NULL;

    pDbCfCfg = (rtk_rg_classifyEntry_t *)kmalloc(sizeof(rtk_rg_classifyEntry_t), GFP_KERNEL);

    gPlatformDb.cfRule[cfIdx].classifyCfg = pDbCfCfg;
    memcpy(pDbCfCfg, pRgCfEntry, sizeof(rtk_rg_classifyEntry_t));
}

static void _delCfCfgFromDb(unsigned int cfIdx)
{
    rtk_rg_classifyEntry_t *pDbCfCfg = NULL;

    pDbCfCfg = (rtk_rg_classifyEntry_t *)gPlatformDb.cfRule[cfIdx].classifyCfg;

    if(pDbCfCfg == NULL)
        return;

    kfree(pDbCfCfg);
    pDbCfCfg = NULL;
}

/*
CF entry maintain
*/
static int _AssignNonUsedCfIndex(unsigned int cfType, omci_rule_pri_t rule_pri, unsigned int *pIndex)
{
#if 0
#if 1
    int i,start,stop;

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

    if (rule_pri)
    {
        /* High priority rule */
        for(i=start;i<=stop;i++)
        {
            if(gPlatformDb.cfRule[i].isCfg==0)
            {
                gPlatformDb.cfRule[i].isCfg = 1;
                *pIndex = i;
                return OMCI_ERR_OK;
            }
        }
    }
    else
    {
        /* Low priority rule */
        for (i = stop; i >= start; i--)
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
#else
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
            rtk_rg_classifyEntry_t *dbCfCfg;
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
                dbCfCfg = (rtk_rg_classifyEntry_t *)gPlatformDb.cfRule[i+1].classifyCfg;
                if(dbCfCfg!=NULL)
                {
                    dbCfCfg->index = i+1;
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rg_classifyEntry_add(dbCfCfg));
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
            rtk_rg_classifyEntry_t *dbCfCfg;
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
                dbCfCfg = (rtk_rg_classifyEntry_t *)gPlatformDb.cfRule[i-1].classifyCfg;
                if(dbCfCfg!=NULL)
                {
                    dbCfCfg->index = i-1;
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rg_classifyEntry_add(dbCfCfg));
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

#endif
}

static void _RemoveUsedCfIndex(int index)
{
    if(index != OMCI_UNUSED_CF && gPlatformDb.cfRule[index].isCfg == 1)
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
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"%s() ret:%d",__FUNCTION__,ret);
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

    if (RT_ERR_OK != rtk_rg_ponmac_queue_get(&queue,&queueCfg))
        return;

    if(rtk_gponapp_omci_tx((rtk_gpon_omci_msg_t *)&omci[0]) != RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"%s() Fail",__FUNCTION__);
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
    if((ret = rtk_rg_ponmac_queue_add(&queue,&queueCfg))!=RT_ERR_OK)
    {
        return ret;
    }
    /*assign strem id to tcont & queue*/
    if((ret = rtk_rg_ponmac_flow2Queue_set(gPlatformDb.omccFlow, &queue))!=RT_ERR_OK)
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
    if((ret = rtk_rg_ponmac_queue_del(&queue))!=RT_ERR_OK)
    {
        RT_ERR(ret,(MOD_GPON | MOD_DAL), "");
        return ret;
    }
    /*assign strem id to tcont & queue*/
    queue.queueId = 0;
    queue.schedulerId = 0;
    if((ret = rtk_rg_ponmac_flow2Queue_set(gPlatformDb.omccFlow, &queue))!=RT_ERR_OK)
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
    if((ret = rtk_rg_gpon_evtHdlOmci_reg(omci_send_to_user))!=RT_ERR_OK)
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
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s() ret:%d",__FUNCTION__,ret);
    }
    return;

}

static int omci_ploam_callback(rtk_gpon_ploam_t *ploam)
{
    int8    onuId;

    if (RT_ERR_OK != rtk_rg_gpon_onuId_get(&onuId))
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



static int omci_GetSVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_rg_acl_svlan_tagif_decision_t *pCsAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCsAct = ACL_SVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCsAct = ACL_SVLAN_TAGIF_UNTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCsAct = ACL_SVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        //fit TR 247
        *pCsAct = ACL_SVLAN_TAGIF_TAGGING_WITH_VSTPID;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetCVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_rg_acl_cvlan_tagif_decision_t *pCAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        *pCAct = ACL_CVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCAct = ACL_CVLAN_TAGIF_UNTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        *pCAct = ACL_CVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        *pCAct = ACL_CVLAN_TAGIF_TAGGING;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetSVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_rg_acl_svlan_svid_decision_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = ACL_SVLAN_SVID_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = ACL_SVLAN_SVID_COPY_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = ACL_SVLAN_SVID_COPY_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        // RG not support NOP
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetSPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_rg_acl_svlan_spri_decision_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = ACL_SVLAN_SPRI_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_INTERNAL_PRI;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_INTERNAL_PRI;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetCVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_rg_acl_cvlan_cvid_decision_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = ACL_CVLAN_CVID_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = ACL_CVLAN_CVID_COPY_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = ACL_CVLAN_CVID_COPY_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        *pVidAct = ACL_CVLAN_CVID_COPY_FROM_INTERNAL_VID;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetCPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_rg_acl_cvlan_cpri_decision_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = ACL_CVLAN_CPRI_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}



static int omci_SetClassifyUsAct(OMCI_VLAN_OPER_ts *pVlanRule,unsigned int flowId,rtk_rg_classifyEntry_t *pUsAct)
{
    if(pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        pUsAct->us_action_field |= CF_US_ACTION_DROP_BIT;
    }
    else
    {
        if(omci_GetSVlanRawAct(pVlanRule->sTagAct.vlanAct,&pUsAct->action_svlan.svlanTagIfDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetCVlanRawAct(pVlanRule->cTagAct.vlanAct,&pUsAct->action_cvlan.cvlanTagIfDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetSVidRawAct(pVlanRule->sTagAct.vidAct,&pUsAct->action_svlan.svlanSvidDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetSPriRawAct(pVlanRule->sTagAct.priAct,&pUsAct->action_svlan.svlanSpriDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetCVidRawAct(pVlanRule->cTagAct.vidAct,&pUsAct->action_cvlan.cvlanCvidDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetCPriRawAct(pVlanRule->cTagAct.priAct,&pUsAct->action_cvlan.cvlanCpriDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }

        pUsAct->action_svlan.assignedSvid = pVlanRule->sTagAct.assignVlan.vid;
        pUsAct->action_svlan.assignedSpri = pVlanRule->sTagAct.assignVlan.pri < 8 ? pVlanRule->sTagAct.assignVlan.pri:0;
        pUsAct->us_action_field |= CF_US_ACTION_STAG_BIT;
        pUsAct->action_cvlan.assignedCvid = pVlanRule->cTagAct.assignVlan.vid;
        pUsAct->action_cvlan.assignedCpri = pVlanRule->cTagAct.assignVlan.pri < 8 ? pVlanRule->cTagAct.assignVlan.pri:0;
        pUsAct->us_action_field |= CF_US_ACTION_CTAG_BIT;
        pUsAct->action_sid_or_llid.assignedSid_or_llid = flowId;
        pUsAct->us_action_field |= CF_US_ACTION_SID_BIT;
    }
    return OMCI_ERR_OK;
}



static int omci_GetDsSVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_rg_acl_svlan_tagif_decision_t *pCsAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
            *pCsAct = ACL_SVLAN_TAGIF_NOP;
        else
            *pCsAct = ACL_SVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCsAct = ACL_SVLAN_TAGIF_UNTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
            *pCsAct = ACL_SVLAN_TAGIF_NOP;
        else
            *pCsAct = ACL_SVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        //fit TR247
        *pCsAct = ACL_SVLAN_TAGIF_TAGGING_WITH_VSTPID;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsCVlanRawAct(OMCI_VLAN_ACT_MODE_e vlanAct,rtk_rg_acl_cvlan_tagif_decision_t *pCAct)
{
    switch(vlanAct){
    case VLAN_ACT_NON: /* ignore tagging member set */
        if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
            *pCAct = ACL_CVLAN_TAGIF_NOP;
        else
            *pCAct = ACL_CVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_REMOVE:
        *pCAct = ACL_CVLAN_TAGIF_UNTAG;
    break;
    case VLAN_ACT_TRANSPARENT:
        if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
            *pCAct = ACL_CVLAN_TAGIF_NOP;
        else
            *pCAct = ACL_CVLAN_TAGIF_TRANSPARENT;
    break;
    case VLAN_ACT_ADD:
    case VLAN_ACT_MODIFY:
    {
        *pCAct = ACL_CVLAN_TAGIF_TAGGING;
    }
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsSVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_rg_acl_svlan_svid_decision_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = ACL_SVLAN_SVID_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = ACL_SVLAN_SVID_COPY_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = ACL_SVLAN_SVID_COPY_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        // RG not support NOP
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsSPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_rg_acl_svlan_spri_decision_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = ACL_SVLAN_SPRI_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_INTERNAL_PRI;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = ACL_SVLAN_SPRI_COPY_FROM_INTERNAL_PRI;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsCVidRawAct(OMCI_VID_ACT_MODE_e vidAct,rtk_rg_acl_cvlan_cvid_decision_t *pVidAct)
{
    switch(vidAct){
    case VID_ACT_ASSIGN:
        *pVidAct = ACL_CVLAN_CVID_ASSIGN;
    break;
    case VID_ACT_COPY_INNER:
        *pVidAct = ACL_CVLAN_CVID_COPY_FROM_2ND_TAG;
    break;
    case VID_ACT_COPY_OUTER:
        *pVidAct = ACL_CVLAN_CVID_COPY_FROM_1ST_TAG;
    break;
    case VID_ACT_TRANSPARENT:
        // RG not support NOP
    default:
        return -1;
    break;
    }
    return 0;
}

static int omci_GetDsCPriRawAct(OMCI_PRI_ACT_MODE_e priAct,rtk_rg_acl_cvlan_cpri_decision_t *pPriAct)
{
    switch(priAct){
    case PRI_ACT_ASSIGN:
        *pPriAct = ACL_CVLAN_CPRI_ASSIGN;
    break;
    case PRI_ACT_COPY_INNER:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_2ND_TAG;
    break;
    case PRI_ACT_COPY_OUTER:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
    break;
    case PRI_ACT_TRANSPARENT:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
    break;
    case PRI_ACT_FROM_DSCP:
        *pPriAct = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
    break;
    default:
        return -1;
    break;
    }
    return 0;
}



static int omci_SetClassifyDsAct(OMCI_VLAN_OPER_ts *pVlanRule,unsigned int uniMask,rtk_rg_classifyEntry_t *pDsAct)
{
    if(pVlanRule->outStyle.isDefaultRule == OMCI_EXTVLAN_REMOVE_TAG_DISCARD)
    {
        pDsAct->action_uni.uniActionDecision = ACL_UNI_FORCE_BY_MASK;
        pDsAct->action_uni.assignedUniPortMask = 1 << 5;
        pDsAct->ds_action_field |= CF_DS_ACTION_UNI_MASK_BIT;
    }
    else
    {
        if(omci_GetDsSVlanRawAct(pVlanRule->sTagAct.vlanAct,&pDsAct->action_svlan.svlanTagIfDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsCVlanRawAct(pVlanRule->cTagAct.vlanAct,&pDsAct->action_cvlan.cvlanTagIfDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsSVidRawAct(pVlanRule->sTagAct.vidAct,&pDsAct->action_svlan.svlanSvidDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsSPriRawAct(pVlanRule->sTagAct.priAct,&pDsAct->action_svlan.svlanSpriDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsCVidRawAct(pVlanRule->cTagAct.vidAct,&pDsAct->action_cvlan.cvlanCvidDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }
        if(omci_GetDsCPriRawAct(pVlanRule->cTagAct.priAct,&pDsAct->action_cvlan.cvlanCpriDecision) ==-1)
        {
            return OMCI_ERR_FAILED;
        }

        pDsAct->action_svlan.assignedSvid = pVlanRule->sTagAct.assignVlan.vid;
        pDsAct->action_svlan.assignedSpri = pVlanRule->sTagAct.assignVlan.pri < 8 ? pVlanRule->sTagAct.assignVlan.pri : 0;
        // only toggle vlan tag bit when RDP 0x2 is disabled or the tag action is UNTAG
        if (OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_RDP_00000002) ||
                ACL_SVLAN_TAGIF_UNTAG == pDsAct->action_svlan.svlanTagIfDecision)
            pDsAct->ds_action_field |= CF_DS_ACTION_STAG_BIT;
        pDsAct->action_cvlan.assignedCvid = pVlanRule->cTagAct.assignVlan.vid;
        pDsAct->action_cvlan.assignedCpri = pVlanRule->cTagAct.assignVlan.pri < 8 ? pVlanRule->cTagAct.assignVlan.pri : 0;
        // only toggle vlan tag bit when RDP 0x2 is disabled or the tag action is UNTAG
        if (OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_RDP_00000002) ||
                ACL_CVLAN_TAGIF_UNTAG == pDsAct->action_cvlan.cvlanTagIfDecision)
            pDsAct->ds_action_field |= CF_DS_ACTION_CTAG_BIT;
        pDsAct->action_uni.uniActionDecision = ACL_UNI_FWD_TO_PORTMASK_ONLY;
        pDsAct->action_uni.assignedUniPortMask = (uniMask | gPlatformDb.cpuPortMask | gPlatformDb.extPortMask);
        pDsAct->ds_action_field |= CF_DS_ACTION_UNI_MASK_BIT;
    }
    return OMCI_ERR_OK;
}

static int omci_SetExtValnClassifyRule(OMCI_VLAN_FILTER_ts *pVlanFilter,rtk_rg_classifyEntry_t *pClassifyCfg, omci_rule_pri_t *pRulePri)
{

    /*Start to handle ExtVlan filter rule, Stag filter*/
    if(pVlanFilter->filterStagMode & VLAN_FILTER_NO_TAG)
    {
        pClassifyCfg->stagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }
    else if (pVlanFilter->filterStagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pClassifyCfg->stagIf = 0;
        pClassifyCfg->filter_fields &= (~EGRESS_STAGIF_BIT);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 0;
    }
    else
    {
        pClassifyCfg->stagIf = 1;
        pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_VID)
    {
        pClassifyCfg->outterTagVid = pVlanFilter->filterSTag.vid;
        pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_PRI)
    {
        pClassifyCfg->outterTagPri = pVlanFilter->filterSTag.pri;
        pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_TCI)
    {
        pClassifyCfg->outterTagVid = pVlanFilter->filterSTag.vid;
        pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
        pClassifyCfg->outterTagPri = pVlanFilter->filterSTag.pri;
        pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 2;
    }

    if(pVlanFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
    {
        pClassifyCfg->internalPri = pVlanFilter->filterSTag.pri;
        pClassifyCfg->filter_fields |= EGRESS_INTERNALPRI_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    /*Ctag filter*/
    if(pVlanFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
    {
        pClassifyCfg->ctagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 1;
    }
    else if (pVlanFilter->filterCtagMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pClassifyCfg->ctagIf = 0;
        pClassifyCfg->filter_fields &= (~EGRESS_CTAGIF_BIT);
        pRulePri->rule_pri |= OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level += 0;
    }
    else
    {
        pClassifyCfg->ctagIf = 1;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
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
                    // TBD
                    /*erVlan.classify_pattern.innerVlan.value = ((pVlanFilter->filterCTag.pri << 13) | pVlanFilter->filterCTag.vid);
                    InnerVlan.classify_pattern.innerVlan.mask = 0xffff;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;*/
                }
                else
                {
                    /*nerVlan.classify_pattern.innerVlan.value = pVlanFilter->filterCTag.vid;
                    InnerVlan.classify_pattern.innerVlan.mask = 0x0fff;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;*/
                }
             }
        }
        else
        {
            pClassifyCfg->outterTagVid = pVlanFilter->filterCTag.vid;
            pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
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
                    // TBD
                    /*nerVlan.classify_pattern.innerVlan.value = pVlanFilter->filterCTag.pri << 13;
                    InnerVlan.classify_pattern.innerVlan.mask = 0xf000;
                    pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                    pRulePri->rule_level += 1;

                    if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                        pClassifyCfg->templateIdx = 1;

                    rtk_classify_field_add(pClassifyCfg,&InnerVlan);*/
                }
            }
        }
        else
        {
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_TCI)
    {

        if((VLAN_FILTER_VID | VLAN_FILTER_PRI | VLAN_FILTER_TCI) & pVlanFilter->filterStagMode)
        {
            /*ApolloMP not support inner vlan filter*/
            if(APOLLOMP_CHIP_ID != gPlatformDb.chipId)
            {
                // TBD
                /*InnerVlan.classify_pattern.innerVlan.value = ((pVlanFilter->filterCTag.pri << 13) | pVlanFilter->filterCTag.vid);
                InnerVlan.classify_pattern.innerVlan.mask = 0xffff;
                pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
                pRulePri->rule_level += 1;

                if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
                    pClassifyCfg->templateIdx = 1;

                rtk_classify_field_add(pClassifyCfg,&InnerVlan);*/
            }
        }
        else
        {
            pClassifyCfg->outterTagVid = pVlanFilter->filterCTag.vid;
            pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            if (RTL9607C_CHIP_ID == gPlatformDb.chipId && CHIP_REV_ID_C > gPlatformDb.chipRev) {
            } else {
                pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
            }

            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 2;
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        if(ETHTYPE_FILTER_NO_CARE != pVlanFilter->etherType)
        {
            switch( pVlanFilter->etherType){
            case ETHTYPE_FILTER_IP:
            pClassifyCfg->etherType = 0x0800;
            break;
            case ETHTYPE_FILTER_PPPOE:
            pClassifyCfg->etherType = 0x8863;
            break;
            case ETHTYPE_FILTER_ARP:
            pClassifyCfg->etherType = 0x0806;
            break;
            case ETHTYPE_FILTER_PPPOE_S:
            pClassifyCfg->etherType = 0x8864;
            break;
            case ETHTYPE_FILTER_IPV6:
            pClassifyCfg->etherType = 0x86dd;
            break;
            default:
            break;
            }
            pClassifyCfg->etherType_mask = 0xffff;
            pClassifyCfg->filter_fields |= EGRESS_ETHERTYPR_BIT;
            pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level += 1;
        }
    }

    if(pVlanFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI)
    {
        pClassifyCfg->internalPri = pVlanFilter->filterCTag.pri;
        pClassifyCfg->filter_fields |= EGRESS_INTERNALPRI_BIT;
        pRulePri->rule_pri |= OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level += 1;
    }

    return OMCI_ERR_OK;
}


static int omci_SetClassifyUsRule(OMCI_VLAN_OPER_ts *pVlanRule,unsigned int srcUni,rtk_rg_classifyEntry_t *pClassifyCfg, omci_rule_pri_t *pRulePri)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = &pVlanRule->filterRule;

    pClassifyCfg->filter_fields = 0;

    if(srcUni != gPlatformDb.uniPortMask)
    {
        /*assigned source uni port */
        pClassifyCfg->uni = srcUni;
        pClassifyCfg->uni_mask = 0x7;
        pClassifyCfg->filter_fields |= EGRESS_UNI_BIT;
    }

    switch(pVlanRule->filterMode){
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        pClassifyCfg->ctagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
        pClassifyCfg->stagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 2;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        pClassifyCfg->ctagIf = 1;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 1;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
        {
            pClassifyCfg->ctagIf = 0;
            pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
            pClassifyCfg->stagIf = 0;
            pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
            pClassifyCfg->internalPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_INTERNALPRI_BIT;
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 3;
        }
        else
        {
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    {
        pClassifyCfg->outterTagVid = pVlanFilter->filterCTag.vid;
        pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
        pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
        pRulePri->rule_level = 1;

        // if filterCtagMode is VID, do not care pbit */
        if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
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
        omci_SetExtValnClassifyRule(pVlanFilter,pClassifyCfg, pRulePri);
    break;
    case VLAN_OPER_MODE_VLANTAG_OPER:
        if (VLAN_FILTER_VID & pVlanFilter->filterCtagMode ||
            VLAN_FILTER_TCI & pVlanFilter->filterStagMode)
        {
            pClassifyCfg->outterTagVid = pVlanFilter->filterCTag.vid;
            pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
            pRulePri->rule_level = 1;

            // if filterCtagMode is VID, do not care pbit */
            if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode ||
                VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
            {
                pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
                pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
                pRulePri->rule_level += 1;
            }
        }
        pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
    break;
    }
    return OMCI_ERR_OK;
}


static int omci_SetClassifyDsRule(OMCI_VLAN_OPER_ts *pDsRule, OMCI_VLAN_OPER_ts *pVlanRule, unsigned int flowId,rtk_rg_classifyEntry_t *pClassifyCfg, omci_rule_pri_t *pRulePri)
{
    OMCI_VLAN_FILTER_ts *pVlanFilter = &pDsRule->filterRule;

    /*assigned source gem port */
    pClassifyCfg->gemidx = flowId;
    pClassifyCfg->gemidx_mask = gPlatformDb.sidMask;
    pClassifyCfg->filter_fields |= EGRESS_GEMIDX_BIT;

    if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000002, pVlanFilter))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "[%d] TBD \n", __LINE__);
    }

    switch(pDsRule->filterMode){
    case VLAN_OPER_MODE_FORWARD_UNTAG:
    {
        pClassifyCfg->ctagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
        pClassifyCfg->stagIf = 0;
        pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 2;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_SINGLETAG:
    {
        pClassifyCfg->ctagIf = 1;
        pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
        pRulePri->rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        pRulePri->rule_level = 1;
    }
    break;
    case VLAN_OPER_MODE_FILTER_INNER_PRI:
    {
        if (pVlanFilter->filterCtagMode & (VLAN_FILTER_DSCP_PRI | VLAN_FILTER_NO_TAG))
        {
            pClassifyCfg->ctagIf = 0;
            pClassifyCfg->filter_fields |= EGRESS_CTAGIF_BIT;
            pClassifyCfg->stagIf = 0;
            pClassifyCfg->filter_fields |= EGRESS_STAGIF_BIT;
            pClassifyCfg->internalPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_INTERNALPRI_BIT;
            pRulePri->rule_pri = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 3;
        }
        else if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode)
        {
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
            pRulePri->rule_pri  = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }
    }
    break;
    case VLAN_OPER_MODE_FILTER_SINGLETAG:
    case VLAN_OPER_MODE_VLANTAG_OPER:
    {
        if(VLAN_FILTER_VID & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            pClassifyCfg->outterTagVid = pVlanFilter->filterCTag.vid;
            pClassifyCfg->filter_fields |= EGRESS_TAGVID_BIT;
            pRulePri->rule_pri  = OMCI_VLAN_RULE_HIGH_PRI;
            pRulePri->rule_level = 1;
        }

        if(VLAN_FILTER_PRI & pVlanFilter->filterCtagMode || VLAN_FILTER_TCI & pVlanFilter->filterCtagMode)
        {
            pClassifyCfg->outterTagPri = pVlanFilter->filterCTag.pri;
            pClassifyCfg->filter_fields |= EGRESS_TAGPRI_BIT;
            pRulePri->rule_pri  = OMCI_VLAN_RULE_HIGH_PRI;
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
        pRulePri->rule_pri  = OMCI_VLAN_RULE_LOW_PRI;
    break;
    case VLAN_OPER_MODE_EXTVLAN:
        omci_SetExtValnClassifyRule(pVlanFilter,pClassifyCfg, pRulePri);
    break;
    }

    return OMCI_ERR_OK;
}

static void omci_SetDsTagActByUsFilter(OMCI_VLAN_ACT_ts *pVlanAct,OMCI_VLAN_FILTER_MODE_e filterMode,OMCI_VLAN_ts filterVlan,
    unsigned int tagNum, OMCI_VLAN_ACT_ts usAct)
{
    if (filterMode & VLAN_FILTER_NO_CARE_TAG)
    {
        pVlanAct->vlanAct = VLAN_ACT_TRANSPARENT;
    }

    if (filterMode & VLAN_FILTER_CARE_TAG)
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
            break;
    }
}

static void omci_GetDownStreamFilterMode(OMCI_VLAN_OPER_MODE_t *pFilterMode, OMCI_VLAN_FILTER_MODE_e sMode,
    OMCI_VLAN_FILTER_MODE_e cMode, const OMCI_VLAN_OPER_ts *pUsRule)
{

    if (VLAN_FILTER_NO_TAG & sMode && VLAN_FILTER_NO_TAG & cMode)
    {
        *pFilterMode = VLAN_OPER_MODE_FORWARD_UNTAG;
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
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "[%d] TBD \n", __LINE__);

        omci_GetDownStreamFilterMode(&pDsRule->filterMode, pDsRule->filterRule.filterStagMode, pDsRule->filterRule.filterCtagMode, pUsRule);
        switch (pDsRule->filterMode)
        {
            case VLAN_OPER_MODE_FILTER_SINGLETAG:
            case VLAN_OPER_MODE_FILTER_INNER_PRI:
                pDsRule->filterRule.filterCTag = pUsRule->filterRule.filterCTag;
                break;
            case VLAN_OPER_MODE_EXTVLAN:
                if (VLAN_FILTER_CARE_TAG == pUsRule->filterRule.filterCtagMode &&
                    VLAN_FILTER_CARE_TAG == pUsRule->filterRule.filterStagMode)
                {
                    pDsRule->filterRule.filterCtagMode = VLAN_FILTER_CARE_TAG;
                    pDsRule->filterRule.filterStagMode = VLAN_FILTER_CARE_TAG;
                }

                /*if (VLAN_FILTER_CARE_TAG & pDsRule->filterRule.filterCtagMode &&
                    VLAN_FILTER_NO_TAG & pUsRule->filterRule.filterCtagMode)
                {
                    pDsRule->filterRule.filterCtagMode = VLAN_FILTER_NO_TAG;
                }*/

                if (pUsRule->filterRule.etherType > ETHTYPE_FILTER_NO_CARE)
                {
                    pDsRule->filterRule.filterCtagMode  |= VLAN_FILTER_ETHTYPE;
                    pDsRule->filterRule.etherType = pUsRule->filterRule.etherType;
                }

                if (pDsRule->filterRule.filterStagMode != pUsRule->filterRule.filterStagMode &&
                    (PRI_ACT_COPY_INNER == pUsRule->sTagAct.priAct ||
                    PRI_ACT_COPY_OUTER == pUsRule->sTagAct.priAct) &&
                    (VLAN_FILTER_PRI & pUsRule->filterRule.filterStagMode))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;
                    if (VLAN_FILTER_PRI & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.pri = pUsRule->filterRule.filterSTag.pri;
                }
                else if (VLAN_FILTER_NO_TAG == pUsRule->filterRule.filterStagMode &&
                        pDsRule->filterRule.filterCtagMode != pUsRule->filterRule.filterCtagMode &&
                        (PRI_ACT_COPY_INNER == pUsRule->cTagAct.priAct ||
                        PRI_ACT_COPY_OUTER == pUsRule->cTagAct.priAct) &&
                        (VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                    if (VLAN_FILTER_PRI & (pDsRule->filterRule.filterCtagMode & (~original)))
                        pDsRule->filterRule.filterCTag.pri = pUsRule->filterRule.filterCTag.pri;
                }
                else if (VLAN_FILTER_DSCP_PRI & pUsRule->filterRule.filterStagMode &&
                        (PRI_ACT_FROM_DSCP != pUsRule->sTagAct.priAct &&
                        PRI_ACT_FROM_DSCP != pUsRule->cTagAct.priAct))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;
                    if (VLAN_FILTER_PRI & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.pri = pUsRule->filterRule.filterSTag.pri;
                }
                else if (VLAN_FILTER_DSCP_PRI & pUsRule->filterRule.filterCtagMode &&
                        (PRI_ACT_FROM_DSCP != pUsRule->cTagAct.priAct &&
                        PRI_ACT_FROM_DSCP != pUsRule->sTagAct.priAct))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                    if (VLAN_FILTER_PRI & (pDsRule->filterRule.filterCtagMode & (~original)))
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
                        if (pDsRule->filterRule.filterStagMode & VLAN_FILTER_VID)
                        {
                            pDsRule->filterRule.filterStagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_TCI;
                        }
                        else if ((pDsRule->filterRule.filterStagMode & VLAN_FILTER_TCI) == 0x0)
                        {
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_PRI;
                        }
                     // TBD ds action prio value
                    }
                    else if (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode ||
                            VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode)
                    {
                        /*Filter Pbit*/
                        if (pDsRule->filterRule.filterStagMode & VLAN_FILTER_VID)
                        {
                            pDsRule->filterRule.filterStagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterStagMode |= VLAN_FILTER_TCI;

                        }
                        else if ((pDsRule->filterRule.filterStagMode & VLAN_FILTER_TCI)== 0x0)
                        {
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
                        if (pDsRule->filterRule.filterCtagMode & VLAN_FILTER_VID)
                        {
                            pDsRule->filterRule.filterCtagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_TCI;

                        }
                        else if ((pDsRule->filterRule.filterCtagMode & VLAN_FILTER_TCI)== 0x0)
                        {
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                        }
                        // TBD ds action prio value
                    }
                    else if (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode ||
                            VLAN_FILTER_PRI & pUsRule->filterRule.filterCtagMode)
                    {
                        /*Filter Pbit*/
                        if (pDsRule->filterRule.filterCtagMode & VLAN_FILTER_VID)
                        {
                            pDsRule->filterRule.filterCtagMode &= (~VLAN_FILTER_VID);
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_TCI;

                        }
                        else if ((pDsRule->filterRule.filterCtagMode & VLAN_FILTER_TCI)== 0x0)
                        {
                            pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_PRI;
                        }
                    }
                }

                /*For US VID copy action <-> DS filter*/
                if (pDsRule->filterRule.filterStagMode != pUsRule->filterRule.filterStagMode &&
                    (VID_ACT_COPY_INNER == pUsRule->sTagAct.vidAct ||
                    VID_ACT_COPY_OUTER == pUsRule->sTagAct.vidAct) &&
                    ((VLAN_FILTER_VID & pUsRule->filterRule.filterStagMode) ||
                    (VLAN_FILTER_TCI & pUsRule->filterRule.filterStagMode)))
                {
                    original = pDsRule->filterRule.filterStagMode;
                    pDsRule->filterRule.filterStagMode |= VLAN_FILTER_VID;
                    if (VLAN_FILTER_VID & (pDsRule->filterRule.filterStagMode & (~original)))
                        pDsRule->filterRule.filterSTag.vid = pUsRule->filterRule.filterSTag.vid;
                }
                else if (VLAN_FILTER_NO_TAG == pUsRule->filterRule.filterStagMode &&
                        pDsRule->filterRule.filterCtagMode != pUsRule->filterRule.filterCtagMode &&
                        (VID_ACT_COPY_INNER == pUsRule->cTagAct.vidAct ||
                        VID_ACT_COPY_OUTER == pUsRule->cTagAct.vidAct) &&
                        ((VLAN_FILTER_VID & pUsRule->filterRule.filterCtagMode) ||
                        (VLAN_FILTER_TCI & pUsRule->filterRule.filterCtagMode)))
                {
                    original = pDsRule->filterRule.filterCtagMode;
                    pDsRule->filterRule.filterCtagMode |= VLAN_FILTER_VID;
                    if (VLAN_FILTER_VID & (pDsRule->filterRule.filterCtagMode & (~original)))
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
        pDsRule->sTagAct.vlanAct = VLAN_ACT_NON;
    }
    break;
    case VLAN_OPER_MODE_FORWARD_ALL:
    {
        pDsRule->cTagAct.vlanAct = VLAN_ACT_NON;
        pDsRule->sTagAct.vlanAct = VLAN_ACT_NON;
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
        pDsRule->cTagAct.vlanAct = VLAN_ACT_NON;
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
            pDsRule->cTagAct.vlanAct = VLAN_ACT_NON;
            pDsRule->sTagAct.vlanAct = VLAN_ACT_NON;
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
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "[%d] TBD \n", __LINE__);

        omci_SetDsTagActByUsFilter(&pDsRule->cTagAct, pVlanFilter->filterCtagMode,pVlanFilter->filterCTag,
            pUsRule->outStyle.outTagNum, pUsRule->cTagAct);

        if (OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_BDP_00000001_USACT2DSCACT, pDsRule))
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "[%d] TBD \n", __LINE__);


        if (!memcmp(&pDsRule->filterRule.filterSTag, &pUsRule->filterRule.filterSTag, sizeof(OMCI_VLAN_ts)) &&
            !memcmp(&pDsRule->filterRule.filterCTag, &pUsRule->filterRule.filterCTag, sizeof(OMCI_VLAN_ts)))
        {
            pDsRule->cTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
            pDsRule->sTagAct.vlanAct = VLAN_ACT_TRANSPARENT;
        }

        if (VLAN_ACT_REMOVE == pDsRule->cTagAct.vlanAct &&
            VLAN_ACT_TRANSPARENT == pDsRule->sTagAct.vlanAct &&
            TPID_8100 == pUsRule->outStyle.tpid)
        {
            OMCI_VLAN_ACT_MODE_e tmp = pDsRule->sTagAct.vlanAct;
            pDsRule->sTagAct.vlanAct = pDsRule->cTagAct.vlanAct;
            pDsRule->cTagAct.vlanAct = tmp;
        }
    }
    break;
    }
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
omcidrv_setVeipUsWanVlanCf(
                           OMCI_VLAN_OPER_ts       *pVlanRule,
                           int                     vid,
                           int                     pri,
                           int                     type,
                           unsigned int            cfIdx,
                           unsigned int            streamId,
                           omci_pon_wan_rule_t     *pPonWanRule,
                           rtk_rg_classifyEntry_t  *pRgCfEntry)
{

    int                     ret = OMCI_ERR_OK;
    omci_rule_pri_t         rule_pri;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    if (!pVlanRule || !pRgCfEntry || !pPonWanRule)
        return OMCI_ERR_FAILED;


    memset(pPonWanRule, 0, sizeof(omci_pon_wan_rule_t));
    pPonWanRule->flowId = streamId;
    pPonWanRule->wanVid = vid;
    pPonWanRule->wanPri = pri;
    pPonWanRule->ponVid = vid;
    pPonWanRule->ponPri = pri;

    memset(pRgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));
    pRgCfEntry->index = cfIdx;
    pRgCfEntry->direction = RTK_RG_CLASSIFY_DIRECTION_UPSTREAM;

    omci_SetClassifyUsAct(pVlanRule, streamId, pRgCfEntry);
    omci_SetClassifyUsRule(pVlanRule, gPlatformDb.cpuPort, pRgCfEntry, &rule_pri);

    if (-1 == vid)
    {
        pRgCfEntry->outterTagVid = 0;
        pRgCfEntry->filter_fields &= ~EGRESS_TAGVID_BIT;
        pRgCfEntry->ctagIf = 0;
        pRgCfEntry->filter_fields |= EGRESS_CTAGIF_BIT;
        pRgCfEntry->stagIf = 0;
        pRgCfEntry->filter_fields |= EGRESS_STAGIF_BIT;
        pPonWanRule->ponVid = -1;
    }

    if (-1 == pri)
    {
        pRgCfEntry->outterTagPri = 0;
        pRgCfEntry->filter_fields &= ~EGRESS_TAGPRI_BIT;
        pPonWanRule->ponPri = -1;
    }

    if (gDrvCtrl.dmMode)
        return ret;

    if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_add(pRgCfEntry)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "[%d] create rule for us wan vlan cf fail, ret = %d", __LINE__, ret);
        ret = OMCI_ERR_FAILED;
    }

    _saveCfCfgToDb(pRgCfEntry->index, pRgCfEntry);
    return ret;
}


static int
omcidrv_setVeipDsWanVlanCf(OMCI_VLAN_OPER_ts    *pVlanRule,
                            int                 vid,
                            int                 pri,
                            int                 type,
                            unsigned int        cfIdx,
                            unsigned int        streamId)
{

    int                     ret;
    rtk_rg_classifyEntry_t  rgCfEntry;
    OMCI_VLAN_OPER_ts       dsVlanRule;
    omci_rule_pri_t         rule_pri;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    if (!pVlanRule)
        return OMCI_ERR_FAILED;

    memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));
    rgCfEntry.index = cfIdx;
    rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM;

    // set cf action
    omci_SetUsRuleInvert2Ds(&dsVlanRule, pVlanRule);
    ret = omci_SetClassifyDsAct(&dsVlanRule,
            (gPlatformDb.uniPortMask) | (1 << gPlatformDb.cpuPort), &rgCfEntry);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set action for ds wan vlan cf fail!");

        return ret;
    }

    // modify ctag
    rgCfEntry.ds_action_field |= CF_DS_ACTION_CTAG_BIT;

    // modify vid
    if (vid >= 0)
    {
        rgCfEntry.action_cvlan.cvlanTagIfDecision = ACL_CVLAN_TAGIF_TAGGING;
        rgCfEntry.action_cvlan.cvlanCvidDecision = ACL_CVLAN_CVID_ASSIGN;
        rgCfEntry.action_cvlan.assignedCvid = vid;

        // modify pri
        if (pri >= 0)
        {
            rgCfEntry.action_cvlan.cvlanCpriDecision = ACL_CVLAN_CPRI_ASSIGN;
            rgCfEntry.action_cvlan.assignedCpri = pri;
        }
    }
    else
        rgCfEntry.action_cvlan.cvlanTagIfDecision = ACL_CVLAN_TAGIF_UNTAG;

    // set cf rule
    ret = omci_SetClassifyDsRule(&dsVlanRule, pVlanRule, streamId, &rgCfEntry, &rule_pri);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set rule for ds wan vlan cf fail!");

        return ret;
    }

    if(VLAN_OPER_MODE_FORWARD_ALL == dsVlanRule.filterMode)
    {
        if (vid >= 0)
        {
            rgCfEntry.outterTagVid = vid;
            rgCfEntry.filter_fields |= EGRESS_TAGVID_BIT;

            // modify pri
            if (pri >= 0)
            {
                rgCfEntry.outterTagVid = pri;
                rgCfEntry.filter_fields |= EGRESS_TAGPRI_BIT;
            }
        }
    }

    // modify gem mask in purpose to make the hw path error
    if (OMCI_MODE_BRIDGE != type)
        rgCfEntry.gemidx_mask = 0xFFFF;

    // invoke driver api
    if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_add(&rgCfEntry)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "create rule for ds wan vlan cf fail, ret = %d", ret);
    }

    _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);

    return ret;
}

static int
omcidrv_setAclLatchCfContent(OMCI_BRIDGE_RULE_ts *pBridgeRule,
                                rtk_rg_aclFilterAndQos_t *pRgAclEntry)
{

    if (!pBridgeRule || !pRgAclEntry)
        return OMCI_ERR_FAILED;

    memset(pRgAclEntry, 0, sizeof(rtk_rg_aclFilterAndQos_t));
    pRgAclEntry->fwding_type_and_direction =
        ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_DOWN_CVLAN_SVLAN;

    pRgAclEntry->filter_fields =
                INGRESS_STREAM_ID_BIT | INGRESS_CTAGIF_BIT | INGRESS_PORT_BIT;
    pRgAclEntry->ingress_stream_id = pBridgeRule->dsFlowId;
    pRgAclEntry->ingress_ctagIf = 1;
    pRgAclEntry->ingress_port_mask.portmask = (1 << gPlatformDb.ponPort);

    // set cf action
    pRgAclEntry->action_type = ACL_ACTION_TYPE_QOS;
    pRgAclEntry->qos_actions = ACL_ACTION_ACL_INGRESS_VID_BIT |
                                    ACL_ACTION_ACL_CVLANTAG_BIT |
                                    ACL_ACTION_DS_UNIMASK_BIT;

    // acl ingress vlan translate
    pRgAclEntry->action_acl_ingress_vid = pBridgeRule->vlanRule.filterRule.filterCTag.vid;

    // ingress cvid set to 1 if multicast connection is forward all
    if (VLAN_OPER_MODE_FORWARD_ALL == pBridgeRule->vlanRule.filterMode)
        pRgAclEntry->action_acl_ingress_vid = 1;

    // ds CF ctag action for egress cvlan
    if(omci_GetDsCVlanRawAct(pBridgeRule->vlanRule.cTagAct.vlanAct,
        &pRgAclEntry->action_acl_cvlan.cvlanTagIfDecision) ==-1)
    {
        return OMCI_ERR_FAILED;
    }
    if(omci_GetDsCVidRawAct(pBridgeRule->vlanRule.cTagAct.vidAct,
        &pRgAclEntry->action_acl_cvlan.cvlanCvidDecision) ==-1)
    {
        return OMCI_ERR_FAILED;
    }
    if(omci_GetDsCPriRawAct(pBridgeRule->vlanRule.cTagAct.priAct,
        &pRgAclEntry->action_acl_cvlan.cvlanCpriDecision) ==-1)
    {
        return OMCI_ERR_FAILED;
    }
    pRgAclEntry->action_acl_cvlan.assignedCvid = pBridgeRule->vlanRule.cTagAct.assignVlan.vid;
    pRgAclEntry->action_acl_cvlan.assignedCpri = (pBridgeRule->vlanRule.cTagAct.assignVlan.pri < 8 ?
                                                pBridgeRule->vlanRule.cTagAct.assignVlan.pri : 0);

    // since this acl affects all multicasting
    // ds uni port mask should NOT be limited to certain ports
    // otherwise, it will overwrites forwarding capabilities
    // ds CF uni port mask
    // if(pBridgeRule->uniMask & (1 << PF_VEIP_PROT))
        pRgAclEntry->downstream_uni_portmask = gPlatformDb.uniPortMask;
    // else
    //      pRgAclEntry->downstream_uni_portmask = pBridgeRule->uniMask;

    return OMCI_ERR_OK;

}

static int
omcidrv_setVeipUsWanIntfCfContent(OMCI_VLAN_OPER_ts             *pVlanRule,
                                    int                         vid,
                                    int                         pri,
                                    int                         netIfIdx,
                                    unsigned int                streamId,
                                    unsigned char               defaultCf,
                                    omci_pon_wan_rule_t         *pPonWanRule,
                                    rtk_rg_aclFilterAndQos_t    *pRgAclEntry)
{
    rtk_rg_cvlan_tag_action_t   *pEntryCtag;

    if (!pVlanRule || !pPonWanRule)
        return OMCI_ERR_FAILED;


    memset(pPonWanRule, 0, sizeof(omci_pon_wan_rule_t));
    pPonWanRule->flowId = streamId;
    pPonWanRule->wanVid = vid;
    pPonWanRule->wanPri = pri;
    pPonWanRule->ponVid = -1;
    pPonWanRule->ponPri = -1;

    pEntryCtag = &pRgAclEntry->action_acl_cvlan;

    memset(pRgAclEntry, 0, sizeof(rtk_rg_aclFilterAndQos_t));
    pRgAclEntry->fwding_type_and_direction =
        ACL_FWD_TYPE_DIR_INGRESS_OR_EGRESS_L34_UP_STREAMID_CVLAN_SVLAN;

    // set cf rule
    pRgAclEntry->filter_fields = EGRESS_INTF_BIT;
    pRgAclEntry->egress_intf_idx = netIfIdx;

    // set tag filter if netIfIdx = 0
    if (0 == netIfIdx)
    {
        pRgAclEntry->filter_fields |= INGRESS_CTAGIF_BIT;

        if (defaultCf || vid < 0)
        {
            pRgAclEntry->ingress_ctagIf = 0;
        }
        else
        {
            pRgAclEntry->ingress_ctagIf = 1;
            pRgAclEntry->filter_fields |= EGRESS_CTAG_VID_BIT;
            pRgAclEntry->egress_ctag_vid = vid;
            pPonWanRule->ponVid = vid;
        }
    }

    // set cf action
    pRgAclEntry->action_type = ACL_ACTION_TYPE_QOS;
    pRgAclEntry->qos_actions =
        ACL_ACTION_STREAM_ID_OR_LLID_BIT | ACL_ACTION_ACL_CVLANTAG_BIT;
    pRgAclEntry->action_stream_id_or_llid = streamId;
    if (VLAN_ACT_NON == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_TRANSPARENT == pVlanRule->cTagAct.vlanAct)
    {
        // assign vid
        if (vid >= 0)
        {
            pEntryCtag->cvlanTagIfDecision = ACL_CVLAN_TAGIF_TAGGING;
            pEntryCtag->cvlanCvidDecision = ACL_CVLAN_CVID_ASSIGN;
            pEntryCtag->assignedCvid = vid;
            pPonWanRule->ponVid = vid;

            if (RTL9602C_CHIP_ID == gPlatformDb.chipId)
                pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_NOP;
            else
            {
                // assign pri
                if (pri >= 0)
                {
                    pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_ASSIGN;
                    pEntryCtag->assignedCpri = pri;
                    pPonWanRule->ponPri = pri;
                }
                else
                {
                    // use internal priority when wan 1p disabled
                    pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
                    pPonWanRule->ponPri = pri;
                }
            }
        }
        else
        {
            pEntryCtag->cvlanTagIfDecision = ACL_CVLAN_TAGIF_UNTAG;
        }
    }
    else if (VLAN_ACT_ADD == pVlanRule->cTagAct.vlanAct ||
            VLAN_ACT_MODIFY == pVlanRule->cTagAct.vlanAct)
    {
        pEntryCtag->cvlanTagIfDecision = ACL_CVLAN_TAGIF_TAGGING;

        // assign vid
        if (VID_ACT_ASSIGN == pVlanRule->cTagAct.vidAct)
        {
            pEntryCtag->cvlanCvidDecision = ACL_CVLAN_CVID_ASSIGN;
            pEntryCtag->assignedCvid = pVlanRule->cTagAct.assignVlan.vid;
            pPonWanRule->ponVid = pVlanRule->cTagAct.assignVlan.vid;
        }
        else
        {
            pEntryCtag->cvlanCvidDecision = ACL_CVLAN_CVID_COPY_FROM_1ST_TAG;
            pPonWanRule->ponVid = pPonWanRule->wanVid;
        }

        if (RTL9602C_CHIP_ID == gPlatformDb.chipId)
            pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_NOP;
        else
        {
            // assign pri
            if (PRI_ACT_ASSIGN == pVlanRule->cTagAct.priAct)
            {
                pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_ASSIGN;
                pEntryCtag->assignedCpri = pVlanRule->cTagAct.assignVlan.pri;
                pPonWanRule->ponPri = pVlanRule->cTagAct.assignVlan.pri;
            }
            else
            {
                pEntryCtag->cvlanCpriDecision = ACL_CVLAN_CPRI_COPY_FROM_1ST_TAG;
                pPonWanRule->ponPri = pri;
            }
        }
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set action for us wan intf cf fail!");

        return OMCI_ERR_FAILED;
    }

    return OMCI_ERR_OK;
}

static int
omcidrv_setVeipUsWanIntfCf(OMCI_VLAN_OPER_ts    *pVlanRule,
                            int                 vid,
                            int                 pri,
                            int                 netIfIdx,
                            unsigned int        *pCfIdx,
                            unsigned int        streamId,
                            unsigned char       defaultCf)
{
    int                         ret = OMCI_ERR_FAILED;
    omci_pon_wan_rule_t         ponWanEntry;
    rtk_rg_aclFilterAndQos_t    rgAclEntry;
    int                         rgAclEntryIdx;

    if (!pVlanRule || !pCfIdx)
        return OMCI_ERR_FAILED;

    // set cf content
    ret = omcidrv_setVeipUsWanIntfCfContent(pVlanRule,
                    vid, pri, netIfIdx, streamId, defaultCf, &ponWanEntry, &rgAclEntry);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "create content for us wan intf cf fail, ret = %d", ret);

        return OMCI_ERR_FAILED;
    }

    // invoke driver api
    ret = rtk_rg_aclFilterAndQos_add(&rgAclEntry, &rgAclEntryIdx);
    if (RT_ERR_RG_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "create rule for us wan intf cf fail, ret = %d", ret);
    }
    else
    {
            gPlatformDb.cfRule[rgAclEntryIdx].isCfg = 1;
            *pCfIdx = rgAclEntryIdx;
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
omcidrv_setVeipRuleByDmm(
    veip_service_t *pVeipEntry,
    int             wanIdx,
    int             vid,
    int             pri,
    int             type,
    int             service,
    int             isBinding,
    int             netIfIdx)
{
    int                     ret = OMCI_ERR_FAILED;
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veipGemFlow_entry_t     *pEntryData;
    uint8                   bFound;
    omci_dm_pon_wan_info_t  dmEntry;
    omci_rule_pri_t         rule_pri;
    rtk_rg_cvlan_info_t     rgCvlan;
    uint8                   i;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
    memset(&gPon_wan_pf_info, 0, sizeof(omci_dmm_pon_wan_pf_info_t));
    memset(&gPonWanEntry, 0, sizeof(omci_pon_wan_rule_t) * OMCI_PON_WAN_RULE_POS_END);

    if (!pVeipEntry)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: VeipEntry is NULL\n",
            __FUNCTION__);
        return OMCI_ERR_FAILED;
    }

    if (!pDmmCb || !pDmmCb->omci_dm_pon_wan_info_set)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "dmm callback is not registered");

        return OMCI_ERR_FAILED;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set veip rule by dmm for wan %d", wanIdx);

    bFound = FALSE;
    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipGemFlowHead)
    {
        pEntryData = list_entry(pEntry, veipGemFlow_entry_t, list);

        if (pEntryData->data.flowId[0] == pVeipEntry->usStreamId)
        {
            bFound = TRUE;
            break;
        }
    }
    if (!bFound)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: Failed to find a veipGemFlow for wanIf[%u]\n",
            __FUNCTION__,
            wanIdx);
        return OMCI_ERR_FAILED;
    }

    memset(&dmEntry, 0, sizeof(omci_dm_pon_wan_info_t));
    gPon_wan_pf_info.wanIdx = wanIdx;
    dmEntry.wanIdx = wanIdx;
    dmEntry.wanType = type;
    dmEntry.gemPortId = pEntryData->data.gemPortId;
    dmEntry.tcontId = pEntryData->data.tcontId;
    for (i = 0; i < gDrvCtrl.wanQueueNum; i++)
    {
        dmEntry.usFlowId[i] = pEntryData->data.flowId[i];
        dmEntry.queueSts[i] = omci_getTcontQid(pEntryData->data.tcontId, pEntryData->data.tcQueueId[i]);
        dmEntry.queueCfg[i].type = STRICT_PRIORITY;
    }

    if (OMCI_MODE_BRIDGE != type)
    {
        // allocate cf for ds vlan translation
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for r-wan ds sw path fail!");

            return OMCI_ERR_FAILED;
        }

        // set cf content for sw path
        // here rgAclEntry is swapped with rgUsSwAclEntry
        // because rgAclEntry is configured ahead of rgUsSwAclEntry
        // i.e., the classification employs first-hit-first-select rules
        ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                vid, pri, netIfIdx, pVeipEntry->usStreamId, FALSE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], &(gPon_wan_pf_info.rgAclEntry)/*&dmEntry.rgAclEntry*/);
        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create cf content for r-wan sw path fail, ret = %d", ret);

            // rollback
            _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
            pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

            memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

            return OMCI_ERR_FAILED;
        }
        else
        {
            /*dmEntry.rgAclEntry.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
            dmEntry.rgAclEntry.ingress_port_idx = gPlatformDb.cpuPort;*/
            gPon_wan_pf_info.rgAclEntry.filter_fields |= INGRESS_EGRESS_PORTIDX_BIT;
            gPon_wan_pf_info.rgAclEntry.ingress_port_idx = gPlatformDb.cpuPort;


            // c-tag priority will be determined by the protocol stack instead of hw default
            if (ACL_CVLAN_TAGIF_TAGGING ==
                    gPon_wan_pf_info.rgAclEntry.action_acl_cvlan.cvlanTagIfDecision
                    /*dmEntry.rgAclEntry.action_acl_cvlan.cvlanTagIfDecision*/)
            {
                if (RTL9602C_CHIP_ID == gPlatformDb.chipId)
                    gPon_wan_pf_info.rgAclEntry.action_acl_cvlan.cvlanCpriDecision = ACL_CVLAN_CPRI_NOP;//dmEntry.rgAclEntry.action_acl_cvlan.cvlanCpriDecision = ACL_CVLAN_CPRI_NOP;
                else
                {
                    /*dmEntry.rgAclEntry.action_acl_cvlan.cvlanCpriDecision =
                        ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;*/
                    // assign pri
                    if (pri >= 0)
                    {
                        gPon_wan_pf_info.rgAclEntry.action_acl_cvlan.cvlanCpriDecision = ACL_CVLAN_CPRI_ASSIGN;
                        gPon_wan_pf_info.rgAclEntry.action_acl_cvlan.assignedCpri = pri;
                    }
                    else
                    {
                        // use internal priority when wan 1p disabled
                        gPon_wan_pf_info.rgAclEntry.action_acl_cvlan.cvlanCpriDecision = ACL_CVLAN_CPRI_COPY_FROM_INTERNAL_PRI;
                    }
                }
                gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].wanPri;
            }
        }

        // set cf content for hw path
        ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                vid, pri, netIfIdx, pVeipEntry->usStreamId, FALSE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], &(gPon_wan_pf_info.rgUsSwAclEntry)/*&dmEntry.rgUsSwAclEntry*/);
        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create cf content for r-wan hw path fail, ret = %d", ret);

            // rollback
            _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
            pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

            memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

            memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], 0, sizeof(omci_pon_wan_rule_t));
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponVid = -1;
            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponPri = -1;

            return OMCI_ERR_FAILED;
        }

        if (RTL9602C_CHIP_ID == gPlatformDb.chipId && CHIP_REV_ID_A == gPlatformDb.chipRev)
        {
            if (OMCI_MODE_PPPOE_V4NAPT_V6 == type || OMCI_MODE_IPOE_V4NAPT_V6 == type)
            {
                ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule, vid, pri,
                        netIfIdx + 8, pVeipEntry->usStreamId, FALSE,
                        &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A], &(gPon_wan_pf_info.rgPatchEntry[PATCH_DS_9602C_A].rule.aclEntry)
                        /*&(dmEntry.rgPatchEntry[PATCH_DS_9602C_A].rule.aclEntry)*/);

                dmEntry.rgPatchEntry[PATCH_DS_9602C_A].valid            = TRUE;
                gPon_wan_pf_info.rgPatchEntry[PATCH_DS_9602C_A].valid    = TRUE;
                dmEntry.rgPatchEntry[PATCH_DS_9602C_A].ruleOpt   = OMCI_CF_RULE_RG_OPT_ACL;
                gPon_wan_pf_info.rgPatchEntry[PATCH_DS_9602C_A].ruleOpt    = OMCI_CF_RULE_RG_OPT_ACL;
                memcpy(&(dmEntry.rgPatchEntry[PATCH_DS_9602C_A].rule), &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A], sizeof(omci_pon_wan_rule_t));

                if (OMCI_ERR_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create cf rule for r-wan hw+8 path fail, ret = %d", ret);

                    // rollback
                    _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                    pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                    memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                    memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], 0, sizeof(omci_pon_wan_rule_t));
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponVid = -1;
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponPri = -1;

                    memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A], 0, sizeof(omci_pon_wan_rule_t));
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A].ponVid = -1;
                    gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DS_9602C_A].ponPri = -1;

                    return OMCI_ERR_FAILED;
                }
            }
        }

        // in case omci translate vlans between wan interface and ani
        if (pVeipEntry->rule.outStyle.outVlan.vid != vid &&
            pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
        {
            // for route mode the vlan translation has to be done for sw path
            // but since l34 module will handle the translation for hw path
            // we make a error in purpose to create sw path rule only
            ret = omcidrv_setVeipDsWanVlanCf(&pVeipEntry->rule,
                    vid, pri, type, pVeipEntry->pSwPathCfIdx[wanIdx], pVeipEntry->dsStreamId);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create ds vlan translation cf for r-wan fail, ret = %d", ret);

                // no rollback for routing mode
                _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                // return OMCI_ERR_FAILED;
            }

            // set cvlan table
            if (!gPlatformDb.rgIvlMcastSupport &&
                    pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
            {
                memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                rgCvlan.vlanId = pVeipEntry->rule.outStyle.outVlan.vid;
                rgCvlan.memberPortMask.portmask =
                    (gPlatformDb.cpuPortMask | (1 << gPlatformDb.ponPort) | gPlatformDb.extPortMask);
                if (vid < 0) {
                    rgCvlan.untagPortMask.portmask =
                        gPlatformDb.cpuPortMask | gPlatformDb.extPortMask;
                }

                ret = rtk_rg_cvlan_add(&rgCvlan);
                if (RT_ERR_RG_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ani vlan for veip fail, ret = %d", ret);
                }
            }
        }
    }
    else
    {
        // allocate cf for ds vlan translation
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for b-wan sw path fail!");

            return OMCI_ERR_FAILED;
        }

        if (!isBinding)
        {
            // only create cf when the service = internet
            if (1 == service)
            {
                /*APOLLOMP CHIP limiation */
                if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                {
                    // create default cf
                    ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                            vid, pri, 0, pVeipEntry->usStreamId, TRUE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], &(gPon_wan_pf_info.rgAclEntry)/*&dmEntry.rgAclEntry*/);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf content for b-wan default path fail, ret = %d", ret);

                        // rollback
                        _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                        pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                        memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                        return OMCI_ERR_FAILED;
                    }

                    if (vid >= 0)
                    {
                        ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                                vid, pri, 0, pVeipEntry->usStreamId, FALSE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], &(gPon_wan_pf_info.rgUsSwAclEntry)/*&dmEntry.rgUsSwAclEntry*/);
                        if (OMCI_ERR_OK != ret)
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                                "create cf content for b-wan hw path fail, ret = %d", ret);

                            // rollback
                            _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                            pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;


                            memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                            memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], 0, sizeof(omci_pon_wan_rule_t));
                            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponVid = -1;
                            gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponPri = -1;

                            return OMCI_ERR_FAILED;
                        }
                    }
                }
                else
                {
                    // set cf content for hw path
                    ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                            vid, pri, netIfIdx, pVeipEntry->usStreamId, FALSE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], &(gPon_wan_pf_info.rgAclEntry)/*&dmEntry.rgAclEntry*/);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf content for b-wan hw path fail, ret = %d", ret);

                        // rollback
                        _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                        pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                        memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                        return OMCI_ERR_FAILED;
                    }
                }
            }
            else
            {
                // mark that no cf is created
                // following translate cf should be ignored
                bFound = FALSE;
            }
        }
        else
        {
            ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                    vid, pri, netIfIdx, pVeipEntry->usStreamId, FALSE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], &(gPon_wan_pf_info.rgAclEntry)/*&dmEntry.rgAclEntry*/);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create cf content for b-wan hw path fail, ret = %d", ret);

                // rollback
                _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                return OMCI_ERR_FAILED;
            }

            // service = internet, create default cf
            if (1 == service)
            {
                /*APOLLOMP CHIP limiation */
                if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                {
                    ret = omcidrv_setVeipUsWanIntfCfContent(&pVeipEntry->rule,
                            vid, pri, 0, pVeipEntry->usStreamId, TRUE, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], &(gPon_wan_pf_info.rgUsSwAclEntry) /*&dmEntry.rgUsSwAclEntry*/);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf content for b-wan default path fail, ret = %d", ret);

                        // rollback
                        _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                        pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                        memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL], 0, sizeof(omci_pon_wan_rule_t));
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponVid = -1;
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_ACL].ponPri = -1;

                        memset(&gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL], 0, sizeof(omci_pon_wan_rule_t));
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponVid = -1;
                        gPonWanEntry[OMCI_PON_WAN_RULE_POS_RG_US_SW_ACL].ponPri = -1;

                        return OMCI_ERR_FAILED;
                    }
                }
            }
        }

        // in case omci translate vlans between wan interface and ani
        if (pVeipEntry->rule.outStyle.outVlan.vid != vid && bFound &&
                pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
        {
            // only create ds vlan translation cf for non-mac-based cvid decision
            if (!gPlatformDb.rgMacBasedTag &&
                    OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
            {
                // create ds vlan translation cf
                ret = omcidrv_setVeipDsWanVlanCf(&pVeipEntry->rule,
                        vid, pri, type, pVeipEntry->pSwPathCfIdx[wanIdx], pVeipEntry->dsStreamId);
                if (OMCI_ERR_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ds vlan translation cf for b-wan fail, ret = %d", ret);

                    // rollback
                    _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
                    pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

                }
            }

            // set cvlan table
            if (!gPlatformDb.rgIvlMcastSupport &&
                    pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
            {
                memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                rgCvlan.vlanId = pVeipEntry->rule.outStyle.outVlan.vid;
                rgCvlan.memberPortMask.portmask =
                    (gPlatformDb.uniPortMask | (1 << gPlatformDb.ponPort) | gPlatformDb.extPortMask);
                if (vid < 0 ||
                        OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
                    rgCvlan.untagPortMask.portmask = gPlatformDb.uniPortMask;

                ret = rtk_rg_cvlan_add(&rgCvlan);
                if (RT_ERR_RG_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ani vlan for veip fail, ret = %d", ret);
                }
            }
        }
    }

    // allocate cf for DPI if turn off HWNAT
    if (OMCI_ERR_OK ==
            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pUsSwPathCfIdx[wanIdx]))
    {
        // for route mode and turn off HWNAT
        ret = omcidrv_setVeipUsWanVlanCf(&pVeipEntry->rule, vid, pri,
                                         type, pVeipEntry->pUsSwPathCfIdx[wanIdx], pVeipEntry->usStreamId,
                                         &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DPI], &(gPon_wan_pf_info.rgPatchEntry[PATCH_DPI].rule.cfEntry)
                                         /*&(dmEntry.rgPatchEntry[PATCH_DPI].rule.cfEntry)*/);

        dmEntry.rgPatchEntry[PATCH_DPI].valid           = TRUE;
        gPon_wan_pf_info.rgPatchEntry[PATCH_DPI].valid   = TRUE;
        dmEntry.rgPatchEntry[PATCH_DPI].ruleOpt         = OMCI_CF_RULE_RG_OPT_CF;
        gPon_wan_pf_info.rgPatchEntry[PATCH_DPI].ruleOpt = OMCI_CF_RULE_RG_OPT_CF;
        memcpy(&(dmEntry.rgPatchEntry[PATCH_DPI].rule), &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DPI], sizeof(omci_pon_wan_rule_t));

        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create us cf for r-wan fail if turn off HWNAT, ret = %d", ret);

            // rollback
            _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
            pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

            _RemoveUsedCfIndex(pVeipEntry->pUsSwPathCfIdx[wanIdx]);
            pVeipEntry->pUsSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

            return OMCI_ERR_FAILED;
        }

    }

#if 1// TBD, refine
    if (OMCI_ERR_OK != omci_dmm_pon_wan_pf_info_entry_add(wanIdx, &gPon_wan_pf_info))
    {

        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "record pon_wan pf_info etnry failed");

        return OMCI_ERR_FAILED;
    }
#endif
    ret = pDmmCb->omci_dm_pon_wan_info_set(&dmEntry);
    if (0 != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "set veip rule by dmm for wan %d fail, ret = %d", wanIdx, ret);

        return OMCI_ERR_FAILED;
    }

    for (i = 0; i < PATCH_END; i++)
    {
        if (gPon_wan_pf_info.rgPatchEntry[i].valid &&
            OMCI_CF_RULE_RG_OPT_CF == gPon_wan_pf_info.rgPatchEntry[i].ruleOpt)
        {
            _saveCfCfgToDb(gPon_wan_pf_info.rgPatchEntry[i].rule.cfEntry.index,
                &(gPon_wan_pf_info.rgPatchEntry[i].rule.cfEntry));
        }
    }

    return OMCI_ERR_OK;
}

static int
omcidrv_setVeipRule(veip_service_t  *pVeipEntry,
                    int             wanIdx,
                    int             vid,
                    int             pri,
                    int             type,
                    int             service,
                    int             isBinding,
                    int             netIfIdx)
{
    int                     ret = OMCI_ERR_FAILED;
    rtk_rg_cvlan_info_t     rgCvlan;
    rtk_rg_classifyEntry_t  rgCfEntry;
    uint8                   bFound;
    omci_rule_pri_t         rule_pri;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    memset(&gPonWanEntry, 0, sizeof(omci_pon_wan_rule_t) * OMCI_PON_WAN_RULE_POS_END);

    if (!pVeipEntry)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: VeipEntry is NULL\n",
            __FUNCTION__);
        return OMCI_ERR_FAILED;
    }

    // check if cf already exists
    if (OMCI_UNUSED_CF != pVeipEntry->pHwPathCfIdx[wanIdx] ||
            OMCI_UNUSED_CF != pVeipEntry->pSwPathCfIdx[wanIdx] ||
            OMCI_UNUSED_CF != pVeipEntry->pUsSwPathCfIdx[wanIdx] ||
            OMCI_UNUSED_CF != pVeipEntry->defaultPathCfIdx)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: there is a existed entry for wanIf[%u]\n",
            __FUNCTION__,
            wanIdx);
        return OMCI_ERR_ENTRY_EXIST;
    }

    // netifidx should be available
    if (netIfIdx < 0 || netIfIdx >= gPlatformDb.intfNum)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: netIfIdx %u is out of bound for wanIf[%u]\n",
            __FUNCTION__,
            netIfIdx,
            wanIdx);
        return OMCI_ERR_FAILED;
    }

    // take control by dmm if dmMode is enabled
    if (gDrvCtrl.dmMode)
        return omcidrv_setVeipRuleByDmm(pVeipEntry, wanIdx, vid, pri, type, service, isBinding, netIfIdx);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set veip rule for wan %d", wanIdx);

    if (OMCI_MODE_BRIDGE != type)
    {
        // allocate cf for ds vlan translation
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for r-wan ds sw path fail!");

            return OMCI_ERR_FAILED;
        }
        // allocate cf of us in order to turn off HWNAT in Romedriver
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pUsSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for r-wan us sw path fail!");

            ret = OMCI_ERR_FAILED;

            goto remove_sw_path_cf;
        }

        // create cf for hw path
        ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                netIfIdx, &pVeipEntry->pHwPathCfIdx[wanIdx], pVeipEntry->usStreamId, FALSE);
        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create cf rule for r-wan hw path fail, ret = %d", ret);

            goto remove_us_sw_path_cf;
        }

        if (RTL9602C_CHIP_ID == gPlatformDb.chipId && CHIP_REV_ID_A == gPlatformDb.chipRev)
        {
            if (OMCI_MODE_PPPOE_V4NAPT_V6 == type || OMCI_MODE_IPOE_V4NAPT_V6 == type)
            {
                pVeipEntry->defaultPathCfIdx = OMCI_UNUSED_CF;

                ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                        netIfIdx + 8, &pVeipEntry->defaultPathCfIdx, pVeipEntry->usStreamId, FALSE);
                if (OMCI_ERR_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create cf rule for r-wan hw+8 path fail, ret = %d", ret);

                    // rollback
                    if(pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
                    {
                        if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx])) != RT_ERR_OK)
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "rollback and delete CF index %d fail, ret = %d", pVeipEntry->pHwPathCfIdx[wanIdx], ret);
                    }

                    goto remove_us_sw_path_cf;
                }
            }
        }

        // for route mode and turn off HWNAT
        ret = omcidrv_setVeipUsWanVlanCf(&pVeipEntry->rule, vid, pri,
                                         type, pVeipEntry->pUsSwPathCfIdx[wanIdx],
                                         pVeipEntry->usStreamId, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DPI], &rgCfEntry);
        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create us sw cf for r-wan fail, ret = %d", ret);

            // rollback
            if(pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
            {
                if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx])) != RT_ERR_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "rollback and delete CF index %d fail, ret = %d", pVeipEntry->pHwPathCfIdx[wanIdx], ret);
            }

            goto remove_hw_path_cf;
        }

        // in case omci translate vlans between wan interface and ani
        if (pVeipEntry->rule.outStyle.outVlan.vid != vid &&
            pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
        {
            // for route mode the vlan translation has to be done for sw path
            // but since l34 module will handle the translation for hw path
            // we make a error in purpose to create sw path rule only
            ret = omcidrv_setVeipDsWanVlanCf(&pVeipEntry->rule,
                    vid, pri, type, pVeipEntry->pSwPathCfIdx[wanIdx], pVeipEntry->dsStreamId);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create ds vlan translation cf for r-wan fail, ret = %d", ret);

                // no rollback for routing mode
                // rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx]);
                // if (OMCI_UNUSED_CF != pVeipEntry->defaultPathCfIdx)
                //  rtk_rg_aclFilterAndQos_del(pVeipEntry->defaultPathCfIdx);

                // goto remove_hw_path_cf;
            }

            // set cvlan table
            if (!gPlatformDb.rgIvlMcastSupport &&
                    pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
            {
                memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                rgCvlan.vlanId = pVeipEntry->rule.outStyle.outVlan.vid;
                rgCvlan.memberPortMask.portmask =
                    (gPlatformDb.cpuPortMask | (1 << gPlatformDb.ponPort) | gPlatformDb.extPortMask);
                if (vid < 0) {
                    rgCvlan.untagPortMask.portmask =
                        (gPlatformDb.cpuPortMask | gPlatformDb.extPortMask);
                }

                ret = rtk_rg_cvlan_add(&rgCvlan);
                if (RT_ERR_RG_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ani vlan %u for veip fail, ret = %d",
                        pVeipEntry->rule.outStyle.outVlan.vid, ret);
                }
            }
        }

        return OMCI_ERR_OK;
    }
    else
    {
        bFound = TRUE;

        // allocate cf for ds vlan translation
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for b-wan sw path fail!");

            return OMCI_ERR_FAILED;
        }

        // allocate cf of us in order to turn off HWNAT in Romedriver
        if (OMCI_ERR_OK !=
                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pVeipEntry->pUsSwPathCfIdx[wanIdx]))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "allocate cf for r-wan sw path fail!");

            ret = OMCI_ERR_FAILED;

            goto remove_sw_path_cf;
        }

        if (!isBinding)
        {
            // only create cf when the service = internet
            if (1 == service)
            {
                /*APOLLOMP CHIP limiation */
                if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                {
                    // create default cf
                    ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                            0, &pVeipEntry->defaultPathCfIdx, pVeipEntry->usStreamId, TRUE);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf rule for b-wan default path fail, ret = %d", ret);

                        goto remove_us_sw_path_cf;
                    }

                    if (vid >= 0)
                    {
                        ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                                0, &pVeipEntry->pHwPathCfIdx[wanIdx], pVeipEntry->usStreamId, FALSE);
                        if (OMCI_ERR_OK != ret)
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                                "create cf rule for b-wan hw path fail, ret = %d", ret);

                            // rollback
                            if(pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
                            {
                                if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->defaultPathCfIdx)) != RT_ERR_OK)
                                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "rollback and delete CF index %d fail, ret = %d", pVeipEntry->defaultPathCfIdx, ret);
                            }

                            goto remove_def_path_cf;
                        }
                    }
                }
                else
                {
                    // create cf for hw path
                    ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                            netIfIdx, &pVeipEntry->pHwPathCfIdx[wanIdx], pVeipEntry->usStreamId, FALSE);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf rule for b-wan hw path fail, ret = %d", ret);

                        goto remove_us_sw_path_cf;
                    }
                }
            }
            else
            {
                // mark that no cf is created
                // following translate cf should be ignored
                bFound = FALSE;
            }
        }
        else
        {
            ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                    netIfIdx, &pVeipEntry->pHwPathCfIdx[wanIdx], pVeipEntry->usStreamId, FALSE);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create cf rule for b-wan hw path fail, ret = %d", ret);

                goto remove_us_sw_path_cf;
            }

            // service = internet, create default cf
            if (1 == service)
            {
                /*APOLLOMP CHIP limiation */
                if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                {
                    ret = omcidrv_setVeipUsWanIntfCf(&pVeipEntry->rule, vid, pri,
                            0, &pVeipEntry->defaultPathCfIdx, pVeipEntry->usStreamId, TRUE);
                    if (OMCI_ERR_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create cf rule for b-wan default path fail, ret = %d", ret);

                        // rollback
                        if(pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
                        {
                            if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx])) != RT_ERR_OK)
                                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Delete CF index %d fail, ret = %d", pVeipEntry->pHwPathCfIdx[wanIdx], ret);
                        }

                        goto remove_def_path_cf;
                    }
                }
            }
        }

        // for route mode and turn off HWNAT
        ret = omcidrv_setVeipUsWanVlanCf(&pVeipEntry->rule, vid, pri,
                                         type, pVeipEntry->pUsSwPathCfIdx[wanIdx],
                                         pVeipEntry->usStreamId, &gPonWanEntry[OMCI_PON_WAN_RULE_POS_PATCH_DPI], &rgCfEntry);
        if (OMCI_ERR_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "create us sw cf for r-wan fail, ret = %d", ret);

            // rollback
            if(pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
            {
                if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx])) != RT_ERR_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Rollback and Delete CF index %d fail, ret = %d", pVeipEntry->pHwPathCfIdx[wanIdx], ret);
            }

            goto remove_def_path_cf;
        }


        // in case omci translate vlans between wan interface and ani
        if (pVeipEntry->rule.outStyle.outVlan.vid != vid && bFound &&
                pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
        {
            // only create ds vlan translation cf for non-mac-based cvid decision
            if (!gPlatformDb.rgMacBasedTag &&
                    OMCIDRV_FEATURE_ERR_FAIL == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
            {
                // create ds vlan translation cf
                ret = omcidrv_setVeipDsWanVlanCf(&pVeipEntry->rule,
                        vid, pri, type, pVeipEntry->pSwPathCfIdx[wanIdx], pVeipEntry->dsStreamId);
                if (OMCI_ERR_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ds vlan translation cf for b-wan fail, ret = %d", ret);

                    // rollback
                    if (pVeipEntry->pHwPathCfIdx[wanIdx] != OMCI_UNUSED_CF)
                    {
                        if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->pHwPathCfIdx[wanIdx])) != RT_ERR_OK)
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Rollback and Delete CF index %d fail, ret = %d", pVeipEntry->pHwPathCfIdx[wanIdx], ret);
                    }
                    if (pVeipEntry->defaultPathCfIdx != OMCI_UNUSED_CF)
                    {
                        if((ret = rtk_rg_aclFilterAndQos_del(pVeipEntry->defaultPathCfIdx)) != RT_ERR_OK)
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Rollback and Delete CF index %d fail, ret = %d", pVeipEntry->defaultPathCfIdx, ret);
                    }

                    goto remove_def_path_cf;
                }
            }

            // set cvlan table
            if (!gPlatformDb.rgIvlMcastSupport &&
                    pVeipEntry->rule.outStyle.outVlan.vid <= 4095)
            {
                memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                rgCvlan.vlanId = pVeipEntry->rule.outStyle.outVlan.vid;
                rgCvlan.memberPortMask.portmask =
                    (gPlatformDb.uniPortMask | (1 << gPlatformDb.ponPort) | gPlatformDb.extPortMask);
                if (vid < 0 ||
                        OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
                    rgCvlan.untagPortMask.portmask = gPlatformDb.uniPortMask;

                ret = rtk_rg_cvlan_add(&rgCvlan);
                if (RT_ERR_RG_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create ani vlan %u for veip fail, ret = %d",
                        pVeipEntry->rule.outStyle.outVlan.vid, ret);
                }
            }
        }

        return OMCI_ERR_OK;
    }

remove_def_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->defaultPathCfIdx);
    pVeipEntry->defaultPathCfIdx = OMCI_UNUSED_CF;

remove_hw_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->pHwPathCfIdx[wanIdx]);
    pVeipEntry->pHwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

remove_us_sw_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->pUsSwPathCfIdx[wanIdx]);
    pVeipEntry->pUsSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

remove_sw_path_cf:
    _RemoveUsedCfIndex(pVeipEntry->pSwPathCfIdx[wanIdx]);
    pVeipEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;

    return ret;
}

static int
omcidrv_delVeipRule(unsigned int servId, int wanIdx)
{
    int             ret;
    veip_service_t  *pEntry;
    int                 vid, pri, type, service, isBinding, netIfIdx;
    unsigned char       isRuleCfg;

    pEntry = veipServ_entry_find(servId);
    if (!pEntry)
        return OMCI_ERR_FAILED;

    if (gDrvCtrl.dmMode && (!pDmmCb || !pDmmCb->omci_dm_pon_wan_info_del))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "dmm callback is not registered");

        return OMCI_ERR_FAILED;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
        "del veip rule%sfor wan %d", gDrvCtrl.dmMode ? " by dmm " : " ", wanIdx);

    /*delete ani cvlan when vlan trnslate between wan interface and ani*/
    omcidrv_getWanInfoByIfIdx(wanIdx, &vid, &pri, &type, &service, &isBinding, &netIfIdx, &isRuleCfg);

    if (pEntry->rule.outStyle.outVlan.vid != vid &&
            !gPlatformDb.rgIvlMcastSupport &&
            pEntry->rule.outStyle.outVlan.vid <= 4095)
    {
        rg_cvlan_del(pEntry->rule.outStyle.outVlan.vid);
    }

    // deallocate cf for default path
    if (OMCI_UNUSED_CF != pEntry->defaultPathCfIdx)
    {
        ret = rtk_rg_aclFilterAndQos_del(pEntry->defaultPathCfIdx);
        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "delete veip cf rule %u fail, ret = %d", pEntry->defaultPathCfIdx, ret);

            return OMCI_ERR_FAILED;
        }
        else
        {
            _RemoveUsedCfIndex(pEntry->defaultPathCfIdx);
            pEntry->defaultPathCfIdx = OMCI_UNUSED_CF;
        }
    }

    // deallocate cf for hw path
    if (OMCI_UNUSED_CF != pEntry->pHwPathCfIdx[wanIdx])
    {
        ret = rtk_rg_aclFilterAndQos_del(pEntry->pHwPathCfIdx[wanIdx]);
        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
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
    if (OMCI_UNUSED_CF != pEntry->pUsSwPathCfIdx[wanIdx])
    {
        ret = rtk_rg_classifyEntry_del(pEntry->pUsSwPathCfIdx[wanIdx]);

        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "delete veip cf rule %u fail, ret = %d", pEntry->pUsSwPathCfIdx[wanIdx], ret);

            return OMCI_ERR_FAILED;
        }
        else
        {
            _RemoveUsedCfIndex(pEntry->pUsSwPathCfIdx[wanIdx]);
            pEntry->pUsSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;
        }
    }

    // deallocate cf for sw path
    if (OMCI_UNUSED_CF != pEntry->pSwPathCfIdx[wanIdx])
    {
        if (pEntry->pSwPathCfIdx[wanIdx] >= gPlatformDb.veipFastStart &&
                pEntry->pSwPathCfIdx[wanIdx] <= gPlatformDb.veipFastStop)
            ret = rtk_rg_aclFilterAndQos_del(pEntry->pSwPathCfIdx[wanIdx]);
        else
            ret = rtk_rg_classifyEntry_del(pEntry->pSwPathCfIdx[wanIdx]);

        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "delete veip cf rule %u fail, ret = %d", pEntry->pSwPathCfIdx[wanIdx], ret);

            return OMCI_ERR_FAILED;
        }
        else
        {
            _RemoveUsedCfIndex(pEntry->pSwPathCfIdx[wanIdx]);
            pEntry->pSwPathCfIdx[wanIdx] = OMCI_UNUSED_CF;
        }
    }

    if (gDrvCtrl.dmMode)
    {
        ret = pDmmCb->omci_dm_pon_wan_info_del(wanIdx);
        if (0 != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "del veip rule by dmm for wan %d fail, ret = %d", wanIdx, ret);

            return OMCI_ERR_FAILED;
        }

#if 1// TBD, refine
        if (OMCI_ERR_OK != omci_dmm_pon_wan_pf_info_entry_del(wanIdx))
        {

            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "restore pon_wan pf_info etnry failed");

            return OMCI_ERR_FAILED;
        }
#endif
    }

    return OMCI_ERR_OK;
}

static int
omcidrv_matchVeipRule(
    OMCI_VLAN_OPER_ts*  pVlanRule,
    int                 vid,
    int                 pri,
    int                 type)
{
    if (!pVlanRule)
    {
        OMCI_LOG (
            OMCI_LOG_LEVEL_ERR,
            "%s: pVlanRule is NULL\n",
            __FUNCTION__);
        return OMCI_ERR_PARAM;
    }

    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_UNTAG:
            // vid should be < 0 for untag packets
            if (vid >= 0) {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: vid %u is greater than Zero\n",
                    __FUNCTION__,
                    vid);
                return OMCI_ERR_FAILED;
            }
            break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
            // vid should be >= 0 for tagged packets
            if (vid < 0) {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: vid %u is smaller than Zero\n",
                    __FUNCTION__,
                    vid);
                return OMCI_ERR_FAILED;
            }
            break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            // vid should be >= 0 for tagged packets
            if (vid < 0)
            {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: vid %u is smaller than Zero\n",
                    __FUNCTION__,
                    vid);
                return OMCI_ERR_FAILED;
            }

            // pri should be match if it's enabled
            if (pri < 0 || pri != pVlanRule->filterRule.filterCTag.pri)
            {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: pri %u isn't matched with pri %u of pVlanRule\n",
                    __FUNCTION__,
                    pri,
                    pVlanRule->filterRule.filterCTag.pri);
                return OMCI_ERR_FAILED;
            }
            break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
            // vid should be match if it's enabled
            if (vid != pVlanRule->filterRule.filterCTag.vid)
            {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: vid %u isn't matched with vid %u of pVlanRule\n",
                    __FUNCTION__,
                    vid,
                    pVlanRule->filterRule.filterCTag.vid);
                return OMCI_ERR_FAILED;
            }

            // pri should be match if it's enabled
            if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI &&
                    pri < 0 && pri != pVlanRule->filterRule.filterCTag.pri)
            {
                OMCI_LOG (
                    OMCI_LOG_LEVEL_WARN,
                    "%s: pri %u isn't matched with pri %u of pVlanRule\n",
                    __FUNCTION__,
                    pri,
                    pVlanRule->filterRule.filterCTag.pri);
                return OMCI_ERR_FAILED;
            }
            break;
        case VLAN_OPER_MODE_VLANTAG_OPER:
            if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_NO_TAG)
            {
                // vid should be < 0 for untag packets
                if (vid >= 0)
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: vid %u is greater than Zero\n",
                        __FUNCTION__,
                        vid);
                    return OMCI_ERR_FAILED;
                }
            }
            else
            {
                // vid should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_VID) &&
                        vid != pVlanRule->filterRule.filterCTag.vid)
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: vid %u isn't matched with vid %u of pVlanRule\n",
                        __FUNCTION__,
                        vid,
                        pVlanRule->filterRule.filterCTag.vid);
                    return OMCI_ERR_FAILED;
                }

                // pri should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI) &&
                        pri >= 0 && pri != pVlanRule->filterRule.filterCTag.pri)
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: pri %u isn't matched with pri %u of pVlanRule\n",
                        __FUNCTION__,
                        pri,
                        pVlanRule->filterRule.filterCTag.pri);
                    return OMCI_ERR_FAILED;
                }
            }
            break;
        case VLAN_OPER_MODE_EXTVLAN:
            // ignore double-tagged entries by filter-out stag mode
            if (!(pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG))
                return OMCI_ERR_FAILED;

            if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_NO_TAG)
            {
                // vid should be < 0 for untag packets
                if (vid >= 0)
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: vid %u is greater than Zero\n",
                        __FUNCTION__,
                        vid);
                    return OMCI_ERR_FAILED;
                }
            }
            else
            {
                // vid should be >= 0 for tagged packets
                if (vid < 0)
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: vid %u is smaller than Zero\n",
                        __FUNCTION__,
                        vid);
                    return OMCI_ERR_FAILED;
                }

                // vid should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_VID) &&
                        (vid < 0 || vid != pVlanRule->filterRule.filterCTag.vid))
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: vid %u isn't matched with vid %u of pVlanRule\n",
                        __FUNCTION__,
                        vid,
                        pVlanRule->filterRule.filterCTag.vid);
                    return OMCI_ERR_FAILED;
                }

                // pri should be match if it's enabled
                if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI ||
                        pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_PRI) &&
                        (pri < 0 || pri != pVlanRule->filterRule.filterCTag.pri))
                {
                    OMCI_LOG (
                        OMCI_LOG_LEVEL_WARN,
                        "%s: pri %u isn't matched with pri %u of pVlanRule\n",
                        __FUNCTION__,
                        pri,
                        pVlanRule->filterRule.filterCTag.pri);
                    return OMCI_ERR_FAILED;
                }

                // ethertype should be match if it's enabled
                if (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_ETHTYPE)
                {
                    // check if it's pppoe
                    if ((ETHTYPE_FILTER_PPPOE == pVlanRule->filterRule.etherType ||
                            ETHTYPE_FILTER_PPPOE_S == pVlanRule->filterRule.etherType) &&
                            (OMCI_MODE_PPPOE != type && OMCI_MODE_PPPOE_V4NAPT_V6 != type))
                    {
                        OMCI_LOG (
                            OMCI_LOG_LEVEL_WARN,
                            "%s: Ethertype is pppoe but type of wanIf isn't\n",
                            __FUNCTION__);
                        return OMCI_ERR_FAILED;
                    }

                    // check if it's ipoe 'cause
                    //  ethertype filter only applies to route wan
                    if (OMCI_MODE_IPOE != type && OMCI_MODE_IPOE_V4NAPT_V6 != type)
                    {
                        OMCI_LOG (
                            OMCI_LOG_LEVEL_WARN,
                            "%s: wanIf isn't IPOE and IPOE_V4NAPT_V6\n",
                            __FUNCTION__);
                        return OMCI_ERR_FAILED;
                    }
                }
            }
            break;
        default:
            break;
    }

    return OMCI_ERR_OK;
}

static int omcidrv_set_ds_tag_operation_filter_by_evlan(
        OMCI_VLAN_FILTER_ts *pFilter, rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t *pTagOp)
{
    if (!pFilter || !pTagOp)
        return OMCI_ERR_FAILED;

    // set stag filter
    pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_STAGIf_BIT;
    if (pFilter->filterStagMode & VLAN_FILTER_NO_TAG)
        pTagOp->ingress_stagIf = 0;
    else
        pTagOp->ingress_stagIf = 1;

    if (pFilter->filterStagMode & VLAN_FILTER_VID)
    {
        pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_SVID_BIT;
        pTagOp->ingress_stag_svid = pFilter->filterSTag.vid;
    }
    if (pFilter->filterStagMode & VLAN_FILTER_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri is not supported\n");
    }
    if (pFilter->filterStagMode & VLAN_FILTER_TCI)
    {
        pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_SVID_BIT;
        pTagOp->ingress_stag_svid = pFilter->filterSTag.vid;

        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter TCI is not supported\n");
    }
    if (pFilter->filterStagMode & VLAN_FILTER_ETHTYPE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter ethertype is not supported\n");
    }
    if (pFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri from dscp is not supported\n");
    }

    // set ctag filter
    pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT;
    if (pFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
        pTagOp->ingress_ctagIf = 0;
    else
        pTagOp->ingress_ctagIf = 1;

    if (pFilter->filterCtagMode & VLAN_FILTER_VID)
    {
        pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_CVID_BIT;
        pTagOp->ingress_ctag_cvid = pFilter->filterCTag.vid;
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri is not supported\n");
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_TCI)
    {
        pTagOp->filter_fields |= GPON_DS_BC_FILTER_INGRESS_CVID_BIT;
        pTagOp->ingress_ctag_cvid = pFilter->filterCTag.vid;

        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter TCI is not supported\n");
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter ethertype is not supported\n");
    }
    if (pFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri from dscp is not supported\n");
    }

    return OMCI_ERR_OK;
}

static void omcidrv_set_ds_tag_operation_treatment_tagmode(
        OMCI_VLAN_ACT_MODE_e vlanAct, rtk_rg_gpon_ds_bc_tag_decision_t *pTagMode)
{
    if (!pTagMode)
        return;

    switch (vlanAct)
    {
        case VLAN_ACT_REMOVE:
            *pTagMode = RTK_RG_GPON_BC_FORCE_UNATG;
            break;
        case VLAN_ACT_TRANSPARENT:
            if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
            {
                *pTagMode = RTK_RG_GPON_BC_FORCE_UNATG;
            } else {
                *pTagMode = RTK_RG_GPON_BC_FORCE_TRANSPARENT;
            }
            break;
        default:
            *pTagMode = RTK_RG_GPON_BC_FORCE_TAGGIN_WITH_CVID;
            break;
    }
}

static int omcidrv_ds_tag_operation_rule_add(OMCI_BRIDGE_RULE_ts *pDsRule)
{
    OMCI_VLAN_OPER_ts                           *pVlanRule;
    OMCI_VLAN_FILTER_ts                         *pFilter;
    OMCI_VLAN_ACT_ts                            *pSTagAct;
    OMCI_VLAN_ACT_ts                            *pCTagAct;
    rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t  tagOp;
    int32                                       tagOpIdx;
    int32                                       ret;

    if (!pDsRule)
        return OMCI_ERR_FAILED;

    pVlanRule = &pDsRule->vlanRule;
    pFilter = &pVlanRule->filterRule;
    pSTagAct = &pVlanRule->sTagAct;
    pCTagAct = &pVlanRule->cTagAct;
    memset(&tagOp, 0, sizeof(tagOp));
    tagOpIdx = MAX_GPON_DS_BC_FILTER_SW_ENTRY_SIZE - pDsRule->servId - 1;

    // skip entries that is destined to drop
    if (OMCI_EXTVLAN_REMOVE_TAG_DISCARD == pVlanRule->outStyle.isDefaultRule)
        return OMCI_ERR_FAILED;

    // set filter
    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_ALL:
            break;
        case VLAN_OPER_MODE_FORWARD_UNTAG:
            tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_STAGIf_BIT;
            tagOp.ingress_stagIf = 0;
            tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT;
            tagOp.ingress_ctagIf = 0;
            break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
            tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT;
            tagOp.ingress_ctagIf = 1;
            break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT;
            tagOp.ingress_ctagIf = 1;
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri is not supported\n");
            }
            break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
        case VLAN_OPER_MODE_VLANTAG_OPER:
            tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_CTAGIf_BIT;
            tagOp.ingress_ctagIf = 1;
            if (pFilter->filterCtagMode & VLAN_FILTER_VID ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_CVID_BIT;
                tagOp.ingress_ctag_cvid =  pFilter->filterCTag.vid;
            }
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "filter pri is not supported\n");
            }
            break;
        case VLAN_OPER_MODE_EXTVLAN:
            ret = omcidrv_set_ds_tag_operation_filter_by_evlan(pFilter, &tagOp);
            if (OMCI_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "set ds tag operation filter by evlan fail, ret = %d", ret);
            }
            break;
        default:
            break;
    }

    // set treatment of tag mode
    omcidrv_set_ds_tag_operation_treatment_tagmode(
        pCTagAct->vlanAct, &tagOp.ctag_action.ctag_decision);

    // set treatment of tag field
    tagOp.ctag_action.assigned_ctag_cvid = pCTagAct->assignVlan.vid;
    tagOp.ctag_action.assigned_ctag_cpri = pCTagAct->assignVlan.pri;

    // set dst port mask
    tagOp.filter_fields |= GPON_DS_BC_FILTER_EGRESS_PORT_BIT;
    if (pDsRule->uniMask & (1 << gPlatformDb.ponPort))
        tagOp.egress_portmask.portmask = (gPlatformDb.uniPortMask | gPlatformDb.extPortMask);
    else
        tagOp.egress_portmask.portmask = pDsRule->uniMask;

    // set ds gem flow id
    tagOp.filter_fields |= GPON_DS_BC_FILTER_INGRESS_STREAMID_BIT;
    tagOp.ingress_stream_id = pDsRule->dsFlowId;

    ret = rtk_rg_gponDsBcFilterAndRemarking_add(&tagOp, &tagOpIdx);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "add ds tag operation fail, ret = %d", ret);
    }

    return ret;
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
                if (pVlanRule->filterRule.filterStagMode & VLAN_FILTER_CARE_TAG)
                {
                    memcpy(&pVlanRule->cTagAct.assignVlan, &pVlanRule->filterRule.filterSTag, sizeof(OMCI_VLAN_ts));
                    pVlanRule->cTagAct.vlanAct = VLAN_ACT_MODIFY;
                    pVlanRule->cTagAct.vidAct = VID_ACT_COPY_OUTER;
                    pVlanRule->cTagAct.priAct = PRI_ACT_COPY_OUTER;

                }
                else
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
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rg_svlanServicePort_get(port, &enableB));
            if ((1 << port) & uniPortMask)
                enableB = ENABLED;
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rg_svlanServicePort_set(port, enableB));
        }
    }

    return OMCI_ERR_OK;

}


static int omcidrv_updateMcBcRuleByDmm(OMCI_BRIDGE_RULE_ts *pDsRule)
{
    OMCI_VLAN_OPER_ts                           *pVlanRule;
    OMCI_VLAN_FILTER_ts                         *pFilter;
    OMCI_VLAN_ACT_ts                            *pSTagAct;
    OMCI_VLAN_ACT_ts                            *pCTagAct;
    omci_dm_ds_bc_mc_info_t                 ds_bc_mc_info;
    int32                                       ret;

    if (!pDsRule)
        return OMCI_ERR_FAILED;

    // delete old rule
    ret = pDmmCb->omci_dm_ds_mc_bc_info_del(pDsRule->servId);
    if (0 != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "del mc bc rule by dmm fail, ret = %d", ret);

        //return OMCI_ERR_FAILED;
    }

    pVlanRule = &pDsRule->vlanRule;
    pFilter = &pVlanRule->filterRule;
    pSTagAct = &pVlanRule->sTagAct;
    pCTagAct = &pVlanRule->cTagAct;

    // skip entries that is destined to drop
    if (OMCI_EXTVLAN_REMOVE_TAG_DISCARD == pVlanRule->outStyle.isDefaultRule)
        return OMCI_ERR_FAILED;

    //
    // add new rule
    //
    memset(&ds_bc_mc_info, 0, sizeof(omci_dm_ds_bc_mc_info_t));

    ds_bc_mc_info.relatedId = pDsRule->servId;
    ds_bc_mc_info.isMcRule = pDsRule->vlanRule.outStyle.isMCRule;

    // set ds gem flow id
    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_STREAMID_BIT;
    ds_bc_mc_info.filterRule.streamID = pDsRule->dsFlowId;

    // set dst port mask
    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_EGRESS_PORT_BIT;
    if (pDsRule->uniMask & (1 << gPlatformDb.ponPort))
        ds_bc_mc_info.filterRule.egressPortMask = (gPlatformDb.uniPortMask | gPlatformDb.extPortMask);
    else
        ds_bc_mc_info.filterRule.egressPortMask = pDsRule->uniMask;

    /* set filter */
    switch (pVlanRule->filterMode)
    {
        case VLAN_OPER_MODE_FORWARD_ALL:
            break;
        case VLAN_OPER_MODE_FORWARD_UNTAG:
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_OUTER_TAGIf_BIT;
            ds_bc_mc_info.filterRule.outerTagIf = 0;
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_TAGIf_BIT;
            ds_bc_mc_info.filterRule.innerTagIf = 0;
            break;
        case VLAN_OPER_MODE_FORWARD_SINGLETAG:
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_TAGIf_BIT;
            ds_bc_mc_info.filterRule.innerTagIf = 1;
            break;
        case VLAN_OPER_MODE_FILTER_INNER_PRI:
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_TAGIf_BIT;
            ds_bc_mc_info.filterRule.innerTagIf = 1;
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_PRI_BIT;
                ds_bc_mc_info.filterRule.innerTagPri = pFilter->filterCTag.pri;
            }
            break;
        case VLAN_OPER_MODE_FILTER_SINGLETAG:
        case VLAN_OPER_MODE_VLANTAG_OPER:
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_TAGIf_BIT;
            ds_bc_mc_info.filterRule.innerTagIf = 1;
            if (pFilter->filterCtagMode & VLAN_FILTER_VID ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_VID_BIT;
                ds_bc_mc_info.filterRule.innerTagVid = pFilter->filterCTag.vid;
            }
            if (pFilter->filterCtagMode & VLAN_FILTER_PRI ||
                    pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_PRI_BIT;
                ds_bc_mc_info.filterRule.innerTagPri = pFilter->filterCTag.pri;
            }
            break;
        case VLAN_OPER_MODE_EXTVLAN:
        {
            //
            // set stag filter
            //
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_OUTER_TAGIf_BIT;

            if (pFilter->filterStagMode & VLAN_FILTER_NO_TAG)
                ds_bc_mc_info.filterRule.outerTagIf = 0;
            else
                ds_bc_mc_info.filterRule.outerTagIf = 1;

            if (pFilter->filterStagMode & VLAN_FILTER_VID)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_OUTER_VID_BIT;
                ds_bc_mc_info.filterRule.outerTagVid = pFilter->filterSTag.vid;
            }

            if (pFilter->filterStagMode & VLAN_FILTER_PRI)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_OUTER_PRI_BIT;
                ds_bc_mc_info.filterRule.outerTagPri = pFilter->filterSTag.pri;
            }

            if (pFilter->filterStagMode & VLAN_FILTER_TCI)
            {
                ds_bc_mc_info.filterRule.filterMask |=
                    (OMCI_DM_FILTER_OUTER_VID_BIT | OMCI_DM_FILTER_OUTER_PRI_BIT);
                ds_bc_mc_info.filterRule.outerTagVid = pFilter->filterSTag.vid;
                ds_bc_mc_info.filterRule.outerTagPri = pFilter->filterSTag.pri;
            }

            if (pFilter->filterStagMode & VLAN_FILTER_ETHTYPE)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                ds_bc_mc_info.filterRule.etherType = pFilter->etherType;
            }

            //
            // TBD: pFilter->filterStagMode & VLAN_FILTER_DSCP_PRI)
            //

            //
            // set ctag filter
            //
            ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_TAGIf_BIT;
            if (pFilter->filterCtagMode & VLAN_FILTER_NO_TAG)
                ds_bc_mc_info.filterRule.innerTagIf = 0;
            else
                ds_bc_mc_info.filterRule.innerTagIf = 1;

            if (pFilter->filterCtagMode & VLAN_FILTER_VID)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_VID_BIT;
                ds_bc_mc_info.filterRule.innerTagVid = pFilter->filterCTag.vid;
            }

            if (pFilter->filterCtagMode & VLAN_FILTER_PRI)
            {
                ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_INNER_PRI_BIT;
                ds_bc_mc_info.filterRule.innerTagPri = pFilter->filterCTag.pri;
            }

            if (pFilter->filterCtagMode & VLAN_FILTER_TCI)
            {
                ds_bc_mc_info.filterRule.filterMask |=
                    (OMCI_DM_FILTER_INNER_VID_BIT | OMCI_DM_FILTER_INNER_PRI_BIT);
                ds_bc_mc_info.filterRule.innerTagVid = pFilter->filterCTag.vid;
                ds_bc_mc_info.filterRule.innerTagPri = pFilter->filterCTag.pri;
            }
            if (pFilter->filterCtagMode & VLAN_FILTER_ETHTYPE)
            {

                if (ETHTYPE_FILTER_IP == pFilter->etherType)
                {
                    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                    ds_bc_mc_info.filterRule.etherType = 0x0800;
                }

                if (ETHTYPE_FILTER_PPPOE== pFilter->etherType)
                {
                    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                    ds_bc_mc_info.filterRule.etherType = 0x8863;
                }


                if (ETHTYPE_FILTER_ARP == pFilter->etherType)
                {
                    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                    ds_bc_mc_info.filterRule.etherType = 0x0806;
                }

                if (ETHTYPE_FILTER_PPPOE_S== pFilter->etherType)
                {
                    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                    ds_bc_mc_info.filterRule.etherType = 0x8864;
                }

                if (ETHTYPE_FILTER_IPV6 == pFilter->etherType)
                {
                    ds_bc_mc_info.filterRule.filterMask |= OMCI_DM_FILTER_ETHERTYPE;
                    ds_bc_mc_info.filterRule.etherType = 0x86dd;
                }
            }
            //
            // TBD: pFilter->filterCtagMode & VLAN_FILTER_DSCP_PRI
            //
            break;
        }
        default:
            break;
    }
    //
    // set treatment of tag mode
    //
    switch (pSTagAct->vlanAct)
    {
        case VLAN_ACT_REMOVE:
            ds_bc_mc_info.outerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_REMOVE_1ST_TAG;
            break;
        case VLAN_ACT_TRANSPARENT:
        case VLAN_ACT_NON:
            if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
            {
                ds_bc_mc_info.outerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_REMOVE_1ST_TAG;
            }
            else
            {
                ds_bc_mc_info.outerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_TRANSPARENT;
            }
            break;
        default:
            ds_bc_mc_info.outerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_TAGGING;
            break;
    }

    ds_bc_mc_info.outerTagAct.assignedVid = pSTagAct->assignVlan.vid;
    ds_bc_mc_info.outerTagAct.assignedPri = pSTagAct->assignVlan.pri;

    switch ( pCTagAct->vlanAct)
    {
        case VLAN_ACT_REMOVE:
            ds_bc_mc_info.innerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_REMOVE_2ND_TAG;
            break;
        case VLAN_ACT_TRANSPARENT:
            if (OMCIDRV_FEATURE_ERR_OK == omcidrv_feature_api(FEATURE_KAPI_RDP_00000002))
            {
                ds_bc_mc_info.innerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_REMOVE_2ND_TAG;
            }
            else
            {
                ds_bc_mc_info.innerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_TRANSPARENT;
            }
            break;
        default:
            ds_bc_mc_info.innerTagAct.tagAction = OMCI_PON_WAN_VLAN_TAGIF_TAGGING;
            break;
    }

    // set treatment of tag field
    ds_bc_mc_info.innerTagAct.assignedVid = pCTagAct->assignVlan.vid;
    ds_bc_mc_info.innerTagAct.assignedPri = pCTagAct->assignVlan.pri;


    ret =  pDmmCb->omci_dm_ds_mc_bc_info_set(&ds_bc_mc_info);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "add ds mc bc fail, ret = %d", ret);
    }

    return ret;
}


static int
omcidrv_updateMcastBcastRule ( mbcast_service_t* pMBEntry,
    OMCI_BRIDGE_RULE_ts* pBridgeRule )
{
    rtk_rg_classifyEntry_t      rgCfEntry;
    rtk_rg_aclFilterAndQos_t    rgAclEntry;
    int                         rgAclEntryIdx;
    int                         ret = OMCI_ERR_OK;
    rtk_rg_cvlan_info_t         rgCvlan;
    rtk_rg_gpon_ds_bc_vlanfilterAndRemarking_t  tagOp;
    int32                                       tagOpIdx;
    omci_rule_pri_t             rule_pri;
    unsigned int                systemTpid;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
    /*Get SVLAN TPID*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_svlanTpid_get(&systemTpid));

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
    if (pMBEntry->uniMask != pBridgeRule->uniMask)
    {
        if (pDmmCb && (pDmmCb->omci_dm_ds_mc_bc_info_del && pDmmCb->omci_dm_ds_mc_bc_info_set))
        {
            ret = omcidrv_updateMcBcRuleByDmm(pBridgeRule);
            printk("%s() %d, ret=%d\n", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        // delete before add entry for ds pkt bcaster
        tagOpIdx = MAX_GPON_DS_BC_FILTER_SW_ENTRY_SIZE - pBridgeRule->servId - 1;
        if (RT_ERR_RG_OK == rtk_rg_gponDsBcFilterAndRemarking_find(&tagOpIdx, &tagOp))
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_gponDsBcFilterAndRemarking_del(tagOpIdx));

        // invoke ds pkt bcaster
        omcidrv_ds_tag_operation_rule_add(pBridgeRule);

        // TBD: filtering double tag
        /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
        /* Setup ACL rule before create CF rule
        omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_DS, &(pBridgeRule->vlanRule), pBridgeRule->uniMask, pBridgeRule->dsFlowId, &dsAclIndex);*/

        //TBD check c-tag filter vlan should be created if vlan table does not found
        if (pMBEntry->rule.filterRule.filterCtagMode & VLAN_FILTER_VID ||
            pMBEntry->rule.filterRule.filterCtagMode & VLAN_FILTER_TCI)
        {
            if (!(pBridgeRule->uniMask & (1 << gPlatformDb.ponPort)) ||
                !gPlatformDb.rgIvlMcastSupport)
            {
                if (pMBEntry->rule.filterRule.filterCTag.vid != gPlatformDb.fwdVLAN_CPU)
                {
                    memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                    rgCvlan.vlanId = pMBEntry->rule.filterRule.filterCTag.vid;

                    // get entry if it exist
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_cvlan_get(&rgCvlan));

                    rgCvlan.memberPortMask.portmask =
                        ((1 << gPlatformDb.ponPort) | gPlatformDb.uniPortMask | gPlatformDb.extPortMask);

                    ret = rtk_rg_cvlan_add(&rgCvlan);
                    if (RT_ERR_RG_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create ani vlan for mbcast fail, ret = %d", ret);
                    }
                }
            }
        }

        /* Due to RG implementation cause
           ctag action cannot follow CF rule of 64-511 range if separate acl and classify rule in 6266.
           Therefore, the rule configuration is different according to each chip.
         */
        if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
        {
            if (TRUE == pBridgeRule->isLatch)
            {
                // acl latch to CF
                if ((pMBEntry->isCfg == 1) && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                {
                    // update
                    if (RT_ERR_RG_OK != (ret = rtk_rg_aclFilterAndQos_del(pMBEntry->dsCfIndex)))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete cf fail, ret = %d", ret);

                        return OMCI_ERR_FAILED;
                    }
                }
                //
                // set cf content
                //
                if (OMCI_ERR_OK != (ret = omcidrv_setAclLatchCfContent(pBridgeRule, &rgAclEntry)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create content for acl latch cf content fail, ret = %d", ret);

                    if (0 == pMBEntry->isCfg && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

                    return OMCI_ERR_FAILED;
                }
                // invoke driver api
                if (RT_ERR_RG_OK != (ret = rtk_rg_aclFilterAndQos_add(&rgAclEntry, &rgAclEntryIdx)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create cf rule of multicast fail, ret = %d", ret);

                    if (0 == pMBEntry->isCfg && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

                    return OMCI_ERR_FAILED;
                }
                else
                {
                    if (pMBEntry->isCfg == 1 && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                    {
                        // release original used cf index
                        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);
                        gPlatformDb.cfRule[rgAclEntryIdx].isCfg = 1;

                    }
                    pMBEntry->dsCfIndex = rgAclEntryIdx;
                    gPlatformDb.cfRule[rgAclEntryIdx].isCfg = 1;
                }
            }
            else
            {
                memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));

                if (pBridgeRule->uniMask & (1 << gPlatformDb.ponPort)) {
                    omci_SetClassifyDsAct(&pBridgeRule->vlanRule, gPlatformDb.uniPortMask, &rgCfEntry);
                }
                else {
                    omci_SetClassifyDsAct(&pBridgeRule->vlanRule, pBridgeRule->uniMask, &rgCfEntry);
                }

                omci_SetClassifyDsRule(&pBridgeRule->vlanRule, &pBridgeRule->vlanRule, pBridgeRule->dsFlowId, &rgCfEntry, &rule_pri);

                if ((pMBEntry->isCfg != 1) || (pMBEntry->dsCfIndex == OMCI_UNUSED_CF))
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pMBEntry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "allocate ds cf type %d for mcast/bcast fail!", PF_CF_TYPE_L2_COMM);

                        return OMCI_ERR_FAILED;
                    }
                }
                else
                {
                    // update
                    if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_del(pMBEntry->dsCfIndex)))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete cf fail, ret = %d", ret);

                        return OMCI_ERR_FAILED;
                    }
                }

                rgCfEntry.index = pMBEntry->dsCfIndex;
                rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM;

                if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_add(&rgCfEntry)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "add cf ds rule %d failed, ret=%d", pMBEntry->dsCfIndex, ret);

                    rg_cf_show(&rgCfEntry);

                    if (0 == pMBEntry->isCfg && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

                    return OMCI_ERR_FAILED;
                }
                _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);
            }
        }
        else
        {
            memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));

            if (pBridgeRule->uniMask & (1 << gPlatformDb.ponPort))
                omci_SetClassifyDsAct(&pBridgeRule->vlanRule, gPlatformDb.uniPortMask, &rgCfEntry);
            else
                omci_SetClassifyDsAct(&pBridgeRule->vlanRule, pBridgeRule->uniMask, &rgCfEntry);

            if (TRUE == pBridgeRule->isLatch &&
                1 == pBridgeRule->vlanRule.filterRule.filterCTag.vid)
            {
                rgCfEntry.gemidx = pBridgeRule->dsFlowId;
                rgCfEntry.gemidx_mask = gPlatformDb.sidMask;
                rgCfEntry.filter_fields |= EGRESS_GEMIDX_BIT;
            }
            else
            {
                omci_SetClassifyDsRule(&pBridgeRule->vlanRule, &pBridgeRule->vlanRule, pBridgeRule->dsFlowId, &rgCfEntry, &rule_pri);
            }

            if ((pMBEntry->isCfg != 1) || (pMBEntry->dsCfIndex == OMCI_UNUSED_CF))
            {
                // TBD: us rg acl filter entry if filtering double table
                unsigned int type = (VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode) ?
                                    PF_CF_TYPE_L2_ETH_FILTER : PF_CF_TYPE_L2_COMM;
                if (OMCI_ERR_OK !=
                        _AssignNonUsedCfIndex(type, rule_pri, &pMBEntry->dsCfIndex))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "allocate ds cf type %d for mcast/bcast fail!", PF_CF_TYPE_L2_COMM);

                    return OMCI_ERR_FAILED;
                }
            }
            else
            {
                // update it and use the same cf index and refer acl inddex
                if (pMBEntry->referAclIdx != OMCI_UNUSED_ACL &&
                    RT_ERR_RG_OK != (ret = rtk_rg_aclFilterAndQos_del(pMBEntry->referAclIdx)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create acl delete for multicast service fail, ret = %d", ret);
                    return OMCI_ERR_FAILED;
                }

                if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_del(pMBEntry->dsCfIndex)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create cf delete  for multicast service fail, ret = %d", ret);
                    return OMCI_ERR_FAILED;
                }
            }

            if (TRUE == pBridgeRule->isLatch &&
                1 == pBridgeRule->vlanRule.filterRule.filterCTag.vid)
            {
                memset(&rgAclEntry, 0, sizeof(rtk_rg_aclFilterAndQos_t));
                rgAclEntry.fwding_type_and_direction    = ACL_FWD_TYPE_DIR_INGRESS_ALL_PACKET;
                rgAclEntry.filter_fields                = INGRESS_STREAM_ID_BIT | INGRESS_PORT_BIT;
                rgAclEntry.ingress_port_mask.portmask   = (1 << gPlatformDb.ponPort);
                rgAclEntry.ingress_stream_id            = pBridgeRule->dsFlowId;
                rgAclEntry.action_type                  = ACL_ACTION_TYPE_QOS;
                rgAclEntry.qos_actions                  = ACL_ACTION_ACL_INGRESS_VID_BIT;
                rgAclEntry.action_acl_ingress_vid       = pBridgeRule->vlanRule.filterRule.filterCTag.vid;

                if (RT_ERR_RG_OK != (ret = rtk_rg_aclFilterAndQos_add(&rgAclEntry, &pMBEntry->referAclIdx)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "create acl rule for multicast ingress vlan translation fail, ret = %d", ret);

                    //restore used cf index
                    if (0 == pMBEntry->isCfg && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                        _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

                    return OMCI_ERR_FAILED;
                }
            }

            rgCfEntry.index = pMBEntry->dsCfIndex;
            rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM;

            if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_add(&rgCfEntry)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "%d add cf ds rule %d failed, ret=%d", __LINE__, pMBEntry->dsCfIndex, ret);
                rg_cf_show(&rgCfEntry);

                if (0 == pMBEntry->isCfg && (pMBEntry->dsCfIndex != OMCI_UNUSED_CF))
                    _RemoveUsedCfIndex(pMBEntry->dsCfIndex);

                return OMCI_ERR_FAILED;
            }
            _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);

            #if 0
            omci_SetServicePort(systemTpid, pBridgeRule->vlanRule.outStyle.outVlan.vid, pBridgeRule);

            if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
            {
                //for downstream receive packet with stag but it cannot learn in l2-table since svlan drop reason.
                /* By default, uplink port aware svlan and downstream uni forward action is set to forwarding
                 * member mask(flood), so svlan member table should be configured uplink port and specfic uni
                 * port as member ports. Due to disable cvlan filtering, it doesn't care vlan member table of CVID  */
                omci_SetMemberPortBySvlan(&classifyCfg, pBridgeRule, systemTpid);
            }
            #endif
        }
    }
    return OMCI_ERR_OK;
}

static int omcidrv_IpHostToVeIpRule(OMCI_BRIDGE_RULE_ts *pNew,
                                    OMCI_BRIDGE_RULE_ts *pCur)
{
    int ignore_uni = FALSE;

    if (!pNew || !pCur)
        return OMCI_ERR_FAILED;

    memcpy(pNew, pCur, sizeof(OMCI_BRIDGE_RULE_ts));

    if ((VLAN_FILTER_NO_TAG & pCur->vlanRule.filterRule.filterStagMode) &&
        (VLAN_FILTER_NO_TAG & pCur->vlanRule.filterRule.filterCtagMode))
        ignore_uni = TRUE;

    if (VLAN_FILTER_NO_CARE_TAG & pCur->vlanRule.filterRule.filterCtagMode)
        ignore_uni = TRUE;

    if (ignore_uni)
    {
        if (pCur->vlanRule.outStyle.outTagNum)
        {
            if (pCur->vlanRule.outStyle.outVlan.pri < 8 && 
                pCur->vlanRule.outStyle.outVlan.vid < 4095 )
            {
                pNew->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_TCI;
                pNew->vlanRule.filterRule.filterCTag.vid = pCur->vlanRule.outStyle.outVlan.vid;
                pNew->vlanRule.filterRule.filterCTag.pri = pCur->vlanRule.outStyle.outVlan.pri;
            }
            else if (pCur->vlanRule.outStyle.outVlan.vid < 4095 &&
                     pCur->vlanRule.outStyle.outVlan.pri >= 8)
            {
            pNew->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_VID;
            pNew->vlanRule.filterRule.filterCTag.vid = pCur->vlanRule.outStyle.outVlan.vid;
            }
            else if (pCur->vlanRule.outStyle.outVlan.vid >= 4095 &&
                     pCur->vlanRule.outStyle.outVlan.pri < 8)
            {
                pNew->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_PRI;
                pNew->vlanRule.filterRule.filterCTag.pri = pCur->vlanRule.outStyle.outVlan.pri;
            }
        }
        else
        {
            pNew->vlanRule.filterRule.filterCtagMode = VLAN_FILTER_NO_CARE_TAG;
        }
    }
    return OMCI_ERR_OK;
}

static int
pf_rtl96xx_UpdateVeipRule(
    int             wanIdx,
    int             vid,
    int             pri,
    int             type,
    int             service,
    int             isBinding,
    int             netIfIdx,
    unsigned char   isRegister)
{
    OMCI_CHIP_ERROR_CODE ret;
    struct list_head    *pEntry;
    struct list_head    *pTmpEntry;
    veip_service_t      *pData;

    if (wanIdx < 0 || wanIdx >= gPlatformDb.intfNum)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_ERR,
            "%s: wanIf[%u] is out of bound",
            __FUNCTION__,
            wanIdx);
        return OMCI_ERR_FAILED;
    }

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);

        if (isRegister)
        {
            // check veip rule and see if it is matched to this wan interface
            if (OMCI_ERR_OK != omcidrv_matchVeipRule(&pData->rule, vid, pri, type))
            {
                continue;
            }
            // set veip rule
            ret = omcidrv_setVeipRule(
                    pData,
                    wanIdx,
                    vid,
                    pri,
                    type,
                    service,
                    isBinding,
                    netIfIdx);
            if (ret == OMCI_ERR_FAILED)
            {
                OMCI_LOG(
                    OMCI_LOG_LEVEL_ERR,
                    "%s: Failed to set VEIP ruls for wanIf[%u]\n",
                    __FUNCTION__,
                    wanIdx);
                return ret;
            }

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
    OMCI_LOG(
        OMCI_LOG_LEVEL_ERR,
        "%s: Failed to %s the veip rule for wanif[%u]\n",
        __FUNCTION__,
        (isRegister ? "modify": "delete"),
        wanIdx);

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
    int                 valid_idx;
    rtk_rg_intfInfo_t   intf_info;

    if (!pBridgeRule)
    {
        OMCI_LOG (
            OMCI_LOG_LEVEL_ERR,
            "%s: pBridgeRule is NULL\n",
            __FUNCTION__);
        return OMCI_ERR_FAILED;
    }

    pVlanRule = &pBridgeRule->vlanRule;

    // get/create veip entry
    pVeipEntry = omcidrv_getVeipEntry(pBridgeRule->servId,
            pVlanRule, pBridgeRule->usFlowId, pBridgeRule->dsFlowId);
    if (!pVeipEntry)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_WARN,
            "%s: get/create veip entry fail!",
            __FUNCTION__);
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

        valid_idx = netIfIdx;
        memset(&intf_info, 0, sizeof(rtk_rg_intfInfo_t));

        if (RT_ERR_OK != rtk_rg_intfInfo_find(&intf_info, &valid_idx) ||
            0 == intf_info.is_wan)
            continue;

        if (OMCI_ERR_OK != omcidrv_matchVeipRule(pVlanRule, vid, pri, type))
            continue;

        // set veip rule
        if (OMCI_ERR_OK != omcidrv_setVeipRule(pVeipEntry,
                wanIdx, vid, pri, type, service, isBinding, netIfIdx))
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

    if((ret = rtk_rg_gpon_tcont_get(&ind,&attr))!=RT_ERR_OK)
    {
        if (RT_ERR_OK == rtk_rg_gpon_tcont_get_physical(&ind, &attr))
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
        ret = rtk_rg_gpon_tcont_create(&ind,&attr);
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
        if (RT_ERR_OK == rtk_rg_ponmac_queue_get(&queue, &queueCfg))
        {
            numOfEmptyQueue ++;
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

                if (RT_ERR_OK == rtk_rg_ponmac_queue_get(&queue, &queueCfg))
                {
                    numOfEmptyQueue = 0;

                    break;
                }
            }

            if (tContId == (schedulerId / 8 * 8 + 8))
                numOfEmptyQueue++;
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
    if (RT_ERR_OK != rtk_rg_gpon_tcont_get(&ind, &attr))
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
            if ((tContId / 8) == (attr.tcont_id / 8))
                continue;

            if (OMCI_ERR_OK != omci_reqTcontEmptyQ(tContId, numOfReqQueue, &queueId))
                continue;

            gPlatformDb.tCont[attr.tcont_id].allocId = 0xff;
            gPlatformDb.tCont[tContId].allocId = ind.alloc_id;
            gPlatformDb.tCont[tContId].qIdFrom = queueId;

            pTcont->tcontId = tContId;

            attr.tcont_id = tContId;
            ind.type = RTK_GPON_TCONT_TYPE_1;

            // destory old tcont and re-create with new id
            rtk_rg_gpon_tcont_destroy(&ind);
            ret = rtk_rg_gpon_tcont_create(&ind, &attr);

            break;
        }
    }

    return ret;
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
            ret = rtk_rg_gpon_usFlow_set(gemFlow->flowId, &usFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"could not create u/s gem flow [0x%x]", (int)ret);
                return OMCI_ERR_FAILED;
            }

            queue.queueId = omci_getTcontQid(gemFlow->tcontId,gemFlow->queueId);
            queue.schedulerId = gemFlow->tcontId;

            if(queue.queueId!=OMCI_DRV_INVALID_QUEUE_ID)
            {
                ret = rtk_rg_ponmac_flow2Queue_set(flowId,&queue);
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"create a u/s gem flow [%d]", flowId);
        }
        else
        {
            /* delete a gem u/s flow */
            memset(&usFlow, 0, sizeof(rtk_gpon_usFlow_attr_t));
            usFlow.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;
            ret = rtk_rg_gpon_usFlow_set(flowId, &usFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"\n delete a u/s gem flow [%d] Fail, ret = 0x%X", flowId, ret);
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

            ret = rtk_rg_gpon_dsFlow_set(flowId, &dsFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"could not create d/s gem flow [0x%x]", (int)ret);

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
            ret = rtk_rg_gpon_dsFlow_set(flowId, &dsFlow);

            if (ret != RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"\n delete a d/s gem flow [%d] Fail, ret = 0x%X", flowId, ret);
                return OMCI_ERR_FAILED;
            }

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete a d/s gem flow [%d]", flowId);
        }
    }
    return OMCI_ERR_OK;
}

/*static int omci_SetPriRemapByDpMarking(uint8 dpMarking)
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
}*/

static int omci_SetDsPriQ(OMCI_PRIQ_ts *pPriQ)
{
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
    if((ret = rtk_rg_ponmac_queue_add(&queue,&queueCfg))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"PriQ add failed, queueId %d, tcontId %d",pPriQ->queueId,pPriQ->tcontId);
        return OMCI_ERR_FAILED;
    }
    // return if dp is supported natively for upstream
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    // invoke priority remap by dp marking for upstream
    /*omci_SetPriRemapByDpMarking(pPriQ->dpMarking);*/ // -> done by omci_CreateUsDpCf

    return OMCI_ERR_OK;

}


static int pf_rtl96xx_ClearPriQueue(OMCI_PRIQ_ts *pPriQ)
{
    int ret;
    rtk_ponmac_queue_t  queue;

    memset(&queue,0,sizeof(rtk_ponmac_queue_t));

    queue.queueId = omci_getTcontQid(pPriQ->tcontId,pPriQ->queueId);
    queue.schedulerId = pPriQ->tcontId;

    if((ret = rtk_rg_ponmac_queue_del(&queue))!=RT_ERR_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"PriQ del failed, queueId %d, tcontId %d",pPriQ->queueId,pPriQ->tcontId);
        return OMCI_ERR_FAILED;
    }

    return OMCI_ERR_OK;

}
#if 0
#define CPRI_SPRI_CFI_DEI_LEN 20
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

    if (!(pAclIdBitmap = (unsigned int *)kzalloc(sizeof(unsigned int) * CPRI_SPRI_CFI_DEI_LEN), GFP_KERNEL)))
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
            for (i = 0; i < CPRI_SPRI_CFI_DEI_LEN; i++)
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

        memcpy(pAclIdBitmap, pEntry->pAclIdBitmap, sizeof(unsigned int) * CPRI_SPRI_CFI_DEI_LEN);

        for (i = 0; i < CPRI_SPRI_CFI_DEI_LEN; i++)
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
#endif
static int
omci_DeleteUsDpCf(uint32 portId, l2_service_t *pL2srv)
{
    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    if (pL2srv->usDpStreamId != pL2srv->usStreamId)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_rg_classifyEntry_del(pL2srv->pUsDpCfIndex[portId]));
        _RemoveUsedCfIndex(pL2srv->pUsDpCfIndex[portId]);
    }

    //omci_cfg_us_dp_stag_acl(FALSE, pL2srv, pL2srv->uniMask);
    return OMCI_ERR_OK;
}

static int omci_MaintainVlanTable(void)
{
    int ret;
    l2_service_t            *pL2Entry = NULL;
    struct list_head        *next = NULL, *tmp=NULL;
    rtk_portmask_t          *pMember, *pUntag;
    //rtk_portmask_t          allPortMask;
    rtk_vlan_t              vid;
    int                     i;
    //struct list_head        *pGroup    = NULL;
    //struct list_head        *pTmpGroup = NULL;
    //struct list_head        *pEntry    = NULL;
   // struct list_head        *pTmpEntry = NULL;
    rtk_rg_cvlan_info_t     rgCvlan;
    int                     cpuPvid = -1;
    //dsAggregated_group_t    *pGroupEntry = NULL;
    //dsAggregated_entry_t    *pEntryData  = NULL;


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

    /* VLAN default configuration
    if((ret = rtk_vlan_init()) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] VLAN Init Fail, ret = %d", __LINE__, ret);*/

    /* Set all VLAN configuration */
    for(vid = 0; vid <= 4094; vid++)
    {
        memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
        rgCvlan.vlanId = vid;

        // get entry if it exist
        if (RT_ERR_RG_OK == (ret = rtk_rg_cvlan_get(&rgCvlan)) &&
            rgCvlan.addedAsCustomerVLAN)
        {
            if (RT_ERR_RG_OK != (ret = rtk_rg_cvlan_del(vid)))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "create ani vlan for mbcast fail, ret = %d", ret);
            }
        }
    }
    /*
    if((ret = rtk_vlan_vlanFunctionEnable_set(DISABLED)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Disable VLAN filter Fail, ret = %d", __LINE__, ret);

    if((ret = rtk_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] Configure Reserved VID Fail, ret = %d", __LINE__, ret);*/

    /* Set all UNI port PVID to 0
    if((ret = rtk_rg_switch_allPortMask_set(&allPortMask)) != RT_ERR_OK)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "[%d] set all portmask fail, ret = %d", __LINE__, ret);*/
    /*
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
    }*/

    // get port-based-vid of cpu port and set all lan port to to same default port-based vid
    if (RT_ERR_RG_OK == (ret = rtk_rg_portBasedCVlanId_get(gPlatformDb.cpuPort, &cpuPvid)))
    {
        for (i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if (i < gPlatformDb.ponPort)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, cpuPvid));
            }
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
                if (pL2Entry->rule.filterMode == VLAN_OPER_MODE_FORWARD_ALL ||
                    pL2Entry->rule.filterMode == VLAN_OPER_MODE_FILTER_INNER_PRI ||
                    pL2Entry->rule.filterMode == VLAN_OPER_MODE_FORWARD_SINGLETAG)
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
                    if((pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_NO_TAG) &&
                       (pL2Entry->rule.cTagAct.vlanAct == VLAN_ACT_ADD) )
                    {
                        /* Add Ctag VID to US untagged packet: add this UNI port to default VLAN as an untagged port */
                        vid = pL2Entry->rule.cTagAct.assignVlan.vid;
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] |= (0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
                    }
                    else if((pL2Entry->rule.filterRule.filterStagMode & VLAN_FILTER_NO_TAG) &&
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
                    /* Filter any CVID*/
                    if (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_CARE_TAG)
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
                    (pL2Entry->rule.filterRule.filterStagMode & VLAN_FILTER_NO_CARE_TAG) &&
                    (pL2Entry->rule.filterRule.filterCtagMode & VLAN_FILTER_VID) &&
                    (pL2Entry->rule.sTagAct.vlanAct == VLAN_ACT_TRANSPARENT) &&
                    (pL2Entry->rule.cTagAct.vlanAct == VLAN_ACT_TRANSPARENT) &&
                    (1 == pL2Entry->rule.outStyle.outTagNum) &&
                    (TPID_88A8 == pL2Entry->rule.outStyle.tpid))
                {
                    /* Filter 1 S-tagged packet and transparent this tagged: Set this UNI port into all 4K VLAN (0~4094) */
                    for(vid = 0; vid <= 4094; vid++)
                    {
                        pMember[vid].bits[0] |= (0x0001 << i);
                        pUntag[vid].bits[0] &= ~(0x0001 << i);
                        pMember[vid].bits[0] |= (0x0001 << gPlatformDb.ponPort);
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
    /* By default, it is controled by rome driver
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_DBG, rtk_svlan_untagAction_set(SVLAN_ACTION_SVLAN, 0));*/

    if (!list_empty(&gPlatformDb.l2Head))
    {
        /* Set all VLAN configuration */
        for(vid = 0; vid <= 4094; vid++)
        {
            if (!gPlatformDb.rgIvlMcastSupport)
            {
                //if (pMBEntry->rule.filterRule.filterCTag.vid != rgInitDb.fwdVLAN_CPU)
                {
                    memset(&rgCvlan, 0, sizeof(rtk_rg_cvlan_info_t));
                    rgCvlan.vlanId = vid;

                    // get entry if it exist
                    if (RT_ERR_RG_OK == (ret = rtk_rg_cvlan_get(&rgCvlan)))
                    {
                        rgCvlan.memberPortMask.portmask |=
                            ((1 << gPlatformDb.ponPort) | pMember[vid].bits[0]);
                    }
                    else
                    {
                        rgCvlan.memberPortMask.portmask =
                            ((1 << gPlatformDb.ponPort) | pMember[vid].bits[0] | gPlatformDb.extPortMask);//gPlatformDb.cpuPortMask
                    }

                    if (RT_ERR_RG_OK != (ret = rtk_rg_cvlan_add(&rgCvlan)))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "create ani vlan for mbcast fail, ret = %d", ret);
                    }
                }
            }
        }
    }
    kfree(pMember);
    kfree(pUntag);

    /* SP2C entry */
    /* uc Aggregated VLAN entry
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
    }*/

    /* mb Aggregated VLAN entry
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
    }*/



    /* Scan all L2 entry for port-based vid */
    list_for_each_safe(next, tmp, &gPlatformDb.l2Head)
    {
        pL2Entry = list_entry(next,l2_service_t,list);

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if (i >= gPlatformDb.ponPort)
                continue;

            if(pL2Entry->uniMask & (1 << i))
            {
                if(VLAN_OPER_MODE_FORWARD_ALL == pL2Entry->rule.filterMode)
                {
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, 0));
                }
                else if(pL2Entry->rule.filterMode == VLAN_OPER_MODE_EXTVLAN)
                {
                    /* Action */
                    if ((VLAN_FILTER_NO_TAG & pL2Entry->rule.filterRule.filterCtagMode) &&
                       (VLAN_ACT_ADD == pL2Entry->rule.cTagAct.vlanAct) )
                    {
                        unsigned int systemTpid;
                        /* Add Ctag VID to US untagged packet: add this UNI port to default VLAN as an untagged port */
                        vid = pL2Entry->rule.cTagAct.assignVlan.vid;
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, vid));

                        // set port-based-vid of pon port if the first TPID = 0x8100,
                        //
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_svlanTpid_get(&systemTpid));
                        if (0x8100 == systemTpid)
                        {
                            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set((rtk_rg_port_idx_t)(gPlatformDb.ponPort), vid));
                        }
                    }
                    else if ((VLAN_FILTER_NO_TAG & pL2Entry->rule.filterRule.filterStagMode) &&
                            (VLAN_ACT_ADD == pL2Entry->rule.sTagAct.vlanAct) )
                    {
                        /* Add Stag VID to US untagged packet: add this UNI port to default VLAN as an untagged port */
                        vid = pL2Entry->rule.sTagAct.assignVlan.vid;
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, vid));
                    }
                    else if ((VLAN_FILTER_NO_TAG & pL2Entry->rule.filterRule.filterCtagMode) &&
                       (VLAN_ACT_TRANSPARENT == pL2Entry->rule.cTagAct.vlanAct))
                    {
                        vid = 4096;
                        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, vid));
                    }
                }
                else if (VLAN_OPER_MODE_FILTER_SINGLETAG == pL2Entry->rule.filterMode &&
                        VLAN_FILTER_NO_CARE_TAG & pL2Entry->rule.filterRule.filterStagMode &&
                        (VLAN_ACT_NON == pL2Entry->rule.sTagAct.vlanAct ||
                        VLAN_ACT_TRANSPARENT == pL2Entry->rule.sTagAct.vlanAct) &&
                        (VLAN_ACT_NON == pL2Entry->rule.cTagAct.vlanAct ||
                        VLAN_ACT_TRANSPARENT == pL2Entry->rule.cTagAct.vlanAct) &&
                        1 == pL2Entry->rule.outStyle.outTagNum &&
                        TPID_88A8 == pL2Entry->rule.outStyle.tpid)
                {
                    /* for 6.1.5: due to rome driver always check cvlan member, it is workround for set pvid of ctag while ingress packet with single stag */
                    // TBD: maybe change outStyle.outVlan.tpid it maybe no care in order to distinguish set pvid or not
                    vid = pL2Entry->rule.outStyle.outVlan.vid;
                    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_rg_portBasedCVlanId_set(i, vid));
                }
            }
        }
    }
    return OMCI_ERR_OK;
}


static int pf_rtl96xx_DeactiveBdgConn(int servId)
{
    int ret;
    int i;
    unsigned int systemTpid;
    l2_service_t *pL2Entry = NULL;
    veip_service_t *pVeipEntry = NULL;
    mbcast_service_t *pMBEntry = NULL;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_svlanTpid_get(&systemTpid));
    pMBEntry = mbcastServ_entry_find(servId);

    if (pMBEntry)
    {
        if (pDmmCb && pDmmCb->omci_dm_ds_mc_bc_info_del)
        {
            if (0 != pDmmCb->omci_dm_ds_mc_bc_info_del(servId))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "%s() del mc bc rule by dmm fail", __FUNCTION__);
            }
        }
        else
        {
            // invoke ds pkt bcaster
            ret = rtk_rg_gponDsBcFilterAndRemarking_del(MAX_GPON_DS_BC_FILTER_SW_ENTRY_SIZE - servId - 1);
            if (RT_ERR_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "del ds tag operation fail, ret = %d", ret);
            }

            if (pMBEntry->rule.filterRule.filterCtagMode & VLAN_FILTER_VID ||
                    pMBEntry->rule.filterRule.filterCtagMode & VLAN_FILTER_TCI)
            {
                if (!(pMBEntry->uniMask & (1 << gPlatformDb.ponPort)) ||
                        !gPlatformDb.rgIvlMcastSupport)
                {
                    if (pMBEntry->rule.filterRule.filterCTag.vid != gPlatformDb.fwdVLAN_CPU)
                        rg_cvlan_del(pMBEntry->rule.filterRule.filterCTag.vid);
                }
            }
            if (pMBEntry->dsCfIndex <= gPlatformDb.veipFastStop)
            {
                if ((ret = rtk_rg_aclFilterAndQos_del(pMBEntry->dsCfIndex)) != RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del ds acl latch cf rule %x failed", pMBEntry->dsCfIndex);
                    //return OMCI_ERR_FAILED;
                }
            }
            else
            {
                if((ret = rtk_rg_classifyEntry_del(pMBEntry->dsCfIndex))!=RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del ds cf rule %x failed", pMBEntry->dsCfIndex);
                    //return OMCI_ERR_FAILED;
                }
            }
            if (OMCI_UNUSED_ACL != pMBEntry->referAclIdx)
            {
                if ((ret = rtk_rg_aclFilterAndQos_del(pMBEntry->referAclIdx)) != RT_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del acl rule %x failed", pMBEntry->referAclIdx);
                    //return OMCI_ERR_FAILED;
                }
                pMBEntry->referAclIdx = OMCI_UNUSED_ACL;
            }

            _RemoveUsedCfIndex(pMBEntry->dsCfIndex);
            pMBEntry->dsCfIndex = OMCI_UNUSED_CF;
            mbcastServ_entry_del(pMBEntry->index);
            /*
            if (RTL9601B_CHIP_ID != gPlatformDb.chipId)
            {
                if((ret = omci_mb_vlan_aggregate_proc()) != OMCI_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_mb_vlan_aggregate_proc Fail, ret = 0x%X", ret);
                }
            }*/
        }
    }

    pL2Entry = l2Serv_entry_find(servId);

    if(pL2Entry)
    {
        /*for us rule*/
        if(PON_GEMPORT_DIRECTION_US == pL2Entry->dir ||
            PON_GEMPORT_DIRECTION_BI == pL2Entry->dir)
        {
            for (i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                if(pL2Entry->pUsCfIndex[i] != OMCI_UNUSED_CF)
                {
                    // TBD, RG not support SVLAN filtering
                    /*if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
                    {
                        omci_delMemberPortBySvlan(pL2Entry->pUsCfIndex[i], systemTpid);
                    }
                    omci_delServicePort(systemTpid, pL2Entry->pUsCfIndex[i]);*/

                    omci_DeleteUsDpCf(i, pL2Entry);

                    if((ret = rtk_rg_classifyEntry_del(pL2Entry->pUsCfIndex[i]))!=RT_ERR_OK)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del us cf rule %x failed", pL2Entry->pUsCfIndex[i]);
                        return OMCI_ERR_FAILED;
                    }
                    _RemoveUsedCfIndex(pL2Entry->pUsCfIndex[i]);
                }
            }
        }
        /*for ds rule*/
        if(PON_GEMPORT_DIRECTION_BI == pL2Entry->dir)
        {
            if((ret = rtk_rg_classifyEntry_del(pL2Entry->dsCfIndex))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del ds cf rule %x failed", pL2Entry->dsCfIndex);
                return OMCI_ERR_FAILED;
            }
            _RemoveUsedCfIndex(pL2Entry->dsCfIndex);
        }

        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "del l2 rule %d for uni 0x%x [us %d ds %d]",
            servId, pL2Entry->uniMask, pL2Entry->usStreamId, pL2Entry->dsStreamId);

        /*delete from list*/
        l2Serv_entry_del(servId);

        /* Process VLAN aggregated CF
        if((ret = omci_l2_vlan_aggregate_proc()) != OMCI_ERR_OK)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_l2_vlan_aggregate_proc Fail, ret = 0x%X", ret);
        } */
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
#if 1
    /* Maintain VLAN table */
    if (gDrvCtrl.devMode == OMCI_DEV_MODE_HYBRID)
        omci_MaintainVlanTable();
#endif
    return OMCI_ERR_OK;
}

static int
omci_CreateUsDpCf(uint32 portId,
        l2_service_t *pL2srv, rtk_rg_classifyEntry_t *orgCfCfg, uint8 isEthTypeFilter, omci_rule_pri_t rule_pri, unsigned int uniMask)
{
    int                     ret;
    rtk_rg_classifyEntry_t  newCfCfg;

    // return if dp is supported natively
    if (gPlatformDb.perTContQueueDp)
        return OMCI_ERR_OK;

    if (!pL2srv || !orgCfCfg)
        return OMCI_ERR_FAILED;

    // nothing has to be done for normal flow
    if (pL2srv->usDpStreamId == pL2srv->usStreamId)
        return OMCI_ERR_OK;


    // copy classify content
    memset(&newCfCfg, 0, sizeof(rtk_rg_classifyEntry_t));
    memcpy(&newCfCfg, orgCfCfg, sizeof(rtk_rg_classifyEntry_t));

    // allocate dp cf index
    if (PQ_DROP_COLOUR_DEI_MARKING == pL2srv->usDpMarking)
    {
        // for dp connection, set dei bit to 1
        /*
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
        */
        // TBD: use rg acl
    }
    else
    {
        // for dp connection, set internal pri to 0
        newCfCfg.internalPri = 0;
        newCfCfg.filter_fields |= EGRESS_INTERNALPRI_BIT;

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

        newCfCfg.index = pL2srv->pUsDpCfIndex[portId];
        // assign dp flow id
        newCfCfg.action_sid_or_llid.assignedSid_or_llid = pL2srv->usDpStreamId;
        newCfCfg.us_action_field |= CF_US_ACTION_SID_BIT;

        // configure dp cf
        if (RT_ERR_OK != (ret = rtk_rg_classifyEntry_add(&newCfCfg)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add us dp cf %u (sid %u, dpsid %d) failed, return code %d",
                newCfCfg.index, pL2srv->usStreamId, pL2srv->usDpStreamId, ret);
            rg_cf_show(&newCfCfg);
        }
        _saveCfCfgToDb(newCfCfg.index, &newCfCfg);
    }


    if (PQ_DROP_COLOUR_DEI_MARKING == pL2srv->usDpMarking)
    {
        /*for normal connection, set dei bit to 0
        if(RTL9602C_CHIP_ID == gPlatformDb.chipId)
        {
            orgCfCfg->templateIdx = 1;
            dei.classify_pattern.dei.value = 0;

        }
        else
        {
            // for normal connection, set dei bit to 0
            //  but it's actually useless...
            //  dei classify needs to reassign cf index to 0 ~ 63
            dei.classify_pattern.dei.value = 0;
        }*/
        // TBD: use rg acl
    }
    else
    {
        // for normal connection, set internal pri to 1
        //interPri.classify_pattern.interPri.value = 1;
        orgCfCfg->internalPri = 1;
        orgCfCfg->filter_fields |= EGRESS_INTERNALPRI_BIT;
    }
    //omci_cfg_us_dp_stag_acl(TRUE, pL2srv, uniMask);
    return OMCI_ERR_OK;
}


/*Assign DS OMCI internal PRI to 7, to avoid priority queue issue*/
static int omci_setOmciInternalPri(void)
{
    int     ret;
    int     aclIdx;
    rtk_rg_aclFilterAndQos_t rgAclCfg;

    memset(&rgAclCfg, 0, sizeof(rtk_rg_aclFilterAndQos_t));

    if(gPlatformDb.chipId == APOLLOMP_CHIP_ID)
    {
        aclIdx = OMCI_UNUSED_ACL;
        rgAclCfg.fwding_type_and_direction    = ACL_FWD_TYPE_DIR_INGRESS_ALL_PACKET;
        rgAclCfg.filter_fields                = INGRESS_STREAM_ID_BIT | INGRESS_PORT_BIT;
        rgAclCfg.ingress_stream_id            = 127;
        rgAclCfg.ingress_port_mask.portmask   = (1 << gPlatformDb.ponPort);
        rgAclCfg.action_type                  = ACL_ACTION_TYPE_QOS;
        rgAclCfg.qos_actions                  = ACL_ACTION_ACL_PRIORITY_BIT;
        rgAclCfg.action_acl_priority          = 7;

        if (RT_ERR_RG_OK != (ret = rtk_rg_aclFilterAndQos_add(&rgAclCfg, &aclIdx)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "add downstream OMCI internal pri fail, return code %d", ret);
            return OMCI_ERR_FAILED;
        }
    }
    return OMCI_ERR_OK;

}


static int pf_rtl96xx_SetL2Rule(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    int ret = OMCI_ERR_OK;
    rtk_rg_classifyEntry_t  rgCfEntry;
    OMCI_VLAN_OPER_ts   dsRule;
    l2_service_t *pL2Entry;
    int i;
    unsigned int minUsIndex, systemTpid;
    omci_rule_pri_t rule_pri;

    memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));

    pL2Entry = l2Serv_entry_find(pBridgeRule->servId);

    if(!pL2Entry)
    {
        if((ret = l2Serv_entry_add(pBridgeRule))!=OMCI_ERR_OK)
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
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_svlanTpid_get(&systemTpid));

    if (0x8100 == systemTpid)
    {
        /*Due to SVLAN TPID change to 0x8100, so single tag Ctag need to change to Stag*/
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
                    // skip cpu port and veip pseudo port
                    if (i == gPlatformDb.cpuPort)
                        continue;

                    minUsIndex = (pL2Entry->pUsCfIndex[i] <= minUsIndex) ?
                        pL2Entry->pUsCfIndex[i] : minUsIndex;
                    if(RT_ERR_OK == rtk_rg_classifyEntry_find(pL2Entry->pUsCfIndex[i], &rgCfEntry))
                    {
                        omci_DeleteUsDpCf(i, pL2Entry);
                        if((ret = rtk_rg_classifyEntry_del(pL2Entry->pUsCfIndex[i]))!=RT_ERR_OK)
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"del us cf rule %d failed", pL2Entry->pUsCfIndex[i]);
                            return OMCI_ERR_FAILED;
                        }
                        _RemoveUsedCfIndex(pL2Entry->pUsCfIndex[i]);
                    }
                }
            }
            /* add new one classify rule without filter uni */
            gPlatformDb.cfRule[minUsIndex].isCfg = 1;
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                // skip cpu port and veip pseudo port
                if (i == gPlatformDb.cpuPort || i == gPlatformDb.ponPort)
                    continue;

                pL2Entry->pUsCfIndex[i] = minUsIndex;
            }
            memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));
            rgCfEntry.index = minUsIndex;
            rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_UPSTREAM;
            omci_SetClassifyUsAct(&pBridgeRule->vlanRule, pBridgeRule->usFlowId, &rgCfEntry);

            memset(&rule_pri, 0x0, sizeof(omci_rule_pri_t));
            omci_SetClassifyUsRule(&pBridgeRule->vlanRule, (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)), &rgCfEntry, &rule_pri);

            omci_CreateUsDpCf(0, pL2Entry, &rgCfEntry, VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
                (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));
            if((ret = rtk_rg_classifyEntry_add(&rgCfEntry))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"[%d] add cf us rule %x failed, ret=%d, uniMask=%d, oldservId=%u, servId=%d", __LINE__, minUsIndex, ret, pBridgeRule->uniMask, pL2Entry->index, pBridgeRule->servId);
                rg_cf_show(&rgCfEntry);
                return OMCI_ERR_FAILED;
            }
            _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);
        }
        else
        {
            // TBD
            /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
            /* Setup ACL rule before create CF rule
            omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_US, &(pBridgeRule->vlanRule), pBridgeRule->uniMask, 0, &usAclIndex);*/

            /* add classfy rule since different rule */
            for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
            {
                // skip veip pseudo port
                if (i == gPlatformDb.ponPort)
                    continue;

                if((pBridgeRule->uniMask & (1 << i))
                    && ((pL2Entry->isCfg != 1) || (OMCI_UNUSED_CF == pL2Entry->pUsCfIndex[i])))
                {

                    memset(&rgCfEntry,0,sizeof(rtk_rg_classifyEntry_t));

                    omci_SetClassifyUsAct(&pBridgeRule->vlanRule,pBridgeRule->usFlowId,&rgCfEntry);

                    omci_SetClassifyUsRule(&pBridgeRule->vlanRule, i, &rgCfEntry,  &rule_pri);

                    if(VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode)
                    {
                        if (OMCI_ERR_OK !=
                                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pL2Entry->pUsCfIndex[i]))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                                "allocate us cf type %d for l2 fail!", PF_CF_TYPE_L2_ETH_FILTER);

                            return OMCI_ERR_FAILED;
                        }
                    }
                    else
                    {
                        if (OMCI_ERR_OK !=
                                _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2Entry->pUsCfIndex[i]))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                                "allocate us cf type %d for l2 fail!", PF_CF_TYPE_L2_COMM);

                            return OMCI_ERR_FAILED;
                        }
                    }

                    rgCfEntry.index = pL2Entry->pUsCfIndex[i];
                    rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_UPSTREAM;

                    omci_CreateUsDpCf(i, pL2Entry, &rgCfEntry, VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode, rule_pri,
                        (pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)));

                    if((ret = rtk_rg_classifyEntry_add(&rgCfEntry))!=RT_ERR_OK)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"[%d] add cf us rule %x failed, ret=%d", __LINE__, pL2Entry->pUsCfIndex[i], ret);
                        rg_cf_show(&rgCfEntry);
                        return OMCI_ERR_FAILED;
                    }
                    _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);
                }
            }
        }
    }


    //for TR-247 test cases verify stag or send packet with stag from UTP port

    // TBD, RG not support SVLAN filtering
    /*if(0x88a8 == systemTpid)
        omci_SetServicePort(pBridgeRule->vlanRule.outStyle.outVlan.vid, pBridgeRule);*/

    //for downstream receive packet with stag but it cannot learn in l2-table since svlan drop reason.
    /* By default, uplink port aware svlan and downstream uni forward action is set to forwarding
     * member mask(flood), so svlan member table should be configured uplink port and specfic uni
     * port as member ports. Due to disable cvlan filtering, it doesn't care vlan member table of CVID  */
    /*omci_SetMemberPortBySvlan(&classifyCfg, pBridgeRule, systemTpid);*/

    if(pBridgeRule->dir==PON_GEMPORT_DIRECTION_BI)
    {
        if(pL2Entry->uniMask != pBridgeRule->uniMask)
        {

            omci_SetUsRuleInvert2Ds(&dsRule,&pBridgeRule->vlanRule);

            // TBD
            /* If this service flow needs to filter double tags + SID => create ACL rule to filter inner tag + SID. */
            /* Setup ACL rule before create CF rule
            omci_setInnerTagAclRule(PON_GEMPORT_DIRECTION_DS, &dsRule, pBridgeRule->uniMask, pBridgeRule->dsFlowId, &dsAclIndex);*/

            memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));
            memset(&rule_pri, 0, sizeof(omci_rule_pri_t));

            omci_SetClassifyDsAct(&dsRule,(pBridgeRule->uniMask & ~(1 << gPlatformDb.ponPort)),&rgCfEntry);
            //TBD: cf pri act
            omci_SetClassifyDsRule(&dsRule, &pBridgeRule->vlanRule, pBridgeRule->dsFlowId,&rgCfEntry, &rule_pri);

            if((pL2Entry->isCfg != 1) || (pL2Entry->dsCfIndex == OMCI_UNUSED_CF))
            {
                if(VLAN_FILTER_ETHTYPE & pBridgeRule->vlanRule.filterRule.filterCtagMode)
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_ETH_FILTER, rule_pri, &pL2Entry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_ETH_FILTER);

                        return OMCI_ERR_FAILED;
                    }
                }
                else
                {
                    if (OMCI_ERR_OK !=
                            _AssignNonUsedCfIndex(PF_CF_TYPE_L2_COMM, rule_pri, &pL2Entry->dsCfIndex))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                            "allocate ds cf type %d for l2 fail!", PF_CF_TYPE_L2_COMM);

                        return OMCI_ERR_FAILED;
                    }
                }
            }

            rgCfEntry.index = pL2Entry->dsCfIndex;
            rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM;

            if((ret = rtk_rg_classifyEntry_add(&rgCfEntry))!=RT_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"add cf ds rule %d failed, ret=%d", pL2Entry->dsCfIndex, ret);
                rg_cf_show(&rgCfEntry);
                return OMCI_ERR_FAILED;
            }
            _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);
        }
    }
    pL2Entry->uniMask = pBridgeRule->uniMask;
    pL2Entry->isCfg = 1;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "set l2 rule %d for uni 0x%x [us %d ds %d]",
        pBridgeRule->servId, pL2Entry->uniMask, pL2Entry->usStreamId, pL2Entry->dsStreamId);

    return ret;
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
    if (OMCI_ERR_OK == (ret = omcidrv_updateMcastBcastRule(pMBEntry, pBridgeRule)))
    {
        pMBEntry->isCfg = 1;
    }
    else
    {
        pMBEntry->isCfg = 0;
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

        /*if (RTL9601B_CHIP_ID != gPlatformDb.chipId)
        {
            if((ret = omci_mb_vlan_aggregate_proc()) != OMCI_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_mb_vlan_aggregate_proc Fail, ret = 0x%X", ret);
            }
        }*/
    }
    else
    {
        OMCI_BRIDGE_RULE_ts tmp;
        memcpy(&tmp, pBridgeRule, sizeof(OMCI_BRIDGE_RULE_ts));

        //exclude cpu port in l2 rule, it is for IP host connection and change to veip Rule
        if (pBridgeRule->uniMask & (~((1 << gPlatformDb.cpuPort) | (1 << gPlatformDb.ponPort))) && gDrvCtrl.devMode == OMCI_DEV_MODE_HYBRID)
        {
                ret = pf_rtl96xx_SetL2Rule(pBridgeRule);

                /* Process VLAN aggregated CF
                if((ret = omci_l2_vlan_aggregate_proc()) != OMCI_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_l2_vlan_aggregate_proc Fail, ret = 0x%X", ret);
                }*/
        }
        else if (pBridgeRule->uniMask & (~(1 << gPlatformDb.ponPort)) && gDrvCtrl.devMode == OMCI_DEV_MODE_BRIDGE)
        {
                ret = pf_rtl96xx_SetL2Rule(pBridgeRule);

                /* Process VLAN aggregated CF
                if((ret = omci_l2_vlan_aggregate_proc()) != OMCI_ERR_OK)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_l2_vlan_aggregate_proc Fail, ret = 0x%X", ret);
                }*/
        }


        if((pBridgeRule->uniMask & (1 << gPlatformDb.ponPort)) &&  gDrvCtrl.devMode != OMCI_DEV_MODE_BRIDGE)
            ret = pf_rtl96xx_SetVeipRule(&tmp);

        if((pBridgeRule->uniMask & (1 << gPlatformDb.cpuPort)) &&  gDrvCtrl.devMode != OMCI_DEV_MODE_BRIDGE)
        {
            OMCI_BRIDGE_RULE_ts newRule;
            ret = omcidrv_IpHostToVeIpRule(&newRule, &tmp);
            ret = pf_rtl96xx_SetVeipRule(&newRule);
        }

        if (gDrvCtrl.devMode == OMCI_DEV_MODE_HYBRID)
        {
            /* Maintain VLAN table */
            omci_MaintainVlanTable();
        }
    }

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

    ret = rtk_rg_gpon_serialNumber_set(&sn);
    return ret;
}

static int pf_rtl96xx_GetSerialNum(char *serial)
{
    int ret;
    rtk_gpon_serialNumber_t sn;

    ret = rtk_rg_gpon_serialNumber_get(&sn);

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

    ret = rtk_rg_gpon_password_set(&gpon_password);
    return ret;
}

static int pf_rtl96xx_ActivateGpon(int activate)
{
    int ret;

    if(activate)
    {
        /*Before activate gpon, set OMCI internal pri. And should set this after RG init*/
        omci_setOmciInternalPri();

        ret = rtk_rg_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);

    }
    else
        ret = rtk_rg_gpon_deActivate();

    return ret;
}
//
//  Default downstream drop should be turn on in order to pass ONU/L2 mode of TR-247 test case
//
#if 0
static int
omci_CreateDsDefaultDropCf(void)
{
    int                 ret, i;
    rtk_rg_classifyEntry_t  rgCfEntry;

    if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
        return OMCI_ERR_OK;

    for (i = gPlatformDb.cfNum - 1; 0 <= i; i--)
    {
        /* search empty entry */
        if (!gPlatformDb.cfRule[i].isCfg)
            break;
    }

    if (-1 < i && i < gPlatformDb.cfNum)
    {
        memset(&rgCfEntry, 0, sizeof(rtk_rg_classifyEntry_t));
        rgCfEntry.index = i;
        rgCfEntry.direction = RTK_RG_CLASSIFY_DIRECTION_DOWNSTREAM;
        rgCfEntry.action_fwd.fwdDecision = ACL_FWD_DROP;
        rgCfEntry.ds_action_field = CF_DS_ACTION_UNI_MASK_BIT;

        if (RT_ERR_RG_OK != (ret = rtk_rg_classifyEntry_add(&rgCfEntry)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "add downstream default drop cf fail, return code %d", ret);
        	rg_cf_show(&rgCfEntry);

        	return OMCI_ERR_FAILED;
        }

        gPlatformDb.cfRule[i].isCfg = 1;
        gPlatformDb.cfRule[i].rulePri.rule_pri = OMCI_VLAN_RULE_LOW_PRI;
        gPlatformDb.cfRule[i].rulePri.rule_level = 0;
        _saveCfCfgToDb(rgCfEntry.index, &rgCfEntry);
    }
    return OMCI_ERR_OK;


}
#endif
static int pf_rtl96xx_ResetMib(void)
{
    int i,ret;
    rtk_gpon_usFlow_attr_t usAttr;
    rtk_gpon_dsFlow_attr_t dsAttr;
    //rtk_port_t port = 0;
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veip_service_t          *pData;
    mbcast_service_t        *pMBcast;
    int                     wanIdx;
    // It should be removeded
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, rtk_classify_cfSel_set(gPlatformDb.ponPort, CLASSIFY_CF_SEL_ENABLE));

    _InitUsedTcontId();

    memset(&usAttr,0,sizeof(rtk_gpon_usFlow_attr_t));
    memset(&dsAttr,0,sizeof(rtk_gpon_dsFlow_attr_t));
    usAttr.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;
    dsAttr.gem_port_id  = RTK_GPON_GEMPORT_ID_NOUSE;

    for(i=0;i<gPlatformDb.maxFlow;i++)
    {
        if(i!=gPlatformDb.omccFlow)
        {
            ret = rtk_rg_gpon_usFlow_set(i,&usAttr);
            ret = rtk_rg_gpon_dsFlow_set(i,&dsAttr);
        }
    }

    if (!pDmmCb || (!(pDmmCb->omci_dm_ds_mc_bc_info_del) && !(pDmmCb->omci_dm_ds_mc_bc_info_set)))
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_gponDsBcFilterAndRemarking_Enable(RTK_RG_DISABLED));
    }
    /*remove software database*/

    // don't use delete all since other app might use this
    //rtk_rg_gponDsBcFilterAndRemarking_del_all();
    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.mbcastHead)
    {
        pMBcast = list_entry(pEntry, mbcast_service_t, list);

        if (pDmmCb && pDmmCb->omci_dm_ds_mc_bc_info_del)
        {
            if (OMCI_ERR_OK != pDmmCb->omci_dm_ds_mc_bc_info_del(pMBcast->index))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "%s() del mc bc rule by dmm fail", __FUNCTION__);

            }
        }
        else
        {
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR,
                rtk_rg_gponDsBcFilterAndRemarking_del(MAX_GPON_DS_BC_FILTER_SW_ENTRY_SIZE - pMBcast->index - 1));
        }

    }
    mbcastServ_entry_delAll();
    l2Serv_entry_delAll();
    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipHead)
    {
        pData = list_entry(pEntry, veip_service_t, list);
        //rtk_rg_cvlan_del(pData->rule.outStyle.outVlan.vid);

        for (wanIdx = 0; wanIdx < gPlatformDb.intfNum; wanIdx++)
        {
            if (!(pData->wanIfBitmap & (1 << wanIdx)))
                continue;

            // del veip rule
            if (OMCI_ERR_OK != omcidrv_delVeipRule(pData->index, wanIdx))
                continue;

            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "del veip rule %u for wan %d", pData->index, wanIdx);

            // modify wan status
            omcidrv_setWanStatusByIfIdx(wanIdx, FALSE);

            pData->wanIfBitmap &= ~(1 << wanIdx);
        }
    }
    veipServ_entry_delAll();
    veipGemFlow_entry_delAll();
    for (i = 0; i < gPlatformDb.cfNum; i++)
    {
        /*remove hw cf rule*/
        if (!gPlatformDb.cfRule[i].isCfg)
            continue;

        if (i <= gPlatformDb.veipFastStop){
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_aclFilterAndQos_del(i));
        }
        else{
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_classifyEntry_del(i));
        }
        _RemoveUsedCfIndex(i);
        /* reset sw cf rule */
        memset(&gPlatformDb.cfRule[i], 0, sizeof(pf_cf_rule_t));
    }
    //
    //  Default downstream drop should be turn on in order to pass ONU/L2 mode of TR-247 test case
    //
    #if 0
    /*Set DS default drop rule*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, omci_CreateDsDefaultDropCf());
    #endif
    ret = rtk_rg_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG);

    /*ret = rtk_vlan_vlanFunctionEnable_set(DISABLED);
    omci_cfg_vlan_table(0);*/

    // TBD, RG not support SVLAN filtering
    if (gDrvCtrl.devMode == OMCI_DEV_MODE_HYBRID)
    {
        for (i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
        {
            if (i <= gPlatformDb.ponPort)
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_svlanServicePort_set(i, ENABLED));
        }
    }

    // set default scheduler type to WRR
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qos_schedulingType_set(RTK_QOS_WRR));


    // reset bandwidth
    for (i = gPlatformDb.etherPortMin; i <= gPlatformDb.etherPortMax; i++)
    {
        if(i == gPlatformDb.ponPort || i == gPlatformDb.rgmiiPort)
            continue;
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_rate_portEgrBandwidthCtrlRate_set(i, MAX_BW_RATE));
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_rate_portIgrBandwidthCtrlRate_set(i, MAX_BW_RATE));
    }

    for (i = 0; i < gPlatformDb.meterNum; i++)
    {
        if (gPlatformDb.rsvMeterId == i)
            continue;
        if (!gPlatformDb.stormCtrl[i].isCfg)
            continue;

        // disable storm control
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_stormControl_del(i));

        // disable meters
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_shareMeter_set(gPlatformDb.stormCtrl[i].meterIdx, 0, RTK_RG_DISABLED));
        memset(&gPlatformDb.stormCtrl[i], 0, sizeof(pf_stormCtrl_t));
    }
    return OMCI_ERR_OK;
}

static int pf_rtl96xx_DumpL2Serv(void)
{
    struct list_head *next,*tmp;
    l2_service_t *cur;
    rtk_rg_classifyEntry_t  rgCfEntry;
    int ret = OMCI_ERR_OK, i;

    printk("======== OMCI L2 Service Rule ============\n");
    list_for_each_safe(next,tmp,&gPlatformDb.l2Head){

        cur = list_entry(next,l2_service_t,list);
        printk("Application ID: %d, uniMask: %u, dsCf=%u ", cur->index, cur->uniMask,  cur->dsCfIndex);

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            printk("usCf[%d]=%u ", i, cur->pUsCfIndex[i]);
        }
        printk("\n");

        for(i = gPlatformDb.allPortMin; i <= gPlatformDb.allPortMax; i++)
        {
            if(cur->pUsCfIndex[i] != OMCI_UNUSED_CF)
            {
                if((ret = rtk_rg_classifyEntry_find(cur->pUsCfIndex[i], &rgCfEntry))!=RT_ERR_OK)
                {
                    continue;
                }
                rg_cf_show(&rgCfEntry);
            }
        }

        if((ret = rtk_rg_classifyEntry_find(cur->dsCfIndex, &rgCfEntry))!=RT_ERR_OK)
        {
            continue;
        }
        rg_cf_show(&rgCfEntry);
        printk("###################################\n");
    }

    printk("=======================================End.\n");
    return ret;
}


static int pf_rtl96xx_DumpVeipServ(void)
{
    struct list_head            *pEntry;
    struct list_head            *pTmpEntry;
    veip_service_t              *pData;
    int                         wanIdx;
    rtk_rg_aclFilterAndQos_t    rgAclEntry;
    int                         rgAclEntryIdx;
    rtk_rg_classifyEntry_t      rgCfEntry;

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
                rgAclEntryIdx = pData->pHwPathCfIdx[wanIdx];
                if (RT_ERR_OK ==
                        rtk_rg_aclFilterAndQos_find(&rgAclEntry, &rgAclEntryIdx))
                    rg_aclqos_show(rgAclEntryIdx, &rgAclEntry);
            }

            // display sw path cf
            if (OMCI_UNUSED_CF != pData->pSwPathCfIdx[wanIdx])
            {
                if (pData->pSwPathCfIdx[wanIdx] >= gPlatformDb.veipFastStart &&
                        pData->pSwPathCfIdx[wanIdx] <= gPlatformDb.veipFastStop)
                {
                    rgAclEntryIdx = pData->pSwPathCfIdx[wanIdx];
                    if (RT_ERR_OK ==
                            rtk_rg_aclFilterAndQos_find(&rgAclEntry, &rgAclEntryIdx))
                        rg_aclqos_show(rgAclEntryIdx, &rgAclEntry);
                }
                else
                {
                    if (RT_ERR_OK ==
                            rtk_rg_classifyEntry_find(pData->pSwPathCfIdx[wanIdx], &rgCfEntry))
                        rg_cf_show(&rgCfEntry);
                }
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
    /*rtk_rg_classifyEntry_t    rgCfEntry;
    int ret = OMCI_ERR_OK, i;*/
    int ret = OMCI_ERR_OK;

    printk("======== OMCI mcast / bcast Service Rule ============\n");
    list_for_each_safe(next, tmp, &gPlatformDb.mbcastHead){

        cur = list_entry(next, mbcast_service_t, list);
        printk("Application ID: %d, isCfg=%u, uniMask: %u, dsCf=%u, referAclIdx=%u \n", cur->index, cur->isCfg, cur->uniMask,  cur->dsCfIndex, cur->referAclIdx);

    /*  if((ret = rtk_rg_classifyEntry_find(cur->dsCfIndex, &rgCfEntry))!=RT_ERR_OK)
        {
            continue;
        }
        rg_cf_show(&rgCfEntry);
        printk("###################################\n");*/
    }

    printk("=======================================End.\n");
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

static int pf_rtl96xx_InitPlatform(void)
{

    int ret;

    /*initial chip*/
    if (OMCI_ERR_OK != rtk_rg_gpon_initial(1))
        return OMCI_ERR_FAILED;

    ret = rtk_rg_classify_unmatchAction_set(CLASSIFY_UNMATCH_PERMIT_WITHOUT_PON);
    /* vid 0 and 4095 as tagging */
    ret = rtk_rg_vlan_reservedVidAction_set(RESVID_ACTION_TAG, RESVID_ACTION_TAG);
    // TBD, RG not support SVLAN filtering
    /*ret = rtk_vlan_vlanFunctionEnable_set(DISABLED);
    ret = rtk_svlan_servicePort_set(gPlatformDb.ponPort, ENABLED);
    if(APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        ret = rtk_svlan_lookupType_set(SVLAN_LOOKUP_C4KVLAN);
    }
    omci_cfg_vlan_table(0);
    rtk_svlan_untagAction_set(SVLAN_ACTION_SVLAN, 0);*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_ERR, omci_init());
    rtk_rg_gpon_evtHdlPloam_reg(omci_ploam_callback);
    //rtk_rg_gpon_activate(RTK_GPONMAC_INIT_STATE_O1);
    return 0;
}


static int pf_rtl96xx_ExitPlatform(void)
{
    rtk_rg_gpon_deActivate();
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, omci_exit());
    rtk_rg_gpon_evtHdlPloam_dreg();
    return 0;
}

static int pf_rtl96xx_SetDscpRemap(OMCI_DSCP2PBIT_ts *pDscp2PbitTable)
{
    int i;
    rtk_rg_qos_priSelWeight_t    weight;

    /*Set DSCP remapping to Pbit table*/
    for(i = 0; i < OMCI_DSCP_NUM; i++)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qosDscpRemapToInternalPri_set(i, pDscp2PbitTable->pbit[i]));
    }

    /*Disable 1p remarking*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qosDot1pPriRemarkByInternalPriEgressPortEnable_set(RTK_RG_MAC_PORT_PON, RTK_RG_DISABLED));

    /*Disable dscp remark, keep original DSCP*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qosDscpRemarkEgressPortEnableAndSrcSelect_set(RTK_RG_MAC_PORT_PON, RTK_RG_DISABLED, RTK_RG_DSCP_RMK_SRC_INT_PRI));

    /*Set DSCP weight higher than port-based/dot1q in group 0*/
    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qosInternalPriDecisionByWeight_get(&weight));

    if (weight.weight_of_dot1q >= weight.weight_of_portBased)
        weight.weight_of_dscp = weight.weight_of_dot1q;
    else
        weight.weight_of_dscp = weight.weight_of_portBased;
    if (RTK_PRI_SEL_WEIGHT_MAX > weight.weight_of_dscp)
        weight.weight_of_dscp += 1;

    OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_qosInternalPriDecisionByWeight_set(weight));
    /*Set priority-selector group of WAN port*/
    // TBD, RG not support multiple groups
    //rtk_qos_portPriSelGroup_set(4, 0);

    return 0;
}

static int pf_rtl96xx_SetMacLearnLimit(OMCI_MACLIMIT_ts *pMacLimit)
{
    rtk_rg_saLearningLimitInfo_t    info;
    rtk_switch_devInfo_t            tDevInfo;
    rtk_rg_port_idx_t               port_id;
    int32                           ret;
    if (!pMacLimit)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_switch_deviceInfo_get(&tDevInfo))
        return RT_ERR_FAILED;

    memset(&info, 0, sizeof(rtk_rg_saLearningLimitInfo_t));

    port_id = pMacLimit->portIdx;

    info.action = SA_LEARN_EXCEED_ACTION_DROP;
    info.learningLimitNumber = (pMacLimit->macLimitNum == tDevInfo.capacityInfo.l2_learn_limit_cnt_max) ? -1 : ((int)(pMacLimit->macLimitNum));
    if (RT_ERR_OK != (ret = rtk_rg_softwareSourceAddrLearningLimit_set(info, port_id)))
    {
        printk("%s(): action=%u, limitNo=%d, port_id=%u, ret=%u\n",__FUNCTION__, info.action, info.learningLimitNumber, port_id, ret);
        return RT_ERR_FAILED;
    }
    return 0;

}

static int pf_rtl96xx_SetMacFilter(OMCI_MACFILTER_ts *pMacFilter)
{
    /*TBD*/

    return 0;
}

static int pf_rtl96xx_GetDevCapabilities(omci_dev_capability_t *p)
{
    rtk_switch_devInfo_t    tDevInfo;
    uint8                   portId;
    uint32                  r;
    uint8                   totalUniPort = 0;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_switch_deviceInfo_get(&tDevInfo))
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
    p->meterNum = 0;

    // learn non-UNI ports info
    p->cpuPort = tDevInfo.cpuPort;
    p->rgmiiPort = tDevInfo.rgmiiPort;
    if (RT_ERR_OK != rtk_rg_switch_phyPortId_get(RTK_PORT_PON, &p->ponPort))
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
                OMCI_LOG(OMCI_LOG_LEVEL_WARN, "FEATURE_KAPI_ME_00000001 is not enabled or exec failed...");
    }

    // learn other capabilities
    p->totalTContNum = gPlatformDb.maxTcont - 1;

    if( OMCIDRV_FEATURE_ERR_OK != omcidrv_feature_api(FEATURE_KAPI_ME_00100000, p))
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "FEATURE_KAPI_ME_00100000 is not enabled or exec failed...");

    p->totalGEMPortNum = gPlatformDb.maxFlow - 1;
    p->totalTContQueueNum =  ((p->totalTContNum) * 8);//gPlatformDb.maxPonQueue - 1;
    p->perUNIQueueNum = gPlatformDb.perUniQueue;
    p->meterNum = gPlatformDb.meterNum;
    p->rsvMeterId = gPlatformDb.rsvMeterId;
    p->totalL2Num = tDevInfo.capacityInfo.l2_learn_limit_cnt_max;

    get_resouce_by_dev_feature(DEV_FEATURE_ALL, p);
    get_resouce_by_dev_feature(DEV_FEATURE_ETH, p);

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetDevIdVersion(omci_dev_id_version_t *p)
{
    uint32  chipId = 0;
    uint32  rev = 0;
    uint32  subType = 0;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_switch_version_get(&chipId, &rev, &subType))
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
        case RTL9607C_CHIP_ID:
            snprintf(p->id, OMCI_DRV_DEV_ID_LEN, "RTL9607C");
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

    if (RT_ERR_OK != rtk_rg_gpon_parameter_get(RTK_GPON_PARA_TYPE_US_DBR, &gponPara))
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

    if (RT_ERR_OK != rtk_rg_ponmac_transceiver_get(type, &data))
        return RT_ERR_FAILED;

    if (sizeof(data.buf) < sizeof(p->data))
        memcpy(p->data, data.buf, sizeof(data.buf));
    else
        memcpy(p->data, data.buf, sizeof(p->data));

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortLinkStatus(omci_port_link_status_t *p)
{
    rtk_rg_portStatusInfo_t     portInfo;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_portStatus_get(p->port, &portInfo)) {
        printk ("%s: Failed to get port %u\n", __FUNCTION__, p->port);
        return RT_ERR_FAILED;
    }

    p->status = (RTK_RG_PORT_LINKUP == portInfo.linkStatus) ? TRUE : FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortSpeedDuplexStatus(omci_port_speed_duplex_status_t *p)
{
    rtk_rg_portStatusInfo_t     portInfo;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_portStatus_get(p->port, &portInfo))
        return RT_ERR_FAILED;

    switch (portInfo.linkSpeed)
    {
        case RTK_RG_PORT_SPEED_10M:
            p->speed = OMCI_PORT_SPEED_10M;
            break;
        case RTK_RG_PORT_SPEED_100M:
            p->speed = OMCI_PORT_SPEED_100M;
            break;
        case RTK_RG_PORT_SPEED_1000M:
            p->speed = OMCI_PORT_SPEED_1000M;
            break;
        default:
            return RT_ERR_FAILED;
            break;
    }

    switch (portInfo.linkDuplex)
    {
        case RTK_RG_PORT_HALF_DUPLEX:
            p->duplex = OMCI_PORT_HALF_DUPLEX;
            break;
        case RTK_RG_PORT_FULL_DUPLEX:
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
    rtk_rg_phyPortAbilityInfo_t     phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if (RT_ERR_OK != rtk_rg_phyPortForceAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    if (p->full_10 && p->full_100 && p->full_1000 &&
            p->half_10 && p->half_100 && p->half_1000)
    {
        // RG API do check parameters even it's useless...
        phyAbility.duplex = RTK_RG_PORT_FULL_DUPLEX;
        phyAbility.speed = RTK_RG_PORT_SPEED_100M;

        phyAbility.valid = RTK_RG_DISABLED;
    }
    else
    {
        phyAbility.valid = RTK_RG_ENABLED;

        if (p->full_10 || p->half_10)
            phyAbility.speed = RTK_RG_PORT_SPEED_10M;

        if (p->full_100 || p->half_100)
            phyAbility.speed = RTK_RG_PORT_SPEED_100M;

        if (p->full_1000 || p->half_1000)
            phyAbility.speed = RTK_RG_PORT_SPEED_1000M;

        if (p->full_10 || p->full_100 || p->full_1000)
            phyAbility.duplex = RTK_RG_PORT_FULL_DUPLEX;
        else
        {
            phyAbility.duplex = RTK_RG_PORT_HALF_DUPLEX;
            phyAbility.flowCtrl = RTK_RG_DISABLED;
        }
    }

    if (RT_ERR_OK != rtk_rg_phyPortForceAbility_set(p->port, phyAbility))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortAutoNegoAbility(omci_port_auto_nego_ability_t *p)
{
    rtk_rg_phyPortAbilityInfo_t     phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_phyPortForceAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    if (RTK_RG_DISABLED == phyAbility.valid)
    {
        p->full_10 = TRUE;
        p->half_10 = TRUE;
        p->full_100 = TRUE;
        p->half_100 = TRUE;
        p->full_1000 = TRUE;
        p->half_1000 = TRUE;
    }
    else
    {
        p->full_10 = FALSE;
        p->half_10 = FALSE;
        p->full_100 = FALSE;
        p->half_100 = FALSE;
        p->full_1000 = FALSE;
        p->half_1000 = FALSE;

        switch (phyAbility.speed)
        {
            case RTK_RG_PORT_SPEED_10M:
                if (RTK_RG_PORT_FULL_DUPLEX == phyAbility.duplex)
                    p->full_10 = TRUE;
                else
                    p->half_10 = TRUE;
                break;
            case RTK_RG_PORT_SPEED_100M:
                if (RTK_RG_PORT_FULL_DUPLEX == phyAbility.duplex)
                    p->full_100 = TRUE;
                else
                    p->half_100 = TRUE;
                break;
            case RTK_RG_PORT_SPEED_1000M:
                if (RTK_RG_PORT_FULL_DUPLEX == phyAbility.duplex)
                    p->full_1000 = TRUE;
                else
                    p->half_1000 = TRUE;
                break;
            default:
                return RT_ERR_FAILED;
                break;
        }
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortState(omci_port_state_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;
    // TBD

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortState(omci_port_state_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    // TBD

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    // mtu modification is not supported

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortMaxFrameSize(omci_port_max_frame_size_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_switch_maxPktLenByPort_get(p->port, &p->size))
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

    if (RT_ERR_OK != rtk_rg_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (p->loopback)
        data |= Loopback_MASK;
    else
        data &= ~Loopback_MASK;

    if (RT_ERR_OK != rtk_rg_port_phyReg_set(p->port, PHY_PAGE_0, PHY_CONTROL_REG, data))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortPhyLoopback(omci_port_loopback_t *p)
{
    uint32  data;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (data & Loopback_MASK)
        p->loopback = TRUE;
    else
        p->loopback = FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    uint32  data;

    if (!p)
        return RT_ERR_FAILED;

    if (gDrvCtrl.omciMirrorState && gDrvCtrl.omciMirroringPort == p->port)
        return RT_ERR_OK;

    if (RT_ERR_OK != rtk_rg_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (p->state)
        data |= PowerDown_MASK;
    else
        data &= ~PowerDown_MASK;

    if (RT_ERR_OK != rtk_rg_port_phyReg_set(p->port, PHY_PAGE_0, PHY_CONTROL_REG, data))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortPhyPwrDown(omci_port_pwr_down_t *p)
{
    uint32  data;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_port_phyReg_get(p->port, PHY_PAGE_0, PHY_CONTROL_REG, &data))
        return RT_ERR_FAILED;

    if (data & PowerDown_MASK)
        p->state = TRUE;
    else
        p->state = FALSE;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPortStat(omci_port_stat_t *p)
{
    rtk_rg_port_mib_info_t  mibInfo;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_portMibInfo_get(p->port, &mibInfo))
        return RT_ERR_FAILED;

    p->ifInOctets                       = mibInfo.ifInOctets;
    p->ifInUcastPkts                    = mibInfo.ifInUcastPkts;
    p->ifInMulticastPkts                = mibInfo.ifInMulticastPkts;
    p->ifInBroadcastPkts                = mibInfo.ifInBroadcastPkts;
    p->ifInDiscards                     = mibInfo.ifInDiscards;
    p->ifOutOctets                      = mibInfo.ifOutOctets;
    p->ifOutUcastPkts                   = mibInfo.ifOutUcastPkts;
    p->ifOutMulticastPkts               = mibInfo.ifOutMulticastPkts;
    p->ifOutBrocastPkts                 = mibInfo.ifOutBrocastPkts;
    p->ifOutDiscards                    = mibInfo.ifOutDiscards;
    p->dot1dTpPortInDiscards            = mibInfo.dot1dTpPortInDiscards;
    p->dot3StatsSingleCollisionFrames   = mibInfo.dot3StatsSingleCollisionFrames;
    p->dot3StatsMultipleCollisionFrames = mibInfo.dot3StatsMultipleCollisionFrames;
    p->dot3StatsDeferredTransmissions   = mibInfo.dot3StatsDeferredTransmissions;
    p->dot3StatsLateCollisions          = mibInfo.dot3StatsLateCollisions;
    p->dot3StatsExcessiveCollisions     = mibInfo.dot3StatsExcessiveCollisions;
    p->dot3InPauseFrames                = mibInfo.dot3InPauseFrames;
    p->dot3OutPauseFrames               = mibInfo.dot3OutPauseFrames;
    p->dot3StatsAligmentErrors          = mibInfo.dot3StatsAligmentErrors;
    p->dot3StatsFCSErrors               = mibInfo.dot3StatsFCSErrors;
    p->dot3StatsSymbolErrors            = mibInfo.dot3StatsSymbolErrors;
    p->dot3StatsFrameTooLongs           = mibInfo.dot3StatsFrameTooLongs;
    p->etherStatsDropEvents             = mibInfo.etherStatsDropEvents;
    p->etherStatsFragments              = mibInfo.etherStatsFragments;
    p->etherStatsJabbers                = mibInfo.etherStatsJabbers;
    p->etherStatsCRCAlignErrors         = mibInfo.etherStatsCRCAlignErrors;
    p->etherStatsTxUndersizePkts        = mibInfo.etherStatsTxUndersizePkts;
    p->etherStatsTxOversizePkts         = mibInfo.etherStatsTxOversizePkts;
    p->etherStatsTxPkts64Octets         = mibInfo.etherStatsTxPkts64Octets;
    p->etherStatsTxPkts65to127Octets    = mibInfo.etherStatsTxPkts65to127Octets;
    p->etherStatsTxPkts128to255Octets   = mibInfo.etherStatsTxPkts128to255Octets;
    p->etherStatsTxPkts256to511Octets   = mibInfo.etherStatsTxPkts256to511Octets;
    p->etherStatsTxPkts512to1023Octets  = mibInfo.etherStatsTxPkts512to1023Octets;
    p->etherStatsTxPkts1024to1518Octets = mibInfo.etherStatsTxPkts1024to1518Octets;
    p->etherStatsTxPkts1519toMaxOctets  = mibInfo.etherStatsTxPkts1519toMaxOctets;
    p->etherStatsTxCRCAlignErrors       = mibInfo.etherStatsTxCRCAlignErrors;
    p->etherStatsRxUndersizePkts        = mibInfo.etherStatsRxUndersizePkts;
    p->etherStatsRxOversizePkts         = mibInfo.etherStatsRxOversizePkts;
    p->etherStatsRxPkts64Octets         = mibInfo.etherStatsRxPkts64Octets;
    p->etherStatsRxPkts65to127Octets    = mibInfo.etherStatsRxPkts65to127Octets;
    p->etherStatsRxPkts128to255Octets   = mibInfo.etherStatsRxPkts128to255Octets;
    p->etherStatsRxPkts256to511Octets   = mibInfo.etherStatsRxPkts256to511Octets;
    p->etherStatsRxPkts512to1023Octets  = mibInfo.etherStatsRxPkts512to1023Octets;
    p->etherStatsRxPkts1024to1518Octets = mibInfo.etherStatsRxPkts1024to1518Octets;
    p->etherStatsRxPkts1519toMaxOctets  = mibInfo.etherStatsRxPkts1519toMaxOctets;

    return RT_ERR_OK;
}

static int pf_rtl96xx_ResetPortStat(unsigned int port)
{
    if (RT_ERR_OK != rtk_rg_portMibInfo_clear(port))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetUsFlowStat(omci_flow_stat_t *p)
{
    rtk_gpon_flow_counter_t     flowCntrs;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_gpon_flowCounter_get(p->flow,
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

    if (RT_ERR_OK != rtk_rg_gpon_flowCounter_get(p->flow,
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

    if (RT_ERR_OK != rtk_rg_gpon_globalCounter_get(
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

    // TBD, RG not support SVLAN filtering
    if (RT_ERR_OK != rtk_rg_svlanTpid_set(p->tpid))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetSvlanTpid(omci_svlan_tpid_t *p)
{
    uint32  svlanTpid;

    if (gDrvCtrl.devMode == OMCI_DEV_MODE_ROUTER)
        return OMCI_ERR_OK;

    if (!p)
        return RT_ERR_FAILED;

    // TBD, RG not support SVLAN filtering
    if (RT_ERR_OK != rtk_rg_svlanTpid_get(&svlanTpid))
        return RT_ERR_FAILED;

    p->tpid = svlanTpid;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetCvlanState(unsigned int *p)
{

    if (!p)
        return RT_ERR_FAILED;

    // TBD, RG not support CVLAN state get

    *p = RTK_RG_ENABLED;

    return RT_ERR_OK;
}


static int pf_rtl96xx_GetGemBlkLen(unsigned short *pGemBlkLen)
{
    uint32 blockSize = 0;
    if (RT_ERR_OK != rtk_rg_gpon_dbruBlockSize_get(&blockSize))
        return RT_ERR_FAILED;

    *pGemBlkLen = (unsigned short)blockSize;
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetGemBlkLen(unsigned short gemBlkLen)
{
    if (RT_ERR_OK != rtk_rg_gpon_dbruBlockSize_set((uint32)gemBlkLen))
        return RT_ERR_FAILED;

    return RT_ERR_OK;

}

static int pf_rtl96xx_GetDrvVersion(char *drvVersion)
{
#ifdef CONFIG_DRV_RELEASE_VERSION
    snprintf(drvVersion, 64, "%s for Rome Driver", CONFIG_DRV_RELEASE_VERSION);
#endif
    return RT_ERR_OK;
}

static int pf_rtl96xx_GetOnuState(PON_ONU_STATE *pOnuState)
{
    rtk_gpon_onuState_t onuState;

    if (RT_ERR_OK != rtk_rg_gpon_onuState_get(&onuState))
        return RT_ERR_FAILED;

    *pOnuState = (PON_ONU_STATE)onuState;
    return RT_ERR_OK;
}

static int pf_rtl96xx_SetPonBwThreshold(omci_pon_bw_threshold_t *pPonBwThreshold)
{
    if (RT_ERR_OK != rtk_rg_ponmac_bwThreshold_set(pPonBwThreshold->bwThreshold, pPonBwThreshold->reqBwThreshold))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_DumpVeipGemFlow(void)
{
    struct list_head        *pEntry;
    struct list_head        *pTmpEntry;
    veipGemFlow_entry_t     *pEntryData;
    unsigned int            i;
    rtk_ponmac_queue_t      queue;

    list_for_each_safe(pEntry, pTmpEntry, &gPlatformDb.veipGemFlowHead)
    {
        pEntryData = list_entry(pEntry, veipGemFlow_entry_t, list);

        printk(" %10s | %10s | %10s | %13s | %10s \n",
            "gemPortId", "tcontId", "flowId", "tcQueueSeqId", "ponQueueId");

        for (i = 0; i < gDrvCtrl.wanQueueNum; i++)
        {
            if (RT_ERR_OK != rtk_rg_ponmac_flow2Queue_get(pEntryData->data.flowId[i], &queue))
            {
                printk("cannot get flow [%u]'s pon queue id!!\n", pEntryData->data.flowId[i]);
                continue;
            }
            printk(" %10u | %10u | %10u | %13u | %10u \n",
                pEntryData->data.gemPortId,
                pEntryData->data.tcontId,
                pEntryData->data.flowId[i],
                pEntryData->data.tcQueueId[i],
                queue.queueId);
        }

        printk("######################################################################\n");
    }

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_SetVeipGemFlow(veipGemFlow_t *p)
{
    veipGemFlow_entry_t     *pEntry;

    if (!p)
        return RT_ERR_FAILED;

    pEntry = veipGemFlow_entry_find(p->gemPortId);
    if (pEntry)
        memcpy(&pEntry->data, p, sizeof(veipGemFlow_t));
    else
    {
        if (OMCI_ERR_OK != veipGemFlow_entry_add(p))
            return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_DelVeipGemFlow(veipGemFlow_t *p)
{
    veipGemFlow_entry_t     *pEntry;

    if (!p)
        return RT_ERR_FAILED;

    pEntry = veipGemFlow_entry_find(p->gemPortId);
    if (!pEntry)
        return RT_ERR_FAILED;

    if (OMCI_ERR_OK != veipGemFlow_entry_del(p->gemPortId))
        return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetUniPortRate(omci_port_rate_t *p)
{
    if (!p)
        return RT_ERR_FAILED;

    if (OMCI_UNI_RATE_DIRECTION_EGRESS == p->dir)
    {
        if (RT_ERR_OK != rtk_rg_rate_portEgrBandwidthCtrlRate_set(p->port, p->rate))
            return RT_ERR_FAILED;
    }
    else
    {
        if (RT_ERR_OK != rtk_rg_rate_portIgrBandwidthCtrlRate_set(p->port, p->rate))
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

    if (RT_ERR_OK != rtk_rg_port_phyAutoNegoAbility_get(p->port, &phyAbility))
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

    if (RT_ERR_OK != rtk_rg_port_phyAutoNegoAbility_set(p->port, &phyAbility))
            return RT_ERR_FAILED;

    return RT_ERR_OK;
}

static int pf_rtl96xx_GetPauseControl(omci_port_pause_ctrl_t *p)
{
    rtk_port_phy_ability_t  phyAbility;

    if (!p)
        return RT_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_port_phyAutoNegoAbility_get(p->port, &phyAbility))
        return RT_ERR_FAILED;

    p->pause_time = phyAbility.FC;

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetDsBcGemFlow(unsigned int *pFlowId)
{
    rtk_rg_enable_t enable;
    int32               ret = RT_ERR_RG_OK;

    if (pDmmCb && (pDmmCb->omci_dm_ds_mc_bc_info_del && pDmmCb->omci_dm_ds_mc_bc_info_set))
        return ret;

    if (!pFlowId)
        return RT_ERR_INPUT;

    if (*pFlowId < gPlatformDb.maxFlow &&
            *pFlowId != gPlatformDb.omccFlow)
        enable = RTK_RG_ENABLED;
    else
        enable = RTK_RG_DISABLED;

    ret = rtk_rg_gponDsBcFilterAndRemarking_Enable(enable);
    if (RT_ERR_RG_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "%s gpon ds bc filter and remarking failed, ret %d",
            (RTK_RG_ENABLED == enable) ? "enable" : "disable", ret);
    }

    return ret;
}

static int pf_rtl96xx_SetDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    rtk_rg_err_code_t               retRgErr;
    int32                           ret;
    rtk_rg_stormControlInfo_t       stormCtrl;
    int                             stormCtrlIdx;
    unsigned int                    cirInKbps;
    unsigned int                    portId;

    if (!pDot1RateMeter)
        return RT_ERR_INPUT;

    if (pDot1RateMeter->dot1Rate.type >= OMCI_DOT1_RATE_TYPE_END)
        return RT_ERR_INPUT;

    ret = rtk_rg_rate_shareMeterMode_set(pDot1RateMeter->meterId, METER_MODE_BIT_RATE);
    if (RT_ERR_OK != ret && RT_ERR_DRIVER_NOT_FOUND != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "set shared meter mode failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    ret = rtk_rg_rate_shareMeterBucket_set(pDot1RateMeter->meterId, pDot1RateMeter->dot1Rate.cbs);
    if (RT_ERR_OK != ret && RT_ERR_INPUT != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "set shared meter bucket failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    cirInKbps = pDot1RateMeter->dot1Rate.cir * 8 / 1024;

    retRgErr = rtk_rg_shareMeter_set(pDot1RateMeter->meterId, cirInKbps, RTK_RG_DISABLED);
    if (RT_ERR_RG_OK != retRgErr)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "set rg shared meter rate failed, ret %d", retRgErr);

        return RT_ERR_RG_FAILED;
    }

    switch (pDot1RateMeter->dot1Rate.type)
    {
        case OMCI_DOT1_RATE_UNICAST_FLOOD:
            stormCtrl.stormType = RTK_RG_STORM_TYPE_UNKNOWN_UNICAST;
            break;
        case OMCI_DOT1_RATE_BROADCAST:
            stormCtrl.stormType = RTK_RG_STORM_TYPE_BROADCAST;
            break;
        case OMCI_DOT1_RATE_MULTICAST_PAYLOAD:
            stormCtrl.stormType = RTK_RG_STORM_TYPE_MULTICAST;
            break;
        default:
            stormCtrl.stormType = RTK_RG_STORM_TYPE_END;
            break;
    }

    stormCtrl.valid = RTK_RG_ENABLED;
    stormCtrl.meterIdx = pDot1RateMeter->meterId;

    for (portId = 0; portId < RTK_MAX_NUM_OF_PORTS; portId++)
    {
        if (!((1 << portId) & pDot1RateMeter->dot1Rate.portMask))
            continue;

        stormCtrl.port = portId;

        retRgErr = rtk_rg_stormControl_add(&stormCtrl, &stormCtrlIdx);
        if (RT_ERR_RG_OK != retRgErr)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "add rg storm control failed, ret %d", retRgErr);

            return RT_ERR_RG_FAILED;
        }

        if (stormCtrlIdx >= gPlatformDb.meterNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "rg storm control index out of range");

            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_stormControl_del(stormCtrlIdx));
            return RT_ERR_RG_FAILED;
        }

        gPlatformDb.stormCtrl[stormCtrlIdx].isCfg = TRUE;
        gPlatformDb.stormCtrl[stormCtrlIdx].meterIdx = stormCtrl.meterIdx;
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_DelDot1RateLimiter(omci_dot1_rate_meter_t *pDot1RateMeter)
{
    int32                   ret;
    unsigned int            stormCtrlIdx;

    if (!pDot1RateMeter)
        return RT_ERR_INPUT;

    for (stormCtrlIdx = 0; stormCtrlIdx < gPlatformDb.meterNum; stormCtrlIdx++)
    {
        if (gPlatformDb.rsvMeterId == stormCtrlIdx)
            continue;
        if (gPlatformDb.stormCtrl[stormCtrlIdx].isCfg != TRUE)
            continue;

        if (gPlatformDb.stormCtrl[stormCtrlIdx].meterIdx != pDot1RateMeter->meterId)
            continue;

        ret = rtk_rg_stormControl_del(stormCtrlIdx);
        if (RT_ERR_RG_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "del rg storm control failed, ret %d", ret);

            return RT_ERR_RG_FAILED;
        }
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetMacAgeTime(unsigned int ageTime)
{
    return  rtk_rg_softwareIdleTime_set(RG_IDLETIME_TYPE_LUT, ageTime);
}


static int pf_rtl96xx_SetLoidAuthStatus(omci_event_msg_t *p)
{
    uint8 authStatus = p->status;

    if((authStatus != PON_ONU_LOID_INITIAL_STATE) && (authStatus != PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION))
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_pon_led_status_set(PON_LED_PON_MODE_GPON, PON_LED_STATE_AUTH_NG));
    }
    else if(authStatus == PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION)
    {
        OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_pon_led_status_set(PON_LED_PON_MODE_GPON, PON_LED_STATE_AUTH_OK));

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

    ret = rtk_gpon_forceEmergencyStop_set(*pState);
    if (RT_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "force emergency stop failed, ret %d", ret);

        return RT_ERR_FAILED;
    }

    return RT_ERR_OK;
}

static int pf_rtl96xx_SetTodInfo ( omci_tod_info_t* pTodInfo)
{
    int32 ret = RT_ERR_FAILED;
    rtk_rg_enable_t enable = DISABLED;
    rtk_pon_tod_t ponTod;
    unsigned long long uTimeStampSecs;

    if (gPlatformDb.chipId == RTL9601B_CHIP_ID ||
        gPlatformDb.chipId == APOLLOMP_CHIP_ID) {
        return (RT_ERR_INPUT);
    }

    if (!pTodInfo) {
        return (RT_ERR_INPUT);
    }

    memset(&ponTod, 0x00, sizeof(rtk_pon_tod_t));

    /* Avoid compile warning */
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
    enable = RTK_RG_ENABLED;
    ret = rtk_time_ponTodTime_set (ponTod);
    if (ret != RT_ERR_OK) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
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

static void _debug_cf_db_info(void)
{
    int i;
    rtk_rg_classifyEntry_t *p = NULL;

    printk("\n\n");
    printk("============= used cf_db ===============\n");

    for (i = 0; i < gPlatformDb.cfNum; i++)
    {
        if (0 == gPlatformDb.cfRule[i].isCfg)
        {
            if (gPlatformDb.cfRule[i].classifyCfg)
                printk("[ERR][NO_FREE_MEM] CF RULE [%d]\n", i);
        }
        else
        {
            if ((p = gPlatformDb.cfRule[i].classifyCfg))
            {
                printk("cf_db_id [%d] cf_rule_id [%d] \n", i, p->index);
            }
            else
            {
                printk("[ERR][NO_CFG] CF RULE [%d]\n", i);
            }
        }

    }
}

static int pf_rtl96xx_DumpDebugInfo(void)
{
    _debug_dump_tcont_info();
    _debug_cf_db_info();

    return OMCI_ERR_OK;
}

static int pf_rtl96xx_ClearPPPoEDb(void)
{
    int32   ret = OMCI_ERR_OK;

    if (APOLLOMP_CHIP_ID == gPlatformDb.chipId)
    {
        ret = gpon_pppoe_db_clear();
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d \n", __FUNCTION__, __LINE__);
    }
    return ret;
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

static void gpon_state_change(intrBcasterMsg_t *pMsgData)
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
            if(pMsgData->intrSubType == GPON_STATE_O5)
            {
                OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_rg_ponmac_bwThreshold_set(21, 14));
            }
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
    .pf_ActiveBdgConn       =pf_rtl96xx_ActiveBdgConn,
    .pf_DeactiveBdgConn     =pf_rtl96xx_DeactiveBdgConn,
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
    .pf_DumpMacFilter               = NULL,
    .pf_DumpFlow2dsPq               = NULL,
    .pf_SetGroupMacFilter           = NULL,
    .pf_GetDrvVersion               = pf_rtl96xx_GetDrvVersion,
    .pf_GetOnuState                 = pf_rtl96xx_GetOnuState,
    .pf_SetSigParameter             = NULL,
    .pf_SetPonBwThreshold           = pf_rtl96xx_SetPonBwThreshold,
    .pf_DumpVeipGemFlow             = pf_rtl96xx_DumpVeipGemFlow,
    .pf_SetVeipGemFlow              = pf_rtl96xx_SetVeipGemFlow,
    .pf_DelVeipGemFlow              = pf_rtl96xx_DelVeipGemFlow,
    .pf_SetUniPortRate              = pf_rtl96xx_SetUniPortRate,
    .pf_SetPauseControl             = pf_rtl96xx_SetPauseControl,
    .pf_GetPauseControl             = pf_rtl96xx_GetPauseControl,
    .pf_SetDsBcGemFlow              = pf_rtl96xx_SetDsBcGemFlow,
    .pf_SetDot1RateLimiter          = pf_rtl96xx_SetDot1RateLimiter,
    .pf_DelDot1RateLimiter          = pf_rtl96xx_DelDot1RateLimiter,
    .pf_GetBgTblPerPort             = NULL,
    .pf_SetMacAgeTime               = pf_rtl96xx_SetMacAgeTime,
    .pf_SetLoidAuthStatus           = pf_rtl96xx_SetLoidAuthStatus,
    .pf_SendOmciEvent               = pf_rtl96xx_SendOmciEvent,
    .pf_SetForceEmergencyStop       = pf_rtl96xx_SetForceEmergencyStop,
    .pf_SetPortBridging             = NULL,
    .pf_SetFloodingPortMask         = NULL,
    .pf_SetTodInfo                  = pf_rtl96xx_SetTodInfo,
    .pf_SetUniQosInfo               = NULL,
    .pf_DumpUniQos                  = NULL,
    .pf_DumpDebugInfo               = pf_rtl96xx_DumpDebugInfo,
    .pf_ClearPPPoEDb                = pf_rtl96xx_ClearPPPoEDb,
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
    unsigned int                wifi_ext_port_max_num = 0, wifi_ext_port_cnt = 0;
    rtk_rg_initParams_t         rgInitDb;

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


    INIT_LIST_HEAD(&ponWanPfInfoHead);

    if (RT_ERR_OK != rtk_rg_gpon_scheInfo_get(&scheInfo))
        return OMCI_ERR_FAILED;

    if (RT_ERR_OK != rtk_rg_switch_deviceInfo_get(&devInfo))
        return OMCI_ERR_FAILED;
    pCapacityInfo = &devInfo.capacityInfo;

    // chip info
    gPlatformDb.chipId = devInfo.chipId;

    if (RT_ERR_OK != rtk_rg_switch_version_get(&cid, &rid, &stype))
        return OMCI_ERR_FAILED;
    // chip revision
    gPlatformDb.chipRev = rid;

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

    gPlatformDb.cpuPortMask = devInfo.cpu.portmask.bits [0];
    gPlatformDb.rgmiiPort = devInfo.rgmiiPort;
    if (RT_ERR_OK != rtk_rg_gpon_port_get((rtk_port_t *)&gPlatformDb.ponPort))
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
    gPlatformDb.uniPortMask |= gPlatformDb.cpuPortMask;
    gPlatformDb.perUniQueue = pCapacityInfo->max_num_of_queue;
    if (devInfo.ext.portNum > 0)
    {
        wifi_ext_port_max_num = 2;
        for (i = devInfo.ext.min; i <= devInfo.ext.max; i++)
        {
            if (wifi_ext_port_max_num <= wifi_ext_port_cnt)
            {
                break;
            }

            wifi_ext_port_cnt++;

            if (RTL9607C_CHIP_ID != gPlatformDb.chipId)
            {
                // plus 1 for offset EXT_CPU
                gPlatformDb.extPortMask |= (1 << (i + gPlatformDb.cpuPort + 1));
                continue;
            }
            //
            // RTL9607C
            //
            gPlatformDb.extPortMask |= (1 << (1 + gPlatformDb.cpuPort + 2));
            gPlatformDb.extPortMask |= (1 << (1 + gPlatformDb.cpuPort + 8));
            break;
        }
    }
    // qos
    gPlatformDb.maxPriSelWeight = pCapacityInfo->pri_sel_weight_max;
    gPlatformDb.perTContQueueDp = FALSE;
    gPlatformDb.perUNIQueueDp = FALSE;
    gPlatformDb.rsvMeterId = ((gPlatformDb.cpuPort % utpPortNum) * 8 + 7);
    gPlatformDb.meterNum = pCapacityInfo->max_num_of_metering;
    gPlatformDb.stormCtrl = kzalloc(sizeof(pf_stormCtrl_t) * gPlatformDb.meterNum, GFP_KERNEL);
    if (!gPlatformDb.stormCtrl)
        return OMCI_ERR_FAILED;

    // cf
    gPlatformDb.cfTotalNum = pCapacityInfo->classify_entry_max;
    gPlatformDb.cfNum = gPlatformDb.cfTotalNum;
    gPlatformDb.cfRule = kzalloc(sizeof(pf_cf_rule_t) * gPlatformDb.cfNum, GFP_KERNEL);
    if (!gPlatformDb.cfRule)
        return OMCI_ERR_FAILED;
    switch (gPlatformDb.chipId)
    {
        case RTL9601B_CHIP_ID:
            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 31;
            gPlatformDb.ethTypeFilterStart  = 32;
            gPlatformDb.ethTypeFilterStop   = 63;
            gPlatformDb.l2CommStart         = 64;
            gPlatformDb.l2CommStop          = 255;
            break;
        case RTL9602C_CHIP_ID:
        case RTL9607C_CHIP_ID:
        {
            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 127;
            /*For 9602C, etherType and L2 common use same CF range*/
            gPlatformDb.ethTypeFilterStart  = 128;
            gPlatformDb.ethTypeFilterStop   = 255;
            gPlatformDb.l2CommStart         = 128;
            gPlatformDb.l2CommStop          = 255;

            /*TBD: Check with RG*/
            OMCI_DRV_ERR_CHK(OMCI_LOG_LEVEL_WARN, rtk_classify_unmatchAction_ds_set(CLASSIFY_UNMATCH_DS_PERMIT));
            break;
        }
        case APOLLOMP_CHIP_ID:
        default:
            gPlatformDb.veipFastStart       = 0;
            gPlatformDb.veipFastStop        = 63;
            gPlatformDb.ethTypeFilterStart  = 64;
            gPlatformDb.ethTypeFilterStop   = 127;
            gPlatformDb.l2CommStart         = 128;
            gPlatformDb.l2CommStop          = 511;
            break;
    }

    // acl
    gPlatformDb.aclNum = pCapacityInfo->max_num_of_acl_rule_entry;
    gPlatformDb.aclActNum = pCapacityInfo->max_num_of_acl_action;

    // l34, intfNum follows sw table size. No mater which chip, SW will handle it if the number of netif is larger than hardware limitation
    gPlatformDb.intfNum = MAX_NETIF_SW_TABLE_SIZE;//pCapacityInfo->l34_netif_table_max;
    gPlatformDb.bindingTblMax = pCapacityInfo->l34_binding_table_max;

    // get rg mac based tag state
    if (RT_ERR_RG_OK == rtk_rg_initParam_get(&rgInitDb))
    {
        gPlatformDb.rgMacBasedTag = rgInitDb.macBasedTagDecision;
        gPlatformDb.rgIvlMcastSupport = rgInitDb.ivlMulticastSupport;
        gPlatformDb.fwdVLAN_CPU = rgInitDb.fwdVLAN_CPU;
        gPlatformDb.fwdVLAN_CPU_SVLAN = rgInitDb.fwdVLAN_CPU_SVLAN;
        printk("\n\n @%s(%d) macBasedTagDecision=[%u], ivlMacast=[%u], fwdVLAN_CPU=[%u], fwdVLAN_CPU_SVLAN=[%u]\n\n",
            __FUNCTION__, __LINE__, gPlatformDb.rgMacBasedTag, gPlatformDb.rgIvlMcastSupport,
            rgInitDb.fwdVLAN_CPU, rgInitDb.fwdVLAN_CPU_SVLAN);
    }

    // callback
    gPlatformDb.pMap = &rtl96xx_mapper;

    return OMCI_ERR_OK;
}

int __init rtk_platform_init(void)
{
    unsigned char isRegistered = FALSE;

    platform_db_init();

    omcidrv_platform_register(&gPlatformDb);
    printk("omci platform attached!\n");

    if (OMCI_ERR_OK != pf_rtl96xx_InitPlatform())
        return OMCI_ERR_FAILED;
    else
        isRegistered = TRUE;

    if(OMCI_ERR_OK != pkt_redirect_kernelApp_sendPkt(PR_KERNEL_UID_BCASTER,
        0, sizeof(unsigned char), &isRegistered))
    {
        printk("%s() %d: send registered command failed !! \n", __FUNCTION__, __LINE__);
    }

    rtk_rg_gpon_callbackExtMsgGetHandle_reg(omci_ioctl_callback);

    if(OMCI_ERR_OK != intr_bcaster_notifier_cb_register(&gponStateChangeNotifier))
    {
        printk("OMCI register bcaster notifier chain Error !! \n");
    }

    return OMCI_ERR_OK;
}

void __exit rtk_platform_exit(void)
{

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

