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
 * Purpose : Definition of ME handler: Multicast subscriber monitor (311)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Multicast subscriber monitor (311)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibMcastSubMonitorTableInfo;
MIB_ATTR_INFO_T  gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ATTR_NUM];
MIB_TABLE_MCASTSUBMONITOR_T gMibMcastSubMonitorDefRow;
MIB_TABLE_OPER_T gMibMcastSubMonitorOper;

extern omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];

static void msm_ipv4_active_group_list_table_dump(MIB_TABLE_MCASTSUBMONITOR_T *pMSM)
{
    msm_attr_ipv4_active_group_list_table_entry_t *pEntry;
    msm_attr_ipv4_active_group_list_table_t *pEntryData;
    unsigned int count = 0;

    OMCI_PRINT("IPv4ActiveGroupListTable");
    LIST_FOREACH(pEntry, &pMSM->IPv4ActiveGroupListTable_head, entries)
    {
        pEntryData = &pEntry->tableEntry;

        OMCI_PRINT("INDEX %d", count);
        OMCI_PRINT("VLAN ID                                 : %u", pEntryData->VlanID);
        OMCI_PRINT("Source IP address                       : %#x", pEntryData->SrcIpAddr);
        OMCI_PRINT("Multicast desination IP address         : %#x", pEntryData->McastDstIpAddr);
        OMCI_PRINT("Best effort actual bandwidth estimate   : %u", pEntryData->BeActualBandwidthEst);
        OMCI_PRINT("Clinet IP address                       : %#x", pEntryData->ClientIpAddr);
        OMCI_PRINT("Elapse time since last join             : %u", pEntryData->ElapseTime);

        count ++;
    }
}

static void msm_ipv6_active_group_list_table_dump(MIB_TABLE_MCASTSUBMONITOR_T *pMSM)
{
    msm_attr_ipv6_active_group_list_table_entry_t *pEntry;
    msm_attr_ipv6_active_group_list_table_t *pEntryData;
    unsigned int count = 0;

    OMCI_PRINT("IPv6ActiveGroupListTable");
    LIST_FOREACH(pEntry, &pMSM->IPv6ActiveGroupListTable_head, entries)
    {
        pEntryData = &pEntry->tableEntry;

        OMCI_PRINT("INDEX %d", count);
        OMCI_PRINT("VLAN ID                                 : %u", pEntryData->VlanID);
        OMCI_PRINT("Source IP address                       : " \
            "0x%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            pEntryData->SrcIpAddr[0], pEntryData->SrcIpAddr[1], pEntryData->SrcIpAddr[2],
            pEntryData->SrcIpAddr[3], pEntryData->SrcIpAddr[4], pEntryData->SrcIpAddr[5],
            pEntryData->SrcIpAddr[6], pEntryData->SrcIpAddr[7], pEntryData->SrcIpAddr[8],
            pEntryData->SrcIpAddr[9], pEntryData->SrcIpAddr[10], pEntryData->SrcIpAddr[11],
            pEntryData->SrcIpAddr[12], pEntryData->SrcIpAddr[13], pEntryData->SrcIpAddr[14],
            pEntryData->SrcIpAddr[15]);
        OMCI_PRINT("Multicast desination IP address         : " \
            "0x%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            pEntryData->McastDstIpAddr[0], pEntryData->McastDstIpAddr[1], pEntryData->McastDstIpAddr[2],
            pEntryData->McastDstIpAddr[3], pEntryData->McastDstIpAddr[4], pEntryData->McastDstIpAddr[5],
            pEntryData->McastDstIpAddr[6], pEntryData->McastDstIpAddr[7], pEntryData->McastDstIpAddr[8],
            pEntryData->McastDstIpAddr[9], pEntryData->McastDstIpAddr[10], pEntryData->McastDstIpAddr[11],
            pEntryData->McastDstIpAddr[12], pEntryData->McastDstIpAddr[13], pEntryData->McastDstIpAddr[14],
            pEntryData->McastDstIpAddr[15]);
        OMCI_PRINT("Best effort actual bandwidth estimate   : %u", pEntryData->BeActualBandwidthEst);
        OMCI_PRINT("Clinet IP address                       : " \
            "0x%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            pEntryData->ClientIpAddr[0], pEntryData->ClientIpAddr[1], pEntryData->ClientIpAddr[2],
            pEntryData->ClientIpAddr[3], pEntryData->ClientIpAddr[4], pEntryData->ClientIpAddr[5],
            pEntryData->ClientIpAddr[6], pEntryData->ClientIpAddr[7], pEntryData->ClientIpAddr[8],
            pEntryData->ClientIpAddr[9], pEntryData->ClientIpAddr[10], pEntryData->ClientIpAddr[11],
            pEntryData->ClientIpAddr[12], pEntryData->ClientIpAddr[13], pEntryData->ClientIpAddr[14],
            pEntryData->ClientIpAddr[15]);
        OMCI_PRINT("Elapse time since last join             : %u", pEntryData->ElapseTime);

        count ++;
    }
}

