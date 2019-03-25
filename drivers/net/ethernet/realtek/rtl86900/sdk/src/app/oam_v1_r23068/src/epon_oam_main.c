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
 * $Revision: 46475 $
 * $Date: 2014-02-14 11:03:12 +0800 (?±ä?, 14 äºŒæ? 2014) $
 *
 * Purpose : Main function of the EPON OAM protocol stack user application
 *           It create two additional threads for packet Rx and state control
 *
 * Feature : Start point of the EPON OAM protocol stack. Use individual threads
 *           for packet Rx and state control
 *
 */

/*
 * Include Files
 */
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>

#include <rtk/l2.h>
#include <rtk/trap.h>
#include <rtk/epon.h>
#include <rtk/pon_led.h>
#include <rtk/oam.h>
#include <hal/common/halctrl.h>

#include "epon_oam_config.h"
#include "epon_oam_rx.h"
#include "epon_oam_db.h"
#include "epon_oam_msgq.h"
#include "epon_oam_dbg.h"
#include "epon_oam_err.h"

#include "ctc_oam.h"
#include "tk_oam.h"

#include "epon_oam_igmp.h"
#include "epon_oam_igmp_db.h"

#ifdef YUEME_CUSTOMIZED_CHANGE
#include <sys/types.h>
#include <fcntl.h>
#endif

/* 
 * Symbol Definition 
 */
#define EPON_OAM_STATEKEEPER_SIG_START          (SIGRTMIN+3)
#define EPON_OAM_HOLDOVER_SIG_START				(EPON_OAM_STATEKEEPER_SIG_START+EPON_OAM_SUPPORT_LLID_NUM)
#define EPON_OAM_DOWNLOADACKRESEND_SIG_START	(EPON_OAM_HOLDOVER_SIG_START+EPON_OAM_SUPPORT_LLID_NUM)
#define EPON_OAM_TWPOWERENABLE_SIG_NUM			(EPON_OAM_DOWNLOADACKRESEND_SIG_START + EPON_OAM_SUPPORT_LLID_NUM)
#define EPON_OAM_DOWNLOADACKRESEND_TIME         (1000) /* In unit of ms */
#define EPON_OAM_KEEPALIVE_TIME                 (5000) /* In unit of ms */

typedef struct epon_inputParam_s {
    unsigned char mac[EPON_OAM_SUPPORT_LLID_NUM][6];
} epon_inputParam_t;

/*  
 * Data Declaration  
 */
static int msgQId;
static timer_t skKeepalive[EPON_OAM_SUPPORT_LLID_NUM];
static timer_t skHoldover[EPON_OAM_SUPPORT_LLID_NUM];
static unsigned int failoverTime;
static struct timespec failoverTimeSpec;
static unsigned int backoffTime;
static struct timespec backoffTimeSpec;
static unsigned char *paramPrompt[] = {
    "-mac",
    NULL
};
int isOnuSilentMode;
static int silentTime;
static int enableSilentMode;
static int silentLedMode;

extern ctc_infoOamVer_t currCtcVer[EPON_OAM_SUPPORT_LLID_NUM];
extern ctc_onuSnInfo_t ctcOnuSn;

static timer_t downloadAckResend[EPON_OAM_SUPPORT_LLID_NUM];
unsigned char downloadAckBuff[EPON_OAM_EVENTDATA_MAX]; /* FIXME: there are only one llid download at the same time*/
unsigned int downloadAckSize = 0;

/* for ctc command ONUTxPowerSupplyControl(0xC7/0x00A1), re-enable tx-power after specified seconds */
static timer_t txpowerEnable;

/* registerNum: save onu registeration times (oam discovery complete)
   registerSuccNum: save onu registeration successful times (oam discovery complete and ctc auth success) */
static unsigned int registerNum[EPON_OAM_SUPPORT_LLID_NUM] = {0};
static unsigned int registerSuccNum[EPON_OAM_SUPPORT_LLID_NUM] = {0};
static const char REGISTER_NUM_FILE[] = "/tmp/oam_register_num";

/* epon link up time */
static int epon_link_up[EPON_OAM_SUPPORT_LLID_NUM] = {0}; /* 0: link down, 1: link up */
static struct timeval epon_uptime_start[EPON_OAM_SUPPORT_LLID_NUM];
static const char EPON_UPTIME_FILE[] = "/tmp/epon_uptime";

/* 
 * Macro Definition 
 */
#define EPON_TIMER_SET(timerId,ms,ret)              \
do {                                                \
    struct itimerspec its;                          \
                                                    \
    its.it_value.tv_sec = ms/1000;                  \
    its.it_value.tv_nsec = (ms%1000) * 1000000;     \
    its.it_interval.tv_sec = its.it_value.tv_sec;   \
    its.it_interval.tv_nsec = its.it_value.tv_nsec; \
    ret = timer_settime(timerId, 0, &its, NULL);    \
} while(0)

static void epon_oam_llid_dereg(unsigned char llidIdx);

static int epon_oam_paraParseMac(int argc, char *argv[], epon_inputParam_t *pParam, int *pNumParsed)
{
    int i;
    unsigned char *pPtr;
    char tmp[3];
    int llidIdx;
    
    /* parse -mac n aabbccddeeff */
    if(argc >= 3)
    {
        *pNumParsed = 3;
        llidIdx = atoi(argv[1]);
        if(llidIdx < EPON_OAM_SUPPORT_LLID_NUM)
        {
            if(strlen(argv[2]) == 12)
            {
                tmp[2] = '\0';
                for(i = 0, pPtr = argv[2]; i < 6; i++, pPtr += 2)
                {
                    tmp[0] = pPtr[0];
                    tmp[1] = pPtr[1];
                    pParam->mac[llidIdx][i] = strtoul(&tmp[0], NULL, 16);
                }
				
                return EPON_OAM_ERR_OK;
            }
        }
    }
    return EPON_OAM_ERR_PARAM;
}

static int (*paramCb[])(int argc, char *argv[], epon_inputParam_t *pParam, int *pNumParsed) = {
    epon_oam_paraParseMac,
    NULL
};

static void dev_link_change_notify(int linkup)
{
    char cmdStr[50];

    if (linkup)
    {
        snprintf(cmdStr, 50, "ethctl enable_nas0_wan 1");
    }
    else
    {
        snprintf(cmdStr, 50, "ethctl enable_nas0_wan 0");
    }

    system(cmdStr);
}

static int epon_oam_paraParse(int argc, char *argv[], epon_inputParam_t *pParam)
{
    int i, j;
    int ret;
    int numParsed;
    unsigned char *pPtr;

    for(i = 1 ; i < argc ; )
    {
        for(j = 0 ; j < paramPrompt[j] != NULL ; j ++)
        {
            pPtr = paramPrompt[j];
            if(!strcmp(pPtr, argv[i]))
            {
                numParsed = 0;
                ret = paramCb[j](argc - i, &argv[i], pParam, &numParsed);
                if(ret)
                {
                    return EPON_OAM_ERR_PARAM;
                }
                i += numParsed;
            }
            else
            {
                i ++;
            }
        }
    }
    return EPON_OAM_ERR_OK;
}

/*  
 * Function Declaration  
 */
/* Function Name:
 *      epon_oam_user_init
 * Description:
 *      Execute user specific initializations
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      EPON_OAM_ERR_OK
 *      ...
 * Note:
 *      All user initialization function should put in here
 */
int epon_oam_user_init(void)
{
    /* For CTC OAM initialization */
    ctc_oam_init();
#if EPON_OAM_SUPPORT_TK
    /* For TK OAM initialization */
    tk_oam_init();
#endif
#if EPON_OAM_SUPPORT_KT
    /* For KT OAM initialization */
    kt_oam_init();
#endif
#if EPON_OAM_SUPPORT_CORTINA
    /* For CORTINA OAM initialization */
    cortina_oam_init();
#endif
}

/* Function Name:
 *      epon_oam_userDb_init
 * Description:
 *      Execute user specific database initializations
 * Input:
 *      None
 * Output:
 *      None
 * Return:
 *      EPON_OAM_ERR_OK
 *      ...
 * Note:
 *      All user database initialization function should put in here
 *      This initialization function will be call every time the specific LLID
 *      is link down
 */
