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
 * Purpose : Definition of OMCI MIB APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI MIB APIs
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/switch.h"
#include "common/error.h"
#include <hal/common/halctrl.h>
#endif
#include "feature_mgmt.h"
#include "mcast_wrapper.h"


static UINT16 g_numOfUsPQ;
static UINT16 g_numOfDsPQ;

extern BOOL gOmciOmitErrEnable;
extern int MIB_TABLE_TOTAL_NUMBER;

extern pthread_mutex_t  gOmciPmOperMutex;

/*
*  Define local APIs
*/
static GOS_ERROR_CODE omci_PonResetOntGOnt2G(void)
{
    MIB_TABLE_ONTG_T  ontg;
    MIB_TABLE_ONT2G_T ont2g;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: ONT-G & ONT2-G");

    MIB_Default(MIB_TABLE_ONTG_INDEX,  &ontg,  sizeof(MIB_TABLE_ONTG_T));
    MIB_Default(MIB_TABLE_ONT2G_INDEX, &ont2g, sizeof(MIB_TABLE_ONT2G_T));

    // Reset ontg
    ontg.EntityID = TXC_ONUG_INSTANCE_ID;

    // 1. set vendor id
    memcpy(ontg.VID, gInfo.sn, MIB_TABLE_ONTG_VID_LEN);

    // 2. set version id and ONT2-G's equipment id
    snprintf(ont2g.EqtID, MIB_TABLE_ONT2G_EQTID_LEN+1, "%s", gInfo.devIdVersion.id);
    snprintf(ontg.Version, MIB_TABLE_ONTG_VERSION_LEN+1, "R%s", gInfo.devIdVersion.version);

    // 3. set serial number
    memcpy(ontg.SerialNum, gInfo.sn, MIB_TABLE_ONTG_SERIALNUM_LEN);

    // 4. set traffic option
    ontg.TraffMgtOpt = ONUG_TM_OPTION_PRIORITY_RATE_CONTROLLED;

    // 5. set Loid
    memcpy(ontg.LogicalOnuID, gInfo.loidCfg.loid, MIB_TABLE_ONTG_LOID_LEN);
    memcpy(ontg.LogicalPassword, gInfo.loidCfg.loidPwd, MIB_TABLE_ONTG_LP_LEN);
    ontg.CredentialsStatus = gInfo.loidCfg.loidAuthStatus;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)));
    OMCI_MeOperCfg(MIB_TABLE_ONTG_INDEX, NULL, &ontg, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ONTG_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);


    // Reset ont2g
    ont2g.EntityID = TXC_ONU2G_INSTANCE_ID;

    // 1. set upstream priority queue number
    ont2g.NumOfPriQ = gInfo.devCapabilities.totalTContQueueNum;

    // 2. set tcont number
    ont2g.NumOfScheduler = gInfo.devCapabilities.totalTContNum;

    // 3. set gem port ctp number
    ont2g.NumOfGemPort = gInfo.devCapabilities.totalGEMPortNum;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONT2G_INDEX, &ont2g, sizeof(MIB_TABLE_ONT2G_T)));
    OMCI_MeOperCfg(MIB_TABLE_ONT2G_INDEX, NULL, &ont2g, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ONT2G_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetOntData(void)
{
    MIB_TABLE_ONTDATA_T ontData;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: ONT data");

    MIB_Default(MIB_TABLE_ONTDATA_INDEX, &ontData, sizeof(MIB_TABLE_ONTDATA_T));

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONTDATA_INDEX, &ontData, sizeof(MIB_TABLE_ONTDATA_T)));
    OMCI_MeOperCfg(MIB_TABLE_ONTDATA_INDEX, NULL, &ontData, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ONTDATA_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetSlotPort(void)
{
    MIB_TABLE_CARDHOLDER_T  cardholder;
    MIB_TABLE_CIRCUITPACK_T circuitPack;
    UINT32                  slotNum;
    UINT32                  slotId;

    MIB_Default(MIB_TABLE_CARDHOLDER_INDEX,  &cardholder,  sizeof(MIB_TABLE_CARDHOLDER_T));
    MIB_Default(MIB_TABLE_CIRCUITPACK_INDEX, &circuitPack, sizeof(MIB_TABLE_CIRCUITPACK_T));

    /*Set UNI slot*/
    for (slotId = 1; slotId < TXC_CARDHLD_SLOT_NUM; slotId++)
    {
        if (TXC_CARDHLD_ETH_FE_SLOT == slotId)
            if (0 == gInfo.devCapabilities.fePortNum)
                continue;

        if (TXC_CARDHLD_ETH_GE_SLOT == slotId)
            if (0 == gInfo.devCapabilities.gePortNum)
                continue;

        if (TXC_CARDHLD_VEIP_SLOT == slotId)
            if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode &&
                (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_BDP_00000100)))
                continue;

        if (TXC_CARDHLD_POTS_SLOT == slotId)
            if (0 == gInfo.devCapabilities.potsPortNum)
                continue;

        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: Cardholder & Circuit pack [%u]", slotId);

        slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slotId);

        if (TXC_CARDHLD_VEIP_SLOT == slotId)
            feature_api(FEATURE_API_ME_00000100, &slotNum);

        /* 1. Reset cardholder */
        cardholder.EntityID         = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotNum;
        cardholder.ActualType       = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
        cardholder.ExpectedType     = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
        if (TXC_CARDHLD_ETH_FE_SLOT == slotId)
        {
            cardholder.ExpectedPortCount = gInfo.devCapabilities.fePortNum;
        }
        else if (TXC_CARDHLD_ETH_GE_SLOT == slotId)
        {
            cardholder.ExpectedPortCount = gInfo.devCapabilities.gePortNum;
        }
        else if (TXC_CARDHLD_VEIP_SLOT == slotId)
        {
            cardholder.ExpectedPortCount = 1;
        }
        else if (TXC_CARDHLD_POTS_SLOT == slotId)
        {
            cardholder.ExpectedPortCount = gInfo.devCapabilities.potsPortNum;
        }
        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_CARDHOLDER_INDEX, &cardholder, sizeof(MIB_TABLE_CARDHOLDER_T)));
        OMCI_MeOperCfg(MIB_TABLE_CARDHOLDER_INDEX, NULL, &cardholder, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_CARDHOLDER_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

        /* 2. Reset circuit pack */
        circuitPack.EntityID    = cardholder.EntityID;
        circuitPack.Type        = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
        memcpy(circuitPack.SerialNum, gInfo.sn, MIB_TABLE_CIRCUITPACK_SERIALNUM_LEN);
        snprintf(circuitPack.Version, MIB_TABLE_CIRCUITPACK_VERSION_LEN, "0");
        memcpy(circuitPack.VID, gInfo.sn, MIB_TABLE_CIRCUITPACK_VID_LEN);
        if (TXC_CARDHLD_ETH_FE_SLOT == slotId)
        {
            circuitPack.NumOfPorts      = gInfo.devCapabilities.fePortNum;
            circuitPack.NumOfTContBuff  = 0;
            circuitPack.NumOfPriQ       = circuitPack.NumOfPorts * gInfo.devCapabilities.perUNIQueueNum;
            circuitPack.NumOfScheduler  = 0;
        }
        else if (TXC_CARDHLD_ETH_GE_SLOT == slotId)
        {
            circuitPack.NumOfPorts      = gInfo.devCapabilities.gePortNum;
            circuitPack.NumOfTContBuff  = 0;
            circuitPack.NumOfPriQ       = circuitPack.NumOfPorts * gInfo.devCapabilities.perUNIQueueNum;
            circuitPack.NumOfScheduler  = 0;
        }
        else if (TXC_CARDHLD_VEIP_SLOT == slotId)
        {
            circuitPack.NumOfPorts      = 1;
            circuitPack.NumOfTContBuff  = 0;
            circuitPack.NumOfPriQ       = circuitPack.NumOfPorts * gInfo.devCapabilities.perUNIQueueNum;
            circuitPack.NumOfScheduler  = 0;
        }
        else if (TXC_CARDHLD_POTS_SLOT == slotId)
        {
            circuitPack.NumOfPorts      = gInfo.devCapabilities.potsPortNum;
            circuitPack.NumOfTContBuff  = 0;
            circuitPack.NumOfPriQ       = 0;
            circuitPack.NumOfScheduler  = 0;
        }
        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_CIRCUITPACK_INDEX, &circuitPack, sizeof(MIB_TABLE_CIRCUITPACK_T)));
        OMCI_MeOperCfg(MIB_TABLE_CIRCUITPACK_INDEX, NULL, &circuitPack, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_CIRCUITPACK_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);
    }

    /*Set PON Slot*/
    slotId = TXC_CARDHLD_PON_SLOT;
    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: Cardholder & Circuit pack [%u]", slotId);

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slotId);

    /* 1. Reset cardholder */
    cardholder.EntityID         = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotNum;
    cardholder.ActualType       = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
    cardholder.ExpectedType     = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
    cardholder.ExpectedPortCount = 1;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_CARDHOLDER_INDEX, &cardholder, sizeof(MIB_TABLE_CARDHOLDER_T)));
    OMCI_MeOperCfg(MIB_TABLE_CARDHOLDER_INDEX, NULL, &cardholder, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_CARDHOLDER_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    /* 2. Reset circuit pack */
    circuitPack.EntityID    = cardholder.EntityID;
    circuitPack.Type        = TXC_GET_CARDTYPE_BY_SLOT_ID(slotId);
    memcpy(circuitPack.SerialNum, gInfo.sn, MIB_TABLE_CIRCUITPACK_SERIALNUM_LEN);
    snprintf(circuitPack.Version, MIB_TABLE_CIRCUITPACK_VERSION_LEN, "0");
    memcpy(circuitPack.VID, gInfo.sn, MIB_TABLE_CIRCUITPACK_VID_LEN);
    circuitPack.NumOfPorts      = 1;
    circuitPack.NumOfTContBuff  = gInfo.devCapabilities.totalTContNum;
    circuitPack.NumOfPriQ       = gInfo.devCapabilities.totalTContQueueNum;
    circuitPack.NumOfScheduler  = gInfo.devCapabilities.totalTContNum;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_CIRCUITPACK_INDEX, &circuitPack, sizeof(MIB_TABLE_CIRCUITPACK_T)));
    OMCI_MeOperCfg(MIB_TABLE_CIRCUITPACK_INDEX, NULL, &circuitPack, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_CIRCUITPACK_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);


    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetPriQ(UINT32 slot, UINT32 port, UINT16 entityId, UINT32 numOfPriQ)
{
    MIB_TABLE_PRIQ_T priQ;
    UINT32           qId;

    MIB_Default(MIB_TABLE_PRIQ_INDEX, &priQ, sizeof(MIB_TABLE_PRIQ_T));

    for (qId = 0; qId < numOfPriQ; qId++)
    {
        /*
         *   slot: for upstream, it is for the ANI slot; for downstream, it is for the UNI slot
         *   port: for upstream, it is the local index of T-CONT; for downstream, it is the UNI port id
         */

        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: Priority queue [%#x,%#x]", slot, port);

        if (slot == TXC_CARDHLD_PON_SLOT)
        {
            priQ.EntityID        = OMCI_MIB_US_TM_ME_ID_BASE + g_numOfUsPQ++;
            priQ.RelatedPort     = (entityId << 16) | (numOfPriQ - qId - 1);

            //omci_wrapper_setPriQueue(&priQ);
        }
        else
        {
            priQ.EntityID        = g_numOfDsPQ++;
            priQ.RelatedPort     = (entityId << 16) | (numOfPriQ - qId - 1);
        }

        priQ.MaxQSize       = TXC_PRIO_Q_MAX_SIZE;
        priQ.AllocQSize     = TXC_PRIO_Q_MAX_SIZE;

        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PRIQ_INDEX, &priQ, sizeof(MIB_TABLE_PRIQ_T)));
        OMCI_MeOperCfg(MIB_TABLE_PRIQ_INDEX, NULL, &priQ, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_PRIQ_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);
    }

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetTrafficScheduler(UINT32 slot, UINT32 tcontId)
{
    MIB_TABLE_SCHEDULER_T  scheduler;
    UINT32                 slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: Traffic scheduler [%u]", tcontId);

    MIB_Default(MIB_TABLE_SCHEDULER_INDEX, &scheduler, sizeof(MIB_TABLE_SCHEDULER_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    scheduler.EntityID  = (slotNum << 8) | tcontId;
    scheduler.TcontPtr  = (slotNum << 8) | tcontId;
    scheduler.Policy    = TCONT_POLICY_WEIGHTED_ROUND_ROBIN;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_SCHEDULER_INDEX, &scheduler, sizeof(MIB_TABLE_SCHEDULER_T)));
    OMCI_MeOperCfg(MIB_TABLE_SCHEDULER_INDEX, NULL, &scheduler, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_SCHEDULER_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetTcont(UINT32 slot)
{
    MIB_TABLE_PRIVATE_TQCFG_T   tqCfg;
    private_tq_cfg_type_t       type = PRIVATE_TQCFG_TYPE_RTK_DEFAULT_DEFINITION;
    MIB_TABLE_TCONT_T tcont;
    UINT32            tcontId, max_tcontId;
    UINT32            slotNum;

#ifdef OMCI_X86
    max_tcontId = 8;
#else
    max_tcontId = gInfo.devCapabilities.totalTContNum;
#endif

    tqCfg.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_PRIVATE_TQCFG_INDEX, &tqCfg, sizeof(MIB_TABLE_PRIVATE_TQCFG_T)))
    {
        type = tqCfg.Type;
    }

    MIB_Default(MIB_TABLE_TCONT_INDEX, &tcont, sizeof(MIB_TABLE_TCONT_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    for (tcontId = 0; tcontId < max_tcontId; tcontId++)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: T-CONT [%u]", tcontId);

        // Reset T-Cont
        tcont.EntityID  = (UINT32)((slotNum << 8) | tcontId);
        tcont.Policy    = TCONT_POLICY_NULL;

        omci_SetTcontInfo(tcontId, tcont.EntityID, TCONT_ALLOC_ID_984_RESERVED);

        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_TCONT_INDEX, &tcont, sizeof(MIB_TABLE_TCONT_T)));
        OMCI_MeOperCfg(MIB_TABLE_TCONT_INDEX, NULL, &tcont, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_TCONT_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

        // Reset Traffic Scheduler
        omci_PonResetTrafficScheduler(slot, tcontId);

        // Reset Upstream Priority Queue
        // TBD, deprecate original equally dispatch method
        //omci_PonResetPriQ(slot, tcontId, tcont.EntityID, gInfo.devCapabilities.totalTContQueueNum / gInfo.devCapabilities.totalTContNum);
        if (type == PRIVATE_TQCFG_TYPE_CUSTOMIZED_DEFINITION)
            omci_PonResetPriQ(slot, tcontId, (slotNum << 8) | tcontId, tqCfg.QueueNumPerTcont[tcontId]);
        else
            omci_PonResetPriQ(slot, tcontId, (slotNum << 8) | tcontId, 8);
    }
#if 0

    if (type == PRIVATE_TQCFG_TYPE_CUSTOMIZED_DEFINITION)
        return GOS_OK;

#ifdef OMCI_X86
    omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 1);
    omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 1);
    omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 1);
    omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 1);
    omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 4);
    omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 4);
    omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 8);
    omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 8);
