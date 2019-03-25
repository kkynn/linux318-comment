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
 * Purpose : Definition of ME handler: GAL Ethernet profile (272)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: GAL Ethernet profile (272)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibGalEthProfTableInfo;
MIB_ATTR_INFO_T  gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ATTR_NUM];
MIB_TABLE_GALETHPROF_T gMibGalEthProfDefRow;
MIB_TABLE_OPER_T gMibGalEthProfOper;


GOS_ERROR_CODE GalEthProfDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n", __FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibGalEthProfTableInfo.Name = "GalEthProf";
    gMibGalEthProfTableInfo.ShortName = "GALETHP";
    gMibGalEthProfTableInfo.Desc = "GAL Ethernet profile";
    gMibGalEthProfTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_GAL_ETHERNET_PROFILE);
    gMibGalEthProfTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibGalEthProfTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibGalEthProfTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibGalEthProfTableInfo.pAttributes = &(gMibGalEthProfAttrInfo[0]);

	gMibGalEthProfTableInfo.attrNum = MIB_TABLE_GALETHPROF_ATTR_NUM;
	gMibGalEthProfTableInfo.entrySize = sizeof(MIB_TABLE_GALETHPROF_T);
	gMibGalEthProfTableInfo.pDefaultRow = &gMibGalEthProfDefRow;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MaxGemPayloadSize";

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "maximun GEM payload size";

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibGalEthProfAttrInfo[MIB_TABLE_GALETHPROF_MAXGEMPAYLOADSIZE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibGalEthProfDefRow.EntityId), 0x00, sizeof(gMibGalEthProfDefRow.EntityId));
    memset(&(gMibGalEthProfDefRow.MaxGemPayloadSize), 0x00, sizeof(gMibGalEthProfDefRow.MaxGemPayloadSize));

    memset(&gMibGalEthProfOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibGalEthProfOper.meOperDrvCfg = GalEthProfDrvCfg;
    gMibGalEthProfOper.meOperConnCheck = NULL;
    gMibGalEthProfOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibGalEthProfOper.meOperConnCfg = NULL;

	MIB_TABLE_GALETHPROF_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibGalEthProfTableInfo, &gMibGalEthProfOper);

    return GOS_OK;
}
