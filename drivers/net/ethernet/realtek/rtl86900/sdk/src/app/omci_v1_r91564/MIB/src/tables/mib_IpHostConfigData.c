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
 * Purpose : Definition of ME handler: IP host config data (134)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: IP host config data (134)
 */

#include "app_basic.h"
#include "omci_task.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibIpHostCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ATTR_NUM];
MIB_TABLE_IP_HOST_CFG_DATA_T gMibIpHostCfgDataDefRow;
MIB_TABLE_OPER_T gMibIpHostCfgDataOper;


GOS_ERROR_CODE
ip_host_cfg_data_drv_cfg_handler(
    void            *pOldRow,
    void            *pNewRow,
    MIB_OPERA_TYPE  operationType,
    MIB_ATTRS_SET   attrSet,
    UINT32          pri)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_IP_HOST_CFG_DATA_T    *pNewMibIPhcd;
    MIB_TABLE_IP_HOST_CFG_DATA_T    *pOldMibIPhcd;
    BOOL                            bFound;
    BOOL                            bDelete;
    if (!pOldRow || !pNewRow)
    {
        return GOS_ERR_PARAM;
    }

    ret = GOS_OK;
    bFound = FALSE;
    bDelete = FALSE;
    pNewMibIPhcd = (MIB_TABLE_IP_HOST_CFG_DATA_T *)pNewRow;
    pOldMibIPhcd = (MIB_TABLE_IP_HOST_CFG_DATA_T *)pOldRow;

    switch (operationType)
    {
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX))
            {
                if (pNewMibIPhcd->IpOptions == IP_HOST_CFG_DATA_IP_OPTIONS_DISABLE_OPTIONS)
                {
                    bDelete = TRUE;
                }
                else if (pNewMibIPhcd->IpOptions != pOldMibIPhcd->IpOptions && 
                        ((pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP &&
                        !(pOldMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP)) ||
                        (!(pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) &&
                        pOldMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) ||
                        (pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK &&
                        !(pOldMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK)) ||
                        (!(pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK) &&
                        pOldMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK) ||
                        (!(pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) &&
                         !(pOldMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) &&
                         pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_IP_STACK)))
                {
                    bFound = TRUE;
                }
            }
            else if (!(pNewMibIPhcd->IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP))
            {
                if ((MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX) &&
                        pNewMibIPhcd->IpAddress != pOldMibIPhcd->IpAddress) ||
                        (MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX) &&
                        pNewMibIPhcd->Mask != pOldMibIPhcd->Mask) ||
                        (MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX) &&
                        pNewMibIPhcd->Gateway != pOldMibIPhcd->Gateway) ||
                        (MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX) &&
                        pNewMibIPhcd->PrimaryDns != pOldMibIPhcd->PrimaryDns) ||
                        ((MIB_IsInAttrSet(&attrSet, MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX) &&
                        pNewMibIPhcd->SecondaryDns != pOldMibIPhcd->SecondaryDns) &&
                        (pNewMibIPhcd->IpOptions != IP_HOST_CFG_DATA_IP_OPTIONS_DISABLE_OPTIONS))) 
                {
                    bFound = TRUE;
                }
            }
            if (bFound)
            {
                BOOL    unlock = TRUE;
                BOOL    none_svc = TRUE;
                MIB_TABLE_IP_HOST_CFG_DATA_T *pIpHost = NULL;
                if (GOS_OK == omci_check_iphost_relation_by_service(IF_SERVICE_TR69, &pIpHost, &unlock) &&
                        pIpHost->EntityID == pNewMibIPhcd->EntityID)
                {
                    omci_setup_mgmt_interface(OP_SET_IF, IF_CHANNEL_MODE_IPOE, IF_SERVICE_TR69, pIpHost, &unlock);
                    none_svc = FALSE;
                }
                if (FAL_OK != feature_api(FEATURE_API_RDP_00000008_ADD_IF_TR69, &none_svc, pNewMibIPhcd, &unlock))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s(): %d Follow standard behavior ", __FUNCTION__, __LINE__);
                }
				
                pIpHost = NULL;
                none_svc = TRUE;
                if (GOS_OK == omci_check_iphost_relation_by_service(IF_SERVICE_SIP, &pIpHost, &unlock) && 
                        pIpHost->EntityID == pNewMibIPhcd->EntityID)
                {
                    omci_setup_mgmt_interface(OP_SET_IF, IF_CHANNEL_MODE_IPOE, IF_SERVICE_SIP, pIpHost, &unlock);
                    none_svc = FALSE;
                }

                if (FAL_OK != feature_api(
                                    FEATURE_API_RDP_00000010_ADD_IF_SIP,
                                    &none_svc,
                                    pNewMibIPhcd,
                                    &unlock))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s(): %d Follow standard behavior ", __FUNCTION__, __LINE__);
                }
            }
            else if (bDelete)
            {
                BOOL unlock = FALSE;
                omci_setup_mgmt_interface(OP_RESET_IF, IF_CHANNEL_MODE_END, IF_SERVICE_ALL, pNewMibIPhcd, &unlock);
            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE ip_host_cfg_data_test_handler(void    *pData)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_msg_norm_baseline_t    *pOmciMsg;
    MIB_TABLE_INDEX             tableIndex = MIB_TABLE_IP_HOST_CFG_DATA_INDEX;

    // make sure the data is available
    if (!pData)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Test data unavailable to proceed: %s",
            MIB_GetTableName(tableIndex));

        return GOS_ERR_PARAM;
    }
    pOmciMsg = (omci_msg_norm_baseline_t *)pData;

    // fill in header
    pOmciMsg->db     = 0;
    pOmciMsg->ar     = 0;
    pOmciMsg->ak     = 0;
    pOmciMsg->type   = OMCI_MSG_TYPE_TEST_RESULT;

    // check if the we support the test type
    if (IP_HOST_CFG_DATA_TEST_TYPE_PING != pOmciMsg->content[0] &&
            IP_HOST_CFG_DATA_TEST_TYPE_TRACEROUTE != pOmciMsg->content[0] &&
            IP_HOST_CFG_DATA_TEST_TYPE_EXTENDED_PING != pOmciMsg->content[0])
        goto out;
    memset(pOmciMsg->content, 0, OMCI_MSG_BASELINE_CONTENTS_LEN);

    // TBD, just want to response error temporarily...
    pOmciMsg->content[0] = IP_HOST_CFG_DATA_TEST_RESULT_TIME_OUT;

    /*
    TBD, handle IP test here!
     */