int epon_oam_userDb_init(
    unsigned char llidIdx)
{
    /* For CTC OAM initialization */
    ctc_oam_db_init(llidIdx);
#if EPON_OAM_SUPPORT_TK
    /* For TK OAM initialization */
    tk_oam_db_init(llidIdx);
#endif
#if EPON_OAM_SUPPORT_KT
    /* For KT OAM initialization */
    kt_oam_db_init(llidIdx);
#endif
#if EPON_OAM_SUPPORT_CORTINA
    /* For CORTINA OAM initialization */
    cortina_oam_db_init(llidIdx);
#endif
}

/* ------------------------------------------------------------------------- */
/* Internal APIs */
int epon_oam_event_send(
    unsigned char llidIdx,
    unsigned int eventId)
{
    int ret;
    oam_msgqEventData_t event;

    event.mtype = eventId;
    event.msgqData.llidIdx = llidIdx;
    event.msgqData.dataSize = 0;
    ret = msgsnd(msgQId, (void *)&event, sizeof(event.msgqData), IPC_NOWAIT);
    if(-1 == ret)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "[OAM:%s:%d] msgsnd failed %d\n", __FILE__, __LINE__, errno);
        return EPON_OAM_ERR_MSGQ;
    }

    return EPON_OAM_ERR_OK;
}

int epon_oam_eventData_send(
    unsigned char llidIdx,
    unsigned int eventId,
    unsigned char *data,
    unsigned short dataLen)
{
    int ret;
    oam_msgqEventData_t eventData;

    if(dataLen > EPON_OAM_EVENTDATA_MAX)
    {
        return EPON_OAM_ERR_PARAM;
    }

    eventData.mtype = eventId;
    eventData.msgqData.llidIdx = llidIdx;
    eventData.msgqData.dataSize = dataLen;
    memcpy(eventData.msgqData.data, data, dataLen);
    ret = msgsnd(msgQId, (void *)&eventData, sizeof(eventData.msgqData), IPC_NOWAIT);
    if(-1 == ret)
    {
        printf("[OAM:%s:%d] msgsnd failed %d\n", __FILE__, __LINE__, errno);
        return EPON_OAM_ERR_MSGQ;
    }
	
    return EPON_OAM_ERR_OK;
}

static void epon_oam_keepalive_timeout(
    int sig,
    siginfo_t *si,
    void *uc)
{
    epon_oam_event_send(sig - EPON_OAM_STATEKEEPER_SIG_START, EPON_OAM_EVENT_KEEPALIVE_TIMEOUT);
    return;
}

static void epon_oam_holdover_timeout(
    int sig)
{
    epon_oam_event_send(sig - EPON_OAM_HOLDOVER_SIG_START, EPON_OAM_EVENT_HOLDOVER_TIMEOUT);
    return;
}

static void epon_oam_download_ack_resend_timeout(
    int sig)
{
	int ret;
	int llidIdx;
	
	if(downloadAckSize > 0)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                    "[OAM:%s:%d] download ack resend, size[%d]\n",
                    __FILE__, __LINE__,downloadAckSize);

		
	    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
	            "[OAM:%s:%d] OAMPDU Tx\n", __FILE__, __LINE__);
	    DUMP_HEX_VALUE(EPON_OAM_DBGFLAG_CTC_INFO, downloadAckBuff, downloadAckSize);
	    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "\n");

		llidIdx = sig - EPON_OAM_DOWNLOADACKRESEND_SIG_START;
		ret = epon_oam_reply_send(llidIdx, EPON_OAMPDU_CODE_ORG_SPEC, downloadAckBuff, downloadAckSize);
   		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_INFO, 
            	"[OAM:%s:%d] epon_oam_reply_send %d\n", __FILE__, __LINE__, EPON_OAMPDU_CODE_ORG_SPEC);
		EPON_TIMER_SET(downloadAckResend[llidIdx], 0, ret);
	}
    return;
}

static void epon_oam_tx_power_enable_timeout(
    int sig)
{
	int ret;
	unsigned int gpio_disTx;
	
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                   "[OAM:%s:%d] re-enable tx power\n",
                    __FILE__, __LINE__);

	/* re-enable the Tx power supply:set ds_tx gpio to 0 */
	ctc_oam_gpio_tx_disable_pin_get(&gpio_disTx);
	rtk_gpio_databit_set(gpio_disTx, 0);
	
	EPON_TIMER_SET(txpowerEnable, 0, ret);
	
    return;
}

static void epon_stateKeeper_timer_init(void)
{
    int i;
    sigset_t sigMask;
    struct sigaction sa;
    struct sigevent sevp;

    /* Create EPON_OAM_SUPPORT_LLID_NUM timers for each LLID
     * Each of the timer uses a signal to tell the difference
     * TODO - try single signal implementation
     */
    for(i = 0;i < EPON_OAM_SUPPORT_LLID_NUM;i++)
    {
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = epon_oam_keepalive_timeout;
        sigemptyset(&sa.sa_mask);
        /* Linux thread imeplementation uses SIGRTMIN ~ SIGRTMIN + 2, skip them */
        if(-1 == sigaction(EPON_OAM_STATEKEEPER_SIG_START + i, &sa, NULL))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] signal action set failed %d %d\n", __FILE__, __LINE__, errno, i);
            continue;
        }

        sevp.sigev_notify = SIGEV_SIGNAL;
        sevp.sigev_signo = EPON_OAM_STATEKEEPER_SIG_START + i;
        sevp.sigev_value.sival_ptr = &skKeepalive[i];
        if(-1 == timer_create(CLOCK_REALTIME, &sevp, &skKeepalive[i]))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] keepalive timer create failed %d\n", __FILE__, __LINE__, errno);
            return;
        }
    }
}

static void epon_holdover_timer_init(void)
{
    int i;
    sigset_t sigMask;
    struct sigaction sa;
    struct sigevent sevp;

    /* Create EPON_OAM_SUPPORT_LLID_NUM timers for each LLID
     * Each of the timer uses a signal to tell the difference
     */
    for(i = 0;i < EPON_OAM_SUPPORT_LLID_NUM;i++)
    {
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = epon_oam_holdover_timeout;
        sigemptyset(&sa.sa_mask);
        /* Linux thread imeplementation uses SIGRTMIN ~ SIGRTMIN + 2, skip them */
        if(-1 == sigaction(EPON_OAM_HOLDOVER_SIG_START + i, &sa, NULL))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] signal action set failed %d %d\n", __FILE__, __LINE__, errno, i);
            continue;
        }

        sevp.sigev_notify = SIGEV_SIGNAL;
        sevp.sigev_signo = EPON_OAM_HOLDOVER_SIG_START + i;
        sevp.sigev_value.sival_ptr = &skHoldover[i];
        if(-1 == timer_create(CLOCK_REALTIME, &sevp, &skHoldover[i]))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] holdover timer create failed %d\n", __FILE__, __LINE__, errno);
            return;
        }
    }
}

static void epon_download_ack_resend_timer_init(void)
{
	int i;
    sigset_t sigMask;
    struct sigaction sa;
    struct sigevent sevp;

    /* Create EPON_OAM_SUPPORT_LLID_NUM timers for each LLID
     * Each of the timer uses a signal to tell the difference
     */
    for(i = 0;i < EPON_OAM_SUPPORT_LLID_NUM;i++)
    {
    	sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = epon_oam_download_ack_resend_timeout;
        sigemptyset(&sa.sa_mask);
        /* Linux thread imeplementation uses SIGRTMIN ~ SIGRTMIN + 2, skip them */
        if(-1 == sigaction(EPON_OAM_DOWNLOADACKRESEND_SIG_START + i, &sa, NULL))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] signal action set failed %d %d\n", __FILE__, __LINE__, errno, i);
            continue;
        }

		sevp.sigev_notify = SIGEV_SIGNAL;
        sevp.sigev_signo = EPON_OAM_DOWNLOADACKRESEND_SIG_START + i;
        sevp.sigev_value.sival_ptr = &downloadAckResend[i];
        if(-1 == timer_create(CLOCK_REALTIME, &sevp, &downloadAckResend[i]))
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] download ack resend timer create failed %d\n", __FILE__, __LINE__, errno);
            return;
        }
    }
}

