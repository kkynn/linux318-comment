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
 * Purpose : Definition of ME handler: Authentication security method (148)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Authentication security method (148)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T                gMibAsmTblInfo;
MIB_ATTR_INFO_T                 gMibAsmAttrInfo[MIB_TABLE_AUTH_SEC_METHOD_ATTR_NUM];
MIB_TABLE_AUTH_SEC_METHOD_T     gMibAsmDefRow;
MIB_TABLE_OPER_T                gMibAsmOper;


GOS_ERROR_CODE auth_sec_method_drv_cfg_handler(void         *pOldRow,
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

    gMibAsmTblInfo.Name = "AuthSecMethod";
    gMibAsmTblInfo.ShortName = "ASM";
    gMibAsmTblInfo.Desc = "Authentication security method";
    gMibAsmTblInfo.ClassId = OMCI_ME_CLASS_AUTH_SECURITY_METHOD;
    gMibAsmTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibAsmTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibAsmTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibAsmTblInfo.pAttributes = &(gMibAsmAttrInfo[0]);
	gMibAsmTblInfo.attrNum = MIB_TABLE_AUTH_SEC_METHOD_ATTR_NUM;
	gMibAsmTblInfo.entrySize = sizeof(MIB_TABLE_AUTH_SEC_METHOD_T);
	gMibAsmTblInfo.pDefaultRow = &gMibAsmDefRow;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibAsmAttrInfo[attrIndex].Name = "EntityId";
    gMibAsmAttrInfo[attrIndex].Desc = "Entity ID";
    gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibAsmAttrInfo[attrIndex].Len = 2;
    gMibAsmAttrInfo[attrIndex].IsIndex = TRUE;
    gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
    gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_VALID_SCHEM_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibAsmAttrInfo[attrIndex].Name = "ValidSchem";
    gMibAsmAttrInfo[attrIndex].Desc = "Validation scheme";
    gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibAsmAttrInfo[attrIndex].Len = 1;
    gMibAsmAttrInfo[attrIndex].IsIndex = FALSE;
    gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
    gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_USERNAME1_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibAsmAttrInfo[attrIndex].Name = "Username1";
	gMibAsmAttrInfo[attrIndex].Desc = "Username 1";
    gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
    gMibAsmAttrInfo[attrIndex].Len = 25;
    gMibAsmAttrInfo[attrIndex].IsIndex = FALSE;
    gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
    gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibAsmAttrInfo[attrIndex].Name = "Password";
	gMibAsmAttrInfo[attrIndex].Desc = "Password";
    gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
    gMibAsmAttrInfo[attrIndex].Len = 25;
    gMibAsmAttrInfo[attrIndex].IsIndex = FALSE;
    gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
    gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_REALM_INDEX - MIB_TABLE_FIRST_INDEX;
	gMibAsmAttrInfo[attrIndex].Name = "Realm";
	gMibAsmAttrInfo[attrIndex].Desc = "Realm";
	gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
	gMibAsmAttrInfo[attrIndex].Len = 25;
	gMibAsmAttrInfo[attrIndex].IsIndex = FALSE;
	gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
	gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
	gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
	gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_AUTH_SEC_METHOD_USERNAME2_INDEX - MIB_TABLE_FIRST_INDEX;
	gMibAsmAttrInfo[attrIndex].Name = "Username2";
	gMibAsmAttrInfo[attrIndex].Desc = "Username 2";
	gMibAsmAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
	gMibAsmAttrInfo[attrIndex].Len = 25;
	gMibAsmAttrInfo[attrIndex].IsIndex = FALSE;
	gMibAsmAttrInfo[attrIndex].MibSave = TRUE;
	gMibAsmAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
	gMibAsmAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibAsmAttrInfo[attrIndex].AvcFlag = FALSE;
	gMibAsmAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	memset(&gMibAsmDefRow, 0x00, sizeof(gMibAsmDefRow));
	strcpy(gMibAsmDefRow.Username1, "");
    strcpy(gMibAsmDefRow.Password, "");
    strcpy(gMibAsmDefRow.Realm, "");
	strcpy(gMibAsmDefRow.Username2, "");

    memset(&gMibAsmOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibAsmOper.meOperDrvCfg = auth_sec_method_drv_cfg_handler;
    gMibAsmOper.meOperConnCfg = NULL;
	gMibAsmOper.meOperConnCheck = NULL;
	gMibAsmOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_AUTH_SEC_METHOD_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibAsmTblInfo, &gMibAsmOper);

    return GOS_OK;
}
