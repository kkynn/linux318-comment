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
#include "gos_type.h"
#include "omci_task.h"
#include "omci_util.h"
#include "omci_msgq.h"


OMCI_MSG_HDR_T* omci_CreateMsg(OMCI_MSG_HDR_T* sendBuf, OMCI_MSG_TYPE type, OMCI_MSG_PRI pri, UINT32 srcMsgQKey, UINT32 len)
{
    //OMCI_TASK_INFO_T* taskInfo = NULL;

    GOS_ASSERT(sendBuf != NULL);


    sendBuf->len       = len;
    sendBuf->priority  = pri;
    sendBuf->type      = type;
    sendBuf->srcApplId = 0; //pAppInfo->applId;
    sendBuf->srcMsgQKey = srcMsgQKey;

    /*
    if (taskInfo)
    {
        sendBuf->srcMsgQKey = taskInfo->msgQKey;
    }
    */

    return sendBuf;
}

GOS_ERROR_CODE OMCI_SendToMsgQ(UINT32 msgKey, OMCI_MSG_HDR_T* pMsg, UINT32 len)
{
    OMCI_MSG_Q_ID msgQId;
    UINT32        tryCounter = 0;

    if (NULL == pMsg)
    {
        return GOS_ERR_PARAM;
    }
    if(len > OMCI_MAX_BYTE_PER_MSG)
    {
        return GOS_ERR_PARAM;
    }

    msgQId = msgget(msgKey, S_IRUSR|S_IWUSR);

    if (OMCI_MSG_ID_INVALID == msgQId)
    {
        return GOS_ERR_NOT_FOUND;
    }

    while (1)
    {
        if (0 != msgsnd(msgQId, pMsg, len - sizeof(long), IPC_NOWAIT))
        {
            if ((EAGAIN == errno) && (tryCounter < 3))
            {
                tryCounter++;
                OMCI_TaskDelay(100);
                continue;
            }
            else
            {
                 return GOS_ERR_SYS_CALL_FAIL;
            }
        }
        else
        {
            return GOS_OK;
        }
    }

    return GOS_OK;
}

GOS_ERROR_CODE OMCI_SendMsg(UINT32 msgQKey, OMCI_MSG_TYPE type, OMCI_MSG_PRI pri, void* pData, UINT32 dataLen)
{
    GOS_ERROR_CODE    ret;
    CHAR              sendBuf[OMCI_MAX_BYTE_PER_MSG];
    OMCI_MSG_HDR_T*   pHdr = NULL;

    GOS_ASSERT((dataLen + sizeof(OMCI_MSG_HDR_T)) <= sizeof(sendBuf));
    memset(sendBuf, 0, sizeof(sendBuf));

    pHdr = omci_CreateMsg((OMCI_MSG_HDR_T*)sendBuf, type, pri, 0, dataLen + sizeof(OMCI_MSG_HDR_T));

    GOS_ASSERT(pHdr != NULL);

    if ((NULL !=  pData) && (0 != dataLen))
    {
        memcpy(((CHAR*)pHdr) + sizeof(OMCI_MSG_HDR_T), pData, dataLen);
    }

    ret = OMCI_SendToMsgQ(msgQKey, pHdr, pHdr->len);

    return ret;
}

GOS_ERROR_CODE OMCI_RecvFromMsgQ(OMCI_MSG_Q_ID msgId, void* pMsgHdr, UINT32 uiMaxBytes, INT32 msgType)
{
    int size;


    if (OMCI_MSG_ID_INVALID == msgId)
    {
        return GOS_ERR_PARAM;
    }

    if (NULL == pMsgHdr)
    {
        return GOS_ERR_PARAM;
    }

    if (uiMaxBytes > OMCI_MAX_BYTE_PER_MSG)
    {
        return GOS_ERR_PARAM;
    }

    if (ERROR == (size = msgrcv(msgId, pMsgHdr, uiMaxBytes, msgType, 0)))
    {
        return GOS_ERR_SYS_CALL_FAIL;
    }

    if (0 == size)
    {
        return GOS_FAIL;
    }

    return GOS_OK;
}

OMCI_MSG_Q_ID omci_CreateMsgQ(UINT32 msgKey)
{
    OMCI_MSG_Q_ID msgId;


    if ((msgId = msgget(msgKey, S_IRUSR|S_IWUSR|IPC_CREAT)) == -1)
    {
        return OMCI_MSG_ID_INVALID;
    }

    return msgId;
}

GOS_ERROR_CODE omci_DeleteMsgQ(OMCI_MSG_Q_ID msgId)
{
    if (OMCI_MSG_ID_INVALID == msgId)
    {
        return GOS_ERR_PARAM;
    }

    if( 0 != msgctl(msgId, IPC_RMID, NULL))
    {
        return GOS_ERR_SYS_CALL_FAIL;
    }

    return GOS_OK;
}