#else
    // TBD, should be read from config file
    if (RTL9601B_CHIP_ID == gInfo.devIdVersion.chipId)
    {
        omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 1);
        omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 1);
        omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 1);
        omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 1);
        omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 4);
        omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 4);
        omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 8);
        omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 8);
    }
    else if (RTL9602C_CHIP_ID == gInfo.devIdVersion.chipId)
    {
        if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode)
        {
            omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 8);
            omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 8);
            omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 4);
            omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 4);
            omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 2);
            omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 2);
            omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 1);
            omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 1);
            omci_PonResetPriQ(slot, 8, (slotNum << 8) | 8, 1);
            omci_PonResetPriQ(slot, 9, (slotNum << 8) | 9, 1);
            omci_PonResetPriQ(slot, 10, (slotNum << 8) | 10, 2);
            omci_PonResetPriQ(slot, 11, (slotNum << 8) | 11, 2);
            omci_PonResetPriQ(slot, 12, (slotNum << 8) | 12, 4);
            omci_PonResetPriQ(slot, 13, (slotNum << 8) | 13, 4);
            omci_PonResetPriQ(slot, 14, (slotNum << 8) | 14, 8);
            omci_PonResetPriQ(slot, 15, (slotNum << 8) | 15, 8);
        }
        else
        {
            // in order to support veip queue expansion
            // 24 queues are reseved for future allocation
            omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 4);
            omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 4);
            omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 2);
            omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 1);
            omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 1);
            omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 1);
            omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 1);
            omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 1);
            omci_PonResetPriQ(slot, 8, (slotNum << 8) | 8, 1);
            omci_PonResetPriQ(slot, 9, (slotNum << 8) | 9, 1);
            omci_PonResetPriQ(slot, 10, (slotNum << 8) | 10, 1);
            omci_PonResetPriQ(slot, 11, (slotNum << 8) | 11, 1);
            omci_PonResetPriQ(slot, 12, (slotNum << 8) | 12, 1);
            omci_PonResetPriQ(slot, 13, (slotNum << 8) | 13, 2);
            omci_PonResetPriQ(slot, 14, (slotNum << 8) | 14, 4);
            omci_PonResetPriQ(slot, 15, (slotNum << 8) | 15, 4);
        }
    }
    else
    {
        if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode)
        {
            omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 8);
            omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 8);
            omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 8);
            omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 8);
            omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 4);
            omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 4);
            omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 4);
            omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 4);
            omci_PonResetPriQ(slot, 8, (slotNum << 8) | 8, 2);
            omci_PonResetPriQ(slot, 9, (slotNum << 8) | 9, 2);
            omci_PonResetPriQ(slot, 10, (slotNum << 8) | 10, 2);
            omci_PonResetPriQ(slot, 11, (slotNum << 8) | 11, 2);
            omci_PonResetPriQ(slot, 12, (slotNum << 8) | 12, 2);
            omci_PonResetPriQ(slot, 13, (slotNum << 8) | 13, 2);
            omci_PonResetPriQ(slot, 14, (slotNum << 8) | 14, 1);
            omci_PonResetPriQ(slot, 15, (slotNum << 8) | 15, 1);
            omci_PonResetPriQ(slot, 16, (slotNum << 8) | 16, 1);
            omci_PonResetPriQ(slot, 17, (slotNum << 8) | 17, 2);
            omci_PonResetPriQ(slot, 18, (slotNum << 8) | 18, 2);
            omci_PonResetPriQ(slot, 19, (slotNum << 8) | 19, 2);
            omci_PonResetPriQ(slot, 20, (slotNum << 8) | 20, 2);
            omci_PonResetPriQ(slot, 21, (slotNum << 8) | 21, 2);
            omci_PonResetPriQ(slot, 22, (slotNum << 8) | 22, 2);
            omci_PonResetPriQ(slot, 23, (slotNum << 8) | 23, 4);
            omci_PonResetPriQ(slot, 24, (slotNum << 8) | 24, 4);
            omci_PonResetPriQ(slot, 25, (slotNum << 8) | 25, 4);
            omci_PonResetPriQ(slot, 26, (slotNum << 8) | 26, 4);
            omci_PonResetPriQ(slot, 27, (slotNum << 8) | 27, 8);
            omci_PonResetPriQ(slot, 28, (slotNum << 8) | 28, 8);
            omci_PonResetPriQ(slot, 29, (slotNum << 8) | 29, 8);
            omci_PonResetPriQ(slot, 30, (slotNum << 8) | 30, 8);
        }
        else
        {
            // in order to support veip queue expansion
            // 24 queues are reseved for future allocation
            omci_PonResetPriQ(slot, 0, (slotNum << 8) | 0, 8);
            omci_PonResetPriQ(slot, 1, (slotNum << 8) | 1, 8);
            omci_PonResetPriQ(slot, 2, (slotNum << 8) | 2, 8);
            omci_PonResetPriQ(slot, 3, (slotNum << 8) | 3, 8);
            omci_PonResetPriQ(slot, 4, (slotNum << 8) | 4, 4);
            omci_PonResetPriQ(slot, 5, (slotNum << 8) | 5, 4);
            omci_PonResetPriQ(slot, 6, (slotNum << 8) | 6, 2);
            omci_PonResetPriQ(slot, 7, (slotNum << 8) | 7, 2);
            omci_PonResetPriQ(slot, 8, (slotNum << 8) | 8, 1);
            omci_PonResetPriQ(slot, 9, (slotNum << 8) | 9, 1);
            omci_PonResetPriQ(slot, 10, (slotNum << 8) | 10, 1);
            omci_PonResetPriQ(slot, 11, (slotNum << 8) | 11, 1);
            omci_PonResetPriQ(slot, 12, (slotNum << 8) | 12, 1);
            omci_PonResetPriQ(slot, 13, (slotNum << 8) | 13, 1);
            omci_PonResetPriQ(slot, 14, (slotNum << 8) | 14, 1);
            omci_PonResetPriQ(slot, 15, (slotNum << 8) | 15, 1);
            omci_PonResetPriQ(slot, 16, (slotNum << 8) | 16, 1);
            omci_PonResetPriQ(slot, 17, (slotNum << 8) | 17, 1);
            omci_PonResetPriQ(slot, 18, (slotNum << 8) | 18, 1);
            omci_PonResetPriQ(slot, 19, (slotNum << 8) | 19, 1);
            omci_PonResetPriQ(slot, 20, (slotNum << 8) | 20, 1);
            omci_PonResetPriQ(slot, 21, (slotNum << 8) | 21, 1);
            omci_PonResetPriQ(slot, 22, (slotNum << 8) | 22, 1);
            omci_PonResetPriQ(slot, 23, (slotNum << 8) | 23, 2);
            omci_PonResetPriQ(slot, 24, (slotNum << 8) | 24, 2);
            omci_PonResetPriQ(slot, 25, (slotNum << 8) | 25, 4);
            omci_PonResetPriQ(slot, 26, (slotNum << 8) | 26, 4);
            omci_PonResetPriQ(slot, 27, (slotNum << 8) | 27, 8);
            omci_PonResetPriQ(slot, 28, (slotNum << 8) | 28, 8);
            omci_PonResetPriQ(slot, 29, (slotNum << 8) | 29, 8);
            omci_PonResetPriQ(slot, 30, (slotNum << 8) | 30, 8);
        }
    }
