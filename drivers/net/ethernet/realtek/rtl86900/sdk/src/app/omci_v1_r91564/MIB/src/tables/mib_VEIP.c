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
 * Purpose : Definition of ME handler: Virtual Ethernet interface point (329)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Virtual Ethernet interface point (329)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibVEIPTableInfo;
MIB_ATTR_INFO_T  gMibVEIPAttrInfo[MIB_TABLE_VEIP_ATTR_NUM];
MIB_TABLE_VEIP_T gMibVEIPDefRow;
MIB_TABLE_OPER_T gMibVEIPOper;


GOS_ERROR_CODE VEIPConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entity,int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_VEIP_T *pVeip;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_VEIP,entity);

	if(!pNodeEntry)
		return GOS_FAIL;

	pEntry = pNodeEntry->mibEntry;
	pVeip = (MIB_TABLE_VEIP_T*)pEntry->pData;
	pConn->pVeip= pVeip;
	OMCI_MeOperConnCheck(MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX,pTree,pConn,pVeip->EntityId,AVL_KEY_VEIP);
	return GOS_OK;
}

GOS_ERROR_CODE VEIPAvlTreeAdd(MIB_TREE_T *pTree,UINT16 veipId)
{
	MIB_TABLE_VEIP_T *pVeip;

	if(pTree==NULL) return GOS_FAIL;

	pVeip = (MIB_TABLE_VEIP_T*)malloc(sizeof(MIB_TABLE_VEIP_T));

	if(!pVeip)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Malloc VEIP Fail");
		return GOS_FAIL;
	}
	memset(pVeip,0,sizeof(MIB_TABLE_VEIP_T));
	pVeip->EntityId = veipId;
	if(MIB_Get(MIB_TABLE_VEIP_INDEX, pVeip, sizeof(MIB_TABLE_VEIP_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Get VEIP Fail");
		free(pVeip);
		return GOS_FAIL;
	}

	if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_VEIP,MIB_TABLE_VEIP_INDEX,pVeip)== NULL)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Add VEIP Node Fail");
		free(pVeip);
		return GOS_FAIL;
	}
	return GOS_OK;
}

