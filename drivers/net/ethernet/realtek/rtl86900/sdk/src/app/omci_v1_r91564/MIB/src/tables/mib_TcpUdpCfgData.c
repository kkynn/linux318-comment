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
 * Purpose : Definition of ME handler: TCP/UDP config data (136)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: TCP/UDP config data (136)
 */

#include "app_basic.h"
#ifndef OMCI_X86
#include "rtk/gpon.h"
#endif

MIB_TABLE_INFO_T gMibTcpUdpCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ATTR_NUM];
MIB_TABLE_TCP_UDP_CFG_DATA_T gMibTcpUdpCfgDataDefRow;
MIB_TABLE_OPER_T gMibTcpUdpCfgDataOper;


GOS_ERROR_CODE tcp_udp_cfg_data_drv_cfg_handler(void            *pOldRow,
                                                void            *pNewRow,
                                                MIB_OPERA_TYPE  operationType,
                                                MIB_ATTRS_SET   attrSet,
                                                UINT32          pri)
{
    GOS_ERROR_CODE      ret = GOS_OK;

    /*
    TBD, handle TCP/UDP settings here!
     */

    return ret;
}


GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibTcpUdpCfgDataTableInfo.Name = "TcpUdpCfgData";
    gMibTcpUdpCfgDataTableInfo.ShortName = "TCPUDP";
    gMibTcpUdpCfgDataTableInfo.Desc = "TCP/UDP config data";
    gMibTcpUdpCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_TCP_UDP_CFG_DATA);
    gMibTcpUdpCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibTcpUdpCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibTcpUdpCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_DELETE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibTcpUdpCfgDataTableInfo.pAttributes = &(gMibTcpUdpCfgDataAttrInfo[0]);

	gMibTcpUdpCfgDataTableInfo.attrNum = MIB_TABLE_TCP_UDP_CFG_DATA_ATTR_NUM;
	gMibTcpUdpCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_TCP_UDP_CFG_DATA_T);
	gMibTcpUdpCfgDataTableInfo.pDefaultRow = &gMibTcpUdpCfgDataDefRow;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "PortId";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Protocol";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].Name = "TosDiffserv";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "IpHostPointer";

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Port ID";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Protocol ";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "TOS/diffserv field";
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "IP host pointer";

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PORT_ID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_PROTOCOL_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_TOS_DIFFSERV_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibTcpUdpCfgDataAttrInfo[MIB_TABLE_TCP_UDP_CFG_DATA_IP_HOST_POINTER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibTcpUdpCfgDataDefRow, 0x00, sizeof(gMibTcpUdpCfgDataDefRow));

    memset(&gMibTcpUdpCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibTcpUdpCfgDataOper.meOperDrvCfg = tcp_udp_cfg_data_drv_cfg_handler;
    gMibTcpUdpCfgDataOper.meOperConnCheck = NULL;
    gMibTcpUdpCfgDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibTcpUdpCfgDataOper.meOperConnCfg = NULL;
    gMibTcpUdpCfgDataOper.meOperPmHandler = NULL;

	MIB_TABLE_TCP_UDP_CFG_DATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibTcpUdpCfgDataTableInfo, &gMibTcpUdpCfgDataOper);

    return GOS_OK;
}
