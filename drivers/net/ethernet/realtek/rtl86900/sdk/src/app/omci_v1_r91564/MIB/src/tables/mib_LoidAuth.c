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

MIB_TABLE_INFO_T gMibLoIdAuthTableInfo;
MIB_ATTR_INFO_T  gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ATTR_NUM];
MIB_TABLE_LOIDAUTH_T gMibLoIdAuthDefRow;
MIB_TABLE_OPER_T gMibLoIdAuthOper;


GOS_ERROR_CODE LoIdAuthDrvCfg(void* pOldRow, void* pNewRow, MIB_OPERA_TYPE operationType, MIB_ATTRS_SET attrSet, UINT32 pri)
{
	MIB_TABLE_LOIDAUTH_T *pLoidAuth, *pOldLoidAuth;
	MIB_TABLE_LOIDAUTH_T mibLoidAuth;
	MIB_TABLE_INDEX     tableIndex = MIB_TABLE_LOIDAUTH_INDEX;

	OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Start %s...", __FUNCTION__);

	pLoidAuth = (MIB_TABLE_LOIDAUTH_T*)pNewRow;
	pOldLoidAuth = (MIB_TABLE_LOIDAUTH_T*)pOldRow;
	mibLoidAuth.EntityId = pLoidAuth->EntityId;

    	switch (operationType){
		case MIB_SET:
		{
            /*When ONU change LOID or Password, reset AuthStatus to init status*/
            if ((MIB_IsInAttrSet(&attrSet, MIB_TABLE_LOIDAUTH_LOID_INDEX) && memcmp(pLoidAuth->LoID, pOldLoidAuth->LoID, sizeof(pLoidAuth->LoID)))
            	|| (MIB_IsInAttrSet(&attrSet, MIB_TABLE_LOIDAUTH_PASSWORD_INDEX) && memcmp(pLoidAuth->Password, pOldLoidAuth->Password, sizeof(pLoidAuth->Password))) )
            {

                if(GOS_OK == MIB_Get(tableIndex, &mibLoidAuth, sizeof(mibLoidAuth)))
                {
                    mibLoidAuth.AuthStatus = PON_ONU_LOID_INITIAL_STATE;
                    MIB_Set(tableIndex, &mibLoidAuth, sizeof(mibLoidAuth));
                }
                else
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                    __FUNCTION__, MIB_GetTableName(tableIndex), mibLoidAuth.EntityId);
                }

				gInfo.loidCfg.loidAuthStatus = PON_ONU_LOID_INITIAL_STATE;
				gInfo.loidCfg.lastLoidAuthStatus = PON_ONU_LOID_INITIAL_STATE;

            }

			if (MIB_IsInAttrSet(&attrSet, MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX))
			{
			    clock_gettime(CLOCK_MONOTONIC, &gInfo.adtInfo.start_time);
				/*Store the AuthStatus to avoid remove by mib reset*/
				gInfo.loidCfg.loidAuthStatus = pLoidAuth->AuthStatus;
				/*Store the Last AuthStatus set by OLT for user get auth result and not be set to 0(init) by non-O5*/
				gInfo.loidCfg.lastLoidAuthStatus = pLoidAuth->AuthStatus;

				/*Store the Number of LOID Register*/
				gInfo.loidCfg.loidAuthNum++;
				/*Store the Number of LOID Register success*/
				if ( PON_ONU_LOID_SUCCESSFUL_AUTHENTICATION == pLoidAuth->AuthStatus)
				{
					gInfo.loidCfg.loidAuthSuccessNum++;
				}

				/*For LED*/
				omci_wrapper_setLoidAuthStatus(pLoidAuth->AuthStatus);
			}
			break;
		}
		default:
			break;
	}
	return GOS_OK;
}

