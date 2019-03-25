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
 * Purpose : Definition of ME handler: ONT-G (256)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ONT-G (256)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibOntgTableInfo;
MIB_ATTR_INFO_T  gMibOntgAttrInfo[MIB_TABLE_ONTG_ATTR_NUM];
MIB_TABLE_ONTG_T gMibOntgDefRow;
MIB_TABLE_OPER_T gMibOntgOper;


GOS_ERROR_CODE OntgDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_ONTG_T *pOntG = (MIB_TABLE_ONTG_T*)pData;

	OMCI_PRINT("EntityID: 0x%02x", pOntG->EntityID);
	OMCI_PRINT("VID: %s", pOntG->VID);
	OMCI_PRINT("Version: %s", pOntG->Version);
	OMCI_PRINT("SerialNum: %c%c%c%c%02hhx%02hhx%02hhx%02hhx",
		pOntG->SerialNum[0], pOntG->SerialNum[1], pOntG->SerialNum[2], pOntG->SerialNum[3],
		pOntG->SerialNum[4], pOntG->SerialNum[5], pOntG->SerialNum[6], pOntG->SerialNum[7]);
	OMCI_PRINT("TraffMgtOpt: %u", pOntG->TraffMgtOpt);
	OMCI_PRINT("AtmCCOpt: %u", pOntG->AtmCCOpt);
	OMCI_PRINT("BatteryBack: %u", pOntG->BatteryBack);
	OMCI_PRINT("AdminState: %u", pOntG->AdminState);
	OMCI_PRINT("OpState: %u", pOntG->OpState);
	OMCI_PRINT("OnuSurvivalTime: %u", pOntG->OnuSurvivalTime);
	OMCI_PRINT("LogicalOnuID: %s", pOntG->LogicalOnuID);
	OMCI_PRINT("LogicalPassword: %s", pOntG->LogicalPassword);
	OMCI_PRINT("CredentialsStatus: %u", pOntG->CredentialsStatus);
	OMCI_PRINT("ExtendedTcLayerOptions: 0x%x", pOntG->ExtendedTcLayerOptions);
	OMCI_PRINT("OntState: %u", pOntG->OntState);

	return GOS_OK;
}

GOS_ERROR_CODE OntgDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE		ret = GOS_OK;
    MIB_TABLE_INDEX		tableIndex = MIB_TABLE_ONTG_INDEX;
	MIB_TABLE_ONTG_T *pOntg, *pOldOntg;
	MIB_TABLE_ONTG_T mibOntg;

	pOntg = (MIB_TABLE_ONTG_T*)pNewRow;
	pOldOntg = (MIB_TABLE_ONTG_T*)pOldRow;
	mibOntg.EntityID = pOntg->EntityID;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
		case MIB_SET:
		{
			/*When ONU change LOID or Password, reset AuthStatus to init status*/
			if ((MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX) && memcmp(pOntg->LogicalOnuID, pOldOntg->LogicalOnuID, sizeof(pOldOntg->LogicalOnuID)))
				|| (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX) && memcmp(pOntg->LogicalPassword, pOldOntg->LogicalPassword, sizeof(pOldOntg->LogicalPassword))) )
			{
                if(GOS_OK == MIB_Get(tableIndex, &mibOntg, sizeof(mibOntg)))
                {
    				mibOntg.CredentialsStatus = PON_ONU_LOID_INITIAL_STATE;
    				MIB_Set(tableIndex, &mibOntg, sizeof(mibOntg));
                }
                else
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                        __FUNCTION__, MIB_GetTableName(tableIndex), mibOntg.EntityID);
                }
			}

			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX))
			{
				/*Store the AuthStatus to avoid remove by mib reset*/
				gInfo.loidCfg.loidAuthStatus = pOntg->CredentialsStatus;
				/*Store the Last AuthStatus set by OLT for user get auth result and not be set to 0(init) by non-O5*/
				gInfo.loidCfg.lastLoidAuthStatus = pOntg->CredentialsStatus;

				/*Store the Number of LOID Register*/
				gInfo.loidCfg.loidAuthNum++;
				/*Store the Number of LOID Register success*/
				if ( PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION == pOntg->CredentialsStatus)
				{
					gInfo.loidCfg.loidAuthSuccessNum++;
				}

				/*For LED*/
				omci_wrapper_setLoidAuthStatus(pOntg->CredentialsStatus);
			}

			break;
		}
		default:
        	ret = GOS_OK;
            break;
    }

	return ret;
}

static GOS_ERROR_CODE onug_alarm_handler(MIB_TABLE_INDEX		tableIndex,
											omci_alm_data_t		alarmData,
											omci_me_instance_t	*pInstanceID,
											BOOL				*pIsUpdated)
{
    mib_alarm_table_t	alarmTable;
    BOOL                isSuppressed;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    *pInstanceID = TXC_ONUG_INSTANCE_ID;

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

        if (OMCI_ALM_NBR_ONUG_POWERING_ALM == alarmData.almNumber)
        {
        	// invoke power shedding processor
        	omci_pwr_shedding_processor(alarmData.almStatus);
        }
	}

    return GOS_OK;
}

