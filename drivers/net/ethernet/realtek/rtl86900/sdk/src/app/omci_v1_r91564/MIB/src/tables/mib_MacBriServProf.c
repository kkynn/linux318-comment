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
 * Purpose : Definition of ME handler: MAC bridge service profile (45)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: MAC bridge service profile (45)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibMacBriServProfTableInfo;
MIB_ATTR_INFO_T  gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATTR_NUM];
MIB_TABLE_MACBRISERVPROF_T gMibMacBriServProfDefRow;
MIB_TABLE_OPER_T		   gMibMacBriServProfOper;


GOS_ERROR_CODE MacBriServProfDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    MIB_TABLE_MACBRISERVPROF_T      *pBridge        = NULL, *pOldBridge = NULL, mibBridge;
    MIB_TREE_T                      *pTree          = NULL;
    MIB_NODE_T                      *pUniNode       = NULL;
    MIB_TREE_DATA_T                 *pUniData       = NULL;
    UINT32                          ageTime         = 300;
    omci_flood_port_info            floodPortInfo;

    pBridge = (MIB_TABLE_MACBRISERVPROF_T *)pNewRow;
	pOldBridge = (MIB_TABLE_MACBRISERVPROF_T *)pOldRow;

    switch (operationType)
    {
        case MIB_ADD:
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX))
            {
                if (0 !=  pBridge->DynamicFilteringAgeingTime)
                {
                    ageTime = pBridge->DynamicFilteringAgeingTime;
                }
                omci_wrapper_setAgeingTime(ageTime);
            }
			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX))
			{
				if ((pOldBridge->AtmBriInd != pBridge->AtmBriInd) || (operationType == MIB_ADD))
	            {
	            	feature_api(FEATURE_API_ME_00004000, &pBridge->AtmBriInd);
				}
    		}
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX) &&
                pOldBridge->DiscardUnknow != pBridge->DiscardUnknow && operationType == MIB_SET)
            {
                // make sure uni relationship is binding.
                if (!(pTree = MIB_AvlTreeSearchByKey(NULL, pBridge->EntityID, AVL_KEY_MACBRISERVPROF)))
                    goto finish;

                if (!(pUniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_UNI)))
                    goto finish;

                if (!(pUniData = &pUniNode->data))
                    goto finish;

                memset(&floodPortInfo, 0, sizeof(omci_flood_port_info));

                if (GOS_OK != omci_get_pptp_eth_uni_port_mask_in_bridge(pBridge->EntityID, &floodPortInfo.portMask))
			        goto finish;

                floodPortInfo.type = OMCI_FLOOD_UNICAST;
                floodPortInfo.act = (TRUE == pBridge->DiscardUnknow ? OMCI_LOOKUP_MISS_ACT_DROP : OMCI_LOOKUP_MISS_ACT_FLOOD);

                omci_wrapper_setFloodingPortMask(&floodPortInfo);
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX) &&
                pOldBridge->MacLearningDepth != pBridge->MacLearningDepth && operationType == MIB_SET)
            {
                UINT32  mac_svc_total = 0, eth_total = 0, count = 0;
                UINT16  port_id;

                eth_total = MIB_GetTableCurEntryCount(MIB_TABLE_ETHUNI_INDEX);
                mac_svc_total = MIB_GetTableCurEntryCount(MIB_TABLE_MACBRISERVPROF_INDEX);

                if (eth_total > mac_svc_total)
                    goto finish;

                if (GOS_OK != (MIB_GetFirst(MIB_TABLE_MACBRISERVPROF_INDEX, &mibBridge, sizeof(MIB_TABLE_MACBRISERVPROF_T))))
                    return GOS_OK;

                while (count < mac_svc_total)
                {
                    count++;
                    UINT16 eth_uni_instance = 0xFFFF;

                    if (!(pTree = MIB_AvlTreeSearchByKey(NULL, mibBridge.EntityID, AVL_KEY_MACBRISERVPROF)))
                        goto next_mac_svc;

                    if (omci_is_one_pptp_eth_uni_number_in_bridge(pTree, &eth_uni_instance))
                    {
                        if (mibBridge.EntityID == pBridge->EntityID)
                        {
                            if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(eth_uni_instance, &port_id))
                            {
                                omci_wrapper_setMacLearnLimit((unsigned int)port_id,
                                                            (unsigned int)(pBridge->MacLearningDepth));
                            }
                        }
                    }

                next_mac_svc:
                    if (GOS_OK != (MIB_GetNext(MIB_TABLE_MACBRISERVPROF_INDEX, &mibBridge, sizeof(MIB_TABLE_MACBRISERVPROF_T))))
                        continue;

                }
            }
            break;
        case MIB_DEL:
            omci_wrapper_setAgeingTime(ageTime);
			omci_wrapper_setPortBridging(TRUE);
            break;
        default:
            return GOS_OK;
    }
