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
 * Purpose : Definition of ME handler: Extended VLAN tagging operation configuration data (171)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Extended VLAN tagging operation configuration data (171)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibExtVlanTagOperCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ATTR_NUM];
MIB_TABLE_EXTVLANTAGOPERCFGDATA_T gMibExtVlanTagOperCfgDataDefRow;
MIB_TABLE_OPER_T gMibExtVlanTagOperCfgDataOper;

extern omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];

static void ExtVlanTagTableDump(MIB_TABLE_EXTVLANTAGOPERCFGDATA_T* pExtVlan)
{
	extvlanTableEntry_t *pEntry;
	omci_extvlan_row_entry_t *pRowEntry;
	int count=0;

	OMCI_PRINT("ReceivedFrameVlanTaggingOperTable");
	LIST_FOREACH(pEntry,&pExtVlan->head,entries){
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("INDEX %d",count);
		OMCI_PRINT("Filter Outer   : PRI %d,VID %d, TPID %d",
		pRowEntry->outerFilterWord.bit.filterOuterPri,
		pRowEntry->outerFilterWord.bit.filterOuterVid,
		pRowEntry->outerFilterWord.bit.filterOuterTpId);
		OMCI_PRINT("Filter Inner   : PRI %d,VID %d, TPID %d, EthType 0x%02x",
		pRowEntry->innerFilterWord.bit.filterInnerPri,
		pRowEntry->innerFilterWord.bit.filterInnerVid,
		pRowEntry->innerFilterWord.bit.filterInnerTpId,
		pRowEntry->innerFilterWord.bit.filterEthType);
		OMCI_PRINT("Treatment Outer   : PRI %d,VID %d, TPID %d, RemoveTags %d",
		pRowEntry->outerTreatmentWord.bit.treatmentOuterPri,
		pRowEntry->outerTreatmentWord.bit.treatmentOuterVid,
		pRowEntry->outerTreatmentWord.bit.treatmentOuterTpId,
		pRowEntry->outerTreatmentWord.bit.treatment);
		OMCI_PRINT("Treatment Inner   : PRI %d,VID %d, TPID %d",
		pRowEntry->innerTreatmentWord.bit.treatmentInnerPri,
		pRowEntry->innerTreatmentWord.bit.treatmentInnerVid,
		pRowEntry->innerTreatmentWord.bit.treatmentInnerTpId);
		count ++;
	}
}

static void evtocd_dscp_to_pbit_mapping_dump(UINT8 *pDscpToPbitMapping)
{
	unsigned char i;

	if (!pDscpToPbitMapping)
		return;

	OMCI_PRINT("DscpToPbitMapping:");
	for (i = 0; i < 24; i += 3)
	{
		OMCI_PRINT("\t0x%02x%02x%02x",
			pDscpToPbitMapping[i],
			pDscpToPbitMapping[i+1],
			pDscpToPbitMapping[i+2]);
	}
}

GOS_ERROR_CODE ExtVlanTagOperCfgDataDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlanTagOperCfgData = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pData;

	OMCI_PRINT("EntityId: 0x%02x", pExtVlanTagOperCfgData->EntityId);
	OMCI_PRINT("AssociationType: %d", pExtVlanTagOperCfgData->AssociationType);
	OMCI_PRINT("ReceivedFrameVlanTagOperTableMaxSize: %d", pExtVlanTagOperCfgData->ReceivedFrameVlanTagOperTableMaxSize);
	OMCI_PRINT("InputTPID: 0x%02x", pExtVlanTagOperCfgData->InputTPID);
	OMCI_PRINT("OutputTPID: 0x%02x", pExtVlanTagOperCfgData->OutputTPID);
	OMCI_PRINT("DsMode: %d", pExtVlanTagOperCfgData->DsMode);
	ExtVlanTagTableDump(pExtVlanTagOperCfgData);
	OMCI_PRINT("AssociatedMePoint: 0x%02x", pExtVlanTagOperCfgData->AssociatedMePoint);
	evtocd_dscp_to_pbit_mapping_dump(pExtVlanTagOperCfgData->DscpToPbitMapping);
	return GOS_OK;
}

