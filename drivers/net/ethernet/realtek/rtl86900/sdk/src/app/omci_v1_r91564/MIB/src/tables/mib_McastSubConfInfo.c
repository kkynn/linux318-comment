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
 * Purpose : Definition of ME handler: Multicast subscriber config info (310)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Multicast subscriber config info (310)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#endif
#include "feature_mgmt.h"

MIB_TABLE_INFO_T gMibMcastSubConfInfoTableInfo;
MIB_ATTR_INFO_T  gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ATTR_NUM];
MIB_TABLE_MCASTSUBCONFINFO_T gMibMcastSubConfInfoDefRow;
MIB_TABLE_OPER_T gMibMcastSubConfInfoOper;

static UINT32
	IsAllRowPartInAllowPreviewEntry(struct allowPreviewGrpTblEntryHead_t *pTmpHead, UINT16 key)
{
	allowPreviewGrpTblEntry_t *ptr = NULL;
	UINT32 result[ROW_PART_2] = {FALSE, FALSE};
	LIST_FOREACH(ptr, pTmpHead, entries)
	{
		if(ptr->tableEntry.tableCtrl.bit.rowKey == key)
		{
			result[ptr->tableEntry.tableCtrl.bit.rowPart] = TRUE;
		}
	}
	if(result[ROW_PART_0] && result[ROW_PART_1])
		return TRUE;

	return FALSE;
}

static void McastSvcPkgTableDump(MIB_TABLE_MCASTSUBCONFINFO_T *pMcastSubConfInfo)
{
	mopTableEntry_t *pEntry = NULL;
	omci_mcast_svc_pkg_raw_entry_t *pRowEntry = NULL;
	UINT16 count=0, i;

	OMCI_PRINT("McastSvcPkgTable (%u)", pMcastSubConfInfo->curMopCnt);
	LIST_FOREACH(pEntry, &pMcastSubConfInfo->MOPhead, entries)
	{
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("----------------------------------");
		OMCI_PRINT("INDEX:\t\t\t%u", count);
		OMCI_PRINT("Row Key:\t\t%u", pRowEntry->tableCtrl.bit.rowKey);
		OMCI_PRINT("vidUni:\t\t\t%u", pRowEntry->vidUni);
		OMCI_PRINT("maxSimGroups:\t\t%u", pRowEntry->maxSimGroups);
		OMCI_PRINT("maxMcastBw:\t\t%u", pRowEntry->maxMcastBw);
		OMCI_PRINT("mcastOperProfPtr:\t%02x", pRowEntry->mcastOperProfPtr);
		printf("Reserved:");
		for(i = 0; i < 8; i++)
			printf(" %02x", pRowEntry->resv[i]);
		count++;
		printf("\n");
	}
	if(0 == count)
	{
		OMCI_PRINT("----------------------------------");
		OMCI_PRINT("No entry");
	}
	return;
}

static void McastAllowedPreviewTableDump(MIB_TABLE_MCASTSUBCONFINFO_T *pMcastSubConfInfo)
{
	allowPreviewGrpTblEntry_t *pEntry = NULL;
	omci_allowed_preview_group_table_entry_t *pRowEntry = NULL;
	UINT16 count=0;

	OMCI_PRINT("Allowed Preview Groups Table: (%u)", pMcastSubConfInfo->curPreGrpCnt);
	LIST_FOREACH(pEntry, &pMcastSubConfInfo->allowPreviewGrpHead, entries)
	{
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("----------------------------------");
		OMCI_PRINT("INDEX:\t\t\t%u", count);
		OMCI_PRINT("Row Key:\t\t%u", pRowEntry->tableCtrl.bit.rowKey);
		switch(pRowEntry->tableCtrl.bit.rowPart)
		{
			case ROW_PART_0:
				OMCI_PRINT("Source IP Addr:\t"IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pRowEntry->row.part0.srcIpAddr));
				OMCI_PRINT("ANI VID:\t\t%u", pRowEntry->row.part0.vidAni);
				OMCI_PRINT("UNI VID:\t\t%u", pRowEntry->row.part0.vidUni);
				break;
			case ROW_PART_1:
				OMCI_PRINT("Destination IP Addr:\t"IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pRowEntry->row.part1.dstIpAddr));
				OMCI_PRINT("Duration:\t\t%u", pRowEntry->row.part1.duration);
				OMCI_PRINT("Time Left:\t\t%u", pRowEntry->row.part1.timeleft);
				break;
			default:
				OMCI_PRINT("Not support rowPartId =%u", pRowEntry->tableCtrl.bit.rowPart);
		}
		count++;
	}
	if(0 == count)
	{
		OMCI_PRINT("----------------------------------");
		OMCI_PRINT("No entry");
	}
	return;
}

