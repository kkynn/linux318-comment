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
 */


#include "app_basic.h"

MIB_TABLE_INFO_T gMibOmciTableInfo;
MIB_ATTR_INFO_T  gMibOmciAttrInfo[MIB_TABLE_OMCI_ATTR_NUM];
MIB_TABLE_OMCI_T gMibOmciDefRow;
MIB_TABLE_OPER_T gMibOmciOper;


extern omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];

GOS_ERROR_CODE OmciDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_OMCI_T *pOmci = (MIB_TABLE_OMCI_T*)pData;
	omci_meType_tbl_entry_t *pEntry;
	UINT32 i = 0;
	CHAR tmpBuf[100];

	OMCI_PRINT("EntityId: 0x%02x", pOmci->EntityId);

	OMCI_PRINT("MsgTypeTbl: ");
	LIST_FOREACH(pEntry,&pOmci->meType_head, entries)
	{
		sprintf(tmpBuf+(i*7), "%05d, ", pEntry->tableEntry.meTypeTbl);

		if( 9 == i )
		{
			OMCI_PRINT("%s", tmpBuf);
			i = 0;
		} else {
			i++;
		}

	}
	if( 9 != i )
	{
		OMCI_PRINT("%s", tmpBuf);
	}

	OMCI_PRINT("%d\n", pOmci->MeTypeTbl_size);
	OMCI_PRINT("MsgTypeTbl: 0x%02x", pOmci->MsgTypeTbl[0]);

	return GOS_OK;
}

GOS_ERROR_CODE OmciDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_OMCI_T *pOmci;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

	pOmci = (MIB_TABLE_OMCI_T*)pNewRow;

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Omci --> ADD");
    	break;
    case MIB_GET:
	{
		MIB_ATTR_INDEX	attrIndex;
		UINT8 *ptr = NULL;
		omci_meType_tbl_entry_t *pEntry;

    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Omci --> GET");

		attrIndex = MIB_TABLE_OMCI_METYPETBL_INDEX;
		if (MIB_IsInAttrSet(&attrSet, attrIndex))
		{
			OMCI_PRINT("OMCI Get ME Table");

			gOmciMulGetData[pri].attribute[attrIndex].attrSize = MIB_TABLE_METYPETBL_LEN * (pOmci->MeTypeTbl_size);
			gOmciMulGetData[pri].attribute[attrIndex].attrIndex = attrIndex;
			gOmciMulGetData[pri].attribute[attrIndex].doneSeqNum = 0;
			gOmciMulGetData[pri].attribute[attrIndex].maxSeqNum =
				(gOmciMulGetData[pri].attribute[attrIndex].attrSize + OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

			ptr  = gOmciMulGetData[pri].attribute[attrIndex].attrValue;

			LIST_FOREACH(pEntry, &pOmci->meType_head, entries)
			{
				memcpy(ptr, &pEntry->tableEntry, MIB_TABLE_METYPETBL_LEN);
				ptr += MIB_TABLE_METYPETBL_LEN;
			}
		}
    	break;
    }
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Omci --> DEL");
    	break;
    default:
    	return GOS_FAIL;
    	break;
    }

	OMCI_PRINT("OMCI END");
    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOmciTableInfo.Name = "Omci";
    gMibOmciTableInfo.ShortName = "OMCI";
    gMibOmciTableInfo.Desc = "OMCI";
    gMibOmciTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_OMCI);
    gMibOmciTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOmciTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibOmciTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibOmciTableInfo.pAttributes = &(gMibOmciAttrInfo[0]);

    gMibOmciTableInfo.attrNum = MIB_TABLE_OMCI_ATTR_NUM;
    gMibOmciTableInfo.entrySize = sizeof(MIB_TABLE_OMCI_T);
    gMibOmciTableInfo.pDefaultRow = &gMibOmciDefRow;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MeTypeTbl";
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MsgTypeTbl";

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ME type table";
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Message type table";

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibOmciAttrInfo[MIB_TABLE_OMCI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_METYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOmciAttrInfo[MIB_TABLE_OMCI_MSGTYPETBL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibOmciDefRow.EntityId = 0;
    memset(&gMibOmciDefRow.MeTypeTbl,0x0, MIB_TABLE_METYPETBL_LEN);
    memset(&gMibOmciDefRow.MsgTypeTbl,0x0, MIB_TABLE_MSGTYPETBL_LEN);
	LIST_INIT(&gMibOmciDefRow.meType_head);
	LIST_INIT(&gMibOmciDefRow.msgType_head);

    memset(&gMibOmciOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibOmciOper.meOperDrvCfg = OmciDrvCfg;
    gMibOmciOper.meOperConnCheck = NULL;
    gMibOmciOper.meOperDump = OmciDumpMib;
    gMibOmciOper.meOperConnCfg = NULL;
    gMibOmciOper.meOperAvlTreeAdd = NULL;
    gMibOmciOper.meOperAlarmHandler = NULL;
    gMibOmciOper.meOperTestHandler = NULL;

    MIB_TABLE_OMCI_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibOmciTableInfo, &gMibOmciOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

