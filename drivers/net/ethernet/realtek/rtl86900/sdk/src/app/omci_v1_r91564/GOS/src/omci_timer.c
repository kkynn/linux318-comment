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
 * Purpose : Definition of OMCI timer APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI timer APIs
 */

#include "omci_timer.h"


#define OMCI_TIMER_PRINT(fmt, arg...) \
         do { printf(fmt, ##arg); printf("\n"); } while (0);

#define LOGIC_SIG_TO_SIGRT(_logic_sig_no) ((_logic_sig_no)+(SIGRTMIN+5))

static LIST_HEAD(omci_timer_head_s, omci_timer_entry_s) gOmciTimerList;


int gTimerSocketFd[2];


pthread_mutex_t gOmciTimerListAccessMutex = PTHREAD_MUTEX_INITIALIZER;

omci_timer_entry_t* omci_timer_search(UINT16    classID,
                                        UINT16  instanceID)
{
    omci_timer_entry_t  *pEntry;

    pthread_mutex_lock(&gOmciTimerListAccessMutex);
    LIST_FOREACH(pEntry, &gOmciTimerList, entries)
    {
        if (pEntry->data.classID != classID)
            continue;
        if (pEntry->data.instanceID != instanceID)
            continue;

        pthread_mutex_unlock(&gOmciTimerListAccessMutex);

        return pEntry;
    }
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);

    return NULL;
}

GOS_ERROR_CODE omci_timer_create(UINT16         classID,
									UINT16		instanceID,
									time_t		intervalSecs,
									long		intervalNanoSecs,
                                    BOOL        isRegular,
                                    UINT32      privData,
                                    timerCB_t   cbFunction,
                                    OMCI_TIMER_SIG_NO       sigNo)
{
    struct sigevent     timerSigEvent;
    struct itimerspec   timerParas;
    omci_timer_entry_t	*pEntry;

    if (omci_timer_search(classID, instanceID))
        return GOS_ERR_DUPLICATED;

    pEntry = malloc(sizeof(omci_timer_entry_t));
    if (!pEntry)
    	return GOS_FAIL;

    // create timer

    timerSigEvent.sigev_notify = SIGEV_SIGNAL;
     timerSigEvent.sigev_signo = LOGIC_SIG_TO_SIGRT(sigNo);
     
    timerSigEvent.sigev_value.sival_ptr = &pEntry->data.timerID;
    if (timer_create(CLOCK_MONOTONIC, &timerSigEvent, &pEntry->data.timerID))
        goto free_mem;

    // start the timer
    memset(&timerParas, 0, sizeof(struct itimerspec));
    timerParas.it_value.tv_sec = intervalSecs;
    timerParas.it_value.tv_nsec = intervalNanoSecs;
    if (isRegular)
        timerParas.it_interval = timerParas.it_value;
    if (timer_settime(pEntry->data.timerID, 0, &timerParas, NULL))
        goto del_timer;

    // fill entry content
    pEntry->data.cbFunction = cbFunction;
    pEntry->data.classID = classID;
    pEntry->data.instanceID = instanceID;
    pEntry->data.privData = privData;

    // insert entry to the list
    pthread_mutex_lock(&gOmciTimerListAccessMutex);
    LIST_INSERT_HEAD(&gOmciTimerList, pEntry, entries);
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);

    return GOS_OK;

del_timer:
	timer_delete(pEntry->data.timerID);

free_mem:
	free(pEntry);

	return GOS_FAIL;
}

GOS_ERROR_CODE omci_timer_restart(UINT16        classID,
                                    UINT16      instanceID,
                                    time_t      intervalSecs,
                                    long        intervalNanoSecs,
                                    BOOL        isRegular)
{
    struct itimerspec   timerParas;
    omci_timer_entry_t  *pEntry;

    pEntry = omci_timer_search(classID, instanceID);
    if (!pEntry)
        return GOS_ERR_NOT_FOUND;

    // restart the timer
    memset(&timerParas, 0, sizeof(struct itimerspec));
    timerParas.it_value.tv_sec = intervalSecs;
    timerParas.it_value.tv_nsec = intervalNanoSecs;
    if (isRegular)
        timerParas.it_interval = timerParas.it_value;
    if (timer_settime(pEntry->data.timerID, 0, &timerParas, NULL))
        return GOS_FAIL;

    return GOS_OK;
}


GOS_ERROR_CODE omci_timer_stop(UINT16        classID,
                                    UINT16      instanceID)
{
    struct itimerspec   timerParas;
    omci_timer_entry_t  *pEntry;

    pEntry = omci_timer_search(classID, instanceID);
    if (!pEntry)
        return GOS_ERR_NOT_FOUND;

    // stop the timer
    memset(&timerParas, 0, sizeof(struct itimerspec));

    if (timer_settime(pEntry->data.timerID, 0, &timerParas, NULL))
        return GOS_FAIL;

    return GOS_OK;
}

GOS_ERROR_CODE omci_timer_delete_by_entry(omci_timer_entry_t	*pEntry)
{
	if (!pEntry)
		return GOS_ERR_PARAM;

    // delete timer by timerID
    if (timer_delete(pEntry->data.timerID))
    	return GOS_FAIL;

    // remove entry from the list
    pthread_mutex_lock(&gOmciTimerListAccessMutex);
	LIST_REMOVE(pEntry, entries);
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);

    free(pEntry);

	return GOS_OK;
}

GOS_ERROR_CODE omci_timer_delete_by_id(UINT16	classID,
									   UINT16	instanceID)
{
    omci_timer_entry_t	*pEntry;

    pEntry = omci_timer_search(classID, instanceID);
	if (pEntry)
	    return omci_timer_delete_by_entry(pEntry);

    return GOS_FAIL;
}

