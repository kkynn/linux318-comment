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
 * Purpose : Definition of ME handler: ME242 (242)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: ME242 (242) for ZTE OLT can be assoicated with ME 243
 */

#include "app_basic.h"
#include "feature_mgmt.h"
#define MIB_TABLE_ME242_ATTR_NUM (3)
#define MIB_TABLE_ME242_ENTITYID_INDEX ((MIB_ATTR_INDEX)1)
#define MIB_TABLE_ME242_POLICY_INDEX ((MIB_ATTR_INDEX)2)
#define MIB_TABLE_ME242_ME243PTR_INDEX ((MIB_ATTR_INDEX)3)


typedef enum {
    ME242_POLICY_DEFAULT = 0xFF,
    ME242_POLICY_SP		 = 0,
    ME242_POLICY_WRR,
    ME242_POLICY_SP_WRR,
} me242_attr_policy_t;

// Table ME 242 entry stucture
typedef struct {
    UINT16 EntityID; // implicitly linked to pptp instance ID
    UINT8  Policy;
    UINT16 Me243Ptr;
} __attribute__((aligned)) MIB_TABLE_ME242_T;


MIB_TABLE_INFO_T gMibMe242TableInfo;
MIB_ATTR_INFO_T  gMibMe242AttrInfo[MIB_TABLE_ME242_ATTR_NUM];
MIB_TABLE_ME242_T gMibMe242DefRow;
MIB_TABLE_OPER_T  gMibMe242Oper;

GOS_ERROR_CODE omci_mib_oper_update(void)
{
    GOS_ERROR_CODE		ret;
    MIB_TABLE_ME242_T   me242;
    UINT16              portId, index;
    omci_uni_qos_info_t uniQos;
    CHAR                qtmp[OMCI_MAX_NUM_OF_PRIORITY], wtmp[OMCI_MAX_NUM_OF_PRIORITY];
    UINT32              me243Size;
    MIB_TABLE_T         *pTable = NULL;

    memset(qtmp, 0, OMCI_MAX_NUM_OF_PRIORITY);
    memset(wtmp, 0, OMCI_MAX_NUM_OF_PRIORITY);

    me242.EntityID = 0x101;

    ret = MIB_Get(MIB_TABLE_ME242_INDEX, &me242, sizeof(MIB_TABLE_ME242_T));

    if (GOS_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Invalid T-Cont for PQ");

        return ret;
    }

    pTable = mib_GetTablePtr(MIB_TABLE_ME243_INDEX);
    if (!pTable)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to no me 243");
        return GOS_OK;
    }
    if (0 == pTable->curEntryCount)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to no me 243");
        return GOS_OK;
    }

    if (0xFF != me242.Policy)
    {

       me243Size = MIB_GetTableEntrySize(MIB_TABLE_ME243_INDEX);

       CHAR *pTmpEntry = (CHAR *)malloc(me243Size);

       if (!pTmpEntry)
       {
           OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Follow standard, so do nothing due to malloc failed");
           return GOS_OK;
       }

       MIB_SetAttrToBuf(MIB_TABLE_ME243_INDEX, MIB_ATTR_FIRST_INDEX, &me242.Me243Ptr, pTmpEntry, sizeof(omci_me_instance_t));

       if (GOS_OK != MIB_Get(MIB_TABLE_ME243_INDEX, pTmpEntry, me243Size))
       {
           OMCI_LOG(OMCI_LOG_LEVEL_WARN, "qos customized is not for zte olt");
           return GOS_OK;
       }

       memcpy(qtmp, (CHAR *)mib_GetAttrPtr(MIB_TABLE_ME243_INDEX, pTmpEntry, 3), OMCI_MAX_NUM_OF_PRIORITY);
       memcpy(wtmp, (CHAR *)mib_GetAttrPtr(MIB_TABLE_ME243_INDEX, pTmpEntry, 4), OMCI_MAX_NUM_OF_PRIORITY);
       free(pTmpEntry);

    }
    // translate me id to switch port id
    if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(me242.EntityID, &portId))
    {
       OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Instance ID translate error:  0x%x", me242.EntityID);

       return GOS_OK;
    }

    memset(&uniQos, 0, sizeof(omci_uni_qos_info_t));
    uniQos.port = portId;
    uniQos.valid = ((0xFF == me242.Policy) ? FALSE : TRUE);
    uniQos.policy = me242.Policy;

    for (index = 0; index < OMCI_MAX_NUM_OF_PRIORITY; index++)
    {
        uniQos.pri2queue[index] = qtmp[index];
        wtmp[index] = (wtmp[index] > 20 ? (wtmp[index] / 5) : wtmp[index]);
        uniQos.weights[index] = wtmp[index];
    }

    ret = omci_wrapper_setUniQos(&uniQos);

    return GOS_OK;

}

