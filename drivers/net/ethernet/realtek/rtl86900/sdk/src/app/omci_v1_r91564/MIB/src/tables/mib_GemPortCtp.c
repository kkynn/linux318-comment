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
 * Purpose : Definition of ME handler: GEM port network CTP (268)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: GEM port network CTP (268)
 */

#include "app_basic.h"
#include <time.h>
#include "feature_mgmt.h"

MIB_TABLE_INFO_T gMibGemPortCtpTableInfo;
MIB_ATTR_INFO_T  gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ATTR_NUM];
MIB_TABLE_GEMPORTCTP_T gMibGemPortCtpDefRow;
MIB_TABLE_OPER_T       gMibGemPortCtpOper;

static void omci_update_conn_by_gemctp(UINT16 entityId)
{
    MIB_TREE_T              *pTree = NULL;
    MIB_NODE_T              *pAniNode = NULL, *pUniNode = NULL;
    MIB_TREE_DATA_T         *pAniData = NULL, *pUniData = NULL;


    if (!(pTree = MIB_AvlTreeSearchByKey(NULL, entityId, AVL_KEY_GEMPORTCTP)))
        return;

    if (!(pAniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_ANI)))
        return;

    if (!(pAniData = &pAniNode->data))
        return;

    if (LIST_EMPTY(&pAniData->treeNodeEntryHead))
        return;

    if (!(pUniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_UNI)))
        return;

    if (!(pUniData = &pUniNode->data))
        return;

    if (LIST_EMPTY(&pUniData->treeNodeEntryHead))
        return;

    MIB_TreeConnUpdate(pTree);

}


GOS_ERROR_CODE GemPortCtpAvlTreeAdd(MIB_TREE_T* pTree,UINT16 gemPortPtr)
{

    MIB_TABLE_GEMPORTCTP_T* pGemPortCtp;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"GemPortCtpAvlTreeAdd");

    if(pTree==NULL) return GOS_FAIL;

    pGemPortCtp = (MIB_TABLE_GEMPORTCTP_T*)malloc(sizeof(MIB_TABLE_GEMPORTCTP_T));
    if(!pGemPortCtp)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Malloc GemPortCtp Fail");
        return GOS_FAIL;
    }
    memset(pGemPortCtp,0,sizeof(MIB_TABLE_GEMPORTCTP_T));
    pGemPortCtp->EntityID = gemPortPtr;
    if(MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, pGemPortCtp, sizeof(MIB_TABLE_GEMPORTCTP_T))!=GOS_OK)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Get Gem Port CTP Fail");
        free(pGemPortCtp);
        return GOS_FAIL;
    }

    if(MIB_AvlTreeNodeAdd(&pTree->root,AVL_KEY_GEMPORTCTP,MIB_TABLE_GEMPORTCTP_INDEX,pGemPortCtp)==NULL)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Add GemPortCtp Node Fail");
        free(pGemPortCtp);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE GemPortCtpAvlTreeDel(MIB_TREE_T *pTree, UINT16 gemPortPtr)
{
    MIB_TABLE_GEMPORTCTP_T  mibGemPortCtp;

    if (NULL == pTree)
        return GOS_FAIL;

    mibGemPortCtp.EntityID = gemPortPtr;
    if (GOS_OK != MIB_Get(MIB_TABLE_GEMPORTCTP_INDEX, &mibGemPortCtp, sizeof(mibGemPortCtp)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMPORTCTP_INDEX), mibGemPortCtp.EntityID);

        return GOS_FAIL;
    }

    if (GOS_OK != MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, AVL_KEY_GEMPORTCTP, mibGemPortCtp.EntityID))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Remove AVL node fail in %s: %s, 0x%x, tree 0x%p",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_GEMPORTCTP_INDEX), mibGemPortCtp.EntityID, pTree);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE GemPortCtpConnCheck(MIB_TREE_T *pTree,MIB_TREE_CONN_T *pConn,omci_me_instance_t entityId,int parm)
{
    MIB_TREE_NODE_ENTRY_T *pNodeEntry;
    MIB_ENTRY_T *pEntry;
    MIB_TABLE_GEMPORTCTP_T *pGemPortCtp;


    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Start %s...",__FUNCTION__);
    pNodeEntry = MIB_AvlTreeEntrySearch(pTree->root,AVL_KEY_GEMPORTCTP,entityId);

    if(!pNodeEntry){
        return GOS_FAIL;
    }

    if((OMCI_TRAF_MODE_8021P_BASE == pConn->traffMode && parm>=8)||
       (OMCI_TRAF_MODE_FLOW_BASE == pConn->traffMode && parm!=0))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s:traffMode:%d para:%d error\n",
            __FUNCTION__,pConn->traffMode,parm);
        return GOS_FAIL;
    }

    pEntry = pNodeEntry->mibEntry;
    pGemPortCtp = (MIB_TABLE_GEMPORTCTP_T*)pEntry->pData;
    pConn->pGemPortCtp[parm] = pGemPortCtp;
    return GOS_OK;
}

