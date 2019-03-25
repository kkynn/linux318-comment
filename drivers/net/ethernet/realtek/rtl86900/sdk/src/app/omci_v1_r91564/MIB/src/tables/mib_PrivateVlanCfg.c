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
 * Purpose : Definition of ME handler: Private Vlan Configuration (65535)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Private Vlan Configuration (65535)
 */

#include "app_basic.h"
#include "omci_task.h"


MIB_TABLE_INFO_T gMibPrivateVlanCfgTableInfo;
MIB_ATTR_INFO_T  gMibPrivateVlanCfgAttrInfo[MIB_TABLE_ANIG_ATTR_NUM];
MIB_TABLE_PRIVATE_VLANCFG_T gMibPrivateVlanCfgDefRow;
MIB_TABLE_OPER_T gMibPrivateVlanCfgOper;


GOS_ERROR_CODE PrivateVlanCfgDrvCfg(
    void                *pOldRow,
    void                *pNewRow,
    MIB_OPERA_TYPE      operationType,
    MIB_ATTRS_SET       attrSet,
    UINT32              pri)
{
	MIB_TREE_T                      *pTree = NULL;
    GOS_ERROR_CODE                  ret = GOS_OK;
	MIB_TABLE_PRIVATE_VLANCFG_T     *pMibPrivateVlanCfg = NULL, *pOldpMibPrivateVlanCfg = NULL;
	MIB_TABLE_MACBRISERVPROF_T      bridge;
    UINT32                          totalNum = 0, count = 0;
    UINT16                          id;

	pMibPrivateVlanCfg = (MIB_TABLE_PRIVATE_VLANCFG_T *)pNewRow;
    pOldpMibPrivateVlanCfg = (MIB_TABLE_PRIVATE_VLANCFG_T *)pOldRow;

    switch (operationType)
    {
        case MIB_SET:
            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Private Vlan CFG ----> SET");
			if (memcmp(pMibPrivateVlanCfg, pOldpMibPrivateVlanCfg, sizeof(MIB_TABLE_PRIVATE_VLANCFG_T)))
            {
                OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (0 == (totalNum = MIB_GetTableCurEntryCount(MIB_TABLE_MACBRISERVPROF_INDEX))), GOS_OK);

                if(GOS_OK == MIB_GetFirst(MIB_TABLE_MACBRISERVPROF_INDEX, &bridge, sizeof(MIB_TABLE_MACBRISERVPROF_T)))
                {
                    while(count < totalNum)
                    {
                        count++;
                        id = bridge.EntityID;
                        if (NULL != (pTree = MIB_AvlTreeSearchByKey(NULL, id, AVL_KEY_MACBRISERVPROF)))
                             ret = MIB_TreeConnUpdate(pTree);

                        if (GOS_OK != MIB_GetNext(MIB_TABLE_MACBRISERVPROF_INDEX, &bridge, sizeof(MIB_TABLE_MACBRISERVPROF_T)))
                        {
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG,"get next fail,count=%d",count);
                            break;
                        }
                    }
                }
            }
            break;
        default:
            break;
    }

    return ret;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibPrivateVlanCfgTableInfo.Name = "PrivateVlanCfg";
    gMibPrivateVlanCfgTableInfo.ShortName = "PVC";
    gMibPrivateVlanCfgTableInfo.Desc = "Private Vlan Configuration";
    gMibPrivateVlanCfgTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_PRIVATE_VLANCFG);
    gMibPrivateVlanCfgTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibPrivateVlanCfgTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PRIVATE);
    gMibPrivateVlanCfgTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET);
    gMibPrivateVlanCfgTableInfo.pAttributes = &(gMibPrivateVlanCfgAttrInfo[0]);
	gMibPrivateVlanCfgTableInfo.attrNum = MIB_TABLE_PRIVATE_VLANCFG_ATTR_NUM;
	gMibPrivateVlanCfgTableInfo.entrySize = sizeof(MIB_TABLE_PRIVATE_VLANCFG_T);
	gMibPrivateVlanCfgTableInfo.pDefaultRow = &gMibPrivateVlanCfgDefRow;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
	gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].Name = "Type";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].Name = "ManualMode";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].Name = "ManualTagVid";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].Name = "ManualTagPri";

	gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].Desc = "Type";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].Desc = "Manual Mode";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].Desc = "Manual Tag Vlan ID";
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].Desc = "Manual Tag Priority";

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_TYPE-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_MODE-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_VID-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;
    gMibPrivateVlanCfgAttrInfo[MIB_TABLE_PRIVATE_VLANCFG_MANUAL_TAG_PRI-MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_PRIVATE;

    memset(&(gMibPrivateVlanCfgDefRow.EntityID), 0x00, sizeof(gMibPrivateVlanCfgDefRow.EntityID));
    gMibPrivateVlanCfgDefRow.Type = PRIVATE_VLANCFG_TYPE_AUTO_DETECTION;
    gMibPrivateVlanCfgDefRow.ManualMode = 0xFF;
    gMibPrivateVlanCfgDefRow.ManualTagVid= 0xFFFF;
    gMibPrivateVlanCfgDefRow.ManualTagPri = 0xFF;

    memset(&gMibPrivateVlanCfgOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibPrivateVlanCfgOper.meOperDrvCfg = PrivateVlanCfgDrvCfg;
	gMibPrivateVlanCfgOper.meOperConnCheck = NULL;
	gMibPrivateVlanCfgOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibPrivateVlanCfgOper.meOperConnCfg = NULL;
    gMibPrivateVlanCfgOper.meOperAvlTreeAdd = NULL;
    gMibPrivateVlanCfgOper.meOperPmHandler = NULL;
    gMibPrivateVlanCfgOper.meOperAlarmHandler = NULL;
    gMibPrivateVlanCfgOper.meOperTestHandler = NULL;

	MIB_TABLE_PRIVATE_VLANCFG_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibPrivateVlanCfgTableInfo, &gMibPrivateVlanCfgOper);

    return GOS_OK;
}
