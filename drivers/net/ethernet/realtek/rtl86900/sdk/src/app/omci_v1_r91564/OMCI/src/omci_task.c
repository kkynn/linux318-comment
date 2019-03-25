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
 * Purpose : Definition of OMCI task APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI task APIs
 */

#include "app_basic.h"
#include "omci_task.h"
#ifndef OMCI_X86
#include "rtk/switch.h"
#include "common/error.h"
#ifdef CONFIG_SDK_APOLLOMP
#include "hal/chipdef/apollomp/apollomp_def.h"
#endif
#ifdef CONFIG_SDK_RTL9601B
#include "hal/chipdef/rtl9601b/rtl9601b_def.h"
#endif
#ifdef CONFIG_SDK_RTL9603B
#include "hal/chipdef/rtl9603b/rtl9603b_def.h"
#endif
#include <rtk/init.h>
#endif
#include "feature_mgmt.h"
#include "mcast_mgmt.h"

static OMCI_APPL_INFO_T* spOmciApplInfo = NULL;

OMCI_STATUS_INFO_ts gInfo;


/*
*   Define local function
*/
static void omci_SigSegvHandle(INT32 signum, siginfo_t* info, void*ptr)
{
	char taskName[OMCI_TASK_NAME_MAX_LEN];

	prctl(PR_GET_NAME, (unsigned long)taskName);
    fprintf(stderr, "! Segmentation Fault ! tid(%d), name(%s)\n", getpid(), taskName);

    exit (-1);
}


static GOS_ERROR_CODE omci_ApplEntry(void)
{
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;
    CHAR*             pUsrData = NULL;
    OMCI_MSG_HDR_T*   pMsg;
    CHAR              msgBuff[OMCI_MAX_BYTE_PER_MSG];
    size_t            msgSize = sizeof(msgBuff) - 8;
    pMsg = (OMCI_MSG_HDR_T*)msgBuff;

    if (pAppInfo->msgHandler)
    {
        while (1)
        {
            if (GOS_OK == OMCI_RecvFromMsgQ(pAppInfo->msgQId, pMsg, msgSize, -1 * OMCI_MSG_PRI_NORMAL))
            {
                if (pMsg->len >= sizeof(OMCI_MSG_HDR_T))
                {
                    pUsrData = (CHAR*)pMsg + sizeof(OMCI_MSG_HDR_T);
                }

                (*pAppInfo->msgHandler)(pUsrData, pMsg->type, pMsg->priority, pMsg->srcMsgQKey);
            }
        }
    }

    return GOS_OK;
}


static void omci_DeinitApp(INT32 arg)
{
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;

    if (pAppInfo->deinit)
    {
        (*pAppInfo->deinit)();
    }

    omci_DeleteMsgQ(pAppInfo->msgQId);
    pAppInfo->msgQId = OMCI_MSG_ID_INVALID;

    exit(0);
}

static GOS_ERROR_CODE omci_TaskEnty(UINT32 taskIndex)
{
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;
    struct sched_param param;


    GOS_ASSERT(pAppInfo);

    while (OMCI_TASK_ID_INVALID == pAppInfo->tasks[taskIndex].taskId)
    {
        OMCI_TaskDelay(100);
    }
	prctl(PR_SET_NAME, (unsigned long)pAppInfo->tasks[taskIndex].name);

    if (pAppInfo->tasks[taskIndex].pEntryFn)
    {
		param.sched_priority = pAppInfo->tasks[taskIndex].priority;
		sched_setscheduler(0, SCHED_FIFO, &param);
        pAppInfo->tasks[taskIndex].pEntryFn(pAppInfo->tasks[taskIndex].pFnArg);
    }

    omci_DeleteMsgQ(pAppInfo->tasks[taskIndex].msgQId);
    pAppInfo->tasks[taskIndex].msgQId = OMCI_MSG_ID_INVALID;

    pAppInfo->tasks[taskIndex].taskId = OMCI_TASK_ID_INVALID;


    return GOS_OK;
}

static void omci_AppInfoApply(void)
{

	omci_wrapper_setLog(gDrvLogLevel);
	omci_wrapper_setDevMode(gInfo.devMode);
    omci_wrapper_setDualMgmtMode(gInfo.dmMode);
	omci_wrapper_setWanQueueNum(gInfo.wanQueueNum);
	omci_wrapper_activateGpon(FALSE);
	omci_wrapper_setSerialNum(gInfo.sn);
	omci_wrapper_setGponPasswd(gInfo.gponPwd);
	omci_wrapper_activateGpon(TRUE);
}