static void epon_tx_power_enable_timer_init(void)
{
	int i;
    sigset_t sigMask;
    struct sigaction sa;
    struct sigevent sevp;
    
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = epon_oam_tx_power_enable_timeout;
    sigemptyset(&sa.sa_mask);
    /* Linux thread imeplementation uses SIGRTMIN ~ SIGRTMIN + 2, skip them */
    if(-1 == sigaction(EPON_OAM_TWPOWERENABLE_SIG_NUM, &sa, NULL))
    {
    	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "[OAM:%s:%d] signal action set failed %d %d\n", __FILE__, __LINE__, errno, i);
        return;
    }

	sevp.sigev_notify = SIGEV_SIGNAL;
    sevp.sigev_signo = EPON_OAM_TWPOWERENABLE_SIG_NUM;
    sevp.sigev_value.sival_ptr = &txpowerEnable;
    if(-1 == timer_create(CLOCK_REALTIME, &sevp, &txpowerEnable))
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "[OAM:%s:%d] download ack resend timer create failed %d\n", __FILE__, __LINE__, errno);
        return;
    }
}

void epon_oam_register_number_increase(unsigned char llidIdx)
{
	registerNum[llidIdx]++;
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
            "[OAM:%s:%d] llid[%d] registerNum[%d]\n", __FILE__, __LINE__, llidIdx, registerNum[llidIdx]);
}

void epon_oam_register_success_number_increase(unsigned char llidIdx)
{
	registerSuccNum[llidIdx]++;
	EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
            "[OAM:%s:%d] llid[%d] registerSuccNum[%d]\n", __FILE__, __LINE__, llidIdx, registerSuccNum[llidIdx]);	
}

void epon_oam_register_number_reset(unsigned char llidIdx)
{
	registerNum[llidIdx] = 0;
	registerSuccNum[llidIdx] = 0;
}

#ifdef YUEME_CUSTOMIZED_CHANGE
/* operations for bsd flock(), also used by the kernel implementation */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* or'd with one of the above to prevent blocking */
#define LOCK_UN		8	/* remove lock */

static int unlock_file_by_flock(int lockfd)
{
	while (flock(lockfd, LOCK_UN) == -1 && errno==EINTR) {
		usleep(1000);
		printf("write unlock failed by flock. errno=%d\n", errno);
	}
	close(lockfd);
	return 0;
}

static int lock_file_by_flock(char *filename, int wait)
{
	int lockfd;

	if ((lockfd = open(filename, O_RDWR|O_CREAT)) == -1) {
		perror("open oam lockfile");
		return lockfd;
	}

	if(wait)
	{
		 while (flock(lockfd, LOCK_EX) == -1 && errno==EINTR) {  //wait util lock can been use.
		 	usleep(1000);
		 	printf("file %s write lock failed by flock. errno=%d\n", filename, errno);
		 }		
	}
	else
	{
		if(flock(lockfd, LOCK_EX|LOCK_NB) == -1)
		{
			close(lockfd);  //if failed to lock it. close lockfd.
			printf("fail to lock file %s\n", filename);
			return -1;
		}
	}

	return lockfd;
}
#endif

void epon_oam_register_number_show(unsigned char llidIdx, unsigned int toFile)
{
	if(toFile)
	{
#ifdef YUEME_CUSTOMIZED_CHANGE
		char tmp[128]={0};
		int fd = lock_file_by_flock(REGISTER_NUM_FILE, 1);
		if(fd<0)
		{
			printf("%s %d: flock failed!\n", __FUNCTION__, __LINE__);
			return;
		}
		snprintf(tmp, sizeof(tmp), "llid=%d\nregisterNum=%d\nregisterSuccNum=%d\n", llidIdx, registerNum[llidIdx], registerSuccNum[llidIdx]);
		write(fd, tmp, strlen(tmp));
		unlock_file_by_flock(fd);
#else
		FILE* fp;
		if(!(fp=fopen(REGISTER_NUM_FILE,"w")))
		{
			return;
		}
		fprintf(fp, "llid=%d\nregisterNum=%d\nregisterSuccNum=%d\n", llidIdx, registerNum[llidIdx],registerSuccNum[llidIdx]);
		fclose(fp);
#endif
	}
	else
	{
		printf("llid=%d\nregisterNum=%d\nregisterSuccNum=%d\n", llidIdx, registerNum[llidIdx], registerSuccNum[llidIdx]);
	}
}

void epon_oam_uptime_show(unsigned char llidIdx, unsigned int toFile)
{
	long uptime;
	
	if(epon_link_up[llidIdx])
	{
		struct timeval current;
		gettimeofday(&current, NULL);
		uptime = current.tv_sec - epon_uptime_start[llidIdx].tv_sec;
	}
	else
		uptime = -1;
	
	if(toFile)
	{
		FILE* fp;
		if(!(fp=fopen(EPON_UPTIME_FILE,"w")))
		{
			return;
		}
		fprintf(fp, "llid=%d\nuptime=%d\n", llidIdx, uptime);
		fclose(fp);
	}
	else
	{
		printf("llid=%d\nuptime=%d\n", llidIdx, uptime);
	}
}

#ifdef SUPPORT_OAM_EVENT_TO_KERNEL
int epon_oam_state_event_send(unsigned char llidIdx, rtk_epon_fsm_status_t state)
{
	rtk_epon_oam_event_t event;

	event.llidIdx = llidIdx;
	event.eventType = RTK_EPON_OAM_STATE;
	event.eventData[0] = state;
	return epon_oam_notify_event_send(&event);	
}
#endif

static int epon_oam_triggerMpcp_reg(
    unsigned char llidIdx)
{
    int ret;
    rtk_epon_llid_entry_t llidEntry;
    rtk_epon_regReq_t regEntry;
    oam_config_t oamConfig;

    /* Trigger register */
    llidEntry.llidIdx = llidIdx;
    ret = rtk_epon_llid_entry_get(&llidEntry);
    if(ret)
    {
        return ret;
    }

    epon_oam_config_get(llidIdx, &oamConfig);
    if((oamConfig.macAddr[0] != 0) ||
       (oamConfig.macAddr[1] != 0) ||
       (oamConfig.macAddr[2] != 0) ||
       (oamConfig.macAddr[3] != 0) ||
       (oamConfig.macAddr[4] != 0) ||
       (oamConfig.macAddr[5] != 0))
    {
        /* MAC address has been configured through input parameter
         * Overwrite the record in EPON LLID table
         */
        memcpy(&llidEntry.mac, oamConfig.macAddr, sizeof(llidEntry.mac));
    }

    llidEntry.valid = DISABLED;
    llidEntry.llid = 0x7fff;
    ret = rtk_epon_llid_entry_set(&llidEntry);
    if(ret)
    {
        return ret;
    }

    ret = rtk_epon_registerReq_get(&regEntry);
    if(ret)
    {
        return ret;
    }
    regEntry.llidIdx = llidIdx;
    regEntry.doRequest = ENABLED;
    ret = rtk_epon_registerReq_set(&regEntry);
    if(ret)
    {
        return ret;
    }

    return 0;
}

static int epon_oam_llidInfo_clear(
    unsigned char llidIdx)
{
    int ret;
    rtk_epon_llid_entry_t llidEntry;
    
    llidEntry.llidIdx = llidIdx;
    ret = rtk_epon_llid_entry_get(&llidEntry);
    if(RT_ERR_OK != ret)
    {
        return ret;
    }

    llidEntry.valid = DISABLED;
    llidEntry.llid = 0x7fff;
    ret = rtk_epon_llid_entry_set(&llidEntry);
    if(RT_ERR_OK != ret)
    {
        return ret;
    }

    return 0;
}

int epon_oam_sendmsg(int msgqid, char* buf,int  buflen,long mtype)
{
	msgbuf_t msg;
	int len = (buflen > sizeof(msg.buf)) ? sizeof(msg.buf) : buflen;
	int ret;

	msg.mtype = mtype;
	memcpy(msg.buf, buf, len);
	msg.buf[len-1]='\0';
	ret=msgsnd(msgqid, (void*)&msg, sizeof(msg.buf),IPC_NOWAIT);
	if(-1 == ret)
    {
        printf("[OAM:%s:%d] msgsnd failed %d\n", __FILE__, __LINE__, errno);
        return EPON_OAM_ERR_MSGQ;
    }
	return EPON_OAM_ERR_OK;
}

