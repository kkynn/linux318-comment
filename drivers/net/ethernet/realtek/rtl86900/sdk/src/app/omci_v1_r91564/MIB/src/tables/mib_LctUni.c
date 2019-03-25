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
 * Purpose : Definition of ME handler: Physical path termination point LCT UNI (83)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Physical path termination point LCT UNI (83)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibLctUniTableInfo;
MIB_ATTR_INFO_T  gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ATTR_NUM];
MIB_TABLE_LCTUNI_T gMibLctUniDefRow;
MIB_TABLE_OPER_T gMibLctUniOper;

GOS_ERROR_CODE LctUniDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE		ret = GOS_OK;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d: operationType=%u. TBD...",
            __FUNCTION__, __LINE__, operationType);

	return ret;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibLctUniTableInfo.Name = "LctUni";
    gMibLctUniTableInfo.ShortName = "LCT";
    gMibLctUniTableInfo.Desc = "PPTP LCT UNI";
    gMibLctUniTableInfo.ClassId = (UINT32)OMCI_ME_CLASS_PPTP_LCT_UNI;
    gMibLctUniTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibLctUniTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibLctUniTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibLctUniTableInfo.pAttributes = &(gMibLctUniAttrInfo[0]);
	gMibLctUniTableInfo.attrNum = MIB_TABLE_LCTUNI_ATTR_NUM;
	gMibLctUniTableInfo.entrySize = sizeof(MIB_TABLE_LCTUNI_T);
	gMibLctUniTableInfo.pDefaultRow = &gMibLctUniDefRow;

    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;


	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Name = "AdminState";
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Administrative State";
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibLctUniAttrInfo[MIB_TABLE_LCTUNI_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	memset(&(gMibLctUniDefRow.EntityID), 0x00, sizeof(gMibLctUniDefRow.EntityID));
	gMibLctUniDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;

    memset(&gMibLctUniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibLctUniOper.meOperDrvCfg = LctUniDrvCfg;
	gMibLctUniOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_PPTP_LCT_UNI_INDEX = tableId;
	MIB_InfoRegister(MIB_TABLE_PPTP_LCT_UNI_INDEX,&gMibLctUniTableInfo,&gMibLctUniOper);

    return GOS_OK;
}
