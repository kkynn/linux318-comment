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
 * Purpose : Definition of ME handler: Multicast operations profile (309)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Multicast operations profile (309)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#endif

MIB_TABLE_INFO_T gMibExtMcastOperProfTableInfo;
MIB_ATTR_INFO_T  gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ATTR_NUM];
MIB_TABLE_EXTMCASTOPERPROF_T gMibExtMcastOperProfDefRow;
MIB_TABLE_OPER_T gMibExtMcastOperProfOper;


static UINT32 IsAllRowPartInExtAclEntry(struct extAclHead *pExtTmpHead, UINT16 key)
{
	extAclTableEntry_t *ptr = NULL;
	UINT32 result[3] = {FALSE, FALSE, FALSE};

	LIST_FOREACH(ptr, pExtTmpHead, entries)
	{
		if(ptr->tableEntry.tableCtrl.bit.rowKey == key)
		{
			result[ptr->tableEntry.tableCtrl.bit.rowPartId] = TRUE;
		}
	}
	if(result[ROW_PART_0] && result[ROW_PART_1] && result[ROW_PART_2])
		return TRUE;
	return FALSE;
}

static GOS_ERROR_CODE ExtAclTableEntryOper(UINT32 aclType, UINT16 portId, UINT8 *pNewTable,
	MIB_TABLE_EXTMCASTOPERPROF_T *pMibExtMcastOperProf, struct extAclHead *pExtTmpHead)
{
	extAclTableEntry_t *ptr = NULL, entry, *pNew = NULL;
	omci_ext_acl_raw_entry_t *pRawEntry = NULL;
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() staring ............%d", __FUNCTION__, __LINE__);

	pRawEntry = &entry.tableEntry;
	memcpy(pRawEntry, pNewTable, sizeof(omci_ext_acl_raw_entry_t));
	pRawEntry->tableCtrl.val =  omci_adjust_tbl_ctrl_by_omcc_ver(GOS_Htons(pRawEntry->tableCtrl.val));
	switch(pRawEntry->tableCtrl.bit.rowPartId)
	{
		case ROW_PART_0:
			pRawEntry->row.format0.gemId = GOS_Htons(pRawEntry->row.format0.gemId);
			pRawEntry->row.format0.aniVid = GOS_Htons(pRawEntry->row.format0.aniVid);
			pRawEntry->row.format0.ImputedGrpBw = GOS_Htonl(pRawEntry->row.format0.ImputedGrpBw);
			pRawEntry->row.format0.previewLen = GOS_Htons(pRawEntry->row.format0.previewLen);
			pRawEntry->row.format0.previewRepeatTime = GOS_Htons(pRawEntry->row.format0.previewRepeatTime);
			pRawEntry->row.format0.previewRepeatCnt = GOS_Htons(pRawEntry->row.format0.previewRepeatCnt);
			pRawEntry->row.format0.previewResetTime = GOS_Htons(pRawEntry->row.format0.previewResetTime);
			pRawEntry->row.format0.vendorSpecificUse = GOS_Htonl(pRawEntry->row.format0.vendorSpecificUse);
			break;
		case ROW_PART_1:
			//for x86 big endian so, should be covert to little endian
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!GOS_HtonByte(pRawEntry->row.format1.dstIpAddrStartRange, 16)), GOS_FAIL);
			break;
		case ROW_PART_2:
			// for x86 big endian so, should be covert to little endian
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!GOS_HtonByte(pRawEntry->row.format2.dstIpAddrEndRange, 16)), GOS_FAIL);
			break;
		case 3:
			break;
		default:
			break;
	}

	switch(pRawEntry->tableCtrl.bit.setCtrl)
	{
		case SET_CTRL_WRITE:
			LIST_FOREACH(ptr, pExtTmpHead, entries)
			{
				if(pRawEntry->tableCtrl.bit.rowPartId == ptr->tableEntry.tableCtrl.bit.rowPartId &&
	  	 			pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					/* overwrite it */
					switch(ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						case ROW_PART_0:
							memcpy(&(ptr->tableEntry.row.format0), &(pRawEntry->row.format0), sizeof(format0_t));
							break;
						case ROW_PART_1:
							memcpy(&(ptr->tableEntry.row.format1), &(pRawEntry->row.format1), sizeof(format1_t));
							break;
						case ROW_PART_2:
							memcpy(&(ptr->tableEntry.row.format2), &(pRawEntry->row.format2), sizeof(format2_t));
							break;
						case 3:
							memcpy(&(ptr->tableEntry.row.format3), &(pRawEntry->row.format3), sizeof(format3_t));
							break;
						default:
							break;
					}
					if(IsAllRowPartInExtAclEntry(pExtTmpHead, ptr->tableEntry.tableCtrl.bit.rowKey))
					{
						MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                            SET_CTRL_WRITE, IP_TYPE_UNKNOWN, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
					}
					return GOS_OK;
				}
			}
			pNew = (extAclTableEntry_t *)malloc(sizeof(extAclTableEntry_t));
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pNew), GOS_FAIL);
			memcpy(pNew, &entry, sizeof(extAclTableEntry_t));
			/*not found, create new entry*/
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add Mcast acl table entry");
			LIST_INSERT_HEAD(pExtTmpHead, pNew, entries);
			if(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX == aclType)
				pMibExtMcastOperProf->curDaclCnt++;
			else if (MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
				pMibExtMcastOperProf->curSaclCnt++;

			if(IsAllRowPartInExtAclEntry(pExtTmpHead, pNew->tableEntry.tableCtrl.bit.rowKey))
			{
				MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                    SET_CTRL_WRITE, IP_TYPE_UNKNOWN, aclType, portId, pNew->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
			}
			break;
		case SET_CTRL_DELETE:
			LIST_FOREACH(ptr, pExtTmpHead, entries)
			{
				if(pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					/*delete all row parts with the same key */
					OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete Mcast dynamic acl entry");
					if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                            SET_CTRL_DELETE, IP_TYPE_UNKNOWN, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
					}
					LIST_REMOVE(ptr, entries);
					free(ptr);
					if(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX == aclType)
						pMibExtMcastOperProf->curDaclCnt--;
					else if(MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
						pMibExtMcastOperProf->curSaclCnt--;
					return GOS_OK;
				}
			}
			break;
		case SET_CTRL_DELETE_ALL:
			ptr = LIST_FIRST(pExtTmpHead);
			while (NULL != ptr)
			{
				if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
				{
					MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                        SET_CTRL_DELETE, IP_TYPE_UNKNOWN, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
				}
		        LIST_REMOVE(ptr, entries);
		        free(ptr);
				ptr = LIST_FIRST(pExtTmpHead);
		    }
			LIST_INIT(pExtTmpHead);
			if(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX == aclType)
				pMibExtMcastOperProf->curDaclCnt = 0;
			else if(MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
				pMibExtMcastOperProf->curSaclCnt = 0;

			break;
		case SET_CTRL_RSV:
		default:
			break;
	}
	return GOS_OK;
}

