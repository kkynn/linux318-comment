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
 * Purpose : Definition of ME handler: FEC PMHD (312)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: FEC PMHD (312)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#include "rtk/gpon.h"
#endif

MIB_TABLE_INFO_T gMibFecPmhdTableInfo;
MIB_ATTR_INFO_T  gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ATTR_NUM];
MIB_TABLE_FEC_PMHD_T gMibFecPmhdDefRow;
MIB_TABLE_OPER_T gMibFecPmhdOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_FEC_PMHD_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_FEC_PMHD_ATTR_NUM];


GOS_ERROR_CODE fec_pmhd_drv_cfg_handler(void            *pOldRow,
                                        void            *pNewRow,
                                        MIB_OPERA_TYPE  operationType,
                                        MIB_ATTRS_SET   attrSet,
                                        UINT32          pri)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_INDEX             tableIndex;
    MIB_TABLE_ANIG_T            mibAniG;
    MIB_TABLE_FEC_PMHD_T        *pMibFecPmhd;
    MIB_TABLE_FEC_PMHD_T        mibCurrentBin;

    pMibFecPmhd = (MIB_TABLE_FEC_PMHD_T *)pNewRow;

    // check if corresponding ANI-G exists
    mibAniG.EntityID = pMibFecPmhd->EntityId;
    ret = MIB_Get(MIB_TABLE_ANIG_INDEX, &mibAniG, sizeof(MIB_TABLE_ANIG_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ANIG_INDEX), mibAniG.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_FEC_PMHD_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibFecPmhd, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibFecPmhd->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibFecPmhd->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibFecPmhd->ThresholdID;
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

GOS_ERROR_CODE fec_pmhd_pm_handler(MIB_TABLE_INDEX          tableIndex,
                                    omci_me_instance_t      instanceID,
                                    omci_pm_oper_type_t     operType,
                                    BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_ANIG_T            mibAniG;
    MIB_TABLE_FEC_PMHD_T        mibFecPmhd;
    MIB_TABLE_FEC_PMHD_T        mibCurrentBin;
    omci_ds_fec_stat_t          dsFecCntrs;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibAniG.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_ANIG_INDEX, &mibAniG, sizeof(MIB_TABLE_ANIG_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ANIG_INDEX), mibAniG.EntityID);

        return GOS_FAIL;
    }

    mibFecPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibFecPmhd, entrySize))
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
        mibCurrentBin.ThresholdID = mibFecPmhd.ThresholdID;

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

    if (OMCI_PM_OPER_UPDATE == operType ||
		OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
		OMCI_PM_OPER_GET_CURRENT_DATA == operType)

    {
        // update PM values
        if (GOS_OK == omci_wrapper_getDsFecStat(&dsFecCntrs))
        {
            m_omci_pm_update_reset_u32_attr(dsFecCntrs.corByte, mibCurrentBin.CorBytes);
            m_omci_pm_update_reset_u32_attr(dsFecCntrs.corCodeword, mibCurrentBin.CorCodeWords);
            m_omci_pm_update_reset_u32_attr(dsFecCntrs.uncorCodeword, mibCurrentBin.UncorCodeWords);
            mibCurrentBin.TotalCodeWords = mibCurrentBin.CorCodeWords + mibCurrentBin.UncorCodeWords;
            //mibCurrentBin.FecSeconds
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
        mibFecPmhd.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibFecPmhd.ThresholdID;
        mibCurrentBin.IntEndTime = mibFecPmhd.IntEndTime;

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
    gMibFecPmhdTableInfo.Name = "FecPmhd";
    gMibFecPmhdTableInfo.ShortName = "FECPMHD";
    gMibFecPmhdTableInfo.Desc = "FEC performance monitoring history data";
    gMibFecPmhdTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_FEC_PMHD);
    gMibFecPmhdTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibFecPmhdTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibFecPmhdTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibFecPmhdTableInfo.pAttributes = &(gMibFecPmhdAttrInfo[0]);

	gMibFecPmhdTableInfo.attrNum = MIB_TABLE_FEC_PMHD_ATTR_NUM;
	gMibFecPmhdTableInfo.entrySize = sizeof(MIB_TABLE_FEC_PMHD_T);
	gMibFecPmhdTableInfo.pDefaultRow = &gMibFecPmhdDefRow;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CorBytes";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CorCodeWords";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UncorCodeWords";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TotalCodeWords";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FecSeconds";

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Corrected bytes";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Corrected code words";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Uncorrectable code words";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Total code words";
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "FEC seconds";

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_TOTAL_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibFecPmhdAttrInfo[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibFecPmhdDefRow.EntityId), 0x00, sizeof(gMibFecPmhdDefRow.EntityId));
    memset(&(gMibFecPmhdDefRow.IntEndTime), 0x00, sizeof(gMibFecPmhdDefRow.IntEndTime));
    memset(&(gMibFecPmhdDefRow.ThresholdID), 0x00, sizeof(gMibFecPmhdDefRow.ThresholdID));
    memset(&(gMibFecPmhdDefRow.CorBytes), 0x00, sizeof(gMibFecPmhdDefRow.CorBytes));
    memset(&(gMibFecPmhdDefRow.CorCodeWords), 0x00, sizeof(gMibFecPmhdDefRow.CorCodeWords));
    memset(&(gMibFecPmhdDefRow.UncorCodeWords), 0x00, sizeof(gMibFecPmhdDefRow.UncorCodeWords));
    memset(&(gMibFecPmhdDefRow.TotalCodeWords), 0x00, sizeof(gMibFecPmhdDefRow.TotalCodeWords));
    memset(&(gMibFecPmhdDefRow.FecSeconds), 0x00, sizeof(gMibFecPmhdDefRow.FecSeconds));

    memset(&gMibFecPmhdOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibFecPmhdOper.meOperDrvCfg = fec_pmhd_drv_cfg_handler;
    gMibFecPmhdOper.meOperConnCheck = NULL;
    gMibFecPmhdOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibFecPmhdOper.meOperConnCfg = NULL;
    gMibFecPmhdOper.meOperPmHandler = fec_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_FEC_PMHD_COR_BYTES_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_FEC_PMHD_COR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_FEC_PMHD_UNCOR_CODE_WORDS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_FEC_PMHD_FEC_SECONDS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;

	MIB_TABLE_FEC_PMHD_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibFecPmhdTableInfo, &gMibFecPmhdOper);

    return GOS_OK;
}
