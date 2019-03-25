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
 * Purpose : Definition of OMCI driver APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI driver APIs
 */

#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#ifndef OMCI_X86
/*driver related*/
#include <pkt_redirect_user.h>

#if (CONFIG_GPON_VERSION >1)
#include <module/gpon/gpon.h>
#include <rtdrv/rtdrv_netfilter.h>
#else
#include <rtk/gpon.h>
#endif

#include <module/gpon/gpon_defs.h>
#include <rtk/classify.h>
#include <rtk/ponmac.h>
#include "module/intr_bcaster/intr_bcaster.h"
#endif
/*OMCI related headler file*/
#include "app_basic.h"
#include "omci_task.h"
#include "feature_mgmt.h"

#define OMCI_GEM_DATA_FLOW_OFFSET           (0)
#define OMCI_SERV_ID_BASE                   (0)
#define OMCI_MAX_SERV_ID        (256)

static omci_pq_info_t           *g_pqInfo = NULL;
static omci_tcont_info_t        *g_tcontInfo = NULL;
static omci_gem_flow_info_t     *g_gemDsFlowInfo = NULL;
static omci_gem_flow_info_t     *g_gemUsFlowInfo = NULL;
static omci_meter_info_t        *g_meterInfo = NULL;

static OMCI_BRIDGE_RULE_ts serviceFlow[OMCI_MAX_SERV_ID];

static omci_dscp2pbit_info_t    g_dscp2pbitInfo;

static int gMsgSocketFd = -1;
static int gCtrlSocketFd = -1;
static int gIntrSocketFd = -1;
extern int gTimerSocketFd[2];

#if !defined(FPGA_DEFINED)
char logFileName[32] = "/tmp/omcilog";
char logFileNameParsed[32] = "/tmp/omcilog.par";
#else
char logFileName[32] = "/var/omcilog";
char logFileNameParsed[32] = "/var/omcilog.par";
#endif
FILE *pLogFile = NULL;
FILE *pLogFileParsed = NULL;

static INT32 omci_ioctl_send(int opt,void *data,int len);
static UINT16 omci_GetAvailUsFlowId(void);
static GOS_ERROR_CODE omci_DeletePriQByQueueId(UINT16 queueId);

static GOS_ERROR_CODE omci_TempDeleteUsGemFlowByTcontId(UINT16 tcontId);
static GOS_ERROR_CODE omci_ReCreateUsGemFlowByTcontId(UINT16 tcontId);
static UINT32 omci_IsWrrPolicy(MIB_TABLE_PRIQ_T *pPriQ, UINT16 *pEntityID);

extern BOOL gOmciOmitErrEnable;

GOS_ERROR_CODE omci_wrapper_initDrvDb(BOOL isInitPq)
{
    unsigned int i;
    if (isInitPq)
    {
        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
        UINT32 pqInfoNum = pTable->curNoUploadCount + gInfo.devCapabilities.totalTContQueueNum;

        // initialize internal pq & tc db
        g_pqInfo = malloc(sizeof(omci_pq_info_t) * pqInfoNum);
        memset(g_pqInfo, 0, sizeof(omci_pq_info_t) * pqInfoNum);
        for (i = 0; i < pqInfoNum; i++)
        {
            g_pqInfo[i].tcontId = OMCI_DRV_INVALID_TCONT_ID;
            g_pqInfo[i].tcQueueId = OMCI_DRV_INVALID_QUEUE_ID;
        }
    }
    else
    {
        g_tcontInfo = malloc(sizeof(omci_tcont_info_t) * gInfo.devCapabilities.totalTContNum);
        memset(g_tcontInfo, 0, sizeof(omci_tcont_info_t) * gInfo.devCapabilities.totalTContNum);
        for (i = 0; i < gInfo.devCapabilities.totalTContNum; i++)
        {
            g_tcontInfo[i].entityId = OMCI_DRV_INVALID_TCONT_ID;
            g_tcontInfo[i].allocId = TCONT_ALLOC_ID_984_RESERVED;
        }

        // initialize internal gem flows db
        g_gemDsFlowInfo = malloc(sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);
        g_gemUsFlowInfo = malloc(sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);
        memset(g_gemDsFlowInfo, 0, sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);
        memset(g_gemUsFlowInfo, 0, sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);

        // initialize internal meter db
        g_meterInfo = malloc(sizeof(omci_meter_info_t) * gInfo.devCapabilities.meterNum);
        memset(g_meterInfo, 0, sizeof(omci_meter_info_t) * gInfo.devCapabilities.meterNum);

        // initialize internal service flows db
        memset(serviceFlow,0,sizeof(OMCI_BRIDGE_RULE_ts)*OMCI_MAX_SERV_ID);
    }
    return GOS_OK;
}

// log functions
static void omci_logOpen()
{
    if (gInfo.logFileMode & OMCI_LOGFILE_MODE_CONSOLE)
    {
        pLogFile = stdout;
        pLogFileParsed = stdout;
    }
    else
    {
        pLogFile = fopen(logFileName,"w+");
        if(pLogFile == NULL)
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Open logfile %s fail", logFileName);

        pLogFileParsed = fopen(logFileNameParsed, "w+");
        if(pLogFileParsed == NULL)
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Open logfile %s fail", logFileNameParsed);
    }
}

static void omci_logClose(void)
{
    if (gInfo.logFileMode & OMCI_LOGFILE_MODE_CONSOLE)
    {
        pLogFile = NULL;
        pLogFileParsed = NULL;
    }
    else
    {
        if(pLogFile != NULL)
            fclose(pLogFile);

        if(pLogFileParsed != NULL)
            fclose(pLogFileParsed);
    }
}

// wrapper message handler
static int omci_Ploam_Handler(rtk_gpon_ploam_t ploam)
{
    /* It is workaround for flow to queue mapping reset to default and doesn't follow spec.
     * The correct method is not reseting to default when onu state leave O5  */
    //omci_wrapper_mibSyncReset();
       OMCI_LOG(OMCI_LOG_LEVEL_DBG,"PLOAM Handler ...");
        return OK;
}

static int gpon_alarm_handler(rtk_gpon_alarm_t alarm)
{
    omci_alm_t  alarmMsg;
    memset(&alarmMsg, 0, sizeof(omci_alm_t));

    switch(alarm.msg[0])
    {
        case SIG_TYPE_SF:
            alarmMsg.almType = OMCI_ALM_TYPE_ANI_G;
            alarmMsg.almData.almNumber = OMCI_ALM_NBR_ANIG_SF;
            break;
        case SIG_TYPE_SD:
            alarmMsg.almType = OMCI_ALM_TYPE_ANI_G;
            alarmMsg.almData.almNumber = OMCI_ALM_NBR_ANIG_SD;
            break;
        default:
            return OK;

    }
    alarmMsg.almData.almStatus = alarm.msg[1];

    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() %d; almNo=%u, state=%u ",
        __FUNCTION__, __LINE__, alarmMsg.almData.almNumber, alarmMsg.almData.almStatus);
    omci_ext_alarm_dispatcher(&alarmMsg);
    return OK;
}




GOS_ERROR_CODE omci_wrapper_signalHandler(void *pData)
{
    fd_set         fdSet;
    timer_t TimerID;

    omci_timer_setSignalMask(OMCI_TASK_SIG_HANDLE_ACCEPT_SIG_MASK);
    FD_ZERO(&fdSet);
    FD_SET(gTimerSocketFd[0], &fdSet);

    for(;;)
    {
        if (select(gTimerSocketFd[0] + 1, &fdSet, NULL, NULL, NULL)){
            recv(gTimerSocketFd[0],&TimerID,sizeof(TimerID),0);
            omci_timer_executed_cb(TimerID);
        }
    }

}




GOS_ERROR_CODE omci_wrapper_msgHandler(void *pData)
{
    fd_set         fdSet;
#ifndef OMCI_X86
    rtk_gpon_pkt_t gponPkt;
    unsigned short len=0;
#endif
    FD_ZERO(&fdSet);
    FD_SET(gMsgSocketFd, &fdSet);

    omci_timer_setSignalMask(OMCI_TASK_MSG_ACCEPT_SIG_MASK);

    omci_logOpen();

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Entering Message Loop ...");
    printf("%s: devMode %d,receiveState %d,usrLogLvl %d, drvLogLvl %d,sn %s\n",__FUNCTION__,
    gInfo.devMode,gInfo.receiveState,gUsrLogLevel,gDrvLogLevel,gInfo.sn);

    while (TRUE==gInfo.receiveState)
    {
        if (select(gMsgSocketFd + 1, &fdSet, NULL, NULL, NULL))
        {
        #ifndef OMCI_X86
            if (ptk_redirect_userApp_recvPkt(gMsgSocketFd,1500, &len,(unsigned char*)&gponPkt) > 0)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_DBG,"GPON PKT Type:%d\n",gponPkt.type);
                switch (gponPkt.type){
                case RTK_GPON_MSG_OMCI:
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Send Msg to MSG Queue!\n");
                    OMCI_SendMsg(OMCI_APPL, OMCI_RX_OMCI_MSG, OMCI_MSG_PRI_NORMAL,
                    (void *)&gponPkt.msg.omci, sizeof(rtk_gpon_omci_msg_t));
                }
                break;
                case RTK_GPON_MSG_PLOAM:
                {
                    omci_Ploam_Handler(gponPkt.msg.ploam);
                }
                break;
                case RTK_GPON_MSG_ALARM:
                {
                    gpon_alarm_handler(gponPkt.msg.alarm);
                }
                break;
                default:
                {
                }
                break;
                }
            }
        #endif
        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Leaving Message Loop ...");
    omci_logClose();
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_createMsgDev(void)
{
    int ret = GOS_OK;
    if (gMsgSocketFd < 0)
    {
        if (-1 == (gMsgSocketFd = socket(PF_NETLINK,SOCK_RAW, NETLINK_USERSOCK)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,"%s Create Socket Fail!,fd=%d\n",__FUNCTION__,gMsgSocketFd);
            return GOS_FAIL;
        }

        // modify NETLINK receive socket buffer size to tolerate burst
        int rcvBufVal;
        socklen_t rcvBufLen = sizeof(rcvBufVal);
        if (0 == getsockopt(gMsgSocketFd, SOL_SOCKET, SO_RCVBUF, &rcvBufVal, &rcvBufLen))
        {
            // divided by 2 because linux double the value on setsockopt()
            rcvBufVal = OMCI_MAX_BYTE_PER_MSG * (UCHAR_MAX + 1) / 2;

            if (setsockopt(gMsgSocketFd, SOL_SOCKET, SO_RCVBUFFORCE, &rcvBufVal, sizeof(rcvBufVal)))
            {
                // try to set it again with a smaller number
                rcvBufVal = rcvBufVal / 4;

                // if it is failed still, report error
                if (setsockopt(gMsgSocketFd, SOL_SOCKET, SO_RCVBUFFORCE, &rcvBufVal, sizeof(rcvBufVal)))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "Set receive socket buffer size (bytes) to %d fail", rcvBufVal);
                }
            }
        }
    }
    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Create Socket OK!,fd=%d\n",gMsgSocketFd);
    #ifndef OMCI_X86
    if((ret = ptk_redirect_userApp_reg(gMsgSocketFd,PR_USER_UID_GPONOMCI,1500))<0)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Register to PKT Redirect Fail!\n");
        return GOS_FAIL;
    }
    #endif
    OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Register to PKT Redirect OK !\n");
    return ret;
}

GOS_ERROR_CODE omci_wrapper_deleteMsgDev(void)
{
    if (gMsgSocketFd >= 0)
    {
        close(gMsgSocketFd);

        gMsgSocketFd = -1;
    }
    return OK;
}

GOS_ERROR_CODE omci_wrapper_sendOmciMsg(void* rawMsg, unsigned int size)
{
    int ret = GOS_OK;
    rtk_gpon_omci_msg_t omci;
    unsigned short len;

    if (!rawMsg)
        return GOS_FAIL;

    if (size < 48 || size > sizeof (omci.msg))
        return GOS_FAIL;

    len = sizeof(rtk_gpon_omci_msg_t);
    memset((void *)&omci, 0, len);
    memcpy(omci.msg, rawMsg, size);
    omci.len = size;
    #ifndef OMCI_X86
    ret = ptk_redirect_userApp_sendPkt(gMsgSocketFd,PR_KERNEL_UID_GPONOMCI,0,len, (unsigned char *)&omci);
    #endif
    OMCI_LOG(OMCI_LOG_LEVEL_DBG,"Send OMCI Msg, ret =%d!\n",ret);

    if (ret < 0)
    {
        return GOS_FAIL;
    }
    return ret;
}

// wrapper interrupt handler
static GOS_ERROR_CODE omci_intr_socket_recv(int     fd,
                                intrBcasterMsg_t    *pMsgData)
{
    int                 recvLen;
    int                 nlmsgLen;
    struct nlmsghdr     *pNlh = NULL;
    struct iovec        iov;
    struct sockaddr_nl  da;
    struct msghdr       msg;

    if (!pMsgData)
        return GOS_ERR_PARAM;

    // allocate memory
    nlmsgLen = NLMSG_SPACE(MAX_BYTE_PER_INTR_BCASTER_MSG);
    pNlh = (struct nlmsghdr *)malloc(nlmsgLen);
    if (!pNlh)
        return GOS_FAIL;

    // fill struct nlmsghdr
    memset(pNlh, 0, nlmsgLen);

    // fill struct iovec
    memset(&iov, 0, sizeof(struct iovec));
    iov.iov_base = (void *)pNlh;
    iov.iov_len = nlmsgLen;

    // fill struct sockaddr_nl
    memset(&da, 0, sizeof(struct sockaddr_nl));
    da.nl_family = AF_NETLINK;

    // fill struct msghdr
    memset(&msg, 0x0, sizeof(struct msghdr));
    msg.msg_name = (void *)&da;
    msg.msg_namelen = sizeof(da);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    // receive message
    recvLen = recvmsg(fd, &msg, 0);
    if (!NLMSG_OK(pNlh, recvLen))
    {
        free(pNlh);
        return GOS_FAIL;
    }

    memcpy(pMsgData, NLMSG_DATA(pNlh), MAX_BYTE_PER_INTR_BCASTER_MSG);
    free(pNlh);

    return GOS_OK;
}

static void omci_intr_link_change_handler(rtk_intr_status_t     intrSubType,
                                            UINT32              portId)
{
    omci_alm_t  alarmMsg;

    // only for PPTP Ethernet UNI
    alarmMsg.almType = OMCI_ALM_TYPE_PPTP_ETHERNET_UNI;
    alarmMsg.almData.almNumber = OMCI_ALM_NBR_ETHERNET_LAN_LOS;
    alarmMsg.almData.almStatus =
        (INTR_STATUS_LINKUP == intrSubType) ? OMCI_ALM_STS_CLEAR : OMCI_ALM_STS_DECLARE;
    alarmMsg.almData.almDetail = portId;

    // trigger alam mechanism
    omci_ext_alarm_dispatcher(&alarmMsg);
}

static void omci_intr_dying_gasp_handler()
{
    omci_alm_t  alarmMsg;

    // only for ONU-G
    alarmMsg.almType = OMCI_ALM_TYPE_ONU_G;
    alarmMsg.almData.almNumber = OMCI_ALM_NBR_ONUG_DYING_GASP;
    alarmMsg.almData.almStatus = OMCI_ALM_STS_DECLARE;

    // trigger alam mechanism
    omci_ext_alarm_dispatcher(&alarmMsg);
}

static void omci_intr_onu_state_change_handler(intrBcasterMsg_t msgData)
{
     MIB_TABLE_LOIDAUTH_T mibLoidAuth;

    /*Change LOID AuthStatus to 0(init value)*/
    if(GPON_STATE_O5 != msgData.intrSubType)
    {
        mibLoidAuth.EntityId = 0x0;

        if (GOS_OK != MIB_Get(MIB_TABLE_LOIDAUTH_INDEX, &mibLoidAuth, sizeof(mibLoidAuth)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Instance not found in %s: %s, 0x%x",
                __FUNCTION__, MIB_GetTableName(MIB_TABLE_LOIDAUTH_INDEX), mibLoidAuth.EntityId);
        }

        mibLoidAuth.AuthStatus = PON_ONU_LOID_INITIAL_STATE;
        gInfo.loidCfg.loidAuthStatus = PON_ONU_LOID_INITIAL_STATE;
        if (GPON_STATE_O1 == msgData.intrSubType)
            gInfo.loidCfg.lastLoidAuthStatus = PON_ONU_LOID_INITIAL_STATE;
        MIB_Set(MIB_TABLE_LOIDAUTH_INDEX, &mibLoidAuth, sizeof(mibLoidAuth));
        memset(&gInfo.adtInfo, 0, sizeof(auth_duration_time_info_t));
    }

    if (GPON_STATE_O5 == msgData.intrSubType)
        clock_gettime(CLOCK_MONOTONIC, &gInfo.adtInfo.start_time);

    gInfo.onuState = msgData.intrSubType;
}

static void omci_intr_loop_indicator_handler(intrBcasterMsg_t msgData)
{
    omci_alm_t                      alarmMsg;
    omci_me_instance_t              pptpEthUniInstanceID;

    alarmMsg.almType = OMCI_ALM_TYPE_LOOP_DETECT;
    alarmMsg.almData.almNumber = 0;
    alarmMsg.almData.almStatus = msgData.intrSubType;

    // translate switch port to me id
    if (GOS_OK != pptp_eth_uni_switch_port_to_me_id(msgData.intrBitMask, &pptpEthUniInstanceID))
    {
        OMCI_LOG (OMCI_LOG_LEVEL_ERR, "Failed toe get the instance ID of the pptpEthuni\n");
        return;
    }

    alarmMsg.almData.almDetail = pptpEthUniInstanceID;

    omci_ext_alarm_dispatcher(&alarmMsg);
}

static
void
omci_intr_port_blocking_handler (
    intrBcasterMsg_t msgData)
{
    omci_alm_t                      alarmMsg;
    omci_me_instance_t              pptpEthUniInstanceID;

    MIB_ENTRY_T                     *pEntry         = NULL;
    MIB_TREE_T                      *pTree          = NULL;
    MIB_NODE_T                      *pNode          = NULL;
    MIB_TREE_NODE_ENTRY_T           *pNodeEntry     = NULL;
    MIB_TABLE_MACBRIPORTCFGDATA_T   *pBridgePort    = NULL;

    alarmMsg.almType = OMCI_ALM_TYPE_MAC_BRIDGE_PORT_CONFIG_DATA;
    alarmMsg.almData.almNumber = OMCI_ALM_NBR_MBPCD_PORT_BLOCKING;
    alarmMsg.almData.almStatus = msgData.intrSubType;

    // translate switch port to me id
    if (GOS_OK != pptp_eth_uni_switch_port_to_me_id(msgData.intrBitMask, &pptpEthUniInstanceID))
    {
        OMCI_LOG (OMCI_LOG_LEVEL_ERR, "Failed toe get the instance ID of the pptpEthuni\n");
        return;
    }

    if (!(pTree = MIB_AvlTreeSearchByKey(NULL, pptpEthUniInstanceID, AVL_KEY_PPTPUNI)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Can't find Root Tree");
        return;
    }

    if (!(pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_UNI)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "Can't find Mac bridge port uni entries");
        return;
    }

    LIST_FOREACH(pNodeEntry, &pNode->data.treeNodeEntryHead, treeNodeEntry)
    {
        pEntry = pNodeEntry->mibEntry;
        pBridgePort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pEntry->pData;

        if (MBPCD_TP_TYPE_PPTP_ETH_UNI == pBridgePort->TPType &&
            pBridgePort->TPPointer == pptpEthUniInstanceID)
        {
            break;
        }
    }

    if (!pBridgePort)
        return;

    alarmMsg.almData.almDetail = pBridgePort->EntityID;

    omci_ext_alarm_dispatcher(&alarmMsg);
}