static GOS_ERROR_CODE ExtMcastTableEntryClear(UINT16 portId, MIB_TABLE_EXTMCASTOPERPROF_T *pMibExtMcastOperProf)
{
	extAclTableEntry_t *ptr = NULL;

	ptr = LIST_FIRST(&pMibExtMcastOperProf->DACLhead);
	while (NULL != ptr)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
            MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_UNKNOWN, MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibExtMcastOperProf->DACLhead);
	}
	LIST_INIT(&pMibExtMcastOperProf->DACLhead);
	pMibExtMcastOperProf->curDaclCnt = 0;

	ptr = NULL;

	ptr = LIST_FIRST(&pMibExtMcastOperProf->SACLhead);
	while (NULL != ptr)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_UNKNOWN, MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pMibExtMcastOperProf);
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibExtMcastOperProf->SACLhead);
	}
	LIST_INIT(&pMibExtMcastOperProf->SACLhead);
	pMibExtMcastOperProf->curSaclCnt = 0;

	//TODO LOST table
	return GOS_OK;
}

static void ExtMcastAclTableDump(UINT32 aclType, MIB_TABLE_EXTMCASTOPERPROF_T *pExtMcastOperProf,
	struct extAclHead *pExtTmpHead)
{
	extAclTableEntry_t *pEntry = NULL;
	omci_ext_acl_raw_entry_t *pRowEntry = NULL;
	UINT16 count=0;
	UINT32 val = 0;
	UINT8 *ptr = NULL;