static GOS_ERROR_CODE MopAclEntryOp(UINT32 op, UINT16 portId, MIB_TABLE_MCASTOPERPROF_T *pMop)
{
	aclTableEntry_t *ptr = NULL;

	ptr = LIST_FIRST(&pMop->DACLhead);
	while (NULL != ptr)
	{
		MCAST_WRAPPER(omci_acl_per_port_set,
            op, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX, portId, (pMop->EntityId << 16 | ptr->tableEntry.tableCtrl.bit.rowKey));
		ptr = LIST_NEXT(ptr, entries);
	}

	ptr = LIST_FIRST(&pMop->SACLhead);
	while (NULL != ptr)
	{
		MCAST_WRAPPER(omci_acl_per_port_set,
            op, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX, portId, (pMop->EntityId << 16 | ptr->tableEntry.tableCtrl.bit.rowKey));
		ptr = LIST_NEXT(ptr, entries);
	}

	if(0xffff != portId)
	{
		MCAST_WRAPPER(omci_mop_profile_per_port_set, op, pMop->EntityId, portId);
	}
	return GOS_OK;
}

static GOS_ERROR_CODE ExtMopAclEntryOp(UINT32 op, UINT16 portId, MIB_TABLE_EXTMCASTOPERPROF_T *pExtMop)
{
	extAclTableEntry_t *ptr = NULL;

	ptr = LIST_FIRST(&pExtMop->DACLhead);
	while (NULL != ptr)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			//TBD sync with RG
			MCAST_WRAPPER(omci_acl_per_port_set,
			    op, MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX, portId, (pExtMop->EntityId << 16 | ptr->tableEntry.tableCtrl.bit.rowKey));
		}
		ptr = LIST_NEXT(ptr, entries);
	}

	ptr = LIST_FIRST(&pExtMop->SACLhead);
	while (NULL != ptr)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			//TBD sync with RG
			MCAST_WRAPPER(omci_acl_per_port_set,
			    op, MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX, portId, (pExtMop->EntityId << 16 | ptr->tableEntry.tableCtrl.bit.rowKey));
		}
		ptr = LIST_NEXT(ptr, entries);
	}

	if(0xffff != portId)
	{
		MCAST_WRAPPER(omci_mop_profile_per_port_set, op, pExtMop->EntityId, portId);
	}
	return GOS_OK;
}