GOS_ERROR_CODE mibTable_init(MIB_TABLE_INDEX tableId)
{
    gMibLoIdAuthTableInfo.Name = "LoIdAuth";
    gMibLoIdAuthTableInfo.ShortName = "LOIDAUTH";
    gMibLoIdAuthTableInfo.Desc = "LOID authentication";
    gMibLoIdAuthTableInfo.ClassId = (UINT32)(OMCI_ME_CLASS_CTC_LOID_AUTHENTICATION);
    gMibLoIdAuthTableInfo.InitType = (UINT32)(OMCI_ME_INIT_TYPE_ONU);
    gMibLoIdAuthTableInfo.StdType = (UINT32)(OMCI_ME_TYPE_PROPRIETARY | OMCI_ME_TYPE_NOT_MIB_UPLOAD);
    gMibLoIdAuthTableInfo.ActionType = (UINT32)(OMCI_ME_ACTION_SET | OMCI_ME_ACTION_GET);
    gMibLoIdAuthTableInfo.pAttributes = &(gMibLoIdAuthAttrInfo[0]);

	gMibLoIdAuthTableInfo.attrNum = MIB_TABLE_LOIDAUTH_ATTR_NUM;
	gMibLoIdAuthTableInfo.entrySize = sizeof(MIB_TABLE_LOIDAUTH_T);
	gMibLoIdAuthTableInfo.pDefaultRow = &gMibLoIdAuthDefRow;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "EntityId";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "OperationId";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].Name = "LoID";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Name = "Password";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Name = "AuthStatus";

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Entity ID";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Operator ID";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "LOID";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Password for ONU authentication";
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Desc = "Authentication status";

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT16;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_STR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].DataType = MIB_ATTR_TYPE_UINT8;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 2;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 4;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].Len = 24;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].Len = 12;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].Len = 1;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = TRUE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].IsIndex = FALSE;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].MibSave = TRUE;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_HEX;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_CHAR;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OutStyle = MIB_ATTR_OUT_DEC;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OltAcc = OMCI_ME_ATTR_ACCESS_READ | OMCI_ME_ATTR_ACCESS_WRITE;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].AvcFlag = FALSE;

    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_ENTITYID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_OPERATIONID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_LOID_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_PASSWORD_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;
    gMibLoIdAuthAttrInfo[MIB_TABLE_LOIDAUTH_AUTHSTATUS_INDEX - MIB_TABLE_FIRST_INDEX].OptionType = OMCI_ME_ATTR_TYPE_MANDATORY;

    memset(&(gMibLoIdAuthDefRow.EntityId), 0x00, sizeof(gMibLoIdAuthDefRow.EntityId));
    snprintf(gMibLoIdAuthDefRow.OperationId, MIB_TABLE_OPERATIONID_LEN, "0");
    gMibLoIdAuthDefRow.OperationId[MIB_TABLE_OPERATIONID_LEN] = '\0';
    snprintf(gMibLoIdAuthDefRow.LoID, MIB_TABLE_LOIDAUTH_LOID_LEN, "0");
    gMibLoIdAuthDefRow.LoID[MIB_TABLE_LOIDAUTH_LOID_LEN] = '\0';
    snprintf(gMibLoIdAuthDefRow.Password, MIB_TABLE_LOIDAUTH_PASSWORD_LEN, "0");
    gMibLoIdAuthDefRow.Password[MIB_TABLE_LOIDAUTH_PASSWORD_LEN] = '\0';
    memset(&(gMibLoIdAuthDefRow.AuthStatus), 0x00, sizeof(gMibLoIdAuthDefRow.AuthStatus));

    memset(&gMibLoIdAuthOper, 0x0, sizeof(MIB_TABLE_OPER_T));
    gMibLoIdAuthOper.meOperDrvCfg = LoIdAuthDrvCfg;
    gMibLoIdAuthOper.meOperConnCheck = NULL;
    gMibLoIdAuthOper.meOperDump = omci_mib_oper_dump_default_handler;
	gMibLoIdAuthOper.meOperConnCfg = NULL;

	MIB_TABLE_LOIDAUTH_INDEX = tableId;
    MIB_InfoRegister(tableId, &gMibLoIdAuthTableInfo, &gMibLoIdAuthOper);

    return GOS_OK;
}

