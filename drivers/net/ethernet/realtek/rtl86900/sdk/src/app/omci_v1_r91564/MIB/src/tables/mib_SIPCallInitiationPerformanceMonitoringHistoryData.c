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
 */


#include "app_basic.h"

MIB_TABLE_INFO_T gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo;
MIB_ATTR_INFO_T  gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow;
MIB_TABLE_OPER_T gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper;


static UINT8    aTcaAlmNumber[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];


GOS_ERROR_CODE
SIPCallInitiationPerformanceMonitoringHistoryDataDrvCfg(
    void* pOldRow,
    void* pNewRow,
    MIB_OPERA_TYPE operationType,
    MIB_ATTRS_SET attrSet,
    UINT32 pri)
{
    MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T *pMibSipCiHistoryPMD;
    MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T mibCurrentBin;
    MIB_TABLE_INDEX                                 tableIndex = MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX;
    MIB_TABLE_SIPAGENTCONFIGDATA_T                  mibSipAgentCfgData;
    GOS_ERROR_CODE                                  ret;
    UINT32                                          entrySize;
    UINT16                                          chid;

    entrySize = MIB_GetTableEntrySize(tableIndex);
    pMibSipCiHistoryPMD =
        (MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T *)pNewRow;

    mibSipAgentCfgData.EntityId = pMibSipCiHistoryPMD->EntityId;
    ret = MIB_Get(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX, &mibSipAgentCfgData, sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T));
    if (GOS_OK != ret)
{
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX), mibSipAgentCfgData.EntityId);

            return GOS_FAIL;
    }

    omci_get_channel_index_by_pots_uni_me_id(pMibSipCiHistoryPMD->EntityId, &chid);

    if (pMibSipCiHistoryPMD->EntityId > 0)
        chid = pMibSipCiHistoryPMD->EntityId - 1;
    else
        chid = 0;

    if ( chid > gInfo.devCapabilities.potsPortNum )
{
        OMCI_PRINT("%s(%d) channel number %u out of range\n" , __FUNCTION__ , __LINE__ , chid);
        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType)
    {
    case MIB_ADD:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPCallInitiationPerformanceMonitoringHistoryData --> ADD");
        // sync control block data from history bin to current bin
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibSipCiHistoryPMD, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), pMibSipCiHistoryPMD->EntityId);
        }

        ret = mib_alarm_table_add(tableIndex, pNewRow);
        break;

    case MIB_SET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPCallInitiationPerformanceMonitoringHistoryData --> SET");
        mibCurrentBin.EntityId = pMibSipCiHistoryPMD->EntityId;
        if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

            return GOS_FAIL;
        }

        // sync control block data from history bin to current bin
        mibCurrentBin.ThresholdData12Id = pMibSipCiHistoryPMD->ThresholdData12Id;
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
        }
        break;
    case MIB_GET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPCallInitiationPerformanceMonitoringHistoryData --> GET");
        break;
    case MIB_DEL:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPCallInitiationPerformanceMonitoringHistoryData --> DEL");
        ret = mib_alarm_table_del(tableIndex, pOldRow);
        break;
    default:
        return GOS_FAIL;
        break;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
