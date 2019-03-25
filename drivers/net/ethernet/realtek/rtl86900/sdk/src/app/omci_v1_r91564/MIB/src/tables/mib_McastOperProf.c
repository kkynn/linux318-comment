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
#include "feature_mgmt.h"

#define IS_EQUAL(arr, start, end, val, pRet) \
do{ \
    UINT32 _i;  \
    *pRet = TRUE; \
    for(_i = start; _i < end; _i ++) \
    { \
        if (val != arr[_i]) *pRet = FALSE; \
    } \
}while(0)


#define IS_ZERO(arr, start, end, pRet) \
do{ \
    UINT32 _i;  \
    *pRet = TRUE; \
    for(_i = start; _i < end; _i ++) \
    { \
        if (0 != arr[_i]) *pRet = FALSE; \
    } \
}while(0)

MIB_TABLE_INFO_T gMibMcastOperProfTableInfo;
MIB_ATTR_INFO_T  gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ATTR_NUM];
MIB_TABLE_MCASTOPERPROF_T gMibMcastOperProfDefRow;
MIB_TABLE_OPER_T gMibMcastOperProfOper;

extern omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];

static UINT32 McastAclGetIpType(struct aclHead *pTmpHead, omci_acl_raw_entry_t *pEntry)
{
	aclTableEntry_t *ptr = NULL;
	UINT32 ipType = IP_TYPE_IPV4, result[3] = {FALSE, FALSE, FALSE};
	LIST_FOREACH(ptr, pTmpHead, entries)
	{
		if(ptr->tableEntry.tableCtrl.bit.rowKey == pEntry->tableCtrl.bit.rowKey &&
			ptr->tableEntry.tableCtrl.bit.rowPartId != pEntry->tableCtrl.bit.rowPartId)
		{
			switch(ptr->tableEntry.tableCtrl.bit.rowPartId)
			{
				case ROW_PART_1:
					IS_ZERO(ptr->tableEntry.rowPart.rowPart1.ipv6Sip, 0, 9, &(result[0]));
					IS_EQUAL(ptr->tableEntry.rowPart.rowPart1.ipv6Sip, 10, 11, 0x0, &(result[1]));
					IS_EQUAL(ptr->tableEntry.rowPart.rowPart1.ipv6Sip, 10, 11, 0xFF, &(result[2]));
					if(result[0] &&  (result[1] || result[2]))
						ipType = IP_TYPE_IPV4;
					else
						ipType = IP_TYPE_IPV6;
					break;
				case ROW_PART_2:
					IS_ZERO(ptr->tableEntry.rowPart.rowPart2.ipv6Dip, 0, 9, &(result[0]));
					IS_EQUAL(ptr->tableEntry.rowPart.rowPart2.ipv6Dip, 10, 11, 0x0, &(result[1]));
					IS_EQUAL(ptr->tableEntry.rowPart.rowPart2.ipv6Dip, 10, 11, 0xFF, &(result[2]));
					if(result[0] &&  (result[1] || result[2]))
						ipType = IP_TYPE_IPV4;
					else
						ipType = IP_TYPE_IPV6;
					break;
				default:
					break;
			}
		}
	}
	return ipType;
}

