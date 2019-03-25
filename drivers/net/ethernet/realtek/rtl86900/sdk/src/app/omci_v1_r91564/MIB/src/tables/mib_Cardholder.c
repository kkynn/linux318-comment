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
 * Purpose : Definition of ME handler: Cardholder (5)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Cardholder (5)
 */

#include "app_basic.h"
#include "feature_mgmt.h"


MIB_TABLE_INFO_T gMibCardholderTableInfo;
MIB_ATTR_INFO_T  gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ATTR_NUM];
MIB_TABLE_CARDHOLDER_T gMibCardholderDefRow;
MIB_TABLE_OPER_T gMibCardholderOper;


GOS_ERROR_CODE CardholderDrvCfg(void            *pOldRow,
                                void            *pNewRow,
                                MIB_OPERA_TYPE  operationType,
                                MIB_ATTRS_SET   attrSet,
                                UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_CARDHOLDER_INDEX;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_CARDHOLDER_ARC_INDEX))
            {
                ret = omci_arc_timer_processor(tableIndex,
                    pOldRow, pNewRow, MIB_TABLE_CARDHOLDER_ARC_INDEX, MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX);
            }
            break;
        default:
            break;
    }

    return ret;
}

static GOS_ERROR_CODE cardholder_alarm_handler(MIB_TABLE_INDEX      tableIndex,
                                                omci_alm_data_t     alarmData,
                                                omci_me_instance_t  *pInstanceID,
                                                BOOL                *pIsUpdated)
{
    mib_alarm_table_t       alarmTable;
    BOOL                    isSuppressed;
    MIB_ENTRY_T             *pEntry;
    MIB_TABLE_CARDHOLDER_T  *pMibCardholder;
    INT16                   slotId = -1;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    slotId = alarmData.almDetail & 0xFF;

    if (!m_omci_is_slot_id_valid(slotId))
        return GOS_ERR_PARAM;

    *pInstanceID = TXC_CARDHLD_SLOT_TYPE_INTEGRATED | slotId;

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
        else
        {
            // find the entry data by using the instanceID
            pEntry = mib_FindEntryByInstanceID(tableIndex, *pInstanceID);
            if (!pEntry)
                return GOS_FAIL;
            pMibCardholder = pEntry->pData;
            if (!pMibCardholder)
                return GOS_FAIL;

            // check if notifications are suppressed by ARC
            if (OMCI_ME_ATTR_ARC_ENABLED == pMibCardholder->ARC)
                *pIsUpdated = FALSE;
        }
    }

    return GOS_OK;
}

static GOS_ERROR_CODE cardholder_avc_cb(MIB_TABLE_INDEX     tableIndex,
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

            MIB_GetAttrSize(tableIndex, attrIndex);
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}


static GOS_ERROR_CODE cardholderPreCheck(MIB_TABLE_INDEX	tableIndex,
                                             void*           	pOldRow,
                                             void*           	pNewRow,
                                             MIB_ATTRS_SET*  	pAttrsSet,
                                             MIB_OPERA_TYPE  	operationType)
{
	MIB_TABLE_CARDHOLDER_T *pMacBriPortCfgData;

	pMacBriPortCfgData = (MIB_TABLE_CARDHOLDER_T *)pNewRow;

	switch (operationType)
    {
    	/*Check attributes*/
		case MIB_SET:
			if (MIB_IsInAttrSet(pAttrsSet, MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX))
			{
				if(!OMCI_IS_PLUGIN_UNIT_TYPE_VALID(pMacBriPortCfgData->ExpectedType))
				{
					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "ExpectedType of CardHolder invalid %x", pMacBriPortCfgData->ExpectedType);
					return GOS_ERR_INVALID_INPUT;
				}
			}
			break;
		default:
			break;
	}
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibCardholderTableInfo.Name = "Cardholder";
    gMibCardholderTableInfo.ShortName = "CH";
    gMibCardholderTableInfo.Desc = "Cardholder";
    gMibCardholderTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_CARDHOLDER);
    gMibCardholderTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibCardholderTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibCardholderTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibCardholderTableInfo.pAttributes = &(gMibCardholderAttrInfo[0]);
	gMibCardholderTableInfo.attrNum = MIB_TABLE_CARDHOLDER_ATTR_NUM;
	gMibCardholderTableInfo.entrySize = sizeof(MIB_TABLE_CARDHOLDER_T);
	gMibCardholderTableInfo.pDefaultRow = &gMibCardholderDefRow;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ActualType";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ExpectedType";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ExpectedPortCount";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ExpectedEqtID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ActualEqtID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ProtPrfPtr";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "InvokeProtSwitch";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ARC";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ArcInterval";

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Actual Plug-in unit Type";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Expected Plug-in unit Type";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Expected port count";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Expected equipment ID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Actual equipment ID";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Protection profile pointer";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Invoke protection switch";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARC";
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Arc interval";

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 20;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 20;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDTYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDPORTCOUNT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALEQTID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_PROTPRFPTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_INVOKEPROTSWITCH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ARC_INTERVAL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibCardholderDefRow.EntityID), 0x00, sizeof(gMibCardholderDefRow.EntityID));
    memset(&(gMibCardholderDefRow.ActualType), 0x00, sizeof(gMibCardholderDefRow.ActualType));
    memset(&(gMibCardholderDefRow.ExpectedType), 0x00, sizeof(gMibCardholderDefRow.ExpectedType));
    memset(&(gMibCardholderDefRow.ExpectedPortCount), 0x00, sizeof(gMibCardholderDefRow.ExpectedPortCount));
    memset(gMibCardholderDefRow.ExpectedEqtID, 0x20, MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_LEN);
    gMibCardholderDefRow.ExpectedEqtID[MIB_TABLE_CARDHOLDER_EXPECTEDEQTID_LEN] = '\0';
    memset(gMibCardholderDefRow.ActualEqtID, 0x20, MIB_TABLE_CARDHOLDER_ACTUALEQTID_LEN);
    gMibCardholderDefRow.ActualEqtID[MIB_TABLE_CARDHOLDER_ACTUALEQTID_LEN] = '\0';
    memset(&(gMibCardholderDefRow.ProtPrfPtr), 0x00, sizeof(gMibCardholderDefRow.ProtPrfPtr));
    memset(&(gMibCardholderDefRow.InvokeProtSwitch), 0x00, sizeof(gMibCardholderDefRow.InvokeProtSwitch));
    memset(&(gMibCardholderDefRow.ARC), 0x00, sizeof(gMibCardholderDefRow.ARC));
    memset(&(gMibCardholderDefRow.ArcInterval), 0x00, sizeof(gMibCardholderDefRow.ArcInterval));

    memset(&gMibCardholderOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibCardholderOper.meOperDrvCfg = CardholderDrvCfg;
	gMibCardholderOper.meOperConnCheck = NULL;
	gMibCardholderOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibCardholderOper.meOperConnCfg = NULL;
    gMibCardholderOper.meOperAlarmHandler = cardholder_alarm_handler;

	MIB_TABLE_CARDHOLDER_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibCardholderTableInfo,&gMibCardholderOper);
    MIB_RegisterCallback(tableId, cardholderPreCheck, cardholder_avc_cb);

	feature_api(FEATURE_API_ME_00008000, &gMibCardholderAttrInfo[MIB_TABLE_CARDHOLDER_ACTUALTYPE_INDEX - MIB_TABLE_FIRST_INDEX]);

    return GOS_OK;
}
