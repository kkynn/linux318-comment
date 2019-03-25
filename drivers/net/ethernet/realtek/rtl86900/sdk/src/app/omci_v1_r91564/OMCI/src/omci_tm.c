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
 * Purpose : Definition of OMCI transceiver monitoring APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI transceiver monitoring APIs
 */

#include "app_basic.h"
#include "omci_task.h"


BOOL			gbOmciTmPausePlayRequest = FALSE;
pthread_mutex_t	gOmciTmPausePlayRequestMutex = PTHREAD_MUTEX_INITIALIZER;

void omci_tm_pause_play_request_updater(BOOL bWrite, BOOL *bPausePlayState)
{
	pthread_mutex_lock(&gOmciTmPausePlayRequestMutex);

	if (bWrite)
		gbOmciTmPausePlayRequest = *bPausePlayState;
	else
		*bPausePlayState = gbOmciTmPausePlayRequest;

	pthread_mutex_unlock(&gOmciTmPausePlayRequestMutex);
}

static void omci_tm_updater(UINT16	classID,
							UINT16	instanceID,
							omci_alm_nbr_anig_t alarm_no)
{
	omci_alm_t			alarmMsg;
    MIB_TABLE_ANIG_T    mibAniG;
	omci_alm_status_t	status;


    mibAniG.EntityID = instanceID;

	if(GOS_OK != MIB_Get(MIB_TABLE_ANIG_INDEX, &mibAniG, sizeof(mibAniG)))
	{
		OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() returns fail", __FUNCTION__);
		return;
	}

	switch(alarm_no)
	{
		case OMCI_ALM_NBR_ANIG_LOW_RX_OPTICAL_PWR:
		{
			double  value;
			unsigned char applyThreshold;

			if (GOS_OK != anig_generic_transceiver_para_updater(
				&value, OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER))
				return;

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), alamNo=%u, rx_pwr=%f ", __FUNCTION__,
				OMCI_ALM_NBR_ANIG_LOW_RX_OPTICAL_PWR, value);

			if (ANIG_DFL_RX_THR == mibAniG.LowOpThreshold)
				applyThreshold = mibAniG.LowDflRxThreshold;
			else
				applyThreshold = mibAniG.LowOpThreshold;

			status = ((value < (((double)applyThreshold) * (-0.5))) ?
				OMCI_ALM_STS_DECLARE : OMCI_ALM_STS_CLEAR);

			break;
		}
		case OMCI_ALM_NBR_ANIG_HIGH_RX_OPTICAL_PWR:
		{
			double value;
			unsigned char applyThreshold;

			if (GOS_OK != anig_generic_transceiver_para_updater(
				&value, OMCI_TRANSCEIVER_STATUS_TYPE_RX_POWER))
				return;

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), alamNo=%u, rx_pwr=%f ", __FUNCTION__,
				OMCI_ALM_NBR_ANIG_HIGH_RX_OPTICAL_PWR, value);

			if (ANIG_DFL_RX_THR == mibAniG.UppOpThreshold)
				applyThreshold = mibAniG.UppDflRxThreshold;
			else
				applyThreshold = mibAniG.UppOpThreshold;

			status = ((value > (((double)applyThreshold) * (-0.5))) ?
				OMCI_ALM_STS_DECLARE : OMCI_ALM_STS_CLEAR);

			break;
		}
		case OMCI_ALM_NBR_ANIG_LOW_TX_OPTICAL_PWR:
		{
			double value;
			unsigned char applyThreshold;

			if (GOS_OK != anig_generic_transceiver_para_updater(
				&value, OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER))
				return;

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), alamNo=%u, tx_pwr=%f ", __FUNCTION__,
				OMCI_ALM_NBR_ANIG_LOW_TX_OPTICAL_PWR, value);

			if (ANIG_DFL_TX_THR == mibAniG.LowTranPowThreshold)
				applyThreshold = mibAniG.LowDflTxThreshold;
			else
				applyThreshold = mibAniG.LowTranPowThreshold;

			//-63.5*2 = -127 = 0x81
			status = ((value < (applyThreshold < 128 ?
				((double)applyThreshold * 0.5) : ((double)applyThreshold * (-0.5)) )) ?
				OMCI_ALM_STS_DECLARE : OMCI_ALM_STS_CLEAR);

			break;
		}
		case OMCI_ALM_NBR_ANIG_HIGH_TX_OPTICAL_PWR:
		{
			double	value;
			unsigned char applyThreshold;

			if (GOS_OK != anig_generic_transceiver_para_updater(
				&value, OMCI_TRANSCEIVER_STATUS_TYPE_TX_POWER))
				return;

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), alamNo=%u, tx_pwr=%f ", __FUNCTION__,
				OMCI_ALM_NBR_ANIG_HIGH_TX_OPTICAL_PWR, value);

			if (ANIG_DFL_TX_THR == mibAniG.UppTranPowThreshold)
				applyThreshold = mibAniG.UppDflTxThreshold;
			else
				applyThreshold = mibAniG.UppTranPowThreshold;

			status = ((value > (applyThreshold < 128 ?
				((double)applyThreshold * 0.5) : ((double)applyThreshold * (-0.5)) )) ?
				OMCI_ALM_STS_DECLARE : OMCI_ALM_STS_CLEAR);

			break;
		}
		case OMCI_ALM_NBR_ANIG_LASER_BIAS_CURRENT:
		{
			double  value;

			if (GOS_OK != anig_generic_transceiver_para_updater(
				&value, OMCI_TRANSCEIVER_STATUS_TYPE_BIAS_CURRENT))
				return;

			OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s(), alamNo=%u, bias_cur=%f ", __FUNCTION__,
				OMCI_ALM_NBR_ANIG_LASER_BIAS_CURRENT, value);

			status = ((value > 50/2*1000) ? OMCI_ALM_STS_DECLARE : OMCI_ALM_STS_CLEAR);

			break;
		}
		default:
			return;
	}

	memset(&alarmMsg, 0x0, sizeof(omci_alm_t));
	alarmMsg.almType = OMCI_ALM_TYPE_ANI_G;
	alarmMsg.almData.almNumber = alarm_no;
	alarmMsg.almData.almStatus = status;

	omci_ext_alarm_dispatcher(&alarmMsg);
	return;
}

