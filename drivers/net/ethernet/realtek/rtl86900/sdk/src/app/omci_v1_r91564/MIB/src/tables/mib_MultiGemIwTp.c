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
 * Purpose : Definition of ME handler: Multicast GEM IWTP (281)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Multicast GEM IWTP (281)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibMultiGemIwTpTableInfo;
MIB_ATTR_INFO_T  gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ATTR_NUM];
MIB_TABLE_MULTIGEMIWTP_T gMibMultiGemIwTpDefRow;
MIB_TABLE_OPER_T	gMibMultiGemIwTpOper;

static GOS_ERROR_CODE Ipv6McastAddrEntryOper(UINT8 *pMcastAddrEntry, MIB_TABLE_MULTIGEMIWTP_T *pMultiGemIwTp)
{
	multiGemIwTpIpv6TableEntry_t *pEntry = NULL, *ptr = NULL, entry;
	omci_ipv6_multicast_addr_entry_t *pRawEntry;
	UINT8 zero[MIB_TABLE_MULTIGEMIWTP_MSB_LEN];

	memcpy(&(entry.tableEntry), (omci_ipv6_multicast_addr_entry_t *)pMcastAddrEntry, sizeof(omci_ipv6_multicast_addr_entry_t));
	memset(&zero, 0x0, MIB_TABLE_MULTIGEMIWTP_MSB_LEN);

	pRawEntry = &entry.tableEntry;

	pRawEntry->ipv6Entry.gemPort 		= GOS_Htons(pRawEntry->ipv6Entry.gemPort);
	pRawEntry->ipv6Entry.secKey 		= GOS_Htons(pRawEntry->ipv6Entry.secKey);
	pRawEntry->ipv6Entry.dstAddrStart 	= GOS_Htonl(pRawEntry->ipv6Entry.dstAddrStart);
	pRawEntry->ipv6Entry.dstAddrStop 	= GOS_Htonl(pRawEntry->ipv6Entry.dstAddrStop);
	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!GOS_HtonByte(pRawEntry->msbAddr, MIB_TABLE_MULTIGEMIWTP_MSB_LEN)), GOS_FAIL);

	/*compare this entry is exist or not*/
	LIST_FOREACH(ptr, &pMultiGemIwTp->ipv6Head, entries)
	{
		if(pRawEntry->ipv6Entry.gemPort == ptr->tableEntry.ipv6Entry.gemPort &&
		   pRawEntry->ipv6Entry.secKey == ptr->tableEntry.ipv6Entry.secKey)
		{
			if(!memcmp(pRawEntry->msbAddr, zero, MIB_TABLE_MULTIGEMIWTP_MSB_LEN))
			{
				/*delete it*/
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete mcast addr entry");
				LIST_REMOVE(ptr, entries);
				free(ptr);
				pMultiGemIwTp->curIpv6EntryCnt--;
			}else
			{
				/* overwrite it */
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"modify mcast addr entry");
				ptr->tableEntry.ipv6Entry.dstAddrStart = pRawEntry->ipv6Entry.dstAddrStart;
				ptr->tableEntry.ipv6Entry.dstAddrStop = pRawEntry->ipv6Entry.dstAddrStop;
			}
			return GOS_OK;
		}
	}

	pEntry = (multiGemIwTpIpv6TableEntry_t *)malloc(sizeof(multiGemIwTpIpv6TableEntry_t));
	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pEntry), GOS_FAIL);
	memcpy(pEntry, &entry, sizeof(multiGemIwTpIpv6TableEntry_t));
	/*not found, create new entry*/
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add ExtVlanTable entry");
	LIST_INSERT_HEAD(&pMultiGemIwTp->ipv6Head, pEntry, entries);
	pMultiGemIwTp->curIpv6EntryCnt++;
	return GOS_OK;
}

