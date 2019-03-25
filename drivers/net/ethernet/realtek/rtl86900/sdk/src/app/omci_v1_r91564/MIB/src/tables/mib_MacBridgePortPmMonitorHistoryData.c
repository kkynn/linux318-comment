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
 * Purpose : Definition of ME handler: MAC bridge port PMHD (52)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: MAC bridge port PMHD (52)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/stat.h"
#endif

MIB_TABLE_INFO_T gMibMacBridgePortPmhdTableInfo;
MIB_ATTR_INFO_T  gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ATTR_NUM];
MIB_TABLE_MACBRIDGEPORTPMHD_T gMibMacBridgePortPmhdDefRow;
MIB_TABLE_OPER_T gMibMacBridgePortPmhdOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_MACBRIDGEPORTPMHD_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_MACBRIDGEPORTPMHD_ATTR_NUM];


GOS_ERROR_CODE MacBridgePortPmhdDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_MACBRIDGEPORTPMHD_T   *pMibMbpPmhd;
    MIB_TABLE_MACBRIDGEPORTPMHD_T   mibCurrentBin;

    pMibMbpPmhd = (MIB_TABLE_MACBRIDGEPORTPMHD_T *)pNewRow;

    // check if corresponding PPTP exists
    mibMBPCD.EntityID = pMibMbpPmhd->EntityId;
    ret = MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_FAIL;
    }

    if (MBPCD_TP_TYPE_PPTP_ETH_UNI != mibMBPCD.TPType)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "TP type is not supported in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_ERR_NOTSUPPORT;
    }

    mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_MACBRIDGEPORTPMHD_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibMbpPmhd, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibMbpPmhd->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibMbpPmhd->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibMbpPmhd->ThresholdID;
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

