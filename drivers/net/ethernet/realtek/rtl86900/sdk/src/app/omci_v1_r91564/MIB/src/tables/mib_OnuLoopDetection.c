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
 * Purpose : Definition of ME handler: CTC ONU LOOP DETECTION (65528)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) CTC ONU LOOP DETECTION (65528)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "common/error.h"
#include "rtk/port.h"
#endif

MIB_TABLE_INFO_T gMibOnuLoopDetectionTableInfo;
MIB_ATTR_INFO_T  gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTIONION_ATTR_NUM];
MIB_TABLE_ONU_LOOP_DETECTION_T gMibOnuLoopDetectionDefRow;
MIB_TABLE_OPER_T gMibOnuLoopDetectionOper;

static GOS_ERROR_CODE
OnuLpDetn_attr_lpDetnMng_set(
    MIB_TABLE_ONU_LOOP_DETECTION_T *pMibOnuLpDetn)
{
    GOS_ERROR_CODE ret = GOS_FAIL;

    printf("%s: value %u\n", __FUNCTION__, pMibOnuLpDetn->LpDetnMng);

    return ret;
}

static GOS_ERROR_CODE
OnuLpDetn_attr_lpPortDown_set(
    MIB_TABLE_ONU_LOOP_DETECTION_T *pMibOnuLpDetn)
{
    GOS_ERROR_CODE ret = GOS_FAIL;

    if (!pMibOnuLpDetn)
    {
        printf("%s: Param is the NULL pointer\n",__FUNCTION__);
        goto ERROR_LPPORTDOWNSET;
    }
    
    if (pMibOnuLpDetn->LpPortDown > END_LOOPEDPORT_DOWN)
    {
        printf(
            "%s: Mode %u of LoopedPortDown isn't supported\n",
            __FUNCTION__,
            pMibOnuLpDetn->LpPortDown);
        goto ERROR_LPPORTDOWNSET;
    }

    printf("%s: value %u\n", __FUNCTION__, pMibOnuLpDetn->LpPortDown);

ERROR_LPPORTDOWNSET:
    return ret;
}

static GOS_ERROR_CODE
OnuLpDetn_attr_lpDetnMsgFreq_set(
    MIB_TABLE_ONU_LOOP_DETECTION_T *pMibOnuLpDetn)
{
    GOS_ERROR_CODE ret = GOS_FAIL;

    if (!pMibOnuLpDetn)
    {
        printf("%s: Param is the NULL pointer\n",__FUNCTION__);
        goto ERROR_LPDETNMSGFREQSET;
    }
    
    if (pMibOnuLpDetn->LpDetnMsgFreq < 1 || pMibOnuLpDetn->LpDetnMsgFreq > 65535)
    {
        printf(
            "%s: Freq %u pps is out of range(1 ~ 65535)\n",
            __FUNCTION__,
            pMibOnuLpDetn->LpDetnMsgFreq);
        goto ERROR_LPDETNMSGFREQSET;
    }

    printf("%s: value %u\n", __FUNCTION__,  pMibOnuLpDetn->LpDetnMsgFreq);

ERROR_LPDETNMSGFREQSET:
    return ret;
}

static GOS_ERROR_CODE
OnuLpDetn_attr_lpRecovIntvl_set(
    MIB_TABLE_ONU_LOOP_DETECTION_T *pMibOnuLpDetn)
{
    GOS_ERROR_CODE ret = GOS_FAIL;

    if (!pMibOnuLpDetn)
    {
        printf("%s: Param is the NULL pointer\n",__FUNCTION__);
        goto ERROR_LPRECOVINTVLSET;
    }

    printf("%s: value %u\n", __FUNCTION__, pMibOnuLpDetn->LpRecovIntvl);

ERROR_LPRECOVINTVLSET:
    return ret;
}

static GOS_ERROR_CODE
OnuLpDetn_attr_PortAndVlanTable_set(
    MIB_TABLE_ONU_LOOP_DETECTION_T *pMibOnuLpDetn)
{
    GOS_ERROR_CODE ret = GOS_FAIL;
    omci_portAndVlan_row_entry_t* rawEntry = NULL;

    if (!pMibOnuLpDetn)
    {
        printf("%s: Param is the NULL pointer\n",__FUNCTION__);
        goto ERROR_PORTANDVLANTABLESET;
    }

    rawEntry = 
        (omci_portAndVlan_row_entry_t*) malloc (sizeof(omci_portAndVlan_row_entry_t));
    if (!rawEntry)
    {
        printf("%s: Failed to allocate memory for rawEntry\n",__FUNCTION__);
        goto ERROR_PORTANDVLANTABLESET;
    }
    
    memcpy(
        rawEntry, 
        pMibOnuLpDetn->PortAndVlanTable, 
        sizeof(char) * MIB_TBL_ONULOOPDETN_ATTR_PORTANDVLANTABLE_LEN);

    printf("%s\n", __FUNCTION__);
    printf("Idx: %u\n", rawEntry->TblIdx);
    printf("MeId: 0x%x\n", rawEntry->MeID);
    printf("Svlan: %u\n", rawEntry->SVLAN);
    printf("Cvlan: %u\n", rawEntry->CVLAN);

    free ( rawEntry );
    rawEntry = NULL;

ERROR_PORTANDVLANTABLESET:
    return ret;
}