static GOS_ERROR_CODE Ipv4McastAddrEntryOper(UINT8 *pMcastAddrEntry, MIB_TABLE_MULTIGEMIWTP_T *pMultiGemIwTp)
{
	multiGemIwTpIpv4TableEntry_t *pEntry = NULL, *ptr = NULL, entry;
	omci_ipv4_multicast_addr_entry_t *pRawEntry;

	memcpy(&(entry.tableEntry), (omci_ipv4_multicast_addr_entry_t *)pMcastAddrEntry, sizeof(omci_ipv4_multicast_addr_entry_t));

	pRawEntry = &entry.tableEntry;

	pRawEntry->ipv4Entry.gemPort 		= GOS_Htons(pRawEntry->ipv4Entry.gemPort);
	pRawEntry->ipv4Entry.secKey 		= GOS_Htons(pRawEntry->ipv4Entry.secKey);
	pRawEntry->ipv4Entry.dstAddrStart 	= GOS_Htonl(pRawEntry->ipv4Entry.dstAddrStart);
	pRawEntry->ipv4Entry.dstAddrStop 	= GOS_Htonl(pRawEntry->ipv4Entry.dstAddrStop);

	/*compare this entry is exist or not*/
	LIST_FOREACH(ptr, &pMultiGemIwTp->ipv4Head, entries)
	{
		if(pRawEntry->ipv4Entry.gemPort == ptr->tableEntry.ipv4Entry.gemPort &&
		   pRawEntry->ipv4Entry.secKey == ptr->tableEntry.ipv4Entry.secKey)
		{
			if(pRawEntry->ipv4Entry.dstAddrStart == 0 && pRawEntry->ipv4Entry.dstAddrStop == 0)
			{
				/*delete it*/
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"delete mcast addr entry");
				LIST_REMOVE(ptr, entries);
				free(ptr);
				pMultiGemIwTp->curIpv4EntryCnt--;
			}else
			{
				/* overwrite it */
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"modify mcast addr entry");
				ptr->tableEntry.ipv4Entry.dstAddrStart = pRawEntry->ipv4Entry.dstAddrStart;
				ptr->tableEntry.ipv4Entry.dstAddrStop = pRawEntry->ipv4Entry.dstAddrStop;
			}
			return GOS_OK;
		}
	}

	pEntry = (multiGemIwTpIpv4TableEntry_t *)malloc(sizeof(multiGemIwTpIpv4TableEntry_t));
	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pEntry), GOS_FAIL);
	memcpy(pEntry, &entry, sizeof(multiGemIwTpIpv4TableEntry_t));
	/*not found, create new entry*/
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"add ExtVlanTable entry");
	LIST_INSERT_HEAD(&pMultiGemIwTp->ipv4Head, pEntry, entries);
	pMultiGemIwTp->curIpv4EntryCnt++;
	return GOS_OK;
}

static GOS_ERROR_CODE McastAddrEntryClear(MIB_TABLE_MULTIGEMIWTP_T *pMultiGemIwTp)
{
	multiGemIwTpIpv4TableEntry_t *pIpv4Entry = NULL;
	multiGemIwTpIpv6TableEntry_t *pIpv6Entry = NULL;

	pIpv4Entry = LIST_FIRST(&pMultiGemIwTp->ipv4Head);
	while(NULL != pIpv4Entry)
	{
		LIST_REMOVE(pIpv4Entry, entries);
		free(pIpv4Entry);
		pIpv4Entry = LIST_FIRST(&pMultiGemIwTp->ipv4Head);
	}
	pMultiGemIwTp->curIpv4EntryCnt = 0;
	LIST_INIT(&pMultiGemIwTp->ipv4Head);


	pIpv6Entry = LIST_FIRST(&pMultiGemIwTp->ipv6Head);
	while(NULL != pIpv6Entry)
	{
		LIST_REMOVE(pIpv6Entry, entries);
		free(pIpv6Entry);
		pIpv6Entry = LIST_FIRST(&pMultiGemIwTp->ipv6Head);
	}
	pMultiGemIwTp->curIpv6EntryCnt = 0;
	LIST_INIT(&pMultiGemIwTp->ipv6Head);

	return GOS_OK;
}

