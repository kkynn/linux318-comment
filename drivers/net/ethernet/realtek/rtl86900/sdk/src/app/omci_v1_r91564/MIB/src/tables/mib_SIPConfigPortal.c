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

MIB_TABLE_INFO_T gMibSIPConfigPortalTableInfo;
MIB_ATTR_INFO_T  gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ATTR_NUM];
MIB_TABLE_SIPCONFIGPORTAL_T gMibSIPConfigPortalDefRow;
MIB_TABLE_OPER_T gMibSIPConfigPortalOper;

GOS_ERROR_CODE SIPConfigPortalDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPConfigPortal --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPConfigPortal --> SET");
    	break;
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPConfigPortal --> GET");
    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"SIPConfigPortal --> DEL");
    	break;
    default:
    	return GOS_FAIL;
    	break;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibSIPConfigPortalTableInfo.Name = "SIPConfigPortal";
    gMibSIPConfigPortalTableInfo.ShortName = "SIPCP";
    gMibSIPConfigPortalTableInfo.Desc = "SIP config portal";
    gMibSIPConfigPortalTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_SIP_CFG_PORTAL);
    gMibSIPConfigPortalTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibSIPConfigPortalTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibSIPConfigPortalTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibSIPConfigPortalTableInfo.pAttributes = &(gMibSIPConfigPortalAttrInfo[0]);

    gMibSIPConfigPortalTableInfo.attrNum = MIB_TABLE_SIPCONFIGPORTAL_ATTR_NUM;
    gMibSIPConfigPortalTableInfo.entrySize = sizeof(MIB_TABLE_SIPCONFIGPORTAL_T);
    gMibSIPConfigPortalTableInfo.pDefaultRow = &gMibSIPConfigPortalDefRow;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ConfigurationTextTable";

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Configuration text table";

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_CONFIGURATIONTEXTTABLE_LEN;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;

    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSIPConfigPortalAttrInfo[MIB_TABLE_SIPCONFIGPORTAL_CONFIGURATIONTEXTTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibSIPConfigPortalDefRow, 0x00, sizeof(MIB_TABLE_SIPCONFIGPORTAL_T));

    memset(&gMibSIPConfigPortalOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibSIPConfigPortalOper.meOperDrvCfg = SIPConfigPortalDrvCfg;
    gMibSIPConfigPortalOper.meOperDump = omci_mib_oper_dump_default_handler;

    MIB_TABLE_SIPCONFIGPORTAL_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibSIPConfigPortalTableInfo, &gMibSIPConfigPortalOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