void epon_oam_cli_proc(
    oam_cli_t *pCli, long reqid)
{
    int ret;
    unsigned int flag;
    unsigned int value;
    unsigned short oamPduFlag;
    oam_config_t oamConfig;
    oam_oamInfo_t oamInfo;
    oam_orgSpecCb_t orgSpecCb;
    ctc_onuAuthLoid_t authLoid;

    switch(pCli->cliType)
    {
    case EPON_OAM_CLI_DBG_SET:
        epon_oam_dbgFlag_get(&flag);
        printf("EPON OAM debug flag 0x%08x -> ", flag);
        epon_oam_dbgFlag_set(pCli->u.cliDbg.flag);
        epon_oam_dbgFlag_get(&flag);
        printf("0x%08x\n", flag);
        break;
    case EPON_OAM_CLI_DBG_GET:
        epon_oam_dbgFlag_get(&flag);
        printf("EPON OAM debug flag 0x%08x\n", flag);
        break;
    case EPON_OAM_CLI_CFGOAM_SET:
        epon_oam_config_get(pCli->u.cliEnable.llidIdx, &oamConfig);
        oamConfig.oamEnabled = pCli->u.cliEnable.enable;
        epon_oam_config_set(pCli->u.cliEnable.llidIdx, &oamConfig);
        break;
    case EPON_OAM_CLI_CFGMAC_SET:
        epon_oam_config_get(pCli->u.cliMac.llidIdx, &oamConfig);
        memcpy(oamConfig.macAddr, pCli->u.cliMac.mac, 6);
        epon_oam_config_set(pCli->u.cliMac.llidIdx, &oamConfig);
        break;
    case EPON_OAM_CLI_CFGAUTOREG_SET:
        epon_oam_config_get(pCli->u.cliAutoReg.llidIdx, &oamConfig);
        oamConfig.autoRegEnable = pCli->u.cliAutoReg.autoRegEnable;
        oamConfig.autoRegTime = pCli->u.cliAutoReg.autoRegTime;
        epon_oam_config_set(pCli->u.cliAutoReg.llidIdx, &oamConfig);
        break;
    case EPON_OAM_CLI_CFGHOLDOVER_SET:
        epon_oam_config_get(pCli->u.cliHoldover.llidIdx, &oamConfig);
        oamConfig.holdoverEnable = pCli->u.cliHoldover.holdoverEnable;
        oamConfig.holdoverTime = pCli->u.cliHoldover.holdoverTime;
        epon_oam_config_set(pCli->u.cliHoldover.llidIdx, &oamConfig);
        break;
    case EPON_OAM_CLI_CFGEVENT_SET:
        epon_oam_config_get(pCli->u.cliEvent.llidIdx, &oamConfig);
        oamConfig.eventRepCnt = pCli->u.cliEvent.eventRepCnt;
        oamConfig.eventRepIntvl = pCli->u.cliEvent.eventRepIntvl;
        epon_oam_config_set(pCli->u.cliEvent.llidIdx, &oamConfig);
        break;
	case EPON_OAM_CLI_CFGKEEPALIVE_SET:
		epon_oam_config_get(pCli->u.cliKeepalive.llidIdx, &oamConfig);
        oamConfig.keepaliveTime = pCli->u.cliKeepalive.keepaliveTime;
        epon_oam_config_set(pCli->u.cliKeepalive.llidIdx, &oamConfig);
		break;
    case EPON_OAM_CLI_CFG_GET:
        epon_oam_config_get(pCli->u.cliLlidIdx.llidIdx, &oamConfig);
        printf("OAM configs for LLID %u\n", pCli->u.cliLlidIdx.llidIdx);
        printf("OAM state: %s\n", oamConfig.oamEnabled ? "enable" : "disable");
        printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
            oamConfig.macAddr[0],oamConfig.macAddr[1],oamConfig.macAddr[2],
            oamConfig.macAddr[3],oamConfig.macAddr[4],oamConfig.macAddr[5]);
        printf("autoReg enable: %s\n", oamConfig.autoRegEnable ? "enable" : "disable");
        printf("autoReg time: %u ms\n", oamConfig.autoRegTime);
        printf("holdover enable: %s\n", oamConfig.holdoverEnable ? "enable" : "disable");
        printf("holdover time: %u ms\n", oamConfig.holdoverTime);
        printf("event repeat count: %u\n", oamConfig.eventRepCnt);
        printf("event repeat interval: %u\n", oamConfig.eventRepIntvl);
		printf("keepalive time: %u ms\n", oamConfig.keepaliveTime);
        break;
    case EPON_OAM_CLI_COUNTER_CLEAR:
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_TX);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_RX);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_TXINFO);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_RXINFO);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_TXORGSPEC);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_RXORGSPEC);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_DROP);
        epon_oam_counter_init(pCli->u.cliEvent.llidIdx, EPON_OAM_COUNTERTYPE_LOSTLINK);
        break;
    case EPON_OAM_CLI_CALLBACK_SET:
        ret = epon_oam_orgSpecCb_set(EPON_OAM_CBTYPE_ORGSPEC, pCli->u.cliCallback.oui, pCli->u.cliCallback.state);
        if(ret != EPON_OAM_ERR_OK)
        {
            printf("No callback for oui %02x%02x%02x\n", pCli->u.cliCallback.oui[0], pCli->u.cliCallback.oui[1], pCli->u.cliCallback.oui[2]);
        }
        break;
    case EPON_OAM_CLI_CALLBACK_GET:
        ret = epon_oam_orgSpecCb_get(EPON_OAM_CBTYPE_ORGSPEC, pCli->u.cliCallback.oui, &orgSpecCb);
        if(ret == EPON_OAM_ERR_OK)
        {
            printf("oui: %02x%02x%02x\n", pCli->u.cliCallback.oui[0], pCli->u.cliCallback.oui[1], pCli->u.cliCallback.oui[2]);
            printf("state: %s\n", (orgSpecCb.state) ? "enable" : "disable");
            printf("callback: 0x%08x\n", orgSpecCb.processor);
        }
        else
        {
            printf("No callback for oui %02x%02x%02x\n", pCli->u.cliCallback.oui[0], pCli->u.cliCallback.oui[1], pCli->u.cliCallback.oui[2]);
        }
        break;
    case EPON_OAM_CLI_COUNTER_GET:
        printf("OAM counters for LLID %u\n", pCli->u.cliLlidIdx.llidIdx);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_TX);
        printf("Tx: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_RX);
        printf("Rx: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_TXINFO);
        printf("Tx Info: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_RXINFO);
        printf("Rx Info: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_TXORGSPEC);
        printf("Tx Orgnization Specific: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_RXORGSPEC);
        printf("Rx Orgnization Specific: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_DROP);
        printf("Drop: %u\n", value);
        value = epon_oam_counter_get(pCli->u.cliLlidIdx.llidIdx, EPON_OAM_COUNTERTYPE_LOSTLINK);
        printf("Lost Link: %u\n", value);
        break;
    case EPON_OAM_CLI_DBGEXT_SET:
        epon_oam_dbgExt_get(&flag);
        printf("EPON OAM extended flag 0x%08x -> ", flag);
        epon_oam_dbgExt_set(pCli->u.cliDbg.flag);
        epon_oam_dbgExt_get(&flag);
        printf("0x%08x\n", flag);
        break;
    case EPON_OAM_CLI_DBGEXT_GET:
        epon_oam_dbgExt_get(&flag);
        printf("EPON OAM extended flag 0x%08x\n", flag);
        break;
    case EPON_OAM_CLI_FAILOVER_SET:
        failoverTime = pCli->u.cliFailover.granularity;
        failoverTimeSpec.tv_sec = failoverTime / 1000;
        failoverTimeSpec.tv_nsec = (failoverTime % 1000) * 1000 * 1000;
        backoffTime = pCli->u.cliFailover.backoff;
        backoffTimeSpec.tv_sec = backoffTime / 1000;
        backoffTimeSpec.tv_nsec = (backoffTime % 1000) * 1000 * 1000;
        break;
    case EPON_OAM_CLI_FAILOVER_GET:
        printf("EPON OAM failover parameters\n");
        printf("Granularity: %u ms\n", failoverTime);
        printf("backoff: %u ms\n", backoffTime);
        break;
    case EPON_OAM_CLI_OAMINFOOUI_SET:
        epon_oam_localInfo_get(pCli->u.cliMac.llidIdx, &oamInfo);
        memcpy(oamInfo.oui, pCli->u.cliMac.mac, 3);
        epon_oam_localInfo_set(pCli->u.cliMac.llidIdx, &oamInfo);
        break;
	case EPON_OAM_CLI_OAMINFO_VENDORID_SET:
        memcpy(ctcOnuSn.vendorID, pCli->u.cliMac.mac, 4);
        break;
    case EPON_OAM_CLI_OAMINFO_GET:
		{
			unsigned char buf[17];
			ctc_onuAuthLoid_t loidAuth;
			
	        epon_oam_localInfo_get(pCli->u.cliMac.llidIdx, &oamInfo);
	        printf("OAM Info for LLID %u\n", pCli->u.cliLlidIdx.llidIdx);
	        printf("OUI: %02x:%02x:%02x\n",
	            oamInfo.oui[0],oamInfo.oui[1],oamInfo.oui[2]);
			printf("CTC vendorID: %c%c%c%c\n",
	            ctcOnuSn.vendorID[0],ctcOnuSn.vendorID[1],
	            ctcOnuSn.vendorID[2],ctcOnuSn.vendorID[3]);	

			memset(buf, 0, 17);
			memcpy(buf, ctcOnuSn.hwVersion, sizeof(ctcOnuSn.hwVersion));
	        printf("CTC hwVersion: %s\n", buf);
			
			memset(buf, 0, 17);
			memcpy(buf, ctcOnuSn.swVersion, sizeof(ctcOnuSn.swVersion));
	        printf("CTC swVersion: %s\n", buf);
	        printf("CTC version: %x\n", currCtcVer[0].version);

			ctc_oam_onuAuthLoid_get(pCli->u.cliMac.llidIdx, &loidAuth);
			printf("ONU LOID: %s\n", loidAuth.loid);
			printf("ONU password: %s\n", loidAuth.password);
    	}
        break;
    case EPON_OAM_CLI_OAMSTATE_GET:
        oamPduFlag = epon_oam_oamPduFlag_get(pCli->u.cliLlidIdx.llidIdx);
        printf("OAMPDU Satisfication\n");
        printf("       Eval Stable\n");
        printf(" Local %4u %6u\n",
            (oamPduFlag & EPON_OAM_FLAG_LOCAL_EVAL) ? 1 : 0,
            (oamPduFlag & EPON_OAM_FLAG_LOCAL_STABLE) ? 1 : 0);
        printf("Remote %4u %6u\n",
            (oamPduFlag & EPON_OAM_FLAG_REMOTE_EVAL) ? 1 : 0,
            (oamPduFlag & EPON_OAM_FLAG_REMOTE_STABLE) ? 1 : 0);
        break;
    case EPON_OAM_CLI_REG_TRIGGER:
		/* need to reset oam status before triggerring mpcp register 
		   FIXME: only support llidIdx 0 to register now */	
		if(pCli->u.cliLlidIdx.llidIdx == 0)
		{
			epon_oam_llid_dereg(pCli->u.cliLlidIdx.llidIdx);
			/*siyuan 2016-08-15: only reset auth state when trigger oam register */
			ctc_oam_auth_state_init(pCli->u.cliLlidIdx.llidIdx);
        	epon_oam_triggerMpcp_reg(pCli->u.cliLlidIdx.llidIdx);
		}
        break;
    case CTC_OAM_CLI_LOID_GET:
    case CTC_OAM_CLI_LOID_SET:
    case CTC_OAM_CLI_VAR_GET:
    case CTC_OAM_CLI_ALARM_GET:
	case CTC_OAM_CLI_LOOPDETECT_SET:
	case CTC_OAM_CLI_LOOPDETECT_GET:
	case CTC_OAM_CLI_VLANSTATUS_SET:
	case CTC_OAM_CLI_VLAN_GET:
	case CTC_OAM_CLI_VLAN_SET:
        ctc_oam_cli_proc(pCli);
        break;
	case CTC_OAM_CLI_LANPORTCHANGE_GET:
		{
			extern int port_link_status_change_times_get(rtk_port_t portindex);
			int i=0;
			int linkchange;
			printf("lan port status changes:\n");
			printf("index\tchangetime\n");
			for(i=0; i < LAN_PORT_NUM; i++)
			{
				int port=PortLogic2PhyID(i);
				if((linkchange = port_link_status_change_times_get(port))!=RT_ERR_FAILED)
					printf("%d\t%d\n",port,linkchange);
			}
		}
		break;
	case CTC_OAM_CLI_SILENT_SET:
		epon_ctc_silent_mode_set(pCli->u.cliEnable.enable);
		break;
	case CTC_OAM_CLI_SILENT_GET:
		printf("ctc silent mode %s\n", (enableSilentMode == 1)?"Enable":"Disable");
		printf("in silent mode pon led %s\n", (silentLedMode == 1)?"blink":"down");
		break;
	case CTC_OAM_CLI_SILENTPONLEDMODE_SET:
		epon_ctc_silent_pon_led_mode_set(pCli->u.cliEnable.enable);
		break;
	case CTC_OAM_CLI_AUTHSUCCTIME_GET:
		{
			unsigned char printbuf[128];
			long authSuccTime;

			ctc_oam_onuAuthSuccTime_get(pCli->u.cliLlidIdx.llidIdx, &authSuccTime);
			snprintf(printbuf, sizeof(printbuf),"authSuccTime: %ld\n", authSuccTime);
			epon_oam_sendmsg(msgQId, printbuf, sizeof(printbuf), reqid);
		}
		break;
    case CTC_OAM_CLI_AUTH_GET:
        {
            unsigned char status, failType;
			unsigned char printbuf[128];
			unsigned char authType;
			unsigned char buf[16];
			ctc_oam_onuAuthType_get(pCli->u.cliLlidIdx.llidIdx, &authType);
			if((authType == CTC_OAM_ONUAUTH_AUTH_MAC) || (authType == CTC_OAM_ONUAUTH_AUTH_IGNORE))
				snprintf(buf, sizeof(buf),"MAC auth");
			else
				snprintf(buf, sizeof(buf),"LOID auth");
				
            ctc_oam_onuAuthState_get(pCli->u.cliLlidIdx.llidIdx, &status, &failType);
            if (CTC_OAM_ONUAUTH_STATE_SUCC == status)
				snprintf(printbuf, sizeof(printbuf),"%s ctc auth mode: success\n", buf);
                //printf("ctc auto mode: success\n");
            else if (CTC_OAM_ONUAUTH_STATE_NOTCOMPLETE== status)
				snprintf(printbuf, sizeof(printbuf),"%s ctc auth mode: not complete\n", buf);
                //printf("ctc auto mode: not complete\n");
            else
            {
				snprintf(printbuf, sizeof(printbuf),"%s ctc auth mode: fail\nFail Type[%d]\n", buf, failType);
            }
                //printf("ctc auto mode: fail\n");
            epon_oam_sendmsg(msgQId, printbuf, sizeof(printbuf), reqid);
        }
        break;
	case CTC_OAM_CLI_REGISTERNUM_GET:
		epon_oam_register_number_show(pCli->u.cliRegisterNumToFile.llidIdx, pCli->u.cliRegisterNumToFile.registerNumToFile);
		break;
	case CTC_OAM_CLI_REGISTERNUM_RESET:
		epon_oam_register_number_reset(pCli->u.cliRegisterNumToFile.llidIdx);
		break;
	case EPON_OAM_IGMP_GET:
		mcast_fastpath_read();
		break;
	case EPON_OAM_IGMP_SET:
		igmp_cli_set(pCli);
		break;
	case EPON_OAM_PORT_MAP_SET:
		set_port_remapping_table(pCli->u.cliPortMap.portMap, pCli->u.cliPortMap.portNum);
		break;
	case EPON_OAM_PORT_MAP_GET:
		show_port_mapping_table();
		break;
	case EPON_OAM_CLI_UPTIME_GET:
		epon_oam_uptime_show(pCli->u.cliUptimeToFile.llidIdx, pCli->u.cliUptimeToFile.uptimeToFile);
		break;
    default:
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_WARN,
                "[OAM:%s:%d] unsupported CLI type %u\n", __FILE__, __LINE__, pCli->cliType);
        break;
    }
}

static void epon_oam_llid_dereg(unsigned char llidIdx)
{
	oam_oamInfo_t oamInfo;
	int ret;
	
	/* Clear HW info */
	epon_oam_llidInfo_clear(llidIdx);
	/* Reset all the runtime database of the specified LLID index */
	epon_oam_discoveryLlid_init(llidIdx);
	epon_oam_oamDb_init(llidIdx);
	epon_oam_userDb_init(llidIdx);
	/* Clear remote OAM info */
	epon_oam_remoteInfo_init(llidIdx);
	/* Clear local OAM info revision */
	epon_oam_localInfo_get(llidIdx, &oamInfo);
	oamInfo.revision = 0;
	epon_oam_localInfo_set(llidIdx, &oamInfo);
	epon_oam_counter_inc(llidIdx, EPON_OAM_COUNTERTYPE_LOSTLINK);
	/* Inform LED module that OAM disconnected */
	rtk_pon_led_status_set(PON_LED_PON_MODE_EPON, PON_LED_STATE_EPONOAM_DOWN);

	/* 9602c support 8 llid entries, so must no set MULTIPLEXER of pon port to discard when deregister one llid entry */
	if(FALSE == is9602C())
	{
		/* Set multiplexer to discard */
		if((ret = rtk_oam_multiplexerAction_set(HAL_GET_PON_PORT(),OAM_MULTIPLEXER_ACTION_DISCARD)) != RT_ERR_OK)
		{
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
				"[OAM:%s:%d] OAM multiplexerAction set failed %d\n", __FILE__, __LINE__, ret);
		}
	}
}

void *epon_oam_stateKeeper(void *argu)
{
    int ret;
    int permits;
    static unsigned short pushedLen = 0;
    unsigned char llidIdx;
    unsigned char dyingGaspBuf[EPON_OAM_DYINGGASP_BUF_LEN];
    key_t msgQKey = 1568;
    oam_msgqEventData_t eventData;
    struct itimerspec its;
    oam_oamInfo_t oamInfo;
	oam_config_t llidConf;

    /* Create message queue for receiving the event from others */
    /* S_IRUSR | S_IWUSR | State keeper can read/write message
     * S_IRGRP | S_IWGRP | All others can read/write message
     * S_IROTH | S_IWOTH   All others can read/write message
     */
    permits = 00666;
    permits |= IPC_CREAT;
    msgQId = msgget(msgQKey, permits);
    if(-1 == msgQId)
    {
        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
            "[OAM:%s:%d] msgq create failed %d\n", __FILE__, __LINE__, errno);
        return NULL;
    }

    /* Init all timers used by state keeper */
    epon_stateKeeper_timer_init();
	epon_holdover_timer_init();
	epon_download_ack_resend_timer_init();
	epon_tx_power_enable_timer_init();
	
	while(1)
	{
		ret = msgrcv(msgQId, (void *) &eventData, sizeof(eventData.msgqData), 0, 0); /* Blocking call */
        if(-1 == ret)
        {
            if(EINTR == errno)
            {
                /* A signal is caught, just continue */
            }
            else
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] msgq recv failed %d\n", __FILE__, __LINE__, errno);
            }
            continue;
        }

        llidIdx = eventData.msgqData.llidIdx;
        if(llidIdx >= EPON_OAM_SUPPORT_LLID_NUM)
        {
            /* Invalid llidIdx, just ignore it */
            continue;
        }

        EPON_OAM_PRINT(EPON_OAM_DBGFLAG_INFO, 
            "LLIDIdx %u EventType %u\n", llidIdx, eventData.mtype);
		
        switch(eventData.mtype)
        {
        case EPON_OAM_EVENT_DISCOVERY_COMPLETE:
            /* For the first registration, put a dying gasp packet into queue 127 */
        #if 0
            if(0 == pushedLen)
        #endif
            {
                epon_oam_dyingGasp_gen(llidIdx, dyingGaspBuf, EPON_OAM_DYINGGASP_BUF_LEN, &pushedLen);
                epon_oam_dyingGasp_send(llidIdx, dyingGaspBuf, pushedLen);
            }
            /* Start keepalive tiemr */
			epon_oam_config_get(llidIdx, &llidConf);
            EPON_TIMER_SET(skKeepalive[llidIdx], llidConf.keepaliveTime, ret);
            /* Inform LED module that OAM discovery complete */
            rtk_pon_led_status_set(PON_LED_PON_MODE_EPON, PON_LED_STATE_EPONOAM_UP);
            if(0 != ret)
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
            }
            /* Set multiplexer to forward */
            if((ret = rtk_oam_multiplexerAction_set(HAL_GET_PON_PORT(),OAM_MULTIPLEXER_ACTION_FORWARD)) != RT_ERR_OK)
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] OAM multiplexerAction set failed %d\n", __FILE__, __LINE__, ret);
            }
			
		#ifdef SUPPORT_OAM_EVENT_TO_KERNEL
			epon_oam_state_event_send(llidIdx, RTK_EPON_STATE_OAM_COMPLETE);
		#endif
			ctc_oam_onuAuthType_set(llidIdx, CTC_OAM_ONUAUTH_AUTH_MAC); /* set auth type to mac auth */
			/* setting to mac auth means redo the register process, reset pon led state */
			rtk_pon_led_status_set(PON_LED_PON_MODE_EPON, PON_LED_STATE_AUTH_NG);
			
			if(epon_link_up[llidIdx] == 0)
			{
				epon_link_up[llidIdx] = 1; /* set epon link up if not receive any los recover message */
				gettimeofday(&epon_uptime_start, NULL);

				/* must after ctc_oam_onuAuthType_set to avoid latency */
				dev_link_change_notify(1); /* set wan device up if not receive any los recover message */
			}
            break;
		case EPON_OAM_EVENT_LOS_RECOVER:
			epon_oam_config_get(llidIdx, &llidConf);
			if (llidConf.holdoverEnable)
			{
				/* stop holdover timer */
				EPON_TIMER_SET(skHoldover[llidIdx], 0, ret);
				if(0 != ret)
				{
					EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
						"[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
				}
			}
			else
			{
				/* Clear HW info */
				epon_oam_llidInfo_clear(llidIdx);
			}
		#ifdef SUPPORT_OAM_EVENT_TO_KERNEL
			/* send event to kernel when pon port is link up */
			epon_oam_state_event_send(llidIdx, RTK_EPON_STATE_LINKUP);
		#endif
            /* wan port link up */
            dev_link_change_notify(1);

			/* save epon link up time */
			gettimeofday(&epon_uptime_start, NULL);
			epon_link_up[llidIdx] = 1;
			break;
		case EPON_OAM_EVENT_LOS:
			epon_oam_config_get(llidIdx, &llidConf);
			if (llidConf.holdoverEnable)
				EPON_TIMER_SET(skHoldover[llidIdx], llidConf.holdoverTime, ret);
			else {
				/* stop keepalive timer */
				EPON_TIMER_SET(skKeepalive[llidIdx], 0, ret);
				if(0 != ret)
				{
					EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
						"[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
				}
				epon_oam_llid_dereg(llidIdx);
			#ifdef SUPPORT_OAM_EVENT_TO_KERNEL
				/* send event to kernel when pon port is link down */
				epon_oam_state_event_send(llidIdx, RTK_EPON_STATE_LOS);
			#endif
				/* wan port link down */
                dev_link_change_notify(0);
		
				epon_link_up[llidIdx] = 0;
				ctc_oam_onuAuthState_set(llidIdx, CTC_OAM_ONUAUTH_STATE_NOTCOMPLETE, 0);
			}
			break;
        case EPON_OAM_EVENT_KEEPALIVE_TIMEOUT:
		case EPON_OAM_EVENT_HOLDOVER_TIMEOUT:
            /* Stop timer */
            EPON_TIMER_SET(skKeepalive[llidIdx], 0, ret);
            if(0 != ret)
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
            }
			EPON_TIMER_SET(skHoldover[llidIdx], 0, ret);
            if(0 != ret)
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
            }

			epon_oam_llid_dereg(llidIdx);
            /* wan port link down */
            if (EPON_OAM_EVENT_HOLDOVER_TIMEOUT == eventData.mtype)
            {
            #ifdef SUPPORT_OAM_EVENT_TO_KERNEL
				/* send event to kernel when pon port is link down */
				epon_oam_state_event_send(llidIdx, RTK_EPON_STATE_LOS);
			#endif
                dev_link_change_notify(0);

				epon_link_up[llidIdx] = 0;
				ctc_oam_onuAuthState_set(llidIdx, CTC_OAM_ONUAUTH_STATE_NOTCOMPLETE, 0);
            }
            break;
        case EPON_OAM_EVENT_KEEPALIVE_RESET:
            /* Valid OAMPDU received, reest the keepalive timer */
			epon_oam_config_get(llidIdx, &llidConf);
            EPON_TIMER_SET(skKeepalive[llidIdx], llidConf.keepaliveTime, ret);
            if(0 != ret)
            {
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
            }
            break;
        case EPON_OAM_EVENT_CLI:
            if(eventData.msgqData.dataSize)
            {
                epon_oam_cli_proc((oam_cli_t *) eventData.msgqData.data, eventData.msgqData.reqid);
            }
            break;
		case EPON_OAM_EVENT_DOWNLOAD_ACK_RESEND_TIMER:
			if(eventData.msgqData.dataSize)
            {
                //start download ack resend timer
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                    "[OAM:%s:%d] start download ack resend timer\n",
                    __FILE__, __LINE__);
                EPON_TIMER_SET(downloadAckResend[llidIdx], EPON_OAM_DOWNLOADACKRESEND_TIME, ret);
				if(0 != ret)
	            {
	                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
	                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
	            }
				memcpy(downloadAckBuff, eventData.msgqData.data, eventData.msgqData.dataSize);
				downloadAckSize = eventData.msgqData.dataSize;
            }
			break;
		case EPON_OAM_EVENT_DOWNLOAD_ACK_RESEND_CLEAR:
			//clear download ack resend timer
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                    "[OAM:%s:%d] clear download ack resend timer\n",
                    __FILE__, __LINE__);
			EPON_TIMER_SET(downloadAckResend[llidIdx], 0, ret);
			if(0 != ret)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
					"[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
			}
			downloadAckSize = 0;
			break;
		case EPON_OAM_EVENT_TX_POWER_ENABLE_TIMER:
			if(eventData.msgqData.dataSize)
            {
            	unsigned short * pTime;
				unsigned int msTime;
				pTime =  (unsigned short *)eventData.msgqData.data;
				msTime = (*pTime) * 1000;
				
				//start re-enable tx power timer 
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                    "[OAM:%s:%d] re-enable tx power after 0x%x seconds\n",
                    __FILE__, __LINE__, *pTime);
				EPON_TIMER_SET(txpowerEnable, msTime, ret);
				if(0 != ret)
	            {
	                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
	                    "[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
	            }
			}
			break;
		case EPON_OAM_EVENT_TX_POWER_ENABLE_CLEAR:
			//clear re-enable tx power timer
			EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO,
                    "[OAM:%s:%d] clear re-enable tx power timer\n",
                    __FILE__, __LINE__);
			EPON_TIMER_SET(txpowerEnable, 0, ret);
			if(0 != ret)
			{
				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
					"[OAM:%s:%d] timer set failed %d\n", __FILE__, __LINE__, errno);
			}
			break;
		case EPON_OAM_EVENT_LOOP_DETECT_ENABLE:
			{
				extern void* ctc_loopdetect(void* argu);
				pthread_t th_loop_detect;

				EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, "[OAM:%s:%d] loopdetect enable\n", __FILE__, __LINE__);
				pthread_create(&th_loop_detect, NULL, &ctc_loopdetect, NULL);
				pthread_detach(th_loop_detect);
			}
			break;
        default:
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_WARN,
                "[OAM:%s:%d] Unsupported event %d\n", __FILE__, __LINE__, eventData.mtype);
            break;
        }
	}

	return NULL;
}

