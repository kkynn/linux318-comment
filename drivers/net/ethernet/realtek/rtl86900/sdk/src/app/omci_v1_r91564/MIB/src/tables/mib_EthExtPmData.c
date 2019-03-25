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
 * Purpose : Definition of ME handler: Ethernet frame extended PM (334)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Ethernet frame extended PM (334)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibEthExtPmDataTableInfo;
MIB_ATTR_INFO_T  gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ATTR_NUM];
MIB_TABLE_ETHEXTPMDATA_T gMibEthExtPmDataDefRow;
MIB_TABLE_OPER_T gMibEthExtPmDataOper;

static UINT8    aTcaAlmNumber[MIB_TABLE_ETHEXTPMDATA_ATTR_NUM];
static UINT8    aTcaAttrMap[MIB_TABLE_ETHEXTPMDATA_ATTR_NUM];


GOS_ERROR_CODE EthExtPmData_DumpMe(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_ETHEXTPMDATA_T *pEthExtPmData = (MIB_TABLE_ETHEXTPMDATA_T*)pData;
    efepm_attr_ctrl_blk_t *pCtrlBlk = (efepm_attr_ctrl_blk_t *)pEthExtPmData->ControlBlock;

	OMCI_PRINT("EntityId: 0x%04x", pEthExtPmData->EntityId);
	OMCI_PRINT("IntEndTime: %u", pEthExtPmData->IntEndTime);
	OMCI_PRINT("ControlBlock:");
    OMCI_PRINT("\tThresholdDataID: 0x%04x", pCtrlBlk->Common.ThresholdDataID);
    OMCI_PRINT("\tParentMeClass: %u", pCtrlBlk->Common.ParentMeClass);
    OMCI_PRINT("\tParentMeInstance: 0x%04x", pCtrlBlk->Common.ParentMeInstance);
    OMCI_PRINT("\tAccumDisable: 0x%04x", pCtrlBlk->Common.AccumDisable);
    OMCI_PRINT("\tTcaDisable: 0x%04x", pCtrlBlk->Common.TcaDisable);
    OMCI_PRINT("\tCtrlFields: 0x%04x", pCtrlBlk->Common.CtrlFields);
    OMCI_PRINT("\tTCI: 0x%04x", pCtrlBlk->TCI);
    OMCI_PRINT("\tReserved: 0x%04x", pCtrlBlk->Reserved);
	OMCI_PRINT("DropEvents: %u", pEthExtPmData->DropEvents);
	OMCI_PRINT("Octets: %u", pEthExtPmData->Octets);
	OMCI_PRINT("Frames: %u", pEthExtPmData->Frames);
	OMCI_PRINT("BroadcastPackets: %u", pEthExtPmData->BroadcastPackets);
	OMCI_PRINT("MulticastPackets: %u", pEthExtPmData->MulticastPackets);
	OMCI_PRINT("CrcErrPackets: %u", pEthExtPmData->CrcErrPackets);
	OMCI_PRINT("UndersizePackets: %u", pEthExtPmData->UndersizePackets);
	OMCI_PRINT("OversizePackets: %u", pEthExtPmData->OversizePackets);
	OMCI_PRINT("Octets64: %u", pEthExtPmData->Octets64);
	OMCI_PRINT("Octets65to127: %u", pEthExtPmData->Octets65to127);
	OMCI_PRINT("Octets128to255: %u", pEthExtPmData->Octets128to255);
	OMCI_PRINT("Octets256to511: %u", pEthExtPmData->Octets256to511);
	OMCI_PRINT("Octets512to1023: %u", pEthExtPmData->Octets512to1023);
	OMCI_PRINT("Octets1024to1518: %u", pEthExtPmData->Octets1024to1518);

	return GOS_OK;
}

