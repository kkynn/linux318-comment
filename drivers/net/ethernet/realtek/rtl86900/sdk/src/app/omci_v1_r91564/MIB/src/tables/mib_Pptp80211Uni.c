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
 * Purpose : Definition of ME handler: PPTP 802.11 UNI (91)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: PPTP 802.11 UNI (91)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T            gMibPptp80211UniTblInfo;
MIB_ATTR_INFO_T             gMibPptp80211UniAttrInfo[MIB_TABLE_PPTP_80211_UNI_ATTR_NUM];
MIB_TABLE_PPTP_80211_UNI_T  gMibPptp80211UniDefRow;
MIB_TABLE_OPER_T            gMibPptp80211UniOper;


GOS_ERROR_CODE pptp_80211_uni_dump_handler(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
    MIB_TABLE_PPTP_80211_UNI_T  *pMibPptp80211Uni;
    unsigned char               i;

    pMibPptp80211Uni = (MIB_TABLE_PPTP_80211_UNI_T *)pData;

	OMCI_PRINT("EntityId: 0x%04x", pMibPptp80211Uni->EntityId);
	OMCI_PRINT("AdminState: %hhu", pMibPptp80211Uni->AdminState);
	OMCI_PRINT("OpState: %hhu", pMibPptp80211Uni->OpState);
    OMCI_PRINT("DataRateTx: 0x%08x%08x",
        pMibPptp80211Uni->DataRateTx.high, pMibPptp80211Uni->DataRateTx.low);
    OMCI_PRINT("DataRateRx: 0x%08x%08x",
        pMibPptp80211Uni->DataRateRx.high, pMibPptp80211Uni->DataRateRx.low);
    OMCI_PRINT("TxPwrLevel:");
    for (i = 0; i < MIB_TABLE_PPTP_80211_UNI_TX_PWR_LVL_LEN; i += 2)
    {
        OMCI_PRINT("0x%02x%02x",
            pMibPptp80211Uni->TxPwrLevel[i], pMibPptp80211Uni->TxPwrLevel[i+1]);
    }
    OMCI_PRINT("Arc: %hhu", pMibPptp80211Uni->Arc);
    OMCI_PRINT("ArcIntvl: %hhu", pMibPptp80211Uni->ArcIntvl);

	return GOS_OK;
}

GOS_ERROR_CODE pptp_80211_uni_drv_cfg_handler(void          *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_PPTP_80211_UNI_INDEX;

    switch (operationType)
    {
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_PPTP_80211_UNI_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_PPTP_80211_UNI_ARC_INDEX, MIB_TABLE_PPTP_80211_UNI_ARC_INTVL_INDEX);
            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE pptp_80211_uni_avc_cb(MIB_TABLE_INDEX     tableIndex,
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

    gMibPptp80211UniTblInfo.Name = "Pptp80211Uni";
    gMibPptp80211UniTblInfo.ShortName = "80211";
    gMibPptp80211UniTblInfo.Desc = "PPTP 802.11 UNI";
    gMibPptp80211UniTblInfo.ClassId = OMCI_ME_CLASS_PPTP_80211_UNI;
    gMibPptp80211UniTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibPptp80211UniTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibPptp80211UniTblInfo.ActionType = OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibPptp80211UniTblInfo.pAttributes = &(gMibPptp80211UniAttrInfo[0]);
	gMibPptp80211UniTblInfo.attrNum = MIB_TABLE_PPTP_80211_UNI_ATTR_NUM;
	gMibPptp80211UniTblInfo.entrySize = sizeof(MIB_TABLE_PPTP_80211_UNI_T);
	gMibPptp80211UniTblInfo.pDefaultRow = &gMibPptp80211UniDefRow;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "EntityId";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "Entity ID";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 2;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_ADMIN_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "AdminState";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "Administrative state";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 1;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_OP_STATE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "OpState";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "Operational state";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 1;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_DATA_RATE_TX_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "DataRateTx";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "dot11 supported data rates tx";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT64;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 8;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_DATA_RATE_RX_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "DataRateRx";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "dot11 supported data rates rx";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT64;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 8;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_TX_PWR_LVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "TxPwrLevel";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "dot11 Tx power levels";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_TABLE;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 16;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_ARC_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "Arc";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "ARC";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 1;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_PPTP_80211_UNI_ARC_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibPptp80211UniAttrInfo[attrIndex].Name = "ArcIntvl";
    gMibPptp80211UniAttrInfo[attrIndex].Desc = "ARC interval";
    gMibPptp80211UniAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPptp80211UniAttrInfo[attrIndex].Len = 1;
    gMibPptp80211UniAttrInfo[attrIndex].IsIndex = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].MibSave = TRUE;
    gMibPptp80211UniAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPptp80211UniAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibPptp80211UniAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibPptp80211UniAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&gMibPptp80211UniDefRow, 0x00, sizeof(gMibPptp80211UniDefRow));

    memset(&gMibPptp80211UniOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibPptp80211UniOper.meOperDrvCfg = pptp_80211_uni_drv_cfg_handler;
	gMibPptp80211UniOper.meOperConnCfg = NULL;
    gMibPptp80211UniOper.meOperConnCheck = NULL;
    gMibPptp80211UniOper.meOperDump = pptp_80211_uni_dump_handler;
    gMibPptp80211UniOper.meOperAlarmHandler = NULL;
    gMibPptp80211UniOper.meOperTestHandler = NULL;

	MIB_TABLE_PPTP_80211_UNI_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibPptp80211UniTblInfo, &gMibPptp80211UniOper);
    MIB_RegisterCallback(tableId, NULL, pptp_80211_uni_avc_cb);

    return GOS_OK;
}
