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
 * Purpose : Definition of ME handler: IEEE 802.1p mapper service profile (130)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: IEEE 802.1p mapper service profile (130)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibMap8021pServProfTableInfo;
MIB_ATTR_INFO_T  gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ATTR_NUM];
MIB_TABLE_MAP8021PSERVPROF_T gMibMap8021pServProfDefRow;
MIB_TABLE_OPER_T gMibMap8021pServProfOper;

static BOOL Map8021pCheckOneGemIwTp(MIB_TREE_T *pTree,MIB_TABLE_MAP8021PSERVPROF_T *pMap802, UINT16 *pGemPortID)
{
    UINT16                          *pIwtpPtrOffset = NULL, entities[2] = {0xFFFF, 0xFFFF};
    UINT8                           i, j, cnt = 0;
    MIB_TABLE_GEMIWTP_T             mibGemIwTp;
    MIB_TABLE_GEMPORTCTP_T          mibGPNC;

    if (!pTree || !pMap802)
        return FALSE;


    pIwtpPtrOffset = &(pMap802->IwTpPtrPbit0);

	for (i = 0; i < 8; i++)
    {
        if (*(pIwtpPtrOffset + i) == 0xFFFF)
            continue;

        for (j = i + 1; j < 8; j++)
        {
            if (*(pIwtpPtrOffset + j) == 0xFFFF)
                continue;

            entities[0] = (*(pIwtpPtrOffset + i));
            entities[1] = (*(pIwtpPtrOffset + j));

            if (entities[0] != entities[1])
                cnt++;
        }
    }

    if (cnt != 0)
        return FALSE;

    mibGemIwTp.EntityID = entities[0];

    if (GOS_OK != MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
        return FALSE;

    mibGPNC.EntityID = mibGemIwTp.GemCtpPtr;

    if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
        return FALSE;

    *pGemPortID = mibGPNC.PortID;
    return TRUE;
}

static MIB_TREE_T* Map8021pServProfGetTree(MIB_TABLE_MAP8021PSERVPROF_T* pMap8021pServProf)
{
	MIB_TREE_T *pTree;
	MIB_TABLE_MACBRIPORTCFGDATA_T port;
	int size,count=0,total=0;

	pTree = MIB_AvlTreeSearchByKey(NULL, pMap8021pServProf->EntityID,AVL_KEY_MAP8021PSERVPROF);

	if(pTree) return pTree;

	size = sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T);

	if((MIB_GetFirst(MIB_TABLE_MACBRIPORTCFGDATA_INDEX,&port,size))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"get first fail");
		return NULL;
	}
	total = MIB_GetTableCurEntryCount(MIB_TABLE_MACBRIPORTCFGDATA_INDEX);
	while(count < total)
	{
		if(port.TPType == MBPCD_TP_TYPE_IEEE_8021P_MAPPER && port.TPPointer == pMap8021pServProf->EntityID)
		{
			pTree = MIB_AvlTreeSearchByKey(NULL, port.EntityID,AVL_KEY_MACBRIPORT_ANI);
			break;
		}
		if((MIB_GetNext(MIB_TABLE_MACBRIPORTCFGDATA_INDEX,&port,size))!=GOS_OK)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"get next fail,count=%d",count);
			return NULL;
		}
		count ++;
	}

	return pTree;
}


static void Map8021pUpdateGemIwTp(MIB_TREE_T *pTree,MIB_TABLE_MAP8021PSERVPROF_T *pMap802)
{

	if(pMap802->IwTpPtrPbit0 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x",__FUNCTION__,0,pMap802->IwTpPtrPbit0);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit0);
	}
	if(pMap802->IwTpPtrPbit1 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,1,pMap802->IwTpPtrPbit1);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit1);
	}
	if(pMap802->IwTpPtrPbit2 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,2,pMap802->IwTpPtrPbit2);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit2);
	}
	if(pMap802->IwTpPtrPbit3 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,3,pMap802->IwTpPtrPbit3);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit3);
	}
	if(pMap802->IwTpPtrPbit4 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,4,pMap802->IwTpPtrPbit4);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit4);
	}
	if(pMap802->IwTpPtrPbit5 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,5,pMap802->IwTpPtrPbit5);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit5);
	}
	if(pMap802->IwTpPtrPbit6 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,6,pMap802->IwTpPtrPbit6);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit6);
	}
	if(pMap802->IwTpPtrPbit7 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] update GemIwTp %x\n",__FUNCTION__,7,pMap802->IwTpPtrPbit7);
		OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX,pTree,pMap802->IwTpPtrPbit7);
	}
}

