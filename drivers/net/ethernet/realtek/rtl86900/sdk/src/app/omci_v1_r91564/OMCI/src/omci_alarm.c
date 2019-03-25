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
 * Purpose : Definition of OMCI alarm APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI alarm APIs
 */

#include "app_basic.h"
#include "omci_task.h"


static UINT8	gOmciAlarmSequenceNumber = 1;

GOS_ERROR_CODE omci_alarm_notify_xmit(MIB_TABLE_INDEX       tableIndex,
								        omci_me_instance_t  instanceID,
								        mib_alarm_table_t   *pAlarmTable)
{
	omci_msg_norm_baseline_t   omciBaselineMsg;

	if (!pAlarmTable)
		return GOS_ERR_PARAM;

	// always reset the content of allocated memory
	memset(&omciBaselineMsg, 0, sizeof(omciBaselineMsg));

	// fill ME ID and instanceID
    omciBaselineMsg.meId.meClass = MIB_GetTableClassId(tableIndex);
    omciBaselineMsg.meId.meInstance = instanceID;

    // fill message type of alarm
    omciBaselineMsg.type = OMCI_MSG_TYPE_ALARM;

    // fill message contents with full alarm table
    memcpy(&omciBaselineMsg.content, pAlarmTable->aBitMask, OMCI_ALARM_NUMBER_BITMASK_IN_BYTE);

    // fill alarm sequence number to the last byte of content
    omciBaselineMsg.content[OMCI_MSG_BASELINE_CONTENTS_LEN - 1] = gOmciAlarmSequenceNumber++;

    // reset alarm sequence number to avoid using 0 by the request of G.988 A.1.4.1
    if (0 == gOmciAlarmSequenceNumber) gOmciAlarmSequenceNumber = 1;

    return (OMCI_SendIndicationToOlt(&omciBaselineMsg, 0) > 0) ? GOS_OK : GOS_FAIL;
}

static GOS_ERROR_CODE omci_alarm_me_handler(MIB_TABLE_INDEX     tableIndex,
                                            omci_alm_data_t     alarmData)
{
	MIB_TABLE_T			*pTable;
	mib_alarm_table_t	alarmTable;
	omci_me_instance_t	instanceID;
	BOOL				isUpdated;

	pTable = mib_GetTablePtr(tableIndex);
    if (!pTable || !pTable->meOper)
        return GOS_ERR_PARAM;

    isUpdated = FALSE;

    if (pTable->meOper->meOperAlarmHandler)
    {
    	if (GOS_OK != pTable->meOper->meOperAlarmHandler(tableIndex, alarmData, &instanceID, &isUpdated))
    	{
			OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Alarm handler returns fail: %s",
			    MIB_GetTableName(tableIndex));

			return GOS_FAIL;
    	}
    }
    else
    {
    	OMCI_LOG(OMCI_LOG_LEVEL_WARN, "No alarm handler has defined: %s",
    		MIB_GetTableName(tableIndex));

    	return GOS_FAIL;
    }

    if (isUpdated)
    {
    	if (GOS_OK != mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
	    {
	    	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
	    		MIB_GetTableName(tableIndex), instanceID);

	    	return GOS_FAIL;
	    }

	    if (GOS_OK != omci_alarm_notify_xmit(tableIndex, instanceID, &alarmTable))
	    {
	    	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send alarm notify fail: %s, 0x%x",
	    		MIB_GetTableName(tableIndex), instanceID);

	    	return GOS_FAIL;
	    }
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_ext_alarm_dispatcher(omci_alm_t     *pData)
{
	MIB_TABLE_INDEX		tableIndex;

    if (!pData)
        return GOS_ERR_PARAM;

    if (!m_omci_is_alm_nbr_valid(pData->almData.almNumber) ||
            !m_omci_is_alm_sts_valid(pData->almData.almStatus))
        return GOS_ERR_PARAM;

    switch (pData->almType)
    {
    	case OMCI_ALM_TYPE_ONU_G:
    		tableIndex = MIB_TABLE_ONTG_INDEX;
    		break;
    	case OMCI_ALM_TYPE_CARDHOLDER:
    		tableIndex = MIB_TABLE_CARDHOLDER_INDEX;
    		break;
    	case OMCI_ALM_TYPE_CIRCUIT_PACK:
    		tableIndex = MIB_TABLE_CIRCUITPACK_INDEX;
    		break;
    	case OMCI_ALM_TYPE_ANI_G:
    		tableIndex = MIB_TABLE_ANIG_INDEX;
    		break;
    	case OMCI_ALM_TYPE_GEM_PORT_NETWORK_CTP:
    		tableIndex = MIB_TABLE_GEMPORTCTP_INDEX;
    		break;
    	case OMCI_ALM_TYPE_PRIORITY_QUEUE:
    		tableIndex = MIB_TABLE_PRIQ_INDEX;
    		break;
    	case OMCI_ALM_TYPE_MAC_BRIDGE_PORT_CONFIG_DATA:
    		tableIndex = MIB_TABLE_MACBRIPORTCFGDATA_INDEX;
    		break;
    	case OMCI_ALM_TYPE_MULTICAST_OPERATIONS_PROFILE:
    		tableIndex = MIB_TABLE_MCASTOPERPROF_INDEX;
    		break;
    	case OMCI_ALM_TYPE_PPTP_ETHERNET_UNI:
    		tableIndex = MIB_TABLE_ETHUNI_INDEX;
    		break;
    	case OMCI_ALM_TYPE_VEIP:
    		tableIndex = MIB_TABLE_VEIP_INDEX;
    		break;
        case OMCI_ALM_TYPE_LOOP_DETECT:
            tableIndex = MIB_TABLE_LOOP_DETECT_INDEX;
            break;
        case OMCI_ALM_TYPE_SELF_LOOP_DETECT:
            tableIndex = MIB_TABLE_ONT_SELFLOOPDETECT_INDEX;
            break;
    	default:
    		return GOS_ERR_PARAM;
    }

    return omci_alarm_me_handler(tableIndex, pData->almData);
}

