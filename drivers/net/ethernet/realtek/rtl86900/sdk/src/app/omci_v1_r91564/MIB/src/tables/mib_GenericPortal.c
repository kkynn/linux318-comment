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
 * Purpose : Definition of ME handler: Generic status portal (330)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Generic status portal (330)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T                    gMibGspTblInfo;
MIB_ATTR_INFO_T                     gMibGspAttrInfo[MIB_TABLE_GENERIC_STATUS_PORTAL_ATTR_NUM];
MIB_TABLE_GENERIC_STATUS_PORTAL_T   gMibGspDefRow;
MIB_TABLE_OPER_T                    gMibGspOper;

extern omci_mulget_info_ts          gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];


GOS_ERROR_CODE generic_status_portal_dump_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   *pMibGSP;
    gsp_attr_status_doc_tbl_entry_t     *pStatusEntry;
    gsp_attr_status_doc_tbl_t           *pStatusData;
    gsp_attr_cfg_doc_tbl_entry_t        *pCfgEntry;
    gsp_attr_cfg_doc_tbl_t              *pCfgData;

    pMibGSP = (MIB_TABLE_GENERIC_STATUS_PORTAL_T *)pData;

	OMCI_PRINT("EntityId: 0x%04x", pMibGSP->EntityId);
    OMCI_PRINT("StatusDocTbl:");
    LIST_FOREACH(pStatusEntry, &pMibGSP->StatusDocTbl_head, entries)
    {
        pStatusData = &pStatusEntry->tableEntry;

        printf("%s", pStatusData->StatusDocTbl);
    }
    OMCI_PRINT("CfgDocTbl:");
    LIST_FOREACH(pCfgEntry, &pMibGSP->CfgDocTbl_head, entries)
    {
        pCfgData = &pCfgEntry->tableEntry;

        printf("%s", pCfgData->CfgDocTbl);
    }
	OMCI_PRINT("AvcReportRate: %d", pMibGSP->AvcReportRate);

	return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_status_doc_tbl_append(
        omci_me_instance_t entityId, UINT8 *pData, UINT32 dataSize)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   mibGsp;
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   *pMibGsp;
    gsp_attr_status_doc_tbl_entry_t     *pEntry;
    gsp_attr_status_doc_tbl_entry_t     *pTempEntry;
    UINT32                              remainSize;
    UINT32                              lastUsedSize;
    UINT32                              lastFreeSize;

    if (0 == dataSize)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "data size cannot be zero");

        return GOS_ERR_INVALID_INPUT;
    }

    mibGsp.EntityId = entityId;

    if (!mib_FindEntry(MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX, &mibGsp, &pMibGsp))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "cannot find generic status portal mib entry");

        return GOS_ERR_NOT_FOUND;
    }

    lastUsedSize = pMibGsp->StatusDocTbl_size % MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
    lastFreeSize = lastUsedSize > 0 ?
        (MIB_TABLE_GSP_STATUS_DOC_TBL_LEN - lastUsedSize) : 0;

    // find the latest entry
    if (LIST_EMPTY(&pMibGsp->StatusDocTbl_head))
        pEntry = NULL;
    else
    {
        LIST_FOREACH(pEntry, &pMibGsp->StatusDocTbl_head, entries)
        {
            if (!LIST_NEXT(pEntry, entries))
                break;
        }
    }

    // last free space >= data size
    if (lastFreeSize >= dataSize && pEntry)
    {
        memcpy(pEntry->tableEntry.StatusDocTbl + lastUsedSize, pData, dataSize);

        pMibGsp->StatusDocTbl_size += dataSize;
    }
    // last free space < data size
    else
    {
        // fill last free space first
        if (lastFreeSize > 0 && pEntry)
            memcpy(pEntry->tableEntry.StatusDocTbl + lastUsedSize, pData, lastFreeSize);

        pTempEntry = pEntry;
        pData += lastFreeSize;
        pMibGsp->StatusDocTbl_size += lastFreeSize;
        remainSize = dataSize - lastFreeSize;

        // append as much full length entry as needed
        while (remainSize >= MIB_TABLE_GSP_STATUS_DOC_TBL_LEN)
        {
            pEntry = malloc(sizeof(gsp_attr_status_doc_tbl_entry_t));

            memcpy(pEntry->tableEntry.StatusDocTbl, pData, MIB_TABLE_GSP_STATUS_DOC_TBL_LEN);

            if (LIST_EMPTY(&pMibGsp->StatusDocTbl_head))
                LIST_INSERT_HEAD(&pMibGsp->StatusDocTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pTempEntry = pEntry;
            pData += MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
            pMibGsp->StatusDocTbl_size += MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
            remainSize -= MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
        }

        // append last entry for the remaining data
        if (remainSize > 0)
        {
            pEntry = malloc(sizeof(gsp_attr_status_doc_tbl_entry_t));

            memcpy(pEntry->tableEntry.StatusDocTbl, pData, remainSize);

            if (LIST_EMPTY(&pMibGsp->StatusDocTbl_head))
                LIST_INSERT_HEAD(&pMibGsp->StatusDocTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pMibGsp->StatusDocTbl_size += remainSize;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_cfg_doc_tbl_append(
        omci_me_instance_t entityId, UINT8 *pData, UINT32 dataSize)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   mibGsp;
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   *pMibGsp;
    gsp_attr_cfg_doc_tbl_entry_t        *pEntry;
    gsp_attr_cfg_doc_tbl_entry_t        *pTempEntry;
    UINT32                              remainSize;
    UINT32                              lastUsedSize;
    UINT32                              lastFreeSize;

    if (0 == dataSize)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "data size cannot be zero");

        return GOS_ERR_INVALID_INPUT;
    }

    mibGsp.EntityId = entityId;

    if (!mib_FindEntry(MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX, &mibGsp, &pMibGsp))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "cannot find generic status portal mib entry");

        return GOS_ERR_NOT_FOUND;
    }

    lastUsedSize = pMibGsp->CfgDocTbl_size % MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
    lastFreeSize = lastUsedSize > 0 ?
        (MIB_TABLE_GSP_CFG_DOC_TBL_LEN - lastUsedSize) : 0;

    // find the latest entry
    if (LIST_EMPTY(&pMibGsp->CfgDocTbl_head))
        pEntry = NULL;
    else
    {
        LIST_FOREACH(pEntry, &pMibGsp->CfgDocTbl_head, entries)
        {
            if (!LIST_NEXT(pEntry, entries))
                break;
        }
    }

    // last free space >= data size
    if (lastFreeSize >= dataSize && pEntry)
    {
        memcpy(pEntry->tableEntry.CfgDocTbl + lastUsedSize, pData, dataSize);

        pMibGsp->CfgDocTbl_size += dataSize;
    }
    // last free space < data size
    else
    {
        // fill last free space first
        if (lastFreeSize > 0 && pEntry)
            memcpy(pEntry->tableEntry.CfgDocTbl + lastUsedSize, pData, lastFreeSize);

        pTempEntry = pEntry;
        pData += lastFreeSize;
        pMibGsp->CfgDocTbl_size += lastFreeSize;
        remainSize = dataSize - lastFreeSize;

        // append as much full length entry as needed
        while (remainSize >= MIB_TABLE_GSP_CFG_DOC_TBL_LEN)
        {
            pEntry = malloc(sizeof(gsp_attr_cfg_doc_tbl_entry_t));

            memcpy(pEntry->tableEntry.CfgDocTbl, pData, MIB_TABLE_GSP_CFG_DOC_TBL_LEN);

            if (LIST_EMPTY(&pMibGsp->CfgDocTbl_head))
                LIST_INSERT_HEAD(&pMibGsp->CfgDocTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pTempEntry = pEntry;
            pData += MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
            pMibGsp->CfgDocTbl_size += MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
            remainSize -= MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
        }

        // append last entry for the remaining data
        if (remainSize > 0)
        {
            pEntry = malloc(sizeof(gsp_attr_cfg_doc_tbl_entry_t));

            memcpy(pEntry->tableEntry.CfgDocTbl, pData, remainSize);

            if (LIST_EMPTY(&pMibGsp->CfgDocTbl_head))
                LIST_INSERT_HEAD(&pMibGsp->CfgDocTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pMibGsp->CfgDocTbl_size += remainSize;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_status_doc_tbl_append_over(
        omci_me_instance_t entityId)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   mibGsp;
    MIB_TABLE_INDEX                     tableIndex;
    MIB_ATTRS_SET                       avcAttrSet;

    tableIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX;
    mibGsp.EntityId = entityId;

    if (GOS_OK == MIB_Get(tableIndex, &mibGsp, sizeof(mibGsp)))
    {
        MIB_ClearAttrSet(&avcAttrSet);
        MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_GENERIC_STATUS_PORTAL_STATUS_DOC_TBL_INDEX);

        // in order to invoke AVC callback
        MIB_SetAttributes(tableIndex, &mibGsp, sizeof(mibGsp), &avcAttrSet);
    }

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_cfg_doc_tbl_append_over(
        omci_me_instance_t entityId)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   mibGsp;
    MIB_TABLE_INDEX                     tableIndex;
    MIB_ATTRS_SET                       avcAttrSet;

    tableIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX;
    mibGsp.EntityId = entityId;

    if (GOS_OK == MIB_Get(tableIndex, &mibGsp, sizeof(mibGsp)))
    {
        MIB_ClearAttrSet(&avcAttrSet);
        MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_GENERIC_STATUS_PORTAL_CFG_DOC_TBL_INDEX);

        // in order to invoke AVC callback
        MIB_SetAttributes(tableIndex, &mibGsp, sizeof(mibGsp), &avcAttrSet);
    }

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_status_doc_tbl_clear(
        MIB_TABLE_GENERIC_STATUS_PORTAL_T *pGsp)
{
    gsp_attr_status_doc_tbl_entry_t *pEntry;
    gsp_attr_status_doc_tbl_entry_t *pTempEntry;

    if (!pGsp)
        return GOS_ERR_INVALID_INPUT;

    // clean up the status doc table
    pEntry = LIST_FIRST(&pGsp->StatusDocTbl_head);
    while (pEntry)
    {
        pTempEntry = LIST_NEXT(pEntry, entries);
        free(pEntry);
        pEntry = pTempEntry;
    }
    LIST_INIT(&pGsp->StatusDocTbl_head);

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_attr_cfg_doc_tbl_clear(
        MIB_TABLE_GENERIC_STATUS_PORTAL_T *pGsp)
{
    gsp_attr_cfg_doc_tbl_entry_t    *pEntry;
    gsp_attr_cfg_doc_tbl_entry_t    *pTempEntry;

    if (!pGsp)
        return GOS_ERR_INVALID_INPUT;

    // clean up the cfg doc table
    pEntry = LIST_FIRST(&pGsp->CfgDocTbl_head);
    while (pEntry)
    {
        pTempEntry = LIST_NEXT(pEntry, entries);
        free(pEntry);
        pEntry = pTempEntry;
    }
    LIST_INIT(&pGsp->CfgDocTbl_head);

    return GOS_OK;
}

GOS_ERROR_CODE generic_status_portal_drv_cfg_handler(void           *pOldRow,
                                                    void            *pNewRow,
                                                    MIB_OPERA_TYPE  operationType,
                                                    MIB_ATTRS_SET   attrSet,
                                                    UINT32          pri)
{
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   *pGsp;
    MIB_TABLE_GENERIC_STATUS_PORTAL_T   *pMibGsp;
    GOS_ERROR_CODE                      ret = GOS_OK;

    pGsp = (MIB_TABLE_GENERIC_STATUS_PORTAL_T *)pNewRow;

    switch (operationType)
    {
        case MIB_GET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GENERIC_STATUS_PORTAL_STATUS_DOC_TBL_INDEX))
            {
                MIB_ATTR_INDEX                      attrIndex;
                omci_mulget_attr_ts                 *pMgAttr;
                UINT8                               *pMgAttrData;
                gsp_attr_status_doc_tbl_entry_t     *pEntry;
                UINT32                              remainSize;

                attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_STATUS_DOC_TBL_INDEX;
                pMgAttr = &gOmciMulGetData[pri].attribute[attrIndex];
                pMgAttrData = pMgAttr->attrValue;
                remainSize = pGsp->StatusDocTbl_size;

                LIST_FOREACH(pEntry, &pGsp->StatusDocTbl_head, entries)
                {
                    if (remainSize >= MIB_TABLE_GSP_STATUS_DOC_TBL_LEN)
                    {
                        memcpy(pMgAttrData, &pEntry->tableEntry, MIB_TABLE_GSP_STATUS_DOC_TBL_LEN);
                        remainSize -= MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
                    }
                    else
                        memcpy(pMgAttrData, &pEntry->tableEntry, remainSize);

                    pMgAttrData += MIB_TABLE_GSP_STATUS_DOC_TBL_LEN;
                }

                pMgAttr->attrSize = pGsp->StatusDocTbl_size;
                pMgAttr->attrIndex = attrIndex;
                pMgAttr->doneSeqNum = 0;
                pMgAttr->maxSeqNum =
                    (pMgAttr->attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
                    OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GENERIC_STATUS_PORTAL_CFG_DOC_TBL_INDEX))
            {
                MIB_ATTR_INDEX                  attrIndex;
                omci_mulget_attr_ts             *pMgAttr;
                UINT8                           *pMgAttrData;
                gsp_attr_cfg_doc_tbl_entry_t    *pEntry;
                UINT32                          remainSize;

                attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_CFG_DOC_TBL_INDEX;
                pMgAttr = &gOmciMulGetData[pri].attribute[attrIndex];
                pMgAttrData = pMgAttr->attrValue;
                remainSize = pGsp->CfgDocTbl_size;

                LIST_FOREACH(pEntry, &pGsp->CfgDocTbl_head, entries)
                {
                    if (remainSize >= MIB_TABLE_GSP_CFG_DOC_TBL_LEN)
                    {
                        memcpy(pMgAttrData, &pEntry->tableEntry, MIB_TABLE_GSP_CFG_DOC_TBL_LEN);
                        remainSize -= MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
                    }
                    else
                        memcpy(pMgAttrData, &pEntry->tableEntry, remainSize);

                    pMgAttrData += MIB_TABLE_GSP_CFG_DOC_TBL_LEN;
                }

                pMgAttr->attrSize = pGsp->CfgDocTbl_size;
                pMgAttr->attrIndex = attrIndex;
                pMgAttr->doneSeqNum = 0;
                pMgAttr->maxSeqNum =
                    (pMgAttr->attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
                    OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
            }
            break;

        case MIB_DEL:
            if (!mib_FindEntry(MIB_TABLE_ONU_REMOTE_DBG_INDEX, pGsp, &pMibGsp))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "cannot find generic status portal mib entry");

                return GOS_ERR_NOT_FOUND;
            }

            // clean up tables
            generic_status_portal_attr_status_doc_tbl_clear(pMibGsp);
            generic_status_portal_attr_cfg_doc_tbl_clear(pMibGsp);

        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE generic_status_portal_avc_cb(MIB_TABLE_INDEX  tableIndex,
                                                    void            *pOldRow,
                                                    void            *pNewRow,
                                                    MIB_ATTRS_SET   *pAttrsSet,
                                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;

    if (!pAttrsSet)
        return GOS_ERR_PARAM;

    if (MIB_SET == operationType)
    {
        if (!pOldRow || !pNewRow)
            return GOS_ERR_PARAM;
    }
    else
        return GOS_OK;

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
    {
        // TBD
        // AVC report rate has not yet implemented

        avcAttrSet = *pAttrsSet;

        for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
                i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
        {
            if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
                continue;

            // for table attributes, only report AVC as TRUE to indicates change
            if (MIB_TABLE_GENERIC_STATUS_PORTAL_STATUS_DOC_TBL_INDEX == attrIndex ||
                    MIB_TABLE_GENERIC_STATUS_PORTAL_CFG_DOC_TBL_INDEX == attrIndex)
            {
                if (pNewRow && pOldRow)
                {
                    MIB_SetAttrSet(&avcAttrSet, attrIndex);
                }
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;

    gMibGspTblInfo.Name = "GenericStatusPortal";
    gMibGspTblInfo.ShortName = "GSP";
    gMibGspTblInfo.Desc = "Generic status portal";
    gMibGspTblInfo.ClassId = OMCI_ME_CLASS_GENERIC_STATUS_PORTAL;
    gMibGspTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibGspTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibGspTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE |
        OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT;
    gMibGspTblInfo.pAttributes = &(gMibGspAttrInfo[0]);
	gMibGspTblInfo.attrNum = MIB_TABLE_GENERIC_STATUS_PORTAL_ATTR_NUM;
	gMibGspTblInfo.entrySize = sizeof(MIB_TABLE_GENERIC_STATUS_PORTAL_T);
	gMibGspTblInfo.pDefaultRow = &gMibGspDefRow;

    attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGspAttrInfo[attrIndex].Name = "EntityId";
    gMibGspAttrInfo[attrIndex].Desc = "Entity ID";
    gMibGspAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGspAttrInfo[attrIndex].Len = 2;
    gMibGspAttrInfo[attrIndex].IsIndex = TRUE;
    gMibGspAttrInfo[attrIndex].MibSave = TRUE;
    gMibGspAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGspAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGspAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibGspAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_STATUS_DOC_TBL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGspAttrInfo[attrIndex].Name = "StatusDocTbl";
    gMibGspAttrInfo[attrIndex].Desc = "Status document table";
    gMibGspAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibGspAttrInfo[attrIndex].Len = 25;
    gMibGspAttrInfo[attrIndex].IsIndex = FALSE;
    gMibGspAttrInfo[attrIndex].MibSave = TRUE;
    gMibGspAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGspAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibGspAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibGspAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_CFG_DOC_TBL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGspAttrInfo[attrIndex].Name = "CfgDocTbl";
	gMibGspAttrInfo[attrIndex].Desc = "Configuration document table";
    gMibGspAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibGspAttrInfo[attrIndex].Len = 25;
    gMibGspAttrInfo[attrIndex].IsIndex = FALSE;
    gMibGspAttrInfo[attrIndex].MibSave = TRUE;
    gMibGspAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGspAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibGspAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibGspAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_GENERIC_STATUS_PORTAL_AVC_REPORT_RATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGspAttrInfo[attrIndex].Name = "AvcReportRate";
	gMibGspAttrInfo[attrIndex].Desc = "AVC report rate";
    gMibGspAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibGspAttrInfo[attrIndex].Len = 1;
    gMibGspAttrInfo[attrIndex].IsIndex = FALSE;
    gMibGspAttrInfo[attrIndex].MibSave = TRUE;
    gMibGspAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGspAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGspAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibGspAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	memset(&gMibGspDefRow, 0x00, sizeof(gMibGspDefRow));
	gMibGspDefRow.AvcReportRate = GENERIC_STATUS_PORTAL_AVC_REPORT_RATE_DISABLE;
    LIST_INIT(&gMibGspDefRow.StatusDocTbl_head);
    LIST_INIT(&gMibGspDefRow.CfgDocTbl_head);

    memset(&gMibGspOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibGspOper.meOperDrvCfg = generic_status_portal_drv_cfg_handler;
    gMibGspOper.meOperConnCfg = NULL;
	gMibGspOper.meOperConnCheck = NULL;
	gMibGspOper.meOperDump = generic_status_portal_dump_handler;

	MIB_TABLE_GENERIC_STATUS_PORTAL_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibGspTblInfo, &gMibGspOper);
    MIB_RegisterCallback(tableId, NULL, generic_status_portal_avc_cb);

    return GOS_OK;
}
