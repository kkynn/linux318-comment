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
 * Purpose : Definition of ME handler: VoIP image (353)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: VoIP image (353)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T        gMibVoipImageTblInfo;
MIB_ATTR_INFO_T         gMibVoipImageAttrInfo[MIB_TABLE_VOIP_IMAGE_ATTR_NUM];
MIB_TABLE_VOIP_IMAGE_T  gMibVoipImageDefRow;
MIB_TABLE_OPER_T        gMibVoipImageOper;


GOS_ERROR_CODE voip_image_drv_cfg_handler(void          *pOldRow,
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

    gMibVoipImageTblInfo.Name = "VoipImage";
    gMibVoipImageTblInfo.ShortName = "VOIPIMG";
    gMibVoipImageTblInfo.Desc = "VoIP image";
    gMibVoipImageTblInfo.ClassId = OMCI_ME_CLASS_HW_EXTENDED_VOIP_IMAGE;
    gMibVoipImageTblInfo.InitType = OMCI_ME_INIT_TYPE_ONU;
    gMibVoipImageTblInfo.StdType = OMCI_ME_TYPE_PROPRIETARY;
    gMibVoipImageTblInfo.ActionType = OMCI_ME_ACTION_GET |
        OMCI_ME_ACTION_START_SW_DOWNLOAD |
        OMCI_ME_ACTION_DOWNLOAD_SECTION |
        OMCI_ME_ACTION_END_SW_DOWNLOAD |
        OMCI_ME_ACTION_ACTIVATE_SW |
        OMCI_ME_ACTION_COMMIT_SW;
    gMibVoipImageTblInfo.pAttributes = &(gMibVoipImageAttrInfo[0]);
	gMibVoipImageTblInfo.attrNum = MIB_TABLE_VOIP_IMAGE_ATTR_NUM;
	gMibVoipImageTblInfo.entrySize = sizeof(MIB_TABLE_VOIP_IMAGE_T);
	gMibVoipImageTblInfo.pDefaultRow = &gMibVoipImageDefRow;

    attrIndex = MIB_TABLE_VOIP_IMAGE_ENTITY_ID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "EntityId";
    gMibVoipImageAttrInfo[attrIndex].Desc = "Entity ID";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT16;
    gMibVoipImageAttrInfo[attrIndex].Len = 2;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = TRUE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_SBC;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_VOIP_IMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "Version";
    gMibVoipImageAttrInfo[attrIndex].Desc = "Version";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_STR;
    gMibVoipImageAttrInfo[attrIndex].Len = 14;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = FALSE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_VOIP_IMAGE_IS_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "IsCommitted";
    gMibVoipImageAttrInfo[attrIndex].Desc = "Is committed";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoipImageAttrInfo[attrIndex].Len = 1;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = FALSE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_VOIP_IMAGE_IS_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "IsActive";
    gMibVoipImageAttrInfo[attrIndex].Desc = "Is active";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoipImageAttrInfo[attrIndex].Len = 1;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = FALSE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_VOIP_IMAGE_IS_VALID_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "IsValid";
    gMibVoipImageAttrInfo[attrIndex].Desc = "Is valid";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT8;
    gMibVoipImageAttrInfo[attrIndex].Len = 1;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = FALSE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_DEC;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    attrIndex = MIB_TABLE_VOIP_IMAGE_CRC32_INDEX - MIB_TABLE_FIRST_INDEX;
    gMibVoipImageAttrInfo[attrIndex].Name = "Crc32";
    gMibVoipImageAttrInfo[attrIndex].Desc = "CRC32";
    gMibVoipImageAttrInfo[attrIndex].DataType = MIB_ATTR_TYPE_UINT32;
    gMibVoipImageAttrInfo[attrIndex].Len = 4;
    gMibVoipImageAttrInfo[attrIndex].IsIndex = FALSE;
    gMibVoipImageAttrInfo[attrIndex].MibSave = TRUE;
    gMibVoipImageAttrInfo[attrIndex].OutStyle = MIB_ATTR_OUT_HEX;
    gMibVoipImageAttrInfo[attrIndex].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;
    gMibVoipImageAttrInfo[attrIndex].AvcFlag = FALSE;
    gMibVoipImageAttrInfo[attrIndex].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&gMibVoipImageDefRow, 0x00, sizeof(gMibVoipImageDefRow));
    strcpy(gMibVoipImageDefRow.Version, "");

    memset(&gMibVoipImageOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibVoipImageOper.meOperDrvCfg = voip_image_drv_cfg_handler;
    gMibVoipImageOper.meOperConnCfg = NULL;
	gMibVoipImageOper.meOperConnCheck = NULL;
	gMibVoipImageOper.meOperDump = omci_mib_oper_dump_default_handler;

	MIB_TABLE_VOIP_IMAGE_INDEX = tableId;
	MIB_InfoRegister(tableId, &gMibVoipImageTblInfo, &gMibVoipImageOper);

    return GOS_OK;
}
