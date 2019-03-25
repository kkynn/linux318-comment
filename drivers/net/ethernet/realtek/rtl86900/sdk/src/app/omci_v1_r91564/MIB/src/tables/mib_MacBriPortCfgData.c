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
 * Purpose : Definition of ME handler: MAC bridge port configuration data (47)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: MAC bridge port configuration data (47)
 */

#include "app_basic.h"
#include "feature_mgmt.h"

MIB_TABLE_INFO_T gMibMacBriPortCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ATTR_NUM];
MIB_TABLE_MACBRIPORTCFGDATA_T gMibMacBriPortCfgDataDefRow;
MIB_TABLE_OPER_T			  gMibMacBriPortCfgDataOper;


static MIB_AVL_KEY_T MacBriPortCfgDataGetKey(int tpType)
{
	MIB_AVL_KEY_T key;
	switch(tpType){
	case MBPCD_TP_TYPE_PPTP_ETH_UNI:
	case MBPCD_TP_TYPE_VEIP:
	case MBPCD_TP_TYPE_IP_HOST_IPV6_HOST:
		key = AVL_KEY_MACBRIPORT_UNI;
	break;
	default:
		key = AVL_KEY_MACBRIPORT_ANI;
	break;
	}

	return key;
}


GOS_ERROR_CODE MacBriPortCfgDataConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId,
int parm)
{
	int index=0;
	GOS_ERROR_CODE ret;
	MIB_TREE_NODE_ENTRY_T *pNodeEntry;
	MIB_ENTRY_T *pEntry;
	MIB_TABLE_MACBRIPORTCFGDATA_T *pData;
	MIB_AVL_KEY_T key = (MIB_AVL_KEY_T)parm;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
	/*check uni*/
	pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,key,entityId);
	/*if not found, check ani*/
	if(!pNodeEntry){
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: can't find MacBridgePortCfgData",__FUNCTION__);
		return GOS_FAIL;
	}
	pEntry = pNodeEntry->mibEntry;
	pData = (MIB_TABLE_MACBRIPORTCFGDATA_T*)pEntry->pData;
	feature_api(FEATURE_API_ME_00000008, pData);

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s:  MacBridgePortCfGData TP Type is %x",__FUNCTION__,pData->TPType);

	switch(pData->TPType)
	{
	case MBPCD_TP_TYPE_IEEE_8021P_MAPPER:
	{
		pConn->traffMode= OMCI_TRAF_MODE_8021P_BASE;
		pConn->pAniPort= pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_MAP8021PSERVPROF_INDEX,pTree,pConn,pData->TPPointer,index);
	}
	break;
	case MBPCD_TP_TYPE_GEM_IWTP:
	{
		pConn->traffMode= OMCI_TRAF_MODE_FLOW_BASE;
		pConn->pAniPort = pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_GEMIWTP_INDEX,pTree,pConn,pData->TPPointer,index);
	}
	break;
	case MBPCD_TP_TYPE_MCAST_GEM_IWTP:
	{
		pConn->traffMode= OMCI_TRAF_MODE_FLOW_BASE;
		pConn->pAniPort = pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_MULTIGEMIWTP_INDEX,pTree,pConn,pData->TPPointer,index);
	}
	break;
	case MBPCD_TP_TYPE_PPTP_ETH_UNI:
	{

		pConn->pUniPort = pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_ETHUNI_INDEX,pTree,pConn,pData->TPPointer,index);
        ret = GOS_OK;
	}
	break;
	case MBPCD_TP_TYPE_VEIP:
	{
		pConn->pUniPort = pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_VEIP_INDEX,pTree,pConn,pData->TPPointer,index);
        ret = GOS_OK;
	}
	break;
	case MBPCD_TP_TYPE_IP_HOST_IPV6_HOST:
	{
		pConn->pUniPort = pData;
		ret = OMCI_MeOperConnCheck(MIB_TABLE_IP_HOST_CFG_DATA_INDEX,pTree,pConn,pData->TPPointer,index);
	}
	break;
	default:
		ret = GOS_FAIL;
	break;
	}

	/*update vlan action related ME*/
	if(ret == GOS_OK)
	{
		OMCI_MeOperConnCheck(MIB_TABLE_VLANTAGFILTERDATA_INDEX,pTree,pConn,pData->EntityID,key);
		OMCI_MeOperConnCheck(MIB_TABLE_VLANTAGOPCFGDATA_INDEX,pTree,pConn,pData->EntityID,key);
		OMCI_MeOperConnCheck(MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX,pTree,pConn,pData->EntityID,key);
	}

	return ret;
}


