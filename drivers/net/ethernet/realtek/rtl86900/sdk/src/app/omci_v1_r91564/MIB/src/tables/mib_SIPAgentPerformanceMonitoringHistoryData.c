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

MIB_TABLE_INFO_T gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo;
MIB_ATTR_INFO_T  gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T gMibSIPAgentPerformanceMonitoringHistoryDataDefRow;
MIB_TABLE_OPER_T gMibSIPAgentPerformanceMonitoringHistoryDataOper;


static UINT8    aTcaAlmNumber[MIB_TABLE_SIPAGENTCONFIGDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_SIPAGENTCONFIGDATA_ATTR_NUM];


GOS_ERROR_CODE SIPAgentPerformanceMonitoringHistoryDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T *pMibSipAgentHistoryPMD;
    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T mibCurrentBin;
    MIB_TABLE_INDEX                                 tableIndex =
    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INDEX;
    MIB_TABLE_SIPAGENTCONFIGDATA_T                  mibSipAgentCfgData;
    GOS_ERROR_CODE                                  ret;
    UINT32                                          entrySize;
    UINT16                                          chid;

    entrySize = MIB_GetTableEntrySize(tableIndex);
    pMibSipAgentHistoryPMD =
        (MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T *)pNewRow;

    mibSipAgentCfgData.EntityId = pMibSipAgentHistoryPMD->EntityId;
    ret = MIB_Get(
              MIB_TABLE_SIPAGENTCONFIGDATA_INDEX,
              &mibSipAgentCfgData,
              sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX), mibSipAgentCfgData.EntityId);

            return GOS_FAIL;
    }

    if (pMibSipAgentHistoryPMD->EntityId > 0)
        chid = pMibSipAgentHistoryPMD->EntityId - 1;
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
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPAgentPerformanceMonitoringHistoryData --> ADD\n");
        // sync control block data from history bin to current bin
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibSipAgentHistoryPMD, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), pMibSipAgentHistoryPMD->EntityId);
        }

        ret = mib_alarm_table_add(tableIndex, pNewRow);
        break;

    case MIB_SET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPAgentPerformanceMonitoringHistoryData --> SET");
        mibCurrentBin.EntityId = pMibSipAgentHistoryPMD->EntityId;
        if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

            return GOS_FAIL;
        }

        // sync control block data from history bin to current bin
        mibCurrentBin.ThresholdData12Id = pMibSipAgentHistoryPMD->ThresholdData12Id;
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
        }
        break;

    case MIB_GET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPAgentPerformanceMonitoringHistoryData --> GET");
        break;
    case MIB_DEL:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPAgentPerformanceMonitoringHistoryData --> DEL");
        ret = mib_alarm_table_del(tableIndex, pOldRow);
        break;
    default:
        return GOS_FAIL;
        break;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