/*when receive a NACK mpcp frame, onu should enter silent mode. 
    In silent mode, onu doesn't send mpcp register frame during a period of time (60s by default). */
void epon_ctc_open_silent_mode()
{
	if(isOnuSilentMode == 0)
	{
		EPON_OAM_PRINT(EPON_OAM_DBGFLAG_CTC_INFO, 
                       "ctc open silent mode\n");
		isOnuSilentMode = 1;
	}

    if (enableSilentMode)
    {
    	if(silentLedMode)
    	{
    		rtk_pon_led_status_set(PON_LED_PON_MODE_EPON, PON_LED_STATE_EPONOAM_TRYING);
    	}
		else
		{
        	rtk_pon_led_status_set(PON_LED_PON_MODE_EPON, PON_LED_STATE_EPONMAC_DOWN);
		}
    }
}

void epon_ctc_silent_mode_set(int enable)
{
	enableSilentMode = enable;
	isOnuSilentMode = 0;
}

/* In silent mode silentLedMode value (0: pon led down, 1: pon led blink) */
void epon_ctc_silent_pon_led_mode_set(int ledMode)
{
	silentLedMode = ledMode;
}

void *epon_oam_failRecover(void *argu)
{
    int ret;
    unsigned char isIdle, oam_discovery_state;
    rtk_enable_t state;
    unsigned short autoRegTime[EPON_OAM_SUPPORT_LLID_NUM] = { 0 };
    oam_config_t llidConf;
    struct timeval now, old;
    struct timespec sleepTime, remainTime;

    /* Init variables */
    failoverTime = EPON_OAM_FAILOVER_GRANULARITY;
    failoverTimeSpec.tv_sec = EPON_OAM_FAILOVER_GRANULARITY / 1000;
    failoverTimeSpec.tv_nsec = (EPON_OAM_FAILOVER_GRANULARITY % 1000) * 1000 * 1000;
    backoffTime = EPON_OAM_FAILOVER_BACKOFF;
    backoffTimeSpec.tv_sec = EPON_OAM_FAILOVER_BACKOFF / 1000;
    backoffTimeSpec.tv_nsec = (EPON_OAM_FAILOVER_BACKOFF % 1000) * 1000 * 1000;
	silentTime = CTC_OAM_SILENT_MODE_TIME;
    gettimeofday(&old, NULL);
    
	while(1)
	{
        sleepTime.tv_sec = failoverTimeSpec.tv_sec;
        sleepTime.tv_nsec = failoverTimeSpec.tv_nsec;
FAIL_RECOVER_WAKE_UP1:
        ret = nanosleep(&sleepTime, &remainTime);
        if((ret == -1) && (EINTR == errno))
        {
            /* Interrupted by signal 
             * Uset the remain time to sleep again
             */
            sleepTime = remainTime;
            goto FAIL_RECOVER_WAKE_UP1;
        }
        else if(ret != 0)
        {
            EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                "[OAM:%s:%d] nanosleep failed %d\n", __FILE__, __LINE__, ret);
        }

        /* Auto registration only support on primary LLID */
        epon_oam_config_get(0, &llidConf);
        epon_oam_discoveryIdleLlid_get(0, &isIdle, &oam_discovery_state);		
        if((!llidConf.autoRegEnable) ||
           (!llidConf.oamEnabled) ||
           (!isIdle))
        {
            /* Check software config first */
            continue;
        }

        /* Check if the physical fiber is connected */
        rtk_epon_losState_get(&state);
        if(DISABLED == state)
        {
            if (0 == autoRegTime[0])
                gettimeofday(&old, NULL);
            
            if(autoRegTime[0] < llidConf.autoRegTime)
            {
                autoRegTime[0] += failoverTime;
            }
			else if(enableSilentMode && isOnuSilentMode)
			{
			    gettimeofday(&now, NULL);

                if (((now.tv_sec*1000+now.tv_usec/1000) - (old.tv_sec*1000+old.tv_usec/1000)) >= silentTime)
                {
                    isOnuSilentMode = 0;
                    goto TRIGGER_REG;
                }
                
				if(autoRegTime[0] >= silentTime)
				{
					isOnuSilentMode = 0;
				}
				autoRegTime[0] += failoverTime;
			}
            else
            {
TRIGGER_REG:
                /* Time up, trigger register */
                epon_oam_triggerMpcp_reg(0);
                autoRegTime[0] = 0;

                /* Backoff before next retry */
                sleepTime.tv_sec = backoffTimeSpec.tv_sec;
                sleepTime.tv_nsec = backoffTimeSpec.tv_nsec;
                EPON_OAM_PRINT(EPON_OAM_DBGFLAG_INFO,
                    "[OAM:%s:%d] Trigger register\n", __FILE__, __LINE__);
FAIL_RECOVER_WAKE_UP2:
                ret = nanosleep(&sleepTime, &remainTime);
                if((ret == -1) && (EINTR == errno))
                {
                    /* Interrupted by signal 
                     * Uset the remain time to sleep again
                     */
                    sleepTime = remainTime;
                    goto FAIL_RECOVER_WAKE_UP2;
                }
                else if(ret != 0)
                {
                    EPON_OAM_PRINT(EPON_OAM_DBGFLAG_ERROR,
                        "[OAM:%s:%d] nanosleep failed %d\n", __FILE__, __LINE__, ret);
                }
            }
        }
	}

	return NULL;
}