GOS_ERROR_CODE omci_wrapper_intrHandler(void    *pData)
{
#ifdef OMCI_X86
    while (1)
    {
        sleep(5);
    }
#else
    fd_set              fdSet;
    FD_ZERO(&fdSet);
    FD_SET(gIntrSocketFd, &fdSet);

    intrBcasterMsg_t    msgData;


    omci_timer_setSignalMask(OMCI_TASK_INST_ACCEPT_SIG_MASK);

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Entering Interrupt Handler Loop...");

    while (1)
    {

        if (select(gIntrSocketFd + 1, &fdSet, NULL, NULL, NULL) < 0)
            break;

        if (!FD_ISSET(gIntrSocketFd, &fdSet))
            continue;

        if (GOS_OK == omci_intr_socket_recv(gIntrSocketFd, &msgData))
        {
            if (MSG_TYPE_LINK_CHANGE == msgData.intrType)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Receive Interrupt: LINK CHANGE");

                omci_intr_link_change_handler(msgData.intrSubType, msgData.intrBitMask);
            }

            if (MSG_TYPE_DYING_GASP == msgData.intrType)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Receive Interrupt: DYING GASP");

                omci_intr_dying_gasp_handler();
            }

            if (MSG_TYPE_ONU_STATE == msgData.intrType)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Receive Interrupt: ONU STATE CHANGE");

                omci_intr_onu_state_change_handler(msgData);
            }

            if (MSG_TYPE_RLDP_LOOP_STATE_CHNG == msgData.intrType)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Receive Interrupt: LOOP STATE CHANGE");
                omci_intr_port_blocking_handler(msgData);
                sleep (3);
                omci_intr_loop_indicator_handler(msgData);
                sleep (3);
            }

        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Leaving Interrupt Handler Loop...");
#endif
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_createIntrSocket(void)
{
    if (gIntrSocketFd < 0)
    {
        struct sockaddr_nl sa;

        if (-1 == (gIntrSocketFd = socket(PF_NETLINK, SOCK_RAW, INTR_BCASTER_NETLINK_TYPE)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Create Interrupt Socket Fail!\n");

            return GOS_FAIL;
        }

        memset(&sa, 0, sizeof(sa));
        sa.nl_family = AF_NETLINK;
        sa.nl_groups = (1 << MSG_TYPE_LINK_CHANGE) | (1 << MSG_TYPE_DYING_GASP) | (1 << MSG_TYPE_ONU_STATE) | (1 << MSG_TYPE_RLDP_LOOP_STATE_CHNG);

        if (-1 == bind(gIntrSocketFd, (struct sockaddr*)&sa, sizeof(struct sockaddr)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Bind Interrupt Socket Fail!\n");

            return GOS_FAIL;
        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Create Interrupt Socket OK!\n");

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_deleteIntrSocket(void)
{
    if (gIntrSocketFd >= 0)
    {
        close(gIntrSocketFd);

        gIntrSocketFd = -1;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Delete Interrupt Socket OK!\n");

    return GOS_OK;
}

// wrapper ctrl functions
GOS_ERROR_CODE omci_wrapper_createCtrlDev(void)
{
    if ((gCtrlSocketFd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
    {
        return -1;
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_deleteCtrlDev(void)
{
    if (gCtrlSocketFd >= 0)
    {
        close(gCtrlSocketFd);

        gCtrlSocketFd = -1;
    }
    return OK;
}

static INT32 omci_ioctl_send(int opt,void *data,int len)
{
    int ret;
#ifdef OMCI_X86
    omci_ioctl_cmd_t ioctlMsg;
#else
    rtk_gpon_extMsg_t ioctlMsg;
#endif
    int rlen;

    if(len > sizeof(ioctlMsg.extValue))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"data over size, len=%d!",len);
        return OMCI_ERR_FAILED;
    }
    ioctlMsg.len =len;
    ioctlMsg.optId = opt;
    memcpy(ioctlMsg.extValue,data,len);
    if (2 == gOmciOmitErrEnable)
        ret = OMCI_ERR_OK;
    else
    {
        rlen = sizeof(rtk_gpon_extMsg_t);
#ifdef OMCI_X86
        ret=getsockopt(gCtrlSocketFd, IPPROTO_IP,OMCIDRV_GPON_EXTMSG_GET, (void *)&ioctlMsg, (socklen_t *)&rlen);
#else
        ret=getsockopt(gCtrlSocketFd, IPPROTO_IP,RTDRV_GPON_EXTMSG_GET, (void *)&ioctlMsg, (socklen_t *)&rlen);
#endif
    }

    memcpy(data,ioctlMsg.extValue,len);

    return ret;
}

void omci_wrapper_dumpSrvId(void)
{
    UINT16 servId=0;

    for(servId = OMCI_SERV_ID_BASE; servId < OMCI_MAX_SERV_ID; servId++)
    {
        printf("SERVID %d: Used: %d, (DIR=%d, USFID=%d, DSFID=%d, SERVID=%d, UNIMASK=%d) \n", servId, serviceFlow[servId].isUsed,
        serviceFlow[servId].dir, serviceFlow[servId].usFlowId, serviceFlow[servId].dsFlowId,
        serviceFlow[servId].servId,serviceFlow[servId].uniMask);
    }
}

void omci_wrapper_dumpFlowGemMap(void)
{
    UINT16 flowId;

    OMCI_PRINT("\n\n");
    OMCI_PRINT("============= used us flow ===============");
    OMCI_PRINT("%8s\t%9s\t%8s", "FlowID", "GemPort", "PQ_EID");

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (g_gemUsFlowInfo[flowId].inUse == FALSE)
            continue;

        OMCI_PRINT("%s\t%s\t%s", "--------", "---------", "--------");
        OMCI_PRINT("%8u\t%9u\t%#8x",
            flowId, g_gemUsFlowInfo[flowId].portId, g_gemUsFlowInfo[flowId].pqEntityId);
    }

    OMCI_PRINT("\n\n");
    OMCI_PRINT("============= used ds Flow ===============");
    OMCI_PRINT("%8s\t%9s\t%8s", "FlowID", "GemPort", "PQ_EID");

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (g_gemDsFlowInfo[flowId].inUse == FALSE)
            continue;

        OMCI_PRINT("%s\t%s\t%s", "--------", "---------", "--------");
        OMCI_PRINT("%8u\t%9u\t%#8x",
            flowId, g_gemDsFlowInfo[flowId].portId, g_gemDsFlowInfo[flowId].pqEntityId);
    }

}

void omci_wrapper_dumpPQ2TC(void)
{
    unsigned int i;
    UINT16       dpQ_eid;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    OMCI_PRINT("\n");
    OMCI_PRINT("============= PQ ===============");
    OMCI_PRINT("%7s\t%6s\t%6s\t%6s\t%6s\t%6s\t%6s\t%7s\t%7s\t%6s",
        "MEID", "QID", "Policy", "P/W", "TCID", "DP", "Queue", "CIR(8K)", "PIR(8K)", "UsedCnt");
    for(i = 0; i < gInfo.devCapabilities.totalTContQueueNum; i++)
    {
        if (PQ_DROP_COLOUR_NO_MARKING == g_pqInfo[i].dpMarking)
            dpQ_eid = OMCI_MIB_US_TM_ME_ID_BASE + g_pqInfo[i].dpQueueId;
        else
        {
            dpQ_eid = OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                        (g_pqInfo[i].dpQueueId - gInfo.devCapabilities.totalTContQueueNum);
        }
        OMCI_PRINT("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s", "-------", "------",
            "------", "------", "------", "------", "------", "-------", "-------", "-------");
        OMCI_PRINT("%#7x\t%6u\t%6s\t%6u\t%6u\t%6u\t%#6x\t%7u\t%7u\t%6u",
            (OMCI_MIB_US_TM_ME_ID_BASE + i), g_pqInfo[i].tcQueueId,
            (PQ_POLICY_STRICT_PRIORITY == g_pqInfo[i].policy) ? "SP" : "WRR",
            (PQ_POLICY_STRICT_PRIORITY == g_pqInfo[i].policy) ?
            g_pqInfo[i].pw.priority : g_pqInfo[i].pw.weight, g_pqInfo[i].tcontId,
            g_pqInfo[i].dpMarking, dpQ_eid,
            g_pqInfo[i].cir, g_pqInfo[i].pir, g_pqInfo[i].usedCnt);
    }
    for(i = gInfo.devCapabilities.totalTContQueueNum; i < pTable->curNoUploadCount + gInfo.devCapabilities.totalTContQueueNum; i++)
    {
        OMCI_PRINT("%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s", "-------", "------",
            "------", "------", "------", "------", "------", "-------", "-------", "-------");
        OMCI_PRINT("%#7x\t%6u\t%6s\t%6u\t%6u\t%6u\t%#6x\t%7u\t%7u\t%6u",
            (OMCI_MIB_US_TM_ME_RSV_ID_BASE + i - gInfo.devCapabilities.totalTContQueueNum), g_pqInfo[i].tcQueueId,
            (PQ_POLICY_STRICT_PRIORITY == g_pqInfo[i].policy) ? "SP" : "WRR",
            (PQ_POLICY_STRICT_PRIORITY == g_pqInfo[i].policy) ?
            g_pqInfo[i].pw.priority : g_pqInfo[i].pw.weight, g_pqInfo[i].tcontId,
            g_pqInfo[i].dpMarking, (OMCI_MIB_US_TM_ME_ID_BASE + g_pqInfo[i].dpQueueId),
            g_pqInfo[i].cir, g_pqInfo[i].pir, g_pqInfo[i].usedCnt);
    }

    OMCI_PRINT("\n");
    OMCI_PRINT("=========== T-Cont =============");
    OMCI_PRINT("%6s\t%6s\t%6s\t%10s",
        "TCID", "MEID", "Alloc", "GEM count");
    for(i = 0; i < gInfo.devCapabilities.totalTContNum; i++)
    {
        OMCI_PRINT("%s\t%s\t%s\t%s",
            "------", "------", "------", "----------");
        OMCI_PRINT("%6u\t%#6x\t%6u\t%10u",
            i, g_tcontInfo[i].entityId,
            g_tcontInfo[i].allocId, g_tcontInfo[i].gemCount);
    }

    omci_wrapper_dumpFlowGemMap();

}

// driver control
GOS_ERROR_CODE omci_SetTcontInfo(UINT16 tcontId, UINT16 entityId, UINT16 allocId)
{
    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    g_tcontInfo[tcontId].entityId = entityId;
    g_tcontInfo[tcontId].allocId = allocId;

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_resetMib(void)
{
    unsigned int i;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_MIB_RESET, NULL, 0))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }

    for (i = 0; i < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; i++)
        omci_DeletePriQByQueueId(i);

    memset(g_pqInfo, 0, sizeof(omci_pq_info_t) * gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount);
    for (i = 0; i < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; i++)
    {
        g_pqInfo[i].tcontId = OMCI_DRV_INVALID_TCONT_ID;
        g_pqInfo[i].tcQueueId = OMCI_DRV_INVALID_QUEUE_ID;
    }
    memset(g_tcontInfo, 0, sizeof(omci_tcont_info_t) * gInfo.devCapabilities.totalTContNum);
    for (i = 0; i < gInfo.devCapabilities.totalTContNum; i++)
    {
        g_tcontInfo[i].entityId = OMCI_DRV_INVALID_TCONT_ID;
        g_tcontInfo[i].allocId = TCONT_ALLOC_ID_984_RESERVED;
    }
    memset(g_gemDsFlowInfo, 0, sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);
    memset(g_gemUsFlowInfo, 0, sizeof(omci_gem_flow_info_t) * gInfo.devCapabilities.totalGEMPortNum);
    memset(g_meterInfo, 0, sizeof(omci_meter_info_t) * gInfo.devCapabilities.meterNum);

    MIB_AvlTreeRemoveAll();
    memset(serviceFlow,0,sizeof(OMCI_BRIDGE_RULE_ts)*OMCI_MAX_SERV_ID);
    //for replay again
    OMCI_ResetHistoryRspMsg();
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setLog(int level)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_LOG_SET, &level, sizeof(int)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setLogToFile(OMCI_LOGFILE_MODE  mode,
                                        UINT32              actMask,
                                        CHAR                *fileName)
{
    //int ret;
    if(0 != actMask)
        gInfo.gLoggingActMask = actMask;

    gInfo.logFileMode = mode;
    if(strlen(fileName) != 0)
    {
        omci_logClose();
        if (gInfo.logFileMode & OMCI_LOGFILE_MODE_CONSOLE)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "%s Output log on console", __FUNCTION__);
        }
        else
        {
            strcpy(logFileName, fileName);
            strcpy(logFileNameParsed, fileName);
            strcat(logFileNameParsed, ".par");
        }
        omci_logOpen();
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getLogToFileCfg(void)
{
    char buff[128];
    int buffLen = 0;
    memset(buff, 0 , sizeof(buff));
    int oldstdout = -1, nul = -1;
    const int stdout_fd = fileno(stdout);
    nul = omci_open_cli_fd();

    if (-1 != nul)
    {
        oldstdout = dup(stdout_fd);
        dup2(nul, stdout_fd);
        close(nul);
    }

    OMCI_PRINT("Mode:");
    if(gInfo.logFileMode == OMCI_LOGFILE_MODE_DISABLE)
    {
        OMCI_PRINT("Without logging to file");
    } else {
        buffLen = sprintf(buff, "logging to file is");

        if(gInfo.logFileMode & OMCI_LOGFILE_MODE_RAW)
            buffLen += sprintf(buff + buffLen, " Raw Mode");

        if(gInfo.logFileMode & OMCI_LOGFILE_MODE_PARSED)
            buffLen += sprintf(buff + buffLen, " Parsed Mode");

        if(gInfo.logFileMode & OMCI_LOGFILE_MODE_WITH_TIMESTAMP)
            buffLen += sprintf(buff + buffLen, " with Time Stamp");

        if(gInfo.logFileMode & OMCI_LOGFILE_MODE_CONSOLE)
            buffLen += sprintf(buff + buffLen, " Console Mode");

        OMCI_PRINT("%s",buff);
    }
    OMCI_PRINT("ActionMask:");
    OMCI_PRINT("0x%X", gInfo.gLoggingActMask);

    OMCI_PRINT("Log File:");
    if (gInfo.logFileMode & OMCI_LOGFILE_MODE_CONSOLE)
    {
        OMCI_PRINT("NA, output console");
    }
    else
    {
        OMCI_PRINT("%s", logFileName);
    }

    if (-1 != nul)
    {
        fflush(stdout);
        if (-1 != oldstdout)
        {
            dup2(oldstdout, stdout_fd);
            close(oldstdout);
        }
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setDevMode(int devMode)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_DEVMODE_SET,&devMode,sizeof(devMode));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

// device info
GOS_ERROR_CODE omci_wrapper_getDevCapabilities(omci_dev_capability_t *p)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_DEV_CAPABILITIES_GET, p, sizeof(omci_dev_capability_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getDevIdVersion(omci_dev_id_version_t *p)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_DEV_ID_VERSION_GET, p, sizeof(omci_dev_id_version_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setDualMgmtMode(int state)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_DUAL_MGMT_MODE_SET, &state, sizeof(int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getDrvVersion(char *drvVersion)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_DRV_VERSION_GET, drvVersion, 64))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

// optical info and control
GOS_ERROR_CODE omci_wrapper_getUsDBRuStatus(unsigned int *pStatus)
{
    INT32   ret;

    if (!pStatus)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_US_DBRU_STATUS_GET, pStatus, sizeof(unsigned int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getTransceiverStatus(omci_transceiver_status_t *p)
{
    INT32   ret;

    if (!p)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_TRANSCEIVER_STATUS_GET, p, sizeof(omci_transceiver_status_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setSignalParameter(OMCI_SIGNAL_PARA_ts *pSigPara)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_SIGPARAMETER_SET, pSigPara, sizeof(OMCI_SIGNAL_PARA_ts)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getOnuState(PON_ONU_STATE *pOnuState)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_ONUSTATE_GET, pOnuState, sizeof(PON_ONU_STATE)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

// pon/ani control
GOS_ERROR_CODE omci_wrapper_setSerialNum(unsigned char *serial)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_SN_SET, serial, 9))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getSerialNum(char *serial)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_SN_GET, serial, 9))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setGponPasswd(unsigned char* gponPwd)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_GPONPWD_SET, gponPwd, 10))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_activateGpon(int activate)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_GPON_ACTIVATE, &activate, sizeof(int)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setGemBlkLen(unsigned short gemBlkLen)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_GEMBLKLEN_SET, &gemBlkLen, sizeof(unsigned short)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_getGemBlkLen(unsigned short *pGemBlkLen)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_GEMBLKLEN_GET, pGemBlkLen, sizeof(unsigned short)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_wrapper_setPonBwThreshold(omci_pon_bw_threshold_t *pPonBwThreshold)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_PON_BWTHRESHOLD_SET, pPonBwThreshold, sizeof(omci_pon_bw_threshold_t)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
        return GOS_FAIL;
    }
    return GOS_OK;
}

static UINT16
omci_GetTcontIdByTcEntityId(UINT16 entityId)
{
    UINT16 tcontId;

    for (tcontId = 0; tcontId < gInfo.devCapabilities.totalTContNum; tcontId++)
    {
        if (g_tcontInfo[tcontId].entityId == entityId)
        {
            return tcontId;
        }
    }

    return OMCI_DRV_INVALID_TCONT_ID;
}

GOS_ERROR_CODE
omci_wrapper_createTcont(UINT16 entityId, UINT16 allocId)
{
    INT32           ret;
    OMCI_TCONT_ts   tcont;
    UINT16          newTcontId;
    UINT16          oldTcontId;
    unsigned int    i;

    tcont.allocId = allocId;

    ret = omci_ioctl_send(OMCI_IOCTL_TCONT_GET, &tcont, sizeof(OMCI_TCONT_ts));
    if (OMCI_ERR_OK != ret)
        return GOS_FAIL;
#ifdef OMCI_X86
    newTcontId = allocId;
#else
    newTcontId = tcont.tcontId;
#endif
    oldTcontId = omci_GetTcontIdByTcEntityId(entityId);

    if (newTcontId != oldTcontId)
    {
        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
        omci_SetTcontInfo(oldTcontId,
            g_tcontInfo[newTcontId].entityId, g_tcontInfo[newTcontId].allocId);

        // update pq info whose tcont is related
        for (i = 0; i < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; i++)
        {
            if (g_pqInfo[i].tcontId == oldTcontId)
                g_pqInfo[i].tcontId = newTcontId;
            else if (g_pqInfo[i].tcontId == newTcontId)
                g_pqInfo[i].tcontId = oldTcontId;
        }
    }
    omci_SetTcontInfo(newTcontId, entityId, allocId);

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,
        "create tcont [%x], allocId [%d]\n", entityId, allocId);

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_deleteTcont(UINT16 entityId, UINT16 allocId)
{
    UINT16  tcontId;

    tcontId = omci_GetTcontIdByTcEntityId(entityId);
    if (OMCI_DRV_INVALID_TCONT_ID == tcontId)
        return GOS_FAIL;

    omci_SetTcontInfo(tcontId, entityId, TCONT_ALLOC_ID_984_RESERVED);

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,
        "delete tcont [%x], allocId [%d]\n", entityId, allocId);

    return GOS_OK;
}

static UINT16
omci_GetTcontByNumOfReqQueue(UINT16 tcontId, unsigned int numOfReqQueue)
{
    INT32           ret;
    OMCI_TCONT_ts   tcont;
    UINT16          entityId;
    UINT16          queueId;

    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    // use tcontId to pass numOfReqQueue
    tcont.tcontId = numOfReqQueue;
    tcont.allocId = g_tcontInfo[tcontId].allocId;

    ret = omci_ioctl_send(OMCI_IOCTL_TCONT_UPDATE, &tcont, sizeof(OMCI_TCONT_ts));
    if (OMCI_ERR_OK != ret)
        return OMCI_DRV_INVALID_TCONT_ID;

    if (tcont.tcontId != tcontId)
    {
        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

        omci_util_swap ( &g_tcontInfo[tcont.tcontId].gemCount, &g_tcontInfo[tcontId].gemCount );

        entityId = g_tcontInfo[tcont.tcontId].entityId;

        // set new tcont info
        omci_SetTcontInfo(tcont.tcontId,
            g_tcontInfo[tcontId].entityId, g_tcontInfo[tcontId].allocId);
        // erase old tcont info
        omci_SetTcontInfo(tcontId,
            entityId, TCONT_ALLOC_ID_984_RESERVED);

        // update pq info whose tcont is related
        for (queueId = 0; queueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; queueId++)
        {
            if (g_pqInfo[queueId].tcontId == tcontId)
                g_pqInfo[queueId].tcontId = tcont.tcontId;
            else if (g_pqInfo[queueId].tcontId == tcont.tcontId)
                g_pqInfo[queueId].tcontId = tcontId;
        }
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,
        "use tcont %u for %u queues", tcont.tcontId, numOfReqQueue);

    return tcont.tcontId;
}

static GOS_ERROR_CODE
omci_CreatePriQByQueueId(UINT16 queueId)
{
    INT32           ret;
    OMCI_PRIQ_ts    priQ;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        return GOS_ERR_PARAM;

    if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    memset(&priQ, 0, sizeof(OMCI_PRIQ_ts));

    priQ.scheduleType = g_pqInfo[queueId].policy;
    priQ.weight = (PQ_POLICY_STRICT_PRIORITY == priQ.scheduleType) ?
        1 : g_pqInfo[queueId].pw.weight;

    priQ.cir = g_pqInfo[queueId].cir;
    priQ.pir = g_pqInfo[queueId].pir;

    priQ.queueId = g_pqInfo[queueId].tcQueueId;
    priQ.tcontId = g_pqInfo[queueId].tcontId;

    priQ.dpMarking = g_pqInfo[queueId].dpMarking;
    priQ.dir = PON_GEMPORT_DIRECTION_US;

    ret = omci_ioctl_send(OMCI_IOCTL_PRIQ_SET, &priQ, sizeof(OMCI_PRIQ_ts));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "create tcont queue [%u] failed!", queueId);

        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "create tcont queue [%u]", queueId);

    return GOS_OK;
}

