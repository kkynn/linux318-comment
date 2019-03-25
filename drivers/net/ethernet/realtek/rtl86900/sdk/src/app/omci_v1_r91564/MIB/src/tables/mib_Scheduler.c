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
 * Purpose : Definition of ME handler: Traffic scheduler (278)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Traffic scheduler (278)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibSchedulerTableInfo;
MIB_ATTR_INFO_T  gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ATTR_NUM];
MIB_TABLE_SCHEDULER_T gMibSchedulerDefRow;
MIB_TABLE_OPER_T	gMibSchedulerOper;


GOS_ERROR_CODE SchedulerDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n",__FUNCTION__);
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibSchedulerTableInfo.Name = "Scheduler";
    gMibSchedulerTableInfo.ShortName = "TSG";
    gMibSchedulerTableInfo.Desc = "Traffic Scheduler-G";
    gMibSchedulerTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_TRAFFIC_SCHEDULER);
    gMibSchedulerTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibSchedulerTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibSchedulerTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibSchedulerTableInfo.pAttributes = &(gMibSchedulerAttrInfo[0]);
	gMibSchedulerTableInfo.attrNum = MIB_TABLE_SCHEDULER_ATTR_NUM;
	gMibSchedulerTableInfo.entrySize = sizeof(MIB_TABLE_SCHEDULER_T);
	gMibSchedulerTableInfo.pDefaultRow = &gMibSchedulerDefRow;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TcontPtr";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SchedulerPtr";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Policy";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PriWeight";

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "T-CONT Pointer";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Traffic Scheduler Pointer";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Policy";
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Priority_Weight";

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_TCONTPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_SCHEDULERPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSchedulerAttrInfo[MIB_TABLE_SCHEDULER_PRIWEIGHT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibSchedulerDefRow.EntityID), 0x00, sizeof(gMibSchedulerDefRow.EntityID));
    memset(&(gMibSchedulerDefRow.TcontPtr), 0x00, sizeof(gMibSchedulerDefRow.TcontPtr));
    memset(&(gMibSchedulerDefRow.SchedulerPtr), 0x00, sizeof(gMibSchedulerDefRow.SchedulerPtr));
    memset(&(gMibSchedulerDefRow.Policy), 0x00, sizeof(gMibSchedulerDefRow.Policy));
    memset(&(gMibSchedulerDefRow.PriWeight), 0x00, sizeof(gMibSchedulerDefRow.PriWeight));

    memset(&gMibSchedulerOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibSchedulerOper.meOperDrvCfg = SchedulerDrvCfg;
	gMibSchedulerOper.meOperConnCheck = NULL;
	gMibSchedulerOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibSchedulerOper.meOperConnCfg = NULL;

	MIB_TABLE_SCHEDULER_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibSchedulerTableInfo,&gMibSchedulerOper);

    return GOS_OK;
}