out:
    // send back the response
    ret = OMCI_SendMsg(OMCI_APPL,
                        OMCI_TX_OMCI_MSG,
                        OMCI_MSG_PRI_NORMAL,
                        pOmciMsg,
                        sizeof(omci_msg_norm_baseline_t));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Unable to send out test result: %s",
            MIB_GetTableName(tableIndex));
    }

    // free allocated memory before leaving
    free(pData);

    return ret;
}

static GOS_ERROR_CODE ip_host_cfg_data_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                                void            *pOldRow,
                                                void            *pNewRow,
                                                MIB_ATTRS_SET   *pAttrsSet,
                                                MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

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

GOS_ERROR_CODE IpHostCfgDataConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entity,int parm)
{
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_IP_HOST_CFG_DATA_T *pIpHost;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_IPHOST,entity);

	if(!pNodeEntry)
		return GOS_FAIL;

	pEntry = pNodeEntry->mibEntry;
	pIpHost = (MIB_TABLE_IP_HOST_CFG_DATA_T*)pEntry->pData;
	pConn->pIpHost= pIpHost;

    OMCI_MeOperConnCheck(MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX, pTree, pConn, pIpHost->EntityID, AVL_KEY_IPHOST);

	OMCI_MeOperConnCheck(MIB_TABLE_VLANTAGOPCFGDATA_INDEX, pTree, pConn, pIpHost->EntityID, AVL_KEY_IPHOST);
	return GOS_OK;
}

GOS_ERROR_CODE IpHostCfgDataAvlTreeAdd(MIB_TREE_T *pTree,UINT16 ipHostCfgDataId)
{
	MIB_TABLE_IP_HOST_CFG_DATA_T *pIpHost;

	if(pTree==NULL) return GOS_FAIL;

	pIpHost = (MIB_TABLE_IP_HOST_CFG_DATA_T *)malloc(sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T));

	if(!pIpHost)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Malloc IpHost Fail");
		return GOS_FAIL;
	}
	memset(pIpHost,0,sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T));
	pIpHost->EntityID = ipHostCfgDataId;
	if(MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, pIpHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T))!=GOS_OK)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Get IpHost Fail");
		free(pIpHost);
		return GOS_FAIL;
	}

	if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_IPHOST,MIB_TABLE_IP_HOST_CFG_DATA_INDEX,pIpHost)== NULL)
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Add IpHost Node Fail");
		free(pIpHost);
		return GOS_FAIL;
	}
	return GOS_OK;
}