	switch (aclType)
	{
		case MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX:
			OMCI_PRINT("\nDynamicAccessControlListTable (%u)", pExtMcastOperProf->curDaclCnt);
			break;
		case MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX:
			OMCI_PRINT("\nStaticAccessControlListTable (%u)", pExtMcastOperProf->curSaclCnt);
			break;
		default:
			OMCI_PRINT("Not support aclType");
	}
	OMCI_PRINT("========================================");
	LIST_FOREACH(pEntry, pExtTmpHead, entries){
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("---------------------------------------");
		OMCI_PRINT("INDEX: %u", count);
		OMCI_PRINT("Row Part Id: %u 	   Row Key: %u",
		pRowEntry->tableCtrl.bit.rowPartId, pRowEntry->tableCtrl.bit.rowKey);
		switch(pRowEntry->tableCtrl.bit.rowPartId)
		{
			case ROW_PART_0:
				OMCI_PRINT("Gem Port Id: %u 	   ANI VID: %u",
				pRowEntry->row.format0.gemId, pRowEntry->row.format0.aniVid);
				OMCI_PRINT("Imputed group bandwidth:\t\t%u", pRowEntry->row.format0.ImputedGrpBw);
				OMCI_PRINT("Preview length:\t\t\t%u", pRowEntry->row.format0.previewLen);
				OMCI_PRINT("Preview repeat time:\t\t%u", pRowEntry->row.format0.previewRepeatTime);
				OMCI_PRINT("Preview repeat count:\t\t%u", pRowEntry->row.format0.previewRepeatCnt);
				OMCI_PRINT("Preview reset time\t\t%u", pRowEntry->row.format0.previewResetTime);
				OMCI_PRINT("Vendor-specific use\t\t%u", pRowEntry->row.format0.vendorSpecificUse);
				OMCI_PRINT("Reserved: %p", pRowEntry->row.format0.resv);
				break;
			case ROW_PART_1:
				ptr = pRowEntry->row.format1.dstIpAddrStartRange;
				memcpy(&val, ptr, sizeof(UINT32));
				if(val)
				{
					OMCI_PRINT("Destination IP address, start of range: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pRowEntry->row.format1.dstIpAddrStartRange));
				}
				else
				{
					//ipv4
					ptr = pRowEntry->row.format1.dstIpAddrStartRange;
					ptr +=12;
					memcpy(&val, ptr, sizeof(UINT32));
					OMCI_PRINT("Destination IP address, start of range: "IPADDR_PRINT"", IPADDR_PRINT_ARG(val));
				}
				OMCI_PRINT("Reserved %p:", pRowEntry->row.format1.resv);
				break;
			case ROW_PART_2:
				ptr = pRowEntry->row.format2.dstIpAddrEndRange;
				memcpy(&val, ptr, sizeof(UINT32));
				if(val)
				{

					OMCI_PRINT("Destination IP address, end of range: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pRowEntry->row.format2.dstIpAddrEndRange));
				}
				else
				{
					ptr = pRowEntry->row.format2.dstIpAddrEndRange;
					ptr +=12;
					memcpy(&val, ptr, sizeof(UINT32));
					OMCI_PRINT("Destination IP address, end of range: "IPADDR_PRINT"", IPADDR_PRINT_ARG(val));
				}
				OMCI_PRINT("Reserved %p:", pRowEntry->row.format2.resv);
				break;
			case 3:
				OMCI_PRINT("Reserved %p:", pRowEntry->row.format3.resv);
				break;
			default:
				break;
		}
		count++;
	}
	if(0 == count)
		OMCI_PRINT("No entry");
	OMCI_PRINT("========================================\n");
	return;
}

static void ExtMcastLostGroupTableDump(MIB_TABLE_EXTMCASTOPERPROF_T *pExtMcastOperProf)
{
	UINT8 i = 0;
	extLostGrpTableEntry_t *pEntry = NULL;
	omci_ext_lost_raw_entry_t *pRowEntry = NULL;
	UINT16 count=0;

	OMCI_PRINT("LostGroupsListTable");
	OMCI_PRINT("========================================");
	LIST_FOREACH(pEntry, &pExtMcastOperProf->LOSThead, entries){
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("---------------------------------------");
		OMCI_PRINT("INDEX: %u", count);
		printf("destination IP address: ");
		for(i = 0; i < MIB_TABLE_EXTQUERIERIPADDR_LEN; i++)
		{
			printf("0x%02x ", pRowEntry->destIp[i]);
		}
		count++;
	}
	if(0 == count)
		OMCI_PRINT("No entry");
	OMCI_PRINT("========================================\n");
	return;
}

GOS_ERROR_CODE ExtMcastOperProfDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	UINT8 *ptr = NULL;
	UINT32 val;
	MIB_TABLE_EXTMCASTOPERPROF_T *pExtMcastOperProf = (MIB_TABLE_EXTMCASTOPERPROF_T*)pData;
	struct extAclHead *pExtTmpHead;

	OMCI_PRINT("%s", "ExtMcastOperProf");

	OMCI_PRINT("EntityId: 0x%02x", pExtMcastOperProf->EntityId);
	OMCI_PRINT("IGMPVersion: %u", pExtMcastOperProf->IGMPVersion);
	OMCI_PRINT("IGMPFun: %u", pExtMcastOperProf->IGMPFun);
	OMCI_PRINT("ImmediateLeave: %u", pExtMcastOperProf->ImmediateLeave);
	OMCI_PRINT("UsIGMPTci: 0x%02x", pExtMcastOperProf->UsIGMPTci);
	OMCI_PRINT("UsIGMPTagControl: %u", pExtMcastOperProf->UsIGMPTagControl);
	OMCI_PRINT("UsIGMPRate: %u", pExtMcastOperProf->UsIGMPRate);

	pExtTmpHead = &pExtMcastOperProf->DACLhead;
	ExtMcastAclTableDump(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX, pExtMcastOperProf, pExtTmpHead);
	pExtTmpHead = &pExtMcastOperProf->SACLhead;
	ExtMcastAclTableDump(MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX, pExtMcastOperProf, pExtTmpHead);
	ExtMcastLostGroupTableDump(pExtMcastOperProf);

	OMCI_PRINT("Robustness: %d", pExtMcastOperProf->Robustness);

	ptr = pExtMcastOperProf->QuerierIpAddress;
	memcpy(&val, ptr, sizeof(UINT32));
	if(val)
	{
		//ipv6
		OMCI_PRINT("QuerierIpAddress: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pExtMcastOperProf->QuerierIpAddress));
	}
	else
	{
		//ipv4
		ptr = pExtMcastOperProf->QuerierIpAddress;
		ptr +=12;
		memcpy(&val, ptr, sizeof(UINT32));
		OMCI_PRINT("QuerierIpAddress: "IPADDR_PRINT"", IPADDR_PRINT_ARG(val));
	}
	OMCI_PRINT("QueryInterval: %u", pExtMcastOperProf->QueryInterval);
	OMCI_PRINT("QueryMaxResponseTime: %u", pExtMcastOperProf->QueryMaxResponseTime);
	OMCI_PRINT("LastMemberQueryInterval: %u", pExtMcastOperProf->LastMemberQueryInterval);
	OMCI_PRINT("UnauthorizedJoinRequestBehaviour: %u", pExtMcastOperProf->UnauthorizedJoinRequestBehaviour);
	OMCI_PRINT("DownstreamIgmpMulticastTci: %02x %02x %02x",
		pExtMcastOperProf->DownstreamIgmpMulticastTci[0],
		pExtMcastOperProf->DownstreamIgmpMulticastTci[1],
		pExtMcastOperProf->DownstreamIgmpMulticastTci[2]);

	return GOS_OK;
}

GOS_ERROR_CODE IsExtMcastOperProfRelated(MIB_TABLE_MCASTSUBCONFINFO_T *pMcastConfInfo, UINT16 mopId)
{
	mopTableEntry_t *pEntry = NULL;
	if(!pMcastConfInfo)
		return FALSE;
	LIST_FOREACH(pEntry, &pMcastConfInfo->MOPhead, entries)
	{
		if(pEntry->tableEntry.mcastOperProfPtr == mopId)
			return TRUE;
	}
	return FALSE;
}

GOS_ERROR_CODE ExtMcastOperProfDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_EXTMCASTOPERPROF_T *pExtMop = NULL, *pMibExtMcastOperProf = NULL, oldExtMop;
	MIB_TABLE_MCASTSUBCONFINFO_T mcastConfInfo;
	MIB_TABLE_MACBRIPORTCFGDATA_T bridgePort, *pMibBridgePort = NULL;
	MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
	MIB_TABLE_VEIP_T mibVeip;
	UINT32 totalNum = 0, count = 0;
	UINT16 portId = 0xffff;
	UINT8 existB = FALSE;

	struct extAclHead *pExtTmpHead;

	if(MIB_GET == operationType) return GOS_OK;

	pExtMop = (MIB_TABLE_EXTMCASTOPERPROF_T *) pNewRow;
	oldExtMop.EntityId = pExtMop->EntityId;

	OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pMibExtMcastOperProf)), GOS_FAIL);

	totalNum = MIB_GetTableCurEntryCount(MIB_TABLE_MCASTSUBCONFINFO_INDEX);
	if (totalNum)
	{
		if(GOS_OK != MIB_GetFirst(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &mcastConfInfo, sizeof(MIB_TABLE_MCASTSUBCONFINFO_T)))
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"exist McastOper. but no McastSubCfgInfo");
		}
		else
		{
			while(count < totalNum)
			{
				count++;
				if(IsExtMcastOperProfRelated(&mcastConfInfo, pExtMop->EntityId))
				{
					existB = TRUE;
					bridgePort.EntityID = mcastConfInfo.EntityId;
					OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &bridgePort, &pMibBridgePort)), GOS_FAIL);

					mibPptpEthUNI.EntityID = pMibBridgePort->TPPointer;
					if(GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
					{
						OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get EthUni Fail");
						mibVeip.EntityId = pMibBridgePort->TPPointer;
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(MIB_TABLE_VEIP_T))), GOS_FAIL);
						portId = gInfo.devCapabilities.ponPort;
					}
					else
					{
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMibBridgePort->TPPointer, &portId)), GOS_FAIL);
					}
					/* MIB_ADD do nothing, Only handler MIB_SET/MIB_DEL with port inforamtion */
					switch(operationType)
					{
						case MIB_SET:
							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX))
								MCAST_WRAPPER(omci_igmp_function_set, pExtMop->EntityId, portId, pExtMop->IGMPFun);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX))
								MCAST_WRAPPER(omci_igmp_version_set, pExtMop->EntityId, portId, pExtMop->IGMPVersion);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX))
								MCAST_WRAPPER(omci_immediate_leave_set, pExtMop->EntityId, portId, pExtMop->ImmediateLeave);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX))
								MCAST_WRAPPER(omci_us_igmp_rate_set, pExtMop->EntityId, portId, pExtMop->UsIGMPRate);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX))
							{
								pExtTmpHead = &pMibExtMcastOperProf->DACLhead;
								ExtAclTableEntryOper(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX, portId, pExtMop->DyACLTable, pMibExtMcastOperProf, pExtTmpHead);
								/* update classify rule for multicast streaming */
 								MIB_TREE_T *pTree = MIB_AvlTreeSearchByKey(NULL, bridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI);
								if(pTree)
									MIB_TreeConnUpdate(pTree);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX))
							{
								pExtTmpHead = &pMibExtMcastOperProf->SACLhead;
								ExtAclTableEntryOper(MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX, portId, pExtMop->StaticACLTable, pMibExtMcastOperProf, pExtTmpHead);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX))
								MCAST_WRAPPER(omci_robustness_set, pExtMop->EntityId, pExtMop->Robustness);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX))
								MCAST_WRAPPER(omci_querier_ip_addr_set, pExtMop->EntityId, (void *)(pExtMop->QuerierIpAddress), MIB_TABLE_EXTQUERIERIPADDR_LEN);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX))
								MCAST_WRAPPER(omci_query_interval_set, pExtMop->EntityId, pExtMop->QueryInterval);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX))
								MCAST_WRAPPER(omci_query_max_response_time_set, pExtMop->EntityId, pExtMop->QueryMaxResponseTime);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX))
								MCAST_WRAPPER(omci_last_member_query_interval_set, pExtMop->EntityId, pExtMop->LastMemberQueryInterval);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX))
							{
								MCAST_WRAPPER(omci_unauthorized_join_behaviour_set, pExtMop->EntityId, pExtMop->UnauthorizedJoinRequestBehaviour);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX))
							{
								MCAST_WRAPPER(omci_ds_igmp_multicast_tci_set, pExtMop->EntityId, pExtMop->DownstreamIgmpMulticastTci);
								/* update classify rule for multicast streaming. Need exist DACL rule or SACL rule entry */
								if(pExtMop->curDaclCnt != 0 || pExtMop->curSaclCnt !=0 )
								{
 									MIB_TREE_T *pTree = MIB_AvlTreeSearchByKey(NULL, bridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI);
									if(pTree)
										MIB_TreeConnUpdate(pTree);
								}
							}
							break;
						case MIB_DEL:
							ExtMcastTableEntryClear(portId, pMibExtMcastOperProf);
							omci_mcast_port_reset(portId, &mcastConfInfo, pMibExtMcastOperProf->EntityId);
							MCAST_WRAPPER(omci_mop_profile_del, pExtMop->EntityId);
							break;
						default:
							break;
					}
				}
				if((MIB_GetNext(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &mcastConfInfo, sizeof(MIB_TABLE_MCASTSUBCONFINFO_T)))!=GOS_OK)
				{
					OMCI_LOG(OMCI_LOG_LEVEL_DBG,"get next fail,count=%d",count);
					break;
				}
			}
		}
	}
	if(!existB)
	{
		/* MIB_ADD do nothing, Only handler ACL rule without port inforamtion */
		/* The configuration with port infomartion need set by McastSubCfgInfo ex: igmp us rate... */
		switch(operationType)
		{
			case MIB_ADD:
			case MIB_SET:
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX))
					MCAST_WRAPPER(omci_robustness_set, pExtMop->EntityId, pExtMop->Robustness);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX))
					MCAST_WRAPPER(omci_querier_ip_addr_set, pExtMop->EntityId, (void *)(pExtMop->QuerierIpAddress), MIB_TABLE_EXTQUERIERIPADDR_LEN);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX))
					MCAST_WRAPPER(omci_query_interval_set, pExtMop->EntityId, pExtMop->QueryInterval);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX))
					MCAST_WRAPPER(omci_query_max_response_time_set, pExtMop->EntityId, pExtMop->QueryMaxResponseTime);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX))
					MCAST_WRAPPER(omci_ds_igmp_multicast_tci_set, pExtMop->EntityId, pExtMop->DownstreamIgmpMulticastTci);

 				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX))
					MCAST_WRAPPER(omci_unauthorized_join_behaviour_set, pExtMop->EntityId, pExtMop->UnauthorizedJoinRequestBehaviour);

 				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX))
					MCAST_WRAPPER(omci_last_member_query_interval_set, pExtMop->EntityId, pExtMop->LastMemberQueryInterval);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX))
				{
					pExtTmpHead = &pMibExtMcastOperProf->DACLhead;
					ExtAclTableEntryOper(MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX, portId, pExtMop->DyACLTable, pMibExtMcastOperProf, pExtTmpHead);
				}
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX))
				{
					pExtTmpHead = &pMibExtMcastOperProf->SACLhead;
					ExtAclTableEntryOper(MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX, portId, pExtMop->StaticACLTable, pMibExtMcastOperProf, pExtTmpHead);
				}
				break;
			case MIB_DEL:
				ExtMcastTableEntryClear(portId, pMibExtMcastOperProf);
				omci_mcast_port_reset(portId, NULL, pMibExtMcastOperProf->EntityId);
				MCAST_WRAPPER(omci_mop_profile_del, pExtMop->EntityId);
				break;
			default:
				break;
		}
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);

	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibExtMcastOperProfTableInfo.Name = "ExtendedMcastOperProf";
    gMibExtMcastOperProfTableInfo.ShortName = "EXTMOP";
    gMibExtMcastOperProfTableInfo.Desc = "Extended Multicast Operation Profiles";
    gMibExtMcastOperProfTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_CTC_EXTENDED_MOP);
    gMibExtMcastOperProfTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibExtMcastOperProfTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibExtMcastOperProfTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibExtMcastOperProfTableInfo.pAttributes = &(gMibExtMcastOperProfAttrInfo[0]);

	gMibExtMcastOperProfTableInfo.attrNum = MIB_TABLE_EXTMCASTOPERPROF_ATTR_NUM;
	gMibExtMcastOperProfTableInfo.entrySize = sizeof(MIB_TABLE_EXTMCASTOPERPROF_T);
	gMibExtMcastOperProfTableInfo.pDefaultRow = &gMibExtMcastOperProfDefRow;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IGMPVersion";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IGMPFun";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ImmediateLeave";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPTci";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPTagControl";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPRate";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DyACLTable";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "StaticACLTable";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LostGroupListTable";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Robustness";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QuerierIpAddress";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QueryInterval";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QueryMaxResponseTime";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LastMemberQueryInterval";
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnauthorizedJoinRequestBehaviour";
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownstreamIgmpMulticastTci";

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IGMP version";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IGMP function";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Support Immediate Leave";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP TCI";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP Tag Control";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP rate";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dynamic Access Control List table";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Static Access control list table";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Lost group list table";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Robustness";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Querier IP Address";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Query interval";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Query max response time";
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Last member query interval";
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Unauthorized join request behaviour";
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream IGMP and Multicast TCI";

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 30;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 30;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 16;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 16;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 3;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_OPTIONAL | OMCI_ME_ATTR_TYPE_TABLE);
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibExtMcastOperProfAttrInfo[MIB_TABLE_EXTMCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibExtMcastOperProfDefRow.EntityId), 0x00, sizeof(gMibExtMcastOperProfDefRow.EntityId));
    gMibExtMcastOperProfDefRow.IGMPVersion = 2;
    memset(&(gMibExtMcastOperProfDefRow.IGMPFun), 0x00, sizeof(gMibExtMcastOperProfDefRow.IGMPFun));
    memset(&(gMibExtMcastOperProfDefRow.ImmediateLeave), 0x00, sizeof(gMibExtMcastOperProfDefRow.ImmediateLeave));
    memset(&(gMibExtMcastOperProfDefRow.UsIGMPTci), 0x00, sizeof(gMibExtMcastOperProfDefRow.UsIGMPTci));
    memset(&(gMibExtMcastOperProfDefRow.UsIGMPTagControl), 0x00, sizeof(gMibExtMcastOperProfDefRow.UsIGMPTagControl));
    memset(&(gMibExtMcastOperProfDefRow.UsIGMPRate), 0x00, sizeof(gMibExtMcastOperProfDefRow.UsIGMPRate));
    memset(gMibExtMcastOperProfDefRow.DyACLTable, 0, MIB_TABLE_EXTDYACLTABLE_LEN);
    memset(gMibExtMcastOperProfDefRow.StaticACLTable, 0, MIB_TABLE_EXTSTATICACLTABLE_LEN);
    memset(gMibExtMcastOperProfDefRow.LostGroupListTable, 0, MIB_TABLE_EXTLOSTGROUPLISTTABLE_LEN);
    memset(&(gMibExtMcastOperProfDefRow.Robustness), 0x00, sizeof(gMibExtMcastOperProfDefRow.Robustness));
    memset(&(gMibExtMcastOperProfDefRow.QuerierIpAddress), 0x00, sizeof(gMibExtMcastOperProfDefRow.QuerierIpAddress));
	gMibExtMcastOperProfDefRow.QueryInterval = 125;
	gMibExtMcastOperProfDefRow.QueryMaxResponseTime = 100;
    gMibExtMcastOperProfDefRow.LastMemberQueryInterval = 10;
    memset(&gMibExtMcastOperProfDefRow.UnauthorizedJoinRequestBehaviour, 0x00, sizeof(gMibExtMcastOperProfDefRow.UnauthorizedJoinRequestBehaviour));
    memset(gMibExtMcastOperProfDefRow.DownstreamIgmpMulticastTci, 0x00, MIB_TABLE_EXTDSIGMPANDMULTICASTTCI_LEN);

    /*add for table type attribute*/
    gMibExtMcastOperProfDefRow.curDaclCnt = 0;
    gMibExtMcastOperProfDefRow.curSaclCnt = 0;

    LIST_INIT(&gMibExtMcastOperProfDefRow.DACLhead);
    LIST_INIT(&gMibExtMcastOperProfDefRow.SACLhead);
    LIST_INIT(&gMibExtMcastOperProfDefRow.LOSThead);

    memset(&gMibExtMcastOperProfOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibExtMcastOperProfOper.meOperDrvCfg = ExtMcastOperProfDrvCfg;
    gMibExtMcastOperProfOper.meOperConnCheck = NULL;
    gMibExtMcastOperProfOper.meOperDump = ExtMcastOperProfDumpMib;
	gMibExtMcastOperProfOper.meOperConnCfg = NULL;
	MIB_TABLE_EXTMCASTOPERPROF_INDEX = tableId;

    MIB_InfoRegister(tableId, &gMibExtMcastOperProfTableInfo, &gMibExtMcastOperProfOper);

    return GOS_OK;
}
