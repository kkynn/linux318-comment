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
 * Purpose : Definition of ME handler: Octet string (307)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Octet string (307)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T 			gMibOsTblInfo;
MIB_ATTR_INFO_T  			gMibOsAttrInfo[MIB_TABLE_OCTET_STRING_ATTR_NUM];
MIB_TABLE_OCTET_STRING_T    gMibOsDefRow;
MIB_TABLE_OPER_T 			gMibOsOper;


GOS_ERROR_CODE octet_string_dump_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_OCTET_STRING_T   *pMibOctetString;
    UINT32                      remainSize;
    UINT32                      i, j;

    pMibOctetString = (MIB_TABLE_OCTET_STRING_T *)pData;

	OMCI_PRINT("EntityId: 0x%04x", pMibOctetString->EntityId);
	OMCI_PRINT("Length: %hu", pMibOctetString->Length);
    remainSize = pMibOctetString->Length;
    for (i = 0; i < 15; i++)
    {
        printf("Part%d: 0x", i + 1);

        for (j = 0; j < MIB_TABLE_OCTET_STRING_PART_LEN; j++)
        {
            if (remainSize > 0)
            {
                printf("%02hhx",
                    *(pMibOctetString->Part1 + MIB_TABLE_OCTET_STRING_PART_LEN * i + j));

                remainSize--;
            }
            else
                break;
        }

        printf("\n");
    }

	return GOS_OK;
}

GOS_ERROR_CODE octet_string_drv_cfg_handler(void            *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;
    MIB_ATTR_INDEX  attrIndexPart2;

    gMibOsTblInfo.Name = "LargeString";
    gMibOsTblInfo.ShortName = "LS";
    gMibOsTblInfo.Desc = "Large string";
    gMibOsTblInfo.ClassId = OMCI_ME_CLASS_OCTET_STRING;
    gMibOsTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibOsTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibOsTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibOsTblInfo.pAttributes = &(gMibOsAttrInfo[0]);
	gMibOsTblInfo.attrNum = MIB_TABLE_OCTET_STRING_ATTR_NUM;
	gMibOsTblInfo.entrySize = sizeof(MIB_TABLE_OCTET_STRING_T);
	gMibOsTblInfo.pDefaultRow = &gMibOsDefRow;

    attrIndex = MIB_TABLE_OCTET_STRING_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOsAttrInfo[attrIndex].Name = "EntityId";
    gMibOsAttrInfo[attrIndex].Desc = "Entity ID";
    gMibOsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOsAttrInfo[attrIndex].Len = 2;
    gMibOsAttrInfo[attrIndex].IsIndex = TRUE;
    gMibOsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibOsAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_OCTET_STRING_LENGTH_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOsAttrInfo[attrIndex].Name = "Length";
    gMibOsAttrInfo[attrIndex].Desc = "Length";
    gMibOsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOsAttrInfo[attrIndex].Len = 2;
    gMibOsAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOsAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibOsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_OCTET_STRING_PART_1_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOsAttrInfo[attrIndex].Name = "Part1";
	gMibOsAttrInfo[attrIndex].Desc = "Part 1";
    gMibOsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibOsAttrInfo[attrIndex].Len = 25;
    gMibOsAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOsAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibOsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndexPart2 = MIB_TABLE_OCTET_STRING_PART_2_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndexPart2], &gMibOsAttrInfo[attrIndex], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndexPart2].Name = "Part2";
    gMibOsAttrInfo[attrIndexPart2].Desc = "Part 2";
    gMibOsAttrInfo[attrIndexPart2].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_OCTET_STRING_PART_3_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part3";
    gMibOsAttrInfo[attrIndex].Desc = "Part 3";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_4_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part4";
    gMibOsAttrInfo[attrIndex].Desc = "Part 4";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_5_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part5";
    gMibOsAttrInfo[attrIndex].Desc = "Part 5";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_6_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part6";
    gMibOsAttrInfo[attrIndex].Desc = "Part 6";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_7_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part7";
    gMibOsAttrInfo[attrIndex].Desc = "Part 7";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_8_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part8";
    gMibOsAttrInfo[attrIndex].Desc = "Part 8";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_9_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part9";
    gMibOsAttrInfo[attrIndex].Desc = "Part 9";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_10_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part10";
    gMibOsAttrInfo[attrIndex].Desc = "Part 10";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_11_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part11";
    gMibOsAttrInfo[attrIndex].Desc = "Part 11";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_12_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part12";
    gMibOsAttrInfo[attrIndex].Desc = "Part 12";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_13_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part13";
    gMibOsAttrInfo[attrIndex].Desc = "Part 13";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_14_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part14";
    gMibOsAttrInfo[attrIndex].Desc = "Part 14";

    attrIndex = MIB_TABLE_OCTET_STRING_PART_15_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOsAttrInfo[attrIndex], &gMibOsAttrInfo[attrIndexPart2], sizeof(MIB_ATTR_INFO_T));
    gMibOsAttrInfo[attrIndex].Name = "Part15";
    gMibOsAttrInfo[attrIndex].Desc = "Part 15";

    memset(&gMibOsDefRow, 0x00, sizeof(gMibOsDefRow));

    memset(&gMibOsOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibOsOper.meOperDrvCfg = octet_string_drv_cfg_handler;
    gMibOsOper.meOperConnCfg = NULL;
	gMibOsOper.meOperConnCheck = NULL;
	gMibOsOper.meOperDump = octet_string_dump_handler;

	MIB_TABLE_OCTET_STRING_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibOsTblInfo, &gMibOsOper);

    return GOS_OK;
}