GOS_ERROR_CODE VEIPAvlTreeDel(MIB_TREE_T *pTree, UINT16 veipId)
{
    MIB_TABLE_VEIP_T        mibVeip;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;


    if (NULL == pTree)
        return GOS_FAIL;

    mibVeip.EntityId = veipId;
    if (GOS_OK != MIB_Get(MIB_TABLE_VEIP_INDEX, &mibVeip, sizeof(mibVeip)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_VEIP_INDEX), mibVeip.EntityId);

        return GOS_FAIL;
    }

    if ((pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_EXTVLAN_UNI)))
    {
        LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            if (!(pEntry = pNodeEntry->mibEntry))
                continue;

            if (!(pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData))
                continue;

            if (EVTOCD_ASSOC_TYPE_VEIP == pExtVlan->AssociationType &&
                veipId == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_UNI, pExtVlan->EntityId);
                break;
            }
        }
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_VEIP, mibVeip.EntityId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_VEIP_INDEX), mibVeip.EntityId, pTree);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE VEIPDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_VEIP_T                *pNewMibVeip;
    MIB_TABLE_VEIP_T                *pOldMibVeip;

    ret = GOS_OK;
    pNewMibVeip = (MIB_TABLE_VEIP_T *)pNewRow;
    pOldMibVeip = (MIB_TABLE_VEIP_T *)pOldRow;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(MIB_TABLE_VEIP_INDEX, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(MIB_TABLE_VEIP_INDEX, pOldRow);
            break;
        case MIB_SET:
            if ((MIB_IsInAttrSet(&attrSet, MIB_TABLE_VEIP_ADMINSTATE_INDEX) &&
                    pNewMibVeip->AdminState != pOldMibVeip->AdminState) ||
                    (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VEIP_TCPUDPPTR_INDEX) &&
                    pNewMibVeip->TcpUdpPtr != pOldMibVeip->TcpUdpPtr))
            {
                BOOL                         unlock = TRUE;
                BOOL                         none_svc = FALSE; 
                MIB_TABLE_IP_HOST_CFG_DATA_T *pIpHost = NULL;
                MIB_TABLE_IP_HOST_CFG_DATA_T  mibIpHost;
                if (FAL_OK == feature_api(
                                    FEATURE_API_RDP_00000008_ADD_IF_TR69, 
                                    &none_svc, 
                                    &mibIpHost, 
                                    &unlock))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s(): %d Follow existed custom behavior ", __FUNCTION__, __LINE__);
                    break;
                }
                if (GOS_OK == omci_check_iphost_relation_by_service(IF_SERVICE_TR69, &pIpHost, &unlock) && TRUE == unlock)
                    omci_setup_mgmt_interface(OP_SET_IF, IF_CHANNEL_MODE_IPOE, IF_SERVICE_TR69, pIpHost, &unlock);

            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE veip_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                            omci_alm_data_t     alarmData,
                                            omci_me_instance_t  *pInstanceID,
                                            BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;
    MIB_ENTRY_T         *pEntry;
    MIB_TABLE_VEIP_T    *pMibVeip;
    omci_me_instance_t  cpInstanceID;
    INT16               slotId = -1, portId = -1;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    // portId should be the veip port number (if more than one, 0-based)
    portId = (alarmData.almDetail & 0xFF) + 1;
    slotId = TXC_CARDHLD_VEIP_SLOT_TYPE_ID;

    feature_api(FEATURE_API_ME_00000100, &slotId);

    *pInstanceID = (slotId << 8) | portId;

    if (GOS_OK != mib_alarm_table_get(tableIndex, *pInstanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), *pInstanceID);

        return GOS_FAIL;
    }

    // update alarm status if it has being changed
    mib_alarm_table_update(&alarmTable, &alarmData, pIsUpdated);

    if (*pIsUpdated)
    {
        if (GOS_OK != mib_alarm_table_set(tableIndex, *pInstanceID, &alarmTable))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
                MIB_GetTableName(tableIndex), *pInstanceID);

            return GOS_FAIL;
        }

        // correct the circuit pack instanceID to the slot it belongs to
        cpInstanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotId;

        // check if notifications are suppressed by parent's admin state
        omci_is_notify_suppressed_by_circuitpack(cpInstanceID, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
        else
        {
            // find the entry data by using the instanceID
            pEntry = mib_FindEntryByInstanceID(tableIndex, *pInstanceID);
            if (!pEntry)
                return GOS_FAIL;
            pMibVeip = pEntry->pData;
            if (!pMibVeip)
                return GOS_FAIL;

            // check if notifications are suppressed by self's admin state
            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibVeip->AdminState)
                *pIsUpdated = FALSE;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE veip_check_cb(MIB_TABLE_INDEX     tableIndex,
                                    void                *pOldRow,
                                    void                *pNewRow,
                                    MIB_ATTRS_SET       *pAttrsSet,
                                    MIB_OPERA_TYPE      operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              attrSize, i;
    MIB_ATTRS_SET       avcAttrSet;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    avcAttrSet = *pAttrsSet;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
            i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
            continue;

        attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

        // change op state according to admin state
        if (MIB_TABLE_VEIP_ADMINSTATE_INDEX == attrIndex)
        {
            UINT8 newValue, oldValue;

            MIB_GetAttrFromBuf(tableIndex, attrIndex, &newValue, pNewRow, attrSize);
            if (pOldRow)
                MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);

            if (!pOldRow || newValue != oldValue)
            {
                MIB_SetAttrToBuf(tableIndex, MIB_TABLE_VEIP_OPERSTATE_INDEX, &newValue, pNewRow, attrSize);
                MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_VEIP_OPERSTATE_INDEX);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

static GOS_ERROR_CODE veip_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                    void            *pOldRow,
                                    void            *pNewRow,
                                    MIB_ATTRS_SET   *pAttrsSet,
                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;
    omci_me_instance_t  instanceID;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

    // correct the instanceID to the slot it belongs to
    instanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | (instanceID >> 8);

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
    gMibVEIPTableInfo.Name = "VEIP";
    gMibVEIPTableInfo.ShortName = "VEIP";
    gMibVEIPTableInfo.Desc = "Virtual Ethernet interface point";
    gMibVEIPTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VIRTUAL_ETHERNET_INTF_POINT);
    gMibVEIPTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibVEIPTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVEIPTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVEIPTableInfo.pAttributes = &(gMibVEIPAttrInfo[0]);
	gMibVEIPTableInfo.attrNum = MIB_TABLE_VEIP_ATTR_NUM;
	gMibVEIPTableInfo.entrySize = sizeof(MIB_TABLE_VEIP_T);
	gMibVEIPTableInfo.pDefaultRow = &gMibVEIPDefRow;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AdminState";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OperState";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "InterDomainName";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TcpUdpPtr";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IansAssignPort";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CustomizedSlotId";

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Administrative state";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational state";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Inter-domain name";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TCP / UDP pointer";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IANA assigned port";
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Customized slot ID";

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_OPERSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_INTERDOMAINNAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_TCPUDPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_IANSASSIGNPORT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVEIPAttrInfo[MIB_TABLE_VEIP_CUSTOMIZEDSLOTID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    memset(&(gMibVEIPDefRow.EntityId), 0x00, sizeof(gMibVEIPDefRow.EntityId));
    gMibVEIPDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_LOCK;
    gMibVEIPDefRow.OperState = OMCI_ME_ATTR_OP_STATE_DISABLED;
    strcpy(gMibVEIPDefRow.InterDomainName, "");
    memset(&(gMibVEIPDefRow.TcpUdpPtr), 0x00, sizeof(gMibVEIPDefRow.TcpUdpPtr));
    memset(&(gMibVEIPDefRow.IansAssignPort), 0x00, sizeof(gMibVEIPDefRow.IansAssignPort));
    gMibVEIPDefRow.CustomizedSlotId = gInfo.veipSlotId;

    memset(&gMibVEIPOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVEIPOper.meOperDrvCfg = VEIPDrvCfg;
    gMibVEIPOper.meOperConnCheck = VEIPConnCheck;
    gMibVEIPOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibVEIPOper.meOperConnCfg = NULL;
	gMibVEIPOper.meOperAvlTreeAdd = VEIPAvlTreeAdd;
    gMibVEIPOper.meOperAvlTreeDel = VEIPAvlTreeDel;
    gMibVEIPOper.meOperAlarmHandler = veip_alarm_handler;

	MIB_TABLE_VEIP_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVEIPTableInfo, &gMibVEIPOper);
    MIB_RegisterCallback(tableId, veip_check_cb, veip_avc_cb);

    return GOS_OK;
}
