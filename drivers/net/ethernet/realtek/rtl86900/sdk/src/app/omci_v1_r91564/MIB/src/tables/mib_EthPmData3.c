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
 * Purpose : Definition of ME handler: Ethernet PMHD 3 (296)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Ethernet PMHD 3 (296)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/stat.h"
#endif

MIB_TABLE_INFO_T gMibEthPmData3TableInfo;
MIB_ATTR_INFO_T  gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ATTR_NUM];
MIB_TABLE_ETHPMDATA3_T gMibEthPmData3DefRow;
MIB_TABLE_OPER_T gMibEthPmData3Oper;

static UINT8    aTcaAlmNumber[MIB_TABLE_ETHPMDATA3_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_ETHPMDATA3_ATTR_NUM];


GOS_ERROR_CODE EthPmData3_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATA3_T          *pMibEthPMHD3;
    MIB_TABLE_ETHPMDATA3_T          mibCurrentBin;

    pMibEthPMHD3 = (MIB_TABLE_ETHPMDATA3_T *)pNewRow;
    mibPptpEthUNI.EntityID = pMibEthPMHD3->EntityId;

    // check if corresponding PPTP exists
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, MIB_GetTableEntrySize(MIB_TABLE_ETHUNI_INDEX));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_ETHPMDATA3_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibEthPMHD3, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthPMHD3->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibEthPMHD3->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibEthPMHD3->ThresholdID;
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

