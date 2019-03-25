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
 * Purpose : Definition of ME handler: RTP PMHD (144)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: RTP PMHD (144)
 */


#include "app_basic.h"

MIB_TABLE_INFO_T gMibRTPPerformanceMonitoringHistoryDataTableInfo;
MIB_ATTR_INFO_T  gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T gMibRTPPerformanceMonitoringHistoryDataDefRow;
MIB_TABLE_OPER_T gMibRTPPerformanceMonitoringHistoryDataOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];

GOS_ERROR_CODE RTPPerformanceMonitoringHistoryDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T *pMibRtpHistoryPMD;
    MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T mibCurrentBin;
    MIB_TABLE_INDEX                                 tableIndex = MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INDEX;
    MIB_TABLE_POTSUNI_T                             mibPots;
    GOS_ERROR_CODE                                  ret;
    UINT32                                          entrySize;
    UINT16                                          chid;

    entrySize = MIB_GetTableEntrySize(tableIndex);
    pMibRtpHistoryPMD = (MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T *)pNewRow;

    mibPots.EntityId = pMibRtpHistoryPMD->EntityId;
    ret = MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_POTSUNI_INDEX), mibPots.EntityId);

            return GOS_FAIL;
    }

    omci_get_channel_index_by_pots_uni_me_id(pMibRtpHistoryPMD->EntityId, &chid);
    if ( chid > gInfo.devCapabilities.potsPortNum )
    {
        OMCI_PRINT("%s(%d) channel number %u out of range\n" , __FUNCTION__ , __LINE__ , chid);
        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType)
    {
    case MIB_ADD:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"RTPPerformanceMonitoringHistoryData --> ADD");
        // sync control block data from history bin to current bin
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibRtpHistoryPMD, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), pMibRtpHistoryPMD->EntityId);
        }

        ret = mib_alarm_table_add(tableIndex, pNewRow);
        break;

    case MIB_SET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"RTPPerformanceMonitoringHistoryData --> SET");
        mibCurrentBin.EntityId = pMibRtpHistoryPMD->EntityId;
        if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

            return GOS_FAIL;
        }

        // sync control block data from history bin to current bin
        mibCurrentBin.ThresholdData12Id = pMibRtpHistoryPMD->ThresholdData12Id;
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
        }
        break;

    case MIB_DEL:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"RTPPerformanceMonitoringHistoryData --> DEL");
        ret = mib_alarm_table_del(tableIndex, pOldRow);
        break;
    default:
        break;
    }

    return ret;
}