GOS_ERROR_CODE McastSubMonitorDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_MCASTSUBMONITOR_T *pMcastSubMonitor = (MIB_TABLE_MCASTSUBMONITOR_T*)pData;

	OMCI_PRINT("EntityId: 0x%02x", pMcastSubMonitor->EntityId);
	OMCI_PRINT("MeType: %d", pMcastSubMonitor->MeType);
	OMCI_PRINT("CurrMcastBandwidth: %d", pMcastSubMonitor->CurrMcastBandwidth);
	OMCI_PRINT("JoinMsgCounter: %d", pMcastSubMonitor->JoinMsgCounter);
	OMCI_PRINT("BandwidthExcCounter: %d", pMcastSubMonitor->BandwidthExcCounter);
    msm_ipv4_active_group_list_table_dump(pMcastSubMonitor);
    msm_ipv6_active_group_list_table_dump(pMcastSubMonitor);

	return GOS_OK;
}

static GOS_ERROR_CODE msm_ipv4_active_group_list_table_entry_clear(MIB_TABLE_MCASTSUBMONITOR_T *pMSM)
{
    msm_attr_ipv4_active_group_list_table_entry_t *pEntry;

    pEntry = LIST_FIRST(&pMSM->IPv4ActiveGroupListTable_head);
    while (pEntry)
    {
        LIST_REMOVE(pEntry, entries);
        free(pEntry);

        pEntry = LIST_FIRST(&pMSM->IPv4ActiveGroupListTable_head);
    }

    return GOS_OK;
}

static GOS_ERROR_CODE msm_ipv6_active_group_list_table_entry_clear(MIB_TABLE_MCASTSUBMONITOR_T *pMSM)
{
    msm_attr_ipv6_active_group_list_table_entry_t *pEntry;

    pEntry = LIST_FIRST(&pMSM->IPv6ActiveGroupListTable_head);
    while (pEntry)
    {
        LIST_REMOVE(pEntry, entries);
        free(pEntry);

        pEntry = LIST_FIRST(&pMSM->IPv6ActiveGroupListTable_head);
    }

    return GOS_OK;
}