static void omci_tm_timer_handler(UINT16	classID,
									UINT16	instanceID,
									UINT32	privData)
{
	PON_ONU_STATE onuState;
	GOS_ERROR_CODE	ret;

	ret = omci_wrapper_getOnuState(&onuState);

	if(GOS_OK == ret && PON_ONU_STATE_OPERATION == onuState)
	{
		omci_alm_nbr_anig_t alarm_no;

		for(alarm_no = OMCI_ALM_NBR_ANIG_MINIMUM;
			alarm_no <= OMCI_ALM_NBR_ANIG_MAXIMUM; alarm_no++)
		{
			//TBD SF/SD
			if(OMCI_ALM_NBR_ANIG_SF == alarm_no ||
				OMCI_ALM_NBR_ANIG_SD == alarm_no)
				continue;

			pthread_mutex_lock(&gOmciTmPausePlayRequestMutex);
			omci_tm_updater(classID, instanceID, alarm_no);
			pthread_mutex_unlock(&gOmciTmPausePlayRequestMutex);
		}
	}
	return;
}


GOS_ERROR_CODE omci_tm_timer_init(void)
{
    if(omci_timer_search(
        MIB_GetTableClassId(MIB_TABLE_ANIG_INDEX),
					((TXC_CARDHLD_PON_SLOT_TYPE_ID << 8) | 1)))
	{
        return omci_timer_restart(
            MIB_GetTableClassId(MIB_TABLE_ANIG_INDEX),
        		((TXC_CARDHLD_PON_SLOT_TYPE_ID << 8) | 1),
        		OMCI_TIMER_TM_INTERVAL_SECS,
        		0, TRUE);
    }
    else
    {
	return omci_timer_create(
		MIB_GetTableClassId(MIB_TABLE_ANIG_INDEX),
		((TXC_CARDHLD_PON_SLOT_TYPE_ID << 8) | 1),
		OMCI_TIMER_TM_INTERVAL_SECS,
    		0, TRUE, 0, omci_tm_timer_handler,OMCI_TIMER_SIG_TM);
    }


}

GOS_ERROR_CODE omci_tm_timer_deinit(void)
{
	return omci_timer_stop(
		MIB_GetTableClassId(MIB_TABLE_ANIG_INDEX),
		((TXC_CARDHLD_PON_SLOT_TYPE_ID << 8) | 1));
}

static GOS_ERROR_CODE omci_tm_task(void *pData)
{
	GOS_ERROR_CODE	ret;
	BOOL			bPausePlayReq;

    omci_timer_setSignalMask(OMCI_TASK_TM_ACCEPT_SIG_MASK);
    
	// initial TM timer
	if (GOS_OK != omci_tm_timer_init())
		return GOS_FAIL;

    // enter forever loop to wait for the timer event
    for (;;)
    {
    	omci_tm_pause_play_request_updater(FALSE, &bPausePlayReq);

    	if (bPausePlayReq)
    	{
    		if (gInfo.tmTimerRunningB)
    			ret = omci_tm_timer_deinit();
			else
    			ret = omci_tm_timer_init();

    		if (GOS_OK == ret)
    			gInfo.tmTimerRunningB = !gInfo.tmTimerRunningB;

    		bPausePlayReq = FALSE;
    		omci_tm_pause_play_request_updater(TRUE, &bPausePlayReq);
    	}

    	// sleep 500 millisecs
    	usleep(500000);
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_tm_task_init(void)
{
	OMCI_TASK_ID taskID;

	taskID = OMCI_SpawnTask("transceiver monitor",
							omci_tm_task,
							NULL,
							OMCI_TASK_PRI_TM,
							FALSE);

	if (OMCI_TASK_ID_INVALID == taskID)
	{
		return GOS_FAIL;
	}

	return GOS_OK;
}