GOS_ERROR_CODE EthExtPmData_CfgMe(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_INDEX                 tableIndex;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHEXTPMDATA_T        *pMibEthFrameExtPmNew;
    MIB_TABLE_ETHEXTPMDATA_T        *pMibEthFrameExtPmOld;
    MIB_TABLE_ETHEXTPMDATA_T        mibCurrentBin;
    efepm_attr_ctrl_blk_t           *pMibEfepmCtrlBlkNew;
    efepm_attr_ctrl_blk_t           *pMibEfepmCtrlBlkOld;

    pMibEthFrameExtPmNew = (MIB_TABLE_ETHEXTPMDATA_T *)pNewRow;
    pMibEfepmCtrlBlkNew = (efepm_attr_ctrl_blk_t *)pMibEthFrameExtPmNew->ControlBlock;
    pMibEthFrameExtPmOld = (MIB_TABLE_ETHEXTPMDATA_T *)pNewRow;
    pMibEfepmCtrlBlkOld = (efepm_attr_ctrl_blk_t *)pMibEthFrameExtPmOld->ControlBlock;

    if (EFEPM_PARENT_ME_CLASS_MBPCD == pMibEfepmCtrlBlkNew->Common.ParentMeClass)
    {
        // check if corresponding MBPCD exists
        mibMBPCD.EntityID = pMibEfepmCtrlBlkNew->Common.ParentMeInstance;
        ret = MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

            return GOS_FAIL;
        }

        if (MBPCD_TP_TYPE_PPTP_ETH_UNI != mibMBPCD.TPType)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "TP type is not correct in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

            return GOS_FAIL;
        }

        // check if corresponding PPTP exists
        mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
        ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

            return GOS_FAIL;
        }
    }
    else if (EFEPM_PARENT_ME_CLASS_PPTP_ETH_UNI == pMibEfepmCtrlBlkNew->Common.ParentMeClass)
    {
        // check if corresponding PPTP exists
        mibPptpEthUNI.EntityID = pMibEfepmCtrlBlkNew->Common.ParentMeInstance;
        ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

            return GOS_FAIL;
        }
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME class is not supported in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHEXTPMDATA_INDEX),
            pMibEfepmCtrlBlkNew->Common.ParentMeClass);

        return GOS_FAIL;
    }

    tableIndex = MIB_TABLE_ETHEXTPMDATA_INDEX;
    entrySize = MIB_GetTableEntrySize(tableIndex);

    switch (operationType)
    {
        case MIB_ADD:
            pMibEthFrameExtPmNew->pLastHistory = malloc(sizeof(MIB_TABLE_ETHEXTPMDATA_T));
            memset(pMibEthFrameExtPmNew->pLastHistory, 0, sizeof(MIB_TABLE_ETHEXTPMDATA_T));

            if (m_omci_ext_pm_accum_all_clear(pMibEfepmCtrlBlkNew->Common.AccumDisable))
            {
                // make sure clear bit remains zero
                pMibEfepmCtrlBlkNew->Common.AccumDisable &= ~(1 << OMCI_EXT_PM_ACCUM_GLOBAL_CLEAR_BIT);
            }

            if (GOS_OK != MIB_Set(tableIndex, pMibEthFrameExtPmNew, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthFrameExtPmNew->EntityId);
            }

            // sync control block data from history bin to current bin
            if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, pMibEthFrameExtPmNew, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), pMibEthFrameExtPmNew->EntityId);
            }

            ret = mib_alarm_table_add(tableIndex, pNewRow);
            break;

        case MIB_DEL:
            if (pMibEthFrameExtPmNew->pLastHistory)
                free(pMibEthFrameExtPmNew->pLastHistory);

            ret = mib_alarm_table_del(tableIndex, pOldRow);
            break;

        case MIB_SET:
            // clear raised tca based on the tca disable
            if (memcmp(&pMibEfepmCtrlBlkNew->Common.TcaDisable,
                    &pMibEfepmCtrlBlkOld->Common.TcaDisable, sizeof(UINT16)))
            {
                if (m_omci_ext_pm_tca_all_disable(pMibEfepmCtrlBlkNew->Common.TcaDisable))
                {
                    omci_pm_clear_all_raised_tca(tableIndex, pMibEthFrameExtPmNew->EntityId);
                }
                else
                {
                    omci_pm_clear_raised_tca(tableIndex, pMibEthFrameExtPmNew->EntityId,
                        aTcaAlmNumber, aTcaAttrMap, &pMibEfepmCtrlBlkNew->Common.TcaDisable);
                }
            }

            if (m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlkNew->Common.CtrlFields) !=
                    m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlkOld->Common.CtrlFields) ||
                    m_omci_ext_pm_accum_all_clear(pMibEfepmCtrlBlkNew->Common.AccumDisable))
            {
                // make sure clear bit remains zero
                pMibEfepmCtrlBlkNew->Common.AccumDisable &= ~(1 << OMCI_EXT_PM_ACCUM_GLOBAL_CLEAR_BIT);

                // clear history bin
                memset(&mibCurrentBin, 0, entrySize);
                mibCurrentBin.EntityId = pMibEthFrameExtPmNew->EntityId;
                memcpy(mibCurrentBin.ControlBlock,
                    pMibEthFrameExtPmNew->ControlBlock, MIB_TABLE_ETHEXTPMDATA_CTRL_BLK_LEN);
                mibCurrentBin.pLastHistory = pMibEthFrameExtPmNew->pLastHistory;

                if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                        __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);
                }

                // clear last history
                memset(pMibEthFrameExtPmNew->pLastHistory, 0, entrySize);

                // clear all raised tca
                omci_pm_clear_all_raised_tca(tableIndex, mibCurrentBin.EntityId);
            }
            else
            {
                mibCurrentBin.EntityId = pMibEthFrameExtPmNew->EntityId;
                if (GOS_OK != MIB_GetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Current bin not found in %s: %s, 0x%x",
                        __FUNCTION__, MIB_GetTableName(tableIndex), mibCurrentBin.EntityId);

                    return GOS_FAIL;
                }
            }

            // sync control block data from history bin to current bin
            memcpy(mibCurrentBin.ControlBlock,
                    pMibEthFrameExtPmNew->ControlBlock, MIB_TABLE_ETHEXTPMDATA_CTRL_BLK_LEN);
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

