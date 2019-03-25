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
 * Purpose : Definition of ME handler: PPTP Ethernet UNI (11)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP Ethernet UNI (11)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#include "rtk/port.h"
#endif

MIB_TABLE_INFO_T gMibLoopDetectTableInfo;
MIB_ATTR_INFO_T  gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ATTR_NUM];
MIB_TABLE_LOOP_DETECT_T gMibLoopDetectDefRow;
MIB_TABLE_OPER_T gMibLoopDetectOper;


GOS_ERROR_CODE LoopDetectDrvCfg(
                            void            *pOldRow,
                            void            *pNewRow,
                            MIB_OPERA_TYPE  operationType,
                            MIB_ATTRS_SET   attrSet,
                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_LOOP_DETECT_INDEX;
    omci_me_instance_t  instanceID;
    UINT16              portId;
    CHAR                rldp_ctrl[36];
    UINT8               state;


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
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Loop Detect Configure ---- > ADD");

			ret = mib_alarm_table_add(tableIndex, pNewRow);

            break;

		case MIB_DEL:
			OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Loop Detect Configure ---- > DEL");

            sprintf(rldp_ctrl, "echo 0 > /proc/rldp_drv/rldp_enable");
            system (rldp_ctrl);

            ret = mib_alarm_table_del(tableIndex, pOldRow);

            break;

        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_LOOP_DETECT_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_LOOP_DETECT_ARC_INDEX, MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX);
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX))
            {
                if (((MIB_TABLE_LOOP_DETECT_T *)pNewRow)->EthLoopDetectCfg !=
                        ((MIB_TABLE_LOOP_DETECT_T *)pOldRow)->EthLoopDetectCfg)
                {
                    state = (((MIB_TABLE_LOOP_DETECT_T *)pNewRow)->EthLoopDetectCfg == 1 ? TRUE : FALSE);
                    sprintf(rldp_ctrl, "echo %u > /proc/rldp_drv/rldp_enable", state);
                    system (rldp_ctrl);
                }
            }

            break;

		default:
            break;
	}

	return ret;
}

static GOS_ERROR_CODE loop_detect_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;

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

		// check if notifications are suppressed by parent's admin state
	    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
    }

    return GOS_OK;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibLoopDetectTableInfo.Name = "LoopDetect";
    gMibLoopDetectTableInfo.ShortName = "LD";
    gMibLoopDetectTableInfo.Desc = "Loop Detect";
    gMibLoopDetectTableInfo.ClassId = OMCI_ME_CLASS_LOOP_DETECT;
    gMibLoopDetectTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibLoopDetectTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibLoopDetectTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibLoopDetectTableInfo.pAttributes = &(gMibLoopDetectAttrInfo[0]);


	gMibLoopDetectTableInfo.attrNum = MIB_TABLE_LOOP_DETECT_ATTR_NUM;
	gMibLoopDetectTableInfo.entrySize = sizeof(MIB_TABLE_LOOP_DETECT_T);
	gMibLoopDetectTableInfo.pDefaultRow = &gMibLoopDetectDefRow;


    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AdminState";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARC";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARCInterval";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EthLoopDetectCfg";

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Administrative state";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Alarm Report Control";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC Interval";
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ETH Loop Detect cfg";

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibLoopDetectAttrInfo[MIB_TABLE_LOOP_DETECT_ETHLOOPDETECTCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibLoopDetectDefRow.EntityID), 0x00, sizeof(gMibLoopDetectDefRow.EntityID));
    gMibLoopDetectDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;
    memset(&(gMibLoopDetectDefRow.ARC), 0x00, sizeof(gMibLoopDetectDefRow.ARC));
    gMibLoopDetectDefRow.ARCInterval = 0;
    gMibLoopDetectDefRow.EthLoopDetectCfg = FALSE;

    memset(&gMibLoopDetectOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibLoopDetectOper.meOperDrvCfg = LoopDetectDrvCfg;
	gMibLoopDetectOper.meOperConnCheck = NULL;
	gMibLoopDetectOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibLoopDetectOper.meOperConnCfg = NULL;
    gMibLoopDetectOper.meOperAlarmHandler = loop_detect_alarm_handler;

	MIB_TABLE_LOOP_DETECT_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibLoopDetectTableInfo, &gMibLoopDetectOper);

    return GOS_OK;
}
