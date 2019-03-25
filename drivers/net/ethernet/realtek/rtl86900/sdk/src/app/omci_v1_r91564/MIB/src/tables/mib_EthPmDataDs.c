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
 * Purpose : Definition of ME handler: Ethernet frame PMHD DS (321)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Ethernet frame PMHD DS (321)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/stat.h"
#endif

MIB_TABLE_INFO_T gMibEthPmDataDsTableInfo;
MIB_ATTR_INFO_T  gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ATTR_NUM];
MIB_TABLE_ETHPMDATADS_T gMibEthPmDataDsDefRow;
MIB_TABLE_OPER_T gMibEthPmDataDsOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_ETHPMDATADS_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_ETHPMDATADS_ATTR_NUM];


GOS_ERROR_CODE EthPmDataDs_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATADS_T         *pMibEthPmhdDS;
    MIB_TABLE_ETHPMDATADS_T         mibCurrentBin;

    pMibEthPmhdDS = (MIB_TABLE_ETHPMDATADS_T *)pNewRow;

    // check if corresponding PPTP exists
    mibMBPCD.EntityID = pMibEthPmhdDS->EntityId;
    ret = MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_FAIL;
    }

    if (MBPCD_TP_TYPE_PPTP_ETH_UNI != mibMBPCD.TPType)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "TP type is not correct in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_FAIL;
    }

    mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_ETHPMDATADS_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibEthPmhdDS, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthPmhdDS->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibEthPmhdDS->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibEthPmhdDS->ThresholdID;
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

