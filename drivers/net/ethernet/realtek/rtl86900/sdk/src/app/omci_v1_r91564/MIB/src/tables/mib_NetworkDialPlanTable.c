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

MIB_TABLE_INFO_T gMibNetworkDialPlanTableTableInfo;
MIB_ATTR_INFO_T  gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ATTR_NUM];
MIB_TABLE_NETWORKDIALPLANTABLE_T gMibNetworkDialPlanTableDefRow;
MIB_TABLE_OPER_T gMibNetworkDialPlanTableOper;

static GOS_ERROR_CODE network_dial_plan_entry_oper(
                                                  MIB_TABLE_NETWORKDIALPLANTABLE_T *pNewDp,
                                                  MIB_TABLE_NETWORKDIALPLANTABLE_T *pOldDp)
{
	dpTableEntry_t *ptr = NULL, entry, *pNew = NULL;
	omci_dial_plan_row_entry_t *pRawEntry = NULL;

	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s() staring ............%d", __FUNCTION__, __LINE__);

	pRawEntry = &entry.tableEntry;
	memcpy(pRawEntry, pNewDp->DialPlanTable, sizeof(omci_dial_plan_row_entry_t));

	switch (pRawEntry->act)
	{
		case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_ENTRY_ACT_ADD:
			LIST_FOREACH(ptr, &pOldDp->head, entries)
			{
				if (pRawEntry->dpId == ptr->tableEntry.dpId)
				{
					memcpy(ptr, &entry, sizeof(dpTableEntry_t));
					return GOS_OK;

				}
			}

			/*not found, create new entry*/
			pNew = (dpTableEntry_t *)malloc(sizeof(dpTableEntry_t));
			OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (!pNew), GOS_FAIL);
			memcpy(pNew, &entry, sizeof(dpTableEntry_t));
			LIST_INSERT_HEAD(&pOldDp->head, pNew, entries);

			pOldDp->DialPlanNumber++;
			break;
		case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_ENTRY_ACT_DEL:
			LIST_FOREACH(ptr, &pOldDp->head, entries)
			{
				if (pRawEntry->dpId == ptr->tableEntry.dpId)
				{
                    LIST_REMOVE(ptr, entries);
                    free(ptr);
                    if (pOldDp->DialPlanNumber > 0)
                        pOldDp->DialPlanNumber--;
                    break;
				}
			}
			break;
		default:
			break;
	}
	return GOS_OK;

}

static void network_dial_plan_table_dump(MIB_TABLE_NETWORKDIALPLANTABLE_T *pMib)
{
	dpTableEntry_t              *pEntry = NULL;
	omci_dial_plan_row_entry_t  *pRowEntry = NULL;
    UINT8                       i;

	OMCI_PRINT("Network Dial Plan Table: (%u)", pMib->DialPlanNumber);
	LIST_FOREACH(pEntry, &pMib->head, entries)
	{
		pRowEntry = &pEntry->tableEntry;
		OMCI_PRINT("----------------------------------");
        OMCI_PRINT("dial_plan_table_id:\t\t\t%u", pRowEntry->dpId);
        for (i = 0; i < MIB_TABLE_DIALPLANTABLE_TOKEN_LEN; i++)
            printf("%c", pRowEntry->dpToken[i]);
        OMCI_PRINT("\n");
	}
	if (0 == pMib->DialPlanNumber)
	{
		OMCI_PRINT("----------------------------------");
		OMCI_PRINT("No entry");
	}
	return;
}


GOS_ERROR_CODE NetworkDialPlanTableDump(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_NETWORKDIALPLANTABLE_T *pMib = (MIB_TABLE_NETWORKDIALPLANTABLE_T*)pData;

	OMCI_PRINT("EntityId: 0x%02x", pMib->EntityId);
	OMCI_PRINT("DialPlanNumber: %u", pMib->DialPlanNumber);
	OMCI_PRINT("DialPlanTableMaxSize: %u", pMib->DialPlanTableMaxSize);
	OMCI_PRINT("CriticalDialTimeout: %u", pMib->CriticalDialTimeout);
	OMCI_PRINT("PartialDialTimeout: %u", pMib->PartialDialTimeout);
    switch (pMib->DialPlanFormat)
    {
        case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_NOT_DEFINED:
            OMCI_PRINT("DialPlanFormat: %s", STR(NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_NOT_DEFINED));
            break;
        case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_H248:
            OMCI_PRINT("DialPlanFormat: %s", STR(NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_H248));
            break;
        case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_NCS:
            OMCI_PRINT("DialPlanFormat: %s", STR(NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_NCS));
            break;
        case NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_VENDOR_SPECIFIC:
            OMCI_PRINT("DialPlanFormat: %s", STR(NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_VENDOR_SPECIFIC));
            break;
        default:
            OMCI_PRINT("DialPlanFormat: %s", "Unknown");
    }
	network_dial_plan_table_dump(pMib);

	return GOS_OK;
}

