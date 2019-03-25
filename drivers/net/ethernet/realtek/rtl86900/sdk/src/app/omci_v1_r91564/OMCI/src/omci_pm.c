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
 * Purpose : Definition of OMCI performance monitoring APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI performance monitoring APIs
 */

#include "app_basic.h"
#include "omci_task.h"


BOOL			gbOmciPmPausePlayRequest = FALSE;
pthread_mutex_t	gOmciPmPausePlayRequestMutex = PTHREAD_MUTEX_INITIALIZER;
BOOL			gbOmciPmSyncRequest = FALSE;
pthread_mutex_t	gOmciPmSyncRequestMutex = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t	gOmciPmOperMutex = PTHREAD_MUTEX_INITIALIZER;
#define OMCI_PM_OPER_PROTECT_LOCK	(pthread_mutex_lock(&gOmciPmOperMutex))
#define OMCI_PM_OPER_PROTECT_UNLOCK	(pthread_mutex_unlock(&gOmciPmOperMutex))


void omci_pm_pause_play_request_updater(BOOL bWrite, BOOL *bPausePlayState)
{
	pthread_mutex_lock(&gOmciPmPausePlayRequestMutex);

	if (bWrite)
		gbOmciPmPausePlayRequest = *bPausePlayState;
	else
		*bPausePlayState = gbOmciPmPausePlayRequest;

	pthread_mutex_unlock(&gOmciPmPausePlayRequestMutex);
}

void omci_pm_sync_request_updater(BOOL bWrite, BOOL *bSyncState)
{
	pthread_mutex_lock(&gOmciPmSyncRequestMutex);

	if (bWrite)
		gbOmciPmSyncRequest = *bSyncState;
	else
		*bSyncState = gbOmciPmSyncRequest;

	pthread_mutex_unlock(&gOmciPmSyncRequestMutex);
}

extern int MIB_TABLE_LAST_INDEX;


GOS_ERROR_CODE omci_pm_update_specified_ME(MIB_TABLE_INDEX tableIndex,omci_me_instance_t entityID)
{
	GOS_ERROR_CODE		ret;
	MIB_TABLE_T 		*pTable = mib_GetTablePtr(tableIndex);
	BOOL				isTcaRaised;

	if (!pTable || !pTable->meOper || !pTable->meOper->meOperPmHandler)
		return GOS_ERR_PARAM;

	// call per ME PM handler to update current data
	if (GOS_OK != (ret = pTable->meOper->meOperPmHandler(
			tableIndex, entityID, OMCI_PM_OPER_GET_CURRENT_DATA, &isTcaRaised)))
	{
		OMCI_LOG(OMCI_LOG_LEVEL_WARN, "PM handler returns fail: %s",
		MIB_GetTableName(tableIndex));
	}
	return ret;

}



void omci_pm_operation_updater(omci_pm_oper_type_t	operType)
{
	GOS_ERROR_CODE		ret;
	MIB_TABLE_INDEX		tableIndex;
	MIB_TABLE_T			*pTable;
	MIB_ENTRY_T         *pEntry;
	omci_me_instance_t	instanceID;
	BOOL				isTcaRaised;
	mib_alarm_table_t	alarmTable;
	mib_alarm_table_t   zeroAlarmTable;

	memset(&zeroAlarmTable, 0, sizeof(mib_alarm_table_t));


    OMCI_PM_OPER_PROTECT_LOCK;
	// update PM for pptp eth uni
	omci_pm_update_pptp_eth_uni(operType);

	// update PM for GEM port
	omci_pm_update_gem_port(operType);

    // update PM for RTP
    omci_pm_update_rtp(operType);

	omci_pm_update_ccpm (operType);
	omci_pm_update_sipAgent (operType);
    omci_pm_update_sipCi (operType);

	for (tableIndex = MIB_TABLE_FIRST_INDEX; tableIndex <= MIB_TABLE_LAST_INDEX;
			tableIndex = MIB_TABLE_NEXT_INDEX(tableIndex))
	{
	    pTable = mib_GetTablePtr(tableIndex);

	    if (!pTable || !pTable->meOper)
	        continue;

		if (!(OMCI_ME_TYPE_STANDARD_PM & MIB_GetTableStdType(tableIndex)) &&
				!(OMCI_ME_TYPE_PROPRIETARY_PM & MIB_GetTableStdType(tableIndex)))
			continue;

		LIST_FOREACH(pEntry, &pTable->entryHead, entries)
	    {
		    if (!pTable->meOper->meOperPmHandler)
		    	continue;

	    	memcpy(&instanceID, pEntry->pData, sizeof(omci_me_instance_t));
	    	isTcaRaised = FALSE;

		    // call per ME PM handler to update TCA
	    	if (GOS_OK != (ret = pTable->meOper->meOperPmHandler(
	    			tableIndex, instanceID, operType, &isTcaRaised)))
	    	{
	    		if (GOS_ERR_NOTSUPPORT != ret)
	    		{
	    			OMCI_LOG(OMCI_LOG_LEVEL_WARN, "PM handler returns fail: %s",
		    			MIB_GetTableName(tableIndex));
	    		}

	    		continue;
	    	}

		    // retrieve alarm table
	    	if (GOS_OK != mib_alarm_table_get(tableIndex, instanceID, &alarmTable))
		    {
		    	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Get alarm table fail: %s, 0x%x",
		    		MIB_GetTableName(tableIndex), instanceID);

		    	continue;
		    }

		    // send TCA if necessary
		    if (OMCI_PM_OPER_UPDATE == operType ||
		    		OMCI_PM_OPER_UPDATE_AND_SWAP == operType)
		    {
			    if (isTcaRaised)
			    {
				    if (GOS_OK !=
				    		omci_alarm_notify_xmit(tableIndex, instanceID, &alarmTable))
				    {
				    	OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Send alarm notify fail: %s, 0x%x",
				    		MIB_GetTableName(tableIndex), instanceID);
				    }
			    }
			}

		    // reset TCA due to SYNC or swap
		    if (OMCI_PM_OPER_RESET == operType ||
		    		OMCI_PM_OPER_SWAP == operType ||
		    		OMCI_PM_OPER_UPDATE_AND_SWAP == operType)
		    {
		    	// extended pm has to handle this by itself
		    	if (MIB_TABLE_ETHEXTPMDATA_INDEX != tableIndex)
		    		omci_pm_clear_all_raised_tca(tableIndex, instanceID);
		    }
	    }
	}
    OMCI_PM_OPER_PROTECT_UNLOCK;
}