GOS_ERROR_CODE ethernet_frame_pmhd_ds_pm_handler(MIB_TABLE_INDEX        tableIndex,
                                                omci_me_instance_t      instanceID,
                                                omci_pm_oper_type_t     operType,
                                                BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATADS_T         mibEthPmhdDS;
    MIB_TABLE_ETHPMDATADS_T         mibCurrentBin;
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
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "TP type is not correct in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

        return GOS_FAIL;
    }

    mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
    if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    mibEthPmhdDS.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibEthPmhdDS, entrySize))
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
        mibCurrentBin.ThresholdID = mibEthPmhdDS.ThresholdID;

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
            //mibCurrentBin.DropEvents
            m_omci_pm_update_accum_attr(pPortCntrs->ifOutOctets, mibCurrentBin.Octets);
            tempCntr = pPortCntrs->etherStatsTxUndersizePkts +
                        pPortCntrs->etherStatsTxPkts64Octets +
                        pPortCntrs->etherStatsTxPkts65to127Octets +
                        pPortCntrs->etherStatsTxPkts128to255Octets +
                        pPortCntrs->etherStatsTxPkts256to511Octets +
                        pPortCntrs->etherStatsTxPkts512to1023Octets +
                        pPortCntrs->etherStatsTxPkts1024to1518Octets +
                        pPortCntrs->etherStatsTxOversizePkts;
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.Packets);
            m_omci_pm_update_accum_attr(pPortCntrs->ifOutBrocastPkts, mibCurrentBin.BroadcastPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->ifOutMulticastPkts, mibCurrentBin.MulticastPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxCRCAlignErrors, mibCurrentBin.CrcErrPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxUndersizePkts, mibCurrentBin.UndersizePackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxOversizePkts, mibCurrentBin.OversizePackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts64Octets, mibCurrentBin.Octets64);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts65to127Octets, mibCurrentBin.Octets65to127);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts128to255Octets, mibCurrentBin.Octets128to255);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts256to511Octets, mibCurrentBin.Octets256to511);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts512to1023Octets, mibCurrentBin.Octets512to1023);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsTxPkts1024to1518Octets, mibCurrentBin.Octets1024to1518);
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
        mibEthPmhdDS.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibEthPmhdDS.ThresholdID;
        mibCurrentBin.IntEndTime = mibEthPmhdDS.IntEndTime;

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
    gMibEthPmDataDsTableInfo.Name = "EthPmDataDs";
    gMibEthPmDataDsTableInfo.ShortName = "ETHFRAMEPMHDDS";
    gMibEthPmDataDsTableInfo.Desc = "Ethernet frame performance monitoring history data downstream";
    gMibEthPmDataDsTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ETHERNET_FRAME_PMHD_DOWNSTREAM);
    gMibEthPmDataDsTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibEthPmDataDsTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibEthPmDataDsTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibEthPmDataDsTableInfo.pAttributes = &(gMibEthPmDataDsAttrInfo[0]);

	gMibEthPmDataDsTableInfo.attrNum = MIB_TABLE_ETHPMDATADS_ATTR_NUM;
	gMibEthPmDataDsTableInfo.entrySize = sizeof(MIB_TABLE_ETHPMDATADS_T);
	gMibEthPmDataDsTableInfo.pDefaultRow = &gMibEthPmDataDsDefRow;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntEndTime";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdID";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DropEvents";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BroadcastPackets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MulticastPackets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CrcErrPackets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UndersizePackets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OversizePackets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets64";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets65to127";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets128to255";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets256to511";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets512to1023";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets1024to1518";

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval End Time";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold Data ID";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Drop Events";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of Octets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of broadcast packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of multicast packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "CRC errored packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of undersize packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of oversize packets";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 64 octets long";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 65..127 octets long";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 128..255 octets long";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 256..511 octets long";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 512..1023 octets long";
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 1024..1518 octets long";

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmDataDsAttrInfo[MIB_TABLE_ETHPMDATADS_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibEthPmDataDsDefRow.EntityId), 0x00, sizeof(gMibEthPmDataDsDefRow.EntityId));
    memset(&(gMibEthPmDataDsDefRow.IntEndTime), 0x00, sizeof(gMibEthPmDataDsDefRow.IntEndTime));
    memset(&(gMibEthPmDataDsDefRow.ThresholdID), 0x00, sizeof(gMibEthPmDataDsDefRow.ThresholdID));
    memset(&(gMibEthPmDataDsDefRow.DropEvents), 0x00, sizeof(gMibEthPmDataDsDefRow.DropEvents));
    memset(&(gMibEthPmDataDsDefRow.Octets), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets));
    memset(&(gMibEthPmDataDsDefRow.Packets), 0x00, sizeof(gMibEthPmDataDsDefRow.Packets));
    memset(&(gMibEthPmDataDsDefRow.BroadcastPackets), 0x00, sizeof(gMibEthPmDataDsDefRow.BroadcastPackets));
    memset(&(gMibEthPmDataDsDefRow.MulticastPackets), 0x00, sizeof(gMibEthPmDataDsDefRow.MulticastPackets));
    memset(&(gMibEthPmDataDsDefRow.CrcErrPackets), 0x00, sizeof(gMibEthPmDataDsDefRow.CrcErrPackets));
    memset(&(gMibEthPmDataDsDefRow.UndersizePackets), 0x00, sizeof(gMibEthPmDataDsDefRow.UndersizePackets));
    memset(&(gMibEthPmDataDsDefRow.OversizePackets), 0x00, sizeof(gMibEthPmDataDsDefRow.OversizePackets));
    memset(&(gMibEthPmDataDsDefRow.Octets64), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets64));
    memset(&(gMibEthPmDataDsDefRow.Octets65to127), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets65to127));
    memset(&(gMibEthPmDataDsDefRow.Octets128to255), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets128to255));
    memset(&(gMibEthPmDataDsDefRow.Octets256to511), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets256to511));
    memset(&(gMibEthPmDataDsDefRow.Octets512to1023), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets512to1023));
    memset(&(gMibEthPmDataDsDefRow.Octets1024to1518), 0x00, sizeof(gMibEthPmDataDsDefRow.Octets1024to1518));

    memset(&gMibEthPmDataDsOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthPmDataDsOper.meOperDrvCfg = EthPmDataDs_CfgMe;
    gMibEthPmDataDsOper.meOperConnCheck = NULL;
	gMibEthPmDataDsOper.meOperConnCfg = NULL;
	gMibEthPmDataDsOper.meOperAvlTreeAdd = NULL;
    gMibEthPmDataDsOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibEthPmDataDsOper.meOperPmHandler = ethernet_frame_pmhd_ds_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_ETHPMDATADS_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_ETHPMDATADS_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_ETHPMDATADS_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_ETHPMDATADS_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;

    MIB_TABLE_ETHPMDATADS_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibEthPmDataDsTableInfo, &gMibEthPmDataDsOper);

    return GOS_OK;
}
