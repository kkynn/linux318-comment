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
 * Purpose : Definition of ME handler: General purpose buffer (308)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: General purpose buffer (308)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T                    gMibGpbTblInfo;
MIB_ATTR_INFO_T                     gMibGpbAttrInfo[MIB_TABLE_GENERAL_PURPOSE_BUFFER_ATTR_NUM];
MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  gMibGpbDefRow;
MIB_TABLE_OPER_T                    gMibGpbOper;

extern omci_mulget_info_ts          gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];


GOS_ERROR_CODE general_purpose_buffer_dump_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  *pMibGPB;
    gpb_attr_buffer_tbl_entry_t         *pEntry;
    gpb_attr_buffer_tbl_t               *pEntryData;
    UINT32                              remainSize;
    UINT32                              i;

    pMibGPB = (MIB_TABLE_GENERAL_PURPOSE_BUFFER_T *)pData;

	OMCI_PRINT("EntityId: 0x%04x", pMibGPB->EntityId);
    OMCI_PRINT("MaxSize: %d", pMibGPB->MaxSize);
    OMCI_PRINT("BufferTbl:");
    remainSize = pMibGPB->BufferTbl_size;
    LIST_FOREACH(pEntry, &pMibGPB->BufferTbl_head, entries)
    {
        pEntryData = &pEntry->tableEntry;

        printf("0x");
        for (i = 0; i < MIB_TABLE_GPB_BUFFER_TBL_LEN; i++)
        {
            if (remainSize-- > 0)
                printf("%02hhx", pEntryData->BufferTbl[i]);
            else
                break;
        }
        printf("\n");
    }

	return GOS_OK;
}

