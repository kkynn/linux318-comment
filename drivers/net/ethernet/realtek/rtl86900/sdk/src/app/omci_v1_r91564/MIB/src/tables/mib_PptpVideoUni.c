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
 * Purpose : Definition of ME handler: PPTP video UNI (82)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP video UNI (82)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T            gMibPptpVideoUniTblInfo;
MIB_ATTR_INFO_T             gMibPptpVideoUniAttrInfo[MIB_TABLE_PPTP_VIDEO_UNI_ATTR_NUM];
MIB_TABLE_PPTP_VIDEO_UNI_T  gMibPptpVideoUniDefRow;
MIB_TABLE_OPER_T            gMibPptpVideoUniOper;


GOS_ERROR_CODE pptp_video_uni_drv_cfg_handler(void          *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_PPTP_VIDEO_UNI_INDEX;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PPTP_VIDEO_UNI_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_PPTP_VIDEO_UNI_ARC_INDEX, MIB_TABLE_PPTP_VIDEO_UNI_ARC_INTVL_INDEX);
            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE pptp_video_uni_alarm_handler(MIB_TABLE_INDEX      tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t           alarmTable;
    BOOL                        isSuppressed;
    MIB_ENTRY_T                 *pEntry;
    MIB_TABLE_PPTP_VIDEO_UNI_T  *pMibPptpVideoUni;

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
        pMibPptpVideoUni = pEntry->pData;
        if (!pMibPptpVideoUni)
            return GOS_FAIL;

        // check if notifications are suppressed by parent's admin state
        omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
        else
        {
            // check if notifications are suppressed by self's admin state
            if (OMCI_ME_ATTR_ADMIN_STATE_LOCK == pMibPptpVideoUni->AdminState)
                *pIsUpdated = FALSE;

            // check if notifications are suppressed by ARC
            if (OMCI_ME_ATTR_ARC_ENABLED == pMibPptpVideoUni->Arc)
                *pIsUpdated = FALSE;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE pptp_video_uni_avc_cb(MIB_TABLE_INDEX     tableIndex,
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

    gMibPptpVideoUniTblInfo.Name = "PptpVideoUni";
    gMibPptpVideoUniTblInfo.ShortName = "VideoUni";
    gMibPptpVideoUniTblInfo.Desc = "PPTP video UNI";
    gMibPptpVideoUniTblInfo.ClassId = OMCI_ME_CLASS_PPTP_VIDEO_UNI;
    gMibPptpVideoUniTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibPptpVideoUniTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibPptpVideoUniTblInfo.ActionType = OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibPptpVideoUniTblInfo.pAttributes = &(gMibPptpVideoUniAttrInfo[0]);
	gMibPptpVideoUniTblInfo.attrNum = MIB_TABLE_PPTP_VIDEO_UNI_ATTR_NUM;
	gMibPptpVideoUniTblInfo.entrySize = sizeof(MIB_TABLE_PPTP_VIDEO_UNI_T);
	gMibPptpVideoUniTblInfo.pDefaultRow = &gMibPptpVideoUniDefRow;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "EntityId";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "Entity ID";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 2;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_ADMIN_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "AdminState";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "Administrative state";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_OP_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "OpState";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "Operational state";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "Arc";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "ARC";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_ARC_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "ArcIntvl";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "ARC interval";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_VIDEO_UNI_PWR_CTRL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptpVideoUniAttrInfo[attrIndex].Name = "PwrCtrl";
    gMibPptpVideoUniAttrInfo[attrIndex].Desc = "Power control";
    gMibPptpVideoUniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptpVideoUniAttrInfo[attrIndex].Len = 1;
    gMibPptpVideoUniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptpVideoUniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptpVideoUniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptpVideoUniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptpVideoUniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&gMibPptpVideoUniDefRow, 0x00, sizeof(gMibPptpVideoUniDefRow));

    memset(&gMibPptpVideoUniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibPptpVideoUniOper.meOperDrvCfg = pptp_video_uni_drv_cfg_handler;
	gMibPptpVideoUniOper.meOperConnCfg = NULL;
    gMibPptpVideoUniOper.meOperConnCheck = NULL;
    gMibPptpVideoUniOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibPptpVideoUniOper.meOperAlarmHandler = pptp_video_uni_alarm_handler;
    gMibPptpVideoUniOper.meOperTestHandler = NULL;

	MIB_TABLE_PPTP_VIDEO_UNI_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibPptpVideoUniTblInfo, &gMibPptpVideoUniOper);
    MIB_RegisterCallback(tableId, NULL, pptp_video_uni_avc_cb);

    return GOS_OK;
}
