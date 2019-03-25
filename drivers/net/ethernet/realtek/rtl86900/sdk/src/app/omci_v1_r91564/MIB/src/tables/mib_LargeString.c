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
 * Purpose : Definition of ME handler: Large string (157)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Large string (157)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T 			gMibLsTblInfo;
MIB_ATTR_INFO_T  			gMibLsAttrInfo[MIB_TABLE_LARGE_STRING_ATTR_NUM];
MIB_TABLE_LARGE_STRING_T    gMibLsDefRow;
MIB_TABLE_OPER_T 			gMibLsOper;


GOS_ERROR_CODE large_string_drv_cfg_handler(void            *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
	return GOS_OK;
}

static GOS_ERROR_CODE large_string_avc_cb(MIB_TABLE_INDEX   tableIndex,
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
    else if (MIB_DEL == operationType)
    {
        if (!pOldRow)
            return GOS_ERR_PARAM;
    }
    else if (MIB_ADD == operationType)
    {
        if (!pNewRow)
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

            MIB_GetAttrSize(tableIndex, attrIndex);
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;
    MIB_ATTR_INDEX  attrIndexPart1;

    gMibLsTblInfo.Name = "LargeString";
    gMibLsTblInfo.ShortName = "LS";
    gMibLsTblInfo.Desc = "Large string";
    gMibLsTblInfo.ClassId = OMCI_ME_CLASS_LARGE_STRING;
    gMibLsTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibLsTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibLsTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibLsTblInfo.pAttributes = &(gMibLsAttrInfo[0]);
	gMibLsTblInfo.attrNum = MIB_TABLE_LARGE_STRING_ATTR_NUM;
	gMibLsTblInfo.entrySize = sizeof(MIB_TABLE_LARGE_STRING_T);
	gMibLsTblInfo.pDefaultRow = &gMibLsDefRow;

    attrIndex = MIB_TABLE_LARGE_STRING_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibLsAttrInfo[attrIndex].Name = "EntityId";
    gMibLsAttrInfo[attrIndex].Desc = "Entity ID";
    gMibLsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibLsAttrInfo[attrIndex].Len = 2;
    gMibLsAttrInfo[attrIndex].IsIndex = TRUE;
    gMibLsAttrInfo[attrIndex].MibSave = TRUE;
    gMibLsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibLsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibLsAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibLsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_LARGE_STRING_NUM_OF_PARTS_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibLsAttrInfo[attrIndex].Name = "NumOfParts";
    gMibLsAttrInfo[attrIndex].Desc = "Number of parts";
    gMibLsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibLsAttrInfo[attrIndex].Len = 1;
    gMibLsAttrInfo[attrIndex].IsIndex = FALSE;
    gMibLsAttrInfo[attrIndex].MibSave = TRUE;
    gMibLsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibLsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibLsAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibLsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndexPart1 = MIB_TABLE_LARGE_STRING_PART_1_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibLsAttrInfo[attrIndexPart1].Name = "Part1";
	gMibLsAttrInfo[attrIndexPart1].Desc = "Part 1";
    gMibLsAttrInfo[attrIndexPart1].DataType = MIB_ATTR_TYPE_STR;
    gMibLsAttrInfo[attrIndexPart1].Len = 25;
    gMibLsAttrInfo[attrIndexPart1].IsIndex = FALSE;
    gMibLsAttrInfo[attrIndexPart1].MibSave = TRUE;
    gMibLsAttrInfo[attrIndexPart1].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibLsAttrInfo[attrIndexPart1].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibLsAttrInfo[attrIndexPart1].AvcFlag = TRUE;
    gMibLsAttrInfo[attrIndexPart1].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_LARGE_STRING_PART_2_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part2";
    gMibLsAttrInfo[attrIndex].Desc = "Part 2";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_3_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part3";
    gMibLsAttrInfo[attrIndex].Desc = "Part 3";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_4_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part4";
    gMibLsAttrInfo[attrIndex].Desc = "Part 4";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_5_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part5";
    gMibLsAttrInfo[attrIndex].Desc = "Part 5";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_6_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part6";
    gMibLsAttrInfo[attrIndex].Desc = "Part 6";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_7_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part7";
    gMibLsAttrInfo[attrIndex].Desc = "Part 7";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_8_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part8";
    gMibLsAttrInfo[attrIndex].Desc = "Part 8";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_9_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part9";
    gMibLsAttrInfo[attrIndex].Desc = "Part 9";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_10_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part10";
    gMibLsAttrInfo[attrIndex].Desc = "Part 10";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_11_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part11";
    gMibLsAttrInfo[attrIndex].Desc = "Part 11";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_12_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part12";
    gMibLsAttrInfo[attrIndex].Desc = "Part 12";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_13_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part13";
    gMibLsAttrInfo[attrIndex].Desc = "Part 13";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_14_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part14";
    gMibLsAttrInfo[attrIndex].Desc = "Part 14";

    attrIndex = MIB_TABLE_LARGE_STRING_PART_15_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibLsAttrInfo[attrIndex], &gMibLsAttrInfo[attrIndexPart1], sizeof(MIB_ATTR_INFO_T));
    gMibLsAttrInfo[attrIndex].Name = "Part15";
    gMibLsAttrInfo[attrIndex].Desc = "Part 15";

    memset(&gMibLsDefRow, 0x00, sizeof(gMibLsDefRow));
	strcpy(gMibLsDefRow.Part1, "");
	strcpy(gMibLsDefRow.Part2, "");
	strcpy(gMibLsDefRow.Part3, "");
	strcpy(gMibLsDefRow.Part4, "");
	strcpy(gMibLsDefRow.Part5, "");
	strcpy(gMibLsDefRow.Part6, "");
	strcpy(gMibLsDefRow.Part7, "");
	strcpy(gMibLsDefRow.Part8, "");
	strcpy(gMibLsDefRow.Part9, "");
	strcpy(gMibLsDefRow.Part10, "");
	strcpy(gMibLsDefRow.Part11, "");
	strcpy(gMibLsDefRow.Part12, "");
	strcpy(gMibLsDefRow.Part13, "");
	strcpy(gMibLsDefRow.Part14, "");
	strcpy(gMibLsDefRow.Part15, "");

    memset(&gMibLsOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibLsOper.meOperDrvCfg = large_string_drv_cfg_handler;
    gMibLsOper.meOperConnCfg = NULL;
	gMibLsOper.meOperConnCheck = NULL;
	gMibLsOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_LARGE_STRING_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibLsTblInfo, &gMibLsOper);
    MIB_RegisterCallback(tableId, NULL, large_string_avc_cb);

    return GOS_OK;
}
