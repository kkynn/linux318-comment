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
 * Purpose : Definition of ME handler: VoIP config data (138)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: VoIP config data (138)
 */


#include "app_basic.h"

MIB_TABLE_INFO_T gMibVoIPConfigDataTableInfo;
MIB_ATTR_INFO_T  gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ATTR_NUM];
MIB_TABLE_VOIPCONFIGDATA_T gMibVoIPConfigDataDefRow;
MIB_TABLE_OPER_T gMibVoIPConfigDataOper;


GOS_ERROR_CODE VoIPConfigDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	GOS_ERROR_CODE              ret = GOS_OK;
	MIB_TABLE_INDEX             tableIndex = MIB_TABLE_VOIPCONFIGDATA_INDEX;
	MIB_TABLE_VOIPCONFIGDATA_T  *pMibVoipConfigData=NULL, *pOldVoipConfigData=NULL;

	pMibVoipConfigData = (MIB_TABLE_VOIPCONFIGDATA_T *)pNewRow;
	pOldVoipConfigData = (MIB_TABLE_VOIPCONFIGDATA_T *)pOldRow;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPConfigData --> ADD");
    	ret = mib_alarm_table_add(tableIndex, pNewRow);
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPConfigData --> SET");
    	if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX) &&
    		pMibVoipConfigData->VOIPConfigurationMethodUsed != pOldVoipConfigData->VOIPConfigurationMethodUsed)
    	{
    		OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPConfigData ConfigurationMethodUsed change old %d new %d", 
			pOldVoipConfigData->VOIPConfigurationMethodUsed, pMibVoipConfigData->VOIPConfigurationMethodUsed);
		VOICE_WRAPPER(omci_voice_config_method_set, pMibVoipConfigData->VOIPConfigurationMethodUsed);
    	}
    	break;
        #if 0
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPConfigData --> GET");

		MIB_Get(tableIndex, &mibVoipConfigData, sizeof(mibVoipConfigData));

		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX))
		{
			mibVoipConfigData.EntityId = pMibVoipConfigData->EntityId;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX))
		{
			mibVoipConfigData.AvailableSignallingProtocols = pMibVoipConfigData->AvailableSignallingProtocols;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX))
		{
			mibVoipConfigData.SignallingProtocolUsed = pMibVoipConfigData->SignallingProtocolUsed;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX))
		{
			mibVoipConfigData.AvailableVoIPConfigurationMethods = pMibVoipConfigData->AvailableVoIPConfigurationMethods;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX))
		{
			mibVoipConfigData.VOIPConfigurationMethodUsed = pMibVoipConfigData->VOIPConfigurationMethodUsed;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX))
		{
			mibVoipConfigData.VOIPConfigurationAddressPointer = pMibVoipConfigData->VOIPConfigurationAddressPointer;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX))
		{
			mibVoipConfigData.VOIPConfigurationState = pMibVoipConfigData->VOIPConfigurationState;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX))
		{
			mibVoipConfigData.RetrieveProfile = pMibVoipConfigData->RetrieveProfile;
		}
		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX))
		{
			strcpy(mibVoipConfigData.ProfileVersion , pMibVoipConfigData->ProfileVersion);
		}

		/*Update to MIB*/
		MIB_Set(tableIndex, &mibVoipConfigData, sizeof(mibVoipConfigData));

    	break;
        #endif
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPConfigData --> DEL");
    	ret = mib_alarm_table_del(tableIndex, pOldRow);
    	break;
    default:
    	return GOS_OK;
    }

    return ret;
}

static GOS_ERROR_CODE voip_cfg_data_alarm_handler(MIB_TABLE_INDEX       tableIndex,
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
    }

    return GOS_OK;
}

