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
 * Purpose : Definition of ME handler: VoIP line status  (141)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: VoIP line status (141)
 */

#include "app_basic.h"

MIB_TABLE_INFO_T gMibVoIPLineStatusTableInfo;
MIB_ATTR_INFO_T  gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ATTR_NUM];
MIB_TABLE_VOIPLINESTATUS_T gMibVoIPLineStatusDefRow;
MIB_TABLE_OPER_T gMibVoIPLineStatusOper;

GOS_ERROR_CODE VoIPLineStatusDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_VOIPLINESTATUS_T *pMibLineStatus,mibLineStatus;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_VOIPLINESTATUS_INDEX;
    pMibLineStatus = (MIB_TABLE_VOIPLINESTATUS_T *)pNewRow;
    omci_VoIP_line_status_t voipLineSts;
	mibLineStatus.EntityId = pMibLineStatus->EntityId;

    omci_get_channel_index_by_pots_uni_me_id(pMibLineStatus->EntityId,&voipLineSts.ch_id);
	if (voipLineSts.ch_id > gInfo.devCapabilities.potsPortNum )
    {
		OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s(%d) channel number %u out of range\n" , __FUNCTION__ , __LINE__ , voipLineSts.ch_id);
	}
    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

    switch (operationType){
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPLineStatus --> ADD");
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPLineStatus --> SET");
    	break;

    case MIB_GET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPLineStatus --> GET");

        if(GOS_OK != MIB_Get(tableIndex, &mibLineStatus, sizeof(mibLineStatus)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibLineStatus.EntityId);

            return GOS_FAIL;
        }
        memset(&voipLineSts, 0, sizeof(omci_VoIP_line_status_t));
	    /*Get from driver*/
        VOICE_WRAPPER(omci_voice_VOIP_line_status_get,  &voipLineSts);


		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX))
		{
            mibLineStatus.VoipVoiceServerStatus = voipLineSts.voipVoiceServerStatus;
			pMibLineStatus->VoipVoiceServerStatus= voipLineSts.voipVoiceServerStatus;
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX))
		{
            mibLineStatus.VoipPortSessionType = voipLineSts.voipPortSessionType;
			pMibLineStatus->VoipPortSessionType= voipLineSts.voipPortSessionType;
		}


		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX))
		{
			mibLineStatus.VoipCodecUsed = voipLineSts.voipCodecUsed;
			pMibLineStatus->VoipCodecUsed= voipLineSts.voipCodecUsed;
		}
#if 0 //not need
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX))
		{
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX))
		{
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX))
		{
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX))
		{
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX))
		{
		}
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX))
		{
		}
#endif
		/*Update to MIB*/
		MIB_Set(tableIndex, &mibLineStatus, sizeof(mibLineStatus));
    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"VoIPLineStatus --> DEL");
    	break;
    default:
    	break;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE voip_line_status_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                                void            *pOldRow,
                                                void            *pNewRow,
                                                MIB_ATTRS_SET   *pAttrsSet,
                                                MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;
    omci_me_instance_t  instanceID;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // read out the instanceID
    MIB_GetAttrFromBuf(tableIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

    // correct the instanceID to the slot it belongs to
    instanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | (instanceID >> 8);

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(instanceID, &isSuppressed);
    if (!isSuppressed)
    {
        GOS_ERROR_CODE              ret;
        MIB_TABLE_INDEX             tablePotsIndex;
        MIB_TABLE_POTSUNI_T   mibPots;

        tablePotsIndex = MIB_TABLE_POTSUNI_INDEX;
        MIB_GetAttrFromBuf(tablePotsIndex, MIB_ATTR_FIRST_INDEX, &instanceID, pNewRow, sizeof(omci_me_instance_t));

        mibPots.EntityId = instanceID;
        ret = MIB_Get(tablePotsIndex, &mibPots, MIB_GetTableEntrySize(tablePotsIndex));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tablePotsIndex), instanceID);
            return ret;
        }

        if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == mibPots.AdminState)
            isSuppressed = TRUE;
    }

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
    gMibVoIPLineStatusTableInfo.Name = "VoIPLineStatus";
    gMibVoIPLineStatusTableInfo.ShortName = "VOIPLS";
    gMibVoIPLineStatusTableInfo.Desc = "VoIP line status";
    gMibVoIPLineStatusTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_VOIP_LINE_STATUS);
    gMibVoIPLineStatusTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibVoIPLineStatusTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibVoIPLineStatusTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET);
    gMibVoIPLineStatusTableInfo.pAttributes = &(gMibVoIPLineStatusAttrInfo[0]);

    gMibVoIPLineStatusTableInfo.attrNum = MIB_TABLE_VOIPLINESTATUS_ATTR_NUM;
    gMibVoIPLineStatusTableInfo.entrySize = sizeof(MIB_TABLE_VOIPLINESTATUS_T);
    gMibVoIPLineStatusTableInfo.pDefaultRow = &gMibVoIPLineStatusDefRow;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipCodecUsed";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipVoiceServerStatus";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipPortSessionType";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipCall1PacketPeriod";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipCall2PacketPeriod";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipCall1DestAddr";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipCall2DestAddr";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VoipLineState";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EmergencyCallStatus";

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip codec used";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip voice server status";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip port session type";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip call 1 packet period";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip call 2 packet period";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip call 1 dest addr";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "voip call 2 dest addr";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Voip line state";
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Emergency call status";

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCODECUSED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPVOICESERVERSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPPORTSESSIONTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2PACKETPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL1DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPCALL2DESTADDR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_VOIPLINESTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibVoIPLineStatusAttrInfo[MIB_TABLE_VOIPLINESTATUS_EMERGENCYCALLSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    gMibVoIPLineStatusDefRow.EntityId = 0;
    gMibVoIPLineStatusDefRow.VoipCodecUsed = 0;
    gMibVoIPLineStatusDefRow.VoipVoiceServerStatus = 0;
    gMibVoIPLineStatusDefRow.VoipPortSessionType = 0;
    gMibVoIPLineStatusDefRow.VoipCall1PacketPeriod = 0;
    gMibVoIPLineStatusDefRow.VoipCall2PacketPeriod = 0;
    strncpy(gMibVoIPLineStatusDefRow.VoipCall1DestAddr, "0", sizeof(gMibVoIPLineStatusDefRow.VoipCall1DestAddr));
    strncpy(gMibVoIPLineStatusDefRow.VoipCall2DestAddr, "0", sizeof(gMibVoIPLineStatusDefRow.VoipCall2DestAddr));
    gMibVoIPLineStatusDefRow.VoipLineState = 0;
    gMibVoIPLineStatusDefRow.EmergencyCallStatus = 0;

    memset(&gMibVoIPLineStatusOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibVoIPLineStatusOper.meOperDrvCfg = VoIPLineStatusDrvCfg;
    gMibVoIPLineStatusOper.meOperConnCheck = NULL;
    gMibVoIPLineStatusOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibVoIPLineStatusOper.meOperConnCfg = NULL;
    gMibVoIPLineStatusOper.meOperAvlTreeAdd = NULL;
    gMibVoIPLineStatusOper.meOperAlarmHandler = NULL;
    gMibVoIPLineStatusOper.meOperTestHandler = NULL;

    MIB_TABLE_VOIPLINESTATUS_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibVoIPLineStatusTableInfo, &gMibVoIPLineStatusOper);
    MIB_RegisterCallback(tableId, NULL, voip_line_status_avc_cb);

    return GOS_OK;
}

