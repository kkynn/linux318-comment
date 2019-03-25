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

MIB_TABLE_INFO_T gMibMGCConfigPortalTableInfo;
MIB_ATTR_INFO_T  gMibMGCConfigPortalAttrInfo[MIB_TABLE_MGCCONFIGPORTAL_ATTR_NUM];
MIB_TABLE_MGCCONFIGPORTAL_T gMibMGCConfigPortalDefRow;
MIB_TABLE_OPER_T gMibMGCConfigPortalOper;

GOS_ERROR_CODE MGCConfigPortalDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"MGCConfigPortal --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"MGCConfigPortal --> SET");
    	break;
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"MGCConfigPortal --> GET");
    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"MGCConfigPortal --> DEL");
    	break;
    default:
    	return GOS_FAIL;
    	break;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMGCConfigPortalTableInfo.Name = "MGCConfigPortal";
    gMibMGCConfigPortalTableInfo.ShortName = "MGCCP";
    gMibMGCConfigPortalTableInfo.Desc = "MGC config portal";
    gMibMGCConfigPortalTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MGC_CFG_PORTAL);
    gMibMGCConfigPortalTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibMGCConfigPortalTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibMGCConfigPortalTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibMGCConfigPortalTableInfo.pAttributes = &(gMibMGCConfigPortalAttrInfo[0]);

    gMibMGCConfigPortalTableInfo.attrNum = MIB_TABLE_MGCCONFIGPORTAL_ATTR_NUM;
    gMibMGCConfigPortalTableInfo.entrySize = sizeof(MIB_TABLE_MGCCONFIGPORTAL_T);
    gMibMGCConfigPortalTableInfo.pDefaultRow = &gMibMGCConfigPortalDefRow;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ConfigurationTextTable";

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Configuration text table";

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_MGCCONFIGPORTAL_CFGTBL_ENTRY_LEN;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;

    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMGCConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibMGCConfigPortalAttrInfo, 0x00, sizeof(MIB_TABLE_MGCCONFIGPORTAL_T));

    memset(&gMibMGCConfigPortalOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibMGCConfigPortalOper.meOperDrvCfg = MGCConfigPortalDrvCfg;
    gMibMGCConfigPortalOper.meOperDump = omci_mib_oper_dump_default_handler;

    MIB_TABLE_MGCCONFIGPORTAL_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibMGCConfigPortalTableInfo, &gMibMGCConfigPortalOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