GOS_ERROR_CODE Map8021pAvlTreeAdd(MIB_TREE_T* pTree,UINT16 map802Ptr)
{

	MIB_TABLE_MAP8021PSERVPROF_T* pMap802;

	if(pTree==NULL) return GOS_FAIL;

	pMap802 = (MIB_TABLE_MAP8021PSERVPROF_T*)malloc(sizeof(MIB_TABLE_MAP8021PSERVPROF_T));
	if(!pMap802)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Malloc Map802 Fail");
		return GOS_FAIL;
	}
	memset(pMap802,0,sizeof(MIB_TABLE_MAP8021PSERVPROF_T));
	pMap802->EntityID = map802Ptr;
	if(MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, pMap802, sizeof(MIB_TABLE_MAP8021PSERVPROF_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get Map802 Fail");
		free(pMap802);
		return GOS_FAIL;
	}

	if(MIB_Set(MIB_TABLE_MAP8021PSERVPROF_INDEX, pMap802, sizeof(MIB_TABLE_MAP8021PSERVPROF_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Set Map802 Fail");
		free(pMap802);
		return GOS_FAIL;
	}

	if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_MAP8021PSERVPROF,MIB_TABLE_MAP8021PSERVPROF_INDEX,pMap802)==NULL)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add Map802 Node Fail");
		free(pMap802);
		return GOS_FAIL;
	}

	Map8021pUpdateGemIwTp(pTree, pMap802);

	return GOS_OK;
}

static GOS_ERROR_CODE Map8021pDeleteGemIwTp(MIB_TREE_T *pTree, MIB_TABLE_MAP8021PSERVPROF_T *pMap802)
{
	if (NULL == pTree || NULL == pMap802)
        return GOS_ERR_PARAM;

	if (pMap802->IwTpPtrPbit0 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,0,pMap802->IwTpPtrPbit1);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit0);
	}
	if (pMap802->IwTpPtrPbit1 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,1,pMap802->IwTpPtrPbit1);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit1);
	}
	if (pMap802->IwTpPtrPbit2 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,2,pMap802->IwTpPtrPbit2);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit2);
	}
	if (pMap802->IwTpPtrPbit3 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,3,pMap802->IwTpPtrPbit3);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit3);
	}
	if (pMap802->IwTpPtrPbit4 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,4,pMap802->IwTpPtrPbit4);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit4);
	}
	if (pMap802->IwTpPtrPbit5 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,5,pMap802->IwTpPtrPbit5);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit5);
	}
	if (pMap802->IwTpPtrPbit6 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,6,pMap802->IwTpPtrPbit6);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit6);
	}
	if (pMap802->IwTpPtrPbit7 != 0xffff)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: [%d] delete GemIwTp %x\n",__FUNCTION__,7,pMap802->IwTpPtrPbit7);
		OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pMap802->IwTpPtrPbit7);
	}

	return GOS_OK;
}

GOS_ERROR_CODE Map8021pAvlTreeDel(MIB_TREE_T *pTree, UINT16 map802Ptr)
{
	MIB_TABLE_MAP8021PSERVPROF_T	mib8021pmsp;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;

	if (NULL == pTree)
        return GOS_FAIL;

	mib8021pmsp.EntityID = map802Ptr;
	if (GOS_OK != MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib8021pmsp, sizeof(mib8021pmsp)))
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MAP8021PSERVPROF_INDEX), mib8021pmsp.EntityID);

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

            if (EVTOCD_ASSOC_TYPE_IEEE_8021P_MAPPER == pExtVlan->AssociationType &&
                map802Ptr == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_ANI, pExtVlan->EntityId);
                break;
            }
        }
    }

	if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_MAP8021PSERVPROF, mib8021pmsp.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_MAP8021PSERVPROF_INDEX), mib8021pmsp.EntityID, pTree);

        return GOS_FAIL;
    }

    Map8021pDeleteGemIwTp(pTree, &mib8021pmsp);

	return GOS_OK;
}

static void ieee8021pmsp_dscp_to_pbit_mapping_dump(UINT8 *pDscpToPbitMapping)
{
	unsigned char i;

	if (!pDscpToPbitMapping)
		return;

	OMCI_PRINT("DscpMap2Pbit:");
	for (i = 0; i < 24; i += 3)
	{
		OMCI_PRINT("\t0x%02x%02x%02x",
			pDscpToPbitMapping[i],
			pDscpToPbitMapping[i+1],
			pDscpToPbitMapping[i+2]);
	}
}