GOS_ERROR_CODE McastSubMonitorDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_MACBRIPORTCFGDATA_T mibMBCD;
    MIB_TABLE_MCASTSUBMONITOR_T *pMSM, *pFoundMib;
	MIB_TABLE_ETHUNI_T mibEthUNI;
	MIB_TABLE_VEIP_T mibVeip;
	MIB_ATTR_INDEX attrIndex;
	UINT16 portId = 0xffff;

    pMSM = (MIB_TABLE_MCASTSUBMONITOR_T *)pNewRow;

    if (!mib_FindEntry(MIB_TABLE_MCASTSUBMONITOR_INDEX, pMSM, &pFoundMib))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "cannot find Multicast subscriber monitor 0x%x", pMSM->EntityId);
        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "find Multicast subscriber monitor at %p", pFoundMib);

    switch (operationType)
    {
        case MIB_ADD:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Multicast subscriber monitor --> ADD");

            LIST_INIT(&pFoundMib->IPv4ActiveGroupListTable_head);
            LIST_INIT(&pFoundMib->IPv6ActiveGroupListTable_head);

            break;

        case MIB_DEL:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Multicast subscriber monitor --> DEL");

            msm_ipv4_active_group_list_table_entry_clear(pFoundMib);
            LIST_INIT(&pFoundMib->IPv4ActiveGroupListTable_head);
            msm_ipv6_active_group_list_table_entry_clear(pFoundMib);
            LIST_INIT(&pFoundMib->IPv6ActiveGroupListTable_head);

            break;

    	case MIB_GET:
		{
			OMCI_PRINT("Multicast subscriber monitor --> GET");

			switch(pMSM->MeType)
			{
				case MSM_ME_TYPE_MBPCD:
					mibMBCD.EntityID = pMSM->EntityId;
					OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T))), GOS_FAIL);

					mibEthUNI.EntityID = mibMBCD.TPPointer;
					if(GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
					{
						OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get EthUni Fail");
						mibVeip.EntityId = mibMBCD.TPPointer;
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(MIB_TABLE_VEIP_T))), GOS_FAIL);
						portId = gInfo.devCapabilities.ponPort;
					}
					else
					{
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != pptp_eth_uni_me_id_to_switch_port(mibMBCD.TPPointer, &portId)), GOS_FAIL);
					}
					break;
				case MSM_ME_TYPE_IEEE_8021P_MAPPER:
					break;
				default:
					OMCI_PRINT("%s() %d MeType %u not support ", __FUNCTION__, __LINE__, pMSM->MeType);
			}
			//for single get
			attrIndex = MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX;
			if (MIB_IsInAttrSet(&attrSet, attrIndex))
			{
				//ipc and shared memory with igmp in SFU
				MCAST_WRAPPER(omci_current_multicast_bandwidth_get, portId, &(pMSM->CurrMcastBandwidth));
			}
			//for single get
			attrIndex = MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX;
			if (MIB_IsInAttrSet(&attrSet, attrIndex))
			{
				//ipc and shared memory with igmp in SFU
				MCAST_WRAPPER(omci_join_message_counter_get, portId, &(pMSM->JoinMsgCounter));
			}
			//for single get
			attrIndex = MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX;
			if (MIB_IsInAttrSet(&attrSet, attrIndex))
			{
				//ipc and shared memory with igmp in SFU
				MCAST_WRAPPER(omci_bandwidth_exceeded_counter_get, portId, &(pMSM->BandwidthExcCounter));
			}
			//for multi get
			attrIndex = MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX;
			if (MIB_IsInAttrSet(&attrSet, attrIndex))
			{
				msm_attr_ipv4_active_group_list_table_entry_t *pEntry = NULL;
				UINT8 *ptr = NULL;
				UINT32 curIpv4ActiveGrpCnt = 0;
				msm_ipv4_active_group_list_table_entry_clear(pMSM);
				MCAST_WRAPPER(omci_active_group_list_table_get, attrIndex, portId, pMSM, &curIpv4ActiveGrpCnt);

				gOmciMulGetData[pri].attribute[attrIndex].attrSize = MIB_TABLE_MSM_IPV4_ACTIVEGROUPLISTTABLE_LEN * curIpv4ActiveGrpCnt;
				gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
				gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
				gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
					(gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

				ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

				LIST_FOREACH(pEntry, &pMSM->IPv4ActiveGroupListTable_head, entries)
				{
					memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_MSM_IPV4_ACTIVEGROUPLISTTABLE_LEN);
					ptr += MIB_TABLE_MSM_IPV4_ACTIVEGROUPLISTTABLE_LEN;
				}
			}
			#if 0 //TBD
			//for multi get
			attrIndex = MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX;
			if (MIB_IsInAttrSet(&attrSet, attrIndex))
			{
				msm_attr_ipv6_active_group_list_table_entry_t *pEntry = NULL;
				UINT8 *ptr = NULL;
				UINT32 curIpv6ActiveGrpCnt = 0;
				msm_ipv6_active_group_list_table_entry_clear(pMSM);
				WAPPER_MCAST_ACTIVEGROUP_GET(attrIndex, portId, pMSM, &curIpv6ActiveGrpCnt);

				gOmciMulGetData[pri].attribute[attrIndex].attrSize = MIB_TABLE_MSM_IPV6_ACTIVEGROUPLISTTABLE_LEN * curIpv6ActiveGrpCnt;
				gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
				gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
				gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
					(gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

				ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

				LIST_FOREACH(pEntry, &pMSM->IPv6ActiveGroupListTable_head, entries)
				{
					memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_MSM_IPV6_ACTIVEGROUPLISTTABLE_LEN);
					ptr += MIB_TABLE_MSM_IPV6_ACTIVEGROUPLISTTABLE_LEN;
				}
			}
			#endif
		}
        default:
            return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMcastSubMonitorTableInfo.Name = "McastSubMonitor";
    gMibMcastSubMonitorTableInfo.ShortName = "MSM";
    gMibMcastSubMonitorTableInfo.Desc = "Multicast subscriber monitor";
    gMibMcastSubMonitorTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MCAST_SUBSCRIBER_MONITOR);
    gMibMcastSubMonitorTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMcastSubMonitorTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMcastSubMonitorTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibMcastSubMonitorTableInfo.pAttributes = &(gMibMcastSubMonitorAttrInfo[0]);

	gMibMcastSubMonitorTableInfo.attrNum = MIB_TABLE_MCASTSUBMONITOR_ATTR_NUM;
	gMibMcastSubMonitorTableInfo.entrySize = sizeof(MIB_TABLE_MCASTSUBMONITOR_T);
	gMibMcastSubMonitorTableInfo.pDefaultRow = &gMibMcastSubMonitorDefRow;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MeType";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrMcastBandwidth";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "JoinMsgCounter";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BandwidthExcCounter";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IPv4ActiveGroupListTable";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IPv6ActiveGroupListTable";

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ME Type";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current multicast bandwidth";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Join message counter";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bandwidth excessed counter";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IPv4 Active group list table";
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IPv6 Active group list table";

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 58;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_CURRMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_JOINMSGCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_BANDWIDTHEXCCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV4_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibMcastSubMonitorAttrInfo[MIB_TABLE_MCASTSUBMONITOR_IPV6_ACTIVEGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_OPTIONAL | OMCI_ME_ATTR_TYPE_TABLE);

    memset(&(gMibMcastSubMonitorDefRow.EntityId), 0x00, sizeof(gMibMcastSubMonitorDefRow.EntityId));
    memset(&(gMibMcastSubMonitorDefRow.MeType), 0x00, sizeof(gMibMcastSubMonitorDefRow.MeType));
    memset(&(gMibMcastSubMonitorDefRow.CurrMcastBandwidth), 0x00, sizeof(gMibMcastSubMonitorDefRow.CurrMcastBandwidth));
    memset(&(gMibMcastSubMonitorDefRow.JoinMsgCounter), 0x00, sizeof(gMibMcastSubMonitorDefRow.JoinMsgCounter));
    memset(&(gMibMcastSubMonitorDefRow.BandwidthExcCounter), 0x00, sizeof(gMibMcastSubMonitorDefRow.BandwidthExcCounter));
    memset(gMibMcastSubMonitorDefRow.IPv4ActiveGroupListTable, 0, MIB_TABLE_MSM_IPV4_ACTIVEGROUPLISTTABLE_LEN);
    memset(gMibMcastSubMonitorDefRow.IPv6ActiveGroupListTable, 0, MIB_TABLE_MSM_IPV6_ACTIVEGROUPLISTTABLE_LEN);
    LIST_INIT(&gMibMcastSubMonitorDefRow.IPv4ActiveGroupListTable_head);
    LIST_INIT(&gMibMcastSubMonitorDefRow.IPv6ActiveGroupListTable_head);

    memset(&gMibMcastSubMonitorOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibMcastSubMonitorOper.meOperDrvCfg = McastSubMonitorDrvCfg;
    gMibMcastSubMonitorOper.meOperConnCheck = NULL;
    gMibMcastSubMonitorOper.meOperDump = McastSubMonitorDumpMib;
	gMibMcastSubMonitorOper.meOperConnCfg = NULL;
	MIB_TABLE_MCASTSUBMONITOR_INDEX = tableId;

    MIB_InfoRegister(tableId, &gMibMcastSubMonitorTableInfo, &gMibMcastSubMonitorOper);

    return GOS_OK;
}

