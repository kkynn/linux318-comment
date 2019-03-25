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

MIB_TABLE_INFO_T gMibVoIPFeatureAccessCodesTableInfo;
MIB_ATTR_INFO_T  gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTR_NUM];
MIB_TABLE_VOIPFEATUREACCESSCODES_T gMibVoIPFeatureAccessCodesDefRow;
MIB_TABLE_OPER_T gMibVoIPFeatureAccessCodesOper;

GOS_ERROR_CODE VoIPFeatureAccessCodesDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    // VoIP feature access codes 9.9.9
	//MIB_TABLE_INDEX     tableIndex = MIB_TABLE_VOIPFEATUREACCESSCODES_INDEX;
	//MIB_TABLE_VOIPFEATUREACCESSCODES_T *pMibVoipFeatureAccessCodes;
    // read out the instanceID
	//pMibVoipFeatureAccessCodes = (MIB_TABLE_VOIPFEATUREACCESSCODES_T*)pNewRow;
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);
    #if 0
	INT32 i;
	UINT32 chid;
	UINT32 proxy = 0;
	UINT32 size=0;

   	mibVoipFeatureAccessCodes.EntityId = pMibVoipFeatureAccessCodes->EntityId;
	const UINT32 entity_id = pMibVoipFeatureAccessCodes->EntityId;

	// SIP user data 9.9.2
	MIB_TABLE_SIPUSERDATA_T   tmpMibSipUserData, *pMibSipUserData;

	// find channel id
	size = sizeof(MIB_TABLE_SIPUSERDATA_T);

	if((MIB_GetFirst(MIB_TABLE_SIPUSERDATA_INDEX, &tmpMibSipUserData , size))==GOS_OK){
		if( tmpMibSipUserData.FeatureCodePointer == entity_id ){
			chid = tmpMibSipUserData.PPTPPointer;
		}
		while( (MIB_GetNext(MIB_TABLE_SIPUSERDATA_INDEX, &tmpMibSipUserData , size))==GOS_OK ){
			if( tmpMibSipUserData.FeatureCodePointer== entity_id ){
				chid = tmpMibSipUserData.PPTPPointer;
			}
		}
	}
	else{
		ODBG_R("find chid wrong\n");
	}

	// check channel id
	if( chid > gInfo.devCapabilities.potsPortNum ){
		ODBG_R("find chid wrong. chid = %u\n" , chid);
		return GOS_FAIL;
	}
	else{
		ODBG_Y("chid = %u\n" , chid);
	}

	voipCfgPortParam_t *pCfgPort = NULL;
	pCfgPort = &g_pShareCfgVoIP->ports[chid];	// assign local config pointer

    #endif
    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPFeatureAccessCodes --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPFeatureAccessCodes --> SET");
    	break;
        #if 0 // not need
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPFeatureAccessCodes --> GET");

		MIB_Get(tableIndex, &mibVoipFeatureAccessCodes, sizeof(mibVoipFeatureAccessCodes));

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX))
		{
			mibVoipFeatureAccessCodes.EntityId = pMibVoipFeatureAccessCodes->EntityId;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.CancelCallWaiting = pMibVoipFeatureAccessCodes->CancelCallWaiting;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.CancelHold = pMibVoipFeatureAccessCodes->CancelHold;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.CancelPark = pMibVoipFeatureAccessCodes->CancelPark;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.CancelIDActivate = pMibVoipFeatureAccessCodes->CancelIDActivate;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.CancelIDdeactivate = pMibVoipFeatureAccessCodes->CancelIDdeactivate;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.DoNotDisturbActivate = pMibVoipFeatureAccessCodes->DoNotDisturbActivate;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.DoNotDisturbDeactivate = pMibVoipFeatureAccessCodes->DoNotDisturbDeactivate;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.DoNotDisturbPINChange = pMibVoipFeatureAccessCodes->DoNotDisturbPINChange;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.EmergencyServiceNumber = pMibVoipFeatureAccessCodes->EmergencyServiceNumber;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.IntercomService = pMibVoipFeatureAccessCodes->IntercomService;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.UnattendedBlindCallTransfer = pMibVoipFeatureAccessCodes->UnattendedBlindCallTransfer;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX))
		{
			//TBD
			//mibVoipFeatureAccessCodes.AttendedCallTransfer = pMibVoipFeatureAccessCodes->AttendedCallTransfer;
		}

		/*Update to MIB*/
		MIB_Set(tableIndex, &mibVoipFeatureAccessCodes, sizeof(mibVoipFeatureAccessCodes));

    	break;
        #endif
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPFeatureAccessCodes --> DEL");
    	break;
    default:
    	break;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibVoIPFeatureAccessCodesTableInfo.Name = "VoIPFeatureAccessCodes";
    gMibVoIPFeatureAccessCodesTableInfo.ShortName = "VOIPFAC";
    gMibVoIPFeatureAccessCodesTableInfo.Desc = "VoIP feature access codes";
    gMibVoIPFeatureAccessCodesTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VOIP_FEATURE_ACCESS_CODES);
    gMibVoIPFeatureAccessCodesTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibVoIPFeatureAccessCodesTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVoIPFeatureAccessCodesTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVoIPFeatureAccessCodesTableInfo.pAttributes = &(gMibVoIPFeatureAccessCodesAttrInfo[0]);

    gMibVoIPFeatureAccessCodesTableInfo.attrNum = MIB_TABLE_VOIPFEATUREACCESSCODES_ATTR_NUM;
    gMibVoIPFeatureAccessCodesTableInfo.entrySize = sizeof(MIB_TABLE_VOIPFEATUREACCESSCODES_T);
    gMibVoIPFeatureAccessCodesTableInfo.pDefaultRow = &gMibVoIPFeatureAccessCodesDefRow;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CancelCallWaiting";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CancelHold";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CancelPark";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CancelIDActivate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CancelIDdeactivate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DoNotDisturbActivate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DoNotDisturbDeactivate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DoNotDisturbPINChange";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EmergencyServiceNumber";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntercomService";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnattendedBlindCallTransfer";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AttendedCallTransfer";

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Cancel call waiting";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Cancel hold";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Cancel park";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Cancel ID activate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Cancel ID deactivate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Do not disturb activate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Do not disturb activate";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Do not disturb PIN change";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Emergency service number";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Intercom service";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Unattended blind call transfer";
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Attended call transfer";

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 5;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELCALLWAITING_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELHOLD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELPARK_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_CANCELIDDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBDEACTIVATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_DONOTDISTURBPINCHANGE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_EMERGENCYSERVICENUMBER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_INTERCOMSERVICE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_UNATTENDEDBLINDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPFeatureAccessCodesAttrInfo[MIB_TABLE_VOIPFEATUREACCESSCODES_ATTENDEDCALLTRANSFER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    gMibVoIPFeatureAccessCodesDefRow.EntityId = 0;
    strncpy(gMibVoIPFeatureAccessCodesDefRow.CancelCallWaiting, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.CancelCallWaiting));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.CancelHold, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.CancelHold));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.CancelPark, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.CancelPark));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.CancelIDActivate, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.CancelIDActivate));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.CancelIDdeactivate, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.CancelIDdeactivate));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbActivate, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbActivate));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbDeactivate, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbDeactivate));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbPINChange, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.DoNotDisturbPINChange));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.EmergencyServiceNumber, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.EmergencyServiceNumber));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.IntercomService, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.IntercomService));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.UnattendedBlindCallTransfer, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.UnattendedBlindCallTransfer));
    strncpy(gMibVoIPFeatureAccessCodesDefRow.AttendedCallTransfer, "0", sizeof(gMibVoIPFeatureAccessCodesDefRow.AttendedCallTransfer));

    memset(&gMibVoIPFeatureAccessCodesOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVoIPFeatureAccessCodesOper.meOperDrvCfg = VoIPFeatureAccessCodesDrvCfg;
    gMibVoIPFeatureAccessCodesOper.meOperConnCheck = NULL;
    gMibVoIPFeatureAccessCodesOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibVoIPFeatureAccessCodesOper.meOperConnCfg = NULL;
    gMibVoIPFeatureAccessCodesOper.meOperAvlTreeAdd = NULL;
    gMibVoIPFeatureAccessCodesOper.meOperAlarmHandler = NULL;
    gMibVoIPFeatureAccessCodesOper.meOperTestHandler = NULL;

    MIB_TABLE_VOIPFEATUREACCESSCODES_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVoIPFeatureAccessCodesTableInfo, &gMibVoIPFeatureAccessCodesOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}