GOS_ERROR_CODE omci_timer_stop_and_delete_by_id(UINT16  classID,
                                                UINT16  instanceID,
                                                time_t  *pRemainedSecs,
                                                long    *pRemainedNanoSecs)
{
    struct itimerspec   newTimerParas;
    struct itimerspec   oldTimerParas;
    omci_timer_entry_t  *pEntry;

    if (!pRemainedSecs || !pRemainedNanoSecs)
        return GOS_ERR_PARAM;

    pEntry = omci_timer_search(classID, instanceID);
    if (!pEntry)
        return GOS_ERR_NOT_FOUND;

    // stop the timer
    memset(&newTimerParas, 0, sizeof(struct itimerspec));
    if (timer_settime(pEntry->data.timerID, 0, &newTimerParas, &oldTimerParas))
        return GOS_FAIL;

    // return the remained time
    *pRemainedSecs = oldTimerParas.it_value.tv_sec;
    *pRemainedNanoSecs = oldTimerParas.it_value.tv_nsec;

    return omci_timer_delete_by_entry(pEntry);
}









/*Realtime signal handle*/
static void omci_timer_sigRtHandle(int sig, siginfo_t *si, void *uc)
{
    int ret;
    int savedErrno = errno ;
    timer_t *pTimerID = si->si_value.sival_ptr;

    if (sig >= (LOGIC_SIG_TO_SIGRT(OMCI_TIMER_SIG_END)) || 
        sig < (LOGIC_SIG_TO_SIGRT(OMCI_TIMER_SIG_MIN)) || !si)
    {
        return;
    }

    ret = send(gTimerSocketFd[1],(char*)pTimerID,sizeof(timer_t),0);

    if(ret < 0)
        printf("Error send: %d %s\n", errno, strerror( errno ));
    
    errno = savedErrno;
}




GOS_ERROR_CODE omci_timer_init(void)
{
    struct sigaction    timerSigAct;
    OMCI_TIMER_SIG_NO signo;
    GOS_ERROR_CODE ret = GOS_OK;

    if(socketpair(PF_UNIX,SOCK_STREAM,0,gTimerSocketFd) == -1)
    {
        OMCI_TIMER_PRINT("Init OMCI timer socket fail");
        return GOS_FAIL;
    }
    
    memset(&timerSigAct, 0, sizeof(struct sigaction));
    timerSigAct.sa_flags = SA_SIGINFO | SA_RESTART;
    timerSigAct.sa_sigaction = omci_timer_sigRtHandle;

    for(signo = 0;signo < OMCI_TIMER_SIG_END ;signo++)
    {
        if (sigaction(LOGIC_SIG_TO_SIGRT(signo), &timerSigAct, NULL))
        {
            OMCI_TIMER_PRINT("OMCI sigaction %d %d fail",signo,LOGIC_SIG_TO_SIGRT(signo));
            ret = GOS_FAIL;
        }
    }
    pthread_mutex_lock(&gOmciTimerListAccessMutex);
    LIST_INIT(&gOmciTimerList);
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);

    return ret;

}


GOS_ERROR_CODE omci_timer_setSignalMask(int sigmask)
{
    sigset_t blockSet;   
    int signo;
    sigemptyset(&blockSet);
    for(signo = 0;signo < OMCI_TIMER_SIG_END;signo++)
    {
        if(!((sigmask >> signo) & 0x1)){
            sigaddset(&blockSet,LOGIC_SIG_TO_SIGRT(signo));
        }
    }

    pthread_sigmask(SIG_SETMASK,&blockSet,NULL); 
    
    return GOS_OK;
}


GOS_ERROR_CODE omci_timer_deinit(void)
{
    struct sigaction    timerSigAct;
    omci_timer_entry_t	*pEntry;
    OMCI_TIMER_SIG_NO signo;

    pthread_mutex_lock(&gOmciTimerListAccessMutex);
    pEntry = LIST_FIRST(&gOmciTimerList);
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);
    while (pEntry)
    {
    	// delete timer by entry
    	if (GOS_OK == omci_timer_delete_by_entry(pEntry))
        {
            pthread_mutex_lock(&gOmciTimerListAccessMutex);
        	pEntry = LIST_FIRST(&gOmciTimerList);
            pthread_mutex_unlock(&gOmciTimerListAccessMutex);
        }
        else
        {
            pthread_mutex_lock(&gOmciTimerListAccessMutex);
        	pEntry = LIST_NEXT(pEntry, entries);
            pthread_mutex_unlock(&gOmciTimerListAccessMutex);
        }
    }

    close(gTimerSocketFd[0]);
    close(gTimerSocketFd[1]);

    // restore the signal handler

    memset(&timerSigAct, 0, sizeof(struct sigaction));
    timerSigAct.sa_handler = SIG_DFL;

    for(signo = 0;signo < OMCI_TIMER_SIG_END ;signo++)
    {
        if (sigaction(LOGIC_SIG_TO_SIGRT(signo), &timerSigAct, NULL))
        return GOS_FAIL;
    }

    return GOS_OK;
}



void omci_timer_executed_cb(timer_t pTimerID)
{
    omci_timer_entry_t	*pEntry;

    pthread_mutex_lock(&gOmciTimerListAccessMutex);
	LIST_FOREACH(pEntry, &gOmciTimerList, entries)
	{
        if (pEntry->data.timerID == pTimerID)
        {
    	    // invoke handler
    	    pthread_mutex_unlock(&gOmciTimerListAccessMutex);
            if (pEntry->data.cbFunction)
            {
                pEntry->data.cbFunction(pEntry->data.classID,
                                        pEntry->data.instanceID,
                                        pEntry->data.privData);
            }
            return;

        }
	}
    pthread_mutex_unlock(&gOmciTimerListAccessMutex);
}