#if 0
static MIB_TREE_T* MultiGemIwTpGetTree(MIB_TABLE_MULTIGEMIWTP_T *multiGemIwTp)
{
	MIB_TREE_T *pTree = NULL;
	/*find avl tree*/
	switch(multiGemIwTp->IwOpt){
	case GEM_IWTP_IW_OPTION_MAC_BRIDGED_LAN:
	{
		pTree = MIB_AvlTreeSearchByKey(NULL,multiGemIwTp->ServProPtr,AVL_KEY_MACBRISERVPROF);
	}
	break;
	case GEM_IWTP_IW_OPTION_IEEE_8021P_MAPPER:
	{
		pTree = MIB_AvlTreeSearchByKey(NULL,multiGemIwTp->ServProPtr,AVL_KEY_MAP8021PSERVPROF);
	}
	break;
	default:
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Not support TP type");
	break;
	}

	return pTree;
}
#endif

GOS_ERROR_CODE MultiGemIwTpDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	multiGemIwTpIpv4TableEntry_t *pEntryV4 = NULL;
	omci_ipv4_multicast_addr_entry_t *pRowEntryV4 = NULL;
	multiGemIwTpIpv6TableEntry_t *pEntryV6 = NULL;
	omci_ipv6_multicast_addr_entry_t *pRowEntryV6 = NULL;
	UINT16 count = 0;

	UINT8 *pAddr = (UINT8 *)malloc(sizeof(UINT8) * (MIB_TABLE_MULTIGEMIWTP_MSB_LEN + sizeof(UINT32)));
	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pAddr), GOS_FAIL);

	MIB_TABLE_MULTIGEMIWTP_T *multiGemIwTp = (MIB_TABLE_MULTIGEMIWTP_T*)pData;
	OMCI_PRINT("EntityID:\t %02x",multiGemIwTp->EntityID);
	OMCI_PRINT("GemCtpPtr:\t %02x",multiGemIwTp->GemCtpPtr);
	OMCI_PRINT("IwOpt:\t %d",multiGemIwTp->IwOpt);
	OMCI_PRINT("ServProPtr:\t %02x",multiGemIwTp->ServProPtr);
	OMCI_PRINT("PptpCounter:\t %d",multiGemIwTp->PptpCounter);
	OMCI_PRINT("OpState:\t %d",multiGemIwTp->OpState);
	OMCI_PRINT("GalProfPtr:\t %02x",multiGemIwTp->GalProfPtr);
	OMCI_PRINT("\n\nIpv4MCastAddrTable:");
	LIST_FOREACH(pEntryV4, &multiGemIwTp->ipv4Head, entries)
	{
		pRowEntryV4 = &pEntryV4->tableEntry;
		OMCI_PRINT("*************************************");
		OMCI_PRINT("INDEX:\t %u", count);
		OMCI_PRINT("Gem Port Id:\t %u", pRowEntryV4->ipv4Entry.gemPort);
		OMCI_PRINT("Sec Key:\t %u", pRowEntryV4->ipv4Entry.secKey);
		OMCI_PRINT("DIP Start: "IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntryV4->ipv4Entry.dstAddrStart));
		OMCI_PRINT("DIP Stop: "IPADDR_PRINT"", IPADDR_PRINT_ARG(pRowEntryV4->ipv4Entry.dstAddrStop));
		count++;
	}

	count = 0;

	OMCI_PRINT("\n\nIpv6MCastAddrTable:");
	LIST_FOREACH(pEntryV6, &multiGemIwTp->ipv6Head, entries)
	{
		pRowEntryV6 = &pEntryV6->tableEntry;
		OMCI_PRINT("**************************************");
		OMCI_PRINT("INDEX:\t %u", count);
		OMCI_PRINT("Gem Port Id:\t %u", pRowEntryV6->ipv6Entry.gemPort);
		OMCI_PRINT("Sec Key:\t %u", pRowEntryV6->ipv6Entry.secKey);

		memset(pAddr, 0, MIB_TABLE_MULTIGEMIWTP_MSB_LEN + sizeof(UINT32));
		memcpy(pAddr, pRowEntryV6->msbAddr, MIB_TABLE_MULTIGEMIWTP_MSB_LEN);
		memcpy(pAddr + MIB_TABLE_MULTIGEMIWTP_MSB_LEN, &(pRowEntryV6->ipv6Entry.dstAddrStart), sizeof(UINT32));
		OMCI_PRINT("IPv6 DIP Start: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pAddr));

		memset(pAddr, 0, MIB_TABLE_MULTIGEMIWTP_MSB_LEN + sizeof(UINT32));
		memcpy(pAddr, pRowEntryV6->msbAddr, MIB_TABLE_MULTIGEMIWTP_MSB_LEN);
		memcpy(pAddr + MIB_TABLE_MULTIGEMIWTP_MSB_LEN, &(pRowEntryV6->ipv6Entry.dstAddrStop), sizeof(UINT32));
		OMCI_PRINT("IPv6 DIP Stop: "IPADDRV6_PRINT"", IPADDRV6_PRINT_ARG(pAddr));
		count++;
	}
	free(pAddr);
	return GOS_OK;
}