finish:
	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: process end\n", __FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE MacBriServProfConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_MACBRISERVPROF_T* pBridge;
	MIB_TREE_T *pTree;


	switch (operationType){
	case MIB_ADD:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Service Profile ----> ADD");
		/*check if new connection is complete*/
		pBridge = (MIB_TABLE_MACBRISERVPROF_T *)pNewRow;
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"bridge id %d", pBridge->EntityID);
		pTree = MIB_AvlTreeCreate(pBridge);

		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Create Bridge Tree faild");
			return GOS_FAIL;
		}
		MIB_TreeConnUpdate(pTree);
	break;
	}
	case MIB_SET:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Service Profile ----> SET");
		/*check if new connection is complete*/
		pBridge = (MIB_TABLE_MACBRISERVPROF_T *)pNewRow;
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"bridge id %d", pBridge->EntityID);
		pTree = MIB_AvlTreeSearchByKey(NULL, pBridge->EntityID,AVL_KEY_MACBRISERVPROF);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Can't find Root Tree");
			return GOS_FAIL;
		}
		MIB_TreeConnUpdate(pTree);
	break;
	}
	case MIB_DEL:
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Bridge Service Profile ---- > DEL");
		/*check if new connection is complete*/
		pBridge = (MIB_TABLE_MACBRISERVPROF_T *)pOldRow;
		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"bridge id %d", pBridge->EntityID);
		pTree = MIB_AvlTreeSearchByKey(NULL, pBridge->EntityID,AVL_KEY_MACBRISERVPROF);
		if(pTree==NULL)
		{
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Search Old Bridge Tree faild");
		return GOS_FAIL;
		}

		/*remove tree*/
		MIB_AvlTreeRemove(pTree);
	break;
	}
	default:
	return GOS_OK;
	}


	return GOS_OK;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
	gMibMacBriServProfTableInfo.Name = "MacBriServProf";
	gMibMacBriServProfTableInfo.ShortName = "MBSP";
	gMibMacBriServProfTableInfo.Desc = "MAC Bridge Service Profile";
	gMibMacBriServProfTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MAC_BRG_SRV_PROFILE);
	gMibMacBriServProfTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
	gMibMacBriServProfTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
	gMibMacBriServProfTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
	gMibMacBriServProfTableInfo.pAttributes = &(gMibMacBriServProfAttrInfo[0]);


	gMibMacBriServProfTableInfo.attrNum = MIB_TABLE_MACBRISERVPROF_ATTR_NUM;
	gMibMacBriServProfTableInfo.entrySize = sizeof(MIB_TABLE_MACBRISERVPROF_T);
	gMibMacBriServProfTableInfo.pDefaultRow = &gMibMacBriServProfDefRow;


    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SpanTreeInd";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LearningInd";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AtmBriInd";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Priority";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxAge";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "HelloTime";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ForwardDelay";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DiscardUnknow";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MacLearningDepth";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DynamicFilteringAgeingTime";

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "SpanningTreeInd";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Learning Indication";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ATMPortBridgingInd";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Priority";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "MaxAge";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "HelloTime";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ForwardDelay";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "DiscardUnknow";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "MacLearningDepth";
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dynamic filtering ageing time";

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_SPANTREEIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_LEARNINGIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_ATMBRIIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_PRIORITY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MAXAGE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_HELLOTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_FORWARDDELAY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DISCARDUNKNOW_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_MACLEARNINGDEPTH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibMacBriServProfAttrInfo[MIB_TABLE_MACBRISERVPROF_DYNAMIC_FILTERING_AGEING_TIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;


    memset(&(gMibMacBriServProfDefRow.EntityID), 0x00, sizeof(gMibMacBriServProfDefRow.EntityID));
    memset(&(gMibMacBriServProfDefRow.SpanTreeInd), 0x00, sizeof(gMibMacBriServProfDefRow.SpanTreeInd));
    memset(&(gMibMacBriServProfDefRow.LearningInd), 0x00, sizeof(gMibMacBriServProfDefRow.LearningInd));
    memset(&(gMibMacBriServProfDefRow.AtmBriInd), 0x00, sizeof(gMibMacBriServProfDefRow.AtmBriInd));
    memset(&(gMibMacBriServProfDefRow.Priority), 0x00, sizeof(gMibMacBriServProfDefRow.Priority));
    memset(&(gMibMacBriServProfDefRow.MaxAge), 0x00, sizeof(gMibMacBriServProfDefRow.MaxAge));
    memset(&(gMibMacBriServProfDefRow.HelloTime), 0x00, sizeof(gMibMacBriServProfDefRow.HelloTime));
    memset(&(gMibMacBriServProfDefRow.ForwardDelay), 0x00, sizeof(gMibMacBriServProfDefRow.ForwardDelay));
    memset(&(gMibMacBriServProfDefRow.DiscardUnknow), 0x00, sizeof(gMibMacBriServProfDefRow.DiscardUnknow));
    memset(&(gMibMacBriServProfDefRow.MacLearningDepth), 0x00, sizeof(gMibMacBriServProfDefRow.MacLearningDepth));
    gMibMacBriServProfDefRow.DynamicFilteringAgeingTime = 300;


    memset(&gMibMacBriServProfOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibMacBriServProfOper.meOperDrvCfg = MacBriServProfDrvCfg;
	gMibMacBriServProfOper.meOperConnCheck = NULL;
	gMibMacBriServProfOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibMacBriServProfOper.meOperConnCfg = MacBriServProfConnCfg;
	MIB_TABLE_MACBRISERVPROF_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibMacBriServProfTableInfo,&gMibMacBriServProfOper);

    return GOS_OK;
}