static GOS_ERROR_CODE MopEntryOper(MIB_ATTRS_SET attrSet, UINT16 portId, MIB_TABLE_MCASTSUBCONFINFO_T *pSubCfgInfo,
	MIB_TABLE_MCASTSUBCONFINFO_T *pMibMcastSubConfInfo, MIB_TABLE_MCASTOPERPROF_T **pMibMop, MIB_TABLE_EXTMCASTOPERPROF_T **pMibExtMop)
{
	mopTableEntry_t *ptr = NULL, *ptrTemp = NULL,*pNew = NULL, entry;
	MIB_TABLE_MCASTOPERPROF_T *pOldMibMop = NULL, mop, oldMop;
	MIB_TABLE_EXTMCASTOPERPROF_T *pOldMibExtMop = NULL, extMop, oldExtMop;
	//UINT8 *pTmp = NULL;
	omci_mcast_svc_pkg_raw_entry_t *pRawEntry = NULL;
	BOOL existB = FALSE;

	/* if no multicast service package table, there is one entry in pMcastSubConfInfo->MOPhead */
	if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX) ||
		MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX))
	{
		if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX))
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d single mop newid=%x", __FUNCTION__, __LINE__, pSubCfgInfo->McastOperProfPtr);
			//add, set
			mop.EntityId = pSubCfgInfo->McastOperProfPtr;
			if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, pMibMop))
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG, "find	mop 0x%x", mop.EntityId);
				existB = TRUE;
			}
			else
			{
				extMop.EntityId = mop.EntityId;
				if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &extMop, pMibExtMop))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_DBG, "find extMop 0x%x", extMop.EntityId);
					existB = TRUE;
				}
			}
            for(ptr = LIST_FIRST(&pMibMcastSubConfInfo->MOPhead); ptr != NULL;)
			{
				/* ALU OLT set mcastOperProfPtr to zero when change svc package table */
				if(pMibMcastSubConfInfo->McastOperProfPtr == ptr->tableEntry.mcastOperProfPtr ||
					0 == pMibMcastSubConfInfo->McastOperProfPtr)
				{
					/* delete old one */
					if(0xffff != portId)
					{
						oldMop.EntityId = ptr->tableEntry.mcastOperProfPtr;
						if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pOldMibMop))
						{
							MopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibMop);
						}
						else
						{
							oldExtMop.EntityId = oldMop.EntityId;
							if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pOldMibExtMop))
							{
								ExtMopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibExtMop);
							}
						}
					}

                    ptrTemp = ptr;
                    ptr = LIST_NEXT(ptr, entries);
                    LIST_REMOVE(ptrTemp, entries);
                    free(ptrTemp);

					pMibMcastSubConfInfo->curMopCnt--;
				}
			}
			if(existB)
			{
				pNew = (mopTableEntry_t *)malloc(sizeof(mopTableEntry_t));
				OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pNew), GOS_FAIL);
				/* add new one */
				pNew->tableEntry.mcastOperProfPtr = pSubCfgInfo->McastOperProfPtr;
				pNew->tableEntry.maxMcastBw = pMibMcastSubConfInfo->MaxMcastBandwidth;
				pNew->tableEntry.maxSimGroups = pMibMcastSubConfInfo->MaxSimGroups;
				pNew->tableEntry.vidUni = USHRT_MAX;
				pNew->tableEntry.tableCtrl.bit.setCtrl = SET_CTRL_WRITE;
				pNew->tableEntry.tableCtrl.bit.reserved = 0;
				pNew->tableEntry.tableCtrl.bit.rowKey = 0x1;
				LIST_INSERT_HEAD(&pMibMcastSubConfInfo->MOPhead, pNew, entries);
				pMibMcastSubConfInfo->curMopCnt++;
				if(0xffff != portId)
				{
					if(*pMibMop)
						MopAclEntryOp(SET_CTRL_WRITE, portId, *pMibMop);
					else if (*pMibExtMop)
						ExtMopAclEntryOp(SET_CTRL_WRITE, portId, *pMibExtMop);
				}
			}

		}
		else if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX))
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d multiple mop", __FUNCTION__, __LINE__);
			pRawEntry = &entry.tableEntry;
			memcpy(pRawEntry, pSubCfgInfo->McastSvcPkgTbl, sizeof(omci_mcast_svc_pkg_raw_entry_t));

			pRawEntry->tableCtrl.val	= GOS_Htons(pRawEntry->tableCtrl.val);
			pRawEntry->vidUni			= GOS_Htons(pRawEntry->vidUni);
			pRawEntry->maxSimGroups		= GOS_Htons(pRawEntry->maxSimGroups);
			pRawEntry->maxMcastBw		= GOS_Htonl(pRawEntry->maxMcastBw);
			pRawEntry->mcastOperProfPtr = GOS_Htons(pRawEntry->mcastOperProfPtr);

			switch(pRawEntry->tableCtrl.bit.setCtrl)
			{
				case SET_CTRL_WRITE:
					mop.EntityId = pRawEntry->mcastOperProfPtr;
					if(FALSE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, pMibMop))
					{
						OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Not found	mop 0x%x", mop.EntityId);
						extMop.EntityId = mop.EntityId;
						OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &extMop, pMibExtMop)), GOS_FAIL);
					}
					LIST_FOREACH(ptr, &pMibMcastSubConfInfo->MOPhead, entries)
					{
						if(pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
						{
							if(0xffff != portId)
							{
								oldMop.EntityId = ptr->tableEntry.mcastOperProfPtr;
								if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pOldMibMop))
								{
									MopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibMop);
								}
								else
								{
									oldExtMop.EntityId = oldMop.EntityId;
									if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pOldMibExtMop))
									{
										ExtMopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibExtMop);
									}
								}

								/* overwrite it */
								ptr->tableEntry.vidUni = pRawEntry->vidUni;
								ptr->tableEntry.maxSimGroups = pRawEntry->maxSimGroups;
								ptr->tableEntry.maxMcastBw = pRawEntry->maxMcastBw;
								ptr->tableEntry.mcastOperProfPtr = pRawEntry->mcastOperProfPtr;
								memcpy(ptr->tableEntry.resv, pRawEntry->resv, sizeof(UINT8) * 8);
								if(*pMibMop)
									MopAclEntryOp(SET_CTRL_WRITE, portId, *pMibMop);
								else if(*pMibExtMop)
									ExtMopAclEntryOp(SET_CTRL_WRITE, portId, *pMibExtMop);
							}
							return GOS_OK;
						}
					}
					/*not found, create new entry*/

					pNew = (mopTableEntry_t *)malloc(sizeof(mopTableEntry_t));
					OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pNew), GOS_FAIL);
					memcpy(pNew, &entry, sizeof(mopTableEntry_t));
					LIST_INSERT_HEAD(&pMibMcastSubConfInfo->MOPhead, pNew, entries);
					pMibMcastSubConfInfo->curMopCnt++;
					if(0xffff != portId)
					{
						if(*pMibMop)
							MopAclEntryOp(SET_CTRL_WRITE, portId, *pMibMop);
						else if(*pMibExtMop)
							ExtMopAclEntryOp(SET_CTRL_WRITE, portId, *pMibExtMop);
					}
					break;
				case SET_CTRL_DELETE:
					LIST_FOREACH(ptr, &pMibMcastSubConfInfo->MOPhead, entries)
					{
						if(pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
						{
							/*delete it*/
							if(0xffff != portId)
							{
								oldMop.EntityId = ptr->tableEntry.mcastOperProfPtr;
								if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pOldMibMop))
								{
									OMCI_LOG(OMCI_LOG_LEVEL_DBG, "delete mopTableEntry_t mop 0x%x", oldMop.EntityId);
									MopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibMop);
								}
								else
								{
									oldExtMop.EntityId = oldMop.EntityId;
									if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pOldMibExtMop))
									{
										OMCI_LOG(OMCI_LOG_LEVEL_DBG, "delete extMopTableEntry_t extMop 0x%x", oldExtMop.EntityId);
										ExtMopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibExtMop);
									}
								}
							}
							LIST_REMOVE(ptr, entries);
							free(ptr);
							pMibMcastSubConfInfo->curMopCnt--;
                            break;
						}
					}
					break;
				case SET_CTRL_DELETE_ALL:
					ptr = LIST_FIRST(&pMibMcastSubConfInfo->MOPhead);
					while (NULL != ptr)
					{
						if(0xffff != portId)
						{
							oldMop.EntityId = ptr->tableEntry.mcastOperProfPtr;
							if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pOldMibMop))
							{
								OMCI_LOG(OMCI_LOG_LEVEL_DBG, "delete mopTableEntry_t mop 0x%x", oldMop.EntityId);
								MopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibMop);
							}
							else
							{
								oldExtMop.EntityId = oldMop.EntityId;
								if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pOldMibExtMop))
								{
									OMCI_LOG(OMCI_LOG_LEVEL_DBG, "delete extMopTableEntry_t extMop 0x%x", oldExtMop.EntityId);
									ExtMopAclEntryOp(SET_CTRL_DELETE, portId, pOldMibExtMop);
								}
							}
						}
				        LIST_REMOVE(ptr, entries);
				        free(ptr);
						ptr = LIST_FIRST(&pMibMcastSubConfInfo->MOPhead);
				    }
					LIST_INIT(&pMibMcastSubConfInfo->MOPhead);
					pMibMcastSubConfInfo->curMopCnt = 0;
					break;
				case SET_CTRL_RSV:
				default:
					break;
			}
		}
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	}
	return GOS_OK;
}