static int comparePqByPriority(const void *a, const void *b)
{
    omci_pq_info_t  *pQueueA;
    omci_pq_info_t  *pQueueB;

    if (!a || !b)
        return 0;

    pQueueA = (omci_pq_info_t *)a;
    pQueueB = (omci_pq_info_t *)b;

    if (PQ_POLICY_STRICT_PRIORITY == pQueueA->policy)
    {
        if (PQ_POLICY_STRICT_PRIORITY == pQueueB->policy)
        {
            if (pQueueA->pw.priority < pQueueB->pw.priority)
                return 1;
            if (pQueueA->pw.priority == pQueueB->pw.priority)
                return 0;
            if (pQueueA->pw.priority > pQueueB->pw.priority)
                return -1;
        }
        else
            return 1;
    }
    else
    {
        if (PQ_POLICY_STRICT_PRIORITY == pQueueB->policy)
            return -1;
        // no need to sort by weight
        /*else
        {
            if (pQueueA->pw.weight < pQueueB->pw.weight)
                return -1;
            if (pQueueA->pw.weight == pQueueB->pw.weight)
                return 0;
            if (pQueueA->pw.weight > pQueueB->pw.weight)
                return 1;
        }*/
    }

    return 0;
}

static GOS_ERROR_CODE
omci_CreatePriQByTcontId(UINT16 tcontId, UINT16 *pNewTcontId)
{
    GOS_ERROR_CODE  ret = GOS_OK;
    omci_pq_info_t  *pQueue;
    UINT16          queueId;
    unsigned int    numOfReqQueue;
    unsigned int    numOfGemcount;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (!pNewTcontId)
        return GOS_ERR_PARAM;

    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    // initial the new id same as parameter
    *pNewTcontId = tcontId;

    // pq should be ready if any gem port has been created
    if (g_tcontInfo[tcontId].gemCount != 0)
    {
        if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_DEL_PQ_CFG,
            gInfo.devCapabilities.totalTContQueueNum, gInfo.devCapabilities.totalTContNum, g_pqInfo,
            (OMCI_DEL_PRIQ_BY_QID_PTR)omci_DeletePriQByQueueId))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
        }
        //return GOS_OK;
    }

    numOfReqQueue = 0;
    for (queueId = 0; queueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; queueId++)
    {
        if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
            continue;

        if (g_pqInfo[queueId].tcontId != tcontId)
            continue;

        numOfReqQueue++;

        if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_SET_QID, &queueId))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
        }
    }

    if (0 == numOfReqQueue)
        return GOS_FAIL;

    if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_UPDATE_PQ_CFG,
        &numOfGemcount, gInfo.devCapabilities.totalTContNum, gInfo.devCapabilities.totalTContQueueNum,
        g_tcontInfo, &numOfReqQueue, g_pqInfo, &tcontId, (OMCI_DEL_PRIQ_BY_QID_PTR)omci_DeletePriQByQueueId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
    }



    if (g_tcontInfo[tcontId].gemCount == 0)
    {
        // find tcont that is capable of create sufficient queues
        *pNewTcontId = omci_GetTcontByNumOfReqQueue(tcontId, numOfReqQueue);
        if (*pNewTcontId >= gInfo.devCapabilities.totalTContNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no tcont available for %u queues", numOfReqQueue);

            return GOS_FAIL;
        }
    }

    // starting to normalized tcQueueId
    pQueue = malloc(sizeof(omci_pq_info_t) * numOfReqQueue);
    if (!pQueue)
        return GOS_FAIL;

    // copy related pq info into temporarily array for sorting
    for (queueId = 0; queueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; queueId++)
    {
        if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
            continue;

        if (g_pqInfo[queueId].tcontId != *pNewTcontId)
            continue;

        memcpy(pQueue, &g_pqInfo[queueId], sizeof(omci_pq_info_t));

        // store queueId into tcQueueId
        pQueue->tcQueueId = queueId;

        pQueue++;
    }
    pQueue = pQueue - numOfReqQueue;

    // sort pQueue array to give out the actual desired queue sequence
    qsort(pQueue, numOfReqQueue, sizeof(omci_pq_info_t), comparePqByPriority);

    // write tcQueueId according to the sorting result
    for (queueId = 0; queueId < numOfReqQueue; queueId++)
    {
        g_pqInfo[pQueue->tcQueueId].tcQueueId = queueId;

        pQueue++;
    }
    pQueue = pQueue - numOfReqQueue;

    // end of normalization
    free(pQueue);

    // create all queues for specified tcont
    for (queueId = 0; queueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; queueId++)
    {
        if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
            continue;

        if (g_pqInfo[queueId].tcontId != *pNewTcontId)
            continue;

        ret = omci_CreatePriQByQueueId(queueId);
    }

    return ret;
}

static GOS_ERROR_CODE omci_DeletePriQByQueueId(UINT16 queueId)
{
    INT32           ret;
    OMCI_PRIQ_ts    priQ;

    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
    if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        return GOS_ERR_PARAM;

    if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    // ignore queue that is never configured
    if (OMCI_DRV_INVALID_QUEUE_ID == g_pqInfo[queueId].tcQueueId)
        return GOS_OK;

    memset(&priQ, 0, sizeof(OMCI_PRIQ_ts));

    priQ.queueId = g_pqInfo[queueId].tcQueueId;
    priQ.tcontId = g_pqInfo[queueId].tcontId;

    ret = omci_ioctl_send(OMCI_IOCTL_PRIQ_DEL, &priQ, sizeof(OMCI_PRIQ_ts));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete tcont queue [%u] failed", queueId);

        return GOS_FAIL;
    }

    g_pqInfo[queueId].tcQueueId = OMCI_DRV_INVALID_QUEUE_ID;

    if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_DEL_QID, g_pqInfo, &queueId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "delete tcont queue [%u]", queueId);

    return GOS_OK;
}

static GOS_ERROR_CODE
omci_DeletePriQByTcontId(UINT16 tcontId, UINT16 qId, UINT32 usedCnt)
{
    GOS_ERROR_CODE  ret = GOS_FAIL;
    UINT16          queueId;

    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (tcontId >= gInfo.devCapabilities.totalTContNum || !pTable)
        return GOS_ERR_PARAM;

    // block deletion if pq still associated with gem ports
    if (0 == usedCnt)
        return omci_DeletePriQByQueueId(qId);

    for (queueId = 0; queueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; queueId++)
    {
        if (g_pqInfo[queueId].tcontId != tcontId ||
            g_pqInfo[queueId].usedCnt != 0)
        {
            continue;
        }
        ret = omci_DeletePriQByQueueId(queueId);
    }

    return ret;
}

static UINT16
omci_GetTcontIdByPqEntityId(UINT16 entityId)
{
    UINT16 queueId;

    if (entityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    gInfo.devCapabilities.totalTContQueueNum;

        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        {
            return OMCI_DRV_INVALID_TCONT_ID;
        }
    }
    else
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_ID_BASE;

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
        {
            return OMCI_DRV_INVALID_TCONT_ID;
        }
    }

    return g_pqInfo[queueId].tcontId;
}

static UINT16
omci_GetTcQueueIdByPqEntityId(UINT16 entityId)
{
    UINT16 queueId;

    if (entityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    gInfo.devCapabilities.totalTContQueueNum;

        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        {
            return OMCI_DRV_INVALID_QUEUE_ID;
        }
    }
    else
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_ID_BASE;

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
        {
            return OMCI_DRV_INVALID_QUEUE_ID;
        }
    }


    return g_pqInfo[queueId].tcQueueId;
}

static UINT8
omci_GetDpMarkingByPqEntityId(UINT16 entityId)
{
    UINT16 queueId;

    if (entityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    gInfo.devCapabilities.totalTContQueueNum;

        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        {
            return PQ_DROP_COLOUR_NO_MARKING;
        }
    }
    else
    {
        queueId = entityId - OMCI_MIB_US_TM_ME_ID_BASE;

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
        {
            return PQ_DROP_COLOUR_NO_MARKING;
        }
    }

    return g_pqInfo[queueId].dpMarking;
}

/* Input argument queueId is index of g_pqInfo array */
static UINT16
omci_GetUsDpFlowByDpQueueId(UINT16 queueId)
{
    UINT16  dpFlowId;
    UINT16  dpQueueId;

    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        return gInfo.devCapabilities.totalGEMPortNum;

    for (dpFlowId = 0; dpFlowId < gInfo.devCapabilities.totalGEMPortNum; dpFlowId++)
    {
        if (FALSE == g_gemUsFlowInfo[dpFlowId].inUse)
            continue;

        if (g_gemUsFlowInfo[dpFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
        {
            dpQueueId = g_gemUsFlowInfo[dpFlowId].pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                gInfo.devCapabilities.totalTContQueueNum;

            if (dpQueueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
                continue;
        }
        else
        {
            /*dpQueueId = g_gemUsFlowInfo[dpFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
            if (dpQueueId >= gInfo.devCapabilities.totalTContQueueNum)*/
                continue;
        }

        if (dpQueueId == g_pqInfo[queueId].dpQueueId)
            break;
    }

    return dpFlowId;
}

static UINT16 omci_GetAvailDsFlowId(void)
{
    UINT16 flowId;

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (g_gemDsFlowInfo[flowId].inUse == FALSE)
            break;
    }

    return flowId;
}

static UINT16 omci_GetAvailUsFlowId(void)
{
    UINT16 flowId;

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (g_gemUsFlowInfo[flowId].inUse == FALSE)
            break;
    }

    return flowId;
}

UINT16 omci_GetDsFlowIdByPortId(UINT16 portId)
{
    UINT16 flowId;

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (TRUE == g_gemDsFlowInfo[flowId].inUse &&
                g_gemDsFlowInfo[flowId].portId == portId)
            break;
    }

    return flowId;
}

UINT16 omci_GetUsFlowIdByPortId(UINT16 portId)
{
    UINT16 flowId;

    for (flowId = OMCI_GEM_DATA_FLOW_OFFSET; flowId < gInfo.devCapabilities.totalGEMPortNum; flowId++)
    {
        if (TRUE == g_gemUsFlowInfo[flowId].inUse &&
                g_gemUsFlowInfo[flowId].portId == portId)
        {
            if (gInfo.devCapabilities.perTContQueueDp)
                break;
            else
            {
                // skip dp flows
                if (PQ_DROP_COLOUR_INTERNAL_MARKING !=
                        omci_GetDpMarkingByPqEntityId(g_gemUsFlowInfo[flowId].pqEntityId))
                    break;
            }
        }
    }

    return flowId;
}
GOS_ERROR_CODE omci_ClearUsedPriority(unsigned char priority)
{
    if (1 == ((g_dscp2pbitInfo.usePbitBitmap >> priority) & 0x1))
    {
        //clear status for usage
        g_dscp2pbitInfo.usePbitBitmap &= ~(1 << priority);

    }
    return GOS_OK;
}

GOS_ERROR_CODE omci_GetNonUsedPriority(unsigned char priority)
{
    if (0 == ((g_dscp2pbitInfo.usePbitBitmap >> priority) & 0x1))
    {
        //update status for usage
        g_dscp2pbitInfo.usePbitBitmap |= (1 << priority);

        return TRUE;
    }

    return FALSE;
}

GOS_ERROR_CODE omci_GetDscp2pbitBitmap(unsigned char *pData)
{
    if (!pData)
        return GOS_ERR_PARAM;

    if (0 == g_dscp2pbitInfo.pbitBitmap)
        return GOS_ERR_DISABLE;

    *pData = g_dscp2pbitInfo.pbitBitmap;

    return GOS_OK;
}

/*
    update us gem flow's pq

    if the tcont pointed by pq is not the one pointed by gem flow,
    the gem flow's pq will be re-selected from those one
    who point to the tcont that is the same as the gem flow
 */
static GOS_ERROR_CODE
omci_SetUsFlowPqByPqEntityIdTcId(UINT16 usFlowId, UINT16 pqEntityId, UINT16 tcontId)
{
    UINT16  queueId;
    UINT16  unmatchedQId;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (usFlowId >= gInfo.devCapabilities.totalGEMPortNum)
        return GOS_ERR_PARAM;

    if (pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        queueId = pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE;
        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
            return GOS_ERR_PARAM;
    }
    else
    {
        queueId = pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
            return GOS_ERR_PARAM;
    }

    g_gemUsFlowInfo[usFlowId].pqEntityId = pqEntityId;

    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    // select queue with unmatched scheduler implies that
    // only the queue priority/weight should be took into considered
    if (g_pqInfo[queueId].tcontId != tcontId)
    {
        for (unmatchedQId = 0;
                unmatchedQId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; unmatchedQId++)
        {
            if (g_pqInfo[unmatchedQId].tcontId != tcontId)
                continue;

            if (g_pqInfo[unmatchedQId].policy != g_pqInfo[queueId].policy)
                continue;

            if (g_pqInfo[unmatchedQId].pw.priority == g_pqInfo[queueId].pw.priority)
                break;
        }
        if (unmatchedQId == gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
            return GOS_FAIL;

        if (unmatchedQId < gInfo.devCapabilities.totalTContQueueNum)
            g_gemUsFlowInfo[usFlowId].pqEntityId = OMCI_MIB_US_TM_ME_ID_BASE + unmatchedQId;
        else
            g_gemUsFlowInfo[usFlowId].pqEntityId = OMCI_MIB_US_TM_ME_RSV_ID_BASE + unmatchedQId;
    }

    return GOS_OK;
}

/*
    check us gem flow's dp marking and
    configure dp flow if any empty pq is available

    return ok if dp is supported natively OR
    there is no dp marking configured on the associated pq
 */
static GOS_ERROR_CODE
omci_SetUsGemFlowDpByFlowId(UINT16 usFlowId, UINT8 isCreate)
{
    INT32               ret;
    OMCI_GEM_FLOW_ts    gemFlow;
    UINT16              dpFlowId;
    UINT16              queueId;
    UINT16              tcontId;

    // return if dp is supported natively
    if (gInfo.devCapabilities.perTContQueueDp)
        return GOS_OK;

    if (usFlowId >= gInfo.devCapabilities.totalGEMPortNum)
        return GOS_ERR_PARAM;

    if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        queueId = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    gInfo.devCapabilities.totalTContQueueNum;

        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
            return GOS_ERR_PARAM;
    }
    else
    {
        queueId = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
            return GOS_ERR_PARAM;
    }
    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s() %d flowId=%u, array queue index=%u",
        __FUNCTION__, __LINE__, usFlowId, queueId);

    if (PQ_DROP_COLOUR_NO_MARKING == g_pqInfo[queueId].dpMarking)
        return GOS_OK;

    tcontId = g_pqInfo[queueId].tcontId;
    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    if (isCreate)
    {
        dpFlowId = omci_GetAvailUsFlowId();
        if (dpFlowId == gInfo.devCapabilities.totalGEMPortNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no available us gem flow");

            return GOS_FAIL;
        }
    }
    else
    {
        dpFlowId = omci_GetUsDpFlowByDpQueueId(queueId);
        if (dpFlowId == gInfo.devCapabilities.totalGEMPortNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

            return GOS_FAIL;
        }
    }
    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s() %d array queue index=%u, it's dpQueueId=%u",
        __FUNCTION__, __LINE__, queueId, g_pqInfo[queueId].dpQueueId);

    // prepare dp gem flow
    gemFlow.flowId = dpFlowId;
    gemFlow.portId = g_gemUsFlowInfo[usFlowId].portId;
    gemFlow.tcontId = tcontId;
    gemFlow.queueId = omci_GetTcQueueIdByPqEntityId(
        OMCI_MIB_US_TM_ME_RSV_ID_BASE + (g_pqInfo[queueId].dpQueueId - \
        gInfo.devCapabilities.totalTContQueueNum));
    gemFlow.isOmcc = FALSE;
    gemFlow.isFilterMcast = FALSE;
    gemFlow.ena = isCreate;
    gemFlow.dir = PON_GEMPORT_DIRECTION_US;

    ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
    if (OMCI_ERR_OK == ret)
    {
        if (isCreate)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s() %d array queue index=%u, it's dpQueueId=%u",
                __FUNCTION__, __LINE__, queueId, g_pqInfo[queueId].dpQueueId);
            g_gemUsFlowInfo[dpFlowId].pqEntityId =
                OMCI_MIB_US_TM_ME_RSV_ID_BASE + (g_pqInfo[queueId].dpQueueId - \
                gInfo.devCapabilities.totalTContQueueNum);
            g_gemUsFlowInfo[dpFlowId].inUse  = TRUE;
            g_gemUsFlowInfo[dpFlowId].portId = gemFlow.portId;
        }
        else
        {
            g_gemUsFlowInfo[dpFlowId].inUse = FALSE;
            g_gemUsFlowInfo[dpFlowId].portId = 0;
        }

        OMCI_LOG(OMCI_LOG_LEVEL_INFO,
            "%s u/s dp gem flow [%u]", isCreate ? "create" : "delete", dpFlowId);
    }
    else
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,
            "%s u/s dp gem flow [%u] fail", isCreate ? "create" : "delete", dpFlowId);

        return GOS_FAIL;
    }

    return GOS_OK;
}
static GOS_ERROR_CODE omci_DeletePriQByQueueId(UINT16 queueId);

