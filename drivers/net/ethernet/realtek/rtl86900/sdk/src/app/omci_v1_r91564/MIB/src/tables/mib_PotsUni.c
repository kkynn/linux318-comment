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
 * Purpose : Definition of ME handler: PPTP POTS UNI (53)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP POTS UNI (53)
 */

#include "app_basic.h"
#include "omci_task.h"


MIB_TABLE_INFO_T gMibPotsUniTableInfo;
MIB_ATTR_INFO_T  gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ATTR_NUM];
MIB_TABLE_POTSUNI_T gMibPotsUniDefRow;
MIB_TABLE_OPER_T gMibPotsUniOper;

GOS_ERROR_CODE PotsUniDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_INDEX     tableIndex = MIB_TABLE_POTSUNI_INDEX;
	MIB_TABLE_POTSUNI_T *pMibPotsUni = NULL, *pOldMibPotsUni = NULL, mibPotsUni;
	GOS_ERROR_CODE      ret = GOS_OK;
    UINT16              chid;
    UINT8 hookState = 0;
	omci_voice_vendor_t             voice_vendor;
    MIB_TABLE_VOIPCONFIGDATA_T   mibVcd;
    mibVcd.EntityId = 0;



	pMibPotsUni = (MIB_TABLE_POTSUNI_T *)pNewRow;
    pOldMibPotsUni = (MIB_TABLE_POTSUNI_T *)pOldRow;
	voice_vendor = (gInfo.voiceVendor ? VOICE_VENDOR_RTK : VOICE_VENDOR_NONE);

    mibPotsUni.EntityId = pMibPotsUni->EntityId;
    omci_get_channel_index_by_pots_uni_me_id(pMibPotsUni->EntityId, &chid);

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);
    switch (operationType)
    {
    case MIB_ADD:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PotsUni --> ADD");
    	if (GOS_OK == MIB_Get(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVcd, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)) &&
		mibVcd.VOIPConfigurationMethodUsed == VCD_CFG_METHOD_USED_OMCI)
       {
            VOICE_WRAPPER(omci_voice_pots_state_set, chid, pMibPotsUni->AdminState);
			omci_voice_vendor_service_cb(voice_vendor, NULL);
       }
    	break;
    case MIB_SET:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PotsUni --> SET");
    	if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_POTSUNI_ARC_INDEX))
    	{
    		ret = omci_arc_timer_processor(tableIndex,
    			pOldRow, pNewRow, MIB_TABLE_POTSUNI_ARC_INDEX, MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX);
    	}

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_POTSUNI_ADMINSTATE_INDEX) &&
            pMibPotsUni->AdminState != pOldMibPotsUni->AdminState)
        {
                if (GOS_OK == MIB_Get(MIB_TABLE_VOIPCONFIGDATA_INDEX, &mibVcd, sizeof(MIB_TABLE_VOIPCONFIGDATA_T)) &&
			mibVcd.VOIPConfigurationMethodUsed == VCD_CFG_METHOD_USED_OMCI)
    		{
    			VOICE_WRAPPER(omci_voice_pots_state_set, chid, pMibPotsUni->AdminState);
    			omci_voice_vendor_service_cb(voice_vendor, NULL);
    		}
        }

    	break;
    case MIB_GET:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PotsUni --> GET");
        
        if (GOS_OK != MIB_Get(tableIndex, &mibPotsUni, sizeof(mibPotsUni)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), mibPotsUni.EntityId);
            return GOS_FAIL;
        }

		VOICE_WRAPPER(omci_voice_hook_state_get, chid, &hookState);
		if (MIB_IsInAttrSet(&attrSet , MIB_TABLE_POTSUNI_HOOKSTATE_INDEX))
	{
		pMibPotsUni->HookState = hookState;
		mibPotsUni.HookState= hookState;
		/*Update to MIB*/
		MIB_Set(tableIndex, &mibPotsUni, sizeof(mibPotsUni));
	}

    	break;
    case MIB_DEL:
    	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PotsUni --> DEL");
    	break;
    default:
        break;
    }

    return ret;
}

