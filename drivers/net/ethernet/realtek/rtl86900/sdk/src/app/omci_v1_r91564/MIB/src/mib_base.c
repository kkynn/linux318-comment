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
 * Purpose : Definition of OMCI MIB related APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI MIB related APIs
 */

#include "gos_general.h"
#include "mib_table.h"
#include "omci_util.h"

#ifdef OMCI_X86
#define TMP_MALLOC_LEN 256
#endif


/*mib_table top head*/
omciMibTables_t gTables;

MIB_TABLE_T      *gCacheTable = NULL;

int MIB_TABLE_LAST_INDEX;
int MIB_TABLE_TOTAL_NUMBER;

GOS_ERROR_CODE mib_SetGetAttribute(MIB_TABLE_INDEX tableIndex, void* pAttr, void* pRow, MIB_ATTR_INDEX attrIndex, BOOL set);


MIB_TABLE_T* mib_GetTablePtr(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_T *np;

    if (!MIB_TABLE_INDEX_VALID(tableIndex))
    {
        return NULL;
    }

    if(gCacheTable!=NULL && gCacheTable->tableIndex == tableIndex)
    {
        return gCacheTable;
    }

    LIST_FOREACH(np,&gTables.head,entries)
    {

        if(np->tableIndex == tableIndex)
        {
            gCacheTable = np;
            return np;
        }
    }

    return NULL;
}

GOS_ERROR_CODE mib_GetNextTablePtr(MIB_TABLE_INDEX *tableIndex)
{
    MIB_TABLE_T *np;

    if (!MIB_TABLE_INDEX_VALID(*tableIndex))
    {
        return GOS_FAIL;
    }

    if(gCacheTable!=NULL && gCacheTable->tableIndex == *tableIndex)
    {
        if ((gCacheTable = LIST_NEXT(gCacheTable, entries)) != NULL)
        {
            *tableIndex = gCacheTable->tableIndex;
            return GOS_OK;
        }

    }

    LIST_FOREACH(np,&gTables.head,entries)
    {

        if(np->tableIndex == *tableIndex)
        {
            break;
        }
    }

    if ((np = LIST_NEXT(np, entries)) != NULL)
    {
        gCacheTable = np;
        *tableIndex = np->tableIndex;
        return GOS_OK;
    }

    return GOS_FAIL;
}

MIB_ATTR_INFO_T* mib_GetAttrInfo(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    MIB_TABLE_INFO_T* pTableInfo = MIB_GetTableInfoPtr(tableIndex);

    if (pTableInfo == NULL)
    {
        return NULL;
    }

    if ((attrIndex >= MIB_ATTR_FIRST_INDEX + MIB_GetTableAttrNum(tableIndex)) ||
         (attrIndex < MIB_ATTR_FIRST_INDEX))
    {
        return NULL;
    }

    return &(pTableInfo->pAttributes[attrIndex - MIB_ATTR_FIRST_INDEX]);
}


GOS_ERROR_CODE mib_InvokeCallbacks(MIB_TABLE_INDEX tableIndex,
                                    void *pOldRow, void *pNewRow,
                                    MIB_ATTRS_SET *pAttrsSet,
                                    MIB_OPERA_TYPE operationType)
{
    MIB_TABLE_T     *pTable;
    UINT32          i, dataSize;
    GOS_ERROR_CODE  cbRet;
    void            *pData = pNewRow;
    MIB_ATTRS_SET   avcAttrSet;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return GOS_ERR_PARAM;

    // Data validation and modification callbacks
    for (i = 0; i < MIB_TABLE_CB_MAX_NUM; i++)
    {
        if (pTable->preCheckCB[i])
        {
            cbRet = (*pTable->preCheckCB[i])(tableIndex, pOldRow, pNewRow, pAttrsSet, operationType);
            if (GOS_OK != cbRet)
            {
                return cbRet;
            }
        }
    }

    // allocate memory for instance data
    if (pNewRow)
    {
        dataSize = MIB_GetTableEntrySize(tableIndex);
        pData = malloc(dataSize);
        if (!pData)
            return GOS_FAIL;
        memcpy(pData, pNewRow, dataSize);
    }

    // use local variable to prevent further modify
    avcAttrSet = *pAttrsSet;

    // AVC report handling callbacks
    for (i = 0; i < MIB_TABLE_CB_MAX_NUM; i++)
    {
        if (pTable->operationCB[i])
        {
            cbRet = (*pTable->operationCB[i])(tableIndex, pOldRow, pData, &avcAttrSet, operationType);
            if (GOS_OK != cbRet)
            {
                if (pNewRow)
                    free(pData);

                return cbRet;
            }
        }
    }

    if (pNewRow)
        free(pData);

    return GOS_OK;
}


INT32 mib_CompareEntry(MIB_TABLE_INDEX tableIndex, void * pRow1, void * pRow2)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32 i;
    INT32  ret = 0;
    CHAR   *strAttr1;
    CHAR   *strAttr2;

    strAttr1 = (CHAR *)malloc(MIB_TABLE_ATTR_MAX_SIZE);
    if (!strAttr1)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory fail for strAttr1 in function %s", __FUNCTION__);
        return -1;
    }

    strAttr2 = (CHAR *)malloc(MIB_TABLE_ATTR_MAX_SIZE);
    if (!strAttr2)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory fail for strAttr2 in function %s", __FUNCTION__);
        free(strAttr1);
        return -1;
    }

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_GetAttrIsIndex(tableIndex, attrIndex))
        {
            ret = 0;
            switch(MIB_GetAttrDataType(tableIndex, attrIndex))
            {
                case MIB_ATTR_TYPE_UINT8:
                {
                    UINT8 value1, value2;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value1, pRow1, 1);
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value2, pRow2, 1);
                    ret = value2 - value1;
                    break;
                }
                case MIB_ATTR_TYPE_UINT16:
                {
                    UINT16 value1, value2;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value1, pRow1, 2);
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value2, pRow2, 2);
                    ret = value2 - value1;
                    break;
                }
                case MIB_ATTR_TYPE_UINT32:
                {
                    UINT32 value1, value2;
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value1, pRow1, 4);
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &value2, pRow2, 4);
                    ret = value2 - value1;
                    break;
                }
                // Not supported
                case MIB_ATTR_TYPE_UINT64:
                {
                    break;
                }
                case MIB_ATTR_TYPE_STR:
                {
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, strAttr1, pRow1, MIB_GetAttrSize(tableIndex, attrIndex));
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, strAttr2, pRow2, MIB_GetAttrSize(tableIndex, attrIndex));
                    ret = memcmp(strAttr2, strAttr1, MIB_GetAttrSize(tableIndex, attrIndex));
                    break;
                }
                // Not supported
                case MIB_ATTR_TYPE_TABLE:
                {
                    break;
                }
                default:
                    break;
            }

            if (0 != ret)
            {
                free(strAttr1);
                free(strAttr2);
                return ret;
            }
        }
    }

    free(strAttr1);
    free(strAttr2);
    return ret;
}

