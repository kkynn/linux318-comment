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
 * Purpose : Definition of ME handler: MAC bridge port bridge table data (50)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: MAC bridge port bridge table data (50)
 */

#include "app_basic.h"

MIB_TABLE_INFO_T gMibMacBriPortBriTblDataTableInfo;
MIB_ATTR_INFO_T  gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ATTR_NUM];
MIB_TABLE_MACBRIPORTBRITBLDATA_T gMibMacBriPortBriTblDataDefRow;
MIB_TABLE_OPER_T			  gMibMacBriPortBriTblDataOper;

extern omci_mulget_info_ts gOmciMulGetData[OMCI_MSG_BASELINE_PRI_NUM];

GOS_ERROR_CODE MacBriPortBriTblDataDumpMib(void *pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_MACBRIPORTBRITBLDATA_T *pMacBriPortBriTblData = (MIB_TABLE_MACBRIPORTBRITBLDATA_T *)pData;

	OMCI_PRINT("EntityId: %02x", pMacBriPortBriTblData->EntityID);

	OMCI_PRINT("Bridge Table:");

	return GOS_OK;
}

GOS_ERROR_CODE MacBriPortBriTblDataDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	GOS_ERROR_CODE					ret = GOS_OK;
    MIB_TABLE_MACBRIPORTCFGDATA_T   bridgePort;
    unsigned char *str = NULL, *pStrPos = NULL;
    int fd = -1, i;

	MIB_TABLE_MACBRIPORTBRITBLDATA_T	*pMacBriPortBriTblData;

	pMacBriPortBriTblData = (MIB_TABLE_MACBRIPORTBRITBLDATA_T *)pNewRow;

	switch (operationType)
	{
		case MIB_ADD:
		{
			/*if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
			{
			}
			else if (MBPCD_TP_TYPE_VEIP == pMacBriPortCfgData->TPType)
			{
			}
			else if (MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
			}
            */
			break;
		}
        case MIB_GET:
    	{
    		if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX))
    		{
    			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Mac Bridge Port Bridge Table --> GET table attribute");

                UINT16                          portIdx;
                UINT8                           *ptr = NULL;
                omci_bridge_tbl_per_port_t      bgTblByPort;

                memset(&bgTblByPort, 0, sizeof(omci_bridge_tbl_per_port_t));

                bridgePort.EntityID = pMacBriPortBriTblData->EntityID;

                if (GOS_OK == MIB_Get(MIB_TABLE_MACBRIPORTCFGDATA_INDEX, &bridgePort, sizeof(MIB_TABLE_MACBRIPORTCFGDATA_T)))
                {
                    fd = open("/dev/omcidrv", O_RDWR);

                    if (fd < 0)
                    {
                        OMCI_PRINT("open failed\n");
                        return GOS_FAIL;
                    }
                    // invoke to driver for getting valid entry by physical port and these result are written in virtual memory.
                    // Finially return entry count

                    if (MBPCD_TP_TYPE_PPTP_ETH_UNI == bridgePort.TPType)
                    {
                        if (GOS_OK != pptp_eth_uni_me_id_to_switch_port(bridgePort.TPPointer, &portIdx))
        				{
        					OMCI_LOG(OMCI_LOG_LEVEL_ERR, "PPTP instance ID translate error: 0x%x", bridgePort.TPPointer);
                            close(fd);
                            return GOS_FAIL;
        				}
                        bgTblByPort.portId = portIdx;
                        omci_wrapper_getBridgeTableByPort(&bgTblByPort);
                        pMacBriPortBriTblData->curBriTblEntryCnt = bgTblByPort.cnt;
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "port port mac entry : %u\n", bgTblByPort.cnt);
                    }
                    else if (MBPCD_TP_TYPE_VEIP == bridgePort.TPType)
                    {
                        // TBD
                    }
                    else if (MBPCD_TP_TYPE_IP_HOST_IPV6_HOST == bridgePort.TPType)
                    {
                        // CPU port
                        bgTblByPort.portId = (UINT16)gInfo.devCapabilities.cpuPort;
                        omci_wrapper_getBridgeTableByPort(&bgTblByPort);
                        pMacBriPortBriTblData->curBriTblEntryCnt = bgTblByPort.cnt;
                    }
                    else
                    {
                        /* TBD: per vlan */
                        bgTblByPort.portId = (UINT16)gInfo.devCapabilities.ponPort;
                        omci_wrapper_getBridgeTableByPort(&bgTblByPort);
                        pMacBriPortBriTblData->curBriTblEntryCnt = bgTblByPort.cnt;
                    }

                    gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].attrSize =
                        MIB_TABLE_BRIDGETABLE_ETNRYLEN * (pMacBriPortBriTblData->curBriTblEntryCnt);

        			gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].attrIndex =
                        MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX;

        			gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].doneSeqNum = 0;

        			gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].maxSeqNum =
        				(gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].attrSize +
        				OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT - 1) / OMCI_MSG_BASELINE_GET_NEXT_RSP_LIMIT;

                    if (pMacBriPortBriTblData->curBriTblEntryCnt)
                    {
            			ptr  = gOmciMulGetData[pri].attribute[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX].attrValue;

                        str = (unsigned char *)mmap(NULL, MMT_BUF_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
                        if (str == MAP_FAILED)
                        {
                            OMCI_PRINT("mmap fail due to %s\n", strerror(errno));
                            close(fd);
                            goto mmap_err;
                        }

                        for (i = 0; i < MIB_TABLE_BRIDGETABLE_ETNRYLEN * (pMacBriPortBriTblData->curBriTblEntryCnt); i++)
                            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%x ", str[i]);

                        pStrPos = str;

                        for (i = 0; i < pMacBriPortBriTblData->curBriTblEntryCnt; i++)
                        {
                            memcpy(ptr, pStrPos, MIB_TABLE_BRIDGETABLE_ETNRYLEN);

                            pStrPos += MIB_TABLE_BRIDGETABLE_ETNRYLEN;
                            ptr += MIB_TABLE_BRIDGETABLE_ETNRYLEN;
                        }
                    }
                    close(fd);
                }
    		}
            mmap_err:
                if (str)
                    munmap(str, MMT_BUF_SIZE);
            break;
    	}
		case MIB_DEL:
		{
			/*if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pMacBriPortCfgData->TPType)
			{
			}
			else if (MBPCD_TP_TYPE_VEIP == pMacBriPortCfgData->TPType)
			{
			}
			else if (MBPCD_TP_TYPE_GEM_IWTP == pMacBriPortCfgData->TPType)
			{
			}*/
			break;
		}
		default:
			ret = GOS_OK;
			break;
	}

	return ret;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibMacBriPortBriTblDataTableInfo.Name = "MacBriPortBriTblData";
    gMibMacBriPortBriTblDataTableInfo.ShortName = "MBPBTD";
    gMibMacBriPortBriTblDataTableInfo.Desc = "MAC Bridge Port Bridge Table Data";
    gMibMacBriPortBriTblDataTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_MAC_BRG_PORT_BRIDGE_TBL_DATA);
    gMibMacBriPortBriTblDataTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibMacBriPortBriTblDataTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibMacBriPortBriTblDataTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET | OMCI_ME_ACTION_GET_NEXT);
    gMibMacBriPortBriTblDataTableInfo.pAttributes = &(gMibMacBriPortBriTblDataAttrInfo[0]);

	gMibMacBriPortBriTblDataTableInfo.attrNum = MIB_TABLE_MACBRIPORTBRITBLDATA_ATTR_NUM;
	gMibMacBriPortBriTblDataTableInfo.entrySize = sizeof(MIB_TABLE_MACBRIPORTBRITBLDATA_T);
	gMibMacBriPortBriTblDataTableInfo.pDefaultRow = &gMibMacBriPortBriTblDataTableInfo;


    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "BriTbl";

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Bridge Table";

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_TABLE;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 8;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibMacBriPortBriTblDataAttrInfo[MIB_TABLE_MACBRIPORTBRITBLDATA_BRIDGETABLE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibMacBriPortBriTblDataDefRow.EntityID), 0x00, sizeof(gMibMacBriPortBriTblDataDefRow.EntityID));
    memset(gMibMacBriPortBriTblDataDefRow.BridgeTable, 0, sizeof(gMibMacBriPortBriTblDataDefRow.BridgeTable));

	gMibMacBriPortBriTblDataDefRow.curBriTblEntryCnt = 0;

    memset(&gMibMacBriPortBriTblDataOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibMacBriPortBriTblDataOper.meOperDrvCfg = MacBriPortBriTblDataDrvCfg;
	gMibMacBriPortBriTblDataOper.meOperConnCheck = NULL;
	gMibMacBriPortBriTblDataOper.meOperDump = MacBriPortBriTblDataDumpMib;
	gMibMacBriPortBriTblDataOper.meOperConnCfg = NULL;
	gMibMacBriPortBriTblDataOper.meOperAlarmHandler = NULL;
	MIB_TABLE_MACBRIPORTBRITBLDATA_INDEX = tableId;

	MIB_InfoRegister(tableId, &gMibMacBriPortBriTblDataTableInfo, &gMibMacBriPortBriTblDataOper);


    return GOS_OK;
}
