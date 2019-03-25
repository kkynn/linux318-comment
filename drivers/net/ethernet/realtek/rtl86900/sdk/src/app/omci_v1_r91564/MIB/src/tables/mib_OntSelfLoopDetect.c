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
 * Purpose : Definition of ME handler: ont self loop detect (244)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ont self loop detect (244)
 */

#include "app_basic.h"

MIB_TABLE_INFO_T gMibOntSelfLoopDetectTableInfo;
MIB_ATTR_INFO_T  gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ATTR_NUM];
MIB_TABLE_ONTSELFLOOPDETECT_T gMibOntSelfLoopDetectDefRow;
MIB_TABLE_OPER_T gMibOntSelfLoopDetectOper;

GOS_ERROR_CODE OntSelfLoopDetectCfg(void            *pOldRow,
                                    void            *pNewRow,
                                    MIB_OPERA_TYPE  operationType,
                                    MIB_ATTRS_SET   attrSet,
                                    UINT32          pri)
{
    GOS_ERROR_CODE                      ret = GOS_OK;
    BOOL                                bWrite = FALSE;
    MIB_TABLE_ONTSELFLOOPDETECT_T       *pNewMib = NULL;
    MIB_TABLE_ONTSELFLOOPDETECT_T       *pOldMib = NULL;
    MIB_TABLE_INDEX                     tableIndex = MIB_TABLE_ONT_SELFLOOPDETECT_INDEX;

    pNewMib = (MIB_TABLE_ONTSELFLOOPDETECT_T *)pNewRow;
    pOldMib = (MIB_TABLE_ONTSELFLOOPDETECT_T *)pOldRow;


	switch (operationType)
	{
		case MIB_ADD:
            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;
        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX) &&
                pNewMib->EnableLoopDetect != pOldMib->EnableLoopDetect)
            {
                /* change disable to enable */
                if (pNewMib->EnableLoopDetect)
                {
                    // TBD
                }

                /* change enable to disable */
                if (!pNewMib->EnableLoopDetect)
                {
                    // TBD
                }
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX) &&
                pNewMib->DetectAction != pOldMib->DetectAction)
            {
                // TBD
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX) &&
                pNewMib->SendingPeriod != pOldMib->SendingPeriod)
            {
                // TBD
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX) &&
                pNewMib->BlockTimer != pOldMib->BlockTimer)
            {
                // TBD
            }

            break;

        case MIB_GET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX))
			{
			    bWrite = TRUE;
			}

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX))
            {
                bWrite = TRUE;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX))
            {
                bWrite = TRUE;
            }

            if (bWrite)
                MIB_Set(tableIndex, pNewMib, sizeof(MIB_TABLE_ONTSELFLOOPDETECT_T));
            break;
		default:
            break;
	}

	return ret;
}

static GOS_ERROR_CODE ont_self_loop_detect_avc_cb(MIB_TABLE_INDEX   tableIndex,
                                                    void            *pOldRow,
                                                    void            *pNewRow,
                                                    MIB_ATTRS_SET   *pAttrsSet,
                                                    MIB_OPERA_TYPE  operationType)
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

static GOS_ERROR_CODE ont_self_loop_detect_alarm_handler(MIB_TABLE_INDEX    tableIndex,
                                                        omci_alm_data_t     alarmData,
                                                        omci_me_instance_t  *pInstanceID,
                                                        BOOL                *pIsUpdated)
{
    mib_alarm_table_t   alarmTable;

    if (!pInstanceID || !pIsUpdated)
        return GOS_ERR_PARAM;

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
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibOntSelfLoopDetectTableInfo.Name = "OntSelfLoopDetect";
    gMibOntSelfLoopDetectTableInfo.ShortName = "SLD";
    gMibOntSelfLoopDetectTableInfo.Desc = "Ont Self Loop Detect";
    gMibOntSelfLoopDetectTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ONT_SELF_LOOP_DETECT);
    gMibOntSelfLoopDetectTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibOntSelfLoopDetectTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibOntSelfLoopDetectTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibOntSelfLoopDetectTableInfo.pAttributes = &(gMibOntSelfLoopDetectAttrInfo[0]);

	gMibOntSelfLoopDetectTableInfo.attrNum = MIB_TABLE_ONTSELFLOOPDETECT_ATTR_NUM;
	gMibOntSelfLoopDetectTableInfo.entrySize = sizeof(MIB_TABLE_ONTSELFLOOPDETECT_T);
	gMibOntSelfLoopDetectTableInfo.pDefaultRow = &gMibOntSelfLoopDetectDefRow;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EnableLoopDetect";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DetectAction";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "SendingPeriod";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BlockTimer";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OnuUniDetectState";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OnuUniBlockState";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnBlockReason";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UnBlockOnuUni";

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Enable loop detect";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Detect action";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Sending period";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Block timer";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Onu uni detect state";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Onu uni block state";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "UnBlock reason";
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "UnBlock onu uni";

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_WRITE;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ENABLELOOPDETECT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_DETECTACTION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_SENDINGPERIOD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_BLOCKTIMER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIDETECTSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_ONUUNIBLOCKSTATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKREASON_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibOntSelfLoopDetectAttrInfo[MIB_TABLE_ONTSELFLOOPDETECT_UNBLOCKONUUNI_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibOntSelfLoopDetectDefRow), 0x00, sizeof(gMibOntSelfLoopDetectDefRow));

    memset(&gMibOntSelfLoopDetectOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibOntSelfLoopDetectOper.meOperDrvCfg = OntSelfLoopDetectCfg;
	gMibOntSelfLoopDetectOper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibOntSelfLoopDetectOper.meOperAlarmHandler = ont_self_loop_detect_alarm_handler;

    MIB_TABLE_ONT_SELFLOOPDETECT_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibOntSelfLoopDetectTableInfo,&gMibOntSelfLoopDetectOper);
    MIB_RegisterCallback(tableId, NULL, ont_self_loop_detect_avc_cb);

    return GOS_OK;
}
