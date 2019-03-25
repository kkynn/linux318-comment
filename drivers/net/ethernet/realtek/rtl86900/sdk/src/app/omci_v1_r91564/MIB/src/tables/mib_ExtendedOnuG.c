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
 * Purpose : Definition of ME handler: Extended ONU-G (65408)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Extended ONU-G (65408)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T            gMibExtOnugTblInfo;
MIB_ATTR_INFO_T             gMibExtOnugAttrInfo[MIB_TABLE_EXTENDED_ONU_G_ATTR_NUM];
MIB_TABLE_EXTENDED_ONU_G_T  gMibExtOnugDefRow;
MIB_TABLE_OPER_T            gMibExtOnugOper;


GOS_ERROR_CODE extended_onu_g_drv_cfg_handler(void              *pOldRow,
                                                void            *pNewRow,
                                                MIB_OPERA_TYPE  operationType,
                                                MIB_ATTRS_SET   attrSet,
                                                UINT32          pri)
{
    feature_api(FEATURE_API_ME_00000400_DRV_CFG, operationType, attrSet);

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;

    gMibExtOnugTblInfo.Name = "ExtendedOnuG";
    gMibExtOnugTblInfo.ShortName = "EXTONUG";
    gMibExtOnugTblInfo.Desc = "Extended ONU-G";
    gMibExtOnugTblInfo.ClassId = OMCI_ME_CLASS_CTC_EXTENDED_ONU_G;
    gMibExtOnugTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibExtOnugTblInfo.StdType = OMCI_ME_TYPE_PROPRIETARY;
    gMibExtOnugTblInfo.ActionType = OMCI_ME_ACTION_SET;
    gMibExtOnugTblInfo.pAttributes = &(gMibExtOnugAttrInfo[0]);
	gMibExtOnugTblInfo.attrNum = MIB_TABLE_EXTENDED_ONU_G_ATTR_NUM;
	gMibExtOnugTblInfo.entrySize = sizeof(MIB_TABLE_EXTENDED_ONU_G_T);
	gMibExtOnugTblInfo.pDefaultRow = &gMibExtOnugDefRow;

    attrIndex = MIB_TABLE_EXTENDED_ONU_G_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibExtOnugAttrInfo[attrIndex].Name = "EntityId";
    gMibExtOnugAttrInfo[attrIndex].Desc = "Entity ID";
    gMibExtOnugAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtOnugAttrInfo[attrIndex].Len = 2;
    gMibExtOnugAttrInfo[attrIndex].IsIndex = TRUE;
    gMibExtOnugAttrInfo[attrIndex].MibSave = TRUE;
    gMibExtOnugAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtOnugAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtOnugAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibExtOnugAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_EXTENDED_ONU_G_RESET_DEFAULT_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibExtOnugAttrInfo[attrIndex].Name = "ResetDefault";
    gMibExtOnugAttrInfo[attrIndex].Desc = "Reset ONU to factory default config";
    gMibExtOnugAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtOnugAttrInfo[attrIndex].Len = 1;
    gMibExtOnugAttrInfo[attrIndex].IsIndex = FALSE;
    gMibExtOnugAttrInfo[attrIndex].MibSave = TRUE;
    gMibExtOnugAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtOnugAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtOnugAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibExtOnugAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibExtOnugDefRow, 0x00, sizeof(gMibExtOnugDefRow));

    memset(&gMibExtOnugOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibExtOnugOper.meOperDrvCfg = extended_onu_g_drv_cfg_handler;
	gMibExtOnugOper.meOperConnCfg = NULL;
    gMibExtOnugOper.meOperConnCheck = NULL;
    gMibExtOnugOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_EXTENDED_ONU_G_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibExtOnugTblInfo, &gMibExtOnugOper);

    return GOS_OK;
}