static char eponoamd_pidfile[] = "/var/run/eponoamd.pid";
static void log_pid()
{
	FILE *f;
	pid_t pid;
	char *pidfile = eponoamd_pidfile;

	pid = getpid();
	if((f = fopen(pidfile, "w")) == NULL)
		return;
	fprintf(f, "%d\n", pid);
	fclose(f);
}

#ifdef CONFIG_SFU_APP
#define CPU_PORT_EGRESS_RATE 1024 /* In unit of kbit. Firmware update is slow, so change rate from 128 to 1024 */
static void limit_unknown_flood()
{
	/* set cpu port egress rate to limit unkown packets flood to cpu port */
	rtk_rate_portEgrBandwidthCtrlRate_set(HAL_GET_CPU_PORT(),CPU_PORT_EGRESS_RATE);

	if(TRUE == is9601B())
	{
		/* cpu port use table 3, let oam packet map to queue 7 */
		rtk_qos_portPriMap_set(HAL_GET_CPU_PORT(), 3);

		/* set remapping-priority of forward-to-cpu packets, the setting doesn't affect oam packet priority */
		rtk_qos_fwd2CpuPriRemap_set(0,0);
		rtk_qos_fwd2CpuPriRemap_set(1,0);
		rtk_qos_fwd2CpuPriRemap_set(2,1);
		rtk_qos_fwd2CpuPriRemap_set(3,1);
		rtk_qos_fwd2CpuPriRemap_set(4,2);
		rtk_qos_fwd2CpuPriRemap_set(5,2);
		rtk_qos_fwd2CpuPriRemap_set(6,3);
		rtk_qos_fwd2CpuPriRemap_set(7,3);
	}
	else
	{
		/* cpu port use table 2, let oam packet map to a high priority queue */
		rtk_qos_portPriMap_set(HAL_GET_CPU_PORT(), 2);
	}
}