GOS_ERROR_CODE GemPortCtpConnCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    MIB_AVL_KEY_T key;
    MIB_TREE_T *pTree;
    MIB_TABLE_GEMPORTCTP_T* pGemPortCtp;

    switch (operationType){
    case MIB_DEL:
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Gem Port Ctp ---- > DEL");
        pGemPortCtp = (MIB_TABLE_GEMPORTCTP_T *)pOldRow;
        key = AVL_KEY_GEMPORTCTP;

        pTree = NULL;
        /*find all tree node entries of GemPortCTP*/
        while(1)
        {
            pTree = MIB_AvlTreeSearchByKey(pTree,pGemPortCtp->EntityID,AVL_KEY_GEMPORTCTP);
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "GemPortCTP in pTree [%p]\n", pTree);
            if(!pTree)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Search GemPortCTP tree faild");
                return GOS_OK;
            }

            /*remove node from tree*/
            MIB_AvlTreeNodeEntryRemoveByKey(pTree->root, key,pGemPortCtp->EntityID);

            /*check connection*/
            MIB_TreeConnUpdate(pTree);
        }
        break;
    }
    default:
        return GOS_OK;
    }

    return GOS_OK;
}

GOS_ERROR_CODE GemPortCtpDrvCfg(void            *pOldRow,
                                void            *pNewRow,
                                MIB_OPERA_TYPE  operationType,
                                MIB_ATTRS_SET   attrSet,
                                UINT32          pri)
{
    GOS_ERROR_CODE                  ret = GOS_OK;
    MIB_TABLE_GEMPORTCTP_T          *pGemPortCtp, *pOldGemPortCtp;
    OMCI_GEM_FLOW_ts                flowCfg;
    MIB_TABLE_TRAFFICDESCRIPTOR_T   mibTD;
    MIB_TABLE_PRIQ_T                tPQ;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_GEMPORTCTP_INDEX;

    pOldGemPortCtp = (MIB_TABLE_GEMPORTCTP_T *)pOldRow;
    pGemPortCtp = (MIB_TABLE_GEMPORTCTP_T *)pNewRow;
    memset(&flowCfg, 0, sizeof(OMCI_GEM_FLOW_ts));
    if (GPNC_DIRECTION_ANI_TO_UNI == pGemPortCtp->Direction)
        flowCfg.dir = PON_GEMPORT_DIRECTION_DS;
    else
    {
        tPQ.EntityID = pGemPortCtp->UsTraffMgmtPtr;

        // on the premise of ONU-G's traffic management option is 0 or 2
        ret = MIB_Get(MIB_TABLE_PRIQ_INDEX, &tPQ, sizeof(MIB_TABLE_PRIQ_T));
        if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_ME_00010000))
        {
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Invalid Priority queue for GEM Port");

                return ret;
            }

            flowCfg.tcontId = pGemPortCtp->TcAdapterPtr;
            flowCfg.queueId = pGemPortCtp->UsTraffMgmtPtr;
        }
        else
        {
            if (FAL_OK != feature_api(FEATURE_API_ME_00010000, pGemPortCtp, &flowCfg))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Invalid Priority queue for GEM Port in cs");

                return ret;
            }
        }
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
            "GEM port -> T-Cont ID [0x%x] PQ ID [0x%x]", flowCfg.tcontId, flowCfg.queueId);

        flowCfg.dir = (GPNC_DIRECTION_BIDIRECTION == pGemPortCtp->Direction) ?
            PON_GEMPORT_DIRECTION_BI : PON_GEMPORT_DIRECTION_US;
    }

    flowCfg.portId = pGemPortCtp->PortID;
    flowCfg.isOmcc = FALSE;
    flowCfg.isFilterMcast = FALSE;

    tPQ.EntityID = pGemPortCtp->DsPriQPtr;
    if (GOS_OK == MIB_Get(MIB_TABLE_PRIQ_INDEX, &tPQ, sizeof(MIB_TABLE_PRIQ_T)))
    {
        UINT16 phyPortId;
        flowCfg.dsQInfo.dsPqOmciPri = tPQ.RelatedPort & 0xFFFF;

        if (GOS_OK == pptp_eth_uni_me_id_to_switch_port(tPQ.RelatedPort >> 16, &phyPortId))
        {
            flowCfg.dsQInfo.portId = phyPortId;
            flowCfg.dsQInfo.policy = (tPQ.Weight > 1 ? PQ_POLICY_WEIGHTED_ROUND_ROBIN : PQ_POLICY_STRICT_PRIORITY);
            flowCfg.dsQInfo.priority = gInfo.devCapabilities.perUNIQueueNum - (tPQ.RelatedPort & 0x7) - 1;
            flowCfg.dsQInfo.weight = tPQ.Weight;
            flowCfg.dsQInfo.dpMarking = tPQ.DropPColorMarking;
        }
    }
    else
    {
        flowCfg.dsQInfo.dsPqOmciPri = 0xFFFF;
    }

    switch (operationType)
    {
        case MIB_ADD:
            if (GOS_OK != mib_alarm_table_add(tableIndex, pNewRow))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Add alarm table fail: %s",
                MIB_GetTableName(tableIndex));
            }
            flowCfg.ena = TRUE;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "GEM port -> ADD");

            break;
        case MIB_DEL:

            // invoke traffic descriptor settings
            if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->UsTraffDescPtr, &mibTD))
            {
                omci_apply_traffic_descriptor_to_gem_port(
                    pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_US, NULL);
            }
            if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->DsTraffDescPtr, &mibTD))
            {
                omci_apply_traffic_descriptor_to_gem_port(
                    pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_DS, NULL);
            }

            if (GOS_OK != mib_alarm_table_del(tableIndex, pOldRow))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Del alarm table fail: %s",
                MIB_GetTableName(tableIndex));
            }
            flowCfg.ena = FALSE;
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "GEM port -> DEL");

            break;

        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX) &&
                pOldGemPortCtp->DsPriQPtr != pGemPortCtp->DsPriQPtr)
            {
                flowCfg.ena = TRUE;
                /* ZTE OLT set ds queue entity id after creating gem port ctp */
                if((ret = omci_wrapper_updateGemFlow(flowCfg)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Update Gem Flow Fail: 0x%X", ret);

                omci_update_conn_by_gemctp(pGemPortCtp->EntityID);
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX) &&
                pOldGemPortCtp->UsTraffDescPtr != pGemPortCtp->UsTraffDescPtr)
            {
                if (0 == pGemPortCtp->UsTraffDescPtr)
                {
                    omci_apply_traffic_descriptor_to_gem_port(
                        pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_US, NULL);
                }
                else
                {
                    if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->UsTraffDescPtr, &mibTD))
                    {
                        omci_apply_traffic_descriptor_to_gem_port(
                            pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_US, &mibTD);
                    }
                }
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX) &&
                pOldGemPortCtp->DsTraffDescPtr != pGemPortCtp->DsTraffDescPtr)
            {
                if (0 == pGemPortCtp->DsTraffDescPtr)
                {
                    omci_apply_traffic_descriptor_to_gem_port(
                        pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_DS, NULL);
                }
                else
                {
                    if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->DsTraffDescPtr, &mibTD))
                    {
                        omci_apply_traffic_descriptor_to_gem_port(
                            pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_DS, &mibTD);
                    }
                }
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX) &&
                pOldGemPortCtp->UsTraffMgmtPtr != pGemPortCtp->UsTraffMgmtPtr)
            {
                flowCfg.queueId = pOldGemPortCtp->UsTraffMgmtPtr;
                flowCfg.ena = FALSE;
                if((ret = omci_wrapper_cfgGemFlow(flowCfg)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Delelte Gem Flow Fail: 0x%X", ret);

                flowCfg.queueId = pGemPortCtp->UsTraffMgmtPtr;
                flowCfg.ena = TRUE;
                if((ret = omci_wrapper_cfgGemFlow(flowCfg)) != GOS_OK)
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Add Gem Flow Fail: 0x%X", ret);
            }

        default:
            return GOS_OK;
    }

    ret = omci_wrapper_cfgGemFlow(flowCfg);

    // invoke traffic descriptor settings
    if (MIB_ADD == operationType && GOS_OK == ret)
    {
        if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->UsTraffDescPtr, &mibTD))
        {
            omci_apply_traffic_descriptor_to_gem_port(
                pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_US, &mibTD);
        }

        if (GOS_OK == omci_is_traffic_descriptor_existed(pGemPortCtp->DsTraffDescPtr, &mibTD))
        {
            omci_apply_traffic_descriptor_to_gem_port(
                pGemPortCtp->EntityID, PON_GEMPORT_DIRECTION_DS, &mibTD);
        }

    }

    return ret;
}