static GOS_ERROR_CODE AllowPreviewEntryOper(UINT16 portId, MIB_TABLE_MCASTSUBCONFINFO_T *pSubCfgInfo,
	MIB_TABLE_MCASTSUBCONFINFO_T *pMibMcastSubConfInfo)
{
	allowPreviewGrpTblEntry_t *ptr = NULL, *pTempTbl = NULL,entry, *pNew = NULL;
	omci_allowed_preview_group_table_entry_t *pRawEntry = NULL;

	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s() staring ............%d", __FUNCTION__, __LINE__);

	pRawEntry = &entry.tableEntry;
	memcpy(pRawEntry, pSubCfgInfo->AllowPreviewGroupsTbl, sizeof(omci_allowed_preview_group_table_entry_t));

	pRawEntry->tableCtrl.val = GOS_Htons(pRawEntry->tableCtrl.val);
	switch (pRawEntry->tableCtrl.bit.rowPart)
	{
		case ROW_PART_0:
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!GOS_HtonByte(pRawEntry->row.part0.srcIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN)), GOS_FAIL);
			pRawEntry->row.part0.vidAni			= GOS_Htons(pRawEntry->row.part0.vidAni);
			pRawEntry->row.part0.vidUni			= GOS_Htons(pRawEntry->row.part0.vidUni);
			break;
		case ROW_PART_1:
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!GOS_HtonByte(pRawEntry->row.part1.dstIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN)), GOS_FAIL);
			pRawEntry->row.part1.duration		= GOS_Htons(pRawEntry->row.part1.duration);
			pRawEntry->row.part1.timeleft		= GOS_Htons(pRawEntry->row.part1.timeleft);
			break;
		default:
			OMCI_PRINT("Not support Row Part ID");
	}

	switch(pRawEntry->tableCtrl.bit.setCtrl)
	{
		case SET_CTRL_WRITE:
			LIST_FOREACH(ptr, &pMibMcastSubConfInfo->allowPreviewGrpHead, entries)
			{
				if(pRawEntry->tableCtrl.bit.rowPart == ptr->tableEntry.tableCtrl.bit.rowPart &&
					pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					switch (ptr->tableEntry.tableCtrl.bit.rowPart)
					{
						/* overwrite allowPreviewGrpHead linked list */
						case ROW_PART_0:
							memcpy(ptr->tableEntry.row.part0.srcIpAddr, pRawEntry->row.part0.srcIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN);
							ptr->tableEntry.row.part0.vidAni			= pRawEntry->row.part0.vidAni;
							ptr->tableEntry.row.part0.vidUni 			= pRawEntry->row.part0.vidUni;
							break;
						case ROW_PART_1:
							memcpy(ptr->tableEntry.row.part1.dstIpAddr, pRawEntry->row.part1.dstIpAddr, MIB_TABLE_PREVIEWGRPIPADDR_LEN);
							ptr->tableEntry.row.part1.duration			= pRawEntry->row.part1.duration;
							ptr->tableEntry.row.part1.timeleft			= pRawEntry->row.part1.timeleft;
							break;
						default:
							OMCI_PRINT("Not support Row Part ID");
					}
					if(IsAllRowPartInAllowPreviewEntry(&pMibMcastSubConfInfo->allowPreviewGrpHead, ptr->tableEntry.tableCtrl.bit.rowKey))
					{
						MCAST_WRAPPER(omci_allowed_preview_groups_table_entry_set,
                            SET_CTRL_WRITE, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastSubConfInfo);
					}
					return GOS_OK;

				}
			}

			/*not found, create new entry*/
			pNew = (allowPreviewGrpTblEntry_t *)malloc(sizeof(allowPreviewGrpTblEntry_t));
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pNew), GOS_FAIL);
			memcpy(pNew, &entry, sizeof(allowPreviewGrpTblEntry_t));
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "add allowed preview group table entry");
			LIST_INSERT_HEAD(&pMibMcastSubConfInfo->allowPreviewGrpHead, pNew, entries);

			if(IsAllRowPartInAllowPreviewEntry(&pMibMcastSubConfInfo->allowPreviewGrpHead, pNew->tableEntry.tableCtrl.bit.rowKey))
			{
				MCAST_WRAPPER(omci_allowed_preview_groups_table_entry_set,
                    SET_CTRL_WRITE, portId, pNew->tableEntry.tableCtrl.bit.rowKey, pMibMcastSubConfInfo);
			}
			pMibMcastSubConfInfo->curPreGrpCnt++;
			break;
		case SET_CTRL_DELETE:
            for(ptr = LIST_FIRST(&pMibMcastSubConfInfo->allowPreviewGrpHead); ptr!=NULL;)
			{
				if(pRawEntry->tableCtrl.bit.rowKey == ptr->tableEntry.tableCtrl.bit.rowKey)
				{
					/*delete all row parts with the same key */
					OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete allowed preview group table entry");

					if (ROW_PART_1 == ptr->tableEntry.tableCtrl.bit.rowPart)
					{
						MCAST_WRAPPER(omci_allowed_preview_groups_table_entry_set,
                            SET_CTRL_DELETE, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastSubConfInfo);
					}
                    pTempTbl = ptr;
                    ptr = LIST_NEXT(ptr, entries);
                    LIST_REMOVE(pTempTbl, entries);
                    free(pTempTbl);
                    pMibMcastSubConfInfo->curPreGrpCnt--;
				}
			}
			break;
		case SET_CTRL_DELETE_ALL:
			ptr = LIST_FIRST(&pMibMcastSubConfInfo->allowPreviewGrpHead);
			while (NULL != ptr)
			{
				if(ROW_PART_1 == ptr->tableEntry.tableCtrl.bit.rowPart)
				{
					MCAST_WRAPPER(omci_allowed_preview_groups_table_entry_set,
                        SET_CTRL_DELETE, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastSubConfInfo);
				}
		        LIST_REMOVE(ptr, entries);
		        free(ptr);
				ptr = LIST_FIRST(&pMibMcastSubConfInfo->allowPreviewGrpHead);
		    }
			LIST_INIT(&pMibMcastSubConfInfo->allowPreviewGrpHead);
			pMibMcastSubConfInfo->curPreGrpCnt = 0;
			break;
		case SET_CTRL_RSV:
		default:
			break;
	}
	return GOS_OK;

}