#define PM_SWAP_INTERVAL_SEC	(900)
#define PM_UPDATE_INTERVAL_SEC	(5)


static void omci_pm_timer_handler(UINT16	classID,
									UINT16	instanceID,
									UINT32	privData)
{
	struct sysinfo	tSysInfo;
	static long		nextTimeUpInSecs = 0;
	static UINT16	pollingCounterInSecs = 0;
	BOOL			bSyncState;

	if (sysinfo(&tSysInfo))
		return;

	// first use initialize
	if (0 == nextTimeUpInSecs)
		nextTimeUpInSecs = tSysInfo.uptime + PM_SWAP_INTERVAL_SEC;

	omci_pm_sync_request_updater(FALSE, &bSyncState);

	// SYNC message received! clear all PM immediately
	if (bSyncState)
	{
		// 1. reset PM values
		// 2. reset interval end time
		// 3. reset TCA alarms
		// 4. no effect for extended PM in continuous mode
		omci_pm_operation_updater(OMCI_PM_OPER_RESET);

		// set the finish time
		nextTimeUpInSecs = tSysInfo.uptime + PM_SWAP_INTERVAL_SEC;
		pollingCounterInSecs = 1;

		bSyncState = FALSE;
		omci_pm_sync_request_updater(TRUE, &bSyncState);

		return;
	}

	// time's up!
	if (tSysInfo.uptime >= nextTimeUpInSecs)
	{
		// 1. update & swap PM values
		// 2. increase interval end time
		// 3. clear TCA alarms for all
		// 4. no effect for extended PM in continuous mode
		omci_pm_operation_updater(OMCI_PM_OPER_UPDATE_AND_SWAP);

		// TBD
		// 5. raise TCA alarms for ratio-based

		// set the finish time
		nextTimeUpInSecs = tSysInfo.uptime + PM_SWAP_INTERVAL_SEC;
		pollingCounterInSecs = 1;
	}
	else
	{
		// polling PM values ONLY at every pollingInterval
		if (0 == (pollingCounterInSecs % PM_UPDATE_INTERVAL_SEC))
		{
			// 1. update PM values
			// 2. raise TCA alarms for non-ratio-based
			omci_pm_operation_updater(OMCI_PM_OPER_UPDATE);
		}

		pollingCounterInSecs++;
	}
}

GOS_ERROR_CODE omci_pm_timer_init(void)
{
    if(omci_timer_search(OMCI_TIMER_RESERVED_CLASS_ID,
					OMCI_TIMER_RESERVED_PM_INSTANCE_ID))
	{
        return omci_timer_restart(OMCI_TIMER_RESERVED_CLASS_ID,
        		OMCI_TIMER_RESERVED_PM_INSTANCE_ID,
        		OMCI_TIMER_PM_COLLECT_INTERVAL_SECS,
        		0, TRUE);
    }
    else
    {
	return omci_timer_create(OMCI_TIMER_RESERVED_CLASS_ID,
							OMCI_TIMER_RESERVED_PM_INSTANCE_ID,
							OMCI_TIMER_PM_COLLECT_INTERVAL_SECS,
							0, TRUE, 0, omci_pm_timer_handler,OMCI_TIMER_SIG_PM);
    }

}

GOS_ERROR_CODE omci_pm_timer_deinit(void)
{
	return omci_timer_stop(OMCI_TIMER_RESERVED_CLASS_ID,
									OMCI_TIMER_RESERVED_PM_INSTANCE_ID);
}

static GOS_ERROR_CODE omci_pm_task(void		*pData)
{
	GOS_ERROR_CODE	ret;
	BOOL			bTimerRunning;
	BOOL			bPausePlayReq;
#ifdef OMCI_X86
	for (;;)
	{
		sleep(5);
	}
#endif
    omci_timer_setSignalMask(OMCI_TASK_PM_ACCEPT_SIG_MASK);

	// initial PM timer
	if (GOS_OK != omci_pm_timer_init())
		return GOS_FAIL;

	bTimerRunning = TRUE;

    // enter forever loop to wait for the timer event
    for (;;)
    {
    	omci_pm_pause_play_request_updater(FALSE, &bPausePlayReq);

    	if (bPausePlayReq)
    	{
    		if (bTimerRunning)
    			ret = omci_pm_timer_deinit();
    		else
    			ret = omci_pm_timer_init();

    		if (GOS_OK == ret)
    			bTimerRunning = !bTimerRunning;

    		bPausePlayReq = FALSE;
    		omci_pm_pause_play_request_updater(TRUE, &bPausePlayReq);
    	}

    	// sleep 500 millisecs
    	usleep(500000);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_pm_task_init(void)
{
	OMCI_TASK_ID taskID;

	taskID = OMCI_SpawnTask("pm collector",
							omci_pm_task,
							NULL,
							OMCI_TASK_PRI_PM,
							FALSE);

	if (OMCI_TASK_ID_INVALID == taskID)
	{
		return GOS_FAIL;
	}

	return GOS_OK;
}