#endif
#endif
    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetAnig(UINT32 slot, UINT32 port)
{
    MIB_TABLE_ANIG_T anig;
    UINT32           slotNum;
    UINT32           usDBRuStatus;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: ANI-G [%u]", port);

    MIB_Default(MIB_TABLE_ANIG_INDEX, &anig, sizeof(MIB_TABLE_ANIG_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    anig.EntityID       = (UINT16)((slotNum << 8) | (port + 1));
    anig.NumOfTcont     = gInfo.devCapabilities.totalTContNum;

    /*Reset GemBlkLen to default value 48*/
    anig.GemBlkLen = 48;
    omci_wrapper_setGemBlkLen(anig.GemBlkLen);
    /*Get from ASIC again to check setting, because some chip only support specific gem block length*/
    omci_wrapper_getGemBlkLen(&anig.GemBlkLen);

    /*Get DBRu State*/
    omci_wrapper_getUsDBRuStatus(&usDBRuStatus);
    anig.SRInd = usDBRuStatus;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ANIG_INDEX, &anig, sizeof(MIB_TABLE_ANIG_T)));
    OMCI_MeOperCfg(MIB_TABLE_ANIG_INDEX, NULL, &anig, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ANIG_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetPptpEth(UINT32 slot, UINT32 port)
{
    MIB_TABLE_ETHUNI_T                      ethUni;
    MIB_TABLE_LOOP_DETECT_T                 loop;
    MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T ontStatistics;
    UINT32                                  slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: PPTP ETH UNI [%#x,%#x]", slot, port);

    MIB_Default(MIB_TABLE_ETHUNI_INDEX, &ethUni, sizeof(MIB_TABLE_ETHUNI_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);
    feature_api(FEATURE_API_ME_00020000,(INT32)-1, (UINT32)1, &slot, &port);


    ethUni.EntityID         = (UINT16)((slotNum << 8) | (port + 1));
    ethUni.SensedType       = TXC_GET_CARDTYPE_BY_SLOT_ID(slot);
    ethUni.ExpectedType     = TXC_GET_CARDTYPE_BY_SLOT_ID(slot);

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ETHUNI_INDEX, &ethUni, sizeof(MIB_TABLE_ETHUNI_T)));
    OMCI_MeOperCfg(MIB_TABLE_ETHUNI_INDEX, NULL, &ethUni, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ETHUNI_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    // Reset Downstream Priority Queue of PPTP ETH
    omci_PonResetPriQ(slot, port, ethUni.EntityID, gInfo.devCapabilities.perUNIQueueNum);


    // Reset Loop Detect
    MIB_Default(MIB_TABLE_LOOP_DETECT_INDEX, &loop, sizeof(MIB_TABLE_LOOP_DETECT_T));
    loop.EntityID = ethUni.EntityID;
    loop.EthLoopDetectCfg = FALSE;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_LOOP_DETECT_INDEX, &loop, sizeof(MIB_TABLE_LOOP_DETECT_T)));
    OMCI_MeOperCfg(MIB_TABLE_LOOP_DETECT_INDEX, NULL, &loop, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_LOOP_DETECT_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    // Reset private tellion ont statistics
    MIB_Default(MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX, &ontStatistics, sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T));
    ontStatistics.EntityId = ethUni.EntityID;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX, &ontStatistics, sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T)));
    OMCI_MeOperCfg(MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX, NULL, &ontStatistics, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetVeIP(UINT32 slot, UINT32 port)
{
    MIB_TABLE_VEIP_T    veip;
    UINT32              slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: VEIP [%#x,%#x]", slot, port);

    MIB_Default(MIB_TABLE_VEIP_INDEX, &veip, sizeof(MIB_TABLE_VEIP_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    feature_api(FEATURE_API_ME_00000100, &slotNum);

    veip.EntityId   = (UINT16)((slotNum << 8) | (port + 1));

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_VEIP_INDEX, &veip, sizeof(MIB_TABLE_VEIP_T)));
    OMCI_MeOperCfg(MIB_TABLE_VEIP_INDEX, NULL, &veip, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_VEIP_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    // Reset Downstream Priority Queue of VEIP
    omci_PonResetPriQ(slot, port, veip.EntityId, gInfo.devCapabilities.perUNIQueueNum);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetPptpPots(UINT32 slot, UINT32 port)
{
    MIB_TABLE_POTSUNI_T       mibPots;
    MIB_TABLE_VOIPLINESTATUS_T    mibVls;
    UINT32                          slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: PPTP POTS UNI [%#x,%#x]", slot, port);

    MIB_Default(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T));
    MIB_Default(MIB_TABLE_VOIPLINESTATUS_INDEX, &mibVls, sizeof(MIB_TABLE_VOIPLINESTATUS_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    mibPots.EntityId = (UINT16)((slotNum << 8) | (port + 1));
    mibVls.EntityId = mibPots.EntityId;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T)));
    OMCI_MeOperCfg(MIB_TABLE_POTSUNI_INDEX, NULL, &mibPots, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_POTSUNI_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_VOIPLINESTATUS_INDEX, &mibVls, sizeof(MIB_TABLE_VOIPLINESTATUS_T)));
    OMCI_MeOperCfg(MIB_TABLE_VOIPLINESTATUS_INDEX, NULL, &mibVls, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_VOIPLINESTATUS_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetPptpLct(UINT32 slot, UINT32 port)
{
    MIB_TABLE_LCTUNI_T  mibLctUni;
    UINT32              slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: PPTP LCT UNI(%u) [%#x,%#x]", MIB_TABLE_PPTP_LCT_UNI_INDEX, slot, port);

    MIB_Default(MIB_TABLE_PPTP_LCT_UNI_INDEX, &mibLctUni, sizeof(MIB_TABLE_LCTUNI_T));

    //slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);
    slotNum = slot;

    mibLctUni.EntityID = (UINT16)((slotNum << 8) | port);

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PPTP_LCT_UNI_INDEX, &mibLctUni, sizeof(MIB_TABLE_LCTUNI_T)));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetUniG(UINT32 slot, UINT32 port)
{
    MIB_TABLE_UNIG_T    uniG;
    UINT32              slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: UNI-G [%#x,%#x]", slot, port);

    MIB_Default(MIB_TABLE_UNIG_INDEX, &uniG, sizeof(MIB_TABLE_UNIG_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slot);

    feature_api(FEATURE_API_ME_00020000, (INT32)-1, (UINT32)1, &slot, &port);

    uniG.EntityID           = (UINT16)((slotNum << 8) | (port + 1));
    if (TXC_CARDHLD_VEIP_SLOT == slot)
    {
        feature_api(FEATURE_API_ME_00000100, &slotNum);

        uniG.EntityID           = (UINT16)((slotNum << 8) | (port + 1));
        uniG.ManageCapability   = UNIG_MGMT_CAPABILITY_OMCI_ONLY;
    }
    else
    {
        if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode)
            uniG.ManageCapability   = UNIG_MGMT_CAPABILITY_OMCI_ONLY;
        else if (OMCI_DEV_MODE_ROUTER == gInfo.devMode)
        {
            uniG.ManageCapability   = UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY;
            if (TXC_CARDHLD_ETH_FE_SLOT == slot || TXC_CARDHLD_ETH_GE_SLOT == slot)
            {
                UINT32 uniPort;
                if (FAL_OK == feature_api(FEATURE_API_RDP_00000001_X_BDG_CONN, gInfo.devMode, uniG.EntityID, &uniPort))
                {
                    uniG.ManageCapability = UNIG_MGMT_CAPABILITY_OMCI_ONLY;
                }
            }
        }
        else
            uniG.ManageCapability   = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

        if (TXC_CARDHLD_POTS_SLOT == slot)
            uniG.ManageCapability   = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;
    }

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_UNIG_INDEX, &uniG, sizeof(MIB_TABLE_UNIG_T)));
    OMCI_MeOperCfg(MIB_TABLE_UNIG_INDEX, NULL, &uniG, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_UNIG_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetPptp(void)
{
    INT32 slotId, portId;

    /*Set UNI side first, dor create DS PriQ first, because entity id is smaller*/
    /*Set UNI side*/
    for (slotId = 1; slotId < TXC_CARDHLD_SLOT_NUM; slotId++)
    {
        if (TXC_CARDHLD_ETH_FE_SLOT == slotId)
        {
            for (portId = 0; portId < gInfo.devCapabilities.fePortNum; portId++)
            {
                omci_PonResetUniG(slotId, portId);
                omci_PonResetPptpEth(slotId, portId);
            }
        }
        else if (TXC_CARDHLD_ETH_GE_SLOT == slotId)
        {
            for (portId = 0; portId < gInfo.devCapabilities.gePortNum; portId++)
            {
                omci_PonResetUniG(slotId, portId);
                omci_PonResetPptpEth(slotId, portId);
            }
        }
        else if (TXC_CARDHLD_VEIP_SLOT == slotId)
        {
            if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP))
            {
                if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode &&
                    (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_BDP_00000100)))
                    continue;
            }
            else
            {
                if (FAL_FAIL == feature_api(FEATURE_API_RDP_00000001_RESET_PPTP, gInfo.devMode))
                    continue;
            }

            omci_PonResetUniG(slotId, 0);
            omci_PonResetVeIP(slotId, 0);
        }
        else if (TXC_CARDHLD_POTS_SLOT == slotId)
        {

            MIB_TABLE_INFO_T *pTableInfo = NULL;

            pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_POTSUNI_INDEX);

            if (pTableInfo)
            {
                for (portId = 0; portId < gInfo.devCapabilities.potsPortNum; portId++)
                {
                    omci_PonResetUniG(slotId, portId);
                    omci_PonResetPptpPots(slotId, portId);
                }
            }
        }
    }
    omci_PonResetPptpLct(0, 0);
    /*Set ANI side*/
    slotId = TXC_CARDHLD_PON_SLOT;
    omci_PonResetTcont(slotId);
    omci_PonResetAnig(slotId, 0);

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetSwImage(void)
{
    MIB_TABLE_SWIMAGE_T mibSoftwareImage;
    char                flagName[OMCI_SW_DL_IMAGE_UBOOT_FLAG_NAME];
    char                flagBuffer[OMCI_SW_DL_IMAGE_UBOOT_FLAG_BUFFER];
    char                *pStr;
    UINT8               commitLater = 2;
    UINT8               activeImage = 0;
    UINT8               commitImage = 0;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: Software image");

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
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Modify sw_commit uboot flag fail");
            }
        }

        // erase sw_commit_later flag
        sprintf(flagName, "sw_commit_later");
        sprintf(flagBuffer, "%s", "");
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Delete sw_commit_later uboot flag fail");
        }
    }

    // check if sw_commit is exist and its value
    sprintf(flagName, "sw_commit");
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        commitImage = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    // image 0
    MIB_Default(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));

    mibSoftwareImage.EntityID = (UINT16) 0;
    sprintf(mibSoftwareImage.Version, TXC_DEFAULT_SW_VERSION);
    mibSoftwareImage.Committed = (0 == commitImage) ? TRUE : FALSE;
    mibSoftwareImage.Active = (0 == activeImage) ? TRUE : FALSE;

    sprintf(flagName, "sw_version%u", mibSoftwareImage.EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        snprintf(mibSoftwareImage.Version, MIB_TABLE_SWIMAGE_VERSION_LEN+1, "%s", pStr);

        free(pStr);
    }

    sprintf(flagName, "sw_valid%u", mibSoftwareImage.EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        mibSoftwareImage.Valid = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    // make sure that an activated image must be also validated image
    if (mibSoftwareImage.Active && mibSoftwareImage.Valid != mibSoftwareImage.Active)
    {
        mibSoftwareImage.Valid = mibSoftwareImage.Active;

        sprintf(flagBuffer, "%u", mibSoftwareImage.Valid);
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Correct image valid state fail");
        }
    }

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)));
    OMCI_MeOperCfg(MIB_TABLE_SWIMAGE_INDEX, NULL, &mibSoftwareImage, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_SWIMAGE_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);


    // image 1
    MIB_Default(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T));

    mibSoftwareImage.EntityID = (UINT16) 1;
    sprintf(mibSoftwareImage.Version, TXC_DEFAULT_SW_VERSION);
    mibSoftwareImage.Committed = (1 == commitImage) ? TRUE : FALSE;
    mibSoftwareImage.Active = (1 == activeImage) ? TRUE : FALSE;

    sprintf(flagName, "sw_version%u", mibSoftwareImage.EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        snprintf(mibSoftwareImage.Version, MIB_TABLE_SWIMAGE_VERSION_LEN+1, "%s", pStr);

        free(pStr);
    }

    sprintf(flagName, "sw_valid%u", mibSoftwareImage.EntityID);
    if ((pStr = omci_sw_dl_uboot_env_get(flagName)))
    {
        mibSoftwareImage.Valid = strtoul(pStr, NULL, 0);

        free(pStr);
    }

    // make sure that an activated image must be also validated image
    if (mibSoftwareImage.Active && mibSoftwareImage.Valid != mibSoftwareImage.Active)
    {
        mibSoftwareImage.Valid = mibSoftwareImage.Active;

        sprintf(flagBuffer, "%u", mibSoftwareImage.Valid);
        if (omci_sw_dl_uboot_env_set(flagName, flagBuffer))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Correct image valid state fail");
        }
    }

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &mibSoftwareImage, sizeof(MIB_TABLE_SWIMAGE_T)));
    OMCI_MeOperCfg(MIB_TABLE_SWIMAGE_INDEX, NULL, &mibSoftwareImage, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_SWIMAGE_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);


    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetOltG(void)
{
    MIB_TABLE_OLTG_T oltG;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: OLT-G");

    MIB_Default(MIB_TABLE_OLTG_INDEX, &oltG, sizeof(MIB_TABLE_OLTG_T));

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_OLTG_INDEX, &oltG, sizeof(MIB_TABLE_OLTG_T)));
    OMCI_MeOperCfg(MIB_TABLE_OLTG_INDEX, NULL, &oltG, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_OLTG_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetTR069Server(void)
{
    MIB_TABLE_TR069MANAGESERVER_T TR069Server;
    UINT32 slotNum;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: BBF TR-069 management server");

    MIB_Default(MIB_TABLE_TR069MANAGESERVER_INDEX, &TR069Server, sizeof(MIB_TABLE_TR069MANAGESERVER_T));

    slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_VEIP_SLOT);

    feature_api(FEATURE_API_ME_00000100, &slotNum);

    TR069Server.EntityID   = (UINT16)((slotNum << 8) | 1);

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_TR069MANAGESERVER_INDEX, &TR069Server, sizeof(MIB_TABLE_TR069MANAGESERVER_T)));
    OMCI_MeOperCfg(MIB_TABLE_TR069MANAGESERVER_INDEX, NULL, &TR069Server, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_TR069MANAGESERVER_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetIpHostCfgData(void)
{
    MIB_TABLE_IP_HOST_CFG_DATA_T    mibIpHostCfgData;
    UINT16                          id;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: IP host config data");

    MIB_Default(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHostCfgData, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T));

    for (id = 0; id < TXC_IPHOST_NUM; id++)
    {
        mibIpHostCfgData.EntityID = id;

        if (GOS_OK != MIB_Set(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHostCfgData, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
            continue;

        OMCI_MeOperCfg(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, NULL, &mibIpHostCfgData, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetLoid(void)
{
    MIB_TABLE_LOIDAUTH_T loid;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Resetting LOID MIB");

    MIB_Default(MIB_TABLE_LOIDAUTH_INDEX, &loid, sizeof(MIB_TABLE_LOIDAUTH_T));

    loid.EntityId = (UINT16) 0;
    loid.AuthStatus = gInfo.loidCfg.loidAuthStatus;

    loid.OperationId[0] = 'C';
    loid.OperationId[1] = 'T';
    loid.OperationId[2] = 'C';
    loid.OperationId[3] = ' ';

    memcpy(loid.LoID, gInfo.loidCfg.loid, MIB_TABLE_LOIDAUTH_LOID_LEN);
    memcpy(loid.Password, gInfo.loidCfg.loidPwd, MIB_TABLE_LOIDAUTH_PASSWORD_LEN);

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_LOIDAUTH_INDEX, &loid, sizeof(MIB_TABLE_LOIDAUTH_T)));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetOnuRemoteDbg(void)
{
    MIB_TABLE_ONU_REMOTE_DBG_T  mibOrd;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: ONU remote debug");

    MIB_Default(MIB_TABLE_ONU_REMOTE_DBG_INDEX, &mibOrd, sizeof(MIB_TABLE_ONU_REMOTE_DBG_T));

    mibOrd.EntityId = 0;
    mibOrd.CmdFmt = ORD_CMT_FMT_ASCII_STRING;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONU_REMOTE_DBG_INDEX, &mibOrd, sizeof(MIB_TABLE_ONU_REMOTE_DBG_T)));
    OMCI_MeOperCfg(MIB_TABLE_ONU_REMOTE_DBG_INDEX, NULL, &mibOrd, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ONU_REMOTE_DBG_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetOnuPwrShedding(void)
{
    MIB_TABLE_ONU_PWR_SHEDDING_T  mibOps;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: ONU power shedding");

    MIB_Default(MIB_TABLE_ONU_PWR_SHEDDING_INDEX, &mibOps, sizeof(MIB_TABLE_ONU_PWR_SHEDDING_T));

    mibOps.EntityId = 0;

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ONU_PWR_SHEDDING_INDEX, &mibOps, sizeof(MIB_TABLE_ONU_PWR_SHEDDING_T)));
    OMCI_MeOperCfg(MIB_TABLE_ONU_PWR_SHEDDING_INDEX, NULL, &mibOps, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ONU_PWR_SHEDDING_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetVoIP(void)
{
    MIB_TABLE_VOIPCONFIGDATA_T   mibVcd;
    MIB_TABLE_INFO_T            *pTableInfo = NULL;

    pTableInfo = MIB_GetTableInfoPtr(MIB_TABLE_VOIPCONFIGDATA_INDEX);

    if (!pTableInfo)
        return GOS_OK;

    if (gInfo.devCapabilities.potsPortNum > 0)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: VoIP config data");

        MIB_Default(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVcd, sizeof(MIB_TABLE_VOIPCONFIGDATA_T));

        mibVcd.EntityId = 0;

        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVcd, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)));
        OMCI_MeOperCfg(MIB_TABLE_VOIPCONFIGDATA_INDEX, NULL, &mibVcd, MIB_ADD,
            omci_GetOltAccAttrSet(MIB_TABLE_VOIPCONFIGDATA_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetVoipImage(void)
{
    feature_api(FEATURE_API_ME_00000020_IS_ENABLED);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetExtOnuG(void)
{
    feature_api(FEATURE_API_ME_00000400_IS_ENABLED);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetOmciMe(void)
{
    MIB_TABLE_OMCI_T mibOmci;
    MIB_TABLE_INDEX tableIndex;
    omci_meType_tbl_entry_t *pEntry,*pTmpEntry;
    UINT32  meType, meClassId;
    UINT16  i, isBiggest=0;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: OMCI ME");

    MIB_Default(MIB_TABLE_OMCI_INDEX, &mibOmci, sizeof(MIB_TABLE_OMCI_T));

    mibOmci.EntityId = 0;
    mibOmci.MeTypeTbl_size = 0;

    /*Set Me Type table*/
    for(i = 0, tableIndex = MIB_TABLE_FIRST_INDEX; i < MIB_TABLE_TOTAL_NUMBER;
        i++, tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        meType = MIB_GetTableStdType(tableIndex);
        if (meType & OMCI_ME_TYPE_PRIVATE)
        {
            continue;
        }
        mibOmci.MeTypeTbl_size ++;
        meClassId = MIB_GetTableClassId(tableIndex);

        pEntry = (omci_meType_tbl_entry_t*)malloc(sizeof(omci_meType_tbl_entry_t));
        if(!pEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Malloc omci_meType_tbl Entry Fail");
            return GOS_FAIL;
        }
        pEntry->tableEntry.meTypeTbl = meClassId;

        /*Insert by class id order*/
        LIST_FOREACH(pTmpEntry, &mibOmci.meType_head, entries)
        {
            if(pTmpEntry->tableEntry.meTypeTbl >= meClassId)
            {
                break;
            }
            if(NULL == LIST_NEXT(pTmpEntry, entries))
            {
                isBiggest = 1;
                break;
            }
        }
        if(isBiggest)
        {
            LIST_INSERT_AFTER(pTmpEntry, pEntry, entries);
            isBiggest = 0;
        }
        else if(pTmpEntry)
        {
            LIST_INSERT_BEFORE(pTmpEntry, pEntry, entries);
        } else {
            LIST_INSERT_HEAD(&mibOmci.meType_head, pEntry, entries);
        }

    }

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_OMCI_INDEX, &mibOmci, sizeof(MIB_TABLE_OMCI_T)));

    return GOS_OK;
}


static GOS_ERROR_CODE omci_PonResetOltLocationCfgData(void)
{
    MIB_TABLE_OLTLOCATIONCFGDATA_T mibOltLocationCfgData;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: OLT Location config data");

    MIB_Default(MIB_TABLE_OLT_LOCATION_CFG_DATA_INDEX, &mibOltLocationCfgData, sizeof(MIB_TABLE_OLTLOCATIONCFGDATA_T));

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_OLT_LOCATION_CFG_DATA_INDEX, &mibOltLocationCfgData, sizeof(MIB_TABLE_OLTLOCATIONCFGDATA_T)));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_get_boa_mib_value_by_name(char *pName, char *pVal)
{
    char                buffer[OMCI_BOA_MIB_BUFFER_LEN];
    char                cmd[OMCI_BOA_MIB_BUFFER_LEN];
    char                *p = NULL;
    FILE                *pFd = NULL;

    if (!pName)
        return GOS_ERR_PARAM;

    memset(buffer, 0, OMCI_BOA_MIB_BUFFER_LEN);
    memset(cmd, 0, OMCI_BOA_MIB_BUFFER_LEN);
#ifdef OMCI_X86
    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "X86 not support flash get %s", pName);
#else
    snprintf(cmd, sizeof(cmd), "flash get %s", pName);

    if (NULL != (pFd = popen(cmd, "r")))
    {
        if (fgets(buffer, sizeof(buffer), pFd))
        {
            if (!(p = strstr(buffer, "=")))
            {
                pclose(pFd);
                return GOS_FAIL;
            }
            p++;
            strncpy(pVal, p, strlen(p));
            pVal[strlen(p) - 1] = '\0';
            pclose(pFd);
            return GOS_OK;
        }
        pclose(pFd);
    }
#endif
    return GOS_FAIL;
}

static void omci_PonRegistrationSyncCircuitPack(MIB_ATTR_INDEX attr_idx)
{
    MIB_TABLE_ONTG_T        ontg;
    MIB_TABLE_SWIMAGE_T     swImg0, swImg1;
    MIB_TABLE_CIRCUITPACK_T circuitPack;
    UINT32                  slotNum;
    UINT32                  slotId;
    BOOL                    bWrite;

    ontg.EntityID = TXC_ONUG_INSTANCE_ID;

    if (GOS_OK != MIB_Get(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)))
        return;

    swImg0.EntityID = 0;
    if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &swImg0, sizeof(MIB_TABLE_SWIMAGE_T)))
        return;

    swImg1.EntityID = 1;
    if (GOS_OK != MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &swImg1, sizeof(MIB_TABLE_SWIMAGE_T)))
        return;

    /*Set UNI slot*/
    for (slotId = TXC_CARDHLD_PON_SLOT; slotId < TXC_CARDHLD_SLOT_NUM; slotId++)
    {
        bWrite = FALSE;

        slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(slotId);

        circuitPack.EntityID    = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotNum;

        if (GOS_OK != MIB_Get(MIB_TABLE_CIRCUITPACK_INDEX, &circuitPack, sizeof(MIB_TABLE_CIRCUITPACK_T)))
            continue;

        if (MIB_TABLE_CIRCUITPACK_VERSION_INDEX == attr_idx)
        {
            memset(circuitPack.Version, 0, MIB_TABLE_CIRCUITPACK_VERSION_LEN+1);
            if (swImg0.Active)
            {
                snprintf(circuitPack.Version, MIB_TABLE_CIRCUITPACK_VERSION_LEN+1, "%s", swImg0.Version);
            }
            else
            {
                snprintf(circuitPack.Version, MIB_TABLE_CIRCUITPACK_VERSION_LEN+1, "%s", swImg1.Version);
            }

            bWrite = TRUE;
        }

        if (MIB_TABLE_CIRCUITPACK_VID_INDEX == attr_idx)
        {
            memset(circuitPack.VID, 0, MIB_TABLE_CIRCUITPACK_VID_LEN+1);
            snprintf(circuitPack.VID, MIB_TABLE_CIRCUITPACK_VID_LEN+1, "%s", ontg.VID);
            bWrite = TRUE;
        }

        if (bWrite)
        {
            MIB_Set(MIB_TABLE_CIRCUITPACK_INDEX, &circuitPack, sizeof(MIB_TABLE_CIRCUITPACK_T));
        }
    }

    return;
}