GOS_ERROR_CODE MacBriPortCfgDataDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	GOS_ERROR_CODE					ret = GOS_OK;
	MIB_TABLE_INDEX					tableIndex = MIB_TABLE_MACBRIPORTCFGDATA_INDEX;
	MIB_TABLE_MACBRIPORTCFGDATA_T	*pMacBriPortCfgData;
    MIB_TABLE_MAP8021PSERVPROF_T    mib1pMapper;
    MIB_TABLE_ETHUNI_T              mibEthUni;
	MIB_TABLE_TRAFFICDESCRIPTOR_T	mibUsTD;
	MIB_TABLE_TRAFFICDESCRIPTOR_T	mibDsTD;
    MIB_TABLE_MACBRISERVPROF_T      bsp;
	UINT8							isUsTdValid = FALSE;
	UINT8							isDsTdValid = FALSE;
    UINT16                          *pIwtpPtrOffset;
    UINT8                           i;
    unig_attr_mgmt_capability_t     mngCapUnig = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;

	pMacBriPortCfgData = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow;
	feature_api(FEATURE_API_ME_00000008, pMacBriPortCfgData);
    if ((MBPCD_TP_TYPE_PPTP_ETH_UNI == ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPType)
        && omci_mngCapOfUnigGet (((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer, &mngCapUnig) != GOS_OK)
    {
        OMCI_LOG (
            OMCI_LOG_LEVEL_WARN,
            "Failed to get UNI-G for TPPointer(0x%03x) of MBPCD (0x%03x)\n",
            ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer,
            ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->EntityID);
    }

    if ( UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY == mngCapUnig)
    {
        OMCI_LOG(
            OMCI_LOG_LEVEL_WARN, "%s: ManageCapability of UNIG(0x%03X) is NON_OMCI_ONLY", 
            __FUNCTION__,
            ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer);
        return GOS_OK;
    }

	switch (operationType)
	{
		case MIB_ADD:
		{
			MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T        macBridgePortFilterTable;
			MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_T    macBridgePortFilterPreassign;
            MIB_TABLE_MACBRIPORTBRITBLDATA_T            macBridgePortBridgeTable;

            /*Set Mac learn depth of UNI port*/
			if(MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType &&
                FAL_OK != feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP) &&
                OMCI_DEV_MODE_ROUTER != gInfo.devMode)
			{
				UINT16 portIdx;
				UINT32 macLimitNum;
				UINT32 newPortMask;
				UINT32 oldPortMask;
                omci_flood_port_info floodPortInfo;

                bsp.EntityID = pMacBriPortCfgData->BridgeIdPtr;
				if (GOS_OK == MIB_Get(MIB_TABLE_MACBRISERVPROF_INDEX, &bsp, sizeof(MIB_TABLE_MACBRISERVPROF_T)))
			    {
                    /*NumOfAllowedMac == 0 means no limited*/
                    macLimitNum = (pMacBriPortCfgData->NumOfAllowedMac != 0 ?
                                    pMacBriPortCfgData->NumOfAllowedMac :
                                    (bsp.MacLearningDepth == 0 ? gInfo.devCapabilities.totalL2Num : bsp.MacLearningDepth));
                }
                else
                {

                    macLimitNum = (pMacBriPortCfgData->NumOfAllowedMac != 0 ? pMacBriPortCfgData->NumOfAllowedMac : gInfo.devCapabilities.totalL2Num);
                }


				/*Get port index form entity id*/
				if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMacBriPortCfgData->TPPointer, &portIdx))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "PPTP instance ID translate error: 0x%x", pMacBriPortCfgData->TPPointer);
					return GOS_FAIL;
				}
				omci_wrapper_setMacLearnLimit((unsigned int)portIdx,(unsigned int)macLimitNum);

				if (GOS_OK != omci_get_pptp_eth_uni_port_mask_in_bridge(pMacBriPortCfgData->BridgeIdPtr, &newPortMask))
			    {
			        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get pptp eth uni port mask in bridge fail");
			    }
			    else
			    {
			    	oldPortMask = newPortMask & ~(1 << portIdx);

			    	omci_update_dot1_rate_limiter_port_mask(pMacBriPortCfgData->BridgeIdPtr, newPortMask, oldPortMask);

                    memset(&floodPortInfo, 0, sizeof(omci_flood_port_info));
                    floodPortInfo.type      = OMCI_FLOOD_UNICAST;
                    floodPortInfo.act = (TRUE == bsp.DiscardUnknow ? OMCI_LOOKUP_MISS_ACT_DROP : OMCI_LOOKUP_MISS_ACT_FLOOD);
                    floodPortInfo.portMask  = (1 << portIdx);
                    omci_wrapper_setFloodingPortMask(&floodPortInfo);
			    }

            }
			else if (MBPCD_TP_TYPE_VEIP == pMacBriPortCfgData->TPType &&
                FAL_OK != feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP))
			{
				MIB_TABLE_GEMPORTCTP_T	*pMibGPNC;
				MIB_TREE_T				*pTree;
				MIB_NODE_T				*pNode;
				MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
				MIB_ENTRY_T				*pEntry;
				UINT32					newPortMask;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_GEMPORTCTP);
					if (pNode)
					{
						LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
						{
							pEntry = pNodeEntry->mibEntry;
							pMibGPNC = (MIB_TABLE_GEMPORTCTP_T *)pEntry->pData;

							if (GPNC_DIRECTION_ANI_TO_UNI != pMibGPNC->Direction)
								omci_wrapper_setUsVeipPriQ(pTree, pMibGPNC->PortID, TRUE);
						}
					}
				}

				if (GOS_OK != omci_get_eth_uni_port_mask_behind_veip(pMacBriPortCfgData->BridgeIdPtr, &newPortMask))
			    {
			        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get eth uni port mask behind veip fail");
			    }
			    else
			    {
			    	omci_update_dot1_rate_limiter_port_mask(pMacBriPortCfgData->BridgeIdPtr, newPortMask, 0);
			    }
			}
			else if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType &&
                (FAL_OK == feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP) &&
                OMCI_DEV_MODE_ROUTER == gInfo.devMode))
			{
				MIB_TABLE_GEMPORTCTP_T	*pMibGPNC;
				MIB_TREE_T				*pTree;
				MIB_NODE_T				*pNode;
				MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
				MIB_ENTRY_T				*pEntry;
				UINT32					newPortMask;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_GEMPORTCTP);
					if (pNode)
					{
						LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
						{
							pEntry = pNodeEntry->mibEntry;
							pMibGPNC = (MIB_TABLE_GEMPORTCTP_T *)pEntry->pData;

							if (GPNC_DIRECTION_ANI_TO_UNI != pMibGPNC->Direction)
								omci_wrapper_setUsVeipPriQ(pTree, pMibGPNC->PortID, TRUE);
						}
					}
				}

				if (GOS_OK != omci_get_eth_uni_port_mask_behind_veip(pMacBriPortCfgData->BridgeIdPtr, &newPortMask))
			    {
			        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get eth uni port mask behind veip fail");
			    }
			    else
			    {
			    	omci_update_dot1_rate_limiter_port_mask(pMacBriPortCfgData->BridgeIdPtr, newPortMask, 0);
			    }
			}
            else if (MBPCD_TP_TYPE_IP_HOST_IPV6_HOST == pMacBriPortCfgData->TPType &&
                OMCI_DEV_MODE_BRIDGE != gInfo.devMode)
			{
				MIB_TABLE_GEMPORTCTP_T	*pMibGPNC;
				MIB_TREE_T				*pTree;
				MIB_NODE_T				*pNode;
				MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
				MIB_ENTRY_T				*pEntry;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_GEMPORTCTP);
					if (pNode)
					{
						LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
						{
							pEntry = pNodeEntry->mibEntry;
							pMibGPNC = (MIB_TABLE_GEMPORTCTP_T *)pEntry->pData;

							if (GPNC_DIRECTION_ANI_TO_UNI != pMibGPNC->Direction)
								omci_wrapper_setUsVeipPriQ(pTree, pMibGPNC->PortID, TRUE);
						}
					}
				}
			}
			else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType)
			{
				MIB_TABLE_MAP8021PSERVPROF_T	mib8021pmsp;
				MIB_TABLE_GEMIWTP_T				mibGemIwTp;
				MIB_TABLE_GEMPORTCTP_T			mibGPNC;
				MIB_TREE_T						*pTree;
				UINT16                          *pIwtpPtrOffset;
				UINT8							bFound;
			    UINT8                           i;
				UINT16                          IwtpPtr = 0xffff;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					mib8021pmsp.EntityID = pMacBriPortCfgData->TPPointer;
					if (GOS_OK == MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib8021pmsp, sizeof(mib8021pmsp)))
					{
						pIwtpPtrOffset = &mib8021pmsp.IwTpPtrPbit0;
						bFound = FALSE;

						for (i = 0; i < 8; i++)
						{
							if(*(pIwtpPtrOffset + i) != 0xffff)
							{
								IwtpPtr = *(pIwtpPtrOffset + i);
								break;
							}
						}

			            if (!bFound)
			            {
			            	mibGemIwTp.EntityID = IwtpPtr;
							if (GOS_OK == MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
							{
								mibGPNC.EntityID = mibGemIwTp.GemCtpPtr;
								if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
								{
									if (GPNC_DIRECTION_ANI_TO_UNI != mibGPNC.Direction)
									{
										// delete first since gem iwtp may created before
										omci_wrapper_setUsVeipPriQ(pTree, mibGPNC.PortID, FALSE);

										omci_wrapper_setUsVeipPriQ(pTree, mibGPNC.PortID, TRUE);
									}
								}
							}
			            }
					}
				}
			}
			else if (MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
				MIB_TABLE_GEMIWTP_T		mibGemIwTp;
				MIB_TABLE_GEMPORTCTP_T	mibGPNC;
				UINT32					flowId;

				// assume that there is only one DS BC GEM in the system
				mibGemIwTp.EntityID = pMacBriPortCfgData->TPPointer;
				if (GOS_OK == MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
			    {
			    	if (GEM_IWTP_IW_OPTION_DOWNSTREAM_BROADCAST == mibGemIwTp.IwOpt)
			    	{
				        mibGPNC.EntityID = mibGemIwTp.GemCtpPtr;
						if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
						{
							flowId = omci_GetDsFlowIdByPortId(mibGPNC.PortID);

							if (gInfo.devCapabilities.totalGEMPortNum != flowId)
						    	omci_wrapper_setDsBcGemFlow(flowId);
						}
					}
			    }
			}

			/*Create related MAC bridge port filter table data ME*/
			memset(&macBridgePortFilterTable, 0, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T));
			macBridgePortFilterTable.EntityId = pMacBriPortCfgData->EntityID;
			GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX, &macBridgePortFilterTable, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T)));
			OMCI_MeOperCfg(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX, NULL, &macBridgePortFilterTable, MIB_ADD,
        		omci_GetOltAccAttrSet(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

			/*Create related bridge port filter pre-assign table ME*/
			memset(&macBridgePortFilterPreassign, 0, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_T));
			macBridgePortFilterPreassign.EntityId = pMacBriPortCfgData->EntityID;
			GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_INDEX, &macBridgePortFilterPreassign, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_T)));
			OMCI_MeOperCfg(MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_INDEX, NULL, &macBridgePortFilterPreassign, MIB_ADD,
        		omci_GetOltAccAttrSet(MIB_TABLE_MACBRIDGEPORTFILTERPREASSIGN_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

            /*Create related MAC bridge port bridge table data ME*/
			memset(&macBridgePortBridgeTable, 0, sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T));
			macBridgePortBridgeTable.EntityID = pMacBriPortCfgData->EntityID;
			GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX, &macBridgePortBridgeTable, sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T)));
			OMCI_MeOperCfg(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX, NULL, &macBridgePortBridgeTable, MIB_ADD,
        		omci_GetOltAccAttrSet(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

			ret = mib_alarm_table_add(tableIndex, pNewRow);
			break;
		}
		case MIB_SET:
		{
            /*Set Mac learn depth of UNI port*/
			if(MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX) && (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType))
			{
				UINT16 portIdx;
				UINT32 macLimitNum;

                bsp.EntityID = pMacBriPortCfgData->BridgeIdPtr;
				if (GOS_OK == MIB_Get(MIB_TABLE_MACBRISERVPROF_INDEX, &bsp, sizeof(MIB_TABLE_MACBRISERVPROF_T)))
			    {
                    /*NumOfAllowedMac == 0 means no limited*/
                    macLimitNum = (pMacBriPortCfgData->NumOfAllowedMac != 0 ?
                                    pMacBriPortCfgData->NumOfAllowedMac :
                                    (bsp.MacLearningDepth == 0 ? gInfo.devCapabilities.totalL2Num : bsp.MacLearningDepth));
                }
                else
                {

                    macLimitNum = (pMacBriPortCfgData->NumOfAllowedMac != 0 ? pMacBriPortCfgData->NumOfAllowedMac : gInfo.devCapabilities.totalL2Num);
                }

				/*Get port index form entity id*/
				if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMacBriPortCfgData->TPPointer, &portIdx))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "PPTP instance ID translate error: 0x%x", pMacBriPortCfgData->TPPointer);
					return GOS_FAIL;
				}
				omci_wrapper_setMacLearnLimit((unsigned int)portIdx,(unsigned int)macLimitNum);
			}

			// invoke traffic descriptor settings
			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX))
			{
				if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType ||
						MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
				{
					if (GOS_OK == omci_is_traffic_descriptor_existed(pMacBriPortCfgData->OutboundTD, &mibUsTD))
						isUsTdValid = TRUE;
				}
                else if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
				{
					isDsTdValid = TRUE;
				}
			}
			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX))
			{
				if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType ||
						MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType ||
						MBPCD_TP_TYPE_MCAST_GEM_IWTP == pMacBriPortCfgData->TPType)
				{
					if (GOS_OK == omci_is_traffic_descriptor_existed(pMacBriPortCfgData->InboundTD, &mibDsTD))
						isDsTdValid = TRUE;
				}
                else if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
				{
					isUsTdValid = TRUE;
				}
			}
			if (isUsTdValid || isDsTdValid)
			{
			    if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
                {
                    mibEthUni.EntityID = pMacBriPortCfgData->TPPointer;

	                if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibEthUni, sizeof(mibEthUni)))
	                    break;

                    if (isUsTdValid)
                    {
                        mibUsTD.EntityId = pMacBriPortCfgData->InboundTD;
                        if (GOS_OK == MIB_Get(MIB_TABLE_TRAFFICDESCRIPTOR_INDEX, &mibUsTD, sizeof(MIB_TABLE_TRAFFICDESCRIPTOR_T)))
                        {
                            omci_apply_traffic_descriptor_to_uni_port(
            					pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_INGRESS, &mibUsTD);
                        }
                        else
                        {
                            omci_apply_traffic_descriptor_to_uni_port(
            					pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_INGRESS, NULL);
                        }
                    }

                    if (isDsTdValid)
                    {
                        mibDsTD.EntityId = pMacBriPortCfgData->OutboundTD;
                        if (GOS_OK == MIB_Get(MIB_TABLE_TRAFFICDESCRIPTOR_INDEX, &mibDsTD, sizeof(MIB_TABLE_TRAFFICDESCRIPTOR_T)))
                        {
	                	    omci_apply_traffic_descriptor_to_uni_port(
        					    pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_EGRESS, &mibDsTD);
                        }
                        else
                        {
	                	    omci_apply_traffic_descriptor_to_uni_port(
        					    pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_EGRESS, NULL);
                        }
                    }
                }
				else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType)
	            {
	                mib1pMapper.EntityID = pMacBriPortCfgData->TPPointer;

	                if (GOS_OK != MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib1pMapper, sizeof(mib1pMapper)))
	                    break;

	                pIwtpPtrOffset = &mib1pMapper.IwTpPtrPbit0;

	                for (i = 0; i < 8; i++)
	                {
	                    if (isUsTdValid)
	                    {
	                    	omci_apply_traffic_descriptor_to_gem_port(
            					*(pIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_US, &mibUsTD);
	                    }

	                    if (isDsTdValid)
	                    {
	                    	omci_apply_traffic_descriptor_to_gem_port(
            					*(pIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_DS, &mibDsTD);
	                    }
	                }
	            }
	            else
	            {
	                if (isUsTdValid)
	                {
	                	omci_apply_traffic_descriptor_to_gem_port(
        					pMacBriPortCfgData->TPPointer, PON_GEMPORT_DIRECTION_US, &mibUsTD);
	                }

	                if (isDsTdValid)
	                {
	                	omci_apply_traffic_descriptor_to_gem_port(
        					pMacBriPortCfgData->TPPointer, PON_GEMPORT_DIRECTION_DS, &mibDsTD);
	                }
	            }
	        }

			break;
		}
		case MIB_DEL:
		{
			/*Set Mac learn depth of UNI port to default*/
			if(MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType &&
                FAL_OK != feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP) &&
                OMCI_DEV_MODE_ROUTER != gInfo.devMode)
			{
				UINT16 portIdx;
				UINT32 newPortMask;
				UINT32 oldPortMask;
                omci_flood_port_info floodPortInfo;

				/*Get port index form entity id*/
				if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pMacBriPortCfgData->TPPointer, &portIdx))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "PPTP instance ID translate error: 0x%x", pMacBriPortCfgData->TPPointer);
					return GOS_FAIL;
				}
				omci_wrapper_setMacLearnLimit((unsigned int)portIdx, gInfo.devCapabilities.totalL2Num);

				if (GOS_OK != omci_get_pptp_eth_uni_port_mask_in_bridge(pMacBriPortCfgData->BridgeIdPtr, &newPortMask))
			    {
			        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get pptp eth uni port mask in bridge fail");
			    }
			    else
			    {
			    	oldPortMask = newPortMask | (1 << portIdx);

			    	omci_update_dot1_rate_limiter_port_mask(pMacBriPortCfgData->BridgeIdPtr, newPortMask, oldPortMask);

                    memset(&floodPortInfo, 0, sizeof(omci_flood_port_info));
                    floodPortInfo.type      = OMCI_FLOOD_UNICAST;
                    floodPortInfo.act       = OMCI_LOOKUP_MISS_ACT_DROP;
                    floodPortInfo.portMask  = (1 << portIdx);
                    omci_wrapper_setFloodingPortMask(&floodPortInfo);
			    }
			}
			else if (MBPCD_TP_TYPE_VEIP == pMacBriPortCfgData->TPType ||
                (FAL_OK == feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP) &&
                OMCI_DEV_MODE_ROUTER == gInfo.devMode))
			{
				MIB_TABLE_GEMPORTCTP_T	*pMibGPNC;
				MIB_TREE_T				*pTree;
				MIB_NODE_T				*pNode;
				MIB_TREE_NODE_ENTRY_T   *pNodeEntry;
				MIB_ENTRY_T				*pEntry;
				UINT32					oldPortMask;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_GEMPORTCTP);
					if (pNode)
					{
						LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
						{
							pEntry = pNodeEntry->mibEntry;
							pMibGPNC = (MIB_TABLE_GEMPORTCTP_T *)pEntry->pData;

							if (GPNC_DIRECTION_ANI_TO_UNI != pMibGPNC->Direction)
								omci_wrapper_setUsVeipPriQ(pTree, pMibGPNC->PortID, FALSE);
						}
					}
				}

				if (GOS_OK != omci_get_eth_uni_port_mask_behind_veip(pMacBriPortCfgData->BridgeIdPtr, &oldPortMask))
			    {
			        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get eth uni port mask behind veip fail");
			    }
			    else
			    {
			    	omci_update_dot1_rate_limiter_port_mask(pMacBriPortCfgData->BridgeIdPtr, 0, oldPortMask);
			    }
			}
			else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType)
			{
				MIB_TABLE_MAP8021PSERVPROF_T	mib8021pmsp;
				MIB_TABLE_GEMIWTP_T				mibGemIwTp;
				MIB_TABLE_GEMPORTCTP_T			mibGPNC;
				MIB_TREE_T						*pTree;
				UINT16                          *pIwtpPtrOffset, IwtpPtr = 0xffff;
				UINT8							bFound;
			    UINT8                           i;

				pTree = MIB_AvlTreeSearchByKey(NULL, pMacBriPortCfgData->BridgeIdPtr, AVL_KEY_MACBRISERVPROF);
				if (pTree)
				{
					mib8021pmsp.EntityID = pMacBriPortCfgData->TPPointer;
					if (GOS_OK == MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib8021pmsp, sizeof(mib8021pmsp)))
					{
						pIwtpPtrOffset = &mib8021pmsp.IwTpPtrPbit0;
						bFound = FALSE;

						for (i = 0; i < 8; i++)
			            {
			            	if(*(pIwtpPtrOffset + i) != 0xffff)
			            	{
			            		IwtpPtr = *(pIwtpPtrOffset + i);
			            		break;
			            	}
			            }

			            if (!bFound)
			            {
			            	mibGemIwTp.EntityID = IwtpPtr;
							if (GOS_OK == MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
							{
								mibGPNC.EntityID = mibGemIwTp.GemCtpPtr;
								if (GOS_OK == MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGPNC, sizeof(mibGPNC)))
								{
									if (GPNC_DIRECTION_ANI_TO_UNI != mibGPNC.Direction)
										omci_wrapper_setUsVeipPriQ(pTree, mibGPNC.PortID, FALSE);
								}
							}
			            }
					}
				}
			}
			else if (MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
				MIB_TABLE_GEMIWTP_T				mibGemIwTp;
				MIB_TABLE_MACBRIPORTCFGDATA_T	mibMBPCD;
				UINT8							bOtherDsBcGemConnected;

				// assume that there is only one DS BC GEM in the system
				mibGemIwTp.EntityID = pMacBriPortCfgData->TPPointer;
				if (GOS_OK == MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
			    {
			    	if (GEM_IWTP_IW_OPTION_DOWNSTREAM_BROADCAST == mibGemIwTp.IwOpt)
			    	{
			    		bOtherDsBcGemConnected = FALSE;

					    ret = MIB_GetFirst(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
					    while (GOS_OK == ret)
					    {
					    	if (mibMBPCD.EntityID == pMacBriPortCfgData->EntityID)
					    		goto get_next;

					    	if (MBPCD_TP_TYPE_GEM_IWTP != mibMBPCD.TPType)
					    		goto get_next;

					    	mibGemIwTp.EntityID = mibMBPCD.TPPointer;
							if (GOS_OK != MIB_Get(MIB_TABLE_GEMIWTP_INDEX, &mibGemIwTp, sizeof(mibGemIwTp)))
								goto get_next;

					        if (GEM_IWTP_IW_OPTION_DOWNSTREAM_BROADCAST == mibGemIwTp.IwOpt)
					        {
					        	bOtherDsBcGemConnected = TRUE;
					        	break;
					        }

get_next:
					        ret = MIB_GetNext(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(mibMBPCD));
					    }

					    if (!bOtherDsBcGemConnected)
						    omci_wrapper_setDsBcGemFlow(UINT_MAX);
					}
			    }
			}

			// invoke traffic descriptor settings
			if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType ||
					MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
				if (GOS_OK == omci_is_traffic_descriptor_existed(pMacBriPortCfgData->OutboundTD, &mibUsTD))
					isUsTdValid = TRUE;
			}
            else if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
            {
                isDsTdValid = TRUE;
            }
			if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType ||
					MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType ||
					MBPCD_TP_TYPE_MCAST_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
				if (GOS_OK == omci_is_traffic_descriptor_existed(pMacBriPortCfgData->InboundTD, &mibDsTD))
					isDsTdValid = TRUE;
			}
            else if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
			{
				isUsTdValid = TRUE;
			}
			if (isUsTdValid || isDsTdValid)
			{

			    if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
                {
                    mibEthUni.EntityID = pMacBriPortCfgData->TPPointer;

	                if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibEthUni, sizeof(mibEthUni)))
	                    break;

                    if (isUsTdValid)
                    {
	                	omci_apply_traffic_descriptor_to_uni_port(
        					pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_INGRESS, NULL);
                    }

                    if (isDsTdValid)
                    {
	                	omci_apply_traffic_descriptor_to_uni_port(
        					pMacBriPortCfgData->TPPointer, OMCI_UNI_RATE_DIRECTION_EGRESS, NULL);
                    }
                }
				else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pMacBriPortCfgData->TPType)
	            {
	                mib1pMapper.EntityID = pMacBriPortCfgData->TPPointer;

	                if (GOS_OK != MIB_Get(MIB_TABLE_MAP8021PSERVPROF_INDEX, &mib1pMapper, sizeof(mib1pMapper)))
	                    break;

	                pIwtpPtrOffset = &mib1pMapper.IwTpPtrPbit0;

	                for (i = 0; i < 8; i++)
	                {
	                    if (isUsTdValid)
	                    {
	                    	omci_apply_traffic_descriptor_to_gem_port(
            					*(pIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_US, NULL);
	                    }

	                    if (isDsTdValid)
	                    {
	                    	omci_apply_traffic_descriptor_to_gem_port(
            					*(pIwtpPtrOffset + i), PON_GEMPORT_DIRECTION_DS, NULL);
	                    }
	                }
	            }
	            else
	            {
	                if (isUsTdValid)
	                {
	                	omci_apply_traffic_descriptor_to_gem_port(
        					pMacBriPortCfgData->TPPointer, PON_GEMPORT_DIRECTION_US, NULL);
	                }

	                if (isDsTdValid)
	                {
	                	omci_apply_traffic_descriptor_to_gem_port(
        					pMacBriPortCfgData->TPPointer, PON_GEMPORT_DIRECTION_DS, NULL);
	                }
	            }
	        }

			ret = mib_alarm_table_del(tableIndex, pOldRow);
			break;
		}
		default:
			ret = GOS_OK;
			break;
	}

	return ret;
}