/*
static BOOL ExtVlanEntryCheck(UINT8 *extVlanEntry)
{
	if(0 == *extVlanEntry)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no extvlan entry ");
		return FALSE;
	}
	else
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"there are extvlan entry ");
		return TRUE;
	}
}
*/

static BOOL ExtVlanEntryCompare(omci_extvlan_row_entry_t *pEntryA,omci_extvlan_row_entry_t *pEntryB)
{
	if(pEntryA->outerFilterWord.val == pEntryB->outerFilterWord.val &&
	   pEntryA->innerFilterWord.val == pEntryB->innerFilterWord.val)
	{
		return 0;
	}

	return 1;
}


static GOS_ERROR_CODE ExtVlanEntryClear(MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan)
{
	extvlanTableEntry_t *pEntry;
	pEntry = LIST_FIRST(&pExtVlan->head);
	while(pEntry!=NULL)
	{
		LIST_REMOVE(pEntry,entries);
		free(pEntry);
		pEntry = LIST_FIRST(&pExtVlan->head);
	}
	LIST_INIT(&pExtVlan->head);
	pExtVlan->curExtTableEntryCnt = 0;
	return GOS_OK;
}


void debug_ExtVlan(UINT8 *table)
{
	printf("[OuterFilter]: %02x %02x %02x %02x\n",table[0],table[1],table[2],table[3]);
	printf("[InnerFilter]: %02x %02x %02x %02x\n",table[4],table[5],table[6],table[7]);
	printf("[OuterTreate]: %02x %02x %02x %02x\n",table[8],table[9],table[10],table[11]);
	printf("[InnerTreate]: %02x %02x %02x %02x\n",table[12],table[13],table[14],table[15]);
}

static MIB_TREE_T* ExtVlanTagOperCfgDataGetTree(MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan,MIB_AVL_KEY_T *key)
{
	MIB_TREE_T *pTree=NULL;

	switch(pExtVlan->AssociationType){
	case EVTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT:
	{
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_MACBRIPORT_UNI);
		if(pTree)
		{
			*key = AVL_KEY_EXTVLAN_UNI;
		}else
		{
			pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_MACBRIPORT_ANI);
			*key = AVL_KEY_EXTVLAN_ANI;
		}
	}
	break;
	case EVTOCD_ASSOC_TYPE_IEEE_8021P_MAPPER:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_MAP8021PSERVPROF);
		*key = AVL_KEY_EXTVLAN_ANI;
	break;
	case EVTOCD_ASSOC_TYPE_PPTP_ETH_UNI:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_PPTPUNI);
		*key = AVL_KEY_EXTVLAN_UNI;
	break;
	case EVTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_IPHOST);
		*key = AVL_KEY_EXTVLAN_UNI;
	break;
	case EVTOCD_ASSOC_TYPE_GEM_IWTP:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_GEMIWTP);
		*key = AVL_KEY_EXTVLAN_ANI;
	break;
	case EVTOCD_ASSOC_TYPE_MCAST_GEM_IWTP:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_MULTIGEMIWTP);
		*key = AVL_KEY_EXTVLAN_ANI;
	break;
	case EVTOCD_ASSOC_TYPE_VEIP:
		pTree = MIB_AvlTreeSearchByKey(NULL, pExtVlan->AssociatedMePoint,AVL_KEY_VEIP);
		*key = AVL_KEY_EXTVLAN_UNI;
	break;
	default:
		OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Can't support this associationType %d",pExtVlan->AssociationType);
	break;
	}
	return pTree;
}