GOS_ERROR_CODE IpHostCfgDataAvlTreeDel(MIB_TREE_T *pTree, UINT16 ipHostCfgDataId)
{
    MIB_TABLE_IP_HOST_CFG_DATA_T mibIpHost;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T   *pExtVlan   = NULL;
    MIB_TREE_NODE_ENTRY_T               *pNodeEntry = NULL;
    MIB_ENTRY_T                         *pEntry     = NULL;
    MIB_NODE_T                          *pNode      = NULL;

    if (NULL == pTree)
        return GOS_FAIL;

    mibIpHost.EntityID = ipHostCfgDataId;
    if (GOS_OK != MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHost, sizeof(mibIpHost)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_IP_HOST_CFG_DATA_INDEX), mibIpHost.EntityID);

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

            if (EVTOCD_ASSOC_TYPE_IP_HOST_IPV6_HOST == pExtVlan->AssociationType &&
                ipHostCfgDataId == pExtVlan->AssociatedMePoint)
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_EXTVLAN_UNI, pExtVlan->EntityId);
                break;
            }
        }
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_IPHOST, mibIpHost.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_IP_HOST_CFG_DATA_INDEX), mibIpHost.EntityID, pTree);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibIpHostCfgDataTableInfo.Name = "IpHostCfgData";
    gMibIpHostCfgDataTableInfo.ShortName = "IPHCD";
    gMibIpHostCfgDataTableInfo.Desc = "IP host config data";
    gMibIpHostCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_IP_HOST_CFG_DATA);
    gMibIpHostCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibIpHostCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibIpHostCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_TEST);
    gMibIpHostCfgDataTableInfo.pAttributes = &(gMibIpHostCfgDataAttrInfo[0]);

	gMibIpHostCfgDataTableInfo.attrNum = MIB_TABLE_IP_HOST_CFG_DATA_ATTR_NUM;
	gMibIpHostCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T);
	gMibIpHostCfgDataTableInfo.pDefaultRow = &gMibIpHostCfgDataDefRow;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IpOptions";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MacAddress";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OnuIdentifier";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IpAddress";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Mask";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Gateway";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PrimaryDns";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SecondaryDns";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrentAddress";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrentMask";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrentGateway";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrentPrimaryDns";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CurrentSecondaryDns";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DomainName";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "HostName";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RelayAgentOptions";

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IP options";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "MAC address";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Onu identifier";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IP address";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Mask";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Gateway";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Primary DNS";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Secondary DNS";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current address";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current mask";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current gateway";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current primary DNS";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Current secondary DNS";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Domain name";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Host name";
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Relay agent options";

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 6;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MAC_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_ONU_IDENTIFIER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_IP_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_ADDRESS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_MASK_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_GATEWAY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_PRIMARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_CURRENT_SECONDARY_DNS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_DOMAIN_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_HOST_NAME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibIpHostCfgDataAttrInfo[MIB_TABLE_IP_HOST_CFG_DATA_RELAY_AGENT_OPTIONS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType =
        OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;

    memset(&gMibIpHostCfgDataDefRow, 0x00, sizeof(gMibIpHostCfgDataDefRow));

    memset(&gMibIpHostCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibIpHostCfgDataOper.meOperDrvCfg = ip_host_cfg_data_drv_cfg_handler;
    gMibIpHostCfgDataOper.meOperConnCheck = IpHostCfgDataConnCheck;
    gMibIpHostCfgDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibIpHostCfgDataOper.meOperConnCfg = NULL;
    gMibIpHostCfgDataOper.meOperTestHandler = ip_host_cfg_data_test_handler;
	gMibIpHostCfgDataOper.meOperAvlTreeAdd = IpHostCfgDataAvlTreeAdd;
	gMibIpHostCfgDataOper.meOperAvlTreeDel = IpHostCfgDataAvlTreeDel;

	MIB_TABLE_IP_HOST_CFG_DATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibIpHostCfgDataTableInfo, &gMibIpHostCfgDataOper);
    MIB_RegisterCallback(tableId, NULL, ip_host_cfg_data_avc_cb);

    return GOS_OK;
}
