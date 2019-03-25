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
 * Purpose : Definition of ME handler: Circuit pack (6)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Circuit pack (6)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibCircuitPackTableInfo;
MIB_ATTR_INFO_T  gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ATTR_NUM];
MIB_TABLE_CIRCUITPACK_T gMibCircuitPackDefRow;
MIB_TABLE_OPER_T	gMibCircuitPackOper;


GOS_ERROR_CODE CircuitPackDumpMib(void* pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_CIRCUITPACK_T *pCircuitPack = (MIB_TABLE_CIRCUITPACK_T*) pData;

	OMCI_PRINT("EntityID: 0x%02x", pCircuitPack->EntityID);
	OMCI_PRINT("Type: %d", pCircuitPack->Type);
    OMCI_PRINT("NumOfPorts: %d", pCircuitPack->NumOfPorts);
	OMCI_PRINT("SerialNum: %c%c%c%c%02hhx%02hhx%02hhx%02hhx",
		pCircuitPack->SerialNum[0], pCircuitPack->SerialNum[1], pCircuitPack->SerialNum[2], pCircuitPack->SerialNum[3],
		pCircuitPack->SerialNum[4], pCircuitPack->SerialNum[5], pCircuitPack->SerialNum[6], pCircuitPack->SerialNum[7]);
    OMCI_PRINT("Version: %s", pCircuitPack->Version);
    OMCI_PRINT("VID: %s", pCircuitPack->VID);
    OMCI_PRINT("AdminState: %d", pCircuitPack->AdminState);
	OMCI_PRINT("OpState: %d", pCircuitPack->OpState);
    OMCI_PRINT("BridgedorIPInd: %d", pCircuitPack->BridgedorIPInd);
	OMCI_PRINT("EqtID: %s", pCircuitPack->EqtID);
    OMCI_PRINT("CardCfg: %d", pCircuitPack->CardCfg);
	OMCI_PRINT("NumOfTContBuff: %d", pCircuitPack->NumOfTContBuff);
	OMCI_PRINT("NumOfPriQ: %d", pCircuitPack->NumOfPriQ);
	OMCI_PRINT("NumOfScheduler: %d", pCircuitPack->NumOfScheduler);
	OMCI_PRINT("PowerShed: %d", pCircuitPack->PowerShed);

	return GOS_OK;
}

GOS_ERROR_CODE CircuitPackDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE      ret;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_CIRCUITPACK_INDEX;

    switch (operationType)
    {
        case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        default:
            ret = GOS_OK;
            break;
    }

    return ret;
}

static GOS_ERROR_CODE circuit_pack_alarm_handler(MIB_TABLE_INDEX        tableIndex,
                                                    omci_alm_data_t     alarmData,
                                                    omci_me_instance_t  *pInstanceID,
                                                    BOOL                *pIsUpdated)
{
    mib_alarm_table_t           alarmTable;
    BOOL                        isSuppressed;
    INT16                       slotId = -1;

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
        omci_is_notify_suppressed_by_circuitpack(*pInstanceID, &isSuppressed);

        if (isSuppressed)
            *pIsUpdated = FALSE;
    }

    return GOS_OK;
}