GOS_ERROR_CODE omci_mib_oper_reset(void)
{
    MIB_TABLE_ME242_T   me242;
	MIB_TABLE_ETHUNI_T	mibPptpEthUNI;
	UINT32				entrySize;
	GOS_ERROR_CODE		ret;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Resetting MIB: me242");

    MIB_Default(MIB_TABLE_ME242_INDEX, &me242, sizeof(MIB_TABLE_ME242_T));

	entrySize = MIB_GetTableEntrySize(MIB_TABLE_ETHUNI_INDEX);
	ret = MIB_GetFirst(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, entrySize);

    while (GOS_OK == ret)
    {
        me242.EntityID = mibPptpEthUNI.EntityID;

        GOS_ASSERT(GOS_OK == MIB_Set(MIB_TABLE_ME242_INDEX, &me242, sizeof(MIB_TABLE_ME242_T)));

        OMCI_MeOperCfg(MIB_TABLE_ME242_INDEX, NULL, &me242, MIB_ADD,
                       omci_GetOltAccAttrSet(MIB_TABLE_ME242_INDEX, OMCI_ME_ATTR_ACCESS_SBC), OMCI_MSG_BASELINE_PRI_LOW);

        ret = MIB_GetNext(MIB_TABLE_ETHUNI_INDEX, &mibPptpEthUNI, entrySize);
    }

    return GOS_OK;
}



GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    UINT8 proprietary_mib_cb_bitmask = 0;

	if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_BDP_00000200))
    {
        return GOS_FAIL;
    }

    memset(&gMibMe242Oper, 0x00, sizeof(MIB_TABLE_OPER_T));
    MIB_TABLE_ME242_INDEX = tableId;

    gMibMe242TableInfo.Name = "Me242";
    gMibMe242TableInfo.ShortName = "ME242";
    gMibMe242TableInfo.Desc = "ME242";
    gMibMe242TableInfo.ClassId = (UINT32)(242);
    gMibMe242TableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibMe242TableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibMe242TableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibMe242TableInfo.pAttributes = &(gMibMe242AttrInfo[0]);
	gMibMe242TableInfo.attrNum = MIB_TABLE_ME242_ATTR_NUM;
	gMibMe242TableInfo.entrySize = sizeof(MIB_TABLE_ME242_T);
	gMibMe242TableInfo.pDefaultRow = &gMibMe242DefRow;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Policy";
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Me243ptr";

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Policy";
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ME 243 Pointer";

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMe242AttrInfo[MIB_TABLE_ME242_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe242AttrInfo[MIB_TABLE_ME242_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMe242AttrInfo[MIB_TABLE_ME242_ME243PTR_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibMe242DefRow.EntityID), 0x00, sizeof(gMibMe242DefRow.EntityID));
    gMibMe242DefRow.Policy = ME242_POLICY_DEFAULT;
    gMibMe242DefRow.Me243Ptr = 0xFF;

	gMibMe242Oper.meOperDump = omci_mib_oper_dump_default_handler;
    gMibMe242Oper.meOperCb[PROPRIETARY_MIB_CB_RESET] = omci_mib_oper_reset;
    gMibMe242Oper.meOperCb[PROPRIETARY_MIB_CB_UPDATE] = omci_mib_oper_update;
    proprietary_mib_cb_bitmask = ((1 << PROPRIETARY_MIB_CB_RESET) | (1 << PROPRIETARY_MIB_CB_UPDATE));

    MIB_InfoRegister(tableId, &gMibMe242TableInfo, &gMibMe242Oper);
    MIB_RegisterCallback(tableId, NULL, NULL);
    MIB_Proprietary_Reg(tableId, proprietary_mib_cb_bitmask);

    return GOS_OK;
}