GOS_ERROR_CODE NetworkDialPlanTableDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    MIB_TABLE_NETWORKDIALPLANTABLE_T *pMibNew = NULL, mibNew, *pMibOld = NULL;

	pMibNew = (MIB_TABLE_NETWORKDIALPLANTABLE_T *) pNewRow;

	mibNew.EntityId = pMibNew->EntityId;

	OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR,
        (!mib_FindEntry(MIB_TABLE_NETWORKDIALPLANTABLE_INDEX, &mibNew, &pMibOld)), GOS_FAIL);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType)
    {
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"NetworkDialPlanTable --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"NetworkDialPlanTable --> SET");

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX))
            network_dial_plan_entry_oper(pMibNew, pMibOld);

    	break;
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"NetworkDialPlanTable --> GET");
    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"NetworkDialPlanTable --> DEL");
    	break;
    default:
    	return GOS_FAIL;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibNetworkDialPlanTableTableInfo.Name = "NetworkDialPlanTable";
    gMibNetworkDialPlanTableTableInfo.ShortName = "NDPTBL";
    gMibNetworkDialPlanTableTableInfo.Desc = "Network dial plan table";
    gMibNetworkDialPlanTableTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_NETWORK_DIAL_PLAN_TBL);
    gMibNetworkDialPlanTableTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibNetworkDialPlanTableTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibNetworkDialPlanTableTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibNetworkDialPlanTableTableInfo.pAttributes = &(gMibNetworkDialPlanTableAttrInfo[0]);

    gMibNetworkDialPlanTableTableInfo.attrNum = MIB_TABLE_NETWORKDIALPLANTABLE_ATTR_NUM;
    gMibNetworkDialPlanTableTableInfo.entrySize = sizeof(MIB_TABLE_NETWORKDIALPLANTABLE_T);
    gMibNetworkDialPlanTableTableInfo.pDefaultRow = &gMibNetworkDialPlanTableDefRow;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DialPlanNumber";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DialPlanTableMaxSize";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CriticalDialTimeout";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PartialDialTimeout";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DialPlanFormat";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DialPlanTable";

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dial plan number";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dial plan table max size";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Critical dial timeout";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Partial dial timeout";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dial plan format";
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Dial plan table";

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 30;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANNUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLEMAXSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_CRITICALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_PARTIALDIALTIMEOUT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANFORMAT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibNetworkDialPlanTableAttrInfo[MIB_TABLE_NETWORKDIALPLANTABLE_DIALPLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibNetworkDialPlanTableDefRow.EntityId = 0;
    gMibNetworkDialPlanTableDefRow.DialPlanNumber = 0;
    gMibNetworkDialPlanTableDefRow.DialPlanTableMaxSize = (1 << 8);   //it is defined by ONU
    gMibNetworkDialPlanTableDefRow.CriticalDialTimeout = 4000;
    gMibNetworkDialPlanTableDefRow.PartialDialTimeout = 16000;
    gMibNetworkDialPlanTableDefRow.DialPlanFormat = NETWORK_DIAL_PLAN_TBL_DIAL_PLAN_FMT_H248;
    memset(gMibNetworkDialPlanTableDefRow.DialPlanTable, 0x00, MIB_TABLE_DIALPLANTABLE_LEN);

    LIST_INIT(&gMibNetworkDialPlanTableDefRow.head);

    memset(&gMibNetworkDialPlanTableOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibNetworkDialPlanTableOper.meOperDrvCfg = NetworkDialPlanTableDrvCfg;
    gMibNetworkDialPlanTableOper.meOperConnCheck = NULL;
    gMibNetworkDialPlanTableOper.meOperDump = NetworkDialPlanTableDump;
    gMibNetworkDialPlanTableOper.meOperConnCfg = NULL;
    gMibNetworkDialPlanTableOper.meOperAvlTreeAdd = NULL;
    gMibNetworkDialPlanTableOper.meOperAlarmHandler = NULL;
    gMibNetworkDialPlanTableOper.meOperTestHandler = NULL;

    MIB_TABLE_NETWORKDIALPLANTABLE_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibNetworkDialPlanTableTableInfo, &gMibNetworkDialPlanTableOper);

    return GOS_OK;
}

