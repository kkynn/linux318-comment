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
 * Purpose : Definition of ME handler: GEM port network CTP PMHD (341)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: GEM port network CTP PMHD (341)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/gpon.h"
#endif

MIB_TABLE_INFO_T gMibGpncPmhdTableInfo;
MIB_ATTR_INFO_T  gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ATTR_NUM];
MIB_TABLE_GPNC_PMHD_T gMibGpncPmhdDefRow;
MIB_TABLE_OPER_T gMibGpncPmhdOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_GPNC_PMHD_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_GPNC_PMHD_ATTR_NUM];


GOS_ERROR_CODE gpnc_pmhd_drv_cfg_handler(void           *pOldRow,
                                        void            *pNewRow,
                                        MIB_OPERA_TYPE  operationType,
                                        MIB_ATTRS_SET   attrSet,
                                        UINT32          pri)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_INDEX             tableIndex;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    MIB_TABLE_GPNC_PMHD_T       *pMibGpncPmhd;
    MIB_TABLE_GPNC_PMHD_T       mibCurrentBin;

    pMibGpncPmhd = (MIB_TABLE_GPNC_PMHD_T *)pNewRow;

    // check if corresponding GEM port exists
    mibGemPort.EntityID = pMibGpncPmhd->EntityId;
    ret = MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGemPort, sizeof(MIB_TABLE_GEMPORTCTP_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMPORTCTP_INDEX), mibGemPort.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_GPNC_PMHD_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibGpncPmhd, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibGpncPmhd->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibGpncPmhd->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibGpncPmhd->ThresholdID;
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

GOS_ERROR_CODE gpnc_pmhd_pm_handler(MIB_TABLE_INDEX         tableIndex,
                                    omci_me_instance_t      instanceID,
                                    omci_pm_oper_type_t     operType,
                                    BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE              ret;
    UINT32                      entrySize;
    MIB_TABLE_GEMPORTCTP_T      mibGemPort;
    MIB_TABLE_GPNC_PMHD_T       mibGpncPmhd;
    MIB_TABLE_GPNC_PMHD_T       mibCurrentBin;
    omci_flow_stat_t            PortCntrs[2];
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

    mibGpncPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibGpncPmhd, entrySize))
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
        mibCurrentBin.ThresholdID = mibGpncPmhd.ThresholdID;

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
                m_omci_pm_update_reset_u32_attr(pPortCntrs->gemBlock, mibCurrentBin.RxGemFrames);
                tempCntr = pPortCntrs->gemByte;
                m_omci_pm_update_reset_u64_attr(tempCntr, mibCurrentBin.RxPayloadBytes);
            }

            // moves to US counters
            pPortCntrs++;

            // us counters
            if (GPNC_DIRECTION_UNI_TO_ANI == mibGemPort.Direction ||
                    GPNC_DIRECTION_BIDIRECTION == mibGemPort.Direction)
            {
                m_omci_pm_update_reset_u32_attr(pPortCntrs->gemBlock, mibCurrentBin.TxGemFrames);
                tempCntr = pPortCntrs->gemByte;
                m_omci_pm_update_reset_u64_attr(tempCntr, mibCurrentBin.TxPayloadBytes);
            }

            //mibCurrentBin.EncryptKeyErrors
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
        mibGpncPmhd.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibGpncPmhd.ThresholdID;
        mibCurrentBin.IntEndTime = mibGpncPmhd.IntEndTime;

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
    gMibGpncPmhdTableInfo.Name = "GpncPmhd";
    gMibGpncPmhdTableInfo.ShortName = "GEMPNCTPPMHD";
    gMibGpncPmhdTableInfo.Desc = "GEM port network CTP performance monitoring history data";
    gMibGpncPmhdTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_GEM_PORT_NETWORK_CTP_PMHD);
    gMibGpncPmhdTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibGpncPmhdTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibGpncPmhdTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibGpncPmhdTableInfo.pAttributes = &(gMibGpncPmhdAttrInfo[0]);

	gMibGpncPmhdTableInfo.attrNum = MIB_TABLE_GPNC_PMHD_ATTR_NUM;
	gMibGpncPmhdTableInfo.entrySize = sizeof(MIB_TABLE_GPNC_PMHD_T);
	gMibGpncPmhdTableInfo.pDefaultRow = &gMibGpncPmhdDefRow;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxGemFrames";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxGemFrames";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxPayloadBytes";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxPayloadBytes";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EncryptKeyErrors";

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval end time";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold data 1/2 id";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transmitted GEM frames";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Received GEM frames";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Received payload bytes";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transmitted payload bytes";
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Encryption key errors";

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 8;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 8;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_GEM_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_RX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_TX_PAYLOAD_BYTES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibGpncPmhdAttrInfo[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibGpncPmhdDefRow.EntityId), 0x00, sizeof(gMibGpncPmhdDefRow.EntityId));
    memset(&(gMibGpncPmhdDefRow.IntEndTime), 0x00, sizeof(gMibGpncPmhdDefRow.IntEndTime));
    memset(&(gMibGpncPmhdDefRow.ThresholdID), 0x00, sizeof(gMibGpncPmhdDefRow.ThresholdID));
    memset(&(gMibGpncPmhdDefRow.TxGemFrames), 0x00, sizeof(gMibGpncPmhdDefRow.TxGemFrames));
    memset(&(gMibGpncPmhdDefRow.RxGemFrames), 0x00, sizeof(gMibGpncPmhdDefRow.RxGemFrames));
    memset(&(gMibGpncPmhdDefRow.RxPayloadBytes), 0x00, sizeof(gMibGpncPmhdDefRow.RxPayloadBytes));
    memset(&(gMibGpncPmhdDefRow.TxPayloadBytes), 0x00, sizeof(gMibGpncPmhdDefRow.TxPayloadBytes));
    memset(&(gMibGpncPmhdDefRow.EncryptKeyErrors), 0x00, sizeof(gMibGpncPmhdDefRow.EncryptKeyErrors));

    memset(&gMibGpncPmhdOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibGpncPmhdOper.meOperDrvCfg = gpnc_pmhd_drv_cfg_handler;
    gMibGpncPmhdOper.meOperConnCheck = NULL;
    gMibGpncPmhdOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibGpncPmhdOper.meOperConnCfg = NULL;
    gMibGpncPmhdOper.meOperPmHandler = gpnc_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_GPNC_PMHD_ENCRYPT_KEY_ERRORS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;

	MIB_TABLE_GPNC_PMHD_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibGpncPmhdTableInfo, &gMibGpncPmhdOper);

    return GOS_OK;
}