GOS_ERROR_CODE 
MacBriPortCfgDataConnCfg(
    void* pOldRow,
    void* pNewRow,
    MIB_OPERA_TYPE  operationType, 
    MIB_ATTRS_SET attrSet, 
    UINT32 pri)
{
	MIB_TABLE_MACBRIPORTCFGDATA_T* pPort, *pOldPort = NULL;
	MIB_TREE_T *pTree = NULL, *pOldTree = NULL;
	int key, vf_key, vo_key, extv_key;
    MIB_TREE_NODE_ENTRY_T *pNodeEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    MIB_TABLE_VLANTAGFILTERDATA_T *pVtfd = NULL;
    MIB_TABLE_VLANTAGOPCFGDATA_T  *pVtod = NULL;
    MIB_TABLE_EXTVLANTAGOPERCFGDATA_T *pExtVlan = NULL;
    MIB_NODE_T  *pNode = NULL;
    BOOL bMatchVtodRem = FALSE, bMatchExtVlanRem = FALSE;
    unig_attr_mgmt_capability_t     mngCapUnig = UNIG_MGMT_CAPABILITY_BOTH_OMCI_NON_OMCI;
    //
    // Check manage Capability of UNI-G
    //
    if ((MBPCD_TP_TYPE_PPTP_ETH_UNI == ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPType)
        && omci_mngCapOfUnigGet (((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer, &mngCapUnig) != GOS_OK)
    {
        OMCI_LOG (
            OMCI_LOG_LEVEL_WARN,
            "Failed to get UNI-G for TPPointer(0x%03x) of MBPCD (0x%03x)\n",
            ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer,
            ((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->EntityID);
    }

    if (UNIG_MGMT_CAPABILITY_NON_OMCI_ONLY == mngCapUnig)
    {
        OMCI_LOG(
			OMCI_LOG_LEVEL_WARN, "%s: ManageCapability of UNIG(0x%03X) is NON_OMCI_ONLY", 
			__FUNCTION__,
			((MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow)->TPPointer);
        return GOS_OK;
    }

	switch (operationType){
	case MIB_ADD:
	{
    		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Port Configure ---- > ADD");
		/*check if new connection is complete*/
		pPort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow;
		feature_api(FEATURE_API_ME_00000008, pPort);
		key = MacBriPortCfgDataGetKey(pPort->TPType);
		/*find avl tree*/
		pTree = MIB_AvlTreeSearchByKey(NULL, pPort->BridgeIdPtr,AVL_KEY_MACBRISERVPROF);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_OK;
		}
		/*add new node to tree*/
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"key is %d, TP Type %d, TP Ptr %x",key,pPort->TPType,pPort->TPPointer);
		if(MIB_AvlTreeNodeAdd(&pTree->root,key,MIB_TABLE_MACBRIPORTCFGDATA_INDEX,pPort)==NULL)
		{
			return GOS_OK;
		}
		if(pPort->TPType == MBPCD_TP_TYPE_PPTP_ETH_UNI)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_ETHUNI_INDEX,pTree,pPort->TPPointer);
		}else
		if(pPort->TPType == MBPCD_TP_TYPE_VEIP)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_VEIP_INDEX,pTree,pPort->TPPointer);
		}else
		if(pPort->TPType == MBPCD_TP_TYPE_IEEE_8021P_MAPPER)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_MAP8021PSERVPROF_INDEX,pTree,pPort->TPPointer);
		}else
		if(pPort->TPType == MBPCD_TP_TYPE_GEM_IWTP)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX, pTree, pPort->TPPointer);
		}else
		if(pPort->TPType == MBPCD_TP_TYPE_MCAST_GEM_IWTP)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_MULTIGEMIWTP_INDEX,pTree,pPort->TPPointer);
		}else
		if(pPort->TPType == MBPCD_TP_TYPE_IP_HOST_IPV6_HOST)
		{
			OMCI_MeOperAvlTreeAdd(MIB_TABLE_IP_HOST_CFG_DATA_INDEX,pTree,pPort->TPPointer);
		}
	break;
	}
	case MIB_SET:
	{
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Port Configure ---- > SET");
		/*check if new connection is complete*/
    	pPort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pNewRow;
        pOldPort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pOldRow;

    	feature_api(FEATURE_API_ME_00000008, pPort);

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX) &&
            MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX) &&
            pPort->TPPointer != pOldPort->TPPointer && pPort->TPType != pOldPort->TPType)
        {
            if (!(pOldTree = MIB_AvlTreeSearchByKey(NULL, pOldPort->BridgeIdPtr, AVL_KEY_MACBRISERVPROF)))
                return GOS_OK;

            if (pOldPort->TPType == MBPCD_TP_TYPE_PPTP_ETH_UNI)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_ETHUNI_INDEX, pOldTree, pOldPort->TPPointer);
    		}
            else if (pOldPort->TPType == MBPCD_TP_TYPE_VEIP)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_VEIP_INDEX, pOldTree, pOldPort->TPPointer);
     		}
            else if (pOldPort->TPType == MBPCD_TP_TYPE_IEEE_8021P_MAPPER)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_MAP8021PSERVPROF_INDEX, pOldTree, pOldPort->TPPointer);
    		}
            else if (pOldPort->TPType == MBPCD_TP_TYPE_GEM_IWTP)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pOldTree, pOldPort->TPPointer);
    		}
            else if (pOldPort->TPType == MBPCD_TP_TYPE_MCAST_GEM_IWTP)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_MULTIGEMIWTP_INDEX, pOldTree, pOldPort->TPPointer);
    		}
            else if (pOldPort->TPType == MBPCD_TP_TYPE_IP_HOST_IPV6_HOST)
    		{
    			OMCI_MeOperAvlTreeDel(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, pOldTree, pOldPort->TPPointer);
    		}


            key = MacBriPortCfgDataGetKey(pOldPort->TPType);

            vf_key = ((AVL_KEY_MACBRIPORT_UNI == key) ? AVL_KEY_VLANTAGFILTER_UNI : AVL_KEY_VLANTAGFILTER_ANI);
            if ((pNodeEntry = MIB_AvlTreeEntrySearch(pOldTree->root, vf_key, pOldPort->EntityID)) &&
                (pEntry = pNodeEntry->mibEntry) && (pVtfd = ((MIB_TABLE_VLANTAGFILTERDATA_T*)pEntry->pData)))
            {
                MIB_AvlTreeNodeEntryRemoveByKey(pOldTree->root, vf_key, pOldPort->EntityID);
            }

            vo_key = ((AVL_KEY_MACBRIPORT_UNI == key) ? AVL_KEY_VLANTAGOPCFG_UNI : AVL_KEY_VLANTAGOPCFG_ANI);

        	if((pNode = MIB_AvlTreeSearch(pOldTree->root, vo_key)))
        	{
        		LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
        		{
        		    if (!(pEntry = pNodeEntry->mibEntry))
                        continue;

                    if (!(pVtod = (MIB_TABLE_VLANTAGOPCFGDATA_T*)pEntry->pData))
                        continue;

                    if (VTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT == pVtod->Type &&
                        pOldPort->EntityID == pVtod->Pointer)
                    {
                        bMatchVtodRem = TRUE;
                        break;
                    }
        		}
        	}

            if (bMatchVtodRem)
                MIB_AvlTreeNodeEntryRemoveByKey(pOldTree->root, vo_key, pVtod->EntityID);

            extv_key = ((AVL_KEY_MACBRIPORT_UNI == key) ? AVL_KEY_EXTVLAN_UNI : AVL_KEY_EXTVLAN_ANI);

            if((pNode = MIB_AvlTreeSearch(pOldTree->root, extv_key)))
            {
                LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
                {
                    if (!(pEntry = pNodeEntry->mibEntry))
                        continue;

                    if (!(pExtVlan = (MIB_TABLE_EXTVLANTAGOPERCFGDATA_T*)pEntry->pData))
                        continue;

                    if (EVTOCD_ASSOC_TYPE_MAC_BRIDGE_PORT == pExtVlan->AssociationType &&
                        pOldPort->EntityID == pExtVlan->AssociatedMePoint)
                    {
                        bMatchExtVlanRem = TRUE;
                        break;
                    }
                }
            }

            if (bMatchExtVlanRem)
                MIB_AvlTreeNodeEntryRemoveByKey(pOldTree->root, extv_key, pExtVlan->EntityId);

            /* remove avltree node of mbpcd from old BridgeIdPtr */
            MIB_AvlTreeNodeEntryRemoveByKey(pOldTree->root, key, pOldPort->EntityID);

            /* find avl tree for new BridgeIdPrt */
    		if (!(pTree = MIB_AvlTreeSearchByKey(NULL, pPort->BridgeIdPtr, AVL_KEY_MACBRISERVPROF)))
    		{
    			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
    			return GOS_OK;
    		}
            key = MacBriPortCfgDataGetKey(pPort->TPType);
            /* add avltree node of mbpcd into new BridgeIdPrt*/
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "key is %d, TP Type %d, TP Ptr %x", key, pPort->TPType, pPort->TPPointer);
            if (MIB_AvlTreeNodeAdd(&pTree->root, key, MIB_TABLE_MACBRIPORTCFGDATA_INDEX, pPort) == NULL)
            {
                return GOS_OK;
            }
            if (pVtfd)
            {
                MIB_AvlTreeNodeAdd(&pTree->root, key, MIB_TABLE_VLANTAGFILTERDATA_INDEX, pVtfd);
            }
            if (bMatchVtodRem && pVtod)
            {
                MIB_AvlTreeNodeAdd(&pTree->root, key, MIB_TABLE_VLANTAGOPCFGDATA_INDEX, pVtod);
            }
            if (bMatchExtVlanRem && pExtVlan)
            {
                MIB_AvlTreeNodeAdd(&pTree->root, key, MIB_TABLE_EXTVLANTAGOPERCFGDATA_INDEX, pExtVlan);
            }
            // assume terminal pointer does not be associated
            if (pPort->TPType == MBPCD_TP_TYPE_PPTP_ETH_UNI)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_ETHUNI_INDEX, pTree, pPort->TPPointer);
    		}
            else if (pPort->TPType == MBPCD_TP_TYPE_VEIP)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_VEIP_INDEX, pTree, pPort->TPPointer);
    		}
            else if (pPort->TPType == MBPCD_TP_TYPE_IEEE_8021P_MAPPER)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_MAP8021PSERVPROF_INDEX, pTree, pPort->TPPointer);
    		}
            else if (pPort->TPType == MBPCD_TP_TYPE_GEM_IWTP)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_GEMIWTP_INDEX, pTree, pPort->TPPointer);
    		}
            else if (pPort->TPType == MBPCD_TP_TYPE_MCAST_GEM_IWTP)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_MULTIGEMIWTP_INDEX, pTree, pPort->TPPointer);
    		}
            else if (pPort->TPType == MBPCD_TP_TYPE_IP_HOST_IPV6_HOST)
    		{
    			OMCI_MeOperAvlTreeAdd(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, pTree, pPort->TPPointer);
    		}
            /* update connection */
        }

	break;
	}
	case MIB_DEL:
	{
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Port Configure ---- > DEL");
		/*check if new connection is complete*/
        pPort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pOldRow;
        feature_api(FEATURE_API_ME_00000008, pPort);
		MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T    macBridgePortFilterTable;
        MIB_TABLE_MACBRIPORTBRITBLDATA_T        macBridgePortBridgeTable;

		/* Delete related MAC bridge port filter table data ME */
		memset(&macBridgePortFilterTable, 0, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T));
		macBridgePortFilterTable.EntityId = pPort->EntityID;
		if(GOS_OK == MIB_Get(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX, &macBridgePortFilterTable,
			sizeof(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T)))
		{
			OMCI_MeOperCfg(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX,
				&macBridgePortFilterTable, &macBridgePortFilterTable,
				MIB_DEL, 0, OMCI_MSG_BASELINE_PRI_LOW);

			if (GOS_OK != MIB_Delete(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX,
				&macBridgePortFilterTable, sizeof(MIB_TABLE_MACBRIDGEPORTFILTERTABLE_T)))
			{
				OMCI_LOG(OMCI_LOG_LEVEL_WARN," MIB_Delete error, tableId=%u, entity=0x%x",
					MIB_TABLE_MACBRIDGEPORTFILTERTABLE_INDEX,
					macBridgePortFilterTable.EntityId);
			}
		}

		/* Delete related MAC bridge port bridge table data ME */
		memset(&macBridgePortBridgeTable, 0, sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T));
		macBridgePortBridgeTable.EntityID = pPort->EntityID;
		if(GOS_OK == MIB_Get(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX, &macBridgePortBridgeTable,
			sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T)))
		{
			/*OMCI_MeOperCfg(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX,
				&macBridgePortBridgeTable, &macBridgePortBridgeTable,
				MIB_DEL, 0, OMCI_MSG_BASELINE_PRI_LOW);*/

			if (GOS_OK != MIB_Delete(MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX,
				&macBridgePortBridgeTable, sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T)))
			{
				OMCI_LOG(OMCI_LOG_LEVEL_WARN," MIB_Delete error, tableId=%u, entity=0x%x",
					MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX,
					macBridgePortBridgeTable.EntityID);
			}
		}
		key = MacBriPortCfgDataGetKey(pPort->TPType);
            	/*find avl tree*/
		pTree = MIB_AvlTreeSearchByKey(NULL, pPort->BridgeIdPtr,AVL_KEY_MACBRISERVPROF);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_OK;
		}
		/*remove node entry from tree*/
		MIB_AvlTreeNodeEntryRemoveByKey(pTree->root,key,pPort->EntityID);
		if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_ETHUNI_INDEX, pTree, pPort->TPPointer);
		}
		else if (MBPCD_TP_TYPE_VEIP == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_VEIP_INDEX, pTree, pPort->TPPointer);
		}
		else if (MBPCD_TP_TYPE_IEEE_8021P_MAPPER == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_MAP8021PSERVPROF_INDEX, pTree, pPort->TPPointer);
		}
		else if (MBPCD_TP_TYPE_GEM_IWTP == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_GEMIWTP_INDEX, pTree, pPort->TPPointer);
		}
		else if (MBPCD_TP_TYPE_MCAST_GEM_IWTP == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_MULTIGEMIWTP_INDEX, pTree, pPort->TPPointer);
		}else if(MBPCD_TP_TYPE_IP_HOST_IPV6_HOST == pPort->TPType)
		{
			OMCI_MeOperAvlTreeDel(MIB_TABLE_IP_HOST_CFG_DATA_INDEX,pTree,pPort->TPPointer);
		}
	break;
	}
	default:
	    return GOS_OK;
	}

	if(pTree)
	{
		MIB_TreeConnUpdate(pTree);
	}
	return GOS_OK;
}

