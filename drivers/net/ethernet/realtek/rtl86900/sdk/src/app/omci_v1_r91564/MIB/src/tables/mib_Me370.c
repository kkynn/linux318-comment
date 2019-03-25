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
 * Purpose : Definition of ME handler: ME 370 get weight per egress queue of uni port (370)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: me 370 (370)
 */

#include "app_basic.h"
#include "feature_mgmt.h"

/* Table private pon queue number per tcont configuration attribute index */
#define MIB_TABLE_ME370_ATTR_NUM            (4)
#define MIB_TABLE_ME370_ENTITYID_INDEX      ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_ME370_FIRST_ATTR		    ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_ME370_SECOND_ATTR		    ((MIB_ATTR_INDEX)3)
#define MIB_TABLE_ME370_THIRD_ATTR		    ((MIB_ATTR_INDEX)4)


/* Table me 370 me stucture */
typedef struct
{
    UINT16 EntityID;
    UINT16 firstAttr;
    UINT32 secondAttr;
    UINT32 thirdAttr;
} __attribute__ ( (aligned) ) MIB_TABLE_ME370_T;



MIB_TABLE_INFO_T gMibMe370TableInfo;
MIB_ATTR_INFO_T gMibMe370AttrInfo[MIB_TABLE_ME370_ATTR_NUM];
MIB_TABLE_ME370_T gMibMe370DefRow;
MIB_TABLE_OPER_T gMibMe370Oper;

GOS_ERROR_CODE omci_mib_me_370_oper_update(void)
{
    GOS_ERROR_CODE		ret = GOS_OK;
    MIB_TABLE_ME370_T   me370;
    UINT16              portId, index;
    omci_uni_qos_info_t uniQos;
    UINT8                tmp[6];
    CHAR                wtmp[OMCI_MAX_NUM_OF_PRIORITY];
    UINT32              me350Size;
    MIB_TABLE_T         *pTable = NULL;
    omci_me_instance_t  entity_id = 0x0;
    UINT32              pri2q = 0;
    MIB_TABLE_PRIQ_T tPQ;
    UINT16           qid;

    memset(tmp, 0, 6);
    memset(wtmp, 0, sizeof(CHAR)*OMCI_MAX_NUM_OF_PRIORITY);

    me370.EntityID = 0x101;

    ret = MIB_Get(MIB_TABLE_ME370_INDEX, &me370, sizeof(MIB_TABLE_ME370_T));

    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Invalid ME370");

        return ret;
    }

    if (0x03000000 != me370.secondAttr)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to no me 370");
        return ret;
    }

    pTable = mib_GetTablePtr(MIB_TABLE_ME350_INDEX);
    if (!pTable)
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to no me 350");
       return GOS_OK;
    }
    if (0 == pTable->curEntryCount)
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to no me 350");
       return GOS_OK;
    }
    me350Size = MIB_GetTableEntrySize(MIB_TABLE_ME350_INDEX);

    CHAR *pTmpEntry = (CHAR *)malloc(me350Size);

    if (!pTmpEntry)
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to malloc failed");
       return GOS_OK;
    }

    MIB_SetAttrToBuf(MIB_TABLE_ME350_INDEX, MIB_ATTR_FIRST_INDEX, &entity_id, pTmpEntry, sizeof(omci_me_instance_t));

    if (GOS_OK != MIB_Get(MIB_TABLE_ME350_INDEX, pTmpEntry, me350Size))
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "qos customized is not for zte olt");
       return GOS_OK;
    }

    memcpy(tmp, ((CHAR *)(mib_GetAttrPtr(MIB_TABLE_ME350_INDEX, pTmpEntry, 16))), 6);
    tmp[3] = (tmp[3] & 0xFF);
    tmp[4] = (tmp[4] & 0xFF);
    tmp[5] = (tmp[5] & 0xFF);
    free(pTmpEntry);

    // translate me id to switch port id
    if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(me370.EntityID, &portId))
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance ID translate error:  0x%x", me370.EntityID);

       return GOS_OK;
    }

    for (index = 0; index < OMCI_MAX_NUM_OF_PRIORITY; index++)
    {
        tPQ.EntityID = index;
        if (GOS_OK == MIB_Get(MIB_TABLE_PRIQ_INDEX, &tPQ, sizeof(MIB_TABLE_PRIQ_T)))
        {
            qid = tPQ.RelatedPort & 0xFFFF;
            wtmp[qid] = (tPQ.Weight > 20 ? (tPQ.Weight / 5) : tPQ.Weight);//tPQ.Weight;

        }
    }

    memset(&uniQos, 0, sizeof(omci_uni_qos_info_t));
    uniQos.port = portId;
    if (tmp[3] != 0x05 || tmp[4] != 0x39 || tmp[5] != 0x77)
    {
        uniQos.valid = TRUE;
        pri2q = (tmp[3] << 16) | (tmp[4] << 8) | (tmp[5]);
    }
    else
    {
        uniQos.valid = FALSE;
    }
    uniQos.policy = 2; //TBD sp+wrr

    for (index = 0; index < OMCI_MAX_NUM_OF_PRIORITY; index++)
    {
        uniQos.pri2queue[index] = ((pri2q >> (24 - ((index + 1) * 3))) & 0x7);;
        uniQos.weights[index] = wtmp[index];

    }

    ret = omci_wrapper_setUniQos(&uniQos);

    return GOS_OK;

}