static UINT32 IsAllRowPartInAclEntry(struct aclHead *pTmpHead, UINT16 key)
{
	aclTableEntry_t *ptr = NULL;
	UINT32 result[3] = {FALSE, FALSE, FALSE};
	LIST_FOREACH(ptr, pTmpHead, entries)
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

static GOS_ERROR_CODE AclTableEntryOper(UINT32 aclType, UINT16 portId, UINT8 *pNewTable,
	MIB_TABLE_MCASTOPERPROF_T *pMibMcastOperProf, struct aclHead *pTmpHead)
{
	aclTableEntry_t *ptr = NULL, entry, *pNew = NULL;
	omci_acl_raw_entry_t *pRawEntry = NULL;
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() staring ............%d", __FUNCTION__, __LINE__);
	UINT32 ipType, allRowPartB = FALSE;
	struct aclGemVlanHead *pDAclGemVlanHead;
	aclGemVlanEntry_t *pAclGemVlanEntry;
	BOOL bFound;
	GOS_ERROR_CODE ret = GOS_OK;

	pRawEntry = &entry.tableEntry;
	memcpy(pRawEntry, pNewTable, sizeof(omci_acl_raw_entry_t));
	pRawEntry->tableCtrl.val = omci_adjust_tbl_ctrl_by_omcc_ver(GOS_Htons(pRawEntry->tableCtrl.val));
	pDAclGemVlanHead = &pMibMcastOperProf->dAclGemVlanHead;

	switch (pRawEntry->tableCtrl.bit.rowPartId)
	{
		case ROW_PART_0:
			pRawEntry->rowPart.rowPart0.gemId			= GOS_Htons(pRawEntry->rowPart.rowPart0.gemId);
			pRawEntry->rowPart.rowPart0.aniVid			= GOS_Htons(pRawEntry->rowPart.rowPart0.aniVid);
			pRawEntry->rowPart.rowPart0.sip				= GOS_Htonl(pRawEntry->rowPart.rowPart0.sip);
			pRawEntry->rowPart.rowPart0.dipStartRange	= GOS_Htonl(pRawEntry->rowPart.rowPart0.dipStartRange);
			pRawEntry->rowPart.rowPart0.dipEndRange		= GOS_Htonl(pRawEntry->rowPart.rowPart0.dipEndRange);
			pRawEntry->rowPart.rowPart0.ImputedGrpBw 	= GOS_Htonl(pRawEntry->rowPart.rowPart0.ImputedGrpBw);
			break;
		case ROW_PART_1:
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!GOS_HtonByte(pRawEntry->rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN)), GOS_FAIL);
			pRawEntry->rowPart.rowPart1.previewLen			= GOS_Htons(pRawEntry->rowPart.rowPart1.previewLen);
			pRawEntry->rowPart.rowPart1.previewRepeatTime	= GOS_Htons(pRawEntry->rowPart.rowPart1.previewRepeatTime);
			pRawEntry->rowPart.rowPart1.previewRepeatCnt	= GOS_Htons(pRawEntry->rowPart.rowPart1.previewRepeatCnt);
			pRawEntry->rowPart.rowPart1.previewResetTime	= GOS_Htons(pRawEntry->rowPart.rowPart1.previewResetTime);
			break;
		case ROW_PART_2:
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!GOS_HtonByte(pRawEntry->rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN)), GOS_FAIL);
			break;
		default:
			OMCI_PRINT("Not support Row Part ID");
	}

	switch(pRawEntry->tableCtrl.bit.setCtrl)
	{
		case SET_CTRL_WRITE:
			LIST_FOREACH(ptr, pTmpHead, entries)
			{
				if(pRawEntry->tableCtrl.bit.rowPartId == ptr->tableEntry.tableCtrl.bit.rowPartId &&
					pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					switch (ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						/* overwrite DACL linked list */
						case ROW_PART_0:
							if (ptr->tableEntry.rowPart.rowPart0.gemId == pRawEntry->rowPart.rowPart0.gemId &&
									ptr->tableEntry.rowPart.rowPart0.aniVid == pRawEntry->rowPart.rowPart0.aniVid)
								ret = GOS_ERR_DISABLE;
							else
							{
								pAclGemVlanEntry = NULL;
								LIST_FOREACH(pAclGemVlanEntry, pDAclGemVlanHead, entries)
								{
									if (ptr->tableEntry.rowPart.rowPart0.gemId == pAclGemVlanEntry->tableEntry.gemId &&
											ptr->tableEntry.rowPart.rowPart0.aniVid == pAclGemVlanEntry->tableEntry.aniVid)
									{
										pAclGemVlanEntry->tableEntry.refCnt--;
										if (0 == pAclGemVlanEntry->tableEntry.refCnt)
										{
											LIST_REMOVE(pAclGemVlanEntry, entries);
											free(pAclGemVlanEntry);
										}
										break;
									}
								}

								bFound = FALSE;
								pAclGemVlanEntry = NULL;
								LIST_FOREACH(pAclGemVlanEntry, pDAclGemVlanHead, entries)
								{
									if (pRawEntry->rowPart.rowPart0.gemId == pAclGemVlanEntry->tableEntry.gemId &&
											pRawEntry->rowPart.rowPart0.aniVid == pAclGemVlanEntry->tableEntry.aniVid)
									{
										pAclGemVlanEntry->tableEntry.refCnt++;
										bFound = TRUE;
										break;
									}
								}
								if (!bFound)
								{
									pAclGemVlanEntry = (aclGemVlanEntry_t *)malloc(sizeof(aclGemVlanEntry_t));
									pAclGemVlanEntry->tableEntry.gemId = pRawEntry->rowPart.rowPart0.gemId;
									pAclGemVlanEntry->tableEntry.aniVid = pRawEntry->rowPart.rowPart0.aniVid;
									pAclGemVlanEntry->tableEntry.refCnt = 1;
									LIST_INSERT_HEAD(pDAclGemVlanHead, pAclGemVlanEntry, entries);
								}
							}

							ptr->tableEntry.rowPart.rowPart0.gemId			= pRawEntry->rowPart.rowPart0.gemId;
							ptr->tableEntry.rowPart.rowPart0.aniVid			= pRawEntry->rowPart.rowPart0.aniVid;
							ptr->tableEntry.rowPart.rowPart0.sip 			= pRawEntry->rowPart.rowPart0.sip;
							ptr->tableEntry.rowPart.rowPart0.dipStartRange	= pRawEntry->rowPart.rowPart0.dipStartRange;
							ptr->tableEntry.rowPart.rowPart0.dipEndRange 	= pRawEntry->rowPart.rowPart0.dipEndRange;
							ptr->tableEntry.rowPart.rowPart0.ImputedGrpBw	= pRawEntry->rowPart.rowPart0.ImputedGrpBw;
							break;
						case ROW_PART_1:
							memcpy(ptr->tableEntry.rowPart.rowPart1.ipv6Sip, pRawEntry->rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
							ptr->tableEntry.rowPart.rowPart1.previewLen			= pRawEntry->rowPart.rowPart1.previewLen;
							ptr->tableEntry.rowPart.rowPart1.previewRepeatTime	= pRawEntry->rowPart.rowPart1.previewRepeatTime;
							ptr->tableEntry.rowPart.rowPart1.previewRepeatCnt	= pRawEntry->rowPart.rowPart1.previewRepeatCnt;
							ptr->tableEntry.rowPart.rowPart1.previewResetTime	= pRawEntry->rowPart.rowPart1.previewResetTime;
							break;
						case ROW_PART_2:
							memcpy(ptr->tableEntry.rowPart.rowPart2.ipv6Dip, pRawEntry->rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN);
							break;
						default:
							OMCI_PRINT("Not support Row Part ID");
					}
					ipType = McastAclGetIpType(pTmpHead, &(ptr->tableEntry));
					if(IP_TYPE_IPV4 == ipType || (TRUE == IsAllRowPartInAclEntry(pTmpHead, ptr->tableEntry.tableCtrl.bit.rowKey)))
					{
						MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                            SET_CTRL_WRITE, ipType, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
					}
					return ret;

				}
			}
			pNew = (aclTableEntry_t *)malloc(sizeof(aclTableEntry_t));
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!pNew), GOS_FAIL);
			memcpy(pNew, &entry, sizeof(aclTableEntry_t));
			/*not found, create new entry*/
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add Mcast acl table entry");
			LIST_INSERT_HEAD(pTmpHead, pNew, entries);
			/* after insert DACL linked list for new row part, del ipv4 acl when part 1 and part 2 are exist  */
			if(TRUE == (allRowPartB = IsAllRowPartInAclEntry(pTmpHead, pNew->tableEntry.tableCtrl.bit.rowKey)))
			{
				MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                    SET_CTRL_DELETE, IP_TYPE_IPV4, aclType, portId, pNew->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
			}
			ipType = McastAclGetIpType(pTmpHead, &(pNew->tableEntry));
			if(IP_TYPE_IPV4 == ipType || (TRUE == allRowPartB))
			{
				MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                    SET_CTRL_WRITE, ipType, aclType, portId, pNew->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
			}
			if(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX == aclType)
				pMibMcastOperProf->curDaclCnt++;
			else if(MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
				pMibMcastOperProf->curSaclCnt++;

			if (ROW_PART_0 == pRawEntry->tableCtrl.bit.rowPartId)
			{
				pAclGemVlanEntry = NULL;
				LIST_FOREACH(pAclGemVlanEntry, pDAclGemVlanHead, entries)
				{
					if (pRawEntry->rowPart.rowPart0.gemId == pAclGemVlanEntry->tableEntry.gemId &&
							pRawEntry->rowPart.rowPart0.aniVid == pAclGemVlanEntry->tableEntry.aniVid)
					{
						pAclGemVlanEntry->tableEntry.refCnt++;
						return GOS_ERR_DISABLE;
					}
				}

				pAclGemVlanEntry = (aclGemVlanEntry_t *)malloc(sizeof(aclGemVlanEntry_t));
				pAclGemVlanEntry->tableEntry.gemId = pRawEntry->rowPart.rowPart0.gemId;
				pAclGemVlanEntry->tableEntry.aniVid = pRawEntry->rowPart.rowPart0.aniVid;
				pAclGemVlanEntry->tableEntry.refCnt = 1;
				LIST_INSERT_HEAD(pDAclGemVlanHead, pAclGemVlanEntry, entries);
			}

			break;
		case SET_CTRL_DELETE:
			LIST_FOREACH(ptr, pTmpHead, entries)
			{
				if(pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					bFound = FALSE;
					if (ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						pAclGemVlanEntry = NULL;
						LIST_FOREACH(pAclGemVlanEntry, pDAclGemVlanHead, entries)
						{
							if (ptr->tableEntry.rowPart.rowPart0.gemId == pAclGemVlanEntry->tableEntry.gemId &&
									ptr->tableEntry.rowPart.rowPart0.aniVid == pAclGemVlanEntry->tableEntry.aniVid)
							{
								pAclGemVlanEntry->tableEntry.refCnt--;
								if (0 == pAclGemVlanEntry->tableEntry.refCnt)
								{
									LIST_REMOVE(pAclGemVlanEntry, entries);
									free(pAclGemVlanEntry);
								}
								bFound = TRUE;
								break;
							}
						}
						if (!bFound)
							ret = GOS_ERR_DISABLE;
					}

					/*delete all row parts with the same key */
					OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete Mcast dynamic acl entry");

					if (ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                            SET_CTRL_DELETE, IP_TYPE_IPV4, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
					}
					else if (ROW_PART_2 ==  ptr->tableEntry.tableCtrl.bit.rowPartId)
					{
						MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                            SET_CTRL_DELETE, IP_TYPE_IPV6, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
					}
					LIST_REMOVE(ptr, entries);
					free(ptr);
					if(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX == aclType)
						pMibMcastOperProf->curDaclCnt--;
					else if (MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
						pMibMcastOperProf->curSaclCnt--;
                    break;
				}
			}
			break;
		case SET_CTRL_DELETE_ALL:
			ptr = LIST_FIRST(pTmpHead);
			while (NULL != ptr)
			{
				if(ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
				{
					MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                        SET_CTRL_DELETE, IP_TYPE_IPV4, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
				}
				else if (ROW_PART_2 ==  ptr->tableEntry.tableCtrl.bit.rowPartId)
				{
					MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                        SET_CTRL_DELETE, IP_TYPE_IPV6, aclType, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
				}
		        LIST_REMOVE(ptr, entries);
		        free(ptr);
				ptr = LIST_FIRST(pTmpHead);
		    }
			LIST_INIT(pTmpHead);
			if(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX == aclType)
				pMibMcastOperProf->curDaclCnt = 0;
			else if(MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
				pMibMcastOperProf->curSaclCnt = 0;

			pAclGemVlanEntry = LIST_FIRST(&pMibMcastOperProf->dAclGemVlanHead);
			while (NULL != pAclGemVlanEntry)
			{
				LIST_REMOVE(pAclGemVlanEntry, entries);
				free(pAclGemVlanEntry);
				pAclGemVlanEntry = LIST_FIRST(&pMibMcastOperProf->dAclGemVlanHead);
			}
			LIST_INIT(&pMibMcastOperProf->dAclGemVlanHead);

			break;
		case SET_CTRL_RSV:
		default:
			break;
	}
	return ret;
}

static GOS_ERROR_CODE McastTableEntryClear(UINT16 portId, MIB_TABLE_MCASTOPERPROF_T *pMibMcastOperProf)
{
	aclTableEntry_t *ptr = NULL;
	aclGemVlanEntry_t *pAclGemVlanEntry;

	ptr = LIST_FIRST(&pMibMcastOperProf->DACLhead);
	while (NULL != ptr)
	{
		if (ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_IPV4, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX,
                portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
		}
		else if (ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_IPV6, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX,
                portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibMcastOperProf->DACLhead);
	}
	LIST_INIT(&pMibMcastOperProf->DACLhead);
	pMibMcastOperProf->curDaclCnt = 0;

	pAclGemVlanEntry = LIST_FIRST(&pMibMcastOperProf->dAclGemVlanHead);
	while (NULL != pAclGemVlanEntry)
	{
		LIST_REMOVE(pAclGemVlanEntry, entries);
		free(pAclGemVlanEntry);
		pAclGemVlanEntry = LIST_FIRST(&pMibMcastOperProf->dAclGemVlanHead);
	}
	LIST_INIT(&pMibMcastOperProf->dAclGemVlanHead);

	ptr = LIST_FIRST(&pMibMcastOperProf->SACLhead);
	while (NULL != ptr)
	{
		if (ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_IPV4, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
		}
		else if (ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			MCAST_WRAPPER(omci_dynamic_acl_table_entry_set,
                SET_CTRL_DELETE, IP_TYPE_IPV6, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastOperProf, NULL);
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibMcastOperProf->SACLhead);
	}
	LIST_INIT(&pMibMcastOperProf->SACLhead);
	pMibMcastOperProf->curSaclCnt = 0;

	//TODO LOST table
	return GOS_OK;
}

static UINT32 GetLastFourBytes(struct aclHead *pTmpHead, UINT8 type, UINT16 key)
{
	aclTableEntry_t *pEntry = NULL;
	omci_acl_raw_entry_t *pRowEntry = NULL;
	UINT32 ret = 0;
	LIST_FOREACH(pEntry, pTmpHead, entries)
	{
		pRowEntry = &pEntry->tableEntry;
		if(pRowEntry->tableCtrl.bit.rowKey == key &&
			ROW_PART_0 == pRowEntry->tableCtrl.bit.rowPartId)
		{
			switch(type)
			{
				case 0: /* sip */
					ret = pRowEntry->rowPart.rowPart0.sip;
					break;
				case 1:
					ret = pRowEntry->rowPart.rowPart0.dipStartRange;
					break;
				case 2:
					ret =pRowEntry->rowPart.rowPart0.dipEndRange;
					break;
				default:
					OMCI_PRINT("Not suuport ");
			}
		}
	}
	return ret;
}

static void McastAclTableDump(UINT32 aclType, MIB_TABLE_MCASTOPERPROF_T *pMcastOperProf,
	struct aclHead *pTmpHead)
{
	aclTableEntry_t *pEntry = NULL;
	omci_acl_raw_entry_t *pRowEntry = NULL;
	UINT16 count = 0;
	UINT32 fourBytes;
	UINT8 *pAddr = (UINT8 *)malloc(sizeof(UINT8) * (LEADING_12BYTE_ADDR_LEN + sizeof(UINT32)));
	if(!pAddr)
	{
		OMCI_PRINT("malloc failed");
		return;
	}

	if(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX == aclType)
	{
		OMCI_PRINT("\nDynamicAccessControlListTable (%u)", pMcastOperProf->curDaclCnt);
	}
	else if (MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX == aclType)
	{
		OMCI_PRINT("\nStaticAccessControlListTable (%u)", pMcastOperProf->curSaclCnt);
	}

	OMCI_PRINT("========================================");
	LIST_FOREACH(pEntry, pTmpHead, entries)
	{
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("---------------------------------------");
		OMCI_PRINT("INDEX: %u", count);
		OMCI_PRINT("Row Part Id: %u \t\t\t Row Key: %u",
			pRowEntry->tableCtrl.bit.rowPartId, pRowEntry->tableCtrl.bit.rowKey);
		switch(pRowEntry->tableCtrl.bit.rowPartId)
		{
			case ROW_PART_0:
				OMCI_PRINT("Gem Port Id: %u \t\t\t ANI VID: %u",
					pRowEntry->rowPart.rowPart0.gemId, pRowEntry->rowPart.rowPart0.aniVid);
				OMCI_PRINT("Source IP Address: \t\t "IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntry->rowPart.rowPart0.sip));
				OMCI_PRINT("DIP Start Range: \t\t "IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntry->rowPart.rowPart0.dipStartRange));
				OMCI_PRINT("DIP End Range: \t\t\t "IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntry->rowPart.rowPart0.dipEndRange));
				OMCI_PRINT("Imputed Group BW: \t\t %u", pRowEntry->rowPart.rowPart0.ImputedGrpBw);
				break;
			case ROW_PART_1:
				memset(pAddr, 0, LEADING_12BYTE_ADDR_LEN + sizeof(UINT32));
				memcpy(pAddr, pRowEntry->rowPart.rowPart1.ipv6Sip, LEADING_12BYTE_ADDR_LEN);
				fourBytes = GetLastFourBytes(pTmpHead, 0, pRowEntry->tableCtrl.bit.rowKey);
				memcpy(pAddr + LEADING_12BYTE_ADDR_LEN, &(fourBytes), sizeof(UINT32));
				OMCI_PRINT("IPV6 Source IP Address: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pAddr));
				OMCI_PRINT("Preview length: \t\t\t %u", pRowEntry->rowPart.rowPart1.previewLen);
				OMCI_PRINT("Preview repeat time: \t\t %u", pRowEntry->rowPart.rowPart1.previewRepeatTime);
				OMCI_PRINT("Preview repeat count: \t\t %u", pRowEntry->rowPart.rowPart1.previewRepeatCnt);
				OMCI_PRINT("Preview reset time: \t\t %u", pRowEntry->rowPart.rowPart1.previewResetTime);
				break;
			case ROW_PART_2:
				memset(pAddr, 0, LEADING_12BYTE_ADDR_LEN + sizeof(UINT32));
				memcpy(pAddr, pRowEntry->rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN);
				fourBytes = GetLastFourBytes(pTmpHead, 1, pRowEntry->tableCtrl.bit.rowKey);
				memcpy(pAddr + LEADING_12BYTE_ADDR_LEN, &(fourBytes), sizeof(UINT32));
				OMCI_PRINT("IPV6 Destination IP Address Start Range: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pAddr));
				memset(pAddr, 0, LEADING_12BYTE_ADDR_LEN + sizeof(UINT32));
				memcpy(pAddr, pRowEntry->rowPart.rowPart2.ipv6Dip, LEADING_12BYTE_ADDR_LEN);
				fourBytes = GetLastFourBytes(pTmpHead, 2, pRowEntry->tableCtrl.bit.rowKey);
				memcpy(pAddr + LEADING_12BYTE_ADDR_LEN, &(fourBytes), sizeof(UINT32));
				OMCI_PRINT("IPV6 Destination IP Address End Range: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pAddr));
				break;
			default:
				OMCI_PRINT("Not support ROW_PART_ID: %u", pRowEntry->tableCtrl.bit.rowPartId);
		}
		count++;
	}
	if(0 == count)
		OMCI_PRINT("No entry");
	OMCI_PRINT("========================================\n");
	free(pAddr);
	return;
}

static void McastLostGroupTableDump(MIB_TABLE_MCASTOPERPROF_T *pMcastOperProf)
{
	lostGrpTableEntry_t *pEntry = NULL;
	omci_lost_raw_entry_t *pRowEntry = NULL;
	UINT16 count=0;

	OMCI_PRINT("LostGroupsListTable");
	OMCI_PRINT("========================================");
	LIST_FOREACH(pEntry, &pMcastOperProf->LOSThead, entries){
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("---------------------------------------");
		OMCI_PRINT("INDEX: %u", count);
		OMCI_PRINT("VID: 						%u", pRowEntry->vid);
		OMCI_PRINT("Source IP Address: 			"IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntry->sip));
		OMCI_PRINT("Destination IP Address: 	"IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntry->mcastDip));
		count++;
	}
	if(0 == count)
		OMCI_PRINT("No entry");
	OMCI_PRINT("========================================\n");
	return;
}