GOS_ERROR_CODE rtp_pmhd_pm_handler(MIB_TABLE_INDEX          tableIndex,
                                    omci_me_instance_t      instanceID,
                                    omci_pm_oper_type_t     operType,
                                    BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                                          ret;
    UINT32                                                  entrySize;
    MIB_TABLE_POTSUNI_T                                     mibPots;
    MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T         mibRtpPmhd;
    MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T         mibCurrentBin;
    omci_RTPPM_history_data_t                                           rtpCntrs;
    omci_RTPPM_history_data_t                                           *pRtpCntrs = (omci_RTPPM_history_data_t*)&rtpCntrs;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibPots.EntityId = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_POTSUNI_INDEX), mibPots.EntityId);

        return GOS_FAIL;
    }

    mibRtpPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibRtpPmhd, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    mibCurrentBin.EntityId = instanceID;
    if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (OMCI_PM_OPER_RESET == operType)
    {
        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibRtpPmhd.ThresholdData12Id;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        return GOS_OK;
    }
    if(OMCI_PM_OPER_GET_CURRENT_DATA == operType)
    {
        omci_pm_update_rtp(OMCI_PM_OPER_GET_CURRENT_DATA);
    }

    if (OMCI_PM_OPER_UPDATE == operType ||
        OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
        OMCI_PM_OPER_GET_CURRENT_DATA == operType)
    {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_rtp(instanceID,pRtpCntrs))
        {
            m_omci_pm_update_accum_attr(pRtpCntrs->RTPErrors, mibCurrentBin.RTPErrors);
            m_omci_pm_update_accum_attr(pRtpCntrs->packetLoss, mibCurrentBin.PacketLoss);
            m_omci_pm_update_accum_attr(pRtpCntrs->maximumJitter, mibCurrentBin.MaximumJitter);
            m_omci_pm_update_accum_attr(pRtpCntrs->maximumTimeRTCP, mibCurrentBin.MaximumTimeBetweenRTCPPackets);
            m_omci_pm_update_accum_attr(pRtpCntrs->bufferUnderflows, mibCurrentBin.BufferUnderflows);
            m_omci_pm_update_accum_attr(pRtpCntrs->bufferOverflows, mibCurrentBin.BufferOverflows);
        }

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        ret = omci_pm_is_threshold_crossed(tableIndex, instanceID,
                &mibCurrentBin, aTcaAlmNumber, aTcaAttrMap, NULL, pIsTcaRaised);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Check threshold error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (OMCI_PM_OPER_SWAP == operType || OMCI_PM_OPER_UPDATE_AND_SWAP == operType)
    {
        // swap PM values
        mibCurrentBin.IntervalEndTime++;
        mibRtpPmhd.IntervalEndTime = mibCurrentBin.IntervalEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibRtpPmhd.ThresholdData12Id;
        mibCurrentBin.IntervalEndTime = mibRtpPmhd.IntervalEndTime;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.Name = "RTPPerformanceMonitoringHistoryData";
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.ShortName = "RTPPMHD";
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.Desc = "RTP performance monitoring history data";
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_RTP_PMHD);
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.pAttributes = &(gMibRTPPerformanceMonitoringHistoryDataAttrInfo[0]);

    gMibRTPPerformanceMonitoringHistoryDataTableInfo.attrNum = MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM;
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.entrySize = sizeof(MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_T);
    gMibRTPPerformanceMonitoringHistoryDataTableInfo.pDefaultRow = &gMibRTPPerformanceMonitoringHistoryDataDefRow;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RTPErrors";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PacketLoss";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaximumJitter";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaximumTimeBetweenRTCPPackets";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BufferUnderflows";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BufferOverflows";

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "RTP errors";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet loss";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Maximum jitter";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Maximum time between RTCP packets";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Buffer underflows";
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Buffer overflows";

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibRTPPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibRTPPerformanceMonitoringHistoryDataDefRow.EntityId = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.IntervalEndTime = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.ThresholdData12Id = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.RTPErrors = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.PacketLoss = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.MaximumJitter = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.MaximumTimeBetweenRTCPPackets = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.BufferUnderflows = 0;
    gMibRTPPerformanceMonitoringHistoryDataDefRow.BufferOverflows = 0;

    memset(&gMibRTPPerformanceMonitoringHistoryDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperDrvCfg = RTPPerformanceMonitoringHistoryDataDrvCfg;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperConnCheck = NULL;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperConnCfg = NULL;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperAvlTreeAdd = NULL;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperAlarmHandler = NULL;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperTestHandler = NULL;
    gMibRTPPerformanceMonitoringHistoryDataOper.meOperPmHandler = rtp_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    aTcaAlmNumber[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX] = 5;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_RTPERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_PACKETLOSS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMJITTER_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_MAXIMUMTIMEBETWEENRTCPPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFERUNDERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX] = 6;
    aTcaAttrMap[MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_BUFFEROVERFLOWS_INDEX - MIB_TABLE_FIRST_INDEX] = 7;

    MIB_TABLE_RTPPERFORMANCEMONITORINGHISTORYDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibRTPPerformanceMonitoringHistoryDataTableInfo, &gMibRTPPerformanceMonitoringHistoryDataOper);

    return GOS_OK;
}

