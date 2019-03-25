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
 * Purpose : Definition of ME handler: Network address (137)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Network address (137)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T            gMibNetworkAddrTblInfo;
MIB_ATTR_INFO_T             gMibNetworkAddrAttrInfo[MIB_TABLE_NETWORK_ADDR_ATTR_NUM];
MIB_TABLE_NETWORK_ADDR_T    gMibNetworkAddrDefRow;
MIB_TABLE_OPER_T            gMibNetworkAddrOper;


GOS_ERROR_CODE network_address_drv_cfg_handler(void         *pOldRow,
                                            void            *pNewRow,
                                            MIB_OPERA_TYPE  operationType,
                                            MIB_ATTRS_SET   attrSet,
                                            UINT32          pri)
{
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    MIB_ATTR_INDEX  attrIndex;

    gMibNetworkAddrTblInfo.Name = "NetworkAddress";
    gMibNetworkAddrTblInfo.ShortName = "NWADDR";
    gMibNetworkAddrTblInfo.Desc = "Network address";
    gMibNetworkAddrTblInfo.ClassId = OMCI_ME_CLASS_NETWORK_ADDRESS;
    gMibNetworkAddrTblInfo.InitType = OMCI_ME_INIT_TYPE_OLT;
    gMibNetworkAddrTblInfo.StdType = OMCI_ME_TYPE_STANDARD;
    gMibNetworkAddrTblInfo.ActionType =
        OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET;
    gMibNetworkAddrTblInfo.pAttributes = &(gMibNetworkAddrAttrInfo[0]);
	gMibNetworkAddrTblInfo.attrNum = MIB_TABLE_NETWORK_ADDR_ATTR_NUM;
	gMibNetworkAddrTblInfo.entrySize = sizeof(MIB_TABLE_NETWORK_ADDR_T);
	gMibNetworkAddrTblInfo.pDefaultRow = &gMibNetworkAddrDefRow;

    attrIndex = MIB_TABLE_NETWORK_ADDR_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibNetworkAddrAttrInfo[attrIndex].Name = "EntityId";
    gMibNetworkAddrAttrInfo[attrIndex].Desc = "Entity ID";
    gMibNetworkAddrAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkAddrAttrInfo[attrIndex].Len = 2;
    gMibNetworkAddrAttrInfo[attrIndex].IsIndex = TRUE;
    gMibNetworkAddrAttrInfo[attrIndex].MibSave = TRUE;
    gMibNetworkAddrAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibNetworkAddrAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkAddrAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibNetworkAddrAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_NETWORK_ADDR_SECURITY_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibNetworkAddrAttrInfo[attrIndex].Name = "SecurityPtr";
    gMibNetworkAddrAttrInfo[attrIndex].Desc = "Security pointer";
    gMibNetworkAddrAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkAddrAttrInfo[attrIndex].Len = 2;
    gMibNetworkAddrAttrInfo[attrIndex].IsIndex = FALSE;
    gMibNetworkAddrAttrInfo[attrIndex].MibSave = TRUE;
    gMibNetworkAddrAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibNetworkAddrAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkAddrAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibNetworkAddrAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_NETWORK_ADDR_ADDR_PTR_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibNetworkAddrAttrInfo[attrIndex].Name = "AddrPtr";
	gMibNetworkAddrAttrInfo[attrIndex].Desc = "Address pointer";
    gMibNetworkAddrAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibNetworkAddrAttrInfo[attrIndex].Len = 2;
    gMibNetworkAddrAttrInfo[attrIndex].IsIndex = FALSE;
    gMibNetworkAddrAttrInfo[attrIndex].MibSave = TRUE;
    gMibNetworkAddrAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibNetworkAddrAttrInfo[attrIndex].OltAcc =
        OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibNetworkAddrAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibNetworkAddrAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

	memset(&gMibNetworkAddrDefRow, 0x00, sizeof(gMibNetworkAddrDefRow));

    memset(&gMibNetworkAddrOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibNetworkAddrOper.meOperDrvCfg = network_address_drv_cfg_handler;
    gMibNetworkAddrOper.meOperConnCfg = NULL;
	gMibNetworkAddrOper.meOperConnCheck = NULL;
	gMibNetworkAddrOper.meOperDump = omci_mib_oper_dump_default_handler;

    MIB_TABLE_NETWORK_ADDR_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibNetworkAddrTblInfo, &gMibNetworkAddrOper);

    return GOS_OK;
}