static void omci_init_device_info(void)
{
    feature_api(FEATURE_API_ME_00020000, (INT32)-1, (UINT32)0);
    omci_wrapper_setPortRemp(gInfo.portRemMap);

    memset(gInfo.devCapabilities.ethPort, 0, sizeof(omci_eth_port_t) * RTK_MAX_NUM_OF_PORTS);

#ifdef OMCI_X86
    gInfo.devIdVersion.chipId = 0;
    gInfo.devCapabilities.fePortNum = 0;
    gInfo.devCapabilities.gePortNum = 4;
    gInfo.devCapabilities.cpuPort = 2;
    gInfo.devCapabilities.rgmiiPort = -1;
    gInfo.devCapabilities.ponPort = 1;
    gInfo.devCapabilities.totalTContNum = 8;
    gInfo.devCapabilities.totalGEMPortNum = 128;
    gInfo.devCapabilities.totalTContQueueNum = 32;
    gInfo.devCapabilities.perUNIQueueNum = 8;
    gInfo.devCapabilities.perTContQueueDp = 0;
    gInfo.devCapabilities.perUNIQueueDp = 0;
#else
    gInfo.devCapabilities.fePortNum = 0;
    gInfo.devCapabilities.gePortNum = 0;
    gInfo.devCapabilities.cpuPort = -1;
    gInfo.devCapabilities.rgmiiPort = -1;
    gInfo.devCapabilities.ponPort = -1;
    gInfo.devCapabilities.totalTContNum = 0;
    gInfo.devCapabilities.totalGEMPortNum = 0;
    gInfo.devCapabilities.totalTContQueueNum = 0;
    gInfo.devCapabilities.perUNIQueueNum = 0;
    gInfo.devCapabilities.perTContQueueDp = 0;
    gInfo.devCapabilities.perUNIQueueDp = 0;

    // get dev capabilities
    omci_wrapper_getDevCapabilities(&gInfo.devCapabilities);

    memset(&gInfo.devIdVersion, 0, sizeof(omci_dev_id_version_t));

    // get dev id & version
    omci_wrapper_getDevIdVersion(&gInfo.devIdVersion);
#endif
}

static void omci_mcast_mgmt_init(void)
{
#ifndef OMCI_X86
	omci_mcast_snp_mode_t snp_mode;

	snp_mode = (gInfo.dmMode ? SNP_MODE_RG : SNP_MODE_RTK);

	gInfo.pMCwrapper = NULL;

    gInfo.pMCwrapper = omci_mcast_wrapper_find(snp_mode);
#endif
	return;
}

static void omci_voice_mgmt_init(void)
{
#ifndef OMCI_X86
	omci_voice_vendor_t voice_vendor;

	voice_vendor = (gInfo.voiceVendor ? VOICE_VENDOR_RTK : VOICE_VENDOR_NONE);

    gInfo.pVCwrapper = NULL;

    omci_voice_vendor_init(voice_vendor);

    gInfo.pVCwrapper = omci_voice_wrapper_find(voice_vendor);
#endif
	return;
}

static void omci_voice_mgmt_deinit(void)
{
    omci_voice_vendor_t voice_vendor;

	voice_vendor = (gInfo.voiceVendor ? VOICE_VENDOR_RTK : VOICE_VENDOR_NONE);

    omci_voice_vendor_deinit(voice_vendor);

    gInfo.pVCwrapper = omci_voice_wrapper_find(voice_vendor);
}

static void omci_feature_mgmt_init(void)
{
    fMgmtInit(gInfo.customize, FEATURE_API_END);
	return;
}

static void omci_feature_mgmt_deinit(void)
{
    OMCI_LOG (OMCI_LOG_LEVEL_WARN, "Deiniting the module of featureMgmt ... \n");
    fMgmtDeinit();
    return;
}

static void omci_interface_mgmt_init(void)
{
    char cmd[CMD_LEN];

    if (OMCI_DEV_MODE_BRIDGE == gInfo.devMode)
    {
#ifndef OMCI_X86
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, CMD_LEN, "echo '%d %s' > %s", gInfo.devCapabilities.ponPort, PON_IF, NIC_PROC_PATH);
        system(cmd);
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, CMD_LEN, "%s %s up", IFCONFIG_CMD, PON_IF);
        system(cmd);
#endif
    }
    return;
}

