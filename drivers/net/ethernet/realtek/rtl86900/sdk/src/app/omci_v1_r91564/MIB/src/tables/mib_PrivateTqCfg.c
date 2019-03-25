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
 * Purpose : Definition of ME handler: Private Tcont Queue Configuration (65534)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Private Tcont Queue Configuration (65534)
 */

#include "app_basic.h"
#include "omci_task.h"


MIB_TABLE_INFO_T gMibPrivateTqCfgTableInfo;
MIB_ATTR_INFO_T  gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ATTR_NUM];
MIB_TABLE_PRIVATE_TQCFG_T gMibPrivateTqCfgDefRow;
MIB_TABLE_OPER_T gMibPrivateTqCfgOper;

GOS_ERROR_CODE PrivateTqCfgDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    UINT16 tcont_index;
    UINT32 slotNum = TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_PON_SLOT);

	MIB_TABLE_PRIVATE_TQCFG_T *pPrivateTqCfg = (MIB_TABLE_PRIVATE_TQCFG_T *)pData;

	OMCI_PRINT("EntityID: 0x%#x", pPrivateTqCfg->EntityID);
	OMCI_PRINT("Type: %u", pPrivateTqCfg->Type);
    OMCI_PRINT("PonSlotId: 0x%#x", pPrivateTqCfg->PonSlotId);

    OMCI_PRINT("\nQueueNumPerTcont:");
    for (tcont_index = 0; tcont_index < gInfo.devCapabilities.totalTContNum; tcont_index++)
    {
        /* tcont _index is the last byte */
        OMCI_PRINT("TcontID[%#x]: %u", (UINT32)((slotNum << 8) | tcont_index), pPrivateTqCfg->QueueNumPerTcont[tcont_index]);
    }

	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibPrivateTqCfgTableInfo.Name = "PrivateTqCfg";
    gMibPrivateTqCfgTableInfo.ShortName = "PTC";
    gMibPrivateTqCfgTableInfo.Desc = "Private Tcont Queue Configuration";
    gMibPrivateTqCfgTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PRIVATE_TQCFG);
    gMibPrivateTqCfgTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibPrivateTqCfgTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PRIVATE);
    gMibPrivateTqCfgTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_SET);
    gMibPrivateTqCfgTableInfo.pAttributes = &(gMibPrivateTqCfgAttrInfo[0]);
	gMibPrivateTqCfgTableInfo.attrNum = MIB_TABLE_PRIVATE_TQCFG_ATTR_NUM;
	gMibPrivateTqCfgTableInfo.entrySize = sizeof(MIB_TABLE_PRIVATE_TQCFG_T);
	gMibPrivateTqCfgTableInfo.pDefaultRow = &gMibPrivateTqCfgDefRow;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
	gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].Name = "Type";
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].Name = "PonSlotId";
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].Name = "QueueNumPerTcont";

	gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].Desc = "Type";
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].Desc = "PON Slot ID";
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].Desc = "Queue Number Per Tcont";

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_PON_QUEUE_MAX_NUM;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_TYPE-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_PON_SLOT_ID-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;
    gMibPrivateTqCfgAttrInfo[MIB_TABLE_PRIVATE_TQCFG_QUEUE_NUM_PER_TCONT-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    memset(&(gMibPrivateTqCfgDefRow), 0x0, sizeof(MIB_TABLE_PRIVATE_TQCFG_T));

    memset(&gMibPrivateTqCfgOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibPrivateTqCfgOper.meOperDrvCfg = NULL;
	gMibPrivateTqCfgOper.meOperConnCheck = NULL;
	gMibPrivateTqCfgOper.meOperDump = PrivateTqCfgDumpMib;
	gMibPrivateTqCfgOper.meOperConnCfg = NULL;
    gMibPrivateTqCfgOper.meOperAvlTreeAdd = NULL;
    gMibPrivateTqCfgOper.meOperPmHandler = NULL;
    gMibPrivateTqCfgOper.meOperAlarmHandler = NULL;
    gMibPrivateTqCfgOper.meOperTestHandler = NULL;

	MIB_TABLE_PRIVATE_TQCFG_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibPrivateTqCfgTableInfo, &gMibPrivateTqCfgOper);

    return GOS_OK;
}
