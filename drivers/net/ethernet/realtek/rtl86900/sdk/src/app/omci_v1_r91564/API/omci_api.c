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
 * Purpose : Definition of OMCI message queue APIs
 *
 * Feature : The file includes the following modules and sub-modules
 *           (1) OMCI message queue APIs
 */


#include "gos_linux.h"
#include "omci_msgq.h"
#include "omci_task.h"
#include "omci_api.h"


#define CMD_KEY 0x6868   /*TBD*/
GOS_ERROR_CODE omci_SendCmdAndGet(PON_OMCI_CMD_T* pData)
{
    GOS_ERROR_CODE    ret;
    CHAR              sendBuf[OMCI_MAX_BYTE_PER_MSG];
    CHAR              recvBuff[OMCI_MAX_BYTE_PER_MSG];
    OMCI_MSG_HDR_T*   pSendMsg = NULL, *pRecvMsg = NULL;
    unsigned int mqid;
    UINT32 dataLen = 0;

    if(pData == NULL)
        return GOS_ERR_INVALID_INPUT;

    GOS_ASSERT((dataLen + sizeof(OMCI_MSG_HDR_T)) <= sizeof(sendBuf));
    memset(sendBuf, 0, sizeof(sendBuf));

    /*MsgQ for receive response*/
    mqid = omci_CreateMsgQ(CMD_KEY);

    /*Send Message*/
    dataLen = sizeof(PON_OMCI_CMD_T);
    pSendMsg = omci_CreateMsg((OMCI_MSG_HDR_T*)sendBuf, OMCI_CMD_MSG, OMCI_MSG_PRI_NORMAL, CMD_KEY, dataLen + sizeof(OMCI_MSG_HDR_T));
    GOS_ASSERT(pSendMsg != NULL);

    if(0 != dataLen)
    {
        memcpy(((CHAR*)pSendMsg) + sizeof(OMCI_MSG_HDR_T), pData, dataLen);
    }
    ret = OMCI_SendToMsgQ(OMCI_APPL, pSendMsg, pSendMsg->len);

    /*Receive response*/
    pRecvMsg = (OMCI_MSG_HDR_T*)recvBuff;
    OMCI_RecvFromMsgQ(mqid, pRecvMsg, OMCI_MAX_BYTE_PER_MSG - 8, -1 * OMCI_MSG_PRI_NORMAL);

    memcpy(pData, ((CHAR*)pRecvMsg) + sizeof(OMCI_MSG_HDR_T), dataLen);

	omci_DeleteMsgQ(mqid);

    return ret;
}

