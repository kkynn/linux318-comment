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
 * Purpose : Definition of ME handler: PPTP video ANI (90)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP video ANI (90)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T            gMibPptpVideoAniTblInfo;
MIB_ATTR_INFO_T             gMibPptpVideoAniAttrInfo[MIB_TABLE_PPTP_VIDEO_ANI_ATTR_NUM];
MIB_TABLE_PPTP_VIDEO_ANI_T  gMibPptpVideoAniDefRow;
MIB_TABLE_OPER_T            gMibPptpVideoAniOper;


GOS_ERROR_CODE pptp_video_ani_drv_cfg_handler(void          *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_PPTP_VIDEO_ANI_INDEX;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PPTP_VIDEO_ANI_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_PPTP_VIDEO_ANI_ARC_INDEX, MIB_TABLE_PPTP_VIDEO_ANI_ARC_INTVL_INDEX);
            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE pptp_video_ani_alarm_handler(MIB_TABLE_INDEX      tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t           alarmTable;
    BOOL                        isSuppressed;
    MIB_ENTRY_T                 *pEntry;
    MIB_TABLE_PPTP_VIDEO_ANI_T  *pMibPptpVideoAni;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    *pInstanceID = 1;

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

        // find the entry data by using the instanceID
        pEntry = mib_FindEntryByInstanceID(tableIndex, *pInstanceID);
        if (!pEntry)
            return GOS_FAIL;
        pMibPptpVideoAni = pEntry->pData;
        if (!pMibPptpVideoAni)
            return GOS_FAIL;

        // check if notifications are suppressed by parent's admin state
        omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
        else
        {
            // check if notifications are suppressed by self's admin state
            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibPptpVideoAni->AdminState)
                *pIsUpdated = FALSE;

            // check if notifications are suppressed by ARC
            if (OMCI_ME_ATTR_ARC_ENABLED == pMibPptpVideoAni->Arc)
                *pIsUpdated = FALSE;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE pptp_video_ani_avc_cb(MIB_TABLE_INDEX     tableIndex,
                                            void                *pOldRow,
                                            void                *pNewRow,
                                            MIB_ATTRS_SET       *pAttrsSet,
                                            MIB_OPERA_TYPE      operationType)
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
    MIB_ATTR_INDEX  attrIndex;

    gMibPptpVideoAniTblInfo.Name = "PptpVideoAni";
    gMibPptpVideoAniTblInfo.ShortName = "VideoAni";
    gMibPptpVideoAniTblInfo.Desc = "PPTP video ANI";
    gMibPptpVideoAniTblInfo.ClassId = OMCI_ME_CLASS_PPTP_VIDEO_ANI;
    gMibPptpVideoAniTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibPptpVideoAniTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibPptpVideoAniTblInfo.ActionType = OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibPptpVideoAniTblInfo.pAttributes = &(gMibPptpVideoAniAttrInfo[0]);
	gMibPptpVideoAniTblInfo.attrNum = MIB_TABLE_PPTP_VIDEO_ANI_ATTR_NUM;
	gMibPptpVideoAniTblInfo.entrySize = sizeof(MIB_TABLE_PPTP_VIDEO_ANI_T);
	gMibPptpVideoAniTblInfo.pDefaultRow = &gMibPptpVideoAniDefRow;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "EntityId";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Entity ID";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 2;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_ADMIN_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "AdminState";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Administrative state";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_OP_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "OpState";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Operational state";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_ARC_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "Arc";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "ARC";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_ARC_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "ArcIntvl";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "ARC interval";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_FREQ_RANGE_LOW_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "FreqRangeLow";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Frequency range low";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_FREQ_RANGE_HIGH_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "FreqRangeHigh";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Frequency range high";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_SIG_CAPABILITY_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "SigCapability";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Signal capability";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_OPTICAL_SIG_LVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "OpticalSigLvl";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Optical signal level";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_PILOT_SIG_LVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "PilotSigLvl";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Pilot signal level";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_SIG_LVL_MIN_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "SigLvlMin";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Signal level min";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_SIG_LVL_MAX_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "SigLvlMax";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Signal level max";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_PILOT_FREQ_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "PilotFreq";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Pilot frequency";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT32;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 4;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_AGC_MODE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "AgcMode";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "AGC mode";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_AGC_SETTING_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "AgcSetting";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "AGC setting";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_VIDEO_LOW_THOLD_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "VideoLowThold";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Video lower optical threshold";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_ANI_VIDEO_HIGH_THOLD_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoAniAttrInfo[attrIndex].Name = "VideoHighThold";
    gMibPptpVideoAniAttrInfo[attrIndex].Desc = "Video lower optical threshold";
    gMibPptpVideoAniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoAniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoAniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoAniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoAniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoAniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoAniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&gMibPptpVideoAniDefRow, 0x00, sizeof(gMibPptpVideoAniDefRow));
    gMibPptpVideoAniDefRow.VideoLowThold = 0xA1;
    gMibPptpVideoAniDefRow.VideoHighThold = 0x19;

    memset(&gMibPptpVideoAniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibPptpVideoAniOper.meOperDrvCfg = pptp_video_ani_drv_cfg_handler;
	gMibPptpVideoAniOper.meOperConnCfg = NULL;
    gMibPptpVideoAniOper.meOperConnCheck = NULL;
    gMibPptpVideoAniOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibPptpVideoAniOper.meOperAlarmHandler = pptp_video_ani_alarm_handler;
    gMibPptpVideoAniOper.meOperTestHandler = NULL;

	MIB_TABLE_PPTP_VIDEO_ANI_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibPptpVideoAniTblInfo, &gMibPptpVideoAniOper);
    MIB_RegisterCallback(tableId, NULL, pptp_video_ani_avc_cb);

    return GOS_OK;
}