static GOS_ERROR_CODE ExtVlanEntryOper(UINT8 *pReceiveEntry,MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan)
{
	extvlanTableEntry_t *pEntry,*pTmpEntry;
	omci_extvlan_row_entry_t *pRowEntry;

	pEntry = (extvlanTableEntry_t*)malloc(sizeof(extvlanTableEntry_t));

	if(!pEntry)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Malloc extvlan Table Entry Fail");
		return GOS_FAIL;
	}

	memcpy(&(pEntry->tableEntry),(omci_extvlan_row_entry_t*)pReceiveEntry,sizeof(omci_extvlan_row_entry_t));

	pRowEntry = &pEntry->tableEntry;

	pRowEntry->outerFilterWord.val 		= GOS_Htonl(pRowEntry->outerFilterWord.val);
	pRowEntry->innerFilterWord.val 		= GOS_Htonl(pRowEntry->innerFilterWord.val);
	pRowEntry->outerTreatmentWord.val 	= GOS_Htonl(pRowEntry->outerTreatmentWord.val);
	pRowEntry->innerTreatmentWord.val 	= GOS_Htonl(pRowEntry->innerTreatmentWord.val);

	/*define in G.988 Page 120:
	The first eight bytes of each entry are guaranteed to be unique, and are used
	to identify table entries (list order, above, refers to a sort on the first
	eight bytes). The OLT deletes a table entry by setting all of its last eight
	bytes to 0xFF.
	*/
	/*compare this entry is exist or not*/
	LIST_FOREACH(pTmpEntry,&pExtVlan->head,entries)
	{
		if(! ExtVlanEntryCompare(pRowEntry,&pTmpEntry->tableEntry))
		{
			if(pRowEntry->innerTreatmentWord.val == 0xffffffff && pRowEntry->outerTreatmentWord.val == 0xffffffff)
			{
				/*delete it*/
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete ExtVlanTable entry");
				LIST_REMOVE(pTmpEntry,entries);
				free(pTmpEntry);
				pExtVlan->curExtTableEntryCnt--;
			}else
			{
				/*modify it */
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"modify ExtVlanTable entry");
				pTmpEntry->tableEntry.innerFilterWord.val = pRowEntry->innerFilterWord.val;
				pTmpEntry->tableEntry.outerTreatmentWord.val = pRowEntry->outerTreatmentWord.val;
				pTmpEntry->tableEntry.innerTreatmentWord.val = pRowEntry->innerTreatmentWord.val;
			}
			free(pEntry);
			return GOS_OK;
		}
	}
	/*not found, create new entry*/
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add ExtVlanTable entry");
	LIST_INSERT_HEAD(&pExtVlan->head,pEntry,entries);
	pExtVlan->curExtTableEntryCnt++;
	return GOS_OK;
}

GOS_ERROR_CODE ExtVlanTagOperCfgDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan,*pMibExtVlan,tmpExtVlan;
	MIB_TREE_T *pTree = NULL;
	MIB_AVL_KEY_T key;
	MIB_ATTR_INDEX attrIndex;
	extvlanTableEntry_t *pEntry = NULL;
	UINT8 *ptr = NULL;

	pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pNewRow;
	tmpExtVlan.EntityId = pExtVlan->EntityId;

	if(!mib_FindEntry(MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX,&tmpExtVlan,&pMibExtVlan))
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"can't find extvlan 0x%x",pExtVlan->EntityId);
		return GOS_OK;
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"find extvlan %p",pMibExtVlan);

	switch(operationType){
	case MIB_ADD:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"ExtVlanTagOperCfg --> ADD");
		LIST_INIT(&pMibExtVlan->head);
	}
	break;
	case MIB_SET:
	{
        UINT8 zeroDscp[MIB_TABLE_DSCPTOPBITMAPPING_LEN] = {0x0};
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"ExtVlanTagOperCfg --> SET");

		// first byte filled with 0 doesn't meant there is no data
		//if(ExtVlanEntryCheck(pExtVlan->ReceivedFrameVlanTaggingOperTable))

		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX))
		{
			ExtVlanEntryOper(pExtVlan->ReceivedFrameVlanTaggingOperTable,pMibExtVlan);
			pTree = ExtVlanTagOperCfgDataGetTree(pExtVlan,&key);
			if(pTree)
				MIB_TreeConnUpdate(pTree);
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX) &&
          0 != memcmp(pExtVlan->DscpToPbitMapping, zeroDscp, MIB_TABLE_DSCPTOPBITMAPPING_LEN))
		{
		    omci_wrapper_setDscpRemap(pExtVlan->DscpToPbitMapping);
		}
	}
	break;
	case MIB_DEL:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"ExtVlanTagOperCfg --> DEL");
		ExtVlanEntryClear(pMibExtVlan);
	}
	break;
	case MIB_GET:
	{
		attrIndex = MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX;
		if (MIB_IsInAttrSet(&attrSet, attrIndex))
		{
			OMCI_PRINT("ExtVlanTagOperCfg --> GET table attribute");

			gOmciMulGetData[pri].attribute[attrIndex].attrSize = MIB_TABLE_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_LEN * (pExtVlan->curExtTableEntryCnt);
			gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
			gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
			gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
				(gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

			ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

			LIST_FOREACH(pEntry, &pExtVlan->head, entries)
			{
				memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_LEN);
				ptr += MIB_TABLE_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_LEN;
			}
		}
	}
	break;
	default:
		return GOS_FAIL;
	break;
	}

	return GOS_OK;
}