static GOS_ERROR_CODE omci_PonResetRegistrationInfo(void)
{
    BOOL                bWrite = FALSE;

    MIB_TABLE_ONTG_T    ontg;
    MIB_TABLE_ONT2G_T   ont2g;
    MIB_TABLE_SWIMAGE_T swImage;
    char                boaMibVal[OMCI_BOA_MIB_BUFFER_LEN];

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: RegistrationInfo");

    ontg.EntityID = TXC_ONUG_INSTANCE_ID;

    if (GOS_OK == MIB_Get(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)))
    {
        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("HW_HWVER", boaMibVal))
        {
            memset(ontg.Version, 0, MIB_TABLE_ONTG_VERSION_LEN+1);
            snprintf(ontg.Version, MIB_TABLE_ONTG_VERSION_LEN+1, "%s", boaMibVal);
            MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T));
        }

        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("PON_VENDOR_ID", boaMibVal))
        {
            memset(ontg.VID, 0, MIB_TABLE_ONTG_VID_LEN+1);
            snprintf(ontg.VID, MIB_TABLE_ONTG_VID_LEN+1, "%s", boaMibVal);
            MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T));
            omci_PonRegistrationSyncCircuitPack(MIB_TABLE_CIRCUITPACK_VID_INDEX);
        }

        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_TM_OPT", boaMibVal))
        {
            switch (strtol(boaMibVal, NULL, 0))
            {
                case 0:
                    ontg.TraffMgtOpt = ONUG_TM_OPTION_PRIORITY_CONTROLLED;
                    break;
                case 1:
                    ontg.TraffMgtOpt = ONUG_TM_OPTION_RATE_CONTROLLED;
                    break;
                case 2:
                default:
                    ontg.TraffMgtOpt = ONUG_TM_OPTION_PRIORITY_RATE_CONTROLLED;
                    break;
            }
            MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T));
        }
    }

    ont2g.EntityID = TXC_ONU2G_INSTANCE_ID;
    bWrite = FALSE;
    if (GOS_OK == MIB_Get(MIB_TABLE_ONT2G_INDEX, &ont2g, sizeof(MIB_TABLE_ONT2G_T)))
    {
        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("OMCC_VER", boaMibVal))
        {
            ont2g.OMCCVer = strtol(boaMibVal, NULL, 0);
            bWrite = TRUE;
        }

        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("GPON_ONU_MODEL", boaMibVal))
        {

            memset(ont2g.EqtID, 0, MIB_TABLE_ONT2G_EQTID_LEN+1);
            snprintf(ont2g.EqtID, MIB_TABLE_ONT2G_EQTID_LEN+1, "%s", boaMibVal);
            bWrite = TRUE;
        }

        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_VENDOR_PRODUCT_CODE", boaMibVal))
        {
            ont2g.VPCode = strtol(boaMibVal, NULL, 0);
            bWrite = TRUE;
        }

        if (bWrite)
        {
            MIB_Set(MIB_TABLE_ONT2G_INDEX, &ont2g, sizeof(MIB_TABLE_ONT2G_T));
        }
    }

    swImage.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &swImage, sizeof(MIB_TABLE_SWIMAGE_T)))
    {
        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_SW_VER1", boaMibVal))
        {
            memset(swImage.Version, 0, MIB_TABLE_SWIMAGE_VERSION_LEN+1);
            snprintf(swImage.Version, MIB_TABLE_SWIMAGE_VERSION_LEN+1, "%s", boaMibVal);
            MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &swImage, sizeof(MIB_TABLE_SWIMAGE_T));
            omci_PonRegistrationSyncCircuitPack(MIB_TABLE_CIRCUITPACK_VERSION_INDEX);
        }
    }

    memset(&swImage, 0, sizeof(MIB_TABLE_SWIMAGE_T));
    swImage.EntityID = 1;
    if (GOS_OK == MIB_Get(MIB_TABLE_SWIMAGE_INDEX, &swImage, sizeof(MIB_TABLE_SWIMAGE_T)))
    {
        memset(boaMibVal, 0 , OMCI_BOA_MIB_BUFFER_LEN);
        if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_SW_VER2", boaMibVal))
        {
            memset(swImage.Version, 0, MIB_TABLE_SWIMAGE_VERSION_LEN+1);
            snprintf(swImage.Version, MIB_TABLE_SWIMAGE_VERSION_LEN+1, "%s", boaMibVal);
            MIB_Set(MIB_TABLE_SWIMAGE_INDEX, &swImage, sizeof(MIB_TABLE_SWIMAGE_T));
            omci_PonRegistrationSyncCircuitPack(MIB_TABLE_CIRCUITPACK_VERSION_INDEX);
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_PonResetMib(void)
{
    GOS_ERROR_CODE ret;


    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "PON Starting reset MIB");

    ret = omci_PonResetOntData();
    GOS_ASSERT(ret == GOS_OK);

    g_numOfUsPQ = 0;
    g_numOfDsPQ = 0;

    ret = omci_PonResetPptp();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetSlotPort();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetOntGOnt2G();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetSwImage();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetOltG();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetTR069Server();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetIpHostCfgData();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetLoid();
    GOS_ASSERT(ret == GOS_OK);

    GOS_ASSERT((FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_ME_00000040_IS_SUPPORTED) ||
        (FAL_OK == feature_api(FEATURE_API_ME_00000040_IS_SUPPORTED))));

    ret = omci_PonResetOnuRemoteDbg();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetOnuPwrShedding();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetVoIP();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetVoipImage();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetExtOnuG();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetOmciMe();
    GOS_ASSERT(ret == GOS_OK);

    ret = omci_PonResetOltLocationCfgData();
    GOS_ASSERT(ret == GOS_OK);

    proprietary_list_proc(PROPRIETARY_MIB_CB_RESET);

    /* initialize MIB from mib load file */
    omci_mib_cfg_load_file();
    /*Reset all tree and mib*/
    //MIB_ClearAll();
    //MIB_AvlTreeRemoveAll();

    ret = omci_PonResetRegistrationInfo();
    GOS_ASSERT(ret == GOS_OK);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_InitOntPrivateVlan(void)
{
    MIB_TABLE_PRIVATE_VLANCFG_T privateVlanCfg;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Initialize Private ME Vlan");

    MIB_Default(MIB_TABLE_PRIVATE_VLANCFG_INDEX, &privateVlanCfg, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T));
    memset(&privateVlanCfg, 0, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T));
    privateVlanCfg.Type = gInfo.iotVlanCfg.vlan_cfg_type;
    privateVlanCfg.ManualMode = gInfo.iotVlanCfg.vlan_cfg_manual_mode;
    privateVlanCfg.ManualTagVid = gInfo.iotVlanCfg.vlan_cfg_manual_vid;
    privateVlanCfg.ManualTagPri = gInfo.iotVlanCfg.vlan_cfg_manual_pri;
    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PRIVATE_VLANCFG_INDEX, &privateVlanCfg, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T)));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_InitOntPrivateTQ(void)
{
    MIB_TABLE_PRIVATE_TQCFG_T privateTqCfg;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Initialize Private Tcont Queue");

    MIB_Default(MIB_TABLE_PRIVATE_TQCFG_INDEX, &privateTqCfg, sizeof(MIB_TABLE_PRIVATE_TQCFG_T));

    memset(&privateTqCfg, 0, sizeof(MIB_TABLE_PRIVATE_TQCFG_T));

    /* TBD follow BOA configuration
    privateTqCfg.Type = gInfo.;
    privateTqCfg.PonSlotId = (PRIVATE_TQCFG_TYPE_RTK_DEFAULT_DEFINITION == privateTqCfg.Type ? TXC_CARDHLD_PON_SLOT_TYPE_ID ? gInfo. ;
    privateTqCfg.QueueNumPerTcont = gInfo.;
    */

    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_PRIVATE_TQCFG_INDEX, &privateTqCfg, sizeof(MIB_TABLE_PRIVATE_TQCFG_T)));

    return GOS_OK;
}

