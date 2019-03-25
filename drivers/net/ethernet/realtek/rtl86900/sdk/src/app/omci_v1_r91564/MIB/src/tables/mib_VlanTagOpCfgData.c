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
 * Purpose : Definition of ME handler: VLAN tagging operation configuration data (78)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: VLAN tagging operation configuration data (78)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibVlanTagOpCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ATTR_NUM];
MIB_TABLE_VLANTAGOPCFGDATA_T gMibVlanTagOpCfgDataDefRow;
MIB_TABLE_OPER_T gMibVlanTagOpCfgDataOper;


static MIB_TREE_T* VlanTagOpCfgDataGetTree(MIB_TABLE_VLANTAGOPCFGDATA_T *pVlanTagOpCfgData,MIB_AVL_KEY_T *pKey)
{
	BOOL isUni = FALSE;
	MIB_TREE_T *pTree;

	switch(pVlanTagOpCfgData->Type)
	{
	case VTOCD_ASSOC_TYPE_DEFAULT:
		isUni = TRUE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->EntityID, AVL_KEY_PPTPUNI);
		if (!pTree)
			pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->EntityID, AVL_KEY_IPHOST);
	break;
	case VTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST:
		isUni = TRUE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_IPHOST);
    break;
	case VTOCD_ASSOC_TYPE_IEEE_8021P_MAPPER:
		isUni = FALSE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_MAP8021PSERVPROF);
	break;
	case VTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT:
		isUni = TRUE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_MACBRIPORT_UNI);
		if(!pTree)
		{
			isUni = FALSE;
			pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_MACBRIPORT_ANI);
		}
	break;
	case VTOCD_ASSOC_TYPE_GEM_IWTP:
		isUni = FALSE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_GEMIWTP);
	break;
	case VTOCD_ASSOC_TYPE_MCAST_GEM_IWTP:
		isUni = FALSE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_MULTIGEMIWTP);
	break;
	case VTOCD_ASSOC_TYPE_PPTP_ETH_UNI:
		isUni = TRUE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_PPTPUNI);
	break;
	case VTOCD_ASSOC_TYPE_VEIP:
		isUni = TRUE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagOpCfgData->Pointer,AVL_KEY_VEIP);
	break;
	default:
		OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Not support association type!");
		return NULL;
	break;
	}
	*pKey = isUni ? AVL_KEY_VLANTAGOPCFG_UNI : AVL_KEY_VLANTAGOPCFG_ANI;
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"key is %d",*pKey);

	return pTree;
}


GOS_ERROR_CODE VlanTagOpCfgDataConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId, int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_NODE_T  *pNode;
	UINT16 relatedMeId;
	MIB_TABLE_VLANTAGOPCFGDATA_T *pVlanTagOpCfgData;
	BOOL bSkipMatch;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	pNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_VLANTAGOPCFG_UNI);

	if(pNode)
	{
		LIST_FOREACH(pNodeEntry,&pNode->data.treeNodeEntryHead, treeNodeEntry)
		{
			pEntry = pNodeEntry->mibEntry;
			pVlanTagOpCfgData = (MIB_TABLE_VLANTAGOPCFGDATA_T*)pEntry->pData;

			bSkipMatch = FALSE;
			switch (parm)
			{
				case AVL_KEY_MACBRIPORT_UNI:
                case AVL_KEY_MACBRIPORT_ANI:
					if (VTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT != pVlanTagOpCfgData->Type)
						bSkipMatch = TRUE;
					break;
				case AVL_KEY_PPTPUNI:
					if (VTOCD_ASSOC_TYPE_PPTP_ETH_UNI != pVlanTagOpCfgData->Type &&
							VTOCD_ASSOC_TYPE_DEFAULT != pVlanTagOpCfgData->Type)
						bSkipMatch = TRUE;
					break;
				case AVL_KEY_VEIP:
					if (VTOCD_ASSOC_TYPE_VEIP != pVlanTagOpCfgData->Type)
						bSkipMatch = TRUE;
					break;
				case AVL_KEY_IPHOST:
					if (VTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST != pVlanTagOpCfgData->Type &&
							VTOCD_ASSOC_TYPE_DEFAULT != pVlanTagOpCfgData->Type)
						bSkipMatch = TRUE;
					break;
				default:
					break;
			}
			if (bSkipMatch)
				continue;

			relatedMeId = (VTOCD_ASSOC_TYPE_DEFAULT == pVlanTagOpCfgData->Type ?
							pVlanTagOpCfgData->EntityID : pVlanTagOpCfgData->Pointer);

			if(relatedMeId == entityId)
			{
				pConn->pUniVlanTagOpCfg = pVlanTagOpCfgData;
				return GOS_OK;
			}
		}
	}

	pNode = MIB_AvlTreeSearch(pTree->root,AVL_KEY_VLANTAGOPCFG_ANI);

	if(pNode)
	{
		LIST_FOREACH(pNodeEntry,&pNode->data.treeNodeEntryHead, treeNodeEntry)
		{
			pEntry = pNodeEntry->mibEntry;
			pVlanTagOpCfgData = (MIB_TABLE_VLANTAGOPCFGDATA_T*)pEntry->pData;

			if(pVlanTagOpCfgData->Pointer == entityId)
			{
				pConn->pAniVlanTagOpCfg = pVlanTagOpCfgData;
				return GOS_OK;
			}
		}
	}
	return GOS_FAIL;
}