GOS_ERROR_CODE ExtVlanTagOperCfgDataConnCheck(MIB_TREE_T *pTree, MIB_TREE_CONN_T *pConn, omci_me_instance_t entityId, int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_NODE_T  *pNode;
	MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan;
	BOOL bSkipMatch;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);

    if (AVL_KEY_MACBRIPORT_ANI != parm)
    {
    	pNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_EXTVLAN_UNI);

    	if(pNode)
    	{

    		LIST_FOREACH(pNodeEntry,&pNode->data.treeNodeEntryHead, treeNodeEntry)
    		{
    			pEntry = pNodeEntry->mibEntry;
    			pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData;

    			bSkipMatch = FALSE;
    			switch (parm)
    			{
    				case AVL_KEY_MACBRIPORT_UNI:
    					if (EVTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT != pExtVlan->AssociationType)
    						bSkipMatch = TRUE;
    					break;
    				case AVL_KEY_PPTPUNI:
    					if (EVTOCD_ASSOC_TYPE_PPTP_ETH_UNI != pExtVlan->AssociationType)
    						bSkipMatch = TRUE;
    					break;
    				case AVL_KEY_VEIP:
    					if (EVTOCD_ASSOC_TYPE_VEIP != pExtVlan->AssociationType)
    						bSkipMatch = TRUE;
    					break;
    				case AVL_KEY_IPHOST:
    					if (EVTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST != pExtVlan->AssociationType)
    						bSkipMatch = TRUE;
    					break;
    				default:
    					break;
    			}
    			if (bSkipMatch)
    				continue;

    			if(pExtVlan->AssociatedMePoint == entityId)
    			{
    				pConn->pUniExtVlanCfg = pExtVlan;
    				return GOS_OK;
    			}
    		}
    	}
    }
	pNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_EXTVLAN_ANI);

	if(pNode)
	{
		LIST_FOREACH(pNodeEntry,&pNode->data.treeNodeEntryHead, treeNodeEntry)
		{
			pEntry = pNodeEntry->mibEntry;
			pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData;

			if(pExtVlan->AssociatedMePoint == entityId)
			{
				pConn->pAniExtVlanCfg = pExtVlan;
				return GOS_OK;
			}
		}
	}
	return GOS_FAIL;
}