/* if downstream multicast data from pon port not found in l2 table, we should drop it */
static void drop_lookup_miss_multicast()
{
	int32 ret;
	rtk_l2_lookupMissType_t type;
    rtk_action_t action;

	/* ipv4-mcast */
	type = DLF_TYPE_IPMC;
	action = ACTION_DROP;

	ret = rtk_l2_portLookupMissAction_set(HAL_GET_PON_PORT(), type, action);
	if(ret != RT_ERR_OK)
	{
		printf("rtk_l2_portLookupMissAction_set failed ret[%d]\n", ret);
	}

	/* multicast */
	type = DLF_TYPE_MCAST;
	action = ACTION_DROP;
	ret = rtk_l2_portLookupMissAction_set(HAL_GET_PON_PORT(), type, action);
	if(ret != RT_ERR_OK)
	{
		printf("rtk_l2_portLookupMissAction_set failed ret[%d]\n", ret);
	}

	if(TRUE == is9602C() || TRUE == is9607C())
	{
		/* 9602c and 9607c have ipv6-mcast-mode */	
		/* ipv6 multicast */
		type = DLF_TYPE_IP6MC;
		action = ACTION_DROP;
		ret = rtk_l2_portLookupMissAction_set(HAL_GET_PON_PORT(), type, action);
		if(ret != RT_ERR_OK)
		{
			printf("rtk_l2_portLookupMissAction_set failed ret[%d]\n", ret);
		}
	}
}

