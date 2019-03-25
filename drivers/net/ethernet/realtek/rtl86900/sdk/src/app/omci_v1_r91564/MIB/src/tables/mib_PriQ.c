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
 * Purpose : Definition of ME handler: Priority queue (277)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Priority queue (277)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibPriQTableInfo;
MIB_ATTR_INFO_T  gMibPriQAttrInfo[MIB_TABLE_PRIQ_ATTR_NUM];
MIB_TABLE_PRIQ_T gMibPriQDefRow;
MIB_TABLE_OPER_T gMibPriQOper;


static void omci_update_conn_by_pq(MIB_TABLE_PRIQ_T *pNewQ)
{
    MIB_TABLE_GEMPORTCTP_T  mib_gem_ctp;
    GOS_ERROR_CODE          ret;
    MIB_TREE_T              *pTree = NULL;
    MIB_NODE_T              *pAniNode = NULL, *pUniNode = NULL;
    MIB_TREE_DATA_T         *pAniData = NULL, *pUniData = NULL;

    ret = MIB_GetFirst(MIB_TABLE_GEMPORTCTP_INDEX, &mib_gem_ctp, sizeof(MIB_TABLE_GEMPORTCTP_T));

    while (GOS_OK == ret)
    {
        if (mib_gem_ctp.DsPriQPtr != pNewQ->EntityID)
            goto next_gem_ctp;

        if (!(pTree = MIB_AvlTreeSearchByKey(pTree, mib_gem_ctp.EntityID, AVL_KEY_GEMPORTCTP)))
            goto next_gem_ctp;

        if (!(pAniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_ANI)))
            goto next_gem_ctp;

        if (!(pAniData = &pAniNode->data))
            goto next_gem_ctp;

        if (LIST_EMPTY(&pAniData->treeNodeEntryHead))
            goto next_gem_ctp;

        if (!(pUniNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_UNI)))
            goto next_gem_ctp;

        if (!(pUniData = &pUniNode->data))
            goto next_gem_ctp;

        if (LIST_EMPTY(&pUniData->treeNodeEntryHead))
            goto next_gem_ctp;

        MIB_TreeConnUpdate(pTree);

next_gem_ctp:
        ret = MIB_GetNext(MIB_TABLE_GEMPORTCTP_INDEX, &mib_gem_ctp, sizeof(MIB_TABLE_GEMPORTCTP_T));
    }
}

GOS_ERROR_CODE PriQDrvCfg(void              *pOldRow,
                            void            *pNewRow,
                            MIB_OPERA_TYPE  operationType,
                            MIB_ATTRS_SET   attrSet,
                            UINT32          pri)
{
    GOS_ERROR_CODE      ret;
    MIB_TABLE_PRIQ_T    *pNewQ;
    MIB_TABLE_PRIQ_T    *pOldQ;
    UINT16              oldData;
    UINT16              newData;

    pNewQ = (MIB_TABLE_PRIQ_T *)pNewRow;
    pOldQ = (MIB_TABLE_PRIQ_T *)pOldRow;

    switch (operationType)
    {
        case MIB_SET:
        {
            oldData = pOldQ->RelatedPort >> 16;
            newData = pNewQ->RelatedPort >> 16;
            if (oldData != newData)
            {
                MIB_TABLE_TCONT_T tTC;

                // not permitted for downstream queues
                if (pNewQ->EntityID < OMCI_MIB_US_TM_ME_ID_BASE)
                    return GOS_FAIL;

                tTC.EntityID = newData;

                ret = MIB_Get(MIB_TABLE_TCONT_INDEX, &tTC, sizeof(MIB_TABLE_TCONT_T));
                if (GOS_OK != ret)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Invalid T-Cont for PQ");

                    return ret;
                }

                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "Pri-Q -> SET: old port [0x%x] new port [0x%x]", oldData, newData);
            }

            oldData = pOldQ->RelatedPort & 0xFFFF;
            newData = pNewQ->RelatedPort & 0xFFFF;
            if (oldData != newData)
            {
                // not permitted for downstream queues
                if (pNewQ->EntityID < OMCI_MIB_US_TM_ME_ID_BASE)
                    return GOS_FAIL;

                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "Pri-Q -> SET: old priority [0x%x] new priority [0x%x]", oldData, newData);
            }

            oldData = pOldQ->SchedulerPtr;
            newData = pNewQ->SchedulerPtr;
            if (oldData != newData)
            {
                MIB_TABLE_SCHEDULER_T tTS;

                // not permitted for downstream queues
                if (pNewQ->EntityID < OMCI_MIB_US_TM_ME_ID_BASE)
                    return GOS_FAIL;

                tTS.EntityID = newData;
                if(tTS.EntityID)
                {
                    ret = MIB_Get(MIB_TABLE_SCHEDULER_INDEX, &tTS, sizeof(MIB_TABLE_SCHEDULER_T));
                    if (GOS_OK != ret)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Invalid traffic scheduler for PQ");

                        return ret;
                    }
                }
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                    "Pri-Q -> SET: old scheduler [0x%x] new scheduler [0x%x]", oldData, newData);
            }

            ret = omci_wrapper_setPriQueue(pNewQ);

            oldData = pOldQ->DropPColorMarking;
            newData = pNewQ->DropPColorMarking;
            if (oldData != newData)
                omci_update_conn_by_pq(pNewQ);

            break;
        }
        case MIB_ADD:
            ret = mib_alarm_table_add(MIB_TABLE_PRIQ_INDEX, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(MIB_TABLE_PRIQ_INDEX, pOldRow);
            break;

        default:
            return GOS_OK;
    }

    return ret;
}