GOS_ERROR_CODE Map8021pServProfDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_MAP8021PSERVPROF_T *p8021Map = (MIB_TABLE_MAP8021PSERVPROF_T*)pData;

	OMCI_PRINT("EntityID: %02x",p8021Map->EntityID);
	OMCI_PRINT("TpPtr: %02x",p8021Map->TpPtr);
	OMCI_PRINT("IwTpPtrPbit0: %02x",p8021Map->IwTpPtrPbit0);
	OMCI_PRINT("IwTpPtrPbit1: %02x",p8021Map->IwTpPtrPbit1);
	OMCI_PRINT("IwTpPtrPbit2: %02x",p8021Map->IwTpPtrPbit2);
	OMCI_PRINT("IwTpPtrPbit3: %02x",p8021Map->IwTpPtrPbit3);
	OMCI_PRINT("IwTpPtrPbit4: %02x",p8021Map->IwTpPtrPbit4);
	OMCI_PRINT("IwTpPtrPbit5: %02x",p8021Map->IwTpPtrPbit5);
	OMCI_PRINT("IwTpPtrPbit6: %02x",p8021Map->IwTpPtrPbit6);
	OMCI_PRINT("IwTpPtrPbit7: %02x",p8021Map->IwTpPtrPbit7);
	OMCI_PRINT("UnmarkFrmOpt: %d",p8021Map->UnmarkFrmOpt);
	ieee8021pmsp_dscp_to_pbit_mapping_dump(p8021Map->DscpMap2Pbit);
	OMCI_PRINT("DefPbitMark: %d",p8021Map->DefPbitMark);
	OMCI_PRINT("TPType: %d",p8021Map->TPType);

	return GOS_OK;
}


GOS_ERROR_CODE Map8021pServProfConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId,int parm)
{
	GOS_ERROR_CODE ret = GOS_FAIL;
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_MAP8021PSERVPROF_T *pMap;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_MAP8021PSERVPROF,entityId);

	if(!pNodeEntry)
	{
		return GOS_FAIL;
	}
	pEntry = pNodeEntry->mibEntry;
	pMap = (MIB_TABLE_MAP8021PSERVPROF_T*)pEntry->pData;
	pConn->p8021Map = pMap;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit0,0) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit1,1) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit2,2) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit3,3) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit4,4) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit5,5) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit6,6) == GOS_OK)
		ret = GOS_OK;

	if(OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pMap->IwTpPtrPbit7,7) == GOS_OK)
		ret = GOS_OK;

	//if 8021p IwTpPtrPbit0~IwTpPtrPbit7 are the same, then trafficMode is flow based
	if (pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit1 && pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit2 &&
		pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit3 && pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit4 &&
		pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit5 && pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit6 &&
		pMap->IwTpPtrPbit0 == pMap->IwTpPtrPbit7 && pMap->IwTpPtrPbit0 != 0xffff)
	{
		pConn->traffMode= OMCI_TRAF_MODE_FLOW_BASE;
	}

	return ret;
}