GOS_ERROR_CODE MultiGemIwTpConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId,int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_MULTIGEMIWTP_T *multiGemIwTp;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);

	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_MULTIGEMIWTP,entityId);

	OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!pNodeEntry), GOS_FAIL);

	pEntry = pNodeEntry->mibEntry;
	multiGemIwTp = (MIB_TABLE_MULTIGEMIWTP_T*)pEntry->pData;
	pConn->pMcastGemIwTp = multiGemIwTp;

	return OMCI_MeOperConnCheck(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pConn,multiGemIwTp->GemCtpPtr,parm);

	return GOS_OK;
}

GOS_ERROR_CODE MultiGemIwTpDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);

    MIB_TABLE_MULTIGEMIWTP_T *pNewMcastGemIwTp = NULL, *pMibMcastGemIwTp = NULL, entry;
	MIB_TABLE_GEMPORTCTP_T gemPortCtp;
	OMCI_GEM_FLOW_ts flow;
	GOS_ERROR_CODE ret = GOS_OK;

    pNewMcastGemIwTp = (MIB_TABLE_MULTIGEMIWTP_T *)pNewRow;

	entry.EntityID = pNewMcastGemIwTp->EntityID;

	if(FALSE == mib_FindEntry(MIB_TABLE_MULTIGEMIWTP_INDEX, &entry, &pMibMcastGemIwTp)) return GOS_OK;

	memset(&gemPortCtp, 0, sizeof(MIB_TABLE_GEMPORTCTP_T));
	gemPortCtp.EntityID = pNewMcastGemIwTp->GemCtpPtr;

	if(GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &gemPortCtp, sizeof(MIB_TABLE_GEMPORTCTP_T))) return GOS_OK;

	flow.portId = gemPortCtp.PortID;
	flow.dir = (PON_GEMPORT_DIRECTION)gemPortCtp.Direction;
	flow.isOmcc = FALSE;
    switch (operationType)
	{
        case MIB_SET:
			flow.ena = TRUE;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "MultiGemIwTpDrvCfg ---- > SET");
			if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX) ||
				MIB_IsInAttrSet(&attrSet, MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX))
			{
				if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX))
				{
					OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_FAIL == Ipv4McastAddrEntryOper(pNewMcastGemIwTp->Ipv4MCastAddrTable, pMibMcastGemIwTp)), GOS_FAIL);
				}
				else
				{
					OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (GOS_FAIL == Ipv6McastAddrEntryOper(pNewMcastGemIwTp->Ipv6MCastAddrTable, pMibMcastGemIwTp)), GOS_FAIL);
				}
				if(1 == pMibMcastGemIwTp->curIpv4EntryCnt ||
					1 == pMibMcastGemIwTp->curIpv6EntryCnt)
				{
					flow.isFilterMcast = TRUE;
					ret = omci_wrapper_cfgGemFlow(flow);
				}
				else if(0 == pMibMcastGemIwTp->curIpv4EntryCnt &&
					0 == pMibMcastGemIwTp->curIpv6EntryCnt)
				{
					flow.isFilterMcast = FALSE;
					ret = omci_wrapper_cfgGemFlow(flow);
				}
				/*TODO: Need to implement if chip support filtering ip range */
			}
            break;
        case MIB_DEL:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "MultiGemIwTpDrvCfg --> DEL");
			McastAddrEntryClear(pMibMcastGemIwTp);

			break;
        default:
            return GOS_OK;
    }
	return ret;

}