GOS_ERROR_CODE
omci_wrapper_updateGemFlow(OMCI_GEM_FLOW_ts data)
{
    OMCI_GEM_FLOW_ts    gemFlow;
    UINT32              dsFlowId;

    if (!data.ena)
        return GOS_OK;

    if (PON_GEMPORT_DIRECTION_DS == data.dir || PON_GEMPORT_DIRECTION_BI == data.dir)
    {
        dsFlowId = omci_GetDsFlowIdByPortId(data.portId);

        data.flowId = dsFlowId;

        memcpy(&gemFlow, &data, sizeof(OMCI_GEM_FLOW_ts));

        gemFlow.dir = PON_GEMPORT_DIRECTION_DS;

        return omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_cfgGemFlow(OMCI_GEM_FLOW_ts data)
{
    INT32               ret;
    OMCI_GEM_FLOW_ts    gemFlow;
    UINT32              usFlowId;
    UINT32              dsFlowId;
    UINT16              newTcontId;

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,
        "Create/Delete GEM port %u for direction %d", data.portId, data.dir);

    if (PON_GEMPORT_DIRECTION_US == data.dir || PON_GEMPORT_DIRECTION_BI == data.dir)
    {
        if (TRUE == data.ena)
        {
            /* create a gem u/s flow */
            usFlowId = omci_GetUsFlowIdByPortId(data.portId);
            if (usFlowId != gInfo.devCapabilities.totalGEMPortNum)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow already exist");

                return GOS_FAIL;
            }

            usFlowId = omci_GetAvailUsFlowId();
            if (usFlowId == gInfo.devCapabilities.totalGEMPortNum)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no available us gem flow");

                return GOS_FAIL;
            }

            MIB_TABLE_PRIQ_T input_priQ;
            input_priQ.EntityID = data.queueId;
            if (GOS_OK != MIB_Get(MIB_TABLE_PRIQ_INDEX, &input_priQ, sizeof(MIB_TABLE_PRIQ_T)))
                return GOS_FAIL;

            omci_wrapper_setPriQueue(&input_priQ);

            data.tcontId = omci_GetTcontIdByTcEntityId(data.tcontId);
            if (GOS_OK != omci_CreatePriQByTcontId(data.tcontId, &newTcontId))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s() create pq for tcont [%u] fail", __FUNCTION__, data.tcontId);

                return GOS_FAIL;
            }
            data.tcontId = newTcontId;

            data.flowId = usFlowId;
            // update gem flow's pq
            omci_SetUsFlowPqByPqEntityIdTcId(usFlowId, data.queueId, newTcontId);
            data.queueId =
                omci_GetTcQueueIdByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId);

            /* Create one direction one time. */
            memcpy(&gemFlow, &data, sizeof(OMCI_GEM_FLOW_ts));
            gemFlow.dir = PON_GEMPORT_DIRECTION_US;

            ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
            if (OMCI_ERR_OK == ret)
            {
                g_gemUsFlowInfo[usFlowId].inUse  = TRUE;
                g_gemUsFlowInfo[usFlowId].portId = data.portId;
                g_tcontInfo[newTcontId].gemCount++;
                if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
                {
                    // due to reserved priority queues don't upload to olt, OLT would not use these priority queues
                    // no increase used count of reserved priority queues.
                    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s() pq_entityID=%x, create u/s gem flow [%u]", __FUNCTION__,
                        g_gemUsFlowInfo[usFlowId].pqEntityId, usFlowId);
                }
                else
                {
                    g_pqInfo[g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE].usedCnt++;
                }

                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "create u/s gem flow [%u]", usFlowId);

                if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_SET_GEM_FLOW,
                    gInfo.devCapabilities.totalGEMPortNum, g_gemUsFlowInfo, &gemFlow,
                    (OMCI_IOCTL_SEND_PTR)omci_ioctl_send, (OMCI_GET_AVAIL_USFLOWID_PTR)omci_GetAvailUsFlowId))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
                }
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "create u/s gem flow [%u] fail", usFlowId);

                return GOS_FAIL;
            }

            // invoke dp flow creation...
            omci_SetUsGemFlowDpByFlowId(usFlowId, TRUE);

            if (g_tcontInfo[newTcontId].gemCount > 1)
            {
                if (GOS_OK != omci_TempDeleteUsGemFlowByTcontId(newTcontId))
                    return GOS_FAIL;

                if (GOS_OK != omci_ReCreateUsGemFlowByTcontId(newTcontId))
                    return GOS_FAIL;
            }
        }
        else
        {
            UINT16 pqInfo_index;
            /* delete a gem u/s flow */
            usFlowId = omci_GetUsFlowIdByPortId(data.portId);
            if (usFlowId == gInfo.devCapabilities.totalGEMPortNum)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

                return GOS_FAIL;
            }
            if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
                pqInfo_index = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + gInfo.devCapabilities.totalTContQueueNum;
            else
                pqInfo_index = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;

            data.flowId = usFlowId;
            data.tcontId = omci_GetTcontIdByTcEntityId(data.tcontId);
            data.queueId = g_pqInfo[pqInfo_index].tcQueueId;

            /* Delete one direction one time. */
            memcpy(&gemFlow, &data, sizeof(OMCI_GEM_FLOW_ts));
            gemFlow.dir = PON_GEMPORT_DIRECTION_US;

            // invoke dp flow deletion...
            omci_SetUsGemFlowDpByFlowId(usFlowId, FALSE);

            ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));

            if (OMCI_ERR_OK == ret)
            {
                g_gemUsFlowInfo[usFlowId].inUse = FALSE;
                g_gemUsFlowInfo[usFlowId].portId = 0;
                g_tcontInfo[data.tcontId].gemCount--;

                if (pqInfo_index < gInfo.devCapabilities.totalTContQueueNum)
                    g_pqInfo[pqInfo_index].usedCnt--;

                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "delete u/s gem flow [%d]", usFlowId);

                if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_SET_GEM_FLOW,
                    gInfo.devCapabilities.totalGEMPortNum, g_gemUsFlowInfo, &gemFlow,
                    (OMCI_IOCTL_SEND_PTR)omci_ioctl_send, (OMCI_GET_AVAIL_USFLOWID_PTR)omci_GetAvailUsFlowId))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "TBD ");
                }
                if (GOS_OK != omci_DeletePriQByTcontId(data.tcontId, pqInfo_index, g_pqInfo[pqInfo_index].usedCnt))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete pq for tcont [%u] fail", data.tcontId);
                }
                else
                {
                    if (0 == g_pqInfo[pqInfo_index].usedCnt)
                    {
                        g_pqInfo[pqInfo_index].tcontId   = OMCI_DRV_INVALID_TCONT_ID;
                        g_pqInfo[pqInfo_index].policy = PQ_POLICY_STRICT_PRIORITY;
                        g_pqInfo[pqInfo_index].pw.priority = 0;
                        g_pqInfo[pqInfo_index].dpMarking = PQ_DROP_COLOUR_NO_MARKING;
                        g_pqInfo[pqInfo_index].dpQueueId = 0;
                    }
                }
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete u/s gem flow [%u] fail", usFlowId);

                return GOS_FAIL;
            }
        }
    }

    if (PON_GEMPORT_DIRECTION_DS == data.dir || PON_GEMPORT_DIRECTION_BI == data.dir)
    {
        if (TRUE == data.ena)
        {
            /* create a gem d/s flow */
            dsFlowId = omci_GetDsFlowIdByPortId(data.portId);
            if (dsFlowId == gInfo.devCapabilities.totalGEMPortNum)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "ds gem flow not found");

                dsFlowId = omci_GetAvailDsFlowId();
                if (dsFlowId == gInfo.devCapabilities.totalGEMPortNum)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no available ds gem flow");

                    return GOS_FAIL;
                }
            }

            data.flowId = dsFlowId;

            /* Create one direction one time. */
            memcpy(&gemFlow, &data, sizeof(OMCI_GEM_FLOW_ts));
            gemFlow.dir = PON_GEMPORT_DIRECTION_DS;

            ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
            if (OMCI_ERR_OK == ret)
            {
                g_gemDsFlowInfo[dsFlowId].inUse  = TRUE;
                g_gemDsFlowInfo[dsFlowId].portId = data.portId;

                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "create d/s gem flow [%d]", dsFlowId);
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "create d/s gem flow [%d] fail", dsFlowId);

                return GOS_FAIL;
            }
        }
        else
        {
            /* delete a gem d/s flow */
            dsFlowId = omci_GetDsFlowIdByPortId(data.portId);
            if (dsFlowId == gInfo.devCapabilities.totalGEMPortNum)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "ds gem flow not found");

                return GOS_FAIL;
            }

            data.flowId = dsFlowId;

            /* Delete one direction one time. */
            memcpy(&gemFlow, &data, sizeof(OMCI_GEM_FLOW_ts));
            gemFlow.dir = PON_GEMPORT_DIRECTION_DS;

            ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
            if (OMCI_ERR_OK == ret)
            {
                g_gemDsFlowInfo[dsFlowId].inUse = FALSE;
                g_gemDsFlowInfo[dsFlowId].portId = 0;

                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "delete d/s gem flow [%u]", dsFlowId);
            }
            else
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete d/s gem flow [%u] fail", dsFlowId);

                return GOS_FAIL;
            }
        }
    }

    return GOS_OK;
}

/*
    update gem flow's pq cir/pir rate

    the gem flow has to be created before setting the rate
 */
GOS_ERROR_CODE
omci_wrapper_setGemFlowRate(OMCI_GEM_FLOW_ts data)
{
    UINT16          usFlowId;
    UINT16          dsFlowId;
    UINT16          queueId;

    if (PON_GEMPORT_DIRECTION_US == data.dir || PON_GEMPORT_DIRECTION_BI == data.dir)
    {
        usFlowId = omci_GetUsFlowIdByPortId(data.portId);
        if (gInfo.devCapabilities.totalGEMPortNum == usFlowId)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

            return GOS_FAIL;
        }

        if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
        {
            queueId = g_gemUsFlowInfo[usFlowId].pqEntityId - \
                        OMCI_MIB_US_TM_ME_RSV_ID_BASE + gInfo.devCapabilities.totalTContQueueNum;
        }
        else
            queueId = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;

        // unit: byte/s -> 8Kbps
        g_pqInfo[queueId].cir = data.cir / 1024;
        g_pqInfo[queueId].pir = data.pir / 1024;

        if (GOS_OK != omci_CreatePriQByQueueId(queueId))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "set us pq [%u] rate for gem flow [%u] fail", queueId, usFlowId);

            return GOS_FAIL;
        }

        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "set us pq [%u] cir %u pir %u for gem flow [%u]",
            queueId, g_pqInfo[queueId].cir, g_pqInfo[queueId].pir, usFlowId);
    }

    if (PON_GEMPORT_DIRECTION_DS == data.dir || PON_GEMPORT_DIRECTION_BI == data.dir)
    {
        dsFlowId = omci_GetDsFlowIdByPortId(data.portId);
        if (gInfo.devCapabilities.totalGEMPortNum == dsFlowId)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "ds gem flow not found");

            return GOS_FAIL;
        }

        // TBD, DS GEM flow rate could be achieved by ACL + shared meters
    }

    return GOS_OK;
}

/*
    re-create us gem flow by tcont

    used for re-create the gem flow that was temporary deleted before
    must be used as a pair with omci_TempDeleteUsGemFlowByTcontId()
 */
static GOS_ERROR_CODE
omci_ReCreateUsGemFlowByTcontId(UINT16 tcontId)
{
    INT32               ret;
    OMCI_GEM_FLOW_ts    gemFlow;
    UINT16              usFlowId;
    UINT16              newTcontId;

    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    if (GOS_OK != omci_CreatePriQByTcontId(tcontId, &newTcontId))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "create pq for tcont [%u] fail", tcontId);

        return GOS_FAIL;
    }

    for (usFlowId = 0; usFlowId < gInfo.devCapabilities.totalGEMPortNum; usFlowId++)
    {
        if (FALSE == g_gemUsFlowInfo[usFlowId].inUse)
            continue;

        if (omci_GetTcontIdByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId) != newTcontId)
            continue;

        // ignore flow that is belongs to dp marked queue
        if (!gInfo.devCapabilities.perTContQueueDp && PQ_DROP_COLOUR_INTERNAL_MARKING ==
                omci_GetDpMarkingByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId))
            continue;

        gemFlow.flowId = usFlowId;
        gemFlow.portId = g_gemUsFlowInfo[usFlowId].portId;
        gemFlow.tcontId = newTcontId;
        gemFlow.queueId = omci_GetTcQueueIdByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId);
        gemFlow.isOmcc = FALSE;
        gemFlow.ena = TRUE;
        gemFlow.dir = PON_GEMPORT_DIRECTION_US;

        ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
        if (OMCI_ERR_OK == ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "create u/s gem flow [%d] priority_entity_id=%x",
                usFlowId, g_gemUsFlowInfo[usFlowId].pqEntityId);

            if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO, "%s() %d:  us flow [%u],  priority_entity_id=%x",
                    __FUNCTION__, __LINE__, usFlowId, g_gemUsFlowInfo[usFlowId].pqEntityId);
            }
            else
            {
                g_tcontInfo[newTcontId].gemCount++;
                g_pqInfo[g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE].usedCnt++;
            }
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "create u/s gem flow [%u] priority_entity_id=%x fail",
                usFlowId, g_gemUsFlowInfo[usFlowId].pqEntityId);

            return GOS_FAIL;
        }

        // invoke dp flow creation...
        omci_SetUsGemFlowDpByFlowId(usFlowId, TRUE);
    }

    return GOS_OK;
}

/*
    delete us gem flow by tcont

    used to delete gem flows for re-creation later
    must be used as a pair with omci_ReCreateUsGemFlowByTcontId()
 */
static GOS_ERROR_CODE
omci_TempDeleteUsGemFlowByTcontId(UINT16 tcontId)
{
    INT32               ret;
    OMCI_GEM_FLOW_ts    gemFlow;
    UINT16              usFlowId;

    if (tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    if (0 == g_tcontInfo[tcontId].gemCount)
        return GOS_OK;

    for (usFlowId = 0; usFlowId < gInfo.devCapabilities.totalGEMPortNum; usFlowId++)
    {
        if (FALSE == g_gemUsFlowInfo[usFlowId].inUse)
            continue;

        if (omci_GetTcontIdByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId) != tcontId)
            continue;

        // ignore flow that is belongs to dp marked queue
        if (!gInfo.devCapabilities.perTContQueueDp && PQ_DROP_COLOUR_INTERNAL_MARKING ==
                omci_GetDpMarkingByPqEntityId(g_gemUsFlowInfo[usFlowId].pqEntityId))
            continue;

        // invoke dp flow deletion...
        omci_SetUsGemFlowDpByFlowId(usFlowId, FALSE);

        gemFlow.flowId = usFlowId;
        gemFlow.ena = FALSE;
        gemFlow.dir = PON_GEMPORT_DIRECTION_US;

        ret = omci_ioctl_send(OMCI_IOCTL_GEMPORT_SET, &gemFlow, sizeof(OMCI_GEM_FLOW_ts));
        if (OMCI_ERR_OK == ret)
        {
            UINT16 pqInfo_index;
            if (g_gemUsFlowInfo[usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
            {
                pqInfo_index = g_gemUsFlowInfo[usFlowId].pqEntityId - \
                    OMCI_MIB_US_TM_ME_RSV_ID_BASE + gInfo.devCapabilities.totalTContQueueNum;
            }
            else
            {
                pqInfo_index = g_gemUsFlowInfo[usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;

                g_tcontInfo[tcontId].gemCount--;

                g_pqInfo[pqInfo_index].usedCnt--;

            }

            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "delete u/s gem flow [%d]", usFlowId);


            if (GOS_OK != omci_DeletePriQByTcontId(tcontId, pqInfo_index, g_pqInfo[pqInfo_index].usedCnt))
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete pq for tcont [%u] fail", tcontId);
            }
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "delete u/s gem flow [%u] fail", usFlowId);

            return GOS_FAIL;
        }
    }

    return GOS_OK;
}

/*
    update us connection's dp flow id
    if the connection connects to pq that is dp marking configured

    the dp flow id will remain the same as normal flow id
    if dp is supported natively OR there is no dp marking configured
 */
static GOS_ERROR_CODE
omci_SetUsConnDpByBrRule(OMCI_BRIDGE_RULE_ts *pBrRule)
{
    UINT16  queueId;
    UINT16  dpFlowId;

    if (pBrRule->usFlowId >= gInfo.devCapabilities.totalGEMPortNum)
        return GOS_ERR_PARAM;

    pBrRule->usDpFlowId = pBrRule->usFlowId;

    // return if dp is supported natively
    if (gInfo.devCapabilities.perTContQueueDp)
        return GOS_OK;

    // modify classify rules if dp marking is enabled
    if (PQ_DROP_COLOUR_NO_MARKING != (pBrRule->usDpMarking =
            omci_GetDpMarkingByPqEntityId(g_gemUsFlowInfo[pBrRule->usFlowId].pqEntityId)))
    {
        if (g_gemUsFlowInfo[pBrRule->usFlowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
        {
            queueId = g_gemUsFlowInfo[pBrRule->usFlowId].pqEntityId - \
                        OMCI_MIB_US_TM_ME_RSV_ID_BASE + gInfo.devCapabilities.totalTContQueueNum;
        }
        else
        {
            queueId = g_gemUsFlowInfo[pBrRule->usFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
        }


        dpFlowId = omci_GetUsDpFlowByDpQueueId(queueId);
        if (dpFlowId == gInfo.devCapabilities.totalGEMPortNum)
            return GOS_FAIL;

        pBrRule->usDpFlowId = dpFlowId;
    }

    return GOS_OK;
}

/*
    del/add all us connection according to specific pq
 */
static GOS_ERROR_CODE
omci_ReSetUsConnDpByPqEntityId(UINT16 pqEntityId)
{
    INT32                   ret;
    OMCI_BRIDGE_RULE_ts     *pBrRule;
    UINT32                  srvId;

    for (srvId = 0; srvId < OMCI_MAX_SERV_ID; srvId++)
    {
        pBrRule = &serviceFlow[srvId];
        if (!pBrRule || !pBrRule->isUsed)
            continue;

        if (pBrRule->usFlowId >= gInfo.devCapabilities.totalGEMPortNum)
            continue;

        if (g_gemUsFlowInfo[pBrRule->usFlowId].pqEntityId != pqEntityId)
            continue;

        // deactivate flow with associated pq
        ret = omci_ioctl_send(OMCI_IOCTL_CF_DEL, &srvId, sizeof(srvId));
        if (OMCI_ERR_OK == ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "del us dp conn [%u]", srvId);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del us dp conn [%u] fail", srvId);

            continue;
        }

        // modify dp flow info
        omci_SetUsConnDpByBrRule(pBrRule);

        // reactivate flow with associated pq
        ret = omci_ioctl_send(OMCI_IOCTL_CF_ADD, pBrRule, sizeof(OMCI_BRIDGE_RULE_ts));
        if (OMCI_ERR_OK == ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "add us dp conn [%u]", srvId);
        }
        else
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "add us dp conn [%u] fail", srvId);

            continue;
        }
    }

    return GOS_OK;
}

/*
    update us pq's dp marking

    when the action is create,
    search if there is any empty queue available for dp usage

    when the action is delete,
    release the allocated dp queue back to the pool

    return ok if dp is supported natively OR
    there is no dp marking configured on the pq
 */