static GOS_ERROR_CODE circuit_pack_check_cb(MIB_TABLE_INDEX     tableIndex,
                                            void                *pOldRow,
                                            void                *pNewRow,
                                            MIB_ATTRS_SET       *pAttrsSet,
                                            MIB_OPERA_TYPE      operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              attrSize, i;
    MIB_ATTRS_SET       avcAttrSet;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    avcAttrSet = *pAttrsSet;

    for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
            i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
    {
        if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
            continue;

        attrSize = MIB_GetAttrSize(tableIndex, attrIndex);

        // change op state according to admin state
        if (MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX == attrIndex)
        {
            UINT8 newValue, oldValue;

            MIB_GetAttrFromBuf(tableIndex, attrIndex, &newValue, pNewRow, attrSize);
            if (pOldRow)
                MIB_GetAttrFromBuf(tableIndex, attrIndex, &oldValue, pOldRow, attrSize);

            if (!pOldRow || newValue != oldValue)
            {
                MIB_SetAttrToBuf(tableIndex, MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX, &newValue, pNewRow, attrSize);
                MIB_SetAttrSet(&avcAttrSet, MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX);
            }
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

static GOS_ERROR_CODE circuit_pack_avc_cb(MIB_TABLE_INDEX   tableIndex,
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

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(instanceID, &isSuppressed);

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

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibCircuitPackTableInfo.Name = "CircuitPack";
    gMibCircuitPackTableInfo.ShortName = "CP";
    gMibCircuitPackTableInfo.Desc = "Circuit Pack";
    gMibCircuitPackTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_CIRCUIT_PACK);
    gMibCircuitPackTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibCircuitPackTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibCircuitPackTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibCircuitPackTableInfo.pAttributes = &(gMibCircuitPackAttrInfo[0]);
	gMibCircuitPackTableInfo.attrNum = MIB_TABLE_CIRCUITPACK_ATTR_NUM;
	gMibCircuitPackTableInfo.entrySize = sizeof(MIB_TABLE_CIRCUITPACK_T);
	gMibCircuitPackTableInfo.pDefaultRow = &gMibCircuitPackDefRow;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Type";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfPorts";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SerialNum";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Version";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "VID";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AdminState";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OpState";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BridgedorIPInd";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EqtID";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CardCfg";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfTContBuff";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfPriQ";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NumOfScheduler";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PowerShed";

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Type";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Number of ports";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Serial Number";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Version";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Vendor id";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Administrative State";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operational State";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bridged or IP router";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Equipment id";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Card Configuration";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Total T-CONT buffer number";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Total Priority Queue number";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Total Traffic Scheduler number";
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Power Shed Override";

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].Len = 8;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 14;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 20;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_TYPE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPORTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_SERIALNUM_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_VID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_ADMINSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_OPSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_BRIDGEDORIPIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_EQTID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_CARDCFG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFTCONTBUFF_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFPRIQ_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_NUMOFSCHEDULER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibCircuitPackAttrInfo[MIB_TABLE_CIRCUITPACK_POWERSHED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&(gMibCircuitPackDefRow.EntityID), 0x00, sizeof(gMibCircuitPackDefRow.EntityID));
    memset(&(gMibCircuitPackDefRow.Type), 0x00, sizeof(gMibCircuitPackDefRow.Type));
    memset(&(gMibCircuitPackDefRow.NumOfPorts), 0x00, sizeof(gMibCircuitPackDefRow.NumOfPorts));
    memset(gMibCircuitPackDefRow.SerialNum, 0x20, MIB_TABLE_CIRCUITPACK_SERIALNUM_LEN);
    gMibCircuitPackDefRow.SerialNum[MIB_TABLE_CIRCUITPACK_SERIALNUM_LEN] = '\0';
    memset(gMibCircuitPackDefRow.Version, 0x20, MIB_TABLE_CIRCUITPACK_VERSION_LEN);
    gMibCircuitPackDefRow.Version[MIB_TABLE_CIRCUITPACK_VERSION_LEN] = '\0';
    memset(gMibCircuitPackDefRow.VID, 0x20, MIB_TABLE_CIRCUITPACK_VID_LEN);
    gMibCircuitPackDefRow.VID[MIB_TABLE_CIRCUITPACK_VID_LEN] = '\0';
    gMibCircuitPackDefRow.AdminState = OMCI_ME_ATTR_ADMIN_STATE_UNLOCK;
    gMibCircuitPackDefRow.OpState = OMCI_ME_ATTR_OP_STATE_ENABLED;
    memset(&(gMibCircuitPackDefRow.BridgedorIPInd), 0x00, sizeof(gMibCircuitPackDefRow.BridgedorIPInd));
    memset(gMibCircuitPackDefRow.EqtID, 0x20, MIB_TABLE_CIRCUITPACK_EQTID_LEN);
    gMibCircuitPackDefRow.EqtID[MIB_TABLE_CIRCUITPACK_EQTID_LEN] = '\0';
    memset(&(gMibCircuitPackDefRow.CardCfg), 0x00, sizeof(gMibCircuitPackDefRow.CardCfg));
    memset(&(gMibCircuitPackDefRow.NumOfTContBuff), 0x00, sizeof(gMibCircuitPackDefRow.NumOfTContBuff));
    memset(&(gMibCircuitPackDefRow.NumOfPriQ), 0x00, sizeof(gMibCircuitPackDefRow.NumOfPriQ));
    memset(&(gMibCircuitPackDefRow.NumOfScheduler), 0x00, sizeof(gMibCircuitPackDefRow.NumOfScheduler));
    memset(&(gMibCircuitPackDefRow.PowerShed), 0x00, sizeof(gMibCircuitPackDefRow.PowerShed));

    memset(&gMibCircuitPackOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibCircuitPackOper.meOperDrvCfg = CircuitPackDrvCfg;
	gMibCircuitPackOper.meOperConnCheck = NULL;
	gMibCircuitPackOper.meOperDump = CircuitPackDumpMib;
	gMibCircuitPackOper.meOperConnCfg = NULL;
    gMibCircuitPackOper.meOperAlarmHandler = circuit_pack_alarm_handler;

	MIB_TABLE_CIRCUITPACK_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibCircuitPackTableInfo,&gMibCircuitPackOper);
    MIB_RegisterCallback(tableId, circuit_pack_check_cb, circuit_pack_avc_cb);

    return GOS_OK;
}
