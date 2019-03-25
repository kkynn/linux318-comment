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
 * Purpose : Definition of ME handler: GEM IWTP (266)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: GEM IWTP (266)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibGemIwTpTableInfo;
MIB_ATTR_INFO_T  gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ATTR_NUM];
MIB_TABLE_GEMIWTP_T gMibGemIwTpDefRow;
MIB_TABLE_OPER_T	gMibGemIwTpOper;


extern GOS_ERROR_CODE MIB_TreeConnUpdate(MIB_TREE_T *pTree);


GOS_ERROR_CODE GemIwTpAvlTreeAdd(MIB_TREE_T* pTree,UINT16 gemIwTpPtr)
{

	MIB_TABLE_GEMIWTP_T* pGemIwTp;

	if(pTree==NULL) return GOS_FAIL;

	pGemIwTp = (MIB_TABLE_GEMIWTP_T*)malloc(sizeof(MIB_TABLE_GEMIWTP_T));
	if(!pGemIwTp)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Malloc GemIwTp Fail");
		return GOS_FAIL;
	}
	memset(pGemIwTp,0,sizeof(MIB_TABLE_GEMIWTP_T));
	pGemIwTp->EntityID = gemIwTpPtr;
	if(MIB_Get(MIB_TABLE_GEMIWTP_INDEX, pGemIwTp, sizeof(MIB_TABLE_GEMIWTP_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get GemIwTp Fail");
		free(pGemIwTp);
		return GOS_FAIL;
	}
#if 0
	/*Temp soulution for Broadcast DS GemIWTP will be link to multiple AVL Tree to cause treeEntry error*/
	if(6 == pGemIwTp->IwOpt)
	{
		if(MIB_AvlTreeNodeAdd2Tail(&pTree->root,AVL_KEY_GEMIWTP,MIB_TABLE_GEMIWTP_INDEX,pGemIwTp)==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add GemIwTp Node Fail");
			free(pGemIwTp);
			return GOS_FAIL;
		}
		/*update for gemport ctp*/
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pGemIwTp->GemCtpPtr);

		return GOS_OK;
	}
#endif
	if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_GEMIWTP,MIB_TABLE_GEMIWTP_INDEX,pGemIwTp)==NULL)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add GemIwTp Node Fail");
		free(pGemIwTp);
		return GOS_FAIL;
	}

	/*update for gemport ctp*/
	OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pGemIwTp->GemCtpPtr);
	return GOS_OK;
}

GOS_ERROR_CODE GemIwTpAvlTreeDel(MIB_TREE_T *pTree, UINT16 gemIwTpPtr)
{
	MIB_TABLE_GEMIWTP_T		mibGemIwTp;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;

	if (NULL == pTree)
        return GOS_FAIL;

	mibGemIwTp.EntityID = gemIwTpPtr;
	if (GOS_OK != MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMIWTP_INDEX), mibGemIwTp.EntityID);

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

            if (EVTOCD_ASSOC_TYPE_GEM_IWTP == pExtVlan->AssociationType &&
                gemIwTpPtr == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_ANI, pExtVlan->EntityId);
                break;
            }
        }
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_GEMIWTP, mibGemIwTp.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMIWTP_INDEX), mibGemIwTp.EntityID, pTree);

        return GOS_FAIL;
    }

	// delete gemport network ctp
	OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMPORTCTP_INDEX, pTree, mibGemIwTp.GemCtpPtr);

	return GOS_OK;
}

static MIB_TREE_T* GemIwTpGetTree(MIB_TABLE_GEMIWTP_T* pGemIwTp)
{
	 MIB_TREE_T *pTree;

	 switch(pGemIwTp->IwOpt)
	 {
	 	case GEM_IWTP_IW_OPTION_IEEE_8021P_MAPPER:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Search Tree by 8021P mapper ID %x",pGemIwTp->ServProPtr);
			pTree = MIB_AvlTreeSearchByKey(NULL, pGemIwTp->ServProPtr,AVL_KEY_MAP8021PSERVPROF);
		break;
		case GEM_IWTP_IW_OPTION_MAC_BRIDGED_LAN:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Search Tree by Bridge ID %x",pGemIwTp->ServProPtr);
			pTree = MIB_AvlTreeSearchByKey(NULL, pGemIwTp->ServProPtr,AVL_KEY_MACBRISERVPROF);
		break;
		default:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Cant' support IwOpt %x",pGemIwTp->IwOpt);
			return NULL;
		break;
	 }
	 return pTree;
}