GOS_ERROR_CODE omci_alarm_reset_sequence_number(void)
{
    gOmciAlarmSequenceNumber = 1;

    return GOS_OK;
}

static UINT16 omci_alarm_snapshot_create_by_me(MIB_TABLE_INDEX  tableIndex)
{
    MIB_TABLE_T         *pTable;
    MIB_ENTRY_T         *pEntry;
    mib_alarm_table_t   zeroAlarmTable;
    UINT8               *pAlmSnapShot;
    UINT32              almSnapShotSize;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return 0;

    // prevent create multiple snapshot
    if (pTable->pAlmSnapShot)
        return 0;

    almSnapShotSize = sizeof(mib_alarm_table_t) + sizeof(omci_me_instance_t);

    // allocate memory
    pTable->pAlmSnapShot = malloc(pTable->curEntryCount * almSnapShotSize);
    if (!pTable->pAlmSnapShot)
        return 0;

    memset(&zeroAlarmTable, 0, sizeof(mib_alarm_table_t));
    pTable->almSnapShotCnt = 0;

    // copy data
    pAlmSnapShot = pTable->pAlmSnapShot;
    LIST_FOREACH(pEntry, &pTable->entryHead, entries)
    {
        if (pEntry->pAlarmTable)
        {
            // skip instance without raised alarm
            if (0 == memcmp(pEntry->pAlarmTable, &zeroAlarmTable, sizeof(mib_alarm_table_t)))
                continue;

            // latch instace ID
            memcpy(pAlmSnapShot, pEntry->pData, sizeof(omci_me_instance_t));
            pAlmSnapShot += sizeof(omci_me_instance_t);

            // latch alarm table
            memcpy(pAlmSnapShot, pEntry->pAlarmTable, sizeof(mib_alarm_table_t));
            pAlmSnapShot += sizeof(mib_alarm_table_t);

            pTable->almSnapShotCnt++;
        }
    }

    // free memory if there is no alarm table
    if (0 == pTable->almSnapShotCnt)
    {
        free(pTable->pAlmSnapShot);
        pTable->pAlmSnapShot = NULL;
    }

    return pTable->almSnapShotCnt;
}

