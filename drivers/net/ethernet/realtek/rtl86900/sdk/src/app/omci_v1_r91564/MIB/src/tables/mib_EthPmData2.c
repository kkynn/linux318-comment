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
 * Purpose : Definition of ME handler: Ethernet PMHD 2 (89)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Ethernet PMHD 2 (89)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/stat.h"
#endif

MIB_TABLE_INFO_T gMibEthPmData2TableInfo;
MIB_ATTR_INFO_T  gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ATTR_NUM];
MIB_TABLE_ETHPMDATA2_T gMibEthPmData2DefRow;
MIB_TABLE_OPER_T gMibEthPmData2Oper;

static UINT8    aTcaAlmNumber[MIB_TABLE_ETHPMDATA2_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_ETHPMDATA2_ATTR_NUM];


GOS_ERROR_CODE EthPmData2_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATA2_T          *pMibEthPMHD2;
    MIB_TABLE_ETHPMDATA2_T          mibCurrentBin;

    pMibEthPMHD2 = (MIB_TABLE_ETHPMDATA2_T *)pNewRow;
    mibPptpEthUNI.EntityID = pMibEthPMHD2->EntityId;

    // check if corresponding PPTP exists
    ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, MIB_GetTableEntrySize(MIB_TABLE_ETHUNI_INDEX));
    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_ETHPMDATA2_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibEthPMHD2, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthPMHD2->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            mibCurrentBin.EntityId = pMibEthPMHD2->EntityId;
            if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                return GOS_FAIL;
            }

            // sync control block data from history bin to current bin
            mibCurrentBin.ThresholdID = pMibEthPMHD2->ThresholdID;
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
            }
            break;

        default:
            break;
    }

    return ret;
}

GOS_ERROR_CODE ethernet_pmhd2_pm_handler(MIB_TABLE_INDEX        tableIndex,
                                        omci_me_instance_t      instanceID,
                                        omci_pm_oper_type_t     operType,
                                        BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHPMDATA2_T          mibEthPMHD2;
    MIB_TABLE_ETHPMDATA2_T          mibCurrentBin;
    omci_port_stat_t                portCntrs;
    omci_port_stat_t                *pPortCntrs = (omci_port_stat_t*)&portCntrs;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibPptpEthUNI.EntityID = instanceID;
    if (GOS_OK != MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), instanceID);

        return GOS_FAIL;
    }

    mibEthPMHD2.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibEthPMHD2, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    mibCurrentBin.EntityId = instanceID;
    if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);

        return GOS_FAIL;
    }

    if (OMCI_PM_OPER_RESET == operType)
    {
        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibEthPMHD2.ThresholdID;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        return GOS_OK;
    }

	if(OMCI_PM_OPER_GET_CURRENT_DATA == operType)
	{
		omci_pm_update_pptp_eth_uni(OMCI_PM_OPER_GET_CURRENT_DATA);
	}

    if (OMCI_PM_OPER_UPDATE == operType ||
		OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
		OMCI_PM_OPER_GET_CURRENT_DATA == operType)
    {
        // update PM values
        if (GOS_OK == omci_pm_getcurrent_pptp_eth_uni(instanceID,pPortCntrs))
        {
            //mibCurrentBin.PppoeFilFrm;
        }

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        ret = omci_pm_is_threshold_crossed(tableIndex, instanceID,
                &mibCurrentBin, aTcaAlmNumber, aTcaAttrMap, NULL, pIsTcaRaised);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Check threshold error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (OMCI_PM_OPER_SWAP == operType || OMCI_PM_OPER_UPDATE_AND_SWAP == operType)
    {
        // swap PM values
        mibCurrentBin.IntEndTime++;
        mibEthPMHD2.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.ThresholdID = mibEthPMHD2.ThresholdID;
        mibCurrentBin.IntEndTime = mibEthPMHD2.IntEndTime;

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibEthPmData2TableInfo.Name = "EthPmData2";
    gMibEthPmData2TableInfo.ShortName = "ETHPMHD2";
    gMibEthPmData2TableInfo.Desc = "Ethernet PM History Data 2";
    gMibEthPmData2TableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ETHERNET_PMHD_2);
    gMibEthPmData2TableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibEthPmData2TableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibEthPmData2TableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibEthPmData2TableInfo.pAttributes = &(gMibEthPmData2AttrInfo[0]);

	gMibEthPmData2TableInfo.attrNum = MIB_TABLE_ETHPMDATA2_ATTR_NUM;
	gMibEthPmData2TableInfo.entrySize = sizeof(MIB_TABLE_ETHPMDATA2_T);
	gMibEthPmData2TableInfo.pDefaultRow = &gMibEthPmData2DefRow;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntEndTime";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ThresholdID";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PppoeFilFrm";

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval End Time";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Threshold Data ID";
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PPPoE Filtered Frame Counter";

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_THRESHOLDID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthPmData2AttrInfo[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    memset(&(gMibEthPmData2DefRow.EntityId), 0x00, sizeof(gMibEthPmData2DefRow.EntityId));
    memset(&(gMibEthPmData2DefRow.IntEndTime), 0x00, sizeof(gMibEthPmData2DefRow.IntEndTime));
    memset(&(gMibEthPmData2DefRow.ThresholdID), 0x00, sizeof(gMibEthPmData2DefRow.ThresholdID));
    memset(&(gMibEthPmData2DefRow.PppoeFilFrm), 0x00, sizeof(gMibEthPmData2DefRow.PppoeFilFrm));

    memset(&gMibEthPmData2Oper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthPmData2Oper.meOperDrvCfg = EthPmData2_CfgMe;
    gMibEthPmData2Oper.meOperConnCheck = NULL;
	gMibEthPmData2Oper.meOperConnCfg = NULL;
	gMibEthPmData2Oper.meOperAvlTreeAdd = NULL;
    gMibEthPmData2Oper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibEthPmData2Oper.meOperPmHandler = ethernet_pmhd2_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX] = 0;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_ETHPMDATA2_PPPOEFILFRM_INDEX - MIB_TABLE_FIRST_INDEX] = 1;

    MIB_TABLE_ETHPMDATA2_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibEthPmData2TableInfo, &gMibEthPmData2Oper);

    return GOS_OK;
}