BOOL mib_FindEntry(MIB_TABLE_INDEX tableIndex, void *pObjectRow, void *ppRetRow)
{
    MIB_TABLE_T    *pTable;
    MIB_ENTRY_T    *pEntry;
    void           *pRowData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow || !ppRetRow)
        return FALSE;

    if (0 == pTable->curEntryCount)
        return FALSE;

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        pRowData = pEntry->pData;

        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData);

            return TRUE;
        }
    }

    return FALSE;
}

BOOL mib_FindPmCurrentBin(MIB_TABLE_INDEX tableIndex, void *pObjectRow, void *ppRetRow)
{
    MIB_TABLE_T    *pTable;
    MIB_ENTRY_T    *pEntry;
    void           *pRowData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow || !ppRetRow)
        return FALSE;

    if (0 == pTable->curEntryCount)
        return FALSE;

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        pRowData = pEntry->pData;

        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            if (!pEntry->pPmCurrentBin)
                return FALSE;

            GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pEntry->pPmCurrentBin);

            return TRUE;
        }
    }

    return FALSE;
}

BOOL mib_FindNextEntry(MIB_TABLE_INDEX tableIndex, void *pObjectRow, void *ppRetRow)
{
    MIB_TABLE_T    *pTable;
    MIB_ENTRY_T    *pEntry, *pNextEntry;
    void           *pRowData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow || !ppRetRow)
        return FALSE;

    if (0 == pTable->curEntryCount)
        return FALSE;

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        pRowData = pEntry->pData;

        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            pNextEntry = LIST_NEXT(pEntry, entries);

            if (pNextEntry)
            {
                pRowData = pNextEntry->pData;

                GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData);

                return TRUE;
            }
            else
                return FALSE;
        }
    }

    return FALSE;
}

BOOL mib_FindSnapshotEntry(MIB_TABLE_INDEX tableIndex, void* pObjectRow, void* ppRetRow)
{
    MIB_TABLE_T*   pTable;
    UINT32         i;
    void*          pRowData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow || !ppRetRow)
    {
        return FALSE;
    }

    if ((NULL == pTable->pSnapshotData) || (0 == pTable->snapshotEntryCount))
    {
        return FALSE;
    }

    pRowData = pTable->pSnapshotData;
    for (i = 1; i <= pTable->snapshotEntryCount; i++)
    {
        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData);
            return TRUE;
        }

        if (0 > mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData);
            return FALSE;
        }

        pRowData = (void*)((UINT32)pRowData + MIB_GetTableEntrySize(tableIndex));
    }

    GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData);

    return FALSE;
}



BOOL mib_FindSnapshotNextEntry(MIB_TABLE_INDEX tableIndex, void* pObjectRow, void* ppRetRow)
{
    MIB_TABLE_T*   pTable;
    UINT32         i;
    void*          pRowData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow || !ppRetRow)
    {
        return FALSE;
    }

    if ((NULL == pTable->pSnapshotData) || (0 == pTable->snapshotEntryCount))
    {
        return FALSE;
    }

    pRowData = pTable->pSnapshotData;
    for (i = 1; i <= pTable->snapshotEntryCount; i++)
    {
        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            if (i == pTable->snapshotEntryCount)
            {
                return FALSE;
            }
            else
            {
                GOS_SetUINT32((UINT32*)ppRetRow, (UINT32)pRowData + MIB_GetTableEntrySize(tableIndex));
                return TRUE;
            }
        }

        pRowData = (void*)((UINT32)pRowData + MIB_GetTableEntrySize(tableIndex));
    }

    return FALSE;
}


GOS_ERROR_CODE mib_AddEntry(MIB_TABLE_INDEX tableIndex, void *pObjectRow, BOOL isNoUpload, BOOL isDisabled)
{
    MIB_TABLE_T         *pTable;
    MIB_ENTRY_T         *pEntry, *pEnrtyTmp = NULL;
    int                 tableSize;
    BOOL                flag = FALSE;
    omci_me_instance_t  new_instance_id, old_instance_id;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow)
        return GOS_FAIL;

    tableSize = MIB_GetTableEntrySize(tableIndex);

    // allocate memory instance metadata
    pEntry = (MIB_ENTRY_T *)malloc(sizeof(MIB_ENTRY_T));
    if (!pEntry)
        return GOS_FAIL;

    // be sure empty the contents of the memory
    memset(pEntry, 0, sizeof(MIB_ENTRY_T));

    pEntry->isNoUpload = isNoUpload;
    pEntry->isDisabled = isDisabled;

    // allocate memory for instance data
    pEntry->pData = malloc(tableSize);
    if (!pEntry->pData)
    {
        free(pEntry);
        return GOS_FAIL;
    }
    // be sure empty the contents of the memory
    memset(pEntry->pData, 0, tableSize);

    // if ME is PM type, allocate more memory for accumulation
    if ((OMCI_ME_TYPE_STANDARD_PM & MIB_GetTableStdType(tableIndex)) ||
            (OMCI_ME_TYPE_PROPRIETARY_PM & MIB_GetTableStdType(tableIndex)))
    {
        pEntry->pPmCurrentBin = malloc(tableSize);
        if (!pEntry->pPmCurrentBin)
        {
            free(pEntry->pData);
            free(pEntry);
            return GOS_FAIL;
        }
        memset(pEntry->pPmCurrentBin, 0, tableSize);
    }

    // copy instance data
    memcpy(pEntry->pData, pObjectRow, tableSize);

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &new_instance_id, pObjectRow, sizeof(omci_me_instance_t));

    // add instance id with smallest into list before.
    LIST_FOREACH(pEnrtyTmp, &pTable->entryHead, entries)
    {
        MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &old_instance_id, pEnrtyTmp->pData, sizeof(omci_me_instance_t));
        if (old_instance_id > new_instance_id)
        {
            break;
        }
        if (NULL == LIST_NEXT(pEnrtyTmp, entries))
        {
            flag = TRUE;
            break;
        }

    }

    if(flag)
    {
        LIST_INSERT_AFTER(pEnrtyTmp, pEntry, entries);
    }
    else if(pEnrtyTmp)
    {
        LIST_INSERT_BEFORE(pEnrtyTmp, pEntry, entries);
    }
    else
    {
        LIST_INSERT_HEAD(&pTable->entryHead, pEntry, entries);
    }

    // instance count++
    pTable->curEntryCount++;

    if (isNoUpload)
        pTable->curNoUploadCount++;

    return GOS_OK;
}


