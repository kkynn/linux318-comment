/*
 * Copyright (C) 2014 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 * Purpose : Definition of OMCI MIB config APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI MIB config APIs
 */

#include "app_basic.h"

#define OMCI_CFG_FILE_MAX_LINE_LEN	1024
#define OMCI_CFG_FILE_LOCATION	"/var/config/omci_mib.cfg"


extern int MIB_TABLE_LAST_INDEX;
extern int MIB_TABLE_TOTAL_NUMBER;


GOS_ERROR_CODE omci_mib_cfg_is_parent_slot_existed(omci_me_instance_t instanceID)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_CARDHOLDER_T  mibCardholder;
    UINT8                   slotId;

    // search if corresponding cardholder slot is exist
    slotId = (instanceID >> 8) & 0xFF;
    if (OMCI_ME_ID_CARDHOLDER_INTEGRATED_SLOT != slotId &&
            OMCI_ME_ID_CARDHOLDER_RESERVED_SLOT != slotId)
    {
        mibCardholder.EntityID = TXC_CARDHLD_SLOT_TYPE_PLUGGABLE | slotId;
        ret = MIB_Get(MIB_TABLE_CARDHOLDER_INDEX, &mibCardholder, sizeof(MIB_TABLE_CARDHOLDER_T));
        if (GOS_OK != ret)
        {
            mibCardholder.EntityID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotId;
            ret = MIB_Get(MIB_TABLE_CARDHOLDER_INDEX, &mibCardholder, sizeof(MIB_TABLE_CARDHOLDER_T));
            if (GOS_OK != ret)
            {
                return GOS_FAIL;
            }
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_onu_g(void *pMibRow)
{
    MIB_TABLE_ONTG_T    *pMibOnuG;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibOnuG = (MIB_TABLE_ONTG_T *)pMibRow;

    // vendor id & serial number are always overwritten by system mib
    memcpy(pMibOnuG->VID, gInfo.sn, MIB_TABLE_ONTG_VID_LEN);
    memcpy(pMibOnuG->SerialNum, gInfo.sn, MIB_TABLE_ONTG_SERIALNUM_LEN);

    // logical onu id & logical password are always overwritten by system mib
    memcpy(pMibOnuG->LogicalOnuID, gInfo.loidCfg.loid, MIB_TABLE_ONTG_LOID_LEN);
    memcpy(pMibOnuG->LogicalPassword, gInfo.loidCfg.loidPwd, MIB_TABLE_ONTG_LP_LEN);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_onu2_g(void *pMibRow)
{
    MIB_TABLE_ONT2G_T   *pMibOnu2G;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibOnu2G = (MIB_TABLE_ONT2G_T *)pMibRow;

    // total (upstream) priority queue number
    pMibOnu2G->NumOfPriQ = gInfo.devCapabilities.totalTContQueueNum;

    // total traffic scheduler number
    pMibOnu2G->NumOfScheduler = gInfo.devCapabilities.totalTContNum;

    // total gem port-id number
    pMibOnu2G->NumOfGemPort = gInfo.devCapabilities.totalGEMPortNum;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_onu_data(void *pMibRow)
{
    MIB_TABLE_ONTDATA_T     *pMibOnuData;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibOnuData = (MIB_TABLE_ONTDATA_T *)pMibRow;

    // mib data sync
    pMibOnuData->MIBDataSync = 0;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_sw_img(void *pMibRow)
{
    MIB_TABLE_SWIMAGE_T     *pMibSwImg;
    char                    flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                    flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    char                    *pStr;
    UINT8                   commitLater = 2;
    UINT8                   activeImage = 0;
    UINT8                   commitImage = 0;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibSwImg = (MIB_TABLE_SWIMAGE_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibSwImg->EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for software image 0x%x is not found", pMibSwImg->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    // check if sw_commit_later is exist and its value
    sprintf(flagName, "sw_commit_later");
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        commitLater = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    // check if sw_active is exist and its value
    sprintf(flagName, "sw_active");
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        activeImage = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    if (commitLater < 2)
    {
        // rewrite sw_commit flag
        if (commitLater == activeImage)
        {
            sprintf(flagName, "sw_commit");
            sprintf(flagBuffer, "%u", commitLater);
            if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: modify sw_commit fail");
            }
        }

        // erase sw_commit_later flag
        sprintf(flagName, "sw_commit_later");
        sprintf(flagBuffer, "%s", "");
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: delete sw_commit_later fail");
        }
    }

    // check if sw_commit is exist and its value
    sprintf(flagName, "sw_commit");
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        commitImage = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    pMibSwImg->Committed =
        (pMibSwImg->EntityID == commitImage) ? TRUE : FALSE;
    pMibSwImg->Active =
        (pMibSwImg->EntityID == activeImage) ? TRUE : FALSE;

    // check if sw_version is exist and its value
    sprintf(flagName, "sw_version%u", pMibSwImg->EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        snprintf(pMibSwImg->Version, MIB_TABLE_SWIMAGE_VERSION_LEN, "%s", pStr);

        free(pStr);
    }
    else
    {
        sprintf(pMibSwImg->Version, "%s", "");
    }

    // check if sw_valid is exist and its value
    sprintf(flagName, "sw_valid%u", pMibSwImg->EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        pMibSwImg->Valid = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    // make sure that an activated image must be also validated image
    if (pMibSwImg->Active && pMibSwImg->Valid != pMibSwImg->Active)
    {
        pMibSwImg->Valid = pMibSwImg->Active;

        sprintf(flagBuffer, "%u", pMibSwImg->Valid);
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: modify %s fail", flagName);
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_cardholder(void *pMibRow)
{
 //   MIB_TABLE_CARDHOLDER_T  *pMibCardholder;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibCardholder = (MIB_TABLE_CARDHOLDER_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_circuit_pack(void *pMibRow)
{
    GOS_ERROR_CODE              ret;
    MIB_TABLE_CARDHOLDER_T      mibCardholder;
    MIB_TABLE_CIRCUITPACK_T     *pMibCircuitPack;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibCircuitPack = (MIB_TABLE_CIRCUITPACK_T *)pMibRow;

    // search if corresponding cardholder is exist
    mibCardholder.EntityID = pMibCircuitPack->EntityID;
    ret = MIB_Get(MIB_TABLE_CARDHOLDER_INDEX, &mibCardholder, sizeof(MIB_TABLE_CARDHOLDER_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for circuit pack 0x%x is not found", pMibCircuitPack->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_onu_pwr_shedding(void *pMibRow)
{
//    MIB_TABLE_ONU_PWR_SHEDDING_T    *pMibOps;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibOps = (MIB_TABLE_ONU_PWR_SHEDDING_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_onu_remote_debug(void *pMibRow)
{
//    MIB_TABLE_ONU_REMOTE_DBG_T  *pMibOrd;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibOrd = (MIB_TABLE_ONU_REMOTE_DBG_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_ani_g(void *pMibRow)
{
    MIB_TABLE_ANIG_T    *pMibAniG;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibAniG = (MIB_TABLE_ANIG_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibAniG->EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for ani-g 0x%x is not found", pMibAniG->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_t_cont(void *pMibRow)
{
    MIB_TABLE_TCONT_T   *pMibTC;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibTC = (MIB_TABLE_TCONT_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibTC->EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for t-cont 0x%x is not found", pMibTC->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_priority_queue(void *pMibRow)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_TCONT_T       mibTC;
    MIB_TABLE_SCHEDULER_T   mibTS;
    MIB_TABLE_PRIQ_T        *pMibPQ;
    omci_me_instance_t      instanceID;
    UINT8                   isUpstream;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibPQ = (MIB_TABLE_PRIQ_T *)pMibRow;

    isUpstream = (pMibPQ->EntityID >> 15) & 0x1;
    if (isUpstream)
    {
        // search if corresponding t-cont is exist
        mibTC.EntityID = (pMibPQ->RelatedPort >> 16) & 0xFFFF;
        ret = MIB_Get(MIB_TABLE_TCONT_INDEX, &mibTC, sizeof(MIB_TABLE_TCONT_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: t-cont for priority queue 0x%x is not found", pMibPQ->EntityID);

            return GOS_ERR_NOT_FOUND;
        }
    }
    else
    {
        // search if corresponding cardholder slot is exist
        instanceID = (pMibPQ->RelatedPort >> 16) & 0xFFFF;
        if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(instanceID))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: cardholder for priority queue 0x%x is not found", pMibPQ->EntityID);

            return GOS_ERR_NOT_FOUND;
        }
    }

    if (pMibPQ->SchedulerPtr)
    {
        // search if corresponding traffic scheduler is exist
        mibTS.EntityID = pMibPQ->SchedulerPtr;
        ret = MIB_Get(MIB_TABLE_SCHEDULER_INDEX, &mibTS, sizeof(MIB_TABLE_SCHEDULER_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: traffic scheduler for priority queue 0x%x is not found", pMibPQ->EntityID);

            return GOS_ERR_NOT_FOUND;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_traffic_scheduler(void *pMibRow)
{
    GOS_ERROR_CODE          ret;
    MIB_TABLE_TCONT_T       mibTC;
    MIB_TABLE_SCHEDULER_T   mibTS;
    MIB_TABLE_SCHEDULER_T   *pMibTS;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibTS = (MIB_TABLE_SCHEDULER_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibTS->EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for traffic scheduler 0x%x is not found", pMibTS->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    if (pMibTS->TcontPtr)
    {
        // search if corresponding t-cont is exist
        mibTC.EntityID = pMibTS->TcontPtr;
        ret = MIB_Get(MIB_TABLE_TCONT_INDEX, &mibTC, sizeof(MIB_TABLE_TCONT_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: t-cont for traffic scheduler 0x%x is not found", pMibTS->EntityID);

            return GOS_ERR_NOT_FOUND;
        }
    }

    if (pMibTS->SchedulerPtr)
    {
        // search if corresponding traffic scheduler is exist
        mibTS.EntityID = pMibTS->SchedulerPtr;
        ret = MIB_Get(MIB_TABLE_SCHEDULER_INDEX, &mibTS, sizeof(MIB_TABLE_SCHEDULER_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: traffic scheduler for traffic scheduler 0x%x is not found", pMibTS->EntityID);

            return GOS_ERR_NOT_FOUND;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_ip_host_cfg_data(void *pMibRow)
{
 //   MIB_TABLE_IP_HOST_CFG_DATA_T    *pMibIPHC;

    if (!pMibRow)
        return GOS_ERR_PARAM;

 //   pMibIPHC = (MIB_TABLE_IP_HOST_CFG_DATA_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_pptp_eth_uni(void *pMibRow, BOOL *pIsDisabled)
{
    MIB_TABLE_ETHUNI_T  *pMibEthUni;

    if (!pMibRow || !pIsDisabled)
        return GOS_ERR_PARAM;

    pMibEthUni = (MIB_TABLE_ETHUNI_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibEthUni->EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for pptp eth uni 0x%x is not found", pMibEthUni->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    // validate physical port id
    if (pMibEthUni->PhysicalPortId >= RTK_MAX_NUM_OF_PORTS)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: invalid physical port id for pptp eth uni 0x%x", pMibEthUni->EntityID);

        return GOS_ERR_PARAM;
    }

    // validate physical port type (skip invalid entry)
    if (!*pIsDisabled &&
            RT_FE_PORT != gInfo.devCapabilities.ethPort[pMibEthUni->PhysicalPortId].portType &&
            RT_GE_PORT != gInfo.devCapabilities.ethPort[pMibEthUni->PhysicalPortId].portType)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: skip invalid physical port type for pptp eth uni 0x%x", pMibEthUni->EntityID);

        *pIsDisabled = TRUE;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_veip(void *pMibRow)
{
    MIB_TABLE_VEIP_T    *pMibVEIP;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibVEIP = (MIB_TABLE_VEIP_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibVEIP->EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for veip 0x%x is not found", pMibVEIP->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_pptp_80211_uni(void *pMibRow)
{
    MIB_TABLE_PPTP_80211_UNI_T  *pMib80211Uni;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMib80211Uni = (MIB_TABLE_PPTP_80211_UNI_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMib80211Uni->EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for pptp 802.11 uni 0x%x is not found", pMib80211Uni->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_pptp_pots_uni(void *pMibRow)
{
    MIB_TABLE_POTSUNI_T     *pMibPotsUni;
    MIB_TABLE_INFO_T        *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);

    if (!pTableInfo)
        return GOS_OK;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibPotsUni = (MIB_TABLE_POTSUNI_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibPotsUni->EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for pptp pots uni 0x%x is not found", pMibPotsUni->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_voip_line_status(void *pMibRow)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_POTSUNI_T       mibPotsUni;
    MIB_TABLE_VOIPLINESTATUS_T    *pMibVls;
    MIB_TABLE_INFO_T              *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_VOIPLINESTATUS_INDEX);
    if (!pTableInfo)
        return GOS_OK;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibVls = (MIB_TABLE_VOIPLINESTATUS_T *)pMibRow;

    // search if corresponding pptp pots uni is exist
    mibPotsUni.EntityId = pMibVls->EntityId;
    ret = MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPotsUni, sizeof(MIB_TABLE_POTSUNI_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: pptp pots uni for voip line status 0x%x is not found", pMibVls->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_voip_cfg_data(void *pMibRow)
{
//    MIB_TABLE_VOIPCONFIGDATA_T  *pMibVcd;
    MIB_TABLE_INFO_T            *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_VOIPCONFIGDATA_INDEX);

    if (!pTableInfo)
        return GOS_OK;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibVcd = (MIB_TABLE_VOIPCONFIGDATA_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_uni_g(void *pMibRow)
{
//    MIB_TABLE_UNIG_T    *pMibUniG;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibUniG = (MIB_TABLE_UNIG_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_olt_g(void *pMibRow)
{
//    MIB_TABLE_OLTG_T    *pMibOltG;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibOltG = (MIB_TABLE_OLTG_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_tr069(void *pMibRow)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_VEIP_T                mibVEIP;
    MIB_TABLE_TR069MANAGESERVER_T   *pMibTr069;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibTr069 = (MIB_TABLE_TR069MANAGESERVER_T *)pMibRow;

    // search if corresponding veip is exist
    mibVEIP.EntityId = pMibTr069->EntityID;
    ret = MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVEIP, sizeof(MIB_TABLE_VEIP_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: veip for tr069 0x%x is not found", pMibTr069->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_pptp_video_uni(void *pMibRow)
{
    MIB_TABLE_PPTP_VIDEO_UNI_T  *pMibVideoUni;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibVideoUni = (MIB_TABLE_PPTP_VIDEO_UNI_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibVideoUni->EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for pptp video uni 0x%x is not found", pMibVideoUni->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_pptp_video_ani(void *pMibRow)
{
    MIB_TABLE_PPTP_VIDEO_ANI_T  *pMibVideoAni;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibVideoAni = (MIB_TABLE_PPTP_VIDEO_ANI_T *)pMibRow;

    // search if corresponding cardholder slot is exist
    if (GOS_OK != omci_mib_cfg_is_parent_slot_existed(pMibVideoAni->EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: cardholder for pptp video ani 0x%x is not found", pMibVideoAni->EntityId);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_ctc_ext_onu_g(void *pMibRow)
{
//    MIB_TABLE_EXTENDED_ONU_G_T  *pMibExtOnuG;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibExtOnuG = (MIB_TABLE_EXTENDED_ONU_G_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_ctc_onu_capability(void *pMibRow)
{
//    MIB_TABLE_ONUCAPABILITY_T   *pMibOnuCapability;

    if (!pMibRow)
        return GOS_ERR_PARAM;

 //   pMibOnuCapability = (MIB_TABLE_ONUCAPABILITY_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_ctc_loid_auth(void *pMibRow)
{
//    MIB_TABLE_LOIDAUTH_T    *pMibLoidAuth;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibLoidAuth = (MIB_TABLE_LOIDAUTH_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_hw_ext_voip_img(void *pMibRow)
{
//    MIB_TABLE_VOIP_IMAGE_T  *pMibVoipImg;

    if (!pMibRow)
        return GOS_ERR_PARAM;

//    pMibVoipImg = (MIB_TABLE_VOIP_IMAGE_T *)pMibRow;

    return GOS_OK;
}

static GOS_ERROR_CODE omci_mib_cfg_loop_detect(void *pMibRow)
{
    GOS_ERROR_CODE              ret;
    MIB_TABLE_ETHUNI_T          mibEthUni;
    MIB_TABLE_LOOP_DETECT_T     *pMibLoopDetect;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    pMibLoopDetect = (MIB_TABLE_LOOP_DETECT_T *)pMibRow;

    // search if corresponding pptp pots uni is exist
    mibEthUni.EntityID = pMibLoopDetect->EntityID;
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibEthUni, sizeof(MIB_TABLE_ETHUNI_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "mib cfg: pptp eth uni for loop detect 0x%x is not found", pMibLoopDetect->EntityID);

        return GOS_ERR_NOT_FOUND;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_mib_cfg_setup_me(omci_me_class_t    classId,
                                                void    *pMibRow,
                                                BOOL    *pIsDisabled)
{
    GOS_ERROR_CODE  ret;

    if (!pMibRow)
        return GOS_ERR_PARAM;

    switch (classId)
    {
        /* G.988 9.1 - Equipment management */
        case OMCI_ME_CLASS_ONU_G:
            ret = omci_mib_cfg_onu_g(pMibRow);
            break;
        case OMCI_ME_CLASS_ONU2_G:
            ret = omci_mib_cfg_onu2_g(pMibRow);
            break;
        case OMCI_ME_CLASS_ONU_DATA:
            ret = omci_mib_cfg_onu_data(pMibRow);
            break;
        case OMCI_ME_CLASS_SOFTWARE_IMAGE:
            ret = omci_mib_cfg_sw_img(pMibRow);
            break;
        case OMCI_ME_CLASS_CARDHOLDER:
            ret = omci_mib_cfg_cardholder(pMibRow);
            break;
        case OMCI_ME_CLASS_CIRCUIT_PACK:
            ret = omci_mib_cfg_circuit_pack(pMibRow);
            break;
        case OMCI_ME_CLASS_ONU_POWER_SHEDDING:
            ret = omci_mib_cfg_onu_pwr_shedding(pMibRow);
            break;
        case OMCI_ME_CLASS_ONU_REMOTE_DEBUG:
            ret = omci_mib_cfg_onu_remote_debug(pMibRow);
            break;

        /* G.988 9.2 - ANI management, traffic management */
        case OMCI_ME_CLASS_ANI_G:
            ret = omci_mib_cfg_ani_g(pMibRow);
            break;
        case OMCI_ME_CLASS_T_CONT:
            ret = omci_mib_cfg_t_cont(pMibRow);
            break;
        case OMCI_ME_CLASS_PRIORITY_QUEUE:
            ret = omci_mib_cfg_priority_queue(pMibRow);
            break;
        case OMCI_ME_CLASS_TRAFFIC_SCHEDULER:
            ret = omci_mib_cfg_traffic_scheduler(pMibRow);
            break;

        /* G.988 9.4 - Layer 3 data services */
        case OMCI_ME_CLASS_IP_HOST_CFG_DATA:
            ret = omci_mib_cfg_ip_host_cfg_data(pMibRow);
            break;

        /* G.988 9.5 - Ethernet services */
        case OMCI_ME_CLASS_PPTP_ETHERNET_UNI:
            ret = omci_mib_cfg_pptp_eth_uni(pMibRow, pIsDisabled);
            break;
        case OMCI_ME_CLASS_VIRTUAL_ETHERNET_INTF_POINT:
            ret = omci_mib_cfg_veip(pMibRow);
            break;

        /* G.988 9.6 - 802.11 services */
        case OMCI_ME_CLASS_PPTP_80211_UNI:
            ret = omci_mib_cfg_pptp_80211_uni(pMibRow);
            break;

        /* G.988 9.9 - Voice services */
        case OMCI_ME_CLASS_PPTP_POTS_UNI:
            ret = omci_mib_cfg_pptp_pots_uni(pMibRow);
            break;
        case OMCI_ME_CLASS_VOIP_LINE_STATUS:
            ret = omci_mib_cfg_voip_line_status(pMibRow);
            break;
        case OMCI_ME_CLASS_VOIP_CFG_DATA:
            ret = omci_mib_cfg_voip_cfg_data(pMibRow);
            break;

        /* G.988 9.12 - General purpose MEs */
        case OMCI_ME_CLASS_UNI_G:
            ret = omci_mib_cfg_uni_g(pMibRow);
            break;
        case OMCI_ME_CLASS_OLT_G:
            ret = omci_mib_cfg_olt_g(pMibRow);
            break;
        case OMCI_ME_CLASS_BBF_TR069_MGMT_SERVER:
            ret = omci_mib_cfg_tr069(pMibRow);
            break;

        /* G.988 9.13 - Miscellaneous services */
        case OMCI_ME_CLASS_PPTP_VIDEO_UNI:
            ret = omci_mib_cfg_pptp_video_uni(pMibRow);
            break;
        case OMCI_ME_CLASS_PPTP_VIDEO_ANI:
            ret = omci_mib_cfg_pptp_video_ani(pMibRow);
            break;

        /* CTC specific */
        case OMCI_ME_CLASS_CTC_EXTENDED_ONU_G:
            ret = omci_mib_cfg_ctc_ext_onu_g(pMibRow);
            break;
        case OMCI_ME_CLASS_CTC_ONU_CAPABILITY:
            ret = omci_mib_cfg_ctc_onu_capability(pMibRow);
            break;
        case OMCI_ME_CLASS_CTC_LOID_AUTHENTICATION:
            ret = omci_mib_cfg_ctc_loid_auth(pMibRow);
            break;

        /* Huawei OLT specific */
        case OMCI_ME_CLASS_HW_EXTENDED_VOIP_IMAGE:
            ret = omci_mib_cfg_hw_ext_voip_img(pMibRow);
            break;

        /* Vendor specific */
        case OMCI_ME_CLASS_LOOP_DETECT:
            ret = omci_mib_cfg_loop_detect(pMibRow);
            break;

        default:
            ret = GOS_OK;
            break;
    }

    return ret;
}

GOS_ERROR_CODE omci_mib_cfg_parse_line(char *pLnBuf)
{
	GOS_ERROR_CODE		ret;
	char				*pToken;
	char				*pNext;
	char				byteStr[3];
	BOOL				isNoUpload;
	BOOL				isDisabled;
	omci_me_class_t		classId;
	omci_me_instance_t	instanceId;
    MIB_TABLE_INDEX		tblIdx;
    MIB_ATTR_INDEX		attrIdx;
    MIB_ATTR_TYPE		attrType;
    MIB_TABLE_INFO_T	*pMibTblInfo;
	MIB_ATTR_INFO_T		*pMibAttrInfo;
	MIB_TABLE_OPER_T	*pMibTblOper;
    UINT32              attrSize;
    UINT32				i, j;
    char				aMibRow[MIB_TABLE_ENTRY_MAX_SIZE];
    char				*pMibRow;

    // initialize
    pMibTblInfo = NULL;
	pMibAttrInfo = NULL;
	pMibTblOper = NULL;
	pMibRow = aMibRow;

	// remember to initialize
	// before invoking tokenizer
	pNext = NULL;

	// start tokenize
	pToken = omci_util_space_tokenize(pLnBuf, &pNext);
	if (!pToken)
    {
    	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: tokenize fail");

    	return GOS_FAIL;
    }

	// first token may contains control symbol
	isNoUpload = FALSE;
	isDisabled = FALSE;
	if (*pToken == '!' || *pToken == '^')
	{
		// !: do not upload
		isNoUpload = strchr(pToken, '!') ? TRUE : FALSE;
		// ^: do not enable
		isDisabled = strchr(pToken, '^') ? TRUE : FALSE;

		// move forward to next token
		pToken = omci_util_space_tokenize(pLnBuf, &pNext);
		if (!pToken)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: tokenize fail");

			return GOS_FAIL;
		}
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mib cfg: loading class id %s", pToken);

	// get class id
	classId = (UINT16)strtoul(pToken, NULL, 0);

	// move forward to next token
	pToken = omci_util_space_tokenize(pLnBuf, &pNext);
	if (!pToken)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: tokenize fail");

		return GOS_FAIL;
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mib cfg: loading instance id %s", pToken);

	// get instance id
	instanceId = (UINT16)strtoul(pToken, NULL, 0);

	// return fail for string conversion error
	if (0 == classId && 0 == instanceId)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: string conversion fail");

		return GOS_FAIL;
	}

	// reset buffer
	memset(pMibRow, 0, sizeof(aMibRow));
	memset(byteStr, 0, sizeof(byteStr));

	// create mib for unsupported me
	tblIdx = MIB_GetTableIndexByClassId(classId);
	if (tblIdx == MIB_TABLE_UNKNOWN_INDEX)
	{
		char	tblNameStr[16];

		// assign me oper info
		pMibTblOper = malloc(sizeof(MIB_TABLE_OPER_T));
    	if (!pMibTblOper)
	    {
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: memory allocation fail");

			return GOS_FAIL;
		}
		memset(pMibTblOper, 0, sizeof(MIB_TABLE_OPER_T));
		pMibTblOper->meOperDump = omci_mib_oper_dump_default_handler;

		// create mib tbl info
		tblIdx = MIB_Register();
        if(tblIdx == MIB_TABLE_UNKNOWN_INDEX)
        {
            free(pMibTblOper);
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: Get tblIdx failed");
            return GOS_FAIL;
        }
	    pMibTblInfo = malloc(sizeof(MIB_TABLE_INFO_T));
	    if (!pMibTblInfo)
	    {
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: memory allocation fail");

			free(pMibTblOper);

			return GOS_FAIL;
		}
	    memset(pMibTblInfo, 0, sizeof(MIB_TABLE_INFO_T));
	    snprintf(tblNameStr, sizeof(tblNameStr), "%hu", classId);
	    pMibTblInfo->Name = strdup(tblNameStr);
	    pMibTblInfo->ShortName = pMibTblInfo->Name;
	    pMibTblInfo->Desc = pMibTblInfo->Name;
	    pMibTblInfo->ClassId = classId;
	    pMibTblInfo->InitType = OMCI_ME_INIT_TYPE_ONU;
	    if ((classId >= 350 && classId <= 399) ||
	    		(classId >= 65280 && classId <= 65535))
	    	pMibTblInfo->StdType = OMCI_ME_TYPE_PROPRIETARY;
	    else
	    	pMibTblInfo->StdType = OMCI_ME_TYPE_STANDARD;
	    pMibTblInfo->ActionType = OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;

	    // create mib attr info
	    pMibAttrInfo = malloc(sizeof(MIB_ATTR_INFO_T));
    	if (!pMibAttrInfo)
	    {
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: memory allocation fail");

			free(pMibTblInfo);
			free(pMibTblOper);

			return GOS_FAIL;
		}
    	memset(pMibAttrInfo, 0, sizeof(MIB_ATTR_INFO_T));
    	snprintf(tblNameStr, sizeof(tblNameStr), "Entity ID");
    	pMibAttrInfo->Name = strdup(tblNameStr);
	    pMibAttrInfo->Desc = pMibAttrInfo->Name;
	    pMibAttrInfo->DataType = MIB_ATTR_TYPE_UINT16;
	    pMibAttrInfo->Len = sizeof(omci_me_instance_t);
	    pMibAttrInfo->IsIndex = TRUE;
	    pMibAttrInfo->MibSave = TRUE;
	    pMibAttrInfo->OltAcc = OMCI_ME_ATTR_ACCESS_READ |
	    			OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
	    pMibAttrInfo->OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

		// save instance id to buffer
		memcpy(pMibRow, &instanceId, sizeof(omci_me_instance_t));
		pMibRow += sizeof(omci_me_instance_t);

		// process the rest tokens
		i = 1;
		while ((pToken = omci_util_space_tokenize(pLnBuf, &pNext)))
	    {
	    	// create mib attr info
	    	pMibAttrInfo = realloc(pMibAttrInfo, sizeof(MIB_ATTR_INFO_T) * (i+1));
	    	if (!pMibAttrInfo)
		    {
				OMCI_LOG(OMCI_LOG_LEVEL_ERR, "mib cfg: memory allocation fail");

				free(pMibAttrInfo);
				free(pMibTblInfo);
				free(pMibTblOper);

				return GOS_FAIL;
			}
	    	memset(&pMibAttrInfo[i], 0, sizeof(MIB_ATTR_INFO_T));
	    	snprintf(tblNameStr, sizeof(tblNameStr), "Attr%hu", i+1);
	    	pMibAttrInfo[i].Name = strdup(tblNameStr);
		    pMibAttrInfo[i].Desc = pMibAttrInfo[i].Name;
		    pMibAttrInfo[i].MibSave = TRUE;
		    pMibAttrInfo[i].OltAcc = OMCI_ME_ATTR_ACCESS_READ |
		    			OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
		    pMibAttrInfo[i].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    		if (pToken[0] == '0' &&
    				(pToken[1] == 'x' || pToken[1] == 'X'))
    		{
    			while (pToken[2] != '\0' && pToken[3] != '\0')
    			{
    				pToken += sizeof(char) * 2;
    				byteStr[0] = pToken[0];
    				byteStr[1] = pToken[1];
    				*pMibRow++ = (UINT8)strtoul(byteStr, NULL, 16);
    				pMibAttrInfo[i].Len++;
    			}
    			switch (pMibAttrInfo[i].Len)
    			{
    				case 1:
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT8;
    					break;
    				case 2:
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT16;
    					break;
    				case 4:
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT32;
    					break;
    				case 8:
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT64;
    					break;
    				default:
    					pMibRow++;
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_STR;
    					break;
    			}
    			pMibAttrInfo[i].OutStyle = MIB_ATTR_OUT_HEX;
    		}
    		else
    		{
    			if (pToken[0] == '"' || pToken[0] == '\'')
    			{
    				pToken[strlen(pToken)-1] = '\0';
    				pToken++;
    				snprintf(pMibRow, strlen(pToken)+1, "%s", pToken);
    				pMibRow += strlen(pToken) + 1;
    				pMibAttrInfo[i].Len = strlen(pToken);
    				pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_STR;
    				pMibAttrInfo[i].OutStyle = MIB_ATTR_OUT_CHAR;
    			}
    			else
    			{
    				unsigned long long	u32data, u64Max;
    				u32data = strtoull(pToken, NULL, 0);
                    u64Max = ULLONG_MAX;
    				if (u32data <= UCHAR_MAX)
    				{
    					*((UINT8 *)pMibRow) = (UINT8)u32data;
    					pMibRow += sizeof(UINT8);
    					pMibAttrInfo[i].Len = 1;
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT8;
    				}
    				else if (u32data <= USHRT_MAX)
    				{
    					*((UINT16 *)pMibRow) = (UINT16)u32data;
    					pMibRow += sizeof(UINT16);
    					pMibAttrInfo[i].Len = 2;
    					pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT16;
    				}
    				else if (u32data <= UINT_MAX)
    				{

    					memcpy(pMibRow, &u32data, sizeof(UINT32));
    					pMibRow += sizeof(UINT32);
    					pMibAttrInfo[i].Len = 4;
						pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT32;
	    			}
                    else if (u32data <= u64Max && errno != ERANGE)
                    {
                        UINT64  u64data;
                        u64data = GOS_BuffToUINT64(pToken, sizeof(u32data));
                        memcpy(pMibRow, &u64data, sizeof(UINT64));
                        pMibRow += sizeof(UINT64);
                        pMibAttrInfo[i].Len = 8;
                        pMibAttrInfo[i].DataType = MIB_ATTR_TYPE_UINT64;
                    }
    				pMibAttrInfo[i].OutStyle = MIB_ATTR_OUT_DEC;
    			}
    		}

    		i++;
	    }

	    // register mib table
	    pMibTblInfo->attrNum = i;
	    pMibTblInfo->entrySize = pMibRow - aMibRow;
	    pMibTblInfo->pAttributes = pMibAttrInfo;
	    ret = MIB_InfoRegister(tblIdx, pMibTblInfo, pMibTblOper);
	    if (GOS_OK != ret)
	    {
	    	OMCI_LOG(OMCI_LOG_LEVEL_ERR,
	    		"mib cfg: create me %u fail %d", classId, ret);

	    	free(pMibAttrInfo);
	    	free(pMibTblInfo);
	    	free(pMibTblOper);

	    	return GOS_FAIL;
	    }
	    MIB_TABLE_TOTAL_NUMBER = MIB_GetTableNum() - 1;
		MIB_TABLE_LAST_INDEX = MIB_TABLE_TOTAL_NUMBER;

	    // rewind buffer
	    pMibRow = aMibRow;
	}
	// create mib for supported me
	else
	{
		// save instance id to buffer
		MIB_SetAttrToBuf(tblIdx, MIB_ATTR_FIRST_INDEX,
			&instanceId, pMibRow, sizeof(omci_me_instance_t));

		// process the rest tokens
		for (attrIdx = MIB_ATTR_NEXT_INDEX(MIB_ATTR_FIRST_INDEX), i = 1;
	            i < MIB_GetTableAttrNum(tblIdx);
	            attrIdx = MIB_ATTR_NEXT_INDEX(attrIdx), i++)
	    {
	    	// skip non SBC attributes if ME is not created by ONU
	    	if (OMCI_ME_INIT_TYPE_ONU != MIB_GetTableInitType(tblIdx))
				if (OMCI_ME_ATTR_ACCESS_SBC != MIB_GetAttrOltAcc(tblIdx, attrIdx))
					continue;

			attrType = MIB_GetAttrDataType(tblIdx, attrIdx);
	        attrSize = MIB_GetAttrSize(tblIdx, attrIdx);

			// skip table attributes for its uncertainty length
	        if (MIB_ATTR_TYPE_TABLE == attrType)
	        	continue;

	        // end loop if there is no more token
	        pToken = omci_util_space_tokenize(pLnBuf, &pNext);
	    	if (!pToken)
	    		break;

	    	// save the rest attributes to buffer
	        switch (attrType)
	        {
	        	case MIB_ATTR_TYPE_UINT8:
	        	{
	        		UINT8	data;
	        		data = (UINT8)strtoul(pToken, NULL, 0);
	        		MIB_SetAttrToBuf(tblIdx, attrIdx, &data, pMibRow, attrSize);
	        		break;
	        	}
	        	case MIB_ATTR_TYPE_UINT16:
	        	{
	        		UINT16	data;
	        		data = (UINT16)strtoul(pToken, NULL, 0);
	        		MIB_SetAttrToBuf(tblIdx, attrIdx, &data, pMibRow, attrSize);
	        		break;
	        	}
	        	case MIB_ATTR_TYPE_UINT32:
	        	{
	        		UINT32	data;
	        		data = (UINT32)strtoul(pToken, NULL, 0);
	        		MIB_SetAttrToBuf(tblIdx, attrIdx, &data, pMibRow, attrSize);
	        		break;
	        	}
	        	case MIB_ATTR_TYPE_UINT64:
	        	{
	        		UINT64	data;
					data = GOS_BuffToUINT64(pToken, attrSize);
					MIB_SetAttrToBuf(tblIdx, attrIdx, &data, pMibRow, attrSize);
	        		break;
	        	}
	        	case MIB_ATTR_TYPE_STR:
	        	{
	        		char	*data;
	        		data = mib_GetAttrPtr(tblIdx, pMibRow, attrIdx);
	        		if (pToken[0] == '0' &&
	        				(pToken[1] == 'x' || pToken[1] == 'X'))
	        		{
	        			for (j = 0; j < (attrSize-1) &&
	        					pToken[2] != '\0' && pToken[3] != '\0'; j++)
	        			{
	        				pToken += sizeof(char) * 2;
	        				byteStr[0] = pToken[0];
	        				byteStr[1] = pToken[1];
	        				data[j] = (UINT8)strtoul(byteStr, NULL, 16);
	        			}
	        		}
	        		else
	        		{
	        			if (pToken[0] == '"' || pToken[0] == '\'')
	        			{
	        				pToken[strlen(pToken)-1] = '\0';
	        				pToken++;
	        			}
	        			snprintf(data, attrSize, "%s", pToken);
	        		}
	        		break;
	        	}
	        	default:
	        		break;
	        }
	    }

	    // invoke setup procedure
	    ret = omci_mib_cfg_setup_me(classId, pMibRow, &isDisabled);
	    if (GOS_OK != ret)
	    {
	    	OMCI_LOG(OMCI_LOG_LEVEL_ERR,
    			"mib cfg: setup me %u/%04x fail %d", classId, instanceId, ret);

	    	return GOS_FAIL;
	    }

	    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
	    	"mib cfg: me %u/%04x setup success", classId, instanceId);
	}

    // save the buffer to the mib or return fail
    ret = MIB_Add(tblIdx, pMibRow, MIB_GetTableEntrySize(tblIdx), isNoUpload, isDisabled);
    if (GOS_OK != ret)
    {
    	OMCI_LOG(OMCI_LOG_LEVEL_ERR,
    		"mib cfg: create me %u/%04x fail %d", classId, instanceId, ret);

        if (GOS_ERR_DUPLICATED == ret)
        {
            ret = MIB_Set(tblIdx, pMibRow, MIB_GetTableEntrySize(tblIdx));
        }

        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "mib cfg: set me %u/%04x fail %d", classId, instanceId, ret);

        	if (pMibAttrInfo)
    	    	free(pMibAttrInfo);
        	if (pMibTblInfo)
    			free(pMibTblInfo);
    		if (pMibTblOper)
        		free(pMibTblOper);

        	return GOS_FAIL;
        }
    }

    // invoke opercfg if it is enabled
    if (!isDisabled)
    {
	    ret = OMCI_MeOperCfg(tblIdx, NULL, pMibRow, MIB_ADD,
        		omci_GetOltAccAttrSet(tblIdx, OMCI_ME_ATTR_ACCESS_SBC),
        		OMCI_MSG_BASELINE_PRI_LOW);
	    if (GOS_OK != ret)
	    {
	    	OMCI_LOG(OMCI_LOG_LEVEL_ERR,
	    		"mib cfg: enable me %u/%04x fail %d", classId, instanceId, ret);

	    	if (pMibAttrInfo)
	    		free(pMibAttrInfo);
	    	if (pMibTblInfo)
				free(pMibTblInfo);
	    	if (pMibTblOper)
	    		free(pMibTblOper);

    		return GOS_FAIL;
	    }
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "mib cfg: me %u/%04x created and %s",
		classId, instanceId, isDisabled ? "disabled" : "enabled");

	return GOS_OK;
}

GOS_ERROR_CODE omci_mib_cfg_load_file()
{
	FILE	*pFD;
	char	*pLnBuf;
	char	lnBuf[OMCI_CFG_FILE_MAX_LINE_LEN];

	pFD = fopen(OMCI_CFG_FILE_LOCATION, "r");
	if (!pFD)
		return GOS_OK; //do nothing

	while (NULL != fgets(lnBuf, OMCI_CFG_FILE_MAX_LINE_LEN, pFD))
	{
		// remove spaces
		pLnBuf = omci_util_trim_space(lnBuf);

		// skip comments or empty line
		if (0 == strlen(pLnBuf) || '#' == pLnBuf[0])
			continue;

		// start parsing the entire line
		if (GOS_OK != omci_mib_cfg_parse_line(pLnBuf))
		{
			OMCI_PRINT("fail to parse mib cfg");

			fclose(pFD);

			return GOS_FAIL;
		}
	}

	fclose(pFD);

	return GOS_OK;
}