static GOS_ERROR_CODE AllowPreviewEntryClear(UINT16 portId, MIB_TABLE_MCASTSUBCONFINFO_T *pMibMcastSubConfInfo)
{
	allowPreviewGrpTblEntry_t *ptr = NULL;

	ptr = LIST_FIRST(&pMibMcastSubConfInfo->allowPreviewGrpHead);
	while (NULL != ptr)
	{
		if(ROW_PART_1 == ptr->tableEntry.tableCtrl.bit.rowPart)
		{
			MCAST_WRAPPER(omci_allowed_preview_groups_table_entry_set,
                SET_CTRL_DELETE, portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMibMcastSubConfInfo);
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibMcastSubConfInfo->allowPreviewGrpHead);
	}
	LIST_INIT(&pMibMcastSubConfInfo->allowPreviewGrpHead);
	pMibMcastSubConfInfo->curPreGrpCnt= 0;
	return GOS_OK;
}
#if 0
static GOS_ERROR_CODE MopAclClear(UINT16 portId, MIB_TABLE_MCASTOPERPROF_T *pMop)
{
	aclTableEntry_t *ptr = NULL;
	LIST_FOREACH(ptr, &pMop->DACLhead, entries)
	{
		if(ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_IPV4, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMop, NULL);
		}
		else if (ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_IPV6, MIB_TABLE_MCASTOPERPROF_DYACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMop, NULL);
		}
	}

	ptr = NULL;
	LIST_FOREACH(ptr, &pMop->SACLhead, entries)
	{
		if(ROW_PART_0 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_IPV4, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMop, NULL);
		}
		else if (ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_IPV6, MIB_TABLE_MCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, pMop, NULL);
		}
	}
	//TODO LOST table
	return GOS_OK;
}

