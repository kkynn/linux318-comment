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
 * Purpose : Definition of ME handler: ONU remote debug (158)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ONU remote debug (158)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T            gMibOrdTblInfo;
MIB_ATTR_INFO_T             gMibOrdAttrInfo[MIB_TABLE_ONU_REMOTE_DBG_ATTR_NUM];
MIB_TABLE_ONU_REMOTE_DBG_T  gMibOrdDefRow;
MIB_TABLE_OPER_T            gMibOrdOper;

extern omci_mulget_info_ts  gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];


GOS_ERROR_CODE onu_remote_dbg_dump_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    MIB_TABLE_ONU_REMOTE_DBG_T  *pMibOrd;
    ord_attr_reply_tbl_entry_t  *pEntry;
    ord_attr_reply_tbl_t        *pEntryData;

    pMibOrd = (MIB_TABLE_ONU_REMOTE_DBG_T *)pData;

	OMCI_PRINT("EntityId: 0x%04x", pMibOrd->EntityId);
	OMCI_PRINT("CmdFmt: %u", pMibOrd->CmdFmt);
	OMCI_PRINT("Cmd: %s", pMibOrd->Cmd);
	OMCI_PRINT("ReplyTbl:");
    LIST_FOREACH(pEntry, &pMibOrd->ReplyTbl_head, entries)
    {
        pEntryData = &pEntry->tableEntry;

        printf("%s", pEntryData->ReplyTbl);
    }

	return GOS_OK;
}

GOS_ERROR_CODE onu_remote_dbg_attr_reply_tbl_clear(MIB_TABLE_ONU_REMOTE_DBG_T *pOrd)
{
    ord_attr_reply_tbl_entry_t  *pEntry;
    ord_attr_reply_tbl_entry_t  *pTempEntry;

    if (!pOrd)
        return GOS_ERR_INVALID_INPUT;

    // clean up the reply table
    pEntry = LIST_FIRST(&pOrd->ReplyTbl_head);
    while (pEntry)
    {
        pTempEntry = LIST_NEXT(pEntry, entries);
        free(pEntry);
        pEntry = pTempEntry;
    }
    LIST_INIT(&pOrd->ReplyTbl_head);

    return GOS_OK;
}