static GOS_ERROR_CODE gpnc_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                            omci_alm_data_t     alarmData,
                                            omci_me_instance_t  *pInstanceID,
                                            BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    // TBD, should be the gem port id
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
    gMibGemPortCtpTableInfo.Name = "GemPortCtp";
    gMibGemPortCtpTableInfo.ShortName = "GEMPNCTP";
    gMibGemPortCtpTableInfo.Desc = "GEM port network CTP";
    gMibGemPortCtpTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_GEM_PORT_NETWORK_CTP);
    gMibGemPortCtpTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibGemPortCtpTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibGemPortCtpTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibGemPortCtpTableInfo.pAttributes = &(gMibGemPortCtpAttrInfo[0]);
    gMibGemPortCtpTableInfo.attrNum = MIB_TABLE_GEMPORTCTP_ATTR_NUM;
    gMibGemPortCtpTableInfo.entrySize = sizeof(MIB_TABLE_GEMPORTCTP_T);
    gMibGemPortCtpTableInfo.pDefaultRow = &gMibGemPortCtpDefRow;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortID";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TcAdapterPtr";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Direction";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsTraffMgmtPtr";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UsTraffDescPtr";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UniCounter";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DsPriQPtr";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EncryptionState";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DsTraffDescPtr";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EncryptionKeyRing";

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "GemPort Id";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tcont pointer";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "direction";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Traffic management pointer for upstream";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Traffic descriptor profile point for upstream";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "UNI counter";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream priority point";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Encrtption state";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Downstream Traffic Descriptor profile point";
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Encryption key ring";

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC ;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_PORTID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_TCADAPTERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DIRECTION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFMGMTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_USTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_UNICOUNTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSPRIQPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_DSTRAFFDESCPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibGemPortCtpAttrInfo[MIB_TABLE_GEMPORTCTP_ENCRYPTIONKEYRING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibGemPortCtpDefRow.EntityID), 0x00, sizeof(gMibGemPortCtpDefRow.EntityID));
    memset(&(gMibGemPortCtpDefRow.PortID), 0x00, sizeof(gMibGemPortCtpDefRow.PortID));
    memset(&(gMibGemPortCtpDefRow.TcAdapterPtr), 0x00, sizeof(gMibGemPortCtpDefRow.TcAdapterPtr));
    memset(&(gMibGemPortCtpDefRow.Direction), 0x00, sizeof(gMibGemPortCtpDefRow.Direction));
    memset(&(gMibGemPortCtpDefRow.UsTraffMgmtPtr), 0x00, sizeof(gMibGemPortCtpDefRow.UsTraffMgmtPtr));
    memset(&(gMibGemPortCtpDefRow.UsTraffDescPtr), 0x00, sizeof(gMibGemPortCtpDefRow.UsTraffDescPtr));
    memset(&(gMibGemPortCtpDefRow.UniCounter), 0x00, sizeof(gMibGemPortCtpDefRow.UniCounter));
    gMibGemPortCtpDefRow.DsPriQPtr = 0xFFFF;
    memset(&(gMibGemPortCtpDefRow.EncryptionState), 0x00, sizeof(gMibGemPortCtpDefRow.EncryptionState));
    memset(&(gMibGemPortCtpDefRow.DsTraffDescPtr), 0x00, sizeof(gMibGemPortCtpDefRow.DsTraffDescPtr));
    memset(&(gMibGemPortCtpDefRow.EncryptionKeyRing), 0x00, sizeof(gMibGemPortCtpDefRow.EncryptionKeyRing));

    memset(&gMibGemPortCtpOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibGemPortCtpOper.meOperDrvCfg = GemPortCtpDrvCfg;
    gMibGemPortCtpOper.meOperConnCheck = GemPortCtpConnCheck;
    gMibGemPortCtpOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibGemPortCtpOper.meOperConnCfg = GemPortCtpConnCfg;
    gMibGemPortCtpOper.meOperAvlTreeAdd = GemPortCtpAvlTreeAdd;
    gMibGemPortCtpOper.meOperAvlTreeDel = GemPortCtpAvlTreeDel;
    gMibGemPortCtpOper.meOperAlarmHandler = gpnc_alarm_handler;

    MIB_TABLE_GEMPORTCTP_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibGemPortCtpTableInfo, &gMibGemPortCtpOper);

    return GOS_OK;
}