static GOS_ERROR_CODE omci_alarm_snapshot_delete_by_me(MIB_TABLE_INDEX  tableIndex)
{
    MIB_TABLE_T     *pTable;

    pTable = mib_GetTablePtr(tableIndex);
    if (!pTable)
        return GOS_ERR_PARAM;

    if (!pTable->pAlmSnapShot)
        return GOS_ERR_NOT_FOUND;

    free(pTable->pAlmSnapShot);
    pTable->pAlmSnapShot = NULL;
    pTable->almSnapShotCnt = 0;

    return GOS_OK;
}

extern int MIB_TABLE_LAST_INDEX;

UINT16 omci_alarm_snapshot_create_all(UINT8     almRetrievalMode)
{
    MIB_TABLE_INDEX     tableIndex;
    UINT16              almSnapShotCnt = 0;

    // not support almRetrievalMode, always use mode 0
    // almRetrievalMode = 0: get all alarms regardless of ARC
    // almRetrievalMode = 1: get all alarms not under ARC

    for (tableIndex = MIB_TABLE_FIRST_INDEX;
            tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
        almSnapShotCnt += omci_alarm_snapshot_create_by_me(tableIndex);

    return almSnapShotCnt;
}

UINT16 omci_alarm_snapshot_get_by_seq(UINT16                almSnapShotSeq,
                                        omci_me_class_t     *pClassID,
                                        omci_me_instance_t  *pInstanceID,
                                        mib_alarm_table_t   *pAlarmTable)
{
    MIB_TABLE_INDEX     tableIndex;
    MIB_TABLE_T         *pTable;
    UINT8               *pAlmSnapShot;
    UINT32              almSnapShotSize;
    UINT16              almSnapShotCnt = 0;

    if (!pClassID || !pInstanceID || !pAlarmTable)
        return GOS_FAIL;

    almSnapShotSize = sizeof(mib_alarm_table_t) + sizeof(omci_me_instance_t);

    for (tableIndex = MIB_TABLE_FIRST_INDEX;
            tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
    {
        pTable = mib_GetTablePtr(tableIndex);
        if (!pTable)
            return GOS_FAIL;

        // skip no entry table
        if (0 == pTable->almSnapShotCnt)
            continue;

        // why no data for entry > 0
        if (!pTable->pAlmSnapShot)
            return GOS_FAIL;

        // add to all snapshot count for move on
        almSnapShotCnt += pTable->almSnapShotCnt;

        // found the target where seq falls into the range
        if (almSnapShotSeq < almSnapShotCnt)
        {
            // shift pointer according to seq
            pAlmSnapShot = pTable->pAlmSnapShot;
            pAlmSnapShot += almSnapShotSize *
                (almSnapShotSeq - (almSnapShotCnt - pTable->almSnapShotCnt));

            // copy data
            memcpy(pInstanceID, pAlmSnapShot, sizeof(omci_me_instance_t));
            pAlmSnapShot += sizeof(omci_me_instance_t);
            memcpy(pAlarmTable, pAlmSnapShot, sizeof(mib_alarm_table_t));

            *pClassID = MIB_GetTableClassId(tableIndex);

            break;
        }
    }

    // return fail for seq out of range
    if (almSnapShotSeq >= almSnapShotCnt)
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE omci_alarm_snapshot_delete_all(void)
{
    MIB_TABLE_INDEX     tableIndex;

    for (tableIndex = MIB_TABLE_FIRST_INDEX;
            tableIndex <= MIB_TABLE_LAST_INDEX; tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
        omci_alarm_snapshot_delete_by_me(tableIndex);

    return GOS_OK;
}

static GOS_ERROR_CODE omci_alarm_snapshot_timer_task(void   *pData)
{
    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Alarm snapshot timer expired");

    // delete all snapshot
    omci_alarm_snapshot_delete_all();

    // delete timer
    return omci_timer_delete_by_id(OMCI_TIMER_RESERVED_CLASS_ID,
                                    OMCI_TIMER_RESERVED_ALM_INSTANCE_ID);
}

void omci_alarm_snapshot_timer_handler(UINT16   classID,
                                        UINT16  instanceID,
                                        UINT32  privData)
{
    // spawn task for resource dellocation
    OMCI_SpawnTask("resources revoker of get alarms",
                    omci_alarm_snapshot_timer_task,
                    NULL,
                    OMCI_TASK_PRI_TIMER,
                    FALSE);
}