GOS_ERROR_CODE general_purpose_buffer_attr_buffer_tbl_append(
        omci_me_instance_t entityId, UINT8 *pData, UINT32 dataSize)
{
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  mibGpb;
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  *pMibGpb;
    gpb_attr_buffer_tbl_entry_t         *pEntry;
    gpb_attr_buffer_tbl_entry_t         *pTempEntry;
    UINT32                              remainSize;
    UINT32                              lastUsedSize;
    UINT32                              lastFreeSize;

    if (0 == dataSize)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "data size cannot be zero");

        return GOS_ERR_INVALID_INPUT;
    }

    mibGpb.EntityId = entityId;

    if (!mib_FindEntry(MIB_TABLE_GENERAL_PURPOSE_BUFFER_INDEX, &mibGpb, &pMibGpb))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "cannot find general purpose buffer mib entry");

        return GOS_ERR_NOT_FOUND;
    }

    lastUsedSize = pMibGpb->BufferTbl_size % MIB_TABLE_GPB_BUFFER_TBL_LEN;
    lastFreeSize = lastUsedSize > 0 ?
        (MIB_TABLE_GPB_BUFFER_TBL_LEN - lastUsedSize) : 0;

    // find the latest entry
    if (LIST_EMPTY(&pMibGpb->BufferTbl_head))
        pEntry = NULL;
    else
    {
        LIST_FOREACH(pEntry, &pMibGpb->BufferTbl_head, entries)
        {
            if (!LIST_NEXT(pEntry, entries))
                break;
        }
    }

    // last free space >= data size
    if (lastFreeSize >= dataSize && pEntry)
    {
        memcpy(pEntry->tableEntry.BufferTbl + lastUsedSize, pData, dataSize);

        pMibGpb->BufferTbl_size += dataSize;
    }
    // last free space < data size
    else
    {
        // fill last free space first
        if (lastFreeSize > 0 && pEntry)
            memcpy(pEntry->tableEntry.BufferTbl + lastUsedSize, pData, lastFreeSize);

        pTempEntry = pEntry;
        pData += lastFreeSize;
        pMibGpb->BufferTbl_size += lastFreeSize;
        remainSize = dataSize - lastFreeSize;

        // append as much full length entry as needed
        while (remainSize >= MIB_TABLE_GPB_BUFFER_TBL_LEN)
        {
            pEntry = malloc(sizeof(gpb_attr_buffer_tbl_entry_t));

            memcpy(pEntry->tableEntry.BufferTbl, pData, MIB_TABLE_GPB_BUFFER_TBL_LEN);

            if (LIST_EMPTY(&pMibGpb->BufferTbl_head))
                LIST_INSERT_HEAD(&pMibGpb->BufferTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pTempEntry = pEntry;
            pData += MIB_TABLE_GPB_BUFFER_TBL_LEN;
            pMibGpb->BufferTbl_size += MIB_TABLE_GPB_BUFFER_TBL_LEN;
            remainSize -= MIB_TABLE_GPB_BUFFER_TBL_LEN;
        }

        // append last entry for the remaining data
        if (remainSize > 0)
        {
            pEntry = malloc(sizeof(gpb_attr_buffer_tbl_entry_t));

            memcpy(pEntry->tableEntry.BufferTbl, pData, remainSize);

            if (LIST_EMPTY(&pMibGpb->BufferTbl_head))
                LIST_INSERT_HEAD(&pMibGpb->BufferTbl_head, pEntry, entries);
            else
                LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

            pMibGpb->BufferTbl_size += remainSize;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE general_purpose_buffer_attr_buffer_tbl_append_over(
        omci_me_instance_t entityId)
{
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  mibGpb;
    MIB_TABLE_INDEX                     tableIndex;
    MIB_ATTRS_SET                       avcAttrSet;

    tableIndex = MIB_TABLE_GENERAL_PURPOSE_BUFFER_INDEX;
    mibGpb.EntityId = entityId;

    if (GOS_OK == MIB_Get(tableIndex, &mibGpb, sizeof(mibGpb)))
    {
        MIB_ClearAttrSet(&avcAttrSet);
        MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX);

        // in order to invoke AVC callback
        MIB_SetAttributes(tableIndex, &mibGpb, sizeof(mibGpb), &avcAttrSet);
    }

    return GOS_OK;
}

GOS_ERROR_CODE general_purpose_buffer_attr_buffer_tbl_clear(
        MIB_TABLE_GENERAL_PURPOSE_BUFFER_T *pGpb)
{
    gpb_attr_buffer_tbl_entry_t *pEntry;
    gpb_attr_buffer_tbl_entry_t *pTempEntry;

    if (!pGpb)
        return GOS_ERR_INVALID_INPUT;

    // clean up the buffer table
    pEntry = LIST_FIRST(&pGpb->BufferTbl_head);
    while (pEntry)
    {
        pTempEntry = LIST_NEXT(pEntry, entries);
        free(pEntry);
        pEntry = pTempEntry;
    }
    LIST_INIT(&pGpb->BufferTbl_head);

    return GOS_OK;
}

GOS_ERROR_CODE general_purpose_buffer_drv_cfg_handler(void          *pOldRow,
                                                    void            *pNewRow,
                                                    MIB_OPERA_TYPE  operationType,
                                                    MIB_ATTRS_SET   attrSet,
                                                    UINT32          pri)
{
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  *pGpb;
    MIB_TABLE_GENERAL_PURPOSE_BUFFER_T  *pMibGpb;
    GOS_ERROR_CODE                      ret = GOS_OK;

    pGpb = (MIB_TABLE_GENERAL_PURPOSE_BUFFER_T *)pNewRow;

    switch (operationType)
    {
        case MIB_GET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX))
            {
                MIB_ATTR_INDEX                  attrIndex;
                omci_mulget_attr_ts             *pMgAttr;
                UINT8                           *pMgAttrData;
                gpb_attr_buffer_tbl_entry_t     *pEntry;
                UINT32                          remainSize;

                attrIndex = MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX;
                pMgAttr = &gOmciMulGetData[pri].attribute[attrIndex];
                pMgAttrData = pMgAttr->attrValue;
                remainSize = pGpb->BufferTbl_size;

                LIST_FOREACH(pEntry, &pGpb->BufferTbl_head, entries)
                {
                    if (remainSize >= MIB_TABLE_GPB_BUFFER_TBL_LEN)
                    {
                        memcpy(pMgAttrData, &pEntry->tableEntry, MIB_TABLE_GPB_BUFFER_TBL_LEN);
                        remainSize -= MIB_TABLE_GPB_BUFFER_TBL_LEN;
                    }
                    else
                        memcpy(pMgAttrData, &pEntry->tableEntry, remainSize);

                    pMgAttrData += MIB_TABLE_GPB_BUFFER_TBL_LEN;
                }

                pMgAttr->attrSize = pGpb->BufferTbl_size;
                pMgAttr->attrIndex = attrIndex;
                pMgAttr->doneSeqNum = 0;
                pMgAttr->maxSeqNum =
                    (pMgAttr->attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
                    OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
            }
            break;

        case MIB_DEL:
            if (!mib_FindEntry(MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX, pGpb, &pMibGpb))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "cannot find general purpose buffer mib entry");

                return GOS_ERR_NOT_FOUND;
            }

            // clean up tables
            general_purpose_buffer_attr_buffer_tbl_clear(pMibGpb);

        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE general_purpose_buffer_avc_cb(MIB_TABLE_INDEX  tableIndex,
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
        avcAttrSet = *pAttrsSet;

        for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
                i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
        {
            if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
                continue;

            // for table attributes, only report AVC as TRUE to indicates change
            if (MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX == attrIndex)
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

    gMibGpbTblInfo.Name = "GeneralPurposeBuffer";
    gMibGpbTblInfo.ShortName = "GPB";
    gMibGpbTblInfo.Desc = "General purpose buffer";
    gMibGpbTblInfo.ClassId = OMCI_ME_CLASS_GENERAL_PURPOSE_BUFFER;
    gMibGpbTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibGpbTblInfo.StdType = OMCI_ME_TYPE_STANDARD | OMCI_ME_TYPE_NOT_MIB_UPLOAD;
    gMibGpbTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT;
    gMibGpbTblInfo.pAttributes = &(gMibGpbAttrInfo[0]);
	gMibGpbTblInfo.attrNum = MIB_TABLE_GENERAL_PURPOSE_BUFFER_ATTR_NUM;
	gMibGpbTblInfo.entrySize = sizeof(MIB_TABLE_GENERAL_PURPOSE_BUFFER_T);
	gMibGpbTblInfo.pDefaultRow = &gMibGpbDefRow;

    attrIndex = MIB_TABLE_GENERAL_PURPOSE_BUFFER_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGpbAttrInfo[attrIndex].Name = "EntityId";
    gMibGpbAttrInfo[attrIndex].Desc = "Entity ID";
    gMibGpbAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGpbAttrInfo[attrIndex].Len = 2;
    gMibGpbAttrInfo[attrIndex].IsIndex = TRUE;
    gMibGpbAttrInfo[attrIndex].MibSave = TRUE;
    gMibGpbAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpbAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGpbAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibGpbAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_GENERAL_PURPOSE_BUFFER_MAX_SIZE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGpbAttrInfo[attrIndex].Name = "MaxSize";
    gMibGpbAttrInfo[attrIndex].Desc = "Maximum size";
    gMibGpbAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT32;
    gMibGpbAttrInfo[attrIndex].Len = 4;
    gMibGpbAttrInfo[attrIndex].IsIndex = FALSE;
    gMibGpbAttrInfo[attrIndex].MibSave = TRUE;
    gMibGpbAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibGpbAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGpbAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibGpbAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_GENERAL_PURPOSE_BUFFER_BUFFER_TBL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibGpbAttrInfo[attrIndex].Name = "BufferTbl";
	gMibGpbAttrInfo[attrIndex].Desc = "Buffer table";
    gMibGpbAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibGpbAttrInfo[attrIndex].Len = 25;
    gMibGpbAttrInfo[attrIndex].IsIndex = FALSE;
    gMibGpbAttrInfo[attrIndex].MibSave = TRUE;
    gMibGpbAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGpbAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibGpbAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibGpbAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	memset(&gMibGpbDefRow, 0x00, sizeof(gMibGpbDefRow));
    LIST_INIT(&gMibGpbDefRow.BufferTbl_head);

    memset(&gMibGpbOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibGpbOper.meOperDrvCfg = general_purpose_buffer_drv_cfg_handler;
    gMibGpbOper.meOperConnCfg = NULL;
	gMibGpbOper.meOperConnCheck = NULL;
	gMibGpbOper.meOperDump = general_purpose_buffer_dump_handler;

	MIB_TABLE_GENERAL_PURPOSE_BUFFER_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibGpbTblInfo, &gMibGpbOper);
    MIB_RegisterCallback(tableId, NULL, general_purpose_buffer_avc_cb);

    return GOS_OK;
}