static GOS_ERROR_CODE ExtMopAclClear(UINT16 portId, MIB_TABLE_EXTMCASTOPERPROF_T *pExtMop)
{
	extAclTableEntry_t *ptr = NULL;
	LIST_FOREACH(ptr, &pExtMop->DACLhead, entries)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_UNKNOWN, MIB_TABLE_EXTMCASTOPERPROF_DYACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pExtMop);
		}
	}

	ptr = NULL;
	LIST_FOREACH(ptr, &pExtMop->SACLhead, entries)
	{
		if(ROW_PART_2 == ptr->tableEntry.tableCtrl.bit.rowPartId)
		{
			WAPPER_MCAST_ACL_SET(SET_CTRL_DELETE, IP_TYPE_UNKNOWN, MIB_TABLE_EXTMCASTOPERPROF_STATICACLTABLE_INDEX,
				portId, ptr->tableEntry.tableCtrl.bit.rowKey, NULL, pExtMop);
		}
	}
	//TODO LOST table
	return GOS_OK;
}
#endif
static GOS_ERROR_CODE MopEntryClear(UINT16 portId, MIB_TABLE_MCASTSUBCONFINFO_T *pMibMcastSubConfInfo)
{
	mopTableEntry_t *ptr = NULL;
	MIB_TABLE_MCASTOPERPROF_T mop, *pMibMop = NULL;
	MIB_TABLE_EXTMCASTOPERPROF_T extMop, *pMibExtMop = NULL;

	ptr = LIST_FIRST(&pMibMcastSubConfInfo->MOPhead);
	while (NULL != ptr)
	{
		mop.EntityId = ptr->tableEntry.mcastOperProfPtr;
		if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, &pMibMop))
		{
			omci_mcast_port_reset(portId, pMibMcastSubConfInfo, pMibMop->EntityId);
			if(0xffff != portId)
				MopAclEntryOp(SET_CTRL_DELETE, portId, pMibMop);

			//TBD: MopAclClear(portId, pMibMop);
		}
		else
		{
			if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &extMop, &pMibExtMop))
			{
				omci_mcast_port_reset(portId, pMibMcastSubConfInfo, pMibExtMop->EntityId);
				if(0xffff != portId)
					ExtMopAclEntryOp(SET_CTRL_DELETE, portId, pMibExtMop);
				//TBD: ExtMopAclClear(portId, pMibExtMop);
			}
		}
		LIST_REMOVE(ptr, entries);
		free(ptr);
		ptr = LIST_FIRST(&pMibMcastSubConfInfo->MOPhead);
	}
	LIST_INIT(&pMibMcastSubConfInfo->MOPhead);
	pMibMcastSubConfInfo->curMopCnt = 0;
	return GOS_OK;
}

GOS_ERROR_CODE McastSubConfInfoDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_MCASTSUBCONFINFO_T *pMcastSubConfInfo = (MIB_TABLE_MCASTSUBCONFINFO_T*)pData;

	OMCI_PRINT("EntityId: 0x%02x", pMcastSubConfInfo->EntityId);
	OMCI_PRINT("MeType: %d", pMcastSubConfInfo->MeType);
	OMCI_PRINT("McastOperProfPtr: 0x%02x", pMcastSubConfInfo->McastOperProfPtr);
	OMCI_PRINT("MaxSimGroups: %u", pMcastSubConfInfo->MaxSimGroups);
	OMCI_PRINT("MaxMcastBandwidth: %d", pMcastSubConfInfo->MaxMcastBandwidth);
	OMCI_PRINT("BandwidthEnforcement: %d", pMcastSubConfInfo->BandwidthEnforcement);
	McastSvcPkgTableDump(pMcastSubConfInfo);
	McastAllowedPreviewTableDump(pMcastSubConfInfo);

	return GOS_OK;
}

