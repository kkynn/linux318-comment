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
 * Purpose : Definition of ME handler: ONU data (2)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ONU data (2)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibOntDataTableInfo;
MIB_ATTR_INFO_T  gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ATTR_NUM];
MIB_TABLE_ONTDATA_T gMibOntDataDefRow;
MIB_TABLE_OPER_T    gMibOntDataOper;


GOS_ERROR_CODE OntDataDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	switch(operationType)
	{
		case MIB_GET:
		{
			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX))
			{
#ifndef OMCI_X86
				if(gInfo.devIdVersion.chipId == APOLLOMP_CHIP_ID)
				{
					//omci_pon_bw_threshold_t ponBwThreshold;
					/*Enlarge PON threshold. */
					/*Becasue Huawei OLT set small bwmap to OMCC when ONU unauthenticated,
					  so set PON threshold small at begin, and enlarge threshold when ONU authenticated*/
					//ponBwThreshold.bwThreshold = 0x15;
					//ponBwThreshold.reqBwThreshold = 0x16;
					// user-space connection mgr will take care
					//omci_wrapper_setPonBwThreshold(&ponBwThreshold);
				}
#endif
			}
			break;
		}
		case MIB_SET:
			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX))
				feature_api(FEATURE_API_ME_00002000);
			break;
		default:
			break;
	}
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n",__FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOntDataTableInfo.Name = "OntData";
    gMibOntDataTableInfo.ShortName = "ONTDATA";
    gMibOntDataTableInfo.Desc = "Ont Data";
    gMibOntDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ONU_DATA);
    gMibOntDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOntDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibOntDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET |
        OMCI_ME_ACTION_MIB_UPLOAD | OMCI_ME_ACTION_MIB_UPLOAD_NEXT | OMCI_ME_ACTION_MIB_RESET |
        OMCI_ME_ACTION_GET_ALL_ALARMS | OMCI_ME_ACTION_GET_ALL_ALARMS_NEXT);
    gMibOntDataTableInfo.pAttributes = &(gMibOntDataAttrInfo[0]);
	gMibOntDataTableInfo.attrNum = MIB_TABLE_ONTDATA_ATTR_NUM;
	gMibOntDataTableInfo.entrySize = sizeof(MIB_TABLE_ONTDATA_T);
	gMibOntDataTableInfo.pDefaultRow = &gMibOntDataDefRow;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MIBDataSync";

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "MIB data sync";

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntDataAttrInfo[MIB_TABLE_ONTDATA_MIBDATASYNC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibOntDataDefRow.EntityID), 0x00, sizeof(gMibOntDataDefRow.EntityID));
    memset(&(gMibOntDataDefRow.MIBDataSync), 0x00, sizeof(gMibOntDataDefRow.MIBDataSync));

    memset(&gMibOntDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibOntDataOper.meOperDrvCfg = OntDataDrvCfg;
	gMibOntDataOper.meOperConnCheck = NULL;
	gMibOntDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibOntDataOper.meOperConnCfg = NULL;

	MIB_TABLE_ONTDATA_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibOntDataTableInfo,&gMibOntDataOper);

    return GOS_OK;
}