GOS_ERROR_CODE
OnuLoopDetectionDrvCfg(
    void *pOldRow,
    void *pNewRow,
    MIB_OPERA_TYPE  operationType,
    MIB_ATTRS_SET   attrSet,
    UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_INDEX     tableIndex = MIB_TABLE_ONULOOPDETECTION_INDEX;
    omci_me_instance_t  instanceID;
    //
    // read out the instanceID
    //
    MIB_GetAttrFromBuf(
        tableIndex,
        MIB_ATTR_FIRST_INDEX,
        &instanceID,
        pNewRow,
        sizeof(omci_me_instance_t));

    switch (operationType)
    {
    case MIB_ADD:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "CTC ONU Loop Detection ---- > ADD");
        OnuLpDetn_attr_lpDetnMng_set(pNewRow);
        OnuLpDetn_attr_lpPortDown_set(pNewRow);
        OnuLpDetn_attr_lpDetnMsgFreq_set(pNewRow);
        OnuLpDetn_attr_lpRecovIntvl_set(pNewRow);
        OnuLpDetn_attr_PortAndVlanTable_set(pNewRow);

        ret = mib_alarm_table_add(tableIndex, pNewRow);

        break;

    case MIB_DEL:
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "CTC ONU Loop Detection ---- > DEL");

        ret = mib_alarm_table_del(tableIndex, pOldRow);
        break;

    case MIB_SET:
        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX))
        {
            OnuLpDetn_attr_lpDetnMng_set(pNewRow);
        }

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX))
        {
            OnuLpDetn_attr_lpPortDown_set(pNewRow);
        }

        if (MIB_IsInAttrSet(
                &attrSet,
                MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX))
        {
            OnuLpDetn_attr_lpDetnMsgFreq_set(pNewRow);
        }

        if (MIB_IsInAttrSet(
                &attrSet,
                MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX))
        {
            OnuLpDetn_attr_lpRecovIntvl_set(pNewRow);
        }

        if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX))
        {
            OnuLpDetn_attr_PortAndVlanTable_set(pNewRow);
        }
        break;

    default:
        break;
    }

    return ret;
}

static GOS_ERROR_CODE
OnuLpDetn_alarm_handler(
    MIB_TABLE_INDEX     tableIndex,
    omci_alm_data_t     alarmData,
    omci_me_instance_t *pInstanceID,
    BOOL *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;
    BOOL                isSuppressed;

    if (!pInstanceID || !pIsUpdated)
    {
        return GOS_ERR_PARAM;
    }

    *pIsUpdated = FALSE;

    // extract instanceID from alarm detail
    *pInstanceID = alarmData.almDetail;

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

        if (isSuppressed) *pIsUpdated = FALSE;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
mibTable_init(
    MIB_TABLE_INDEX tableId)
{
    gMibOnuLoopDetectionTableInfo.Name = "CTCOnuOnuLoopDetection";
    gMibOnuLoopDetectionTableInfo.ShortName = "CTCLD";
    gMibOnuLoopDetectionTableInfo.Desc = "CTC Loop Detection";
    gMibOnuLoopDetectionTableInfo.ClassId = OMCI_ME_CLASS_CTC_ONU_LOOP_DETECTION;
    gMibOnuLoopDetectionTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOnuLoopDetectionTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibOnuLoopDetectionTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibOnuLoopDetectionTableInfo.pAttributes = &(gMibOnuLoopDetectionAttrInfo[0]);

    gMibOnuLoopDetectionTableInfo.attrNum = MIB_TABLE_ONU_LOOP_DETECTIONION_ATTR_NUM;
    gMibOnuLoopDetectionTableInfo.entrySize = sizeof(MIB_TABLE_ONU_LOOP_DETECTION_T);
    gMibOnuLoopDetectionTableInfo.pDefaultRow = &gMibOnuLoopDetectionDefRow;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OperatorID";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LoopDetnMng";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LoopedPortDown";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LoopDetnMsgFreq";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LoopRecovIntvl";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortAndVlanTbl";

    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "OperatorID";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LoopDetnMng";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LoopedPortDown";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LoopDetnMsgFreq";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LoopRecovIntvl";
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PortAndVlanTbl";

    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 7;

    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;


    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_OPERATORID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMNG_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPEDPORTDOWN_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPDETECTIONMSGFREQ_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_LOOPRECOVERYINTVL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOnuLoopDetectionAttrInfo[MIB_TABLE_ONU_LOOP_DETECTION_PORTANDVLANTABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibOnuLoopDetectionDefRow.EntityID), 0x00, sizeof(gMibOnuLoopDetectionDefRow.EntityID));
    memcpy(&gMibOnuLoopDetectionDefRow.OperatorID, "CTC", sizeof(char) * 3);
    gMibOnuLoopDetectionDefRow.LpDetnMng = END_ONU_LOOP_DETECTIONMNG;
    gMibOnuLoopDetectionDefRow.LpPortDown = END_LOOPEDPORT_DOWN;
    gMibOnuLoopDetectionDefRow.LpDetnMsgFreq = 8;
    gMibOnuLoopDetectionDefRow.LpRecovIntvl  = 300;
    memset(gMibOnuLoopDetectionDefRow.PortAndVlanTable, 0x00, sizeof(char) * 7);

    memset(&gMibOnuLoopDetectionOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibOnuLoopDetectionOper.meOperDrvCfg = OnuLoopDetectionDrvCfg;
    gMibOnuLoopDetectionOper.meOperConnCheck = NULL;
    gMibOnuLoopDetectionOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibOnuLoopDetectionOper.meOperConnCfg = NULL;
    gMibOnuLoopDetectionOper.meOperAlarmHandler = OnuLpDetn_alarm_handler;

    MIB_TABLE_ONULOOPDETECTION_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibOnuLoopDetectionTableInfo, &gMibOnuLoopDetectionOper);

    return GOS_OK;
}