GOS_ERROR_CODE McastGemIwTpAvlTreeAdd(MIB_TREE_T *pTree,UINT16 mcastId)
{

	MIB_TABLE_MULTIGEMIWTP_T *pMcastGemIwTp;

	if(pTree==NULL) return GOS_FAIL;

	pMcastGemIwTp = (MIB_TABLE_MULTIGEMIWTP_T*)malloc(sizeof(MIB_TABLE_MULTIGEMIWTP_T));
	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pMcastGemIwTp), GOS_FAIL);

	memset(pMcastGemIwTp,0,sizeof(MIB_TABLE_MULTIGEMIWTP_T));
	pMcastGemIwTp->EntityID = mcastId;
	if(MIB_Get(MIB_TABLE_MULTIGEMIWTP_INDEX, pMcastGemIwTp, sizeof(MIB_TABLE_MULTIGEMIWTP_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get McastGemIwTp Fail");
		free(pMcastGemIwTp);
		return GOS_FAIL;
	}

	if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_MULTIGEMIWTP,MIB_TABLE_MULTIGEMIWTP_INDEX,pMcastGemIwTp)==NULL)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add McastGemIwTp Node Fail");
		free(pMcastGemIwTp);
		return GOS_FAIL;
	}

	OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pMcastGemIwTp->GemCtpPtr);

	return GOS_OK;
}

GOS_ERROR_CODE McastGemIwTpAvlTreeDel(MIB_TREE_T *pTree, UINT16 mcastId)
{
	MIB_TABLE_MULTIGEMIWTP_T	mibMcastGemIwTp;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;

	if (NULL == pTree)
        return GOS_FAIL;

	mibMcastGemIwTp.EntityID = mcastId;
	if (GOS_OK != MIB_Get(MIB_TABLE_MULTIGEMIWTP_INDEX, &mibMcastGemIwTp, sizeof(mibMcastGemIwTp)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MULTIGEMIWTP_INDEX), mibMcastGemIwTp.EntityID);

        return GOS_FAIL;
    }

    if ((pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_EXTVLAN_ANI)))
    {
        LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            if (!(pEntry = pNodeEntry->mibEntry))
                continue;

            if (!(pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData))
                continue;

            if (EVTOCD_ASSOC_TYPE_MCAST_GEM_IWTP == pExtVlan->AssociationType &&
                mcastId == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_ANI, pExtVlan->EntityId);
                break;
            }
        }
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_MULTIGEMIWTP, mibMcastGemIwTp.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MULTIGEMIWTP_INDEX), mibMcastGemIwTp.EntityID, pTree);

        return GOS_FAIL;
    }

    // delete gemport network ctp
	OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMPORTCTP_INDEX, pTree, mibMcastGemIwTp.GemCtpPtr);

	return GOS_OK;
}

GOS_ERROR_CODE MultiGemIwTpConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_MULTIGEMIWTP_T*  	   multiGemIwTp;
	MIB_TREE_T *pTree;
	int key = AVL_KEY_MULTIGEMIWTP;
    MIB_NODE_T  *pAniNode;
    MIB_TREE_NODE_ENTRY_T *pAniDataEntry;
    MIB_ENTRY_T *pAniEntry;
    MIB_TREE_DATA_T *pAniData;
    MIB_TABLE_MACBRIPORTCFGDATA_T *pAniPort;

	/*check if new connection is complete*/
	multiGemIwTp = (MIB_TABLE_MULTIGEMIWTP_T *)pNewRow;

	if (!(pTree = MIB_AvlTreeSearchByKey(NULL, multiGemIwTp->EntityID, AVL_KEY_MULTIGEMIWTP)))
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
		return GOS_FAIL;
	}

	switch (operationType)
	{
		case MIB_ADD:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Multicast GemIwTp Configure ---- > ADD");
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_MULTIGEMIWTP_INDEX,multiGemIwTp)), GOS_FAIL);
			/*update for gemport ctp*/
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,multiGemIwTp->GemCtpPtr);
		break;
		case MIB_SET:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Multicast Configure ---- > SET");
			break;
		case MIB_DEL:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Multicast Configure ---- > DEL");

        	if (!(pAniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_ANI)))
        	{
        		OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: can't find MacBridgePortData on ANI side", __FUNCTION__);
        		goto finish;
        	}
        	pAniData = &pAniNode->data;
            LIST_FOREACH(pAniDataEntry,&pAniData->treeNodeEntryHead, treeNodeEntry)
            {
        		pAniEntry = pAniDataEntry->mibEntry;
        		pAniPort = (MIB_TABLE_MACBRIPORTCFGDATA_T*) pAniEntry->pData;

                if (MBPCD_TP_TYPE_MCAST_GEM_IWTP == pAniPort->TPType &&
                    pAniPort->TPPointer == multiGemIwTp->EntityID)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ANI port =%x", __FUNCTION__, pAniPort->EntityID);
                    MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_MACBRIPORT_ANI, pAniPort->EntityID);
                    break;
                }
        	}
        finish:
			MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, key, multiGemIwTp->EntityID);
			break;
		default:
			break;
	}
	/*check connection*/
	MIB_TreeConnUpdate(pTree);

	return GOS_OK;
}