GOS_ERROR_CODE mib_DeleteEntry(MIB_TABLE_INDEX tableIndex, void *pObjectRow)
{
    MIB_TABLE_T    *pTable;
    MIB_ENTRY_T    *pEntry;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pObjectRow)
        return GOS_FAIL;

    if (0 == pTable->curEntryCount)
        return GOS_FAIL;

    // search instance list
    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        if(!memcmp(pObjectRow, pEntry->pData, MIB_GetTableEntrySize(tableIndex)))
        {
            // del instance from list
            LIST_REMOVE(pEntry,entries);

            // free memories
            if (pEntry->pAlarmTable)
                free(pEntry->pAlarmTable);
            if (pEntry->pPmCurrentBin)
                free(pEntry->pPmCurrentBin);
            free(pEntry->pData);
            free(pEntry);

            // instance count--
            pTable->curEntryCount--;

            return GOS_OK;
        }
    }

    return GOS_FAIL;
}




GOS_ERROR_CODE mib_SetAttributes(MIB_TABLE_INDEX tableIndex, void* pObjRow, void* pRefRow, MIB_ATTRS_SET* pAttrsSet)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32         i;
    CHAR           *attrBuf;
    GOS_ERROR_CODE retVal = GOS_OK;

    attrBuf = (CHAR *)malloc(MIB_TABLE_ATTR_MAX_SIZE);
    if (!attrBuf)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory fail for attrBuf in function %s", __FUNCTION__);
        return GOS_FAIL;
    }

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(pAttrsSet, attrIndex))
        {
            if (GOS_OK != MIB_GetAttrFromBuf(tableIndex, attrIndex, attrBuf, pRefRow, MIB_GetAttrSize(tableIndex, attrIndex)))
            {
                retVal = GOS_FAIL;
                break;
            }
            if (GOS_OK != mib_SetGetAttribute(tableIndex, attrBuf, pObjRow, attrIndex, TRUE))
            {
                retVal = GOS_FAIL;
                break;
            }
        }
    }

    free(attrBuf);
    return retVal;
}


BOOL mib_TableSave(MIB_TABLE_INDEX tableIndex)
{
    MIB_ATTR_INDEX attrIndex;
    UINT32 i;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_GetAttrMibSave(tableIndex, attrIndex))
        {
            return TRUE;
        }
    }

    return FALSE;
}


void* mib_GetAttrPtr(MIB_TABLE_INDEX tableIndex, void* pRow, MIB_ATTR_INDEX attrIndex)
{
    MIB_ATTR_INDEX tempAttrIndex;
    UINT32 i;
    UINT32 offset = (UINT32)pRow;


    for (tempAttrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, tempAttrIndex = MIB_ATTR_NEXT_INDEX(tempAttrIndex))
    {
        switch(MIB_GetAttrDataType(tableIndex, tempAttrIndex))
        {
            case MIB_ATTR_TYPE_UINT16:
            {
                offset = (offset + 1) / 2 * 2;
                break;
            }
            case MIB_ATTR_TYPE_UINT32:
            case MIB_ATTR_TYPE_UINT64:
            {
                offset = (offset + 3) / 4 * 4;
                break;
            }
            default:
                break;
        }

        if (tempAttrIndex == attrIndex)
        {
            return (void*)offset;
        }

        offset += MIB_GetAttrSize(tableIndex, tempAttrIndex);
    }

    return NULL;
}


GOS_ERROR_CODE mib_SetGetAttribute(MIB_TABLE_INDEX tableIndex, void* pAttr, void* pRow, MIB_ATTR_INDEX attrIndex, BOOL set)
{
    void*  attrPtr = mib_GetAttrPtr(tableIndex, pRow, attrIndex);
    UINT32 attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

    if (attrPtr)
    {
        if (set)
        {
            memcpy(attrPtr, pAttr, attrSize);
        }
        else
        {
            memcpy(pAttr, attrPtr, attrSize);
        }

        return GOS_OK;
    }

    return GOS_FAIL;
}