GOS_ERROR_CODE onu_remote_dbg_drv_cfg_handler(void              *pOldRow,
                                                void            *pNewRow,
                                                MIB_OPERA_TYPE  operationType,
                                                MIB_ATTRS_SET   attrSet,
                                                UINT32          pri)
{
    MIB_TABLE_ONU_REMOTE_DBG_T  *pOrd;
    MIB_TABLE_ONU_REMOTE_DBG_T  *pMibOrd;
    GOS_ERROR_CODE              ret = GOS_OK;

    pOrd = (MIB_TABLE_ONU_REMOTE_DBG_T *)pNewRow;

    switch (operationType)
    {
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_REMOTE_DBG_CMD_INDEX))
            {
                ord_attr_reply_tbl_entry_t  *pEntry;
                ord_attr_reply_tbl_entry_t  *pTempEntry;

                if (!mib_FindEntry(MIB_TABLE_ONU_REMOTE_DBG_INDEX, pOrd, &pMibOrd))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                            "cannot find onu remote debug mib entry");

                    return GOS_ERR_NOT_FOUND;
                }

                // clean up tables
                onu_remote_dbg_attr_reply_tbl_clear(pMibOrd);

                if (0 == strcmp(pOrd->Cmd, "help"))
                {
                    // TBD
                }
                else
                {
                    char    replyBuf[MIB_TABLE_ORD_REPLY_TBL_LEN];
                    FILE    *pResultFD;

                    memset(replyBuf, 0, sizeof(replyBuf));

                    pResultFD = popen(pOrd->Cmd, "r");
                    if (!pResultFD)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                            "popen fail for onu remote debug window");

                        return GOS_FAIL;
                    }

                    // find the latest entry
                    if (LIST_EMPTY(&pMibOrd->ReplyTbl_head))
                        pTempEntry = NULL;
                    else
                    {
                        LIST_FOREACH(pTempEntry, &pMibOrd->ReplyTbl_head, entries)
                        {
                            if (!LIST_NEXT(pTempEntry, entries))
                                break;
                        }
                    }

                    while (fgets(replyBuf, sizeof(replyBuf), pResultFD))
                    {
                        pEntry = malloc(sizeof(ord_attr_reply_tbl_entry_t));

                        memcpy(pEntry->tableEntry.ReplyTbl, replyBuf, sizeof(replyBuf));

                        if (LIST_EMPTY(&pMibOrd->ReplyTbl_head))
                            LIST_INSERT_HEAD(&pMibOrd->ReplyTbl_head, pEntry, entries);
                        else
                            LIST_INSERT_AFTER(pTempEntry, pEntry, entries);

                        pTempEntry = pEntry;
                    }

                    pclose(pResultFD);
                }
            }
            break;

        case MIB_GET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_REMOTE_DBG_REPLY_TBL_INDEX))
            {
                MIB_ATTR_INDEX              attrIndex;
                omci_mulget_attr_ts         *pMgAttr;
                UINT8                       *pMgAttrData;
                ord_attr_reply_tbl_entry_t  *pEntry;
                UINT8                       entryCnt = 0;

                attrIndex = MIB_TABLE_ONU_REMOTE_DBG_REPLY_TBL_INDEX;
                pMgAttr = &gOmciMulGetData[pri].attribute[attrIndex];
                pMgAttrData = pMgAttr->attrValue;

                LIST_FOREACH(pEntry, &pOrd->ReplyTbl_head, entries)
                {
                    memcpy(pMgAttrData, &pEntry->tableEntry, MIB_TABLE_ORD_REPLY_TBL_LEN);
                    pMgAttrData += MIB_TABLE_ORD_REPLY_TBL_LEN;
                    entryCnt++;
                }

                pMgAttr->attrSize = MIB_TABLE_ORD_REPLY_TBL_LEN * entryCnt;
                pMgAttr->attrIndex = attrIndex;
                pMgAttr->doneSeqNum = 0;
                pMgAttr->maxSeqNum =
                    (pMgAttr->attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) /
                    OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;
            }
            break;

        case MIB_DEL:
            if (!mib_FindEntry(MIB_TABLE_ONU_REMOTE_DBG_INDEX, pOrd, &pMibOrd))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                        "cannot find onu remote debug mib entry");

                return GOS_ERR_NOT_FOUND;
            }

            // clean up tables
            onu_remote_dbg_attr_reply_tbl_clear(pMibOrd);

        default:
            break;
    }

    return ret;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;

    gMibOrdTblInfo.Name = "OnuRemoteDebug";
    gMibOrdTblInfo.ShortName = "ORD";
    gMibOrdTblInfo.Desc = "ONU remote debug";
    gMibOrdTblInfo.ClassId = OMCI_ME_CLASS_ONU_REMOTE_DEBUG;
    gMibOrdTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibOrdTblInfo.StdType = OMCI_ME_TYPE_STANDARD | OMCI_ME_TYPE_NOT_MIB_UPLOAD;
    gMibOrdTblInfo.ActionType =
        OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT;
    gMibOrdTblInfo.pAttributes = &(gMibOrdAttrInfo[0]);
	gMibOrdTblInfo.attrNum = MIB_TABLE_ONU_REMOTE_DBG_ATTR_NUM;
	gMibOrdTblInfo.entrySize = sizeof(MIB_TABLE_ONU_REMOTE_DBG_T);
	gMibOrdTblInfo.pDefaultRow = &gMibOrdDefRow;

    attrIndex = MIB_TABLE_ONU_REMOTE_DBG_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOrdAttrInfo[attrIndex].Name = "EntityId";
    gMibOrdAttrInfo[attrIndex].Desc = "Entity ID";
    gMibOrdAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOrdAttrInfo[attrIndex].Len = 2;
    gMibOrdAttrInfo[attrIndex].IsIndex = TRUE;
    gMibOrdAttrInfo[attrIndex].MibSave = TRUE;
    gMibOrdAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOrdAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibOrdAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOrdAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_ONU_REMOTE_DBG_CMD_FMT_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOrdAttrInfo[attrIndex].Name = "CmdFmt";
    gMibOrdAttrInfo[attrIndex].Desc = "Command format";
    gMibOrdAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOrdAttrInfo[attrIndex].Len = 1;
    gMibOrdAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOrdAttrInfo[attrIndex].MibSave = TRUE;
    gMibOrdAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOrdAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOrdAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOrdAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_ONU_REMOTE_DBG_CMD_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOrdAttrInfo[attrIndex].Name = "Cmd";
    gMibOrdAttrInfo[attrIndex].Desc = "Command";
    gMibOrdAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
    gMibOrdAttrInfo[attrIndex].Len = 25;
    gMibOrdAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOrdAttrInfo[attrIndex].MibSave = TRUE;
    gMibOrdAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibOrdAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOrdAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOrdAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_ONU_REMOTE_DBG_REPLY_TBL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOrdAttrInfo[attrIndex].Name = "ReplyTbl";
    gMibOrdAttrInfo[attrIndex].Desc = "Reply table";
    gMibOrdAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibOrdAttrInfo[attrIndex].Len = 25;
    gMibOrdAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOrdAttrInfo[attrIndex].MibSave = TRUE;
    gMibOrdAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOrdAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOrdAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOrdAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibOrdDefRow, 0x00, sizeof(gMibOrdDefRow));
    LIST_INIT(&gMibOrdDefRow.ReplyTbl_head);

    memset(&gMibOrdOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibOrdOper.meOperDrvCfg = onu_remote_dbg_drv_cfg_handler;
	gMibOrdOper.meOperConnCfg = NULL;
    gMibOrdOper.meOperConnCheck = NULL;
    gMibOrdOper.meOperDump = onu_remote_dbg_dump_handler;

	MIB_TABLE_ONU_REMOTE_DBG_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibOrdTblInfo, &gMibOrdOper);

    return GOS_OK;
}