GOS_ERROR_CODE GemIwTpConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId,int parm)
{
	MIB_TABLE_GEMIWTP_T *gemIwTp;
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;


	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...,index=%d",__FUNCTION__,parm);
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_GEMIWTP,entityId);
	if(pNodeEntry==NULL) return GOS_FAIL;

    if((OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode && parm>=8)||
       (OMCI_TRAF_MODE_FLOW_BASE == pConn->traffMode && parm!=0))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s:traffMode:%d para:%d error\n",
            __FUNCTION__,pConn->traffMode,parm);
        return GOS_FAIL;
    }
	pEntry = pNodeEntry->mibEntry;
	gemIwTp = (MIB_TABLE_GEMIWTP_T*)pEntry->pData;
	pConn->pGemIwTp[parm] = gemIwTp;
	pConn->pMcastGemIwTp = NULL;

	return OMCI_MeOperConnCheck(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pConn,gemIwTp->GemCtpPtr,parm);
}

GOS_ERROR_CODE GemIwTpDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE GemIwTpConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType,  MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_GEMPORTCTP_T mibGPNC;
	 MIB_TABLE_GEMIWTP_T *pGemIwTp;
	 MIB_TREE_T *pTree;
	 MIB_AVL_KEY_T key;


	 switch (operationType)
	 {
		 case MIB_ADD:
		 {
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Gem InterWorking TP ---- > ADD");

			pGemIwTp = (MIB_TABLE_GEMIWTP_T*)pNewRow;

			pTree =GemIwTpGetTree(pGemIwTp);

			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_OK;
			}
			/*add new node to tree*/
			key = AVL_KEY_GEMIWTP;

			// update priq for veip
			mibGPNC.EntityID = pGemIwTp->GemCtpPtr;
		    if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX,
		    		&mibGPNC, MIB_GetTableEntrySize(MIB_TABLE_GEMPORTCTP_INDEX)))
		    {
		    	if (GPNC_DIRECTION_ANI_TO_UNI != mibGPNC.Direction)
		        	omci_wrapper_setUsVeipPriQ(pTree, mibGPNC.PortID, TRUE);
		    }

			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"key is %d",key);
			if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_GEMIWTP_INDEX,pGemIwTp)==NULL)
			{
				return GOS_OK;
			}
			/*update for gemport ctp*/
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pGemIwTp->GemCtpPtr);
			break;
		 }
		 case MIB_SET:
		 {
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Gem InterWorking TP ---- > SET");

			pGemIwTp = (MIB_TABLE_GEMIWTP_T*)pNewRow;

			pTree =GemIwTpGetTree(pGemIwTp);

			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_FAIL;
			}
			/*update for gemport ctp*/
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMPORTCTP_INDEX,pTree,pGemIwTp->GemCtpPtr);
			break;
		 }
		 case MIB_DEL:
		 {
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Gem InterWorking TP ---- > DEL");
			pGemIwTp = (MIB_TABLE_GEMIWTP_T*)pOldRow;
			pTree = MIB_AvlTreeSearchByKey(NULL,pGemIwTp->EntityID,AVL_KEY_GEMIWTP);

			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_FAIL;
			}

			// update priq for veip
			mibGPNC.EntityID = pGemIwTp->GemCtpPtr;
		    if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX,
		    		&mibGPNC, MIB_GetTableEntrySize(MIB_TABLE_GEMPORTCTP_INDEX)))
		    {
		    	if (GPNC_DIRECTION_ANI_TO_UNI != mibGPNC.Direction)
		        	omci_wrapper_setUsVeipPriQ(pTree, mibGPNC.PortID, FALSE);
		    }

			/*remove entries*/
			MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_GEMIWTP, pGemIwTp->EntityID);
			break;
		 }
		 default:
			 return GOS_OK;
	 }

	 /*check connection*/
	 MIB_TreeConnUpdate(pTree);

	 return GOS_OK;

}