GOS_ERROR_CODE mib_DeleteSubTableEntry(MIB_TABLE_INDEX tableIndex, omci_me_instance_t entityId)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    CHAR                rowBuff[MIB_TABLE_ENTRY_MAX_SIZE];
    MIB_TABLE_INDEX     subTableIndex;
    GOS_ERROR_CODE      ret = GOS_OK;
    omci_me_instance_t  entryEntityId;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_ATTR_TYPE_TABLE == MIB_GetAttrDataType(tableIndex, attrIndex))
        {
            subTableIndex = MIB_GetTableIndexByName(MIB_GetAttrName(tableIndex, attrIndex));
            if (MIB_TABLE_INDEX_VALID(subTableIndex))
            {
                ret = MIB_GetFirst(subTableIndex, rowBuff, MIB_GetTableEntrySize(subTableIndex));
                while (GOS_OK == ret)
                {
                    MIB_GetAttrFromBuf(subTableIndex, MIB_ATTR_FIRST_INDEX, &entryEntityId, rowBuff, sizeof(UINT16));
                    if (entryEntityId == entityId)
                    {
                        MIB_Delete(subTableIndex, rowBuff, MIB_GetTableEntrySize(subTableIndex));
                    }
                    ret = MIB_GetNext(subTableIndex, rowBuff, MIB_GetTableEntrySize(subTableIndex));
                }
            }
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_Clear(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_T     *pTable = NULL;
    MIB_ENTRY_T     *pEntry = NULL;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return GOS_ERR_PARAM;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "MIB_Clear: table %s", MIB_GetTableName(tableIndex));

    if (pTable->curEntryCount + pTable->curNoUploadCount)
    {
        // search instance list
        while (!LIST_EMPTY(&pTable->entryHead))
        {
            pEntry = LIST_FIRST(&pTable->entryHead);
            LIST_REMOVE(pEntry, entries);

            // free memories
            if (pEntry->pAlarmTable)
                free(pEntry->pAlarmTable);
            if (pEntry->pPmCurrentBin)
                free(pEntry->pPmCurrentBin);
            free(pEntry->pData);
            free(pEntry);
        }

        pTable->curEntryCount = 0;
        pTable->curNoUploadCount = 0;
    }

    LIST_INIT(&pTable->entryHead);

    return GOS_OK;
}


MIB_TABLE_INDEX MIB_GetTableIndexByName(const CHAR* name)
{
    MIB_TABLE_INDEX   tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (0 == strcmp(name, MIB_GetTableName(tableIndex)))
        {
            return tableIndex;
        }
    }
    /* for short name */
    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (0 == strcmp(name, MIB_GetTableShortName(tableIndex)))
        {
            return tableIndex;
        }
    }
    return MIB_TABLE_UNKNOWN_INDEX;
}


UINT32 MIB_GetTableCurEntryCount(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_T* pTable;

    pTable = mib_GetTablePtr(tableIndex);

    if (!pTable)
    {
        return 0;
    }

    return pTable->curEntryCount;
}


UINT32 MIB_GetTableEntrySizeBeforePadded(MIB_TABLE_INDEX tableIndex)
{
    MIB_ATTR_INDEX    attrIndex;
    UINT32            i;
    UINT32            size = 0;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        size += MIB_GetAttrLen(tableIndex, attrIndex);
    }

    return size;
}


MIB_ATTR_INDEX MIB_GetAttrIndexByName(MIB_TABLE_INDEX tableIndex, const CHAR* name)
{
    MIB_ATTR_INDEX    attrIndex;
    UINT32            i;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_GetTableAttrNum(tableIndex);
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (0 == strcmp(MIB_GetAttrName(tableIndex, attrIndex), name))
        {
            return attrIndex;
        }
    }

    return MIB_ATTR_UNKNOWN_INDEX;
}


UINT32 MIB_GetAttrSize(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex)
{
    switch(MIB_GetAttrDataType(tableIndex, attrIndex))
    {
        case MIB_ATTR_TYPE_UINT8:
            return 1;
        case MIB_ATTR_TYPE_UINT16:
            return 2;
        case MIB_ATTR_TYPE_UINT32:
            return 4;
        case MIB_ATTR_TYPE_UINT64:
            return 8;
        case MIB_ATTR_TYPE_STR:
            return MIB_GetAttrLen(tableIndex, attrIndex) + 1;
        case MIB_ATTR_TYPE_TABLE:
            return MIB_GetAttrLen(tableIndex, attrIndex);
    }

    return 0;
}

GOS_ERROR_CODE MIB_Proprietary_Reg(MIB_TABLE_INDEX tableIndex, UINT8 cbBitMask)
{
    proprietary_mib_t *pMib = (proprietary_mib_t *)malloc(sizeof(proprietary_mib_t));

    if (!pMib)
        return GOS_FAIL;

    memset(pMib, 0, sizeof(proprietary_mib_t));

    pMib->tableId = tableIndex;
    pMib->cbBitMask = cbBitMask;
    LIST_INSERT_HEAD(&proprietaryMibHead, pMib, entries);

    return GOS_OK;
}


GOS_ERROR_CODE MIB_Proprietary_UnReg(void)
{
    proprietary_mib_t *pTmpEntry = NULL,*pRemoveEntry = NULL;

    for (pTmpEntry = LIST_FIRST(&proprietaryMibHead); pTmpEntry!=NULL;)
    {
        pRemoveEntry = pTmpEntry;
        pTmpEntry = LIST_NEXT(pTmpEntry,entries);
        LIST_REMOVE(pRemoveEntry,entries);
        free(pRemoveEntry);
    }
    return GOS_OK;
}


GOS_ERROR_CODE MIB_RegisterCallback(MIB_TABLE_INDEX tableIndex, MIB_CALLBACK_FUNC pfnPreCheck, MIB_CALLBACK_FUNC pfnCallback)
{
    UINT32         i;
    MIB_TABLE_T*   pTable;


    if ((NULL == pfnPreCheck) && (NULL == pfnCallback))
    {
        return GOS_ERR_PARAM;
    }

    pTable = mib_GetTablePtr(tableIndex);

    if (!pTable)
    {
        return GOS_ERR_PARAM;
    }

    if (pfnPreCheck)
    {
        for (i = 0; i < MIB_TABLE_CB_MAX_NUM; i++)
        {
            if (pTable->preCheckCB[i] == pfnPreCheck)
            {
                return GOS_ERR_DUPLICATED;
            }

            if (0 == pTable->preCheckCB[i])
            {
                pTable->preCheckCB[i] = pfnPreCheck;
                break;
            }
        }

        GOS_ASSERT(i != MIB_TABLE_CB_MAX_NUM);
    }

    if (pfnCallback)
    {
        for (i = 0; i < MIB_TABLE_CB_MAX_NUM; i++)
        {
            if (pTable->operationCB[i] == pfnCallback)
            {
                return GOS_ERR_DUPLICATED;
            }

            if (0 == pTable->operationCB[i])
            {
                pTable->operationCB[i] = pfnCallback;
                break;
            }
        }

        GOS_ASSERT(i != MIB_TABLE_CB_MAX_NUM);
    }

    return GOS_OK;
}