GOS_ERROR_CODE McastOperProfDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_MCASTOPERPROF_T *pMcastOperProf = (MIB_TABLE_MCASTOPERPROF_T*)pData;
	struct aclHead *pTmpHead;

	OMCI_PRINT("EntityId: 0x%02x", pMcastOperProf->EntityId);
	OMCI_PRINT("IGMPVersion: %u", pMcastOperProf->IGMPVersion);
	OMCI_PRINT("IGMPFun: %u", pMcastOperProf->IGMPFun);
	OMCI_PRINT("ImmediateLeave: %u", pMcastOperProf->ImmediateLeave);
	OMCI_PRINT("UsIGMPTci: 0x%02x", pMcastOperProf->UsIGMPTci);
	OMCI_PRINT("UsIGMPTagControl: %u", pMcastOperProf->UsIGMPTagControl);
	OMCI_PRINT("UsIGMPRate: %u", pMcastOperProf->UsIGMPRate);

	pTmpHead = &pMcastOperProf->DACLhead;
	McastAclTableDump(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX, pMcastOperProf, pTmpHead);
	pTmpHead = &pMcastOperProf->SACLhead;
	McastAclTableDump(MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX, pMcastOperProf, pTmpHead);
	McastLostGroupTableDump(pMcastOperProf);

	OMCI_PRINT("Robustness: %d", pMcastOperProf->Robustness);
	OMCI_PRINT("QuerierIpAddress: "IPADDR_PRINT"", IPADDR_PRINT_ARG(pMcastOperProf->QuerierIpAddress));
	OMCI_PRINT("QueryInterval: %u", pMcastOperProf->QueryInterval);
	OMCI_PRINT("QueryMaxResponseTime: %u", pMcastOperProf->QueryMaxResponseTime);
	OMCI_PRINT("LastMemberQueryInterval: %u", pMcastOperProf->LastMemberQueryInterval);
	OMCI_PRINT("UnauthorizedJoinRequestBehaviour: %u", pMcastOperProf->UnauthorizedJoinRequestBehaviour);
	OMCI_PRINT("DownstreamIgmpMulticastTci: %02x %02x %02x\n\n\n",
		pMcastOperProf->DownstreamIgmpMulticastTci[0],
		pMcastOperProf->DownstreamIgmpMulticastTci[1],
		pMcastOperProf->DownstreamIgmpMulticastTci[2]);

	return GOS_OK;
}