static GOS_ERROR_CODE omci_InitOntPrivateMe(void)
{
    GOS_ERROR_CODE ret = GOS_OK;

    if (GOS_OK != (ret = omci_InitOntPrivateTQ()))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Initialize Private Tcont Queue Failed!");
    }

    if (GOS_OK != (ret = omci_InitOntPrivateVlan()))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Initialize Private Vlan Failed!");
    }

    return ret;
}

static GOS_ERROR_CODE omci_InitOntMe(void)
{
    GOS_ERROR_CODE   ret;
    MIB_TABLE_ONTG_T ontg;

    /* init private me in onu */
    if (GOS_OK != omci_InitOntPrivateMe())
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Initialize Private MEs Failed!");
    }

    MIB_ClearPublic();

    /* RESET PON MIB */
    ret = omci_PonResetMib();
    GOS_ASSERT(ret == GOS_OK);

    ontg.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)))
    {
        ontg.OntState = PON_ONU_STATE_INITIAL;
        MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T));
    }

    return ret;
}

static GOS_ERROR_CODE omci_ResetMacLearnLimit(void)
{
    UINT32              portId;
    UINT16              i;
    omci_me_instance_t  instanceID;
    unig_attr_mgmt_capability_t mngCapUnig = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

    for (portId = 0; portId < gInfo.devCapabilities.fePortNum; portId++)
    {
        instanceID = (UINT16)(((TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_ETH_FE_SLOT)) << 8) | (portId + 1));
        //
        // Check manage Capability of UNI-G
        //
        if (omci_mngCapOfUnigGet ( instanceID, &mngCapUnig) != GOS_OK)
        {
            OMCI_LOG (
                OMCI_LOG_LEVEL_WARN,
                "Failed to get UNI-G for Eth UNI(0x%03x)\n",
                instanceID);
        }

        if ( UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY == mngCapUnig)
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_WARN, "%s: ManageCapability of UNIG(0x%03X) is NON_OMCI_ONLY",
                __FUNCTION__,
                instanceID);
            continue;
        }

        if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(instanceID, &i))
        {
            omci_wrapper_setMacLearnLimit(i, gInfo.devCapabilities.totalL2Num);
        }
    }

    for (portId = 0; portId < gInfo.devCapabilities.gePortNum; portId++)
    {
        instanceID = (UINT16)(((TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_ETH_GE_SLOT)) << 8) | (portId + 1));
        //
        // Check manage Capability of UNI-G
        //
        if (omci_mngCapOfUnigGet ( instanceID, &mngCapUnig) != GOS_OK)
        {
            OMCI_LOG (
                OMCI_LOG_LEVEL_WARN,
                "Failed to get UNI-G for Eth UNI(0x%03x)\n",
                instanceID);
        }

        if ( UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY == mngCapUnig)
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_WARN, "%s: ManageCapability of UNIG(0x%03X) is NON_OMCI_ONLY",
                __FUNCTION__,
                instanceID);
            continue;
        }

        if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(instanceID, &i))
        {
            omci_wrapper_setMacLearnLimit(i, gInfo.devCapabilities.totalL2Num);
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE omci_ResetAllMacFilter(void)
{
    MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T *pMibMacBridgePortFilterTable;
    macFilterTableEntry_t *pTmpMacFilterEntry, *pRemoveMacFilterEntry;
    MIB_TABLE_MACBRIPORTCFGDATA_T mbpcd;
    OMCI_MACFILTER_ts macFilterSetting;
    MIB_TABLE_T *pTable;
    MIB_ENTRY_T *pEntry;
    unig_attr_mgmt_capability_t mngCapUnig = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

    pTable = mib_GetTablePtr(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX);

    if(!pTable)
        return GOS_FAIL;

    LIST_FOREACH(pEntry,&pTable->entryHead,entries){
        pMibMacBridgePortFilterTable =  (MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T *)pEntry->pData;
        for(pTmpMacFilterEntry = LIST_FIRST(&pMibMacBridgePortFilterTable->head); pTmpMacFilterEntry!=NULL;){
            /*Set to ASIC*/
            macFilterSetting.id = pMibMacBridgePortFilterTable->EntityId;
            macFilterSetting.entryAct = MAC_FILTER_ENTRY_ACT_REMOVE;
            macFilterSetting.mac[0] = (pTmpMacFilterEntry->tableEntry.firstWord.bit.mac >> 8) & 0xFF;
            macFilterSetting.mac[1] = pTmpMacFilterEntry->tableEntry.firstWord.bit.mac & 0xFF;
            memcpy(&macFilterSetting.mac[2], &pTmpMacFilterEntry->tableEntry.secondWord.val, 4);
            mbpcd.EntityID = pMibMacBridgePortFilterTable->EntityId;
            if(GOS_OK == MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mbpcd, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T)))
            {
                if(MBPCD_TP_TYPE_PPTP_ETH_UNI == mbpcd.TPType)
                {
                    //
                    // Check manage Capability of UNI-G
                    //
                    if (omci_mngCapOfUnigGet ( mbpcd.TPPointer, &mngCapUnig) != GOS_OK)
                    {
                        OMCI_LOG (
                            OMCI_LOG_LEVEL_WARN,
                            "Failed to get UNI-G for Eth UNI(0x%03x)\n",
                            mbpcd.TPPointer);
                    }

                    if ( UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY == mngCapUnig)
                    {
                        OMCI_LOG(
                            OMCI_LOG_LEVEL_ERR, "%s: ManageCapability of UNIG(0x%03X) is NON_OMCI_ONLY",
                            __FUNCTION__,
                            mbpcd.TPPointer);
                        continue;
                    }

                    macFilterSetting.dir = MAC_FILTER_DIR_DS;
                }
                else if((MBPCD_TP_TYPE_GEM_IWTP == mbpcd.TPType) ||
                    (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == mbpcd.TPType))
                {
                    macFilterSetting.dir = MAC_FILTER_DIR_US;

                }
            }
            macFilterSetting.portMask = 0xf;
            macFilterSetting.macType = pTmpMacFilterEntry->tableEntry.firstWord.bit.isSa;
            omci_wrapper_setMacFilter(&macFilterSetting);

            pRemoveMacFilterEntry = pTmpMacFilterEntry;
            pTmpMacFilterEntry = LIST_NEXT(pTmpMacFilterEntry,entries);
            LIST_REMOVE(pRemoveMacFilterEntry,entries);
            free(pRemoveMacFilterEntry);
        }
        pMibMacBridgePortFilterTable->curMacTableEntryCnt = 0;
    }
    return GOS_OK;
}