GOS_ERROR_CODE MIB_RegisterCallbackToAll(MIB_CALLBACK_FUNC pfnPreCheck, MIB_CALLBACK_FUNC pfnCallback)
{
    MIB_TABLE_INDEX tableIndex;

    if ((NULL == pfnPreCheck) && (NULL == pfnCallback))
    {
        return GOS_ERR_PARAM;
    }

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        MIB_RegisterCallback(tableIndex, pfnPreCheck, pfnCallback);
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_ClearAttrSet(MIB_ATTRS_SET* pAttrSet)
{
    memset(pAttrSet, 0x00, sizeof(MIB_ATTRS_SET));

    return GOS_OK;
}

GOS_ERROR_CODE MIB_UnSetAttrSet(MIB_ATTRS_SET* pAttrSet, MIB_ATTR_INDEX attrIndex)
{
    (*pAttrSet) = (*pAttrSet) & ~(1 << (attrIndex - MIB_ATTR_FIRST_INDEX));
    return GOS_OK;
}

GOS_ERROR_CODE MIB_SetAttrSet(MIB_ATTRS_SET* pAttrSet, MIB_ATTR_INDEX attrIndex)
{
    (*pAttrSet) = (*pAttrSet) | (1 << (attrIndex - MIB_ATTR_FIRST_INDEX));
    return GOS_OK;
}


BOOL MIB_IsInAttrSet(MIB_ATTRS_SET* pAttrSet, MIB_ATTR_INDEX attrIndex)
{
    return ((*pAttrSet) & (1 << (attrIndex - MIB_ATTR_FIRST_INDEX)));
}


GOS_ERROR_CODE MIB_FullAttrSet(MIB_ATTRS_SET* pAttrSet)
{
    *pAttrSet = (MIB_ATTRS_SET)0xFFFFFFFF;
    return GOS_OK;
}


UINT32 MIB_GetAttrNumOfSet(MIB_ATTRS_SET* pAttrSet)
{
    MIB_ATTR_INDEX    attrIndex;
    UINT32            i;
    UINT32            count = 0;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0; i < MIB_TABLE_ATTR_MAX_NUM;
         i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (MIB_IsInAttrSet(pAttrSet, attrIndex))
        {
            count++;
        }
    }

    return count;
}


MIB_ENTRY_T* MIB_GetTableEntry(MIB_TABLE_INDEX tableIndex, void* pObjectRow)
{
    MIB_TABLE_T*   pTable;
    MIB_ENTRY_T *np;
    void *pRowData;

    pTable = mib_GetTablePtr(tableIndex);

    if (!pTable || !pObjectRow)
    {
        return NULL;
    }

    if (0 == pTable->curEntryCount)
    {
        return NULL;
    }

    LIST_FOREACH(np,&pTable->entryHead,entries)
    {
        pRowData = np->pData;
        if (0 == mib_CompareEntry(tableIndex, pRowData, pObjectRow))
        {
            return np;
        }
    }


    return NULL;
}

GOS_ERROR_CODE MIB_GetCurrentData(MIB_TABLE_INDEX tableIndex, void *pRow,
    UINT32 rowLen,omci_me_instance_t entityID)

{
    /*Update the newest data to current bin file of specified ME*/
    omci_pm_update_specified_ME(tableIndex,entityID);
    return MIB_GetPmCurrentBin(tableIndex, pRow, rowLen);

}