GOS_ERROR_CODE mac_bridge_port_pmhd_pm_handler(MIB_TABLE_INDEX          tableIndex,
                                                omci_me_instance_t      instanceID,
                                                omci_pm_oper_type_t     operType,
                                                BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_MACBRIDGEPORTPMHD_T   mibMbpPmhd;
    MIB_TABLE_MACBRIDGEPORTPMHD_T   mibCurrentBin;
    omci_port_stat_t                portCntrs;
    omci_port_stat_t                *pPortCntrs = (omci_port_stat_t*)&portCntrs;
    unsigned long long              tempCntr;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibMBPCD.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_FAIL;
    }

    if (MBPCD_TP_TYPE_PPTP_ETH_UNI != mibMBPCD.TPType)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "TP type is not supported in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_ERR_NOTSUPPORT;
    }

    mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
    if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    mibMbpPmhd.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibMbpPmhd, entrySize))
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
        mibCurrentBin.ThresholdID = mibMbpPmhd.ThresholdID;

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
        if (GOS_OK == omci_pm_getcurrent_pptp_eth_uni(mibPptpEthUNI.EntityID,pPortCntrs))
        {
            tempCntr = pPortCntrs->etherStatsTxUndersizePkts +
                        pPortCntrs->etherStatsTxPkts64Octets +
                        pPortCntrs->etherStatsTxPkts65to127Octets +
                        pPortCntrs->etherStatsTxPkts128to255Octets +
                        pPortCntrs->etherStatsTxPkts256to511Octets +
                        pPortCntrs->etherStatsTxPkts512to1023Octets +
                        pPortCntrs->etherStatsTxPkts1024to1518Octets +
                        pPortCntrs->etherStatsTxOversizePkts;
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.ForwardedFrameCounter);
            m_omci_pm_update_accum_attr(pPortCntrs->dot3StatsExcessiveCollisions, mibCurrentBin.DelayExceededDiscard);
            //mibCurrentBin.MtuExceededDiscard
            tempCntr = pPortCntrs->etherStatsFragments +
                        pPortCntrs->etherStatsRxUndersizePkts +
                        pPortCntrs->etherStatsRxPkts64Octets +
                        pPortCntrs->etherStatsRxPkts65to127Octets +
                        pPortCntrs->etherStatsRxPkts128to255Octets +
                        pPortCntrs->etherStatsRxPkts256to511Octets +
                        pPortCntrs->etherStatsRxPkts512to1023Octets +
                        pPortCntrs->etherStatsRxPkts1024to1518Octets +
                        pPortCntrs->etherStatsRxOversizePkts +
                        pPortCntrs->etherStatsJabbers;
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.ReceivedFrameCounter);

			tempCntr = pPortCntrs->etherStatsFragments+
						pPortCntrs->etherStatsJabbers+
						pPortCntrs->etherStatsCRCAlignErrors+
						pPortCntrs->dot3StatsSymbolErrors;
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.ReceivedAndDiscardedCounter);

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
        mibMbpPmhd.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibMbpPmhd.ThresholdID;
        mibCurrentBin.IntEndTime = mibMbpPmhd.IntEndTime;

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
    gMibMacBridgePortPmhdTableInfo.Name = "MacBridgePortPmMonitorHistoryData";
    gMibMacBridgePortPmhdTableInfo.ShortName = "MBPPMHD";
    gMibMacBridgePortPmhdTableInfo.Desc = "MAC bridge port performance monitoring history data";
    gMibMacBridgePortPmhdTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MAC_BRG_PORT_PMHD);
    gMibMacBridgePortPmhdTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMacBridgePortPmhdTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibMacBridgePortPmhdTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibMacBridgePortPmhdTableInfo.pAttributes = &(gMibMacBridgePortPmhdAttrInfo[0]);

	gMibMacBridgePortPmhdTableInfo.attrNum = MIB_TABLE_MACBRIDGEPORTPMHD_ATTR_NUM;
	gMibMacBridgePortPmhdTableInfo.entrySize = sizeof(MIB_TABLE_MACBRIDGEPORTPMHD_T);
	gMibMacBridgePortPmhdTableInfo.pDefaultRow = &gMibMacBridgePortPmhdDefRow;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntervalEndTime";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdData12Id";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ForwardedFrameCounter";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DelayExceededDiscard";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MtuExceededDiscard";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedFrameCounter";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedAndDiscardedCounter";

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "interval end time";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "threshold data 1/2 id";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "forwarded frame counter";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "delay exceeded discard";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "MTU Exceeded discard";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "receoved frame counter";
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "received and discard counter";

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_INTERVALENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_THRESHOLDDATA12ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_FORWARDEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDFRAMECOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibMacBridgePortPmhdAttrInfo[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibMacBridgePortPmhdDefRow.EntityId), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.EntityId));
    memset(&(gMibMacBridgePortPmhdDefRow.IntEndTime), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.IntEndTime));
    memset(&(gMibMacBridgePortPmhdDefRow.ThresholdID), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.ThresholdID));
    memset(&(gMibMacBridgePortPmhdDefRow.ForwardedFrameCounter), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.ForwardedFrameCounter));
    memset(&(gMibMacBridgePortPmhdDefRow.DelayExceededDiscard), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.DelayExceededDiscard));
    memset(&(gMibMacBridgePortPmhdDefRow.MtuExceededDiscard), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.MtuExceededDiscard));
    memset(&(gMibMacBridgePortPmhdDefRow.ReceivedFrameCounter), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.ReceivedFrameCounter));
    memset(&(gMibMacBridgePortPmhdDefRow.ReceivedAndDiscardedCounter), 0x00, sizeof(gMibMacBridgePortPmhdDefRow.ReceivedAndDiscardedCounter));

    memset(&gMibMacBridgePortPmhdOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibMacBridgePortPmhdOper.meOperDrvCfg = MacBridgePortPmhdDrvCfg;
    gMibMacBridgePortPmhdOper.meOperConnCheck = NULL;
    gMibMacBridgePortPmhdOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibMacBridgePortPmhdOper.meOperConnCfg = NULL;
    gMibMacBridgePortPmhdOper.meOperPmHandler = mac_bridge_port_pmhd_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_MACBRIDGEPORTPMHD_DELAYEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_MACBRIDGEPORTPMHD_MTUEXCEEDEDDISCARD_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_MACBRIDGEPORTPMHD_RECEIVEDANDDISCARDEDCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX] = 3;

	MIB_TABLE_MACBRIDGEPORTPMHD_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibMacBridgePortPmhdTableInfo, &gMibMacBridgePortPmhdOper);

    return GOS_OK;
}