sipCallInitiation_pmhd_pm_handler (
    MIB_TABLE_INDEX         tableIndex,
    omci_me_instance_t      instanceID,
    omci_pm_oper_type_t     operType,
    BOOL                    *pIsTcaRaised
)
{
    GOS_ERROR_CODE                                          ret;
    UINT32                                                  entrySize;
    MIB_TABLE_SIPAGENTCONFIGDATA_T                          mibSipAgentCfgData;
    MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T mibSipCiPmhd;
    MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T mibCurrentBin;
    omci_SIPCIPM_history_data_t sipCiCntrs;
    omci_SIPCIPM_history_data_t *pSipCiCntrs = (omci_SIPCIPM_history_data_t *)&sipCiCntrs;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibSipAgentCfgData.EntityId = instanceID;
    if (GOS_OK != MIB_Get(
                      MIB_TABLE_SIPAGENTCONFIGDATA_INDEX,
                      &mibSipAgentCfgData,
                      sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                 __FUNCTION__, MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX), mibSipAgentCfgData.EntityId);

        return GOS_FAIL;
    }

    mibSipCiPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibSipCiPmhd, entrySize)) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                 __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    mibCurrentBin.EntityId = instanceID;
    if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize)) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                 __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (OMCI_PM_OPER_RESET == operType) {
        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibSipCiPmhd.ThresholdData12Id;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        return GOS_OK;
    }
    if (OMCI_PM_OPER_GET_CURRENT_DATA == operType) {
        omci_pm_update_sipCi (OMCI_PM_OPER_GET_CURRENT_DATA);
    }

    if (OMCI_PM_OPER_UPDATE == operType ||
        OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
        OMCI_PM_OPER_GET_CURRENT_DATA == operType) {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_sipCi (instanceID, pSipCiCntrs)) {
            m_omci_pm_update_accum_attr(pSipCiCntrs->failedConnectCounter, mibCurrentBin.FailedToConnectCounter);
            m_omci_pm_update_accum_attr(pSipCiCntrs->failedValidateCounter, mibCurrentBin.FailedToValidateCounter);
            m_omci_pm_update_accum_attr(pSipCiCntrs->timeoutCounter, mibCurrentBin.TimeoutCounter);
            m_omci_pm_update_accum_attr(pSipCiCntrs->failureReceivedCounter, mibCurrentBin.FailureReceivedCounter);
            m_omci_pm_update_accum_attr(pSipCiCntrs->failedAuthenticateCounter, mibCurrentBin.FailedToAuthenticateCounter);
        }

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        ret = omci_pm_is_threshold_crossed(tableIndex, instanceID,
                                           &mibCurrentBin, aTcaAlmNumber, aTcaAttrMap, NULL, pIsTcaRaised);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Check threshold error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (OMCI_PM_OPER_SWAP == operType || OMCI_PM_OPER_UPDATE_AND_SWAP == operType) {
        // swap PM values
        mibCurrentBin.IntervalEndTime++;
        mibSipCiPmhd.IntervalEndTime = mibCurrentBin.IntervalEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibSipCiPmhd.ThresholdData12Id;
        mibCurrentBin.IntervalEndTime = mibSipCiPmhd.IntervalEndTime;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.Name = "SIPCallInitiationPerformanceMonitoringHistoryData";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.ShortName = "SIPCPMHD";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.Desc = "SIP call initiation performance monitoring history data";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_SIP_CALL_INITIATION_PMHD);
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.pAttributes = &(gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[0]);

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.attrNum = MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.entrySize = sizeof(MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_T);
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo.pDefaultRow = &gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FailedToConnectCounter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FailedToValidateCounter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TimeoutCounter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FailureReceivedCounter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FailedToAuthenticateCounter";

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Failed to connect counter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Failed to validate counter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Timeout counter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Failure received counter";
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Failed to authenticate counter";

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOCONNECTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOVALIDATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_TIMEOUTCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILURERECEIVEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_FAILEDTOAUTHENTICATECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.EntityId = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.IntervalEndTime = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.ThresholdData12Id = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.FailedToConnectCounter = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.FailedToValidateCounter = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.TimeoutCounter = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.FailureReceivedCounter = 0;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataDefRow.FailedToAuthenticateCounter = 0;

    memset(&gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperDrvCfg = SIPCallInitiationPerformanceMonitoringHistoryDataDrvCfg;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperConnCheck = NULL;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperConnCfg = NULL;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperAvlTreeAdd = NULL;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperAlarmHandler = NULL;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperTestHandler = NULL;
    gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper.meOperPmHandler   = sipCallInitiation_pmhd_pm_handler;

    MIB_TABLE_SIPCALLINITIATIONPERFORMANCEMONITORINGHISTORYDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibSIPCallInitiationPerformanceMonitoringHistoryDataTableInfo, &gMibSIPCallInitiationPerformanceMonitoringHistoryDataOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