static GOS_ERROR_CODE omci_ResetOntSetting(void)
{

    omci_ResetMacLearnLimit();
    omci_ResetAllMacFilter();

    return GOS_OK;
}


/*
*  Define Global APIs for MIB init and self-create
*/

GOS_ERROR_CODE OMCI_ResetMib(void)
{
    GOS_ERROR_CODE   ret;
    MIB_TABLE_ONTG_T ontg;
    PON_ONU_STATE    ponState = PON_ONU_STATE_UNKNOWN;
    mgmt_cfg_msg_t   mgmtInfo;
    UINT8            cus_onu_type = 0xFF;
    MIB_TABLE_VOIPCONFIGDATA_T   mibVcd;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Reseting MIB by OLT");

    pthread_mutex_lock(&gOmciPmOperMutex);

    // backup ONT PON state
    ontg.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)))
    {
        ponState = ontg.OntState;
    }

    feature_api(FEATURE_API_ME_00000040_GET_INFO, &cus_onu_type);

    /* clean mcast info */
    MCAST_WRAPPER(omci_config_init);

    /* clean IpHost and ACS info */
    memset(&mgmtInfo, 0, sizeof(mgmt_cfg_msg_t));
    mgmtInfo.op_id = OP_RESET_ALL;
    feature_api(FEATURE_API_L3SVC_MGMT_CFG_SET, &mgmtInfo, sizeof(mgmt_cfg_msg_t));

    /*clear MIB*/
    MIB_ClearPublic();

    /*reset MIB default values*/
    ret = omci_PonResetMib();
    GOS_ASSERT(ret == GOS_OK);

    /*clean ONU setting which set by OLT*/
    omci_ResetOntSetting();

    /* update ONT PON state*/
    ontg.EntityID = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T)))
    {
        ontg.OntState = ponState;
        MIB_Set(MIB_TABLE_ONTG_INDEX, &ontg, sizeof(MIB_TABLE_ONTG_T));
    }

    feature_api(FEATURE_API_ME_00000040_SET_INFO, &cus_onu_type);

    mibVcd.EntityId = 0;
    if (GOS_OK == MIB_Get(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVcd, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)) &&
        mibVcd.VOIPConfigurationMethodUsed == VCD_CFG_METHOD_USED_OMCI)
    {
            OMCI_PRINT("[anson]%s() %d, reset voice default if configuration method is set to VCD_CFG_METHOD_USED_OMCI", __FUNCTION__, __LINE__);
            VOICE_WRAPPER(omci_voice_config_reset);
    }
    pthread_mutex_unlock(&gOmciPmOperMutex);

    return GOS_OK;
}