/*
*  Define Global Function for Task
*/
GOS_ERROR_CODE OMCI_Init(void)
{
	OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Start OMCI application initializing ... ");

	/*For HAL usage
	rtk_core_init();*/

    /* initial feature mgmt */
	omci_feature_mgmt_init();

    /* initial mcast mgmt */
	omci_mcast_mgmt_init();

    /* initial voice mgmt */
	omci_voice_mgmt_init();
    VOICE_WRAPPER(omci_voice_config_init);

    // create ctrl device
    omci_wrapper_createCtrlDev();

    // initial device info
    omci_init_device_info();

    // init driver db
    omci_wrapper_initDrvDb(0);

    // create msg device
    omci_wrapper_createMsgDev();


    omci_timer_setSignalMask(OMCI_TASK_MAIN_ACCEPT_SIG_MASK);

    OMCI_SpawnTask("msg handler",
                    omci_wrapper_msgHandler,
                    NULL,
                    OMCI_TASK_PRI_MAIN,
                    TRUE);

    /* initial tree connection configuration task */
    MIB_TreeCfgTaskInit();

    /* initial interface mgmt */
    omci_interface_mgmt_init();

    /* initialize MIB */
	OMCI_InitMib();

    // init driver db
    omci_wrapper_initDrvDb(1);

	/* initialize state machine for ME Multi-Actions */
	OMCI_InitStateMachines();

	/* initialize message facility */
	OMCI_ResetHistoryRspMsg();
	OMCI_ResetLastRspMsgIndex();

    // initial timer
    omci_timer_init();

    /*initial signal handler*/
    OMCI_SpawnTask("Signal handler",
                    omci_wrapper_signalHandler,
                    NULL,
                    OMCI_TASK_PRI_SIG,
                    FALSE);

    // initial PM task
    omci_pm_task_init();



    /*Initial TM task */
    gInfo.tmTimerRunningB = TRUE;
    omci_tm_task_init();

    // initial interrupt handler
    omci_wrapper_createIntrSocket();
    OMCI_SpawnTask("interrupt handler",
                    omci_wrapper_intrHandler,
                    NULL,
                    OMCI_TASK_PRI_INTR,
                    FALSE);

	/*apply initial setting*/
	omci_AppInfoApply();

	OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Finish OMCI application initializing ... \n");

	return GOS_OK;
}


GOS_ERROR_CODE OMCI_DeInit(void)
{
    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Start OMCI application deinitializing ... \n");

    omci_voice_mgmt_deinit();

    omci_wrapper_deleteIntrSocket();

    gInfo.receiveState = FALSE;

    omci_wrapper_deleteMsgDev();

    omci_wrapper_deleteCtrlDev();

    MIB_Proprietary_UnReg();

    omci_feature_mgmt_deinit();

    MIB_Deinit();

    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Finish OMCI application deinitializing ... \n");

    return GOS_OK;
}