GOS_ERROR_CODE McastSubConfInfoDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_MCASTSUBCONFINFO_T *pSubCfgInfo = NULL, newMcastConfInfo, *pMibMcastSubConfInfo = NULL;
	MIB_TABLE_MACBRIPORTCFGDATA_T *pMibBridgePort = NULL, oldBridgePort;
	MIB_TABLE_MCASTOPERPROF_T *pMibMop = NULL, oldMop, *pOldMibMop = NULL;
	MIB_TABLE_EXTMCASTOPERPROF_T *pMibExtMop = NULL, oldExtMop, *pOldMibExtMop = NULL;
	MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
	MIB_TABLE_VEIP_T mibVeip;
	GOS_ERROR_CODE                  ret;
	UINT16 portId = 0xffff;
	BOOL bindUniB = FALSE, trapB = FALSE;
	MIB_TREE_T *pTree = NULL;
	UINT8 mcastSubCfgCnt = 0;
	mopTableEntry_t *ptr = NULL;

	if(MIB_GET == operationType) return GOS_OK;

	pSubCfgInfo = (MIB_TABLE_MCASTSUBCONFINFO_T *) pNewRow;
	newMcastConfInfo.EntityId = pSubCfgInfo->EntityId;

	OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &newMcastConfInfo, &pMibMcastSubConfInfo)), GOS_FAIL);

	mcastSubCfgCnt = MIB_GetTableCurEntryCount(MIB_TABLE_MCASTSUBCONFINFO_INDEX);

	oldBridgePort.EntityID = pSubCfgInfo->EntityId;
	ret =  mib_FindEntry(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &oldBridgePort, &pMibBridgePort);
	if(FALSE == ret)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find mac bridge port 0x%x", pSubCfgInfo->EntityId);
	}
	else
	{
		bindUniB = TRUE;
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
		pTree = MIB_AvlTreeSearchByKey(NULL, oldBridgePort.EntityID, AVL_KEY_MACBRIPORT_UNI);
		switch (operationType)
		{
			case MIB_ADD:
				OMCI_LOG(OMCI_LOG_LEVEL_DBG, "McastSubConfInfoMopEntryOp with UNI --> ADD(%x)", attrSet);

				//SET_BY_CREATE attribute pass to igmp process.
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX))
					MCAST_WRAPPER(omci_max_simultaneous_groups_set, portId, pSubCfgInfo->MaxSimGroups);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX))
					MCAST_WRAPPER(omci_max_multicast_bandwidth_set, portId, pSubCfgInfo->MaxMcastBandwidth);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX))
					MCAST_WRAPPER(omci_bandwidth_enforcement_set, portId, pSubCfgInfo->BandwidthEnforcement);

				MopEntryOper(attrSet, portId, pSubCfgInfo, pMibMcastSubConfInfo, &pMibMop, &pMibExtMop);

				if(pMibMop)
				{
					MCAST_WRAPPER(omci_igmp_function_set, pMibMop->EntityId, portId, pMibMop->IGMPFun);
                    MCAST_WRAPPER(omci_igmp_version_set, pMibMop->EntityId, portId, pMibMop->IGMPVersion);
					MCAST_WRAPPER(omci_immediate_leave_set, pMibMop->EntityId, portId, pMibMop->ImmediateLeave);
					MCAST_WRAPPER(omci_us_igmp_rate_set, pMibMop->EntityId, portId, pMibMop->UsIGMPRate);

					trapB = TRUE;
				}
				else if(pMibExtMop)
				{
					MCAST_WRAPPER(omci_igmp_function_set, pMibExtMop->EntityId, portId, pMibExtMop->IGMPFun);
                    MCAST_WRAPPER(omci_igmp_version_set, pMibExtMop->EntityId, portId, pMibExtMop->IGMPVersion);
					MCAST_WRAPPER(omci_immediate_leave_set, pMibExtMop->EntityId, portId, pMibExtMop->ImmediateLeave);
					MCAST_WRAPPER(omci_us_igmp_rate_set, pMibExtMop->EntityId, portId, pMibExtMop->UsIGMPRate);

					trapB = TRUE;
				}

				MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, portId, trapB);
				if(mcastSubCfgCnt > 0 && mcastSubCfgCnt <= gInfo.devCapabilities.fePortNum + gInfo.devCapabilities.gePortNum)
				{
					MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, (UINT16)gInfo.devCapabilities.ponPort, trapB); //pon port trap igmp
				}
				break;
			case MIB_SET:
				OMCI_LOG(OMCI_LOG_LEVEL_DBG, "McastSubConfInfoMopEntryOp with UNI(%u) --> SET(%x)", portId, attrSet);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX))
					MCAST_WRAPPER(omci_max_simultaneous_groups_set, portId, pSubCfgInfo->MaxSimGroups);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX))
					MCAST_WRAPPER(omci_max_multicast_bandwidth_set, portId, pSubCfgInfo->MaxMcastBandwidth);

				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX))
					MCAST_WRAPPER(omci_bandwidth_enforcement_set, portId, pSubCfgInfo->BandwidthEnforcement);

				MopEntryOper(attrSet, portId, pSubCfgInfo, pMibMcastSubConfInfo, &pMibMop, &pMibExtMop);


                if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX) ||
					MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX))
				{
					//Only support unauthorized Join request behavior mcast profile list for the specific port are the same value
					LIST_FOREACH(ptr, &pMibMcastSubConfInfo->MOPhead, entries)
					{
						oldMop.EntityId = ptr->tableEntry.mcastOperProfPtr;
						if(TRUE == mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &oldMop, &pOldMibMop))
						{
							MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, portId, TRUE);
							if(mcastSubCfgCnt > 0 && mcastSubCfgCnt <= gInfo.devCapabilities.fePortNum + gInfo.devCapabilities.gePortNum)
							{
								MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, (UINT16)gInfo.devCapabilities.ponPort, TRUE); //pon port trap igmp
							}
							break;
						}
						else
						{
							oldExtMop.EntityId = oldMop.EntityId;
							if(TRUE == mib_FindEntry(MIB_TABLE_EXTMCASTOPERPROF_INDEX, &oldExtMop, &pOldMibExtMop))
							{
								MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, portId, TRUE);
								if(mcastSubCfgCnt > 0 && mcastSubCfgCnt <= gInfo.devCapabilities.fePortNum + gInfo.devCapabilities.gePortNum)
								{
									MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, (UINT16)gInfo.devCapabilities.ponPort, TRUE); //pon port trap igmp
								}
								break;
							}
						}
					}
				}
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "fun =%s, line=%d ", __FUNCTION__, __LINE__);
					AllowPreviewEntryOper(portId, pSubCfgInfo, pMibMcastSubConfInfo);
				}
				break;
			case MIB_DEL:
				OMCI_LOG(OMCI_LOG_LEVEL_DBG, "McastSubConfInfoMopEntryOp with UNI(%u) --> DEL", portId);
                if (pTree)
                    MIB_TreeConnUpdate(pTree);
				MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, portId, FALSE);
				if (1 == mcastSubCfgCnt)
				{
					MCAST_WRAPPER(omci_ctrl_pkt_behaviour_set, (UINT16)gInfo.devCapabilities.ponPort, FALSE); //pon port trap igmp
				}
				//reset mcast info and acl rule for this port
				MopEntryClear(portId, pMibMcastSubConfInfo);
				AllowPreviewEntryClear(portId, pMibMcastSubConfInfo);
				break;
			default:
				return GOS_OK;
		}
		if (pTree && MIB_DEL != operationType)
			MIB_TreeConnUpdate(pTree);

	}

	if(!bindUniB)
	{
		switch(operationType)
		{
			case MIB_ADD:
			case MIB_SET:
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"McastSubConfInfoMopEntryOp --> ADD/SET");
				MopEntryOper(attrSet, portId, pSubCfgInfo, pMibMcastSubConfInfo, &pMibMop, &pMibExtMop);
				break;
			case MIB_DEL:
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"McastSubConfInfoMopEntryOp --> DEL");
				MopEntryClear(portId, pMibMcastSubConfInfo);
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
    gMibMcastSubConfInfoTableInfo.Name = "McastSubConfInfo";
    gMibMcastSubConfInfoTableInfo.ShortName = "MCSCI";
    gMibMcastSubConfInfoTableInfo.Desc = "Multicast subscriber configure inforation";
    gMibMcastSubConfInfoTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MCAST_SUBSCRIBER_CFG_INFO);
    gMibMcastSubConfInfoTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMcastSubConfInfoTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMcastSubConfInfoTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMcastSubConfInfoTableInfo.pAttributes = &(gMibMcastSubConfInfoAttrInfo[0]);

	gMibMcastSubConfInfoTableInfo.attrNum = MIB_TABLE_MCASTSUBCONFINFO_ATTR_NUM;
	gMibMcastSubConfInfoTableInfo.entrySize = sizeof(MIB_TABLE_MCASTSUBCONFINFO_T);
	gMibMcastSubConfInfoTableInfo.pDefaultRow = &gMibMcastSubConfInfoDefRow;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MeType";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "McastOperProfPtr";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxSimGroups";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxMcastBandwidth";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BandwidthEnforcement";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "McastSvcPkgTbl";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AllowPreviewGroupsTbl";

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ME Type";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Multicast operation profile pointer";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Max simultaneous groups";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Max multicast bandwidth";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bandwidth enforcement";
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Multicast service package table";
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Allowed preview groups table";

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_SVCPKGTBL_LEN;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_ALLOWPREVIEWGRPTBL_LEN;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_METYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTOPERPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXSIMGROUPS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MAXMCASTBANDWIDTH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_BANDWIDTHENFORCEMENT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_OPTIONAL | OMCI_ME_ATTR_TYPE_TABLE);
	gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_ALLOWPREVIEWGRPTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_OPTIONAL | OMCI_ME_ATTR_TYPE_TABLE);

    memset(&(gMibMcastSubConfInfoDefRow.EntityId), 0x00, sizeof(gMibMcastSubConfInfoDefRow.EntityId));
    memset(&(gMibMcastSubConfInfoDefRow.MeType), 0x00, sizeof(gMibMcastSubConfInfoDefRow.MeType));
    memset(&(gMibMcastSubConfInfoDefRow.McastOperProfPtr), 0x00, sizeof(gMibMcastSubConfInfoDefRow.McastOperProfPtr));
    memset(&(gMibMcastSubConfInfoDefRow.MaxSimGroups), 0x00, sizeof(gMibMcastSubConfInfoDefRow.MaxSimGroups));
    memset(&(gMibMcastSubConfInfoDefRow.MaxMcastBandwidth), 0x00, sizeof(gMibMcastSubConfInfoDefRow.MaxMcastBandwidth));
    memset(&(gMibMcastSubConfInfoDefRow.BandwidthEnforcement), 0x00, sizeof(gMibMcastSubConfInfoDefRow.BandwidthEnforcement));
    memset(gMibMcastSubConfInfoDefRow.McastSvcPkgTbl, 0, MIB_TABLE_SVCPKGTBL_LEN);
    memset(gMibMcastSubConfInfoDefRow.AllowPreviewGroupsTbl, 0, MIB_TABLE_ALLOWPREVIEWGRPTBL_LEN);

    gMibMcastSubConfInfoDefRow.curMopCnt = 0;
    /* for Multicast service package table and McastOperProfPtr  */
    LIST_INIT(&gMibMcastSubConfInfoDefRow.MOPhead);
    LIST_INIT(&gMibMcastSubConfInfoDefRow.allowPreviewGrpHead);

    memset(&gMibMcastSubConfInfoOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibMcastSubConfInfoOper.meOperDrvCfg = McastSubConfInfoDrvCfg;
    gMibMcastSubConfInfoOper.meOperConnCheck = NULL;
    gMibMcastSubConfInfoOper.meOperDump = McastSubConfInfoDumpMib;
	gMibMcastSubConfInfoOper.meOperConnCfg = NULL;
	MIB_TABLE_MCASTSUBCONFINFO_INDEX = tableId;

	feature_api(FEATURE_API_MC_00000004, &gMibMcastSubConfInfoAttrInfo[MIB_TABLE_MCASTSUBCONFINFO_MCASTSVCPKGTABLE_INDEX - MIB_TABLE_FIRST_INDEX]);

    MIB_InfoRegister(tableId, &gMibMcastSubConfInfoTableInfo, &gMibMcastSubConfInfoOper);

    return GOS_OK;
}

