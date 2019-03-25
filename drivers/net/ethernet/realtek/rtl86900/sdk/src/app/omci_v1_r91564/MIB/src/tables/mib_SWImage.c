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
 * Purpose : Definition of ME handler: Software image (7)
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) ME handler: Software image (7)
 */

#include "app_basic.h"


MIB_TABLE_INFO_T gMibSWImageTableInfo;
MIB_ATTR_INFO_T  gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ATTR_NUM];
MIB_TABLE_SWIMAGE_T gMibSWImageDefRow;
MIB_TABLE_OPER_T gMibSWImageOper;


GOS_ERROR_CODE SWImageDumpMib(void* pData, MIB_TABLE_INFO_T *pTblInfo)
{
	MIB_TABLE_SWIMAGE_T *pSwImage = (MIB_TABLE_SWIMAGE_T*)pData;

	OMCI_PRINT("EntityID: 0x%02x", pSwImage->EntityID);
	OMCI_PRINT("Active: %d", pSwImage->Active);
	OMCI_PRINT("Committed: %d", pSwImage->Committed);
	OMCI_PRINT("Valid: %d", pSwImage->Valid);
	OMCI_PRINT("Version: %s", pSwImage->Version);
    OMCI_PRINT("ProductCode: %s", pSwImage->ProductCode);
    OMCI_PRINT("ImageHash: " \
        "0x%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
        pSwImage->ImageHash[0], pSwImage->ImageHash[1], pSwImage->ImageHash[2], pSwImage->ImageHash[3],
        pSwImage->ImageHash[4], pSwImage->ImageHash[5], pSwImage->ImageHash[6], pSwImage->ImageHash[7],
        pSwImage->ImageHash[8], pSwImage->ImageHash[9], pSwImage->ImageHash[10], pSwImage->ImageHash[11],
        pSwImage->ImageHash[12], pSwImage->ImageHash[13], pSwImage->ImageHash[14], pSwImage->ImageHash[15]);

	return GOS_OK;
}

GOS_ERROR_CODE SWImageDrvCfg(void* pOldRow,void* pNewRow,MIB_OPERA_TYPE  operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	OMCI_LOG(OMCI_LOG_LEVEL_DBG,"%s: process end\n",__FUNCTION__);
	return GOS_OK;
}

static GOS_ERROR_CODE software_image_avc_cb(MIB_TABLE_INDEX     tableIndex,
                                            void                *pOldRow,
                                            void                *pNewRow,
                                            MIB_ATTRS_SET       *pAttrsSet,
                                            MIB_OPERA_TYPE      operationType)
{
    MIB_ATTR_INDEX      attrIndex;
    UINT32              i;
    MIB_ATTRS_SET       avcAttrSet;
    BOOL                isSuppressed;

    if (MIB_SET != operationType && MIB_ADD != operationType)
        return GOS_OK;

    if (!pNewRow || !pAttrsSet)
        return GOS_ERR_PARAM;

    // check if notifications are suppressed
    omci_is_notify_suppressed_by_circuitpack(0xFF, &isSuppressed);

    if (isSuppressed)
        MIB_ClearAttrSet(&avcAttrSet);
    else
    {
        avcAttrSet = *pAttrsSet;

        for (attrIndex = MIB_ATTR_FIRST_INDEX, i = 0;
                i < MIB_GetTableAttrNum(tableIndex); i++, attrIndex = MIB_ATTR_NEXT_INDEX(attrIndex))
        {
            if (!MIB_IsInAttrSet(pAttrsSet, attrIndex))
                continue;
        }
    }

    if (avcAttrSet != *pAttrsSet)
        *pAttrsSet = avcAttrSet;

    return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibSWImageTableInfo.Name = "SWImage";
    gMibSWImageTableInfo.ShortName = "SWIMG";
    gMibSWImageTableInfo.Desc = "Software Image";
    gMibSWImageTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_SOFTWARE_IMAGE);
    gMibSWImageTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibSWImageTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_STANDARD);
    gMibSWImageTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_GET | OMCI_ME_ACTION_ACTIVATE_SW | OMCI_ME_ACTION_COMMIT_SW |
        OMCI_ME_ACTION_START_SW_DOWNLOAD | OMCI_ME_ACTION_DOWNLOAD_SECTION | OMCI_ME_ACTION_END_SW_DOWNLOAD);
    gMibSWImageTableInfo.pAttributes = &(gMibSWImageAttrInfo[0]);
	gMibSWImageTableInfo.attrNum = MIB_TABLE_SWIMAGE_ATTR_NUM;
	gMibSWImageTableInfo.entrySize = sizeof(MIB_TABLE_SWIMAGE_T);
	gMibSWImageTableInfo.pDefaultRow = &gMibSWImageDefRow;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityID";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Version";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Committed";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Active";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Valid";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ProductCode";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].Name = "ImageHash";

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Version";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Is committed";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Is active";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Is valid";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Product code";
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Image hash";

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].Len = 14;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].Len = 25;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].Len = 16;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = TRUE;

    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VERSION_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_COMMITTED_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_ACTIVE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_VALID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_PRODUCT_CODE_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;
    gMibSWImageAttrInfo[MIB_TABLE_SWIMAGE_IMAGE_HASH_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_O_NOT_SUPPORT;

    memset(&(gMibSWImageDefRow.EntityID), 0x00, sizeof(gMibSWImageDefRow.EntityID));
    strcpy(gMibSWImageDefRow.Version, "");
    gMibSWImageDefRow.Committed = FALSE;
    gMibSWImageDefRow.Active = FALSE;
    gMibSWImageDefRow.Valid = FALSE;
    strcpy(gMibSWImageDefRow.ProductCode, "");
    memset(gMibSWImageDefRow.ImageHash, 0x00, MIB_TABLE_SWIMAGE_IMAGE_HASH_LEN);

    memset(&gMibSWImageOper, 0x0, sizeof(MIB_TABLE_OPER_T));
	gMibSWImageOper.meOperDrvCfg = SWImageDrvCfg;
	gMibSWImageOper.meOperConnCheck = NULL;
	gMibSWImageOper.meOperDump = SWImageDumpMib;
	gMibSWImageOper.meOperConnCfg = NULL;

	MIB_TABLE_SWIMAGE_INDEX = tableId;
	MIB_InfoRegister(tableId,&gMibSWImageTableInfo,&gMibSWImageOper);
    MIB_RegisterCallback(tableId, NULL, software_image_avc_cb);

    return GOS_OK;
}
