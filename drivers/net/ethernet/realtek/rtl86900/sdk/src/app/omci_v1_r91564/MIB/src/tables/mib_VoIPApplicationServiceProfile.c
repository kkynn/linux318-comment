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

MIB_TABLE_INFO_T gMibVoIPApplicationServiceProfileTableInfo;
MIB_ATTR_INFO_T  gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ATTR_NUM];
MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_T gMibVoIPApplicationServiceProfileDefRow;
MIB_TABLE_OPER_T gMibVoIPApplicationServiceProfileOper;

GOS_ERROR_CODE VoIPApplicationServiceProfileDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	// VoIP application service profile 9.9.8
	//MIB_TABLE_INDEX     tableIndex = MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_INDEX;
	//MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_T   *pMibVoipApplicationServiceProfile;
	omci_voice_vendor_t                         voice_vendor;

	voice_vendor = (gInfo.voiceVendor ? VOICE_VENDOR_RTK : VOICE_VENDOR_NONE);

    // read out the instanceID
	//pMibVoipApplicationServiceProfile= (MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_T *)pNewRow;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPApplicationServiceProfile --> ADD");
    	break;
    case MIB_SET:
    case MIB_DEL:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "SIPUserData --> %s", (MIB_DEL == operationType ? (STR(MIB_DEL)) : (STR(MIB_SET))));
        omci_voice_vendor_service_cb(voice_vendor, NULL);
    	break;
    default:
    	break;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibVoIPApplicationServiceProfileTableInfo.Name = "VoIPApplicationServiceProfile";
    gMibVoIPApplicationServiceProfileTableInfo.ShortName = "VOIPASP";
    gMibVoIPApplicationServiceProfileTableInfo.Desc = "VoIP application service profile";
    gMibVoIPApplicationServiceProfileTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VOIP_APPLICATION_SRV_PROFILE);
    gMibVoIPApplicationServiceProfileTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibVoIPApplicationServiceProfileTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVoIPApplicationServiceProfileTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVoIPApplicationServiceProfileTableInfo.pAttributes = &(gMibVoIPApplicationServiceProfileAttrInfo[0]);

    gMibVoIPApplicationServiceProfileTableInfo.attrNum = MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ATTR_NUM;
    gMibVoIPApplicationServiceProfileTableInfo.entrySize = sizeof(MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_T);
    gMibVoIPApplicationServiceProfileTableInfo.pDefaultRow = &gMibVoIPApplicationServiceProfileDefRow;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CIDFeatures";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallWaitingFeatures";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallProgressOrTransferFeatures";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CallPresentationFeatures";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DirectConnectFeature";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DirectConnectURIPointer";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BridgedLineAgentURIPointer";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ConferenceFactoryURIPointer";

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "CID features";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call waiting features";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call progress or transfer features";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Call presentation features";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Direct connect feature";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Direct connect URI pointer";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bridged line agent URI pointer";
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Conference factory URI pointer";

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CIDFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLWAITINGFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPROGRESSORTRANSFERFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CALLPRESENTATIONFEATURES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTFEATURE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_DIRECTCONNECTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_BRIDGEDLINEAGENTURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPApplicationServiceProfileAttrInfo[MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_CONFERENCEFACTORYURIPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibVoIPApplicationServiceProfileDefRow.EntityId = 0;
    gMibVoIPApplicationServiceProfileDefRow.CIDFeatures = 0;
    gMibVoIPApplicationServiceProfileDefRow.CallWaitingFeatures = 0;
    gMibVoIPApplicationServiceProfileDefRow.CallProgressOrTransferFeatures = 0;
    gMibVoIPApplicationServiceProfileDefRow.CallPresentationFeatures = 0;
    gMibVoIPApplicationServiceProfileDefRow.DirectConnectFeature = 0;
    gMibVoIPApplicationServiceProfileDefRow.DirectConnectURIPointer = 0xffff;
    gMibVoIPApplicationServiceProfileDefRow.BridgedLineAgentURIPointer = 0xffff;
    gMibVoIPApplicationServiceProfileDefRow.ConferenceFactoryURIPointer = 0xffff;

    memset(&gMibVoIPApplicationServiceProfileOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVoIPApplicationServiceProfileOper.meOperDrvCfg = VoIPApplicationServiceProfileDrvCfg;
    gMibVoIPApplicationServiceProfileOper.meOperConnCheck = NULL;
    gMibVoIPApplicationServiceProfileOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibVoIPApplicationServiceProfileOper.meOperConnCfg = NULL;
    gMibVoIPApplicationServiceProfileOper.meOperAvlTreeAdd = NULL;
    gMibVoIPApplicationServiceProfileOper.meOperAlarmHandler = NULL;
    gMibVoIPApplicationServiceProfileOper.meOperTestHandler = NULL;

    MIB_TABLE_VOIPAPPLICATIONSERVICEPROFILE_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVoIPApplicationServiceProfileTableInfo, &gMibVoIPApplicationServiceProfileOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