GOS_ERROR_CODE MIB_Get(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if (mib_FindEntry(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);
        MIB_FullAttrSet(&attrSet);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pTmpEntry, &attrSet, MIB_GET))
        {
            memcpy(pRow, pTmpEntry, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_GetPmCurrentBin(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if (mib_FindPmCurrentBin(tableIndex, pRow, &pMibRow))
    {
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
        memcpy(pTmpEntry, pMibRow, rowLen);
        MIB_FullAttrSet(&attrSet);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pTmpEntry, &attrSet, MIB_GET))
        {
            memcpy(pRow, pTmpEntry, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        free(pTmpEntry);
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_GetNext(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // start search entry
    if (mib_FindNextEntry(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s",
                    __FUNCTION__, MIB_GetTableName(tableIndex));
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s",
                __FUNCTION__, MIB_GetTableName(tableIndex));
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);
        MIB_FullAttrSet(&attrSet);

        // read out the instanceID
        MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pTmpEntry, &attrSet, MIB_GETNEXT))
        {
            memcpy(pRow, pTmpEntry, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_GetFirst(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;
    MIB_TABLE_T         *pTable;
    MIB_ENTRY_T         *pEntry;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // start search entry
    if (pTable->curEntryCount)
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s",
                    __FUNCTION__, MIB_GetTableName(tableIndex));
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s",
                __FUNCTION__, MIB_GetTableName(tableIndex));
            return ret;
        }
#endif
        pEntry = LIST_FIRST(&pTable->entryHead);
        memcpy(pTmpEntry, pEntry->pData, rowLen);
        MIB_FullAttrSet(&attrSet);

        // read out the instanceID
        MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pTmpEntry, sizeof(omci_me_instance_t));

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pTmpEntry, &attrSet, MIB_GET))
        {
            memcpy(pRow, pTmpEntry, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_Set(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // make all attributes in the set mask
    MIB_FullAttrSet(&attrSet);

    // start search entry
    if (mib_FindEntry(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pRow, &attrSet, MIB_SET))
        {
            memcpy(pMibRow, pRow, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }
    else
    {
        if (GOS_OK == mib_InvokeCallbacks(tableIndex, NULL, pRow, &attrSet, MIB_ADD))
        {
            if (GOS_OK == mib_AddEntry(tableIndex, pRow, FALSE, FALSE))
            {
                ret = GOS_OK;
            }
            else
                mib_InvokeCallbacks(tableIndex, pRow, NULL, &attrSet, MIB_DEL);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_SetPmCurrentBin(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if (mib_FindPmCurrentBin(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);
        MIB_FullAttrSet(&attrSet);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pRow, &attrSet, MIB_SET))
        {
            memcpy(pMibRow, pRow, rowLen);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_Add(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen, BOOL isNoUpload, BOOL isDisabled)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // make all attributes in the set mask
    MIB_FullAttrSet(&attrSet);

    // start search entry
    if (mib_FindEntry(tableIndex, pRow, &pMibRow))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance exist in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_ERR_DUPLICATED;
    }
    else
    {
        if (GOS_OK == mib_InvokeCallbacks(tableIndex, NULL, pRow, &attrSet, MIB_ADD))
        {
            if (GOS_OK == mib_AddEntry(tableIndex, pRow, isNoUpload, isDisabled))
            {
                ret = GOS_OK;
            }
            else
                mib_InvokeCallbacks(tableIndex, pRow, NULL, &attrSet, MIB_DEL);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_SetAttributes(MIB_TABLE_INDEX tableIndex, void *pRow, UINT32 rowLen, MIB_ATTRS_SET *pAttrSet)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;

    if (!pRow || !pAttrSet)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if (mib_FindEntry(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, pRow, pAttrSet, MIB_SET))
        {
            ret = mib_SetAttributes(tableIndex, pMibRow, pRow, pAttrSet);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE MIB_Delete(MIB_TABLE_INDEX tableIndex, void* pRow, UINT32 rowLen)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    void                *pMibRow;
    omci_me_instance_t  instanceID;
    MIB_ATTRS_SET       attrSet;

    if (!pRow)
        return GOS_ERR_PARAM;

    // make sure the buffer size is matched
    if (rowLen != MIB_GetTableEntrySize(tableIndex))
        return ret;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if (mib_FindEntry(tableIndex, pRow, &pMibRow))
    {
#ifdef OMCI_X86
        CHAR tmp[TMP_MALLOC_LEN];
        CHAR *pTmpEntry=NULL;

        if(rowLen<TMP_MALLOC_LEN)
        {
            pTmpEntry=tmp;
        }
        else
        {
            pTmpEntry = (CHAR *)malloc(rowLen);
            if (!pTmpEntry)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
                return ret;
            }
        }
#else
        CHAR *pTmpEntry = (CHAR *)malloc(rowLen);
        if (!pTmpEntry)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for callback fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }
#endif
        memcpy(pTmpEntry, pMibRow, rowLen);
        MIB_FullAttrSet(&attrSet);

        if (GOS_OK == mib_InvokeCallbacks(tableIndex, pTmpEntry, NULL, &attrSet, MIB_DEL))
        {
            mib_DeleteEntry(tableIndex, pMibRow);
            //mib_DeleteSubTableEntry(tableIndex, instanceID);
            ret = GOS_OK;
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Callbacks return failure in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
#ifdef OMCI_X86
        if(rowLen>=TMP_MALLOC_LEN)
            free(pTmpEntry);
#else
        free(pTmpEntry);
#endif
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}


GOS_ERROR_CODE MIB_ClearAll(void)
{
   MIB_TABLE_INDEX tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        MIB_Clear(tableIndex);
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_Default(MIB_TABLE_INDEX tableIndex, void* pRow, UINT32 rowLen)
{
    void* pDefaultRow = MIB_GetDefaultRow(tableIndex);
    GOS_ASSERT(rowLen == MIB_GetTableEntrySize(tableIndex));

    if (pRow)
    {
        memcpy(pRow, pDefaultRow, rowLen);
        return GOS_OK;
    }

    return GOS_ERR_PARAM;
}


GOS_ERROR_CODE MIB_SetAttrToBuf(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex, void *pAttr, void *pBuf, UINT32 size)
{
    GOS_ASSERT(size == MIB_GetAttrSize(tableIndex, attrIndex));

    return mib_SetGetAttribute(tableIndex, pAttr, pBuf, attrIndex, TRUE);
}


GOS_ERROR_CODE MIB_GetAttrFromBuf(MIB_TABLE_INDEX tableIndex, MIB_ATTR_INDEX attrIndex, void *pAttr, void *pBuf, UINT32 size)
{
    GOS_ASSERT(size == MIB_GetAttrSize(tableIndex, attrIndex));
    return mib_SetGetAttribute(tableIndex, pAttr, pBuf, attrIndex, FALSE);
}


GOS_ERROR_CODE
MIB_CreateSnapshot(
    MIB_TABLE_INDEX tableIndex)
{
    int i=0;
    MIB_TABLE_T*   pTable;
    MIB_ENTRY_T*   pEntry;
    INT32 entrySize, entryCnt = 0;
    void *curPtr;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: TableId %d", __FUNCTION__, tableIndex);
    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s: err parem",__FUNCTION__);
        return GOS_ERR_PARAM;
    }

    if (pTable->pSnapshotData)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s: err duplicated", __FUNCTION__);
        return GOS_ERR_DUPLICATED;
    }
    entrySize= MIB_GetTableEntrySize(tableIndex);

    entryCnt = pTable->curEntryCount - pTable->curNoUploadCount;
    pTable->pSnapshotData = malloc(entryCnt * entrySize);
    GOS_ASSERT(pTable->pSnapshotData);
    memset(pTable->pSnapshotData, 0x00, entryCnt * entrySize);
    pTable->snapshotEntryCount = entryCnt;

    curPtr = pTable->pSnapshotData;
    LIST_FOREACH(pEntry,&pTable->entryHead,entries)
    {
        if (pEntry->isNoUpload)
            continue;

        memcpy(curPtr,pEntry->pData, entrySize);

        curPtr+=entrySize;
        i++;
    }
    return GOS_OK;
}



GOS_ERROR_CODE MIB_DeleteSnapshot(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_T*   pTable;
    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
    {
        return GOS_ERR_PARAM;
    }

    if (!pTable->pSnapshotData)
    {
        return GOS_ERR_DUPLICATED;
    }

    free(pTable->pSnapshotData);
    pTable->pSnapshotData = NULL;
    pTable->snapshotEntryCount = 0;

    return GOS_OK;
}


UINT32 MIB_GetSnapshotEntryCount(MIB_TABLE_INDEX tableIndex)
{
    MIB_TABLE_T*   pTable;
    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
    {
        return GOS_ERR_PARAM;
    }

    return pTable->snapshotEntryCount;
}


GOS_ERROR_CODE MIB_GetSnapshot(MIB_TABLE_INDEX tableIndex, void* pRow, UINT32 rowLen)
{
    void*          pMibRow;
    GOS_ERROR_CODE ret = GOS_FAIL;

    GOS_ASSERT(rowLen == MIB_GetTableEntrySize(tableIndex));

    if (mib_FindSnapshotEntry(tableIndex, pRow, &pMibRow))
    {
        memcpy(pRow, pMibRow, rowLen);
        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Entry not found in MIB_GetSnapshot: %s",
                  MIB_GetTableName(tableIndex));
    }

    return ret;
}


GOS_ERROR_CODE MIB_GetSnapshotNext(MIB_TABLE_INDEX tableIndex, void* pRow, UINT32 rowLen)
{
    void*          pMibRow;
    GOS_ERROR_CODE ret = GOS_FAIL;

    GOS_ASSERT(rowLen == MIB_GetTableEntrySize(tableIndex));

    if (mib_FindSnapshotNextEntry(tableIndex, pRow, &pMibRow))
    {
        memcpy(pRow, pMibRow, rowLen);
        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Entry not found in MIB_GetSnapshotNext: %s",
                  MIB_GetTableName(tableIndex));
    }

    return ret;
}


GOS_ERROR_CODE MIB_GetSnapshotFirst(MIB_TABLE_INDEX tableIndex, void* pRow, UINT32 rowLen)
{
    MIB_TABLE_T*   pTable;
    GOS_ERROR_CODE ret = GOS_FAIL;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
    {
        return GOS_ERR_PARAM;
    }

    GOS_ASSERT(rowLen == MIB_GetTableEntrySize(tableIndex));

    if (pTable->snapshotEntryCount)
    {
        memcpy(pRow, pTable->pSnapshotData, rowLen);
        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Entry not found in MIB_GetSnapshotFirst: %s",
                  MIB_GetTableName(tableIndex));
    }

    return ret;
}


GOS_ERROR_CODE MIB_InfoRegister(MIB_TABLE_INDEX tableIdx,MIB_TABLE_INFO_T *pTableInfo, MIB_TABLE_OPER_T *oper)
{
    MIB_TABLE_T *pTable;

    pTable = mib_GetTablePtr(tableIdx);

    if(pTable)
    {
        pTable->curEntryCount = 0;
        pTable->snapshotEntryCount = 0;
        pTable->almSnapShotCnt = 0;
        pTable->pSnapshotData = NULL;
        pTable->pAlmSnapShot = NULL;
        pTable->pTableInfo = pTableInfo;
        pTable->meOper = oper;
        return GOS_OK;
    }

    return GOS_ERR_INVALID_INPUT;
}

GOS_ERROR_CODE MIB_TableSortingByTableIndex(void)
{
    MIB_TABLE_T *pTable;
    MIB_TABLE_T *pNextTable;
    int i, j;

    for (i = 1; i < MIB_TABLE_TOTAL_NUMBER; i++)
    {
        for (j = 0, pTable = LIST_FIRST(&gTables.head); j < MIB_TABLE_TOTAL_NUMBER - 1; j++)
        {
            pNextTable = LIST_NEXT(pTable, entries);
            if (pTable->tableIndex> pNextTable->tableIndex)
            {
                LIST_REMOVE(pTable, entries);
                LIST_INSERT_AFTER(pNextTable, pTable, entries);
            }
            else
                pTable = pNextTable;
        }
    }
    return GOS_OK;
}

MIB_TABLE_INDEX MIB_Register(void)
{
    MIB_TABLE_T *pTable;

    pTable = (MIB_TABLE_T*)malloc(sizeof(MIB_TABLE_T));

    if(!pTable)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Malloc mib table Fail");
        return MIB_TABLE_UNKNOWN_INDEX;
    }

    memset(pTable, 0x00, sizeof(MIB_TABLE_T));

    if(pTable)
    {
        pTable->tableIndex = gTables.tablesCount++;
        pTable->curEntryCount = 0;
        pTable->snapshotEntryCount = 0;
        pTable->almSnapShotCnt = 0;
        pTable->pSnapshotData = NULL;
        pTable->pAlmSnapShot = NULL;
        pTable->pTableInfo = NULL;
        pTable->meOper = NULL;
        LIST_INIT(&(pTable->entryHead));
        LIST_INSERT_HEAD(&gTables.head, pTable, entries);
    }

    return pTable->tableIndex;
}


GOS_ERROR_CODE MIB_UnRegister(MIB_TABLE_INDEX tableIndex) {

    MIB_TABLE_T *pTable ;
    for (pTable = gTables.head.lh_first; pTable != NULL; pTable = pTable->entries.le_next) {
           if ( pTable->tableIndex == tableIndex ) {
            LIST_REMOVE(pTable, entries);
            if(pTable){
                free(pTable);
            }
            gTables.tablesCount--;
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,"%s: unregister a tableId %d!",__FUNCTION__,tableIndex);
            return GOS_OK;
           }
    }
    return GOS_ERR_INVALID_INPUT;
}


int MIB_GetTableNum(void)
{
    return gTables.tablesCount;
}



GOS_ERROR_CODE MIB_ShowAll(void)
{
   MIB_TABLE_T *pTable;

    for (pTable = gTables.head.lh_first; pTable != NULL; pTable = pTable->entries.le_next)
    {

        OMCI_PRINT("TableId [%d] Name: %s!",pTable->tableIndex,pTable->pTableInfo->Name);

    }

    return GOS_OK;
}

MIB_TABLE_INDEX MIB_GetTableIndexByClassId(omci_me_class_t classId)
{
    MIB_TABLE_INDEX   tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (classId == MIB_GetTableClassId(tableIndex))
        {
            return tableIndex;
        }
    }

    return MIB_TABLE_UNKNOWN_INDEX;
}

GOS_ERROR_CODE MIB_GetNextClassIdTableIndex(MIB_TABLE_INDEX *tableIndex)
{
    return mib_GetNextTablePtr(tableIndex);
}

MIB_TABLE_INDEX MIB_GetFirstTableindex(void)
{
    MIB_TABLE_T *pTable;
    pTable = LIST_FIRST(&gTables.head);
    return pTable->tableIndex;
}


BOOL MIB_TableSupportAction(MIB_TABLE_INDEX tableIndex, omci_me_action_mask_t action)
{
    UINT32 actionSet = MIB_GetTableActionType(tableIndex);

    return (actionSet & action);
}


GOS_ERROR_CODE MIB_CreatePublicTblSnapshot(void)
{
   MIB_TABLE_INDEX tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (!(OMCI_ME_TYPE_PRIVATE & MIB_GetTableStdType(tableIndex)))
        {
            MIB_CreateSnapshot(tableIndex);
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_DeletePublicTblSnapshot(void)
{
    MIB_TABLE_INDEX tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (!(OMCI_ME_TYPE_PRIVATE & MIB_GetTableStdType(tableIndex)))
        {
            MIB_DeleteSnapshot(tableIndex);
        }
    }

    return GOS_OK;
}


GOS_ERROR_CODE MIB_ClearPublic(void)
{
    MIB_TABLE_INDEX tableIndex;
    for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        if (!(OMCI_ME_TYPE_PRIVATE & MIB_GetTableStdType(tableIndex)))
        {
            MIB_Clear(tableIndex);
        }
    }

    return GOS_OK;
}

MIB_ENTRY_T* mib_FindEntryByInstanceID(MIB_TABLE_INDEX tableIndex, omci_me_instance_t instanceID)
{
    MIB_TABLE_T     *pTable;
    MIB_ENTRY_T     *pEntry;
    void            *pAttrData;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return NULL;

    if (0 == pTable->curEntryCount)
        return NULL;

    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        pAttrData = mib_GetAttrPtr(tableIndex, pEntry->pData, MIB_ATTR_FIRST_INDEX);
        if (!pAttrData)
            continue;

        if (0 == memcmp(pAttrData, &instanceID, sizeof(omci_me_instance_t)))
            return pEntry;
    }

    return NULL;
}