GOS_ERROR_CODE OMCI_AppInit(OMCI_APPL_ID appId, const CHAR* appName)
{
    OMCI_APPL_INFO_T* pAppInfo;
    OMCI_TASK_INFO_T  taskInfo;
    UINT32           taskIndex;
    struct sched_param  param;
    struct sigaction action;

    memset(&action, 0, sizeof(action));
    action.sa_sigaction = omci_SigSegvHandle;
    action.sa_flags     = SA_SIGINFO||SA_STACK;

    if (sigaction(SIGSEGV, &action, NULL) < 0)
    {
        return GOS_FAIL;
    }

    param.sched_priority = OMCI_TASK_PRI_MAIN - 1;
    sched_setscheduler(getpid(), SCHED_RR, &param);

    spOmciApplInfo = (OMCI_APPL_INFO_T*)malloc(sizeof(OMCI_APPL_INFO_T));
    GOS_ASSERT(spOmciApplInfo != NULL);
    memset(spOmciApplInfo, 0x00, sizeof(OMCI_APPL_INFO_T));

    pAppInfo = spOmciApplInfo;

    for (taskIndex = 0; taskIndex < OMCI_MAX_TASKS_PER_APPL; taskIndex++)
    {
        memset(&pAppInfo->tasks[taskIndex], 0x00, sizeof(OMCI_TASK_INFO_T));
        pAppInfo->tasks[taskIndex].taskId = OMCI_TASK_ID_INVALID;
    }

    pAppInfo->applId = appId;
    strncpy(pAppInfo->name, appName, OMCI_TASK_NAME_MAX_LEN);
    pAppInfo->applPid    = getpid();
    pAppInfo->mainTaskId = syscall(SYS_gettid);

    pAppInfo->msgQId = omci_CreateMsgQ(OMCI_MSGQKEY(appId, 0));
    GOS_ASSERT(pAppInfo->msgQId != OMCI_MSG_ID_INVALID);

    // modify IPC message queue size to tolerate burst
    struct msqid_ds tMsqDs;
    if (0 == msgctl(pAppInfo->msgQId, IPC_STAT, &tMsqDs))
    {
        tMsqDs.msg_qbytes = OMCI_MAX_BYTE_PER_MSG * (UCHAR_MAX + 1);

        if (msgctl(pAppInfo->msgQId, IPC_SET, &tMsqDs))
        {
            // try to set it again with a smaller number
            tMsqDs.msg_qbytes = tMsqDs.msg_qbytes / 4;

            // if it is failed still, report error
            if (msgctl(pAppInfo->msgQId, IPC_SET, &tMsqDs))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                    "Set IPC message queue size (bytes) to %lu fail", tMsqDs.msg_qbytes);
            }
        }
    }

    memset(&taskInfo, 0x00, sizeof(OMCI_TASK_INFO_T));
    taskInfo.applId     = pAppInfo->applId;
    strncpy(taskInfo.name, appName, OMCI_TASK_NAME_MAX_LEN);
    taskInfo.pEntryFn   = NULL;
    taskInfo.taskNumber = 0;
    taskInfo.msgQId     = pAppInfo->msgQId;
    taskInfo.msgQKey    = OMCI_MSGQKEY(pAppInfo->applId, 0);
    taskInfo.taskId     = pAppInfo->mainTaskId;
    memcpy(&(pAppInfo->tasks[0]), &taskInfo, sizeof(OMCI_TASK_INFO_T));

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_AppStart(OMCI_APPL_INIT_PTR pInitFn, OMCI_APPL_MSG_HANDLER_PTR pMsgHandlerFn, OMCI_APPL_DEINIT_PTR pDeinitFn)
{
    OMCI_APPL_INFO_T* pAppInfo =  spOmciApplInfo;

    pAppInfo->init       = pInitFn;
    pAppInfo->msgHandler = pMsgHandlerFn;
    pAppInfo->deinit     = pDeinitFn;

    if (pInitFn)
    {
        (*pInitFn)();
    }

    signal(SIGTERM, omci_DeinitApp);

    omci_ApplEntry();

    omci_DeinitApp(0);

    return GOS_OK;
}





OMCI_TASK_ID OMCI_SpawnTask(const CHAR          *pTaskName,
                            OMCI_TASK_ENTRY     pEntryFn,
                            void                *pFnArg,
                            UINT32              priority,
                            BOOL                needMsgQ)
{
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;
    OMCI_TASK_INFO_T  taskInfo;
    UINT32            taskIndex;
    pthread_t         thread_a;
    struct sched_param sched;
    struct msqid_ds tMsqDs;

    GOS_ASSERT(pAppInfo);
    memset(&taskInfo, 0x00, sizeof(OMCI_TASK_INFO_T));

    taskInfo.applId    = pAppInfo->applId;
    strncpy(taskInfo.name, pTaskName, OMCI_TASK_NAME_MAX_LEN);
    taskInfo.taskId    = OMCI_TASK_ID_INVALID;
    taskInfo.pEntryFn  = pEntryFn;
    taskInfo.pFnArg    = pFnArg;
    taskInfo.priority  = priority;

    for (taskIndex = 0; taskIndex < OMCI_MAX_TASKS_PER_APPL; taskIndex++)
    {
        if (pAppInfo->tasks[taskIndex].taskId == OMCI_TASK_ID_INVALID)
        {
            taskInfo.taskNumber = taskIndex;
            memcpy(&(pAppInfo->tasks[taskIndex]), &taskInfo, sizeof(OMCI_TASK_INFO_T));
            break;
        }
    }
    if (OMCI_MAX_TASKS_PER_APPL == taskIndex)
    {
        return OMCI_TASK_ID_INVALID;
    }
    if (needMsgQ)
    {
        pAppInfo->tasks[taskIndex].msgQId = omci_CreateMsgQ(OMCI_MSGQKEY(pAppInfo->applId, taskIndex));

        if (OMCI_MSG_ID_INVALID == pAppInfo->tasks[taskIndex].msgQId)
        {
            pAppInfo->tasks[taskIndex].taskId = OMCI_TASK_ID_INVALID;
            return OMCI_TASK_ID_INVALID;
        }

        if (0 == msgctl(pAppInfo->tasks[taskIndex].msgQId, IPC_STAT, &tMsqDs))
        {
            tMsqDs.msg_qbytes = OMCI_MAX_BYTE_PER_MSG * (UCHAR_MAX + 1);

            if (msgctl(pAppInfo->tasks[taskIndex].msgQId, IPC_SET, &tMsqDs))
            {
                // try to set it again with a smaller number
                tMsqDs.msg_qbytes = tMsqDs.msg_qbytes / 4;

                // if it is failed still, report error
                if (msgctl(pAppInfo->tasks[taskIndex].msgQId, IPC_SET, &tMsqDs))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                        "Set IPC message queue size (bytes) to %lu fail", tMsqDs.msg_qbytes);
                }
            }
        }
    }
    else
    {
        pAppInfo->tasks[taskIndex].msgQId = OMCI_MSG_ID_INVALID;
    }
    pAppInfo->tasks[taskIndex].msgQKey = OMCI_MSGQKEY(pAppInfo->applId, taskIndex);

    if (0 != pthread_create(&thread_a, NULL, (void*)omci_TaskEnty, (void*)taskIndex))
    {
        GOS_ASSERT(0);
    }