GOS_ERROR_CODE ExtVlanTagOperCfgDataConnCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TREE_T *pTree;
	MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan;
	MIB_AVL_KEY_T key;

	switch(operationType){
	case MIB_ADD:
	{
		pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pNewRow;
		pTree = ExtVlanTagOperCfgDataGetTree(pExtVlan,&key);

		if(!pTree) return GOS_FAIL;

		if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX,pExtVlan)==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add ExtVlanTagCfgData Node Fail");
			return GOS_FAIL;
		}
	}
	break;
	case MIB_SET:
		pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pNewRow;
		pTree = ExtVlanTagOperCfgDataGetTree(pExtVlan,&key);
		if(!pTree) return GOS_FAIL;
	break;
	case MIB_DEL:
		pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pOldRow;
		pTree = ExtVlanTagOperCfgDataGetTree(pExtVlan,&key);
		if(!pTree)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"can't find root tree");
			return GOS_FAIL;
		}
		MIB_AvlTreeNodeEntryRemoveByKey(pTree->root,key,pExtVlan->EntityId);
        MIB_TreeConnUpdate(pTree);
	break;
	default:
		return GOS_FAIL;
	break;
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	return GOS_OK;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibExtVlanTagOperCfgDataTableInfo.Name = "ExtVlanTagOperCfgData";
    gMibExtVlanTagOperCfgDataTableInfo.ShortName = "ETVTOCD";
    gMibExtVlanTagOperCfgDataTableInfo.Desc = "Extended VLAN tagging operation configure data";
    gMibExtVlanTagOperCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_EXTENDED_VLAN_TAGGING_OP_CFG_DATA);
    gMibExtVlanTagOperCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibExtVlanTagOperCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibExtVlanTagOperCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibExtVlanTagOperCfgDataTableInfo.pAttributes = &(gMibExtVlanTagOperCfgDataAttrInfo[0]);

	gMibExtVlanTagOperCfgDataTableInfo.attrNum = MIB_TABLE_EXTVLANTAGOPERCFGDATA_ATTR_NUM;
	gMibExtVlanTagOperCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_EXTVLANTAGOPERCFGDATA_T);
	gMibExtVlanTagOperCfgDataTableInfo.pDefaultRow = &gMibExtVlanTagOperCfgDataDefRow;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AssociationType";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedFrameVlanTagOperTableMaxSize";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "InputTPID";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OutputTPID";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DsMode";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReceivedFrameVlanTaggingOperTable";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AssociatedMePoint";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DscpToPbitMapping";

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Type of the ME associated";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "specifies peak information rate";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "special TPID value for operations.";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "special TPID value for operations.";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "downstream mode";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "table for filters tags upstream frames";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "associated ME pointer";
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "DSCP to Pbit mapping";

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 16;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ| OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGOPERTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_INPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_OUTPUTTPID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSMODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_ASSOCIATEDMEPOINT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtVlanTagOperCfgDataAttrInfo[MIB_TABLE_EXTVLANTAGOPERCFGDATA_DSCPTOPBITMAPPING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibExtVlanTagOperCfgDataDefRow.EntityId), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.EntityId));
    memset(&(gMibExtVlanTagOperCfgDataDefRow.AssociationType), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.AssociationType));
    memset(&(gMibExtVlanTagOperCfgDataDefRow.ReceivedFrameVlanTagOperTableMaxSize), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.ReceivedFrameVlanTagOperTableMaxSize));
    memset(&(gMibExtVlanTagOperCfgDataDefRow.InputTPID), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.InputTPID));
    memset(&(gMibExtVlanTagOperCfgDataDefRow.OutputTPID), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.OutputTPID));
    memset(&(gMibExtVlanTagOperCfgDataDefRow.DsMode), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.DsMode));
    memset(gMibExtVlanTagOperCfgDataDefRow.ReceivedFrameVlanTaggingOperTable, 0, MIB_TABLE_RECEIVEDFRAMEVLANTAGGINGOPERTABLE_LEN);
    memset(&(gMibExtVlanTagOperCfgDataDefRow.AssociatedMePoint), 0x00, sizeof(gMibExtVlanTagOperCfgDataDefRow.AssociatedMePoint));
    memset(gMibExtVlanTagOperCfgDataDefRow.DscpToPbitMapping, 0, MIB_TABLE_DSCPTOPBITMAPPING_LEN);

	/*add for table type attribute*/
	LIST_INIT(&gMibExtVlanTagOperCfgDataDefRow.head);
	gMibExtVlanTagOperCfgDataDefRow.curExtTableEntryCnt = 0;

    memset(&gMibExtVlanTagOperCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibExtVlanTagOperCfgDataOper.meOperDrvCfg = ExtVlanTagOperCfgDataDrvCfg;
    gMibExtVlanTagOperCfgDataOper.meOperConnCheck = ExtVlanTagOperCfgDataConnCheck;
    gMibExtVlanTagOperCfgDataOper.meOperDump = ExtVlanTagOperCfgDataDumpMib;
	gMibExtVlanTagOperCfgDataOper.meOperConnCfg = ExtVlanTagOperCfgDataConnCfg;

	MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibExtVlanTagOperCfgDataTableInfo, &gMibExtVlanTagOperCfgDataOper);

    return GOS_OK;
}