GOS_ERROR_CODE IsMcastOperProfRelated(MIB_TABLE_MCASTSUBCONFINFO_T *pMcastConfInfo, UINT16 mopId)
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

GOS_ERROR_CODE
MibAttrTypeTableMibGet (
    MIB_TABLE_MCASTOPERPROF_T* pMop,
    MIB_ATTRS_SET attrSet,
    UINT32        pri)
{
    MIB_ATTR_INDEX attrIndex;
    aclTableEntry_t* pEntry = NULL;
    UINT8* ptr = NULL;

    if (!pMop) {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: Mop Pointer is NULL\n", __FUNCTION__);
        return GOS_FAIL;
    }

    if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX)) {
        attrIndex = MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX;
        gOmciMulGetData[pri].attribute[attrIndex].attrSize =
            MIB_TABLE_DYACLTABLE_LEN * (pMop->curDaclCnt);
        gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
        gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
        gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
            (gOmciMulGetData[pri].attribute[attrIndex].attrSize +
                 OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
            OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

        ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

        LIST_FOREACH(pEntry, &pMop->DACLhead, entries) {
            memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_DYACLTABLE_LEN);
            ptr += MIB_TABLE_DYACLTABLE_LEN;
        }
        OMCI_LOG (
            OMCI_LOG_LEVEL_DBG,
            "Mib_Get: Dacl Len %ud\n",
            gOmciMulGetData[pri].attribute[attrIndex].attrSize);
    } else if (MIB_IsInAttrSet(
                   &attrSet,
                   MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX)) {

        attrIndex = MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX;
        gOmciMulGetData[pri].attribute[attrIndex].attrSize =
            MIB_TABLE_STATICACLTABLE_LEN * (pMop->curSaclCnt);
        gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
        gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
        gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
            (gOmciMulGetData[pri].attribute[attrIndex].attrSize +
                 OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
            OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

        ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

        LIST_FOREACH(pEntry, &pMop->SACLhead, entries) {
            memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_STATICACLTABLE_LEN);
            ptr += MIB_TABLE_STATICACLTABLE_LEN;
        }
    } else if (MIB_IsInAttrSet(
                   &attrSet,
                   MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX)) {
    }

    return GOS_OK;
}