GOS_ERROR_CODE VlanTagOpCfgDataDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE VlanTagOpCfgDataConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_AVL_KEY_T key;
	MIB_TABLE_VLANTAGOPCFGDATA_T *pVlanTagOpCfgData;
	MIB_TREE_T *pTree = NULL;


    switch (operationType)
    {
        case MIB_ADD:
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Operation ---- > ADD");

            /*check if new connection is complete*/
            pVlanTagOpCfgData = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pNewRow;
            /*find avl tree*/
			pTree =VlanTagOpCfgDataGetTree(pVlanTagOpCfgData,&key);
			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_FAIL;
			}
			/*add new node to tree*/
			if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_VLANTAGOPCFGDATA_INDEX,pVlanTagOpCfgData)==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add VlanTagCfgData Mapper Node Fail");
				return GOS_FAIL;
			}
            break;
        }

        case MIB_SET:
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Operation ---- > SET");
			pVlanTagOpCfgData = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pNewRow;
            /*find avl tree*/
			pTree =VlanTagOpCfgDataGetTree(pVlanTagOpCfgData,&key);
			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_FAIL;
			}
            break;
        }

        case MIB_DEL:
        {
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Operation ---- > DEL");
			pVlanTagOpCfgData = (MIB_TABLE_VLANTAGOPCFGDATA_T *)pNewRow;
			/*find avl tree*/
			pTree =VlanTagOpCfgDataGetTree(pVlanTagOpCfgData,&key);
			if(pTree==NULL)
			{
				OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
				return GOS_FAIL;
			}
			MIB_AvlTreeNodeEntryRemoveByKey(pTree->root,key,pVlanTagOpCfgData->EntityID);
			break;
        }

        default:
            return GOS_OK;
    }

	MIB_TreeConnUpdate(pTree);
    return GOS_OK;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibVlanTagOpCfgDataTableInfo.Name = "VlanTagOpCfgData";
    gMibVlanTagOpCfgDataTableInfo.ShortName = "VTOCD";
    gMibVlanTagOpCfgDataTableInfo.Desc = "VLAN Tag Operate Configuration Data";
    gMibVlanTagOpCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VLAN_TAGGING_OP_CFG_DATA);
    gMibVlanTagOpCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibVlanTagOpCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVlanTagOpCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVlanTagOpCfgDataTableInfo.pAttributes = &(gMibVlanTagOpCfgDataAttrInfo[0]);



	gMibVlanTagOpCfgDataTableInfo.attrNum = MIB_TABLE_VLANTAGOPCFGDATA_ATTR_NUM;
	gMibVlanTagOpCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_VLANTAGOPCFGDATA_T);
	gMibVlanTagOpCfgDataTableInfo.pDefaultRow = &gMibVlanTagOpCfgDataDefRow;


    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsTagOpMode";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsTagTci";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DsTagOpMode";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Type";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Pointer";

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream VLAN Tagging Operation Mode";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Upstream VLAN Tag TCI Value";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream VLAN Tagging Operation Mode";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Association Type";
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Association ME Pointer";

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_USTAGTCI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_DSTAGOPMODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVlanTagOpCfgDataAttrInfo[MIB_TABLE_VLANTAGOPCFGDATA_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;


    memset(&(gMibVlanTagOpCfgDataDefRow.EntityID), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.EntityID));
    memset(&(gMibVlanTagOpCfgDataDefRow.UsTagOpMode), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.UsTagOpMode));
    memset(&(gMibVlanTagOpCfgDataDefRow.UsTagTci), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.UsTagTci));
    memset(&(gMibVlanTagOpCfgDataDefRow.DsTagOpMode), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.DsTagOpMode));
    memset(&(gMibVlanTagOpCfgDataDefRow.Type), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.Type));
    memset(&(gMibVlanTagOpCfgDataDefRow.Pointer), 0x00, sizeof(gMibVlanTagOpCfgDataDefRow.Pointer));


    memset(&gMibVlanTagOpCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibVlanTagOpCfgDataOper.meOperDrvCfg = VlanTagOpCfgDataDrvCfg;
	gMibVlanTagOpCfgDataOper.meOperConnCheck = VlanTagOpCfgDataConnCheck;
	gMibVlanTagOpCfgDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibVlanTagOpCfgDataOper.meOperConnCfg = VlanTagOpCfgDataConnCfg;

	MIB_TABLE_VLANTAGOPCFGDATA_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibVlanTagOpCfgDataTableInfo,&gMibVlanTagOpCfgDataOper);

    return GOS_OK;
}
