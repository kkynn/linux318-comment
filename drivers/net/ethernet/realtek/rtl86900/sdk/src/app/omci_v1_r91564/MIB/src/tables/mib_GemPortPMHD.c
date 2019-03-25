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
 * Purpose : Definition of ME handler: GEM port PMHD (267)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: GEM port PMHD (267)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/gpon.h"
#endif

MIB_TABLE_INFO_T gMibGemPortPmhdTableInfo;
MIB_ATTR_INFO_T  gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ATTR_NUM];
MIB_TABLE_GEM_PORT_PMHD_T gMibGemPortPmhdDefRow;
MIB_TABLE_OPER_T gMibGemPortPmhdOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_GEM_PORT_PMHD_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_GEM_PORT_PMHD_ATTR_NUM];


GOS_ERROR_CODE gem_port_pmhd_drv_cfg_handler(void           *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_INDEX             tableIndex;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    MIB_TABLE_GEM_PORT_PMHD_T   *pMibGemPortPmhd;
    MIB_TABLE_GEM_PORT_PMHD_T   mibCurrentBin;

    pMibGemPortPmhd = (MIB_TABLE_GEM_PORT_PMHD_T *)pNewRow;

    // check if corresponding GEM port exists
    mibGemPort.EntityID = pMibGemPortPmhd->EntityId;
    ret = MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGemPort, sizeof(MIB_TABLE_GEMPORTCTP_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMPORTCTP_INDEX), mibGemPort.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_GEM_PORT_PMHD_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibGemPortPmhd, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibGemPortPmhd->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibGemPortPmhd->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibGemPortPmhd->ThresholdID;
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

GOS_ERROR_CODE gem_port_pmhd_pm_handler(MIB_TABLE_INDEX         tableIndex,
                                        omci_me_instance_t      instanceID,
                                        omci_pm_oper_type_t     operType,
                                        BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    MIB_TABLE_GEM_PORT_PMHD_T   mibGemPortPmhd;
    MIB_TABLE_GEM_PORT_PMHD_T   mibCurrentBin;
    omci_flow_stat_t            PortCntrs[2];/*ds/us*/
    omci_flow_stat_t            *pPortCntrs = (omci_flow_stat_t*)&PortCntrs[0];
    unsigned long long          tempCntr;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibGemPort.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGemPort, sizeof(MIB_TABLE_GEMPORTCTP_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMPORTCTP_INDEX), mibGemPort.EntityID);

        return GOS_FAIL;
    }

    mibGemPortPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibGemPortPmhd, entrySize))
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
        mibCurrentBin.ThresholdID = mibGemPortPmhd.ThresholdID;

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
		omci_pm_update_gem_port(OMCI_PM_OPER_GET_CURRENT_DATA);
	}

    if (OMCI_PM_OPER_UPDATE == operType ||
		OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
		OMCI_PM_OPER_GET_CURRENT_DATA == operType)

    {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_gem_port(instanceID,pPortCntrs))
        {
            // ds counters
            if (GPNC_DIRECTION_ANI_TO_UNI == mibGemPort.Direction ||
                    GPNC_DIRECTION_BIDIRECTION == mibGemPort.Direction)
            {
                //mibCurrentBin.LostPkts
                tempCntr = pPortCntrs->gemBlock;
                m_omci_pm_update_reset_u64_attr(tempCntr, mibCurrentBin.ReceivedPkts);
                // TBD, should be aligned with GEM block length of ANI-G
                tempCntr = (mibCurrentBin.ReceivedPkts.low +
                    ((unsigned long long)mibCurrentBin.ReceivedPkts.high << 32)) / 48;
                mibCurrentBin.ReceivedBlocks.high = tempCntr >> 32;
                mibCurrentBin.ReceivedBlocks.low = tempCntr & 0xFFFFFFFF;
            }

            // moves to US counters
            pPortCntrs++;

            // us counters
            if (GPNC_DIRECTION_UNI_TO_ANI == mibGemPort.Direction ||
                    GPNC_DIRECTION_BIDIRECTION == mibGemPort.Direction)
            {
                //mibCurrentBin.MisinsertedPkts
                tempCntr = pPortCntrs->gemBlock;
                m_omci_pm_update_reset_u64_attr(tempCntr, mibCurrentBin.TransmittedPkts);
                // TBD, should be aligned with GEM block length of ANI-G
                tempCntr = (mibCurrentBin.TransmittedPkts.low +
                    ((unsigned long long)mibCurrentBin.TransmittedPkts.high << 32)) / 48;
                mibCurrentBin.TransmittedBlocks.high = tempCntr >> 32;
                mibCurrentBin.TransmittedBlocks.low = tempCntr & 0xFFFFFFFF;
            }

            //mibCurrentBin.ImpairedBlocks
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
        mibGemPortPmhd.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibGemPortPmhd.ThresholdID;
        mibCurrentBin.IntEndTime = mibGemPortPmhd.IntEndTime;

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
    gMibGemPortPmhdTableInfo.Name = "GemPortPmhd";
    gMibGemPortPmhdTableInfo.ShortName = "GEMPPMHD";
    gMibGemPortPmhdTableInfo.Desc = "GEM port performance monitoring history data";
    gMibGemPortPmhdTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_GEM_PORT_PMHD);
    gMibGemPortPmhdTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibGemPortPmhdTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibGemPortPmhdTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibGemPortPmhdTableInfo.pAttributes = &(gMibGemPortPmhdAttrInfo[0]);

    gMibGemPortPmhdTableInfo.attrNum = MIB_TABLE_GEM_PORT_PMHD_ATTR_NUM;
    gMibGemPortPmhdTableInfo.entrySize = sizeof(MIB_TABLE_GEM_PORT_PMHD_T);
    gMibGemPortPmhdTableInfo.pDefaultRow = &gMibGemPortPmhdDefRow;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LostPkts";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MisinsertedPkts";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedPkts";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedBlocks";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TransmittedBlocks";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ImpairedBlocks";

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Lost packets";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Misinserted packets";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Received packets";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Received blocks";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transmitted blocks";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Impaired blocks";

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_RECEIVED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    // private attributes
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TransmittedPkts";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transmitted packets";
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortPmhdAttrInfo[MIB_TABLE_GEM_PORT_PMHD_TRANSMITTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibGemPortPmhdDefRow.EntityId), 0x00, sizeof(gMibGemPortPmhdDefRow.EntityId));
    memset(&(gMibGemPortPmhdDefRow.IntEndTime), 0x00, sizeof(gMibGemPortPmhdDefRow.IntEndTime));
    memset(&(gMibGemPortPmhdDefRow.ThresholdID), 0x00, sizeof(gMibGemPortPmhdDefRow.ThresholdID));
    memset(&(gMibGemPortPmhdDefRow.LostPkts), 0x00, sizeof(gMibGemPortPmhdDefRow.LostPkts));
    memset(&(gMibGemPortPmhdDefRow.MisinsertedPkts), 0x00, sizeof(gMibGemPortPmhdDefRow.MisinsertedPkts));
    memset(&(gMibGemPortPmhdDefRow.ReceivedPkts), 0x00, sizeof(gMibGemPortPmhdDefRow.ReceivedPkts));
    memset(&(gMibGemPortPmhdDefRow.ReceivedBlocks), 0x00, sizeof(gMibGemPortPmhdDefRow.ReceivedBlocks));
    memset(&(gMibGemPortPmhdDefRow.TransmittedBlocks), 0x00, sizeof(gMibGemPortPmhdDefRow.TransmittedBlocks));
    memset(&(gMibGemPortPmhdDefRow.ImpairedBlocks), 0x00, sizeof(gMibGemPortPmhdDefRow.ImpairedBlocks));
    memset(&(gMibGemPortPmhdDefRow.TransmittedPkts), 0x00, sizeof(gMibGemPortPmhdDefRow.TransmittedPkts));

    memset(&gMibGemPortPmhdOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibGemPortPmhdOper.meOperDrvCfg = gem_port_pmhd_drv_cfg_handler;
    gMibGemPortPmhdOper.meOperConnCheck = NULL;
    gMibGemPortPmhdOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibGemPortPmhdOper.meOperConnCfg = NULL;
    gMibGemPortPmhdOper.meOperPmHandler = gem_port_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_GEM_PORT_PMHD_LOST_PKTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_GEM_PORT_PMHD_MISINSERTED_PKTS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_GEM_PORT_PMHD_IMPAIRED_BLOCKS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;

    MIB_TABLE_GEM_PORT_PMHD_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibGemPortPmhdTableInfo, &gMibGemPortPmhdOper);

    return GOS_OK;
}