static GOS_ERROR_CODE gem_iwtp_check_cb(MIB_TABLE_INDEX		tableIndex,
	                                    void            	*pOldRow,
	                                    void            	*pNewRow,
	                                    MIB_ATTRS_SET   	*pAttrsSet,
	                                    MIB_OPERA_TYPE  	operationType)
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

        MIB_GetAttrSize(tableIndex, attrIndex);

        // change op state according to GEM port network CTP pointer
        if (MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX == attrIndex)
        {
            if (!pNewRow || !pOldRow)
            {
            	UINT8 opState;

            	opState = pNewRow ? OMCI_ME_ATTR_OP_STATE_ENABLED : OMCI_ME_ATTR_OP_STATE_DISABLED;

                MIB_SetAttrToBuf(tableIndex, MIB_TABLE_GEMIWTP_OPSTATE_INDEX, &opState,
                	pNewRow ? pNewRow : pOldRow, MIB_GetAttrSize(tableIndex, MIB_TABLE_GEMIWTP_OPSTATE_INDEX));
                MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_GEMIWTP_OPSTATE_INDEX);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

static GOS_ERROR_CODE gem_iwtp_avc_cb(MIB_TABLE_INDEX	tableIndex,
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

	        MIB_GetAttrSize(tableIndex, attrIndex);
	    }
	}

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibGemIwTpTableInfo.Name = "GemIwTp";
    gMibGemIwTpTableInfo.ShortName = "GEMIWTP";
    gMibGemIwTpTableInfo.Desc = "GEM Interworking TP";
    gMibGemIwTpTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_GEM_IWTP);
    gMibGemIwTpTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibGemIwTpTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibGemIwTpTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibGemIwTpTableInfo.pAttributes = &(gMibGemIwTpAttrInfo[0]);


	gMibGemIwTpTableInfo.attrNum = MIB_TABLE_GEMIWTP_ATTR_NUM;
	gMibGemIwTpTableInfo.entrySize = sizeof(MIB_TABLE_GEMIWTP_T);
	gMibGemIwTpTableInfo.pDefaultRow = &gMibGemIwTpDefRow;


    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GemCtpPtr";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwOpt";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ServProPtr";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtr";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PptpCounter";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpState";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GalProfPtr";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "GalLoopbackCfg";

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GEM Port Network CTP Connectivity Pointer";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interworking Option";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Service Profile Pointer";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interworking Termination Point Pointer";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PPTP Counter";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational state";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GAL Profile Pointer";
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GAL Loopback Configuration";

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GEMCTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWOPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_SERVPROPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_PPTPCOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALPROFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemIwTpAttrInfo[MIB_TABLE_GEMIWTP_GALLOOPBACKCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;


    memset(&(gMibGemIwTpDefRow.EntityID), 0x00, sizeof(gMibGemIwTpDefRow.EntityID));
    memset(&(gMibGemIwTpDefRow.GemCtpPtr), 0x00, sizeof(gMibGemIwTpDefRow.GemCtpPtr));
    memset(&(gMibGemIwTpDefRow.IwOpt), 0x00, sizeof(gMibGemIwTpDefRow.IwOpt));
    memset(&(gMibGemIwTpDefRow.ServProPtr), 0x00, sizeof(gMibGemIwTpDefRow.ServProPtr));
    memset(&(gMibGemIwTpDefRow.IwTpPtr), 0x00, sizeof(gMibGemIwTpDefRow.IwTpPtr));
    memset(&(gMibGemIwTpDefRow.PptpCounter), 0x00, sizeof(gMibGemIwTpDefRow.PptpCounter));
    gMibGemIwTpDefRow.OpState = OMCI_ME_ATTR_OP_STATE_DISABLED;
    memset(&(gMibGemIwTpDefRow.GalProfPtr), 0x00, sizeof(gMibGemIwTpDefRow.GalProfPtr));
    memset(&(gMibGemIwTpDefRow.GalLoopbackCfg), 0x00, sizeof(gMibGemIwTpDefRow.GalLoopbackCfg));

    memset(&gMibGemIwTpOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibGemIwTpOper.meOperDrvCfg = GemIwTpDrvCfg;
	gMibGemIwTpOper.meOperConnCheck = GemIwTpConnCheck;
	gMibGemIwTpOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibGemIwTpOper.meOperConnCfg = GemIwTpConnCfg;
	gMibGemIwTpOper.meOperAvlTreeAdd = GemIwTpAvlTreeAdd;
	gMibGemIwTpOper.meOperAvlTreeDel = GemIwTpAvlTreeDel;

	MIB_TABLE_GEMIWTP_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibGemIwTpTableInfo,&gMibGemIwTpOper);
	MIB_RegisterCallback(tableId, gem_iwtp_check_cb, gem_iwtp_avc_cb);

    return GOS_OK;
}