static void remove_cpu_port_from_lookup_miss_flood_pmask(void)
{
	rtk_l2_lookupMissType_t type;
    rtk_portmask_t pmask;
    int32 ret;

    if(TRUE != is9601B())
        return;
    
    /* ipv4-mcast */
    type = DLF_TYPE_IPMC;
    rtk_l2_lookupMissFloodPortMask_get(type, &pmask);
    RTK_PORTMASK_PORT_CLEAR(pmask, HAL_GET_CPU_PORT());
    rtk_l2_lookupMissFloodPortMask_set(type, &pmask);

    /* unicast*/
    type = DLF_TYPE_UCAST;
    rtk_l2_lookupMissFloodPortMask_get(type, &pmask);
    RTK_PORTMASK_PORT_CLEAR(pmask, HAL_GET_CPU_PORT());
    rtk_l2_lookupMissFloodPortMask_set(type, &pmask);

    /* multicast */
    type = DLF_TYPE_MCAST;
    rtk_l2_lookupMissFloodPortMask_get(type, &pmask);
    RTK_PORTMASK_PORT_CLEAR(pmask, HAL_GET_CPU_PORT());
    rtk_l2_lookupMissFloodPortMask_set(type, &pmask);

    /* broadcast */
    type = DLF_TYPE_BCAST;
    rtk_l2_lookupMissFloodPortMask_get(type, &pmask);
    RTK_PORTMASK_PORT_CLEAR(pmask, HAL_GET_CPU_PORT());
    rtk_l2_lookupMissFloodPortMask_set(type, &pmask);
}
#endif

/* set LOS led to work
 * LOS led will blink if pon port is link-down
 * LOS led will be down if pon port is link-up 
 */
static void set_los_led()
{
	unsigned char buf[2];
	unsigned int pon_led_spec_type = 0;
	int ret;
	
	ret = ctc_oam_flash_var_get("PON_LED_SPEC", buf, sizeof(buf));
	if(ret == EPON_OAM_ERR_OK)
	{
		pon_led_spec_type = atoi(buf);
	}
	
	ret = rtk_pon_led_SpecType_set(pon_led_spec_type);
	if(ret != RT_ERR_OK)
	{
		printf("rtk_pon_led_SpecType_set failed, ret = %d\n", ret);	
	}
}

int
main(
	int argc,
	char *argv[])
{
    int i;
    int ret;
    oam_config_t oamConfig;
    epon_inputParam_t param;
	rtk_epon_llid_entry_t llidEntry;
	pthread_t th1, th2, th3;

	log_pid();
	
    memset(&param, 0, sizeof(param));
    ret = epon_oam_paraParse(argc, argv, &param);

    rtk_epon_init();

    rtk_core_init();

    /* Disable VLAN function by default */
    rtk_vlan_vlanFunctionEnable_set(DISABLED);

    /* Disable SVLAN function by default */
    rtk_svlan_svlanFunctionEnable_set(DISABLED);
    
    /* Allow OAM packet to be trapped */
    rtk_trap_oamPduAction_set(ACTION_TRAP2CPU);

	/*init loop detect variable*/
	loopdetct_var_init();

    /* Init database */
    epon_oam_database_init();
    epon_oam_discovery_init();
    epon_oam_user_init();
    for(i = 0;i < EPON_OAM_SUPPORT_LLID_NUM;i++)
    {
        epon_oam_discoveryLlid_init(i);
        epon_oam_userDb_init(i);
        epon_oam_config_get(i, &oamConfig);
        memcpy(oamConfig.macAddr, param.mac[i], 6);
        epon_oam_config_set(i, &oamConfig);
		
		/* set register mac-address */
		llidEntry.llidIdx = i;
		rtk_epon_llid_entry_get(&llidEntry);
		memcpy(llidEntry.mac.octet, param.mac[i], 6);
		rtk_epon_llid_entry_set(&llidEntry);
		printf("set llid %d mac-addr %02x%02x%02x%02x%02x%02x\n", i, 
				param.mac[i][0], param.mac[i][1], param.mac[i][2], 
				param.mac[i][3], param.mac[i][4], param.mac[i][5]);
    }

#ifndef CONFIG_EPON_OAM_DUMMY_MODE
#ifdef CONFIG_SFU_APP
	limit_unknown_flood();
	drop_lookup_miss_multicast();

    /* Set lookup miss flood port mask exclude cpu port */
    remove_cpu_port_from_lookup_miss_flood_pmask();
#endif
	set_los_led();
#endif

	/* sleep 20 seconds before processing oam packets to fix the problem 
	   that can't register with some OLT when reboot the modem */
	sleep(20);

    pthread_create(&th1, NULL, &epon_oam_rxThread, NULL);
    pthread_create(&th2, NULL, &epon_oam_stateKeeper, NULL);
    pthread_create(&th3, NULL, &epon_oam_failRecover, NULL);

#ifndef CONFIG_EPON_OAM_DUMMY_MODE
	epon_igmp_init();
#endif

    while(1)
    {
        sleep(100);
    }

    return 0;
}