static GOS_ERROR_CODE mc_gem_iwtp_check_cb(MIB_TABLE_INDEX  tableIndex,
		                                    void            *pOldRow,
		                                    void            *pNewRow,
		                                    MIB_ATTRS_SET   *pAttrsSet,
		                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;

    if (!pAttrsSet)
        return GOS_ERR_PARAM;

    if (MIB_SET == operationType)
    {
		if (!pOldRow || !pNewRow)
        	return GOS_ERR_PARAM;
    }
    else if (MIB_DEL == operationType)
    {
		if (!pOldRow)
        	return GOS_ERR_PARAM;
    }
    else if (MIB_ADD == operationType)
    {
		if (!pNewRow)
        	return GOS_ERR_PARAM;
    }
    else
    	return GOS_OK;

    avcAttrSet = *pAttrsSet;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
            i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
            continue;


        // change op state according to GEM port network CTP pointer
        if (MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX == attrIndex)
        {
            if (!pNewRow || !pOldRow)
            {
            	UINT8 opState;

            	opState = pNewRow ? OMCI_ME_ATTR_OP_STATE_ENABLED : OMCI_ME_ATTR_OP_STATE_DISABLED;

                MIB_SetAttrToBuf(tableIndex, MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX, &opState,
                	pNewRow ? pNewRow : pOldRow, MIB_GetAttrSize(tableIndex, MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX));
                MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

static GOS_ERROR_CODE mc_gem_iwtp_avc_cb(MIB_TABLE_INDEX	tableIndex,
											void            *pOldRow,
		                                    void            *pNewRow,
		                                    MIB_ATTRS_SET   *pAttrsSet,
		                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX		attrIndex;
    UINT32				i;
    MIB_ATTRS_SET		avcAttrSet;
    BOOL                isSuppressed;
    omci_me_instance_t	instanceID;

    if (!pAttrsSet)
        return GOS_ERR_PARAM;

    if (MIB_SET == operationType)
    {
		if (!pOldRow || !pNewRow)
        	return GOS_ERR_PARAM;
    }
    else if (MIB_DEL == operationType)
    {
		if (!pOldRow)
        	return GOS_ERR_PARAM;
    }
    else if (MIB_ADD == operationType)
    {
		if (!pNewRow)
        	return GOS_ERR_PARAM;
    }
    else
    	return GOS_OK;

    // correct the instanceID to the slot it belongs to
    instanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | TXC_CARDHLD_PON_SLOT_TYPE_ID;

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(instanceID, &isSuppressed);

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
	{
		avcAttrSet = *pAttrsSet;

	    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
	            i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
	    {
	        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
	            continue;
	    }
	}

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMultiGemIwTpTableInfo.Name = "MultiGemIwTp";
    gMibMultiGemIwTpTableInfo.ShortName = "MCGEMIWTP";
    gMibMultiGemIwTpTableInfo.Desc = "Multicast GEM Interworking TP";
    gMibMultiGemIwTpTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MCAST_GEM_IWTP);
    gMibMultiGemIwTpTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMultiGemIwTpTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMultiGemIwTpTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMultiGemIwTpTableInfo.pAttributes = &(gMibMultiGemIwTpAttrInfo[0]);

	gMibMultiGemIwTpTableInfo.attrNum = MIB_TABLE_MULTIGEMIWTP_ATTR_NUM;
	gMibMultiGemIwTpTableInfo.entrySize = sizeof(MIB_TABLE_MULTIGEMIWTP_T);
	gMibMultiGemIwTpTableInfo.pDefaultRow = &gMibMultiGemIwTpDefRow;



    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GemCtpPtr";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwOpt";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ServProPtr";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtr";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PptpCounter";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpState";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GalProfPtr";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GalLoopbackCfg";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IPv4MCastAddrTable";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IPv6MCastAddrTable";

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GEM Port Network CTP Connectivity Pointer";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interworking Option";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Service Profile Pointer";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interworking Termination Point Pointer";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PPTP Counter";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational state";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GAL Profile Pointer";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GAL Loopback Configuration";
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Ipv4MulticastAddressTable";
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Ipv6MulticastAddressTable";

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_LEN;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_LEN;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_TABLE);
	gMibMultiGemIwTpAttrInfo[MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_OPTIONAL | OMCI_ME_ATTR_TYPE_TABLE);


    memset(&(gMibMultiGemIwTpDefRow.EntityID), 0x00, sizeof(gMibMultiGemIwTpDefRow.EntityID));
    memset(&(gMibMultiGemIwTpDefRow.GemCtpPtr), 0x00, sizeof(gMibMultiGemIwTpDefRow.GemCtpPtr));
    memset(&(gMibMultiGemIwTpDefRow.IwOpt), 0x00, sizeof(gMibMultiGemIwTpDefRow.IwOpt));
    memset(&(gMibMultiGemIwTpDefRow.ServProPtr), 0x00, sizeof(gMibMultiGemIwTpDefRow.ServProPtr));
    memset(&(gMibMultiGemIwTpDefRow.IwTpPtr), 0x00, sizeof(gMibMultiGemIwTpDefRow.IwTpPtr));
    memset(&(gMibMultiGemIwTpDefRow.PptpCounter), 0x00, sizeof(gMibMultiGemIwTpDefRow.PptpCounter));
    gMibMultiGemIwTpDefRow.OpState = OMCI_ME_ATTR_OP_STATE_DISABLED;
    memset(&(gMibMultiGemIwTpDefRow.GalProfPtr), 0x00, sizeof(gMibMultiGemIwTpDefRow.GalProfPtr));
    memset(&(gMibMultiGemIwTpDefRow.GalLoopbackCfg), 0x00, sizeof(gMibMultiGemIwTpDefRow.GalLoopbackCfg));
    memset(gMibMultiGemIwTpDefRow.Ipv4MCastAddrTable, 0, MIB_TABLE_MULTIGEMIWTP_IPV4MCASTADDRTABLE_LEN);
    memset(gMibMultiGemIwTpDefRow.Ipv6MCastAddrTable, 0, MIB_TABLE_MULTIGEMIWTP_IPV6MCASTADDRTABLE_LEN);

    gMibMultiGemIwTpDefRow.curIpv4EntryCnt = 0;
	gMibMultiGemIwTpDefRow.curIpv6EntryCnt = 0;

    LIST_INIT(&gMibMultiGemIwTpDefRow.ipv4Head);
    LIST_INIT(&gMibMultiGemIwTpDefRow.ipv6Head);

    memset(&gMibMultiGemIwTpOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibMultiGemIwTpOper.meOperDrvCfg 	= MultiGemIwTpDrvCfg;
	gMibMultiGemIwTpOper.meOperDump = MultiGemIwTpDumpMib;
	gMibMultiGemIwTpOper.meOperConnCheck = MultiGemIwTpConnCheck;
	gMibMultiGemIwTpOper.meOperConnCfg = MultiGemIwTpConnCfg;
	gMibMultiGemIwTpOper.meOperAvlTreeAdd = McastGemIwTpAvlTreeAdd;
	gMibMultiGemIwTpOper.meOperAvlTreeDel = McastGemIwTpAvlTreeDel;
	MIB_TABLE_MULTIGEMIWTP_INDEX = tableId;

	MIB_InfoRegister(tableId,&gMibMultiGemIwTpTableInfo,&gMibMultiGemIwTpOper);
	MIB_RegisterCallback(tableId, mc_gem_iwtp_check_cb, mc_gem_iwtp_avc_cb);

    return GOS_OK;
}
