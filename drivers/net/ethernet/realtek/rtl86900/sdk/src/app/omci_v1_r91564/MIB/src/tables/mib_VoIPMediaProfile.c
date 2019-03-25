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

MIB_TABLE_INFO_T gMibVoIPMediaProfileTableInfo;
MIB_ATTR_INFO_T  gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ATTR_NUM];
MIB_TABLE_VOIPMEDIAPROFILE_T gMibVoIPMediaProfileDefRow;
MIB_TABLE_OPER_T gMibVoIPMediaProfileOper;

GOS_ERROR_CODE VoIPMediaProfileDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    //MIB_TABLE_INDEX     tableIndex = MIB_TABLE_VOIPMEDIAPROFILE_INDEX;
	//MIB_TABLE_VOIPMEDIAPROFILE_T *pMibVoipMediaProfile;

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    // read out the instanceID
	//pMibVoipMediaProfile = (MIB_TABLE_VOIPMEDIAPROFILE_T*)pNewRow;
    #if 0
    INT32 i;

	UINT32 chid;
	UINT32 size=0;

   	//mibVoipMediaProfile.EntityId = pMibVoipMediaProfile->EntityId;
	const UINT32 entity_id = pMibVoipMediaProfile->EntityId;

	// VoIP voice CTP 9.9.4
	MIB_TABLE_VOIPVOICECTP_T mibVoIPVoiceCtp, *pMibVoIPVoiceCtp;


	// find channel id
	size = sizeof(MIB_TABLE_VOIPVOICECTP_T);

	if((MIB_GetFirst(MIB_TABLE_VOIPVOICECTP_INDEX, &mibVoIPVoiceCtp , size))==GOS_OK){
		if( mibVoIPVoiceCtp.VOIPMediaProfilePointer == entity_id ){
			chid = mibVoIPVoiceCtp.PPTPPointer;
		}
		while( (MIB_GetNext(MIB_TABLE_VOIPVOICECTP_INDEX, &mibVoIPVoiceCtp , size))==GOS_OK ){
			if( mibVoIPVoiceCtp.VOIPMediaProfilePointer == entity_id ){
				chid = mibVoIPVoiceCtp.PPTPPointer;
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
	pCfgPort = &g_pShareCfgVoIP->ports[chid];	// assign config pointer
    #endif

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPMediaProfile --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPMediaProfile --> SET");
    	break;
        #if 0 // Not need
    case MIB_GET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPMediaProfile --> GET");
		MIB_Get(tableIndex, &mibVoipMediaProfile, sizeof(mibVoipMediaProfile));

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX))
		{
			mibVoipMediaProfile.EntityId = pMibVoipMediaProfile->EntityId;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX))
		{
			mibVoipMediaProfile.FaxMode = pCfgPort->useT38;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX))
		{
			mibVoipMediaProfile.VoiceServiceProfilePointer = pMibVoipMediaProfile->VoiceServiceProfilePointer;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX))
		{
			mibVoipMediaProfile.CodecSelection1stOrder = pMibVoipMediaProfile->CodecSelection1stOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX))
		{
			mibVoipMediaProfile.PacketPeriodSelection1stOrder = pMibVoipMediaProfile->PacketPeriodSelection1stOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX))
		{
			mibVoipMediaProfile.SilenceSuppression1stOrder = pMibVoipMediaProfile->SilenceSuppression1stOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX))
		{
			mibVoipMediaProfile.CodecSelection2ndOrder = pMibVoipMediaProfile->CodecSelection2ndOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX))
		{
			mibVoipMediaProfile.PacketPeriodSelection2ndOrder = pMibVoipMediaProfile->PacketPeriodSelection2ndOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX))
		{
			mibVoipMediaProfile.SilenceSuppression2ndOrder = pMibVoipMediaProfile->SilenceSuppression2ndOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX))
		{
			mibVoipMediaProfile.CodecSelection3rdOrder = pMibVoipMediaProfile->CodecSelection3rdOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX))
		{
			mibVoipMediaProfile.PacketPeriodSelection3rdOrder = pMibVoipMediaProfile->PacketPeriodSelection3rdOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX))
		{
			mibVoipMediaProfile.SilenceSuppression3rdOrder = pMibVoipMediaProfile->SilenceSuppression3rdOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX ))
		{
			mibVoipMediaProfile.CodecSelection4thOrder = pMibVoipMediaProfile->CodecSelection4thOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX))
		{
			mibVoipMediaProfile.PacketPeriodSelection4thOrder = pMibVoipMediaProfile->PacketPeriodSelection4thOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX))
		{
			mibVoipMediaProfile.SilenceSuppression4thOrder = pMibVoipMediaProfile->SilenceSuppression4thOrder;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX))
		{
			if( pCfgPort->dtmf_mode == DTMF_RFC2833 ){
				mibVoipMediaProfile.OOBDTMF = 1;
			}
			else{
				mibVoipMediaProfile.OOBDTMF = 0;
			}

			//mibVoipMediaProfile.OOBDTMF = pMibVoipMediaProfile->OOBDTMF;
		}

		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX))
		{
			mibVoipMediaProfile.RTPProfilePointer = pMibVoipMediaProfile->RTPProfilePointer;
		}

		/*Update to MIB*/
		MIB_Set(tableIndex, &mibVoipMediaProfile, sizeof(mibVoipMediaProfile));

    	break;
        #endif
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPMediaProfile --> DEL");
    	break;
    default:
    	break;
    }

    return GOS_OK;
}
GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibVoIPMediaProfileTableInfo.Name = "VoIPMediaProfile";
    gMibVoIPMediaProfileTableInfo.ShortName = "VOIPMP";
    gMibVoIPMediaProfileTableInfo.Desc = "VoIP Media Profile";
    gMibVoIPMediaProfileTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VOIP_MEDIA_PROFILE);
    gMibVoIPMediaProfileTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibVoIPMediaProfileTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVoIPMediaProfileTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibVoIPMediaProfileTableInfo.pAttributes = &(gMibVoIPMediaProfileAttrInfo[0]);

    gMibVoIPMediaProfileTableInfo.attrNum = MIB_TABLE_VOIPMEDIAPROFILE_ATTR_NUM;
    gMibVoIPMediaProfileTableInfo.entrySize = sizeof(MIB_TABLE_VOIPMEDIAPROFILE_T);
    gMibVoIPMediaProfileTableInfo.pDefaultRow = &gMibVoIPMediaProfileDefRow;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "FaxMode";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoiceServiceProfilePointer";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CodecSelection1stOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PacketPeriodSelection1stOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SilenceSuppression1stOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CodecSelection2ndOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PacketPeriodSelection2ndOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SilenceSuppression2ndOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CodecSelection3rdOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PacketPeriodSelection3rdOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SilenceSuppression3rdOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CodecSelection4thOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PacketPeriodSelection4thOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SilenceSuppression4thOrder";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OOBDTMF";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RTPProfilePointer";

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Fax mode";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Voice service profile pointer";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Codec selection 1st order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet period selection 1st order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Silence suppression 1st order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Codec selection 2nd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet period selection 2nd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Silence suppression 2nd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Codec selection 3rd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet period selection 3rd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Silence suppression 3rd order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Codec selection 4th order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Packet period selection 4th order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Silence suppression 4th order";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "OOB DTMF";
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "RTP profile pointer";

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_FAXMODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_VOICESERVICEPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION1STORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION2NDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION3RDORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_CODECSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_PACKETPERIODSELECTION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_SILENCESUPPRESSION4THORDER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_OOBDTMF_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPMediaProfileAttrInfo[MIB_TABLE_VOIPMEDIAPROFILE_RTPPROFILEPOINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    gMibVoIPMediaProfileDefRow.EntityId = 0;
    gMibVoIPMediaProfileDefRow.FaxMode = 0;
    gMibVoIPMediaProfileDefRow.VoiceServiceProfilePointer = 0;
    gMibVoIPMediaProfileDefRow.CodecSelection1stOrder = 0;
    gMibVoIPMediaProfileDefRow.PacketPeriodSelection1stOrder = 10;
    gMibVoIPMediaProfileDefRow.SilenceSuppression1stOrder = 0;
    gMibVoIPMediaProfileDefRow.CodecSelection2ndOrder = 0;
    gMibVoIPMediaProfileDefRow.PacketPeriodSelection2ndOrder = 10;
    gMibVoIPMediaProfileDefRow.SilenceSuppression2ndOrder = 0;
    gMibVoIPMediaProfileDefRow.CodecSelection3rdOrder = 0;
    gMibVoIPMediaProfileDefRow.PacketPeriodSelection3rdOrder = 10;
    gMibVoIPMediaProfileDefRow.SilenceSuppression3rdOrder = 0;
    gMibVoIPMediaProfileDefRow.CodecSelection4thOrder = 0;
    gMibVoIPMediaProfileDefRow.PacketPeriodSelection4thOrder = 10;
    gMibVoIPMediaProfileDefRow.SilenceSuppression4thOrder = 0;
    gMibVoIPMediaProfileDefRow.OOBDTMF = 0;
    gMibVoIPMediaProfileDefRow.RTPProfilePointer = 0;

    memset(&gMibVoIPMediaProfileOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVoIPMediaProfileOper.meOperDrvCfg = VoIPMediaProfileDrvCfg;
    gMibVoIPMediaProfileOper.meOperConnCheck = NULL;
    gMibVoIPMediaProfileOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibVoIPMediaProfileOper.meOperConnCfg = NULL;
    gMibVoIPMediaProfileOper.meOperAvlTreeAdd = NULL;
    gMibVoIPMediaProfileOper.meOperAlarmHandler = NULL;
    gMibVoIPMediaProfileOper.meOperTestHandler = NULL;

    MIB_TABLE_VOIPMEDIAPROFILE_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVoIPMediaProfileTableInfo, &gMibVoIPMediaProfileOper);
    MIB_RegisterCallback(tableId, NULL, NULL);

    return GOS_OK;
}
