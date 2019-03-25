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
 * Purpose : Definition of ME handler: OLG-G (131)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: OLG-G (131)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#include "rtk/time.h"
#endif

MIB_TABLE_INFO_T gMibOltGTableInfo;
MIB_ATTR_INFO_T  gMibOltGAttrInfo[MIB_TABLE_OLTG_ATTR_NUM];
MIB_TABLE_OLTG_T gMibOltGDefRow;
MIB_TABLE_OPER_T gMibOltGOper;


GOS_ERROR_CODE OltGDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_OLTG_T *pOltG = (MIB_TABLE_OLTG_T*)pData;
    oltg_attr_tod_info_t *ptTodInfo = (oltg_attr_tod_info_t *)pOltG->ToDInfo;

	OMCI_PRINT("EntityId: 0x%02x", pOltG->EntityId);
	OMCI_PRINT("OltVendorId: 0x%02x", pOltG->OltVendorId);
	OMCI_PRINT("EquipId: %s", pOltG->EquipId);
	OMCI_PRINT("Version: %s", pOltG->Version);
	OMCI_PRINT("ToDInfo:");
    OMCI_PRINT("\tSequence number of GEM superframe: 0x%x", ptTodInfo->uSeqNumOfGemSuperframe);
    OMCI_PRINT("\tTimestamp: secs %llu, nanosecs %u",
        (unsigned long long)ptTodInfo->uTimeStampSecs, ptTodInfo->uTimeStampNanosecs);

	return GOS_OK;
}

GOS_ERROR_CODE
OltGDrvCfg(
	void* pOldRow,
	void* pNewRow,
	MIB_OPERA_TYPE operationType,
	MIB_ATTRS_SET attrSet,
	UINT32 pri)
{
	GOS_ERROR_CODE		ret = GOS_OK;
	MIB_TABLE_OLTG_T*	pOltg;

	pOltg = (MIB_TABLE_OLTG_T*)pNewRow;

	if (operationType == MIB_SET &&
			MIB_IsInAttrSet(&attrSet, MIB_TABLE_OLTG_TODINFO_INDEX)) {
		ret = omci_wrapper_setTodInfo((omci_tod_info_t*)pOltg->ToDInfo);
		if (GOS_OK != ret) {
			return (GOS_FAIL);
		}
	}

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s: process end\n", __FUNCTION__);
	return (ret);
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOltGTableInfo.Name = "OltG";
    gMibOltGTableInfo.ShortName = "OLTG";
    gMibOltGTableInfo.Desc = "OLT-G";
    gMibOltGTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_OLT_G);
    gMibOltGTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOltGTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibOltGTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibOltGTableInfo.pAttributes = &(gMibOltGAttrInfo[0]);
	gMibOltGTableInfo.attrNum = MIB_TABLE_OLTG_ATTR_NUM;
	gMibOltGTableInfo.entrySize = sizeof(MIB_TABLE_OLTG_T);
	gMibOltGTableInfo.pDefaultRow = &gMibOltGDefRow;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OltVendorId";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EquipId";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Version";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ToDInfo";

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "OLT vendor Id";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "equipment id";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "version of the OLT";
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Time of day information";

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 20;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 14;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].Len = 14;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibOltGAttrInfo[MIB_TABLE_OLTG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_OLTVENDORID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_EQUIPID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOltGAttrInfo[MIB_TABLE_OLTG_TODINFO_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibOltGDefRow.EntityId), 0x00, sizeof(gMibOltGDefRow.EntityId));
    memset(&(gMibOltGDefRow.OltVendorId), 0x00, sizeof(gMibOltGDefRow.OltVendorId));
    memset(gMibOltGDefRow.EquipId, 0x20, MIB_TABLE_EQUIPID_LEN);
    gMibOltGDefRow.EquipId[MIB_TABLE_EQUIPID_LEN] = '\0';
    snprintf(gMibOltGDefRow.Version, MIB_TABLE_VERSION_LEN, "0");
    gMibOltGDefRow.Version[MIB_TABLE_VERSION_LEN] = '\0';
    memset(gMibOltGDefRow.ToDInfo, 0, MIB_TABLE_TODINFO_LEN);

    memset(&gMibOltGOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibOltGOper.meOperDrvCfg = OltGDrvCfg;
    gMibOltGOper.meOperConnCheck = NULL;
    gMibOltGOper.meOperDump = OltGDumpMib;
	gMibOltGOper.meOperConnCfg =  NULL;

	MIB_TABLE_OLTG_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibOltGTableInfo, &gMibOltGOper);

    return GOS_OK;
}