BOOL mib_FindAlarmTable(MIB_TABLE_INDEX     tableIndex,
                        omci_me_instance_t  instanceID,
                        mib_alarm_table_t   **ppAlarmTable)
{
    MIB_ENTRY_T     *pEntry;

    pEntry = mib_FindEntryByInstanceID(tableIndex, instanceID);
    if (!pEntry)
        return FALSE;

    if (!pEntry->pAlarmTable)
        return FALSE;

    *ppAlarmTable = pEntry->pAlarmTable;

    return TRUE;
}

// must be called after MIB_Set() for new entry
GOS_ERROR_CODE mib_alarm_table_add(MIB_TABLE_INDEX tableIndex, void *pRow)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    MIB_ENTRY_T         *pEntry;
    omci_me_instance_t  instanceID;

    if (!pRow)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if ((pEntry = MIB_GetTableEntry(tableIndex, pRow)))
    {
        mib_alarm_table_t *pAlarmTable = malloc(sizeof(mib_alarm_table_t));
        if (!pAlarmTable)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Allocate memory for alarm table fail in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            return ret;
        }

        memset(pAlarmTable, 0, sizeof(mib_alarm_table_t));
        pEntry->pAlarmTable = pAlarmTable;

        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

// must be called before MIB_Delete()
GOS_ERROR_CODE mib_alarm_table_del(MIB_TABLE_INDEX tableIndex, void *pRow)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    MIB_ENTRY_T         *pEntry;
    omci_me_instance_t  instanceID;

    if (!pRow)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pRow, sizeof(omci_me_instance_t));

    // start search entry
    if ((pEntry = MIB_GetTableEntry(tableIndex, pRow)))
    {
        if (pEntry->pAlarmTable)
        {
            free(pEntry->pAlarmTable);
            pEntry->pAlarmTable = NULL;
        }

        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE mib_alarm_table_get(MIB_TABLE_INDEX      tableIndex,
                                    omci_me_instance_t  instanceID,
                                    mib_alarm_table_t   *pAlarmTable)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    mib_alarm_table_t   *pInstAlmTbl;

    if (!pAlarmTable)
        return GOS_ERR_PARAM;

    // start search entry
    if (mib_FindAlarmTable(tableIndex, instanceID, &pInstAlmTbl))
    {
        memcpy(pAlarmTable, pInstAlmTbl, sizeof(mib_alarm_table_t));

        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE mib_alarm_table_set(MIB_TABLE_INDEX      tableIndex,
                                    omci_me_instance_t  instanceID,
                                    mib_alarm_table_t   *pAlarmTable)
{
    GOS_ERROR_CODE      ret = GOS_FAIL;
    mib_alarm_table_t   *pInstAlmTbl;

    if (!pAlarmTable)
        return GOS_ERR_PARAM;

    // start search entry
    if (mib_FindAlarmTable(tableIndex, instanceID, &pInstAlmTbl))
    {
        memcpy(pInstAlmTbl, pAlarmTable, sizeof(mib_alarm_table_t));

        ret = GOS_OK;
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
    }

    if (GOS_OK == ret)
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: ME %s, instance 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

    return ret;
}

GOS_ERROR_CODE mib_alarm_table_update(mib_alarm_table_t     *pAlarmTable,
                                        omci_alm_data_t     *pAlarmData,
                                        BOOL                *pIsUpdated)
{
    if (!pAlarmTable || !pAlarmData || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;


    // update alarm status if it has being changed
    if (pAlarmData->almStatus != m_omci_get_alm_sts(pAlarmTable->aBitMask, pAlarmData->almNumber))
    {
        if (OMCI_ALM_STS_DECLARE == pAlarmData->almStatus)
        {
            pAlarmTable->aBitMask[m_omci_alm_nbr_in_byte(pAlarmData->almNumber)] |=
                m_omci_alm_nbr_in_bit(pAlarmData->almNumber);
        }
        else
        {
            pAlarmTable->aBitMask[m_omci_alm_nbr_in_byte(pAlarmData->almNumber)] &=
                ~m_omci_alm_nbr_in_bit(pAlarmData->almNumber);
        }

        *pIsUpdated = TRUE;
    }

    return GOS_OK;
}