static GOS_ERROR_CODE onug_avc_cb(MIB_TABLE_INDEX	tableIndex,
									void			*pOldRow,
                                    void            *pNewRow,
                                    MIB_ATTRS_SET   *pAttrsSet,
                                    MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX		attrIndex;
    UINT32				i;
    MIB_ATTRS_SET		avcAttrSet;
    BOOL                isSuppressed;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
	{
		avcAttrSet = *pAttrsSet;

	    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
	            i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
	    {
	        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
	            continue;
	    }
	}

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOntgTableInfo.Name = "Ontg";
    gMibOntgTableInfo.ShortName = "ONTG";
    gMibOntgTableInfo.Desc = "Ont-g";
    gMibOntgTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ONU_G);
    gMibOntgTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOntgTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibOntgTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_REBOOT | OMCI_ME_ACTION_SYNCHRONIZE_TIME);
    gMibOntgTableInfo.pAttributes = &(gMibOntgAttrInfo[0]);
	gMibOntgTableInfo.attrNum = MIB_TABLE_ONTG_ATTR_NUM;
	gMibOntgTableInfo.entrySize = sizeof(MIB_TABLE_ONTG_T);
	gMibOntgTableInfo.pDefaultRow = &gMibOntgDefRow;

    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_ENTITYID_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].Name = "VID";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Vendor ID";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VID_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].Name = "Version";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Version";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].Len = 14;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_VERSION_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].Name = "SerialNum";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Serial Number";
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].Len = 8;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntgAttrInfo[MIB_TABLE_ONTG_SERIALNUM_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].Name = "TraffMgtOpt";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Traffic Management Option";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_TRAFFMGTOPT_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].Name = "AtmCCOpt";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "VP/VC Cross-connection Function Option";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ATMCCOPT_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].Name = "BatteryBack";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Battery Backup";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_BATTERYBACK_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Name = "AdminState";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Administrative State";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ADMINSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Name = "OpState";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Operational State";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_OPSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].Name = "OnuSurvivalTime";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "ONU survival time";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONU_SURVIVAL_TIME_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].Name = "LogicalOnuID";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Logical ONU ID";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].Len = 24;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_ONU_ID_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].Name = "LogicalPassword";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Logical password";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].Len = 12;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_LOGICAL_PASSWORD_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].Name = "CredentialsStatus";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Credentials status";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_CREDENTIALS_STATUS_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].Name = "ExtendedTcLayerOptions";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Extended TC-layer options";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].Len = 2;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_EXTENDED_TC_LAYER_OPTIONS_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;

	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Name = "OntState";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "ONT PON State";
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].Len = 1;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
	gMibOntgAttrInfo[MIB_TABLE_ONTG_ONTSTATE_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

	memset(&(gMibOntgDefRow.EntityID), 0x00, sizeof(gMibOntgDefRow.EntityID));
	memset(gMibOntgDefRow.VID, 0, MIB_TABLE_ONTG_VID_LEN);
	gMibOntgDefRow.VID[MIB_TABLE_ONTG_VID_LEN] = '\0';
	snprintf(gMibOntgDefRow.Version, MIB_TABLE_ONTG_VERSION_LEN, "0");
	gMibOntgDefRow.Version[MIB_TABLE_ONTG_VERSION_LEN] = '\0';
	memset(gMibOntgDefRow.SerialNum, 0, MIB_TABLE_ONTG_SERIALNUM_LEN);
	gMibOntgDefRow.SerialNum[MIB_TABLE_ONTG_SERIALNUM_LEN] = '\0';
	gMibOntgDefRow.TraffMgtOpt = ONUG_TM_OPTION_PRIORITY_CONTROLLED;
	memset(&(gMibOntgDefRow.AtmCCOpt), 0x00, sizeof(gMibOntgDefRow.AtmCCOpt));
	gMibOntgDefRow.BatteryBack = FALSE;
	gMibOntgDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;
    gMibOntgDefRow.OpState = OMCI_ME_ATTR_OP_STATE_ENABLED;
	gMibOntgDefRow.OnuSurvivalTime = 0;
	memset(gMibOntgDefRow.LogicalOnuID, 0, MIB_TABLE_ONTG_LOID_LEN);
	gMibOntgDefRow.LogicalOnuID[MIB_TABLE_ONTG_LOID_LEN] = '\0';
	memset(gMibOntgDefRow.LogicalPassword, 0, MIB_TABLE_ONTG_LP_LEN);
	gMibOntgDefRow.LogicalPassword[MIB_TABLE_ONTG_LP_LEN] = '\0';
	gMibOntgDefRow.CredentialsStatus = PON_ONU_LOID_INITIAL_STATE;
	gMibOntgDefRow.ExtendedTcLayerOptions = 0;
	gMibOntgDefRow.OntState = MIB_ATTR_DEF_SPACE;

    memset(&gMibOntgOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibOntgOper.meOperDrvCfg = OntgDrvCfg;
	gMibOntgOper.meOperConnCheck = NULL;
	gMibOntgOper.meOperDump = OntgDumpMib;
	gMibOntgOper.meOperConnCfg = NULL;
	gMibOntgOper.meOperAlarmHandler = onug_alarm_handler;

	MIB_TABLE_ONTG_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibOntgTableInfo,&gMibOntgOper);
	MIB_RegisterCallback(tableId, NULL, onug_avc_cb);

    return GOS_OK;
}