static GOS_ERROR_CODE voip_cfg_data_avc_cb(MIB_TABLE_INDEX  tableIndex,
                                            void            *pOldRow,
                                            void            *pNewRow,
                                            MIB_ATTRS_SET   *pAttrsSet,
                                            MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
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
    gMibVoIPConfigDataTableInfo.Name = "VoIPConfigData";
    gMibVoIPConfigDataTableInfo.ShortName = "VOIPCD";
    gMibVoIPConfigDataTableInfo.Desc = "VoIP config data";
    gMibVoIPConfigDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VOIP_CFG_DATA);
    gMibVoIPConfigDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibVoIPConfigDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVoIPConfigDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVoIPConfigDataTableInfo.pAttributes = &(gMibVoIPConfigDataAttrInfo[0]);

    gMibVoIPConfigDataTableInfo.attrNum = MIB_TABLE_VOIPCONFIGDATA_ATTR_NUM;
    gMibVoIPConfigDataTableInfo.entrySize = sizeof(MIB_TABLE_VOIPCONFIGDATA_T);
    gMibVoIPConfigDataTableInfo.pDefaultRow = &gMibVoIPConfigDataDefRow;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AvailableSignallingProtocols";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SignallingProtocolUsed";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AvailableVoIPConfigurationMethods";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VOIPConfigurationMethodUsed";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VOIPConfigurationAddressPointer";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VOIPConfigurationState";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RetrieveProfile";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ProfileVersion";

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Available signalling protocols";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Signalling protocol used";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Available VoIP configuration methods";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "VOIP configuration method used";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "VOIP configuration address pointer";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "VOIP configuration state";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Retrieve profile";
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Profile version";

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;

    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLESIGNALLINGPROTOCOLS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_SIGNALLINGPROTOCOLUSED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_AVAILABLEVOIPCONFIGURATIONMETHODS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONMETHODUSED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONADDRESSPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_VOIPCONFIGURATIONSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_RETRIEVEPROFILE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPConfigDataAttrInfo[MIB_TABLE_VOIPCONFIGDATA_PROFILEVERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibVoIPConfigDataDefRow.EntityId = 0;
    gMibVoIPConfigDataDefRow.AvailableSignallingProtocols |= VCD_AVAILABLE_SIG_PROTOCOL_SIP;  //SIP
    gMibVoIPConfigDataDefRow.SignallingProtocolUsed = VCD_SIG_PROTOCOL_USED_SIP; //SIP
    gMibVoIPConfigDataDefRow.AvailableVoIPConfigurationMethods |= (VCD_AVAILABLE_VOIP_CFG_METHOD_OMCI | VCD_AVAILABLE_VOIP_CFG_METHOD_TR069);
    gMibVoIPConfigDataDefRow.VOIPConfigurationMethodUsed = VCD_CFG_METHOD_USED_TR069; // RT069
    gMibVoIPConfigDataDefRow.VOIPConfigurationAddressPointer = 0xffff;
    gMibVoIPConfigDataDefRow.VOIPConfigurationState = 0;
    gMibVoIPConfigDataDefRow.RetrieveProfile = 0;
    strncpy(gMibVoIPConfigDataDefRow.ProfileVersion, "0", sizeof(gMibVoIPConfigDataDefRow.ProfileVersion));

    memset(&gMibVoIPConfigDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVoIPConfigDataOper.meOperDrvCfg = VoIPConfigDataDrvCfg;
    gMibVoIPConfigDataOper.meOperConnCheck = NULL;
    gMibVoIPConfigDataOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibVoIPConfigDataOper.meOperConnCfg = NULL;
    gMibVoIPConfigDataOper.meOperAvlTreeAdd = NULL;
    gMibVoIPConfigDataOper.meOperAlarmHandler = voip_cfg_data_alarm_handler;
    gMibVoIPConfigDataOper.meOperTestHandler = NULL;

    MIB_TABLE_VOIPCONFIGDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVoIPConfigDataTableInfo, &gMibVoIPConfigDataOper);
    MIB_RegisterCallback(tableId, NULL, voip_cfg_data_avc_cb);

    return GOS_OK;
}

