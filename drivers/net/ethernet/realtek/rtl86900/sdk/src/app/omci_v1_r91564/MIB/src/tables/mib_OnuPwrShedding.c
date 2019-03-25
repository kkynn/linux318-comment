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
 * Purpose : Definition of ME handler: ONU power shedding (133)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ONU power shedding (133)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T                gMibOpsTblInfo;
MIB_ATTR_INFO_T                 gMibOpsAttrInfo[MIB_TABLE_ONU_PWR_SHEDDING_ATTR_NUM];
MIB_TABLE_ONU_PWR_SHEDDING_T    gMibOpsDefRow;
MIB_TABLE_OPER_T                gMibOpsOper;


GOS_ERROR_CODE onu_pwr_shedding_drv_cfg_handler(void            *pOldRow,
                                                void            *pNewRow,
                                                MIB_OPERA_TYPE  operationType,
                                                MIB_ATTRS_SET   attrSet,
                                                UINT32          pri)
{
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_ONU_PWR_SHEDDING_T    *pOps;
    MIB_TABLE_ONU_PWR_SHEDDING_T    *pMibOps;

    tableIndex = MIB_TABLE_ONU_PWR_SHEDDING_INDEX;
    pOps = (MIB_TABLE_ONU_PWR_SHEDDING_T *)pNewRow;

    if (!mib_FindEntry(tableIndex, pOps, &pMibOps))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "cannot find onu power shedding mib entry");

        return GOS_ERR_NOT_FOUND;
    }

    switch (operationType)
    {
        case MIB_SET:
            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_PWR_SHEDDING_DATA_SHED_INTVL_INDEX))
            {
                pMibOps->DataShedIntvlRemained = pOps->DataShedIntvl;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_PWR_SHEDDING_VOICE_SHED_INTVL_INDEX))
            {
                pMibOps->VoiceShedIntvlRemained = pOps->VoiceShedIntvl;
            }

            if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_ONU_PWR_SHEDDING_RESTORE_PWR_INTVL_INDEX))
            {
                pMibOps->DataResetIntvlRemained = pOps->RestorePwrIntvl;
                pMibOps->VoiceResetIntvlRemained = pOps->RestorePwrIntvl;
            }

            break;

        default:
            break;
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;
    MIB_ATTR_INDEX  attrIdxDataShedIntvl;

    gMibOpsTblInfo.Name = "OnuPwrShedding";
    gMibOpsTblInfo.ShortName = "OPS";
    gMibOpsTblInfo.Desc = "ONU power shedding";
    gMibOpsTblInfo.ClassId = OMCI_ME_CLASS_ONU_POWER_SHEDDING;
    gMibOpsTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibOpsTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibOpsTblInfo.ActionType = OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibOpsTblInfo.pAttributes = &(gMibOpsAttrInfo[0]);
	gMibOpsTblInfo.attrNum = MIB_TABLE_ONU_PWR_SHEDDING_ATTR_NUM;
	gMibOpsTblInfo.entrySize = sizeof(MIB_TABLE_ONU_PWR_SHEDDING_T);
	gMibOpsTblInfo.pDefaultRow = &gMibOpsDefRow;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOpsAttrInfo[attrIndex].Name = "EntityId";
    gMibOpsAttrInfo[attrIndex].Desc = "Entity ID";
    gMibOpsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOpsAttrInfo[attrIndex].Len = 2;
    gMibOpsAttrInfo[attrIndex].IsIndex = TRUE;
    gMibOpsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOpsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOpsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibOpsAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_RESTORE_PWR_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOpsAttrInfo[attrIndex].Name = "RestorePwrIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Restore power timer reset interval";
    gMibOpsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOpsAttrInfo[attrIndex].Len = 2;
    gMibOpsAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOpsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOpsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOpsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOpsAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIdxDataShedIntvl = MIB_TABLE_ONU_PWR_SHEDDING_DATA_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].Name = "DataShedIntvl";
    gMibOpsAttrInfo[attrIdxDataShedIntvl].Desc = "Data class shedding interval";
    gMibOpsAttrInfo[attrIdxDataShedIntvl].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].Len = 2;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].IsIndex = FALSE;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].MibSave = TRUE;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].OutStyle = MIB_ATTR_OUT_DEC;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].AvcFlag = FALSE;
    gMibOpsAttrInfo[attrIdxDataShedIntvl].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_VOICE_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "VoiceShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Voice class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_VIDEO_OVERLAY_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "VideoOverlayShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Video overlay class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_VIDEO_RETURN_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "VideoReturnShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Video return class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_DSL_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "DslShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "DSL class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_ATM_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "AtmShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "ATM class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_CES_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "CesShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "CES class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_FRAME_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "FrameShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Frame class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_SDH_SONET_SHED_INTVL_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "SdhSonetShedIntvl";
    gMibOpsAttrInfo[attrIndex].Desc = "Sdh-sonet class shedding interval";

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_SHEDDING_STATUS_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibOpsAttrInfo[attrIndex].Name = "SheddingStatus";
    gMibOpsAttrInfo[attrIndex].Desc = "Shedding status";
    gMibOpsAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibOpsAttrInfo[attrIndex].Len = 2;
    gMibOpsAttrInfo[attrIndex].IsIndex = FALSE;
    gMibOpsAttrInfo[attrIndex].MibSave = TRUE;
    gMibOpsAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibOpsAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibOpsAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_DATA_SHED_INTVL_REMAINED_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "DataShedIntvlRemained";
    gMibOpsAttrInfo[attrIndex].Desc = "Remained data class shedding interval";
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_DATA_RESET_INTVL_REMAINED_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "DataResetIntvlRemained";
    gMibOpsAttrInfo[attrIndex].Desc = "Remained data class reset interval";
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_VOICE_SHED_INTVL_REMAINED_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "VoiceShedIntvlRemained";
    gMibOpsAttrInfo[attrIndex].Desc = "Remained voice class shedding interval";
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    attrIndex = MIB_TABLE_ONU_PWR_SHEDDING_VOICE_RESET_INTVL_REMAINED_INDEX - MIB_TABLE_FIRST_INDEX;
    memcpy(&gMibOpsAttrInfo[attrIndex], &gMibOpsAttrInfo[attrIdxDataShedIntvl], sizeof(MIB_ATTR_INFO_T));
    gMibOpsAttrInfo[attrIndex].Name = "VoiceResetIntvlRemained";
    gMibOpsAttrInfo[attrIndex].Desc = "Remained voice class reset interval";
    gMibOpsAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    memset(&gMibOpsDefRow, 0x00, sizeof(gMibOpsDefRow));

    memset(&gMibOpsOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibOpsOper.meOperDrvCfg = onu_pwr_shedding_drv_cfg_handler;
	gMibOpsOper.meOperConnCfg = NULL;
    gMibOpsOper.meOperConnCheck = NULL;
    gMibOpsOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_ONU_PWR_SHEDDING_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibOpsTblInfo, &gMibOpsOper);

    return GOS_OK;
}