GOS_ERROR_CODE omci_mib_me_370_oper_reset(void)
{
    MIB_TABLE_ME370_T me370;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: me370");
    MIB_Default(MIB_TABLE_ME370_INDEX, &me370, sizeof(MIB_TABLE_ME370_T));

    me370.EntityID = 0x101;
    me370.firstAttr = 0x0201;
    me370.secondAttr = 0x00000000;
    me370.thirdAttr  = 0x00000000;
    GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ME370_INDEX, &me370, sizeof(MIB_TABLE_ME370_T)));
    OMCI_MeOperCfg(MIB_TABLE_ME370_INDEX, NULL, &me370, MIB_ADD,
        omci_GetOltAccAttrSet(MIB_TABLE_ME370_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

    return GOS_OK;
}


GOS_ERROR_CODE mibTable_init ( MIB_TABLE_INDEX tableId )
{
    UINT8 proprietary_mib_cb_bitmask = 0;

	if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_BDP_00000200))
    {
        return GOS_FAIL;
    }

    MIB_TABLE_ME370_INDEX = tableId;
    memset(&(gMibMe370Oper), 0, sizeof(MIB_TABLE_OPER_T));

    gMibMe370TableInfo.Name = "HW ME370";
    gMibMe370TableInfo.ShortName = "HW370";
    gMibMe370TableInfo.Desc = "huawei ME370";
    gMibMe370TableInfo.ClassId = (UINT32)(370);
    gMibMe370TableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibMe370TableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibMe370TableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMe370TableInfo.pAttributes = &(gMibMe370AttrInfo[0]);
    gMibMe370TableInfo.attrNum = MIB_TABLE_ME370_ATTR_NUM;
    gMibMe370TableInfo.entrySize = sizeof(MIB_TABLE_ME370_T);
    gMibMe370TableInfo.pDefaultRow = &gMibMe370DefRow;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].Name = "First";
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].Name = "Second";
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].Name = "Third";

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].Desc = "First";
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].Desc = "Second";
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].Desc = "Third";

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].MibSave = FALSE;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMe370AttrInfo[MIB_TABLE_ME370_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe370AttrInfo[MIB_TABLE_ME370_FIRST_ATTR - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe370AttrInfo[MIB_TABLE_ME370_SECOND_ATTR - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe370AttrInfo[MIB_TABLE_ME370_THIRD_ATTR - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;


    gMibMe370Oper.meOperCb[PROPRIETARY_MIB_CB_RESET] = omci_mib_me_370_oper_reset;
    gMibMe370Oper.meOperCb[PROPRIETARY_MIB_CB_UPDATE] = omci_mib_me_370_oper_update;
    gMibMe370Oper.meOperDump = omci_mib_oper_dump_default_handler;
    proprietary_mib_cb_bitmask = ((1 << PROPRIETARY_MIB_CB_RESET) | (1 << PROPRIETARY_MIB_CB_UPDATE));


    MIB_InfoRegister(tableId, &gMibMe370TableInfo, &gMibMe370Oper);
    MIB_RegisterCallback(tableId, NULL, NULL);
    MIB_Proprietary_Reg(tableId, proprietary_mib_cb_bitmask);

    return GOS_OK;
}