sipAgent_pmhd_pm_handler(
    MIB_TABLE_INDEX         tableIndex,
    omci_me_instance_t      instanceID,
    omci_pm_oper_type_t     operType,
    BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                                          ret;
    UINT32                                                  entrySize;
    MIB_TABLE_SIPAGENTCONFIGDATA_T                          mibSipAgentCfgData;
    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T    mibSipAgentPmhd;
    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T    mibCurrentBin;
    omci_SIPAGPM_history_data_t                             sipAgentCntrs;
    omci_SIPAGPM_history_data_t*                            pSipAgentCntrs = (omci_SIPAGPM_history_data_t *)&sipAgentCntrs;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibSipAgentCfgData.EntityId = instanceID;
    if (GOS_OK != MIB_Get( MIB_TABLE_SIPAGENTCONFIGDATA_INDEX, &mibSipAgentCfgData, sizeof(MIB_TABLE_SIPAGENTCONFIGDATA_T))) {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
                 __FUNCTION__, MIB_GetTableName(MIB_TABLE_SIPAGENTCONFIGDATA_INDEX), mibSipAgentCfgData.EntityId);

        return GOS_FAIL;
    }

    mibSipAgentPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibSipAgentPmhd, entrySize)) {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
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
        mibCurrentBin.ThresholdData12Id = mibSipAgentPmhd.ThresholdData12Id;

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
        OMCI_LOG(
            OMCI_LOG_LEVEL_INFO,
            "Get VOIP's counter for %s (InstanceID 0x%x) of PM from VOIP module\n",
            MIB_GetTableName(tableIndex),
            instanceID);
        omci_pm_update_sipAgent (OMCI_PM_OPER_GET_CURRENT_DATA);
    }

    if (OMCI_PM_OPER_UPDATE == operType ||
        OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
        OMCI_PM_OPER_GET_CURRENT_DATA == operType) {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_sipAgent (instanceID, pSipAgentCntrs)) {
            OMCI_LOG(
                OMCI_LOG_LEVEL_INFO,
                "Update %s (InstanceID 0x%x) of PM\n",
                MIB_GetTableName(tableIndex),
                instanceID);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxInviteReqs, mibCurrentBin.RxInviteReqs);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxInviteRetrans, mibCurrentBin.RxInviteRetrans);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxNoninviteReqs, mibCurrentBin.RxNoninviteReqs);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxNoninviteRetrans, mibCurrentBin.RxNoninviteRetrans);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxResponse, mibCurrentBin.RxResponse);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->rxResponseRetransmissions, mibCurrentBin.RxResponseRetransmissions);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txInviteReqs, mibCurrentBin.TxInviteReqs);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txInviteRetrans, mibCurrentBin.TxInviteRetrans);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txNoninviteReqs, mibCurrentBin.TxNonInviteReqs);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txNoninviteRetrans, mibCurrentBin.TxNonInviteRetrans);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txResponse, mibCurrentBin.TxResponse);
            m_omci_pm_update_accum_attr(pSipAgentCntrs->txResponseRetransmissions, mibCurrentBin.TxResponseRetransmissions);
        } else {
            OMCI_LOG(
                OMCI_LOG_LEVEL_ERR,
                "Failed to get currentData for %s (InstanceID 0x%x) of PM\n",
                MIB_GetTableName(tableIndex),
                instanceID);
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
        mibSipAgentPmhd.IntervalEndTime = mibCurrentBin.IntervalEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibSipAgentPmhd.ThresholdData12Id;
        mibCurrentBin.IntervalEndTime = mibSipAgentPmhd.IntervalEndTime;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize)) {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                     __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.Name = "SIPAgentPerformanceMonitoringHistoryData";
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.ShortName = "SIPAPMHD";
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.Desc = "SIP agent performance monitoring history data";
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_SIP_AGENT_PMHD);
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.pAttributes = &(gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[0]);

    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.attrNum = MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM;
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.entrySize = sizeof(MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_T);
    gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo.pDefaultRow = &gMibSIPAgentPerformanceMonitoringHistoryDataDefRow;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Transactions";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxInviteReqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxInviteRetrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxNoninviteReqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxNoninviteRetrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxResponse";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxResponseRetransmissions";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxInviteReqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxInviteRetrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxNonInviteReqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxNonInviteRetrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxResponse";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxResponseRetransmissions";

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transactions";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx invite reqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx invite retrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx noninvite reqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx noninvite retrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx response";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx response retransmissions";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx invite reqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx invite retrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx noninvite reqs";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx non invite retrans";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx response";
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx response retransmissions";

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TRANSACTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_RXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITEREQS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXNONINVITERETRANS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibSIPAgentPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_TXRESPONSERETRANSMISSIONS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.EntityId = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.IntervalEndTime = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.ThresholdData12Id = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.Transactions = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxInviteReqs = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxInviteRetrans = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxNoninviteReqs = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxNoninviteRetrans = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxResponse = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.RxResponseRetransmissions = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxInviteReqs = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxInviteRetrans = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxNonInviteReqs = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxNonInviteRetrans = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxResponse = 0;
    gMibSIPAgentPerformanceMonitoringHistoryDataDefRow.TxResponseRetransmissions = 0;

    memset(&gMibSIPAgentPerformanceMonitoringHistoryDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperDrvCfg = SIPAgentPerformanceMonitoringHistoryDataDrvCfg;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperConnCheck = NULL;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperConnCfg = NULL;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperAvlTreeAdd = NULL;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperAlarmHandler = NULL;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperTestHandler = NULL;
    gMibSIPAgentPerformanceMonitoringHistoryDataOper.meOperPmHandler   = sipAgent_pmhd_pm_handler;

    MIB_TABLE_SIPAGENTPERFORMANCEMONITORINGHISTORYDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibSIPAgentPerformanceMonitoringHistoryDataTableInfo, &gMibSIPAgentPerformanceMonitoringHistoryDataOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