GOS_ERROR_CODE Map8021pServProfDrvCfg(void				*pOldRow,
										void			*pNewRow,
										MIB_OPERA_TYPE	operationType,
										MIB_ATTRS_SET	attrSet,
										UINT32			pri)
{
	GOS_ERROR_CODE                  ret;
    MIB_TABLE_MAP8021PSERVPROF_T	*pNew1pMapper;
    MIB_TABLE_MAP8021PSERVPROF_T	*pOld1pMapper;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_TRAFFICDESCRIPTOR_T	mibUsTD;
	MIB_TABLE_TRAFFICDESCRIPTOR_T	mibDsTD;
	UINT8							isUsTdValid = FALSE;
	UINT8							isDsTdValid = FALSE;
    UINT16                          *pNewIwtpPtrOffset;
    UINT16                          *pOldIwtpPtrOffset;
    UINT8                           i;

    // only need to check SET condition
    if (MIB_SET != operationType)
    	return GOS_OK;

    pNew1pMapper = (MIB_TABLE_MAP8021PSERVPROF_T *)pNewRow;
    pOld1pMapper = (MIB_TABLE_MAP8021PSERVPROF_T *)pOldRow;

    // search mac bridge port who is related to this 1p mapper
    ret = MIB_GetFirst(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
    while (GOS_OK == ret)
    {
        if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == mibMBPCD.TPType &&
        		pNew1pMapper->EntityID == mibMBPCD.TPPointer)
        {
        	if (GOS_OK == omci_is_traffic_descriptor_existed(mibMBPCD.OutboundTD, &mibUsTD))
	        	isUsTdValid = TRUE;

	        if (GOS_OK == omci_is_traffic_descriptor_existed(mibMBPCD.InboundTD, &mibDsTD))
	        	isDsTdValid = TRUE;

	        if (isUsTdValid || isDsTdValid)
	        {
	            pNewIwtpPtrOffset = &pNew1pMapper->IwTpPtrPbit0;
	            pOldIwtpPtrOffset = &pOld1pMapper->IwTpPtrPbit0;

	            for (i = 0; i < 8; i++)
	            {
	            	if (*(pNewIwtpPtrOffset + i) == *(pOldIwtpPtrOffset + i))
	            		continue;

	            	if (isUsTdValid)
	            	{
            			omci_apply_traffic_descriptor_to_gem_port(
            				*(pNewIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_US, &mibUsTD);

            			omci_apply_traffic_descriptor_to_gem_port(
            				*(pOldIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_US, NULL);
	            	}

	            	if (isDsTdValid)
	            	{
            			omci_apply_traffic_descriptor_to_gem_port(
            				*(pNewIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_DS, &mibDsTD);

            			omci_apply_traffic_descriptor_to_gem_port(
            				*(pOldIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_DS, NULL);
	            	}
	            }
	        }

	        break;
    	}

        ret = MIB_GetNext(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
    }

    MIB_TREE_T *pTree = NULL;
    UINT16      gemPortId;

    if (!MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX) &&
        !MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX))
        goto done;

    if (!(pTree = Map8021pServProfGetTree(pNew1pMapper)))
        goto done;

    if (Map8021pCheckOneGemIwTp(pTree, pNew1pMapper, &gemPortId))
    {
        // delete first since gem iwtp may created before
        omci_wrapper_setUsVeipPriQ(pTree, gemPortId, FALSE);

        omci_wrapper_setUsVeipPriQ(pTree, gemPortId, TRUE);
    }
done:
    return ret;
}

GOS_ERROR_CODE Map8021pServProfConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{

	MIB_TABLE_MAP8021PSERVPROF_T*  map8021 = NULL;
	MIB_TABLE_MAP8021PSERVPROF_T*  oldMap8021 = NULL;
	MIB_TREE_T *pTree = NULL, *pTreeEntry = NULL;
	int key;

	switch (operationType){
	case MIB_ADD:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"802.1p Mapper Configure ---- > ADD");

		/*check if new connection is complete*/
		map8021 = (MIB_TABLE_MAP8021PSERVPROF_T *)pNewRow;
		/*find avl tree*/
		pTree =Map8021pServProfGetTree(map8021);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_OK;
		}
		/*add new node to tree*/
		key = AVL_KEY_MAP8021PSERVPROF;

		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"key is %d",key);
		if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_MAP8021PSERVPROF_INDEX,map8021)==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add 8021p Mapper Node Fail");
			return GOS_OK;
		}

		/*update for gemIwTp*/
		Map8021pUpdateGemIwTp(pTree,map8021);
	}
	break;
	case MIB_SET:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"802.1p Mapper Configure ---- > SET");
		/*check if new connection is complete*/
		map8021 = (MIB_TABLE_MAP8021PSERVPROF_T *)pNewRow;
		oldMap8021 = (MIB_TABLE_MAP8021PSERVPROF_T *)pOldRow;

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX))
		{
		    if (MAP_8021P_UNMARKED_OPT_DSCP21P == map8021->UnmarkFrmOpt)
            {
                omci_wrapper_setDscpRemap(map8021->DscpMap2Pbit);
            }
		}

        pTreeEntry = MIB_TreeFirstGet();
        while (pTreeEntry)
        {
            if (NULL != MIB_AvlTreeEntrySearch(pTreeEntry->root, AVL_KEY_MAP8021PSERVPROF,  map8021->EntityID))
            {
                /*update for gemIwTp*/
    		    Map8021pDeleteGemIwTp(pTreeEntry, oldMap8021);
    		    Map8021pUpdateGemIwTp(pTreeEntry, map8021);
            }
            pTreeEntry = LIST_NEXT(pTreeEntry, entries);
        }
	}
	break;
	case MIB_DEL:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"802.1p Mapper Configure ---- > DEL");
		 /*check if new connection is complete*/
		map8021 = (MIB_TABLE_MAP8021PSERVPROF_T *)pOldRow;

        pTreeEntry = MIB_TreeFirstGet();
        while (pTreeEntry)
        {
            if (NULL != MIB_AvlTreeEntrySearch(pTreeEntry->root, AVL_KEY_MAP8021PSERVPROF, map8021->EntityID))
            {
                /*update for gemIwTp*/
                Map8021pDeleteGemIwTp(pTreeEntry,map8021);
                MIB_AvlTreeNodeEntryRemoveByKey(pTreeEntry->root, AVL_KEY_MAP8021PSERVPROF, map8021->EntityID);
            }
            pTreeEntry = LIST_NEXT(pTreeEntry, entries);
        }
	}
	break;
	default:
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"802.1p Mapper Configure %X", operationType);
            return GOS_OK;
	}
    
	pTreeEntry = MIB_TreeFirstGet();
    MIB_TreeConnUpdate(pTreeEntry);
	
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMap8021pServProfTableInfo.Name = "Map8021pServProf";
    gMibMap8021pServProfTableInfo.ShortName = "MAP1P";
    gMibMap8021pServProfTableInfo.Desc = "802.1p Mapper Service Profile";
    gMibMap8021pServProfTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_IEEE_802DOT1P_MAPPER_SRV_PROFILE);
    gMibMap8021pServProfTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMap8021pServProfTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMap8021pServProfTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMap8021pServProfTableInfo.pAttributes = &(gMibMap8021pServProfAttrInfo[0]);


	gMibMap8021pServProfTableInfo.attrNum = MIB_TABLE_MAP8021PSERVPROF_ATTR_NUM;
	gMibMap8021pServProfTableInfo.entrySize = sizeof(MIB_TABLE_MAP8021PSERVPROF_T);
	gMibMap8021pServProfTableInfo.pDefaultRow = &gMibMap8021pServProfDefRow;


    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TPPtr";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit0";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit1";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit2";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit3";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit4";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit5";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit6";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IwTpPtrPbit7";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnmarkFrmOpt";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DscpMap2Pbit";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DefPbitMark";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TPType";

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TP pointer";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 0)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 1)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 2)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 3)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 4)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 5)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 6)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interwork TP Pointer (for P-Bit priority 7)";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Unmarked Frame Option";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "DSCP to P-Bit Mapping";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Default P-Bit Marking";
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TP Type";

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_PPTPUNIPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT0_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT1_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT2_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT3_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT4_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT5_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT6_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_IWTPPTRPBIT7_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_UNMARKFRMOPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_DEFPBITMARK_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMap8021pServProfAttrInfo[MIB_TABLE_MAP8021PSERVPROF_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;


    memset(&(gMibMap8021pServProfDefRow.EntityID), 0x00, sizeof(gMibMap8021pServProfDefRow.EntityID));
    memset(&(gMibMap8021pServProfDefRow.TpPtr), 0xFF, sizeof(gMibMap8021pServProfDefRow.TpPtr));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit0), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit0));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit1), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit1));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit2), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit2));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit3), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit3));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit4), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit4));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit5), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit5));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit6), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit6));
    memset(&(gMibMap8021pServProfDefRow.IwTpPtrPbit7), 0xFF, sizeof(gMibMap8021pServProfDefRow.IwTpPtrPbit7));
    memset(&(gMibMap8021pServProfDefRow.UnmarkFrmOpt), 0x00, sizeof(gMibMap8021pServProfDefRow.UnmarkFrmOpt));
    memset(gMibMap8021pServProfDefRow.DscpMap2Pbit, 0, MIB_TABLE_MAP8021PSERVPROF_DSCPMAP2PBIT_LEN);
    memset(&(gMibMap8021pServProfDefRow.DefPbitMark), 0x00, sizeof(gMibMap8021pServProfDefRow.DefPbitMark));
    memset(&(gMibMap8021pServProfDefRow.TPType), 0x00, sizeof(gMibMap8021pServProfDefRow.TPType));


    memset(&gMibMap8021pServProfOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibMap8021pServProfOper.meOperDrvCfg = Map8021pServProfDrvCfg;
	gMibMap8021pServProfOper.meOperConnCheck = Map8021pServProfConnCheck;
	gMibMap8021pServProfOper.meOperDump = Map8021pServProfDumpMib;
	gMibMap8021pServProfOper.meOperConnCfg = Map8021pServProfConnCfg;
	gMibMap8021pServProfOper.meOperAvlTreeAdd = Map8021pAvlTreeAdd;
	gMibMap8021pServProfOper.meOperAvlTreeDel = Map8021pAvlTreeDel;
	MIB_TABLE_MAP8021PSERVPROF_INDEX = tableId;

	MIB_InfoRegister(tableId,&gMibMap8021pServProfTableInfo,&gMibMap8021pServProfOper);

    return GOS_OK;
}