static GOS_ERROR_CODE
omci_SetUsPriQDpByQueueId(UINT16 queueId, UINT8 dpMarking)
{
    UINT16  dpQueueId;
    UINT8   isCreate;
    UINT16  gemCount;

    // return if dp is supported natively
    if (gInfo.devCapabilities.perTContQueueDp)
        return GOS_OK;

    if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
        return GOS_ERR_PARAM;

    if (g_pqInfo[queueId].tcontId >= gInfo.devCapabilities.totalTContNum)
        return GOS_ERR_PARAM;

    // not support WRR queues for drop precedence marking
    if (g_pqInfo[queueId].policy != PQ_POLICY_STRICT_PRIORITY)
        return GOS_ERR_NOTSUPPORT;

    // reject if drop precedence marking is already activated
    if (g_pqInfo[queueId].dpMarking != PQ_DROP_COLOUR_NO_MARKING &&
            dpMarking != PQ_DROP_COLOUR_NO_MARKING)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "modify pq dp marking on the fly is not supported");

        return GOS_ERR_NOTSUPPORT;
    }

    // only support DEI or PCP drop precedence marking
    if (PQ_DROP_COLOUR_NO_MARKING != dpMarking &&
            PQ_DROP_COLOUR_DEI_MARKING != dpMarking &&
            PQ_DROP_COLOUR_PCP_8P0D_MARKING != dpMarking &&
            PQ_DROP_COLOUR_PCP_7P1D_MARKING != dpMarking &&
            PQ_DROP_COLOUR_PCP_6P2D_MARKING != dpMarking &&
            PQ_DROP_COLOUR_PCP_5P3D_MARKING != dpMarking)
        return GOS_ERR_NOTSUPPORT;

    if (PQ_DROP_COLOUR_NO_MARKING == g_pqInfo[queueId].dpMarking)
    {
        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
        // find a queue which is never assigned to any t-cont
        for (dpQueueId = gInfo.devCapabilities.totalTContQueueNum;
            dpQueueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; dpQueueId++)
        {
            if (OMCI_DRV_INVALID_TCONT_ID == g_pqInfo[dpQueueId].tcontId)
                break;
        }
        // no queues available for drop precedence marking
        if (dpQueueId == gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no pq available for drop precedence marking");

            return GOS_ERR_NOTSUPPORT;
        }

        isCreate = TRUE;
    }
    else
        isCreate = FALSE;

    gemCount = g_tcontInfo[g_pqInfo[queueId].tcontId].gemCount;

    // delete gem flows
    if (gemCount != 0 &&
            GOS_OK != omci_TempDeleteUsGemFlowByTcontId(g_pqInfo[queueId].tcontId))
        return GOS_FAIL;

    if (isCreate)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "attach dp pq %u to queue %u", dpQueueId, queueId);

        g_pqInfo[dpQueueId].tcontId = g_pqInfo[queueId].tcontId;
        g_pqInfo[dpQueueId].policy = PQ_POLICY_STRICT_PRIORITY;
        // OMCI takes only 0 (highest) ~ 0xFFF, make it smallest
        g_pqInfo[dpQueueId].pw.priority = 0x1000;
        // use internal marking to mark this queue as supplement queue
        g_pqInfo[dpQueueId].dpMarking = PQ_DROP_COLOUR_INTERNAL_MARKING;

        g_pqInfo[queueId].dpMarking = dpMarking;
        g_pqInfo[queueId].dpQueueId = dpQueueId;
    }
    else
    {
        dpQueueId = g_pqInfo[queueId].dpQueueId;

        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "detach dp pq %u from queue %u", dpQueueId, queueId);

        g_pqInfo[dpQueueId].tcontId = OMCI_DRV_INVALID_TCONT_ID;
        g_pqInfo[dpQueueId].tcQueueId = OMCI_DRV_INVALID_QUEUE_ID;
        g_pqInfo[dpQueueId].pw.priority = 0;
        g_pqInfo[dpQueueId].dpMarking = PQ_DROP_COLOUR_NO_MARKING;

        g_pqInfo[queueId].dpMarking = PQ_DROP_COLOUR_NO_MARKING;
        g_pqInfo[queueId].dpQueueId = 0;
    }

    // recreate pq & gem flows
    if (gemCount != 0 &&
            GOS_OK != omci_ReCreateUsGemFlowByTcontId(g_pqInfo[queueId].tcontId))
        return GOS_FAIL;

    // recreate conn
    if (gemCount != 0)
        return omci_ReSetUsConnDpByPqEntityId(OMCI_MIB_US_TM_ME_ID_BASE + queueId);

    return GOS_OK;
}

/*
    set ds pq

    dp marking is the only meaningful parameter
    scheduling policy is ignored and fix to strict priority
    i.e., the port and priority is not permitted for modification
 */
static GOS_ERROR_CODE
omci_SetDsPriQByMib(MIB_TABLE_PRIQ_T *pPriQ)
{
    INT32           ret;
    OMCI_PRIQ_ts    priQ;
    UINT16          portId;

    if (!pPriQ)
        return GOS_ERR_PARAM;

    memset(&priQ, 0, sizeof(OMCI_PRIQ_ts));

    if (pPriQ->Weight <= 1)
    {
        priQ.scheduleType = PQ_POLICY_STRICT_PRIORITY;
        priQ.weight = 1;
    }
    else
    {
        priQ.scheduleType = PQ_POLICY_WEIGHTED_ROUND_ROBIN;
        priQ.weight = pPriQ->Weight;
    }

    // TBD, should be taken from traffic descriptor
    priQ.cir = 0;
    priQ.pir = 0;

    priQ.queueId = pPriQ->RelatedPort & 0xFFFF;
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_ERR, (GOS_OK != pptp_eth_uni_me_id_to_switch_port(pPriQ->RelatedPort >> 16, &portId)), GOS_FAIL);
    priQ.tcontId = portId;

    priQ.dpMarking = pPriQ->DropPColorMarking;
    priQ.dir = PON_GEMPORT_DIRECTION_DS;

    ret = omci_ioctl_send(OMCI_IOCTL_PRIQ_SET, &priQ, sizeof(OMCI_PRIQ_ts));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set ds queue [%#x] failed!", pPriQ->EntityID);

        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "set ds queue [%#x]", pPriQ->EntityID);

    return GOS_OK;
}

static UINT32 omci_IsWrrPolicy(MIB_TABLE_PRIQ_T *pPriQ, UINT16 *pEntityID)
{
    MIB_TABLE_TCONT_T       mibTC;
    MIB_TABLE_SCHEDULER_T   mibTS;

    *pEntityID = 0xFFFF;

    if (!pPriQ)
        return FALSE;

    if (pPriQ->SchedulerPtr >= OMCI_MIB_US_TM_ME_ID_BASE)
    {
        // traffic scheduler ME

        mibTS.EntityID = pPriQ->SchedulerPtr;
        if (GOS_OK != MIB_Get(MIB_TABLE_SCHEDULER_INDEX, &mibTS, sizeof(MIB_TABLE_SCHEDULER_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "%s(): traffic scheduler 0x%x is not found", __FUNCTION__, mibTS.EntityID);

            return FALSE;
        }
        *pEntityID = mibTS.EntityID;
        return (TCONT_POLICY_WEIGHTED_ROUND_ROBIN == mibTS.Policy ? TRUE : FALSE);
    }
    else
    {
        // T-CONT ME

        mibTC.EntityID = (pPriQ->RelatedPort >> 16) & 0xFFFF;
        if (GOS_OK != MIB_Get(MIB_TABLE_TCONT_INDEX, &mibTC, sizeof(MIB_TABLE_TCONT_T)))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                "%s(): t-cont 0x%x is not found", __FUNCTION__, mibTC.EntityID);

            return FALSE;
        }
        *pEntityID = mibTC.EntityID;
        return (TCONT_POLICY_WEIGHTED_ROUND_ROBIN == mibTC.Policy ? TRUE : FALSE);
    }
}


GOS_ERROR_CODE
omci_wrapper_setPriQueue(MIB_TABLE_PRIQ_T *pPriQ)
{
    GOS_ERROR_CODE  ret = GOS_OK;
    UINT16          queueId;
    UINT16          oldTcontId;
    UINT16          newTcontId;
    UINT16          oldGemCount;
    UINT16          newGemCount;
    UINT16          tcEntityId;

    if (pPriQ->EntityID < OMCI_MIB_US_TM_ME_ID_BASE)
        return omci_SetDsPriQByMib(pPriQ);

    queueId = pPriQ->EntityID - OMCI_MIB_US_TM_ME_ID_BASE;

    // PQ is set before the PQ is REALLY created
    if (OMCI_DRV_INVALID_QUEUE_ID == g_pqInfo[queueId].tcQueueId)
    {
        // WRR
        if (omci_IsWrrPolicy(pPriQ, &tcEntityId))
        {
            if (pPriQ->DropPColorMarking != g_pqInfo[queueId].dpMarking)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "pq dp marking cannot be set under wrr queues");

                return GOS_ERR_NOTSUPPORT;
            }

            newTcontId = omci_GetTcontIdByTcEntityId(tcEntityId);

            g_pqInfo[queueId].policy = PQ_POLICY_WEIGHTED_ROUND_ROBIN;
            g_pqInfo[queueId].pw.weight = pPriQ->Weight;

            g_pqInfo[queueId].tcontId = newTcontId;
        }
        // SP
        else
        {
            if (0xFFFF == tcEntityId)
                return GOS_ERR_NOTSUPPORT;

            newTcontId = omci_GetTcontIdByTcEntityId(tcEntityId);

            g_pqInfo[queueId].policy = PQ_POLICY_STRICT_PRIORITY;
            g_pqInfo[queueId].pw.priority = (pPriQ->RelatedPort & 0xFFFF);

            g_pqInfo[queueId].tcontId = newTcontId;

        }
    }
    // PQ is set after the PQ is created
    else
    {
        // WRR
        if (omci_IsWrrPolicy(pPriQ, &tcEntityId))
        {
            if (pPriQ->DropPColorMarking != g_pqInfo[queueId].dpMarking)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                    "pq dp marking cannot be set under wrr queues");

                return GOS_ERR_NOTSUPPORT;
            }

            newTcontId = omci_GetTcontIdByTcEntityId(tcEntityId);
            newGemCount = g_tcontInfo[newTcontId].gemCount;

            // same policy within same scheduler
            if (PQ_POLICY_WEIGHTED_ROUND_ROBIN == g_pqInfo[queueId].policy &&
                    newTcontId == g_pqInfo[queueId].tcontId)
            {
                // just re-assign the weight
                if (pPriQ->Weight != g_pqInfo[queueId].pw.weight)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                        "wrr queue %u change weight to %u", queueId, pPriQ->Weight);

                    g_pqInfo[queueId].pw.weight = pPriQ->Weight;

                    ret = omci_CreatePriQByQueueId(queueId);
                }
            }
            else
            {
                oldTcontId = g_pqInfo[queueId].tcontId;
                oldGemCount = g_tcontInfo[oldTcontId].gemCount;

                // move a in-use queue to never created t-cont is not possible
                if (newTcontId != oldTcontId &&
                        TCONT_ALLOC_ID_984_RESERVED == g_tcontInfo[newTcontId].allocId)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "cannot move in-use queue %u to unused t-cont %u", queueId, newTcontId);

                    return GOS_ERR_NOTSUPPORT;
                }

                if (g_pqInfo[queueId].policy != PQ_POLICY_WEIGHTED_ROUND_ROBIN)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                        "sp queue %u change policy to wrr", queueId);
                }
                if (newTcontId != oldTcontId)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                        "queue %u change scheduler from %u to %u", queueId, oldTcontId, newTcontId);
                }

                // policy change OR scheduler change
                if (oldGemCount != 0 &&
                        GOS_OK != omci_TempDeleteUsGemFlowByTcontId(oldTcontId))
                    return GOS_FAIL;

                if (newTcontId != oldTcontId && newGemCount != 0 &&
                        GOS_OK != omci_TempDeleteUsGemFlowByTcontId(newTcontId))
                    return GOS_FAIL;

                g_pqInfo[queueId].policy = PQ_POLICY_WEIGHTED_ROUND_ROBIN;
                g_pqInfo[queueId].pw.weight = pPriQ->Weight;
                g_pqInfo[queueId].tcontId = newTcontId;

                if (oldGemCount != 0 &&
                        GOS_OK != (ret = omci_ReCreateUsGemFlowByTcontId(oldTcontId)))
                    return GOS_FAIL;

                if (newTcontId != oldTcontId && newGemCount != 0 &&
                        GOS_OK != (ret = omci_ReCreateUsGemFlowByTcontId(newTcontId)))
                    return GOS_FAIL;
            }
        }
        // SP
        else
        {
            if (0xFFFF == tcEntityId)
                return GOS_ERR_NOTSUPPORT;

            newTcontId = omci_GetTcontIdByTcEntityId(tcEntityId);
            if (newTcontId == OMCI_DRV_INVALID_TCONT_ID)
            {
                return GOS_ERR_NOT_FOUND;
            }
            oldTcontId = g_pqInfo[queueId].tcontId;
            newGemCount = g_tcontInfo[newTcontId].gemCount;
            oldGemCount = g_tcontInfo[oldTcontId].gemCount;

            // policy change OR priority change OR t-cont change
            if (PQ_POLICY_STRICT_PRIORITY != g_pqInfo[queueId].policy ||
                    (pPriQ->RelatedPort & 0xFFFF) != g_pqInfo[queueId].pw.priority ||
                    newTcontId != oldTcontId)
            {
                if (pPriQ->DropPColorMarking != g_pqInfo[queueId].dpMarking)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "pq dp marking cannot be set together with other modification");

                    return GOS_ERR_NOTSUPPORT;
                }

                // move a in-use queue to never created t-cont is not possible
                if (newTcontId != oldTcontId &&
                        TCONT_ALLOC_ID_984_RESERVED == g_tcontInfo[newTcontId].allocId)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "cannot move in-use queue %u to unused t-cont %u", queueId, newTcontId);

                    return GOS_ERR_NOTSUPPORT;
                }

                if (PQ_POLICY_STRICT_PRIORITY != g_pqInfo[queueId].policy)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN,
                        "wrr queue %u change policy to sp", queueId);
                }
                else
                {
                    if ((pPriQ->RelatedPort & 0xFFFF) != g_pqInfo[queueId].pw.priority)
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "sp queue %u change priority to %u",
                            queueId, (pPriQ->RelatedPort & 0xFFFF));
                    }
                }
                if (newTcontId != oldTcontId)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_WARN, "queue %u change t-cont from %u to %u",
                        queueId, oldTcontId, newTcontId);
                }

                if (oldGemCount != 0 &&
                        GOS_OK != omci_TempDeleteUsGemFlowByTcontId(oldTcontId))
                    return GOS_FAIL;

                if (newTcontId != oldTcontId && newGemCount != 0 &&
                        GOS_OK != omci_TempDeleteUsGemFlowByTcontId(newTcontId))
                    return GOS_FAIL;

                g_pqInfo[queueId].policy = PQ_POLICY_STRICT_PRIORITY;
                g_pqInfo[queueId].pw.priority = (pPriQ->RelatedPort & 0xFFFF);
                g_pqInfo[queueId].tcontId = newTcontId;

                if (oldGemCount != 0 &&
                        GOS_OK != (ret = omci_ReCreateUsGemFlowByTcontId(oldTcontId)))
                    return GOS_FAIL;

                if (newTcontId != oldTcontId && newGemCount != 0 &&
                        GOS_OK != (ret = omci_ReCreateUsGemFlowByTcontId(newTcontId)))
                    return GOS_FAIL;
            }
        }
    }

    if (g_pqInfo[queueId].dpMarking != pPriQ->DropPColorMarking && GOS_OK == ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN, "modify pq dp marking is detected");

        ret = omci_SetUsPriQDpByQueueId(queueId, pPriQ->DropPColorMarking);
    }

    return ret;
}

GOS_ERROR_CODE
omci_wrapper_setDsBcGemFlow(unsigned int flowId)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_DS_BC_GEM_FLOW_SET, &flowId, sizeof(flowId));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setForceEmergencyStop(unsigned int state)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_FORCE_EMERGENCY_STOP_SET, &state, sizeof(state));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