GOS_ERROR_CODE ethernet_frame_extended_pm_handler(MIB_TABLE_INDEX           tableIndex,
                                                    omci_me_instance_t      instanceID,
                                                    omci_pm_oper_type_t     operType,
                                                    BOOL                    *pIsTcaRaised)
{
    GOS_ERROR_CODE                  ret;
    UINT32                          entrySize;
    MIB_TABLE_MACBRIPORTCFGDATA_T   mibMBPCD;
    MIB_TABLE_ETHUNI_T              mibPptpEthUNI;
    MIB_TABLE_ETHEXTPMDATA_T        mibEthFrameExtPm;
    MIB_TABLE_ETHEXTPMDATA_T        mibCurrentBin;
    MIB_TABLE_ETHEXTPMDATA_T        *pMibLastEfepm;
    efepm_attr_ctrl_blk_t           *pMibEfepmCtrlBlk;
    omci_port_stat_t                portCntrs;
    omci_port_stat_t                *pPortCntrs = (omci_port_stat_t*)&portCntrs;
    unsigned long long              tempCntr;

    if (!pIsTcaRaised)
        return GOS_ERR_PARAM;

    // get table entry size
    entrySize = MIB_GetTableEntrySize(tableIndex);

    mibEthFrameExtPm.EntityId = instanceID;
    if (GOS_OK != MIB_Get(tableIndex, &mibEthFrameExtPm, entrySize))
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

    pMibEfepmCtrlBlk = (efepm_attr_ctrl_blk_t *)mibEthFrameExtPm.ControlBlock;
    pMibLastEfepm = (MIB_TABLE_ETHEXTPMDATA_T *)mibEthFrameExtPm.pLastHistory;

    if (EFEPM_PARENT_ME_CLASS_MBPCD == pMibEfepmCtrlBlk->Common.ParentMeClass)
    {
        // check if corresponding MBPCD exists
        mibMBPCD.EntityID = pMibEfepmCtrlBlk->Common.ParentMeInstance;
        ret = MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &mibMBPCD, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

            return GOS_FAIL;
        }

        if (MBPCD_TP_TYPE_PPTP_ETH_UNI != mibMBPCD.TPType)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "TP type is not correct in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_MACBRIPORTCFGDATA_INDEX), mibMBPCD.EntityID);

            return GOS_FAIL;
        }

        // check if corresponding PPTP exists
        mibPptpEthUNI.EntityID = mibMBPCD.TPPointer;
        ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

            return GOS_FAIL;
        }
    }
    else if (EFEPM_PARENT_ME_CLASS_PPTP_ETH_UNI == pMibEfepmCtrlBlk->Common.ParentMeClass)
    {
        // check if corresponding PPTP exists
        mibPptpEthUNI.EntityID = pMibEfepmCtrlBlk->Common.ParentMeInstance;
        ret = MIB_Get(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, sizeof(MIB_TABLE_ETHUNI_T));
        if (GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHUNI_INDEX), mibPptpEthUNI.EntityID);

            return GOS_FAIL;
        }
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Parent ME class is not supported in %s: %s, 0x%x",
            __FUNCTION__, MIB_GetTableName(MIB_TABLE_ETHEXTPMDATA_INDEX),
            pMibEfepmCtrlBlk->Common.ParentMeClass);

        return GOS_FAIL;
    }

    if (OMCI_PM_OPER_RESET == operType)
    {
        if (!m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlk->Common.CtrlFields))
        {
            memset(&mibCurrentBin, 0, entrySize);
            mibCurrentBin.EntityId = instanceID;
            memcpy(mibCurrentBin.ControlBlock,
                    mibEthFrameExtPm.ControlBlock, MIB_TABLE_ETHEXTPMDATA_CTRL_BLK_LEN);
            mibCurrentBin.pLastHistory = mibEthFrameExtPm.pLastHistory;

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
    }

	if(OMCI_PM_OPER_GET_CURRENT_DATA == operType)
	{
		omci_pm_update_pptp_eth_uni(OMCI_PM_OPER_GET_CURRENT_DATA);
	}

    if (OMCI_PM_OPER_UPDATE == operType || OMCI_PM_OPER_UPDATE_AND_SWAP == operType || OMCI_PM_OPER_GET_CURRENT_DATA == operType||
            (OMCI_PM_OPER_RESET == operType  && m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlk->Common.CtrlFields)))
    {
        // update PM values if accumulation is not disabled
        if (!m_omci_ext_pm_accum_all_disable(pMibEfepmCtrlBlk->Common.AccumDisable) &&
                GOS_OK == omci_pm_getcurrent_pptp_eth_uni(mibPptpEthUNI.EntityID,pPortCntrs))
        {
            if (EFEPM_CTRL_FIELDS_DIRECTION_DOWNSTREAM ==
                    m_omci_ext_pm_directionality(pMibEfepmCtrlBlk->Common.CtrlFields))
            {
                //mibCurrentBin.DropEvents
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifOutOctets + pMibLastEfepm->Octets), mibCurrentBin.Octets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX))
                {
                    tempCntr = pPortCntrs->etherStatsTxUndersizePkts +
                                pPortCntrs->etherStatsTxPkts64Octets +
                                pPortCntrs->etherStatsTxPkts65to127Octets +
                                pPortCntrs->etherStatsTxPkts128to255Octets +
                                pPortCntrs->etherStatsTxPkts256to511Octets +
                                pPortCntrs->etherStatsTxPkts512to1023Octets +
                                pPortCntrs->etherStatsTxPkts1024to1518Octets +
                                pPortCntrs->etherStatsTxOversizePkts;
                    m_omci_pm_update_accum_attr((tempCntr + pMibLastEfepm->Frames), mibCurrentBin.Frames);
                }
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifOutBrocastPkts + pMibLastEfepm->BroadcastPackets), mibCurrentBin.BroadcastPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifOutMulticastPkts + pMibLastEfepm->MulticastPackets), mibCurrentBin.MulticastPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxCRCAlignErrors + pMibLastEfepm->CrcErrPackets), mibCurrentBin.CrcErrPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxUndersizePkts + pMibLastEfepm->UndersizePackets), mibCurrentBin.UndersizePackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxOversizePkts + pMibLastEfepm->OversizePackets), mibCurrentBin.OversizePackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts64Octets + pMibLastEfepm->Octets64), mibCurrentBin.Octets64);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts65to127Octets + pMibLastEfepm->Octets65to127), mibCurrentBin.Octets65to127);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts128to255Octets + pMibLastEfepm->Octets128to255), mibCurrentBin.Octets128to255);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts256to511Octets + pMibLastEfepm->Octets256to511), mibCurrentBin.Octets256to511);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts512to1023Octets + pMibLastEfepm->Octets512to1023), mibCurrentBin.Octets512to1023);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsTxPkts1024to1518Octets + pMibLastEfepm->Octets1024to1518), mibCurrentBin.Octets1024to1518);
            }
            else
            {
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsDropEvents + pMibLastEfepm->DropEvents), mibCurrentBin.DropEvents);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifInOctets + pMibLastEfepm->Octets), mibCurrentBin.Octets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX))
                {
                    tempCntr = pPortCntrs->etherStatsFragments +
                                pPortCntrs->etherStatsRxUndersizePkts +
                                pPortCntrs->etherStatsRxPkts64Octets +
                                pPortCntrs->etherStatsRxPkts65to127Octets +
                                pPortCntrs->etherStatsRxPkts128to255Octets +
                                pPortCntrs->etherStatsRxPkts256to511Octets +
                                pPortCntrs->etherStatsRxPkts512to1023Octets +
                                pPortCntrs->etherStatsRxPkts1024to1518Octets +
                                pPortCntrs->etherStatsRxOversizePkts +
                                pPortCntrs->etherStatsJabbers;
                    m_omci_pm_update_accum_attr((tempCntr + pMibLastEfepm->Frames), mibCurrentBin.Frames);
                }
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifInBroadcastPkts + pMibLastEfepm->BroadcastPackets), mibCurrentBin.BroadcastPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->ifInMulticastPkts + pMibLastEfepm->MulticastPackets), mibCurrentBin.MulticastPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsCRCAlignErrors + pMibLastEfepm->CrcErrPackets), mibCurrentBin.CrcErrPackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxUndersizePkts + pMibLastEfepm->UndersizePackets), mibCurrentBin.UndersizePackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxOversizePkts + pMibLastEfepm->OversizePackets), mibCurrentBin.OversizePackets);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts64Octets + pMibLastEfepm->Octets64), mibCurrentBin.Octets64);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts65to127Octets + pMibLastEfepm->Octets65to127), mibCurrentBin.Octets65to127);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts128to255Octets + pMibLastEfepm->Octets128to255), mibCurrentBin.Octets128to255);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts256to511Octets + pMibLastEfepm->Octets256to511), mibCurrentBin.Octets256to511);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts512to1023Octets + pMibLastEfepm->Octets512to1023), mibCurrentBin.Octets512to1023);
                if (!m_omci_ext_pm_accum_disable(pMibEfepmCtrlBlk->Common.AccumDisable, MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX))
                    m_omci_pm_update_accum_attr((pPortCntrs->etherStatsRxPkts1024to1518Octets + pMibLastEfepm->Octets1024to1518), mibCurrentBin.Octets1024to1518);
            }
        }

        if (GOS_OK != MIB_SetPmCurrentBin(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set current bin error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        // update to history bin as well if it's in continuous accumulation mode
        if (m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlk->Common.CtrlFields))
        {
            if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
            }
        }

        ret = omci_pm_is_threshold_crossed(tableIndex, instanceID, &mibCurrentBin,
                aTcaAlmNumber, aTcaAttrMap, &pMibEfepmCtrlBlk->Common.TcaDisable, pIsTcaRaised);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Check threshold error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }
    }

    if (OMCI_PM_OPER_SWAP == operType || OMCI_PM_OPER_UPDATE_AND_SWAP == operType ||
            (OMCI_PM_OPER_RESET == operType &&
                m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlk->Common.CtrlFields)))
    {
        if (m_omci_ext_pm_continous_accum_mode(pMibEfepmCtrlBlk->Common.CtrlFields))
        {
            // underlying statistics will be reset regardless of accumulation mode,
            // so the last values need to be remembered in order to add it back
            memcpy(pMibLastEfepm, &mibCurrentBin, entrySize);

            return GOS_OK;
        }

        // swap PM values
        mibCurrentBin.IntEndTime++;
        mibEthFrameExtPm.IntEndTime = mibCurrentBin.IntEndTime;

        if (GOS_OK != MIB_Set(tableIndex, &mibCurrentBin, entrySize))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Set instance error in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(tableIndex), instanceID);
        }

        memset(&mibCurrentBin, 0, entrySize);
        mibCurrentBin.EntityId = instanceID;
        mibCurrentBin.IntEndTime = mibEthFrameExtPm.IntEndTime;
        memcpy(mibCurrentBin.ControlBlock,
                mibEthFrameExtPm.ControlBlock, MIB_TABLE_ETHEXTPMDATA_CTRL_BLK_LEN);
        mibCurrentBin.pLastHistory = mibEthFrameExtPm.pLastHistory;

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
    gMibEthExtPmDataTableInfo.Name = "EthExtPmData";
    gMibEthExtPmDataTableInfo.ShortName = "ETHFRAMEEXTPM";
    gMibEthExtPmDataTableInfo.Desc = "Ethernet frame extended PM";
    gMibEthExtPmDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_ETHERNET_FRAME_EXTENDED_PM);
    gMibEthExtPmDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibEthExtPmDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD_PM);
    gMibEthExtPmDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_CURRENT_DATA);
    gMibEthExtPmDataTableInfo.pAttributes = &(gMibEthExtPmDataAttrInfo[0]);

	gMibEthExtPmDataTableInfo.attrNum = MIB_TABLE_ETHEXTPMDATA_ATTR_NUM;
	gMibEthExtPmDataTableInfo.entrySize = sizeof(MIB_TABLE_ETHEXTPMDATA_T);
	gMibEthExtPmDataTableInfo.pDefaultRow = &gMibEthExtPmDataDefRow;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IntEndTime";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ControlBlock";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "DropEvents";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Frames";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BroadcastPackets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "MulticastPackets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "CrcErrPackets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "UndersizePackets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OversizePackets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets64";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets65to127";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets128to255";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets256to511";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets512to1023";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Octets1024to1518";

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Interval End Time";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Control Block";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Drop Events";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of Octets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Frames";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of broadcast packets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of multicast packets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "CRC errored packets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of undersize packets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The total number of oversize packets";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 64 octets long";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 65..127 octets long";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 128..255 octets long";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 256..511 octets long";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 512..1023 octets long";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "The number of packets that were 1024..1518 octets long";

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].Len = 16;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_INTENDTIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CONTROLBLOCK_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_FRAMES_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_BROADCASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_MULTICASTPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS64_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS65TO127_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS128TO255_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS256TO511_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS512TO1023_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_OCTETS1024TO1518_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = (OMCI_ME_ATTR_TYPE_MANDATORY | OMCI_ME_ATTR_TYPE_PM);

    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].Name = "LastHistoryPointer";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].Desc = "Last History Pointer";
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibEthExtPmDataAttrInfo[MIB_TABLE_ETHEXTPMDATA_LAST_HISTORY_PTR_INDEX-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    memset(&(gMibEthExtPmDataDefRow.EntityId), 0x00, sizeof(gMibEthExtPmDataDefRow.EntityId));
    memset(&(gMibEthExtPmDataDefRow.IntEndTime), 0x00, sizeof(gMibEthExtPmDataDefRow.IntEndTime));
    memset(gMibEthExtPmDataDefRow.ControlBlock, 0x00, MIB_TABLE_ETHEXTPMDATA_CTRL_BLK_LEN);
    memset(&(gMibEthExtPmDataDefRow.DropEvents), 0x00, sizeof(gMibEthExtPmDataDefRow.DropEvents));
    memset(&(gMibEthExtPmDataDefRow.Octets), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets));
    memset(&(gMibEthExtPmDataDefRow.Frames), 0x00, sizeof(gMibEthExtPmDataDefRow.Frames));
    memset(&(gMibEthExtPmDataDefRow.BroadcastPackets), 0x00, sizeof(gMibEthExtPmDataDefRow.BroadcastPackets));
    memset(&(gMibEthExtPmDataDefRow.MulticastPackets), 0x00, sizeof(gMibEthExtPmDataDefRow.MulticastPackets));
    memset(&(gMibEthExtPmDataDefRow.CrcErrPackets), 0x00, sizeof(gMibEthExtPmDataDefRow.CrcErrPackets));
    memset(&(gMibEthExtPmDataDefRow.UndersizePackets), 0x00, sizeof(gMibEthExtPmDataDefRow.UndersizePackets));
    memset(&(gMibEthExtPmDataDefRow.OversizePackets), 0x00, sizeof(gMibEthExtPmDataDefRow.OversizePackets));
    memset(&(gMibEthExtPmDataDefRow.Octets64), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets64));
    memset(&(gMibEthExtPmDataDefRow.Octets65to127), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets65to127));
    memset(&(gMibEthExtPmDataDefRow.Octets128to255), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets128to255));
    memset(&(gMibEthExtPmDataDefRow.Octets256to511), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets256to511));
    memset(&(gMibEthExtPmDataDefRow.Octets512to1023), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets512to1023));
    memset(&(gMibEthExtPmDataDefRow.Octets1024to1518), 0x00, sizeof(gMibEthExtPmDataDefRow.Octets1024to1518));
    gMibEthExtPmDataDefRow.pLastHistory = NULL;

    memset(&gMibEthExtPmDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibEthExtPmDataOper.meOperDrvCfg = EthExtPmData_CfgMe;
    gMibEthExtPmDataOper.meOperConnCheck = NULL;
	gMibEthExtPmDataOper.meOperConnCfg = NULL;
	gMibEthExtPmDataOper.meOperAvlTreeAdd = NULL;
    gMibEthExtPmDataOper.meOperDump = EthExtPmData_DumpMe;
    gMibEthExtPmDataOper.meOperPmHandler = ethernet_frame_extended_pm_handler;

    // for PM ME only, indicates the Threshold value attribute mapping and alarm number
    memset(aTcaAlmNumber, 0, sizeof(aTcaAlmNumber));
    aTcaAlmNumber[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAlmNumber[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAlmNumber[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAlmNumber[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;
    memset(aTcaAttrMap, 0, sizeof(aTcaAttrMap));
    aTcaAttrMap[MIB_TABLE_ETHEXTPMDATA_DROPEVENTS_INDEX - MIB_TABLE_FIRST_INDEX] = 1;
    aTcaAttrMap[MIB_TABLE_ETHEXTPMDATA_CRCERRPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 2;
    aTcaAttrMap[MIB_TABLE_ETHEXTPMDATA_UNDERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 3;
    aTcaAttrMap[MIB_TABLE_ETHEXTPMDATA_OVERSIZEPACKETS_INDEX - MIB_TABLE_FIRST_INDEX] = 4;

    MIB_TABLE_ETHEXTPMDATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibEthExtPmDataTableInfo, &gMibEthExtPmDataOper);

    return GOS_OK;
}
