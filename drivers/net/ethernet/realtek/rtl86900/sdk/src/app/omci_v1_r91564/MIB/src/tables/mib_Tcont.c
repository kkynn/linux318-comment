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
 * Purpose : Definition of ME handler: T-CONT (262)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: T-CONT (262)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibTcontTableInfo;
MIB_ATTR_INFO_T  gMibTcontAttrInfo[MIB_TABLE_TCONT_ATTR_NUM];
MIB_TABLE_TCONT_T gMibTcontDefRow;
MIB_TABLE_OPER_T  gMibTcontTableOper;


GOS_ERROR_CODE TcontDrvCfg(void             *pOldRow,
                            void            *pNewRow,
                            MIB_OPERA_TYPE  operationType,
                            MIB_ATTRS_SET   attrSet,
                            UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;
    MIB_TABLE_TCONT_T   *pNewTcont;
    MIB_TABLE_TCONT_T   *pOldTcont;

    pNewTcont = (MIB_TABLE_TCONT_T *)pNewRow;
    pOldTcont = (MIB_TABLE_TCONT_T *)pOldRow;

    switch (operationType)
    {
        case MIB_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,
                "T-Cont -> SET: old allocId [0x%x] new allocId [0x%x]", pOldTcont->AllocID, pNewTcont->AllocID);

            // When set an invalid allocId, it will remove the old allocId.
            if (m_TCONT_IS_ALLOC_ID_984_RESERVED(pNewTcont->AllocID) &&
                    m_TCONT_IS_ALLOC_ID_984_LEGAL(pOldTcont->AllocID))
            {
                ret = omci_wrapper_deleteTcont(pNewTcont->EntityID, pOldTcont->AllocID);
            }

            // When set an valid allocId, it will add the new allocId.
            if (m_TCONT_IS_ALLOC_ID_984_LEGAL(pNewTcont->AllocID))
            {
                ret = omci_wrapper_createTcont(pNewTcont->EntityID, pNewTcont->AllocID);
            }

            break;
        default:
            break;
    }

    return ret;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibTcontTableInfo.Name = "Tcont";
    gMibTcontTableInfo.ShortName = "TCONT";
    gMibTcontTableInfo.Desc = "T-CONT";
    gMibTcontTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_T_CONT);
    gMibTcontTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibTcontTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibTcontTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibTcontTableInfo.pAttributes = &(gMibTcontAttrInfo[0]);
	gMibTcontTableInfo.attrNum = MIB_TABLE_TCONT_ATTR_NUM;
	gMibTcontTableInfo.entrySize = sizeof(MIB_TABLE_TCONT_T);
	gMibTcontTableInfo.pDefaultRow = &gMibTcontDefRow;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AllocID";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ModeInd";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Policy";

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Alloc ID";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Mode Indicator";
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Policy";

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibTcontAttrInfo[MIB_TABLE_TCONT_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_ALLOCID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_MODEIND_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcontAttrInfo[MIB_TABLE_TCONT_POLICY_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibTcontDefRow.EntityID), 0x00, sizeof(gMibTcontDefRow.EntityID));
    gMibTcontDefRow.AllocID = TCONT_ALLOC_ID_984_RESERVED;
    gMibTcontDefRow.ModeInd = 1;
    gMibTcontDefRow.Policy = TCONT_POLICY_NULL;

    memset(&gMibTcontTableOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibTcontTableOper.meOperDrvCfg = TcontDrvCfg;
	gMibTcontTableOper.meOperConnCheck = NULL;
	gMibTcontTableOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibTcontTableOper.meOperConnCfg = NULL;

	MIB_TABLE_TCONT_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibTcontTableInfo,&gMibTcontTableOper);

    return GOS_OK;
}
