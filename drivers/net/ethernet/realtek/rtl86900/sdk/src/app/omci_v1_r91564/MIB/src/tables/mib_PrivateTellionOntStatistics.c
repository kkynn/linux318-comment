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
 * Purpose : Definition of ME handler: private tellion ont statistics (255)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: private tellion ont statistics (255)
 */

#include "app_basic.h"

MIB_TABLE_INFO_T gMibPrivateTellionOntStatisticsTableInfo;
MIB_ATTR_INFO_T  gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ATTR_NUM];
MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T gMibPrivateTellionOntStatisticsDefRow;
MIB_TABLE_OPER_T gMibPrivateTellionOntStatisticsOper;


GOS_ERROR_CODE PrivateTellionOntStatistics_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                              ret;
    BOOL                                        bWrite = FALSE;
    MIB_TABLE_INDEX                             tableIndex;
    MIB_TABLE_ETHUNI_T                          mibPptpEthUNI;
    MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T    *pMibNew = NULL, *pMibOld = NULL, ontStatistics;
    UINT16					                    portId;
    omci_port_stat_t                            mibCnts;
    unsigned long long                          tempCntr;

    pMibNew = (MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T *)pNewRow;
    pMibOld = (MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T *)pOldRow;
    mibPptpEthUNI.EntityID = pMibNew->EntityId;

    // check if corresponding PPTP exists
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, MIB_GetTableEntrySize(MIB_TABLE_ETHUNI_INDEX));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }
    if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(mibPptpEthUNI.EntityID, &portId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "convert switch port not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);
        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX;

    switch (operationType)
    {
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX) &&
                pMibNew->reset != pMibOld->reset)
            {
                // clear sw and asic counter
                omci_wrapper_resetPortStat(portId);
                memset(&ontStatistics, 0, sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T));
                ontStatistics.EntityId = pMibNew->EntityId;
                MIB_Set(tableIndex, &ontStatistics, sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T));
            }
            break;
        case MIB_GET:
    		mibCnts.port = portId;
        	omci_wrapper_getPortStat(&mibCnts);

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX))
            {
                tempCntr = mibCnts.etherStatsFragments +
                            mibCnts.etherStatsRxUndersizePkts +
                            mibCnts.etherStatsRxPkts64Octets +
                            mibCnts.etherStatsRxPkts65to127Octets +
                            mibCnts.etherStatsRxPkts128to255Octets +
                            mibCnts.etherStatsRxPkts256to511Octets +
                            mibCnts.etherStatsRxPkts512to1023Octets +
                            mibCnts.etherStatsRxPkts1024to1518Octets +
                            mibCnts.etherStatsRxOversizePkts +
                            mibCnts.etherStatsJabbers;

                m_omci_pm_update_accum_attr(tempCntr, pMibNew->total_frame_up);

                bWrite = TRUE;
            }
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.dot3InPauseFrames, pMibNew->pause_frame_up);

                bWrite = TRUE;
            }
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX))
            {
                tempCntr = mibCnts.dot3StatsAligmentErrors +
                            mibCnts.dot3StatsFCSErrors +
                            mibCnts.dot3StatsFrameTooLongs;

                m_omci_pm_update_accum_attr(tempCntr, pMibNew->error_frame_up);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifInUcastPkts, pMibNew->uc_frame_up);

                bWrite = TRUE;
            }
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifInMulticastPkts, pMibNew->mc_frame_up);

                bWrite = TRUE;
            }
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifInBroadcastPkts, pMibNew->bc_frame_up);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX))
            {
                tempCntr = mibCnts.etherStatsDropEvents +
                            mibCnts.dot1dTpPortInDiscards +
                            mibCnts.ifInDiscards;
                m_omci_pm_update_accum_attr(tempCntr, pMibNew->dropped_frame_up);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX))
            {
                tempCntr = mibCnts.etherStatsTxUndersizePkts +
                            mibCnts.etherStatsTxPkts64Octets +
                            mibCnts.etherStatsTxPkts65to127Octets +
                            mibCnts.etherStatsTxPkts128to255Octets +
                            mibCnts.etherStatsTxPkts256to511Octets +
                            mibCnts.etherStatsTxPkts512to1023Octets +
                            mibCnts.etherStatsTxPkts1024to1518Octets +
                            mibCnts.etherStatsTxPkts1519toMaxOctets +
                            mibCnts.etherStatsTxOversizePkts +
                            mibCnts.ifOutUcastPkts +
                            mibCnts.ifOutMulticastPkts +
                            mibCnts.ifOutBrocastPkts;

                m_omci_pm_update_accum_attr(tempCntr, pMibNew->total_frame_down);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX))
            {

                m_omci_pm_update_accum_attr(mibCnts.dot3OutPauseFrames, pMibNew->pause_frame_down);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX))
            {
                // always zero
                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifOutUcastPkts, pMibNew->uc_frame_down);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifOutMulticastPkts, pMibNew->mc_frame_down);

                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifOutBrocastPkts, pMibNew->bc_frame_down);

                bWrite = TRUE;
            }
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX))
            {
                m_omci_pm_update_accum_attr(mibCnts.ifOutDiscards, pMibNew->dropped_frame_down);

                bWrite = TRUE;
            }
            if (bWrite)
                MIB_Set(tableIndex, pMibNew, sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T));
            break;
        default:
            break;
    }

	return ret;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibPrivateTellionOntStatisticsTableInfo.Name = "PrivateTellionOntStatistics";
    gMibPrivateTellionOntStatisticsTableInfo.ShortName = "ONTSTATISTICS";
    gMibPrivateTellionOntStatisticsTableInfo.Desc = "Private tellion ont statistics";
    gMibPrivateTellionOntStatisticsTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PRIVATE_TELLION_ONT_STATISTICS);
    gMibPrivateTellionOntStatisticsTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibPrivateTellionOntStatisticsTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibPrivateTellionOntStatisticsTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibPrivateTellionOntStatisticsTableInfo.pAttributes = &(gMibPrivateTellionOntStatisticsAttrInfo[0]);

	gMibPrivateTellionOntStatisticsTableInfo.attrNum = MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ATTR_NUM;
	gMibPrivateTellionOntStatisticsTableInfo.entrySize = sizeof(MIB_TABLE_PRIVATETELLIONONTSTATISTICS_T);
	gMibPrivateTellionOntStatisticsTableInfo.pDefaultRow = &gMibPrivateTellionOntStatisticsDefRow;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Reset";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpTotalFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpPauseFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpErrorFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpUcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpMcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpBcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UpDropFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownTotalFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownPauseFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownErrorFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownUcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownMcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownBcFrame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownDropFrame";

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Reset";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream total frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream pause frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream error frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream unicast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream multicast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream broadcast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream dropped frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream total frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream pause frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream error frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream unicast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream multicast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream broadcast frame";
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream dropped frame";

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_RESET_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_UPDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNTOTALFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNPAUSEFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNERRORFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNUCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNMCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNBCFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);
    gMibPrivateTellionOntStatisticsAttrInfo[MIB_TABLE_PRIVATETELLIONONTSTATISTICS_DOWNDROPFRAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY);

    memset(&(gMibPrivateTellionOntStatisticsDefRow), 0x00, sizeof(gMibPrivateTellionOntStatisticsDefRow));

    memset(&gMibPrivateTellionOntStatisticsOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibPrivateTellionOntStatisticsOper.meOperDrvCfg = PrivateTellionOntStatistics_CfgMe;
    gMibPrivateTellionOntStatisticsOper.meOperDump = omci_mib_oper_dump_default_handler;

    MIB_TABLE_PRIVATE_TELLION_ONT_STATISTICS_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibPrivateTellionOntStatisticsTableInfo, &gMibPrivateTellionOntStatisticsOper);

    return GOS_OK;
}
