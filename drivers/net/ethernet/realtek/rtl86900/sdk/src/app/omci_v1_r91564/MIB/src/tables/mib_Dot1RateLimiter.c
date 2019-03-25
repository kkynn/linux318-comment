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
 * Purpose : Definition of ME handler: Dot1 rate limiter (298)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Dot1 rate limiter (298)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T                gMibDrlTblInfo;
MIB_ATTR_INFO_T                 gMibDrlAttrInfo[MIB_TABLE_DOT1_RATE_LIMITER_ATTR_NUM];
MIB_TABLE_DOT1_RATE_LIMITER_T   gMibDrlDefRow;
MIB_TABLE_OPER_T                gMibDrlOper;


GOS_ERROR_CODE dot1_rate_limiter_drv_cfg_handler(void           *pOldRow,
                                                void            *pNewRow,
                                                MIB_OPERA_TYPE  operationType,
                                                MIB_ATTRS_SET   attrSet,
                                                UINT32          pri)
{
    GOS_ERROR_CODE                  ret;
    MIB_TABLE_DOT1_RATE_LIMITER_T   *pNewDrl;
    MIB_TABLE_DOT1_RATE_LIMITER_T   *pOldDrl;
    MIB_TABLE_TRAFFICDESCRIPTOR_T   mibTd;
    omci_dot1_rate_t                dot1Rate;

    pNewDrl = (MIB_TABLE_DOT1_RATE_LIMITER_T *)pNewRow;
    pOldDrl = (MIB_TABLE_DOT1_RATE_LIMITER_T *)pOldRow;

    if (DOT1_RATE_LIMITER_TP_TYPE_IEEE_8021P_MAPPER == pNewDrl->TpType)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "rate limiter for 802.1p mapper is not supported");

        return GOS_ERR_NOTSUPPORT;
    }

    if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode)
    {
        if (GOS_OK != omci_get_pptp_eth_uni_port_mask_in_bridge(pNewDrl->ParentMePtr, &dot1Rate.portMask))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get pptp eth uni port mask in bridge fail");

            return GOS_FAIL;
        }
    }
    else
    {
        if (GOS_OK != omci_get_eth_uni_port_mask_behind_veip(pNewDrl->ParentMePtr, &dot1Rate.portMask))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "get eth uni port mask behind veip fail");

            return GOS_FAIL;
        }
    }

    if (0 == dot1Rate.portMask)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "no associated pptp eth uni is found");

        return GOS_OK;
    }

    // deal with unicast flood
    dot1Rate.type = OMCI_DOT1_RATE_UNICAST_FLOOD;

    ret = omci_is_traffic_descriptor_existed(pNewDrl->UsUcFloodRatePtr, &mibTd);
    if (GOS_OK == ret && MIB_DEL != operationType)
    {
        if (MIB_ADD == operationType || (MIB_SET == operationType &&
                pNewDrl->UsUcFloodRatePtr != pOldDrl->UsUcFloodRatePtr))
        {
            dot1Rate.cir = mibTd.CIR;
            dot1Rate.cbs = mibTd.CBS;

            ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for uc flood fail");

                return GOS_FAIL;
            }
        }
    }
    else
    {
        ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for uc flood fail");

            return GOS_FAIL;
        }
    }

    // deal with broadcast
    dot1Rate.type = OMCI_DOT1_RATE_BROADCAST;

    ret = omci_is_traffic_descriptor_existed(pNewDrl->UsBcRatePtr, &mibTd);
    if (GOS_OK == ret && MIB_DEL != operationType)
    {
        if (MIB_ADD == operationType || (MIB_SET == operationType &&
                pNewDrl->UsBcRatePtr != pOldDrl->UsBcRatePtr))
        {
            dot1Rate.cir = mibTd.CIR;
            dot1Rate.cbs = mibTd.CBS;

            ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for bc fail");

                return GOS_FAIL;
            }
        }
    }
    else
    {
        ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for bc fail");

            return GOS_FAIL;
        }
    }

    // deal with multicast payload
    dot1Rate.type = OMCI_DOT1_RATE_MULTICAST_PAYLOAD;

    ret = omci_is_traffic_descriptor_existed(pNewDrl->UsMcPayloadRatePtr, &mibTd);
    if (GOS_OK == ret && MIB_DEL != operationType)
    {
        if (MIB_ADD == operationType || (MIB_SET == operationType &&
                pNewDrl->UsMcPayloadRatePtr != pOldDrl->UsMcPayloadRatePtr))
        {
            dot1Rate.cir = mibTd.CIR;
            dot1Rate.cbs = mibTd.CBS;

            ret = omci_wrapper_setDot1RateLimiter(&dot1Rate);
            if (GOS_OK != ret)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set dot1 rate limiter for mc payload fail");

                return GOS_FAIL;
            }
        }
    }
    else
    {
        ret = omci_wrapper_delDot1RateLimiter(&dot1Rate);
        if (GOS_OK != ret && GOS_ERR_NOT_FOUND != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del dot1 rate limiter for mc payload fail");

            return GOS_FAIL;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;

    gMibDrlTblInfo.Name = "Dot1RateLimiter";
    gMibDrlTblInfo.ShortName = "DRL";
    gMibDrlTblInfo.Desc = "Dot1 rate limiter";
    gMibDrlTblInfo.ClassId = OMCI_ME_CLASS_DOT1_RATE_LIMITER;
    gMibDrlTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibDrlTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibDrlTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibDrlTblInfo.pAttributes = &(gMibDrlAttrInfo[0]);
	gMibDrlTblInfo.attrNum = MIB_TABLE_DOT1_RATE_LIMITER_ATTR_NUM;
	gMibDrlTblInfo.entrySize = sizeof(MIB_TABLE_DOT1_RATE_LIMITER_T);
	gMibDrlTblInfo.pDefaultRow = &gMibDrlDefRow;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "EntityId";
    gMibDrlAttrInfo[attrIndex].Desc = "Entity ID";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibDrlAttrInfo[attrIndex].Len = 2;
    gMibDrlAttrInfo[attrIndex].IsIndex = TRUE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibDrlAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_PARENT_ME_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "ParentMePtr";
    gMibDrlAttrInfo[attrIndex].Desc = "Parent ME pointer";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibDrlAttrInfo[attrIndex].Len = 2;
    gMibDrlAttrInfo[attrIndex].IsIndex = FALSE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibDrlAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_TP_TYPE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "TpType";
    gMibDrlAttrInfo[attrIndex].Desc = "TP type";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibDrlAttrInfo[attrIndex].Len = 1;
    gMibDrlAttrInfo[attrIndex].IsIndex = FALSE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibDrlAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_US_UC_FLOOD_RATE_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "UsUcFloodRatePtr";
    gMibDrlAttrInfo[attrIndex].Desc = "Upstream unicast flood rate pointer";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibDrlAttrInfo[attrIndex].Len = 2;
    gMibDrlAttrInfo[attrIndex].IsIndex = FALSE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibDrlAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_US_BC_RATE_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "UsBcRatePtr";
    gMibDrlAttrInfo[attrIndex].Desc = "Upstream broadcast rate pointer";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibDrlAttrInfo[attrIndex].Len = 2;
    gMibDrlAttrInfo[attrIndex].IsIndex = FALSE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibDrlAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    attrIndex = MIB_TABLE_DOT1_RATE_LIMITER_US_MC_PAYLOAD_RATE_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibDrlAttrInfo[attrIndex].Name = "UsMcPayloadRatePtr";
    gMibDrlAttrInfo[attrIndex].Desc = "Upstream multicast payload rate pointer";
    gMibDrlAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibDrlAttrInfo[attrIndex].Len = 2;
    gMibDrlAttrInfo[attrIndex].IsIndex = FALSE;
    gMibDrlAttrInfo[attrIndex].MibSave = TRUE;
    gMibDrlAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibDrlAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibDrlAttrInfo[attrIndex].AvcFlag = TRUE;
    gMibDrlAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_OPTIONAL;

    memset(&gMibDrlDefRow, 0x00, sizeof(gMibDrlDefRow));
    memset(&gMibDrlOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibDrlOper.meOperDrvCfg = dot1_rate_limiter_drv_cfg_handler;
	gMibDrlOper.meOperConnCfg = NULL;
    gMibDrlOper.meOperConnCheck = NULL;
    gMibDrlOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_DOT1_RATE_LIMITER_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibDrlTblInfo, &gMibDrlOper);

    return GOS_OK;
}