// uni info and control
GOS_ERROR_CODE
omci_wrapper_getPortLinkStatus(omci_port_link_status_t *pStatus)
{
    INT32   ret;

    if (!pStatus)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_LINK_STATUS_GET, pStatus, sizeof(omci_port_link_status_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPortSpeedDuplexStatus(omci_port_speed_duplex_status_t *pStatus)
{
    INT32   ret;

    if (!pStatus)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_SPEED_DUPLEX_STATUS_GET, pStatus, sizeof(omci_port_speed_duplex_status_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPortAutoNegoAbility(omci_port_auto_nego_ability_t *pAbility)
{
    INT32   ret;

    if (!pAbility)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_AUTO_NEGO_ABILITY_SET, pAbility, sizeof(omci_port_auto_nego_ability_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPortAutoNegoAbility(omci_port_auto_nego_ability_t *pAbility)
{
    INT32   ret;

    if (!pAbility)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_AUTO_NEGO_ABILITY_GET, pAbility, sizeof(omci_port_auto_nego_ability_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPortState(omci_port_state_t *pState)
{
    INT32   ret;

    if (!pState)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_STATE_SET, pState, sizeof(omci_port_state_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPortState(omci_port_state_t *pState)
{
    INT32   ret;

    if (!pState)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_STATE_GET, pState, sizeof(omci_port_state_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setMaxFrameSize(omci_port_max_frame_size_t *pSize)
{
    INT32   ret;

    if (!pSize)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_MAX_FRAME_SIZE_SET, pSize, sizeof(omci_port_max_frame_size_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getMaxFrameSize(omci_port_max_frame_size_t *pSize)
{
    INT32   ret;

    if (!pSize)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_MAX_FRAME_SIZE_GET, pSize, sizeof(omci_port_max_frame_size_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPhyLoopback(omci_port_loopback_t *pLoopback)
{
    INT32   ret;

    if (!pLoopback)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PHY_LOOPBACK_SET, pLoopback, sizeof(omci_port_loopback_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPhyLoopback(omci_port_loopback_t *pLoopback)
{
    INT32   ret;

    if (!pLoopback)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PHY_LOOPBACK_GET, pLoopback, sizeof(omci_port_loopback_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPhyPwrDown(omci_port_pwr_down_t *pPwrDown)
{
    INT32   ret;

    if (!pPwrDown)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PHY_PWR_DOWN_SET, pPwrDown, sizeof(omci_port_pwr_down_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPhyPwrDown(omci_port_pwr_down_t *pPwrDown)
{
    INT32   ret;

    if (!pPwrDown)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PHY_PWR_DOWN_GET, pPwrDown, sizeof(omci_port_pwr_down_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

// statistics
GOS_ERROR_CODE
omci_wrapper_getPortStat(omci_port_stat_t *pStat)
{
    INT32   ret;

    if (!pStat)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_PORT_GET, pStat, sizeof(omci_port_stat_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_resetPortStat(unsigned int port)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_PORT_CLEAR, &port, sizeof(unsigned int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getUsFlowStat(omci_flow_stat_t *pStat)
{
    INT32   ret;

    if (!pStat)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_US_FLOW_GET, pStat, sizeof(omci_flow_stat_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_resetUsFlowStat(unsigned int flow)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_US_FLOW_CLEAR, &flow, sizeof(unsigned int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getDsFlowStat(omci_flow_stat_t *pStat)
{
    INT32   ret;

    if (!pStat)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_DS_FLOW_GET, pStat, sizeof(omci_flow_stat_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_resetDsFlowStat(unsigned int flow)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_DS_FLOW_CLEAR, &flow, sizeof(unsigned int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getDsFecStat(omci_ds_fec_stat_t *pStat)
{
    INT32   ret;

    if (!pStat)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_DS_FEC_GET, pStat, sizeof(omci_ds_fec_stat_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_resetDsFecStat(void)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_CNTR_DS_FEC_CLEAR, NULL, 0);
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setLoidAuthStatus(UINT8 authStatus)
{
    INT32               ret;
    omci_event_msg_t    omciEventMsg;

    memset(&omciEventMsg, 0, sizeof(omci_event_msg_t));
    omciEventMsg.subType = OMCI_EVENT_TYPE_LOID_AUTHENTICATION;
    omciEventMsg.status = authStatus;

    ret = omci_ioctl_send(OMCI_IOCTL_LOID_AUTH_STATUS_SET, &omciEventMsg, sizeof(omci_event_msg_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setUniQos(omci_uni_qos_info_t *pUniQos)
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_UNI_QOS_SET, pUniQos, sizeof(omci_uni_qos_info_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;

}


GOS_ERROR_CODE
omci_FindMcastByPort(UINT32                 op,
                    UINT16                  id,
                    int                     phyPort,
                    OMCI_BRIDGE_RULE_ts     *pBridgeRule)
{
    MIB_TABLE_MCASTSUBCONFINFO_T mcastConfInfo, *pMibMcastSubConfInfo = NULL;
    MIB_TABLE_MACBRIPORTCFGDATA_T *pBridgePort = NULL;
    MIB_TABLE_MCASTOPERPROF_T *pMibMop = NULL, mop;
    MIB_TREE_T *pTree = NULL;
    MIB_NODE_T *pNode = NULL;
    MIB_TREE_NODE_ENTRY_T *pDataEntry = NULL;
    MIB_ENTRY_T *pEntry = NULL;
    mopTableEntry_t *ptr = NULL;

    pTree = MIB_AvlTreeSearchByKey(NULL, id, AVL_KEY_PPTPUNI);
    if(pTree && (NULL != (pNode = MIB_AvlTreeSearch(pTree->root, AVL_KEY_MACBRIPORT_UNI))))
    {
        LIST_FOREACH(pDataEntry,&pNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            pEntry = pDataEntry->mibEntry;
            pBridgePort = (MIB_TABLE_MACBRIPORTCFGDATA_T *)pEntry->pData;

            if(pBridgePort->TPPointer == id && MBPCD_TP_TYPE_PPTP_ETH_UNI == pBridgePort->TPType)
            {
                mcastConfInfo.EntityId = pBridgePort->EntityID;
                if(FALSE == mib_FindEntry(MIB_TABLE_MCASTSUBCONFINFO_INDEX, &mcastConfInfo, &pMibMcastSubConfInfo))
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "can't find mcast subconfinfo 0x%x", mcastConfInfo.EntityId);
                    return GOS_FAIL;
                }
                LIST_FOREACH(ptr, &pMibMcastSubConfInfo->MOPhead, entries)
                {
                    mop.EntityId = ptr->tableEntry.mcastOperProfPtr;
                    OMCI_ERR_CHK(OMCI_LOG_LEVEL_WARN, (!mib_FindEntry(MIB_TABLE_MCASTOPERPROF_INDEX, &mop, &pMibMop)), GOS_FAIL);

                    MCAST_WRAPPER(omci_connection_rule_set, op, phyPort, pMibMop, pBridgeRule);
                    OMCI_LOG(OMCI_LOG_LEVEL_DBG, "%s() op=%u, uniMask =%u, servId=%u, UTP port=%u, PPTPID=%x, subInfoId=%x", __FUNCTION__, op,
                            pBridgeRule->uniMask, pBridgeRule->servId, phyPort, id, mcastConfInfo.EntityId);
                }
            }
        }
    }
    return GOS_OK;
}

void
omci_UpdateMcastInfo(UINT32                 op,
                    OMCI_BRIDGE_RULE_ts     *pBridgeRule)
{
    int i = 0, bitVal;
    UINT16 id;

    if((PON_GEMPORT_DIRECTION_BI == pBridgeRule->dir)
        && (!(pBridgeRule->uniMask & (1 << gInfo.devCapabilities.ponPort))))
    {
        bitVal = pBridgeRule->uniMask;
        while(bitVal)
        {
            if((1 << i) & pBridgeRule->uniMask)
            {
                //TBD: VEIP slot
                if(GOS_OK == pptp_eth_uni_switch_port_to_me_id(i, &id))
                {
                    if(GOS_OK != omci_FindMcastByPort(op, id, i, pBridgeRule))
                    {
                        OMCI_LOG(OMCI_LOG_LEVEL_DBG, "omci_FindMcastByPort FAIL");
                    }
                }
            }
            bitVal &= ~(1 << i);
            i++;
        }
    }
}

static BOOL
omci_isIgnoreTpid(CHAR                      type,
                  OMCI_BRIDGE_RULE_ts       *bridgeRule1,
                  OMCI_BRIDGE_RULE_ts       *bridgeRule2)
{
    switch (type)
    {
        case 'C':
            if ((VLAN_FILTER_NO_TAG & bridgeRule1->vlanRule.filterRule.filterCtagMode) &&
                (VLAN_FILTER_NO_TAG & bridgeRule1->vlanRule.filterRule.filterStagMode) &&
                (VLAN_ACT_ADD == bridgeRule1->vlanRule.cTagAct.vlanAct) &&
                (EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_INNER == bridgeRule1->vlanRule.cTagAct.assignVlan.tpid) &&
                (EVTOCD_TBL_TREATMENT_TPID_8100 == bridgeRule2->vlanRule.cTagAct.assignVlan.tpid))
                return TRUE;
            break;
        case 'S':
            //TBD
            break;
        default:
            // outStyle
            if ((VLAN_FILTER_NO_TAG & bridgeRule1->vlanRule.filterRule.filterCtagMode) &&
                (VLAN_FILTER_NO_TAG & bridgeRule1->vlanRule.filterRule.filterStagMode) &&
                (VLAN_ACT_ADD == bridgeRule1->vlanRule.cTagAct.vlanAct) &&
                (EVTOCD_TBL_TREATMENT_TPID_COPY_FROM_INNER == bridgeRule1->vlanRule.outStyle.outVlan.tpid) &&
                (EVTOCD_TBL_TREATMENT_TPID_8100 == bridgeRule2->vlanRule.outStyle.outVlan.tpid))
                return TRUE;
    }
    return FALSE;
}

static BOOL
omci_isLikeBridgeRuleSameExcludeUni(OMCI_BRIDGE_RULE_ts *pBR1, OMCI_BRIDGE_RULE_ts *pBR2)
{
    if (pBR1->usFlowId == pBR2->usFlowId &&
        pBR1->dsFlowId == pBR2->dsFlowId &&
        pBR1->vlanRule.filterMode != pBR2->vlanRule.filterMode &&
        ( (((VLAN_FILTER_NO_TAG & pBR1->vlanRule.filterRule.filterStagMode) &&
            (VLAN_OPER_MODE_EXTVLAN == pBR1->vlanRule.filterMode)) &&
           ((VLAN_FILTER_NO_CARE_TAG & pBR2->vlanRule.filterRule.filterStagMode) &&
            (VLAN_OPER_MODE_FILTER_SINGLETAG == pBR2->vlanRule.filterMode)) ) ||
          (((VLAN_FILTER_NO_TAG & pBR2->vlanRule.filterRule.filterStagMode) &&
            (VLAN_OPER_MODE_EXTVLAN == pBR2->vlanRule.filterMode)) &&
           ((VLAN_FILTER_NO_CARE_TAG & pBR1->vlanRule.filterRule.filterStagMode) &&
            (VLAN_OPER_MODE_FILTER_SINGLETAG == pBR1->vlanRule.filterMode)) )
        ) &&
        ((VLAN_FILTER_VID & pBR1->vlanRule.filterRule.filterCtagMode) &&
         (VLAN_FILTER_VID & pBR2->vlanRule.filterRule.filterCtagMode) &&
         !(VLAN_FILTER_PRI & pBR1->vlanRule.filterRule.filterCtagMode) &&
         !(VLAN_FILTER_PRI & pBR2->vlanRule.filterRule.filterCtagMode)) &&
        (pBR1->vlanRule.filterRule.etherType == pBR2->vlanRule.filterRule.etherType) &&
        ( (VLAN_ACT_TRANSPARENT == pBR1->vlanRule.sTagAct.vlanAct ||
           VLAN_ACT_NON == pBR1->vlanRule.sTagAct.vlanAct) &&
          (VLAN_ACT_TRANSPARENT == pBR2->vlanRule.sTagAct.vlanAct ||
           VLAN_ACT_NON == pBR2->vlanRule.sTagAct.vlanAct) ) &&
        ( (VLAN_ACT_TRANSPARENT == pBR1->vlanRule.cTagAct.vlanAct ||
           VLAN_ACT_NON == pBR1->vlanRule.cTagAct.vlanAct) &&
          (VLAN_ACT_TRANSPARENT == pBR2->vlanRule.cTagAct.vlanAct ||
           VLAN_ACT_NON == pBR2->vlanRule.cTagAct.vlanAct) ) )
    {
        return TRUE;
    }

    if (pBR1->usFlowId == pBR2->usFlowId &&
        pBR1->dsFlowId == pBR2->dsFlowId &&
        ((VLAN_OPER_MODE_FORWARD_UNTAG == pBR1->vlanRule.filterMode &&
         VLAN_OPER_MODE_EXTVLAN == pBR2->vlanRule.filterMode &&
         (VLAN_FILTER_NO_TAG & pBR2->vlanRule.filterRule.filterStagMode) &&
         (VLAN_FILTER_NO_TAG & pBR2->vlanRule.filterRule.filterCtagMode) &&
         (VLAN_ACT_TRANSPARENT == pBR2->vlanRule.sTagAct.vlanAct ||
           VLAN_ACT_NON == pBR2->vlanRule.sTagAct.vlanAct) &&
         (VLAN_ACT_TRANSPARENT == pBR2->vlanRule.cTagAct.vlanAct ||
           VLAN_ACT_NON == pBR2->vlanRule.cTagAct.vlanAct) ) ||
        (VLAN_OPER_MODE_FORWARD_UNTAG == pBR2->vlanRule.filterMode &&
         VLAN_OPER_MODE_EXTVLAN == pBR1->vlanRule.filterMode &&
         (VLAN_FILTER_NO_TAG & pBR1->vlanRule.filterRule.filterStagMode) &&
         (VLAN_FILTER_NO_TAG & pBR1->vlanRule.filterRule.filterCtagMode) &&
         (VLAN_ACT_TRANSPARENT == pBR1->vlanRule.sTagAct.vlanAct ||
           VLAN_ACT_NON == pBR1->vlanRule.sTagAct.vlanAct) &&
         (VLAN_ACT_TRANSPARENT == pBR1->vlanRule.cTagAct.vlanAct ||
           VLAN_ACT_NON == pBR1->vlanRule.cTagAct.vlanAct))))
    {
       return TRUE;

    }
    return FALSE;
}

static BOOL
isLikeVlanAct(OMCI_VLAN_ACT_ts act1, OMCI_VLAN_ACT_ts act2)
{
    if (act1.vlanAct != act2.vlanAct)
    {
        if ( (VLAN_ACT_TRANSPARENT == act1.vlanAct || VLAN_ACT_NON == act1.vlanAct) &&
             (VLAN_ACT_TRANSPARENT == act2.vlanAct || VLAN_ACT_NON == act2.vlanAct) )
        {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL
omci_isBridgeRuleSameExcludeUni(OMCI_BRIDGE_RULE_ts     *bridgeRule1,
                                OMCI_BRIDGE_RULE_ts     *bridgeRule2)
{
    if (omci_isLikeBridgeRuleSameExcludeUni(bridgeRule1, bridgeRule2)) return TRUE;

    if((bridgeRule1->usFlowId != bridgeRule2->usFlowId)
        || (bridgeRule1->dsFlowId != bridgeRule2->dsFlowId)
        || (bridgeRule1->dir != bridgeRule2->dir)
        || (bridgeRule1->vlanRule.filterMode != bridgeRule2->vlanRule.filterMode)
        || (bridgeRule1->vlanRule.filterRule.filterCtagMode != bridgeRule2->vlanRule.filterRule.filterCtagMode)
        || (bridgeRule1->vlanRule.filterRule.filterCTag.tpid != bridgeRule2->vlanRule.filterRule.filterCTag.tpid)
        || (bridgeRule1->vlanRule.filterRule.filterCTag.pri != bridgeRule2->vlanRule.filterRule.filterCTag.pri)
        || (bridgeRule1->vlanRule.filterRule.filterCTag.vid != bridgeRule2->vlanRule.filterRule.filterCTag.vid)
        || (bridgeRule1->vlanRule.filterRule.filterStagMode != bridgeRule2->vlanRule.filterRule.filterStagMode)
        || (bridgeRule1->vlanRule.filterRule.filterSTag.tpid != bridgeRule2->vlanRule.filterRule.filterSTag.tpid)
        || (bridgeRule1->vlanRule.filterRule.filterSTag.pri != bridgeRule2->vlanRule.filterRule.filterSTag.pri)
        || (bridgeRule1->vlanRule.filterRule.filterSTag.vid != bridgeRule2->vlanRule.filterRule.filterSTag.vid)
        || (bridgeRule1->vlanRule.filterRule.etherType != bridgeRule2->vlanRule.filterRule.etherType)
        || (bridgeRule1->vlanRule.cTagAct.vlanAct != bridgeRule2->vlanRule.cTagAct.vlanAct)
        || (bridgeRule1->vlanRule.cTagAct.vidAct != bridgeRule2->vlanRule.cTagAct.vidAct)
        || (bridgeRule1->vlanRule.cTagAct.priAct != bridgeRule2->vlanRule.cTagAct.priAct)
        || (bridgeRule1->vlanRule.cTagAct.assignVlan.pri != bridgeRule2->vlanRule.cTagAct.assignVlan.pri)
        || (bridgeRule1->vlanRule.cTagAct.assignVlan.vid != bridgeRule2->vlanRule.cTagAct.assignVlan.vid)
        || (bridgeRule1->vlanRule.cTagAct.assignVlan.tpid != bridgeRule2->vlanRule.cTagAct.assignVlan.tpid && !omci_isIgnoreTpid('C', bridgeRule1, bridgeRule2))
        || (bridgeRule1->vlanRule.sTagAct.vlanAct != bridgeRule2->vlanRule.sTagAct.vlanAct && !isLikeVlanAct(bridgeRule1->vlanRule.sTagAct, bridgeRule2->vlanRule.sTagAct))
        || (bridgeRule1->vlanRule.sTagAct.vidAct != bridgeRule2->vlanRule.sTagAct.vidAct)
        || (bridgeRule1->vlanRule.sTagAct.priAct != bridgeRule2->vlanRule.sTagAct.priAct)
        || (bridgeRule1->vlanRule.sTagAct.assignVlan.pri != bridgeRule2->vlanRule.sTagAct.assignVlan.pri)
        || (bridgeRule1->vlanRule.sTagAct.assignVlan.vid != bridgeRule2->vlanRule.sTagAct.assignVlan.vid)
        || (bridgeRule1->vlanRule.sTagAct.assignVlan.tpid != bridgeRule2->vlanRule.sTagAct.assignVlan.tpid) //&& !omci_isIgnoreTpid('S', bridgeRule1, bridgeRule2)
        || (bridgeRule1->vlanRule.outStyle.isDefaultRule != bridgeRule2->vlanRule.outStyle.isDefaultRule)
        || (bridgeRule1->vlanRule.outStyle.outTagNum != bridgeRule2->vlanRule.outStyle.outTagNum)
        || (bridgeRule1->vlanRule.outStyle.tpid != bridgeRule2->vlanRule.outStyle.tpid)
        || (bridgeRule1->vlanRule.outStyle.dsMode != bridgeRule2->vlanRule.outStyle.dsMode)
        || (bridgeRule1->vlanRule.outStyle.outVlan.pri != bridgeRule2->vlanRule.outStyle.outVlan.pri)
        || (bridgeRule1->vlanRule.outStyle.outVlan.vid != bridgeRule2->vlanRule.outStyle.outVlan.vid)
        || (bridgeRule1->vlanRule.outStyle.outVlan.tpid != bridgeRule2->vlanRule.outStyle.outVlan.tpid && !omci_isIgnoreTpid('O', bridgeRule1, bridgeRule2)))
        {
            return FALSE;
        }

    return TRUE;
}

static int
omci_UpdateServInfo(OMCI_BRIDGE_RULE_ts *pBridgeRule)
{
    UINT16 servId;

    for(servId = OMCI_SERV_ID_BASE; servId < OMCI_MAX_SERV_ID; servId++)
    {
        if(TRUE == serviceFlow[servId].isUsed)
        {
            if(TRUE == omci_isBridgeRuleSameExcludeUni(pBridgeRule, &serviceFlow[servId]))
            {
                /*same bridgeRule existed, update UNI mask*/
                serviceFlow[servId].uniMask |= pBridgeRule->uniMask;
                return servId;
            }
        }
    }

    /*Not found, add new*/
    memcpy(&serviceFlow[pBridgeRule->servId], pBridgeRule, sizeof(OMCI_BRIDGE_RULE_ts));
    return pBridgeRule->servId;
}

static UINT16 omci_GetAvailServId(void)
{
    UINT16 servId=0;

    for(servId = OMCI_SERV_ID_BASE; servId < OMCI_MAX_SERV_ID; servId++)
    {
        if (serviceFlow[servId].isUsed == FALSE)
            break;
    }

    return servId;
}

static BOOL omci_FindL3MgmtRule(void)
{
    char                buffer[OMCI_BOA_MIB_BUFFER_LEN];
    FILE                *pFd = NULL;

    if (NULL != (pFd = popen("iptables -vL INPUT", "r")))
    {
        while (fgets(buffer, sizeof(buffer), pFd))
        {
            if (NULL != strstr(buffer, PON_IF))
            {
                pclose(pFd);
                return TRUE;
            }
        }
        pclose(pFd);
    }

    return FALSE;
}


static GOS_ERROR_CODE omci_set_ip_connectivity(
    BOOL act, PON_GEMPORT_DIRECTION dir, int ingress, OMCI_VLAN_OPER_ts *pVlanRule)
{
    MIB_TABLE_IP_HOST_CFG_DATA_T ipHost;
    unsigned int vid;
    CHAR cmd[CMD_LEN];
    int ret;

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (gInfo.dmMode), GOS_OK);
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (!pVlanRule), GOS_OK);
    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (PON_GEMPORT_DIRECTION_DS == dir), GOS_OK);

    memset(&ipHost, 0, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T));
    ipHost.EntityID = ingress;

    OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG,
        (GOS_OK != MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &ipHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T))), GOS_OK);

    if ((pVlanRule->filterRule.filterStagMode & VLAN_FILTER_VID) ||
        (pVlanRule->filterRule.filterStagMode & VLAN_FILTER_TCI))
    {
        vid = pVlanRule->filterRule.filterSTag.vid;
    }
    else if ((pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_VID) ||
            (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_TCI))
    {
        vid = pVlanRule->filterRule.filterCTag.vid;
    }
    else
    {
        vid = pVlanRule->outStyle.outVlan.vid;
        OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (vid > 4095), GOS_OK);
    }

    if (act)
    {
        if (!(ipHost.IpOptions & IP_HOST_CFG_DATA_IP_OPTIONS_ENABLE_DHCP) &&
            (0 != ipHost.IpAddress) && (0 != ipHost.Mask))
        {
            /* add */
            if ((pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG) &&
               (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_NO_TAG))
            {
                memset(cmd, 0, CMD_LEN);
                snprintf(cmd, CMD_LEN, "%s %s "IPADDR_PRINT" netmask "IPADDR_PRINT" up",
                        IFCONFIG_CMD, PON_IF, IPADDR_PRINT_ARG(ipHost.IpAddress), IPADDR_PRINT_ARG(ipHost.Mask));
                system(cmd);

                memset(cmd, 0, CMD_LEN);
                snprintf(cmd, CMD_LEN, "%s -I INPUT -i %s -j ACCEPT", IPTABLES_CMD, PON_IF);
                system(cmd);

            }
            else
            {
                memset(cmd, 0, CMD_LEN);
                snprintf(cmd, CMD_LEN, "%s add %s %u", VCONFIG_CMD, PON_IF, vid);
                ret = system(cmd);
                if (-1 == ret || FALSE == WIFEXITED(ret) || 0 != WEXITSTATUS(ret))
                {
                    return GOS_OK;
                }

                memset(cmd, 0, CMD_LEN);
                snprintf(cmd, CMD_LEN, "%s %s.%u "IPADDR_PRINT" netmask "IPADDR_PRINT" up",
                        IFCONFIG_CMD, PON_IF, vid, IPADDR_PRINT_ARG(ipHost.IpAddress), IPADDR_PRINT_ARG(ipHost.Mask));
                system(cmd);

                memset(cmd, 0, CMD_LEN);
                snprintf(cmd, CMD_LEN, "%s -I INPUT -i %s.%u -j ACCEPT", IPTABLES_CMD, PON_IF, vid);
                system(cmd);
            }
        }
    }
    else
    {
        /* remove */
        OMCI_ERR_CHK(OMCI_LOG_LEVEL_DBG, (FALSE == omci_FindL3MgmtRule()), GOS_OK);
        if ((pVlanRule->filterRule.filterStagMode & VLAN_FILTER_NO_TAG) &&
               (pVlanRule->filterRule.filterCtagMode & VLAN_FILTER_NO_TAG))
        {
            memset(cmd, 0, CMD_LEN);
            snprintf(cmd, CMD_LEN, "%s -D INPUT -i %s -j ACCEPT", IPTABLES_CMD, PON_IF);
            system(cmd);

            memset(cmd, 0, CMD_LEN);
            snprintf(cmd, CMD_LEN, "%s %s down", IFCONFIG_CMD, PON_IF);
            system(cmd);
        }
        else
        {
            memset(cmd, 0, CMD_LEN);
            snprintf(cmd, CMD_LEN, "%s -D INPUT -i %s.%u -j ACCEPT", IPTABLES_CMD, PON_IF, vid);
            system(cmd);

            memset(cmd, 0, CMD_LEN);
            snprintf(cmd, CMD_LEN, "%s %s.%u down", IFCONFIG_CMD, PON_IF, vid);
            system(cmd);

            memset(cmd, 0, CMD_LEN);
            snprintf(cmd, CMD_LEN, "%s rem %s.%u", VCONFIG_CMD, PON_IF, vid);
            ret = system(cmd);

            if (-1 == ret || FALSE == WIFEXITED(ret) || 0 != WEXITSTATUS(ret))
            {
                return GOS_OK;
            }
        }
    }
    return GOS_OK;

}

void omci_update_conn_if_mod_tpid(void)
{
    int sid;
    INT32 ret;
    unsigned int cur_tpid;
    OMCI_BRIDGE_RULE_ts rule_buf;

    if (GOS_OK != omci_wrapper_getSvlanTpid(0, &cur_tpid))
        return;
    if (0x8100 != cur_tpid)
        return;

    for(sid = OMCI_SERV_ID_BASE; sid < OMCI_MAX_SERV_ID; sid++)
    {
        if (serviceFlow[sid].isUsed == 0)
            continue;
        memset(&rule_buf, 0, sizeof(OMCI_BRIDGE_RULE_ts));
        memcpy(&rule_buf, &serviceFlow[sid], sizeof(OMCI_BRIDGE_RULE_ts));

        ret = omci_ioctl_send(OMCI_IOCTL_CF_DEL,&sid, sizeof(sid));
        if(GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s del sid:%d",__FUNCTION__,sid);
        }

        ret = omci_ioctl_send(OMCI_IOCTL_CF_ADD, &rule_buf, sizeof(OMCI_BRIDGE_RULE_ts));
        if(GOS_OK != ret)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s add sid:%d",__FUNCTION__,sid);
        }
    }
    return;
}

GOS_ERROR_CODE
omci_wrapper_activeBdgConn(omci_vlan_rule_t *pRule)
{
    INT32    ret = GOS_OK, servId;
    OMCI_BRIDGE_RULE_ts bridgeRule, rule_buf;
    UINT16    uniPort, veipInstanceId;
    UINT32    veipSlotId;
    MIB_TABLE_IP_HOST_CFG_DATA_T ipHost;

    ipHost.EntityID = pRule->ingress;

    memset(&bridgeRule,0,sizeof(OMCI_BRIDGE_RULE_ts));
    memset(&rule_buf, 0, sizeof(OMCI_BRIDGE_RULE_ts));

    bridgeRule.isLatch = pRule->isLatchB;

    /*port mapping*/
    veipSlotId = TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_VEIP_SLOT);
    feature_api(FEATURE_API_ME_00000100, &veipSlotId);
    veipInstanceId = (UINT16)((veipSlotId << 8) | 1);
    if (veipInstanceId == pRule->ingress)
    {
        uniPort = gInfo.devCapabilities.ponPort;
    }
    else if (pRule->ingress < 0)
    {
        uniPort = 0xFFFF;
    }
    else if (GOS_OK == MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &ipHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
    {
        uniPort = gInfo.devCapabilities.cpuPort;
        omci_set_ip_connectivity(TRUE, pRule->dir, pRule->ingress, &pRule->vlanRule);
    }
    else if (GOS_OK != pptp_eth_uni_me_id_to_switch_port((UINT16)pRule->ingress, &uniPort))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_WARN,"Can't find UNI %x for GemPort %d", pRule->ingress, pRule->outgress);
        pRule->servId = -1;
        return GOS_FAIL;
    }

    if (FAL_ERR_DISABLE == feature_api(FEATURE_API_RDP_00000001_X_BDG_CONN, gInfo.devMode, pRule->ingress, &uniPort))
    {
        if(FAL_OK == feature_api(FEATURE_API_RDP_00000004, pRule->ingress, &uniPort))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "Support unknow UNI conn");
        } else {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Disable for VEIP BDG conn");
            pRule->servId = -1;
            return GOS_ERR_DISABLE;
        }
    }

    if (PON_GEMPORT_DIRECTION_US == pRule->dir || PON_GEMPORT_DIRECTION_BI == pRule->dir)
    {
        if (FAL_OK != feature_api(FEATURE_API_BDP_00000004_ACTIVE_BDG_CONN,
            gInfo.devCapabilities.totalTContQueueNum, g_pqInfo,
            &bridgeRule, pRule->vlanRule.filterRule.filterCTag.pri,
            gInfo.devCapabilities.totalGEMPortNum))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_DBG, "get us flow by port Id ");
            bridgeRule.usFlowId =  omci_GetUsFlowIdByPortId(pRule->outgress);
        }
    }
    if(PON_GEMPORT_DIRECTION_DS == pRule->dir || PON_GEMPORT_DIRECTION_BI == pRule->dir)
    {
        bridgeRule.dsFlowId =  omci_GetDsFlowIdByPortId(pRule->outgress);
    }

    if(gInfo.devCapabilities.totalGEMPortNum == bridgeRule.usFlowId || gInfo.devCapabilities.totalGEMPortNum == bridgeRule.dsFlowId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Can't find flow ID from UNI %x ,GemPort %d", pRule->ingress, pRule->outgress);
        pRule->servId = -1;
        return GOS_FAIL;
    }
    if (0xFFFF == uniPort)
    {
        UINT32 uniPm  = 0;
        omci_get_all_eth_uni_port_mask(&uniPm);
        bridgeRule.uniMask = uniPm | (1 << gInfo.devCapabilities.ponPort);
    }
    else
    {
        bridgeRule.uniMask = (1 << uniPort);
    }

    bridgeRule.servId =  omci_GetAvailServId();
    memcpy(&bridgeRule.vlanRule, &pRule->vlanRule, sizeof(OMCI_VLAN_OPER_ts));
    bridgeRule.isUsed = TRUE;
    bridgeRule.dir    =  pRule->dir;
    omci_SetUsConnDpByBrRule(&bridgeRule);

    servId = omci_UpdateServInfo(&bridgeRule);
    omci_UpdateMcastInfo(SET_CTRL_WRITE, &serviceFlow[servId]);

    OMCI_LOG(OMCI_LOG_LEVEL_INFO,
        "Active Bridge Connection, IN : 0x%x, OUT : %d, uniMask=%d, bsvcId=%d, svcId=%d", pRule->ingress, pRule->outgress, serviceFlow[servId].uniMask,serviceFlow[servId].servId, servId);
    memcpy(&rule_buf, &serviceFlow[servId], sizeof(OMCI_BRIDGE_RULE_ts));
    ret = omci_ioctl_send(OMCI_IOCTL_CF_ADD, &rule_buf, sizeof(OMCI_BRIDGE_RULE_ts));
    if (ret == OMCI_ERR_OK)
    {
        pRule->servId = servId;
    }else{
        serviceFlow[servId].isUsed = FALSE;
    }

    usleep(1000);
    return (ret==OMCI_ERR_OK) ? GOS_OK : GOS_FAIL;
}

GOS_ERROR_CODE
omci_wrapper_deactiveBdgConn(int servId, int ingress)
{
    INT32 ret = OMCI_ERR_OK, uniMask;
    OMCI_BRIDGE_RULE_ts *pBridgeRule, rule_buf;
    UINT16 uniPort = USHRT_MAX, veipInstanceId;
    UINT32 veipSlotId;
    MIB_TABLE_IP_HOST_CFG_DATA_T ipHost;

    ipHost.EntityID = ingress;
    veipSlotId = TXC_GET_SLOT_NUM_BY_SLOT_ID(TXC_CARDHLD_VEIP_SLOT);
    feature_api(FEATURE_API_ME_00000100, &veipSlotId);

    veipInstanceId = (UINT16)((veipSlotId << 8) | 1);

    if (veipInstanceId == ingress)
    {
        uniPort = gInfo.devCapabilities.ponPort;
    }
    else if (ingress < 0)
    {
        uniPort = USHRT_MAX;
    }
    /*For IpHostConfigData, TBD: for all IpHost entity*/
    else if (GOS_OK == MIB_Get(MIB_TABLE_IP_HOST_CFG_DATA_INDEX, &ipHost, sizeof(MIB_TABLE_IP_HOST_CFG_DATA_T)))
    {
        uniPort = gInfo.devCapabilities.cpuPort;
    }
    else if (GOS_OK != pptp_eth_uni_me_id_to_switch_port((UINT16)ingress, &uniPort))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Can't find physical port ID %u", ingress);
        return GOS_FAIL;
    }

    if (FAL_ERR_DISABLE == feature_api(FEATURE_API_RDP_00000001_X_BDG_CONN, gInfo.devMode, ingress, &uniPort))
    {
        if(FAL_OK == feature_api(FEATURE_API_RDP_00000004, ingress, &uniPort))
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "Support unknow UNI conn");
        } else {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO, "Disable for VEIP BDG conn");
            return GOS_ERR_DISABLE;
        }
    }

    if((servId >= 0 && servId < OMCI_MAX_SERV_ID))
    {
        pBridgeRule = &serviceFlow[servId];

        if (pBridgeRule->uniMask & (1 << gInfo.devCapabilities.cpuPort))
            omci_set_ip_connectivity(FALSE, pBridgeRule->dir, ingress, &pBridgeRule->vlanRule);

        uniMask = pBridgeRule->uniMask & (~(1 << uniPort));
        if (uniMask == 0 || uniPort == USHRT_MAX)
        {
            if (PON_GEMPORT_DIRECTION_BI == pBridgeRule->dir &&
                (PRI_ACT_FROM_DSCP == pBridgeRule->vlanRule.sTagAct.priAct ||
                 PRI_ACT_FROM_DSCP == pBridgeRule->vlanRule.cTagAct.priAct))
            {
                omci_ClearUsedPriority((unsigned char)(pBridgeRule->vlanRule.outStyle.outVlan.pri));
            }

            /*Delete this flow*/
            omci_UpdateMcastInfo(SET_CTRL_DELETE, &serviceFlow[servId]);
            ret = omci_ioctl_send(OMCI_IOCTL_CF_DEL,&servId,sizeof(servId));

            if (ret != OMCI_ERR_OK)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_ERR,"Del Class Rule < ---- FAIL");
                return GOS_FAIL;
            }else
            {
                memset(pBridgeRule,0,sizeof(OMCI_BRIDGE_RULE_ts));
                pBridgeRule->isUsed = FALSE;
                OMCI_LOG(OMCI_LOG_LEVEL_INFO,"Del Class Rule %d < ---- OK",servId);
            }
        }else
        {
            /*update the uniMask*/

            pBridgeRule->uniMask = uniMask;
            omci_UpdateMcastInfo(SET_CTRL_DELETE, &serviceFlow[servId]);
            ret = omci_ioctl_send(OMCI_IOCTL_CF_DEL,&servId,sizeof(servId));
            omci_UpdateMcastInfo(SET_CTRL_WRITE, pBridgeRule);
            memset(&rule_buf, 0, sizeof(OMCI_BRIDGE_RULE_ts));
            memcpy(&rule_buf, pBridgeRule, sizeof(OMCI_BRIDGE_RULE_ts));
            ret = omci_ioctl_send(OMCI_IOCTL_CF_ADD, &rule_buf, sizeof(OMCI_BRIDGE_RULE_ts));
        }

    }

    usleep(1000);
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setDscpRemap(UINT8 *pDscpToPbitMapping)
{
    UINT32 i, j;
    UINT32 tempPbit, dscpCnt = 0;

    memset(&g_dscp2pbitInfo, 0x0, sizeof(omci_dscp2pbit_info_t));

    for(i = 0; i < MIB_TABLE_DSCPTOPBITMAPPING_LEN; i += 3)
    {
        tempPbit = (pDscpToPbitMapping[i] << 16) + (pDscpToPbitMapping[i + 1] << 8)
                   + (pDscpToPbitMapping[i + 2]);
        for(j = 0; j < 8 ; j++)
        {
            g_dscp2pbitInfo.dscp2PbitTable.pbit[dscpCnt] = (tempPbit & 0xE00000) >> 21;
            if (g_dscp2pbitInfo.lastDscp2PbitTable.pbit[dscpCnt] !=
                g_dscp2pbitInfo.dscp2PbitTable.pbit[dscpCnt])
            {
                g_dscp2pbitInfo.pbitBitmap |= (1 << g_dscp2pbitInfo.dscp2PbitTable.pbit[dscpCnt]);
                g_dscp2pbitInfo.lastDscp2PbitTable.pbit[dscpCnt] =
                                g_dscp2pbitInfo.dscp2PbitTable.pbit[dscpCnt];
            }
            dscpCnt ++;
            tempPbit <<= 3;
        }
    }

    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_DSCPREMAP_SET, &(g_dscp2pbitInfo.dscp2PbitTable), sizeof(OMCI_DSCP2PBIT_ts)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setMacLearnLimit(unsigned int portIdx, unsigned int macLimitNum)
{
    OMCI_MACLIMIT_ts macLimit;

    macLimit.portIdx = portIdx;
    macLimit.macLimitNum = macLimitNum;

    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_MACLIMIT_SET, &macLimit, sizeof(OMCI_MACLIMIT_ts)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setMacFilter(OMCI_MACFILTER_ts *pMacFilter)
{

    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_MACFILTER_SET, pMacFilter, sizeof(OMCI_MACFILTER_ts)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setGroupMacFilter(OMCI_GROUPMACFILTER_ts *pGroupMacFilter)
{
    if(OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_GROUPMACFILTER_SET, pGroupMacFilter, sizeof(OMCI_GROUPMACFILTER_ts)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);
        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setSvlanTpid(unsigned int index, unsigned int tpid)
{
    INT32               ret;
    omci_svlan_tpid_t   data;

    data.index = index;
    data.tpid = tpid;

    ret = omci_ioctl_send(OMCI_IOCTL_SVLAN_TPID_SET, &data, sizeof(omci_svlan_tpid_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getSvlanTpid(unsigned int index, unsigned int *pTpid)
{
    INT32               ret;
    omci_svlan_tpid_t   data;

    if (!pTpid)
        return GOS_ERR_PARAM;

    data.index = index;

    ret = omci_ioctl_send(OMCI_IOCTL_SVLAN_TPID_GET, &data, sizeof(omci_svlan_tpid_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    *pTpid = data.tpid;

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getCvlanState(unsigned int *state)
{
    INT32               ret;

    if (!state)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_CVLAN_STATE_GET, state, sizeof(unsigned int));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

// veip control
static GOS_ERROR_CODE
omci_GetUsVeipExpandedPq(UINT16 oriQueueId, UINT16 *pExpQueueId)
{
    UINT16  newQueueId;
    MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);

    if (!pExpQueueId)
        return GOS_ERR_PARAM;

    for (newQueueId = gInfo.devCapabilities.totalTContQueueNum;
            newQueueId < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; newQueueId++)
    {
        if (g_pqInfo[newQueueId].tcontId != g_pqInfo[oriQueueId].tcontId)
            continue;

        // skip original queue
        if (newQueueId == oriQueueId)
            continue;

        if (PQ_DROP_COLOUR_INTERNAL_MARKING != g_pqInfo[newQueueId].dpMarking)
            continue;

        if (g_pqInfo[newQueueId].dpQueueId == oriQueueId)
        {
            *pExpQueueId = newQueueId;

            return GOS_OK;
        }
    }

    return GOS_ERR_NOT_FOUND;
}

static GOS_ERROR_CODE
omci_SetUsVeipGemFlow(UINT16 gemPortId)
{
    INT32                   ret;
    veipGemFlow_t           data;
    UINT16                  queueId;
    UINT16                  flowId;
    UINT8                   i;

    flowId = omci_GetUsFlowIdByPortId(gemPortId);
    if (gInfo.devCapabilities.totalGEMPortNum == flowId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

        return GOS_FAIL;
    }

    if (g_gemUsFlowInfo[flowId].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
    {
        MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
        queueId = g_gemUsFlowInfo[flowId].pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    gInfo.devCapabilities.totalTContQueueNum;
        if (queueId >= gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us queue not found");

            return GOS_FAIL;
        }
    }
    else
    {
        queueId = g_gemUsFlowInfo[flowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
        if (queueId >= gInfo.devCapabilities.totalTContQueueNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us queue not found");

            return GOS_FAIL;
        }
    }

    memset(&data, 0, sizeof(data));
    data.gemPortId = gemPortId;
    data.tcontId = g_pqInfo[queueId].tcontId;

    // fill flow
    data.flowId[0] = flowId;
    for (i = 1; i < gInfo.wanQueueNum ; i++)
    {
        for (data.flowId[i] = 0;
                data.flowId[i] < gInfo.devCapabilities.totalGEMPortNum; data.flowId[i]++)
        {
            if (FALSE == g_gemUsFlowInfo[data.flowId[i]].inUse)
                continue;

            // skip original flow
            if (data.flowId[i] == flowId)
                continue;

            if (g_gemUsFlowInfo[data.flowId[i]].portId == g_gemUsFlowInfo[flowId].portId)
            {
                // will be set to true later
                g_gemUsFlowInfo[data.flowId[i]].inUse = FALSE;

                break;
            }
        }
    }

    // fill queue
    data.tcQueueId[0] = g_pqInfo[queueId].tcQueueId;
    for (i = 1; i < gInfo.wanQueueNum; i++)
    {
        if (data.flowId[i] < gInfo.devCapabilities.totalGEMPortNum)
        {
            if (g_gemUsFlowInfo[data.flowId[i]].pqEntityId >= OMCI_MIB_US_TM_ME_RSV_ID_BASE)
            {
                queueId = g_gemUsFlowInfo[data.flowId[i]].pqEntityId - OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                            gInfo.devCapabilities.totalTContQueueNum;;
            }
            else
            {
                queueId = g_gemUsFlowInfo[data.flowId[i]].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
            }

            data.tcQueueId[i] = g_pqInfo[queueId].tcQueueId;

            // set to true
            g_gemUsFlowInfo[data.flowId[i]].inUse = TRUE;
        }
    }

    ret = omci_ioctl_send(OMCI_IOCTL_VEIP_GEM_FLOW_SET, &data, sizeof(data));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "set veip gem flow [%u] failed!", flowId);

        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "set veip gem flow [%u]", flowId);

    return GOS_OK;
}

static GOS_ERROR_CODE
omci_DelUsVeipGemFlow(UINT16 gemPortId)
{
    INT32                   ret;
    veipGemFlow_t   data;
    UINT16                  flowId;

    flowId = omci_GetUsFlowIdByPortId(gemPortId);
    if (gInfo.devCapabilities.totalGEMPortNum == flowId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

        return GOS_FAIL;
    }

    memset(&data, 0, sizeof(data));
    data.gemPortId = g_gemUsFlowInfo[flowId].portId;

    ret = omci_ioctl_send(OMCI_IOCTL_VEIP_GEM_FLOW_DEL, &data, sizeof(data));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "del veip gem flow [%u] failed!", flowId);

        return GOS_FAIL;
    }

    OMCI_LOG(OMCI_LOG_LEVEL_INFO, "del veip gem flow [%u]", flowId);

    return GOS_OK;
}

/*
    set us pq for veip if necessary

    for every gem flow that belongs to bridge with veip inside
    an expansion of priority queues and pseudo flows will be needed
    this is an advance feature in order to satisfy qos within wan interfaces
 */
GOS_ERROR_CODE
omci_wrapper_setUsVeipPriQ(MIB_TREE_T *pMibTree, UINT16 gemPortId, UINT8 isCreate)
{
    UINT8               bFound, bExpand;
    UINT16              oriQueueId;
    UINT16              expQueueId[WAN_PONMAC_QUEUE_MAX - 1];
    UINT16              oriFlowId;
    UINT16              expFlowId[WAN_PONMAC_QUEUE_MAX - 1];
    UINT16              gemCount;
    UINT8               i, j;
    UINT16              dpFlowId;

    if (!gInfo.dmMode)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_INFO, "dual mgmt mode is set to off");

        return GOS_ERR_DISABLE;
    }

    if (!pMibTree)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no mib tree is given");

        return GOS_ERR_PARAM;
    }
/*
//Marked below code in order to omci don't care uni relationship. BUT there are still uni vlan filtering condition.
    if (FAL_ERR_NOT_REGISTER == feature_api_is_registered(FEATURE_API_RDP_00000001_RESET_PPTP) ||
        OMCI_DEV_MODE_ROUTER != gInfo.devMode)
    {

        // return if veip is not part of the bridge
        pMibNode = MIB_AvlTreeSearch(pMibTree->root, AVL_KEY_VEIP);
        if (!pMibNode)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "veip is not part of the bridge");

            return GOS_ERR_NOT_FOUND;
        }

        bFound = FALSE;
        LIST_FOREACH(pDataEntry, &pMibNode->data.treeNodeEntryHead, treeNodeEntry)
        {
            pMibEntry = pDataEntry->mibEntry;
            pMibVeip = (MIB_TABLE_VEIP_T *)pMibEntry->pData;

            // only 1 veip will be exist in the system
            // so there is no need to check instance id
            //if (pMibVeip->EntityId == xxx)
            {
                bFound = TRUE;
                break;
            }
        }

        // return if veip is not part of the bridge
        if (!bFound)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_WARN, "veip is not part of the bridge");

            return GOS_ERR_NOT_FOUND;
        }
    }
*/
    oriFlowId = omci_GetUsFlowIdByPortId(gemPortId);
    if (gInfo.devCapabilities.totalGEMPortNum == oriFlowId)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us gem flow not found");

        return GOS_FAIL;
    }

    oriQueueId = g_gemUsFlowInfo[oriFlowId].pqEntityId - OMCI_MIB_US_TM_ME_ID_BASE;
    if (oriQueueId >= gInfo.devCapabilities.totalTContQueueNum)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "us queue not found");

        return GOS_FAIL;
    }

    if (isCreate)
    {
        // check if the queue has been expanded or not
        bFound = FALSE;
        for (i = 0; i < (gInfo.wanQueueNum - 1); i++)
        {
            if (GOS_OK != omci_GetUsVeipExpandedPq(oriQueueId, &expQueueId[i]))
                continue;

            // mark the queue temporarily
            g_pqInfo[expQueueId[i]].dpMarking = PQ_DROP_COLOUR_NO_MARKING;

            bFound = TRUE;
        }

        if (bFound)
        {
            // revert the queue marking
            for (j = 0; j < i; j++)
                g_pqInfo[expQueueId[j]].dpMarking = PQ_DROP_COLOUR_INTERNAL_MARKING;
        }
        else
        {
            MIB_TABLE_T *pTable = mib_GetTablePtr(MIB_TABLE_PRIQ_INDEX);
            for (i = 0; i < (gInfo.wanQueueNum - 1); i++)
            {
                // find a queue which is never assigned to any t-cont
                for (expQueueId[i] = gInfo.devCapabilities.totalTContQueueNum;
                        expQueueId[i] < gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount; expQueueId[i]++)
                {
                    if (OMCI_DRV_INVALID_TCONT_ID == g_pqInfo[expQueueId[i]].tcontId)
                        break;
                }
                // no queues available for veip queue expansion
                if (expQueueId[i] == gInfo.devCapabilities.totalTContQueueNum + pTable->curNoUploadCount)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "no pq available for veip queue expansion");

                    // rollback
                    goto remove_queues;
                }

                OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                    "allocate expanded queue %u for queue %u", expQueueId[i], oriQueueId);

                g_pqInfo[expQueueId[i]].tcontId = g_pqInfo[oriQueueId].tcontId;
                // use internal marking to mark this queue as supplement queue
                g_pqInfo[expQueueId[i]].dpMarking = PQ_DROP_COLOUR_INTERNAL_MARKING;
            }
        }

        if (!bFound)
            bExpand = FALSE;
        else
        {
            /* no found */
            bExpand = FALSE;
            for (dpFlowId = 0; dpFlowId < gInfo.devCapabilities.totalGEMPortNum; dpFlowId++)
            {
                if (FALSE == g_gemUsFlowInfo[dpFlowId].inUse ||
                    g_gemUsFlowInfo[dpFlowId].pqEntityId < OMCI_MIB_US_TM_ME_RSV_ID_BASE)
                    continue;
                /* found */
                if (gemPortId == g_gemUsFlowInfo[dpFlowId].portId)
                {
                    bExpand = TRUE;
                    break;
                }
            }
        }
        if (!bExpand)
        {
            // allocate gem flows
            for (i = 0; i < (gInfo.wanQueueNum - 1); i++)
            {
                expFlowId[i] = omci_GetAvailUsFlowId();
                if (expFlowId[i] == gInfo.devCapabilities.totalGEMPortNum)
                {
                    OMCI_LOG(OMCI_LOG_LEVEL_ERR,
                        "no gem flow available for veip queue expansion");

                    // rollback
                    goto remove_flows;
                }

                OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                    "%s() allocate expanded flow %u for flow %u", __FUNCTION__, expFlowId[i], oriFlowId);

                // flows should be set with same gem port id and same pq
                g_gemUsFlowInfo[expFlowId[i]].inUse = TRUE;
                g_gemUsFlowInfo[expFlowId[i]].portId = g_gemUsFlowInfo[oriFlowId].portId;
                g_gemUsFlowInfo[expFlowId[i]].pqEntityId = OMCI_MIB_US_TM_ME_RSV_ID_BASE + \
                    (expQueueId[i] - gInfo.devCapabilities.totalTContQueueNum);
            }
        }
        if (!bFound)
        {

            gemCount = g_tcontInfo[g_pqInfo[oriQueueId].tcontId].gemCount;

            // delete gem flows
            if (gemCount != 0 &&
                    GOS_OK != omci_TempDeleteUsGemFlowByTcontId(g_pqInfo[oriQueueId].tcontId))
            {
                // rollback
                goto remove_flows;
            }

            for (i = 0; i < (gInfo.wanQueueNum - 1); i++)
            {
                OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                    "attach veip pq %u to queue %u", expQueueId[i], oriQueueId);

                g_pqInfo[expQueueId[i]].policy = PQ_POLICY_STRICT_PRIORITY;
                g_pqInfo[expQueueId[i]].pw.priority = g_pqInfo[oriQueueId].pw.priority;

                // assign queue relationships
                g_pqInfo[expQueueId[i]].dpQueueId = oriQueueId;
            }

            // recreate pq & gem flows
            if (gemCount != 0 &&
                    GOS_OK != omci_ReCreateUsGemFlowByTcontId(g_pqInfo[oriQueueId].tcontId))
            {
                // rollback
                goto remove_flows;
            }

            // recreate conn
            if (gemCount != 0 &&
                    GOS_OK != omci_ReSetUsConnDpByPqEntityId(OMCI_MIB_US_TM_ME_ID_BASE + oriQueueId))
            {
                // rollback
                goto remove_flows;
            }
        }

        // sync to vgf
        omci_SetUsVeipGemFlow(gemPortId);

        return GOS_OK;

remove_flows:
        for (j = 0; j < i; j++)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                "deallocate expanded flow %u for flow %u", expFlowId[j], oriFlowId);

            g_gemUsFlowInfo[expFlowId[j]].inUse = FALSE;
            g_gemUsFlowInfo[expFlowId[j]].portId = 0;
            g_gemUsFlowInfo[expFlowId[j]].pqEntityId = OMCI_DRV_INVALID_QUEUE_ID;
        }
        i = (gInfo.wanQueueNum - 1);

remove_queues:
        for (j = 0; j < i; j++)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                "deallocate expanded queue %u for queue %u", expQueueId[j], oriQueueId);

            g_pqInfo[expQueueId[j]].tcontId = OMCI_DRV_INVALID_TCONT_ID;
            g_pqInfo[expQueueId[j]].dpMarking = PQ_DROP_COLOUR_NO_MARKING;
        }

        return GOS_FAIL;
    }
    else
    {
        // deallocate gem flows
        for (expFlowId[0] = 0;
                expFlowId[0] < gInfo.devCapabilities.totalGEMPortNum; expFlowId[0]++)
        {
            if (FALSE == g_gemUsFlowInfo[expFlowId[0]].inUse)
                continue;

            // skip original flow
            if (expFlowId[0] == oriFlowId)
                continue;

            if (g_gemUsFlowInfo[expFlowId[0]].portId != g_gemUsFlowInfo[oriFlowId].portId)
                continue;

            OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                "deallocate expanded flow %u for flow %u", expFlowId[0], oriFlowId);

            g_gemUsFlowInfo[expFlowId[0]].inUse = FALSE;
            g_gemUsFlowInfo[expFlowId[0]].portId = 0;
            g_gemUsFlowInfo[expFlowId[0]].pqEntityId = OMCI_DRV_INVALID_QUEUE_ID;
        }

        // make sure the queue has no reference by other flows
        bFound = FALSE;
        for (expFlowId[0] = 0;
                expFlowId[0] < gInfo.devCapabilities.totalGEMPortNum; expFlowId[0]++)
        {
            if (FALSE == g_gemUsFlowInfo[expFlowId[0]].inUse)
                continue;

            // skip original flow
            if (expFlowId[0] == oriFlowId)
                continue;

            if (g_gemUsFlowInfo[expFlowId[0]].pqEntityId == g_gemUsFlowInfo[oriFlowId].pqEntityId)
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            gemCount = g_tcontInfo[g_pqInfo[oriQueueId].tcontId].gemCount;

            // delete gem flows
            if (gemCount != 0 &&
                    GOS_OK != omci_TempDeleteUsGemFlowByTcontId(g_pqInfo[oriQueueId].tcontId))
                return GOS_FAIL;

            // find queues which are assigned to specified t-cont
            for (i = 0; i < (gInfo.wanQueueNum - 1); i++)
            {
                if (GOS_OK != omci_GetUsVeipExpandedPq(oriQueueId, &expQueueId[0]))
                    continue;

                OMCI_LOG(OMCI_LOG_LEVEL_INFO,
                    "detach veip pq %u from queue %u", expQueueId[0], oriQueueId);

                g_pqInfo[expQueueId[0]].tcontId = OMCI_DRV_INVALID_TCONT_ID;
                g_pqInfo[expQueueId[0]].tcQueueId = OMCI_DRV_INVALID_QUEUE_ID;
                g_pqInfo[expQueueId[0]].policy = PQ_POLICY_STRICT_PRIORITY;
                g_pqInfo[expQueueId[0]].pw.priority = 0;

                // release queue relationships
                g_pqInfo[expQueueId[0]].dpMarking = PQ_DROP_COLOUR_NO_MARKING;
                g_pqInfo[expQueueId[0]].dpQueueId = 0;
            }

            // recreate pq & gem flows
            if (gemCount != 0 &&
                    GOS_OK != omci_ReCreateUsGemFlowByTcontId(g_pqInfo[oriQueueId].tcontId))
                return GOS_FAIL;

            // recreate conn
            if (gemCount != 0 &&
                    GOS_OK != omci_ReSetUsConnDpByPqEntityId(OMCI_MIB_US_TM_ME_ID_BASE + oriQueueId))
                return GOS_FAIL;
        }

        // sync to vgf
        omci_DelUsVeipGemFlow(gemPortId);

        return GOS_OK;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setUniPortRate(omci_port_rate_t *pRatePerPort)
{
    INT32   ret;

    if (!pRatePerPort)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_UNI_PORT_RATE, pRatePerPort, sizeof(omci_port_rate_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPauseControl(omci_port_pause_ctrl_t *pPortPauseCtrl)
{
    INT32   ret;

    if (!pPortPauseCtrl)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PAUSE_CONTROL_SET, pPortPauseCtrl, sizeof(omci_port_pause_ctrl_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getPauseControl(omci_port_pause_ctrl_t *pPortPauseCtrl)
{
    INT32   ret;

    if (!pPortPauseCtrl)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_PORT_PAUSE_CONTROL_GET, pPortPauseCtrl, sizeof(omci_port_pause_ctrl_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setDot1RateLimiter(omci_dot1_rate_t *pDot1Rate)
{
    INT32                   ret;
    omci_dot1_rate_meter_t  dot1RateMeter;
    UINT32                  meterId;

    if (!pDot1Rate)
        return GOS_ERR_PARAM;

    // search for existence
    for (meterId = 0; meterId < gInfo.devCapabilities.meterNum; meterId++)
    {
        if (gInfo.devCapabilities.rsvMeterId == meterId)
            continue;
        if (g_meterInfo[meterId].inUse == FALSE)
            continue;

        if (g_meterInfo[meterId].dot1PortMask == pDot1Rate->portMask &&
                g_meterInfo[meterId].dot1Type == pDot1Rate->type)
            break;
    }
    // create it if meter is not yet exist
    if (meterId == gInfo.devCapabilities.meterNum)
    {
        for (meterId = 0; meterId < gInfo.devCapabilities.meterNum; meterId++)
        {
            if (gInfo.devCapabilities.rsvMeterId == meterId)
                continue;
            if (g_meterInfo[meterId].inUse == FALSE)
                break;;
        }
        if (meterId == gInfo.devCapabilities.meterNum)
        {
            OMCI_LOG(OMCI_LOG_LEVEL_ERR, "no meter available for dot1 rate limiter");

            return GOS_FAIL;
        }
    }

    dot1RateMeter.meterId = meterId;
    memcpy(&dot1RateMeter.dot1Rate, pDot1Rate, sizeof(omci_dot1_rate_t));

    ret = omci_ioctl_send(OMCI_IOCTL_DOT1_RATE_LIMITER_SET, &dot1RateMeter, sizeof(omci_dot1_rate_meter_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    g_meterInfo[meterId].inUse = TRUE;
    g_meterInfo[meterId].dot1PortMask = pDot1Rate->portMask;
    g_meterInfo[meterId].dot1Type = pDot1Rate->type;

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_delDot1RateLimiter(omci_dot1_rate_t *pDot1Rate)
{
    INT32                   ret;
    omci_dot1_rate_meter_t  dot1RateMeter;
    UINT32                  meterId;

    if (!pDot1Rate)
        return GOS_ERR_PARAM;

    // search for existence
    for (meterId = 0; meterId < gInfo.devCapabilities.meterNum; meterId++)
    {
        if (gInfo.devCapabilities.rsvMeterId == meterId)
            continue;
        if (g_meterInfo[meterId].inUse == FALSE)
            continue;

        if (g_meterInfo[meterId].dot1PortMask == pDot1Rate->portMask &&
                g_meterInfo[meterId].dot1Type == pDot1Rate->type)
            break;
    }
    // return fail if meter is not exist
    if (meterId == gInfo.devCapabilities.meterNum)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "meter is not found for dot1 rate limiter");

        return GOS_ERR_NOT_FOUND;
    }

    dot1RateMeter.meterId = meterId;
    memcpy(&dot1RateMeter.dot1Rate, pDot1Rate, sizeof(omci_dot1_rate_t));

    ret = omci_ioctl_send(OMCI_IOCTL_DOT1_RATE_LIMITER_DEL, &dot1RateMeter, sizeof(omci_dot1_rate_meter_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    g_meterInfo[meterId].inUse = FALSE;

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_getBridgeTableByPort(omci_bridge_tbl_per_port_t *p)
{
    INT32   ret;

    if (!p)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_BG_TBL_PER_PORT_GET, p, sizeof(omci_bridge_tbl_per_port_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setAgeingTime(UINT32 ageTime)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_MAC_AGEING_TIME_SET, &ageTime, sizeof(UINT32)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_sendOmciEvent(omci_event_msg_t *p)
{
    INT32   ret;

    if (!p)
        return GOS_ERR_PARAM;

    ret = omci_ioctl_send(OMCI_IOCTL_SEND_OMCI_EVENT, p, sizeof(omci_event_msg_t));
    if (OMCI_ERR_OK != ret)
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return GOS_FAIL;
    }

    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPortBridging(UINT32 enable)
{
    CHAR cmd[CMD_LEN];

    /*Set kenel Lan interfaces bridging or not*/
    if(enable)
    {
        memset(cmd, 0, CMD_LEN);
        snprintf(cmd, CMD_LEN, "ebtables -D FORWARD -i eth0+ -o eth0+ -j DROP");
        system(cmd);
    }
    else
    {
        memset(cmd, 0, CMD_LEN);
        snprintf(cmd, CMD_LEN, "ebtables -I FORWARD -i eth0+ -o eth0+ -j DROP");
        system(cmd);
    }

    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_PORT_BRIDGING_SET, &enable, sizeof(UINT32)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setFloodingPortMask(omci_flood_port_info *pFloodInfo)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_FLOOD_PORT_MASK_SET, pFloodInfo, sizeof(omci_flood_port_info)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setWanQueueNum(int num)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_WAN_QUEUE_NUM_SET, &num, sizeof(int)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setPortRemp(int *p)
{
    if (OMCI_ERR_OK != omci_ioctl_send(OMCI_IOCTL_PORT_REMAP_SET, p, (sizeof(int) * OMCI_PORT_REMAP_MAX_INDEX)))
    {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed", __FUNCTION__);

        return GOS_FAIL;
    }
    return GOS_OK;
}

GOS_ERROR_CODE
omci_wrapper_setTodInfo ( omci_tod_info_t* pTodStatus )
{
    INT32   ret;

    ret = omci_ioctl_send(OMCI_IOCTL_TOD_INFO_SET, pTodStatus, sizeof(omci_tod_info_t));
    if (OMCI_ERR_OK != ret) {
        OMCI_LOG(OMCI_LOG_LEVEL_ERR, "%s ioctl failed %d", __FUNCTION__, ret);

        return (GOS_FAIL);
    }

    return (GOS_OK);
}


