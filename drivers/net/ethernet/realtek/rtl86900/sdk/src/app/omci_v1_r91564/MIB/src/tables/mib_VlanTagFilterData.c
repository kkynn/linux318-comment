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
 * Purpose : Definition of ME handler: VLAN tagging filter data (84)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: VLAN tagging filter data (84)
 */

#include "app_basic.h"
#include "feature_export_api.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibVlanTagFilterDataTableInfo;
MIB_ATTR_INFO_T  gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ATTR_NUM];
MIB_TABLE_VLANTAGFILTERDATA_T gMibVlanTagFilterDataDefRow;
MIB_TABLE_OPER_T gMibVlanTagFilterDataOper;


static MIB_TREE_T* VlanTagFilterDataGetTree(MIB_TABLE_VLANTAGFILTERDATA_T *pVlanTagFilterData,MIB_AVL_KEY_T* pKey)
{
	MIB_TREE_T *pTree;
	BOOL isUni = FALSE;
	isUni = TRUE;
	pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagFilterData->EntityID,AVL_KEY_MACBRIPORT_UNI);
	if(!pTree)
	{
		isUni = FALSE;
		pTree = MIB_AvlTreeSearchByKey(NULL, pVlanTagFilterData->EntityID,AVL_KEY_MACBRIPORT_ANI);
	}
	*pKey = isUni ? AVL_KEY_VLANTAGFILTER_UNI : AVL_KEY_VLANTAGFILTER_ANI;
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"key is %d",*pKey);

	return pTree;
}

GOS_ERROR_CODE VlanTagFilterDataDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	int i,arraySize = 0;
	UINT16 tci;
	MIB_TABLE_VLANTAGFILTERDATA_T *pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*)pData;

	OMCI_PRINT("EntityID: 0x%02x",pVlanTagFilter->EntityID);

	arraySize = pVlanTagFilter->NumOfEntries*2;
	for(i=0;i< arraySize ;i+=2)
	{
		tci = pVlanTagFilter->FilterTbl[i] << 8 | pVlanTagFilter->FilterTbl[i+1];
		OMCI_PRINT("FilterTbl[%d]: PRI %d,CFI %d, VID %d",i,(tci >> 13) & 0x7,(tci >> 12) & 0x1,tci & 0xfff);
	}
	OMCI_PRINT("FwdOp:  0x%02x",pVlanTagFilter->FwdOp);
	OMCI_PRINT("NumOfEntries: %d",pVlanTagFilter->NumOfEntries);
	return GOS_OK;
}

GOS_ERROR_CODE VlanTagFilterConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId, int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_VLANTAGFILTERDATA_T *pVlanTagFilter;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	/*implict relationship*/
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_VLANTAGFILTER_UNI,entityId);

	if(pNodeEntry){
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: found entity Id 0x02%x",__FUNCTION__,entityId);
		pEntry = pNodeEntry->mibEntry;
		pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*)pEntry->pData;
		pConn->pUniVlanTagFilter = pVlanTagFilter;
	}else
	{
		pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_VLANTAGFILTER_ANI,entityId);
		if(pNodeEntry){
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: found entity Id 0x02%x",__FUNCTION__,entityId);
			pEntry = pNodeEntry->mibEntry;
			pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*)pEntry->pData;
			pConn->pAniVlanTagFilter = pVlanTagFilter;
		}
	}

	return GOS_OK;
}


GOS_ERROR_CODE VlanTagFilterDataDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE VlanTagFilterDataConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_VLANTAGFILTERDATA_T *pVlanTagFilter;
	MIB_TREE_T *pTree;
	MIB_AVL_KEY_T key;

	switch (operationType){
	case MIB_ADD:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Filter Data ---- > ADD");
		pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*) pNewRow;
		pTree = VlanTagFilterDataGetTree(pVlanTagFilter,&key);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_FAIL;
		}

        if (FAL_FAIL == feature_api(FEATURE_API_BDP_00000400, key, pVlanTagFilter))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"BDP 0x4000 fail");
        }

		/*add new node to tree*/
		if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_VLANTAGFILTERDATA_INDEX,pVlanTagFilter)==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add VlanTagCfgData Mapper Node Fail");
			return GOS_FAIL;
		}
	}
	break;
	case MIB_SET:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Filter Data ---- > SET");
		pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*) pNewRow;
		pTree = VlanTagFilterDataGetTree(pVlanTagFilter,&key);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_FAIL;
		}

        if (FAL_FAIL == feature_api(FEATURE_API_BDP_00000400, key, pVlanTagFilter))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"BDP 0x4000 fail");
        }
    }
        break;
	case MIB_DEL:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VLAN Tag Filter Data ---- > DEL");
		pVlanTagFilter = (MIB_TABLE_VLANTAGFILTERDATA_T*) pNewRow;
		pTree = VlanTagFilterDataGetTree(pVlanTagFilter,&key);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_FAIL;
		}
		/*delete new node to tree*/
		MIB_AvlTreeNodeEntryRemoveByKey(pTree->root,key,pVlanTagFilter->EntityID);
	}
        break;
	default:
		return GOS_OK;
	}

	MIB_TreeConnUpdate(pTree);
    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibVlanTagFilterDataTableInfo.Name = "VlanTagFilterData";
    gMibVlanTagFilterDataTableInfo.ShortName = "VTFD";
    gMibVlanTagFilterDataTableInfo.Desc = "VLAN Tag Filter Data";
    gMibVlanTagFilterDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VLAN_TAGGING_FILTER_DATA);
    gMibVlanTagFilterDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibVlanTagFilterDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVlanTagFilterDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVlanTagFilterDataTableInfo.pAttributes = &(gMibVlanTagFilterDataAttrInfo[0]);


	gMibVlanTagFilterDataTableInfo.attrNum = MIB_TABLE_VLANTAGFILTERDATA_ATTR_NUM;
	gMibVlanTagFilterDataTableInfo.entrySize = sizeof(MIB_TABLE_VLANTAGFILTERDATA_T);
	gMibVlanTagFilterDataTableInfo.pDefaultRow = &gMibVlanTagFilterDataDefRow;


    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FilterTbl";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FwdOp";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfEntries";

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "VLAN Filter Table";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Forward Operation";
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Number of Entries";

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_FWDOP_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVlanTagFilterDataAttrInfo[MIB_TABLE_VLANTAGFILTERDATA_NUMOFENTRIES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;


    memset(&(gMibVlanTagFilterDataDefRow.EntityID), 0x00, sizeof(gMibVlanTagFilterDataDefRow.EntityID));
    memset(gMibVlanTagFilterDataDefRow.FilterTbl, 0x00, MIB_TABLE_VLANTAGFILTERDATA_FILTERTBL_LEN);
    gMibVlanTagFilterDataDefRow.FwdOp = MIB_ATTR_DEF_EMPTY;
    gMibVlanTagFilterDataDefRow.NumOfEntries = MIB_ATTR_DEF_EMPTY;


    memset(&gMibVlanTagFilterDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibVlanTagFilterDataOper.meOperDrvCfg = VlanTagFilterDataDrvCfg;
	gMibVlanTagFilterDataOper.meOperConnCheck = VlanTagFilterConnCheck;
	gMibVlanTagFilterDataOper.meOperDump = VlanTagFilterDataDumpMib;
	gMibVlanTagFilterDataOper.meOperConnCfg = VlanTagFilterDataConnCfg;

	MIB_TABLE_VLANTAGFILTERDATA_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibVlanTagFilterDataTableInfo,&gMibVlanTagFilterDataOper);


    return GOS_OK;
}
