/*
 * Copyright (C) 2012 Realtek Semiconductor Corp.
 * All Rights Reserved.
 *
 * This program is the proprietary software of Realtek Semiconductor
 * Corporation and/or its licensors, and only be used, duplicated,
 * modified or distributed under the authorized license from Realtek.
 *
 * ANY USE OF THE SOFTWARE OTHER THAN AS AUTHORIZED UNDER
 * THIS LICENSE OR COPYRIGHT LAW IS PROHIBITED.
 *
 */


#include "app_basic.h"
#include "feature_mgmt.h"

MIB_TABLE_INFO_T gMibExtendedIpHostCfgDataTableInfo;
MIB_ATTR_INFO_T  gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ATTR_NUM];
MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T gMibExtendedIpHostCfgDataDefRow;
MIB_TABLE_OPER_T gMibExtendedIpHostCfgDataOper;


GOS_ERROR_CODE ExtendedIpHostCfgDataDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{

    MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T               *pMibExtendedIpHostCfgData = NULL;
    MIB_TABLE_IP_HOST_CFG_DATA_T    mibIpHost;
    MIB_TABLE_TCP_UDP_CFG_DATA_T    mibTcpUdpCfgData;
    UINT32                          tcpUdpNum, count = 0;
    BOOL                            bFound = FALSE;
    mgmt_cfg_msg_t                  mgmtInfo;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: ExtendedIpHostCfgData  process start \n",__FUNCTION__);

	pMibExtendedIpHostCfgData = (MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T*)pNewRow;

    mibIpHost.EntityID = pMibExtendedIpHostCfgData->EntityId;
    if (GOS_OK != MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &mibIpHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
		return GOS_OK;

    switch (operationType)
    {
        case MIB_ADD:
            // check this ip host does not associated with tcp udp
            if (0 == (tcpUdpNum = MIB_GetTableCurEntryCount(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX)))
                bFound = FALSE;

            if (GOS_OK != (MIB_GetFirst(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX, &mibTcpUdpCfgData, sizeof(MIB_TABLE_TCP_UDP_CFG_DATA_T))))
                bFound = FALSE;

            while (count < tcpUdpNum)
            {
                count++;

                if (pMibExtendedIpHostCfgData->EntityId == mibTcpUdpCfgData.IpHostPointer)
                    bFound = TRUE;

                if (GOS_OK != (MIB_GetNext(MIB_TABLE_TCP_UDP_CFG_DATA_INDEX, &mibTcpUdpCfgData, sizeof(MIB_TABLE_TCP_UDP_CFG_DATA_T))))
                    continue;
            }
            if (!bFound)
                omci_setup_mgmt_interface(OP_SET_IF, IF_CHANNEL_MODE_PPPOE, IF_SERVICE_DATA, &mibIpHost, NULL);
            break;
        case MIB_SET:
            memset(&mgmtInfo, 0, sizeof(mgmt_cfg_msg_t));
            mgmtInfo.op_id = OP_SET_PPPOE;
            mgmtInfo.cfg.pppoe.related_if_id = pMibExtendedIpHostCfgData->EntityId;
            mgmtInfo.cfg.pppoe.nat_enabled = pMibExtendedIpHostCfgData->NatStatus;
            mgmtInfo.cfg.pppoe.auth_method = pMibExtendedIpHostCfgData->Mode;
            mgmtInfo.cfg.pppoe.conn_type = pMibExtendedIpHostCfgData->Connect;
            mgmtInfo.cfg.pppoe.release_time = pMibExtendedIpHostCfgData->ReleaseTime;
            strncpy(mgmtInfo.cfg.pppoe.username, pMibExtendedIpHostCfgData->User, MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN);
            strncpy(mgmtInfo.cfg.pppoe.password, pMibExtendedIpHostCfgData->Password, MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN);
            feature_api(FEATURE_API_L3SVC_MGMT_CFG_SET, &mgmtInfo, sizeof(mgmt_cfg_msg_t));
			break;
        case MIB_DEL:
            // remove pppoe interface for internet and the iphost change to DHCP mode or static mode
            omci_setup_mgmt_interface(OP_RESET_IF, IF_CHANNEL_MODE_PPPOE, IF_SERVICE_DATA, &mibIpHost, NULL);
            break;
        default:
            break;
    }

	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: ExtendedIpHostCfgData end\n",__FUNCTION__);

	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibExtendedIpHostCfgDataTableInfo.Name = "ExtIpHostCfgData";
    gMibExtendedIpHostCfgDataTableInfo.ShortName = "EXTIPHCD";
    gMibExtendedIpHostCfgDataTableInfo.Desc = "Pppoe interface";
    gMibExtendedIpHostCfgDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_EXTENDED_IP_HOST_CFG_DATA);
    gMibExtendedIpHostCfgDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_OLT);
    gMibExtendedIpHostCfgDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY);
    gMibExtendedIpHostCfgDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_CREATE | OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibExtendedIpHostCfgDataTableInfo.pAttributes = &(gMibExtendedIpHostCfgDataAttrInfo[0]);

	gMibExtendedIpHostCfgDataTableInfo.attrNum = MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ATTR_NUM;
	gMibExtendedIpHostCfgDataTableInfo.entrySize = sizeof(MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T);
	gMibExtendedIpHostCfgDataTableInfo.pDefaultRow = &gMibExtendedIpHostCfgDataDefRow;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "NatStatus";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Mode";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Connect";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ReleaseTime";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].Name = "User";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Password";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "State";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OnlineDuration";

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Nat Status";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Mode";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Connect method";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "ARelease Time";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "User";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Password";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "PPPoE state";
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Online Duration";

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT32;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Len = MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE | OMCI_ME_ATTR_ACCESS_SBC;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_NATSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_MODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_CONNECT_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_RELEASETIME_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_USER_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STATE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibExtendedIpHostCfgDataAttrInfo[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_ONLINE_DURATION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibExtendedIpHostCfgDataDefRow, 0x00, sizeof(MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_T));
    gMibExtendedIpHostCfgDataDefRow.ReleaseTime = 0x04B0;
    gMibExtendedIpHostCfgDataDefRow.User[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN] = '\0';
    gMibExtendedIpHostCfgDataDefRow.Password[MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_STRING_PART_LEN] = '\0';
    memset(&gMibExtendedIpHostCfgDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibExtendedIpHostCfgDataOper.meOperDrvCfg = ExtendedIpHostCfgDataDrvCfg;
    gMibExtendedIpHostCfgDataOper.meOperConnCheck = NULL;
    gMibExtendedIpHostCfgDataOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibExtendedIpHostCfgDataOper.meOperConnCfg = NULL;

	MIB_TABLE_EXTENDED_IP_HOST_CFG_DATA_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibExtendedIpHostCfgDataTableInfo, &gMibExtendedIpHostCfgDataOper);

    return GOS_OK;
}

