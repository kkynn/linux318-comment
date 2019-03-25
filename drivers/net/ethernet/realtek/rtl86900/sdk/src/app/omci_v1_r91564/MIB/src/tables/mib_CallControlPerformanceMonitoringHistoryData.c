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
 * Purpose : Definition of ME handler: Call control PMHD (140)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Call control PMHD (140)
 */


#include "app_basic.h"

MIB_TABLE_INFO_T gMibCallControlPerformanceMonitoringHistoryDataTableInfo;
MIB_ATTR_INFO_T  gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T gMibCallControlPerformanceMonitoringHistoryDataDefRow;
MIB_TABLE_OPER_T gMibCallControlPerformanceMonitoringHistoryDataOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM];

GOS_ERROR_CODE CallControlPerformanceMonitoringHistoryDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_INDEX             tableIndex = MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INDEX;
    MIB_TABLE_POTSUNI_T                                     mibPots; 	
    MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T  *pMibCallCtrlPmhd;
    MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T  mibCurrentBin;

    pMibCallCtrlPmhd = (MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T *)pNewRow;

    mibPots.EntityId = pMibCallCtrlPmhd->EntityId;
    ret = MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T));
	
	entrySize = MIB_GetTableEntrySize(tableIndex);	
	
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_POTSUNI_INDEX), mibPots.EntityId);

        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType)
	{
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"CallControlPerformanceMonitoringHistoryData --> ADD");

// sync control block data from history bin to current bin
        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibCallCtrlPmhd, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), pMibCallCtrlPmhd->EntityId);
        }

        ret = mib_alarm_table_add(tableIndex, pNewRow);
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"CallControlPerformanceMonitoringHistoryData --> SET");
    	mibCurrentBin.EntityId = pMibCallCtrlPmhd->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdData12Id = pMibCallCtrlPmhd->ThresholdData12Id;
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
            }
    	break;
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"CallControlPerformanceMonitoringHistoryData --> GET");
    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"CallControlPerformanceMonitoringHistoryData --> DEL");
    	ret = mib_alarm_table_del(tableIndex, pOldRow);
    	break;
    default:
        return GOS_FAIL;
    	break;
    }

    return ret;
}

GOS_ERROR_CODE
call_ctrl_pmhd_pm_handler(
    MIB_TABLE_INDEX        tableIndex,
    omci_me_instance_t      instanceID,
    omci_pm_oper_type_t     operType,
    BOOL* pIsTcaRaised)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_POTSUNI_T                                     mibPots; 
    MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T  mibCallCtrlPmhd;
    MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T  mibCurrentBin;
    omci_CCPM_history_data_t  ccpmCntrs;
    omci_CCPM_history_data_t* pCcpmCntrs = (omci_CCPM_history_data_t *)&ccpmCntrs;

    if (!pIsTcaRaised)
    {
        return GOS_ERR_PARAM;
    }

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibPots.EntityId = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_POTSUNI_INDEX, &mibPots, sizeof(MIB_TABLE_POTSUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_POTSUNI_INDEX), mibPots.EntityId);

        return GOS_FAIL;
    }

    mibCallCtrlPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibCallCtrlPmhd, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    mibCurrentBin.EntityId = instanceID;
    if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Current bin not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (OMCI_PM_OPER_RESET == operType)
    {
        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibCallCtrlPmhd.ThresholdData12Id;

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
        OMCI_LOG(
            OMCI_LOG_LEVEL_INFO,
            "Get VOIP's counter for %s (InstanceID 0x%x) of PM from VOIP module\n",
            MIB_GetTableName(tableIndex),
            instanceID); 
        omci_pm_update_ccpm (OMCI_PM_OPER_GET_CURRENT_DATA);  
    }

    if (OMCI_PM_OPER_UPDATE == operType || 
        OMCI_PM_OPER_UPDATE_AND_SWAP == operType || 
        OMCI_PM_OPER_GET_CURRENT_DATA == operType)
    {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_ccpm (instanceID,pCcpmCntrs))
        {
            OMCI_LOG(
                OMCI_LOG_LEVEL_INFO, 
                "Update %s (InstanceID 0x%x) of PM\n",
                MIB_GetTableName(tableIndex), 
                instanceID);
            m_omci_pm_update_accum_attr(pCcpmCntrs->setupFailures, mibCurrentBin.CallSetupFailures);
            m_omci_pm_update_accum_attr(pCcpmCntrs->setupTimer, mibCurrentBin.CallSetupTimer);
            m_omci_pm_update_accum_attr(pCcpmCntrs->terminateFailures, mibCurrentBin.CallTerminateFailures);
            m_omci_pm_update_accum_attr(pCcpmCntrs->portReleases, mibCurrentBin.AnalogPortReleases);
            m_omci_pm_update_accum_attr(pCcpmCntrs->portOffhookTimer, mibCurrentBin.AnalogPortOffhookTimer);
        } else {
            OMCI_LOG(
                OMCI_LOG_LEVEL_ERR,
                "Failed to get currentData for %s (InstanceID 0x%x) of PM\n",
                MIB_GetTableName(tableIndex),
                instanceID);
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
        mibCallCtrlPmhd.IntervalEndTime = mibCurrentBin.IntervalEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdData12Id = mibCallCtrlPmhd.ThresholdData12Id;
        mibCurrentBin.IntervalEndTime = mibCallCtrlPmhd.IntervalEndTime;

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
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.Name = "CallControlPerformanceMonitoringHistoryData";
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.ShortName = "CCPMHD";
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.Desc = "Call control performance monitoring history data";
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_CALL_CTRL_PMHD);
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.pAttributes = &(gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[0]);

    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.attrNum = MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ATTR_NUM;
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.entrySize = sizeof(MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_T);
    gMibCallControlPerformanceMonitoringHistoryDataTableInfo.pDefaultRow = &gMibCallControlPerformanceMonitoringHistoryDataDefRow;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallSetupFailures";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallSetupTimer";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallTerminateFailures";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AnalogPortReleases";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AnalogPortOffhookTimer";

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call setup failures";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call setup timer";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call terminate failures";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Analog port releases";
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Analog port off-hook timer";

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCallControlPerformanceMonitoringHistoryDataAttrInfo[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibCallControlPerformanceMonitoringHistoryDataDefRow.EntityId = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.IntervalEndTime = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.ThresholdData12Id = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.CallSetupFailures = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.CallSetupTimer = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.CallTerminateFailures = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.AnalogPortReleases = 0;
    gMibCallControlPerformanceMonitoringHistoryDataDefRow.AnalogPortOffhookTimer = 0;

    memset(&gMibCallControlPerformanceMonitoringHistoryDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperDrvCfg = CallControlPerformanceMonitoringHistoryDataDrvCfg;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperConnCheck = NULL;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperConnCfg = NULL;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperAvlTreeAdd = NULL;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperAlarmHandler = NULL;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperTestHandler = NULL;
    gMibCallControlPerformanceMonitoringHistoryDataOper.meOperPmHandler = call_ctrl_pmhd_pm_handler;

   // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAlmNumber[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPFAILURES_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLSETUPTIMER_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_CALLTERMINATEFAILURES_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTRELEASES_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    aTcaAttrMap[MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_ANALOGPORTOFFHOOKTIMER_INDEX - MIB_TABLE_FIRST_INDEX] = 5;

    MIB_TABLE_CALLCONTROLPERFORMANCEMONITORINGHISTORYDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibCallControlPerformanceMonitoringHistoryDataTableInfo, &gMibCallControlPerformanceMonitoringHistoryDataOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