static GOS_ERROR_CODE pots_uni_test_handler(void    *pData)
{
    GOS_ERROR_CODE              ret = GOS_OK;
    omci_msg_norm_baseline_t    *pOmciMsg;
    MIB_TABLE_INDEX             tableIndex = MIB_TABLE_POTSUNI_INDEX;
    UINT16                      chid;
    //BOOL                        is_busy = FALSE;
    pots_uni_test_mode_t        test_mode;
    UINT8                       select_test;

    // make sure the data is available
    if (!pData)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Test data unavailable to proceed: %s",
            MIB_GetTableName(tableIndex));

        return GOS_ERR_PARAM;
    }
    pOmciMsg = (omci_msg_norm_baseline_t *)pData;

    // fill in header
    pOmciMsg->db     = 0;
    pOmciMsg->ar     = 0;
    pOmciMsg->ak     = 0;
    pOmciMsg->type   = OMCI_MSG_TYPE_TEST_RESULT;

    test_mode = (((pOmciMsg->content[0]) >> 7) ? POTS_UNI_TEST_MODE_FORCED : POTS_UNI_TEST_MODE_NORMAL);

    select_test = ((pOmciMsg->content[0]) & 0xF);

    /* check if the we support the test type
    if (ANIG_TEST_TYPE_SELF_TEST != pOmciMsg->content[0])
        goto out;*/

    memset(pOmciMsg->content, 0, OMCI_MSG_BASELINE_CONTENTS_LEN);

    omci_get_channel_index_by_pots_uni_me_id(pOmciMsg->meId.meInstance, &chid);

    if ( chid > gInfo.devCapabilities.potsPortNum )
    {
        OMCI_PRINT("%s(%d) channel number %u out of range\n" , __FUNCTION__ , __LINE__ , chid);
        goto out;
    }

    // TBD: check line busy by chid

    if (POTS_UNI_TEST_MODE_NORMAL == test_mode)
        goto out; //deny test

    // TBD: get real result of hazardous
    if (POTS_UNI_TEST_TYPE_HAZARDOUS_POTENTIAL_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED << 5);

    // TBD: get real result of foreign emf
    if (POTS_UNI_TEST_TYPE_FOREIGN_EMF_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED << 4);

    // TBD: get real result of resistive faults
    if (POTS_UNI_TEST_TYPE_RESISTIVE_FAULTS_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED << 3);

    // TBD: get real result of receiver off hook
    if (POTS_UNI_TEST_TYPE_RECEIVER_OFF_HOOOK_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED << 2);

    // TBD: get real result of ringer
    if (POTS_UNI_TEST_TYPE_RINGER_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED << 1);

    // TBD: get real result of nt1 DC signature
    if (POTS_UNI_TEST_TYPE_NT1_DC_SIGNATURE_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[0] |= (POTS_UNI_TEST_RESULT_PASSED);

    // TBD: get real result of self test
    if (POTS_UNI_TEST_TYPE_SELF_TEST == select_test ||
        POTS_UNI_TEST_TYPE_ALL_MLT_TEST == select_test)
        pOmciMsg->content[1] |= (POTS_UNI_TEST_RESULT_NOT_COMPLETED);

out:
    // send back the response
    ret = OMCI_SendMsg(OMCI_APPL,
                        OMCI_TX_OMCI_MSG,
                        OMCI_MSG_PRI_NORMAL,
                        pOmciMsg,
                        sizeof(omci_msg_norm_baseline_t));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Unable to send out test result: %s",
            MIB_GetTableName(tableIndex));
    }

    // free allocated memory before leaving
    free(pData);

    return ret;
}