GOS_ERROR_CODE McastOperProfDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_MCASTOPERPROF_T *pMop = NULL, *pOldMop = NULL, *pMibMcastOperProf = NULL, oldMop;
	MIB_TABLE_MCASTSUBCONFINFO_T mcastConfInfo;
	MIB_TABLE_MACBRIPORTCFGDATA_T bridgePort, *pMibBridgePort = NULL;
	MIB_TABLE_VEIP_T mibVeip;
	UINT32 totalNum = 0, count = 0;
	UINT16 portId = 0xffff;
	UINT8 existB = FALSE;
	MIB_TREE_T *pTree = NULL;
	struct aclHead *pTmpHead;
	GOS_ERROR_CODE ret = GOS_OK;

	pOldMop = (MIB_TABLE_MCASTOPERPROF_T *) pOldRow;
	pMop = (MIB_TABLE_MCASTOPERPROF_T *) pNewRow;
	oldMop.EntityId = pMop->EntityId;

	OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pMibMcastOperProf)), GOS_FAIL);
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
				if(IsMcastOperProfRelated(&mcastConfInfo, pMop->EntityId))
				{
					existB = TRUE;
					bridgePort.EntityID = mcastConfInfo.EntityId;

					OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &bridgePort, &pMibBridgePort)), GOS_FAIL);

					if(GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMibBridgePort->TPPointer, &portId))
					{
						OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't mapping to physical port by bridge port 0x%x", pMibBridgePort->TPPointer);
						mibVeip.EntityId = pMibBridgePort->TPPointer;
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(MIB_TABLE_VEIP_T))), GOS_FAIL);
						portId = gInfo.devCapabilities.ponPort;
					}
					/* MIB_ADD do nothing, Only handler MIB_SET/MIB_DEL with port inforamtion */
					switch(operationType)
					{
						case MIB_SET:
							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX))
								MCAST_WRAPPER(omci_igmp_function_set, pMop->EntityId, portId, pMop->IGMPFun);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX))
								MCAST_WRAPPER(omci_igmp_version_set, pMop->EntityId, portId, pMop->IGMPVersion);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX))
								MCAST_WRAPPER(omci_immediate_leave_set, pMop->EntityId, portId, pMop->ImmediateLeave);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX))
								MCAST_WRAPPER(omci_us_igmp_rate_set, pMop->EntityId, portId, pMop->UsIGMPRate);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX))
							{
								pTmpHead = &pMibMcastOperProf->DACLhead;
								ret = AclTableEntryOper(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX, portId, pMop->DyACLTable, pMibMcastOperProf, pTmpHead);
								/* update classify rule for multicast streaming */
								if(GOS_ERR_DISABLE != ret && NULL != (pTree = MIB_AvlTreeSearchByKey(NULL, bridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI))) {
                                    MIB_TreeConnUpdate(pTree);
                                }
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX))
							{
								pTmpHead = &pMibMcastOperProf->SACLhead;
								ret = AclTableEntryOper(MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX, portId, pMop->StaticACLTable, pMibMcastOperProf, pTmpHead);
								if(GOS_ERR_DISABLE != ret && NULL != (pTree = MIB_AvlTreeSearchByKey(NULL, bridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI)))
									MIB_TreeConnUpdate(pTree);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX))
								MCAST_WRAPPER(omci_robustness_set, pMop->EntityId, pMop->Robustness);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX))
								MCAST_WRAPPER(omci_querier_ip_addr_set, pMop->EntityId, (void *)(&(pMop->QuerierIpAddress)), 4);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX))
								MCAST_WRAPPER(omci_query_interval_set, pMop->EntityId, pMop->QueryInterval);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX))
								MCAST_WRAPPER(omci_query_max_response_time_set, pMop->EntityId, pMop->QueryMaxResponseTime);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX))
								MCAST_WRAPPER(omci_last_member_query_interval_set, pMop->EntityId, pMop->LastMemberQueryInterval);

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX))
							{
								MCAST_WRAPPER(omci_unauthorized_join_behaviour_set, pMop->EntityId, pMop->UnauthorizedJoinRequestBehaviour);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX) ||
								MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX))
							{
								//compare new and old value
								if(pOldMop->UsIGMPTagControl != pMop->UsIGMPTagControl || pOldMop->UsIGMPTci != pMop->UsIGMPTci)
									MCAST_WRAPPER(omci_us_igmp_tag_info_set, pMop);
							}

							if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX))
							{
								feature_api(FEATURE_API_MC_00000001, pMop->DownstreamIgmpMulticastTci);
								if(memcmp(pOldMop->DownstreamIgmpMulticastTci,
									pMop->DownstreamIgmpMulticastTci, MIB_TABLE_DSIGMPANDMULTICASTTCI_LEN)) {
									MCAST_WRAPPER(omci_ds_igmp_multicast_tci_set, pMop->EntityId, pMop->DownstreamIgmpMulticastTci);
								}
								/* update classify rule for multicast streaming. Need exist DACL rule or SACL rule entry */
								if(0 != pMop->DownstreamIgmpMulticastTci[0])
								{
									if(NULL != (pTree = MIB_AvlTreeSearchByKey(NULL, bridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI)))
										MIB_TreeConnUpdate(pTree);
								}
							}
							break;
						case MIB_DEL:
							McastTableEntryClear(portId, pMibMcastOperProf);
							omci_mcast_port_reset(portId, &mcastConfInfo, pMibMcastOperProf->EntityId);
							MCAST_WRAPPER(omci_mop_profile_del, pMop->EntityId);
							break;
                        case MIB_GET:
                            OMCI_LOG(
                                OMCI_LOG_LEVEL_DBG,
                                "McastOperProf --> GET table attribute");
                            MibAttrTypeTableMibGet(
                                pOldMop,
                                attrSet,
                                pri);
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
                if(operationType == MIB_ADD)
                {
                    MCAST_WRAPPER(omci_mop_profile_add, pMop->EntityId);
                    // force apply attributes that is not toggle as set-by-create
                    MCAST_WRAPPER(omci_unauthorized_join_behaviour_set, pMop->EntityId, pMop->UnauthorizedJoinRequestBehaviour);
                    MCAST_WRAPPER(omci_last_member_query_interval_set, pMop->EntityId, pMop->LastMemberQueryInterval);
                }

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX))
					MCAST_WRAPPER(omci_robustness_set, pMop->EntityId, pMop->Robustness);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX))
					MCAST_WRAPPER(omci_querier_ip_addr_set, pMop->EntityId, (void *)(&(pMop->QuerierIpAddress)), 4);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX))
					MCAST_WRAPPER(omci_query_interval_set, pMop->EntityId, pMop->QueryInterval);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX))
					MCAST_WRAPPER(omci_query_max_response_time_set, pMop->EntityId, pMop->QueryMaxResponseTime);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX))
				{
					feature_api(FEATURE_API_MC_00000001, pMop->DownstreamIgmpMulticastTci);
					if(MIB_ADD == operationType || (!memcmp(pOldMop->DownstreamIgmpMulticastTci,
						pMop->DownstreamIgmpMulticastTci, MIB_TABLE_DSIGMPANDMULTICASTTCI_LEN)))
						MCAST_WRAPPER(omci_ds_igmp_multicast_tci_set, pMop->EntityId, pMop->DownstreamIgmpMulticastTci);
				}
 				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX))
					MCAST_WRAPPER(omci_unauthorized_join_behaviour_set, pMop->EntityId, pMop->UnauthorizedJoinRequestBehaviour);

 				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX))
					MCAST_WRAPPER(omci_last_member_query_interval_set, pMop->EntityId, pMop->LastMemberQueryInterval);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX))
				{
					pTmpHead = &pMibMcastOperProf->DACLhead;
					AclTableEntryOper(MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX, portId, pMop->DyACLTable, pMibMcastOperProf, pTmpHead);
				}
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX))
				{
					pTmpHead = &pMibMcastOperProf->SACLhead;
					AclTableEntryOper(MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX, portId, pMop->StaticACLTable, pMibMcastOperProf, pTmpHead);
				}

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX) ||
					MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX))
				{
					//compare new and old value
					if(MIB_ADD == operationType || (pOldMop->UsIGMPTagControl != pMop->UsIGMPTagControl ||
						pOldMop->UsIGMPTci != pMop->UsIGMPTci))
						MCAST_WRAPPER(omci_us_igmp_tag_info_set, pMop);
				}
				break;
			case MIB_DEL:
				McastTableEntryClear(portId, pMibMcastOperProf);
				//mcast_port_reset(portId, NULL, pMibMcastOperProf->EntityId);
				MCAST_WRAPPER(omci_mop_profile_del, pMop->EntityId);
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
    gMibMcastOperProfTableInfo.Name = "McastOperProf";
    gMibMcastOperProfTableInfo.ShortName = "MOP";
    gMibMcastOperProfTableInfo.Desc = "Multicast Operation Profile";
    gMibMcastOperProfTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MCAST_OPERATIONS_PROFILE);
    gMibMcastOperProfTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMcastOperProfTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMcastOperProfTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibMcastOperProfTableInfo.pAttributes = &(gMibMcastOperProfAttrInfo[0]);

	gMibMcastOperProfTableInfo.attrNum = MIB_TABLE_MCASTOPERPROF_ATTR_NUM;
	gMibMcastOperProfTableInfo.entrySize = sizeof(MIB_TABLE_MCASTOPERPROF_T);
	gMibMcastOperProfTableInfo.pDefaultRow = &gMibMcastOperProfDefRow;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IGMPVersion";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IGMPFun";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ImmediateLeave";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPTci";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPTagControl";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsIGMPRate";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DyACLTable";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "StaticACLTable";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LostGroupListTable";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Robustness";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QuerierIpAddress";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QueryInterval";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QueryMaxResponseTime";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LastMemberQueryInterval";
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnauthorizedJoinRequestBehaviour";
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DownstreamIgmpMulticastTci";

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IGMP version";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IGMP function";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Support Immediate Leave";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP TCI";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP Tag Control";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream IGMP rate";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dynamic Access Control List table";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Static Access control list table";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Lost group list table";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Robustness";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Querier IP Address";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Query interval";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Query max response time";
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Last member query interval";
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Unauthorized join request behaviour";
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream IGMP and Multicast TCI";

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 10;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 3;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IGMPFUN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_IMMEDIATELEAVE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTCI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPTAGCONTROL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_USIGMPRATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LOSTGROUPLISTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT | OMCI_ME_ATTR_TYPE_TABLE);
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_ROBUSTNESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERIERIPADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_QUERYMAXRESPONSETIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_LASTMEMBERQUERYINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_UNAUTHORIZEDJOINREQUESTBEHAVIOUR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastOperProfAttrInfo[MIB_TABLE_MCASTOPERPROF_DOWNSTREAMIGMPMULTICASTTCI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibMcastOperProfDefRow.EntityId), 0x00, sizeof(gMibMcastOperProfDefRow.EntityId));
    gMibMcastOperProfDefRow.IGMPVersion = 2;
    memset(&(gMibMcastOperProfDefRow.IGMPFun), 0x00, sizeof(gMibMcastOperProfDefRow.IGMPFun));
    memset(&(gMibMcastOperProfDefRow.ImmediateLeave), 0x00, sizeof(gMibMcastOperProfDefRow.ImmediateLeave));
    memset(&(gMibMcastOperProfDefRow.UsIGMPTci), 0x00, sizeof(gMibMcastOperProfDefRow.UsIGMPTci));
    memset(&(gMibMcastOperProfDefRow.UsIGMPTagControl), 0x00, sizeof(gMibMcastOperProfDefRow.UsIGMPTagControl));
    memset(&(gMibMcastOperProfDefRow.UsIGMPRate), 0x00, sizeof(gMibMcastOperProfDefRow.UsIGMPRate));
    memset(gMibMcastOperProfDefRow.DyACLTable, 0, MIB_TABLE_DYACLTABLE_LEN);
    memset(gMibMcastOperProfDefRow.StaticACLTable, 0, MIB_TABLE_STATICACLTABLE_LEN);
    memset(gMibMcastOperProfDefRow.LostGroupListTable, 0, MIB_TABLE_LOSTGROUPLISTTABLE_LEN);
	gMibMcastOperProfDefRow.Robustness = 0;
    memset(&(gMibMcastOperProfDefRow.QuerierIpAddress), 0x00, sizeof(gMibMcastOperProfDefRow.QuerierIpAddress));
	gMibMcastOperProfDefRow.QueryInterval = 125;
	gMibMcastOperProfDefRow.QueryMaxResponseTime = 100;
    gMibMcastOperProfDefRow.LastMemberQueryInterval = 10;
    memset(&gMibMcastOperProfDefRow.UnauthorizedJoinRequestBehaviour, 0x00, sizeof(gMibMcastOperProfDefRow.UnauthorizedJoinRequestBehaviour));
    feature_api(FEATURE_API_MC_00000002, &gMibMcastOperProfDefRow.UnauthorizedJoinRequestBehaviour);
    memset(gMibMcastOperProfDefRow.DownstreamIgmpMulticastTci, 0x00, MIB_TABLE_DSIGMPANDMULTICASTTCI_LEN);

    /*add for table type attribute*/
    gMibMcastOperProfDefRow.curDaclCnt = 0;
    gMibMcastOperProfDefRow.curSaclCnt = 0;
    LIST_INIT(&gMibMcastOperProfDefRow.DACLhead);
    LIST_INIT(&gMibMcastOperProfDefRow.dAclGemVlanHead);
    LIST_INIT(&gMibMcastOperProfDefRow.SACLhead);
    LIST_INIT(&gMibMcastOperProfDefRow.LOSThead);

    memset(&gMibMcastOperProfOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibMcastOperProfOper.meOperDrvCfg = McastOperProfDrvCfg;
    gMibMcastOperProfOper.meOperConnCheck = NULL;
    gMibMcastOperProfOper.meOperDump = McastOperProfDumpMib;
	gMibMcastOperProfOper.meOperConnCfg = NULL;
	MIB_TABLE_MCASTOPERPROF_INDEX = tableId;

    MIB_InfoRegister(tableId, &gMibMcastOperProfTableInfo, &gMibMcastOperProfOper);

    return GOS_OK;
}
