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
 * Purpose : Definition of ME handler: ont system management (240)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ont system management (240)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibOntSystemMgmtTableInfo;
MIB_ATTR_INFO_T  gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ATTR_NUM];
MIB_TABLE_ONTSYSTEMMGMT_T gMibOntSystemMgmtDefRow;
MIB_TABLE_OPER_T gMibOntSystemMgmtOper;

GOS_ERROR_CODE OntSystemMgmtDrvCfg(void            *pOldRow,
                                   void            *pNewRow,
                                   MIB_OPERA_TYPE  operationType,
                                   MIB_ATTRS_SET   attrSet,
                                   UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_ONT_SYSTEM_MANAGEMENT;
    omci_me_instance_t  instanceID;
    UINT16              portId;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex,
        MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

    // translate me id to switch port id
    if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(instanceID, &portId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance ID translate error: %s, 0x%x",
            MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

	switch (operationType)
	{
		case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:

            //TBD

            break;

		default:
            break;
	}

	return ret;
}

static GOS_ERROR_CODE ont_system_mgmt_alarm_handler(MIB_TABLE_INDEX     tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    *pInstanceID = alarmData.almDetail;

    if (GOS_OK != mib_alarm_table_get(tableIndex, *pInstanceID, &alarmTable))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
            MIB_GetTableName(tableIndex), *pInstanceID);

        return GOS_FAIL;
    }

    // update alarm status if it has being changed
    mib_alarm_table_update(&alarmTable, &alarmData, pIsUpdated);

    if (*pIsUpdated)
    {
        if (GOS_OK != mib_alarm_table_set(tableIndex, *pInstanceID, &alarmTable))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set alarm table fail: %s, 0x%x",
                MIB_GetTableName(tableIndex), *pInstanceID);

            return GOS_FAIL;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOntSystemMgmtTableInfo.Name = "OntSystemMgmt";
    gMibOntSystemMgmtTableInfo.ShortName = "OntSysMgmt";
    gMibOntSystemMgmtTableInfo.Desc = "ONT system management";
    gMibOntSystemMgmtTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ONT_SYSTEM_MANAGEMENT);
    gMibOntSystemMgmtTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOntSystemMgmtTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibOntSystemMgmtTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibOntSystemMgmtTableInfo.pAttributes = &(gMibOntSystemMgmtAttrInfo[0]);

	gMibOntSystemMgmtTableInfo.attrNum = MIB_TABLE_ONT_SYSTEM_MGMT_ATTR_NUM;
	gMibOntSystemMgmtTableInfo.entrySize = sizeof(MIB_TABLE_ONTSYSTEMMGMT_T);
	gMibOntSystemMgmtTableInfo.pDefaultRow = &gMibOntSystemMgmtDefRow;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DisOpticalTransceiver";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PollingIntv";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PollingCntRogueOnt";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EnableAlarm";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AlarmingCount";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "WaitingTimeForNoResp";

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Disable optical transceiver";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Polling interval";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Polling count for rogue ont";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Enable alarm";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Alarming count";
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Waiting time for no response";

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_DISOPTICALTRANSCEIVER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_POLLINGCNTROGUEONT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ENABLEALARM_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_ALARMINGCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibOntSystemMgmtAttrInfo[MIB_TABLE_ONT_SYSTEM_MGMT_WAITINGTIMEFORNORESPONSE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibOntSystemMgmtDefRow), 0x00, sizeof(gMibOntSystemMgmtDefRow));
    gMibOntSystemMgmtDefRow.PollingInterval = 20; //unit : millisecond; 0 : polling is not used
    gMibOntSystemMgmtDefRow.PollingCntRogueONT = 150; // TBD
    gMibOntSystemMgmtDefRow.EnableAlarm = ONT_SYSTEM_MGMT_ALARM_ENABLED;
    gMibOntSystemMgmtDefRow.AlarmingCount = 1;
    gMibOntSystemMgmtDefRow.WaitingTimeForNoResponse = 1000; // TBD, //unit : millisecond;

    memset(&gMibOntSystemMgmtOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibOntSystemMgmtOper.meOperDrvCfg = OntSystemMgmtDrvCfg;
	gMibOntSystemMgmtOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibOntSystemMgmtOper.meOperAlarmHandler = ont_system_mgmt_alarm_handler;

	MIB_TABLE_ONT_SYSTEM_MANAGEMENT = tableId;
	MIB_InfoRegister(tableId,&gMibOntSystemMgmtTableInfo,&gMibOntSystemMgmtOper);

    return GOS_OK;
}