GOS_ERROR_CODE OMCI_InitMib(void)
{
    /* initialize MIB table for MEs */
    MIB_Init(NULL, NULL);

    /* initialize MEs created by ONT in the MIB */
    omci_InitOntMe();

    /* register Callback function to MIB */
    if (GOS_OK != MIB_RegisterCallbackToAll(NULL, omci_AvcCallback))
    {
        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_Protocol_RspCtrl(void *ptr)
{
    CHAR boaMibVal[OMCI_BOA_MIB_BUFFER_LEN];
    CHAR cmd[OMCI_BOA_MIB_BUFFER_LEN];

    memset(boaMibVal, 0, OMCI_BOA_MIB_BUFFER_LEN);

    if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_FAKE_OK", boaMibVal))
    {
        if (!ptr)
        {
            gOmciOmitErrEnable = strtol(boaMibVal, NULL, 0);
        }
        else
        {
            memset(cmd, 0, OMCI_BOA_MIB_BUFFER_LEN);
#ifdef OMCI_X86
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "X86 not support flash set %s %u", "OMCI_FAKE_OK", gOmciOmitErrEnable);
#else
            snprintf(cmd, sizeof(cmd), "flash set %s %u", "OMCI_FAKE_OK", gOmciOmitErrEnable);

            system(cmd);
#endif
        }
    }
    return GOS_OK;
}


GOS_ERROR_CODE OMCI_WanQueue_Set(void)
{
    CHAR boaMibVal[OMCI_BOA_MIB_BUFFER_LEN];

    memset(boaMibVal, 0, OMCI_BOA_MIB_BUFFER_LEN);

    if (GOS_OK == omci_get_boa_mib_value_by_name("OMCI_WAN_QOS_QUEUE_NUM", boaMibVal))
    {
        gInfo.wanQueueNum = strtol(boaMibVal, NULL, 0);
    }
    else
    {
        gInfo.wanQueueNum = WAN_PONMAC_QUEUE_MAX;
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_PortRemap_Set(void)
{
    CHAR boaMibVal[OMCI_BOA_MIB_BUFFER_LEN];
    CHAR *delim = ",";
    CHAR *pch = NULL;
    UINT32  index = 0;

    memset(boaMibVal, 0, OMCI_BOA_MIB_BUFFER_LEN);

    if (GOS_OK == omci_get_boa_mib_value_by_name("PORT_REMAPPING", boaMibVal))
    {
        pch = strtok(boaMibVal, delim);
        while (pch != NULL)
        {
            gInfo.portRemMap[index] = strtoul(pch, NULL, 0);
            pch = strtok (NULL, delim);
            index++;
        }
    }

#if defined(FPGA_DEFINED)
    gInfo.portRemMap[0]=0;
    gInfo.portRemMap[1]=1;
    gInfo.portRemMap[2]=2;
    gInfo.portRemMap[3]=3;
#endif
    return GOS_OK;
}