static GOS_ERROR_CODE pq_alarm_handler(MIB_TABLE_INDEX      tableIndex,
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
    // TBD, should be the queue id
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
    gMibPriQTableInfo.Name = "PriQ";
    gMibPriQTableInfo.ShortName = "PQ";
    gMibPriQTableInfo.Desc = "Priority Queue-G";
    gMibPriQTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PRIORITY_QUEUE);
    gMibPriQTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibPriQTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibPriQTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibPriQTableInfo.pAttributes = &(gMibPriQAttrInfo[0]);
	gMibPriQTableInfo.attrNum = MIB_TABLE_PRIQ_ATTR_NUM;
	gMibPriQTableInfo.entrySize = sizeof(MIB_TABLE_PRIQ_T);
	gMibPriQTableInfo.pDefaultRow = &gMibPriQDefRow;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QCfgOpt";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxQSize";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AllocQSize";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ResetInterval";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Threshold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RelatedPort";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SchedulerPtr";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Weight";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BPOperation";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BPTime";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BPOccThreshold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BPClrThreshold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PktDropQThold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PktDropMaxP";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "QueueDropWQ";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DropPColorMarking";

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Queue Configuration Option";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Maximum Queue Size";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Allocated Queue Size";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Discard-cell/block-counter Reset Interval";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold Value for Discarded Cells or Blocks due to Buffer Overflow";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Related Port";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Traffic Scheduler-G Pointer";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Weight";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Back Pressure Operation";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Back Pressure Time";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Back Pressure Occur Queue Threshold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Back Pressure Clear Queue Threshold";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet drop queue thresholds";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet drop max_p";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Queue drop w_q";
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Drop precedence colour marking";

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT64;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 8;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QCFGOPT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_MAXQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_ALLOCQSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RESETINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_RELATEDPORT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_WEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOPERATION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPOCCTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_BPCLRTHRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_QUEUE_THRESHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_PKT_DROP_MAX_PROBABILITY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_QUEUE_DROP_AVG_COEFFICIENT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPriQAttrInfo[MIB_TABLE_PRIQ_DROP_PRECEDENCE_COLOUR_MARKING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibPriQDefRow.EntityID), 0x00, sizeof(gMibPriQDefRow.EntityID));
    gMibPriQDefRow.QCfgOpt = PQ_QUEUE_CFG_OPT_SHARED_BUFFER;
    memset(&(gMibPriQDefRow.MaxQSize), 0x00, sizeof(gMibPriQDefRow.MaxQSize));
    memset(&(gMibPriQDefRow.AllocQSize), 0x00, sizeof(gMibPriQDefRow.AllocQSize));
    memset(&(gMibPriQDefRow.ResetInterval), 0x00, sizeof(gMibPriQDefRow.ResetInterval));
    memset(&(gMibPriQDefRow.Threshold), 0x00, sizeof(gMibPriQDefRow.Threshold));
    memset(&(gMibPriQDefRow.RelatedPort), 0x00, sizeof(gMibPriQDefRow.RelatedPort));
    memset(&(gMibPriQDefRow.SchedulerPtr), 0x00, sizeof(gMibPriQDefRow.SchedulerPtr));
    gMibPriQDefRow.Weight = 1;
    memset(&(gMibPriQDefRow.BPOperation), 0x00, sizeof(gMibPriQDefRow.BPOperation));
    memset(&(gMibPriQDefRow.BPTime), 0x00, sizeof(gMibPriQDefRow.BPTime));
    memset(&(gMibPriQDefRow.BPOccThreshold), 0x00, sizeof(gMibPriQDefRow.BPOccThreshold));
    memset(&(gMibPriQDefRow.BPClrThreshold), 0x00, sizeof(gMibPriQDefRow.BPClrThreshold));
    memset(&(gMibPriQDefRow.PktDropQThold), 0x00, sizeof(gMibPriQDefRow.PktDropQThold));
    gMibPriQDefRow.PktDropMaxP = 255;
    gMibPriQDefRow.QueueDropWQ = 9;
    gMibPriQDefRow.DropPColorMarking = PQ_DROP_COLOUR_NO_MARKING;

    memset(&gMibPriQOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibPriQOper.meOperDrvCfg = PriQDrvCfg;
	gMibPriQOper.meOperConnCheck = NULL;
	gMibPriQOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibPriQOper.meOperConnCfg = NULL;
    gMibPriQOper.meOperAlarmHandler = pq_alarm_handler;

	MIB_TABLE_PRIQ_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibPriQTableInfo,&gMibPriQOper);

    return GOS_OK;
}
