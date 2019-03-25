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
 * Purpose : Definition of ME handler: Ethernet PMHD (24)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Ethernet PMHD (24)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/stat.h"
#endif

MIB_TABLE_INFO_T gMibEthPmHistoryDataTableInfo;
MIB_ATTR_INFO_T  gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ATTR_NUM];
MIB_TABLE_ETHPMHISTORYDATA_T gMibEthPmHistoryDataDefRow;
MIB_TABLE_OPER_T gMibEthPmHistoryDataOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_ATTR_NUM];


GOS_ERROR_CODE EthPmHistoryData_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMHISTORYDATA_T    *pMibEthPMHD;
    MIB_TABLE_ETHPMHISTORYDATA_T    mibCurrentBin;

    pMibEthPMHD = (MIB_TABLE_ETHPMHISTORYDATA_T *)pNewRow;
    mibPptpEthUNI.EntityID = pMibEthPMHD->EntityId;

    // check if corresponding PPTP exists
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, MIB_GetTableEntrySize(MIB_TABLE_ETHUNI_INDEX));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_ETHPMHISTORYDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibEthPMHD, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthPMHD->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibEthPMHD->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibEthPMHD->ThresholdID;
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
            }
            break;

        default:
            break;
    }

	return ret;
}

GOS_ERROR_CODE ethernet_pmhd_pm_handler(MIB_TABLE_INDEX         tableIndex,
                                        omci_me_instance_t      instanceID,
                                        omci_pm_oper_type_t     operType,
                                        BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMHISTORYDATA_T    mibEthPMHD;
    MIB_TABLE_ETHPMHISTORYDATA_T    mibCurrentBin;
    omci_port_stat_t                portCntrs;
    omci_port_stat_t                *pPortCntrs = (omci_port_stat_t*)&portCntrs;
    unsigned long long              tempCntr;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibPptpEthUNI.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), instanceID);

        return GOS_FAIL;
    }

    mibEthPMHD.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibEthPMHD, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
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
        mibCurrentBin.ThresholdID = mibEthPMHD.ThresholdID;

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
		omci_pm_update_pptp_eth_uni(OMCI_PM_OPER_GET_CURRENT_DATA);
	}

    if (OMCI_PM_OPER_UPDATE == operType ||
		OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
		OMCI_PM_OPER_GET_CURRENT_DATA == operType)
    {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_pptp_eth_uni(instanceID,pPortCntrs))
        {
            tempCntr = pPortCntrs->etherStatsFragments +
                        pPortCntrs->etherStatsCRCAlignErrors +
                        pPortCntrs->etherStatsJabbers;
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.FCSErrors);
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsExcessiveCollisions, mibCurrentBin.ExcCollision);
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsLateCollisions, mibCurrentBin.LateCollision);
            //mibCurrentBin.FrameTooLongs;
            //mibCurrentBin.RxBufOverflow;
            //mibCurrentBin.TxBufOverflow;
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsSingleCollisionFrames, mibCurrentBin.SingleCollision);
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsMultipleCollisionFrames, mibCurrentBin.MultiCollision);
            //mibCurrentBin.SQECounter;
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsDeferredTransmissions, mibCurrentBin.TxDeferred);
            //mibCurrentBin.TxInterMacErr;
            //mibCurrentBin.CarrierSenseErr;
            //mibCurrentBin.AlignmentErr;
            //mibCurrentBin.RxInterMacErr;
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
        mibCurrentBin.IntEndTime++;
        mibEthPMHD.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibEthPMHD.ThresholdID;
        mibCurrentBin.IntEndTime = mibEthPMHD.IntEndTime;

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
    gMibEthPmHistoryDataTableInfo.Name = "EthPmHistoryData";
    gMibEthPmHistoryDataTableInfo.ShortName = "ETHPMHD";
    gMibEthPmHistoryDataTableInfo.Desc = "Ethernet performance monitoring history data";
    gMibEthPmHistoryDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ETHERNET_PMHD);
    gMibEthPmHistoryDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibEthPmHistoryDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibEthPmHistoryDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibEthPmHistoryDataTableInfo.pAttributes = &(gMibEthPmHistoryDataAttrInfo[0]);

	gMibEthPmHistoryDataTableInfo.attrNum = MIB_TABLE_ETHPMHISTORYDATA_ATTR_NUM;
	gMibEthPmHistoryDataTableInfo.entrySize = sizeof(MIB_TABLE_ETHPMHISTORYDATA_T);
	gMibEthPmHistoryDataTableInfo.pDefaultRow = &gMibEthPmHistoryDataDefRow;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntEndTime";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdID";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FCSErrors";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ExcCollision";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LateCollision";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FrameTooLongs";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxBufOverflow";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxBufOverflow";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SingleCollision";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MultiCollision";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SQECounter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxDeferred";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxInterMacErr";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CarrierSenseErr";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AlignmentErr";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxInterMacErr";

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval End Time";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold Data ID";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "FCS Errors";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Excessive collision counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Late collision counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "FrameTooLongs";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "BufferOverflows on receive";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "BufferOverflows on transmit";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Single collision frame counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Multiple collisions frame counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "SQE Counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Deferred transmission counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "InternalMACTransmit ErrorCounter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "CarrierSenseError counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "AlignmentError counter";
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "InternalMACReceive ErrorCounter";

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmHistoryDataAttrInfo[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibEthPmHistoryDataDefRow.EntityId), 0x00, sizeof(gMibEthPmHistoryDataDefRow.EntityId));
    memset(&(gMibEthPmHistoryDataDefRow.IntEndTime), 0x00, sizeof(gMibEthPmHistoryDataDefRow.IntEndTime));
    memset(&(gMibEthPmHistoryDataDefRow.ThresholdID), 0x00, sizeof(gMibEthPmHistoryDataDefRow.ThresholdID));
    memset(&(gMibEthPmHistoryDataDefRow.FCSErrors), 0x00, sizeof(gMibEthPmHistoryDataDefRow.FCSErrors));
    memset(&(gMibEthPmHistoryDataDefRow.ExcCollision), 0x00, sizeof(gMibEthPmHistoryDataDefRow.ExcCollision));
    memset(&(gMibEthPmHistoryDataDefRow.LateCollision), 0x00, sizeof(gMibEthPmHistoryDataDefRow.LateCollision));
    memset(&(gMibEthPmHistoryDataDefRow.FrameTooLongs), 0x00, sizeof(gMibEthPmHistoryDataDefRow.FrameTooLongs));
    memset(&(gMibEthPmHistoryDataDefRow.RxBufOverflow), 0x00, sizeof(gMibEthPmHistoryDataDefRow.RxBufOverflow));
    memset(&(gMibEthPmHistoryDataDefRow.TxBufOverflow), 0x00, sizeof(gMibEthPmHistoryDataDefRow.TxBufOverflow));
    memset(&(gMibEthPmHistoryDataDefRow.SingleCollision), 0x00, sizeof(gMibEthPmHistoryDataDefRow.SingleCollision));
    memset(&(gMibEthPmHistoryDataDefRow.MultiCollision), 0x00, sizeof(gMibEthPmHistoryDataDefRow.MultiCollision));
    memset(&(gMibEthPmHistoryDataDefRow.SQECounter), 0x00, sizeof(gMibEthPmHistoryDataDefRow.SQECounter));
    memset(&(gMibEthPmHistoryDataDefRow.TxDeferred), 0x00, sizeof(gMibEthPmHistoryDataDefRow.TxDeferred));
    memset(&(gMibEthPmHistoryDataDefRow.TxInterMacErr), 0x00, sizeof(gMibEthPmHistoryDataDefRow.TxInterMacErr));
    memset(&(gMibEthPmHistoryDataDefRow.CarrierSenseErr), 0x00, sizeof(gMibEthPmHistoryDataDefRow.CarrierSenseErr));
    memset(&(gMibEthPmHistoryDataDefRow.AlignmentErr), 0x00, sizeof(gMibEthPmHistoryDataDefRow.AlignmentErr));
    memset(&(gMibEthPmHistoryDataDefRow.RxInterMacErr), 0x00, sizeof(gMibEthPmHistoryDataDefRow.RxInterMacErr));

    memset(&gMibEthPmHistoryDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthPmHistoryDataOper.meOperDrvCfg = EthPmHistoryData_CfgMe;
    gMibEthPmHistoryDataOper.meOperConnCheck = NULL;
	gMibEthPmHistoryDataOper.meOperConnCfg = NULL;
	gMibEthPmHistoryDataOper.meOperAvlTreeAdd = NULL;
    gMibEthPmHistoryDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibEthPmHistoryDataOper.meOperPmHandler = ethernet_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX] = 5;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 6;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 7;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX] = 8;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX] = 9;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX] = 10;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX] = 11;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX] = 12;
    aTcaAlmNumber[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX] = 13;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_FCSERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_EXCCOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_LATECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_FRAMETOOLONGS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_RXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX] = 5;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_TXBUFOVERFLOW_INDEX - MIB_TABLE_FIRST_INDEX] = 6;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_SINGLECOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 7;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_MULTICOLLISION_INDEX - MIB_TABLE_FIRST_INDEX] = 8;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_SQECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX] = 9;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_TXDEFERRED_INDEX - MIB_TABLE_FIRST_INDEX] = 10;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_TXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX] = 11;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_CARRIERSENSEERR_INDEX - MIB_TABLE_FIRST_INDEX] = 12;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_ALIGNMENTERR_INDEX - MIB_TABLE_FIRST_INDEX] = 13;
    aTcaAttrMap[MIB_TABLE_ETHPMHISTORYDATA_RXINTERMACERR_INDEX - MIB_TABLE_FIRST_INDEX] = 14;

    MIB_TABLE_ETHPMHISTORYDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibEthPmHistoryDataTableInfo, &gMibEthPmHistoryDataOper);

    return GOS_OK;
}