GOS_ERROR_CODE ethernet_pmhd3_pm_handler(MIB_TABLE_INDEX        tableIndex,
                                        omci_me_instance_t      instanceID,
                                        omci_pm_oper_type_t     operType,
                                        BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATA3_T          mibEthPMHD3;
    MIB_TABLE_ETHPMDATA3_T          mibCurrentBin;
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

    mibEthPMHD3.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibEthPMHD3, entrySize))
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
        mibCurrentBin.ThresholdID = mibEthPMHD3.ThresholdID;

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
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsDropEvents, mibCurrentBin.DropEvents);
            m_omci_pm_update_accum_attr(pPortCntrs->ifInOctets, mibCurrentBin.Octets);
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
            m_omci_pm_update_accum_attr(tempCntr, mibCurrentBin.Packets);
            m_omci_pm_update_accum_attr(pPortCntrs->ifInBroadcastPkts, mibCurrentBin.BroadcastPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->ifInMulticastPkts, mibCurrentBin.MulticastPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxUndersizePkts, mibCurrentBin.UndersizePackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsFragments, mibCurrentBin.FragmentsPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsJabbers, mibCurrentBin.JabbersPackets);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts64Octets, mibCurrentBin.Octets64);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts65to127Octets, mibCurrentBin.Octets65to127);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts128to255Octets, mibCurrentBin.Octets128to255);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts256to511Octets, mibCurrentBin.Octets256to511);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts512to1023Octets, mibCurrentBin.Octets512to1023);
            m_omci_pm_update_accum_attr(pPortCntrs->etherStatsRxPkts1024to1518Octets, mibCurrentBin.Octets1024to1518);
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
        mibEthPMHD3.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibEthPMHD3.ThresholdID;
        mibCurrentBin.IntEndTime = mibEthPMHD3.IntEndTime;

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
    gMibEthPmData3TableInfo.Name = "EthPmData3";
    gMibEthPmData3TableInfo.ShortName = "ETHPMHD3";
    gMibEthPmData3TableInfo.Desc = "Ethernet PM History Data 3";
    gMibEthPmData3TableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ETHERNET_PMHD_3);
    gMibEthPmData3TableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibEthPmData3TableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibEthPmData3TableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibEthPmData3TableInfo.pAttributes = &(gMibEthPmData3AttrInfo[0]);

	gMibEthPmData3TableInfo.attrNum = MIB_TABLE_ETHPMDATA3_ATTR_NUM;
	gMibEthPmData3TableInfo.entrySize = sizeof(MIB_TABLE_ETHPMDATA3_T);
	gMibEthPmData3TableInfo.pDefaultRow = &gMibEthPmData3DefRow;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntEndTime";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdID";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DropEvents";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BroadcastPackets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MulticastPackets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UndersizePackets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FragmentsPackets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "JabbersPackets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets64";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets65to127";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets128to255";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets256to511";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets512to1023";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets1024to1518";

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval End Time";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold Data ID";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Drop Events";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of Octets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of broadcast packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of multicast packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of undersize packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of fragments packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of Jabber packets";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 64 octets long";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 65..127 octets long";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 128..255 octets long";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 256..511 octets long";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 512..1023 octets long";
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 1024..1518 octets long";

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_PACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData3AttrInfo[MIB_TABLE_ETHPMDATA3_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibEthPmData3DefRow.EntityId), 0x00, sizeof(gMibEthPmData3DefRow.EntityId));
    memset(&(gMibEthPmData3DefRow.IntEndTime), 0x00, sizeof(gMibEthPmData3DefRow.IntEndTime));
    memset(&(gMibEthPmData3DefRow.ThresholdID), 0x00, sizeof(gMibEthPmData3DefRow.ThresholdID));
    memset(&(gMibEthPmData3DefRow.DropEvents), 0x00, sizeof(gMibEthPmData3DefRow.DropEvents));
    memset(&(gMibEthPmData3DefRow.Octets), 0x00, sizeof(gMibEthPmData3DefRow.Octets));
    memset(&(gMibEthPmData3DefRow.Packets), 0x00, sizeof(gMibEthPmData3DefRow.Packets));
    memset(&(gMibEthPmData3DefRow.BroadcastPackets), 0x00, sizeof(gMibEthPmData3DefRow.BroadcastPackets));
    memset(&(gMibEthPmData3DefRow.MulticastPackets), 0x00, sizeof(gMibEthPmData3DefRow.MulticastPackets));
    memset(&(gMibEthPmData3DefRow.UndersizePackets), 0x00, sizeof(gMibEthPmData3DefRow.UndersizePackets));
    memset(&(gMibEthPmData3DefRow.FragmentsPackets), 0x00, sizeof(gMibEthPmData3DefRow.FragmentsPackets));
    memset(&(gMibEthPmData3DefRow.JabbersPackets), 0x00, sizeof(gMibEthPmData3DefRow.JabbersPackets));
    memset(&(gMibEthPmData3DefRow.Octets64), 0x00, sizeof(gMibEthPmData3DefRow.Octets64));
    memset(&(gMibEthPmData3DefRow.Octets65to127), 0x00, sizeof(gMibEthPmData3DefRow.Octets65to127));
    memset(&(gMibEthPmData3DefRow.Octets128to255), 0x00, sizeof(gMibEthPmData3DefRow.Octets128to255));
    memset(&(gMibEthPmData3DefRow.Octets256to511), 0x00, sizeof(gMibEthPmData3DefRow.Octets256to511));
    memset(&(gMibEthPmData3DefRow.Octets512to1023), 0x00, sizeof(gMibEthPmData3DefRow.Octets512to1023));
    memset(&(gMibEthPmData3DefRow.Octets1024to1518), 0x00, sizeof(gMibEthPmData3DefRow.Octets1024to1518));

    memset(&gMibEthPmData3Oper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthPmData3Oper.meOperDrvCfg = EthPmData3_CfgMe;
    gMibEthPmData3Oper.meOperConnCheck = NULL;
	gMibEthPmData3Oper.meOperConnCfg = NULL;
	gMibEthPmData3Oper.meOperAvlTreeAdd = NULL;
    gMibEthPmData3Oper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibEthPmData3Oper.meOperPmHandler = ethernet_pmhd3_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_ETHPMDATA3_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_ETHPMDATA3_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_ETHPMDATA3_FRAGMENTSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_ETHPMDATA3_JABBERSPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;

    MIB_TABLE_ETHPMDATA3_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibEthPmData3TableInfo, &gMibEthPmData3Oper);

    return GOS_OK;
}