#ifndef OMCI_X86
    /*set priority*/
    sched.sched_priority = priority;
    if(0 != pthread_setschedparam(thread_a,SCHED_FIFO,&sched))
    {
		GOS_ASSERT(0);
    }
#endif
    pAppInfo->tasks[taskIndex].taskId = (OMCI_TASK_ID)thread_a;

    if (0 != pthread_detach(thread_a))
    {
        GOS_ASSERT(0);
    }

    return pAppInfo->tasks[taskIndex].taskId;
}

OMCI_TASK_INFO_T* OMCI_GetTaskInfo(OMCI_TASK_ID taskId)
{
    UINT32            taskIndex;
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;


    if (OMCI_TASK_ID_INVALID == taskId)
    {
        return NULL;
    }

    GOS_ASSERT(pAppInfo);

    for (taskIndex = 0; taskIndex < OMCI_MAX_TASKS_PER_APPL; taskIndex++)
    {
        if (pAppInfo->tasks[taskIndex].taskId == taskId)
        {
            return &pAppInfo->tasks[taskIndex];
        }
    }

    return NULL;
}


GOS_ERROR_CODE OMCI_AppInfoSet(OMCI_STATUS_INFO_ts *pInfo)
{

    memset(&gInfo,0,sizeof(OMCI_STATUS_INFO_ts));
    memcpy(&gInfo,pInfo,sizeof(OMCI_STATUS_INFO_ts));

    OMCI_Protocol_RspCtrl(NULL);
	OMCI_WanQueue_Set();
    OMCI_PortRemap_Set();
    return GOS_OK;
}

void OMCI_DumpTask(void)
{
    UINT32            taskIndex;
    OMCI_APPL_INFO_T* pAppInfo = spOmciApplInfo;

    OMCI_PRINT("APP id          : %x", pAppInfo->applId);
    OMCI_PRINT("APP name        : %s", pAppInfo->name);
    OMCI_PRINT("APP mainTaskId  : %x", pAppInfo->mainTaskId);
    OMCI_PRINT("APP pid         : %x", pAppInfo->applPid);
    OMCI_PRINT("APP msgQId      : %x", pAppInfo->msgQId);
    OMCI_PRINT("##########################################");
    for (taskIndex = 0; taskIndex < OMCI_MAX_TASKS_PER_APPL; taskIndex++)
    {
        if (OMCI_TASK_ID_INVALID != pAppInfo->tasks[taskIndex].taskId)
        {
            OMCI_PRINT("Task Id         : %x", pAppInfo->tasks[taskIndex].taskId);
            OMCI_PRINT("Task Name       : %s", pAppInfo->tasks[taskIndex].name);
            OMCI_PRINT("Task appId      : %x", pAppInfo->tasks[taskIndex].applId);
            OMCI_PRINT("Task msgQId     : %x", pAppInfo->tasks[taskIndex].msgQId);
            OMCI_PRINT("Task msgQKey    : %d", pAppInfo->tasks[taskIndex].msgQKey);
            OMCI_PRINT("==========================================");
        }
    }
}