static GOS_ERROR_CODE pots_uni_avc_cb(MIB_TABLE_INDEX  tableIndex,
                                            void            *pOldRow,
                                            void            *pNewRow,
                                            MIB_ATTRS_SET   *pAttrsSet,
                                            MIB_OPERA_TYPE  operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              attrSize, i;
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
        if (MIB_SET == operationType && pOldRow)
        {
            UINT8 adminState;

            MIB_GetAttrFromBuf(tableIndex, MIB_TABLE_POTSUNI_ADMINSTATE_INDEX, &adminState, pOldRow, sizeof(UINT8));

            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == adminState)
                isSuppressed = TRUE;
        }
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

            attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

            // admin state AVC has special handling according to G.988 9.9.1
            if (MIB_TABLE_POTSUNI_ADMINSTATE_INDEX == attrIndex)
            {
                if (MIB_SET == operationType)
                {
                    UINT8 newValue, oldValue;

                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &newValue, pNewRow, attrSize);
                    MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);

                    // only report on the transition from shutdown to lock
                    if (OMCI_ME_ATTR_ADMIN_STATE_SHUTDOWN == oldValue &&
                            OMCI_ME_ATTR_ADMIN_STATE_LOCK == newValue)
                        MIB_SetAttrSet(&avcAttrSet, attrIndex);
                    else
                        MIB_UnSetAttrSet(&avcAttrSet, attrIndex);
                }
                else
                    MIB_UnSetAttrSet(&avcAttrSet, attrIndex);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibPotsUniTableInfo.Name = "PotsUni";
    gMibPotsUniTableInfo.ShortName = "POTS";
    gMibPotsUniTableInfo.Desc = "PPTP POTS UNI";
    gMibPotsUniTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PPTP_POTS_UNI);
    gMibPotsUniTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibPotsUniTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibPotsUniTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_TEST);
    gMibPotsUniTableInfo.pAttributes = &(gMibPotsUniAttrInfo[0]);

    gMibPotsUniTableInfo.attrNum = MIB_TABLE_POTSUNI_ATTR_NUM;
    gMibPotsUniTableInfo.entrySize = sizeof(MIB_TABLE_POTSUNI_T);
    gMibPotsUniTableInfo.pDefaultRow = &gMibPotsUniDefRow;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AdminState";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IWTPPtr";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARC";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARCInterval";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Impedance";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxPath";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "RxGain";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TxGain";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpState";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "HookState";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "POTSHoldoverTime";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NominalFeedVoltage";

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Administrative state";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interworking TP pointer";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC Interval";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Impedance";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Transmission path";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Rx gain";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Tx gain";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational state";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Hook state";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "POTS holdover time";
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Nominal feed voltage";

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IWTPPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_ARCINTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_IMPEDANCE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXPATH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_RXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_TXGAIN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_HOOKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_POTSHOLDOVERTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibPotsUniAttrInfo[MIB_TABLE_POTSUNI_NOMINALFEEDVOLTAGE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    gMibPotsUniDefRow.EntityId = 0;
    gMibPotsUniDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;
    gMibPotsUniDefRow.IWTPPtr = 0;
    gMibPotsUniDefRow.ARC = 0;
    gMibPotsUniDefRow.ARCInterval = 0;
    gMibPotsUniDefRow.Impedance = 0;
    gMibPotsUniDefRow.TxPath = 0;
    gMibPotsUniDefRow.RxGain = 0;
    gMibPotsUniDefRow.TxGain = 0;
    gMibPotsUniDefRow.OpState = OMCI_ME_ATTR_OP_STATE_ENABLED;
    gMibPotsUniDefRow.HookState = 0;
    gMibPotsUniDefRow.POTSHoldoverTime = 0;
    gMibPotsUniDefRow.NominalFeedVoltage = 0;

    memset(&gMibPotsUniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibPotsUniOper.meOperDrvCfg = PotsUniDrvCfg;
    gMibPotsUniOper.meOperConnCheck = NULL;
    gMibPotsUniOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibPotsUniOper.meOperConnCfg = NULL;
    gMibPotsUniOper.meOperAvlTreeAdd = NULL;
    gMibPotsUniOper.meOperAlarmHandler = NULL;
    gMibPotsUniOper.meOperTestHandler = pots_uni_test_handler;

    MIB_TABLE_POTSUNI_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibPotsUniTableInfo, &gMibPotsUniOper);
    MIB_RegisterCallback(tableId, NULL, pots_uni_avc_cb);

    return GOS_OK;
}

