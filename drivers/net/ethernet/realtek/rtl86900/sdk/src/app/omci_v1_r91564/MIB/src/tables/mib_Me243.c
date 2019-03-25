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
 * Purpose : Definition of ME handler: uni schedule configuration (243)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: uni schedule configuration (243)
 */

#include "app_basic.h"
#include "feature_mgmt.h"

/* Table private pon queue number per tcont configuration attribute index */
#define MIB_TABLE_ME243_ATTR_NUM            (4)
#define MIB_TABLE_ME243_ENTITYID_INDEX      ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_ME243_QNUM_ATTR		    ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_ME243_QMAP_ATTR		    ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_ME243_QWEIGHT_ATTR		((MIB_ATTR_INDEX)4)

/* Table me 243 me stucture */
typedef struct
{
    UINT16  EntityID;
    UINT8   NumOfQueue;
    UINT64  queueMap;
    UINT64  queueWeight;
} __attribute__((aligned)) MIB_TABLE_ME243_T;

MIB_TABLE_INFO_T gMibMe243TableInfo;
MIB_ATTR_INFO_T gMibMe243AttrInfo[MIB_TABLE_ME243_ATTR_NUM];
MIB_TABLE_ME243_T gMibMe243DefRow;
MIB_TABLE_OPER_T gMibMe243Oper;

GOS_ERROR_CODE mibTable_init ( MIB_TABLE_INDEX tableId )
{
	if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_BDP_00000200))
    {
        return GOS_FAIL;
    }

    MIB_TABLE_ME243_INDEX = tableId;
    memset(&(gMibMe243Oper), 0x0, sizeof(MIB_TABLE_OPER_T));

    gMibMe243TableInfo.Name = "ZTE ME243";
    gMibMe243TableInfo.ShortName = "ME243";
    gMibMe243TableInfo.Desc = "zte proprietary ME 243";
    gMibMe243TableInfo.ClassId = (UINT32)(243);
    gMibMe243TableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibMe243TableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibMe243TableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMe243TableInfo.pAttributes = &(gMibMe243AttrInfo[0]);
    gMibMe243TableInfo.attrNum = MIB_TABLE_ME243_ATTR_NUM;
    gMibMe243TableInfo.entrySize = sizeof(MIB_TABLE_ME243_T);
    gMibMe243TableInfo.pDefaultRow = &gMibMe243DefRow;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name   = "EntityID";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].Name       = "NumOfQueue";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].Name      = "queueMap";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].Name       = "queueWeight";

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc   = "Entity ID";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].Desc       = "Number of queue";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].Desc      = "Queue Map";
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].Desc       = "Queue Weight";

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType   = MIB_ATTR_TYPE_UINT16;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].DataType       = MIB_ATTR_TYPE_UINT8;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].DataType      = MIB_ATTR_TYPE_UINT64;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].DataType       = MIB_ATTR_TYPE_UINT64;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len    = 2;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].Len        = 1;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].Len       = 8;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].Len        = 8;


    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex    = TRUE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex        = FALSE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex       = FALSE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex        = FALSE;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave    = TRUE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].MibSave        = TRUE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].MibSave       = TRUE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].MibSave        = TRUE;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle   = MIB_ATTR_OUT_HEX;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle       = MIB_ATTR_OUT_DEC;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle      = MIB_ATTR_OUT_HEX;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle       = MIB_ATTR_OUT_HEX;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc     = OMCI_ME_ATTR_ACCESS_READ;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc         = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc        = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc         = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag    = FALSE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag        = FALSE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag       = FALSE;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag        = FALSE;

    gMibMe243AttrInfo[MIB_TABLE_ME243_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QNUM_ATTR - MIB_TABLE_FIRST_INDEX].OptionType     = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QMAP_ATTR - MIB_TABLE_FIRST_INDEX].OptionType    = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe243AttrInfo[MIB_TABLE_ME243_QWEIGHT_ATTR - MIB_TABLE_FIRST_INDEX].OptionType     = OMCI_ME_ATTR_TYPE_MANDATORY;


    gMibMe243Oper.meOperDump = omci_mib_oper_dump_default_handler;



    MIB_InfoRegister(tableId, &gMibMe243TableInfo, &gMibMe243Oper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}