static GOS_ERROR_CODE mbpcd_alarm_handler(MIB_TABLE_INDEX		tableIndex,
											omci_alm_data_t		alarmData,
											omci_me_instance_t	*pInstanceID,
											BOOL				*pIsUpdated)
{
    mib_alarm_table_t	alarmTable;
    BOOL                isSuppressed;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    // TBD, should be the mac bridge port id
    *pInstanceID = alarmData.almDetail;

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

		// check if notifications are suppressed by parent's admin state
	    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
	}

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMacBriPortCfgDataTableInfo.Name = "MacBriPortCfgData";
    gMibMacBriPortCfgDataTableInfo.ShortName = "MBPCD";
    gMibMacBriPortCfgDataTableInfo.Desc = "MAC Bridge Port Configuration Data";
    gMibMacBriPortCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MAC_BRG_PORT_CFG_DATA);
    gMibMacBriPortCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMacBriPortCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMacBriPortCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMacBriPortCfgDataTableInfo.pAttributes = &(gMibMacBriPortCfgDataAttrInfo[0]);



	gMibMacBriPortCfgDataTableInfo.attrNum = MIB_TABLE_MACBRIPORTCFGDATA_ATTR_NUM;
	gMibMacBriPortCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T);
	gMibMacBriPortCfgDataTableInfo.pDefaultRow = &gMibMacBriPortCfgDataDefRow;


    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BridgeIdPtr";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortNum";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TPType";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TPPointer";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortPriority";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortPathCost";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortSpanTreeInd";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EncapMethod";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LanFCSInd";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortMacAddr";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OutboundTD";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "InboundTD";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfAllowedMac";

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bridge Id Pointer";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port Num";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TP Type";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TP Pointer";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port Priority";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port Path Cost";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port Spanning Tree Ind";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Encapsulation Method";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LAN FCS Ind";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port MAC Address";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Outbound TD Point";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Inbound TD Point";
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Number of Allowed MAC";

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 6;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_BRIDGEIDPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTNUM_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_TPPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTPATHCOST_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTSPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_ENCAPMETHOD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_LANFCSIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_OUTBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_INBOUNDTD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriPortCfgDataAttrInfo[MIB_TABLE_MACBRIPORTCFGDATA_NUMOFALLOWEDMAC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;


    memset(&(gMibMacBriPortCfgDataDefRow.EntityID), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.EntityID));
    memset(&(gMibMacBriPortCfgDataDefRow.BridgeIdPtr), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.BridgeIdPtr));
    memset(&(gMibMacBriPortCfgDataDefRow.PortNum), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.PortNum));
    gMibMacBriPortCfgDataDefRow.TPType = MIB_ATTR_DEF_SPACE;
    gMibMacBriPortCfgDataDefRow.TPPointer = MIB_ATTR_DEF_SPACE;
    memset(&(gMibMacBriPortCfgDataDefRow.PortPriority), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.PortPriority));
    gMibMacBriPortCfgDataDefRow.PortPathCost = MIB_ATTR_DEF_SPACE;
    memset(&(gMibMacBriPortCfgDataDefRow.PortSpanTreeInd), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.PortSpanTreeInd));
    memset(&(gMibMacBriPortCfgDataDefRow.EncapMethod), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.EncapMethod));
    memset(&(gMibMacBriPortCfgDataDefRow.LanFCSInd), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.LanFCSInd));
    memset(gMibMacBriPortCfgDataDefRow.PortMacAddr, 0, MIB_TABLE_MACBRIPORTCFGDATA_PORTMACADDR_LEN);
    memset(&(gMibMacBriPortCfgDataDefRow.OutboundTD), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.OutboundTD));
    memset(&(gMibMacBriPortCfgDataDefRow.InboundTD), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.InboundTD));
    memset(&(gMibMacBriPortCfgDataDefRow.NumOfAllowedMac), 0x00, sizeof(gMibMacBriPortCfgDataDefRow.NumOfAllowedMac));

    memset(&gMibMacBriPortCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibMacBriPortCfgDataOper.meOperDrvCfg = MacBriPortCfgDataDrvCfg;
	gMibMacBriPortCfgDataOper.meOperConnCheck = MacBriPortCfgDataConnCheck;
	gMibMacBriPortCfgDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibMacBriPortCfgDataOper.meOperConnCfg = MacBriPortCfgDataConnCfg;
	gMibMacBriPortCfgDataOper.meOperAlarmHandler = mbpcd_alarm_handler;
	MIB_TABLE_MACBRIPORTCFGDATA_INDEX = tableId;

	MIB_InfoRegister(tableId,&gMibMacBriPortCfgDataTableInfo,&gMibMacBriPortCfgDataOper);


    return GOS_OK;
}
